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
#include <bmd/bmd_device.h>
#include <bmdi/arch/xgsd_dma.h>

#include <cdk/arch/xgsd_chip.h>
#include <cdk/cdk_debug.h>
#include <cdk/cdk_util.h>
#include <cdk/chip/bcm53540_a0_defs.h>

#include "bcm53540_a0_bmd.h"
#include "bcm53540_a0_internal.h"

#define PIPE_RESET_TIMEOUT_MSEC         5

static int
_misc_init(int unit)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int idx;
    ING_HW_RESET_CONTROL_1r_t ing_rst_ctl_1;
    ING_HW_RESET_CONTROL_2r_t ing_rst_ctl_2;
    EGR_HW_RESET_CONTROL_0r_t egr_rst_ctl_0;
    EGR_HW_RESET_CONTROL_1r_t egr_rst_ctl_1;
    PGW_GE_MODE_REGr_t pgw_ge_mode;
    MISCCONFIGr_t misc_cfg;
    CMIC_RATE_ADJUSTr_t rate_adjust;
    CMIC_RATE_ADJUST_INT_MDIOr_t rate_adjust_int_mdio;
    CMIC_MIIM_CONFIGr_t miim_cfg;
    RDBGC0_SELECTr_t rdbgc0_select;
    ING_PHYS_TO_LOGIC_MAPm_t ing_p2l;
    EGR_LOGIC_TO_PHYS_MAPr_t egr_l2p;
    MMU_LOGIC_TO_PHYS_MAPr_t mmu_l2p;
    GPORT_CONFIGr_t gcfg;
    cdk_pbmp_t pbmp;
    int lport, port;
    int bidx;
    int num_lport, num_port;
    int freq, target_freq, divisor, dividend, delay;
    int ge0,ge1;

    BMD_CHECK_UNIT(unit);

    /* Clear IPIPE/EIPIE Memories */
    /* Reset the IPIPE and EPIPE block */
    ING_HW_RESET_CONTROL_1r_CLR(ing_rst_ctl_1);
    ioerr += WRITE_ING_HW_RESET_CONTROL_1r(unit, ing_rst_ctl_1);
    ING_HW_RESET_CONTROL_2r_CLR(ing_rst_ctl_2);
    ING_HW_RESET_CONTROL_2r_RESET_ALLf_SET(ing_rst_ctl_2, 1);
    ING_HW_RESET_CONTROL_2r_VALIDf_SET(ing_rst_ctl_2, 1);
    ING_HW_RESET_CONTROL_2r_COUNTf_SET(ing_rst_ctl_2, 0x4000);
    ioerr += WRITE_ING_HW_RESET_CONTROL_2r(unit, ing_rst_ctl_2);

    EGR_HW_RESET_CONTROL_0r_CLR(egr_rst_ctl_0);
    EGR_HW_RESET_CONTROL_1r_CLR(egr_rst_ctl_1);
    ioerr += WRITE_EGR_HW_RESET_CONTROL_0r(unit, egr_rst_ctl_0);
    EGR_HW_RESET_CONTROL_1r_RESET_ALLf_SET(egr_rst_ctl_1, 1);
    EGR_HW_RESET_CONTROL_1r_VALIDf_SET(egr_rst_ctl_1, 1);
    EGR_HW_RESET_CONTROL_1r_COUNTf_SET(egr_rst_ctl_1, 0x2000);
    ioerr += WRITE_EGR_HW_RESET_CONTROL_1r(unit, egr_rst_ctl_1);

    /* Wait for IPIPE memory initialization done. */
    for (idx = 0; idx < PIPE_RESET_TIMEOUT_MSEC; idx++) {
        ioerr += READ_ING_HW_RESET_CONTROL_2r(unit, &ing_rst_ctl_2);
        if (ING_HW_RESET_CONTROL_2r_DONEf_GET(ing_rst_ctl_2)) {
            break;
        }
        BMD_SYS_USLEEP(1000);
    }
    if (idx >= PIPE_RESET_TIMEOUT_MSEC) {
        CDK_WARN(("bcm53540_a0_bmd_init[%d]: IPIPE reset timeout\n", unit));
        return ioerr ? CDK_E_IO : CDK_E_TIMEOUT;
    }

    /* Wait for EPIPE memory initialization done. */
    for (idx = 0; idx < PIPE_RESET_TIMEOUT_MSEC; idx++) {
        ioerr += READ_EGR_HW_RESET_CONTROL_1r(unit, &egr_rst_ctl_1);
        if (EGR_HW_RESET_CONTROL_1r_DONEf_GET(egr_rst_ctl_1)) {
            break;
        }
        BMD_SYS_USLEEP(1000);
    }
    if (idx >= PIPE_RESET_TIMEOUT_MSEC) {
        CDK_WARN(("bcm53540_a0_bmd_init[%d]: EPIPE reset timeout\n", unit));
        return ioerr ? CDK_E_IO : CDK_E_TIMEOUT;
    }

    /* Clear pipe reset registers */
    ING_HW_RESET_CONTROL_2r_CLR(ing_rst_ctl_2);
    ioerr += WRITE_ING_HW_RESET_CONTROL_2r(unit, ing_rst_ctl_2);
    EGR_HW_RESET_CONTROL_1r_CLR(egr_rst_ctl_1);
    ioerr += WRITE_EGR_HW_RESET_CONTROL_1r(unit, egr_rst_ctl_1);

    /* PGW_GE TDM mode */
    /* The value of the PGW_GE TDM mode depends on the active port numbers */
    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_PGW_GE, &pbmp);
    for (bidx = 0; bidx <= 2; bidx++) {
        ge0 = 0;
        ge1 = 0;
        CDK_PBMP_ITER(pbmp, port) {
            if (PMQ_BLKNUM(port) == bidx) {
                if (P2L(unit, port) == -1) {
                    continue;
                }
                if (SUBPORT(unit, BLKTYPE_PMQ, port) < 8) {
                    ge0++;
                } else {
                    ge1++;
                }
            }
        }

        ioerr += READ_PGW_GE_MODE_REGr(unit, bidx, 0, &pgw_ge_mode);
        PGW_GE_MODE_REGr_GP0_TDM_MODEf_SET(pgw_ge_mode, ge0);
        ioerr += WRITE_PGW_GE_MODE_REGr(unit, bidx, 0, pgw_ge_mode);
        ioerr += READ_PGW_GE_MODE_REGr(unit, bidx, 1, &pgw_ge_mode);
        PGW_GE_MODE_REGr_GP0_TDM_MODEf_SET(pgw_ge_mode, ge1);
        ioerr += WRITE_PGW_GE_MODE_REGr(unit, bidx, 1, pgw_ge_mode);
    }

    /* Ingress physical to logical port mapping */
    num_port = ING_PHYS_TO_LOGIC_MAPm_MAX;
    for (port = 0; port <= num_port; port++) {
        lport = P2L(unit, port);
        if (lport == -1) {
            lport = 0x3f;
        }
        ING_PHYS_TO_LOGIC_MAPm_CLR(ing_p2l);
        ING_PHYS_TO_LOGIC_MAPm_LOGICAL_PORT_NUMBERf_SET(ing_p2l, lport);
        ioerr += WRITE_ING_PHYS_TO_LOGIC_MAPm(unit, port, ing_p2l);
    }

    /* Egress logical to physical port mapping */
    num_lport = PORT_TABm_MAX;
    for (lport = 0; lport <= num_lport; lport++) {
        port = L2P(unit, lport);
        if (port == -1) {
            port = 0x3f;
        }
        EGR_LOGIC_TO_PHYS_MAPr_CLR(egr_l2p);
        EGR_LOGIC_TO_PHYS_MAPr_PHYSICAL_PORT_NUMBERf_SET(egr_l2p, port);
        ioerr += WRITE_EGR_LOGIC_TO_PHYS_MAPr(unit, lport, egr_l2p);
    }

    /* MMU logical to physical port mapping */
    for (lport = 0; lport <= num_lport; lport++) {
        port = M2P(unit, lport);
        if (port != -1) {
            MMU_LOGIC_TO_PHYS_MAPr_CLR(mmu_l2p);
            MMU_LOGIC_TO_PHYS_MAPr_PHYS_PORTf_SET(mmu_l2p, port);
            ioerr += WRITE_MMU_LOGIC_TO_PHYS_MAPr(unit, lport, mmu_l2p);
        }
    }

    /* Port related init */
    /* Enable GPORT */
    GPORT_CONFIGr_CLR(gcfg);
    GPORT_CONFIGr_CLR_CNTf_SET(gcfg, 1);
    GPORT_CONFIGr_GPORT_ENf_SET(gcfg, 1);
    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_GPORT, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        if (SUBPORT(unit, BLKTYPE_GPORT, port) == 0) {
            WRITE_GPORT_CONFIGr(unit, gcfg, port);
        }
    }
    GPORT_CONFIGr_CLR_CNTf_SET(gcfg, 0);
    CDK_PBMP_ITER(pbmp, port) {
        if (SUBPORT(unit, BLKTYPE_GPORT, port) == 0) {
            WRITE_GPORT_CONFIGr(unit, gcfg, port);
        }
    }

    /* Enable Field Processor metering clock */
    ioerr += READ_MISCCONFIGr(unit, &misc_cfg);
    MISCCONFIGr_METERING_CLK_ENf_SET(misc_cfg, 1);
    ioerr += WRITE_MISCCONFIGr(unit, misc_cfg);

    freq = CORE_CLK(unit);
    /*
     * Set external MDIO freq to around 10MHz
     * target_freq = core_clock_freq * DIVIDEND / DIVISOR / 2
     */
    target_freq = 10;
    divisor = (freq + (target_freq * 2 - 1)) / (target_freq * 2);
    dividend = 1;

    CMIC_RATE_ADJUSTr_CLR(rate_adjust);
    CMIC_RATE_ADJUSTr_DIVISORf_SET(rate_adjust, divisor);
    CMIC_RATE_ADJUSTr_DIVIDENDf_SET(rate_adjust, dividend);
    ioerr += WRITE_CMIC_RATE_ADJUSTr(unit, rate_adjust);

    /*
     * Set internal MDIO freq to around 10MHz
     * Valid range is from 2.5MHz to 12.5MHz
     * target_freq = core_clock_freq * DIVIDEND / DIVISOR / 2
     * or DIVISOR = core_clock_freq * DIVIDENT / (target_freq * 2)
     */
    target_freq = 10;
    divisor = (freq + (target_freq * 2 - 1)) / (target_freq * 2);
    dividend = 1;

    CMIC_RATE_ADJUST_INT_MDIOr_CLR(rate_adjust_int_mdio);
    CMIC_RATE_ADJUST_INT_MDIOr_DIVISORf_SET(rate_adjust_int_mdio, divisor);
    CMIC_RATE_ADJUST_INT_MDIOr_DIVIDENDf_SET(rate_adjust_int_mdio, dividend);
    ioerr += WRITE_CMIC_RATE_ADJUST_INT_MDIOr(unit, rate_adjust_int_mdio);

    delay = -1;
    if (delay >= 1 && delay <= 15) {
        ioerr += READ_CMIC_MIIM_CONFIGr(unit, &miim_cfg);
        CMIC_MIIM_CONFIGr_MDIO_OUT_DELAYf_SET(miim_cfg, delay);
        ioerr += WRITE_CMIC_MIIM_CONFIGr(unit, miim_cfg);
    }

    /* Configure discard counter */
    RDBGC0_SELECTr_CLR(rdbgc0_select);
    RDBGC0_SELECTr_BITMAPf_SET(rdbgc0_select, 0x0400ad11);
    ioerr += WRITE_RDBGC0_SELECTr(unit, rdbgc0_select);

    return ioerr ? CDK_E_IO : rv;
}

