/*
 *
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56670_A0 == 1

#include <bmd/bmd.h>

#include <cdk/chip/bcm56670_a0_defs.h>
#include <cdk/cdk_debug.h>

#include "bcm56670_a0_bmd.h"
#include "bcm56670_a0_internal.h"

/*
 * Function: _soc_tsc_disable
 *
 * Purpose:
 *     Function to dsiable unused TSCs in the chip.
 */
static void
_tsc_disable(int unit)
{
    uint32_t tsc_enable = 0;
    int lane, tsc, port;
    cdk_pbmp_t clpbmp;
    TOP_TSC_ENABLEr_t top_tsc;

    bcm56670_a0_clport_pbmp_get(unit, &clpbmp);
    for (port = 1; port <= PORTS_PER_PIPE; port = (port + PORTS_PER_XLP)) {

        tsc = (port - 1) / PORTS_PER_XLP;
        for (lane = 0; lane < PORTS_PER_XLP; lane++) {
            if (P2L(unit, port) != -1) {
                tsc_enable |= (1 << tsc);
                if (CDK_PBMP_MEMBER(clpbmp, port)) {
                    tsc_enable |= (1 << (tsc + 1));
                    tsc_enable |= (1 << (tsc + 2));
                }
                break;
            }
        }
    }
    /* TSC2/3/6/9/12/13 are master PLL cores. Never disable them */
    tsc_enable |= 0x324c;
    
    TOP_TSC_ENABLEr_SET(top_tsc, tsc_enable);
    WRITE_TOP_TSC_ENABLEr(unit, top_tsc);
}


/*
 * function: _tsc_xgxs_reset
 * purpose:  Reset all TSCs associated with the passed port.
*/
static void
_tsc_xgxs_reset(int unit, int port, int reg_idx)
{
    int ioerr = 0;
    cdk_pbmp_t clpbmp, xlpbmp;
    CLPORT_XGXS0_CTRL_REGr_t clport_ctrl;
    XLPORT_XGXS0_CTRL_REGr_t xl_ctrl;
    int sleep_usec = 100000;

    bcm56670_a0_clport_pbmp_get(unit, &clpbmp);
    bcm56670_a0_xlport_pbmp_get(unit, &xlpbmp);

    if (CDK_PBMP_MEMBER(clpbmp, port)) {
        /*
         * Reference clock selection
         */
        ioerr += READ_CLPORT_XGXS0_CTRL_REGr(unit, &clport_ctrl, port);
        CLPORT_XGXS0_CTRL_REGr_REFIN_ENf_SET(clport_ctrl, 1);
        ioerr += WRITE_CLPORT_XGXS0_CTRL_REGr(unit, clport_ctrl, port);

        /*
         * port init sequence must be adjusted to bring tsc out of reset properly
         */
        CLPORT_XGXS0_CTRL_REGr_IDDQf_SET(clport_ctrl, 0);

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
        ioerr += READ_XLPORT_XGXS0_CTRL_REGr(unit, &xl_ctrl, port);
        XLPORT_XGXS0_CTRL_REGr_REFIN_ENf_SET(xl_ctrl, 1);
        ioerr += WRITE_XLPORT_XGXS0_CTRL_REGr(unit, xl_ctrl, port);
        BMD_SYS_USLEEP(sleep_usec);

        /* Deassert power down */
        XLPORT_XGXS0_CTRL_REGr_PWRDWNf_SET(xl_ctrl, 0);
        ioerr += WRITE_XLPORT_XGXS0_CTRL_REGr(unit, xl_ctrl, port);
        BMD_SYS_USLEEP(sleep_usec);

        /* Reset XGXS */
        XLPORT_XGXS0_CTRL_REGr_RSTB_HWf_SET(xl_ctrl, 0);
        ioerr += WRITE_XLPORT_XGXS0_CTRL_REGr(unit, xl_ctrl, port);
        BMD_SYS_USLEEP(sleep_usec);

        /* Bring XGXS out of reset */
        XLPORT_XGXS0_CTRL_REGr_RSTB_HWf_SET(xl_ctrl, 1);
        ioerr += WRITE_XLPORT_XGXS0_CTRL_REGr(unit, xl_ctrl, port);
        BMD_SYS_USLEEP(sleep_usec);
    }
}
#if 0
static cdk_pbmp_t pm_ref_master_bitmap;
#endif
typedef struct tsc_dpll_refclk_connect
{
    int pll0_refclk_select;      /* 0: select PAD ref clock; 1: select internal LC ref clock, refer to pll0_lc_refclk_select*/
    int pll0_master_enable;      /* 0: disable; 1: enable ref clock output for other PLL */
    int pll0_lc_refclk_select;   /* 0: select pll0_lc_refclk; 1 select pll1_lc_refclk */
    int pll1_refclk_select;      /* 0: select PAD ref clock; 1: select internal LC ref clock, refer to pll1_lc_refclk_select*/
    int pll1_master_enable;      /* 0: disable; 1: enable ref clock output for other PLL */
    int pll1_lc_refclk_select;   /* 0: select pll0_lc_refclk; 1: select pll1_lc_refclk */
} tsc_dpll_refclk_connect_t;

