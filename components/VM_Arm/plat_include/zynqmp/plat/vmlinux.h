/*
 * Copyright 2019, DornerWorks
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#define GIC_NODE_PATH "/amba_apu@0/interrupt-controller@f9010000"

static const int linux_pt_irqs[] = {};

static const int free_plat_interrupts[] =  { -1 };

static const char *plat_keep_devices[] = {
    "/timer",
    "/psci",
    "/pmu",
    "/amba/ethernet@ff0b0000",
    "/amba/ethernet@ff0c0000",
    "/amba/ethernet@ff0d0000",
    "/amba/serial@ff010000"
};
static const char *plat_keep_device_and_disable[] = {};
static const char *plat_keep_device_and_subtree[] = {
    GIC_NODE_PATH,
    "/amba/ethernet@ff0e0000",
    "/amba/pcie@fd0e0000",
};
static const char *plat_keep_device_and_subtree_and_disable[] = {};
static const char *plat_linux_bootcmdline = "";
static const char *plat_linux_stdout = "";