static int
_mmu_tdm_init(int unit)
{
    int ioerr = 0;
    IARB_TDM_CONTROLr_t iarb_tdm_ctrl;
    IARB_TDM_TABLEm_t iarb_tdm;
    MMU_ARB_TDM_TABLEm_t mmu_arb_tdm;
    const int *tdm_seq;
    int tdm_size, tdm_max;
    int idx;
    int port, mport;

    /* Get default TDM sequence for this configuration */
    tdm_size = bcm53540_a0_tdm_default(unit, &tdm_seq);
    if (tdm_size <= 0) {
        return CDK_E_INTERNAL;
    }
    tdm_max = tdm_size - 1;

    ioerr += READ_IARB_TDM_CONTROLr(unit, &iarb_tdm_ctrl);
    IARB_TDM_CONTROLr_DISABLEf_SET(iarb_tdm_ctrl, 1);
    IARB_TDM_CONTROLr_TDM_WRAP_PTRf_SET(iarb_tdm_ctrl, 83);
    ioerr += WRITE_IARB_TDM_CONTROLr(unit, iarb_tdm_ctrl);

    for (idx = 0; idx < tdm_size; idx++) {
        port = tdm_seq[idx];
        mport = port;
        if (mport != 63) {
            mport = P2M(unit, port);
        }
        IARB_TDM_TABLEm_CLR(iarb_tdm);
        IARB_TDM_TABLEm_PORT_NUMf_SET(iarb_tdm, port);
        ioerr += WRITE_IARB_TDM_TABLEm(unit, idx, iarb_tdm);

        MMU_ARB_TDM_TABLEm_CLR(mmu_arb_tdm);
        MMU_ARB_TDM_TABLEm_PORT_NUMf_SET(mmu_arb_tdm, mport);
        if (idx == (tdm_size - 1)) {
            MMU_ARB_TDM_TABLEm_WRAP_ENf_SET(mmu_arb_tdm, 1);
        }
        ioerr += WRITE_MMU_ARB_TDM_TABLEm(unit, idx, mmu_arb_tdm);

        CDK_VERB(("%s", (idx == 0) ? "TDM seq:" : ""));
        CDK_VERB(("%s", (idx & 0xf) == 0 ? "\n" : ""));
        CDK_VERB(("%3d ", tdm_seq[idx]));
    }
    CDK_VERB(("\n"));

    /* Enable arbiter */
    ioerr += READ_IARB_TDM_CONTROLr(unit, &iarb_tdm_ctrl);
    IARB_TDM_CONTROLr_DISABLEf_SET(iarb_tdm_ctrl, 0);
    IARB_TDM_CONTROLr_TDM_WRAP_PTRf_SET(iarb_tdm_ctrl, tdm_max);
    ioerr += WRITE_IARB_TDM_CONTROLr(unit, iarb_tdm_ctrl);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

static int
_ceiling_func(uint32_t numerators, uint32_t denominator)
{
    uint32_t result;
    if (denominator == 0) {
        return 0xffffffff;
    }
    result = numerators / denominator;
    if (numerators % denominator != 0) {
        result++;
    }
    return result;
}


static int
_mmu_lossy_init_helper(int unit)
{
    int port, lport;
    int idx;
    int cos;
    uint32_t fval;
    cdk_pbmp_t all_pbmp;
    cdk_pbmp_t pbmp_cpu;
    cdk_pbmp_t pbmp_uplink;
    cdk_pbmp_t pbmp_uplink_1g;
    cdk_pbmp_t pbmp_uplink_2dot5g;
    cdk_pbmp_t pbmp_downlink;
    cdk_pbmp_t pbmp_downlink_1g;
    cdk_pbmp_t pbmp_downlink_2dot5g;
    int standard_jumbo_frame;
    int cell_size;
    int ethernet_mtu_cell;
    int standard_jumbo_frame_cell;
    int total_physical_memory;
    int total_cell_memory_for_admission;
    int reserved_for_cfap;
    int skidmarker;
    int prefetch;
    int cfapfullsetpoint;
    int total_advertised_cell_memory;
    int number_of_uplink_ports;
    int number_of_uplink_ports_1g;
    int number_of_uplink_ports_2dot5g;
    int number_of_downlink_ports;
    int number_of_downlink_ports_1g;
    int number_of_downlink_ports_2dot5g;
    int queue_port_limit_ratio;
    int egress_queue_min_reserve_uplink_ports_lossy;
    int egress_queue_min_reserve_downlink_ports_lossy;
    int egress_queue_min_reserve_cpu_ports;
    int egress_xq_min_reserve_lossy_ports;
    int num_lossy_queues;
    int mmu_xoff_cpkt_thrs_uports;
    int mmu_xoff_cpkt_thrs_dports;
    int mmu_xoff_1g_dports;
    int mmu_xoff_2dot5g_dports;
    int mmu_xoff_cell_thrs_uports;
    int num_cpu_queues;
    int num_cpu_ports;
    int numxqs_per_uplink_ports;
    int numxqs_per_downlink_ports_and_cpu_port;
    int xoff_1g_dports;
    int xoff_2dot5g_dports;
    int xoff_cell_thrs_uports;
    int xoff_pkt_thrs_uport;
    int xoff_pkt_thrs_dport;
    int disc_pg_upport;
    int disc_pg_dwnport;
    int total_reserved_cells_for_uplink_ports;
    int total_reserved_cells_for_downlink_ports;
    int total_reserved_cells_for_cpu_port;
    int total_reserved;
    int shared_space_cells;
    int reserved_xqs_per_uplink_port;
    int shared_xqs_per_uplink_port;
    int reserved_xqs_per_downlink_port;
    int shared_xqs_per_downlink_port;
    int gbllimitsetlimit_up;
    int holcospktsetlimit0_up;
    int holcospktsetlimit3_up;
    int dynxqcntport_up;
    int dyncelllimit_dyncellsetlimit_up;
    int holcospktsetlimit0_pktsetlimit_down_1;
    int holcospktsetlimit3_pktsetlimit_down_1;
    int dynxqcntport_dynxqcntport_down_1;
    int lwmcoscellsetlimit0_cellsetlimit_down_1;
    int lwmcoscellsetlimit3_cellsetlimit_down_1;
    int holcoscellmaxlimit0_cellmaxlimit_down_1;
    int holcoscellmaxlimit3_cellmaxlimit_down_1;
    int dyncelllimit_dyncellsetlimit_down_1;
    int holcospktsetlimit0_pktsetlimit_down_2dot5;
    int holcospktsetlimit3_pktsetlimit_down_2dot5;
    int dynxqcntport_dynxqcntport_down_2dot5;
    int lwmcoscellsetlimit0_cellsetlimit_down_2dot5;
    int lwmcoscellsetlimit3_cellsetlimit_down_2dot5;
    int holcoscellmaxlimit0_cellmaxlimit_down_2dot5;
    int holcoscellmaxlimit3_cellmaxlimit_down_2dot5;
    int dyncelllimit_dyncellsetlimit_down_2dot5;
    int holcosminxqcnt0_holcosminxqcnt_cpu;
    int holcosminxqcnt3_holcosminxqcnt_cpu;
    int holcospktsetlimit0_pktsetlimit_cpu;
    int holcospktsetlimit3_pktsetlimit_cpu;
    int lwmcoscellsetlimit0_cellsetlimit_cpu;
    int lwmcoscellsetlimit3_cellsetlimit_cpu;
    int holcoscellmaxlimit0_cellmaxlimit_cpu;
    int holcoscellmaxlimit3_cellmaxlimit_cpu;
    int dynxqcntport_dynxqcntport_cpu;
    int dyncelllimit_dyncellsetlimit_cpu;
    CFAPFULLTHRESHOLDr_t cfapfullthreshold;
    GBLLIMITSETLIMITr_t gbllimitsetlimit;
    GBLLIMITRESETLIMITr_t gbllimitresetlimit;
    TOTALDYNCELLSETLIMITr_t totaldyncellsetlimit;
    TOTALDYNCELLRESETLIMITr_t totaldyncellresetlimit;
    MISCCONFIGr_t miscconfig;
    PG_CTRL0r_t pg_ctrl0;
    PG_CTRL1r_t pg_ctrl1;
    PG2TCr_t pg2tc;
    IBPPKTSETLIMITr_t ibppktsetlimit;
    MMU_FC_RX_ENr_t mmu_fc_rx_en;
    MMU_FC_TX_ENr_t mmu_fc_tx_en;
    PGCELLLIMITr_t pgcelllimit;
    PGDISCARDSETLIMITr_t pgdiscardsetlimit;
    HOLCOSMINXQCNTr_t holcosminxqcnt;
    HOLCOSPKTSETLIMITr_t holcospktsetlimit;
    HOLCOSPKTRESETLIMITr_t holcospktresetlimit;
    CNGCOSPKTLIMIT0r_t cngcospktlimit0;
    CNGCOSPKTLIMIT1r_t cngcospktlimit1;
    CNGPORTPKTLIMITr_t cngportpktlimit;
    DYNXQCNTPORTr_t dynxqcntport;
    DYNRESETLIMPORTr_t dynresetlimport;
    LWMCOSCELLSETLIMITr_t lwmcoscellsetlimit;
    HOLCOSCELLMAXLIMITr_t holcoscellmaxlimit;
    DYNCELLLIMITr_t dyncelllimit;
    COLOR_DROP_ENr_t color_drop_en;
    SHARED_POOL_CTRLr_t shared_pool_ctrl;

    /* setup port bitmap according the port max speed for lossy
     */
    num_cpu_ports = 0;
    number_of_uplink_ports = 0;
    number_of_uplink_ports_1g = 0;
    number_of_uplink_ports_2dot5g = 0;
    number_of_downlink_ports = 0;
    number_of_downlink_ports_1g = 0;
    number_of_downlink_ports_2dot5g = 0;

    CDK_PBMP_CLEAR(pbmp_cpu);
    CDK_PBMP_CLEAR(pbmp_uplink);
    CDK_PBMP_CLEAR(pbmp_downlink);
    CDK_PBMP_CLEAR(pbmp_downlink_1g);
    CDK_PBMP_CLEAR(pbmp_downlink_2dot5g);

    bcm53540_a0_all_pbmp_get(unit, &all_pbmp);

    CDK_PBMP_ITER(all_pbmp, port) {
        lport = P2L(unit, port);
        if (lport == -1) {
            continue;
        }

        if (port == CMIC_PORT) {
            num_cpu_ports++;
        } else if (34 <= port && port <= 37){
            /* SGMII4P1 */
            number_of_uplink_ports++;
            CDK_PBMP_PORT_ADD(pbmp_uplink, port);

            if (SPEED_MAX(unit, port) == 2500) {
                number_of_uplink_ports_2dot5g++;
                CDK_PBMP_PORT_ADD(pbmp_uplink_2dot5g, port);
            }else{
                number_of_uplink_ports_1g++;
                CDK_PBMP_PORT_ADD(pbmp_uplink_1g, port);
            }
        }else {
            number_of_downlink_ports++;
            CDK_PBMP_PORT_ADD(pbmp_downlink, port);

            if (SPEED_MAX(unit, port) == 2500) {
                number_of_downlink_ports_2dot5g++;
                CDK_PBMP_PORT_ADD(pbmp_downlink_2dot5g, port);
            }else{
                number_of_downlink_ports_1g++;
                CDK_PBMP_PORT_ADD(pbmp_downlink_1g, port);
            }
        }
    }

    standard_jumbo_frame = 9216;
    cell_size = 128;
    ethernet_mtu_cell = _ceiling_func(15 * 1024 / 10, cell_size);
    standard_jumbo_frame_cell = _ceiling_func(standard_jumbo_frame, cell_size);
    total_physical_memory = 4 * 1024;

    reserved_for_cfap = 31 * 2 + 7 + 29;
    skidmarker = 7;
    prefetch = 9;
    cfapfullsetpoint = total_physical_memory - reserved_for_cfap;
    total_cell_memory_for_admission = cfapfullsetpoint;
    total_advertised_cell_memory = total_cell_memory_for_admission;
    queue_port_limit_ratio = 4;
    egress_queue_min_reserve_uplink_ports_lossy = ethernet_mtu_cell;
    egress_queue_min_reserve_downlink_ports_lossy = ethernet_mtu_cell;
    egress_queue_min_reserve_cpu_ports = ethernet_mtu_cell;
    egress_xq_min_reserve_lossy_ports = ethernet_mtu_cell;
    num_lossy_queues = 4;
    mmu_xoff_cpkt_thrs_uports = total_advertised_cell_memory;
    mmu_xoff_cpkt_thrs_dports = total_advertised_cell_memory;
    mmu_xoff_1g_dports = total_advertised_cell_memory;
    mmu_xoff_2dot5g_dports = total_advertised_cell_memory;
    mmu_xoff_cell_thrs_uports = total_advertised_cell_memory;
    num_cpu_queues = 8;
    num_cpu_ports = 1;
    numxqs_per_uplink_ports = 512;
    numxqs_per_downlink_ports_and_cpu_port = 512;
    xoff_1g_dports = mmu_xoff_1g_dports;
    xoff_2dot5g_dports = mmu_xoff_2dot5g_dports;
    xoff_cell_thrs_uports = mmu_xoff_cell_thrs_uports;
    xoff_pkt_thrs_uport = mmu_xoff_cpkt_thrs_uports;
    xoff_pkt_thrs_dport = mmu_xoff_cpkt_thrs_dports;

    disc_pg_upport = xoff_1g_dports;
    disc_pg_dwnport = total_advertised_cell_memory;

    total_reserved_cells_for_uplink_ports
        = egress_queue_min_reserve_uplink_ports_lossy
          * number_of_uplink_ports * num_lossy_queues;
    total_reserved_cells_for_downlink_ports
        = number_of_downlink_ports
          * egress_queue_min_reserve_downlink_ports_lossy
          * num_lossy_queues;
    total_reserved_cells_for_cpu_port
        = num_cpu_ports * egress_queue_min_reserve_cpu_ports
          * num_cpu_queues;
    total_reserved
        = total_reserved_cells_for_uplink_ports
          + total_reserved_cells_for_downlink_ports
          + total_reserved_cells_for_cpu_port;
    shared_space_cells = total_advertised_cell_memory - total_reserved;
    reserved_xqs_per_uplink_port
        = egress_xq_min_reserve_lossy_ports
          * num_lossy_queues;
    shared_xqs_per_uplink_port
          = numxqs_per_uplink_ports - reserved_xqs_per_uplink_port;
    reserved_xqs_per_downlink_port
        = egress_xq_min_reserve_lossy_ports
          * num_lossy_queues;
    shared_xqs_per_downlink_port
        = numxqs_per_downlink_ports_and_cpu_port
          - reserved_xqs_per_downlink_port;
    gbllimitsetlimit_up = total_cell_memory_for_admission;

    holcospktsetlimit0_up = shared_xqs_per_uplink_port + egress_xq_min_reserve_lossy_ports;

    holcospktsetlimit3_up = shared_xqs_per_uplink_port + egress_xq_min_reserve_lossy_ports;

    dynxqcntport_up = shared_xqs_per_uplink_port - skidmarker - prefetch;

    dyncelllimit_dyncellsetlimit_up = shared_space_cells;

    holcospktsetlimit0_pktsetlimit_down_1
        = shared_xqs_per_downlink_port
          + egress_xq_min_reserve_lossy_ports;

    holcospktsetlimit3_pktsetlimit_down_1
        = shared_xqs_per_downlink_port
          + egress_xq_min_reserve_lossy_ports;

    dynxqcntport_dynxqcntport_down_1
          = shared_xqs_per_downlink_port - skidmarker - prefetch;
    lwmcoscellsetlimit0_cellsetlimit_down_1 =
              egress_queue_min_reserve_downlink_ports_lossy;

    lwmcoscellsetlimit3_cellsetlimit_down_1 =
              egress_queue_min_reserve_downlink_ports_lossy;

    holcoscellmaxlimit0_cellmaxlimit_down_1
        = _ceiling_func(shared_space_cells, queue_port_limit_ratio)
          + lwmcoscellsetlimit0_cellsetlimit_down_1;

    holcoscellmaxlimit3_cellmaxlimit_down_1
        = _ceiling_func(shared_space_cells, queue_port_limit_ratio)
          + lwmcoscellsetlimit3_cellsetlimit_down_1;

    dyncelllimit_dyncellsetlimit_down_1 = shared_space_cells;

    holcospktsetlimit0_pktsetlimit_down_2dot5
        = shared_xqs_per_downlink_port
          + egress_xq_min_reserve_lossy_ports;

    holcospktsetlimit3_pktsetlimit_down_2dot5
        = shared_xqs_per_downlink_port
          + egress_xq_min_reserve_lossy_ports;

    dynxqcntport_dynxqcntport_down_2dot5
          = shared_xqs_per_downlink_port - skidmarker - prefetch;
    lwmcoscellsetlimit0_cellsetlimit_down_2dot5 =
              egress_queue_min_reserve_downlink_ports_lossy;

    lwmcoscellsetlimit3_cellsetlimit_down_2dot5 =
              egress_queue_min_reserve_downlink_ports_lossy;


    holcoscellmaxlimit0_cellmaxlimit_down_2dot5
        = _ceiling_func(shared_space_cells, queue_port_limit_ratio)
          + lwmcoscellsetlimit0_cellsetlimit_down_2dot5;

    holcoscellmaxlimit3_cellmaxlimit_down_2dot5
        = _ceiling_func(shared_space_cells, queue_port_limit_ratio)
          + lwmcoscellsetlimit3_cellsetlimit_down_2dot5;


    dyncelllimit_dyncellsetlimit_down_2dot5 = shared_space_cells;

    holcosminxqcnt0_holcosminxqcnt_cpu = egress_queue_min_reserve_cpu_ports;
    holcosminxqcnt3_holcosminxqcnt_cpu = egress_queue_min_reserve_cpu_ports;
    holcospktsetlimit0_pktsetlimit_cpu =
              shared_xqs_per_downlink_port + holcosminxqcnt0_holcosminxqcnt_cpu;
    holcospktsetlimit3_pktsetlimit_cpu =
              shared_xqs_per_downlink_port + holcosminxqcnt3_holcosminxqcnt_cpu;


    dynxqcntport_dynxqcntport_cpu =
              shared_xqs_per_downlink_port - skidmarker - prefetch;

    lwmcoscellsetlimit0_cellsetlimit_cpu = egress_queue_min_reserve_cpu_ports;
    lwmcoscellsetlimit3_cellsetlimit_cpu = egress_queue_min_reserve_cpu_ports;
    holcoscellmaxlimit0_cellmaxlimit_cpu =
              _ceiling_func(shared_space_cells, queue_port_limit_ratio) +
              lwmcoscellsetlimit0_cellsetlimit_cpu;
    holcoscellmaxlimit3_cellmaxlimit_cpu =
              _ceiling_func(shared_space_cells, queue_port_limit_ratio) +
              lwmcoscellsetlimit3_cellsetlimit_cpu;

    dyncelllimit_dyncellsetlimit_cpu = shared_space_cells;

    /* system-based */
    READ_CFAPFULLTHRESHOLDr(unit, &cfapfullthreshold);
    CFAPFULLTHRESHOLDr_CFAPFULLSETPOINTf_SET(cfapfullthreshold, cfapfullsetpoint);
    WRITE_CFAPFULLTHRESHOLDr(unit, cfapfullthreshold);

    fval = cfapfullsetpoint - standard_jumbo_frame_cell;
    CFAPFULLTHRESHOLDr_CFAPFULLRESETPOINTf_SET(cfapfullthreshold, fval);
    WRITE_CFAPFULLTHRESHOLDr(unit, cfapfullthreshold);

    GBLLIMITSETLIMITr_SET(gbllimitsetlimit, total_cell_memory_for_admission);
    WRITE_GBLLIMITSETLIMITr(unit, gbllimitsetlimit);

    GBLLIMITRESETLIMITr_SET(gbllimitresetlimit, gbllimitsetlimit_up);
    WRITE_GBLLIMITRESETLIMITr(unit, gbllimitresetlimit);

    TOTALDYNCELLSETLIMITr_SET(totaldyncellsetlimit, shared_space_cells);
    WRITE_TOTALDYNCELLSETLIMITr(unit, totaldyncellsetlimit);

    fval = shared_space_cells - standard_jumbo_frame_cell * 2;
    TOTALDYNCELLRESETLIMITr_SET(totaldyncellresetlimit, fval);
    WRITE_TOTALDYNCELLRESETLIMITr(unit, totaldyncellresetlimit);

    READ_MISCCONFIGr(unit, &miscconfig);
    MISCCONFIGr_MULTIPLE_ACCOUNTING_FIX_ENf_SET(miscconfig, 1);
    MISCCONFIGr_CNG_DROP_ENf_SET(miscconfig, 0);
    MISCCONFIGr_DYN_XQ_ENf_SET(miscconfig, 1);
    MISCCONFIGr_HOL_CELL_SOP_DROP_ENf_SET(miscconfig, 1);
    MISCCONFIGr_DYNAMIC_MEMORY_ENf_SET(miscconfig, 1);
    MISCCONFIGr_SKIDMARKERf_SET(miscconfig, 3);
    WRITE_MISCCONFIGr(unit, miscconfig);

    /* port-based : uplink */
    CDK_PBMP_ITER(pbmp_uplink, port) {
        lport = P2L(unit, port);

        PG_CTRL0r_CLR(pg_ctrl0);
        PG_CTRL0r_PPFC_PG_ENf_SET(pg_ctrl0, 0);
        PG_CTRL0r_PRI0_GRPf_SET(pg_ctrl0, 0);
        PG_CTRL0r_PRI1_GRPf_SET(pg_ctrl0, 1);
        PG_CTRL0r_PRI2_GRPf_SET(pg_ctrl0, 2);
        PG_CTRL0r_PRI3_GRPf_SET(pg_ctrl0, 3);
        PG_CTRL0r_PRI4_GRPf_SET(pg_ctrl0, 4);
        PG_CTRL0r_PRI5_GRPf_SET(pg_ctrl0, 5);
        PG_CTRL0r_PRI6_GRPf_SET(pg_ctrl0, 6);
        PG_CTRL0r_PRI7_GRPf_SET(pg_ctrl0, 7);
        WRITE_PG_CTRL0r(unit, lport, pg_ctrl0);

        PG_CTRL1r_CLR(pg_ctrl1);
        PG_CTRL1r_PRI8_GRPf_SET(pg_ctrl1, 0);
        PG_CTRL1r_PRI9_GRPf_SET(pg_ctrl1, 1);
        PG_CTRL1r_PRI10_GRPf_SET(pg_ctrl1, 2);
        PG_CTRL1r_PRI11_GRPf_SET(pg_ctrl1, 3);
        PG_CTRL1r_PRI12_GRPf_SET(pg_ctrl1, 4);
        PG_CTRL1r_PRI13_GRPf_SET(pg_ctrl1, 5);
        PG_CTRL1r_PRI14_GRPf_SET(pg_ctrl1, 6);
        PG_CTRL1r_PRI15_GRPf_SET(pg_ctrl1, 7);
        WRITE_PG_CTRL1r(unit, lport, pg_ctrl1);

        PG2TCr_CLR(pg2tc);
        for (idx = 0; idx <= 7; idx++) {
            WRITE_PG2TCr(unit, lport, idx, pg2tc);
        }

        /* IBPPKTSETLIMITr, index 0 */
        IBPPKTSETLIMITr_CLR(ibppktsetlimit);
        IBPPKTSETLIMITr_PKTSETLIMITf_SET(ibppktsetlimit, xoff_pkt_thrs_uport);
        IBPPKTSETLIMITr_RESETLIMITSELf_SET(ibppktsetlimit, 3);
        WRITE_IBPPKTSETLIMITr(unit, lport, ibppktsetlimit);

        /* MMU_FC_RX_ENr, index 0 */
        MMU_FC_RX_ENr_CLR(mmu_fc_rx_en);
        WRITE_MMU_FC_RX_ENr(unit, lport, mmu_fc_rx_en);

        MMU_FC_TX_ENr_CLR(mmu_fc_tx_en);
        WRITE_MMU_FC_TX_ENr(unit, lport, mmu_fc_tx_en);

        /* HOLCOSPKTSETLIMITr, index 0 ~ 7 */
        for (idx = 0; idx <= 7; idx++) {
            cos = idx;

            if (cos <= 2) {
                fval = holcospktsetlimit0_up;
            } else if (cos == 3) {
                fval = holcospktsetlimit3_up;
            } else {
                fval = 0;
            }
            HOLCOSPKTSETLIMITr_SET(holcospktsetlimit, fval);
            WRITE_HOLCOSPKTSETLIMITr(unit, lport, idx, holcospktsetlimit);
        }

        /* HOLCOSPKTRESETLIMITr, index 0 ~ 7 */
        for (idx = 0; idx <= 7; idx++) {
            cos = idx;

            if (cos <= 2) {
                fval = holcospktsetlimit0_up - 1;
            } else if (cos == 3) {
                fval = holcospktsetlimit3_up - 1;
            } else {
                fval = 0;
            }
            HOLCOSPKTRESETLIMITr_SET(holcospktresetlimit, fval);
            WRITE_HOLCOSPKTRESETLIMITr(unit, lport, idx, holcospktresetlimit);
        }

        /* HOLCOSMINXQCNTr, index 0 ~ 7 */
        for (idx = 0; idx <= 7; idx++) {
            cos = idx;

            if (cos <= 2) {
                fval = egress_xq_min_reserve_lossy_ports;
            } else {
                fval = 0;
            }
            HOLCOSMINXQCNTr_SET(holcosminxqcnt, fval);
            WRITE_HOLCOSMINXQCNTr(unit, lport, idx, holcosminxqcnt);
        }

        /* LWMCOSCELLSETLIMITr, index 0 ~ 7 */
        for (idx = 0; idx <= 7; idx++) {
            cos = idx;

            LWMCOSCELLSETLIMITr_CLR(lwmcoscellsetlimit);
            if (cos <= 2) {
                fval = egress_queue_min_reserve_uplink_ports_lossy;
            } else {
                fval = 0;
            }
            LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit, fval);
            WRITE_LWMCOSCELLSETLIMITr(unit, lport, idx, lwmcoscellsetlimit);
        }

        /* HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
        for (idx = 0; idx <= 7; idx++) {
            cos = idx;

            READ_HOLCOSCELLMAXLIMITr(unit, lport, idx, &holcoscellmaxlimit);
            if (cos <= 2) {
                fval = holcoscellmaxlimit0_cellmaxlimit_down_1;
            } else if (cos == 3) {
                fval = holcoscellmaxlimit3_cellmaxlimit_down_1;
            } else {
                fval = 0;
            }
            HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, fval);
            WRITE_HOLCOSCELLMAXLIMITr(unit, lport, idx, holcoscellmaxlimit);
        }

        /* PGCELLLIMITr, index 0 ~ 7 */
        PGCELLLIMITr_CLR(pgcelllimit);
        for (idx = 0; idx <= 7; idx++) {
            PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit, xoff_cell_thrs_uports);
            PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit, xoff_cell_thrs_uports);
            WRITE_PGCELLLIMITr(unit, lport, idx, pgcelllimit);
        }

        /* PGDISCARDSETLIMITr, index 0 ~ 7 */
        for (idx = 0; idx <= 7; idx++) {
            PGDISCARDSETLIMITr_SET(pgdiscardsetlimit, disc_pg_upport);
            WRITE_PGDISCARDSETLIMITr(unit, lport, idx, pgdiscardsetlimit);
        }

        /* CNGCOSPKTLIMIT0r, index 0 ~ 7 */
        CNGCOSPKTLIMIT0r_CLR(cngcospktlimit0);
        for (idx = 0; idx <= 7; idx++) {
            CNGCOSPKTLIMIT0r_SET(cngcospktlimit0, numxqs_per_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT0r(unit, lport, idx, cngcospktlimit0);
        }

        /* CNGCOSPKTLIMIT1r, index 0 ~ 7 */
        for (idx = 0; idx <= 7; idx++) {
            CNGCOSPKTLIMIT1r_SET(cngcospktlimit1, numxqs_per_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT1r(unit, lport, idx, cngcospktlimit1);
        }

        /* CNGPORTPKTLIMIT0r, index 0 */
        CNGPORTPKTLIMITr_SET(cngportpktlimit, numxqs_per_uplink_ports - 1);
        WRITE_CNGPORTPKTLIMITr(unit, lport, 0, cngportpktlimit);

        /* CNGPORTPKTLIMIT1r, index 0 */
        CNGPORTPKTLIMITr_SET(cngportpktlimit, numxqs_per_uplink_ports - 1);
        WRITE_CNGPORTPKTLIMITr(unit, lport, 1, cngportpktlimit);

        /* DYNXQCNTPORTr, index 0 */
        DYNXQCNTPORTr_SET(dynxqcntport, dynxqcntport_up);
        WRITE_DYNXQCNTPORTr(unit, lport, dynxqcntport);

        /* DYNRESETLIMPORTr, index 0 */
        DYNRESETLIMPORTr_SET(dynresetlimport, dynxqcntport_up - 2);
        WRITE_DYNRESETLIMPORTr(unit, lport, dynresetlimport);

        /* DYNCELLLIMITr, index 0 */
        DYNCELLLIMITr_CLR(dyncelllimit);
        DYNCELLLIMITr_DYNCELLSETLIMITf_SET(dyncelllimit,
                            shared_space_cells);
        DYNCELLLIMITr_DYNCELLRESETLIMITf_SET(dyncelllimit,
                            dyncelllimit_dyncellsetlimit_up -
                            (2 * ethernet_mtu_cell));
        WRITE_DYNCELLLIMITr(unit, lport, dyncelllimit);

        COLOR_DROP_ENr_CLR(color_drop_en);
        WRITE_COLOR_DROP_ENr(unit, lport, color_drop_en);

        /* SHARED_POOL_CTRLr, index 0 */
        SHARED_POOL_CTRLr_CLR(shared_pool_ctrl);
        SHARED_POOL_CTRLr_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl, 255);
        SHARED_POOL_CTRLr_SHARED_POOL_DISCARD_ENf_SET(shared_pool_ctrl, 255);
        WRITE_SHARED_POOL_CTRLr(unit, lport, shared_pool_ctrl);
    }

    /* port-based : downlink 1G */
    CDK_PBMP_ITER(pbmp_downlink_1g, port) {
        lport = P2L(unit, port);

        PG_CTRL0r_CLR(pg_ctrl0);
        PG_CTRL0r_PPFC_PG_ENf_SET(pg_ctrl0, 0);
        PG_CTRL0r_PRI0_GRPf_SET(pg_ctrl0, 0);
        PG_CTRL0r_PRI1_GRPf_SET(pg_ctrl0, 1);
        PG_CTRL0r_PRI2_GRPf_SET(pg_ctrl0, 2);
        PG_CTRL0r_PRI3_GRPf_SET(pg_ctrl0, 3);
        PG_CTRL0r_PRI4_GRPf_SET(pg_ctrl0, 4);
        PG_CTRL0r_PRI5_GRPf_SET(pg_ctrl0, 5);
        PG_CTRL0r_PRI6_GRPf_SET(pg_ctrl0, 6);
        PG_CTRL0r_PRI7_GRPf_SET(pg_ctrl0, 7);
        WRITE_PG_CTRL0r(unit, lport, pg_ctrl0);

        PG_CTRL1r_CLR(pg_ctrl1);
        PG_CTRL1r_PRI8_GRPf_SET(pg_ctrl1, 0);
        PG_CTRL1r_PRI9_GRPf_SET(pg_ctrl1, 1);
        PG_CTRL1r_PRI10_GRPf_SET(pg_ctrl1, 2);
        PG_CTRL1r_PRI11_GRPf_SET(pg_ctrl1, 3);
        PG_CTRL1r_PRI12_GRPf_SET(pg_ctrl1, 4);
        PG_CTRL1r_PRI13_GRPf_SET(pg_ctrl1, 5);
        PG_CTRL1r_PRI14_GRPf_SET(pg_ctrl1, 6);
        PG_CTRL1r_PRI15_GRPf_SET(pg_ctrl1, 7);
        WRITE_PG_CTRL1r(unit, lport, pg_ctrl1);

        PG2TCr_CLR(pg2tc);
        for (idx = 0; idx <= 7; idx++) {
            WRITE_PG2TCr(unit, lport, idx, pg2tc);
        }

        /* IBPPKTSETLIMITr, index 0 */
        IBPPKTSETLIMITr_CLR(ibppktsetlimit);
        IBPPKTSETLIMITr_PKTSETLIMITf_SET(ibppktsetlimit, xoff_pkt_thrs_dport);
        IBPPKTSETLIMITr_RESETLIMITSELf_SET(ibppktsetlimit, 3);
        WRITE_IBPPKTSETLIMITr(unit, lport, ibppktsetlimit);

        /* MMU_FC_RX_ENr, index 0 */
        MMU_FC_RX_ENr_CLR(mmu_fc_rx_en);
        WRITE_MMU_FC_RX_ENr(unit, lport, mmu_fc_rx_en);

        MMU_FC_TX_ENr_CLR(mmu_fc_tx_en);
        WRITE_MMU_FC_TX_ENr(unit, lport, mmu_fc_tx_en);

        /* HOLCOSPKTSETLIMITr, index 0 ~ 7 */
        for (idx = 0; idx <= 7; idx++) {
            cos = idx;

            if (cos <= 2) {
                fval = holcospktsetlimit0_pktsetlimit_down_1;
            } else if (cos == 3) {
                fval = holcospktsetlimit3_pktsetlimit_down_1;
            } else {
                fval = 0;
            }
            HOLCOSPKTSETLIMITr_SET(holcospktsetlimit, fval);
            WRITE_HOLCOSPKTSETLIMITr(unit, lport, idx, holcospktsetlimit);
        }

        /* HOLCOSPKTRESETLIMITr, index 0 ~ 7 */
        for (idx = 0; idx <= 7; idx++) {
            cos = idx;

            if (cos <= 2) {
                fval = holcospktsetlimit0_pktsetlimit_down_1 - 1;
            } else if (cos == 3) {
                fval = holcospktsetlimit3_pktsetlimit_down_1 - 1;
            } else {
                fval = 0;
            }
            HOLCOSPKTRESETLIMITr_SET(holcospktresetlimit, fval);
            WRITE_HOLCOSPKTRESETLIMITr(unit, lport, idx, holcospktresetlimit);
        }

        /* HOLCOSMINXQCNTr, index 0 ~ 7 */
        for (idx = 0; idx <= 7; idx++) {
            cos = idx;

            if (cos <= 2) {
                HOLCOSMINXQCNTr_SET(holcosminxqcnt, egress_xq_min_reserve_lossy_ports);
            } else {
                HOLCOSMINXQCNTr_SET(holcosminxqcnt, 0);
            }
            WRITE_HOLCOSMINXQCNTr(unit, lport, idx, holcosminxqcnt);
        }

        /* LWMCOSCELLSETLIMITr, index 0 ~ 7 */
        for (idx = 0; idx <= 7; idx++) {
            cos = idx;

            LWMCOSCELLSETLIMITr_CLR(lwmcoscellsetlimit);
            if (cos <= 2) {
                fval = egress_queue_min_reserve_uplink_ports_lossy;
            } else {
                fval = 0;
            }
            LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit, fval);
            WRITE_LWMCOSCELLSETLIMITr(unit, lport, idx, lwmcoscellsetlimit);
        }

        /* HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
        for (idx = 0; idx <= 7; idx++) {
            cos = idx;

            READ_HOLCOSCELLMAXLIMITr(unit, lport, idx, &holcoscellmaxlimit);
            if (cos <= 3) {
                fval = holcoscellmaxlimit3_cellmaxlimit_down_1;
            } else {
                fval = 0;
            }
            HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, fval);
            WRITE_HOLCOSCELLMAXLIMITr(unit, lport, idx, holcoscellmaxlimit);
        }

        /* PGCELLLIMITr, index 0 ~ 7 */
        PGCELLLIMITr_CLR(pgcelllimit);
        for (idx = 0; idx <= 7; idx++) {
            PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit, xoff_1g_dports);
            PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit, xoff_1g_dports);
            WRITE_PGCELLLIMITr(unit, lport, idx, pgcelllimit);
        }

        /* PGDISCARDSETLIMITr, index 0 ~ 7 */
        for (idx = 0; idx <= 7; idx++) {
            PGDISCARDSETLIMITr_SET(pgdiscardsetlimit, disc_pg_dwnport);
            WRITE_PGDISCARDSETLIMITr(unit, lport, idx, pgdiscardsetlimit);
        }

        /* CNGCOSPKTLIMIT0r, index 0 ~ 7 */
        CNGCOSPKTLIMIT0r_CLR(cngcospktlimit0);
        for (idx = 0; idx <= 7; idx++) {
            CNGCOSPKTLIMIT0r_SET(cngcospktlimit0, numxqs_per_downlink_ports_and_cpu_port - 1);
            WRITE_CNGCOSPKTLIMIT0r(unit, lport, idx, cngcospktlimit0);
        }

        /* CNGCOSPKTLIMIT1r, index 0 ~ 7 */
        for (idx = 0; idx <= 7; idx++) {
            CNGCOSPKTLIMIT1r_SET(cngcospktlimit1, numxqs_per_downlink_ports_and_cpu_port - 1);
            WRITE_CNGCOSPKTLIMIT1r(unit, lport, idx, cngcospktlimit1);
        }

        /* CNGPORTPKTLIMITr */
        CNGPORTPKTLIMITr_SET(cngportpktlimit, numxqs_per_downlink_ports_and_cpu_port - 1);
        WRITE_CNGPORTPKTLIMITr(unit, lport, 0, cngportpktlimit);
        WRITE_CNGPORTPKTLIMITr(unit, lport, 1, cngportpktlimit);

        /* DYNXQCNTPORTr, index 0 */
        DYNXQCNTPORTr_SET(dynxqcntport,
                            shared_xqs_per_downlink_port - skidmarker - prefetch);
        WRITE_DYNXQCNTPORTr(unit, lport, dynxqcntport);

        /* DYNRESETLIMPORTr, index 0 */
        DYNRESETLIMPORTr_SET(dynresetlimport, dynxqcntport_dynxqcntport_down_1 - 2);
        WRITE_DYNRESETLIMPORTr(unit, lport, dynresetlimport);

        /* DYNCELLLIMITr, index 0 */
        fval = dyncelllimit_dyncellsetlimit_down_1 - 2 * ethernet_mtu_cell;
        DYNCELLLIMITr_CLR(dyncelllimit);
        DYNCELLLIMITr_DYNCELLSETLIMITf_SET(dyncelllimit, shared_space_cells);
        DYNCELLLIMITr_DYNCELLRESETLIMITf_SET(dyncelllimit, fval);
        WRITE_DYNCELLLIMITr(unit, lport, dyncelllimit);

        COLOR_DROP_ENr_CLR(color_drop_en);
        WRITE_COLOR_DROP_ENr(unit, lport, color_drop_en);

        /* SHARED_POOL_CTRLr, index 0 */
        SHARED_POOL_CTRLr_CLR(shared_pool_ctrl);
        SHARED_POOL_CTRLr_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl, 255);
        SHARED_POOL_CTRLr_SHARED_POOL_DISCARD_ENf_SET(shared_pool_ctrl, 255);
        WRITE_SHARED_POOL_CTRLr(unit, lport, shared_pool_ctrl);
    }

    /* port-based : downlink 2.5G */
    CDK_PBMP_ITER(pbmp_downlink_2dot5g, port) {
        lport = P2L(unit, port);

        PG_CTRL0r_CLR(pg_ctrl0);
        PG_CTRL0r_PPFC_PG_ENf_SET(pg_ctrl0, 0);
        PG_CTRL0r_PRI0_GRPf_SET(pg_ctrl0, 0);
        PG_CTRL0r_PRI1_GRPf_SET(pg_ctrl0, 1);
        PG_CTRL0r_PRI2_GRPf_SET(pg_ctrl0, 2);
        PG_CTRL0r_PRI3_GRPf_SET(pg_ctrl0, 3);
        PG_CTRL0r_PRI4_GRPf_SET(pg_ctrl0, 4);
        PG_CTRL0r_PRI5_GRPf_SET(pg_ctrl0, 5);
        PG_CTRL0r_PRI6_GRPf_SET(pg_ctrl0, 6);
        PG_CTRL0r_PRI7_GRPf_SET(pg_ctrl0, 7);
        WRITE_PG_CTRL0r(unit, lport, pg_ctrl0);

        PG_CTRL1r_CLR(pg_ctrl1);
        PG_CTRL1r_PRI8_GRPf_SET(pg_ctrl1, 0);
        PG_CTRL1r_PRI9_GRPf_SET(pg_ctrl1, 1);
        PG_CTRL1r_PRI10_GRPf_SET(pg_ctrl1, 2);
        PG_CTRL1r_PRI11_GRPf_SET(pg_ctrl1, 3);
        PG_CTRL1r_PRI12_GRPf_SET(pg_ctrl1, 4);
        PG_CTRL1r_PRI13_GRPf_SET(pg_ctrl1, 5);
        PG_CTRL1r_PRI14_GRPf_SET(pg_ctrl1, 6);
        PG_CTRL1r_PRI15_GRPf_SET(pg_ctrl1, 7);
        WRITE_PG_CTRL1r(unit, lport, pg_ctrl1);

        PG2TCr_CLR(pg2tc);
        for (idx = 0; idx <= 7; idx++) {
            WRITE_PG2TCr(unit, lport, idx, pg2tc);
        }

        /* IBPPKTSETLIMITr, index 0 */
        IBPPKTSETLIMITr_CLR(ibppktsetlimit);
        IBPPKTSETLIMITr_PKTSETLIMITf_SET(ibppktsetlimit, xoff_pkt_thrs_uport);
        IBPPKTSETLIMITr_RESETLIMITSELf_SET(ibppktsetlimit, 3);
        WRITE_IBPPKTSETLIMITr(unit, lport, ibppktsetlimit);

        /* MMU_FC_RX_ENr, index 0 */
        MMU_FC_RX_ENr_CLR(mmu_fc_rx_en);
        WRITE_MMU_FC_RX_ENr(unit, lport, mmu_fc_rx_en);

        MMU_FC_TX_ENr_CLR(mmu_fc_tx_en);
        WRITE_MMU_FC_TX_ENr(unit, lport, mmu_fc_tx_en);

        /* HOLCOSPKTSETLIMITr, index 0 ~ 7 */
        for (idx = 0; idx <= 7; idx++) {
            cos = idx;

            if (cos <= 2) {
                fval = holcospktsetlimit0_pktsetlimit_down_2dot5;
            } else if (cos == 3) {
                fval = holcospktsetlimit3_pktsetlimit_down_2dot5;
            } else {
                fval = 0;
            }
            HOLCOSPKTSETLIMITr_SET(holcospktsetlimit, fval);
            WRITE_HOLCOSPKTSETLIMITr(unit, lport, idx, holcospktsetlimit);
        }

        /* HOLCOSPKTRESETLIMITr, index 0 ~ 7 */
        for (idx = 0; idx <= 7; idx++) {
            cos = idx;

            if (cos <= 2) {
                fval = holcospktsetlimit0_pktsetlimit_down_2dot5 - 1;
            } else if (cos == 3) {
                fval = holcospktsetlimit3_pktsetlimit_down_2dot5 - 1;
            } else {
                fval = 0;
            }
            HOLCOSPKTRESETLIMITr_SET(holcospktresetlimit, fval);
            WRITE_HOLCOSPKTRESETLIMITr(unit, lport, idx, holcospktresetlimit);
        }

        /* HOLCOSMINXQCNTr, index 0 ~ 7 */
        for (idx = 0; idx <= 7; idx++) {
            cos = idx;

            if (cos <= 2) {
                fval = egress_xq_min_reserve_lossy_ports;
            } else {
                fval = 0;
            }
            HOLCOSMINXQCNTr_SET(holcosminxqcnt, fval);
            WRITE_HOLCOSMINXQCNTr(unit, lport, idx, holcosminxqcnt);
        }

        /* LWMCOSCELLSETLIMITr, index 0 ~ 7 */
        for (idx = 0; idx <= 7; idx++) {
            cos = idx;

            LWMCOSCELLSETLIMITr_CLR(lwmcoscellsetlimit);
            if (cos <= 2) {
                fval = egress_queue_min_reserve_uplink_ports_lossy;
            } else {
                fval = 0;
            }
            LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit, fval);
            WRITE_LWMCOSCELLSETLIMITr(unit, lport, idx, lwmcoscellsetlimit);
        }

        /* HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
        for (idx = 0; idx <= 7; idx++) {
            cos = idx;

            READ_HOLCOSCELLMAXLIMITr(unit, lport, idx, &holcoscellmaxlimit);
            if (cos <= 2) {
                fval = holcoscellmaxlimit0_cellmaxlimit_down_2dot5;
            } else if (cos == 3) {
                fval = holcoscellmaxlimit3_cellmaxlimit_down_2dot5;
            } else {
                fval = 0;
            }
            HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, fval);
            WRITE_HOLCOSCELLMAXLIMITr(unit, lport, idx, holcoscellmaxlimit);
        }

        /* PGCELLLIMITr, index 0 ~ 7 */
        PGCELLLIMITr_CLR(pgcelllimit);
        for (idx = 0; idx <= 7; idx++) {
            PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit, xoff_2dot5g_dports);
            PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit, xoff_2dot5g_dports);
            WRITE_PGCELLLIMITr(unit, lport, idx, pgcelllimit);
        }

        /* PGDISCARDSETLIMITr, index 0 ~ 7 */
        for (idx = 0; idx <= 7; idx++) {
            PGDISCARDSETLIMITr_SET(pgdiscardsetlimit, disc_pg_dwnport);
            WRITE_PGDISCARDSETLIMITr(unit, lport, idx, pgdiscardsetlimit);
        }

        /* CNGCOSPKTLIMIT0r, index 0 ~ 7 */
        CNGCOSPKTLIMIT0r_CLR(cngcospktlimit0);
        for (idx = 0; idx <= 7; idx++) {
            CNGCOSPKTLIMIT0r_SET(cngcospktlimit0, numxqs_per_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT0r(unit, lport, idx, cngcospktlimit0);
        }

        /* CNGCOSPKTLIMIT1r, index 0 ~ 7 */
        for (idx = 0; idx <= 7; idx++) {
            CNGCOSPKTLIMIT1r_SET(cngcospktlimit1, numxqs_per_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT1r(unit, lport, idx, cngcospktlimit1);
        }

        /* CNGPORTPKTLIMITr */
        CNGPORTPKTLIMITr_SET(cngportpktlimit, numxqs_per_uplink_ports - 1);
        WRITE_CNGPORTPKTLIMITr(unit, lport, 0, cngportpktlimit);
        WRITE_CNGPORTPKTLIMITr(unit, lport, 1, cngportpktlimit);

        /* DYNXQCNTPORTr, index 0 */
        DYNXQCNTPORTr_SET(dynxqcntport,
                            shared_xqs_per_uplink_port - skidmarker - prefetch);
        WRITE_DYNXQCNTPORTr(unit, lport, dynxqcntport);

        /* DYNRESETLIMPORTr, index 0 */
        DYNRESETLIMPORTr_SET(dynresetlimport, dynxqcntport_dynxqcntport_down_2dot5 - 2);
        WRITE_DYNRESETLIMPORTr(unit, lport, dynresetlimport);

        /* DYNCELLLIMITr, index 0 */
        DYNCELLLIMITr_CLR(dyncelllimit);
        DYNCELLLIMITr_DYNCELLSETLIMITf_SET(dyncelllimit,
                            shared_space_cells);
        DYNCELLLIMITr_DYNCELLRESETLIMITf_SET(dyncelllimit,
                            dyncelllimit_dyncellsetlimit_down_2dot5 -
                            (2 * ethernet_mtu_cell));
        WRITE_DYNCELLLIMITr(unit, lport, dyncelllimit);

        COLOR_DROP_ENr_CLR(color_drop_en);
        WRITE_COLOR_DROP_ENr(unit, lport, color_drop_en);

        /* SHARED_POOL_CTRLr, index 0 */
        SHARED_POOL_CTRLr_CLR(shared_pool_ctrl);
        SHARED_POOL_CTRLr_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl, 255);
        SHARED_POOL_CTRLr_SHARED_POOL_DISCARD_ENf_SET(shared_pool_ctrl, 255);
        WRITE_SHARED_POOL_CTRLr(unit, lport, shared_pool_ctrl);
    }

    /* CMIC */
    lport = CMIC_PORT;

    PG_CTRL0r_CLR(pg_ctrl0);
    PG_CTRL0r_PPFC_PG_ENf_SET(pg_ctrl0, 0);
    PG_CTRL0r_PRI0_GRPf_SET(pg_ctrl0, 0);
    PG_CTRL0r_PRI1_GRPf_SET(pg_ctrl0, 1);
    PG_CTRL0r_PRI2_GRPf_SET(pg_ctrl0, 2);
    PG_CTRL0r_PRI3_GRPf_SET(pg_ctrl0, 3);
    PG_CTRL0r_PRI4_GRPf_SET(pg_ctrl0, 4);
    PG_CTRL0r_PRI5_GRPf_SET(pg_ctrl0, 5);
    PG_CTRL0r_PRI6_GRPf_SET(pg_ctrl0, 6);
    PG_CTRL0r_PRI7_GRPf_SET(pg_ctrl0, 7);
    WRITE_PG_CTRL0r(unit, lport, pg_ctrl0);

    PG_CTRL1r_CLR(pg_ctrl1);
    PG_CTRL1r_PRI8_GRPf_SET(pg_ctrl1, 0);
    PG_CTRL1r_PRI9_GRPf_SET(pg_ctrl1, 1);
    PG_CTRL1r_PRI10_GRPf_SET(pg_ctrl1, 2);
    PG_CTRL1r_PRI11_GRPf_SET(pg_ctrl1, 3);
    PG_CTRL1r_PRI12_GRPf_SET(pg_ctrl1, 4);
    PG_CTRL1r_PRI13_GRPf_SET(pg_ctrl1, 5);
    PG_CTRL1r_PRI14_GRPf_SET(pg_ctrl1, 6);
    PG_CTRL1r_PRI15_GRPf_SET(pg_ctrl1, 7);
    WRITE_PG_CTRL1r(unit, lport, pg_ctrl1);

    PG2TCr_CLR(pg2tc);
    for (idx = 0; idx <= 7; idx++) {
        WRITE_PG2TCr(unit, lport, idx, pg2tc);
    }

    /* IBPPKTSETLIMITr, index 0 */
    IBPPKTSETLIMITr_CLR(ibppktsetlimit);
    IBPPKTSETLIMITr_PKTSETLIMITf_SET(ibppktsetlimit, xoff_pkt_thrs_uport);
    IBPPKTSETLIMITr_RESETLIMITSELf_SET(ibppktsetlimit, 3);
    WRITE_IBPPKTSETLIMITr(unit, lport, ibppktsetlimit);

    /* MMU_FC_RX_ENr, index 0 */
    MMU_FC_RX_ENr_CLR(mmu_fc_rx_en);
    WRITE_MMU_FC_RX_ENr(unit, lport, mmu_fc_rx_en);

    MMU_FC_TX_ENr_CLR(mmu_fc_tx_en);
    WRITE_MMU_FC_TX_ENr(unit, lport, mmu_fc_tx_en);

    /* HOLCOSPKTSETLIMITr, index 0 ~ 7 */
    for (idx = 0; idx <= 7; idx++) {
        cos = idx;

        if (cos <= 2) {
            fval = holcospktsetlimit0_pktsetlimit_cpu;
        } else if (cos == 3) {
            fval = holcospktsetlimit3_pktsetlimit_cpu;
        } else {
            fval = 0;
        }
        HOLCOSPKTSETLIMITr_SET(holcospktsetlimit, fval);
        WRITE_HOLCOSPKTSETLIMITr(unit, lport, idx, holcospktsetlimit);
    }

    /* HOLCOSPKTRESETLIMITr, index 0 ~ 7 */
    for (idx = 0; idx <= 7; idx++) {
        cos = idx;

        if (cos <= 2) {
            fval = holcospktsetlimit0_pktsetlimit_cpu - 1;
        } else if (cos == 3) {
            fval = holcospktsetlimit3_pktsetlimit_cpu - 1;
        } else {
            fval = 0;
        }
        HOLCOSPKTRESETLIMITr_SET(holcospktresetlimit, fval);
        WRITE_HOLCOSPKTRESETLIMITr(unit, lport, idx, holcospktresetlimit);
    }

    /* HOLCOSMINXQCNTr, index 0 ~ 7 */
    for (idx = 0; idx <= 7; idx++) {
        cos = idx;

        if (cos <= 2) {
            fval = egress_xq_min_reserve_lossy_ports;
        } else {
            fval = 0;
        }
        HOLCOSMINXQCNTr_SET(holcosminxqcnt, fval);
        WRITE_HOLCOSMINXQCNTr(unit, lport, idx, holcosminxqcnt);
    }

    /* LWMCOSCELLSETLIMITr, index 0 ~ 7 */
    for (idx = 0; idx <= 7; idx++) {
        cos = idx;

        LWMCOSCELLSETLIMITr_CLR(lwmcoscellsetlimit);
        if (cos <= 2) {
            fval = egress_queue_min_reserve_uplink_ports_lossy;
        } else {
            fval = 0;
        }
        LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit, fval);
        WRITE_LWMCOSCELLSETLIMITr(unit, lport, idx, lwmcoscellsetlimit);
    }

        /* HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
    for (idx = 0; idx <= 7; idx++) {
        cos = idx;

        READ_HOLCOSCELLMAXLIMITr(unit, lport, idx, &holcoscellmaxlimit);
        if (cos <= 2) {
            fval = holcoscellmaxlimit0_cellmaxlimit_cpu;
        } else if (cos == 3) {
            fval = holcoscellmaxlimit3_cellmaxlimit_cpu;
        } else {
            fval = 0;
        }
        HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, fval);
        WRITE_HOLCOSCELLMAXLIMITr(unit, lport, idx, holcoscellmaxlimit);
    }

    /* PGCELLLIMITr, index 0 ~ 7 */
    PGCELLLIMITr_CLR(pgcelllimit);
    for (idx = 0; idx <= 7; idx++) {
        PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit, xoff_cell_thrs_uports);
        PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit, xoff_cell_thrs_uports);
        WRITE_PGCELLLIMITr(unit, lport, idx, pgcelllimit);
    }

    /* PGDISCARDSETLIMITr, index 0 ~ 7 */
    for (idx = 0; idx <= 7; idx++) {
        PGDISCARDSETLIMITr_SET(pgdiscardsetlimit, disc_pg_dwnport);
        WRITE_PGDISCARDSETLIMITr(unit, lport, idx, pgdiscardsetlimit);
    }

    /* CNGCOSPKTLIMIT0r, index 0 ~ 7 */
    CNGCOSPKTLIMIT0r_CLR(cngcospktlimit0);
    for (idx = 0; idx <= 7; idx++) {
        CNGCOSPKTLIMIT0r_SET(cngcospktlimit0, numxqs_per_uplink_ports - 1);
        WRITE_CNGCOSPKTLIMIT0r(unit, lport, idx, cngcospktlimit0);
    }

    /* CNGCOSPKTLIMIT1r, index 0 ~ 7 */
    for (idx = 0; idx <= 7; idx++) {
        CNGCOSPKTLIMIT1r_SET(cngcospktlimit1, numxqs_per_uplink_ports - 1);
        WRITE_CNGCOSPKTLIMIT1r(unit, lport, idx, cngcospktlimit1);
    }

    /* CNGPORTPKTLIMITr*/
    CNGPORTPKTLIMITr_SET(cngportpktlimit, numxqs_per_uplink_ports - 1);
    WRITE_CNGPORTPKTLIMITr(unit, lport, 0, cngportpktlimit);
    WRITE_CNGPORTPKTLIMITr(unit, lport, 1, cngportpktlimit);

    /* DYNXQCNTPORTr, index 0 */
    fval = shared_xqs_per_uplink_port - skidmarker - prefetch;
    DYNXQCNTPORTr_SET(dynxqcntport, fval);
    WRITE_DYNXQCNTPORTr(unit, lport, dynxqcntport);

    /* DYNRESETLIMPORTr, index 0 */
    DYNRESETLIMPORTr_SET(dynresetlimport, dynxqcntport_dynxqcntport_cpu - 2);
    WRITE_DYNRESETLIMPORTr(unit, lport, dynresetlimport);

    /* DYNCELLLIMITr, index 0 */
    fval = dyncelllimit_dyncellsetlimit_cpu - 2 * ethernet_mtu_cell;
    DYNCELLLIMITr_CLR(dyncelllimit);
    DYNCELLLIMITr_DYNCELLSETLIMITf_SET(dyncelllimit, shared_space_cells);
    DYNCELLLIMITr_DYNCELLRESETLIMITf_SET(dyncelllimit, fval);
    WRITE_DYNCELLLIMITr(unit, lport, dyncelllimit);

    COLOR_DROP_ENr_CLR(color_drop_en);
    WRITE_COLOR_DROP_ENr(unit, lport, color_drop_en);

    /* SHARED_POOL_CTRLr, index 0 */
    SHARED_POOL_CTRLr_CLR(shared_pool_ctrl);
    SHARED_POOL_CTRLr_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl, 255);
    SHARED_POOL_CTRLr_SHARED_POOL_DISCARD_ENf_SET(shared_pool_ctrl, 255);
    WRITE_SHARED_POOL_CTRLr(unit, lport, shared_pool_ctrl);

    return CDK_E_NONE;
}

