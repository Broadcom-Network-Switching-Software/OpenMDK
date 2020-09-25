/*
 *
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53570_A0 == 1

#include <bmd/bmd.h>
#include <bmd/bmd_device.h>
#include <cdk/arch/xgsd_chip.h>
#include <cdk/cdk_debug.h>
#include <cdk/cdk_util.h>
#include <cdk/chip/bcm53570_a0_defs.h>
#include <bmd/bmd_phy_ctrl.h>
#include <bmdi/arch/xgsd_dma.h>
#include "bcm53570_a0_bmd.h"
#include "bcm53570_a0_internal.h"

#define PIPE_RESET_TIMEOUT_MSEC         500
#define LLS_RESET_TIMEOUT_MSEC          50

#define NUM_RQEQ_COS                    12
#define NUM_RQEI_COS                    12
#define NUM_RQEI_COS                    12
#define NUM_RDEQ_COS                    16
#define NUM_RDEI_COS                    8

#define FW_ALIGN_BYTES                  16
#define FW_ALIGN_MASK                   (FW_ALIGN_BYTES - 1)

#define JUMBO_MAXSZ                     0x3fe8

/* Transmit CRC Modes */
#define XLMAC_CRC_APPEND        0x0
#define XLMAC_CRC_KEEP          0x1
#define XLMAC_CRC_REPLACE       0x2
#define XLMAC_CRC_PER_PKT_MODE  0x3

#define FD_XE_IPG   96
#define FD_HG_IPG   64
#define FD_HG2_IPG  96

#define XLMAC_RUNT_THRESHOLD_IEEE       0x40

