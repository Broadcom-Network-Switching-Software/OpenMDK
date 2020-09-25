/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53540_A0 == 1

#include <bmd/bmd.h>

#include <cdk/cdk_device.h>
#include <cdk/cdk_debug.h>
#include <cdk/cdk_error.h>
#include <cdk/chip/bcm53540_a0_defs.h>

#include "bcm53540_a0_bmd.h"
#include "bcm53540_a0_internal.h"

int 
bcm53540_a0_bmd_vlan_port_get(int unit, int vlan, int *plist, int *utlist)
{
    int ioerr = 0;
    int lport;
    VLAN_TABm_t vlan_tab;
    EGR_VLANm_t egr_vlan;
    uint32_t pbm;
    uint32_t utpbm;
    uint32_t mask;
    int port, pdx, udx;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_VLAN(unit, vlan);

    ioerr += READ_VLAN_TABm(unit, vlan, &vlan_tab);
    if (VLAN_TABm_VALIDf_GET(vlan_tab) == 0) {
        return ioerr ? CDK_E_IO : CDK_E_NOT_FOUND;
    }

    ioerr += READ_EGR_VLANm(unit, vlan, &egr_vlan);
    pbm = VLAN_TABm_PORT_BITMAP_LOf_GET(vlan_tab);
    utpbm = EGR_VLANm_UT_BITMAP_LOf_GET(egr_vlan);
    mask = 1;
    pdx = 0;
    udx = 0;

    /* Loop over lport bitmask */
    for (lport = 0; lport < 32 && lport < BMD_CONFIG_MAX_PORTS; lport++) {
        port = L2P(unit, lport);
        if (mask & pbm) {
            plist[pdx++] = port;
        }
        if (mask & utpbm) {
            utlist[udx++] = port;
        }
        mask <<= 1;
    }

    /* Terminate lists */
    plist[pdx] = -1;
    utlist[udx] = -1;

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

#endif /* CDK_CONFIG_INCLUDE_BCM53540_A0 */