static int
_mmu_init(int unit)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    CFAPCONFIGr_t cfapconfig;
    PKTAGINGTIMERr_t pkt_ag_tim;
    PKTAGINGLIMITr_t pkt_ag_lim;
    MMUPORTENABLEr_t mmu_port_en;
    IP_TO_CMICM_CREDIT_TRANSFERr_t cmic_cred_xfer;

    /* Setup TDM for MMU */
    rv = _mmu_tdm_init(unit);

    ioerr += READ_CFAPCONFIGr(unit, &cfapconfig);
    CFAPCONFIGr_CFAPPOOLSIZEf_SET(cfapconfig, MMU_CFAPm_MAX);
    ioerr += WRITE_CFAPCONFIGr(unit, cfapconfig);

    rv = _mmu_lossy_init_helper(unit);

    /* Enable IP to CMICM credit transfer */
    IP_TO_CMICM_CREDIT_TRANSFERr_CLR(cmic_cred_xfer);
    IP_TO_CMICM_CREDIT_TRANSFERr_TRANSFER_ENABLEf_SET(cmic_cred_xfer, 1);
    IP_TO_CMICM_CREDIT_TRANSFERr_NUM_OF_CREDITSf_SET(cmic_cred_xfer, 32);
    ioerr += WRITE_IP_TO_CMICM_CREDIT_TRANSFERr(unit, cmic_cred_xfer);

    /* Disable packet aging on all COSQs */
    PKTAGINGTIMERr_CLR(pkt_ag_tim);
    ioerr += WRITE_PKTAGINGTIMERr(unit, pkt_ag_tim);
    PKTAGINGLIMITr_CLR(pkt_ag_lim);
    ioerr += WRITE_PKTAGINGLIMITr(unit, pkt_ag_lim);

    MMUPORTENABLEr_MMUPORTENABLEf_SET(mmu_port_en, 0x3ffffffd);
    ioerr += WRITE_MMUPORTENABLEr(unit, mmu_port_en);

    return ioerr ? CDK_E_IO : rv;
}

