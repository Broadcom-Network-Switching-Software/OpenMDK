/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <cdk/cdk_device.h>
#include <cdk/cdk_string.h>
#include <cdk/cdk_error.h>
#include <cdk/cdk_assert.h>
#include <cdk/cdk_debug.h>

#define CDK_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_fl,_cn,_pf,_pd,_r0,_r1) \
    { _vn, _dv, _pi },
#define CDK_DEVLIST_INCLUDE_ALL
static struct {
    uint16_t  vendor_id;
    uint16_t  device_id;
    uint32_t model_info;
} dev_table[] = {
#include <cdk/cdk_devlist.h>
    { 0, 0 } /* end-of-list */
};

/*
 * Find probe info
 */
int 
cdk_dev_probe_info_get(cdk_dev_id_t *id, cdk_dev_probe_info_t *pi)
{
    int idx, match_idx;

    CDK_MEMSET(pi, 0, sizeof(*pi));
    match_idx = -1;
    idx = 0;
    while (dev_table[idx].vendor_id) {
        if (id->vendor_id == dev_table[idx].vendor_id &&
            id->device_id == dev_table[idx].device_id) {
            /* Found a match */
            match_idx = idx;
            break;
        }
        idx++;
    }
    if (match_idx >= 0) {
        pi->model_info = dev_table[match_idx].model_info;
        return CDK_E_NONE; 
    }

    return CDK_E_NOT_FOUND; 
}
