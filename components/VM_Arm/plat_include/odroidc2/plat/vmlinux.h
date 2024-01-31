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
};
static const char *plat_keep_device_and_disable[] = {};
static const char *plat_keep_device_and_subtree[] = {
    // "/reserved-memory",
    "/firmware",
    "/efuse",
    "/scpi",
    // "/soc/sram@c8000000",
    // GIC_NODE_PATH,
    // "/soc/bus@c1100000/interrupt-controller@9880",
    // "/soc/bus@c1100000/reset-controller@4404",
    // "/soc/ethernet@c9410000",
    "/soc",
    //"/soc/bus@c8100000/sys-ctrl@0"

    //  reserved-memory
    //  cpus
    //  arm-pmu {
    //  psci {
    //  timer {
    //  xtal-clk {
    //  firmware {
    //  efuse {
    //  scpi {
    //  soc {
    //      sram@c8000000 {
    //      bus@c8100000 {
    //          sys-ctrl@0 {
    //              power-controller-vpu {
    //              clock-controller {
    //          cec@100 {
    //          ao-secure@140 {
    //          serial@4c0 {
    //          serial@4e0 {
    //          i2c@500 {
    //          pwm@550 {
    //          ir@580 {
    //          pinctrl@14 {
    //      periphs@c8834000 {
    //          rng {
    //          pinctrl@4b0 {
    //      bus@c8838000 {
    //          video-lut@48 {
    //      bus@c883c000 {
    //          system-controller@0 {
    //              clock-controller {
    //          mailbox@404 {
    //      ethernet@c9410000 {
    //          mdio {
    //      apb@d0000000 {
    //          mmc@70000 {
    //          mmc@72000 {
    //          mmc@74000 {
    //          gpu@c0000 {
    //      vpu@d0100000 {
    //      hdmi-tx@c883a000 {
    //      phy@c0000000 {
    //      phy@c0000020 {
    //      usb@c9000000 {
    //      usb@c9100000 {
    //  memory@0 {
    //  regulator-usb-pwrs {
    //  leds {
    //  regulator-tflash_vdd {
    //  gpio-regulator-tf_io {
    //  regulator-vcc1v8 {
    //  regulator-vcc3v3 {
    //  emmc-pwrseq {
    //  hdmi-connector {
};
static const char *plat_keep_device_and_subtree_and_disable[] = {};
