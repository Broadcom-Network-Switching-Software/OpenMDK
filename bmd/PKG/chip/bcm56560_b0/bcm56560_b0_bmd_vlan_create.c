/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56560_B0 == 1

#include <bmd/bmd.h>
#include <cdk/cdk_debug.h>
#include <cdk/chip/bcm56560_b0_defs.h>

#include "bcm56560_b0_bmd.h"

int 
bcm56560_b0_bmd_vlan_create(int unit, int vlan)
{
    int ioerr = 0;
    VLAN_TABm_t vlan_tab;
    EGR_VLANm_t egr_vlan;
    VLAN_MPLSm_t vlan_mpls;
    
    BMD_CHECK_UNIT(unit);
    BMD_CHECK_VLAN(unit, vlan);
    ioerr += READ_VLAN_TABm(unit, vlan, &vlan_tab);
    if (VLAN_TABm_STGf_GET(vlan_tab) == 1) {
        return CDK_E_EXISTS;
    }
    VLAN_TABm_STGf_SET(vlan_tab, 1);
    VLAN_TABm_VALIDf_SET(vlan_tab, 1);
    VLAN_TABm_VLAN_PROFILE_PTRf_SET(vlan_tab, VLAN_PROFILE_TABm_MAX);
    ioerr += WRITE_VLAN_TABm(unit, vlan, vlan_tab);

    /* VLAN membership for local device ports is controlled by 
     * shared profile table.
     */
    ioerr += READ_VLAN_MPLSm(unit, vlan, &vlan_mpls);
    VLAN_MPLSm_MEMBERSHIP_PROFILE_PTRf_SET(vlan_mpls, vlan);
    VLAN_MPLSm_EN_IFILTERf_SET(vlan_mpls, 1);
    ioerr += WRITE_VLAN_MPLSm(unit, vlan, vlan_mpls);
    
    EGR_VLANm_CLR(egr_vlan);
    EGR_VLANm_STGf_SET(egr_vlan, 1);
    EGR_VLANm_VALIDf_SET(egr_vlan, 1);
    EGR_VLANm_MEMBERSHIP_PROFILE_PTRf_SET(egr_vlan, vlan);
    EGR_VLANm_EN_EFILTERf_SET(egr_vlan, 1);
    ioerr += WRITE_EGR_VLANm(unit, vlan, egr_vlan);
    return ioerr ? CDK_E_IO : CDK_E_NONE;
}
#endif /* CDK_CONFIG_INCLUDE_BCM56560_B0 */

