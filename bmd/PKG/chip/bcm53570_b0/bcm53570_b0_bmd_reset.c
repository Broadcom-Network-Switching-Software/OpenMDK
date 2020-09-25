/*
 *
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53570_B0 == 1

#include <bmd/bmd.h>

#include <cdk/arch/xgsd_chip.h>
#include <cdk/cdk_debug.h>
#include <cdk/arch/xgsd_miim.h>
#include <cdk/chip/bcm53570_b0_defs.h>

#include "bcm53570_b0_bmd.h"
#include "bcm53570_b0_internal.h"

#define RESET_SLEEP_USEC                100
#define PLL_LOCK_MSEC                   500

static int
_gpio_set(int unit, int pin, int output, int val)
{
    int ioerr = 0;
    uint32_t fval = 0;
    CMIC_GP_OUT_ENr_t cmic_gp_out_en;
    CMIC_GP_DATA_OUTr_t cmic_gp_data_out;

    ioerr += READ_CMIC_GP_OUT_ENr(unit, &cmic_gp_out_en);
    fval = CMIC_GP_OUT_ENr_OUT_ENABLEf_GET(cmic_gp_out_en);
    if (output) {
        fval |= 1 << pin;
    } else {
        fval &= ~(1 << pin);
    }
    CMIC_GP_OUT_ENr_OUT_ENABLEf_SET(cmic_gp_out_en, fval);
    ioerr += WRITE_CMIC_GP_OUT_ENr(unit, cmic_gp_out_en);

    if (output) {
        /* coverity[result_independent_of_operands] */
        ioerr += READ_CMIC_GP_DATA_OUTr(unit, &cmic_gp_data_out);
        fval = CMIC_GP_DATA_OUTr_DATA_OUTf_GET(cmic_gp_data_out);
        if (val) {
            fval |= (val << pin);
        } else {
            fval &= ~(1 << pin);
        }
        CMIC_GP_DATA_OUTr_DATA_OUTf_SET(cmic_gp_data_out, fval);
        ioerr += WRITE_CMIC_GP_DATA_OUTr(unit, cmic_gp_data_out);
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

static void
_sgmii4px2_reset(int unit, int port, int reg_idx)
{
    GPORT_SGMII_CTRL_REGr_t sgmii_ctrl;
    int blk_idx;

    blk_idx = BLKIDX(unit, BLKTYPE_PMQ, port);

    if (blk_idx == -1) {
        return;
    }
    /*
     * Reference clock selection
     */
    READ_GPORT_SGMII_CTRL_REGr(unit, blk_idx, reg_idx, &sgmii_ctrl);
    GPORT_SGMII_CTRL_REGr_IDDQf_SET(sgmii_ctrl, 1);
    GPORT_SGMII_CTRL_REGr_PWRDWNf_SET(sgmii_ctrl, 1);
    WRITE_GPORT_SGMII_CTRL_REGr(unit, blk_idx, reg_idx, sgmii_ctrl);

    BMD_SYS_USLEEP(1100);

    /* Analog section powered */
    GPORT_SGMII_CTRL_REGr_IDDQf_SET(sgmii_ctrl, 0);
    /* Deassert power down */
    GPORT_SGMII_CTRL_REGr_PWRDWNf_SET(sgmii_ctrl, 0);
    WRITE_GPORT_SGMII_CTRL_REGr(unit, blk_idx, reg_idx, sgmii_ctrl);
    BMD_SYS_USLEEP(1100);

    /* Bring SGMII out of reset */
    GPORT_SGMII_CTRL_REGr_RSTB_HWf_SET(sgmii_ctrl, 1);
    WRITE_GPORT_SGMII_CTRL_REGr(unit, blk_idx, reg_idx, sgmii_ctrl);
    BMD_SYS_USLEEP(1100);

    /* Activate MDIO on SGMII */
    GPORT_SGMII_CTRL_REGr_RSTB_MDIOREGSf_SET(sgmii_ctrl, 1);
    WRITE_GPORT_SGMII_CTRL_REGr(unit, blk_idx, reg_idx, sgmii_ctrl);
    BMD_SYS_USLEEP(1100);

    /* Activate clocks */
    GPORT_SGMII_CTRL_REGr_RSTB_PLLf_SET(sgmii_ctrl, 1);
    WRITE_GPORT_SGMII_CTRL_REGr(unit, blk_idx, reg_idx, sgmii_ctrl);
}

