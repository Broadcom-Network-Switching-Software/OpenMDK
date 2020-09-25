#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56450_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#include <cdk/chip/bcm56450_a0_defs.h>

#include "bcm56450_a0_bmd.h"
#include "bcm56450_a0_internal.h"

int
bcm56450_a0_bmd_vlan_port_remove(int unit, int vlan, int port)
{
    int ioerr = 0;
    int ppport;
    VLAN_TABm_t vlan_tab;
    EGR_VLANm_t egr_vlan;
    uint32_t pbmp, mask;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_VLAN(unit, vlan);
    BMD_CHECK_PORT(unit, port);

    ppport = P2PP(unit, port);

    ioerr += READ_VLAN_TABm(unit, vlan, &vlan_tab);
    if (VLAN_TABm_VALIDf_GET(vlan_tab) == 0) {
        return ioerr ? CDK_E_IO : CDK_E_NOT_FOUND;
    }
    ioerr += READ_EGR_VLANm(unit, vlan, &egr_vlan);
    if (ppport >= 32) {
        mask = 1 << (ppport - 32);
        pbmp = VLAN_TABm_PORT_BITMAP_W1f_GET(vlan_tab);
        if ((pbmp & mask) == 0) {
            return ioerr ? CDK_E_IO : CDK_E_NOT_FOUND;
        }
        VLAN_TABm_PORT_BITMAP_W1f_SET(vlan_tab, pbmp & ~mask);
        pbmp = EGR_VLANm_PORT_BITMAP_W1f_GET(egr_vlan);
        EGR_VLANm_PORT_BITMAP_W1f_SET(egr_vlan, pbmp & ~mask);
        pbmp = EGR_VLANm_UT_BITMAP_W1f_GET(egr_vlan);
        EGR_VLANm_UT_BITMAP_W1f_SET(egr_vlan, pbmp & ~mask);
    } else {
        mask = 1 << ppport;
        pbmp = VLAN_TABm_PORT_BITMAP_W0f_GET(vlan_tab);
        if ((pbmp & mask) == 0) {
            return ioerr ? CDK_E_IO : CDK_E_NOT_FOUND;
        }
        VLAN_TABm_PORT_BITMAP_W0f_SET(vlan_tab, pbmp & ~mask);
        pbmp = EGR_VLANm_PORT_BITMAP_W0f_GET(egr_vlan);
        EGR_VLANm_PORT_BITMAP_W0f_SET(egr_vlan, pbmp & ~mask);
        pbmp = EGR_VLANm_UT_BITMAP_W0f_GET(egr_vlan);
        EGR_VLANm_UT_BITMAP_W0f_SET(egr_vlan, pbmp & ~mask);
    }
    ioerr += WRITE_VLAN_TABm(unit, vlan, vlan_tab);
    ioerr += WRITE_EGR_VLANm(unit, vlan, egr_vlan);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}
#endif /* CDK_CONFIG_INCLUDE_BCM56450_A0 */
