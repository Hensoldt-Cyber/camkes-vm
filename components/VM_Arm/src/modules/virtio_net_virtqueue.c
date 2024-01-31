/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <autoconf.h>
#include <arm_vm/gen_config.h>
#include <vmlinux.h>
#include <netinet/ether.h>

#include <camkes.h>

#include <virtqueue.h>
#include <vswitch.h>
#include <camkes/virtqueue.h>
#include <virtioarm/virtio_net.h>

static virtio_net_t *virtio_net = NULL;
static vswitch_t virtio_vswitch;

extern vmm_pci_space_t *pci;
extern vmm_io_port_list_t *io_ports;

void self_mac(uint8_t *mac)
{
    struct ether_addr res;
    struct ether_addr *resp;
    resp = ether_aton_r(vswitch_mac_address, &res);
    if (resp == NULL) {
        ZF_LOGF("Failed to get MAC address");
    }
    memcpy(mac, res.ether_addr_octet, ETH_ALEN);
}

static int tx_virtqueue_forward(char *eth_buffer, size_t length, virtio_net_t *virtio_net)
{
    for (int i = 0; i < virtio_vswitch.n_connected; i++) {
        vswitch_node_t *destnode = vswitch_get_destnode_by_index(&virtio_vswitch, i);
        if (destnode == NULL) {
            /* This could happen in the broadcast case if there are holes in
             * the array, though that would still be odd.
             */
            continue;
        }
        virtqueue_driver_t *vq = destnode->virtqueues.send_queue;
        if (camkes_virtqueue_driver_scatter_send_buffer(vq, (void *)eth_buffer, length) < 0) {
            ZF_LOGE("Unknown error while enqueuing available buffer for dest "
                    PR_MAC802_ADDR ".",
                    PR_MAC802_ADDR_ARGS(&(destnode->addr)));
            continue;
        }
        vq->notify();
    }
    return 0;
}

static void virtio_net_notify_free_send(vswitch_node_t *node)
{
    virtqueue_driver_t *vq = node->virtqueues.send_queue;
    void *buf = NULL;
    unsigned int buf_size = 0;
    uint32_t wr_len = 0;
    vq_flags_t flag;
    virtqueue_ring_object_t handle;
    while (virtqueue_get_used_buf(vq, &handle, &wr_len)) {
        while (camkes_virtqueue_driver_gather_buffer(vq, &handle, &buf, &buf_size, &flag) >= 0) {
            /* Clean up and free the buffer we allocated */
            camkes_virtqueue_buffer_free(vq, buf);
        }
    }
}

static void virtio_net_notify_recv(vswitch_node_t *node)
{
    virtqueue_device_t *vq = node->virtqueues.recv_queue;

    /* Since this is not thread-safe, it's fine to have just one temp buffer
     * here that receives the data from the queue and it then passed to the
     * sender. We expect to see only standard ethernet frames and discard
     * anything else, e.g. jumbo frames.
     */
    static char buf[MAX_MTU];

    for (;;) {
        int err;
        virtqueue_ring_object_t handle;
        if (!virtqueue_get_available_buf(vq, &handle)) {
            return;
        }

        /* We have a handle, so there is one or more buffer attached that are
         * supposed to contain an ethernet frame.
         */
        size_t len = virtqueue_scattered_available_size(vq, &handle);
        /* Currently we support normal ethernet frames only, but no jumbo
         * frames. Getting a length of zero can happen when no data is left
         * in the queue, because we already processed it earlier. The driver
         * is not supposed to put zero-length packets in the queue.
         */
        if (len > sizeof(buf)) {
            ZF_LOGW("Dropping unsupported large frame (%zu > %zu)", len, sizeof(buf));
            /* Discard frame, len is set to 0 because we don't have any payload
             * data for the driver, it should just release the buffers.
             */
            if (!virtqueue_add_used_buf(vq, &handle, 0)) {
                /* This is not supposed to happen, and there is nothing we can
                 * do in this case.
                 */
                ZF_LOGW("Could not release queue buffer");
            }
            continue;
        }

        /* It's save to call this even when len is zero */
        if (camkes_virtqueue_device_gather_copy_buffer(vq, &handle, buf, len)) {
            ZF_LOGW("Dropping frame for " PR_MAC802_ADDR ": Can't gather vq buffer.",
                    PR_MAC802_ADDR_ARGS(&(node->addr)));
            continue;
        }

        /* It's save to call this even when len is zero */
        err = virtio_net_rx(buf, len, virtio_net);
        if (err) {
            ZF_LOGE("Unable to forward received buffer to the guest, (%d)", err);
        }
    }

    /* We always raise a notification, even is we discarded frames or received
     * zero-length frames.
     */
    vq->notify();
}

