/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56860_A0 == 1

#include <bmd/bmd.h>

#include <cdk/chip/bcm56860_a0_defs.h>
#include <cdk/cdk_debug.h>

#include "bcm56860_a0_bmd.h"
#include "bcm56860_a0_internal.h"

#define RESET_SLEEP_USEC                100
#define PLL_LOCK_MSEC                   100

static int
_wait_for_pll_lock(int unit)
{
    int ioerr = 0;
    int msec, idx, locks;
    TOP_XG_PLL_STATUSr_t pll_status[4];
    TOP_TS_PLL_STATUSr_t ts_pll_status;
    TOP_BS_PLL_STATUSr_t bs_pll_status;
    TOP_CORE_PLL_STATUSr_t core_pll_status;
    TOP_MCS_PLL_STATUSr_t mcs_pll_status;

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
        CDK_WARN(("bcm56860_a0_bmd_reset[%d]: LC PLL did not lock, status = ",
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
        ioerr += READ_TOP_BS_PLL_STATUSr(unit, &bs_pll_status);
        if (TOP_TS_PLL_STATUSr_PLL_LOCKf_GET(ts_pll_status) == 1 &&
            TOP_BS_PLL_STATUSr_PLL_LOCKf_GET(bs_pll_status) == 1) {
            break;
        }
        BMD_SYS_USLEEP(1000);
    }
    if (msec >= PLL_LOCK_MSEC) {
        CDK_WARN(("bcm56860_a0_bmd_reset[%d]: "
                  "TS/BS PLL did not lock, status = "
                  "0x%08"PRIx32" 0x%08"PRIx32"\n",
                  unit,
                  TOP_TS_PLL_STATUSr_GET(ts_pll_status), 
                  TOP_BS_PLL_STATUSr_GET(bs_pll_status)));
    }

    /* Wait for core/mcs PLL locks */
    for (msec = 0; msec < PLL_LOCK_MSEC; msec++) {
#if BMD_CONFIG_SIMULATION
        if (msec == 0) break;
#endif
        ioerr += READ_TOP_CORE_PLL_STATUSr(unit, &core_pll_status);
        ioerr += READ_TOP_MCS_PLL_STATUSr(unit, &mcs_pll_status);
        if (TOP_CORE_PLL_STATUSr_PLL_LOCKf_GET(core_pll_status) == 1 &&
            TOP_MCS_PLL_STATUSr_PLL_LOCKf_GET(mcs_pll_status) == 1) {
            break;
        }
        BMD_SYS_USLEEP(1000);
    }
    if (msec >= PLL_LOCK_MSEC) {
        CDK_WARN(("bcm56860_a0_bmd_reset[%d]: "
                  "CORE/MCS PLL did not lock, status = "
                  "0x%08"PRIx32" 0x%08"PRIx32"\n",
                  unit,
                  TOP_CORE_PLL_STATUSr_GET(core_pll_status), 
                  TOP_MCS_PLL_STATUSr_GET(mcs_pll_status)));
    }

    return ioerr;
}

int
bcm56860_a0_tsc_xgxs_reset(int unit, int port, int tsc_idx)
{
    int ioerr = 0;
    PGW_TSC_CTRL_REGr_t tsc_ctrl;
    int idx, start_idx, end_idx, tsc_num;
    int pgw_blkidx = PGW_BLKIDX(port);

    start_idx = 0;
    if (bcm56860_a0_port_speed_max(unit, port) < 100000) {
        /* Reset only one TSC for non-100G port */
        start_idx = tsc_idx;
        tsc_num = 1;
    } else {
        /* Reset TSC triplet for 100G port */
        tsc_num = 3;
        if (pgw_blkidx & 1) {
            start_idx = 1;
        }
    }
    end_idx = start_idx + tsc_num;

    /* Reference clock selection */
    for (idx = start_idx; idx < end_idx; idx++) {
        ioerr += READ_PGW_TSC_CTRL_REGr(unit, pgw_blkidx, idx, &tsc_ctrl);
        PGW_TSC_CTRL_REGr_REFIN_ENf_SET(tsc_ctrl, 1);
        ioerr += WRITE_PGW_TSC_CTRL_REGr(unit, pgw_blkidx, idx, tsc_ctrl);
    }

    /* Deassert power down */
    for (idx = start_idx; idx < end_idx; idx++) {
        ioerr += READ_PGW_TSC_CTRL_REGr(unit, pgw_blkidx, idx, &tsc_ctrl);
        PGW_TSC_CTRL_REGr_PWRDWNf_SET(tsc_ctrl, 0);
        ioerr += WRITE_PGW_TSC_CTRL_REGr(unit, pgw_blkidx, idx, tsc_ctrl);
        BMD_SYS_USLEEP(RESET_SLEEP_USEC);
    }

    /* Reset XGXS */
    for (idx = start_idx; idx < end_idx; idx++) {
        ioerr += READ_PGW_TSC_CTRL_REGr(unit, pgw_blkidx, idx, &tsc_ctrl);
        PGW_TSC_CTRL_REGr_RSTB_HWf_SET(tsc_ctrl, 0);
        ioerr += WRITE_PGW_TSC_CTRL_REGr(unit, pgw_blkidx, idx, tsc_ctrl);
        BMD_SYS_USLEEP(RESET_SLEEP_USEC);
    }

    /* Bring XGXS out of reset */
    for (idx = start_idx; idx < end_idx; idx++) {
        ioerr += READ_PGW_TSC_CTRL_REGr(unit, pgw_blkidx, idx, &tsc_ctrl);
        PGW_TSC_CTRL_REGr_RSTB_HWf_SET(tsc_ctrl, 1);
        ioerr += WRITE_PGW_TSC_CTRL_REGr(unit, pgw_blkidx, idx, tsc_ctrl);
        BMD_SYS_USLEEP(RESET_SLEEP_USEC);
    }

    return ioerr;
}

int 
bcm56860_a0_bmd_reset(int unit)
{
    int ioerr = 0;
    int wait_usec = 100000;
    int port, idx;
    cdk_pbmp_t pbmp;
    CMIC_CPS_RESETr_t cps_reset;
    TOP_XG_PLL_CTRL_1r_t pll_ctrl_1;
    TOP_XG_PLL_CTRL_2r_t pll_ctrl_2;
    TOP_TS_PLL_CTRL_1r_t ts_pll_ctrl_1;
    TOP_TS_PLL_CTRL_3r_t ts_pll_ctrl_3;
    TOP_TS_PLL_CTRL_4r_t ts_pll_ctrl_4;
    TOP_TS_PLL_CTRL_5r_t ts_pll_ctrl_5;
    TOP_BS_PLL_CTRL_0r_t bs_pll_ctrl_0;
    TOP_BS_PLL_CTRL_1r_t bs_pll_ctrl_1;
    TOP_BS_PLL_CTRL_3r_t bs_pll_ctrl_3;
    TOP_BS_PLL_CTRL_4r_t bs_pll_ctrl_4;
    TOP_BS_PLL_CTRL_5r_t bs_pll_ctrl_5;
    TOP_SOFT_RESET_REGr_t top_sreset;
    TOP_SOFT_RESET_REG_2r_t top_sreset_2;
    CMIC_SBUS_RING_MAPr_t sbus_ring_map;
    CMIC_SBUS_TIMEOUTr_t sbus_to;
    TOP_MISC_CONTROLr_t misc_ctrl;
    TOP_PVTMON_CTRL_1r_t pvtmon_ctrl;
    uint32_t ring_map[] = { 0x33052100, 0x33776644, 0x33333333, 0x44444444,
                            0x66666644, 0x77776666, 0x00777777, 0x00005550 };

    BMD_CHECK_UNIT(unit);

    /* Initialize endian mode for correct reset access */
    ioerr += cdk_xgsm_cmic_init(unit);

    /* Pull reset line */
    ioerr += READ_CMIC_CPS_RESETr(unit, &cps_reset);
    CMIC_CPS_RESETr_CPS_RESETf_SET(cps_reset, 1);
    ioerr += WRITE_CMIC_CPS_RESETr(unit, cps_reset);

    /* Wait for all tables to initialize */
    BMD_SYS_USLEEP(wait_usec);

    /* Re-initialize endian mode after reset */
    ioerr += cdk_xgsm_cmic_init(unit);

    /*
     * SBUS ring and block number:
     * ring 0: IP(1)
     * ring 1: EP(2)
     * ring 2: MMU(3)
     * ring 3: PGW_CL0(6), CPORT0(14), XLPORT0(15)-XLPORT3(18)
     *         PGW_CL1(7), CPORT1(19), XLPORT4(20)-XLPORT7(23)
     * ring 4: PGW_CL2(8), CPORT2(24), XLPORT8(25)-XLPORT11(28)
     *         PGW_CL3(9), CPORT3(29), XLPORT12(30)-XLPORT15(33)
     * ring 5: OTPC(4), TOP(57), SER(58), AVS(59)
     * ring 6: PGW_CL4(10), CPORT4(34), XLPORT16(35)-XLPORT19(38)
     *         PGW_CL5(11), CPORT5(39), XLPORT20(40)-XLPORT23(43)
     * ring 7: PGW_CL6(12), CPORT6(44), XLPORT24(45)-XLPORT27(48)
     *         PGW_CL7(13), CPORT7(49), XLPORT28(50)-XLPORT31(53)
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

    /* Program LCPLL frequency */
    for (idx = 0; idx < 4; idx++) {
        ioerr += READ_TOP_XG_PLL_CTRL_1r(unit, idx, &pll_ctrl_1);
        TOP_XG_PLL_CTRL_1r_PDIVf_SET(pll_ctrl_1, 7);
        ioerr += WRITE_TOP_XG_PLL_CTRL_1r(unit, idx, pll_ctrl_1);
        ioerr += READ_TOP_XG_PLL_CTRL_2r(unit, idx, &pll_ctrl_2);
        TOP_XG_PLL_CTRL_2r_NDIV_INTf_SET(pll_ctrl_2, 140);
        ioerr += WRITE_TOP_XG_PLL_CTRL_2r(unit, idx, pll_ctrl_2);
    }
    ioerr += READ_TOP_MISC_CONTROLr(unit, &misc_ctrl);
    TOP_MISC_CONTROLr_CMIC_TO_XG_PLL0_SW_OVWRf_SET(misc_ctrl, 1);
    TOP_MISC_CONTROLr_CMIC_TO_XG_PLL1_SW_OVWRf_SET(misc_ctrl, 1);
    TOP_MISC_CONTROLr_CMIC_TO_XG_PLL2_SW_OVWRf_SET(misc_ctrl, 1);
    TOP_MISC_CONTROLr_CMIC_TO_XG_PLL3_SW_OVWRf_SET(misc_ctrl, 1);
    ioerr += WRITE_TOP_MISC_CONTROLr(unit, misc_ctrl);

    /* Configure TS PLL */ 
    ioerr += READ_TOP_TS_PLL_CTRL_3r(unit, &ts_pll_ctrl_3);
    TOP_TS_PLL_CTRL_3r_PDIVf_SET(ts_pll_ctrl_3, 1);
    TOP_TS_PLL_CTRL_3r_CH0_MDIVf_SET(ts_pll_ctrl_3, 14);
    ioerr += WRITE_TOP_TS_PLL_CTRL_3r(unit, ts_pll_ctrl_3);

    ioerr += READ_TOP_TS_PLL_CTRL_4r(unit, &ts_pll_ctrl_4);
    TOP_TS_PLL_CTRL_4r_NDIV_INTf_SET(ts_pll_ctrl_4, 140);
    TOP_TS_PLL_CTRL_4r_NDIV_FRACf_SET(ts_pll_ctrl_4, 0);
    ioerr += WRITE_TOP_TS_PLL_CTRL_4r(unit, ts_pll_ctrl_4);
   
    ioerr += READ_TOP_TS_PLL_CTRL_5r(unit, &ts_pll_ctrl_5);
    TOP_TS_PLL_CTRL_5r_KAf_SET(ts_pll_ctrl_5, 0);
    TOP_TS_PLL_CTRL_5r_KIf_SET(ts_pll_ctrl_5, 2);
    TOP_TS_PLL_CTRL_5r_KPf_SET(ts_pll_ctrl_5, 3);
    ioerr += WRITE_TOP_TS_PLL_CTRL_5r(unit, ts_pll_ctrl_5);

    ioerr += READ_TOP_TS_PLL_CTRL_1r(unit, &ts_pll_ctrl_1);
    TOP_TS_PLL_CTRL_1r_FREF_SELf_SET(ts_pll_ctrl_1, 0);
    ioerr += WRITE_TOP_TS_PLL_CTRL_1r(unit, ts_pll_ctrl_1);

    /* Configure BS PLL */
    ioerr += READ_TOP_BS_PLL_CTRL_0r(unit, &bs_pll_ctrl_0);
    TOP_BS_PLL_CTRL_0r_VCO_FB_DIV2f_SET(bs_pll_ctrl_0, 1);
    ioerr += WRITE_TOP_BS_PLL_CTRL_0r(unit, bs_pll_ctrl_0);

    ioerr += READ_TOP_BS_PLL_CTRL_3r(unit, &bs_pll_ctrl_3);
    TOP_BS_PLL_CTRL_3r_PDIVf_SET(bs_pll_ctrl_3, 1);
    TOP_BS_PLL_CTRL_3r_CH0_MDIVf_SET(bs_pll_ctrl_3, 250);
    ioerr += WRITE_TOP_BS_PLL_CTRL_3r(unit, bs_pll_ctrl_3);

    ioerr += READ_TOP_BS_PLL_CTRL_4r(unit, &bs_pll_ctrl_4);
    TOP_BS_PLL_CTRL_4r_NDIV_INTf_SET(bs_pll_ctrl_4, 140);
    TOP_BS_PLL_CTRL_4r_NDIV_FRACf_SET(bs_pll_ctrl_4, 0);
    ioerr += WRITE_TOP_BS_PLL_CTRL_4r(unit, bs_pll_ctrl_4);

    ioerr += READ_TOP_BS_PLL_CTRL_5r(unit, &bs_pll_ctrl_5);
    TOP_BS_PLL_CTRL_5r_KAf_SET(bs_pll_ctrl_5, 0);
    TOP_BS_PLL_CTRL_5r_KIf_SET(bs_pll_ctrl_5, 2);
    TOP_BS_PLL_CTRL_5r_KPf_SET(bs_pll_ctrl_5, 3);
    ioerr += WRITE_TOP_BS_PLL_CTRL_5r(unit, bs_pll_ctrl_5);

    ioerr += READ_TOP_BS_PLL_CTRL_1r(unit, &bs_pll_ctrl_1);
    TOP_BS_PLL_CTRL_1r_FREF_SELf_SET(bs_pll_ctrl_1, 0);
    ioerr += WRITE_TOP_BS_PLL_CTRL_1r(unit, bs_pll_ctrl_1);

    /* Bring PLLs out of reset */
    ioerr += READ_TOP_SOFT_RESET_REG_2r(unit, &top_sreset_2);
    TOP_SOFT_RESET_REG_2r_TOP_XG_PLL0_RST_Lf_SET(top_sreset_2, 1);
    TOP_SOFT_RESET_REG_2r_TOP_XG_PLL1_RST_Lf_SET(top_sreset_2, 1);
    TOP_SOFT_RESET_REG_2r_TOP_XG_PLL2_RST_Lf_SET(top_sreset_2, 1);
    TOP_SOFT_RESET_REG_2r_TOP_XG_PLL3_RST_Lf_SET(top_sreset_2, 1);
    TOP_SOFT_RESET_REG_2r_TOP_TS_PLL_RST_Lf_SET(top_sreset_2, 1);
    TOP_SOFT_RESET_REG_2r_TOP_BS_PLL_RST_Lf_SET(top_sreset_2, 1);
    TOP_SOFT_RESET_REG_2r_TOP_ARS_PMB_CLK_ENf_SET(top_sreset_2, 1);
    TOP_SOFT_RESET_REG_2r_TOP_AVS_PMB_RST_Lf_SET(top_sreset_2, 1);
    TOP_SOFT_RESET_REG_2r_TOP_ARS_RST_Lf_SET(top_sreset_2, 1);
    TOP_SOFT_RESET_REG_2r_TOP_AVS_PVTMON_RST_Lf_SET(top_sreset_2, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REG_2r(unit, top_sreset_2);
    BMD_SYS_USLEEP(wait_usec);

    /* Initialize LCPLLs */
    ioerr += _wait_for_pll_lock(unit);

    /* De-assert PLLs post reset */
    TOP_SOFT_RESET_REG_2r_TOP_XG_PLL0_POST_RST_Lf_SET(top_sreset_2, 1);
    TOP_SOFT_RESET_REG_2r_TOP_XG_PLL1_POST_RST_Lf_SET(top_sreset_2, 1);
    TOP_SOFT_RESET_REG_2r_TOP_XG_PLL2_POST_RST_Lf_SET(top_sreset_2, 1);
    TOP_SOFT_RESET_REG_2r_TOP_XG_PLL3_POST_RST_Lf_SET(top_sreset_2, 1);
    TOP_SOFT_RESET_REG_2r_TOP_TS_PLL_POST_RST_Lf_SET(top_sreset_2, 1);
    TOP_SOFT_RESET_REG_2r_TOP_BS_PLL_POST_RST_Lf_SET(top_sreset_2, 1);
    TOP_SOFT_RESET_REG_2r_TOP_PVT_MON_MIN_RST_Lf_SET(top_sreset_2, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REG_2r(unit, top_sreset_2);
    BMD_SYS_USLEEP(wait_usec);

    /* Bring port blocks and timesync out of reset */
    ioerr += TOP_SOFT_RESET_REGr_CLR(top_sreset);
    TOP_SOFT_RESET_REGr_TOP_CLP0_RST_Lf_SET(top_sreset, 1);
    TOP_SOFT_RESET_REGr_TOP_CLP1_RST_Lf_SET(top_sreset, 1);
    TOP_SOFT_RESET_REGr_TOP_CLP2_RST_Lf_SET(top_sreset, 1);
    TOP_SOFT_RESET_REGr_TOP_CLP3_RST_Lf_SET(top_sreset, 1);
    TOP_SOFT_RESET_REGr_TOP_CLP4_RST_Lf_SET(top_sreset, 1);
    TOP_SOFT_RESET_REGr_TOP_CLP5_RST_Lf_SET(top_sreset, 1);
    TOP_SOFT_RESET_REGr_TOP_CLP6_RST_Lf_SET(top_sreset, 1);
    TOP_SOFT_RESET_REGr_TOP_CLP7_RST_Lf_SET(top_sreset, 1);
    TOP_SOFT_RESET_REGr_TOP_TS_RST_Lf_SET(top_sreset, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REGr(unit, top_sreset);
    BMD_SYS_USLEEP(wait_usec);

    /* Bring IP, EP and MMU blocks out of reset */
    ioerr += READ_TOP_SOFT_RESET_REGr(unit, &top_sreset);
    TOP_SOFT_RESET_REGr_TOP_EP_RST_Lf_SET(top_sreset, 1);
    TOP_SOFT_RESET_REGr_TOP_IP_RST_Lf_SET(top_sreset, 1);
    TOP_SOFT_RESET_REGr_TOP_MMU_RST_Lf_SET(top_sreset, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REGr(unit, top_sreset);
    BMD_SYS_USLEEP(wait_usec);

    /* PVT monitor reset */
    ioerr += READ_TOP_PVTMON_CTRL_1r(unit, &pvtmon_ctrl);
    TOP_PVTMON_CTRL_1r_PVTMON_ADC_RESETBf_SET(pvtmon_ctrl, 1);
    ioerr += WRITE_TOP_PVTMON_CTRL_1r(unit, pvtmon_ctrl);
    TOP_PVTMON_CTRL_1r_PVTMON_ADC_RESETBf_SET(pvtmon_ctrl, 0);
    ioerr += WRITE_TOP_PVTMON_CTRL_1r(unit, pvtmon_ctrl);
    TOP_PVTMON_CTRL_1r_PVTMON_ADC_RESETBf_SET(pvtmon_ctrl, 1);
    ioerr += WRITE_TOP_PVTMON_CTRL_1r(unit, pvtmon_ctrl);

    /* Temperature monitor reset */
    ioerr += READ_TOP_SOFT_RESET_REG_2r(unit, &top_sreset_2);
    TOP_SOFT_RESET_REG_2r_TOP_PVT_MON_MIN_RST_Lf_SET(top_sreset_2, 0);
    ioerr += WRITE_TOP_SOFT_RESET_REG_2r(unit, top_sreset_2);
    TOP_SOFT_RESET_REG_2r_TOP_PVT_MON_MIN_RST_Lf_SET(top_sreset_2, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REG_2r(unit, top_sreset_2);
    BMD_SYS_USLEEP(wait_usec);

    /* Reset all XPORTs */
    CDK_XGSM_BLKTYPE_PBMP_GET(unit, BLKTYPE_XLPORT, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        /* We only need to reset first port in each block */
        if (XLPORT_SUBPORT(port) == 0) {
            ioerr += bcm56860_a0_tsc_xgxs_reset(unit, port, 0);
        }
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;

}
#endif /* CDK_CONFIG_INCLUDE_BCM56860_A0 */

