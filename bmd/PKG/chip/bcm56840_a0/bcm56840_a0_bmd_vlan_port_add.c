#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56840_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#include <cdk/chip/bcm56840_a0_defs.h>

#include "bcm56840_a0_bmd.h"
#include "bcm56840_a0_internal.h"

int
bcm56840_a0_read_egr_vlan(int unit, int vlan, EGR_VLANm_t *egr_vlan)
{
    int ioerr = 0;
    int wsize = CDK_BYTES2WORDS(EGR_VLANm_SIZE);
    int idx;
    uint32_t wdata;
    EGR_SBS_CONTROLr_t sbs_ctrl;
    EGR_VLANm_t egr_vlan_x, egr_vlan_y;

    ioerr += READ_EGR_VLANm(unit, vlan, &egr_vlan_x);

    EGR_SBS_CONTROLr_CLR(sbs_ctrl);
    EGR_SBS_CONTROLr_PIPE_SELECTf_SET(sbs_ctrl, 1);
    WRITE_EGR_SBS_CONTROLr(unit, sbs_ctrl);

    ioerr += READ_EGR_VLANm(unit, vlan, &egr_vlan_y);

    EGR_SBS_CONTROLr_PIPE_SELECTf_SET(sbs_ctrl, 0);
    WRITE_EGR_SBS_CONTROLr(unit, sbs_ctrl);

    for (idx = 0; idx < wsize; idx++) {
        wdata = EGR_VLANm_GET(egr_vlan_x, idx);
        wdata |= EGR_VLANm_GET(egr_vlan_y, idx);
        EGR_VLANm_SET(*egr_vlan, idx, wdata);
    }

    return ioerr;
}

int
bcm56840_a0_bmd_vlan_port_add(int unit, int vlan, int port, uint32_t flags)
{
    int ioerr = 0;
    int lport;
    VLAN_TABm_t vlan_tab;
    EGR_VLANm_t egr_vlan;
    uint32_t pbmp, mask;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_VLAN(unit, vlan);
    BMD_CHECK_PORT(unit, port);

    lport = P2L(unit, port);

    ioerr += READ_VLAN_TABm(unit, vlan, &vlan_tab);
    if (VLAN_TABm_VALIDf_GET(vlan_tab) == 0) {
        return ioerr ? CDK_E_IO : CDK_E_NOT_FOUND;
    }
    ioerr += bcm56840_a0_read_egr_vlan(unit, vlan, &egr_vlan);
    if (lport >= 64) {
        mask = 1 << (lport - 64);
        pbmp = VLAN_TABm_PORT_BITMAP_W2f_GET(vlan_tab);
        if (pbmp & mask) {
            return ioerr ? CDK_E_IO : CDK_E_EXISTS;
        }
        VLAN_TABm_PORT_BITMAP_W2f_SET(vlan_tab, pbmp | mask);
        pbmp = EGR_VLANm_PORT_BITMAP_W2f_GET(egr_vlan);
        EGR_VLANm_PORT_BITMAP_W2f_SET(egr_vlan, pbmp | mask);
        if (flags & BMD_VLAN_PORT_F_UNTAGGED) {
            pbmp = EGR_VLANm_UT_BITMAP_W2f_GET(egr_vlan);
            EGR_VLANm_UT_BITMAP_W2f_SET(egr_vlan, pbmp | mask);
        }
    } else if (lport >= 32) {
        mask = 1 << (lport - 32);
        pbmp = VLAN_TABm_PORT_BITMAP_W1f_GET(vlan_tab);
        if (pbmp & mask) {
            return ioerr ? CDK_E_IO : CDK_E_EXISTS;
        }
        VLAN_TABm_PORT_BITMAP_W1f_SET(vlan_tab, pbmp | mask);
        pbmp = EGR_VLANm_PORT_BITMAP_W1f_GET(egr_vlan);
        EGR_VLANm_PORT_BITMAP_W1f_SET(egr_vlan, pbmp | mask);
        if (flags & BMD_VLAN_PORT_F_UNTAGGED) {
            pbmp = EGR_VLANm_UT_BITMAP_W1f_GET(egr_vlan);
            EGR_VLANm_UT_BITMAP_W1f_SET(egr_vlan, pbmp | mask);
        }
    } else {
        mask = 1 << lport;
        pbmp = VLAN_TABm_PORT_BITMAP_W0f_GET(vlan_tab);
        if (pbmp & mask) {
            return ioerr ? CDK_E_IO : CDK_E_EXISTS;
        }
        VLAN_TABm_PORT_BITMAP_W0f_SET(vlan_tab, pbmp | mask);
        pbmp = EGR_VLANm_PORT_BITMAP_W0f_GET(egr_vlan);
        EGR_VLANm_PORT_BITMAP_W0f_SET(egr_vlan, pbmp | mask);
        if (flags & BMD_VLAN_PORT_F_UNTAGGED) {
            pbmp = EGR_VLANm_UT_BITMAP_W0f_GET(egr_vlan);
            EGR_VLANm_UT_BITMAP_W0f_SET(egr_vlan, pbmp | mask);
        }
    }
    ioerr += WRITE_VLAN_TABm(unit, vlan, vlan_tab);
    ioerr += WRITE_EGR_VLANm(unit, vlan, egr_vlan);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

#endif /* CDK_CONFIG_INCLUDE_BCM56840_A0 */