#define PM_NUM  16

/* CPM cores: TSC0 - TSC2, TSC13 - TSC15               */
/*           select pll0_lc_refclk (122.88MHz) as CPRI */
/*           select pll1_lc_refclk (156.25MHz) as ETH  */
/* PM4X25 cores: TSC3 - TSC4, TSC11 - TSC12            */
/*           TSC3/TSC12 are master PLL core            */
/*           select pll0_lc_refclk                     */
/* PM4X10 cores: TSC5 - TSC7, TSC8 - TSC10             */
/*           TSC6/TSC9 are master PLL core             */
/*           select pll0_lc_refclk                     */
/* This table is for all CPM cores working in Ethernet */
tsc_dpll_refclk_connect_t monterey_tsc_refclk_connect[PM_NUM] =
{
    /* TSC0 CPM4X25 */
    /* PLL0/PLL1 select LC refclk;  disable refclk output; select pll1_lc_refclk */
    {1, 0, 1,  1, 0, 1},
    /* TSC1 CPM4X25 */
    /* PLL0/PLL1 select LC refclk,  disable refclk output, select pll1_lc_refclk */
    {1, 0, 1,  1, 0, 1},
    /* TSC2 CPM4X25 PLL1 master PLL*/
    /* PLL0 select LC refclk,  disable refclk output, select pll1_lc_refclk */
    /* PLL1 select LC refclk,  enable refclk output, select pll1_lc_refclk */
    {1, 0, 1,  1, 1, 1},
    /* TSC3 PM4X25 PLL0 master PLL*/
    /* PLL0 select LC refclk,  enable refclk output, select pll0_lc_refclk */
    /* PLL1 select LC refclk,  disable refclk output, select pll0_lc_refclk */
    {1, 1, 0,  1, 0, 0},
    /* TSC4 PM4X25 */
    /* PLL0/PLL1 select LC refclk,  disable refclk output, select pll0_lc_refclk */
    {1, 0, 0,  1, 0, 0},
    /* TSC5 PM4X10 */
    /* PLL0/PLL1 select LC refclk,  disable refclk output, select pll0_lc_refclk */
    {1, 0, 0,  1, 0, 0},
    /* TSC6 PM4X10  PLL0/PLL1 master pll*/
    /* PLL0/PLL1 select LC refclk,  enable refclk output, select pll0_lc_refclk */
    {1, 1, 0,  1, 1, 0},
    /* TSC7 PM4X10 */
    /* PLL0/PLL1 select LC refclk,  disable refclk output, select pll0_lc_refclk */
    {1, 0, 0,  1, 0, 0},
    /* TSC8 PM4X10 */
    /* PLL0/PLL1 select LC refclk,  disable refclk output, select pll0_lc_refclk */
    {1, 0, 0,  1, 0, 0},
    /* TSC9 PM4X10  PLL0/PLL1 master pll*/
    /* PLL0/PLL1 select LC refclk,  enable refclk output, select pll0_lc_refclk */
    {1, 1, 0,  1, 1, 0},
    /* TSC10 PM4X10 */
    /* PLL0/PLL1 select LC refclk,  disable refclk output, select pll0_lc_refclk */
    {1, 0, 0,  1, 0, 0},
    /* TSC11 PM4X25 */
    /* PLL0/PLL1 select LC refclk,  disable refclk output, select pll0_lc_refclk */
    {1, 0, 0,  1, 0, 0},
    /* TSC12 PM4X25 PLL0 master PLL*/
    /* PLL0 select LC refclk,  enable refclk output, select pll0_lc_refclk */
    /* PLL1 select LC refclk,  disable refclk output, select pll0_lc_refclk */
    {1, 1, 0,  1, 0, 0},
    /* TSC13 CPM4X25 PLL1 master PLL*/
    /* PLL0 select LC refclk,  disable refclk output, select pll1_lc_refclk */
    /* PLL1 select LC refclk,  enable refclk output, select pll1_lc_refclk */
    {1, 0, 1,  1, 1, 1},
    /* TSC14 CPM4X25 */
    /* PLL0/PLL1 select LC refclk,  disable refclk output, select pll1_lc_refclk */
    {1, 0, 1,  1, 0, 1},
    /* TSC15 CPM4X25 */
    /* PLL0/PLL1 select LC refclk,  disable refclk output, select pll1_lc_refclk */
    {1, 0, 1,  1, 0, 1}
};

