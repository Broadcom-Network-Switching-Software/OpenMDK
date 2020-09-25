/*
 *
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
 
#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56260_B0 == 1

#include <bmd/bmd.h>

#include <cdk/arch/xgsd_chip.h>
#include <cdk/cdk_debug.h>
#include <cdk/arch/xgsd_miim.h>
#include <cdk/chip/bcm56260_b0_defs.h>

#include "bcm56260_b0_bmd.h"
#include "bcm56260_b0_internal.h"

#define PIPE_RESET_TIMEOUT_MSEC         50

int
bcm56260_b0_bmd_reset(int unit)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int wait_usec = 10000;
    CMIC_CPS_RESETr_t cmic_cps;
    int idx;
    CMIC_SBUS_RING_MAPr_t sbus_ring_map;
    uint32_t ring_map[] = { 0x66034000, 0x55312222, 0x03103775,
                            0x00000000 };
    int port;
    TOP_MISC_CONTROL_1r_t misc_ctrl_1;
    TOP_SOFT_RESET_REG_2r_t soft_reset_2;
    TOP_XGXS_PLL_CONTROL_4r_t pll_ctrl_4;
    TOP_SOFT_RESET_REGr_t soft_reset;
    ING_HW_RESET_CONTROL_2r_t ing_hw_rst_ctrl_2;
    EGR_HW_RESET_CONTROL_1r_t egr_hw_rst_ctrl_1;
    TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_3r_t top_board_sync0_pll_ctrl_reg_3;
    TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_4r_t top_board_sync0_pll_ctrl_reg_4;
    TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_6r_t top_board_sync0_pll_ctrl_reg_6;
    TOP_XGXS_PLL_CONTROL_1r_t top_xgxs_pll_ctrl_1;
    TOP_XGXS_PLL_CONTROL_2r_t top_xgxs_pll_ctrl_2;
    TOP_XGXS_PLL_CONTROL_3r_t top_xgxs_pll_ctrl_3;
    TOP_XGXS_PLL_CONTROL_4r_t top_xgxs_pll_ctrl_4;
    TOP_BROAD_SYNC1_PLL_CTRL_REGISTER_3r_t top_board_sync1_pll_ctrl_reg_3;
    TOP_BROAD_SYNC1_PLL_CTRL_REGISTER_6r_t top_board_sync1_pll_ctrl_reg_6;
    TOP_LCPLL_SOFT_RESET_REGr_t top_lcpll_soft_reset_reg;
    TOP_BROAD_SYNC1_PLL_CTRL_REGISTER_4r_t top_board_sync1_pll_ctrl_reg_4;
    TOP_SERDES_LCPLL_FBDIV_CTRLr_t top_serdes_lcpll_fbdiv_ctrl;
    TOP_XGXS_PLL_CONTROL_7r_t top_xgxs_pll_ctrl_7;
    TOP_XGXS_PLL_STATUSr_t top_xgxs_pll_status;
    ING_HW_RESET_CONTROL_1r_t ing_hw_rst_ctrl_1;
    EGR_HW_RESET_CONTROL_0r_t egr_hw_rst_ctrl_0;
    CMIC_SBUS_TIMEOUTr_t cmic_sbus_timeout;
    TOP_MMU_DDR_PHY_CTRLr_t mmu_ddr_phy_ctrl;
    TOP_CI_PHY_CONTROLr_t top_ci_phy_ctrl;
    XPORT_XGXS_CTRLr_t xport_xgxs_ctrl;
    XLPORT_XGXS0_CTRL_REGr_t xlport_xgxs0_ctrl;
    XLPORT_MAC_CONTROLr_t xlport_mac_ctrl;
    cdk_pbmp_t pbmp;

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

    ioerr += READ_TOP_XGXS_PLL_CONTROL_4r(unit, 0, &pll_ctrl_4);
    TOP_XGXS_PLL_CONTROL_4r_CH1_CLK_ENABLEf_SET(pll_ctrl_4, 0);
    ioerr += WRITE_TOP_XGXS_PLL_CONTROL_4r(unit, 0, pll_ctrl_4);

    /* Master LCPLL */
    TOP_XGXS_PLL_CONTROL_3r_SET(top_xgxs_pll_ctrl_3, 0x00805004);
    ioerr += WRITE_TOP_XGXS_PLL_CONTROL_3r(unit, 0, top_xgxs_pll_ctrl_3);

    ioerr += READ_TOP_XGXS_PLL_CONTROL_7r(unit, 0, &top_xgxs_pll_ctrl_7);
    TOP_XGXS_PLL_CONTROL_7r_LDO_CTRLf_SET(top_xgxs_pll_ctrl_7, 0x22);
    ioerr += WRITE_TOP_XGXS_PLL_CONTROL_7r(unit, 0, top_xgxs_pll_ctrl_7);

    ioerr += READ_TOP_XGXS_PLL_CONTROL_4r(unit, 0, &top_xgxs_pll_ctrl_4);
    TOP_XGXS_PLL_CONTROL_4r_CP1f_SET(top_xgxs_pll_ctrl_4, 0x1);
    TOP_XGXS_PLL_CONTROL_4r_CPf_SET(top_xgxs_pll_ctrl_4, 0x3);
    TOP_XGXS_PLL_CONTROL_4r_CZf_SET(top_xgxs_pll_ctrl_4, 0x3);
    TOP_XGXS_PLL_CONTROL_4r_RPf_SET(top_xgxs_pll_ctrl_4, 0x7);
    TOP_XGXS_PLL_CONTROL_4r_RZf_SET(top_xgxs_pll_ctrl_4, 0x2);
    TOP_XGXS_PLL_CONTROL_4r_ICPf_SET(top_xgxs_pll_ctrl_4, 0xA);
    TOP_XGXS_PLL_CONTROL_4r_VCO_GAINf_SET(top_xgxs_pll_ctrl_4, 0xF);
    ioerr += WRITE_TOP_XGXS_PLL_CONTROL_4r(unit, 0, top_xgxs_pll_ctrl_4);

    /* Serdes LCPLL */
    TOP_XGXS_PLL_CONTROL_1r_SET(top_xgxs_pll_ctrl_1, 0x197d0714);
    ioerr += WRITE_TOP_XGXS_PLL_CONTROL_1r(unit, 1, top_xgxs_pll_ctrl_1);

    TOP_XGXS_PLL_CONTROL_2r_SET(top_xgxs_pll_ctrl_2, 0x40004015);
    ioerr += WRITE_TOP_XGXS_PLL_CONTROL_2r(unit, 1, top_xgxs_pll_ctrl_2);

    TOP_XGXS_PLL_CONTROL_3r_SET(top_xgxs_pll_ctrl_3, 0x897004);
    ioerr += WRITE_TOP_XGXS_PLL_CONTROL_3r(unit, 1, top_xgxs_pll_ctrl_3);

    ioerr += READ_TOP_XGXS_PLL_CONTROL_7r(unit, 1, &top_xgxs_pll_ctrl_7);
    TOP_XGXS_PLL_CONTROL_7r_LDO_CTRLf_SET(top_xgxs_pll_ctrl_7, 0x22);
    ioerr += WRITE_TOP_XGXS_PLL_CONTROL_7r(unit, 1, top_xgxs_pll_ctrl_7);

    ioerr += READ_TOP_XGXS_PLL_CONTROL_4r(unit, 1, &top_xgxs_pll_ctrl_4);
    TOP_XGXS_PLL_CONTROL_4r_CP1f_SET(top_xgxs_pll_ctrl_4, 0x1);
    TOP_XGXS_PLL_CONTROL_4r_CPf_SET(top_xgxs_pll_ctrl_4, 0x3);
    TOP_XGXS_PLL_CONTROL_4r_CZf_SET(top_xgxs_pll_ctrl_4, 0x3);
    TOP_XGXS_PLL_CONTROL_4r_RPf_SET(top_xgxs_pll_ctrl_4, 0x3);
    TOP_XGXS_PLL_CONTROL_4r_RZf_SET(top_xgxs_pll_ctrl_4, 0);
    TOP_XGXS_PLL_CONTROL_4r_ICPf_SET(top_xgxs_pll_ctrl_4, 0xA);
    TOP_XGXS_PLL_CONTROL_4r_VCO_GAINf_SET(top_xgxs_pll_ctrl_4, 0xF);
    ioerr += WRITE_TOP_XGXS_PLL_CONTROL_4r(unit, 1, top_xgxs_pll_ctrl_4);

    /* Broad Sync0 LCPLL */
    TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_3r_SET(top_board_sync0_pll_ctrl_reg_3, 0x00805004);
    ioerr += WRITE_TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_3r(unit, top_board_sync0_pll_ctrl_reg_3);

    ioerr += READ_TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_6r(unit, &top_board_sync0_pll_ctrl_reg_6);
    TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_6r_LDO_CTRLf_SET(top_board_sync0_pll_ctrl_reg_6, 0x22);
    ioerr += WRITE_TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_6r(unit, top_board_sync0_pll_ctrl_reg_6);

    READ_TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_4r(unit, &top_board_sync0_pll_ctrl_reg_4);
    TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_4r_CP1f_SET(top_board_sync0_pll_ctrl_reg_4, 0x1);
    TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_4r_CPf_SET(top_board_sync0_pll_ctrl_reg_4, 0x3);
    TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_4r_CZf_SET(top_board_sync0_pll_ctrl_reg_4, 0x3);
    TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_4r_RPf_SET(top_board_sync0_pll_ctrl_reg_4, 0x7);
    TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_4r_RZf_SET(top_board_sync0_pll_ctrl_reg_4, 0x2);
    TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_4r_ICPf_SET(top_board_sync0_pll_ctrl_reg_4, 0xA);
    TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_4r_VCO_GAINf_SET(top_board_sync0_pll_ctrl_reg_4, 0xF);
    WRITE_TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_4r(unit, top_board_sync0_pll_ctrl_reg_4);

    /* Broad Sync1 LCPLL */
    TOP_BROAD_SYNC1_PLL_CTRL_REGISTER_3r_SET(top_board_sync1_pll_ctrl_reg_3, 0x00805004);
    ioerr += WRITE_TOP_BROAD_SYNC1_PLL_CTRL_REGISTER_3r(unit, top_board_sync1_pll_ctrl_reg_3);

    ioerr += READ_TOP_BROAD_SYNC1_PLL_CTRL_REGISTER_6r(unit, &top_board_sync1_pll_ctrl_reg_6);
    TOP_BROAD_SYNC1_PLL_CTRL_REGISTER_6r_LDO_CTRLf_SET(top_board_sync1_pll_ctrl_reg_6, 0x22);
    ioerr += WRITE_TOP_BROAD_SYNC1_PLL_CTRL_REGISTER_6r(unit, top_board_sync1_pll_ctrl_reg_6);

    READ_TOP_BROAD_SYNC1_PLL_CTRL_REGISTER_4r(unit, &top_board_sync1_pll_ctrl_reg_4);
    TOP_BROAD_SYNC1_PLL_CTRL_REGISTER_4r_CP1f_SET(top_board_sync1_pll_ctrl_reg_4, 0x1);
    TOP_BROAD_SYNC1_PLL_CTRL_REGISTER_4r_CPf_SET(top_board_sync1_pll_ctrl_reg_4, 0x3);
    TOP_BROAD_SYNC1_PLL_CTRL_REGISTER_4r_CZf_SET(top_board_sync1_pll_ctrl_reg_4, 0x3);
    TOP_BROAD_SYNC1_PLL_CTRL_REGISTER_4r_RPf_SET(top_board_sync1_pll_ctrl_reg_4, 0x7);
    TOP_BROAD_SYNC1_PLL_CTRL_REGISTER_4r_RZf_SET(top_board_sync1_pll_ctrl_reg_4, 0x2);
    TOP_BROAD_SYNC1_PLL_CTRL_REGISTER_4r_ICPf_SET(top_board_sync1_pll_ctrl_reg_4, 0xA);
    TOP_BROAD_SYNC1_PLL_CTRL_REGISTER_4r_VCO_GAINf_SET(top_board_sync1_pll_ctrl_reg_4, 0xF);
    WRITE_TOP_BROAD_SYNC1_PLL_CTRL_REGISTER_4r(unit, top_board_sync1_pll_ctrl_reg_4);

    /* Top Serdes LCPLL settings */
    TOP_SERDES_LCPLL_FBDIV_CTRLr_SET(top_serdes_lcpll_fbdiv_ctrl, 0x7d0);
    ioerr += WRITE_TOP_SERDES_LCPLL_FBDIV_CTRLr(unit, 1, top_serdes_lcpll_fbdiv_ctrl);

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
    READ_TOP_XGXS_PLL_STATUSr(unit, 1, &top_xgxs_pll_status);
    if (!(TOP_XGXS_PLL_STATUSr_TOP_XGPLL0_LOCKf_GET(top_xgxs_pll_status))) {
        CDK_PRINTF("LCPLL 1 not locked");
    }

    /* Configure TS PLL */
    ioerr += READ_TOP_MISC_CONTROL_1r(unit,&misc_ctrl_1);
    TOP_MISC_CONTROL_1r_TS_PLL_CLK_IN_SELf_SET(misc_ctrl_1, 0);
    ioerr += WRITE_TOP_MISC_CONTROL_1r(unit, misc_ctrl_1);

    /* 250Mhz TS PLL implies 4ns resolution */
    ioerr += READ_TOP_SOFT_RESET_REG_2r(unit, &soft_reset_2);
    TOP_SOFT_RESET_REG_2r_TOP_TS_PLL_RST_Lf_SET(soft_reset_2, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REG_2r(unit, soft_reset_2);

    BMD_SYS_USLEEP(wait_usec);

    ioerr += READ_TOP_SOFT_RESET_REG_2r(unit, &soft_reset_2);
    TOP_SOFT_RESET_REG_2r_TOP_TS_PLL_POST_RST_Lf_SET(soft_reset_2, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REG_2r(unit, soft_reset_2);

    /* Bring port blocks out of reset */
    ioerr += READ_TOP_SOFT_RESET_REGr(unit, &soft_reset);
    TOP_SOFT_RESET_REGr_TOP_MXQ0_RST_Lf_SET(soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_MXQ1_RST_Lf_SET(soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_MXQ2_RST_Lf_SET(soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_MXQ3_RST_Lf_SET(soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_MXQ4_RST_Lf_SET(soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_MXQ5_RST_Lf_SET(soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_PM_RST_Lf_SET(soft_reset, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REGr(unit, soft_reset);

    BMD_SYS_USLEEP(wait_usec);

    /* Bring network sync out of reset */
    ioerr += READ_TOP_SOFT_RESET_REGr(unit, &soft_reset);
    TOP_SOFT_RESET_REGr_TOP_MXQ0_HOTSWAP_RST_Lf_SET(soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_MXQ1_HOTSWAP_RST_Lf_SET(soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_MXQ2_HOTSWAP_RST_Lf_SET(soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_MXQ3_HOTSWAP_RST_Lf_SET(soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_MXQ4_HOTSWAP_RST_Lf_SET(soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_MXQ5_HOTSWAP_RST_Lf_SET(soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_PM_HOTSWAP_RST_Lf_SET(soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_NS_RST_Lf_SET(soft_reset, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REGr(unit, soft_reset);

    BMD_SYS_USLEEP(wait_usec);

    CMIC_SBUS_TIMEOUTr_SET(cmic_sbus_timeout, 0x7d0);
    ioerr += WRITE_CMIC_SBUS_TIMEOUTr(unit, cmic_sbus_timeout);

    ioerr += READ_TOP_MMU_DDR_PHY_CTRLr(unit, &mmu_ddr_phy_ctrl);
    TOP_MMU_DDR_PHY_CTRLr_I_PWRONIN_PHYf_SET(mmu_ddr_phy_ctrl, 1);
    ioerr += WRITE_TOP_MMU_DDR_PHY_CTRLr(unit, mmu_ddr_phy_ctrl);

    BMD_SYS_USLEEP(1000);

    ioerr += READ_TOP_MMU_DDR_PHY_CTRLr(unit, &mmu_ddr_phy_ctrl);
    TOP_MMU_DDR_PHY_CTRLr_I_PWROKIN_PHYf_SET(mmu_ddr_phy_ctrl, 1);
    ioerr += WRITE_TOP_MMU_DDR_PHY_CTRLr(unit, mmu_ddr_phy_ctrl);
    BMD_SYS_USLEEP(1000);

    ioerr += READ_TOP_MMU_DDR_PHY_CTRLr(unit, &mmu_ddr_phy_ctrl);
    TOP_MMU_DDR_PHY_CTRLr_I_ISO_PHY_DFIf_SET(mmu_ddr_phy_ctrl, 1);
    ioerr += WRITE_TOP_MMU_DDR_PHY_CTRLr(unit, mmu_ddr_phy_ctrl);
    BMD_SYS_USLEEP(1000);

    TOP_CI_PHY_CONTROLr_CLR(top_ci_phy_ctrl);
    TOP_CI_PHY_CONTROLr_DDR_RESET_Nf_SET(top_ci_phy_ctrl, 1);
    WRITE_TOP_CI_PHY_CONTROLr(unit, top_ci_phy_ctrl);
    BMD_SYS_USLEEP(1000);

    ioerr += READ_TOP_SOFT_RESET_REGr(unit, &soft_reset);
    TOP_SOFT_RESET_REGr_TOP_AVS_RST_Lf_SET(soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_OOBFC0_RST_Lf_SET(soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_OOBFC1_RST_Lf_SET(soft_reset, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REGr(unit, soft_reset);

    ioerr += READ_TOP_SOFT_RESET_REGr(unit, &soft_reset);
    TOP_SOFT_RESET_REGr_TOP_EP_RST_Lf_SET(soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_IP_RST_Lf_SET(soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_MMU_RST_Lf_SET(soft_reset, 1);
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
        CDK_WARN(("bcm56260_a0_bmd_reset[%d]: IPIPE reset timeout\n", unit));
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
        CDK_WARN(("bcm56260_a0_bmd_reset[%d]: EPIPE reset timeout\n", unit));
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

    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_MXQPORT, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        if (MXQPORT_SUBPORT(unit, port) == 0) {
            ioerr += READ_XPORT_XGXS_CTRLr(unit, &xport_xgxs_ctrl, port);
            XPORT_XGXS_CTRLr_PWRDWNf_SET(xport_xgxs_ctrl, 0);
            ioerr += WRITE_XPORT_XGXS_CTRLr(unit, xport_xgxs_ctrl, port);
            XPORT_XGXS_CTRLr_RSTB_HWf_SET(xport_xgxs_ctrl, 1);
            ioerr += WRITE_XPORT_XGXS_CTRLr(unit, xport_xgxs_ctrl, port);
            XPORT_XGXS_CTRLr_IDDQf_SET(xport_xgxs_ctrl, 0);
            ioerr += WRITE_XPORT_XGXS_CTRLr(unit, xport_xgxs_ctrl, port);
            XPORT_XGXS_CTRLr_RSTB_PLLf_SET(xport_xgxs_ctrl, 1);
            ioerr += WRITE_XPORT_XGXS_CTRLr(unit, xport_xgxs_ctrl, port);
            XPORT_XGXS_CTRLr_RSTB_MDIOREGSf_SET(xport_xgxs_ctrl, 1);
            ioerr += WRITE_XPORT_XGXS_CTRLr(unit, xport_xgxs_ctrl, port);
            XPORT_XGXS_CTRLr_TXD1G_FIFO_RSTBf_SET(xport_xgxs_ctrl, 0xf);
            ioerr += WRITE_XPORT_XGXS_CTRLr(unit, xport_xgxs_ctrl, port);
            XPORT_XGXS_CTRLr_TXD10G_FIFO_RSTBf_SET(xport_xgxs_ctrl, 1);
            ioerr += WRITE_XPORT_XGXS_CTRLr(unit, xport_xgxs_ctrl, port);
        }
    }

    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_XLPORT, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        /* XGXS MAC initialization steps.    */

        if (XLPORT_SUBPORT(unit,port) == 0) {
            /* Release reset (if asserted) to allow bigmac to initialize */
            ioerr += READ_XLPORT_XGXS0_CTRL_REGr(unit, &xlport_xgxs0_ctrl, port);
            XLPORT_XGXS0_CTRL_REGr_IDDQf_SET(xlport_xgxs0_ctrl, 0);
            XLPORT_XGXS0_CTRL_REGr_PWRDWNf_SET(xlport_xgxs0_ctrl, 0);
            XLPORT_XGXS0_CTRL_REGr_RSTB_HWf_SET(xlport_xgxs0_ctrl, 1);
            ioerr += WRITE_XLPORT_XGXS0_CTRL_REGr(unit, xlport_xgxs0_ctrl, port);

            BMD_SYS_USLEEP(1100);
        }
    }

    /* MAC reset */
    CDK_PBMP_ITER(pbmp, port) {
        /* We only need to reset first port in each block */
        if (XLPORT_SUBPORT(unit, port) == 0) {
            ioerr += READ_XLPORT_MAC_CONTROLr(unit, &xlport_mac_ctrl, port);
            XLPORT_MAC_CONTROLr_XMAC0_RESETf_SET(xlport_mac_ctrl, 1);
            ioerr += WRITE_XLPORT_MAC_CONTROLr(unit, xlport_mac_ctrl, port);

            BMD_SYS_USLEEP(10);

            XLPORT_MAC_CONTROLr_XMAC0_RESETf_SET(xlport_mac_ctrl, 0);
            ioerr += WRITE_XLPORT_MAC_CONTROLr(unit, xlport_mac_ctrl, port);
        }
    }

    return ioerr ? CDK_E_IO : rv;
}

#endif /* CDK_CONFIG_INCLUDE_BCM56260_B0 */
