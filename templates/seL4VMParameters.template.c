/*
 * Copyright 2023, DornerWorks
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <camkes.h>
#include <arm_vm/gen_config.h>

/*- set vm_address_config = configuration[me.name].get('vm_address_config') -*/
/*- set vm_image_config = configuration[me.name].get('vm_image_config') -*/
/*- set linux_address_config = configuration[me.name].get('linux_address_config') -*/
/*- set linux_image_config = configuration[me.name].get('linux_image_config') -*/

const vm_config_t vm_config = {

/*- if vm_address_config -*/

    .ram_base = /*? vm_address_config.get('ram_base') ?*/,
    .ram_paddr_base = /*? vm_address_config.get('ram_paddr_base') ?*/,
    .ram_size = /*? vm_address_config.get('ram_size') ?*/,
    .dtb_addr = /*? vm_address_config.get('dtb_addr') ?*/,
    .initrd_addr = /*? vm_address_config.get('initrd_addr') ?*/,

/*- if vm_address_config.get('kernel_entry_addr') != '-1' -*/
    .entry_addr = /*? vm_address_config.get('kernel_entry_addr') ?*/,
/*- else -*/

#warning Entry address has been calculated. Please use vm_address_config.kernel_entry_addr

#ifdef CONFIG_ARCH_AARCH64
    .entry_addr = /*? vm_address_config.get('ram_base') ?*/ + 0x80000,
#else
    .entry_addr = /*? vm_address_config.get('ram_base') ?*/ + 0x8000,
#endif

/*- endif -*/

/*- else -*/

#warning You are using the deprecated linux_address_config structure. Please use the vm_address_config structure instead

    .ram_base = /*? linux_address_config.get('linux_ram_base') ?*/,
    .ram_paddr_base = /*? linux_address_config.get('linux_ram_paddr_base') ?*/,
    .ram_size = /*? linux_address_config.get('linux_ram_size') ?*/,
    .ram_offset = /*? linux_address_config.get('linux_ram_offset') ?*/,
    .dtb_addr = /*? linux_address_config.get('dtb_addr') ?*/,
    .initrd_max_size = /*? linux_address_config.get('initrd_max_size') ?*/,
    .initrd_addr = /*? linux_address_config.get('initrd_addr') ?*/,

#ifdef CONFIG_ARCH_AARCH64
    .entry_addr = /*? linux_address_config.get('linux_ram_base') ?*/ + 0x80000,
#else
    .entry_addr = /*? linux_address_config.get('linux_ram_base') ?*/ + 0x8000,
#endif

/*- endif -*/

/*- if vm_image_config -*/

    .kernel_name = "/*? vm_image_config.get('kernel_name') ?*/",
    .dtb_name = "/*? vm_image_config.get('dtb_name', "") ?*/",
    .initrd_name = "/*? vm_image_config.get('initrd_name', "") ?*/",

    .kernel_bootcmdline = "/*? vm_image_config.get('kernel_bootcmdline', "") ?*/",
    .kernel_stdout = "/*? vm_image_config.get('kernel_stdout', "") ?*/",
    .dtb_base_name = "/*? vm_image_config.get('dtb_base_name', "") ?*/",

    .provide_initrd = /*? vm_image_config.get('provide_initrd') ?*/,
    .generate_dtb = /*? vm_image_config.get('generate_dtb') ?*/,
    .provide_dtb = /*? vm_image_config.get('provide_dtb') ?*/,
    .map_one_to_one = /*? vm_image_config.get('map_one_to_one') ?*/,
    .clean_cache = /*? vm_image_config.get('clean_cache') ?*/,

/*- else -*/

#warning "You are using the deprecated linux_image_config structure. The provide_initrd, generate_dtb, provide_dtb, \
    map_one_to_one, and clean_cache flags are set to replicate previous behavior, which may cause your configuration to break. \
    Please use the vm_image_config structure instead."

    .kernel_name = "/*? linux_image_config.get('linux_name') ?*/",
    .dtb_name = "/*? linux_image_config.get('dtb_name') ?*/",
    .initrd_name = "/*? linux_image_config.get('initrd_name') ?*/",

    .kernel_bootcmdline = "/*? linux_image_config.get('linux_bootcmdline') ?*/",
    .kernel_stdout = "/*? linux_image_config.get('linux_stdout') ?*/",
    .dtb_base_name = "/*? linux_image_config.get('dtb_base_name') ?*/",

#ifdef CONFIG_VM_INITRD_FILE
#warning VmInitRdFile is a deprecated setting. Please remove and use vm_image_config.provide_initrd
    .provide_initrd = 1,
#else
    .provide_initrd = 0,
#endif

#ifdef CONFIG_VM_DTB_FILE
#warning VmDtbFile is a deprecated setting. Please remove and use vm_image_config.provide_dtb
    .provide_dtb = 1,
    .generate_dtb = 0,
#else
    .provide_dtb = 0,
    .generate_dtb = 1,
#endif

#if defined(CONFIG_PLAT_EXYNOS5) || defined(CONFIG_PLAT_QEMU_ARM_VIRT) || defined(CONFIG_PLAT_TX2)
    .map_one_to_one = 1,
#else
    .map_one_to_one = 0,
#endif

#if defined(CONFIG_PLAT_TX1) || defined(CONFIG_PLAT_TX2)
    .clean_cache = 1,
#else
    .clean_cache = 0,
#endif

/*- endif -*/

};