#define monterey_tsc_is_master_pll(pm)  \
     monterey_tsc_refclk_connect[pm].pll0_master_enable || \
     monterey_tsc_refclk_connect[pm].pll1_master_enable

static int
_tsc_refclk_set(int unit)
{
    int ioerr = 0;
    int port, pm;
    tsc_dpll_refclk_connect_t *refclk_connect;
    int sleep_usec = 1100;
#if 0
    int bidx, subport;
    int obm;
#endif
    CLPORT_XGXS0_CTRL_REGr_t clport_ctrl;
    XLPORT_XGXS0_CTRL_REGr_t xlport_ctrl;
    CLPORT_PMD_PLL0_CTRL_CONFIGr_t clp_pll0;
    CLPORT_PMD_PLL1_CTRL_CONFIGr_t clp_pll1;
    XLPORT_PMD_PLL_CTRL_CONFIGr_t xlp_pll;
#if 0
    pm_ref_master_bitmap = 0; 
#endif
    for(pm = 0; pm < PM_NUM; pm++) {
        port = pm * 4 + 1;
        refclk_connect = &monterey_tsc_refclk_connect[pm];

        /* set refclock connect */
        if (IS_FALCON(unit, port)) {
            ioerr += READ_CLPORT_XGXS0_CTRL_REGr(unit, &clport_ctrl, port);
            CLPORT_XGXS0_CTRL_REGr_PLL0_REFIN_ENf_SET(clport_ctrl, refclk_connect->pll0_refclk_select);
            CLPORT_XGXS0_CTRL_REGr_PLL0_REFOUT_ENf_SET(clport_ctrl, refclk_connect->pll0_master_enable);
            CLPORT_XGXS0_CTRL_REGr_PLL0_LC_REFSELf_SET(clport_ctrl, refclk_connect->pll0_lc_refclk_select);

            CLPORT_XGXS0_CTRL_REGr_PLL1_REFIN_ENf_SET(clport_ctrl, refclk_connect->pll1_refclk_select);
            CLPORT_XGXS0_CTRL_REGr_PLL1_REFOUT_ENf_SET(clport_ctrl, refclk_connect->pll1_master_enable);
            CLPORT_XGXS0_CTRL_REGr_PLL1_LC_REFSELf_SET(clport_ctrl, refclk_connect->pll1_lc_refclk_select);
            ioerr += WRITE_CLPORT_XGXS0_CTRL_REGr(unit, clport_ctrl, port);

            /* OOS master PLL core */
            if(monterey_tsc_is_master_pll(pm)) {
                /* De-assert IDDQ */
                CLPORT_XGXS0_CTRL_REGr_IDDQf_SET(clport_ctrl, 0);
                ioerr += WRITE_CLPORT_XGXS0_CTRL_REGr(unit, clport_ctrl, port);
                
                /* De-assert power down */
                CLPORT_XGXS0_CTRL_REGr_PWRDWNf_SET(clport_ctrl, 0);
                ioerr += WRITE_CLPORT_XGXS0_CTRL_REGr(unit, clport_ctrl, port);
                BMD_SYS_USLEEP(sleep_usec);

                /* Reset XGXS */
                CLPORT_XGXS0_CTRL_REGr_RSTB_HWf_SET(clport_ctrl, 0);
                ioerr += WRITE_CLPORT_XGXS0_CTRL_REGr(unit, clport_ctrl, port);
                BMD_SYS_USLEEP(sleep_usec + 10000);

                /* Bring XGXS out of reset */
                CLPORT_XGXS0_CTRL_REGr_RSTB_HWf_SET(clport_ctrl, 0);
                ioerr += WRITE_CLPORT_XGXS0_CTRL_REGr(unit, clport_ctrl, port);
#if 0

                /* Mark as master pll core */
                bidx = bcm56670_a0_block_index_get(unit, port, &obm, &subport);
                pm_ref_master_bitmap |= 1 << bidx;
#endif
            }
            
            /* set internal termination to 200 Ohm */
            if (refclk_connect->pll0_master_enable) {
                ioerr += READ_CLPORT_PMD_PLL0_CTRL_CONFIGr(unit, &clp_pll0, port);
                CLPORT_PMD_PLL0_CTRL_CONFIGr_PLL0_RTERM200f_SET(clp_pll0, 1);
                ioerr += WRITE_CLPORT_PMD_PLL0_CTRL_CONFIGr(unit, clp_pll0, port);
            }

            if (refclk_connect->pll1_master_enable) {
                ioerr += READ_CLPORT_PMD_PLL1_CTRL_CONFIGr(unit, &clp_pll1, port);
                CLPORT_PMD_PLL1_CTRL_CONFIGr_PLL1_RTERM200f_SET(clp_pll1, 1);
                ioerr += WRITE_CLPORT_PMD_PLL1_CTRL_CONFIGr(unit, clp_pll1, port);
            }
        } else {
            ioerr += READ_XLPORT_XGXS0_CTRL_REGr(unit, &xlport_ctrl, port);
            XLPORT_XGXS0_CTRL_REGr_PLL0_REFIN_ENf_SET(xlport_ctrl, refclk_connect->pll0_refclk_select);
            XLPORT_XGXS0_CTRL_REGr_PLL0_REFOUT_ENf_SET(xlport_ctrl, refclk_connect->pll0_master_enable);
            XLPORT_XGXS0_CTRL_REGr_PLL0_LC_REFSELf_SET(xlport_ctrl, refclk_connect->pll0_lc_refclk_select);

            XLPORT_XGXS0_CTRL_REGr_PLL1_REFIN_ENf_SET(xlport_ctrl, refclk_connect->pll1_refclk_select);
            XLPORT_XGXS0_CTRL_REGr_PLL1_REFOUT_ENf_SET(xlport_ctrl, refclk_connect->pll1_master_enable);
            XLPORT_XGXS0_CTRL_REGr_PLL1_LC_REFSELf_SET(xlport_ctrl, refclk_connect->pll1_lc_refclk_select);
            ioerr += WRITE_XLPORT_XGXS0_CTRL_REGr(unit, xlport_ctrl, port);

            /* OOS master PLL core */
            if(monterey_tsc_is_master_pll(pm)) {
                /* De-assert IDDQ */
                XLPORT_XGXS0_CTRL_REGr_IDDQf_SET(xlport_ctrl, 0);
                ioerr += WRITE_XLPORT_XGXS0_CTRL_REGr(unit, xlport_ctrl, port);
                
                /* De-assert power down */
                XLPORT_XGXS0_CTRL_REGr_PWRDWNf_SET(xlport_ctrl, 0);
                ioerr += WRITE_XLPORT_XGXS0_CTRL_REGr(unit, xlport_ctrl, port);
                BMD_SYS_USLEEP(sleep_usec);

                /* Reset XGXS */
                XLPORT_XGXS0_CTRL_REGr_RSTB_HWf_SET(xlport_ctrl, 0);
                ioerr += WRITE_XLPORT_XGXS0_CTRL_REGr(unit, xlport_ctrl, port);
                BMD_SYS_USLEEP(sleep_usec + 10000);

                /* Bring XGXS out of reset */
                XLPORT_XGXS0_CTRL_REGr_RSTB_HWf_SET(xlport_ctrl, 0);
                ioerr += WRITE_XLPORT_XGXS0_CTRL_REGr(unit, xlport_ctrl, port);
#if 0

                /* Mark as master pll core */
                bidx = bcm56670_a0_block_index_get(unit, port, &obm, &subport);
                pm_ref_master_bitmap |= 1 << bidx;
#endif
            }
            
            /* set internal termination to 200 Ohm */
            if (refclk_connect->pll0_master_enable || refclk_connect->pll1_master_enable) {
                ioerr += READ_XLPORT_PMD_PLL_CTRL_CONFIGr(unit, &xlp_pll, port);
                XLPORT_PMD_PLL_CTRL_CONFIGr_REFCLK_TERM_SELf_SET(xlp_pll, 1);
                ioerr += WRITE_XLPORT_PMD_PLL_CTRL_CONFIGr(unit, xlp_pll, port);
            }
        }
    }
    
    return CDK_E_NONE;
}

