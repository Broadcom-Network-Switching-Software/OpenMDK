/*
 *
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56270_A0 == 1

#include <bmd/bmd.h>

#include <cdk/arch/xgsd_chip.h>
#include <cdk/cdk_debug.h>
#include <cdk/arch/xgsd_miim.h>
#include <cdk/chip/bcm56270_a0_defs.h>

#include "bcm56270_a0_bmd.h"
#include "bcm56270_a0_internal.h"

#define PIPE_RESET_TIMEOUT_MSEC         50

static void
soc_xgxs_reset(int unit, int port)
{
    int ioerr = 0;
    cdk_pbmp_t pbmp;
    XPORT_XGXS_CTRLr_t xport_xgxs_ctrl;
    XLPORT_XGXS0_CTRL_REGr_t xlport_ctrl;

    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_MXQPORT, &pbmp);
    if (CDK_PBMP_MEMBER(pbmp, port) && (MXQPORT_SUBPORT(unit, port) == 0)) {
        /*
        * XGXS MAC initialization steps.
        *
        * A minimum delay is required between various initialization steps.
        * There is no maximum delay.  The values given are very conservative
        * including the timeout for PLL lock.
        */
        /* Release reset (if asserted) to allow bigmac to initialize */
        ioerr += READ_XPORT_XGXS_CTRLr(unit, &xport_xgxs_ctrl, port);
        XPORT_XGXS_CTRLr_IDDQf_SET(xport_xgxs_ctrl, 0);
        XPORT_XGXS_CTRLr_PWRDWNf_SET(xport_xgxs_ctrl, 0);
        XPORT_XGXS_CTRLr_RSTB_HWf_SET(xport_xgxs_ctrl, 1);
        ioerr += WRITE_XPORT_XGXS_CTRLr(unit, xport_xgxs_ctrl, port);
        BMD_SYS_USLEEP(1100);

        /* Power down and reset */
        ioerr += READ_XPORT_XGXS_CTRLr(unit, &xport_xgxs_ctrl, port);
        XPORT_XGXS_CTRLr_PWRDWNf_SET(xport_xgxs_ctrl, 1);
        XPORT_XGXS_CTRLr_IDDQf_SET(xport_xgxs_ctrl, 1);
        XPORT_XGXS_CTRLr_RSTB_HWf_SET(xport_xgxs_ctrl, 0);
        XPORT_XGXS_CTRLr_TXD1G_FIFO_RSTBf_SET(xport_xgxs_ctrl, 0);
        XPORT_XGXS_CTRLr_TXD10G_FIFO_RSTBf_SET(xport_xgxs_ctrl, 0);
        XPORT_XGXS_CTRLr_RSTB_MDIOREGSf_SET(xport_xgxs_ctrl, 0);
        XPORT_XGXS_CTRLr_RSTB_PLLf_SET(xport_xgxs_ctrl, 0);
        ioerr += WRITE_XPORT_XGXS_CTRLr(unit, xport_xgxs_ctrl, port);
        BMD_SYS_USLEEP(1100);

        /*
        * Bring up both digital and analog clocks
        *
        * NOTE: Many MAC registers are not accessible until the PLL is locked.
        * An S-Channel timeout will occur before that.
        */
        ioerr += READ_XPORT_XGXS_CTRLr(unit, &xport_xgxs_ctrl, port);
        XPORT_XGXS_CTRLr_PWRDWNf_SET(xport_xgxs_ctrl, 0);
        XPORT_XGXS_CTRLr_IDDQf_SET(xport_xgxs_ctrl, 0);
        ioerr += WRITE_XPORT_XGXS_CTRLr(unit, xport_xgxs_ctrl, port);
        BMD_SYS_USLEEP(1100);

        /* Bring XGXS out of reset, AFIFO_RST stays 1.  */
        ioerr += READ_XPORT_XGXS_CTRLr(unit, &xport_xgxs_ctrl, port);
        XPORT_XGXS_CTRLr_RSTB_HWf_SET(xport_xgxs_ctrl, 1);
        ioerr += WRITE_XPORT_XGXS_CTRLr(unit, xport_xgxs_ctrl, port);
        BMD_SYS_USLEEP(1100);

        /* Bring MDIO registers out of reset */
        ioerr += READ_XPORT_XGXS_CTRLr(unit, &xport_xgxs_ctrl, port);
        XPORT_XGXS_CTRLr_RSTB_MDIOREGSf_SET(xport_xgxs_ctrl, 1);
        ioerr += WRITE_XPORT_XGXS_CTRLr(unit, xport_xgxs_ctrl, port);

        /* Activate all clocks */
        ioerr += READ_XPORT_XGXS_CTRLr(unit, &xport_xgxs_ctrl, port);
        XPORT_XGXS_CTRLr_RSTB_PLLf_SET(xport_xgxs_ctrl, 1);
        ioerr += WRITE_XPORT_XGXS_CTRLr(unit, xport_xgxs_ctrl, port);

        /* Bring Tx FIFO out of reset */
        ioerr += READ_XPORT_XGXS_CTRLr(unit, &xport_xgxs_ctrl, port);
        XPORT_XGXS_CTRLr_TXD1G_FIFO_RSTBf_SET(xport_xgxs_ctrl, 0xf);
        XPORT_XGXS_CTRLr_TXD10G_FIFO_RSTBf_SET(xport_xgxs_ctrl, 1);
        ioerr += WRITE_XPORT_XGXS_CTRLr(unit, xport_xgxs_ctrl, port);
    }

    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_XLPORT, &pbmp);
    if (CDK_PBMP_MEMBER(pbmp, port) && (XLPORT_SUBPORT(unit, port) == 0)) {
        /*
        * XGXS MAC initialization steps.
        *
        * A minimum delay is required between various initialization steps.
        * There is no maximum delay.  The values given are very conservative
        * including the timeout for PLL lock.
        */
        /* Release reset (if asserted) to allow bigmac to initialize */
        ioerr += READ_XLPORT_XGXS0_CTRL_REGr(unit, &xlport_ctrl, port);
        XLPORT_XGXS0_CTRL_REGr_IDDQf_SET(xlport_ctrl, 0);
        XLPORT_XGXS0_CTRL_REGr_PWRDWNf_SET(xlport_ctrl, 0);
        XLPORT_XGXS0_CTRL_REGr_RSTB_HWf_SET(xlport_ctrl, 1);
        ioerr += WRITE_XLPORT_XGXS0_CTRL_REGr(unit, xlport_ctrl, port);
        BMD_SYS_USLEEP(1100);

        ioerr += READ_XLPORT_XGXS0_CTRL_REGr(unit, &xlport_ctrl, port);
        XLPORT_XGXS0_CTRL_REGr_PWRDWNf_SET(xlport_ctrl, 1);
        XLPORT_XGXS0_CTRL_REGr_IDDQf_SET(xlport_ctrl, 1);
        XLPORT_XGXS0_CTRL_REGr_RSTB_HWf_SET(xlport_ctrl, 0);
        ioerr += WRITE_XLPORT_XGXS0_CTRL_REGr(unit, xlport_ctrl, port);
        BMD_SYS_USLEEP(1100);

        ioerr += READ_XLPORT_XGXS0_CTRL_REGr(unit, &xlport_ctrl, port);
        XLPORT_XGXS0_CTRL_REGr_PWRDWNf_SET(xlport_ctrl, 0);
        XLPORT_XGXS0_CTRL_REGr_IDDQf_SET(xlport_ctrl, 0);
        ioerr += WRITE_XLPORT_XGXS0_CTRL_REGr(unit, xlport_ctrl, port);
        BMD_SYS_USLEEP(1100);

        ioerr += READ_XLPORT_XGXS0_CTRL_REGr(unit, &xlport_ctrl, port);
        XLPORT_XGXS0_CTRL_REGr_RSTB_HWf_SET(xlport_ctrl, 1);
        ioerr += WRITE_XLPORT_XGXS0_CTRL_REGr(unit, xlport_ctrl, port);
        BMD_SYS_USLEEP(1100);
    }
}

