/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56560_A0 == 1

#include <bmd/bmd.h>

#include <cdk/chip/bcm56560_a0_defs.h>
#include <cdk/cdk_debug.h>

#include "bcm56560_a0_bmd.h"
#include "bcm56560_a0_internal.h"

/*
 * Function: _soc_tsc_disable
 *
 * Purpose:
 *     Function to dsiable unused TSCs in the chip.
 */
static void
_soc_tsc_disable(int unit)
{
    uint32_t tsc_enable = 0;
    int lane, phy_port, tsc, port;
    cdk_pbmp_t cxxpbmp, clpbmp;
    TOP_TSC_ENABLEr_t top_tsc;
    
    bcm56560_a0_cxxport_pbmp_get(unit, &cxxpbmp);
    bcm56560_a0_clport_pbmp_get(unit, &clpbmp);
    for (phy_port = 1;
         phy_port <= PORTS_PER_PIPE;
         phy_port = (phy_port + PORTS_PER_XLP)) {

        tsc = (phy_port - 1) / PORTS_PER_XLP;
        for (lane = 0; lane < PORTS_PER_XLP; lane++) {
            port = P2L(unit, (phy_port + lane));
            if (port != -1) {
                tsc_enable |= (1 << tsc);
                if (CDK_PBMP_MEMBER(cxxpbmp, port) && 
                    CDK_PBMP_MEMBER(clpbmp, port)) {
                    tsc_enable |= (1 << (tsc + 1));
                    tsc_enable |= (1 << (tsc + 2));
                }
                break;
            }
        }
    }
    TOP_TSC_ENABLEr_SET(top_tsc, tsc_enable);
    WRITE_TOP_TSC_ENABLEr(unit, top_tsc);
}


