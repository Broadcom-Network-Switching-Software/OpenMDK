/*
 * 
 * DO NOT EDIT THIS FILE!
 * This file is auto-generated.
 * Edits to this file will be lost when it is regenerated.
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

/* Remapping not allowed in dispatch */
#define BMD_CONFIG_API_PREFIX_DISABLE
#include <bmd/bmd.h>
#include <bmdi/bmd_devtype.h>
/* BMD Devlist macros for generating the dispatch table for this function */

/* Externs for the required functions */
#define BMD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_fl,_cn,_pf,_pd,_r0,_r1) \
extern int _bd##_bmd_test_interrupt_assert(int unit);
#include <bmdi/bmd_devlist.h>

/* Dispatch table for this function, which uses the externs above */
#define BMD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_fl,_cn,_pf,_pd,_r0,_r1) \
_bd##_bmd_test_interrupt_assert,
static int (*__dispatch_table[])(int unit) =
{
#include <bmdi/bmd_devlist.h>
0
};


int 
bmd_test_interrupt_assert(
    int unit)
{
    bmd_dev_type_t dev_type;

    if(!CDK_DEV_EXISTS(unit)) {
        /* Unit has not been created */
        return CDK_E_UNIT;
    }

    if((dev_type = BMD_DEV_TYPE(unit)) == bmdDevTypeNone) {
        /* The BMD does not support this device */
        return CDK_E_CONFIG;
    }

    if(__dispatch_table[dev_type] == 0) {
        /* This vector not supported for this device */
        return CDK_E_UNAVAIL;
    }

    return __dispatch_table[dev_type](unit);
}

