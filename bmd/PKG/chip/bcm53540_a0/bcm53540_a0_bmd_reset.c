/*
 *
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53540_A0 == 1

#include <bmd/bmd.h>

#include <cdk/arch/xgsd_chip.h>
#include <cdk/cdk_debug.h>
#include <cdk/arch/xgsd_miim.h>
#include <cdk/chip/bcm53540_a0_defs.h>

#include "bcm53540_a0_bmd.h"
#include "bcm53540_a0_internal.h"

static int
_sgmii_init(int unit, int sgmii_inst)
{
    int blk;
    TOP_SGMII_CTRL_REGr_t top_sgmii_ctrl;
    GPORT_SGMII0_CTRL_REGr_t gp_sgmii_ctrl;
    int sleep_usec = 1100;

    if (sgmii_inst == 0) {
        /* TOP_SGMII_CTRL_REGr */
        READ_TOP_SGMII_CTRL_REGr(unit, &top_sgmii_ctrl);
        TOP_SGMII_CTRL_REGr_IDDQf_SET(top_sgmii_ctrl, 0);
        /* Deassert power down */
        TOP_SGMII_CTRL_REGr_PWRDWNf_SET(top_sgmii_ctrl, 0);
        WRITE_TOP_SGMII_CTRL_REGr(unit, top_sgmii_ctrl);
        BMD_SYS_USLEEP(sleep_usec);

        /* Reset SGMII */
        TOP_SGMII_CTRL_REGr_RSTB_HWf_SET(top_sgmii_ctrl, 0);
        WRITE_TOP_SGMII_CTRL_REGr(unit, top_sgmii_ctrl);
        BMD_SYS_USLEEP(sleep_usec + 10000);

        /* Bring SGMII out of reset */
        TOP_SGMII_CTRL_REGr_RSTB_HWf_SET(top_sgmii_ctrl, 1);
        WRITE_TOP_SGMII_CTRL_REGr(unit, top_sgmii_ctrl);
        BMD_SYS_USLEEP(sleep_usec);

        /* Activate MDIO on SGMII */
        TOP_SGMII_CTRL_REGr_RSTB_MDIOREGSf_SET(top_sgmii_ctrl, 1);
        WRITE_TOP_SGMII_CTRL_REGr(unit, top_sgmii_ctrl);
        BMD_SYS_USLEEP(sleep_usec);

        /* Activate clocks */
        TOP_SGMII_CTRL_REGr_RSTB_PLLf_SET(top_sgmii_ctrl, 1);
        WRITE_TOP_SGMII_CTRL_REGr(unit, top_sgmii_ctrl);
    } else if (sgmii_inst == 1) {
        blk = 2;
        /* Deassert power down */
        READ_GPORT_SGMII0_CTRL_REGr(unit, blk, &gp_sgmii_ctrl);
        GPORT_SGMII0_CTRL_REGr_IDDQf_SET(gp_sgmii_ctrl, 0);
        GPORT_SGMII0_CTRL_REGr_PWRDWNf_SET(gp_sgmii_ctrl, 0);
        WRITE_GPORT_SGMII0_CTRL_REGr(unit, blk, gp_sgmii_ctrl);
        BMD_SYS_USLEEP(sleep_usec);

        /* Reset SGMII */
        GPORT_SGMII0_CTRL_REGr_RSTB_HWf_SET(gp_sgmii_ctrl, 0);
        WRITE_GPORT_SGMII0_CTRL_REGr(unit, blk, gp_sgmii_ctrl);
        BMD_SYS_USLEEP(sleep_usec + 10000);

        /* Bring SGMII out of reset */
        GPORT_SGMII0_CTRL_REGr_RSTB_HWf_SET(gp_sgmii_ctrl, 1);
        WRITE_GPORT_SGMII0_CTRL_REGr(unit, blk, gp_sgmii_ctrl);
        BMD_SYS_USLEEP(sleep_usec);

        /* Activate MDIO on SGMII */
        GPORT_SGMII0_CTRL_REGr_RSTB_MDIOREGSf_SET(gp_sgmii_ctrl, 1);
        WRITE_GPORT_SGMII0_CTRL_REGr(unit, blk, gp_sgmii_ctrl);
        BMD_SYS_USLEEP(sleep_usec);

        /* Activate clocks */
        GPORT_SGMII0_CTRL_REGr_RSTB_PLLf_SET(gp_sgmii_ctrl, 1);
        WRITE_GPORT_SGMII0_CTRL_REGr(unit, blk, gp_sgmii_ctrl);
    }

    return CDK_E_NONE;
}