static int
_port_init(int unit, int port)
{
    int ioerr = 0;
    EGR_ENABLEm_t egr_enable;
    EGR_PORT_64r_t egr_port;
    IEGR_PORT_64r_t iegr_port;
    EGR_VLAN_CONTROL_1r_t egr_vlan_ctrl1;
    PORT_TABm_t port_tab;
    int lport;

    lport = P2L(unit, port);
    if (lport < 0) {
        return 0;
    }

    /* Default port VLAN and tag action, enable L2 HW learning */
    ioerr += READ_PORT_TABm(unit, lport, &port_tab);
    PORT_TABm_PORT_VIDf_SET(port_tab, 1);
    PORT_TABm_FILTER_ENABLEf_SET(port_tab, 1);
    PORT_TABm_OUTER_TPID_ENABLEf_SET(port_tab, 1);
    PORT_TABm_CML_FLAGS_NEWf_SET(port_tab, 8);
    PORT_TABm_CML_FLAGS_MOVEf_SET(port_tab, 8);
    ioerr += WRITE_PORT_TABm(unit, lport, port_tab);

    /* Filter VLAN on egress */
    ioerr += READ_EGR_PORT_64r(unit, lport, &egr_port);
    EGR_PORT_64r_EN_EFILTERf_SET(egr_port, 1);
    ioerr += WRITE_EGR_PORT_64r(unit, lport, egr_port);

    /* Directed Mirroring ON by default */
    ioerr += READ_EGR_PORT_64r(unit, lport, &egr_port);
    EGR_PORT_64r_EM_SRCMOD_CHANGEf_SET(egr_port, 1);
    ioerr += WRITE_EGR_PORT_64r(unit, lport, egr_port);
    ioerr += READ_IEGR_PORT_64r(unit, lport, &iegr_port);
    IEGR_PORT_64r_EM_SRCMOD_CHANGEf_SET(iegr_port, 1);
    ioerr += WRITE_IEGR_PORT_64r(unit, lport, iegr_port);

    /* Configure egress VLAN for backward compatibility */
    EGR_VLAN_CONTROL_1r_CLR(egr_vlan_ctrl1);
    EGR_VLAN_CONTROL_1r_VT_MISS_UNTAGf_SET(egr_vlan_ctrl1, 0);
    EGR_VLAN_CONTROL_1r_REMARK_OUTER_DOT1Pf_SET(egr_vlan_ctrl1, 1);
    ioerr += WRITE_EGR_VLAN_CONTROL_1r(unit, lport, egr_vlan_ctrl1);

    /* Egress enable */
    EGR_ENABLEm_CLR(egr_enable);
    EGR_ENABLEm_PRT_ENABLEf_SET(egr_enable, 1);
    ioerr += WRITE_EGR_ENABLEm(unit, port, egr_enable);

    return ioerr;
}