static void
_tsc_reset(int unit)
{
    cdk_pbmp_t pbmp;
    int port;
    int bidx;
    int obm, subport;
    CLPORT_MAC_CONTROLr_t cl_mac_ctrl;
    XLPORT_MAC_CONTROLr_t xl_mac_ctrl;
    CLPORT_XGXS0_CTRL_REGr_t clport_ctrl;
    TOP_CTRL_CONFIGr_t top_ctrl_cfg;
    CPMPORT_PMD_CTRLr_t cpm_pmd_ctrl;
    CPRI_RST_CTRLr_t cpi_rst;

    /* Set tsc reference clock connection */
    _tsc_refclk_set(unit);

    bcm56670_a0_clport_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        if (((port - 1) % 4) != 0) {
            continue;
        }
        if(!(monterey_tsc_is_master_pll(port))) {
            _tsc_xgxs_reset(unit, port, 0);
        }
        
        READ_CLPORT_MAC_CONTROLr(unit, &cl_mac_ctrl, port);
        CLPORT_MAC_CONTROLr_XMAC0_RESETf_SET(cl_mac_ctrl, 0);
        WRITE_CLPORT_MAC_CONTROLr(unit, cl_mac_ctrl, port);
    }

    bcm56670_a0_xlport_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        if (((port - 1) % 4) != 0) {
            /* already done */
            continue;
        }
        
        if(!(monterey_tsc_is_master_pll(port))) {
            _tsc_xgxs_reset(unit, port, 0);
        }
        READ_XLPORT_MAC_CONTROLr(unit, &xl_mac_ctrl, port);
        XLPORT_MAC_CONTROLr_XMAC0_RESETf_SET(xl_mac_ctrl, 0);
        WRITE_XLPORT_MAC_CONTROLr(unit, xl_mac_ctrl, port);
    }
    
    bcm56670_a0_cpri_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        /* PLL0 select pll0_lc_refclk for CPRI, keep PLL1 pll1_lc_refclk for Ethernet */
        READ_CLPORT_XGXS0_CTRL_REGr(unit, &clport_ctrl, port);
        CLPORT_XGXS0_CTRL_REGr_PLL0_LC_REFSELf_SET(clport_ctrl, 0);
        WRITE_CLPORT_XGXS0_CTRL_REGr(unit, clport_ctrl, port);

        bidx = bcm56670_a0_cpri_block_index_get(unit, port, &obm, &subport);
        READ_TOP_CTRL_CONFIGr(unit, bidx, &top_ctrl_cfg);
        TOP_CTRL_CONFIGr_CIP_TOP_CTRLf_SET(top_ctrl_cfg, 0);
        TOP_CTRL_CONFIGr_PMD_IDDQf_SET(top_ctrl_cfg, 0);
        TOP_CTRL_CONFIGr_CIP_TOP_CTRLf_SET(top_ctrl_cfg, 0);
        WRITE_TOP_CTRL_CONFIGr(unit, bidx, top_ctrl_cfg);
        READ_CPMPORT_PMD_CTRLr(unit, bidx, &cpm_pmd_ctrl);
        CPMPORT_PMD_CTRLr_PMD_POR_H_RSTBf_SET(cpm_pmd_ctrl, 0);
        WRITE_CPMPORT_PMD_CTRLr(unit, bidx, cpm_pmd_ctrl);

        READ_CPRI_RST_CTRLr(unit, port, &cpi_rst);
        CPRI_RST_CTRLr_RESET_TX_Hf_SET(cpi_rst, 0);
        CPRI_RST_CTRLr_RESET_RX_Hf_SET(cpi_rst, 0);
        CPRI_RST_CTRLr_RESET_CIP_TX_Hf_SET(cpi_rst, 0);
        CPRI_RST_CTRLr_RESET_CIP_RX_Hf_SET(cpi_rst, 0);
        CPRI_RST_CTRLr_RESET_DATAPATH_RX_Hf_SET(cpi_rst, 0);
        CPRI_RST_CTRLr_RESET_DATAPATH_TX_Hf_SET(cpi_rst, 0);
        WRITE_CPRI_RST_CTRLr(unit, port, cpi_rst);
    }
}

