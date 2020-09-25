/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56960_A0 == 1

#include <bmd/bmd.h>

#include <cdk/chip/bcm56960_a0_defs.h>
#include <cdk/cdk_debug.h>

#include "bcm56960_a0_bmd.h"
#include "bcm56960_a0_internal.h"

#define PLL_LOCK_MSEC                   100
#define RESET_SLEEP_USEC                100

#define NUM_PRIV_L2_BANKS               2
#define NUM_PRIV_L3_BANKS               4

static int
_wait_for_pll_lock(int unit)
{
    int ioerr = 0;
    int msec, idx, locks;
    TOP_XG_PLL_STATUSr_t pll_status[4];
    TOP_TS_PLL_STATUSr_t ts_pll_status;
    TOP_BS_PLL0_STATUSr_t bs_pll0_status;

    /* Wait for LC PLL locks */
    for (msec = 0; msec < PLL_LOCK_MSEC; msec++) {
#if BMD_CONFIG_SIMULATION
        if (msec == 0) break;
#endif
        locks = 0;
        for (idx = 0; idx < COUNTOF(pll_status); idx++) {
            ioerr += READ_TOP_XG_PLL_STATUSr(unit, idx, &pll_status[idx]);
            locks += TOP_XG_PLL_STATUSr_TOP_XGPLL_LOCKf_GET(pll_status[idx]);
        }
        if (locks == COUNTOF(pll_status)) {
            break;
        }
        BMD_SYS_USLEEP(1000);
    }
    if (msec >= PLL_LOCK_MSEC) {
        CDK_WARN(("bcm56960_a0_bmd_reset[%d]: LC PLL did not lock, status = ",
                  unit));
        for (idx = 0; idx < COUNTOF(pll_status); idx++) {
            CDK_WARN(("0x%08"PRIx32" ",
                      TOP_XG_PLL_STATUSr_GET(pll_status[idx])));
        }
        CDK_WARN(("\n"));
    }

    /* Wait for timesync/broadsync PLL locks */
    for (msec = 0; msec < PLL_LOCK_MSEC; msec++) {
#if BMD_CONFIG_SIMULATION
        if (msec == 0) break;
#endif
        ioerr += READ_TOP_TS_PLL_STATUSr(unit, &ts_pll_status);
        ioerr += READ_TOP_BS_PLL0_STATUSr(unit, &bs_pll0_status);
        if (TOP_TS_PLL_STATUSr_PLL_LOCKf_GET(ts_pll_status) == 1 &&
            TOP_BS_PLL0_STATUSr_PLL_LOCKf_GET(bs_pll0_status) == 1) {
            break;
        }
        BMD_SYS_USLEEP(1000);
    }
    if (msec >= PLL_LOCK_MSEC) {
        CDK_WARN(("bcm56960_a0_bmd_reset[%d]: "
                  "TS/BS PLL did not lock, status = "
                  "0x%08"PRIx32" 0x%08"PRIx32"\n",
                  unit,
                  TOP_TS_PLL_STATUSr_GET(ts_pll_status), 
                  TOP_BS_PLL0_STATUSr_GET(bs_pll0_status)));
    }

    return ioerr;
}