static int
_gport_init(int unit, int port)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    COMMAND_CONFIGr_t command_cfg;
    PAUSE_CONTROLr_t pause_ctrl;
    PAUSE_QUANTr_t pause_quant;
    MAC_PFC_REFRESH_CTRLr_t mac_pfc_refresh;
    TX_IPG_LENGTHr_t tx_ipg_len;
    cdk_pbmp_t pbmp;

    ioerr += _port_init(unit, port);

    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_GPORT, &pbmp);
    if (CDK_PBMP_MEMBER(pbmp, port)) {
        /* GXMAC init */
        ioerr += READ_COMMAND_CONFIGr(unit, port, &command_cfg);
        COMMAND_CONFIGr_SW_RESETf_SET(command_cfg, 1);
        ioerr += WRITE_COMMAND_CONFIGr(unit, port, command_cfg);

        /* Ensure that MAC and loopback mode is disabled */
        ioerr += READ_COMMAND_CONFIGr(unit, port, &command_cfg);
        COMMAND_CONFIGr_LOOP_ENAf_SET(command_cfg, 0);
        COMMAND_CONFIGr_RX_ENAf_SET(command_cfg, 0);
        COMMAND_CONFIGr_TX_ENAf_SET(command_cfg, 0);
        ioerr += WRITE_COMMAND_CONFIGr(unit, port, command_cfg);

        ioerr += READ_COMMAND_CONFIGr(unit, port, &command_cfg);
        COMMAND_CONFIGr_SW_RESETf_SET(command_cfg, 0);
        ioerr += WRITE_COMMAND_CONFIGr(unit, port, command_cfg);

        PAUSE_CONTROLr_CLR(pause_ctrl);
        PAUSE_CONTROLr_ENABLEf_SET(pause_ctrl, 1);
        PAUSE_CONTROLr_VALUEf_SET(pause_ctrl, 0x1ffff);
        ioerr += WRITE_PAUSE_CONTROLr(unit, port, pause_ctrl);

        PAUSE_QUANTr_SET(pause_quant, 0xffff);
        ioerr += WRITE_PAUSE_QUANTr(unit, port, pause_quant);

        ioerr += READ_MAC_PFC_REFRESH_CTRLr(unit, port, &mac_pfc_refresh);
        MAC_PFC_REFRESH_CTRLr_PFC_REFRESH_ENf_SET(mac_pfc_refresh, 1);
        MAC_PFC_REFRESH_CTRLr_PFC_REFRESH_TIMERf_SET(mac_pfc_refresh, 0xc000);
        ioerr += WRITE_MAC_PFC_REFRESH_CTRLr(unit, port, mac_pfc_refresh);

        TX_IPG_LENGTHr_SET(tx_ipg_len, 12);
        ioerr += WRITE_TX_IPG_LENGTHr(unit, port, tx_ipg_len);
    }

    return ioerr ? CDK_E_IO : rv;
}