static void
_tscq_xgxs_reset(int unit, int port)
{
    GPORT_XGXS0_CTRL_REGr_t gport_xgxs0_ctrl;
    int blk_idx;

    blk_idx = BLKIDX(unit, BLKTYPE_PMQ, port);

    READ_GPORT_XGXS0_CTRL_REGr(unit, blk_idx, &gport_xgxs0_ctrl);
    GPORT_XGXS0_CTRL_REGr_REFIN_ENf_SET(gport_xgxs0_ctrl, 1);
    WRITE_GPORT_XGXS0_CTRL_REGr(unit, blk_idx, gport_xgxs0_ctrl);

    /* Deassert power down */
    GPORT_XGXS0_CTRL_REGr_PWRDWNf_SET(gport_xgxs0_ctrl, 0);
    WRITE_GPORT_XGXS0_CTRL_REGr(unit, blk_idx, gport_xgxs0_ctrl);
    BMD_SYS_USLEEP(1100);

    /* Reset XGXS */
    GPORT_XGXS0_CTRL_REGr_RSTB_HWf_SET(gport_xgxs0_ctrl, 0);
    WRITE_GPORT_XGXS0_CTRL_REGr(unit, blk_idx, gport_xgxs0_ctrl);
    BMD_SYS_USLEEP(11100);

    /* Bring XGXS out of reset */
    GPORT_XGXS0_CTRL_REGr_RSTB_HWf_SET(gport_xgxs0_ctrl, 1);
    WRITE_GPORT_XGXS0_CTRL_REGr(unit, blk_idx, gport_xgxs0_ctrl);
    BMD_SYS_USLEEP(1100);

    /* Bring reference clock out of reset */
    GPORT_XGXS0_CTRL_REGr_RSTB_REFCLKf_SET(gport_xgxs0_ctrl, 1);
    WRITE_GPORT_XGXS0_CTRL_REGr(unit, blk_idx, gport_xgxs0_ctrl);

    /* Activate clocks */
    GPORT_XGXS0_CTRL_REGr_RSTB_PLLf_SET(gport_xgxs0_ctrl, 1);
    WRITE_GPORT_XGXS0_CTRL_REGr(unit, blk_idx, gport_xgxs0_ctrl);
}

static void
_tsc_xlcl_xgxs_reset(int unit, int port)
{
    XLPORT_XGXS0_CTRL_REGr_t xlport_xgxs0_ctrl;
    CLPORT_XGXS0_CTRL_REGr_t clport_xgxs0_ctrl;

    if (IS_XL(unit, port)) {
        READ_XLPORT_XGXS0_CTRL_REGr(unit, &xlport_xgxs0_ctrl, port);
        XLPORT_XGXS0_CTRL_REGr_REFIN_ENf_SET(xlport_xgxs0_ctrl, 1);
        WRITE_XLPORT_XGXS0_CTRL_REGr(unit, xlport_xgxs0_ctrl, port);

        XLPORT_XGXS0_CTRL_REGr_IDDQf_SET(xlport_xgxs0_ctrl, 0);
        /* Deassert power down */
        XLPORT_XGXS0_CTRL_REGr_PWRDWNf_SET(xlport_xgxs0_ctrl, 0);
        WRITE_XLPORT_XGXS0_CTRL_REGr(unit, xlport_xgxs0_ctrl, port);
        BMD_SYS_USLEEP(1100);

        /* Reset XGXS */
        XLPORT_XGXS0_CTRL_REGr_RSTB_HWf_SET(xlport_xgxs0_ctrl, 0);
        WRITE_XLPORT_XGXS0_CTRL_REGr(unit, xlport_xgxs0_ctrl, port);
        BMD_SYS_USLEEP(11100);

        /* Bring XGXS out of reset */
        XLPORT_XGXS0_CTRL_REGr_RSTB_HWf_SET(xlport_xgxs0_ctrl, 1);
        WRITE_XLPORT_XGXS0_CTRL_REGr(unit, xlport_xgxs0_ctrl, port);
        BMD_SYS_USLEEP(1100);
    } else if (IS_CL(unit, port)) {
        READ_CLPORT_XGXS0_CTRL_REGr(unit, &clport_xgxs0_ctrl, port);
        CLPORT_XGXS0_CTRL_REGr_REFIN_ENf_SET(clport_xgxs0_ctrl, 1);
        WRITE_CLPORT_XGXS0_CTRL_REGr(unit, clport_xgxs0_ctrl, port);

        CLPORT_XGXS0_CTRL_REGr_IDDQf_SET(clport_xgxs0_ctrl, 0);
        /* Deassert power down */
        CLPORT_XGXS0_CTRL_REGr_PWRDWNf_SET(clport_xgxs0_ctrl, 0);
        WRITE_CLPORT_XGXS0_CTRL_REGr(unit, clport_xgxs0_ctrl, port);
        BMD_SYS_USLEEP(1100);

        /* Reset XGXS */
        CLPORT_XGXS0_CTRL_REGr_RSTB_HWf_SET(clport_xgxs0_ctrl, 0);
        WRITE_CLPORT_XGXS0_CTRL_REGr(unit, clport_xgxs0_ctrl, port);
        BMD_SYS_USLEEP(11100);

        /* Bring XGXS out of reset */
        CLPORT_XGXS0_CTRL_REGr_RSTB_HWf_SET(clport_xgxs0_ctrl, 1);
        WRITE_CLPORT_XGXS0_CTRL_REGr(unit, clport_xgxs0_ctrl, port);
        BMD_SYS_USLEEP(1100);
    }
}

