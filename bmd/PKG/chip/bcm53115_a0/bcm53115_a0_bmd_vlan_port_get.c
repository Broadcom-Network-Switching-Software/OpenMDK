#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53115_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#include <cdk/chip/bcm53115_a0_defs.h>

#include "bcm53115_a0_bmd.h"

int
bcm53115_a0_bmd_vlan_port_get(int unit, int vlan, int *plist, int *utlist)
{
    int ioerr = 0;
    VLAN_1Qm_t vlan_tab;
    uint32_t pbmp;
    uint32_t utbmp;
    uint32_t mask;
    int port, pdx, udx;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_VLAN(unit, vlan);

    ioerr += READ_VLAN_1Qm(unit, vlan, &vlan_tab);
    if (VLAN_1Qm_MSPT_IDf_GET(vlan_tab) == 0) {
        return ioerr ? CDK_E_IO : CDK_E_NOT_FOUND;
    }

    pbmp = VLAN_1Qm_FORWARD_MAPf_GET(vlan_tab);
    utbmp = VLAN_1Qm_UNTAG_MAPf_GET(vlan_tab);
    mask = 1;
    pdx = 0;
    udx = 0;
    /* Loop over port bitmasks */
    for (port = 0; port < BMD_CONFIG_MAX_PORTS; port++) {
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

#endif /* CDK_CONFIG_INCLUDE_BCM53115_A0 */