int
bcm53540_a0_bmd_init(int unit)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    GPORT_RSV_MASKr_t gport_rsv_mask;
    GPORT_CONFIGr_t gport_cfg;
    VLAN_PROFILE_TABm_t vlan_profile;
    ING_VLAN_TAG_ACTION_PROFILEm_t vlan_action;
    EGR_VLAN_TAG_ACTION_PROFILEm_t egr_action;
    cdk_pbmp_t gpbmp;
    int port;
#if BMD_CONFIG_INCLUDE_DMA
    int idx;
#endif

#if BMD_CONFIG_INCLUDE_PHY == 1
    int lanes;
#endif

    BMD_CHECK_UNIT(unit);

    rv = _misc_init(unit);
    if (CDK_FAILURE(rv)) {
        return rv;
    }

    /* Initialize MMU */
    rv = _mmu_init(unit);
    if (CDK_FAILURE(rv)) {
        return rv;
    }

    /* Default VLAN profile */
    VLAN_PROFILE_TABm_CLR(vlan_profile);
    VLAN_PROFILE_TABm_L2_PFMf_SET(vlan_profile, 1);
    VLAN_PROFILE_TABm_L3_IPV4_PFMf_SET(vlan_profile, 1);
    VLAN_PROFILE_TABm_L3_IPV6_PFMf_SET(vlan_profile, 1);
    VLAN_PROFILE_TABm_IPMCV6_ENABLEf_SET(vlan_profile, 1);
    VLAN_PROFILE_TABm_IPMCV4_ENABLEf_SET(vlan_profile, 1);
    VLAN_PROFILE_TABm_IPMCV6_L2_ENABLEf_SET(vlan_profile, 1);
    VLAN_PROFILE_TABm_IPMCV4_L2_ENABLEf_SET(vlan_profile, 1);
    VLAN_PROFILE_TABm_IPV6L3_ENABLEf_SET(vlan_profile, 1);
    VLAN_PROFILE_TABm_IPV4L3_ENABLEf_SET(vlan_profile, 1);
    ioerr += WRITE_VLAN_PROFILE_TABm(unit, VLAN_PROFILE_TABm_MAX, vlan_profile);

    /* Ensure that all incoming packets get tagged appropriately */
    ING_VLAN_TAG_ACTION_PROFILEm_CLR(vlan_action);
    ING_VLAN_TAG_ACTION_PROFILEm_UT_OTAG_ACTIONf_SET(vlan_action, 1);
    ING_VLAN_TAG_ACTION_PROFILEm_SIT_PITAG_ACTIONf_SET(vlan_action, 3);
    ING_VLAN_TAG_ACTION_PROFILEm_SIT_OTAG_ACTIONf_SET(vlan_action, 1);
    ING_VLAN_TAG_ACTION_PROFILEm_SOT_POTAG_ACTIONf_SET(vlan_action, 2);
    ING_VLAN_TAG_ACTION_PROFILEm_DT_POTAG_ACTIONf_SET(vlan_action, 2);
    ioerr += WRITE_ING_VLAN_TAG_ACTION_PROFILEm(unit, 0, vlan_action);

    /* Create special egress action profile for HiGig ports */
    EGR_VLAN_TAG_ACTION_PROFILEm_CLR(egr_action);
    EGR_VLAN_TAG_ACTION_PROFILEm_SOT_OTAG_ACTIONf_SET(egr_action, 3);
    EGR_VLAN_TAG_ACTION_PROFILEm_DT_OTAG_ACTIONf_SET(egr_action, 3);
    ioerr += WRITE_EGR_VLAN_TAG_ACTION_PROFILEm(unit, 1, egr_action);

    /* Fix-up packet purge filtering */
    GPORT_RSV_MASKr_SET(gport_rsv_mask, 0x20058);
    ioerr += WRITE_GPORT_RSV_MASKr(unit, gport_rsv_mask, -1);

    /* Enable GPORTs and clear counters */
    ioerr += READ_GPORT_CONFIGr(unit, &gport_cfg, -1);
    GPORT_CONFIGr_CLR_CNTf_SET(gport_cfg, 1);
    ioerr += WRITE_GPORT_CONFIGr(unit, gport_cfg, -1);
    GPORT_CONFIGr_GPORT_ENf_SET(gport_cfg, 1);
    ioerr += WRITE_GPORT_CONFIGr(unit, gport_cfg, -1);
    GPORT_CONFIGr_CLR_CNTf_SET(gport_cfg, 0);
    ioerr += WRITE_GPORT_CONFIGr(unit, gport_cfg, -1);

    /* Configure GPORTs */
    bcm53540_a0_gport_pbmp_get(unit, &gpbmp);
    CDK_PBMP_ITER(gpbmp, port) {
        if (CDK_SUCCESS(rv)) {
            rv += _gport_init(unit, port);
        }
    }

    CDK_PBMP_ITER(gpbmp, port) {
#if BMD_CONFIG_INCLUDE_PHY == 1
        if (CDK_SUCCESS(rv)) {
            rv = bmd_phy_probe(unit, port);
        }
        lanes = 1;
        if (lanes == 4) {
            rv = bmd_phy_mode_set(unit, port, "viper",
                                        BMD_PHY_MODE_SERDES, 0);
        } else if (lanes == 2) {
            rv = bmd_phy_mode_set(unit, port, "viper",
                                        BMD_PHY_MODE_SERDES, 1);
            rv = bmd_phy_mode_set(unit, port, "viper",
                                        BMD_PHY_MODE_2LANE, 1);
        } else { /* lanes = 1 */
            rv = bmd_phy_mode_set(unit, port, "viper",
                                        BMD_PHY_MODE_SERDES, 1);
            rv = bmd_phy_mode_set(unit, port, "viper",
                                        BMD_PHY_MODE_2LANE, 0);
        }

        if (CDK_SUCCESS(rv)) {
            rv = bmd_phy_init(unit, port);
        }
#endif /* BMD_CONFIG_INCLUDE_PHY == 1 */
    }


#if BMD_CONFIG_INCLUDE_DMA
    /* Common port initialization for CPU port */
    ioerr += _port_init(unit, CMIC_PORT);
    if (CDK_SUCCESS(rv)) {
        rv = bmd_xgsd_dma_init(unit);
    }

    /* Additional configuration required when on PCI bus */
    if (CDK_DEV_FLAGS(unit) & CDK_DEV_MBUS_PCI) {
        CMIC_CMC_HOSTMEM_ADDR_REMAPr_t hostmem_remap;
        uint32_t remap_val[] = { 0x144d2450, 0x19617595, 0x1e75c6da, 0x1f };

        /* Send DMA data to external host memory when on PCI bus */
        for (idx = 0; idx < COUNTOF(remap_val); idx++) {
            CMIC_CMC_HOSTMEM_ADDR_REMAPr_SET(hostmem_remap, remap_val[idx]);
            ioerr += WRITE_CMIC_CMC_HOSTMEM_ADDR_REMAPr(unit, idx, hostmem_remap);
        }
    }
#endif

    return ioerr ? CDK_E_IO : rv;
}
#endif /* CDK_CONFIG_INCLUDE_BCM53540_A0 */
