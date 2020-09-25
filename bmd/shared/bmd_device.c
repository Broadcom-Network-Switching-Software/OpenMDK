/*
 *
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>
#include <bmd/bmd_device.h>

#include <bmdi/bmd_devtype.h>

bmd_dev_t bmd_dev[BMD_CONFIG_MAX_UNITS];

/*
 * This function converts a cdkDevType to a bmdDevType. 
 * bmdDevTypes are always a subset of the available cdkDevTypes. 
 */
bmd_dev_type_t
bmd_device_type(int unit)
{
    /*
     * Define a BMD_DEVLIST_MACRO which generates a switch 
     * statement for the cdk dev types. 
     */
#define BMD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_fl,_cn,_pf,_pd,_r0,_r1) \
case cdkDevType_##_bd: return bmdDevType##_bd; 
    
    switch(CDK_DEV_TYPE(unit)) {
#include <bmdi/bmd_devlist.h>
    default: return bmdDevTypeNone; 
    }
}
