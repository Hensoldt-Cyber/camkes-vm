/*
 * Copyright 2023, DornerWorks
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

typedef struct {
    unsigned long ram_base;
    unsigned long ram_paddr_base;
    unsigned long ram_size;

    unsigned long dtb_addr;
    unsigned long initrd_addr;
    unsigned long entry_addr;

    /*- if not configuration[me.name].get('vm_address_config') -*/
    unsigned long initrd_max_size;
    unsigned long ram_offset;
    /*- endif -*/

    int provide_initrd;
    int generate_dtb;
    int provide_dtb;
    int map_one_to_one;
    int clean_cache;

    char *kernel_name;
    char *dtb_name;
    char *initrd_name;
    char *kernel_bootcmdline;
    char *kernel_stdout;
    char *dtb_base_name;
} vm_config_t;

extern const vm_config_t vm_config;
