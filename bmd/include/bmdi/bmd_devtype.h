/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef __BMD_DEVTYPE_H__
#define __BMD_DEVTYPE_H__

#include <bmd_config.h>
#include <cdk/cdk_device.h>

/*
 * BMD_DEV_TYPE Enumeration. 
 */
#define BMD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_fl,_cn,_pf,_pd,_r0,_r1) \
bmdDevType##_bd,

typedef enum bmd_dev_type_e {
#include <bmdi/bmd_devlist.h>
    bmdDevTypeCount,
    bmdDevTypeNone = -1
} bmd_dev_type_t; 

extern bmd_dev_type_t
bmd_device_type(int unit); 

#define BMD_DEV_TYPE(unit) bmd_device_type(unit)

#endif /* __BMD_DEVTYPE_H__ */