static int
_tdm_init(int unit)
{
    int ioerr = 0;
    const int *default_tdm_seq;
    int tdm_seq[320];
    int idx, tdm_size;
    IARB_TDM_TABLEm_t iarb_tdm_table;
    IARB_TDM_CONTROLr_t iarb_tdm_ctrl;
    MMU_ARB_TDM_TABLEm_t mmu_arb_tdm_table;
    int val;

    /* Get default TDM sequence for this configuration */
    tdm_size = bcm53570_a0_tdm_default(unit, &default_tdm_seq);
    if (tdm_size <= 0 || tdm_size > COUNTOF(tdm_seq)) {
        return CDK_E_INTERNAL;
    }

    /* Make local copy of TDM sequence */
    for (idx = 0; idx < tdm_size; idx++) {
        tdm_seq[idx] = default_tdm_seq[idx];
    }

    /* Disable IARB TDM before programming */
    ioerr += READ_IARB_TDM_CONTROLr(unit, &iarb_tdm_ctrl);
    IARB_TDM_CONTROLr_DISABLEf_SET(iarb_tdm_ctrl, 1);
    IARB_TDM_CONTROLr_TDM_WRAP_PTRf_SET(iarb_tdm_ctrl, tdm_size -1);
    ioerr += WRITE_IARB_TDM_CONTROLr(unit, iarb_tdm_ctrl);

    for (idx = 0; idx < tdm_size; idx++) {
        IARB_TDM_TABLEm_CLR(iarb_tdm_table);
        IARB_TDM_TABLEm_PORT_NUMf_SET(iarb_tdm_table, tdm_seq[idx]);
        ioerr += WRITE_IARB_TDM_TABLEm(unit, idx, iarb_tdm_table);

        val = (tdm_seq[idx] != 127) ? P2M(unit, tdm_seq[idx]) : 127;
        MMU_ARB_TDM_TABLEm_CLR(mmu_arb_tdm_table);
        MMU_ARB_TDM_TABLEm_PORT_NUMf_SET(mmu_arb_tdm_table, val);
        if (idx == (tdm_size - 1)) {
            /* WRAP_EN = 1 */
            MMU_ARB_TDM_TABLEm_WRAP_ENf_SET(mmu_arb_tdm_table, 1);
        }
        ioerr += WRITE_MMU_ARB_TDM_TABLEm(unit, idx, mmu_arb_tdm_table);
    }

    /* DISABLE = 0, TDM_WRAP_PTR = TDM_SIZE-1 */
    IARB_TDM_CONTROLr_DISABLEf_SET(iarb_tdm_ctrl, 0);
    IARB_TDM_CONTROLr_TDM_WRAP_PTRf_SET(iarb_tdm_ctrl, tdm_size - 1);
    ioerr += WRITE_IARB_TDM_CONTROLr(unit, iarb_tdm_ctrl);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

static int
ceiling_func(uint32_t numerators, uint32_t denominator)
{
    uint32_t result;
    if (denominator == 0) {
        return 0xFFFFFFFF;
    }
    result = numerators / denominator;
    if (numerators % denominator != 0) {
        result++;
    }
    return result;
}

static int
_mmu_init_helper_lossy(int unit)
{
    int lport, pport, mport;
    int idx;
    cdk_pbmp_t pbmp_cpu;
    cdk_pbmp_t pbmp_uplink;
    cdk_pbmp_t pbmp_downlink_1g;
    cdk_pbmp_t pbmp_downlink_2dot5g;
    cdk_pbmp_t pbmp_all, xlport_pbmp, clport_pbmp, gport_pbmp;
    int standard_jumbo_frame;
    int cell_size;
    int ethernet_mtu_cell;
    int standard_jumbo_frame_cell;
    int total_physical_memory;
    int ttl_c_mem_adm;
    int number_of_used_memory_banks;
    int reserved_for_cfap;
    int skidmarker;
    int prefetch;
    int cfapfullsetpoint;
    int number_of_uplink_ports;
    int number_of_downlink_ports;
    int queue_port_limit_ratio;
    int egress_queue_min_reserve_uplink_ports_lossy;
    int egress_queue_min_reserve_downlink_ports_lossy;
    int egrqmin_rcp;
    int egrxqmin_rlsyp;
    int num_lossy_queues;
    int num_cpu_queues;
    int num_cpu_ports;
    int numxqs_per_uplink_ports;
    int numxqs_per_downlink_ports_and_cpu_port;
    int total_reserved_cells_for_uplink_ports;
    int total_reserved_cells_for_downlink_ports;
    int total_reserved_cells_for_cpu_port;
    int total_reserved;
    int shared_space_cells;
    int reserved_xqs_per_uplink_port;
    int shared_xqs_per_uplink_port;
    int reserved_xqs_per_downlink_port;
    int shared_xqs_per_downlink_port;
    int lwmcoscellsetlimit0_cellsetlimit_up;
    int lwmcoscellsetlimit7_cellsetlimit_up;
    int lwmcoscellsetlimit_qlayer0_cellsetlimit_up;
    int lwmcoscellsetlimit_qlayer7_cellsetlimit_up;
    int holcoscellmaxlimit_qlayer0_cellmaxlimit_up;
    int holcoscellmaxlimit_qlayer7_cellmaxlimit_up;
    int holcoscellmaxlimit_qlayer8_cellmaxlimit_up;
    int dyncelllimit_dyncellsetlimit_up;
    int holcospktsetlimit_qgroup0_pktsetlimit_up;
    int holcoscellmaxlimit_qgroup0_cellmaxlimit_up;
    int holcospktsetlimit_qlayer0_pktsetlimit_down_1;
    int holcospktsetlimit_qlayer7_pktsetlimit_down_1;
    int holcospktsetlimit_qlayer8_pktsetlimit_down_1;
    int dynxqcntport_dynxqcntport_down_1;
    int holcoscellmaxlimit_qlayer0_cellmaxlimit_down_1;
    int holcoscellmaxlimit_qlayer7_cellmaxlimit_down_1;
    int holcoscellmaxlimit_qlayer8_cellmaxlimit_down_1;
    int dyncelllimit_dyncellsetlimit_down_1;
    int holcospktsetlimit_qgroup0_pktsetlimit_down_1;
    int holcoscellmaxlimit_qgroup0_cellmaxlimit_down_1;
    int holcospktsetlimit_qlayer0_pktsetlimit_down_2dot5;
    int holcospktsetlimit_qlayer7_pktsetlimit_down_2dot5;
    int holcospktsetlimit_qlayer8_pktsetlimit_down_2dot5;
    int dynxqcntport_dynxqcntport_down_2dot5;
    int holcoscellmaxlimit_qlayer0_cellmaxlimit_down_2dot5;
    int holcoscellmaxlimit_qlayer7_cellmaxlimit_down_2dot5;
    int holcoscellmaxlimit_qlayer8_cellmaxlimit_down_2dot5;
    int dyncelllimit_dyncellsetlimit_down_2dot5;
    int holcospktsetlimit_qgroup0_pktsetlimit_down_2dot5;
    int holcoscellmaxlimit_qgroup0_cellmaxlimit_down_2dot5;
    int holcospktsetlimit0_pktsetlimit_cpu;
    int holcospktsetlimit7_pktsetlimit_cpu;
    int dynxqcntport_dynxqcntport_cpu;
    int holcoscellmaxlimit0_cellmaxlimit_cpu;
    int holcoscellmaxlimit7_cellmaxlimit_cpu;
    CFAPFULLTHRESHOLDr_t cfapfthr;
    GBLLIMITSETLIMITr_t glimslim;
    GBLLIMITRESETLIMITr_t glimrlim;
    TOTALDYNCELLSETLIMITr_t totaldyncellsetlimit;
    TOTALDYNCELLRESETLIMITr_t tdyncellrlim;
    TWO_LAYER_SCH_MODEr_t two_layer_sch_mode;
    MISCCONFIGr_t miscconfig;
    MMUPORTTXENABLEr_t mmuporttxenable;
    PG_CTRL0r_t pg_ctrl0;
    PG_CTRL1r_t pg_ctrl1;
    PG2TCr_t pg2tc;
    IBPPKTSETLIMITr_t ibppktsetlimit;
    MMU_FC_RX_ENr_t mmu_fc_rx_en;
    MMU_FC_TX_ENr_t mmu_fc_tx_en;
    PGCELLLIMITr_t pgcelllimit;
    PGDISCARDSETLIMITr_t pgdiscardsetlimit;
    HOLCOSMINXQCNTr_t holcosminxqcnt;
    HOLCOSMINXQCNT_QLAYERr_t hcminxqc_qlr;
    HOLCOSPKTSETLIMITr_t holcospktsetlimit;
    HOLCOSPKTSETLIMIT_QLAYERr_t hcpktslmt_qlr;
    HOLCOSPKTRESETLIMITr_t holcospktresetlimit;
    HOLCOSPKTRESETLIMIT_QLAYERr_t hcpktrslmt_qlr;
    CNGCOSPKTLIMIT0r_t cngcospktlimit0;
    CNGCOSPKTLIMIT1r_t cngcospktlimit1;
    CNGCOSPKTLIMIT0_QLAYERr_t ccpktlmt0_qlr;
    CNGCOSPKTLIMIT1_QLAYERr_t ccpktlmt1_qlr;
    CNGPORTPKTLIMIT0r_t cngportpktlimit0;
    CNGPORTPKTLIMIT1r_t cngportpktlimit1;
    DYNXQCNTPORTr_t dynxqcntport;
    DYNRESETLIMPORTr_t dynresetlimport;
    LWMCOSCELLSETLIMITr_t lwmcoscellsetlimit;
    LWMCOSCELLSETLIMIT_QLAYERr_t lccellslmt_qlr;
    HOLCOSCELLMAXLIMITr_t holcoscellmaxlimit;
    HOLCOSCELLMAXLIMIT_QLAYERr_t hccellmaxlmt_qlr;
    DYNCELLLIMITr_t dyncelllimit;
    COLOR_DROP_ENr_t color_drop_en;
    COLOR_DROP_EN_QLAYERr_t color_drop_en_qlayer;
    HOLCOSPKTSETLIMIT_QGROUPr_t hcpktslmt_qgp;
    HOLCOSPKTRESETLIMIT_QGROUPr_t hcpktrslmt_qgp;
    CNGCOSPKTLIMIT0_QGROUPr_t ccpktlmt0_qgp;
    CNGCOSPKTLIMIT1_QGROUPr_t ccpktlmt1_qgp;
    HOLCOSCELLMAXLIMIT_QGROUPr_t hccellmaxlmt_qgp;
    COLOR_DROP_EN_QGROUPr_t color_drop_en_qgroup;
    SHARED_POOL_CTRLr_t shared_pool_ctrl;
    SHARED_POOL_CTRL_EXT1r_t shared_pool_ctrl_ext1;
    SHARED_POOL_CTRL_EXT2r_t shared_pool_ctrl_ext2;
    uint32_t fval, fval1;
    
    bcm53570_a0_all_pbmp_get(unit, &pbmp_all);
    CDK_PBMP_ADD(pbmp_all, CMIC_PORT);
    bcm53570_a0_xlport_pbmp_get(unit, &xlport_pbmp);
    bcm53570_a0_clport_pbmp_get(unit, &clport_pbmp);
    bcm53570_a0_gport_pbmp_get(unit, &gport_pbmp);
    /* setup port bitmap according the port max speed for lossy
     *   TSC/TSCF    : uplink port
     *   QGMII/SGMII : downlink port
     */
    num_cpu_ports = 0;
    number_of_uplink_ports = 0;
    number_of_downlink_ports = 0;
    CDK_PBMP_CLEAR(pbmp_cpu);
    CDK_PBMP_CLEAR(pbmp_uplink);
    CDK_PBMP_CLEAR(pbmp_downlink_1g);
    CDK_PBMP_CLEAR(pbmp_downlink_2dot5g);
    for (pport = 0; pport < MAX_PHY_PORTS; pport++) {
        lport = P2L(unit, pport);
        if ((lport == -1) || (L2P(unit, lport) == -1)) {
            continue;
        }
        if (pport == CMIC_PORT) {
            num_cpu_ports++;
            CDK_PBMP_ADD(pbmp_cpu, pport);
        } else if (CDK_PBMP_MEMBER(xlport_pbmp, pport)
                || CDK_PBMP_MEMBER(clport_pbmp, pport)) {
            number_of_uplink_ports++;
            CDK_PBMP_ADD(pbmp_uplink, pport);
        } else if (CDK_PBMP_MEMBER(gport_pbmp, pport)) {
            number_of_downlink_ports++;
            if (SPEED_MAX(unit, pport) > 1000) {
                CDK_PBMP_ADD(pbmp_downlink_2dot5g, pport);
            } else {
                CDK_PBMP_ADD(pbmp_downlink_1g, pport);
            }
        }
    }

    standard_jumbo_frame = 9216;
    cell_size = 144;
    ethernet_mtu_cell = ceiling_func(15 * 1024 / 10, cell_size);
    standard_jumbo_frame_cell = ceiling_func(standard_jumbo_frame, cell_size);
    total_physical_memory = 24 * 1024;
    ttl_c_mem_adm = 225 * 1024 / 10;
    number_of_used_memory_banks = 8;
    reserved_for_cfap = (65) * 2 + number_of_used_memory_banks * 4;
    skidmarker = 7;
    prefetch = 64 + 4;
    cfapfullsetpoint = total_physical_memory - reserved_for_cfap;
    queue_port_limit_ratio = 8;
    egress_queue_min_reserve_uplink_ports_lossy = ethernet_mtu_cell;
    egress_queue_min_reserve_downlink_ports_lossy = ethernet_mtu_cell;
    egrqmin_rcp = ethernet_mtu_cell;
    egrxqmin_rlsyp = ethernet_mtu_cell;
    num_lossy_queues = 8;
    num_cpu_queues = 8;
    num_cpu_ports = 1;
    numxqs_per_uplink_ports = 6 * 1024;
    numxqs_per_downlink_ports_and_cpu_port = 2 * 1024;
    total_reserved_cells_for_uplink_ports
        = egress_queue_min_reserve_uplink_ports_lossy
          * number_of_uplink_ports * num_lossy_queues;
    total_reserved_cells_for_downlink_ports
        = number_of_downlink_ports
          * egress_queue_min_reserve_downlink_ports_lossy
          * (num_lossy_queues);
    total_reserved_cells_for_cpu_port
        = num_cpu_ports * egrqmin_rcp
          * num_cpu_queues;
    total_reserved
        = total_reserved_cells_for_uplink_ports
          + total_reserved_cells_for_downlink_ports
          + total_reserved_cells_for_cpu_port;
    shared_space_cells = ttl_c_mem_adm - total_reserved;
    reserved_xqs_per_uplink_port
        = egrxqmin_rlsyp
          * num_lossy_queues;
    shared_xqs_per_uplink_port
          = numxqs_per_uplink_ports - reserved_xqs_per_uplink_port;
    reserved_xqs_per_downlink_port
        = egrxqmin_rlsyp
          * num_lossy_queues;
    shared_xqs_per_downlink_port
        = numxqs_per_downlink_ports_and_cpu_port
          - reserved_xqs_per_downlink_port;
    lwmcoscellsetlimit0_cellsetlimit_up
          = egress_queue_min_reserve_uplink_ports_lossy;
    lwmcoscellsetlimit7_cellsetlimit_up
          = egress_queue_min_reserve_uplink_ports_lossy;
    lwmcoscellsetlimit_qlayer0_cellsetlimit_up
          = egress_queue_min_reserve_uplink_ports_lossy;
    lwmcoscellsetlimit_qlayer7_cellsetlimit_up
          = egress_queue_min_reserve_uplink_ports_lossy;
    holcoscellmaxlimit_qlayer0_cellmaxlimit_up
        = ceiling_func(shared_space_cells, queue_port_limit_ratio)
          + lwmcoscellsetlimit_qlayer0_cellsetlimit_up;
    holcoscellmaxlimit_qlayer7_cellmaxlimit_up
        = ceiling_func(shared_space_cells, queue_port_limit_ratio)
          + lwmcoscellsetlimit_qlayer7_cellsetlimit_up;
    holcoscellmaxlimit_qlayer8_cellmaxlimit_up
        = ceiling_func(shared_space_cells, queue_port_limit_ratio);
    dyncelllimit_dyncellsetlimit_up = shared_space_cells;
    holcospktsetlimit_qgroup0_pktsetlimit_up = numxqs_per_uplink_ports - 1;
    holcoscellmaxlimit_qgroup0_cellmaxlimit_up
        = ceiling_func(ttl_c_mem_adm, queue_port_limit_ratio);
    holcospktsetlimit_qlayer0_pktsetlimit_down_1
        = shared_xqs_per_downlink_port
          + egrxqmin_rlsyp;
    holcospktsetlimit_qlayer7_pktsetlimit_down_1
        = shared_xqs_per_downlink_port
          + egrxqmin_rlsyp;
    holcospktsetlimit_qlayer8_pktsetlimit_down_1
        = shared_xqs_per_downlink_port;
    dynxqcntport_dynxqcntport_down_1
          = shared_xqs_per_downlink_port - skidmarker - prefetch;
    holcoscellmaxlimit_qlayer0_cellmaxlimit_down_1
        = ceiling_func(shared_space_cells, queue_port_limit_ratio)
          + lwmcoscellsetlimit_qlayer0_cellsetlimit_up;
    holcoscellmaxlimit_qlayer7_cellmaxlimit_down_1
        = ceiling_func(shared_space_cells, queue_port_limit_ratio)
          + lwmcoscellsetlimit_qlayer7_cellsetlimit_up;
    holcoscellmaxlimit_qlayer8_cellmaxlimit_down_1
        = ceiling_func(shared_space_cells, queue_port_limit_ratio);
    dyncelllimit_dyncellsetlimit_down_1 = shared_space_cells;
    holcospktsetlimit_qgroup0_pktsetlimit_down_1
          = numxqs_per_downlink_ports_and_cpu_port - 1;
    holcoscellmaxlimit_qgroup0_cellmaxlimit_down_1
        = ceiling_func(ttl_c_mem_adm, queue_port_limit_ratio);
    holcospktsetlimit_qlayer0_pktsetlimit_down_2dot5
        = shared_xqs_per_downlink_port
          + egrxqmin_rlsyp;
    holcospktsetlimit_qlayer7_pktsetlimit_down_2dot5
        = shared_xqs_per_downlink_port
          + egrxqmin_rlsyp;
    holcospktsetlimit_qlayer8_pktsetlimit_down_2dot5
        = shared_xqs_per_downlink_port;
    dynxqcntport_dynxqcntport_down_2dot5
          = shared_xqs_per_downlink_port - skidmarker - prefetch;
    holcoscellmaxlimit_qlayer0_cellmaxlimit_down_2dot5
        = ceiling_func(shared_space_cells, queue_port_limit_ratio)
          + lwmcoscellsetlimit_qlayer0_cellsetlimit_up;
    holcoscellmaxlimit_qlayer7_cellmaxlimit_down_2dot5
        = ceiling_func(shared_space_cells, queue_port_limit_ratio)
          + lwmcoscellsetlimit_qlayer7_cellsetlimit_up;
    holcoscellmaxlimit_qlayer8_cellmaxlimit_down_2dot5
        = ceiling_func(shared_space_cells, queue_port_limit_ratio);
    dyncelllimit_dyncellsetlimit_down_2dot5 = shared_space_cells;
    holcospktsetlimit_qgroup0_pktsetlimit_down_2dot5
          = numxqs_per_downlink_ports_and_cpu_port - 1;
    holcoscellmaxlimit_qgroup0_cellmaxlimit_down_2dot5
        = ceiling_func(ttl_c_mem_adm, queue_port_limit_ratio);
    holcospktsetlimit0_pktsetlimit_cpu =
              shared_xqs_per_downlink_port + egrqmin_rcp;
    holcospktsetlimit7_pktsetlimit_cpu =
              shared_xqs_per_downlink_port + egrqmin_rcp;
    dynxqcntport_dynxqcntport_cpu =
              shared_xqs_per_downlink_port - skidmarker - prefetch;
    holcoscellmaxlimit0_cellmaxlimit_cpu =
              ceiling_func(shared_space_cells, queue_port_limit_ratio) +
              lwmcoscellsetlimit0_cellsetlimit_up;
    holcoscellmaxlimit7_cellmaxlimit_cpu =
              ceiling_func(shared_space_cells, queue_port_limit_ratio) +
              lwmcoscellsetlimit7_cellsetlimit_up;

    if ((shared_space_cells * cell_size)/1024 <= 800) {
        CDK_PRINTF("ERROR : Shared Pool Is Small, should be larger than 800 (value=%d)\n",
                 (shared_space_cells * cell_size)/1024);
        return CDK_E_PARAM;
    }

    /* system-based */
    CFAPFULLTHRESHOLDr_CLR(cfapfthr);
    fval = cfapfullsetpoint;
    CFAPFULLTHRESHOLDr_CFAPFULLSETPOINTf_SET(cfapfthr, fval);
    fval -= (standard_jumbo_frame_cell * 2);
    CFAPFULLTHRESHOLDr_CFAPFULLRESETPOINTf_SET(cfapfthr, fval);
    WRITE_CFAPFULLTHRESHOLDr(unit, cfapfthr);
    
    GBLLIMITSETLIMITr_SET(glimslim, ttl_c_mem_adm);
    WRITE_GBLLIMITSETLIMITr(unit, glimslim);

    GBLLIMITRESETLIMITr_SET(glimrlim, ttl_c_mem_adm);
    WRITE_GBLLIMITRESETLIMITr(unit, glimrlim);

    TOTALDYNCELLSETLIMITr_SET(totaldyncellsetlimit, shared_space_cells);
    WRITE_TOTALDYNCELLSETLIMITr(unit, totaldyncellsetlimit);

    fval = shared_space_cells - (standard_jumbo_frame_cell * 2);
    TOTALDYNCELLRESETLIMITr_SET(tdyncellrlim, fval);
    WRITE_TOTALDYNCELLRESETLIMITr(unit, tdyncellrlim);

    CDK_PBMP_ITER(pbmp_all, pport) {
        mport = P2M(unit, pport);
        READ_TWO_LAYER_SCH_MODEr(unit, mport, &two_layer_sch_mode);
        TWO_LAYER_SCH_MODEr_SCH_MODEf_SET(two_layer_sch_mode, 0);
        WRITE_TWO_LAYER_SCH_MODEr(unit, mport, two_layer_sch_mode);
    }

    READ_MISCCONFIGr(unit, &miscconfig);
    MISCCONFIGr_MULTIPLE_ACCOUNTING_FIX_ENf_SET(miscconfig, 1);
    MISCCONFIGr_CNG_DROP_ENf_SET(miscconfig, 0);
    MISCCONFIGr_DYN_XQ_ENf_SET(miscconfig, 1);
    MISCCONFIGr_HOL_CELL_SOP_DROP_ENf_SET(miscconfig, 1);
    MISCCONFIGr_DYNAMIC_MEMORY_ENf_SET(miscconfig, 1);
    MISCCONFIGr_SKIDMARKERf_SET(miscconfig, 3); /* 3 for skidmarker=7 */
    WRITE_MISCCONFIGr(unit, miscconfig);

    MMUPORTTXENABLEr_SET(mmuporttxenable, 0xFFFFFFFF);
    WRITE_MMUPORTTXENABLEr(unit, 0, mmuporttxenable);

    MMUPORTTXENABLEr_SET(mmuporttxenable, 0xFFFFFFFF);
    WRITE_MMUPORTTXENABLEr(unit, 1, mmuporttxenable);

    MMUPORTTXENABLEr_SET(mmuporttxenable, 3);
    WRITE_MMUPORTTXENABLEr(unit, 2, mmuporttxenable);

    /* port-based : uplink */
    CDK_PBMP_ITER(pbmp_uplink, pport) {
        mport = P2M(unit, pport);

        /* PG_CTRL0r, index 0 */
        READ_PG_CTRL0r(unit, mport, &pg_ctrl0);
        PG_CTRL0r_PPFC_PG_ENf_SET(pg_ctrl0, 0);
        PG_CTRL0r_PRI0_GRPf_SET(pg_ctrl0, 0);
        PG_CTRL0r_PRI1_GRPf_SET(pg_ctrl0, 1);
        PG_CTRL0r_PRI2_GRPf_SET(pg_ctrl0, 2);
        PG_CTRL0r_PRI3_GRPf_SET(pg_ctrl0, 3);
        PG_CTRL0r_PRI4_GRPf_SET(pg_ctrl0, 4);
        PG_CTRL0r_PRI5_GRPf_SET(pg_ctrl0, 5);
        PG_CTRL0r_PRI6_GRPf_SET(pg_ctrl0, 6);
        PG_CTRL0r_PRI7_GRPf_SET(pg_ctrl0, 7);
        WRITE_PG_CTRL0r(unit, mport, pg_ctrl0);

        /* PG_CTRL1r, index 0 */
        READ_PG_CTRL1r(unit, mport, &pg_ctrl1);
        PG_CTRL1r_PRI8_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI9_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI10_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI11_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI12_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI13_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI14_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI15_GRPf_SET(pg_ctrl1, 7);
        WRITE_PG_CTRL1r(unit, mport, pg_ctrl1);

        /* PG2TCr, index 0 ~ 7 */
        for (idx = 0; idx <= 7; idx++) {
            PG2TCr_SET(pg2tc, 0);
            WRITE_PG2TCr(unit, mport, idx, pg2tc);
        }

        /* IBPPKTSETLIMITr, index 0 */
        IBPPKTSETLIMITr_CLR(ibppktsetlimit);
        IBPPKTSETLIMITr_PKTSETLIMITf_SET(ibppktsetlimit, ttl_c_mem_adm);
        IBPPKTSETLIMITr_RESETLIMITSELf_SET(ibppktsetlimit, 3);
        WRITE_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);

        /* MMU_FC_RX_ENr, index 0 */
        MMU_FC_RX_ENr_SET(mmu_fc_rx_en, 0);
        WRITE_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);

        /* MMU_FC_TX_ENr, index 0 */
        MMU_FC_TX_ENr_SET(mmu_fc_tx_en, 0);
        WRITE_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);

        /* PGCELLLIMITr, index 0 ~ 7 */
        for (idx = 0; idx <= 7; idx++) {
            PGCELLLIMITr_CLR(pgcelllimit);
            PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit, ttl_c_mem_adm);
            PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit, ttl_c_mem_adm);
            WRITE_PGCELLLIMITr(unit, mport, idx, pgcelllimit);
        }

        /* PGDISCARDSETLIMITr, index 0 ~ 7 */
        for (idx = 0; idx <= 7; idx++) {
            PGDISCARDSETLIMITr_SET(pgdiscardsetlimit, ttl_c_mem_adm);
            WRITE_PGDISCARDSETLIMITr(unit, mport, idx, pgdiscardsetlimit);
        }

        /* HOLCOSMINXQCNT_QLAYERr, index 0 ~ 7 */
        for (idx = 0; idx <= 7; idx++) {
            HOLCOSMINXQCNT_QLAYERr_SET(hcminxqc_qlr, egrxqmin_rlsyp);
            WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mport, idx, hcminxqc_qlr);
        }

        /* HOLCOSMINXQCNT_QLAYERr, index 8 ~ 63 */
        for (idx = 8; idx <= 63; idx++) {
            HOLCOSMINXQCNT_QLAYERr_SET(hcminxqc_qlr, 0);
            WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mport, idx, hcminxqc_qlr);
        }

        /* HOLCOSPKTSETLIMIT_QLAYERr, index 0 ~ 7 */
        fval = shared_xqs_per_uplink_port + egrxqmin_rlsyp;
        for (idx = 0; idx <= 7; idx++) {
            HOLCOSPKTSETLIMIT_QLAYERr_SET(hcpktslmt_qlr, fval);
            WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mport, idx, hcpktslmt_qlr);
        }

        /* HOLCOSPKTSETLIMIT_QLAYERr, index 8 ~ 63 */
        fval = shared_xqs_per_uplink_port;
        for (idx = 8; idx <= 63; idx++) {
            HOLCOSPKTSETLIMIT_QLAYERr_SET(hcpktslmt_qlr, fval);
            WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mport, idx, hcpktslmt_qlr);
        }

        /* HOLCOSPKTRESETLIMIT_QLAYERr, index 0 ~ 6 */
        fval = shared_xqs_per_uplink_port + egrxqmin_rlsyp - 1;
        for (idx = 0; idx <= 6; idx++) {
            HOLCOSPKTRESETLIMIT_QLAYERr_SET(hcpktrslmt_qlr, fval);
            WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, idx, hcpktrslmt_qlr);
        }

        /* HOLCOSPKTRESETLIMIT_QLAYERr, index 7 */
        fval = shared_xqs_per_uplink_port + egrxqmin_rlsyp - 1;
        HOLCOSPKTRESETLIMIT_QLAYERr_SET(hcpktrslmt_qlr, fval);
        WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, 7, hcpktrslmt_qlr);

        /* HOLCOSPKTRESETLIMIT_QLAYERr, index 8 ~ 63 */
        fval = shared_xqs_per_uplink_port - 1;
        for (idx = 8; idx <= 63; idx++) {
            HOLCOSPKTRESETLIMIT_QLAYERr_SET(hcpktrslmt_qlr, fval);
            WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, idx, hcpktrslmt_qlr);
        }

        /* CNGCOSPKTLIMIT0_QLAYERr, index 0 ~ 63 */
        fval = numxqs_per_uplink_ports - 1;
        for (idx = 0; idx <= 63; idx++) {
            CNGCOSPKTLIMIT0_QLAYERr_SET(ccpktlmt0_qlr, fval);
            WRITE_CNGCOSPKTLIMIT0_QLAYERr(unit, mport, idx, ccpktlmt0_qlr);
        }

        /* CNGCOSPKTLIMIT1_QLAYERr, index 0 ~ 63 */
        for (idx = 0; idx <= 63; idx++) {
            CNGCOSPKTLIMIT1_QLAYERr_SET(ccpktlmt1_qlr, fval);
            WRITE_CNGCOSPKTLIMIT1_QLAYERr(unit, mport, idx, ccpktlmt1_qlr);
        }

        /* CNGPORTPKTLIMIT0r, index 0 */
        CNGPORTPKTLIMIT0r_SET(cngportpktlimit0, fval);
        WRITE_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);

        /* CNGPORTPKTLIMIT1r, index 0 */
        CNGPORTPKTLIMIT1r_SET(cngportpktlimit1, fval);
        WRITE_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);

        /* DYNXQCNTPORTr, index 0 */
        fval = shared_xqs_per_uplink_port - skidmarker - prefetch;
        DYNXQCNTPORTr_SET(dynxqcntport, fval);
        WRITE_DYNXQCNTPORTr(unit, mport, dynxqcntport);

        /* DYNRESETLIMPORTr, index 0 */
        fval = fval - 2;
        DYNRESETLIMPORTr_SET(dynresetlimport, fval);
        WRITE_DYNRESETLIMPORTr(unit, mport, dynresetlimport);

        /* LWMCOSCELLSETLIMIT_QLAYERr, index 0 ~ 7 */
        fval = egress_queue_min_reserve_uplink_ports_lossy;
        for (idx = 0; idx <= 7; idx++) {
            LWMCOSCELLSETLIMIT_QLAYERr_CLR(lccellslmt_qlr);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(lccellslmt_qlr, fval);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(lccellslmt_qlr, fval);
            WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, idx, lccellslmt_qlr);
        }

        /* LWMCOSCELLSETLIMIT_QLAYERr, index 8 ~ 63 */
        for (idx = 8; idx <= 63; idx++) {
            LWMCOSCELLSETLIMIT_QLAYERr_CLR(lccellslmt_qlr);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(lccellslmt_qlr, 0);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(lccellslmt_qlr, 0);
            WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, idx, lccellslmt_qlr);
        }

        /* HOLCOSCELLMAXLIMIT_QLAYERr, index 0 ~ 6 */
        fval = ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                            lwmcoscellsetlimit_qlayer0_cellsetlimit_up;
        fval1 = holcoscellmaxlimit_qlayer0_cellmaxlimit_up - ethernet_mtu_cell;
        for (idx = 0; idx <= 6; idx++) {
            HOLCOSCELLMAXLIMIT_QLAYERr_CLR(hccellmaxlmt_qlr);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(hccellmaxlmt_qlr, fval);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(hccellmaxlmt_qlr, fval1);
            WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, idx, hccellmaxlmt_qlr);
        }

        /* HOLCOSCELLMAXLIMIT_QLAYERr, index 7 */
        HOLCOSCELLMAXLIMIT_QLAYERr_CLR(hccellmaxlmt_qlr);
        fval = ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                            lwmcoscellsetlimit_qlayer7_cellsetlimit_up;
        HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(hccellmaxlmt_qlr, fval);
        fval1 = holcoscellmaxlimit_qlayer7_cellmaxlimit_up - ethernet_mtu_cell;
        HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(hccellmaxlmt_qlr, fval1);
        WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, 7, hccellmaxlmt_qlr);

        /* HOLCOSCELLMAXLIMIT_QLAYERr, index 8 ~ 63 */
        fval = ceiling_func(shared_space_cells, queue_port_limit_ratio);
        fval1 = holcoscellmaxlimit_qlayer8_cellmaxlimit_up - ethernet_mtu_cell;
        for (idx = 8; idx <= 63; idx++) {
            HOLCOSCELLMAXLIMIT_QLAYERr_CLR(hccellmaxlmt_qlr);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(hccellmaxlmt_qlr, fval);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(hccellmaxlmt_qlr, fval1);
            WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, idx, hccellmaxlmt_qlr);
        }

        /* DYNCELLLIMITr, index 0 */
        fval = dyncelllimit_dyncellsetlimit_up - (2 * ethernet_mtu_cell);
        DYNCELLLIMITr_CLR(dyncelllimit);
        DYNCELLLIMITr_DYNCELLSETLIMITf_SET(dyncelllimit, shared_space_cells);
        DYNCELLLIMITr_DYNCELLRESETLIMITf_SET(dyncelllimit, fval);
        WRITE_DYNCELLLIMITr(unit, mport, dyncelllimit);

        /* COLOR_DROP_EN_QLAYERr, index 0 */
        COLOR_DROP_EN_QLAYERr_SET(color_drop_en_qlayer, 0);
        WRITE_COLOR_DROP_EN_QLAYERr(unit, mport, 0, color_drop_en_qlayer);

        /* HOLCOSPKTSETLIMIT_QGROUPr, index 0 ~ 7 */
        for (idx = 0; idx <= 7; idx++) {
            HOLCOSPKTSETLIMIT_QGROUPr_SET(hcpktslmt_qgp, numxqs_per_uplink_ports - 1);
            WRITE_HOLCOSPKTSETLIMIT_QGROUPr(unit, mport, idx, hcpktslmt_qgp);
        }

        /* HOLCOSPKTRESETLIMIT_QGROUPr, index 0 ~ 7 */
        fval = holcospktsetlimit_qgroup0_pktsetlimit_up - 1;
        for (idx = 0; idx <= 7; idx++) {
            HOLCOSPKTRESETLIMIT_QGROUPr_SET(hcpktrslmt_qgp, fval);
            WRITE_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mport, idx, hcpktrslmt_qgp);
        }

        /* CNGCOSPKTLIMIT0_QGROUPr, index 0 ~ 7 */
        for (idx = 0; idx <= 7; idx++) {
            CNGCOSPKTLIMIT0_QGROUPr_SET(ccpktlmt0_qgp, numxqs_per_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT0_QGROUPr(unit, mport, idx, ccpktlmt0_qgp);
        }

        /* CNGCOSPKTLIMIT1_QGROUPr, index 0 ~ 7 */
        for (idx = 0; idx <= 7; idx++) {
            CNGCOSPKTLIMIT1_QGROUPr_SET(ccpktlmt1_qgp, numxqs_per_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT1_QGROUPr(unit, mport, idx, ccpktlmt1_qgp);
        }

        /* HOLCOSCELLMAXLIMIT_QGROUPr, index 0 ~ 7 */
        fval = ceiling_func(ttl_c_mem_adm, queue_port_limit_ratio);
        fval1 = holcoscellmaxlimit_qgroup0_cellmaxlimit_up - ethernet_mtu_cell;
        for (idx = 0; idx <= 7; idx++) {
            HOLCOSCELLMAXLIMIT_QGROUPr_CLR(hccellmaxlmt_qgp);
            HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXLIMITf_SET(hccellmaxlmt_qgp, fval);
            HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXRESUMELIMITf_SET(hccellmaxlmt_qgp, fval1);
            WRITE_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mport, idx, hccellmaxlmt_qgp);
        }

        /* COLOR_DROP_EN_QGROUPr, index 0 */
        COLOR_DROP_EN_QGROUPr_SET(color_drop_en_qgroup, 0);
        WRITE_COLOR_DROP_EN_QGROUPr(unit, mport, color_drop_en_qgroup);

        /* SHARED_POOL_CTRLr, index 0 */
        SHARED_POOL_CTRLr_CLR(shared_pool_ctrl);
        SHARED_POOL_CTRLr_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl, 255);
        SHARED_POOL_CTRLr_SHARED_POOL_DISCARD_ENf_SET(shared_pool_ctrl, 255);
        SHARED_POOL_CTRLr_SHARED_POOL_XOFF_ENf_SET(shared_pool_ctrl, 0);
        WRITE_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);

        /* SHARED_POOL_CTRL_EXT1r, index 0 */
        READ_SHARED_POOL_CTRL_EXT1r(unit, mport, &shared_pool_ctrl_ext1);
        SHARED_POOL_CTRL_EXT1r_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl_ext1,
                            0xFFFFFF);
        WRITE_SHARED_POOL_CTRL_EXT1r(unit, mport, shared_pool_ctrl_ext1);

        /* SHARED_POOL_CTRL_EXT2r, index 0 */
        SHARED_POOL_CTRL_EXT2r_SET(shared_pool_ctrl_ext2, 0xFFFFFFFF);
        WRITE_SHARED_POOL_CTRL_EXT2r(unit, mport, shared_pool_ctrl_ext2);
    }

    /* port-based : downlink 1G */
    CDK_PBMP_ITER(pbmp_downlink_1g, pport) {
        mport = P2M(unit, pport);

        /* PG_CTRL0r, index 0 */
        READ_PG_CTRL0r(unit, mport, &pg_ctrl0);
        PG_CTRL0r_PPFC_PG_ENf_SET(pg_ctrl0, 0);
        PG_CTRL0r_PRI0_GRPf_SET(pg_ctrl0, 0);
        PG_CTRL0r_PRI1_GRPf_SET(pg_ctrl0, 1);
        PG_CTRL0r_PRI2_GRPf_SET(pg_ctrl0, 2);
        PG_CTRL0r_PRI3_GRPf_SET(pg_ctrl0, 3);
        PG_CTRL0r_PRI4_GRPf_SET(pg_ctrl0, 4);
        PG_CTRL0r_PRI5_GRPf_SET(pg_ctrl0, 5);
        PG_CTRL0r_PRI6_GRPf_SET(pg_ctrl0, 6);
        PG_CTRL0r_PRI7_GRPf_SET(pg_ctrl0, 7);
        WRITE_PG_CTRL0r(unit, mport, pg_ctrl0);

        /* PG_CTRL1r, index 0 */
        READ_PG_CTRL1r(unit, mport, &pg_ctrl1);
        PG_CTRL1r_PRI8_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI9_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI10_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI11_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI12_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI13_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI14_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI15_GRPf_SET(pg_ctrl1, 7);
        WRITE_PG_CTRL1r(unit, mport, pg_ctrl1);

        /* PG2TCr, index 0 ~ 7 */
        for (idx = 0; idx <= 7; idx++) {
            PG2TCr_SET(pg2tc, 0);
            WRITE_PG2TCr(unit, mport, idx, pg2tc);
        }

        /* IBPPKTSETLIMITr, index 0 */
        fval = ttl_c_mem_adm;
        IBPPKTSETLIMITr_CLR(ibppktsetlimit);
        IBPPKTSETLIMITr_PKTSETLIMITf_SET(ibppktsetlimit, fval);
        IBPPKTSETLIMITr_RESETLIMITSELf_SET(ibppktsetlimit, 3);
        WRITE_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);

        /* MMU_FC_RX_ENr, index 0 */
        MMU_FC_RX_ENr_SET(mmu_fc_rx_en, 0);
        WRITE_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);

        /* MMU_FC_TX_ENr, index 0 */
        MMU_FC_TX_ENr_SET(mmu_fc_tx_en, 0);
        WRITE_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);

        /* PGCELLLIMITr, index 0 ~ 6 */
        for (idx = 0; idx <= 6; idx++) {
            PGCELLLIMITr_CLR(pgcelllimit);
            PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit, ttl_c_mem_adm);
            PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit, ttl_c_mem_adm);
            WRITE_PGCELLLIMITr(unit, mport, idx, pgcelllimit);
        }

        /* PGCELLLIMITr, index 7 */
        fval = ttl_c_mem_adm;
        PGCELLLIMITr_CLR(pgcelllimit);
        PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit, fval);
        PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit, fval);
        WRITE_PGCELLLIMITr(unit, mport, 7, pgcelllimit);

        /* PGDISCARDSETLIMITr, index 0 ~ 6 */
        for (idx = 0; idx <= 6; idx++) {
            PGDISCARDSETLIMITr_SET(pgdiscardsetlimit, ttl_c_mem_adm);
            WRITE_PGDISCARDSETLIMITr(unit, mport, idx, pgdiscardsetlimit);
        }

        /* PGDISCARDSETLIMITr, index 7 */
        PGDISCARDSETLIMITr_SET(pgdiscardsetlimit, ttl_c_mem_adm);
        WRITE_PGDISCARDSETLIMITr(unit, mport, 7, pgdiscardsetlimit);

        /* HOLCOSMINXQCNT_QLAYERr, index 0 ~ 7 */
        for (idx = 0; idx <= 7; idx++) {
            HOLCOSMINXQCNT_QLAYERr_SET(hcminxqc_qlr, egrxqmin_rlsyp);
            WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mport, idx, hcminxqc_qlr);
        }

        /* HOLCOSMINXQCNT_QLAYERr, index 8 ~ 63 */
        for (idx = 8; idx <= 63; idx++) {
            HOLCOSMINXQCNT_QLAYERr_SET(hcminxqc_qlr, 0);
            WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mport, idx, hcminxqc_qlr);
        }

        /* HOLCOSPKTSETLIMIT_QLAYERr, index 0 ~ 7 */
        fval = shared_xqs_per_downlink_port + egrxqmin_rlsyp;
        for (idx = 0; idx <= 7; idx++) {
            HOLCOSPKTSETLIMIT_QLAYERr_SET(hcpktslmt_qlr, fval);
            WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mport, idx, hcpktslmt_qlr);
        }

        /* HOLCOSPKTSETLIMIT_QLAYERr, index 8 ~ 63 */
        fval = shared_xqs_per_downlink_port;
        for (idx = 8; idx <= 63; idx++) {
            HOLCOSPKTSETLIMIT_QLAYERr_SET(hcpktslmt_qlr, fval);
            WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mport, idx, hcpktslmt_qlr);
        }

        /* HOLCOSPKTRESETLIMIT_QLAYERr, index 0 ~ 6 */
        fval = holcospktsetlimit_qlayer0_pktsetlimit_down_1 - 1;
        for (idx = 0; idx <= 6; idx++) {
            HOLCOSPKTRESETLIMIT_QLAYERr_SET(hcpktrslmt_qlr, fval);
            WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, idx, hcpktrslmt_qlr);
        }

        /* HOLCOSPKTRESETLIMIT_QLAYERr, index 7 */
        fval = holcospktsetlimit_qlayer7_pktsetlimit_down_1 - 1;
        HOLCOSPKTRESETLIMIT_QLAYERr_SET(hcpktrslmt_qlr, fval);
        WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, 7, hcpktrslmt_qlr);

        /* HOLCOSPKTRESETLIMIT_QLAYERr, index 8 ~ 63 */
        fval = holcospktsetlimit_qlayer8_pktsetlimit_down_1 - 1;
        for (idx = 8; idx <= 63; idx++) {
            HOLCOSPKTRESETLIMIT_QLAYERr_SET(hcpktrslmt_qlr, fval);
            WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, idx, hcpktrslmt_qlr);
        }

        /* CNGCOSPKTLIMIT0_QLAYERr, index 0 ~ 63 */
        fval = numxqs_per_downlink_ports_and_cpu_port - 1;
        for (idx = 0; idx <= 63; idx++) {
            CNGCOSPKTLIMIT0_QLAYERr_SET(ccpktlmt0_qlr, fval);
            WRITE_CNGCOSPKTLIMIT0_QLAYERr(unit, mport, idx, ccpktlmt0_qlr);
        }

        /* CNGCOSPKTLIMIT1_QLAYERr, index 0 ~ 63 */
        fval = numxqs_per_downlink_ports_and_cpu_port - 1;
        for (idx = 0; idx <= 63; idx++) {
            CNGCOSPKTLIMIT1_QLAYERr_SET(ccpktlmt1_qlr, fval);
            WRITE_CNGCOSPKTLIMIT1_QLAYERr(unit, mport, idx, ccpktlmt1_qlr);
        }

        /* CNGPORTPKTLIMIT0r, index 0 */
        fval = numxqs_per_downlink_ports_and_cpu_port - 1;
        CNGPORTPKTLIMIT0r_SET(cngportpktlimit0, fval);
        WRITE_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);

        /* CNGPORTPKTLIMIT1r, index 0 */
        fval = numxqs_per_downlink_ports_and_cpu_port - 1;
        CNGPORTPKTLIMIT1r_SET(cngportpktlimit1, fval);
        WRITE_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);

        /* DYNXQCNTPORTr, index 0 */
        fval = shared_xqs_per_downlink_port - skidmarker - prefetch;
        DYNXQCNTPORTr_SET(dynxqcntport, fval);
        WRITE_DYNXQCNTPORTr(unit, mport, dynxqcntport);

        /* DYNRESETLIMPORTr, index 0 */
        fval = dynxqcntport_dynxqcntport_down_1 - 2;
        DYNRESETLIMPORTr_SET(dynresetlimport, fval);
        WRITE_DYNRESETLIMPORTr(unit, mport, dynresetlimport);

        /* LWMCOSCELLSETLIMIT_QLAYERr, index 0 ~ 7 */
        fval = egress_queue_min_reserve_downlink_ports_lossy;
        for (idx = 0; idx <= 7; idx++) {
            LWMCOSCELLSETLIMIT_QLAYERr_CLR(lccellslmt_qlr);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(lccellslmt_qlr, fval);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(lccellslmt_qlr, fval);
            WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, idx, lccellslmt_qlr);
        }

        /* LWMCOSCELLSETLIMIT_QLAYERr, index 8 ~ 63 */
        for (idx = 8; idx <= 63; idx++) {
            LWMCOSCELLSETLIMIT_QLAYERr_CLR(lccellslmt_qlr);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(lccellslmt_qlr, 0);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(lccellslmt_qlr, 0);
            WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, idx, lccellslmt_qlr);
        }

        /* HOLCOSCELLMAXLIMIT_QLAYERr, index 0 ~ 6 */
        fval = ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                            lwmcoscellsetlimit_qlayer0_cellsetlimit_up;
        fval = holcoscellmaxlimit_qlayer0_cellmaxlimit_down_1 -
                            ethernet_mtu_cell;
        for (idx = 0; idx <= 6; idx++) {
            HOLCOSCELLMAXLIMIT_QLAYERr_CLR(hccellmaxlmt_qlr);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(hccellmaxlmt_qlr, fval);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(hccellmaxlmt_qlr, fval1);
            WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, idx, hccellmaxlmt_qlr);
        }

        /* HOLCOSCELLMAXLIMIT_QLAYERr, index 7 */
        HOLCOSCELLMAXLIMIT_QLAYERr_CLR(hccellmaxlmt_qlr);
        fval = ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                            lwmcoscellsetlimit_qlayer7_cellsetlimit_up;
        HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(hccellmaxlmt_qlr, fval);
        fval = holcoscellmaxlimit_qlayer7_cellmaxlimit_down_1 -
                            ethernet_mtu_cell;
        HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(hccellmaxlmt_qlr, fval);
        WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, 7, hccellmaxlmt_qlr);

        /* HOLCOSCELLMAXLIMIT_QLAYERr, index 8 ~ 63 */
        fval = ceiling_func(shared_space_cells, queue_port_limit_ratio);
        fval1 = holcoscellmaxlimit_qlayer8_cellmaxlimit_down_1 - ethernet_mtu_cell;
        for (idx = 8; idx <= 63; idx++) {
            HOLCOSCELLMAXLIMIT_QLAYERr_CLR(hccellmaxlmt_qlr);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(hccellmaxlmt_qlr, fval);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(hccellmaxlmt_qlr, fval1);
            WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, idx, hccellmaxlmt_qlr);
        }

        /* DYNCELLLIMITr, index 0 */
        fval = dyncelllimit_dyncellsetlimit_down_1 - (ethernet_mtu_cell * 2);
        DYNCELLLIMITr_CLR(dyncelllimit);
        DYNCELLLIMITr_DYNCELLSETLIMITf_SET(dyncelllimit, shared_space_cells);
        DYNCELLLIMITr_DYNCELLRESETLIMITf_SET(dyncelllimit, fval);
        WRITE_DYNCELLLIMITr(unit, mport, dyncelllimit);

        /* COLOR_DROP_EN_QLAYERr, index 0 */
        COLOR_DROP_EN_QLAYERr_SET(color_drop_en_qlayer, 0);
        WRITE_COLOR_DROP_EN_QLAYERr(unit, mport, 0, color_drop_en_qlayer);

        /* HOLCOSPKTSETLIMIT_QGROUPr, index 0 ~ 7 */
        fval = numxqs_per_downlink_ports_and_cpu_port - 1;
        for (idx = 0; idx <= 7; idx++) {
            HOLCOSPKTSETLIMIT_QGROUPr_SET(hcpktslmt_qgp, fval);
            WRITE_HOLCOSPKTSETLIMIT_QGROUPr(unit, mport, idx, hcpktslmt_qgp);
        }

        /* HOLCOSPKTRESETLIMIT_QGROUPr, index 0 ~ 7 */
        fval = holcospktsetlimit_qgroup0_pktsetlimit_down_1 - 1;
        for (idx = 0; idx <= 7; idx++) {
            HOLCOSPKTRESETLIMIT_QGROUPr_SET(hcpktrslmt_qgp, fval);
            WRITE_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mport, idx, hcpktrslmt_qgp);
        }

        /* CNGCOSPKTLIMIT0_QGROUPr, index 0 ~ 7 */
        fval = numxqs_per_downlink_ports_and_cpu_port - 1;
        for (idx = 0; idx <= 7; idx++) {
            CNGCOSPKTLIMIT0_QGROUPr_SET(ccpktlmt0_qgp, fval);
            WRITE_CNGCOSPKTLIMIT0_QGROUPr(unit, mport, idx, ccpktlmt0_qgp);
        }

        /* CNGCOSPKTLIMIT1_QGROUPr, index 0 ~ 7 */
        fval = numxqs_per_downlink_ports_and_cpu_port - 1;
        for (idx = 0; idx <= 7; idx++) {
            CNGCOSPKTLIMIT1_QGROUPr_SET(ccpktlmt1_qgp, fval);
            WRITE_CNGCOSPKTLIMIT1_QGROUPr(unit, mport, idx, ccpktlmt1_qgp);
        }

        /* HOLCOSCELLMAXLIMIT_QGROUPr, index 0 ~ 7 */
        fval = ceiling_func(ttl_c_mem_adm,
                                             queue_port_limit_ratio);
        fval1 = holcoscellmaxlimit_qgroup0_cellmaxlimit_down_1 -
                                ethernet_mtu_cell;
        for (idx = 0; idx <= 7; idx++) {
            HOLCOSCELLMAXLIMIT_QGROUPr_CLR(hccellmaxlmt_qgp);
            
            HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXLIMITf_SET(hccellmaxlmt_qgp, fval);
            HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXRESUMELIMITf_SET(hccellmaxlmt_qgp, fval1);
            WRITE_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mport, idx, hccellmaxlmt_qgp);
        }

        /* COLOR_DROP_EN_QGROUPr, index 0 */
        COLOR_DROP_EN_QGROUPr_SET(color_drop_en_qgroup, 0);
        WRITE_COLOR_DROP_EN_QGROUPr(unit, mport, color_drop_en_qgroup);

        /* SHARED_POOL_CTRLr, index 0 */
        SHARED_POOL_CTRLr_CLR(shared_pool_ctrl);
        SHARED_POOL_CTRLr_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl, 255);
        SHARED_POOL_CTRLr_SHARED_POOL_DISCARD_ENf_SET(shared_pool_ctrl, 255);
        SHARED_POOL_CTRLr_SHARED_POOL_XOFF_ENf_SET(shared_pool_ctrl, 0);
        WRITE_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);

        /* SHARED_POOL_CTRL_EXT1r, index 0 */
        READ_SHARED_POOL_CTRL_EXT1r(unit, mport, &shared_pool_ctrl_ext1);
        SHARED_POOL_CTRL_EXT1r_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl_ext1,
                            0xFFFFFF);
        WRITE_SHARED_POOL_CTRL_EXT1r(unit, mport, shared_pool_ctrl_ext1);

        /* SHARED_POOL_CTRL_EXT2r, index 0 */
        SHARED_POOL_CTRL_EXT2r_SET(shared_pool_ctrl_ext2, 0xFFFFFFFF);
        WRITE_SHARED_POOL_CTRL_EXT2r(unit, mport, shared_pool_ctrl_ext2);
    }

    /* port-based : downlink 2.5G */
    CDK_PBMP_ITER(pbmp_downlink_2dot5g, pport) {
        mport = P2M(unit, pport);

        /* PG_CTRL0r, index 0 */
        READ_PG_CTRL0r(unit, mport, &pg_ctrl0);
        PG_CTRL0r_PPFC_PG_ENf_SET(pg_ctrl0, 0);
        PG_CTRL0r_PRI0_GRPf_SET(pg_ctrl0, 0);
        PG_CTRL0r_PRI1_GRPf_SET(pg_ctrl0, 1);
        PG_CTRL0r_PRI2_GRPf_SET(pg_ctrl0, 2);
        PG_CTRL0r_PRI3_GRPf_SET(pg_ctrl0, 3);
        PG_CTRL0r_PRI4_GRPf_SET(pg_ctrl0, 4);
        PG_CTRL0r_PRI5_GRPf_SET(pg_ctrl0, 5);
        PG_CTRL0r_PRI6_GRPf_SET(pg_ctrl0, 6);
        PG_CTRL0r_PRI7_GRPf_SET(pg_ctrl0, 7);
        WRITE_PG_CTRL0r(unit, mport, pg_ctrl0);

        /* PG_CTRL1r, index 0 */
        READ_PG_CTRL1r(unit, mport, &pg_ctrl1);
        PG_CTRL1r_PRI8_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI9_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI10_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI11_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI12_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI13_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI14_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI15_GRPf_SET(pg_ctrl1, 7);
        WRITE_PG_CTRL1r(unit, mport, pg_ctrl1);

        /* PG2TCr, index 0 ~ 7 */
        for (idx = 0; idx <= 7; idx++) {
            PG2TCr_SET(pg2tc, 0);
            WRITE_PG2TCr(unit, mport, idx, pg2tc);
        }

        /* IBPPKTSETLIMITr, index 0 */
        fval = ttl_c_mem_adm;
        IBPPKTSETLIMITr_CLR(ibppktsetlimit);
        IBPPKTSETLIMITr_PKTSETLIMITf_SET(ibppktsetlimit, fval);
        IBPPKTSETLIMITr_RESETLIMITSELf_SET(ibppktsetlimit, 3);
        WRITE_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);

        /* MMU_FC_RX_ENr, index 0 */
        MMU_FC_RX_ENr_SET(mmu_fc_rx_en, 0);
        WRITE_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);

        /* MMU_FC_TX_ENr, index 0 */
        MMU_FC_TX_ENr_SET(mmu_fc_tx_en, 0);
        WRITE_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);

        /* PGCELLLIMITr, index 0 ~ 6 */
        for (idx = 0; idx <= 6; idx++) {
            PGCELLLIMITr_CLR(pgcelllimit);
            PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit, ttl_c_mem_adm);
            PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit, ttl_c_mem_adm);
            WRITE_PGCELLLIMITr(unit, mport, idx, pgcelllimit);
        }

        /* PGCELLLIMITr, index 7 */
        fval = ttl_c_mem_adm;
        PGCELLLIMITr_CLR(pgcelllimit);
        PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit, fval);
        PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit, fval);
        WRITE_PGCELLLIMITr(unit, mport, 7, pgcelllimit);

        /* PGDISCARDSETLIMITr, index 0 ~ 6 */
        fval = ttl_c_mem_adm;
        for (idx = 0; idx <= 6; idx++) {
            PGDISCARDSETLIMITr_SET(pgdiscardsetlimit, fval);
            WRITE_PGDISCARDSETLIMITr(unit, mport, idx, pgdiscardsetlimit);
        }

        /* PGDISCARDSETLIMITr, index 7 */
        fval = ttl_c_mem_adm;
        PGDISCARDSETLIMITr_SET(pgdiscardsetlimit, fval);
        WRITE_PGDISCARDSETLIMITr(unit, mport, 7, pgdiscardsetlimit);

        /* HOLCOSMINXQCNT_QLAYERr, index 0 ~ 7 */
        for (idx = 0; idx <= 7; idx++) {
            HOLCOSMINXQCNT_QLAYERr_SET(hcminxqc_qlr, egrxqmin_rlsyp);
            WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mport, idx, hcminxqc_qlr);
        }

        /* HOLCOSMINXQCNT_QLAYERr, index 8 ~ 63 */
        for (idx = 8; idx <= 63; idx++) {
            HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(hcminxqc_qlr, 0);
            WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mport, idx, hcminxqc_qlr);
        }

        /* HOLCOSPKTSETLIMIT_QLAYERr, index 0 ~ 7 */
        fval = shared_xqs_per_downlink_port + egrxqmin_rlsyp;
        for (idx = 0; idx <= 7; idx++) {
            HOLCOSPKTSETLIMIT_QLAYERr_SET(hcpktslmt_qlr, fval);
            WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mport, idx, hcpktslmt_qlr);
        }

        /* HOLCOSPKTSETLIMIT_QLAYERr, index 8 ~ 63 */
        for (idx = 8; idx <= 63; idx++) {
            HOLCOSPKTSETLIMIT_QLAYERr_SET(hcpktslmt_qlr, shared_xqs_per_downlink_port);
            WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mport, idx, hcpktslmt_qlr);
        }
        

        /* HOLCOSPKTRESETLIMIT_QLAYERr, index 0 ~ 6 */
        fval = holcospktsetlimit_qlayer0_pktsetlimit_down_2dot5 - 1;
        for (idx = 0; idx <= 6; idx++) {
            HOLCOSPKTRESETLIMIT_QLAYERr_SET(hcpktrslmt_qlr, fval);
            WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, idx, hcpktrslmt_qlr);
        }
        

        /* HOLCOSPKTRESETLIMIT_QLAYERr, index 7 */
        fval = holcospktsetlimit_qlayer7_pktsetlimit_down_2dot5 - 1;
        HOLCOSPKTRESETLIMIT_QLAYERr_SET(hcpktrslmt_qlr, fval);
        WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, 7, hcpktrslmt_qlr);
        

        /* HOLCOSPKTRESETLIMIT_QLAYERr, index 8 ~ 63 */
        fval = holcospktsetlimit_qlayer8_pktsetlimit_down_2dot5 - 1;
        for (idx = 8; idx <= 63; idx++) {
            HOLCOSPKTRESETLIMIT_QLAYERr_SET(hcpktrslmt_qlr, fval);
            WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, idx, hcpktrslmt_qlr);
        }

        /* CNGCOSPKTLIMIT0_QLAYERr, index 0 ~ 63 */
        fval = numxqs_per_downlink_ports_and_cpu_port - 1;
        for (idx = 0; idx <= 63; idx++) {
            CNGCOSPKTLIMIT0_QLAYERr_SET(ccpktlmt0_qlr, fval);
            WRITE_CNGCOSPKTLIMIT0_QLAYERr(unit, mport, idx, ccpktlmt0_qlr);
        }

        /* CNGCOSPKTLIMIT1_QLAYERr, index 0 ~ 63 */
        fval = numxqs_per_downlink_ports_and_cpu_port - 1;
        for (idx = 0; idx <= 63; idx++) {
            CNGCOSPKTLIMIT1_QLAYERr_SET(ccpktlmt1_qlr, fval);
            WRITE_CNGCOSPKTLIMIT1_QLAYERr(unit, mport, idx, ccpktlmt1_qlr);
        }

        /* CNGPORTPKTLIMIT0r, index 0 */
        fval = numxqs_per_downlink_ports_and_cpu_port - 1;
        CNGPORTPKTLIMIT0r_SET(cngportpktlimit0, fval);
        WRITE_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);

        /* CNGPORTPKTLIMIT1r, index 0 */
        fval = numxqs_per_downlink_ports_and_cpu_port - 1;
        CNGPORTPKTLIMIT1r_SET(cngportpktlimit1, fval);
        WRITE_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);

        /* DYNXQCNTPORTr, index 0 */
        fval = shared_xqs_per_downlink_port - skidmarker - prefetch;
        DYNXQCNTPORTr_SET(dynxqcntport, fval);
        WRITE_DYNXQCNTPORTr(unit, mport, dynxqcntport);

        /* DYNRESETLIMPORTr, index 0 */
        fval = dynxqcntport_dynxqcntport_down_2dot5 - 2;
        DYNRESETLIMPORTr_SET(dynresetlimport, fval);
        WRITE_DYNRESETLIMPORTr(unit, mport, dynresetlimport);

        /* LWMCOSCELLSETLIMIT_QLAYERr, index 0 ~ 7 */
        fval = egress_queue_min_reserve_downlink_ports_lossy;
        for (idx = 0; idx <= 7; idx++) {
            LWMCOSCELLSETLIMIT_QLAYERr_CLR(lccellslmt_qlr);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(lccellslmt_qlr, fval);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(lccellslmt_qlr, fval);
            WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, idx, lccellslmt_qlr);
        }

        /* LWMCOSCELLSETLIMIT_QLAYERr, index 8 ~ 63 */
        for (idx = 8; idx <= 63; idx++) {
            LWMCOSCELLSETLIMIT_QLAYERr_CLR(lccellslmt_qlr);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(lccellslmt_qlr, 0);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(lccellslmt_qlr, 0);
            WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, idx, lccellslmt_qlr);
        }

        /* HOLCOSCELLMAXLIMIT_QLAYERr, index 0 ~ 6 */
        fval = ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                                lwmcoscellsetlimit_qlayer0_cellsetlimit_up;
        fval1 = holcoscellmaxlimit_qlayer0_cellmaxlimit_down_2dot5 -
                                ethernet_mtu_cell;
        for (idx = 0; idx <= 6; idx++) {
            HOLCOSCELLMAXLIMIT_QLAYERr_CLR(hccellmaxlmt_qlr);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(hccellmaxlmt_qlr, fval);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(hccellmaxlmt_qlr, fval1);
            WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, idx, hccellmaxlmt_qlr);
        }

        /* HOLCOSCELLMAXLIMIT_QLAYERr, index 7 */
        HOLCOSCELLMAXLIMIT_QLAYERr_CLR(hccellmaxlmt_qlr);
        fval = ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                            lwmcoscellsetlimit_qlayer7_cellsetlimit_up;
        HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(hccellmaxlmt_qlr, fval);
        fval = holcoscellmaxlimit_qlayer7_cellmaxlimit_down_2dot5 -
                            ethernet_mtu_cell;
        HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(hccellmaxlmt_qlr, fval);
        WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, 7, hccellmaxlmt_qlr);

        /* HOLCOSCELLMAXLIMIT_QLAYERr, index 8 ~ 63 */
        fval = ceiling_func(shared_space_cells, queue_port_limit_ratio);
        fval1 = holcoscellmaxlimit_qlayer8_cellmaxlimit_down_2dot5 - ethernet_mtu_cell;
        for (idx = 8; idx <= 63; idx++) {
            HOLCOSCELLMAXLIMIT_QLAYERr_CLR(hccellmaxlmt_qlr);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(hccellmaxlmt_qlr, fval);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(hccellmaxlmt_qlr, fval1);
            WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, idx, hccellmaxlmt_qlr);
        }

        /* DYNCELLLIMITr, index 0 */
        fval = dyncelllimit_dyncellsetlimit_down_2dot5 - (ethernet_mtu_cell * 2);
        DYNCELLLIMITr_CLR(dyncelllimit);
        DYNCELLLIMITr_DYNCELLSETLIMITf_SET(dyncelllimit, shared_space_cells);
        DYNCELLLIMITr_DYNCELLRESETLIMITf_SET(dyncelllimit, fval);
        WRITE_DYNCELLLIMITr(unit, mport, dyncelllimit);

        /* COLOR_DROP_EN_QLAYERr, index 0 */
        COLOR_DROP_EN_QLAYERr_SET(color_drop_en_qlayer, 0);
        WRITE_COLOR_DROP_EN_QLAYERr(unit, mport, 0, color_drop_en_qlayer);

        /* HOLCOSPKTSETLIMIT_QGROUPr, index 0 ~ 7 */
        fval = numxqs_per_downlink_ports_and_cpu_port - 1;
        for (idx = 0; idx <= 7; idx++) {
            HOLCOSPKTSETLIMIT_QGROUPr_SET(hcpktslmt_qgp, fval);
            WRITE_HOLCOSPKTSETLIMIT_QGROUPr(unit, mport, idx, hcpktslmt_qgp);
        }

        /* HOLCOSPKTRESETLIMIT_QGROUPr, index 0 ~ 7 */
        fval = holcospktsetlimit_qgroup0_pktsetlimit_down_2dot5 - 1;
        for (idx = 0; idx <= 7; idx++) {
            HOLCOSPKTRESETLIMIT_QGROUPr_SET(hcpktrslmt_qgp, fval);
            WRITE_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mport, idx, hcpktrslmt_qgp);
        }

        /* CNGCOSPKTLIMIT0_QGROUPr, index 0 ~ 7 */
        fval = numxqs_per_downlink_ports_and_cpu_port - 1;
        for (idx = 0; idx <= 7; idx++) {
            CNGCOSPKTLIMIT0_QGROUPr_SET(ccpktlmt0_qgp, fval);
            WRITE_CNGCOSPKTLIMIT0_QGROUPr(unit, mport, idx, ccpktlmt0_qgp);
        }

        /* CNGCOSPKTLIMIT1_QGROUPr, index 0 ~ 7 */
        fval = numxqs_per_downlink_ports_and_cpu_port - 1;
        for (idx = 0; idx <= 7; idx++) {
            CNGCOSPKTLIMIT1_QGROUPr_SET(ccpktlmt1_qgp, fval);
            WRITE_CNGCOSPKTLIMIT1_QGROUPr(unit, mport, idx, ccpktlmt1_qgp);
        }

        /* HOLCOSCELLMAXLIMIT_QGROUPr, index 0 ~ 7 */
        fval = ceiling_func(ttl_c_mem_adm, queue_port_limit_ratio);
        fval1 = holcoscellmaxlimit_qgroup0_cellmaxlimit_down_2dot5 - ethernet_mtu_cell;
        for (idx = 0; idx <= 7; idx++) {
            HOLCOSCELLMAXLIMIT_QGROUPr_CLR(hccellmaxlmt_qgp);
            HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXLIMITf_SET(hccellmaxlmt_qgp, fval);
            HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXRESUMELIMITf_SET(hccellmaxlmt_qgp, fval1);
            WRITE_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mport, idx, hccellmaxlmt_qgp);
        }

        /* COLOR_DROP_EN_QGROUPr, index 0 */
        COLOR_DROP_EN_QGROUPr_SET(color_drop_en_qgroup, 0);
        WRITE_COLOR_DROP_EN_QGROUPr(unit, mport, color_drop_en_qgroup);

        /* SHARED_POOL_CTRLr, index 0 */
        SHARED_POOL_CTRLr_CLR(shared_pool_ctrl);
        SHARED_POOL_CTRLr_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl, 255);
        SHARED_POOL_CTRLr_SHARED_POOL_DISCARD_ENf_SET(shared_pool_ctrl, 255);
        SHARED_POOL_CTRLr_SHARED_POOL_XOFF_ENf_SET(shared_pool_ctrl, 0);
        WRITE_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);

        /* SHARED_POOL_CTRL_EXT1r, index 0 */
        READ_SHARED_POOL_CTRL_EXT1r(unit, mport, &shared_pool_ctrl_ext1);
        SHARED_POOL_CTRL_EXT1r_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl_ext1,
                            0xFFFFFF);
        WRITE_SHARED_POOL_CTRL_EXT1r(unit, mport, shared_pool_ctrl_ext1);

        /* SHARED_POOL_CTRL_EXT2r, index 0 */
        SHARED_POOL_CTRL_EXT2r_SET(shared_pool_ctrl_ext2, 0xFFFFFFFF);
        WRITE_SHARED_POOL_CTRL_EXT2r(unit, mport, shared_pool_ctrl_ext2);
    }

    /* port-based : cpu port (lport=pport=mport=0) */
    CDK_PBMP_ITER(pbmp_cpu, mport) {
        /* PG_CTRL0r, index 0 */
        READ_PG_CTRL0r(unit, mport, &pg_ctrl0);
        PG_CTRL0r_PPFC_PG_ENf_SET(pg_ctrl0, 0);
        PG_CTRL0r_PRI0_GRPf_SET(pg_ctrl0, 0);
        PG_CTRL0r_PRI1_GRPf_SET(pg_ctrl0, 1);
        PG_CTRL0r_PRI2_GRPf_SET(pg_ctrl0, 2);
        PG_CTRL0r_PRI3_GRPf_SET(pg_ctrl0, 3);
        PG_CTRL0r_PRI4_GRPf_SET(pg_ctrl0, 4);
        PG_CTRL0r_PRI5_GRPf_SET(pg_ctrl0, 5);
        PG_CTRL0r_PRI6_GRPf_SET(pg_ctrl0, 6);
        PG_CTRL0r_PRI7_GRPf_SET(pg_ctrl0, 7);
        WRITE_PG_CTRL0r(unit, mport, pg_ctrl0);

        /* PG_CTRL1r, index 0 */
        READ_PG_CTRL1r(unit, mport, &pg_ctrl1);
        PG_CTRL1r_PRI8_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI9_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI10_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI11_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI12_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI13_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI14_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI15_GRPf_SET(pg_ctrl1, 7);
        WRITE_PG_CTRL1r(unit, mport, pg_ctrl1);

       /* PG2TCr, index 0 ~ 7 */
       for (idx = 0; idx <= 7; idx++) {
           PG2TCr_SET(pg2tc, 0);
           WRITE_PG2TCr(unit, mport, idx, pg2tc);
       }

       /* IBPPKTSETLIMITr, index 0 */
       IBPPKTSETLIMITr_CLR(ibppktsetlimit);
       IBPPKTSETLIMITr_PKTSETLIMITf_SET(ibppktsetlimit, ttl_c_mem_adm);
       IBPPKTSETLIMITr_RESETLIMITSELf_SET(ibppktsetlimit, 3);
       WRITE_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);

       /* MMU_FC_RX_ENr, index 0 */
       MMU_FC_RX_ENr_SET(mmu_fc_rx_en, 0);
       WRITE_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);

       /* MMU_FC_TX_ENr, index 0 */
       MMU_FC_TX_ENr_SET(mmu_fc_tx_en, 0);
       WRITE_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);

       /* PGCELLLIMITr, index 0 ~ 7 */
       for (idx = 0; idx <= 7; idx++) {
           PGCELLLIMITr_CLR(pgcelllimit);
           PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit, ttl_c_mem_adm);
           PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit, ttl_c_mem_adm);
           WRITE_PGCELLLIMITr(unit, mport, idx, pgcelllimit);
       }

       /* PGDISCARDSETLIMITr, index 0 ~ 7 */
       fval = ttl_c_mem_adm;
       for (idx = 0; idx <= 7; idx++) {
           PGDISCARDSETLIMITr_SET(pgdiscardsetlimit, fval);
           WRITE_PGDISCARDSETLIMITr(unit, mport, idx, pgdiscardsetlimit);
       }

       /* HOLCOSMINXQCNTr, index 0 ~ 7 */
       for (idx = 0; idx <= 7; idx++) {
           HOLCOSMINXQCNTr_SET(holcosminxqcnt, egrqmin_rcp);
           WRITE_HOLCOSMINXQCNTr(unit, mport, idx, holcosminxqcnt);
       }

       /* HOLCOSPKTSETLIMITr, index 0 ~ 6 */
       fval = shared_xqs_per_downlink_port + egrqmin_rcp;
       for (idx = 0; idx <= 6; idx++) {
           HOLCOSPKTSETLIMITr_SET(holcospktsetlimit, fval);
           WRITE_HOLCOSPKTSETLIMITr(unit, mport, idx, holcospktsetlimit);
       }

       /* HOLCOSPKTSETLIMITr, index 7 */
       fval = shared_xqs_per_downlink_port + egrqmin_rcp;
       HOLCOSPKTSETLIMITr_SET(holcospktsetlimit, fval);
       WRITE_HOLCOSPKTSETLIMITr(unit, mport, 7, holcospktsetlimit);

       /* HOLCOSPKTRESETLIMITr, index 0 ~ 6 */
       fval = holcospktsetlimit0_pktsetlimit_cpu - 1;
       for (idx = 0; idx <= 6; idx++) {
           HOLCOSPKTRESETLIMITr_SET(holcospktresetlimit, fval);
           WRITE_HOLCOSPKTRESETLIMITr(unit, mport, idx, holcospktresetlimit);
       }

       /* HOLCOSPKTRESETLIMITr, index 7 */
       fval = holcospktsetlimit7_pktsetlimit_cpu - 1;
       HOLCOSPKTRESETLIMITr_SET(holcospktresetlimit, fval);
       WRITE_HOLCOSPKTRESETLIMITr(unit, mport, 7, holcospktresetlimit);

       /* CNGCOSPKTLIMIT0r, index 0 ~ 7 */
       fval = numxqs_per_downlink_ports_and_cpu_port - 1;
       for (idx = 0; idx <= 7; idx++) {
           CNGCOSPKTLIMIT0r_SET(cngcospktlimit0, fval);
           WRITE_CNGCOSPKTLIMIT0r(unit, mport, idx, cngcospktlimit0);
       }

       /* CNGCOSPKTLIMIT1r, index 0 ~ 7 */
       fval = numxqs_per_downlink_ports_and_cpu_port - 1;
       for (idx = 0; idx <= 7; idx++) {
           CNGCOSPKTLIMIT1r_SET(cngcospktlimit1, fval);
           WRITE_CNGCOSPKTLIMIT1r(unit, mport, idx, cngcospktlimit1);
       }

       /* CNGPORTPKTLIMIT0r, index 0 */
       fval = numxqs_per_downlink_ports_and_cpu_port - 1;
       CNGPORTPKTLIMIT0r_SET(cngportpktlimit0, fval);
       WRITE_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);

       /* CNGPORTPKTLIMIT1r, index 0 */
       fval = numxqs_per_downlink_ports_and_cpu_port - 1;
       CNGPORTPKTLIMIT1r_SET(cngportpktlimit1, fval);
       WRITE_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);

       /* DYNXQCNTPORTr, index 0 */
       fval = shared_xqs_per_downlink_port - skidmarker - prefetch;
       DYNXQCNTPORTr_SET(dynxqcntport, fval);
       WRITE_DYNXQCNTPORTr(unit, mport, dynxqcntport);

       /* DYNRESETLIMPORTr, index 0 */
       fval = dynxqcntport_dynxqcntport_cpu - 2;
       DYNRESETLIMPORTr_SET(dynresetlimport, fval);
       WRITE_DYNRESETLIMPORTr(unit, mport, dynresetlimport);

       /* LWMCOSCELLSETLIMITr, index 0 ~ 7 */
       fval = egrqmin_rcp;
       for (idx = 0; idx <= 7; idx++) {
           LWMCOSCELLSETLIMITr_CLR(lwmcoscellsetlimit);
           LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit, fval);
           LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit, fval);
           WRITE_LWMCOSCELLSETLIMITr(unit, mport, idx, lwmcoscellsetlimit);
       }

       /* HOLCOSCELLMAXLIMITr, index 0 ~ 6 */
       fval = ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                               lwmcoscellsetlimit0_cellsetlimit_up;
       fval = holcoscellmaxlimit0_cellmaxlimit_cpu - ethernet_mtu_cell;
       for (idx = 0; idx <= 6; idx++) {
           HOLCOSCELLMAXLIMITr_CLR(holcoscellmaxlimit);
           HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, fval);
           HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit, fval1);
           WRITE_HOLCOSCELLMAXLIMITr(unit, mport, idx, holcoscellmaxlimit);
       }

       /* HOLCOSCELLMAXLIMITr, index 7 */
       HOLCOSCELLMAXLIMITr_CLR(holcoscellmaxlimit);
       fval = ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                           lwmcoscellsetlimit7_cellsetlimit_up;
       HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, fval);
       fval = holcoscellmaxlimit7_cellmaxlimit_cpu - ethernet_mtu_cell;
       HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit, fval);
       WRITE_HOLCOSCELLMAXLIMITr(unit, mport, 7, holcoscellmaxlimit);

       /* DYNCELLLIMITr, index 0 */
       fval = shared_space_cells - ethernet_mtu_cell * 2;
       DYNCELLLIMITr_CLR(dyncelllimit);
       DYNCELLLIMITr_DYNCELLSETLIMITf_SET(dyncelllimit, shared_space_cells);
       DYNCELLLIMITr_DYNCELLRESETLIMITf_SET(dyncelllimit, fval);
       WRITE_DYNCELLLIMITr(unit, mport, dyncelllimit);

       /* COLOR_DROP_ENr, index 0 */
       COLOR_DROP_ENr_SET(color_drop_en, 0);
       WRITE_COLOR_DROP_ENr(unit, mport, color_drop_en);

       /* SHARED_POOL_CTRLr, index 0 */
       SHARED_POOL_CTRLr_CLR(shared_pool_ctrl);
       SHARED_POOL_CTRLr_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl, 255);
       SHARED_POOL_CTRLr_SHARED_POOL_DISCARD_ENf_SET(shared_pool_ctrl, 255);
       SHARED_POOL_CTRLr_SHARED_POOL_XOFF_ENf_SET(shared_pool_ctrl, 0);
       WRITE_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);

    }
    return CDK_E_NONE;
}


