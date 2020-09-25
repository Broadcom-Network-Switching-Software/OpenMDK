#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56218_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#include <cdk/chip/bcm56218_a0_defs.h>

#include "bcm56218_a0_bmd.h"

int
bcm56218_a0_bmd_vlan_port_add(int unit, int vlan, int port, uint32_t flags)
{
    int ioerr = 0;
    VLAN_TABm_t vlan_tab;
    EGR_VLANm_t egr_vlan;
    uint32_t pbmp;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_VLAN(unit, vlan);
    BMD_CHECK_PORT(unit, port);

    ioerr += READ_VLAN_TABm(unit, vlan, &vlan_tab);
    if (VLAN_TABm_VALIDf_GET(vlan_tab) == 0) {
        return ioerr ? CDK_E_IO : CDK_E_NOT_FOUND;
    }
    if (port >= 32) {
        pbmp = VLAN_TABm_PORT_BITMAP_HIf_GET(vlan_tab);
        if (pbmp & (1 << (port - 32))) {
            return ioerr ? CDK_E_IO : CDK_E_EXISTS;
        }
        VLAN_TABm_PORT_BITMAP_HIf_SET(vlan_tab, pbmp | (1 << (port - 32)));
    } else {
        pbmp = VLAN_TABm_PORT_BITMAP_LOf_GET(vlan_tab);
        if (pbmp & (1 << port)) {
            return ioerr ? CDK_E_IO : CDK_E_EXISTS;
        }
        VLAN_TABm_PORT_BITMAP_LOf_SET(vlan_tab, pbmp | (1 << port));
    }
    ioerr += WRITE_VLAN_TABm(unit, vlan, vlan_tab);

    if (flags & BMD_VLAN_PORT_F_UNTAGGED) {
        ioerr += READ_EGR_VLANm(unit, vlan, &egr_vlan);
        if (port >= 32) {
            pbmp = EGR_VLANm_UT_BITMAP_HIf_GET(egr_vlan);
            EGR_VLANm_UT_BITMAP_HIf_SET(egr_vlan, pbmp | (1 << (port - 32)));
        } else {
            pbmp = EGR_VLANm_UT_BITMAP_LOf_GET(egr_vlan);
            EGR_VLANm_UT_BITMAP_LOf_SET(egr_vlan, pbmp | (1 << port));
        }
        ioerr += WRITE_EGR_VLANm(unit, vlan, egr_vlan);
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

#endif /* CDK_CONFIG_INCLUDE_BCM56218_A0 */