static void
_tsc_reset(int unit)
{
    int port;
    cdk_pbmp_t xlpbmp, clpbmp;
    XLPORT_MAC_CONTROLr_t xlp_mac;
    CLPORT_MAC_CONTROLr_t clp_mac;
    int idx;
    int blk_pmq[5] = {2, 10, 18, 26, 42};

    /* Reset SGMII4Px2 */
    for (idx = 0; idx < 5; idx++) {
        port = blk_pmq[idx];
        _sgmii4px2_reset(unit, port, 0);
        _sgmii4px2_reset(unit, port, 1);
    }

    /* Reset QTC */
    for (idx = 0; idx < 5; idx++) {
        port = blk_pmq[idx];
        if (IS_TSCQ(port)) {
            _tscq_xgxs_reset(unit, port);
        }
    }

    /* Reset TSCE */
    bcm53570_b0_xlport_pbmp_get(unit, &xlpbmp);
    CDK_PBMP_ITER(xlpbmp, port) {
        if (SUBPORT(unit, port) == 0) {
            _tsc_xlcl_xgxs_reset(unit, port);
        }
    }

    /* Reset TSCF */
    bcm53570_b0_clport_pbmp_get(unit, &clpbmp);
    CDK_PBMP_ITER(clpbmp, port) {
        if (SUBPORT(unit, port) == 0) {
            _tsc_xlcl_xgxs_reset(unit, port);
        }
    }

    CDK_PBMP_ITER(xlpbmp, port) {
        if (SUBPORT(unit, port) == 0) {
            READ_XLPORT_MAC_CONTROLr(unit, &xlp_mac, port);
            XLPORT_MAC_CONTROLr_XMAC0_RESETf_SET(xlp_mac, 1);
            WRITE_XLPORT_MAC_CONTROLr(unit, xlp_mac, port);
            BMD_SYS_USLEEP(10);

            XLPORT_MAC_CONTROLr_XMAC0_RESETf_SET(xlp_mac, 0);
            WRITE_XLPORT_MAC_CONTROLr(unit, xlp_mac, port);
        }
    }

    bcm53570_b0_clport_pbmp_get(unit, &clpbmp);
    CDK_PBMP_ITER(clpbmp, port) {
        if (SUBPORT(unit, port) == 0) {
            READ_CLPORT_MAC_CONTROLr(unit, &clp_mac, port);
            CLPORT_MAC_CONTROLr_XMAC0_RESETf_SET(clp_mac, 1);
            WRITE_CLPORT_MAC_CONTROLr(unit, clp_mac, port);
            BMD_SYS_USLEEP(10);

            CLPORT_MAC_CONTROLr_XMAC0_RESETf_SET(clp_mac, 0);
            WRITE_CLPORT_MAC_CONTROLr(unit, clp_mac, port);
        }
    }
}

