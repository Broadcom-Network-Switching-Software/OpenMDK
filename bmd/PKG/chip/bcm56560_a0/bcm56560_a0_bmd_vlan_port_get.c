/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56560_A0 == 1

#include <bmd/bmd.h>

#include <cdk/chip/bcm56560_a0_defs.h>

#include "bcm56560_a0_bmd.h"
#include "bcm56560_a0_internal.h"

int 
bcm56560_a0_bmd_vlan_port_get(int unit, int vlan, int *plist, int *utlist)
{
    int ioerr = 0;
    int lport;
    VLAN_TABm_t vlan_tab;
    EGR_VLANm_t egr_vlan;
    uint32_t pbm[PBM_LPORT_WORDS];
    uint32_t utpbm[PBM_LPORT_WORDS];
    int port, pdx, udx;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_VLAN(unit, vlan);

    ioerr += READ_VLAN_TABm(unit, vlan, &vlan_tab);
    if (VLAN_TABm_VALIDf_GET(vlan_tab) == 0) {
        return ioerr ? CDK_E_IO : CDK_E_NOT_FOUND;
    }

    ioerr += READ_EGR_VLANm(unit, vlan, &egr_vlan);
    VLAN_TABm_PORT_BITMAPf_GET(vlan_tab, pbm);
    EGR_VLANm_UT_BITMAPf_GET(egr_vlan, utpbm);
    pdx = 0;
    udx = 0;
    for (lport = 0; lport < BMD_CONFIG_MAX_PORTS; lport++) {
        if (lport >= NUM_LOGIC_PORTS) {
            break;
        }
        port = L2P(unit, lport);
        if (PBM_MEMBER(pbm, lport)) {
            plist[pdx++] = port;
        }
        if (PBM_MEMBER(utpbm, lport)) {
            utlist[udx++] = port;
        }
    }
    /* Terminate lists */
    plist[pdx] = -1;
    utlist[udx] = -1;

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}
#endif /* CDK_CONFIG_INCLUDE_BCM56560_A0 */