static int
_clport_reset(int unit, int port)
{
    int ioerr = 0;
    CLPORT_MAC_CONTROLr_t cmac_ctrl;
    CLPORT_XGXS0_CTRL_REGr_t xgxs_ctrl;

    ioerr += READ_CLPORT_MAC_CONTROLr(unit, &cmac_ctrl, port);
    CLPORT_MAC_CONTROLr_XMAC0_RESETf_SET(cmac_ctrl, 1);
    ioerr += WRITE_CLPORT_MAC_CONTROLr(unit, cmac_ctrl, port);
    BMD_SYS_USLEEP(10);
    CLPORT_MAC_CONTROLr_XMAC0_RESETf_SET(cmac_ctrl, 0);
    ioerr += WRITE_CLPORT_MAC_CONTROLr(unit, cmac_ctrl, port);

    /* Configure clock source */
    ioerr += READ_CLPORT_XGXS0_CTRL_REGr(unit, &xgxs_ctrl, port);
    CLPORT_XGXS0_CTRL_REGr_REFIN_ENf_SET(xgxs_ctrl, 1);
    ioerr += WRITE_CLPORT_XGXS0_CTRL_REGr(unit, xgxs_ctrl, port);
     
    /* Deassert power down */
    ioerr += READ_CLPORT_XGXS0_CTRL_REGr(unit, &xgxs_ctrl, port);
    CLPORT_XGXS0_CTRL_REGr_PWRDWNf_SET(xgxs_ctrl, 0);
    ioerr += WRITE_CLPORT_XGXS0_CTRL_REGr(unit, xgxs_ctrl, port);
    BMD_SYS_USLEEP(RESET_SLEEP_USEC);
    
    /* Reset XGXS */
    ioerr += READ_CLPORT_XGXS0_CTRL_REGr(unit, &xgxs_ctrl, port);
    CLPORT_XGXS0_CTRL_REGr_RSTB_HWf_SET(xgxs_ctrl, 0);
    ioerr += WRITE_CLPORT_XGXS0_CTRL_REGr(unit, xgxs_ctrl, port);
    BMD_SYS_USLEEP(RESET_SLEEP_USEC + 10000);

    /* Bring XGXS out of reset */
    CLPORT_XGXS0_CTRL_REGr_RSTB_HWf_SET(xgxs_ctrl, 1);
    ioerr += WRITE_CLPORT_XGXS0_CTRL_REGr(unit, xgxs_ctrl, port);
    BMD_SYS_USLEEP(RESET_SLEEP_USEC);

    return ioerr;
}

static int
_xlport_reset(int unit, int port)
{
    int ioerr = 0;
    XLPORT_MAC_CONTROLr_t xmac_ctrl;
    XLPORT_XGXS0_CTRL_REGr_t xgxs_ctrl;

    ioerr += READ_XLPORT_MAC_CONTROLr(unit, &xmac_ctrl, port);
    XLPORT_MAC_CONTROLr_XMAC0_RESETf_SET(xmac_ctrl, 1);
    ioerr += WRITE_XLPORT_MAC_CONTROLr(unit, xmac_ctrl, port);
    BMD_SYS_USLEEP(10);
    XLPORT_MAC_CONTROLr_XMAC0_RESETf_SET(xmac_ctrl, 0);
    ioerr += WRITE_XLPORT_MAC_CONTROLr(unit, xmac_ctrl, port);

    /* Configure clock source */
    ioerr += READ_XLPORT_XGXS0_CTRL_REGr(unit, &xgxs_ctrl, port);
    XLPORT_XGXS0_CTRL_REGr_REFIN_ENf_SET(xgxs_ctrl, 1);
    ioerr += WRITE_XLPORT_XGXS0_CTRL_REGr(unit, xgxs_ctrl, port);
     
    /* Deassert power down */
    ioerr += READ_XLPORT_XGXS0_CTRL_REGr(unit, &xgxs_ctrl, port);
    XLPORT_XGXS0_CTRL_REGr_PWRDWNf_SET(xgxs_ctrl, 0);
    ioerr += WRITE_XLPORT_XGXS0_CTRL_REGr(unit, xgxs_ctrl, port);
    BMD_SYS_USLEEP(RESET_SLEEP_USEC);
    
    /* Reset XGXS */
    ioerr += READ_XLPORT_XGXS0_CTRL_REGr(unit, &xgxs_ctrl, port);
    XLPORT_XGXS0_CTRL_REGr_RSTB_HWf_SET(xgxs_ctrl, 0);
    ioerr += WRITE_XLPORT_XGXS0_CTRL_REGr(unit, xgxs_ctrl, port);
    BMD_SYS_USLEEP(RESET_SLEEP_USEC + 10000);

    /* Bring XGXS out of reset */
    XLPORT_XGXS0_CTRL_REGr_RSTB_HWf_SET(xgxs_ctrl, 1);
    ioerr += WRITE_XLPORT_XGXS0_CTRL_REGr(unit, xgxs_ctrl, port);
    BMD_SYS_USLEEP(RESET_SLEEP_USEC);

    return ioerr;
}

