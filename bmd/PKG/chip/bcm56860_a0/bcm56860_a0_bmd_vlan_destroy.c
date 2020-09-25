/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56860_A0 == 1

#include <bmd/bmd.h>

#include <cdk/chip/bcm56860_a0_defs.h>

#include "bcm56860_a0_bmd.h"

int 
bcm56860_a0_bmd_vlan_destroy(int unit, int vlan)
{
    int ioerr = 0;
    EGR_VLANm_t egr_vlan;
    VLAN_TABm_t vlan_tab;
    VLAN_MPLSm_t vlan_mpls;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_VLAN(unit, vlan);

    ioerr += READ_VLAN_TABm(unit, vlan, &vlan_tab);
    if (VLAN_TABm_VALIDf_GET(vlan_tab) == 0) {
        return ioerr ? CDK_E_IO : CDK_E_NOT_FOUND;
    }
    VLAN_TABm_CLR(vlan_tab);
    ioerr += WRITE_VLAN_TABm(unit, vlan, vlan_tab);

    EGR_VLANm_CLR(egr_vlan);
    ioerr += WRITE_EGR_VLANm(unit, vlan, egr_vlan);

    ioerr += READ_VLAN_MPLSm(unit, vlan, &vlan_mpls);
    VLAN_MPLSm_MEMBERSHIP_PROFILE_PTRf_SET(vlan_mpls, 0);
    VLAN_MPLSm_EN_IFILTERf_SET(vlan_mpls, 0);
    ioerr += WRITE_VLAN_MPLSm(unit, vlan, vlan_mpls);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}
#endif /* CDK_CONFIG_INCLUDE_BCM56860_A0 */