static void
_mmu_init(int unit)
{
    cdk_pbmp_t pbmp_all, mmu_pbmp;
    int mmu_port, pport;
    IP_TO_CMICM_CREDIT_TRANSFERr_t credit_transter;
    MMUPORTENABLEr_t mmuportenable;

    _tdm_init(unit);

    IP_TO_CMICM_CREDIT_TRANSFERr_CLR(credit_transter);
    IP_TO_CMICM_CREDIT_TRANSFERr_TRANSFER_ENABLEf_SET(credit_transter, 1);
    IP_TO_CMICM_CREDIT_TRANSFERr_NUM_OF_CREDITSf_SET(credit_transter, 32);
    WRITE_IP_TO_CMICM_CREDIT_TRANSFERr(unit, credit_transter);

    _mmu_init_helper_lossy(unit);

    /* Port enable */
    bcm53570_a0_all_pbmp_get(unit, &pbmp_all);
    CDK_PBMP_PORT_ADD(pbmp_all, CMIC_PORT);
    CDK_PBMP_CLEAR(mmu_pbmp);
    CDK_PBMP_ITER(pbmp_all, pport) {
        mmu_port = P2M(unit, pport);
        CDK_PBMP_PORT_ADD(mmu_pbmp, mmu_port);
    }

    /* Add CPU port */
    MMUPORTENABLEr_CLR(mmuportenable);
    MMUPORTENABLEr_MMUPORTENABLEf_SET(mmuportenable, CDK_PBMP_WORD_GET(mmu_pbmp, 0));
    WRITE_MMUPORTENABLEr(unit, 0, mmuportenable);

    MMUPORTENABLEr_CLR(mmuportenable);
    MMUPORTENABLEr_MMUPORTENABLEf_SET(mmuportenable, CDK_PBMP_WORD_GET(mmu_pbmp, 1));
    WRITE_MMUPORTENABLEr(unit, 1, mmuportenable);

    MMUPORTENABLEr_CLR(mmuportenable);
    MMUPORTENABLEr_MMUPORTENABLEf_SET(mmuportenable, CDK_PBMP_WORD_GET(mmu_pbmp, 2));
    WRITE_MMUPORTENABLEr(unit, 2, mmuportenable);
}

