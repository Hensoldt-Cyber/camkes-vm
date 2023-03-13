/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <camkes.h>
#include <sel4vm/guest_vm.h>
#include <sel4vm/guest_memory.h>
#include <sel4vm/guest_memory_helpers.h>
#include <sel4vm/guest_ram.h>
#include <sel4vmmplatsupport/guest_memory_util.h>
#include <vmlinux.h>

#include "../fdt_manipulation.h"

extern char gen_dtb_buf[];

void WEAK init_ram_module(vm_t *vm, void *cookie)
{
    int err = vm_ram_register_at(vm, ram_base, ram_size, vm->mem.map_one_to_one);
    if (err) {
        ZF_LOGE("Failed to register RAM (%d)\n", err);
        return;
    }

    if (generate_dtb) {
        err = fdt_generate_memory_node(gen_dtb_buf, ram_base, ram_size);
        if (err) {
            ZF_LOGE("Couldn't generate memory_node (%d)\n", err);
            return;
        }
    }
}

DEFINE_MODULE(init_ram, NULL, init_ram_module)