static const uint32_t _qgphy_core_pbmp[6] = {
    0xF, 0xF0, 0xF00, 0xF000, 0xF0000, 0xF00000
};

#define WH2_QGPHY_CORE_AMAC             5

static int
_qgphy_init(int unit, uint32_t qgphy_core_map, uint32_t qgphy5_lane)
{
    int amac = 0;
    int sleep_usec = 1100;
    TOP_SOFT_RESET_REGr_t top_sreset;
    TOP_QGPHY_CTRL_0r_t qgphy_ctrl0;
    TOP_QGPHY_CTRL_2r_t qgphy_ctrl2;
    int idx;
    uint32_t pbmp, core, core_temp, iddq_pwr, iddq_bias;
    uint32_t pbmp_temp;

    READ_TOP_SOFT_RESET_REGr(unit, &top_sreset);
    core = TOP_SOFT_RESET_REGr_TOP_QGPHY_RST_Lf_GET(top_sreset);

    /* Check if gphy5 is up and used by AMAC */
    if (core & (1 << WH2_QGPHY_CORE_AMAC)) {
        if (CDK_DEV_FLAGS(unit) & CDK_DEV_MBUS_AXI) {
            amac = 1;
        }
    }

    /* Power up the QGPHY */
    pbmp_temp = 0;
    for (idx = 0; idx < 6; idx++) {
        if (qgphy_core_map & (1 << idx)) {
            pbmp_temp |= _qgphy_core_pbmp[idx];
        }
    }
    if (amac) {
        pbmp_temp &= ~_qgphy_core_pbmp[WH2_QGPHY_CORE_AMAC];
    }

    READ_TOP_QGPHY_CTRL_0r(unit, &qgphy_ctrl0);
    pbmp = TOP_QGPHY_CTRL_0r_EXT_PWRDOWNf_GET(qgphy_ctrl0);
    pbmp &= ~pbmp_temp;

    TOP_QGPHY_CTRL_0r_EXT_PWRDOWNf_SET(qgphy_ctrl0, pbmp);
    WRITE_TOP_QGPHY_CTRL_0r(unit, qgphy_ctrl0);
    BMD_SYS_USLEEP(sleep_usec);

    /* Release iddq isolation */
    core = qgphy_core_map;
    if (amac || (qgphy5_lane == 0x3)) {
        core &= ~(1 << 5);
    }
    READ_TOP_QGPHY_CTRL_2r(unit, &qgphy_ctrl2);
    iddq_pwr = TOP_QGPHY_CTRL_2r_GPHY_IDDQ_GLOBAL_PWRf_GET(qgphy_ctrl2);
    iddq_bias = TOP_QGPHY_CTRL_2r_IDDQ_BIASf_GET(qgphy_ctrl2);
    iddq_pwr &= ~core;
    iddq_bias &= ~core;
    TOP_QGPHY_CTRL_2r_GPHY_IDDQ_GLOBAL_PWRf_SET(qgphy_ctrl2, iddq_pwr);
    TOP_QGPHY_CTRL_2r_IDDQ_BIASf_SET(qgphy_ctrl2, iddq_bias);
    WRITE_TOP_QGPHY_CTRL_2r(unit, qgphy_ctrl2);
    BMD_SYS_USLEEP(sleep_usec);

    /* Toggle reset */
    READ_TOP_SOFT_RESET_REGr(unit, &top_sreset);
    core_temp = TOP_SOFT_RESET_REGr_TOP_QGPHY_RST_Lf_GET(top_sreset);
    core_temp |= core;
    TOP_SOFT_RESET_REGr_TOP_QGPHY_RST_Lf_SET(top_sreset, core_temp);
    WRITE_TOP_SOFT_RESET_REGr(unit, top_sreset);
    BMD_SYS_USLEEP(sleep_usec);

    /* Full Quad power down */
    pbmp_temp = 0;
    core_temp = 0x3F;
    core_temp &= ~qgphy_core_map;
    /* Power down qgphy 5 first then enable some bitmap later */
    if (amac) {
        /* Avoid power down qgphy5 when amac is using it */
        core_temp &= ~(1 << WH2_QGPHY_CORE_AMAC);
    } else if (qgphy5_lane == 0x3) {
        core_temp |= (1 << WH2_QGPHY_CORE_AMAC);
    }

    if (core_temp) {
        for (idx = 0; idx < 6; idx++) {
            if (core_temp & (1 << idx)) {
                pbmp_temp |= _qgphy_core_pbmp[idx];
            }
        }
        READ_TOP_QGPHY_CTRL_0r(unit, &qgphy_ctrl0);
        pbmp = TOP_QGPHY_CTRL_0r_EXT_PWRDOWNf_GET(qgphy_ctrl0);
        pbmp |= pbmp_temp;
        TOP_QGPHY_CTRL_0r_EXT_PWRDOWNf_SET(qgphy_ctrl0, pbmp);
        WRITE_TOP_QGPHY_CTRL_0r(unit, qgphy_ctrl0);

        /* Toggle iddq isolation and reset */
        READ_TOP_QGPHY_CTRL_2r(unit, &qgphy_ctrl2);
        iddq_pwr = TOP_QGPHY_CTRL_2r_GPHY_IDDQ_GLOBAL_PWRf_GET(qgphy_ctrl2);
        iddq_bias = TOP_QGPHY_CTRL_2r_IDDQ_BIASf_GET(qgphy_ctrl2);
        iddq_pwr |= core_temp;
        iddq_bias |= core_temp;

        TOP_QGPHY_CTRL_2r_GPHY_IDDQ_GLOBAL_PWRf_SET(qgphy_ctrl2, iddq_pwr);
        TOP_QGPHY_CTRL_2r_IDDQ_BIASf_SET(qgphy_ctrl2, iddq_pwr);
        WRITE_TOP_QGPHY_CTRL_2r(unit, qgphy_ctrl2);
        BMD_SYS_USLEEP(sleep_usec);

        READ_TOP_SOFT_RESET_REGr(unit, &top_sreset);
        core = TOP_SOFT_RESET_REGr_TOP_QGPHY_RST_Lf_GET(top_sreset);
        core |= core_temp;

        TOP_SOFT_RESET_REGr_TOP_QGPHY_RST_Lf_SET(top_sreset, core);
        WRITE_TOP_SOFT_RESET_REGr(unit, top_sreset);
        BMD_SYS_USLEEP(sleep_usec);

        /* iddq_pw1 = 0,  iddq_bias = 0, reset_n = 0*/
        READ_TOP_QGPHY_CTRL_2r(unit, &qgphy_ctrl2);
        iddq_pwr = TOP_QGPHY_CTRL_2r_GPHY_IDDQ_GLOBAL_PWRf_GET(qgphy_ctrl2);
        iddq_bias = TOP_QGPHY_CTRL_2r_IDDQ_BIASf_GET(qgphy_ctrl2);
        iddq_pwr &= ~core_temp;
        iddq_bias &= ~core_temp;

        TOP_QGPHY_CTRL_2r_GPHY_IDDQ_GLOBAL_PWRf_SET(qgphy_ctrl2, iddq_pwr);
        TOP_QGPHY_CTRL_2r_IDDQ_BIASf_SET(qgphy_ctrl2, iddq_pwr);
        WRITE_TOP_QGPHY_CTRL_2r(unit, qgphy_ctrl2);
        BMD_SYS_USLEEP(sleep_usec);

        READ_TOP_SOFT_RESET_REGr(unit, &top_sreset);
        core = TOP_SOFT_RESET_REGr_TOP_QGPHY_RST_Lf_GET(top_sreset);
        core &= ~core_temp;

        TOP_SOFT_RESET_REGr_TOP_QGPHY_RST_Lf_SET(top_sreset, core);
        WRITE_TOP_SOFT_RESET_REGr(unit, top_sreset);
        BMD_SYS_USLEEP(sleep_usec);

        /* reset_n = 1, iddq_pw1 = 1,  iddq_bias = 1 */
        READ_TOP_SOFT_RESET_REGr(unit, &top_sreset);
        core = TOP_SOFT_RESET_REGr_TOP_QGPHY_RST_Lf_GET(top_sreset);
        core |= core_temp;

        TOP_SOFT_RESET_REGr_TOP_QGPHY_RST_Lf_SET(top_sreset, core);
        WRITE_TOP_SOFT_RESET_REGr(unit, top_sreset);
        BMD_SYS_USLEEP(sleep_usec);

        READ_TOP_QGPHY_CTRL_2r(unit, &qgphy_ctrl2);
        iddq_pwr = TOP_QGPHY_CTRL_2r_GPHY_IDDQ_GLOBAL_PWRf_GET(qgphy_ctrl2);
        iddq_bias = TOP_QGPHY_CTRL_2r_IDDQ_BIASf_GET(qgphy_ctrl2);
        iddq_pwr |= core_temp;
        iddq_bias |= core_temp;

        TOP_QGPHY_CTRL_2r_GPHY_IDDQ_GLOBAL_PWRf_SET(qgphy_ctrl2, iddq_pwr);
        TOP_QGPHY_CTRL_2r_IDDQ_BIASf_SET(qgphy_ctrl2, iddq_pwr);
        WRITE_TOP_QGPHY_CTRL_2r(unit, qgphy_ctrl2);
        BMD_SYS_USLEEP(sleep_usec);
    }

    /* Partial power up */
    if (qgphy5_lane == 0x3) {
        if (!amac) {
            /* iddq_pw1 = 0,  iddq_bias = 0 */
            READ_TOP_QGPHY_CTRL_2r(unit, &qgphy_ctrl2);
            iddq_pwr = TOP_QGPHY_CTRL_2r_GPHY_IDDQ_GLOBAL_PWRf_GET(qgphy_ctrl2);
            iddq_bias = TOP_QGPHY_CTRL_2r_IDDQ_BIASf_GET(qgphy_ctrl2);
            iddq_pwr &= ~(1 << 5);
            iddq_bias &= ~(1 << 5);
            TOP_QGPHY_CTRL_2r_GPHY_IDDQ_GLOBAL_PWRf_SET(qgphy_ctrl2, iddq_pwr);
            TOP_QGPHY_CTRL_2r_IDDQ_BIASf_SET(qgphy_ctrl2, iddq_pwr);
            WRITE_TOP_QGPHY_CTRL_2r(unit, qgphy_ctrl2);
            BMD_SYS_USLEEP(sleep_usec);
        }

        /* Power up qghphy lane */
        READ_TOP_QGPHY_CTRL_0r(unit, &qgphy_ctrl0);
        pbmp = TOP_QGPHY_CTRL_0r_EXT_PWRDOWNf_GET(qgphy_ctrl0);
        pbmp &= ~(qgphy5_lane << 20);
        TOP_QGPHY_CTRL_0r_EXT_PWRDOWNf_SET(qgphy_ctrl0, pbmp);
        WRITE_TOP_QGPHY_CTRL_0r(unit, qgphy_ctrl0);
        BMD_SYS_USLEEP(sleep_usec);
    }

    return CDK_E_NONE;
}