static int
_xl_port_credit_reset(int unit, int port)
{
    int ioerr = 0;
    int bindex;
    PGW_XL_TXFIFO_CTRLr_t pgw_xl_txfifo_ctrl;
    XLPORT_ENABLE_REGr_t xlport_enable_reg;
    EGR_PORT_CREDIT_RESETm_t egr_port_credit_reset;

    /* Should only be called by XLMAC/CLMAC driver */
    if (!IS_XL(unit, port)) {
        return CDK_E_NONE;
    }

    bindex = SUBPORT(unit, port);
    ioerr += READ_XLPORT_ENABLE_REGr(unit, &xlport_enable_reg, port);
    if(bindex == 0) {
        XLPORT_ENABLE_REGr_PORT0f_SET(xlport_enable_reg,0);
    } else if(bindex == 1) {
        XLPORT_ENABLE_REGr_PORT1f_SET(xlport_enable_reg,0);
    } else if(bindex == 2) {
        XLPORT_ENABLE_REGr_PORT2f_SET(xlport_enable_reg,0);
    } else if(bindex == 3) {
        XLPORT_ENABLE_REGr_PORT3f_SET(xlport_enable_reg,0);
    }
    ioerr += WRITE_XLPORT_ENABLE_REGr(unit, xlport_enable_reg, port);

    ioerr += EGR_PORT_CREDIT_RESETm_CLR(egr_port_credit_reset);
    ioerr += EGR_PORT_CREDIT_RESETm_VALUEf_SET(egr_port_credit_reset, 1);
    /* To clear the port credit(per physical port) */
    ioerr += WRITE_EGR_PORT_CREDIT_RESETm(unit, port, egr_port_credit_reset);

    ioerr += READ_PGW_XL_TXFIFO_CTRLr(unit, port, &pgw_xl_txfifo_ctrl);
    PGW_XL_TXFIFO_CTRLr_MAC_CLR_COUNTf_SET(pgw_xl_txfifo_ctrl, 1);
    PGW_XL_TXFIFO_CTRLr_CORE_CLR_COUNTf_SET(pgw_xl_txfifo_ctrl, 1);
    ioerr += WRITE_PGW_XL_TXFIFO_CTRLr(unit, port, pgw_xl_txfifo_ctrl);
    BMD_SYS_USLEEP(1000);

    ioerr += EGR_PORT_CREDIT_RESETm_VALUEf_SET(egr_port_credit_reset, 0);
    /* To clear the port credit(per physical port) */
    ioerr += WRITE_EGR_PORT_CREDIT_RESETm(unit, port, egr_port_credit_reset);

    PGW_XL_TXFIFO_CTRLr_MAC_CLR_COUNTf_SET(pgw_xl_txfifo_ctrl, 0);
    PGW_XL_TXFIFO_CTRLr_CORE_CLR_COUNTf_SET(pgw_xl_txfifo_ctrl, 0);
    ioerr += WRITE_PGW_XL_TXFIFO_CTRLr(unit, port, pgw_xl_txfifo_ctrl);

    if(bindex == 0) {
        XLPORT_ENABLE_REGr_PORT0f_SET(xlport_enable_reg,1);
    } else if(bindex == 1) {
        XLPORT_ENABLE_REGr_PORT1f_SET(xlport_enable_reg,1);
    } else if(bindex == 2) {
        XLPORT_ENABLE_REGr_PORT2f_SET(xlport_enable_reg,1);
    } else if(bindex == 3) {
        XLPORT_ENABLE_REGr_PORT3f_SET(xlport_enable_reg,1);
    }
    ioerr += WRITE_XLPORT_ENABLE_REGr(unit, xlport_enable_reg, port);

    return CDK_E_NONE;
}