/* Get number of banks for the hash table according to the config */
static int
_hash_bank_count_get(int unit, BCM56960_A0_ENUM_t mem, int *num_banks)
{
    int count;

    switch (mem) {
    case L2Xm_ENUM:
        /*
         * 4k entries in each of 2 dedicated L2 bank
         * 32k entries in each shared bank
         */
        count = L2Xm_MAX - L2Xm_MIN + 1;;
        *num_banks = 2 + (count - 2 * 4 * 1024) / (32 * 1024);
        break;
    case L3_ENTRY_ONLYm_ENUM:
    case L3_ENTRY_IPV4_UNICASTm_ENUM:
    case L3_ENTRY_IPV4_MULTICASTm_ENUM:
    case L3_ENTRY_IPV6_UNICASTm_ENUM:
    case L3_ENTRY_IPV6_MULTICASTm_ENUM:
        /*
         * 2k entries in each of 4 dedicated L3 bank
         * 32k entries in each shared bank
         */
        count = L3_ENTRY_ONLYm_MAX - L3_ENTRY_ONLYm_MIN + 1;
        *num_banks = 4 + (count - 4 * 2 * 1024) / (32 * 1024);
        break;
    case EXACT_MATCH_2m_ENUM:
    case EXACT_MATCH_4m_ENUM:
        /*
         * 16k entries in each shared bank
         */
        count = EXACT_MATCH_2m_MAX - EXACT_MATCH_2m_MIN + 1;
        *num_banks = count / (16 * 1024);
        break;
    case MPLS_ENTRYm_ENUM:
    case VLAN_XLATEm_ENUM:
    case VLAN_MACm_ENUM:
    case EGR_VLAN_XLATEm_ENUM:
    case ING_VP_VLAN_MEMBERSHIPm_ENUM:
    case EGR_VP_VLAN_MEMBERSHIPm_ENUM:
    case ING_DNAT_ADDRESS_TYPEm_ENUM:
        *num_banks = 2;
        break;
    default:
        return CDK_E_INTERNAL;
    }

    return CDK_E_NONE;
}

