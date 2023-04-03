/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#define IRQ_SPI_OFFSET 32
#define GIC_NODE_PATH  "/soc/interrupt-controller@c4301000"

static const int linux_pt_irqs[] = {};

static const int free_plat_interrupts[] =  { 50 + IRQ_SPI_OFFSET };
static const char *plat_keep_devices[] = {
    "/timer",
    "/xtal-clk",
    "/arm-pmu",
    "/psci",
    "/firmware",
    "/efuse",
    "/scpi",
};
static const char *plat_keep_device_and_disable[] = {};
static const char *plat_keep_device_and_subtree[] = {
    GIC_NODE_PATH,
    "soc/bus@c1100000/interrupt-controller@9880",
    "soc/bus@c1100000/reset-controller@4404"

};
static const char *plat_keep_device_and_subtree_and_disable[] = {};