int
bcm56270_a0_bmd_reset(int unit)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int wait_usec = 10000;
    int port;
    TOP_SOFT_RESET_REG_2r_t soft_reset_2;
    TOP_SOFT_RESET_REG_3r_t soft_reset_3;
    int idx;
    CMIC_SBUS_TIMEOUTr_t cmic_sbus_timeout;
    CMIC_CPS_RESETr_t cmic_cps;
    CMIC_SBUS_RING_MAPr_t sbus_ring_map;
    uint32_t ring_map[] = { 0x62034000, 0x60377531, 0x00000031,
                            0x00000000, 0x00000000, 0x00000000,
                            0x00000000, 0x00000000};
    TOP_XGXS_PLL_CONTROL_1r_t top_xgxs_pll_ctrl_1;
    TOP_XGXS_PLL_CONTROL_2r_t top_xgxs_pll_ctrl_2;
    TOP_XGXS_PLL_CONTROL_3r_t top_xgxs_pll_ctrl_3;
    TOP_XGXS_PLL_CONTROL_4r_t top_xgxs_pll_ctrl_4;
    TOP_XGXS_PLL_CONTROL_6r_t top_xgxs_pll_ctrl_6;
    TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_3r_t top_bs0_pll_ctrl3;
    TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_4r_t top_bs0_pll_ctrl4;
    TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_6r_t top_bs0_pll_ctrl6;
    TOP_BROAD_SYNC1_PLL_CTRL_REGISTER_3r_t top_bs1_pll_ctrl3;
    TOP_BROAD_SYNC1_PLL_CTRL_REGISTER_6r_t top_bs1_pll_ctrl6;
    TOP_LCPLL_SOFT_RESET_REGr_t top_lcpll_soft_reset_reg;
    TOP_BROAD_SYNC1_PLL_CTRL_REGISTER_4r_t top_bs1_pll_ctrl4;
    TOP_MASTER_LCPLL_FBDIV_CTRLr_t top_master_lcpll_fbdiv;
    TOP_XGXS_PLL_STATUSr_t top_xgxs_pll_status;
    TOP_SOFT_RESET_REGr_t soft_reset;
    ING_HW_RESET_CONTROL_1r_t ing_hw_rst_ctrl_1;
    EGR_HW_RESET_CONTROL_0r_t egr_hw_rst_ctrl_0;
    ING_HW_RESET_CONTROL_2r_t ing_hw_rst_ctrl_2;
    EGR_HW_RESET_CONTROL_1r_t egr_hw_rst_ctrl_1;
    TOP_TIME_SYNC_PLL_CTRL_REGISTER_1r_t top_tspll_ctl1;
    TOP_TIME_SYNC_PLL_CTRL_REGISTER_2r_t top_tspll_ctl2;
    TOP_TIME_SYNC_PLL_CTRL_REGISTER_3r_t top_tspll_ctl3;
    TOP_TIME_SYNC_PLL_CTRL_REGISTER_5r_t top_tspll_ctl5;
    CMIC_BS0_CONFIGr_t cmic_bs0_cfg;
    CMIC_BS1_CONFIGr_t cmic_bs1_cfg;
    TOP_BROAD_SYNC0_LCPLL_FBDIV_CTRLr_t top_bs0_lcpll_fbdiv;
    TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_1r_t top_bs0_pll_ctrl1;
    TOP_BROAD_SYNC1_LCPLL_FBDIV_CTRLr_t top_bs1_lcpll_fbdiv;
    TOP_BROAD_SYNC1_PLL_CTRL_REGISTER_1r_t top_bs1_pll_ctrl1;
    XLPORT_MAC_CONTROLr_t xlport_mac_ctrl;
    cdk_pbmp_t pbmp;
    XPORT_XMAC_CONTROLr_t xport_xmac_ctrl;

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

    /* Master LCPLL */
    TOP_XGXS_PLL_CONTROL_1r_SET(top_xgxs_pll_ctrl_1, 0x197d2014);
    ioerr += WRITE_TOP_XGXS_PLL_CONTROL_1r(unit, 0, top_xgxs_pll_ctrl_1);

    TOP_XGXS_PLL_CONTROL_2r_SET(top_xgxs_pll_ctrl_2, 0x40004015);
    ioerr += WRITE_TOP_XGXS_PLL_CONTROL_2r(unit, 0, top_xgxs_pll_ctrl_2);

    TOP_XGXS_PLL_CONTROL_3r_SET(top_xgxs_pll_ctrl_3, 0x897004);
    ioerr += WRITE_TOP_XGXS_PLL_CONTROL_3r(unit, 0, top_xgxs_pll_ctrl_3);

    ioerr += READ_TOP_XGXS_PLL_CONTROL_4r(unit, 0, &top_xgxs_pll_ctrl_4);
    TOP_XGXS_PLL_CONTROL_4r_CP1f_SET(top_xgxs_pll_ctrl_4, 0x1);
    TOP_XGXS_PLL_CONTROL_4r_CPf_SET(top_xgxs_pll_ctrl_4, 0x3);
    TOP_XGXS_PLL_CONTROL_4r_CZf_SET(top_xgxs_pll_ctrl_4, 0x3);
    TOP_XGXS_PLL_CONTROL_4r_RPf_SET(top_xgxs_pll_ctrl_4, 0x3);
    TOP_XGXS_PLL_CONTROL_4r_RZf_SET(top_xgxs_pll_ctrl_4, 0);
    TOP_XGXS_PLL_CONTROL_4r_ICPf_SET(top_xgxs_pll_ctrl_4, 0xA);
    TOP_XGXS_PLL_CONTROL_4r_VCO_GAINf_SET(top_xgxs_pll_ctrl_4, 0xF);
    ioerr += WRITE_TOP_XGXS_PLL_CONTROL_4r(unit, 0, top_xgxs_pll_ctrl_4);

    ioerr += READ_TOP_XGXS_PLL_CONTROL_6r(unit, 0, &top_xgxs_pll_ctrl_6);
    TOP_XGXS_PLL_CONTROL_6r_CH2_MDIVf_SET(top_xgxs_pll_ctrl_6, 0x7);
    TOP_XGXS_PLL_CONTROL_6r_PDIVf_SET(top_xgxs_pll_ctrl_6, 0x1);
    ioerr += WRITE_TOP_XGXS_PLL_CONTROL_6r(unit, 0, top_xgxs_pll_ctrl_6);

    /* Broad Sync0 LCPLL */
    TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_3r_SET(top_bs0_pll_ctrl3, 0x00805004);
    ioerr += WRITE_TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_3r(unit, top_bs0_pll_ctrl3);

    ioerr += READ_TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_6r(unit, &top_bs0_pll_ctrl6);
    TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_6r_LDO_CTRLf_SET(top_bs0_pll_ctrl6, 0x22);
    ioerr += WRITE_TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_6r(unit, top_bs0_pll_ctrl6);

    READ_TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_4r(unit, &top_bs0_pll_ctrl4);
    TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_4r_CP1f_SET(top_bs0_pll_ctrl4, 0x1);
    TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_4r_CPf_SET(top_bs0_pll_ctrl4, 0x3);
    TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_4r_CZf_SET(top_bs0_pll_ctrl4, 0x3);
    TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_4r_RPf_SET(top_bs0_pll_ctrl4, 0x7);
    TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_4r_RZf_SET(top_bs0_pll_ctrl4, 0x2);
    TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_4r_ICPf_SET(top_bs0_pll_ctrl4, 0xA);
    TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_4r_VCO_GAINf_SET(top_bs0_pll_ctrl4, 0xF);
    WRITE_TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_4r(unit, top_bs0_pll_ctrl4);

    /* Broad Sync1 LCPLL */
    TOP_BROAD_SYNC1_PLL_CTRL_REGISTER_3r_SET(top_bs1_pll_ctrl3, 0x00805004);
    ioerr += WRITE_TOP_BROAD_SYNC1_PLL_CTRL_REGISTER_3r(unit, top_bs1_pll_ctrl3);

    ioerr += READ_TOP_BROAD_SYNC1_PLL_CTRL_REGISTER_6r(unit, &top_bs1_pll_ctrl6);
    TOP_BROAD_SYNC1_PLL_CTRL_REGISTER_6r_LDO_CTRLf_SET(top_bs1_pll_ctrl6, 0x22);
    ioerr += WRITE_TOP_BROAD_SYNC1_PLL_CTRL_REGISTER_6r(unit, top_bs1_pll_ctrl6);

    READ_TOP_BROAD_SYNC1_PLL_CTRL_REGISTER_4r(unit, &top_bs1_pll_ctrl4);
    TOP_BROAD_SYNC1_PLL_CTRL_REGISTER_4r_CP1f_SET(top_bs1_pll_ctrl4, 0x1);
    TOP_BROAD_SYNC1_PLL_CTRL_REGISTER_4r_CPf_SET(top_bs1_pll_ctrl4, 0x3);
    TOP_BROAD_SYNC1_PLL_CTRL_REGISTER_4r_CZf_SET(top_bs1_pll_ctrl4, 0x3);
    TOP_BROAD_SYNC1_PLL_CTRL_REGISTER_4r_RPf_SET(top_bs1_pll_ctrl4, 0x7);
    TOP_BROAD_SYNC1_PLL_CTRL_REGISTER_4r_RZf_SET(top_bs1_pll_ctrl4, 0x2);
    TOP_BROAD_SYNC1_PLL_CTRL_REGISTER_4r_ICPf_SET(top_bs1_pll_ctrl4, 0xA);
    TOP_BROAD_SYNC1_PLL_CTRL_REGISTER_4r_VCO_GAINf_SET(top_bs1_pll_ctrl4, 0xF);
    WRITE_TOP_BROAD_SYNC1_PLL_CTRL_REGISTER_4r(unit, top_bs1_pll_ctrl4);

    /* Top Master LCPLL settings */
    TOP_MASTER_LCPLL_FBDIV_CTRLr_SET(top_master_lcpll_fbdiv, 0x7d0);
    ioerr += WRITE_TOP_MASTER_LCPLL_FBDIV_CTRLr(unit, 1, top_master_lcpll_fbdiv);

    /* End of PLL settings. Togle LCPLL soft reset. */
    READ_TOP_LCPLL_SOFT_RESET_REGr(unit, &top_lcpll_soft_reset_reg);
    TOP_LCPLL_SOFT_RESET_REGr_LCPLL_SOFT_RESETf_SET(top_lcpll_soft_reset_reg, 0x1);
    WRITE_TOP_LCPLL_SOFT_RESET_REGr(unit, top_lcpll_soft_reset_reg);
    BMD_SYS_USLEEP(wait_usec);

    TOP_LCPLL_SOFT_RESET_REGr_LCPLL_SOFT_RESETf_SET(top_lcpll_soft_reset_reg, 0x0);
    WRITE_TOP_LCPLL_SOFT_RESET_REGr(unit, top_lcpll_soft_reset_reg);
    BMD_SYS_USLEEP(wait_usec);

    /* Wait for LCPLL lock */
    READ_TOP_XGXS_PLL_STATUSr(unit, 0, &top_xgxs_pll_status);
    if (!(TOP_XGXS_PLL_STATUSr_TOP_XGPLL0_LOCKf_GET(top_xgxs_pll_status))) {
        CDK_PRINTF("LCPLL 0 not locked");
    }

    /* top_soft_reset_reg */
    /* Bring IP, EP, and MMU blocks out of reset */
    ioerr += READ_TOP_SOFT_RESET_REGr(unit, &soft_reset);
    TOP_SOFT_RESET_REGr_TOP_ARS_RST_Lf_SET(soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_AVS_RST_Lf_SET(soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_NS_RST_Lf_SET(soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_EP_RST_Lf_SET(soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_IP_RST_Lf_SET(soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_MMU_RST_Lf_SET(soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_MXQ_HOTSWAP_RST_Lf_SET(soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_PM1_RST_Lf_SET(soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_PM0_RST_Lf_SET(soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_MXQ_RST_Lf_SET(soft_reset, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REGr(unit, soft_reset);

    /* IPIPE and EPIPE configurations */
    /* Reset IPITE and EPIPE block */
    ING_HW_RESET_CONTROL_1r_CLR(ing_hw_rst_ctrl_1);
    ioerr += WRITE_ING_HW_RESET_CONTROL_1r(unit, ing_hw_rst_ctrl_1);

    ING_HW_RESET_CONTROL_2r_CLR(ing_hw_rst_ctrl_2);
    ING_HW_RESET_CONTROL_2r_RESET_ALLf_SET(ing_hw_rst_ctrl_2, 1);
    ING_HW_RESET_CONTROL_2r_VALIDf_SET(ing_hw_rst_ctrl_2, 1);
    ING_HW_RESET_CONTROL_2r_COUNTf_SET(ing_hw_rst_ctrl_2, 0x4000);
    ioerr += WRITE_ING_HW_RESET_CONTROL_2r(unit, ing_hw_rst_ctrl_2);

    EGR_HW_RESET_CONTROL_0r_CLR(egr_hw_rst_ctrl_0);
    ioerr += WRITE_EGR_HW_RESET_CONTROL_0r(unit, egr_hw_rst_ctrl_0);

    ioerr += READ_EGR_HW_RESET_CONTROL_1r (unit, &egr_hw_rst_ctrl_1);
    EGR_HW_RESET_CONTROL_1r_RESET_ALLf_SET(egr_hw_rst_ctrl_1, 1);
    EGR_HW_RESET_CONTROL_1r_VALIDf_SET(egr_hw_rst_ctrl_1, 1);
    /* 'EGR_VLAN_XLATE' initialization need to clear one more entry than
    *   its max entry number(0x4000).
    */
    EGR_HW_RESET_CONTROL_1r_COUNTf_SET(egr_hw_rst_ctrl_1, 0x4001);
    ioerr += WRITE_EGR_HW_RESET_CONTROL_1r(unit, egr_hw_rst_ctrl_1);
    for (idx = 0; idx < PIPE_RESET_TIMEOUT_MSEC; idx++) {
        ioerr += READ_ING_HW_RESET_CONTROL_2r(unit, &ing_hw_rst_ctrl_2);
        if (ING_HW_RESET_CONTROL_2r_DONEf_GET(ing_hw_rst_ctrl_2)) {
            break;
        }
        BMD_SYS_USLEEP(1000);
    }
    if (idx >= PIPE_RESET_TIMEOUT_MSEC) {
        CDK_WARN(("bcm56270_a0_bmd_reset[%d]: IPIPE reset timeout\n", unit));
        return ioerr ? CDK_E_IO : CDK_E_TIMEOUT;
    }

    for (idx = 0; idx < PIPE_RESET_TIMEOUT_MSEC; idx++) {
        ioerr += READ_EGR_HW_RESET_CONTROL_1r(unit, &egr_hw_rst_ctrl_1);
        if (EGR_HW_RESET_CONTROL_1r_DONEf_GET(egr_hw_rst_ctrl_1)) {
            break;
        }
        BMD_SYS_USLEEP(1000);
    }
    if (idx >= PIPE_RESET_TIMEOUT_MSEC) {
        CDK_WARN(("bcm56270_a0_bmd_reset[%d]: EPIPE reset timeout\n", unit));
        return ioerr ? CDK_E_IO : CDK_E_TIMEOUT;
    }

    ioerr += READ_ING_HW_RESET_CONTROL_2r(unit, &ing_hw_rst_ctrl_2);
    ING_HW_RESET_CONTROL_2r_RESET_ALLf_SET(ing_hw_rst_ctrl_2, 0);
    ING_HW_RESET_CONTROL_2r_CMIC_REQ_ENABLEf_SET(ing_hw_rst_ctrl_2, 1);
    ioerr += WRITE_ING_HW_RESET_CONTROL_2r(unit, ing_hw_rst_ctrl_2);

    ioerr += READ_EGR_HW_RESET_CONTROL_1r(unit, &egr_hw_rst_ctrl_1);
    EGR_HW_RESET_CONTROL_1r_RESET_ALLf_SET(egr_hw_rst_ctrl_1, 0);
    EGR_HW_RESET_CONTROL_1r_DONEf_SET(egr_hw_rst_ctrl_1, 0);
    ioerr += WRITE_EGR_HW_RESET_CONTROL_1r(unit, egr_hw_rst_ctrl_1);

    /* Metrolite TS and BS PLL configurations */
    /* Put TS, BS PLLs to reset before changing PLL control registers */
    ioerr += READ_TOP_SOFT_RESET_REG_2r(unit, &soft_reset_2);
    TOP_SOFT_RESET_REG_2r_TOP_TS_PLL_RST_Lf_SET(soft_reset_2, 0);
    TOP_SOFT_RESET_REG_2r_TOP_TS_PLL_POST_RST_Lf_SET(soft_reset_2, 0);
    ioerr += WRITE_TOP_SOFT_RESET_REG_2r(unit, soft_reset_2);

    ioerr += READ_TOP_SOFT_RESET_REG_3r(unit, &soft_reset_3);
    TOP_SOFT_RESET_REG_3r_TOP_BS0_PLL_RST_Lf_SET(soft_reset_3, 0);
    TOP_SOFT_RESET_REG_3r_TOP_BS1_PLL_RST_Lf_SET(soft_reset_3, 0);
    TOP_SOFT_RESET_REG_3r_TOP_BS0_PLL_POST_RST_Lf_SET(soft_reset_3, 0);
    TOP_SOFT_RESET_REG_3r_TOP_BS1_PLL_POST_RST_Lf_SET(soft_reset_3, 0);
    ioerr += WRITE_TOP_SOFT_RESET_REG_3r(unit, soft_reset_3);

    /* TS PLL */
    ioerr += READ_TOP_TIME_SYNC_PLL_CTRL_REGISTER_1r(unit, &top_tspll_ctl1);
    TOP_TIME_SYNC_PLL_CTRL_REGISTER_1r_FREF_SELf_SET(top_tspll_ctl1, 0);
    ioerr += WRITE_TOP_TIME_SYNC_PLL_CTRL_REGISTER_1r(unit, top_tspll_ctl1);

    ioerr += READ_TOP_TIME_SYNC_PLL_CTRL_REGISTER_2r(unit,&top_tspll_ctl2);
    TOP_TIME_SYNC_PLL_CTRL_REGISTER_2r_KAf_SET(top_tspll_ctl2, 0);
    TOP_TIME_SYNC_PLL_CTRL_REGISTER_2r_KIf_SET(top_tspll_ctl2, 2);
    TOP_TIME_SYNC_PLL_CTRL_REGISTER_2r_KPf_SET(top_tspll_ctl2, 3);
    TOP_TIME_SYNC_PLL_CTRL_REGISTER_2r_NDIV_FRACf_SET(top_tspll_ctl2, 0);
    ioerr += WRITE_TOP_TIME_SYNC_PLL_CTRL_REGISTER_2r(unit, top_tspll_ctl2);

    ioerr += READ_TOP_TIME_SYNC_PLL_CTRL_REGISTER_3r(unit, &top_tspll_ctl3);
    TOP_TIME_SYNC_PLL_CTRL_REGISTER_3r_NDIV_INTf_SET(top_tspll_ctl3, 70);
    ioerr += WRITE_TOP_TIME_SYNC_PLL_CTRL_REGISTER_3r(unit, top_tspll_ctl3);

    ioerr += READ_TOP_TIME_SYNC_PLL_CTRL_REGISTER_5r(unit, &top_tspll_ctl5);
    TOP_TIME_SYNC_PLL_CTRL_REGISTER_5r_PDIVf_SET(top_tspll_ctl5, 1);
    TOP_TIME_SYNC_PLL_CTRL_REGISTER_5r_CH0_MDIVf_SET(top_tspll_ctl5, 7);
    TOP_TIME_SYNC_PLL_CTRL_REGISTER_5r_CH1_MDIVf_SET(top_tspll_ctl5, 7);
    ioerr += WRITE_TOP_TIME_SYNC_PLL_CTRL_REGISTER_5r(unit, top_tspll_ctl5);

    /* Configure Broadsync PLL's */
    /* Both BSPLLs configured the same, for 20MHz output by default */
    TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_6r_SET(top_bs0_pll_ctrl6, 0xc0200022);
    ioerr += WRITE_TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_6r(unit, top_bs0_pll_ctrl6);

    TOP_BROAD_SYNC1_PLL_CTRL_REGISTER_6r_SET(top_bs1_pll_ctrl6, 0xc0200022);
    ioerr += WRITE_TOP_BROAD_SYNC1_PLL_CTRL_REGISTER_6r(unit, top_bs1_pll_ctrl6);

    /* as a signal to upper-level code that the BroadSync is newly initialized
     * disable BroadSync0/1 bitclock output.  Checked in time.c / 1588 firmware
     */
    ioerr += READ_CMIC_BS0_CONFIGr(unit, &cmic_bs0_cfg);
    CMIC_BS0_CONFIGr_BS_CLK_OUTPUT_ENABLEf_SET(cmic_bs0_cfg, 0);
    ioerr += WRITE_CMIC_BS0_CONFIGr(unit, cmic_bs0_cfg);

    ioerr += READ_CMIC_BS1_CONFIGr(unit, &cmic_bs1_cfg);
    CMIC_BS1_CONFIGr_BS_CLK_OUTPUT_ENABLEf_SET(cmic_bs1_cfg, 0);
    ioerr += WRITE_CMIC_BS1_CONFIGr(unit, cmic_bs1_cfg);

    /* bs pll - 0 configuration */
    ioerr += READ_TOP_BROAD_SYNC0_LCPLL_FBDIV_CTRLr(unit,1, &top_bs0_lcpll_fbdiv);
    TOP_BROAD_SYNC0_LCPLL_FBDIV_CTRLr_BROAD_SYNC0_LCPLL_FBDIV_0f_SET
                                                (top_bs0_lcpll_fbdiv, 0xf80);
    WRITE_TOP_BROAD_SYNC0_LCPLL_FBDIV_CTRLr(unit, 1, top_bs0_lcpll_fbdiv);

    ioerr += READ_TOP_BROAD_SYNC0_LCPLL_FBDIV_CTRLr(unit, 0, &top_bs0_lcpll_fbdiv);
    TOP_BROAD_SYNC0_LCPLL_FBDIV_CTRLr_BROAD_SYNC0_LCPLL_FBDIV_0f_SET
                                                (top_bs0_lcpll_fbdiv, 0);
    ioerr += WRITE_TOP_BROAD_SYNC0_LCPLL_FBDIV_CTRLr(unit, 0, top_bs0_lcpll_fbdiv);

    /* for BS , use channel 0 */
    ioerr += READ_TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_1r(unit,&top_bs0_pll_ctrl1);
    TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_1r_CH1_MDIVf_SET(top_bs0_pll_ctrl1, 155);
    ioerr += WRITE_TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_1r(unit, top_bs0_pll_ctrl1);

    /* bs pll - 1 configuration */
    ioerr += READ_TOP_BROAD_SYNC1_LCPLL_FBDIV_CTRLr(unit, 1, &top_bs1_lcpll_fbdiv);
    TOP_BROAD_SYNC1_LCPLL_FBDIV_CTRLr_BROAD_SYNC1_LCPLL_FBDIV_0f_SET
                                                (top_bs1_lcpll_fbdiv, 0xf80);
    ioerr += WRITE_TOP_BROAD_SYNC1_LCPLL_FBDIV_CTRLr(unit, 1, top_bs1_lcpll_fbdiv);

    ioerr += READ_TOP_BROAD_SYNC1_LCPLL_FBDIV_CTRLr(unit, 0, &top_bs1_lcpll_fbdiv);
    TOP_BROAD_SYNC1_LCPLL_FBDIV_CTRLr_BROAD_SYNC1_LCPLL_FBDIV_0f_SET
                                                (top_bs1_lcpll_fbdiv, 0);
    ioerr += WRITE_TOP_BROAD_SYNC1_LCPLL_FBDIV_CTRLr(unit, 0, top_bs1_lcpll_fbdiv);

    ioerr += READ_TOP_BROAD_SYNC1_PLL_CTRL_REGISTER_1r(unit,&top_bs1_pll_ctrl1);
    TOP_BROAD_SYNC1_PLL_CTRL_REGISTER_1r_CH1_MDIVf_SET(top_bs1_pll_ctrl1, 155);
    ioerr += WRITE_TOP_BROAD_SYNC1_PLL_CTRL_REGISTER_1r(unit, top_bs1_pll_ctrl1);
    BMD_SYS_USLEEP(wait_usec);

    ioerr += READ_TOP_SOFT_RESET_REG_2r(unit, &soft_reset_2);
    TOP_SOFT_RESET_REG_2r_TOP_TS_PLL_RST_Lf_SET(soft_reset_2, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REG_2r(unit, soft_reset_2);
    BMD_SYS_USLEEP(wait_usec);

    ioerr += READ_TOP_SOFT_RESET_REG_2r(unit, &soft_reset_2);
    TOP_SOFT_RESET_REG_2r_TOP_TS_PLL_POST_RST_Lf_SET(soft_reset_2, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REG_2r(unit, soft_reset_2);

    /* Bring BS PLL out of reset */
    ioerr += READ_TOP_SOFT_RESET_REG_3r(unit, &soft_reset_3);
    TOP_SOFT_RESET_REG_3r_TOP_BS0_PLL_RST_Lf_SET(soft_reset_3, 1);
    TOP_SOFT_RESET_REG_3r_TOP_BS1_PLL_RST_Lf_SET(soft_reset_3, 1);
    TOP_SOFT_RESET_REG_3r_TOP_BS0_PLL_POST_RST_Lf_SET(soft_reset_3, 1);
    TOP_SOFT_RESET_REG_3r_TOP_BS1_PLL_POST_RST_Lf_SET(soft_reset_3, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REG_3r(unit, soft_reset_3);
    BMD_SYS_USLEEP(wait_usec);

    /* MAC reset */
    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_MXQPORT, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        if (MXQPORT_SUBPORT(unit, port) == 0) {
            soc_xgxs_reset(unit, port);

            ioerr += READ_XPORT_XMAC_CONTROLr(unit, &xport_xmac_ctrl, port);
            XPORT_XMAC_CONTROLr_XMAC_RESETf_SET(xport_xmac_ctrl, 1);
            ioerr += WRITE_XPORT_XMAC_CONTROLr(unit, xport_xmac_ctrl, port);
            BMD_SYS_USLEEP(10);

            XPORT_XMAC_CONTROLr_XMAC_RESETf_SET(xport_xmac_ctrl, 0);
            ioerr += WRITE_XPORT_XMAC_CONTROLr(unit, xport_xmac_ctrl, port);
        }
    }

    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_XLPORT, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        if (XLPORT_SUBPORT(unit, port) == 0) {
            ioerr += READ_XLPORT_MAC_CONTROLr(unit, &xlport_mac_ctrl, port);
            XLPORT_MAC_CONTROLr_XMAC0_RESETf_SET(xlport_mac_ctrl, 1);
            ioerr += WRITE_XLPORT_MAC_CONTROLr(unit, xlport_mac_ctrl, port);
            BMD_SYS_USLEEP(10);

            XLPORT_MAC_CONTROLr_XMAC0_RESETf_SET(xlport_mac_ctrl, 0);
            ioerr += WRITE_XLPORT_MAC_CONTROLr(unit, xlport_mac_ctrl, port);

            soc_xgxs_reset(unit, port);
        }
    }

    return ioerr ? CDK_E_IO : rv;
}

#endif /* CDK_CONFIG_INCLUDE_BCM56270_A0 */