int 
bcm56960_a0_bmd_reset(int unit)
{
    int ioerr = 0;
    int rv;
    int wait_usec = 10000;
    int idx, port;
    int num_banks, shared_bank, shared_bank_offset = 0;
    int num_shared_l2_banks, num_shared_l3_banks, num_shared_fpem_banks;
    cdk_pbmp_t pbmp;
    CMIC_SBUS_RING_MAPr_t sbus_ring_map;
    CMIC_SBUS_TIMEOUTr_t sbus_to;
    CMIC_CPS_RESETr_t cps_reset;
    TOP_SOFT_RESET_REGr_t top_sreset;
    TOP_SOFT_RESET_REG_2r_t top_sreset_2;
    TOP_SOFT_RESET_REG_3r_t top_sreset_3;
    TOP_CORE_PLL1_CTRL_1r_t core_pll_ctrl_1;
    TOP_XG_PLL_CTRL_0r_t pll_ctrl_0;
    TOP_XG_PLL_CTRL_4r_t pll_ctrl_4;
    TOP_TS_PLL_CTRL_0r_t ts_pll_ctrl_0;
    TOP_TS_PLL_CTRL_2r_t ts_pll_ctrl_2;
    TOP_TS_PLL_CTRL_3r_t ts_pll_ctrl_3;
    TOP_TS_PLL_CTRL_4r_t ts_pll_ctrl_4;
    TOP_BS_PLL0_CTRL_0r_t bs_pll0_ctrl_0;
    TOP_BS_PLL0_CTRL_2r_t bs_pll0_ctrl_2;
    TOP_BS_PLL0_CTRL_3r_t bs_pll0_ctrl_3;
    TOP_BS_PLL0_CTRL_4r_t bs_pll0_ctrl_4;
    TOP_BS_PLL1_CTRL_0r_t bs_pll1_ctrl_0;
    TOP_BS_PLL1_CTRL_2r_t bs_pll1_ctrl_2;
    TOP_BS_PLL1_CTRL_3r_t bs_pll1_ctrl_3;
    TOP_BS_PLL1_CTRL_4r_t bs_pll1_ctrl_4;
    TOP_PVTMON_CTRL_0r_t pvtmon_ctrl;
    ISS_BANK_CONFIGr_t bank_cfg;
    ISS_LOG_TO_PHY_BANK_MAPr_t phy_bank_map;
    ISS_LOG_TO_PHY_BANK_MAP_2r_t phy_bank_map_2;
    ISS_LOG_TO_PHY_BANK_MAP_3r_t phy_bank_map_3;
    ISS_MEMORY_CONTROL_84r_t mem_ctrl_84;
    uint32_t ring_map[] = { 0x55222100, 0x30004005, 0x43333333, 0x44444444,
                            0x34444444, 0x03333333, 0x30000000, 0x00005000 };

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
     * ring 0: IP(1), LBPORT0(54), LBPORT1(51), LBPORT2(52), LBPORT3(53)
     * ring 1: EP(2)
     * ring 2: MMU_XPE(3), MMU_SC(4), MMU_GLB(5)
     * ring 3: PM7(22), PM6(21), PM5(20), PM4(19), PM3(18), PM2(17), PM1(16),
     *           PM0(15), PM31(46), PM30(45), PM29(44), PM28(43), PM27(42),
     *           PM26(41), PM25(40), PM24(39), CLPORT32(55)
     * ring 4: PM32(11), PM8(23), PM9(24), PM10(25), PM11(26), PM12(27),
     *           PM13(28), PM14(29), PM15(30), PM16(31), PM17(32), PM18(33),
     *           PM19(34), PM20(35), PM21(36), PM22(37), PM23(38)
     * ring 5: OTPC(6), AVS(59), TOP(7), SER(8)
     */
    for (idx = 0; idx < COUNTOF(ring_map); idx++) {
        CMIC_SBUS_RING_MAPr_SET(sbus_ring_map, ring_map[idx]);
        ioerr += WRITE_CMIC_SBUS_RING_MAPr(unit, idx, sbus_ring_map);
    }
    CMIC_SBUS_TIMEOUTr_SET(sbus_to, 0x7d0);
    ioerr += WRITE_CMIC_SBUS_TIMEOUTr(unit, sbus_to);

    /* Reset all blocks */
    TOP_SOFT_RESET_REGr_CLR(top_sreset);
    ioerr += WRITE_TOP_SOFT_RESET_REGr(unit, top_sreset);

    /* Power on core pLL1 */
    ioerr += READ_TOP_CORE_PLL1_CTRL_1r(unit, &core_pll_ctrl_1);
    TOP_CORE_PLL1_CTRL_1r_PWRONf_SET(core_pll_ctrl_1, 0);
    ioerr += WRITE_TOP_CORE_PLL1_CTRL_1r(unit, core_pll_ctrl_1);

    /* Program LCPLL frequency */
    for (idx = 0; idx < 4; idx++) {
        ioerr += READ_TOP_XG_PLL_CTRL_4r(unit, idx, &pll_ctrl_4);
        TOP_XG_PLL_CTRL_4r_POST_RST_SELf_SET(pll_ctrl_4, 3);
        ioerr += WRITE_TOP_XG_PLL_CTRL_4r(unit, idx, pll_ctrl_4);

        ioerr += READ_TOP_XG_PLL_CTRL_0r(unit, idx, &pll_ctrl_0);
        TOP_XG_PLL_CTRL_0r_PDIVf_SET(pll_ctrl_0, 1);
        TOP_XG_PLL_CTRL_0r_NDIV_INTf_SET(pll_ctrl_0, 20);
        ioerr += WRITE_TOP_XG_PLL_CTRL_0r(unit, idx, pll_ctrl_0);
    }

    /* Configure TS PLL */ 
    ioerr += READ_TOP_TS_PLL_CTRL_0r(unit, &ts_pll_ctrl_0);
    TOP_TS_PLL_CTRL_0r_POST_RESETB_SELECTf_SET(ts_pll_ctrl_0, 3);
    ioerr += WRITE_TOP_TS_PLL_CTRL_0r(unit, ts_pll_ctrl_0);

    ioerr += READ_TOP_TS_PLL_CTRL_2r(unit, &ts_pll_ctrl_2);
    TOP_TS_PLL_CTRL_2r_PDIVf_SET(ts_pll_ctrl_2, 2);
    TOP_TS_PLL_CTRL_2r_CH0_MDIVf_SET(ts_pll_ctrl_2, 5);
    ioerr += WRITE_TOP_TS_PLL_CTRL_2r(unit, ts_pll_ctrl_2);

    ioerr += READ_TOP_TS_PLL_CTRL_3r(unit, &ts_pll_ctrl_3);
    TOP_TS_PLL_CTRL_3r_NDIV_INTf_SET(ts_pll_ctrl_3, 100);
    TOP_TS_PLL_CTRL_3r_NDIV_FRACf_SET(ts_pll_ctrl_3, 0);
    ioerr += WRITE_TOP_TS_PLL_CTRL_3r(unit, ts_pll_ctrl_3);

    ioerr += READ_TOP_TS_PLL_CTRL_4r(unit, &ts_pll_ctrl_4);
    TOP_TS_PLL_CTRL_4r_KAf_SET(ts_pll_ctrl_4, 0);
    TOP_TS_PLL_CTRL_4r_KIf_SET(ts_pll_ctrl_4, 2);
    TOP_TS_PLL_CTRL_4r_KPf_SET(ts_pll_ctrl_4, 3);
    ioerr += WRITE_TOP_TS_PLL_CTRL_4r(unit, ts_pll_ctrl_4);

    /* Configure BS PLL0 */
    ioerr += READ_TOP_BS_PLL0_CTRL_0r(unit, &bs_pll0_ctrl_0);
    TOP_BS_PLL0_CTRL_0r_POST_RESETB_SELECTf_SET(bs_pll0_ctrl_0, 3);
    ioerr += WRITE_TOP_BS_PLL0_CTRL_0r(unit, bs_pll0_ctrl_0);

    ioerr += READ_TOP_BS_PLL0_CTRL_2r(unit, &bs_pll0_ctrl_2);
    TOP_BS_PLL0_CTRL_2r_PDIVf_SET(bs_pll0_ctrl_2, 2);
    TOP_BS_PLL0_CTRL_2r_CH0_MDIVf_SET(bs_pll0_ctrl_2, 125);
    ioerr += WRITE_TOP_BS_PLL0_CTRL_2r(unit, bs_pll0_ctrl_2);

    ioerr += READ_TOP_BS_PLL0_CTRL_3r(unit, &bs_pll0_ctrl_3);
    TOP_BS_PLL0_CTRL_3r_NDIV_INTf_SET(bs_pll0_ctrl_3, 100);
    TOP_BS_PLL0_CTRL_3r_NDIV_FRACf_SET(bs_pll0_ctrl_3, 0);
    ioerr += WRITE_TOP_BS_PLL0_CTRL_3r(unit, bs_pll0_ctrl_3);

    ioerr += READ_TOP_BS_PLL0_CTRL_4r(unit, &bs_pll0_ctrl_4);
    TOP_BS_PLL0_CTRL_4r_KAf_SET(bs_pll0_ctrl_4, 0);
    TOP_BS_PLL0_CTRL_4r_KIf_SET(bs_pll0_ctrl_4, 2);
    TOP_BS_PLL0_CTRL_4r_KPf_SET(bs_pll0_ctrl_4, 3);
    ioerr += WRITE_TOP_BS_PLL0_CTRL_4r(unit, bs_pll0_ctrl_4);

    /* Configure BS PLL1 */
    ioerr += READ_TOP_BS_PLL1_CTRL_0r(unit, &bs_pll1_ctrl_0);
    TOP_BS_PLL1_CTRL_0r_POST_RESETB_SELECTf_SET(bs_pll1_ctrl_0, 3);
    ioerr += WRITE_TOP_BS_PLL1_CTRL_0r(unit, bs_pll1_ctrl_0);

    ioerr += READ_TOP_BS_PLL1_CTRL_2r(unit, &bs_pll1_ctrl_2);
    TOP_BS_PLL1_CTRL_2r_PDIVf_SET(bs_pll1_ctrl_2, 2);
    TOP_BS_PLL1_CTRL_2r_CH0_MDIVf_SET(bs_pll1_ctrl_2, 125);
    ioerr += WRITE_TOP_BS_PLL1_CTRL_2r(unit, bs_pll1_ctrl_2);

    ioerr += READ_TOP_BS_PLL1_CTRL_3r(unit, &bs_pll1_ctrl_3);
    TOP_BS_PLL1_CTRL_3r_NDIV_INTf_SET(bs_pll1_ctrl_3, 100);
    TOP_BS_PLL1_CTRL_3r_NDIV_FRACf_SET(bs_pll1_ctrl_3, 0);
    ioerr += WRITE_TOP_BS_PLL1_CTRL_3r(unit, bs_pll1_ctrl_3);

    ioerr += READ_TOP_BS_PLL1_CTRL_4r(unit, &bs_pll1_ctrl_4);
    TOP_BS_PLL1_CTRL_4r_KAf_SET(bs_pll1_ctrl_4, 0);
    TOP_BS_PLL1_CTRL_4r_KIf_SET(bs_pll1_ctrl_4, 2);
    TOP_BS_PLL1_CTRL_4r_KPf_SET(bs_pll1_ctrl_4, 3);
    ioerr += WRITE_TOP_BS_PLL1_CTRL_4r(unit, bs_pll1_ctrl_4);

    /* Bring PLLs out of reset */
    ioerr += READ_TOP_SOFT_RESET_REG_2r(unit, &top_sreset_2);
    TOP_SOFT_RESET_REG_2r_TOP_XG_PLL0_RST_Lf_SET(top_sreset_2, 1);
    TOP_SOFT_RESET_REG_2r_TOP_XG_PLL1_RST_Lf_SET(top_sreset_2, 1);
    TOP_SOFT_RESET_REG_2r_TOP_XG_PLL2_RST_Lf_SET(top_sreset_2, 1);
    TOP_SOFT_RESET_REG_2r_TOP_XG_PLL3_RST_Lf_SET(top_sreset_2, 1);
    TOP_SOFT_RESET_REG_2r_TOP_TS_PLL_RST_Lf_SET(top_sreset_2, 1);
    TOP_SOFT_RESET_REG_2r_TOP_BS_PLL0_RST_Lf_SET(top_sreset_2, 1);
    TOP_SOFT_RESET_REG_2r_TOP_BS_PLL1_RST_Lf_SET(top_sreset_2, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REG_2r(unit, top_sreset_2);
    BMD_SYS_USLEEP(wait_usec);

    /* Initialize LCPLLs */
    ioerr += _wait_for_pll_lock(unit);

    /* De-assert LCPLL's post reset */
    ioerr += READ_TOP_SOFT_RESET_REG_2r(unit, &top_sreset_2);
    TOP_SOFT_RESET_REG_2r_TOP_XG_PLL0_POST_RST_Lf_SET(top_sreset_2, 1);
    TOP_SOFT_RESET_REG_2r_TOP_XG_PLL1_POST_RST_Lf_SET(top_sreset_2, 1);
    TOP_SOFT_RESET_REG_2r_TOP_XG_PLL2_POST_RST_Lf_SET(top_sreset_2, 1);
    TOP_SOFT_RESET_REG_2r_TOP_XG_PLL3_POST_RST_Lf_SET(top_sreset_2, 1);
    TOP_SOFT_RESET_REG_2r_TOP_TS_PLL_POST_RST_Lf_SET(top_sreset_2, 1);
    TOP_SOFT_RESET_REG_2r_TOP_BS_PLL0_POST_RST_Lf_SET(top_sreset_2, 1);
    TOP_SOFT_RESET_REG_2r_TOP_BS_PLL1_POST_RST_Lf_SET(top_sreset_2, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REG_2r(unit, top_sreset_2);
    BMD_SYS_USLEEP(wait_usec);

    /* Set correct value for BG_ADJ */
    ioerr += READ_TOP_PVTMON_CTRL_0r(unit, &pvtmon_ctrl);
    TOP_PVTMON_CTRL_0r_BG_ADJf_SET(pvtmon_ctrl, 0);
    ioerr += WRITE_TOP_PVTMON_CTRL_0r(unit, pvtmon_ctrl);

    /* Bring port blocks out of reset */
    TOP_SOFT_RESET_REGr_CLR(top_sreset);
    TOP_SOFT_RESET_REGr_TOP_PM32_RST_Lf_SET(top_sreset, 1);
    TOP_SOFT_RESET_REGr_TOP_TS_RST_Lf_SET(top_sreset, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REGr(unit, top_sreset);
    TOP_SOFT_RESET_REG_3r_SET(top_sreset_3, 0xffffffff);
    ioerr += WRITE_TOP_SOFT_RESET_REG_3r(unit, top_sreset_3);
    BMD_SYS_USLEEP(wait_usec);

    /* Bring IP, EP, and MMU blocks out of reset */
    ioerr += READ_TOP_SOFT_RESET_REGr(unit, &top_sreset);
    TOP_SOFT_RESET_REGr_TOP_IP_RST_Lf_SET(top_sreset, 1);
    TOP_SOFT_RESET_REGr_TOP_EP_RST_Lf_SET(top_sreset, 1);
    TOP_SOFT_RESET_REGr_TOP_MMU_RST_Lf_SET(top_sreset, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REGr(unit, top_sreset);
    BMD_SYS_USLEEP(wait_usec);

    rv = _hash_bank_count_get(unit, L2Xm_ENUM, &num_banks);
    if (CDK_FAILURE(rv)) {
        return rv;
    }
    num_shared_l2_banks = num_banks - NUM_PRIV_L2_BANKS;
    rv = _hash_bank_count_get(unit, L3_ENTRY_ONLYm_ENUM, &num_banks);
    if (CDK_FAILURE(rv)) {
        return rv;
    }
    num_shared_l3_banks = num_banks - NUM_PRIV_L3_BANKS;
    rv = _hash_bank_count_get(unit, EXACT_MATCH_2m_ENUM, &num_shared_fpem_banks);
    if (CDK_FAILURE(rv)) {
        return rv;
    }

    if (L3_DEFIP_ALPM_IPV4m_MAX > L3_DEFIP_ALPM_IPV4m_MIN) {
        int non_alpm = num_shared_fpem_banks + num_shared_l3_banks + 
                       num_shared_l2_banks;

        ISS_BANK_CONFIGr_CLR(bank_cfg);
        ISS_LOG_TO_PHY_BANK_MAPr_CLR(phy_bank_map);

        if ((non_alpm) && (non_alpm <= 2)) {
        /* If Shared banks are used between ALPM and non-ALPM memories,
         * then ALPM uses Shared Bank B2, B3 and non-ALPM uses B4, B5 banks
         */
            ISS_BANK_CONFIGr_ALPM_ENTRY_BANK_CONFIGf_SET(bank_cfg, 3);

            ISS_LOG_TO_PHY_BANK_MAPr_ALPM_BANK_MODEf_SET(phy_bank_map, 1);

            ioerr += READ_ISS_LOG_TO_PHY_BANK_MAP_2r(unit, &phy_bank_map_2);
            ISS_LOG_TO_PHY_BANK_MAP_2r_ALPM_BANK_MODEf_SET(phy_bank_map_2, 1);
            ioerr += WRITE_ISS_LOG_TO_PHY_BANK_MAP_2r(unit, phy_bank_map_2);

            ioerr += READ_ISS_MEMORY_CONTROL_84r(unit, 0, &mem_ctrl_84);
            ISS_MEMORY_CONTROL_84r_BYPASS_ISS_MEMORY_LPf_SET(mem_ctrl_84, 0x3);
            ioerr += WRITE_ISS_MEMORY_CONTROL_84r(unit, -1, mem_ctrl_84);
            
            shared_bank_offset = 2;            
        } else {
            ISS_BANK_CONFIGr_ALPM_ENTRY_BANK_CONFIGf_SET(bank_cfg, 0xf);

            ISS_LOG_TO_PHY_BANK_MAPr_ALPM_BANK_MODEf_SET(phy_bank_map, 0);

            ioerr += READ_ISS_LOG_TO_PHY_BANK_MAP_2r(unit, &phy_bank_map_2);
            ISS_LOG_TO_PHY_BANK_MAP_2r_ALPM_BANK_MODEf_SET(phy_bank_map_2, 0);
            ioerr += WRITE_ISS_LOG_TO_PHY_BANK_MAP_2r(unit, phy_bank_map_2);

            ioerr += READ_ISS_MEMORY_CONTROL_84r(unit, 0, &mem_ctrl_84);
            ISS_MEMORY_CONTROL_84r_BYPASS_ISS_MEMORY_LPf_SET(mem_ctrl_84, 0xf);
            ioerr += WRITE_ISS_MEMORY_CONTROL_84r(unit, -1, mem_ctrl_84);
        }
    }

    ISS_BANK_CONFIGr_L2_ENTRY_BANK_CONFIGf_SET(bank_cfg, 
                        ((1 << num_shared_l2_banks) - 1) << shared_bank_offset);
    shared_bank = idx + shared_bank_offset;
    ISS_LOG_TO_PHY_BANK_MAPr_L2_ENTRY_BANK_2f_SET(phy_bank_map, shared_bank);
    ISS_LOG_TO_PHY_BANK_MAPr_L2_ENTRY_BANK_3f_SET(phy_bank_map, shared_bank);
    ISS_LOG_TO_PHY_BANK_MAPr_L2_ENTRY_BANK_4f_SET(phy_bank_map, shared_bank);
    ISS_LOG_TO_PHY_BANK_MAPr_L2_ENTRY_BANK_5f_SET(phy_bank_map, shared_bank);

    shared_bank_offset += num_shared_l2_banks;
    ISS_BANK_CONFIGr_L3_ENTRY_BANK_CONFIGf_SET(bank_cfg, 
                        ((1 << num_shared_l3_banks) - 1) << shared_bank_offset);

    shared_bank = idx + shared_bank_offset;
    ISS_LOG_TO_PHY_BANK_MAPr_L3_ENTRY_BANK_4f_SET(phy_bank_map, shared_bank);
    ISS_LOG_TO_PHY_BANK_MAPr_L3_ENTRY_BANK_5f_SET(phy_bank_map, shared_bank);
    ISS_LOG_TO_PHY_BANK_MAPr_L3_ENTRY_BANK_6f_SET(phy_bank_map, shared_bank);
    ISS_LOG_TO_PHY_BANK_MAPr_L3_ENTRY_BANK_7f_SET(phy_bank_map, shared_bank);

    shared_bank_offset += num_shared_l3_banks;
    ISS_BANK_CONFIGr_FPEM_ENTRY_BANK_CONFIGf_SET(bank_cfg, 
                      ((1 << num_shared_fpem_banks) - 1) << shared_bank_offset);
                      
    shared_bank = idx + shared_bank_offset;
    ISS_LOG_TO_PHY_BANK_MAPr_FPEM_ENTRY_BANK_0f_SET(phy_bank_map, shared_bank);
    ISS_LOG_TO_PHY_BANK_MAPr_FPEM_ENTRY_BANK_1f_SET(phy_bank_map, shared_bank);
    ISS_LOG_TO_PHY_BANK_MAPr_FPEM_ENTRY_BANK_2f_SET(phy_bank_map, shared_bank);
    ISS_LOG_TO_PHY_BANK_MAPr_FPEM_ENTRY_BANK_3f_SET(phy_bank_map, shared_bank);

    ISS_LOG_TO_PHY_BANK_MAP_3r_CLR(phy_bank_map_3);
    ISS_LOG_TO_PHY_BANK_MAP_3r_FPEM_ENTRY_BANK_0f_SET(phy_bank_map_3, 
                                                      shared_bank);
    ISS_LOG_TO_PHY_BANK_MAP_3r_FPEM_ENTRY_BANK_1f_SET(phy_bank_map_3, 
                                                      shared_bank);
    ISS_LOG_TO_PHY_BANK_MAP_3r_FPEM_ENTRY_BANK_2f_SET(phy_bank_map_3, 
                                                      shared_bank);
    ISS_LOG_TO_PHY_BANK_MAP_3r_FPEM_ENTRY_BANK_3f_SET(phy_bank_map_3, 
                                                      shared_bank);

    ioerr += WRITE_ISS_BANK_CONFIGr(unit, bank_cfg);
    ioerr += WRITE_ISS_LOG_TO_PHY_BANK_MAPr(unit, phy_bank_map);
    ioerr += WRITE_ISS_LOG_TO_PHY_BANK_MAP_3r(unit, phy_bank_map_3);

    /* Reset port blocks */
    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_CLPORT, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        if (CLPORT_SUBPORT(unit, port) == 0) {
            ioerr += _clport_reset(unit, port);
        }
    }
    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_XLPORT, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        ioerr += _xlport_reset(unit, port);
        /* Only one single XLPORT block in TH */
        break;
    }

    return ioerr ? CDK_E_IO : rv;
}
#endif /* CDK_CONFIG_INCLUDE_BCM56960_A0 */