/*
 * function: _soc_tsc_xgxs_reset
 * purpose:  Reset all TSCs associated with the passed port.
*/
static void
_soc_tsc_xgxs_reset(int unit, int port, int reg_idx)
{
    int ioerr = 0;
    cdk_pbmp_t cxxpbmp, clpbmp, xlpbmp, xlbpbmp;
    CXXPORT_XGXS0_CTRL0_REGr_t cxxport_ctrl0;
    CXXPORT_XGXS0_CTRL1_REGr_t cxxport_ctrl1;
    CXXPORT_XGXS0_CTRL2_REGr_t cxxport_ctrl2;
    CLPORT_XGXS0_CTRL_REGr_t clport_ctrl;
    XLPORT_XGXS0_CTRL_REGr_t xlp_ctrl0;    
    int sleep_usec = 100000;
    
    bcm56560_a0_cxxport_pbmp_get(unit, &cxxpbmp);
    bcm56560_a0_clport_pbmp_get(unit, &clpbmp);
    bcm56560_a0_xlport_pbmp_get(unit, &xlpbmp);
    bcm56560_a0_xlbport_pbmp_get(unit, &xlbpbmp);
    CDK_PBMP_OR(xlpbmp, xlbpbmp);

    if (CDK_PBMP_MEMBER(cxxpbmp, port)) {
        /* CXXPORT_XGXS0_CTRL0_REGr */
        /*
        * Reference clock selection
        */
        ioerr += READ_CXXPORT_XGXS0_CTRL0_REGr(unit, port, &cxxport_ctrl0);
        CXXPORT_XGXS0_CTRL0_REGr_REFIN_ENf_SET(cxxport_ctrl0, 1);
        ioerr += WRITE_CXXPORT_XGXS0_CTRL0_REGr(unit, port, cxxport_ctrl0);
    
        /* Deassert power down */
        CXXPORT_XGXS0_CTRL0_REGr_PWRDWNf_SET(cxxport_ctrl0, 0);
        ioerr += WRITE_CXXPORT_XGXS0_CTRL0_REGr(unit, port, cxxport_ctrl0);
        BMD_SYS_USLEEP(sleep_usec);
    
        /* Reset XGXS */
        CXXPORT_XGXS0_CTRL0_REGr_RSTB_HWf_SET(cxxport_ctrl0, 0);
        ioerr += WRITE_CXXPORT_XGXS0_CTRL0_REGr(unit, port, cxxport_ctrl0);
        BMD_SYS_USLEEP(sleep_usec);
    
        /* Bring XGXS out of reset */
        CXXPORT_XGXS0_CTRL0_REGr_RSTB_HWf_SET(cxxport_ctrl0, 1);
        ioerr += WRITE_CXXPORT_XGXS0_CTRL0_REGr(unit, port, cxxport_ctrl0);
        BMD_SYS_USLEEP(sleep_usec);
        /* CXXPORT_XGXS0_CTRL1_REGr */
        /*
        * Reference clock selection
        */
        ioerr += READ_CXXPORT_XGXS0_CTRL1_REGr(unit, port, &cxxport_ctrl1);
        CXXPORT_XGXS0_CTRL1_REGr_REFIN_ENf_SET(cxxport_ctrl1, 1);
        ioerr += WRITE_CXXPORT_XGXS0_CTRL1_REGr(unit, port, cxxport_ctrl1);
    
        /* Deassert power down */
        CXXPORT_XGXS0_CTRL1_REGr_PWRDWNf_SET(cxxport_ctrl1, 0);
        ioerr += WRITE_CXXPORT_XGXS0_CTRL1_REGr(unit, port, cxxport_ctrl1);
        BMD_SYS_USLEEP(sleep_usec);
    
        /* Reset XGXS */
        CXXPORT_XGXS0_CTRL1_REGr_RSTB_HWf_SET(cxxport_ctrl1, 0);
        ioerr += WRITE_CXXPORT_XGXS0_CTRL1_REGr(unit, port, cxxport_ctrl1);
        BMD_SYS_USLEEP(sleep_usec);
    
        /* Bring XGXS out of reset */
        CXXPORT_XGXS0_CTRL1_REGr_RSTB_HWf_SET(cxxport_ctrl1, 1);
        ioerr += WRITE_CXXPORT_XGXS0_CTRL1_REGr(unit, port, cxxport_ctrl1);
        BMD_SYS_USLEEP(sleep_usec);

        /* CXXPORT_XGXS0_CTRL2_REGr */
        /*
        * Reference clock selection
        */
        ioerr += READ_CXXPORT_XGXS0_CTRL2_REGr(unit, port, &cxxport_ctrl2);
        CXXPORT_XGXS0_CTRL2_REGr_REFIN_ENf_SET(cxxport_ctrl2, 1);
        ioerr += WRITE_CXXPORT_XGXS0_CTRL2_REGr(unit, port, cxxport_ctrl2);
    
        /* Deassert power down */
        CXXPORT_XGXS0_CTRL2_REGr_PWRDWNf_SET(cxxport_ctrl2, 0);
        ioerr += WRITE_CXXPORT_XGXS0_CTRL2_REGr(unit, port, cxxport_ctrl2);
        BMD_SYS_USLEEP(sleep_usec);
    
        /* Reset XGXS */
        CXXPORT_XGXS0_CTRL2_REGr_RSTB_HWf_SET(cxxport_ctrl2, 0);
        ioerr += WRITE_CXXPORT_XGXS0_CTRL2_REGr(unit, port, cxxport_ctrl2);
        BMD_SYS_USLEEP(sleep_usec);
    
        /* Bring XGXS out of reset */
        CXXPORT_XGXS0_CTRL2_REGr_RSTB_HWf_SET(cxxport_ctrl2, 1);
        ioerr += WRITE_CXXPORT_XGXS0_CTRL2_REGr(unit, port, cxxport_ctrl2);
        BMD_SYS_USLEEP(sleep_usec);
        
    } 
    
    if (CDK_PBMP_MEMBER(clpbmp, port)) {
        /* CLPORT_XGXS0_CTRL_REGr */
        /*
        * Reference clock selection
        */
        ioerr += READ_CLPORT_XGXS0_CTRL_REGr(unit, &clport_ctrl, port);
        CLPORT_XGXS0_CTRL_REGr_REFIN_ENf_SET(clport_ctrl, 1);
        ioerr += WRITE_CLPORT_XGXS0_CTRL_REGr(unit, clport_ctrl, port);
    
        /* Deassert power down */
        CLPORT_XGXS0_CTRL_REGr_PWRDWNf_SET(clport_ctrl, 0);
        ioerr += WRITE_CLPORT_XGXS0_CTRL_REGr(unit, clport_ctrl, port);
        BMD_SYS_USLEEP(sleep_usec);
    
        /* Reset XGXS */
        CLPORT_XGXS0_CTRL_REGr_RSTB_HWf_SET(clport_ctrl, 0);
        ioerr += WRITE_CLPORT_XGXS0_CTRL_REGr(unit, clport_ctrl, port);
        BMD_SYS_USLEEP(sleep_usec);
    
        /* Bring XGXS out of reset */
        CLPORT_XGXS0_CTRL_REGr_RSTB_HWf_SET(clport_ctrl, 1);
        ioerr += WRITE_CLPORT_XGXS0_CTRL_REGr(unit, clport_ctrl, port);
        BMD_SYS_USLEEP(sleep_usec);
        
    } else if (CDK_PBMP_MEMBER(xlpbmp, port)) {
        /*
         * Reference clock selection
         */
        ioerr += READ_XLPORT_XGXS0_CTRL_REGr(unit, &xlp_ctrl0, port);
        XLPORT_XGXS0_CTRL_REGr_REFIN_ENf_SET(xlp_ctrl0, 1);
        ioerr += WRITE_XLPORT_XGXS0_CTRL_REGr(unit, xlp_ctrl0, port);
        
        XLPORT_XGXS0_CTRL_REGr_IDDQf_SET(xlp_ctrl0, 0);
        ioerr += WRITE_XLPORT_XGXS0_CTRL_REGr(unit, xlp_ctrl0, port);
        
        /* Deassert power down */
        XLPORT_XGXS0_CTRL_REGr_PWRDWNf_SET(xlp_ctrl0, 0);
        ioerr += WRITE_XLPORT_XGXS0_CTRL_REGr(unit, xlp_ctrl0, port);
        BMD_SYS_USLEEP(sleep_usec);

        /* Reset XGXS */
        XLPORT_XGXS0_CTRL_REGr_RSTB_HWf_SET(xlp_ctrl0, 0);
        ioerr += WRITE_XLPORT_XGXS0_CTRL_REGr(unit, xlp_ctrl0, port);
        BMD_SYS_USLEEP(sleep_usec);

        /* Bring XGXS out of reset */
        XLPORT_XGXS0_CTRL_REGr_RSTB_HWf_SET(xlp_ctrl0, 1);
        ioerr += WRITE_XLPORT_XGXS0_CTRL_REGr(unit, xlp_ctrl0, port);
        BMD_SYS_USLEEP(sleep_usec);
    }
}

