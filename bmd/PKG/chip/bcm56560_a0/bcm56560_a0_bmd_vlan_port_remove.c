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
bcm56560_a0_bmd_vlan_port_remove(int unit, int vlan, int port)
{
    int ioerr = 0;
    int lport;
    VLAN_TABm_t vlan_tab;
    EGR_VLANm_t egr_vlan;
    ING_VLAN_VFI_MEMBERSHIPm_t ing_vlan_member;
    EGR_VLAN_VFI_MEMBERSHIPm_t egr_vlan_member;
    uint32_t pbm[PBM_LPORT_WORDS];

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_VLAN(unit, vlan);
    BMD_CHECK_PORT(unit, port);

    lport = P2L(unit, port);

    ioerr += READ_VLAN_TABm(unit, vlan, &vlan_tab);
    if (VLAN_TABm_VALIDf_GET(vlan_tab) == 0) {
        return ioerr ? CDK_E_IO : CDK_E_NOT_FOUND;
    }
    ioerr += READ_EGR_VLANm(unit, vlan, &egr_vlan);
    VLAN_TABm_PORT_BITMAPf_GET(vlan_tab, pbm);
    if (PBM_MEMBER(pbm, lport) == 0) {
        return ioerr ? CDK_E_IO : CDK_E_NOT_FOUND;
    }
    PBM_PORT_REMOVE(pbm, lport);
    VLAN_TABm_PORT_BITMAPf_SET(vlan_tab, pbm);
    EGR_VLANm_UT_BITMAPf_GET(egr_vlan, pbm);
    PBM_PORT_REMOVE(pbm, lport);
    EGR_VLANm_UT_BITMAPf_SET(egr_vlan, pbm);
    ioerr += WRITE_VLAN_TABm(unit, vlan, vlan_tab);
    ioerr += WRITE_EGR_VLANm(unit, vlan, egr_vlan);

    /* VLAN membership for local device ports is controlled by 
     * shared profile table.
     */
    ioerr += READ_ING_VLAN_VFI_MEMBERSHIPm(unit, vlan, &ing_vlan_member);
    ING_VLAN_VFI_MEMBERSHIPm_ING_PORT_BITMAPf_GET(ing_vlan_member, pbm);
    PBM_PORT_REMOVE(pbm, lport);
    ING_VLAN_VFI_MEMBERSHIPm_ING_PORT_BITMAPf_SET(ing_vlan_member, pbm);
    ioerr += WRITE_ING_VLAN_VFI_MEMBERSHIPm(unit, vlan, ing_vlan_member);

    ioerr += READ_EGR_VLAN_VFI_MEMBERSHIPm(unit, vlan, &egr_vlan_member);
    EGR_VLAN_VFI_MEMBERSHIPm_PORT_BITMAPf_GET(egr_vlan_member, pbm);
    PBM_PORT_REMOVE(pbm, lport);
    EGR_VLAN_VFI_MEMBERSHIPm_PORT_BITMAPf_SET(egr_vlan_member, pbm);
    ioerr += WRITE_EGR_VLAN_VFI_MEMBERSHIPm(unit, vlan, egr_vlan_member);
    
    return ioerr ? CDK_E_IO : CDK_E_NONE;
}
#endif /* CDK_CONFIG_INCLUDE_BCM56560_A0 */