int
bcm56670_a0_bmd_reset(int unit)
{
    int ioerr = 0;
    int idx;
#if 0
    CMIC_CPS_RESETr_t cps_reset;
#endif
    uint32_t ring_map[] = { 0x43052100, 0x33333333, 0x44444333, 0x50444444,
                            0x00643055, 0x00000000, 0x00000000, 0x00000000 };
    CMIC_SBUS_RING_MAPr_t sbus_ring_map;
    CMIC_SBUS_TIMEOUTr_t sbus_to;
    TOP_PVTMON_CTRL_1r_t pvtmon_ctrl;
    int wait_usec = 10000;
    TOP_SOFT_RESET_REGr_t top_sreset_reg;
    TOP_SOFT_RESET_REG_2r_t top_sreset_2;
    TOP_BS_PLL_CTRL_3r_t top_bs_pll_ctrl3;
    TOP_PVTMON_CTRL_0r_t top_pvtmon_ctrl0;
    TOP_SOFT_RESET_REG_3r_t top_sreset_reg3;
    TOP_PVTMON_INTR_THRESHOLDr_t top_pvtmon_thr;
    TOP_PVTMON_MASKr_t top_pvtmon_mask;
    uint32_t temp_thr, rval;
    
    BMD_CHECK_UNIT(unit);

    /* Initialize endian mode for correct reset access */
    ioerr += cdk_xgsd_cmic_init(unit);
#if 0
    /* Pull reset line */
    ioerr += READ_CMIC_CPS_RESETr(unit, &cps_reset);
    CMIC_CPS_RESETr_CPS_RESETf_SET(cps_reset, 1);
    ioerr += WRITE_CMIC_CPS_RESETr(unit, cps_reset);
#endif
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
    TOP_PVTMON_CTRL_0r_MODEf_SET(top_pvtmon_ctrl0, 2);
    ioerr += WRITE_TOP_PVTMON_CTRL_0r(unit, top_pvtmon_ctrl0);

    /* Enable high temperature interrupt monitoring.
     * Default on: pvtmon6 (close to core_plls at center of die). */
    for (idx = 0; idx < 8; idx++) {
        temp_thr = (410040 - 130 * 1000) / 487;
        ioerr += READ_TOP_PVTMON_INTR_THRESHOLDr(unit, idx, &top_pvtmon_thr);
        TOP_PVTMON_INTR_THRESHOLDr_MAX_THRESHOLDf_SET(top_pvtmon_thr, temp_thr);
        ioerr += WRITE_TOP_PVTMON_INTR_THRESHOLDr(unit, idx, top_pvtmon_thr);
        
        /* 2 bits per pvtmon, using min */
        if ((1 << 6) & (1 << idx)) {
            rval |= (1 << (idx * 2 + 1)); 
        }
    }
    TOP_PVTMON_MASKr_SET(top_pvtmon_mask, rval);
    ioerr += WRITE_TOP_PVTMON_MASKr(unit, top_pvtmon_mask);

    /* Bring port blocks out of reset */
    TOP_SOFT_RESET_REGr_CLR(top_sreset_reg);
    TOP_SOFT_RESET_REGr_TOP_PGW0_RST_Lf_SET(top_sreset_reg, 1);
    TOP_SOFT_RESET_REGr_TOP_PGW1_RST_Lf_SET(top_sreset_reg, 1);
    TOP_SOFT_RESET_REGr_TOP_TS_RST_Lf_SET(top_sreset_reg, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REGr(unit, top_sreset_reg);
    BMD_SYS_USLEEP(wait_usec);

    /* Bring IP, EP, and MMU blocks out of reset */
    ioerr += READ_TOP_SOFT_RESET_REGr(unit, &top_sreset_reg);
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
    TOP_SOFT_RESET_REG_3r_TOP_PM14_RST_Lf_SET(top_sreset_reg3, 1);
    TOP_SOFT_RESET_REG_3r_TOP_PM15_RST_Lf_SET(top_sreset_reg3, 1);
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
    _tsc_disable(unit);

    _tsc_reset(unit);

    return ioerr ? CDK_E_IO : CDK_E_NONE;

}
#endif /* CDK_CONFIG_INCLUDE_BCM56670_A0 */