static int virtio_net_notify(vm_t *vmm, void *cookie)
{
    for (int i = 0; i < VSWITCH_NUM_NODES; i++) {
        vswitch_node_t *node = &virtio_vswitch.nodes[i];
        virtqueue_device_t *vq_rx = node->virtqueues.recv_queue;
        if (vq_rx && VQ_DEV_POLL(vq_rx)) {
            virtio_net_notify_recv(node);
        }
        virtqueue_driver_t *vq_tx = node->virtqueues.send_queue;
        if (vq_tx && VQ_DRV_POLL(vq_tx)) {
            virtio_net_notify_free_send(node);
        }
    }
    return 0;
}

void make_virtqueue_virtio_net(vm_t *vm, void *cookie)
{
    int err;
    virtio_net_callbacks_t callbacks = {
        .tx_callback = tx_virtqueue_forward,
        .irq_callback = NULL,
        .get_mac_addr_callback = self_mac,
    };
    virtio_net = virtio_net_init(vm, &callbacks, pci, io_ports);

    err = vswitch_init(&virtio_vswitch);
    if (err) {
        ZF_LOGF("Unable to initialise vswitch library");
    }

    for (int i = 0; i < ARRAY_SIZE(vswitch_layout); i++) {
        struct vswitch_mapping mac_mapping = vswitch_layout[i];
        struct ether_addr guest_macaddr;
        struct ether_addr *res = ether_aton_r(mac_mapping.mac_addr, &guest_macaddr);
        seL4_CPtr recv_notif;
        seL4_CPtr recv_badge;
        virtqueue_device_t *vq_recv = malloc(sizeof(*vq_recv));
        if (!vq_recv) {
            ZF_LOGF("Unable to initialise recv virtqueue for MAC address: %s", mac_mapping.mac_addr);
        }
        virtqueue_driver_t *vq_send = malloc(sizeof(*vq_send));
        if (!vq_send) {
            ZF_LOGF("Unable to initialise send virtqueue for MAC address: %s", mac_mapping.mac_addr);
        }

        /* Initialise recv virtqueue */
        err = camkes_virtqueue_device_init_with_recv(vq_recv, mac_mapping.recv_id, &recv_notif, &recv_badge);
        ZF_LOGF_IF(err, "Unable to initialise recv virtqueue");
        err = register_async_event_handler(recv_badge, virtio_net_notify, NULL);
        ZF_LOGF_IF(err, "Failed to register_async_event_handler for recv channel on MAC address: %s", mac_mapping.mac_addr);

        /* Initialise send virtqueue */
        err = camkes_virtqueue_driver_init_with_recv(vq_send, mac_mapping.send_id, &recv_notif, &recv_badge);
        ZF_LOGF_IF(err, "Unable to initialise send virtqueue");
        err = register_async_event_handler(recv_badge, virtio_net_notify, NULL);
        ZF_LOGF_IF(err, "Failed to register_async_event_handler for send channel on MAC address: %s", mac_mapping.mac_addr);

        err = vswitch_connect(&virtio_vswitch, &guest_macaddr, vq_send, vq_recv);
        if (err) {
            ZF_LOGF("Unable to initialise vswitch for MAC address: %s", mac_mapping.mac_addr);
        }
    }
}

DEFINE_MODULE(virtio_net, NULL, make_virtqueue_virtio_net)
