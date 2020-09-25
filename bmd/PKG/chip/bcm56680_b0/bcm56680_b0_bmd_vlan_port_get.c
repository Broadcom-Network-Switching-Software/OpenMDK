#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56680_B0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#include <cdk/chip/bcm56680_b0_defs.h>

#include "bcm56680_b0_bmd.h"

int
bcm56680_b0_bmd_vlan_port_get(int unit, int vlan, int *plist, int *utlist)
{
    int ioerr = 0;
    VLAN_TABm_t vlan_tab;
    EGR_VLANm_t egr_vlan;
    uint32_t pbmp;
    uint32_t utbmp;
    uint32_t mask;
    int port, pdx, udx;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_VLAN(unit, vlan);

    ioerr += READ_VLAN_TABm(unit, vlan, &vlan_tab);
    if (VLAN_TABm_VALIDf_GET(vlan_tab) == 0) {
        return ioerr ? CDK_E_IO : CDK_E_NOT_FOUND;
    }

    ioerr += READ_EGR_VLANm(unit, vlan, &egr_vlan);
    pbmp = VLAN_TABm_PORT_BITMAP_LOf_GET(vlan_tab);
    utbmp = EGR_VLANm_UT_BITMAP_LOf_GET(egr_vlan);
    mask = 1;
    pdx = 0;
    udx = 0;
    /* Loop over lower port bitmasks */
    for (port = 0; port < 32 && port < BMD_CONFIG_MAX_PORTS; port++) {
        if (mask & pbmp) {
            plist[pdx++] = port;
        }
        if (mask & utbmp) {
            utlist[udx++] = port;
        }
        mask <<= 1;
    }
    pbmp = VLAN_TABm_PORT_BITMAP_HIf_GET(vlan_tab);
    utbmp = EGR_VLANm_UT_BITMAP_HIf_GET(egr_vlan);
    mask = 1;
    /* Loop over upper port bitmasks */
    for (port = 32; port < BMD_CONFIG_MAX_PORTS; port++) {
        if (mask & pbmp) {
            plist[pdx++] = port;
        }
        if (mask & utbmp) {
            utlist[udx++] = port;
        }
        mask <<= 1;
    }
    /* Terminate lists */
    plist[pdx] = -1;
    utlist[udx] = -1;

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

#endif /* CDK_CONFIG_INCLUDE_BCM56680_B0 */