#define _STRAP_GPHY_SGMII_SEL_0  (1 << 0)
#define _STRAP_GPHY_SGMII_SEL_1  (1 << 1)
static int
_pll_config(unit)
{
    int ioerr = 0;
    int wait_usec = 10000;
    TOP_STRAP_STATUS_1r_t strap_status_1;
    TOP_CORE_PLL_CTRL4r_t top_pll_ctrl4;
    TOP_MISC_CONTROL_1r_t top_misc_ctrl_1;
    uint32_t qgphy_core_map, qgphy5_lane, sgmii_4p0_lane;
    uint32_t strap_sts_1, val;
    TOP_SOFT_RESET_REGr_t top_sreset;
    uint32_t option, mdiv;

    switch (CORE_CLK(unit)) {
    case 62:
        mdiv = 0x28;
        break;
    case 41:
        mdiv = 0x3c;
        break;
    default:
        mdiv = 0x18;
        break;
    }
    ioerr += READ_TOP_CORE_PLL_CTRL4r(unit, &top_pll_ctrl4);
    TOP_CORE_PLL_CTRL4r_MSTR_CH0_MDIVf_SET(top_pll_ctrl4, mdiv);
    ioerr += WRITE_TOP_CORE_PLL_CTRL4r(unit, top_pll_ctrl4);

    READ_TOP_MISC_CONTROL_1r(unit, &top_misc_ctrl_1);
    TOP_MISC_CONTROL_1r_CMIC_TO_CORE_PLL_LOADf_SET(top_misc_ctrl_1, 1);
    ioerr += WRITE_TOP_MISC_CONTROL_1r(unit, top_misc_ctrl_1);

    /* Check option and strap pin status to do the proper initialization */
    option = bcm53540_a0_sku_option_get(unit);
    qgphy_core_map = QGPHY_CORE(unit);
    qgphy5_lane = QGPHY5_LANE(unit);
    sgmii_4p0_lane = SGMII_4P0_LANE(unit);
    option = option % 6;
    ioerr += READ_TOP_STRAP_STATUS_1r(unit, &strap_status_1);
    /* check strap v.s. option */
    strap_sts_1 = TOP_STRAP_STATUS_1r_GET(strap_status_1);
    val = (strap_sts_1 >> 1) & 0x3;
    switch (option) {
        case 0:
        case 2:
            if (val != 3) {
                CDK_ERR(("Invalid option (unmatch with strap pin)\n"));
                return CDK_E_INTERNAL;
            }
            break;
        case 3:
        case 4:
            if (val != 1) {
                CDK_ERR(("Invalid option (unmatch with strap pin)\n"));
                return CDK_E_INTERNAL;
            }
            break;
        case 1:
        case 5:
            if (val != 0) {
                CDK_ERR(("Invalid option (unmatch with strap pin)\n"));
                return CDK_E_INTERNAL;
            }
            break;
    }

    if (val & _STRAP_GPHY_SGMII_SEL_0) {
        /* front ports select GPHY */
        if (!(qgphy_core_map & 0x20) || !(qgphy5_lane & 0x3)) {
            CDK_ERR(("Invalid option (unmatch with strap pin)\n"));
            return CDK_E_INTERNAL;
        }
    }
    if (val & _STRAP_GPHY_SGMII_SEL_1) {
        /* front ports select GPHY, OOB port select SGMII */
        if (!(qgphy_core_map & 0x20) || !(qgphy5_lane & 0xC)) {
            CDK_ERR(("Invalid option (unmatch with strap pin)\n"));
            return CDK_E_INTERNAL;
        }
    }

    /* Bring port block out of reset */
    /* GPHY or SGMII configuration need to be considered here */
    if (sgmii_4p0_lane) {
        _sgmii_init(unit, 0);
    }
    _qgphy_init(unit, qgphy_core_map, qgphy5_lane);

    ioerr += READ_TOP_SOFT_RESET_REGr(unit, &top_sreset);
    TOP_SOFT_RESET_REGr_TOP_GXP0_RST_Lf_SET(top_sreset, 1);
    TOP_SOFT_RESET_REGr_TOP_GXP1_RST_Lf_SET(top_sreset, 1);
    TOP_SOFT_RESET_REGr_TOP_GXP2_RST_Lf_SET(top_sreset, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REGr(unit, top_sreset);
    BMD_SYS_USLEEP(wait_usec);

    return ioerr;
}

static int
_gphy_pmq_init(int unit, int blk)
{
    int sleep_usec = 1100;
    GPORT_XGXS0_CTRL_REGr_t gp_ctrl;

    READ_GPORT_XGXS0_CTRL_REGr(unit, blk, &gp_ctrl);
    GPORT_XGXS0_CTRL_REGr_IDDQf_SET(gp_ctrl, 0);
    /* Deassert power down */
    GPORT_XGXS0_CTRL_REGr_PWRDWNf_SET(gp_ctrl, 0);
    WRITE_GPORT_XGXS0_CTRL_REGr(unit, blk, gp_ctrl);
    BMD_SYS_USLEEP(sleep_usec);

    /* Reset XGXS */
    GPORT_XGXS0_CTRL_REGr_RSTB_HWf_SET(gp_ctrl, 0);
    WRITE_GPORT_XGXS0_CTRL_REGr(unit, blk, gp_ctrl);
    BMD_SYS_USLEEP(sleep_usec + 10000);

    /* Bring XGXS out of reset */
    GPORT_XGXS0_CTRL_REGr_RSTB_HWf_SET(gp_ctrl, 1);
    WRITE_GPORT_XGXS0_CTRL_REGr(unit, blk, gp_ctrl);
    BMD_SYS_USLEEP(sleep_usec);

    /* Activate MDIO on SGMII */
    GPORT_XGXS0_CTRL_REGr_RSTB_REFCLKf_SET(gp_ctrl, 1);
    WRITE_GPORT_XGXS0_CTRL_REGr(unit, blk, gp_ctrl);
    BMD_SYS_USLEEP(sleep_usec);

    /* Activate clocks */
    GPORT_XGXS0_CTRL_REGr_RSTB_PLLf_SET(gp_ctrl, 1);
    WRITE_GPORT_XGXS0_CTRL_REGr(unit, blk, gp_ctrl);

    return CDK_E_NONE;
}

static int
_port_reset(int unit)
{
    int blk;

    for (blk = 0; blk <= 2; blk++) {
        if (blk == 2) {
            /* reset SGMII_4P1 */
            _sgmii_init(unit, 1);
        } else {
            _gphy_pmq_init(unit, blk);
        }
    }

    return CDK_E_NONE;
}

int
bcm53540_a0_bmd_reset(int unit)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int wait_usec = 10000;
    int idx;
    CMIC_CPS_RESETr_t cmic_cps;
    CMIC_SBUS_RING_MAPr_t sbus_ring_map;
    CMIC_SBUS_TIMEOUTr_t cmic_sbus_timeout;
    uint32_t ring_map[] = { 0x00000000, 0x00430000, 0x00005064,
                            0x00000000, 0x77772222, 0x00000111 };
    TOP_SOFT_RESET_REGr_t top_sreset;
    TOP_SOFT_RESET_REG_2r_t top_sreset_2;

    BMD_CHECK_UNIT(unit);

    /* Initialize endian mode for correct reset access */
    ioerr += cdk_xgsd_cmic_init(unit);

    /* Pull reset line */
    ioerr += READ_CMIC_CPS_RESETr(unit, &cmic_cps);
    CMIC_CPS_RESETr_CPS_RESETf_SET(cmic_cps, 1);
    ioerr += WRITE_CMIC_CPS_RESETr(unit, cmic_cps);

    /* Wait for all tables to initialize */
    BMD_SYS_USLEEP(wait_usec);

    /* Re-initialize endian mode after reset */
    ioerr += cdk_xgsd_cmic_init(unit);

    for (idx = 0; idx < COUNTOF(ring_map); idx++) {
        CMIC_SBUS_RING_MAPr_SET(sbus_ring_map, ring_map[idx]);
        ioerr += WRITE_CMIC_SBUS_RING_MAPr(unit, idx, sbus_ring_map);
    }

    CMIC_SBUS_TIMEOUTr_SET(cmic_sbus_timeout, 0x7d0);
    ioerr += WRITE_CMIC_SBUS_TIMEOUTr(unit, cmic_sbus_timeout);
    BMD_SYS_USLEEP(wait_usec);

    ioerr += _pll_config(unit);

    /* Bring network sync out of reset */
    READ_TOP_SOFT_RESET_REGr(unit, &top_sreset);
    TOP_SOFT_RESET_REGr_TOP_TS_RST_Lf_SET(top_sreset, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REGr(unit, top_sreset);
    BMD_SYS_USLEEP(wait_usec);

    READ_TOP_SOFT_RESET_REG_2r(unit, &top_sreset_2);
    TOP_SOFT_RESET_REG_2r_TOP_TS_PLL_RST_Lf_SET(top_sreset_2, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REG_2r(unit, top_sreset_2);
    TOP_SOFT_RESET_REG_2r_TOP_TS_PLL_POST_RST_Lf_SET(top_sreset_2, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REG_2r(unit, top_sreset_2);

    /* Bring IP, EP, and MMU blocks out of reset */
    READ_TOP_SOFT_RESET_REGr(unit, &top_sreset);
    TOP_SOFT_RESET_REGr_TOP_EP_RST_Lf_SET(top_sreset, 1);
    TOP_SOFT_RESET_REGr_TOP_IP_RST_Lf_SET(top_sreset, 1);
    TOP_SOFT_RESET_REGr_TOP_MMU_RST_Lf_SET(top_sreset, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REGr(unit, top_sreset);
    BMD_SYS_USLEEP(wait_usec);

    _port_reset(unit);

    return ioerr ? CDK_E_IO : rv;
}

#endif /* CDK_CONFIG_INCLUDE_BCM53540_A0 */