static void
_soc_tsc_reset(int unit)
{
    cdk_pbmp_t pbmp;
    int port;
    XLPORT_XGXS0_CTRL_REGr_t xlp_ctrl0;
  
    bcm56560_a0_cxxport_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        _soc_tsc_xgxs_reset(unit, port, 0);
        _soc_tsc_xgxs_reset(unit, port, 1);
        _soc_tsc_xgxs_reset(unit, port, 2);
    }
       
    bcm56560_a0_all_front_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        if (((port - 1) % 4) != 0) {
            /* already done */
            continue;
        }

        _soc_tsc_xgxs_reset(unit, port, 0);
    }
 
    /* TSC reset is controlled by PortMod internally */
    bcm56560_a0_xlport_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        if (((port - 1) % 4) != 0) {
            /* already done */
            continue;
        }
        READ_XLPORT_XGXS0_CTRL_REGr(unit, &xlp_ctrl0, port);
        XLPORT_XGXS0_CTRL_REGr_RSTB_HWf_SET(xlp_ctrl0, 0);
        XLPORT_XGXS0_CTRL_REGr_PWRDWNf_SET(xlp_ctrl0, 1);
        XLPORT_XGXS0_CTRL_REGr_IDDQf_SET(xlp_ctrl0, 1);
        WRITE_XLPORT_XGXS0_CTRL_REGr(unit, xlp_ctrl0, port);
        
        READ_XLPORT_XGXS0_CTRL_REGr(unit, &xlp_ctrl0, port);
        XLPORT_XGXS0_CTRL_REGr_RSTB_HWf_SET(xlp_ctrl0, 1);
        XLPORT_XGXS0_CTRL_REGr_PWRDWNf_SET(xlp_ctrl0, 0);
        XLPORT_XGXS0_CTRL_REGr_IDDQf_SET(xlp_ctrl0, 0);
        WRITE_XLPORT_XGXS0_CTRL_REGr(unit, xlp_ctrl0, port);        
    }
    bcm56560_a0_xlbport_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        if (((port - 1) % 4) != 0) {
            /* already done */
            continue;
        }
        READ_XLPORT_XGXS0_CTRL_REGr(unit, &xlp_ctrl0, port);
        XLPORT_XGXS0_CTRL_REGr_RSTB_HWf_SET(xlp_ctrl0, 0);
        XLPORT_XGXS0_CTRL_REGr_PWRDWNf_SET(xlp_ctrl0, 1);
        XLPORT_XGXS0_CTRL_REGr_IDDQf_SET(xlp_ctrl0, 1);
        WRITE_XLPORT_XGXS0_CTRL_REGr(unit, xlp_ctrl0, port);
        
        READ_XLPORT_XGXS0_CTRL_REGr(unit, &xlp_ctrl0, port);
        XLPORT_XGXS0_CTRL_REGr_RSTB_HWf_SET(xlp_ctrl0, 1);
        XLPORT_XGXS0_CTRL_REGr_PWRDWNf_SET(xlp_ctrl0, 0);
        XLPORT_XGXS0_CTRL_REGr_IDDQf_SET(xlp_ctrl0, 0);
        WRITE_XLPORT_XGXS0_CTRL_REGr(unit, xlp_ctrl0, port);        
    }
    
}

