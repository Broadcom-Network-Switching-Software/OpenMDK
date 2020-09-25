#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56514_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#include <cdk/chip/bcm56514_a0_defs.h>

#include "bcm56514_a0_bmd.h"

int
bcm56514_a0_bmd_vlan_create(int unit, int vlan)
{
    int ioerr = 0;
    VLAN_TABm_t vlan_tab;
    EGR_VLANm_t egr_vlan;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_VLAN(unit, vlan);

    ioerr += READ_VLAN_TABm(unit, vlan, &vlan_tab);
    if (VLAN_TABm_STGf_GET(vlan_tab) == 1) {
        return CDK_E_EXISTS;
    }
    VLAN_TABm_STGf_SET(vlan_tab, 1);
    VLAN_TABm_VALIDf_SET(vlan_tab, 1);
    VLAN_TABm_VLAN_PROFILE_PTRf_SET(vlan_tab, 0);
    ioerr += WRITE_VLAN_TABm(unit, vlan, vlan_tab);

    ioerr += READ_EGR_VLANm(unit, vlan, &egr_vlan);
    EGR_VLANm_STGf_SET(egr_vlan, 1);
    EGR_VLANm_VALIDf_SET(egr_vlan, 1);
    EGR_VLANm_UT_BITMAPf_SET(egr_vlan, 0);
    ioerr += WRITE_EGR_VLANm(unit, vlan, egr_vlan);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}
#endif /* CDK_CONFIG_INCLUDE_BCM56514_A0 */