int
bcm53570_b0_bmd_reset(int unit)
{
    int ioerr = 0, rv = CDK_E_NONE;
    uint32_t to_usec = 10000;
    CMIC_CPS_RESETr_t cmic_cps;
    CMIC_SBUS_TIMEOUTr_t cmic_sbus_timeout;
    CMIC_SBUS_RING_MAPr_t sbus_ring_map;
    TOP_SOFT_RESET_REGr_t top_soft_reset;
    TOP_SOFT_RESET_REG_2r_t top_soft_reset2;
    TOP_CORE_PLL_CTRL4r_t top_pll4;
    TOP_MISC_CONTROL_1r_t top_misc1;
    TOP_XG_PLL_CTRL_0r_t top_xg_pll_ctrl_0;
    TOP_XG_PLL_CTRL_6r_t top_xg_pll_ctrl_6;
    cdk_pbmp_t pbmp;
    int port;
    TOP_STRAP_STATUS_1r_t strap_status;
    PGW_CTRL_0r_t pgw_ctrl_0;
    uint32_t strap_val, disable_tsc, disable_qtc;
    uint32_t freq;
    int idx;
    uint32_t ring_map[] = { 0x11110100, 0x00430070, 0x00005064,
                            0x02220111, 0x02220222, 0x22222222 };

    /* Initialize endian mode for correct reset access */
    ioerr += cdk_xgsd_cmic_init(unit);

    /* Pull reset line */
    ioerr += READ_CMIC_CPS_RESETr(unit, &cmic_cps);
    CMIC_CPS_RESETr_CPS_RESETf_SET(cmic_cps, 1);
    ioerr += WRITE_CMIC_CPS_RESETr(unit, cmic_cps);

    /* Wait for all tables to initialize */
    BMD_SYS_USLEEP(to_usec);

    /* Re-initialize endian mode after reset */
    ioerr += cdk_xgsd_cmic_init(unit);
    for (idx = 0; idx < COUNTOF(ring_map); idx++) {
        CMIC_SBUS_RING_MAPr_SET(sbus_ring_map, ring_map[idx]);
        ioerr += WRITE_CMIC_SBUS_RING_MAPr(unit, idx, sbus_ring_map);
    }

    CMIC_SBUS_TIMEOUTr_CLR(cmic_sbus_timeout);
    CMIC_SBUS_TIMEOUTr_TIMEOUT_VALf_SET(cmic_sbus_timeout, 0x7d0);
    ioerr += WRITE_CMIC_SBUS_TIMEOUTr(unit, cmic_sbus_timeout);

    /* Skip polling TOP_XG_PLL0_STATUS since we have to keep going anyway */
    BMD_SYS_USLEEP(to_usec);

    /* Core clock PLL setting */
    switch (FREQ(unit)) {
    case 125:
        freq = 0x1c;
        break;
    case 392:
    case 500:
        freq = 0x7;
        break;
    case 450:
        freq = 0x5;
        break;
    default:
        /* FREQ_583 */
        freq = 0x6;
        break;
    }

    ioerr += READ_TOP_CORE_PLL_CTRL4r(unit, &top_pll4);
    TOP_CORE_PLL_CTRL4r_MSTR_CH0_MDIVf_SET(top_pll4, freq);
    ioerr += WRITE_TOP_CORE_PLL_CTRL4r(unit, top_pll4);

    ioerr += READ_TOP_MISC_CONTROL_1r(unit, &top_misc1);
    TOP_MISC_CONTROL_1r_CMIC_TO_CORE_PLL_LOADf_SET(top_misc1, 1);
    ioerr += WRITE_TOP_MISC_CONTROL_1r(unit, top_misc1);

    /* Pull GPIO3 low to reset the ext. PHY */
    rv = _gpio_set(unit, 3, TRUE, 0);

    ioerr += READ_TOP_XG_PLL_CTRL_0r(unit, 0, &top_xg_pll_ctrl_0);
    TOP_XG_PLL_CTRL_0r_CH5_MDIVf_SET(top_xg_pll_ctrl_0, 0x19);
    ioerr += WRITE_TOP_XG_PLL_CTRL_0r(unit, 0, top_xg_pll_ctrl_0);

    ioerr += READ_TOP_XG_PLL_CTRL_6r(unit, 0, &top_xg_pll_ctrl_6);
    TOP_XG_PLL_CTRL_6r_MSC_CTRLf_SET(top_xg_pll_ctrl_6, 0x71a2);
    ioerr += WRITE_TOP_XG_PLL_CTRL_6r(unit, 0, top_xg_pll_ctrl_6);

    /* Pull  GPIO high to leave the reset state */
    _gpio_set(unit, 3, TRUE, 1);

    /* Unused TSCx or QTCx should be disabled */
    /* disable_tsc = 0~0xff(TSCF, TSCE6,..,TSCE0, disable_qtc = qtc1,qtc0 */
    /* Base on the strap pin to disable the TSC or QTC blocks */
    ioerr += READ_TOP_STRAP_STATUS_1r(unit, &strap_status);
    strap_val = TOP_STRAP_STATUS_1r_GET(strap_status);

    disable_tsc = ((strap_val >> 2) & 0xff);
    disable_qtc = (strap_val & 0x3);
    if (disable_tsc || disable_qtc) {
        bcm53570_b0_block_disable(unit, disable_tsc, disable_qtc);
    }
    disable_tsc = 0xff;
    disable_qtc = 0x3;
    bcm53570_b0_tscq_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        disable_qtc &= ~(1 << ((port - 26) >> 4));
    }
    bcm53570_b0_xlport_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        disable_tsc &= ~(1 << ((port - 58) >> 2));
    }

    bcm53570_b0_clport_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        disable_tsc &= ~(1 << 7);
    }

    ioerr += READ_PGW_CTRL_0r(unit, &pgw_ctrl_0);
    PGW_CTRL_0r_SW_PM4X10_DISABLEf_SET(pgw_ctrl_0, disable_tsc & 0x7f);
    PGW_CTRL_0r_SW_QTC_DISABLEf_SET(pgw_ctrl_0, disable_qtc);
    PGW_CTRL_0r_SW_PM4X25_DISABLEf_SET(pgw_ctrl_0, (disable_tsc >> 7) & 0x1);
    ioerr += WRITE_PGW_CTRL_0r(unit, pgw_ctrl_0);
    BMD_SYS_USLEEP(to_usec);

    /*
     * Bring port blocks out of reset
     */
    ioerr += READ_TOP_SOFT_RESET_REGr(unit,  &top_soft_reset);
    TOP_SOFT_RESET_REGr_TOP_GE8P_RST_Lf_SET(top_soft_reset, 0x7);
    TOP_SOFT_RESET_REGr_TOP_GEP0_RST_Lf_SET(top_soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_GEP1_RST_Lf_SET(top_soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_XLP0_RST_Lf_SET(top_soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_XLP1_RST_Lf_SET(top_soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_XLP2_RST_Lf_SET(top_soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_XLP3_RST_Lf_SET(top_soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_XLP4_RST_Lf_SET(top_soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_XLP5_RST_Lf_SET(top_soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_XLP6_RST_Lf_SET(top_soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_CLP0_RST_Lf_SET(top_soft_reset, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REGr(unit, top_soft_reset);
    BMD_SYS_USLEEP(to_usec);

    /* Bring network sync out of reset */
    ioerr += READ_TOP_SOFT_RESET_REGr(unit,  &top_soft_reset);
    TOP_SOFT_RESET_REGr_TOP_TS_RST_Lf_SET(top_soft_reset,1);
    ioerr += WRITE_TOP_SOFT_RESET_REGr(unit, top_soft_reset);
    BMD_SYS_USLEEP(to_usec);

    ioerr += READ_TOP_SOFT_RESET_REG_2r(unit, &top_soft_reset2);
    TOP_SOFT_RESET_REG_2r_TOP_TS_PLL_RST_Lf_SET(top_soft_reset2, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REG_2r(unit, top_soft_reset2);
    TOP_SOFT_RESET_REG_2r_TOP_TS_PLL_POST_RST_Lf_SET(top_soft_reset2, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REG_2r(unit, top_soft_reset2);

    /* Bring IP, EP, and MMU blocks out of reset */
    ioerr += READ_TOP_SOFT_RESET_REGr(unit, &top_soft_reset);
    TOP_SOFT_RESET_REGr_TOP_EP_RST_Lf_SET(top_soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_IP_RST_Lf_SET(top_soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_MMU_RST_Lf_SET(top_soft_reset, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REGr(unit, top_soft_reset);
    BMD_SYS_USLEEP(to_usec);

    _tsc_reset(unit);

    return ioerr ? CDK_E_IO : rv;
}

#endif /* CDK_CONFIG_INCLUDE_BCM53570_B0 */