int
bcm53570_a0_cl_port_credit_reset(int unit, int port)
{
    int ioerr = 0;
    int bindex;
    PGW_CL_TXFIFO_CTRLr_t pgw_cl_txfifo_ctrl;
    CLPORT_ENABLE_REGr_t clport_enable_reg;
    EGR_PORT_CREDIT_RESETm_t egr_port_credit_reset;

    /* Should only be called by XLMAC/CLMAC driver */
    if (!IS_CL(unit, port)) {
        return CDK_E_NONE;
    }

    bindex = SUBPORT(unit, port);

    ioerr += READ_CLPORT_ENABLE_REGr(unit, &clport_enable_reg, port);
    if(bindex == 0) {
        CLPORT_ENABLE_REGr_PORT0f_SET(clport_enable_reg,0);
    } else if(bindex == 1) {
        CLPORT_ENABLE_REGr_PORT1f_SET(clport_enable_reg,0);
    } else if(bindex == 2) {
        CLPORT_ENABLE_REGr_PORT2f_SET(clport_enable_reg,0);
    } else if(bindex == 3) {
        CLPORT_ENABLE_REGr_PORT3f_SET(clport_enable_reg,0);
    }
    ioerr += WRITE_CLPORT_ENABLE_REGr(unit, clport_enable_reg, port);

    ioerr += EGR_PORT_CREDIT_RESETm_CLR(egr_port_credit_reset);
    ioerr += EGR_PORT_CREDIT_RESETm_VALUEf_SET(egr_port_credit_reset, 1);
    /* To clear the port credit(per physical port) */
    ioerr += WRITE_EGR_PORT_CREDIT_RESETm(unit, port, egr_port_credit_reset);

    ioerr += READ_PGW_CL_TXFIFO_CTRLr(unit, port, &pgw_cl_txfifo_ctrl);
    PGW_CL_TXFIFO_CTRLr_MAC_CLR_COUNTf_SET(pgw_cl_txfifo_ctrl, 1);
    PGW_CL_TXFIFO_CTRLr_CORE_CLR_COUNTf_SET(pgw_cl_txfifo_ctrl, 1);
    ioerr += WRITE_PGW_CL_TXFIFO_CTRLr(unit, port, pgw_cl_txfifo_ctrl);

    BMD_SYS_USLEEP(1000);

    ioerr += EGR_PORT_CREDIT_RESETm_VALUEf_SET(egr_port_credit_reset, 0);
    /* To clear the port credit(per physical port) */
    ioerr += WRITE_EGR_PORT_CREDIT_RESETm(unit, port, egr_port_credit_reset);

    PGW_CL_TXFIFO_CTRLr_MAC_CLR_COUNTf_SET(pgw_cl_txfifo_ctrl, 0);
    PGW_CL_TXFIFO_CTRLr_CORE_CLR_COUNTf_SET(pgw_cl_txfifo_ctrl, 0);
    ioerr += WRITE_PGW_CL_TXFIFO_CTRLr(unit, port, pgw_cl_txfifo_ctrl);

    if(bindex == 0) {
        CLPORT_ENABLE_REGr_PORT0f_SET(clport_enable_reg,1);
    } else if(bindex == 1) {
        CLPORT_ENABLE_REGr_PORT1f_SET(clport_enable_reg,1);
    } else if(bindex == 2) {
        CLPORT_ENABLE_REGr_PORT2f_SET(clport_enable_reg,1);
    } else if(bindex == 3) {
        CLPORT_ENABLE_REGr_PORT3f_SET(clport_enable_reg,1);
    }
    ioerr += WRITE_CLPORT_ENABLE_REGr(unit, clport_enable_reg, port);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

int
bcm53570_a0_xlport_init(int unit, int port)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    XLPORT_ENABLE_REGr_t xlport_enable;
    EGR_PORT_CREDIT_RESETm_t egr_port_credit_reset;
    PGW_XL_TXFIFO_CTRLr_t xlport_txfifo_ctrl;
    XLMAC_CTRLr_t xlmac_ctrl;
    XLMAC_RX_CTRLr_t xlmac_rx_ctrl;
    XLMAC_TX_CTRLr_t xlmac_tx_ctrl;
    XLMAC_PAUSE_CTRLr_t xlmac_pause_ctrl;
    XLMAC_RX_MAX_SIZEr_t xlmac_rx_max_size;
    XLMAC_MODEr_t xlmac_mode;
    XLMAC_RX_LSS_CTRLr_t xlmac_rx_lss_ctrl;
    XLMAC_PFC_CTRLr_t xlmac_pfc_ctrl;
    cdk_pbmp_t pbmp;
    int lport, speed_max;
    int idx, pidx;

    bcm53570_a0_xlport_pbmp_get(unit, &pbmp);
    if (CDK_PBMP_MEMBER(pbmp, port)) {
        speed_max = bcm53570_a0_port_speed_max(unit, port);

        if (SUBPORT(unit, port) == 0) {
            /* Disable XLPORT */
            XLPORT_ENABLE_REGr_CLR(xlport_enable);
            ioerr += WRITE_XLPORT_ENABLE_REGr(unit, xlport_enable, port);

            for (pidx = port; pidx <= (port + 3); pidx++) {
                lport = P2L(unit, pidx);
                if (lport < 0) {
                    continue;
                }

                /* CREDIT reset and MAC count clear */
                ioerr += READ_EGR_PORT_CREDIT_RESETm(unit, pidx, &egr_port_credit_reset);
                EGR_PORT_CREDIT_RESETm_VALUEf_SET(egr_port_credit_reset, 1);
                ioerr += WRITE_EGR_PORT_CREDIT_RESETm(unit, pidx, egr_port_credit_reset);

                ioerr += READ_PGW_XL_TXFIFO_CTRLr(unit, pidx, &xlport_txfifo_ctrl);
                PGW_XL_TXFIFO_CTRLr_MAC_CLR_COUNTf_SET(xlport_txfifo_ctrl, 1);
                PGW_XL_TXFIFO_CTRLr_CORE_CLR_COUNTf_SET(xlport_txfifo_ctrl, 1);
                ioerr += WRITE_PGW_XL_TXFIFO_CTRLr(unit, pidx, xlport_txfifo_ctrl);

                BMD_SYS_USLEEP(1000);

                ioerr += READ_EGR_PORT_CREDIT_RESETm(unit, pidx, &egr_port_credit_reset);
                EGR_PORT_CREDIT_RESETm_VALUEf_SET(egr_port_credit_reset, 0);
                ioerr += WRITE_EGR_PORT_CREDIT_RESETm(unit, pidx, egr_port_credit_reset);

                ioerr += READ_PGW_XL_TXFIFO_CTRLr(unit, pidx, &xlport_txfifo_ctrl);
                PGW_XL_TXFIFO_CTRLr_MAC_CLR_COUNTf_SET(xlport_txfifo_ctrl, 0);
                PGW_XL_TXFIFO_CTRLr_CORE_CLR_COUNTf_SET(xlport_txfifo_ctrl, 0);
                ioerr += WRITE_PGW_XL_TXFIFO_CTRLr(unit, pidx, xlport_txfifo_ctrl);
            }

            /* Enable XLPORT */
            for (idx = 0; idx <= 3; idx++) {
                lport = P2L(unit, port + idx);
                if (lport < 0) {
                    continue;
                }
                if (idx == 0) {
                    XLPORT_ENABLE_REGr_PORT0f_SET(xlport_enable, 1);
                } else if (idx == 1) {
                    XLPORT_ENABLE_REGr_PORT1f_SET(xlport_enable, 1);
                } else if (idx == 2) {
                    XLPORT_ENABLE_REGr_PORT2f_SET(xlport_enable, 1);
                } else if (idx == 3) {
                    XLPORT_ENABLE_REGr_PORT3f_SET(xlport_enable, 1);
                }
            }
            ioerr += WRITE_XLPORT_ENABLE_REGr(unit, xlport_enable, port);
        }

        /* XLMAC init */
        XLMAC_CTRLr_CLR(xlmac_ctrl);
        XLMAC_CTRLr_SOFT_RESETf_SET(xlmac_ctrl, 1);
        ioerr += WRITE_XLMAC_CTRLr(unit, port, xlmac_ctrl);

        ioerr += READ_XLMAC_CTRLr(unit, port, &xlmac_ctrl);
        XLMAC_CTRLr_SOFT_RESETf_SET(xlmac_ctrl, 0);
        XLMAC_CTRLr_SW_LINK_STATUSf_SET(xlmac_ctrl, 1);
        ioerr += WRITE_XLMAC_CTRLr(unit, port, xlmac_ctrl);

        ioerr += READ_XLMAC_RX_CTRLr(unit, port, &xlmac_rx_ctrl);
        XLMAC_RX_CTRLr_STRIP_CRCf_SET(xlmac_rx_ctrl, 0);
        XLMAC_RX_CTRLr_STRICT_PREAMBLEf_SET(xlmac_rx_ctrl, 0);
        ioerr += WRITE_XLMAC_RX_CTRLr(unit, port, xlmac_rx_ctrl);

        ioerr += READ_XLMAC_TX_CTRLr(unit, port, &xlmac_tx_ctrl);
        XLMAC_TX_CTRLr_AVERAGE_IPGf_SET(xlmac_tx_ctrl, 0xc);
        XLMAC_TX_CTRLr_CRC_MODEf_SET(xlmac_tx_ctrl, 2);
        ioerr += WRITE_XLMAC_TX_CTRLr(unit, port, xlmac_tx_ctrl);

        ioerr += READ_XLMAC_PAUSE_CTRLr(unit, port, &xlmac_pause_ctrl);
        XLMAC_PAUSE_CTRLr_TX_PAUSE_ENf_SET(xlmac_pause_ctrl, 1);
        XLMAC_PAUSE_CTRLr_RX_PAUSE_ENf_SET(xlmac_pause_ctrl, 1);
        ioerr += WRITE_XLMAC_PAUSE_CTRLr(unit, port, xlmac_pause_ctrl);

        ioerr += READ_XLMAC_PFC_CTRLr(unit, port, &xlmac_pfc_ctrl);
        XLMAC_PFC_CTRLr_PFC_REFRESH_ENf_SET(xlmac_pfc_ctrl, 1);
        ioerr += WRITE_XLMAC_PFC_CTRLr(unit, port, xlmac_pfc_ctrl);

        ioerr += READ_XLMAC_TX_CTRLr(unit, port, &xlmac_tx_ctrl);
        XLMAC_TX_CTRLr_THROT_NUMf_SET(xlmac_tx_ctrl, 0);
        ioerr += WRITE_XLMAC_TX_CTRLr(unit, port, xlmac_tx_ctrl);

        ioerr += READ_XLMAC_RX_MAX_SIZEr(unit, port, &xlmac_rx_max_size);
        XLMAC_RX_MAX_SIZEr_RX_MAX_SIZEf_SET(xlmac_rx_max_size, JUMBO_MAXSZ);
        ioerr += WRITE_XLMAC_RX_MAX_SIZEr(unit, port, xlmac_rx_max_size);

        ioerr += READ_XLMAC_MODEr(unit, port, &xlmac_mode);
        XLMAC_MODEr_HDR_MODEf_SET(xlmac_mode, 0);
        XLMAC_MODEr_SPEED_MODEf_SET(xlmac_mode, 4);
        if (speed_max == 1000) {
            XLMAC_MODEr_SPEED_MODEf_SET(xlmac_mode, 2);
        } else if (speed_max == 2500) {
            XLMAC_MODEr_SPEED_MODEf_SET(xlmac_mode, 3);
        }
        ioerr += WRITE_XLMAC_MODEr(unit, port, xlmac_mode);

        ioerr += READ_XLMAC_RX_LSS_CTRLr(unit, port, &xlmac_rx_lss_ctrl);
        XLMAC_RX_LSS_CTRLr_DROP_TX_DATA_ON_LOCAL_FAULTf_SET(xlmac_rx_lss_ctrl, 1);
        XLMAC_RX_LSS_CTRLr_DROP_TX_DATA_ON_REMOTE_FAULTf_SET(xlmac_rx_lss_ctrl, 1);
        XLMAC_RX_LSS_CTRLr_DROP_TX_DATA_ON_LINK_INTERRUPTf_SET(xlmac_rx_lss_ctrl, 1);
        ioerr += WRITE_XLMAC_RX_LSS_CTRLr(unit, port, xlmac_rx_lss_ctrl);

        /* Disable loopback and bring XLMAC out of reset */
        ioerr += READ_XLMAC_CTRLr(unit, port, &xlmac_ctrl);
        XLMAC_CTRLr_RX_ENf_SET(xlmac_ctrl, 1);
        XLMAC_CTRLr_TX_ENf_SET(xlmac_ctrl, 1);
        ioerr += WRITE_XLMAC_CTRLr(unit, port, xlmac_ctrl);
    }

    return ioerr ? CDK_E_IO : rv;
}

int
bcm53570_a0_clport_init(int unit, int port)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    CLPORT_ENABLE_REGr_t clport_enable;
    EGR_PORT_CREDIT_RESETm_t egr_port_credit_reset;
    PGW_CL_TXFIFO_CTRLr_t clport_txfifo_ctrl;
    CLMAC_CTRLr_t clmac_ctrl;
    CLMAC_RX_CTRLr_t clmac_rx_ctrl;
    CLMAC_TX_CTRLr_t clmac_tx_ctrl;
    CLMAC_PAUSE_CTRLr_t clmac_pause_ctrl;
    CLMAC_RX_MAX_SIZEr_t clmac_rx_max_size;
    CLMAC_MODEr_t clmac_mode;
    CLMAC_RX_LSS_CTRLr_t clmac_rx_lss_ctrl;
    CLMAC_PFC_CTRLr_t clmac_pfc_ctrl;
    cdk_pbmp_t pbmp;
    int lport, speed_max;
    int idx, pidx;

    bcm53570_a0_clport_pbmp_get(unit, &pbmp);
    if (CDK_PBMP_MEMBER(pbmp, port)) {
        speed_max = bcm53570_a0_port_speed_max(unit, port);

        if (SUBPORT(unit, port) == 0) {
            /* Disable CLPORT */
            CLPORT_ENABLE_REGr_CLR(clport_enable);
            ioerr += WRITE_CLPORT_ENABLE_REGr(unit, clport_enable, port);

            for (pidx = port; pidx <= (port + 3); pidx++) {
                lport = P2L(unit, pidx);
                if (lport < 0) {
                    continue;
                }

                /* CREDIT reset and MAC count clear */
                ioerr += READ_EGR_PORT_CREDIT_RESETm(unit, pidx, &egr_port_credit_reset);
                EGR_PORT_CREDIT_RESETm_VALUEf_SET(egr_port_credit_reset, 1);
                ioerr += WRITE_EGR_PORT_CREDIT_RESETm(unit, pidx, egr_port_credit_reset);

                ioerr += READ_PGW_CL_TXFIFO_CTRLr(unit, pidx, &clport_txfifo_ctrl);
                PGW_CL_TXFIFO_CTRLr_MAC_CLR_COUNTf_SET(clport_txfifo_ctrl, 1);
                PGW_CL_TXFIFO_CTRLr_CORE_CLR_COUNTf_SET(clport_txfifo_ctrl, 1);
                ioerr += WRITE_PGW_CL_TXFIFO_CTRLr(unit, pidx, clport_txfifo_ctrl);

                BMD_SYS_USLEEP(1000);

                ioerr += READ_EGR_PORT_CREDIT_RESETm(unit, pidx, &egr_port_credit_reset);
                EGR_PORT_CREDIT_RESETm_VALUEf_SET(egr_port_credit_reset, 0);
                ioerr += WRITE_EGR_PORT_CREDIT_RESETm(unit, pidx, egr_port_credit_reset);

                ioerr += READ_PGW_CL_TXFIFO_CTRLr(unit, pidx, &clport_txfifo_ctrl);
                PGW_CL_TXFIFO_CTRLr_MAC_CLR_COUNTf_SET(clport_txfifo_ctrl, 0);
                PGW_CL_TXFIFO_CTRLr_CORE_CLR_COUNTf_SET(clport_txfifo_ctrl, 0);
                ioerr += WRITE_PGW_CL_TXFIFO_CTRLr(unit, pidx, clport_txfifo_ctrl);
            }

            /* Enable CLPORT */
            for (idx = 0; idx <= 3; idx++) {
                lport = P2L(unit, port + idx);
                if (lport < 0) {
                    continue;
                }
                if (idx == 0) {
                    CLPORT_ENABLE_REGr_PORT0f_SET(clport_enable, 1);
                } else if (idx == 1) {
                    CLPORT_ENABLE_REGr_PORT1f_SET(clport_enable, 1);
                } else if (idx == 2) {
                    CLPORT_ENABLE_REGr_PORT2f_SET(clport_enable, 1);
                } else if (idx == 3) {
                    CLPORT_ENABLE_REGr_PORT3f_SET(clport_enable, 1);
                }
            }
            ioerr += WRITE_CLPORT_ENABLE_REGr(unit, clport_enable, port);
        }

        /* CLMAC init */
        CLMAC_CTRLr_CLR(clmac_ctrl);
        CLMAC_CTRLr_SOFT_RESETf_SET(clmac_ctrl, 1);
        ioerr += WRITE_CLMAC_CTRLr(unit, port, clmac_ctrl);

        ioerr += READ_CLMAC_CTRLr(unit, port, &clmac_ctrl);
        CLMAC_CTRLr_SOFT_RESETf_SET(clmac_ctrl, 0);
        CLMAC_CTRLr_SW_LINK_STATUSf_SET(clmac_ctrl, 1);
        ioerr += WRITE_CLMAC_CTRLr(unit, port, clmac_ctrl);

        ioerr += READ_CLMAC_RX_CTRLr(unit, port, &clmac_rx_ctrl);
        CLMAC_RX_CTRLr_STRIP_CRCf_SET(clmac_rx_ctrl, 0);
        CLMAC_RX_CTRLr_STRICT_PREAMBLEf_SET(clmac_rx_ctrl, 0);
        ioerr += WRITE_CLMAC_RX_CTRLr(unit, port, clmac_rx_ctrl);

        ioerr += READ_CLMAC_TX_CTRLr(unit, port, &clmac_tx_ctrl);
        CLMAC_TX_CTRLr_AVERAGE_IPGf_SET(clmac_tx_ctrl, 0xc);
        CLMAC_TX_CTRLr_CRC_MODEf_SET(clmac_tx_ctrl, 2);
        ioerr += WRITE_CLMAC_TX_CTRLr(unit, port, clmac_tx_ctrl);

        ioerr += READ_CLMAC_PAUSE_CTRLr(unit, port, &clmac_pause_ctrl);
        CLMAC_PAUSE_CTRLr_TX_PAUSE_ENf_SET(clmac_pause_ctrl, 1);
        CLMAC_PAUSE_CTRLr_RX_PAUSE_ENf_SET(clmac_pause_ctrl, 1);
        ioerr += WRITE_CLMAC_PAUSE_CTRLr(unit, port, clmac_pause_ctrl);

        ioerr += READ_CLMAC_PFC_CTRLr(unit, port, &clmac_pfc_ctrl);
        CLMAC_PFC_CTRLr_PFC_REFRESH_ENf_SET(clmac_pfc_ctrl, 1);
        ioerr += WRITE_CLMAC_PFC_CTRLr(unit, port, clmac_pfc_ctrl);

        ioerr += READ_CLMAC_TX_CTRLr(unit, port, &clmac_tx_ctrl);
        CLMAC_TX_CTRLr_THROT_NUMf_SET(clmac_tx_ctrl, 0);
        ioerr += WRITE_CLMAC_TX_CTRLr(unit, port, clmac_tx_ctrl);

        ioerr += READ_CLMAC_RX_MAX_SIZEr(unit, port, &clmac_rx_max_size);
        CLMAC_RX_MAX_SIZEr_RX_MAX_SIZEf_SET(clmac_rx_max_size, JUMBO_MAXSZ);
        ioerr += WRITE_CLMAC_RX_MAX_SIZEr(unit, port, clmac_rx_max_size);

        ioerr += READ_CLMAC_MODEr(unit, port, &clmac_mode);
        CLMAC_MODEr_HDR_MODEf_SET(clmac_mode, 0);
        CLMAC_MODEr_SPEED_MODEf_SET(clmac_mode, 4);
        if (speed_max == 1000) {
            CLMAC_MODEr_SPEED_MODEf_SET(clmac_mode, 2);
        } else if (speed_max == 2500) {
            CLMAC_MODEr_SPEED_MODEf_SET(clmac_mode, 3);
        }
        ioerr += WRITE_CLMAC_MODEr(unit, port, clmac_mode);

        ioerr += READ_CLMAC_RX_LSS_CTRLr(unit, port, &clmac_rx_lss_ctrl);
        CLMAC_RX_LSS_CTRLr_DROP_TX_DATA_ON_LOCAL_FAULTf_SET(clmac_rx_lss_ctrl, 1);
        CLMAC_RX_LSS_CTRLr_DROP_TX_DATA_ON_REMOTE_FAULTf_SET(clmac_rx_lss_ctrl, 1);
        CLMAC_RX_LSS_CTRLr_DROP_TX_DATA_ON_LINK_INTERRUPTf_SET(clmac_rx_lss_ctrl, 1);
        ioerr += WRITE_CLMAC_RX_LSS_CTRLr(unit, port, clmac_rx_lss_ctrl);

        /* Disable loopback and bring CLMAC out of reset */
        ioerr += READ_CLMAC_CTRLr(unit, port, &clmac_ctrl);
        CLMAC_CTRLr_RX_ENf_SET(clmac_ctrl, 1);
        CLMAC_CTRLr_TX_ENf_SET(clmac_ctrl, 1);
        ioerr += WRITE_CLMAC_CTRLr(unit, port, clmac_ctrl);
    }

    return ioerr ? CDK_E_IO : rv;
}