int 
bcm56560_a0_bmd_reset(int unit)
{
    int ioerr = 0;
    int idx;
    CMIC_CPS_RESETr_t cps_reset;
    CMIC_SBUS_RING_MAPr_t sbus_ring_map;
    CMIC_SBUS_TIMEOUTr_t sbus_to;
    TOP_CORE_CLK_FREQ_SELr_t top_core_clk_freq;
    TOP_PVTMON_CTRL_1r_t pvtmon_ctrl;
    int wait_usec = 10000;    
    TOP_SOFT_RESET_REGr_t top_sreset_reg;
    TOP_SOFT_RESET_REG_2r_t top_sreset_2;
    TOP_BS_PLL_CTRL_3r_t top_bs_pll_ctrl3;
    TOP_PVTMON_CTRL_0r_t top_pvtmon_ctrl0;
    TOP_SOFT_RESET_REG_3r_t top_sreset_reg3;
    int sel;
    static const uint32_t apache_freq_list[] = { CHIP_FLAG_FREQ841, 
                CHIP_FLAG_FREQ793, CHIP_FLAG_FREQ575, 0, CHIP_FLAG_FREQ435};
    uint32_t ring_map[] = { 0x43052100, 0x33333333, 0x44444333, 0x50444444,
                            0x00060555, 0x00000000, 0x00000000, 0x00000000 };

    BMD_CHECK_UNIT(unit);

    /* Initialize endian mode for correct reset access */
    ioerr += cdk_xgsd_cmic_init(unit);

    /* Pull reset line */
    ioerr += READ_CMIC_CPS_RESETr(unit, &cps_reset);
    CMIC_CPS_RESETr_CPS_RESETf_SET(cps_reset, 1);
    ioerr += WRITE_CMIC_CPS_RESETr(unit, cps_reset);

    /* Wait for all tables to initialize */
    BMD_SYS_USLEEP(wait_usec);

    /* Re-initialize endian mode after reset */
    ioerr += cdk_xgsd_cmic_init(unit);

    /*
     * SBUS ring and block number:
     * ring 0: IP(1)
     * ring 1: EP(2)
     * ring 2: MMU(3)
     */
    for (idx = 0; idx < COUNTOF(ring_map); idx++) {
        CMIC_SBUS_RING_MAPr_SET(sbus_ring_map, ring_map[idx]);
        ioerr += WRITE_CMIC_SBUS_RING_MAPr(unit, idx, sbus_ring_map);
    }
    CMIC_SBUS_TIMEOUTr_SET(sbus_to, 0x1000);
    ioerr += WRITE_CMIC_SBUS_TIMEOUTr(unit, sbus_to);
    BMD_SYS_USLEEP(wait_usec);
    
    if ((CDK_XGSD_FLAGS(unit) & CHIP_FLAG_FREQ793) != CHIP_FLAG_FREQ793) {
        for (idx = 0; idx < 5; idx++) {
            if (apache_freq_list[idx] == 
                    (CDK_XGSD_FLAGS(unit) & apache_freq_list[idx])) {
                sel = idx;
            }
        }
        ioerr += READ_TOP_CORE_CLK_FREQ_SELr(unit, &top_core_clk_freq);
        TOP_CORE_CLK_FREQ_SELr_SW_CORE_CLK_SEL_ENf_SET(top_core_clk_freq, 1);
        ioerr += WRITE_TOP_CORE_CLK_FREQ_SELr(unit, top_core_clk_freq);
        TOP_CORE_CLK_FREQ_SELr_CORE_CLK_FREQ_SELf_SET(top_core_clk_freq, sel);
        ioerr += WRITE_TOP_CORE_CLK_FREQ_SELr(unit, top_core_clk_freq);
        TOP_CORE_CLK_FREQ_SELr_SW_CORE_CLK_SEL_ENf_SET(top_core_clk_freq, 1);
        ioerr += WRITE_TOP_CORE_CLK_FREQ_SELr(unit, top_core_clk_freq);
        TOP_CORE_CLK_FREQ_SELr_SW_CORE_CLK_SEL_ENf_SET(top_core_clk_freq, 0);
        ioerr += WRITE_TOP_CORE_CLK_FREQ_SELr(unit, top_core_clk_freq);
    }
    /* Put TS, BS PLLs to reset before changing PLL control registers */
    ioerr += READ_TOP_SOFT_RESET_REG_2r(unit, &top_sreset_2);
    TOP_SOFT_RESET_REG_2r_TOP_TS_PLL_RST_Lf_SET(top_sreset_2, 0);
    TOP_SOFT_RESET_REG_2r_TOP_BS_PLL0_RST_Lf_SET(top_sreset_2, 0);
    TOP_SOFT_RESET_REG_2r_TOP_BS_PLL1_RST_Lf_SET(top_sreset_2, 0);
    TOP_SOFT_RESET_REG_2r_TOP_TS_PLL_POST_RST_Lf_SET(top_sreset_2, 0);
    TOP_SOFT_RESET_REG_2r_TOP_BS_PLL0_POST_RST_Lf_SET(top_sreset_2, 0);
    TOP_SOFT_RESET_REG_2r_TOP_BS_PLL1_POST_RST_Lf_SET(top_sreset_2, 0);
    ioerr += WRITE_TOP_SOFT_RESET_REG_2r(unit, top_sreset_2);

    /* PLL settings for internal reference are retained to default except for MDIV */
    /* BS PLLs Fout by default is 14MHz. Adjust MDIV to make it as 20MHz */
    ioerr += READ_TOP_BS_PLL_CTRL_3r(unit, 0, &top_bs_pll_ctrl3);
    TOP_BS_PLL_CTRL_3r_CH0_MDIVf_SET(top_bs_pll_ctrl3, 175);
    ioerr += WRITE_TOP_BS_PLL_CTRL_3r(unit, 0, top_bs_pll_ctrl3);

    ioerr += READ_TOP_BS_PLL_CTRL_3r(unit, 1, &top_bs_pll_ctrl3);
    TOP_BS_PLL_CTRL_3r_CH0_MDIVf_SET(top_bs_pll_ctrl3, 175);
    ioerr += WRITE_TOP_BS_PLL_CTRL_3r(unit, 1, top_bs_pll_ctrl3);

    /* Bring LCPLL, time sync PLL, BroadSync PLL out of reset */
    ioerr += READ_TOP_SOFT_RESET_REG_2r(unit, &top_sreset_2);
    TOP_SOFT_RESET_REG_2r_TOP_TS_PLL_RST_Lf_SET(top_sreset_2, 1);
    TOP_SOFT_RESET_REG_2r_TOP_BS_PLL0_RST_Lf_SET(top_sreset_2, 1);
    TOP_SOFT_RESET_REG_2r_TOP_BS_PLL1_RST_Lf_SET(top_sreset_2, 1);
    TOP_SOFT_RESET_REG_2r_TOP_AVS_PMB_RST_Lf_SET(top_sreset_2, 1);
    TOP_SOFT_RESET_REG_2r_TOP_ARS_RST_Lf_SET(top_sreset_2, 1);
    TOP_SOFT_RESET_REG_2r_TOP_AVS_PVTMON_RST_Lf_SET(top_sreset_2, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REG_2r(unit, top_sreset_2);
    
    BMD_SYS_USLEEP(wait_usec);

    /* De-assert LCPLL's post reset */
    ioerr += READ_TOP_SOFT_RESET_REG_2r(unit, &top_sreset_2);
    TOP_SOFT_RESET_REG_2r_TOP_TS_PLL_POST_RST_Lf_SET(top_sreset_2, 1);
    TOP_SOFT_RESET_REG_2r_TOP_BS_PLL0_POST_RST_Lf_SET(top_sreset_2, 1);
    TOP_SOFT_RESET_REG_2r_TOP_BS_PLL1_POST_RST_Lf_SET(top_sreset_2, 1);
    TOP_SOFT_RESET_REG_2r_TOP_PVT_MON_MIN_RST_Lf_SET(top_sreset_2, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REG_2r(unit, top_sreset_2);
    
    BMD_SYS_USLEEP(wait_usec);

    /* Set correct value for BG_ADJ */
    ioerr += READ_TOP_PVTMON_CTRL_0r(unit, &top_pvtmon_ctrl0);
    TOP_PVTMON_CTRL_0r_BG_ADJf_SET(top_pvtmon_ctrl0, 0);
    ioerr += WRITE_TOP_PVTMON_CTRL_0r(unit, top_pvtmon_ctrl0);

    /* Bring port blocks out of reset */
    TOP_SOFT_RESET_REGr_CLR(top_sreset_reg);
    TOP_SOFT_RESET_REGr_TOP_PGW0_RST_Lf_SET(top_sreset_reg, 1);
    TOP_SOFT_RESET_REGr_TOP_PGW1_RST_Lf_SET(top_sreset_reg, 1);
    TOP_SOFT_RESET_REGr_TOP_TS_RST_Lf_SET(top_sreset_reg, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REGr(unit, top_sreset_reg);

    BMD_SYS_USLEEP(wait_usec);

    /* Bring IP, EP, and MMU blocks out of reset */
    ioerr += READ_TOP_SOFT_RESET_REGr(unit, &top_sreset_reg);
    TOP_SOFT_RESET_REGr_TOP_RDB_RST_Lf_SET(top_sreset_reg, 1);
    TOP_SOFT_RESET_REGr_TOP_MMU_RST_Lf_SET(top_sreset_reg, 1);
    TOP_SOFT_RESET_REGr_TOP_IP_RST_Lf_SET(top_sreset_reg, 1);
    TOP_SOFT_RESET_REGr_TOP_EP_RST_Lf_SET(top_sreset_reg, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REGr(unit, top_sreset_reg);

    BMD_SYS_USLEEP(wait_usec);

    /* Bring PMs out of reset */
    ioerr += READ_TOP_SOFT_RESET_REG_3r(unit, &top_sreset_reg3);
    TOP_SOFT_RESET_REG_3r_TOP_PM0_RST_Lf_SET(top_sreset_reg3, 1);
    TOP_SOFT_RESET_REG_3r_TOP_PM1_RST_Lf_SET(top_sreset_reg3, 1);
    TOP_SOFT_RESET_REG_3r_TOP_PM2_RST_Lf_SET(top_sreset_reg3, 1);
    TOP_SOFT_RESET_REG_3r_TOP_PM3_RST_Lf_SET(top_sreset_reg3, 1);
    TOP_SOFT_RESET_REG_3r_TOP_PM4_RST_Lf_SET(top_sreset_reg3, 1);
    TOP_SOFT_RESET_REG_3r_TOP_PM5_RST_Lf_SET(top_sreset_reg3, 1);
    TOP_SOFT_RESET_REG_3r_TOP_PM6_RST_Lf_SET(top_sreset_reg3, 1);
    TOP_SOFT_RESET_REG_3r_TOP_PM7_RST_Lf_SET(top_sreset_reg3, 1);
    TOP_SOFT_RESET_REG_3r_TOP_PM8_RST_Lf_SET(top_sreset_reg3, 1);
    TOP_SOFT_RESET_REG_3r_TOP_PM9_RST_Lf_SET(top_sreset_reg3, 1);
    TOP_SOFT_RESET_REG_3r_TOP_PM10_RST_Lf_SET(top_sreset_reg3, 1);
    TOP_SOFT_RESET_REG_3r_TOP_PM11_RST_Lf_SET(top_sreset_reg3, 1);
    TOP_SOFT_RESET_REG_3r_TOP_PM12_RST_Lf_SET(top_sreset_reg3, 1);
    TOP_SOFT_RESET_REG_3r_TOP_PM13_RST_Lf_SET(top_sreset_reg3, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REG_3r(unit, top_sreset_reg3);
    BMD_SYS_USLEEP(wait_usec);
    
    ioerr += READ_TOP_PVTMON_CTRL_1r(unit, &pvtmon_ctrl);
    TOP_PVTMON_CTRL_1r_PVTMON_ADC_RESETBf_SET(pvtmon_ctrl, 1);
    ioerr += WRITE_TOP_PVTMON_CTRL_1r(unit, pvtmon_ctrl);
    TOP_PVTMON_CTRL_1r_PVTMON_ADC_RESETBf_SET(pvtmon_ctrl, 0);
    ioerr += WRITE_TOP_PVTMON_CTRL_1r(unit, pvtmon_ctrl);
    TOP_PVTMON_CTRL_1r_PVTMON_ADC_RESETBf_SET(pvtmon_ctrl, 1);
    ioerr += WRITE_TOP_PVTMON_CTRL_1r(unit, pvtmon_ctrl);

    /* Bypass unused Port Blocks */
    _soc_tsc_disable(unit);
   
    _soc_tsc_reset(unit);

    return ioerr ? CDK_E_IO : CDK_E_NONE;

}
#endif /* CDK_CONFIG_INCLUDE_BCM56560_A0 */