static int
_mac_xl_init(int unit, int port)
{
    int ioerr = 0;
    XLMAC_CTRLr_t xlmac_ctrl;
    XLMAC_PAUSE_CTRLr_t xlmac_pause_ctrl;
    XLMAC_RX_CTRLr_t xlmac_rx_ctrl;
    XLMAC_TX_CTRLr_t xlmac_tx_ctrl;
    XLMAC_PFC_CTRLr_t xlmac_pfc_ctrl;
    XLMAC_RX_MAX_SIZEr_t xlmac_rx_max_size;
    XLMAC_MODEr_t xlmac_mode;
    XLMAC_RX_LSS_CTRLr_t xlmac_rx_lss_ctrl;
    int ipg;
    int mode;
    int runt;

    /* Disable Tx/Rx, assume that MAC is stable (or out of reset) */
    ioerr += READ_XLMAC_CTRLr(unit, port, &xlmac_ctrl);
    /* Reset EP credit before de-assert SOFT_RESET */
    if(XLMAC_CTRLr_SOFT_RESETf_GET(xlmac_ctrl)){
        _xl_port_credit_reset(unit, port);
    }
    XLMAC_CTRLr_SOFT_RESETf_SET(xlmac_ctrl, 0);
    XLMAC_CTRLr_RX_ENf_SET(xlmac_ctrl, 0);
    XLMAC_CTRLr_TX_ENf_SET(xlmac_ctrl, 1);
    XLMAC_CTRLr_XGMII_IPG_CHECK_DISABLEf_SET(xlmac_ctrl,
                    (BMD_PORT_PROPERTIES(unit, port) & BMD_PORT_HG) ? 1 : 0);

    ioerr += WRITE_XLMAC_CTRLr(unit, port, xlmac_ctrl);

    ioerr += READ_XLMAC_PAUSE_CTRLr(unit, port, &xlmac_pause_ctrl);
    XLMAC_PAUSE_CTRLr_TX_PAUSE_ENf_SET(xlmac_pause_ctrl, 1);
    XLMAC_PAUSE_CTRLr_RX_PAUSE_ENf_SET(xlmac_pause_ctrl, 1);
    ioerr += WRITE_XLMAC_PAUSE_CTRLr(unit, port, xlmac_pause_ctrl);

    ioerr += READ_XLMAC_PFC_CTRLr(unit, port, &xlmac_pfc_ctrl);
    XLMAC_PFC_CTRLr_PFC_REFRESH_ENf_SET(xlmac_pfc_ctrl, 1);
    ioerr += WRITE_XLMAC_PFC_CTRLr(unit, port, xlmac_pfc_ctrl);

    /* Set jumbo max size (16360 byte payload) */
    XLMAC_RX_MAX_SIZEr_CLR(xlmac_rx_max_size);
    XLMAC_RX_MAX_SIZEr_RX_MAX_SIZEf_SET(xlmac_rx_max_size, JUMBO_MAXSZ);
    ioerr += WRITE_XLMAC_RX_MAX_SIZEr(unit, port, xlmac_rx_max_size);

    XLMAC_MODEr_CLR(xlmac_mode);
    switch (SPEED_MAX(unit, port)) {
    case 10:
        mode = COMMAND_CONFIG_SPEED_10;
        break;
    case 100:
        mode = COMMAND_CONFIG_SPEED_100;
        break;
    case 1000:
        mode = COMMAND_CONFIG_SPEED_1000;
        break;
    case 2500:
        mode = COMMAND_CONFIG_SPEED_2500;
        break;
    default:
        mode = COMMAND_CONFIG_SPEED_10000;
        break;
    }
    XLMAC_MODEr_SPEED_MODEf_SET(xlmac_mode, mode);
    ioerr += WRITE_XLMAC_MODEr(unit, port, xlmac_mode);

    /* init IPG and RUNT_THRESHOLD after port encap mode been established. */
    ipg = FD_XE_IPG;
    runt = XLMAC_RUNT_THRESHOLD_IEEE;

    ioerr += READ_XLMAC_TX_CTRLr(unit, port, &xlmac_tx_ctrl);
    XLMAC_TX_CTRLr_AVERAGE_IPGf_SET(xlmac_tx_ctrl, ((ipg / 8) & 0x1f) );
    XLMAC_TX_CTRLr_CRC_MODEf_SET(xlmac_tx_ctrl, XLMAC_CRC_PER_PKT_MODE );
    ioerr += WRITE_XLMAC_TX_CTRLr(unit, port, xlmac_tx_ctrl);

    ioerr += READ_XLMAC_RX_CTRLr(unit, port, &xlmac_rx_ctrl);
    XLMAC_RX_CTRLr_STRIP_CRCf_SET(xlmac_rx_ctrl, 0);
    XLMAC_RX_CTRLr_STRICT_PREAMBLEf_SET(xlmac_rx_ctrl,
        SPEED_MAX(unit, port) >= 10000 &&
        (BMD_PORT_PROPERTIES(unit, port) & BMD_PORT_XE) ? 1 : 0);

    /* assigning RUNT_THRESHOLD (per encap mode) */
    XLMAC_RX_CTRLr_RUNT_THRESHOLDf_SET(xlmac_rx_ctrl, runt);
    ioerr += WRITE_XLMAC_RX_CTRLr(unit, port, xlmac_rx_ctrl);

    ioerr += READ_XLMAC_RX_LSS_CTRLr(unit, port, &xlmac_rx_lss_ctrl);
    XLMAC_RX_LSS_CTRLr_DROP_TX_DATA_ON_LOCAL_FAULTf_SET(xlmac_rx_lss_ctrl, 1);
    XLMAC_RX_LSS_CTRLr_DROP_TX_DATA_ON_REMOTE_FAULTf_SET(xlmac_rx_lss_ctrl, 1);
    XLMAC_RX_LSS_CTRLr_DROP_TX_DATA_ON_LINK_INTERRUPTf_SET(xlmac_rx_lss_ctrl, 1);
    ioerr += WRITE_XLMAC_RX_LSS_CTRLr(unit, port, xlmac_rx_lss_ctrl);

    /* Disable loopback and bring XLMAC out of reset */
    ioerr += READ_XLMAC_CTRLr(unit, port, &xlmac_ctrl);
    XLMAC_CTRLr_LOCAL_LPBKf_SET(xlmac_ctrl, 0);
    XLMAC_CTRLr_RX_ENf_SET(xlmac_ctrl, 1);
    XLMAC_CTRLr_TX_ENf_SET(xlmac_ctrl, 1);
    ioerr += WRITE_XLMAC_CTRLr(unit, port, xlmac_ctrl);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

static int
_mac_cl_init(int unit, int port)
{
    int ioerr = 0;
    CLMAC_CTRLr_t clmac_ctrl;
    CLMAC_PAUSE_CTRLr_t clmac_pause_ctrl;
    CLMAC_RX_CTRLr_t clmac_rx_ctrl;
    CLMAC_TX_CTRLr_t clmac_tx_ctrl;
    CLMAC_PFC_CTRLr_t clmac_pfc_ctrl;
    CLMAC_RX_MAX_SIZEr_t clmac_rx_max_size;
    CLMAC_RX_LSS_CTRLr_t clmac_rx_lss_ctrl;
    int ipg;
    int runt;

    /* Disable Tx/Rx, assume that MAC is stable (or out of reset) */
    ioerr += READ_CLMAC_CTRLr(unit, port, &clmac_ctrl);
    /* Reset EP credit before de-assert SOFT_RESET */
    if(CLMAC_CTRLr_SOFT_RESETf_GET(clmac_ctrl)){
        bcm53570_a0_cl_port_credit_reset(unit, port);
    }
    CLMAC_CTRLr_SOFT_RESETf_SET(clmac_ctrl, 0);
    CLMAC_CTRLr_RX_ENf_SET(clmac_ctrl, 0);
    CLMAC_CTRLr_TX_ENf_SET(clmac_ctrl, 0);
    CLMAC_CTRLr_XGMII_IPG_CHECK_DISABLEf_SET(clmac_ctrl,
                    (BMD_PORT_PROPERTIES(unit, port) & BMD_PORT_HG) ? 1 : 0);
    ioerr += WRITE_CLMAC_CTRLr(unit, port, clmac_ctrl);

    /* init IPG and RUNT_THRESHOLD after port encap mode been established. */
    ipg = (BMD_PORT_PROPERTIES(unit, port) & BMD_PORT_HG)? FD_HG_IPG : FD_XE_IPG;
    runt = XLMAC_RUNT_THRESHOLD_IEEE;

    ioerr += READ_CLMAC_TX_CTRLr(unit, port, &clmac_tx_ctrl);
    CLMAC_TX_CTRLr_AVERAGE_IPGf_SET(clmac_tx_ctrl, ((ipg / 8) & 0x1f) );
    CLMAC_TX_CTRLr_CRC_MODEf_SET(clmac_tx_ctrl, XLMAC_CRC_PER_PKT_MODE );
    ioerr += WRITE_CLMAC_TX_CTRLr(unit, port, clmac_tx_ctrl);

    ioerr += READ_CLMAC_PAUSE_CTRLr(unit, port, &clmac_pause_ctrl);
    CLMAC_PAUSE_CTRLr_TX_PAUSE_ENf_SET(clmac_pause_ctrl, 1);
    CLMAC_PAUSE_CTRLr_RX_PAUSE_ENf_SET(clmac_pause_ctrl, 1);
    ioerr += WRITE_CLMAC_PAUSE_CTRLr(unit, port, clmac_pause_ctrl);

    ioerr += READ_CLMAC_PFC_CTRLr(unit, port, &clmac_pfc_ctrl);
    CLMAC_PFC_CTRLr_PFC_REFRESH_ENf_SET(clmac_pfc_ctrl, 1);
    ioerr += WRITE_CLMAC_PFC_CTRLr(unit, port, clmac_pfc_ctrl);

    /* Set jumbo max size (16360 byte payload) */
    CLMAC_RX_MAX_SIZEr_CLR(clmac_rx_max_size);
    CLMAC_RX_MAX_SIZEr_RX_MAX_SIZEf_SET(clmac_rx_max_size, JUMBO_MAXSZ);
    ioerr += WRITE_CLMAC_RX_MAX_SIZEr(unit, port, clmac_rx_max_size);

    ioerr += READ_CLMAC_RX_CTRLr(unit, port, &clmac_rx_ctrl);
    CLMAC_RX_CTRLr_STRIP_CRCf_SET(clmac_rx_ctrl, 0);
    CLMAC_RX_CTRLr_STRICT_PREAMBLEf_SET(clmac_rx_ctrl,
        SPEED_MAX(unit, port) >= 10000 &&
        (BMD_PORT_PROPERTIES(unit, port) & BMD_PORT_XE) ? 1 : 0);

    /* assigning RUNT_THRESHOLD (per encap mode) */
    CLMAC_RX_CTRLr_RUNT_THRESHOLDf_SET(clmac_rx_ctrl, runt);
    ioerr += WRITE_CLMAC_RX_CTRLr(unit, port, clmac_rx_ctrl);

    ioerr += READ_CLMAC_RX_LSS_CTRLr(unit, port, &clmac_rx_lss_ctrl);
    CLMAC_RX_LSS_CTRLr_DROP_TX_DATA_ON_LOCAL_FAULTf_SET(clmac_rx_lss_ctrl, 1);
    CLMAC_RX_LSS_CTRLr_DROP_TX_DATA_ON_REMOTE_FAULTf_SET(clmac_rx_lss_ctrl, 1);
    CLMAC_RX_LSS_CTRLr_DROP_TX_DATA_ON_LINK_INTERRUPTf_SET(clmac_rx_lss_ctrl, 1);
    ioerr += WRITE_CLMAC_RX_LSS_CTRLr(unit, port, clmac_rx_lss_ctrl);

    /* Disable loopback and bring CLMAC out of reset */
    ioerr += READ_CLMAC_CTRLr(unit, port, &clmac_ctrl);
    CLMAC_CTRLr_LOCAL_LPBKf_SET(clmac_ctrl, 0);
    CLMAC_CTRLr_RX_ENf_SET(clmac_ctrl, 1);
    CLMAC_CTRLr_TX_ENf_SET(clmac_ctrl, 1);
    ioerr += WRITE_CLMAC_CTRLr(unit, port, clmac_ctrl);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

static int
_mac_uni_sw_reset(int unit, int lport, int reset_assert)
{
    int ioerr = 0, rv = CDK_E_NONE;
    int reset_sleep_usec = 2;
    COMMAND_CONFIGr_t command_config;

    ioerr += READ_COMMAND_CONFIGr(unit, lport, &command_config);
    if (reset_assert) {
        /* SIDE EFFECT: TX and RX are disabled when SW_RESET is set. */
        /* Assert SW_RESET */
        COMMAND_CONFIGr_SW_RESETf_SET(command_config, 1);
    } else {
        /* Deassert SW_RESET */
        COMMAND_CONFIGr_SW_RESETf_SET(command_config, 0);
    }
    ioerr += WRITE_COMMAND_CONFIGr(unit, lport, command_config);

    BMD_SYS_USLEEP(reset_sleep_usec);

    return ioerr ? CDK_E_IO : rv;
}

static int
_mac_uni_init(int unit, int port)
{
    int ioerr = 0, rv = CDK_E_NONE;
    int lport = P2L(unit, port);
    COMMAND_CONFIGr_t command_config;
    GPORT_RSV_MASKr_t gport_rsv_mask;
    GPORT_STAT_UPDATE_MASKr_t gport_stat_update_mask;
    TX_IPG_LENGTHr_t tx_ipg_length;

    /* First put the MAC in reset and sleep */
    rv = _mac_uni_sw_reset(unit, lport, TRUE);

    /* Do the initialization */
    /*
     *   ETH_SPEEDf = 1000, PROMIS_ENf = 1, CRC_FWDf=1, PAUSE_FWDf = 1
     */
    ioerr += READ_COMMAND_CONFIGr(unit, lport, &command_config);
    COMMAND_CONFIGr_CLR(command_config);
    COMMAND_CONFIGr_SW_RESETf_SET(command_config, 1);
    COMMAND_CONFIGr_PROMIS_ENf_SET(command_config, 1);
    COMMAND_CONFIGr_PAUSE_FWDf_SET(command_config, 1);
    COMMAND_CONFIGr_NO_LGTH_CHECKf_SET(command_config, 1);
    COMMAND_CONFIGr_ETH_SPEEDf_SET(command_config, 2);
    COMMAND_CONFIGr_CRC_FWDf_SET(command_config, 1);
    ioerr += WRITE_COMMAND_CONFIGr(unit, lport, command_config);

    /* Initialize mask for purging packet data received from the MAC */
    GPORT_RSV_MASKr_CLR(gport_rsv_mask);
    GPORT_RSV_MASKr_MASKf_SET(gport_rsv_mask, 0x70);
    ioerr += WRITE_GPORT_RSV_MASKr(unit, gport_rsv_mask, lport);

    GPORT_STAT_UPDATE_MASKr_CLR(gport_stat_update_mask);
    GPORT_STAT_UPDATE_MASKr_MASKf_SET(gport_stat_update_mask, 0x70);
    ioerr += WRITE_GPORT_STAT_UPDATE_MASKr(unit, gport_stat_update_mask, lport);

    /* Bring the UniMAC out of reset */
    rv = _mac_uni_sw_reset(unit, lport, FALSE);

    TX_IPG_LENGTHr_CLR(tx_ipg_length);
    TX_IPG_LENGTHr_TX_IPG_LENGTHf_SET(tx_ipg_length, 12);
    ioerr += WRITE_TX_IPG_LENGTHr(unit, lport, tx_ipg_length);

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
    ioerr += READ_PORT_TABm(unit,lport,&port_tab);
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


static void
_gport_init(int unit, int port)
{
    int ioerr = 0;
    IPG_HD_BKP_CNTLr_t ipg_hd_bkp_cntl;
    uint32_t oenable;

    /* Common port initialization */
    ioerr += _port_init(unit, port);

    _mac_uni_init(unit, port);

    ioerr += READ_IPG_HD_BKP_CNTLr(unit, port, &ipg_hd_bkp_cntl);
    oenable = IPG_HD_BKP_CNTLr_HD_FC_ENAf_GET(ipg_hd_bkp_cntl);
    if (oenable != 0) {
        IPG_HD_BKP_CNTLr_HD_FC_ENAf_SET(ipg_hd_bkp_cntl, 0);
        ioerr += WRITE_IPG_HD_BKP_CNTLr(unit, port, ipg_hd_bkp_cntl);
    }
}

static void
_clport_init(int unit, int port)
{
    int ioerr = 0;

    /* Common port initialization */
    ioerr += _port_init(unit, port);

    _mac_cl_init(unit, port);
}

static void
_xlport_init(int unit, int port)
{
    int ioerr = 0;
    XLPORT_MAC_CONTROLr_t xlp_mac_ctrl;

    /* Common port initialization */
    ioerr += _port_init(unit, port);

    if (BLK_PORT(unit, port) == 0) {
        ioerr += READ_XLPORT_MAC_CONTROLr(unit, &xlp_mac_ctrl, port);
        XLPORT_MAC_CONTROLr_XMAC0_RESETf_SET(xlp_mac_ctrl, 1);
        ioerr += WRITE_XLPORT_MAC_CONTROLr(unit, xlp_mac_ctrl, port);

        BMD_SYS_USLEEP(10);

        ioerr += READ_XLPORT_MAC_CONTROLr(unit, &xlp_mac_ctrl, port);
        XLPORT_MAC_CONTROLr_XMAC0_RESETf_SET(xlp_mac_ctrl, 0);
        ioerr += WRITE_XLPORT_MAC_CONTROLr(unit, xlp_mac_ctrl, port);
    }
    _mac_xl_init(unit, port);
}

#if BMD_CONFIG_INCLUDE_PHY == 1
static int
_firmware_helper(void *ctx, uint32_t offset, uint32_t size, void *data)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int unit, port;
    const char *drv_name;
    XLPORT_WC_UCMEM_CTRLr_t xl_ucmem_ctrl;
    XLPORT_WC_UCMEM_DATAm_t xl_ucmem_data;
    CLPORT_WC_UCMEM_CTRLr_t cl_ucmem_ctrl;
    CLPORT_WC_UCMEM_DATAm_t cl_ucmem_data;
    GPORT_WC_UCMEM_CTRLr_t g_ucmem_ctrl;
    GPORT_WC_UCMEM_DATAm_t g_ucmem_data;
    uint32_t wbuf[4];
    uint32_t wdata;
    uint32_t *fw_data;
    uint32_t *fw_entry;
    uint32_t fw_size;
    uint32_t idx, wdx;
    uint32_t be_host;

    /* Get unit, port and driver name from context */
    bmd_phy_fw_info_get(ctx, &unit, &port, &drv_name);

    /* Check if PHY driver requests optimized MDC clock */
    if (data == NULL) {
        CMIC_RATE_ADJUSTr_t rate_adjust;
        uint32_t val = 1;

        /* Offset value is MDC clock in kHz (or zero for default) */
        if (offset) {
            val = offset / 1500;
        }
        ioerr += READ_CMIC_RATE_ADJUSTr(unit, &rate_adjust);
        CMIC_RATE_ADJUSTr_DIVIDENDf_SET(rate_adjust, val);
        ioerr += WRITE_CMIC_RATE_ADJUSTr(unit, rate_adjust);

        return ioerr ? CDK_E_IO : CDK_E_NONE;
    }

    if (size == 0) {
        return CDK_E_INTERNAL;
    }

    /* Aligned firmware size */
    fw_size = (size + FW_ALIGN_MASK) & ~FW_ALIGN_MASK;

    /* We need to byte swap on big endian host */
    be_host = 1;
    if (*((uint8_t *)&be_host) == 1) {
        be_host = 0;
    }

    if (CDK_STRSTR(drv_name, "tscf")) {
        /* Enable parallel bus access */
        ioerr += READ_CLPORT_WC_UCMEM_CTRLr(unit, &cl_ucmem_ctrl, port);
        CLPORT_WC_UCMEM_CTRLr_ACCESS_MODEf_SET(cl_ucmem_ctrl, 1);
        ioerr += WRITE_CLPORT_WC_UCMEM_CTRLr(unit, cl_ucmem_ctrl, port);

        /* DMA buffer needs 32-bit words in little endian order */
        fw_data = (uint32_t *)data;
        for (idx = 0; idx < fw_size; idx += 16) {
            if (idx + 15 < size) {
                fw_entry = &fw_data[idx >> 2];
            } else {
                /* Use staging buffer for modulo bytes */
                CDK_MEMSET(wbuf, 0, sizeof(wbuf));
                CDK_MEMCPY(wbuf, &fw_data[idx >> 2], 16 - (fw_size - size));
                fw_entry = wbuf;
            }
            for (wdx = 0; wdx < 4; wdx++) {
                wdata = fw_entry[wdx];
                if (be_host) {
                    wdata = cdk_util_swap32(wdata);
                }
                CLPORT_WC_UCMEM_DATAm_SET(cl_ucmem_data, wdx, wdata);
            }
            WRITE_CLPORT_WC_UCMEM_DATAm(unit, (idx >> 4), cl_ucmem_data, port);
        }

        /* Disable parallel bus access */
        CLPORT_WC_UCMEM_CTRLr_ACCESS_MODEf_SET(cl_ucmem_ctrl, 0);
        ioerr += WRITE_CLPORT_WC_UCMEM_CTRLr(unit, cl_ucmem_ctrl, port);
	} else if (CDK_STRSTR(drv_name, "tsce")) {
        /* Enable parallel bus access */
        ioerr += READ_XLPORT_WC_UCMEM_CTRLr(unit, &xl_ucmem_ctrl, port);
        XLPORT_WC_UCMEM_CTRLr_ACCESS_MODEf_SET(xl_ucmem_ctrl, 1);
        ioerr += WRITE_XLPORT_WC_UCMEM_CTRLr(unit, xl_ucmem_ctrl, port);

        /* DMA buffer needs 32-bit words in little endian order */
        fw_data = (uint32_t *)data;
        for (idx = 0; idx < fw_size; idx += 16) {
            if (idx + 15 < size) {
                fw_entry = &fw_data[idx >> 2];
            } else {
                /* Use staging buffer for modulo bytes */
                CDK_MEMSET(wbuf, 0, sizeof(wbuf));
                CDK_MEMCPY(wbuf, &fw_data[idx >> 2], 16 - (fw_size - size));
                fw_entry = wbuf;
            }
            for (wdx = 0; wdx < 4; wdx++) {
                wdata = fw_entry[wdx];
                if (be_host) {
                    wdata = cdk_util_swap32(wdata);
                }
                XLPORT_WC_UCMEM_DATAm_SET(xl_ucmem_data, wdx, wdata);
            }
            WRITE_XLPORT_WC_UCMEM_DATAm(unit, (idx >> 4), xl_ucmem_data, port);
        }

        /* Disable parallel bus access */
        XLPORT_WC_UCMEM_CTRLr_ACCESS_MODEf_SET(xl_ucmem_ctrl, 0);
        ioerr += WRITE_XLPORT_WC_UCMEM_CTRLr(unit, xl_ucmem_ctrl, port);

	} else if (CDK_STRSTR(drv_name, "qtce")) {
        int bidx = BLKIDX(unit, BLKTYPE_PMQ, port);

        /* Enable parallel bus access */
        ioerr += READ_GPORT_WC_UCMEM_CTRLr(unit, bidx, &g_ucmem_ctrl);
        GPORT_WC_UCMEM_CTRLr_ACCESS_MODEf_SET(g_ucmem_ctrl, 1);
        ioerr += WRITE_GPORT_WC_UCMEM_CTRLr(unit, bidx, g_ucmem_ctrl);

        /* DMA buffer needs 32-bit words in little endian order */
        fw_data = (uint32_t *)data;
        for (idx = 0; idx < fw_size; idx += 16) {
            if (idx + 15 < size) {
                fw_entry = &fw_data[idx >> 2];
            } else {
                /* Use staging buffer for modulo bytes */
                CDK_MEMSET(wbuf, 0, sizeof(wbuf));
                CDK_MEMCPY(wbuf, &fw_data[idx >> 2], 16 - (fw_size - size));
                fw_entry = wbuf;
            }
            for (wdx = 0; wdx < 4; wdx++) {
                wdata = fw_entry[wdx];
                if (be_host) {
                    wdata = cdk_util_swap32(wdata);
                }
                GPORT_WC_UCMEM_DATAm_SET(g_ucmem_data, wdx, wdata);
            }
            WRITE_GPORT_WC_UCMEM_DATAm(unit, bidx, (idx >> 4), g_ucmem_data);
        }

        /* Disable parallel bus access */
        GPORT_WC_UCMEM_CTRLr_ACCESS_MODEf_SET(g_ucmem_ctrl, 0);
        ioerr += WRITE_GPORT_WC_UCMEM_CTRLr(unit, bidx, g_ucmem_ctrl);
    }

    return ioerr ? CDK_E_IO : rv;
}
#endif /* BMD_CONFIG_INCLUDE_PHY == 1 */

#define PIPE_INIT_COUNT 50000
static void
soc_pipe_mem_clear(unit)
{
    int i;
    ING_HW_RESET_CONTROL_1r_t ing_hw_reset_control_1;
    ING_HW_RESET_CONTROL_2r_t ing_hw_reset_control_2;

    EGR_HW_RESET_CONTROL_0r_t egr_hw_reset_control_0;
    EGR_HW_RESET_CONTROL_1r_t egr_hw_reset_control_1;

    /*
     * Reset the IPIPE and EPIPE block
     */
    ING_HW_RESET_CONTROL_1r_CLR(ing_hw_reset_control_1);
    WRITE_ING_HW_RESET_CONTROL_1r(unit, ing_hw_reset_control_1);

    /* Set count to # entries in largest IPIPE table, L2_ENTRYm */
    ING_HW_RESET_CONTROL_2r_CLR(ing_hw_reset_control_2);
    ING_HW_RESET_CONTROL_2r_RESET_ALLf_SET(ing_hw_reset_control_2, 1);
    ING_HW_RESET_CONTROL_2r_VALIDf_SET(ing_hw_reset_control_2, 1);
    ING_HW_RESET_CONTROL_2r_COUNTf_SET(ing_hw_reset_control_2, 0x8000);
    WRITE_ING_HW_RESET_CONTROL_2r(unit, ing_hw_reset_control_2);

    EGR_HW_RESET_CONTROL_0r_CLR(egr_hw_reset_control_0);
    WRITE_EGR_HW_RESET_CONTROL_0r(unit, egr_hw_reset_control_0);

    EGR_HW_RESET_CONTROL_1r_CLR(egr_hw_reset_control_1);
    EGR_HW_RESET_CONTROL_1r_RESET_ALLf_SET(egr_hw_reset_control_1, 1);
    EGR_HW_RESET_CONTROL_1r_VALIDf_SET(egr_hw_reset_control_1, 1);
    /* Set count to # entries in largest EPIPE table, EGR_VLANm */
    EGR_HW_RESET_CONTROL_1r_COUNTf_SET(egr_hw_reset_control_1, 0x2000);
    WRITE_EGR_HW_RESET_CONTROL_1r(unit, egr_hw_reset_control_1);

    BMD_SYS_USLEEP(50000);
    /* Wait for IPIPE memory initialization done. */
    i = 0;
    do {
        READ_ING_HW_RESET_CONTROL_2r(unit, &ing_hw_reset_control_2);
        if (ING_HW_RESET_CONTROL_2r_DONEf_GET(ing_hw_reset_control_2)) {
            break;
        }
        i++;
        if (i > PIPE_INIT_COUNT) {
            CDK_PRINTF("unit = %d: ING_HW_RESET timeout  \n", unit);
            break;
        }
        BMD_SYS_USLEEP(100);
    } while(1);

    /* Wait for EPIPE memory initialization done. */
    i = 0;
    do {
        READ_EGR_HW_RESET_CONTROL_1r(unit, &egr_hw_reset_control_1);
        if (EGR_HW_RESET_CONTROL_1r_DONEf_GET(egr_hw_reset_control_1)) {
            break;
        }
        i++;
        if (i > PIPE_INIT_COUNT) {
            CDK_PRINTF("unit = %d: EGR_HW_RESET timeout  \n", unit);
            break;
        }
        BMD_SYS_USLEEP(100);
    } while(1);

    ING_HW_RESET_CONTROL_2r_CLR(ing_hw_reset_control_2);
    WRITE_ING_HW_RESET_CONTROL_2r(unit, ing_hw_reset_control_2);
    EGR_HW_RESET_CONTROL_1r_CLR(egr_hw_reset_control_1);
    WRITE_EGR_HW_RESET_CONTROL_1r(unit, egr_hw_reset_control_1);
}

static void
soc_mib_reset(unit)
{
    cdk_pbmp_t pbmp_all;
    int port, lport;
    XLPORT_MIB_RESETr_t xlport_mib_reset;
    CLPORT_MIB_RESETr_t clport_mib_reset;

    bcm53570_a0_all_pbmp_get(unit, &pbmp_all);

    /* Reset XLPORT and CLPORT MIB counter */
    CDK_PBMP_ITER(pbmp_all, port) {
        lport = P2L(unit, port);

        if (IS_XL(unit, port) && (BLK_PORT(unit, port) == 0)) {
            XLPORT_MIB_RESETr_CLR(xlport_mib_reset);
            XLPORT_MIB_RESETr_CLR_CNTf_SET(xlport_mib_reset, 0xf);
            WRITE_XLPORT_MIB_RESETr(unit, xlport_mib_reset, lport);
            XLPORT_MIB_RESETr_CLR_CNTf_SET(xlport_mib_reset, 0);
            WRITE_XLPORT_MIB_RESETr(unit, xlport_mib_reset, lport);
        }

        if (IS_CL(unit, port) && (BLK_PORT(unit, port) == 0)) {
            CLPORT_MIB_RESETr_CLR(clport_mib_reset);
            CLPORT_MIB_RESETr_CLR_CNTf_SET(clport_mib_reset, 0xf);
            WRITE_CLPORT_MIB_RESETr(unit, clport_mib_reset, lport);
            CLPORT_MIB_RESETr_CLR_CNTf_SET(clport_mib_reset, 0);
            WRITE_CLPORT_MIB_RESETr(unit, clport_mib_reset, lport);
        }
    }
}

static void
soc_init_port_mapping(int unit)
{
    int lport, pport, mmu_port;
    uint32_t pbmp[PBM_PORT_WORDS];
    int idx, shift;

    ING_PHYS_TO_LOGIC_MAPm_t ing_ptl_port_mapping;
    EGR_LOGIC_TO_PHYS_MAPr_t egr_ltp_port_mapping;
    EGR_TDM_PORT_MAPm_t egr_tdm_port_map;
    MMU_PORT_TO_PHY_PORT_MAPPINGr_t mtp_port_mapping;
    MMU_PORT_TO_LOGIC_PORT_MAPPINGr_t mtl_port_mapping;

    /* Ingress physical to logical port mapping */
    for (pport = 0; pport < MAX_PHY_PORTS; pport++) {
        ING_PHYS_TO_LOGIC_MAPm_CLR(ing_ptl_port_mapping);
    if (P2L(unit, pport) == -1) {
        ING_PHYS_TO_LOGIC_MAPm_LOGICAL_PORT_NUMBERf_SET(ing_ptl_port_mapping, 0x7F);
    } else {
        ING_PHYS_TO_LOGIC_MAPm_LOGICAL_PORT_NUMBERf_SET(ing_ptl_port_mapping,
                            P2L(unit, pport));
    }
        WRITE_ING_PHYS_TO_LOGIC_MAPm(unit, pport,ing_ptl_port_mapping);
    }

    /* Egress logical to physical port mapping */
    for (lport = 0; lport < MAX_LOG_PORTS; lport++) {
        EGR_LOGIC_TO_PHYS_MAPr_CLR(egr_ltp_port_mapping);
        EGR_LOGIC_TO_PHYS_MAPr_PHYSICAL_PORT_NUMBERf_SET(egr_ltp_port_mapping,
                         ((L2P(unit, lport)== -1) ? 0x7F : (L2P(unit, lport))));
        WRITE_EGR_LOGIC_TO_PHYS_MAPr(unit, lport, egr_ltp_port_mapping);
    }

    /* EGR_TDM_PORT_MAPm */
    EGR_TDM_PORT_MAPm_CLR(egr_tdm_port_map);

    for (idx = 0; idx < PBM_PORT_WORDS; idx++) {
        pbmp[idx] = 0;
    }
    for (lport = 0; lport < MAX_LOG_PORTS; lport++) {
        pport = L2P(unit, lport);
        /*
         * Physical port of loopback port(1) is assigned -1.
         * The port should be programmed on physical port 1.
         */
        if (lport == 1) {
            pport = 1;
        }
        if (pport < 0) {
            continue;
        }
        idx = pport / 32;
        shift = pport - idx * 32;
        pbmp[idx] |= 1 << shift;
    }
    for (idx = 0; idx < PBM_PORT_WORDS; idx++) {
        EGR_TDM_PORT_MAPm_SET(egr_tdm_port_map, idx, pbmp[idx]);
        WRITE_EGR_TDM_PORT_MAPm(unit, idx, egr_tdm_port_map);
    }

    /* MMU to physical port mapping and MMU to logical port mapping */
    for (mmu_port = 0; mmu_port < MAX_MMU_PORTS; mmu_port++) {
        /* MMU to physical port */
        pport = M2P(unit, mmu_port);
        /* Physical to logical port */
        lport = P2L(unit, pport);

        if (pport == 1) {
            /* skip loopback port */
            continue;
        }

        if (pport == -1) {
            /* skip not mapped mmu port */
            continue;
        }

        MMU_PORT_TO_PHY_PORT_MAPPINGr_CLR(mtp_port_mapping);
        MMU_PORT_TO_PHY_PORT_MAPPINGr_PHY_PORTf_SET(mtp_port_mapping, pport);
        WRITE_MMU_PORT_TO_PHY_PORT_MAPPINGr(unit, mmu_port, mtp_port_mapping);

        if (lport == -1) {
            lport = 1;
        }

        MMU_PORT_TO_LOGIC_PORT_MAPPINGr_CLR(mtl_port_mapping);
        MMU_PORT_TO_LOGIC_PORT_MAPPINGr_LOGIC_PORTf_SET(mtl_port_mapping, lport);
        WRITE_MMU_PORT_TO_LOGIC_PORT_MAPPINGr(unit, mmu_port, mtl_port_mapping);
    }
}

static void
_misc_init(int unit)
{
    int ioerr = 0;
    cdk_pbmp_t gxpbmp, xlpbmp, clpbmp, pbmp_all;
    int port, lport;
    int blk_gcfg[7] = {2, 10, 18, 26, 34, 42, 50};
    GPORT_CONFIGr_t gport_config;
    XLPORT_SOFT_RESETr_t xlport_sreset;
    XLPORT_MODE_REGr_t xlport_mode;
    XLPORT_ENABLE_REGr_t xlport_enable;
    CLPORT_SOFT_RESETr_t clport_sreset;
    CLPORT_MODE_REGr_t clport_mode;
    CLPORT_ENABLE_REGr_t clport_enable;
    MISCCONFIGr_t miscconfig;
    L2_AUX_HASH_CONTROLr_t l2_aux_hash_control;
    L3_AUX_HASH_CONTROLr_t l3_aux_hash_control;
    EGR_ENABLEm_t egr_enable;
    ING_CONFIG_64r_t ing_cfg;
    EGR_CONFIG_1r_t egr_cfg1;
    EGR_VLAN_CONTROL_1r_t egr_vlan_control_1;
    EGR_VLAN_CONTROL_1r_t egr_vlan_ctrl1;
    SW2_FP_DST_ACTION_CONTROLr_t sw2_fp_dst_action_control;
    CMIC_RATE_ADJUSTr_t cmic_rate_adjust_ext_mdio;
    CMIC_RATE_ADJUST_INT_MDIOr_t cmic_rate_adjust_int_mdio;
    int i;
    int port_mode;
    uint32_t divisor;

    /* Clear IPIPE/EIPIE Memories */
    soc_pipe_mem_clear(unit);

    /* Clear MIB counter */
    soc_mib_reset(unit);

    soc_init_port_mapping(unit);

    /* GMAC init  */
    bcm53570_a0_all_pbmp_get(unit, &pbmp_all);

    bcm53570_a0_xlport_pbmp_get(unit, &xlpbmp);
    bcm53570_a0_clport_pbmp_get(unit, &clpbmp);
    CDK_PBMP_CLEAR(gxpbmp);
    CDK_PBMP_OR(gxpbmp, xlpbmp);
    CDK_PBMP_OR(gxpbmp, clpbmp);

    GPORT_CONFIGr_CLR(gport_config);
    GPORT_CONFIGr_CLR_CNTf_SET(gport_config, 1);
    GPORT_CONFIGr_GPORT_ENf_SET(gport_config, 1);
    for (i = 0; i < 7; i++) {
        port = blk_gcfg[i];
        /* Clear counter and enable gport */
        WRITE_GPORT_CONFIGr(unit, gport_config, port);
    }

    GPORT_CONFIGr_CLR_CNTf_SET(gport_config, 0);
    for (i = 0; i < 7; i++) {
        port = blk_gcfg[i];
        /* Clear counter and enable gport */
        WRITE_GPORT_CONFIGr(unit, gport_config, port);
    }

    /* XLPORT and CLPORT blocks init */
    CDK_PBMP_ITER(gxpbmp, port) {
        if (IS_XL(unit, port) && (SUBPORT(unit, port) == 0)) {
            XLPORT_SOFT_RESETr_CLR(xlport_sreset);
            for (i = 0; i <= 3; i++) {
                lport = P2L(unit, port + i);
                if (lport == -1) {
                    continue;
                }

                if (i == 0) {
                    XLPORT_SOFT_RESETr_PORT0f_SET(xlport_sreset, 1);
                } else if (i == 1) {
                    XLPORT_SOFT_RESETr_PORT1f_SET(xlport_sreset, 1);
                } else if (i == 2) {
                    XLPORT_SOFT_RESETr_PORT2f_SET(xlport_sreset, 1);
                } else if (i == 3) {
                    XLPORT_SOFT_RESETr_PORT3f_SET(xlport_sreset, 1);
                }
            }
            WRITE_XLPORT_SOFT_RESETr(unit, xlport_sreset, lport);

            XLPORT_MODE_REGr_CLR(xlport_mode);
            if (SUBPORT(unit, port) == 0) {
                port_mode = SOC_PORT_MODE(unit, port);
                if (port_mode == XPORT_MODE_QUAD) {
                    if (P2L(unit, port + 2) >= MIN_LOG_PORTS &&
                        SOC_PORT_MODE(unit, port + 2) == XPORT_MODE_DUAL) {
                        port_mode = XPORT_MODE_TRI_012;
                    }
                 } else if (port_mode == XPORT_MODE_DUAL) {
                    if (P2L(unit, port + 2) >= MIN_LOG_PORTS &&
                        SOC_PORT_MODE(unit, port + 2) == XPORT_MODE_QUAD) {
                        port_mode = XPORT_MODE_TRI_023;
                    }
                }
                XLPORT_MODE_REGr_XPORT0_PHY_PORT_MODEf_SET(xlport_mode, port_mode);
                XLPORT_MODE_REGr_XPORT0_CORE_PORT_MODEf_SET(xlport_mode, port_mode);
                WRITE_XLPORT_MODE_REGr(unit, xlport_mode, port);
            }


            /* De-assert XLPORT soft reset */
            XLPORT_SOFT_RESETr_CLR(xlport_sreset);
            WRITE_XLPORT_SOFT_RESETr(unit, xlport_sreset, port);

            /* Enable XLPORT */
            XLPORT_ENABLE_REGr_CLR(xlport_enable);
            for (i = 0; i <= 3; i++) {
                if (P2L(unit, port + i) == -1) {
                    continue;
                }

                if (i == 0) {
                    XLPORT_ENABLE_REGr_PORT0f_SET(xlport_enable, 1);
                } else if (i == 1) {
                    XLPORT_ENABLE_REGr_PORT1f_SET(xlport_enable, 1);
                } else if (i == 2) {
                    XLPORT_ENABLE_REGr_PORT2f_SET(xlport_enable, 1);
                } else if (i == 3) {
                    XLPORT_ENABLE_REGr_PORT3f_SET(xlport_enable, 1);
                }
            }
            WRITE_XLPORT_ENABLE_REGr(unit, xlport_enable, port);
        } else if (IS_CL(unit, port) && (SUBPORT(unit, port) == 0)) {
            CLPORT_SOFT_RESETr_CLR(clport_sreset);
            for (i = 0; i <= 3; i++) {
                if (P2L(unit, port + i) == -1) {
                    continue;
                }

                if (i == 0) {
                    CLPORT_SOFT_RESETr_PORT0f_SET(clport_sreset, 1);
                } else if (i == 1) {
                    CLPORT_SOFT_RESETr_PORT1f_SET(clport_sreset, 1);
                } else if (i == 2) {
                    CLPORT_SOFT_RESETr_PORT2f_SET(clport_sreset, 1);
                } else if (i == 3) {
                    CLPORT_SOFT_RESETr_PORT3f_SET(clport_sreset, 1);
                }
            }
            WRITE_CLPORT_SOFT_RESETr(unit, clport_sreset, port);

            CLPORT_MODE_REGr_CLR(clport_mode);
            if (SUBPORT(unit, port) == 0) {
                port_mode = SOC_PORT_MODE(unit, port);
                if (port_mode == XPORT_MODE_QUAD) {
                    if (P2L(unit, port + 2) >= MIN_LOG_PORTS &&
                        SOC_PORT_MODE(unit, port + 2) == XPORT_MODE_DUAL) {
                        port_mode = XPORT_MODE_TRI_012;
                    }
                 } else if (port_mode == XPORT_MODE_DUAL) {
                    if (P2L(unit, port + 2) >= MIN_LOG_PORTS &&
                        SOC_PORT_MODE(unit, port + 2) == XPORT_MODE_QUAD) {
                        port_mode = XPORT_MODE_TRI_023;
                    }
                }
                CLPORT_MODE_REGr_XPORT0_PHY_PORT_MODEf_SET(clport_mode, port_mode);
                CLPORT_MODE_REGr_XPORT0_CORE_PORT_MODEf_SET(clport_mode, port_mode);
                WRITE_CLPORT_MODE_REGr(unit, clport_mode, port);
            }


            /* De-assert XLPORT soft reset */
            CLPORT_SOFT_RESETr_CLR(clport_sreset);
            WRITE_CLPORT_SOFT_RESETr(unit, clport_sreset, port);

            /* Enable XLPORT */
            CLPORT_ENABLE_REGr_CLR(clport_enable);
            for (i = 0; i <= 3; i++) {
                if (P2L(unit, port + i) == -1) {
                    continue;
                }

                if (i == 0) {
                    CLPORT_ENABLE_REGr_PORT0f_SET(clport_enable, 1);
                } else if (i == 1) {
                    CLPORT_ENABLE_REGr_PORT1f_SET(clport_enable, 1);
                } else if (i == 2) {
                    CLPORT_ENABLE_REGr_PORT2f_SET(clport_enable, 1);
                } else if (i == 3) {
                    CLPORT_ENABLE_REGr_PORT3f_SET(clport_enable, 1);
                }
            }
            WRITE_CLPORT_ENABLE_REGr(unit, clport_enable, port);
        }
    }

    READ_MISCCONFIGr(unit, &miscconfig);
    MISCCONFIGr_METERING_CLK_ENf_SET(miscconfig, 1);
    WRITE_MISCCONFIGr(unit, miscconfig);

    /* Enable dual hash on L2 and L3 tables */
    L2_AUX_HASH_CONTROLr_CLR(l2_aux_hash_control);
    L2_AUX_HASH_CONTROLr_ENABLEf_SET(l2_aux_hash_control, 1);
    L2_AUX_HASH_CONTROLr_HASH_SELECTf_SET(l2_aux_hash_control, 2);
    L2_AUX_HASH_CONTROLr_INSERT_LEAST_FULL_HALFf_SET(l2_aux_hash_control, 1);
    WRITE_L2_AUX_HASH_CONTROLr(unit, l2_aux_hash_control);

    L3_AUX_HASH_CONTROLr_CLR(l3_aux_hash_control);
    L3_AUX_HASH_CONTROLr_ENABLEf_SET(l3_aux_hash_control, 1);
    L3_AUX_HASH_CONTROLr_HASH_SELECTf_SET(l3_aux_hash_control, 2);
    L3_AUX_HASH_CONTROLr_INSERT_LEAST_FULL_HALFf_SET(l3_aux_hash_control, 1);
    WRITE_L3_AUX_HASH_CONTROLr(unit, l3_aux_hash_control);

    /*
     * Egress Enable
     */
    EGR_ENABLEm_CLR(egr_enable);
    EGR_ENABLEm_PRT_ENABLEf_SET(egr_enable, 1);
    CDK_PBMP_ITER(pbmp_all, port) {
        WRITE_EGR_ENABLEm(unit, port, egr_enable);
    }

    ioerr += READ_ING_CONFIG_64r(unit, &ing_cfg);
    ING_CONFIG_64r_L3SRC_HIT_ENABLEf_SET(ing_cfg, 1);
    ING_CONFIG_64r_L2DST_HIT_ENABLEf_SET(ing_cfg, 1);
    ING_CONFIG_64r_APPLY_EGR_MASK_ON_L2f_SET(ing_cfg, 1);
    ING_CONFIG_64r_APPLY_EGR_MASK_ON_L3f_SET(ing_cfg, 1);
    /* Enable both ARP & RARP */
    ING_CONFIG_64r_ARP_RARP_TO_FPf_SET(ing_cfg, 0x3);
    ING_CONFIG_64r_ARP_VALIDATION_ENf_SET(ing_cfg, 1);
    ING_CONFIG_64r_IGNORE_HG_HDR_LAG_FAILOVERf_SET(ing_cfg, 1);
    ioerr += WRITE_ING_CONFIG_64r(unit, ing_cfg);

    ioerr += READ_EGR_CONFIG_1r(unit, &egr_cfg1);
    EGR_CONFIG_1r_RING_MODEf_SET(egr_cfg1, 1);
    ioerr += WRITE_EGR_CONFIG_1r(unit, egr_cfg1);

    /* The HW defaults for EGR_VLAN_CONTROL_1.VT_MISS_UNTAG == 1, which
     * causes the outer tag to be removed from packets that don't have
     * a hit in the egress vlan tranlation table. Set to 0 to disable this.
     */
    EGR_VLAN_CONTROL_1r_CLR(egr_vlan_control_1);
    CDK_PBMP_ITER(pbmp_all, port) {
        WRITE_EGR_VLAN_CONTROL_1r(unit, P2L(unit, port), egr_vlan_control_1);
    }

    CDK_PBMP_ITER(pbmp_all, port) {
        ioerr += READ_EGR_VLAN_CONTROL_1r(unit, P2L(unit, port), &egr_vlan_ctrl1);
        EGR_VLAN_CONTROL_1r_VT_MISS_UNTAGf_SET(egr_vlan_ctrl1, 0);
        EGR_VLAN_CONTROL_1r_REMARK_OUTER_DOT1Pf_SET(egr_vlan_ctrl1, 1);
        ioerr += WRITE_EGR_VLAN_CONTROL_1r(unit, P2L(unit, port), egr_vlan_ctrl1);
    }

    /* Setup SW2_FP_DST_ACTION_CONTROL */
    SW2_FP_DST_ACTION_CONTROLr_CLR(sw2_fp_dst_action_control);
    SW2_FP_DST_ACTION_CONTROLr_SRC_REMOVAL_ENf_SET(sw2_fp_dst_action_control, 1);
    SW2_FP_DST_ACTION_CONTROLr_LAG_RES_ENf_SET(sw2_fp_dst_action_control, 1);
    WRITE_SW2_FP_DST_ACTION_CONTROLr(unit, sw2_fp_dst_action_control);

    divisor = (FREQ(unit) + 19) / 20;
    /*
     * Set reference clock (based on 200MHz core clock)
     * to be 200MHz * (1/40) = 5MHz
     */
    CMIC_RATE_ADJUSTr_CLR(cmic_rate_adjust_ext_mdio);
    CMIC_RATE_ADJUSTr_DIVISORf_SET(cmic_rate_adjust_ext_mdio, divisor);
    CMIC_RATE_ADJUSTr_DIVIDENDf_SET(cmic_rate_adjust_ext_mdio, 0x1);
    WRITE_CMIC_RATE_ADJUSTr(unit, cmic_rate_adjust_ext_mdio);

    /* Match the Internal MDC freq with above for External MDC */
    CMIC_RATE_ADJUST_INT_MDIOr_CLR(cmic_rate_adjust_int_mdio);
    CMIC_RATE_ADJUST_INT_MDIOr_DIVISORf_SET(cmic_rate_adjust_int_mdio, divisor);
    CMIC_RATE_ADJUST_INT_MDIOr_DIVIDENDf_SET(cmic_rate_adjust_int_mdio, 0x1);
    WRITE_CMIC_RATE_ADJUST_INT_MDIOr(unit, cmic_rate_adjust_int_mdio);
}

int
bcm53570_a0_bmd_init(int unit)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int port;
    cdk_pbmp_t xlport_pbmp, pbmp_all, clport_pbmp, gport_pbmp;
    XLPORT_MAC_RSV_MASKr_t xlmac_mac_rsv_mask;
    CLPORT_MAC_RSV_MASKr_t clmac_mac_rsv_mask;
    GPORT_RSV_MASKr_t gmac_rsv_mask;
    VLAN_PROFILE_TABm_t vlan_profile;
    ING_VLAN_TAG_ACTION_PROFILEm_t vlan_action;
    EGR_VLAN_TAG_ACTION_PROFILEm_t egr_action;
#if BMD_CONFIG_INCLUDE_PHY == 1
        int speed_max;
        uint32_t ability= 0x0;
        int lanes;
#endif
    BMD_CHECK_UNIT(unit);

    _misc_init(unit);

    _mmu_init(unit);

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

    bcm53570_a0_all_pbmp_get(unit, &pbmp_all);
    bcm53570_a0_clport_pbmp_get(unit, &clport_pbmp);
    bcm53570_a0_xlport_pbmp_get(unit, &xlport_pbmp);
    bcm53570_a0_gport_pbmp_get(unit, &gport_pbmp);

    CDK_PBMP_ITER(pbmp_all, port) {
        if (CDK_PBMP_MEMBER(xlport_pbmp, port)) {
            ioerr += READ_XLPORT_MAC_RSV_MASKr(unit, port, &xlmac_mac_rsv_mask);
            XLPORT_MAC_RSV_MASKr_SET(xlmac_mac_rsv_mask, 0x58);
            ioerr += WRITE_XLPORT_MAC_RSV_MASKr(unit, port, xlmac_mac_rsv_mask);
        } else if (CDK_PBMP_MEMBER(clport_pbmp, port)) {
            ioerr += READ_CLPORT_MAC_RSV_MASKr(unit, port, &clmac_mac_rsv_mask);
            CLPORT_MAC_RSV_MASKr_SET(clmac_mac_rsv_mask, 0x58);
            ioerr += WRITE_CLPORT_MAC_RSV_MASKr(unit, port, clmac_mac_rsv_mask);
        } else {
            ioerr += READ_GPORT_RSV_MASKr(unit, &gmac_rsv_mask, port);
            GPORT_RSV_MASKr_SET(gmac_rsv_mask, 0x58);
            ioerr += WRITE_GPORT_RSV_MASKr(unit, gmac_rsv_mask, port);
        }
    }

    CDK_PBMP_ITER(gport_pbmp, port) {
#if BMD_CONFIG_INCLUDE_PHY == 1
        if (CDK_SUCCESS(rv)) {
            rv = bmd_phy_probe(unit, port);
        }
        lanes = bcm53570_a0_port_num_lanes(unit, port);

        if (IS_TSCQ(port)) {
            if (CDK_SUCCESS(rv)) {
                if (lanes == 4) {
                    rv = bmd_phy_mode_set(unit, port, "qtce",
                                                BMD_PHY_MODE_SERDES, 0);
                } else if (lanes == 2) {
                    rv = bmd_phy_mode_set(unit, port, "qtce",
                                                BMD_PHY_MODE_SERDES, 1);
                    rv = bmd_phy_mode_set(unit, port, "qtce",
                                                BMD_PHY_MODE_2LANE, 1);
                } else { /* lanes = 1 */
                    rv = bmd_phy_mode_set(unit, port, "qtce",
                                                BMD_PHY_MODE_SERDES, 1);
                    rv = bmd_phy_mode_set(unit, port, "qtce",
                                                BMD_PHY_MODE_2LANE, 0);
                }
            }
            speed_max = bcm53570_a0_port_speed_max(unit, port);
            ability = (BMD_PHY_ABIL_10MB_FD | BMD_PHY_ABIL_100MB_FD);
            if (speed_max == 1000) {
                ability |= BMD_PHY_ABIL_10MB_FD;
            } else if (speed_max == 2500) {
                ability |= BMD_PHY_ABIL_2500MB;
            }
            if (CDK_SUCCESS(rv)) {
                rv = bmd_phy_default_ability_set(unit, port, ability);
            }
            if (CDK_SUCCESS(rv)) {
                rv = bmd_phy_fw_helper_set(unit, port, _firmware_helper);
            }

            if (CDK_SUCCESS(rv)) {
                rv = bmd_phy_init(unit, port);
            }
        } else {
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

            /* viper set the ability in glu layer */
            if (CDK_SUCCESS(rv)) {
                rv = bmd_phy_fw_helper_set(unit, port, _firmware_helper);
            }

            if (CDK_SUCCESS(rv)) {
                rv = bmd_phy_init(unit, port);
            }
        }
#endif /* BMD_CONFIG_INCLUDE_PHY == 1 */

        _gport_init(unit, port);
    } /* end of SGMII PHY init */

    CDK_PBMP_ITER(xlport_pbmp, port) {
#if BMD_CONFIG_INCLUDE_PHY == 1
        if (CDK_SUCCESS(rv)) {
            rv = bmd_phy_probe(unit, port);
        }

        lanes = bcm53570_a0_port_num_lanes(unit, port);
        if (CDK_SUCCESS(rv)) {
            if (lanes == 4) {
                rv = bmd_phy_mode_set(unit, port, "tsce_dpll",
                                            BMD_PHY_MODE_SERDES, 0);
            } else if (lanes == 2) {
                rv = bmd_phy_mode_set(unit, port, "tsce_dpll",
                                            BMD_PHY_MODE_SERDES, 1);
                rv = bmd_phy_mode_set(unit, port, "tsce_dpll",
                                            BMD_PHY_MODE_2LANE, 1);
            } else { /* lanes = 1 */
                rv = bmd_phy_mode_set(unit, port, "tsce_dpll",
                                            BMD_PHY_MODE_SERDES, 1);
                rv = bmd_phy_mode_set(unit, port, "tsce_dpll",
                                            BMD_PHY_MODE_2LANE, 0);
            }
        }

        if (CDK_SUCCESS(rv)) {
            rv = bmd_phy_fw_helper_set(unit, port, _firmware_helper);
        }

        if (CDK_SUCCESS(rv)) {
            rv = bmd_phy_init(unit, port);
        }
#endif /* BMD_CONFIG_INCLUDE_PHY == 1 */

        _xlport_init(unit, port);
    } /* end of TSCQ/TSCE PHY init */

    CDK_PBMP_ITER(clport_pbmp, port) {
#if BMD_CONFIG_INCLUDE_PHY == 1
        if (CDK_SUCCESS(rv)) {
            rv = bmd_phy_probe(unit, port);
        }
        lanes = bcm53570_a0_port_num_lanes(unit, port);
        if (CDK_SUCCESS(rv)) {
            if (lanes == 4) {
                rv = bmd_phy_mode_set(unit, port, "tscf",
                                            BMD_PHY_MODE_SERDES, 0);
            } else if (lanes == 2) {
                rv = bmd_phy_mode_set(unit, port, "tscf",
                                            BMD_PHY_MODE_SERDES, 1);
                rv = bmd_phy_mode_set(unit, port, "tscf",
                                            BMD_PHY_MODE_2LANE, 1);
            } else { /* lanes = 1 */
                rv = bmd_phy_mode_set(unit, port, "tscf",
                                            BMD_PHY_MODE_SERDES, 1);
                rv = bmd_phy_mode_set(unit, port, "tscf",
                                            BMD_PHY_MODE_2LANE, 0);
            }
        }

        if (CDK_SUCCESS(rv)) {
            rv = bmd_phy_fw_helper_set(unit, port, _firmware_helper);
        }
        if (CDK_SUCCESS(rv)) {
            rv = bmd_phy_init(unit, port);
        }
#endif /* BMD_CONFIG_INCLUDE_PHY == 1 */

        _clport_init(unit, port);
    } /* end of TSCF PHY init */

    /* Common port initialization for CPU port */
    ioerr += _port_init(unit, CMIC_PORT);

#if BMD_CONFIG_INCLUDE_DMA
    if (CDK_SUCCESS(rv)) {
        rv = bmd_xgsd_dma_init(unit);
    }

    if (CDK_DEV_FLAGS(unit) & CDK_DEV_MBUS_PCI) {
        int idx;
        CMIC_CMC_HOSTMEM_ADDR_REMAPr_t hostmem_remap;
        uint32_t remap_val[] = { 0x144d2450, 0x19617595, 0x1e75c6da, 0x1f };

        /* Send DMA data to external host memory when on PCI bus */
        for (idx = 0; idx < COUNTOF(remap_val); idx++) {
            CMIC_CMC_HOSTMEM_ADDR_REMAPr_SET(hostmem_remap, remap_val[idx]);
            ioerr += WRITE_CMIC_CMC_HOSTMEM_ADDR_REMAPr(unit, idx, hostmem_remap);
        }
    }
#endif /* BMD_CONFIG_INCLUDE_DMA */

    return ioerr ? CDK_E_IO : rv;
}
#endif /* CDK_CONFIG_INCLUDE_BCM53570_A0 */