/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56450_A0 == 1

#include <bmd/bmd.h>
#include <bmd/bmd_device.h>
#include <cdk/arch/xgsm_chip.h>
#include <cdk/cdk_debug.h>
#include <cdk/chip/bcm56450_a0_defs.h>
#include <bmd/bmd_phy_ctrl.h>
#include <bmdi/arch/xgsm_dma.h>
#include "bcm56450_a0_bmd.h"
#include "bcm56450_a0_internal.h"

#define PIPE_RESET_TIMEOUT_MSEC         50
#define LLS_RESET_TIMEOUT_MSEC          50

#define NUM_RQEQ_COS                    12
#define NUM_RQEI_COS                    12
#define NUM_RQEI_COS                    12
#define NUM_RDEQ_COS                    16
#define NUM_RDEI_COS                    8

STATIC int
_mmu_tdm_init(int unit)
{
    int ioerr = 0;
    const int *default_tdm_seq;
    int tdm_seq[108];
    int idx, tdm_size;
    IARB_TDM_CONTROLr_t iarb_tdm_ctrl;
    IARB_TDM_TABLEm_t iarb_tdm;
    LLS_PORT_TDMm_t  lls_tdm;
    LLS_TDM_CAL_CFGr_t cal_cfg;
    
    /* Get default TDM sequence for this configuration */
    tdm_size = bcm56450_a0_tdm_default(unit, &default_tdm_seq);
    if (tdm_size <= 0 || tdm_size > COUNTOF(tdm_seq)) {
        return CDK_E_INTERNAL;
    }

    /* Make local copy of TDM sequence */
    for (idx = 0; idx < tdm_size; idx++) {
        tdm_seq[idx] = default_tdm_seq[idx];
    }

    /* Disable IARB TDM before programming */
    ioerr += (READ_IARB_TDM_CONTROLr(unit, &iarb_tdm_ctrl));
    IARB_TDM_CONTROLr_DISABLEf_SET(iarb_tdm_ctrl, 1);
    IARB_TDM_CONTROLr_TDM_WRAP_PTRf_SET(iarb_tdm_ctrl, tdm_size -1);
    ioerr += (WRITE_IARB_TDM_CONTROLr(unit, iarb_tdm_ctrl));

    for (idx = 0; idx < tdm_size; idx++) {
        IARB_TDM_TABLEm_CLR(iarb_tdm);
        IARB_TDM_TABLEm_PORT_NUMf_SET(iarb_tdm, tdm_seq[idx]);
        ioerr += (WRITE_IARB_TDM_TABLEm(unit, idx, iarb_tdm));

        if ((idx % 2) == 0) {
            /* Two entries per mem entry */
            LLS_PORT_TDMm_CLR(lls_tdm);
            LLS_PORT_TDMm_PORT_ID_0f_SET(lls_tdm, tdm_seq[idx]);
            LLS_PORT_TDMm_PORT_ID_0_ENABLEf_SET(lls_tdm, 
                                                ((tdm_seq[idx] <= 41) ? 1 : 0));
        } else {
            LLS_PORT_TDMm_PORT_ID_1f_SET(lls_tdm, tdm_seq[idx]);
            LLS_PORT_TDMm_PORT_ID_1_ENABLEf_SET(lls_tdm, 
                                                ((tdm_seq[idx] <= 41) ? 1 : 0));
            ioerr += (WRITE_LLS_PORT_TDMm(unit, (idx / 2), lls_tdm));
            ioerr += (WRITE_LLS_PORT_TDMm(unit, 128 + (idx / 2), lls_tdm));
        }
    }
    if (tdm_size % 2) {
        ioerr += (WRITE_LLS_PORT_TDMm(unit, (idx / 2), lls_tdm));
        ioerr += (WRITE_LLS_PORT_TDMm(unit, 128 + (idx / 2), lls_tdm));
    }

    ioerr += (READ_IARB_TDM_CONTROLr(unit, &iarb_tdm_ctrl));
    IARB_TDM_CONTROLr_DISABLEf_SET(iarb_tdm_ctrl, 0);
    IARB_TDM_CONTROLr_TDM_WRAP_PTRf_SET(iarb_tdm_ctrl, tdm_size - 1);
    ioerr += (WRITE_IARB_TDM_CONTROLr(unit, iarb_tdm_ctrl));

    LLS_TDM_CAL_CFGr_CLR(cal_cfg);
    LLS_TDM_CAL_CFGr_END_Af_SET(cal_cfg, tdm_size - 1);
    LLS_TDM_CAL_CFGr_END_Bf_SET(cal_cfg, tdm_size - 1);
    LLS_TDM_CAL_CFGr_ENABLEf_SET(cal_cfg, 1);
    ioerr += (WRITE_LLS_TDM_CAL_CFGr(unit, cal_cfg));
    
    return ioerr;
}

static int
_mmu_init(int unit)
{
    int ioerr = 0;
    int port, ppport, mport, lport;
    int blkidx, subidx, idx = 0;
    int nxtaddr = 0;
    int speed_max;
    int op_node, ceil, q_offset, q_limit;
    cdk_pbmp_t pbmp, mmu_pbmp;
    LLS_SOFT_RESETr_t soft_reset;
    LLS_INITr_t lls_init;
    TOQ_EXT_MEM_BW_MAP_TABLEr_t toq_ext_mem; 
    DEQ_EFIFO_CFGr_t deq_efifo;
    TOQ_PORT_BW_CTRLr_t toq_port_bw_ctrl;
    DEQ_EFIFO_CFG_COMPLETEr_t eq_efifo_cfg;
    LLS_CONFIG0r_t lls_config;
    LLS_MAX_REFRESH_ENABLEr_t lls_max_refresh;
    LLS_MIN_REFRESH_ENABLEr_t lls_min_refresh;
    LLS_PORT_CONFIGm_t lls_port_cfg;
    LLS_L0_PARENTm_t l0_parent;
    LLS_L1_PARENTm_t l1_parent;
    LLS_L0_CONFIGm_t l0_config;
    LLS_L1_CONFIGm_t l1_config;
    LLS_L2_PARENTm_t l2_parent;
    EGR_QUEUE_TO_PP_PORT_MAPm_t egr_queue_to_pp_port;
    ING_COS_MODEr_t cos_mode;
    RQE_PP_PORT_CONFIGr_t rqe_port;
    RQE_SCHEDULER_CONFIGr_t rqe_scheduler_cfg;
    RQE_SCHEDULER_WEIGHT_L0_QUEUEr_t rqe_scheduler_weight_l0;
    RQE_SCHEDULER_WEIGHT_L1_QUEUEr_t rqe_scheduler_weight_l1;
    INPUT_PORT_RX_ENABLE_64r_t port_rx_enable;
    THDIEXT_INPUT_PORT_RX_ENABLE_64r_t iext_rx_enable;
    THDIEMA_INPUT_PORT_RX_ENABLE_64r_t iema_rx_enable;
    THDIRQE_INPUT_PORT_RX_ENABLE_64r_t irqe_rx_enable;
    THDIQEN_INPUT_PORT_RX_ENABLE_64r_t iqen_rx_enable;
    PORT_MAX_PKT_SIZEr_t port_max;
    THDIEXT_PORT_MAX_PKT_SIZEr_t iext_port_max; 
    THDIEMA_PORT_MAX_PKT_SIZEr_t iema_port_max;
    THDIRQE_PORT_MAX_PKT_SIZEr_t irqe_port_max;
    THDIQEN_PORT_MAX_PKT_SIZEr_t iqen_port_max;
    PORT_PRI_XON_ENABLEr_t port_pri_xon;
    THDIEXT_PORT_PRI_XON_ENABLEr_t iext_pri_xon; 
    THDIEMA_PORT_PRI_XON_ENABLEr_t iema_pri_xon;
    THDIRQE_PORT_PRI_XON_ENABLEr_t irqe_pri_xon;
    THDIQEN_PORT_PRI_XON_ENABLEr_t iqen_pri_xon;
    THDI_BYPASSr_t thdi_bpass;
    THDIQEN_THDI_BYPASSr_t iqen_bpass;
    THDIRQE_THDI_BYPASSr_t irqe_bpass;
    THDIEXT_THDI_BYPASSr_t iext_bpass;
    THDIEMA_THDI_BYPASSr_t iema_bpass;
    THDO_BYPASSr_t thdo_bpass;
    PORT_PRI_GRPr_t port_pri_grp;
    THDIEXT_PORT_PRI_GRPr_t iext_pri_grp;
    THDIEMA_PORT_PRI_GRPr_t iema_pri_grp;
    THDIRQE_PORT_PRI_GRPr_t irqe_pri_grp;
    THDIQEN_PORT_PRI_GRPr_t iqen_pri_grp;
    MMU_ENQ_IP_PRI_TO_PG_PROFILE_0r_t pri_to_pg_profile;
    MMU_ENQ_PROFILE_0_PRI_GRPr_t profile_0_pri_gpr;
    MMU_ENQ_PROFILE_1_PRI_GRPr_t profile_1_pri_gpr;
    MMU_ENQ_PROFILE_2_PRI_GRPr_t profile_2_pri_gpr;
    MMU_ENQ_PROFILE_3_PRI_GRPr_t profile_3_pri_gpr;
    BUFFER_CELL_LIMIT_SPr_t buf_cell;
    THDIRQE_BUFFER_CELL_LIMIT_SPr_t irqe_buf_cell;
    THDIQEN_BUFFER_CELL_LIMIT_SPr_t iqen_buf_cell;
    BUFFER_CELL_LIMIT_SP_SHAREDr_t sp_shared;
    THDIEXT_BUFFER_CELL_LIMIT_SP_SHAREDr_t iext_sp_shared;
    THDIEMA_BUFFER_CELL_LIMIT_SP_SHAREDr_t iema_sp_shared;
    THDIRQE_BUFFER_CELL_LIMIT_SP_SHAREDr_t irqe_sp_shared;
    THDIQEN_BUFFER_CELL_LIMIT_SP_SHAREDr_t iqen_sp_shared;
    CELL_RESET_LIMIT_OFFSET_SPr_t cell_reset;    
    THDIRQE_CELL_RESET_LIMIT_OFFSET_SPr_t irqe_cell_reset;    
    THDIQEN_CELL_RESET_LIMIT_OFFSET_SPr_t iqen_cell_reset;    
    GLOBAL_HDRM_LIMITr_t glb_hdrm;
    THDIRQE_GLOBAL_HDRM_LIMITr_t irqe_glb_hdrm;
    THDIQEN_GLOBAL_HDRM_LIMITr_t iqen_glb_hdrm;
    THDO_MISCCONFIGr_t thd0_misc;
    OP_THR_CONFIGr_t op_thr;
    CFAPIFULLSETPOINTr_t cfapifullsp;
    CFAPIFULLRESETPOINTr_t cfapifullrp;
    COLOR_AWAREr_t color_aware;
    OP_BUFFER_SHARED_LIMIT_CELLIr_t op_shr_cell;
    OP_BUFFER_SHARED_LIMIT_QENTRYr_t op_shr_qentry;
    OP_BUFFER_LIMIT_YELLOW_QENTRYr_t op_yel_qentry;
    OP_BUFFER_LIMIT_RED_QENTRYr_t op_red_qentry;
    OP_BUFFER_SHARED_LIMIT_THDORQEQr_t op_shr_rqeq;
    OP_BUFFER_SHARED_LIMIT_RESUME_CELLIr_t op_sh_res_cell;
    OP_BUFFER_LIMIT_YELLOW_CELLIr_t op_yel_cell;
    OP_BUFFER_LIMIT_RED_CELLIr_t op_red_cell;
    OP_BUFFER_LIMIT_RESUME_YELLOW_CELLIr_t op_res_yel_cell;
    OP_BUFFER_LIMIT_RESUME_RED_CELLIr_t op_res_red_cell;
    OP_BUFFER_SHARED_LIMIT_RESUME_QENTRYr_t op_sh_res_qentry;
    OP_BUFFER_LIMIT_RESUME_YELLOW_QENTRYr_t op_res_yel_qentry;
    OP_BUFFER_LIMIT_RESUME_RED_QENTRYr_t op_res_red_qentry;
    OP_BUFFER_SHARED_LIMIT_THDORDEQr_t op_sh_rdeq;
    OP_BUFFER_LIMIT_YELLOW_THDORDEQr_t op_yel_rdeq;
    OP_BUFFER_LIMIT_RED_THDORDEQr_t op_red_rdeq;
    OP_BUFFER_LIMIT_RESUME_YELLOW_THDORDEQr_t op_res_yel_rdeq;
    OP_BUFFER_LIMIT_RESUME_RED_THDORDEQr_t op_res_red_rdeq;
    OP_BUFFER_SHARED_LIMIT_RESUME_THDORQEQr_t op_sh_res_rqeq;
    OP_BUFFER_LIMIT_YELLOW_THDORQEQr_t op_yel_rqeq;
    OP_BUFFER_LIMIT_RESUME_YELLOW_THDORQEQr_t op_res_yel_rqeq;
    OP_BUFFER_LIMIT_RED_THDORQEQr_t op_red_rqeq;
    OP_BUFFER_LIMIT_RESUME_RED_THDORQEQr_t op_res_red_rqeq;
    OP_BUFFER_SHARED_LIMIT_RESUME_THDORDEQr_t op_sh_res_rdeq;
    OP_QUEUE_CONFIG_THDORQEIr_t opq_rqei;
    OP_QUEUE_CONFIG_THDORQEQr_t opq_rqeq;
    OP_QUEUE_CONFIG1_THDORQEIr_t opq_rqei1;
    OP_QUEUE_CONFIG1_THDORQEQr_t opq_rqeq1;
    OP_QUEUE_RESET_OFFSET_THDORQEIr_t opq_rst_rqei;
    OP_QUEUE_RESET_OFFSET_THDORQEQr_t opq_rst_rqeq;
    OP_QUEUE_LIMIT_YELLOW_THDORQEIr_t opq_yel_rqei;
    OP_QUEUE_LIMIT_RED_THDORQEIr_t opq_red_rqei;
    OP_QUEUE_RESET_OFFSET_YELLOW_THDORQEIr_t opq_rst_yel_rqei;
    OP_QUEUE_RESET_OFFSET_RED_THDORQEIr_t opq_rst_red_rqei;
    OP_QUEUE_RESET_OFFSET_YELLOW_THDORQEQr_t opq_rst_yel_rqeq;
    OP_QUEUE_RESET_OFFSET_RED_THDORQEQr_t opq_rst_red_rqeq;
    OP_QUEUE_LIMIT_YELLOW_THDORQEQr_t opq_yel_rqeq;
    OP_QUEUE_LIMIT_RED_THDORQEQr_t opq_red_rqeq;
    OP_QUEUE_CONFIG1_THDORDEQr_t opq_cfg1_rdeq;
    OP_QUEUE_CONFIG_THDORDEQr_t opq_cfg_rdeq;
    OP_QUEUE_RESET_OFFSET_THDORDEQr_t opq_rst_rdeq;
    OP_QUEUE_LIMIT_YELLOW_THDORDEQr_t opq_yel_rdeq;
    OP_QUEUE_LIMIT_RED_THDORDEQr_t opq_red_rdeq;
    OP_QUEUE_RESET_OFFSET_YELLOW_THDORDEQr_t opq_rst_yel_rdeq;
    OP_QUEUE_RESET_OFFSET_RED_THDORDEQr_t opq_rst_red_rdeq;
    OP_QUEUE_CONFIG1_THDORDEIr_t opq_cfg1_rdei;
    OP_QUEUE_CONFIG_THDORDEIr_t opq_cfg_rdei;
    OP_QUEUE_RESET_OFFSET_THDORDEIr_t opq_rst_rdei;
    OP_QUEUE_LIMIT_YELLOW_THDORDEIr_t opq_yel_rdei;
    OP_QUEUE_LIMIT_RED_THDORDEIr_t opq_red_rdei;
    OP_QUEUE_RESET_OFFSET_YELLOW_THDORDEIr_t opq_rst_yel_rdei;
    OP_QUEUE_RESET_OFFSET_RED_THDORDEIr_t opq_rst_red_rdei;
    THDI_PORT_SP_CONFIGm_t port_sp_cfg;
    THDI_PORT_PG_CONFIGm_t port_pg_cfg;
    THDIEXT_THDI_PORT_PG_CONFIGm_t iext_port_pg_cfg;
    THDIEMA_THDI_PORT_PG_CONFIGm_t iema_port_pg_cfg;
    THDIRQE_THDI_PORT_SP_CONFIGm_t irqe_port_sp_cfg;
    THDIRQE_THDI_PORT_PG_CONFIGm_t irqe_port_pg_cfg;
    THDIQEN_THDI_PORT_SP_CONFIGm_t iqen_port_sp_cfg;
    THDIQEN_THDI_PORT_PG_CONFIGm_t iqen_port_pg_cfg;
    MMU_THDO_QCONFIG_CELLm_t qcfg_cell;  
    MMU_THDO_QOFFSET_CELLm_t qoff_cell;
    MMU_THDO_QCONFIG_QENTRYm_t qcfg_qentry;
    MMU_THDO_QOFFSET_QENTRYm_t qoff_qentry;
    MMU_THDO_OPNCONFIG_CELLm_t opncfg_cell;
    MMU_THDO_OPNCONFIG_QENTRYm_t opncfg_qentry;
    MMU_AGING_LMT_INTm_t age_int;
    MMU_AGING_LMT_EXTm_t age_ext;
    PORT_MAX_PKT_SIZEr_t port_max_pkt_size;
    PORT_PAUSE_ENABLE_64r_t port_pause_enable;
    THDIEXT_PORT_PAUSE_ENABLE_64r_t iext_pause_enable;
    THDIQEN_PORT_PAUSE_ENABLE_64r_t iqen_pause_enable;
    THDIEMA_PORT_PAUSE_ENABLE_64r_t iema_pause_enable;
    
    LLS_SOFT_RESETr_CLR(soft_reset);
    LLS_SOFT_RESETr_SOFT_RESETf_SET(soft_reset, 0);
    ioerr  += WRITE_LLS_SOFT_RESETr(unit, soft_reset);

    LLS_INITr_CLR(lls_init);
    LLS_INITr_INITf_SET(lls_init, 1);
    ioerr  += WRITE_LLS_INITr(unit, lls_init);
    BMD_SYS_USLEEP(50000);

    for (idx = 0; idx < LLS_RESET_TIMEOUT_MSEC; idx++) {
        ioerr += READ_LLS_INITr(unit, &lls_init);
        if (LLS_INITr_INIT_DONEf_GET(lls_init)) {
            break;
        }
        BMD_SYS_USLEEP(1000);
    }
    if (idx >= LLS_RESET_TIMEOUT_MSEC) {
        CDK_WARN(("bcm56450_a0_bmd_init[%d]: LLS INIT timeout\n", unit));
        return ioerr ? CDK_E_IO : CDK_E_TIMEOUT;
    }

    /* Setup TDM for MMU Arb & LLS */
    _mmu_tdm_init(unit);

    for (idx = 0; idx < 16; idx++) {
        ioerr += READ_TOQ_EXT_MEM_BW_MAP_TABLEr(unit, idx, &toq_ext_mem);
        TOQ_EXT_MEM_BW_MAP_TABLEr_GBL_GUARENTEE_BW_LIMITf_SET(toq_ext_mem,1450);
        TOQ_EXT_MEM_BW_MAP_TABLEr_WR_PHASEf_SET(toq_ext_mem, 142);
        TOQ_EXT_MEM_BW_MAP_TABLEr_RD_PHASEf_SET(toq_ext_mem, 138);
        ioerr += WRITE_TOQ_EXT_MEM_BW_MAP_TABLEr(unit, idx, toq_ext_mem);
    }
    
    ioerr += READ_DEQ_EFIFO_CFGr(unit, CMIC_PORT, &deq_efifo);
    DEQ_EFIFO_CFGr_EGRESS_FIFO_START_ADDRESSf_SET(deq_efifo, nxtaddr);
    DEQ_EFIFO_CFGr_EGRESS_FIFO_XMIT_THRESHOLDf_SET(deq_efifo, 0);
    DEQ_EFIFO_CFGr_EGRESS_FIFO_LINK_PHYf_SET(deq_efifo, 0);
    DEQ_EFIFO_CFGr_EGRESS_FIFO_DEPTHf_SET(deq_efifo, 10);
    ioerr += WRITE_DEQ_EFIFO_CFGr(unit, CMIC_PORT, deq_efifo);

    /* Port BW Ctrl */
    ioerr += READ_TOQ_PORT_BW_CTRLr(unit, CMIC_PORT, &toq_port_bw_ctrl);
    TOQ_PORT_BW_CTRLr_PORT_BWf_SET(toq_port_bw_ctrl, 50);
    TOQ_PORT_BW_CTRLr_START_THRESHOLDf_SET(toq_port_bw_ctrl, 127);
    ioerr += WRITE_TOQ_PORT_BW_CTRLr(unit, CMIC_PORT, toq_port_bw_ctrl);
    
    nxtaddr += 20;
    for (blkidx = 0; blkidx < NUM_MXQBLOCKS; blkidx++) {
        for (subidx = 0; subidx < MXQPORTS_PER_BLOCK ; subidx++) {
            if ((port = bcm56450_a0_mxqport_from_index(unit, blkidx, subidx)) <= 0) {
                continue;
            }
            speed_max = bcm56450_a0_port_speed_max(unit, port);
            mport = P2M(unit, port);
            
            ioerr += READ_DEQ_EFIFO_CFGr(unit, mport, &deq_efifo);
            DEQ_EFIFO_CFGr_EGRESS_FIFO_START_ADDRESSf_SET(deq_efifo, nxtaddr);
            DEQ_EFIFO_CFGr_EGRESS_FIFO_XMIT_THRESHOLDf_SET(deq_efifo, 0);
            DEQ_EFIFO_CFGr_EGRESS_FIFO_LINK_PHYf_SET(deq_efifo, 0);
            if (speed_max <= 1000) {
                nxtaddr += 10;
                DEQ_EFIFO_CFGr_EGRESS_FIFO_DEPTHf_SET(deq_efifo, 10);
            } else if (speed_max <= 2500) {
                nxtaddr += 14;
                DEQ_EFIFO_CFGr_EGRESS_FIFO_DEPTHf_SET(deq_efifo, 14);
            } else if (speed_max <= 13000) {
                nxtaddr += 56;
                DEQ_EFIFO_CFGr_EGRESS_FIFO_DEPTHf_SET(deq_efifo, 56);
            } else { /* >= 13G */
                nxtaddr += 112;
                DEQ_EFIFO_CFGr_EGRESS_FIFO_DEPTHf_SET(deq_efifo, 112);
            }
            ioerr += WRITE_DEQ_EFIFO_CFGr(unit, mport, deq_efifo);

            /* Port BW Ctrl */
            ioerr += READ_TOQ_PORT_BW_CTRLr(unit, mport, &toq_port_bw_ctrl);
            if (speed_max >= 10000) {
                TOQ_PORT_BW_CTRLr_PORT_BWf_SET(toq_port_bw_ctrl, 500);
                TOQ_PORT_BW_CTRLr_START_THRESHOLDf_SET(toq_port_bw_ctrl, 34);
            } else if (speed_max == 2500) {
                TOQ_PORT_BW_CTRLr_PORT_BWf_SET(toq_port_bw_ctrl, 125);
                TOQ_PORT_BW_CTRLr_START_THRESHOLDf_SET(toq_port_bw_ctrl, 19);
            } else {
                TOQ_PORT_BW_CTRLr_PORT_BWf_SET(toq_port_bw_ctrl, 50);
                TOQ_PORT_BW_CTRLr_START_THRESHOLDf_SET(toq_port_bw_ctrl, 7);
            }
            ioerr += WRITE_TOQ_PORT_BW_CTRLr(unit, mport, toq_port_bw_ctrl);
        }
    }

    DEQ_EFIFO_CFG_COMPLETEr_CLR(eq_efifo_cfg); 
    DEQ_EFIFO_CFG_COMPLETEr_EGRESS_FIFO_CONFIGURATION_COMPLETEf_SET(eq_efifo_cfg, 1);
    ioerr += WRITE_DEQ_EFIFO_CFG_COMPLETEr(unit, eq_efifo_cfg);

    /* Enable LLS */
    LLS_CONFIG0r_CLR(lls_config);
    LLS_CONFIG0r_DEQUEUE_ENABLEf_SET(lls_config, 1);
    LLS_CONFIG0r_ENQUEUE_ENABLEf_SET(lls_config, 1);
    LLS_CONFIG0r_FC_ENABLEf_SET(lls_config, 1);
    LLS_CONFIG0r_MIN_ENABLEf_SET(lls_config, 1);
    LLS_CONFIG0r_PORT_SCHEDULER_ENABLEf_SET(lls_config, 1);
    LLS_CONFIG0r_SHAPER_ENABLEf_SET(lls_config, 1);
    LLS_CONFIG0r_SPRI_VECT_MODE_ENABLEf_SET(lls_config, 1);
    ioerr += WRITE_LLS_CONFIG0r(unit, lls_config);

    /* Enable shaper background refresh */
    LLS_MAX_REFRESH_ENABLEr_CLR(lls_max_refresh);
    LLS_MAX_REFRESH_ENABLEr_L0_MAX_REFRESH_ENABLEf_SET(lls_max_refresh, 1);
    LLS_MAX_REFRESH_ENABLEr_L1_MAX_REFRESH_ENABLEf_SET(lls_max_refresh, 1);
    LLS_MAX_REFRESH_ENABLEr_L2_MAX_REFRESH_ENABLEf_SET(lls_max_refresh, 1);
    LLS_MAX_REFRESH_ENABLEr_PORT_MAX_REFRESH_ENABLEf_SET(lls_max_refresh, 1);
    LLS_MAX_REFRESH_ENABLEr_S0_MAX_REFRESH_ENABLEf_SET(lls_max_refresh, 1);
    LLS_MAX_REFRESH_ENABLEr_S1_MAX_REFRESH_ENABLEf_SET(lls_max_refresh, 1);
    ioerr += WRITE_LLS_MAX_REFRESH_ENABLEr(unit, lls_max_refresh);

    LLS_MIN_REFRESH_ENABLEr_L0_MIN_REFRESH_ENABLEf_SET(lls_min_refresh, 1);
    LLS_MIN_REFRESH_ENABLEr_L1_MIN_REFRESH_ENABLEf_SET(lls_min_refresh, 1);
    LLS_MIN_REFRESH_ENABLEr_L2_MIN_REFRESH_ENABLEf_SET(lls_min_refresh, 1);
    ioerr += WRITE_LLS_MIN_REFRESH_ENABLEr(unit, lls_min_refresh);

    /* RQE configuration */
    RQE_SCHEDULER_CONFIGr_CLR(rqe_scheduler_cfg);
    RQE_SCHEDULER_CONFIGr_L0_MCM_MODEf_SET(rqe_scheduler_cfg, 1);
    RQE_SCHEDULER_CONFIGr_L0_CC_MODEf_SET(rqe_scheduler_cfg, 1);
    RQE_SCHEDULER_CONFIGr_L0_UCM_MODEf_SET(rqe_scheduler_cfg, 1);
    RQE_SCHEDULER_CONFIGr_L1_MODEf_SET(rqe_scheduler_cfg, 1);
    ioerr += WRITE_RQE_SCHEDULER_CONFIGr(unit, rqe_scheduler_cfg);

    RQE_SCHEDULER_WEIGHT_L0_QUEUEr_CLR(rqe_scheduler_weight_l0);
    RQE_SCHEDULER_WEIGHT_L0_QUEUEr_WRR_WEIGHTf_SET(rqe_scheduler_weight_l0, 1);
    for (idx = 0; idx < 12; idx++) {
        ioerr += WRITE_RQE_SCHEDULER_WEIGHT_L0_QUEUEr(unit, idx, rqe_scheduler_weight_l0);
    }

    RQE_SCHEDULER_WEIGHT_L1_QUEUEr_CLR(rqe_scheduler_weight_l1);
    RQE_SCHEDULER_WEIGHT_L1_QUEUEr_WRR_WEIGHTf_SET(rqe_scheduler_weight_l1, 1);
    for (idx = 0; idx < 3; idx++) {
        ioerr += WRITE_RQE_SCHEDULER_WEIGHT_L1_QUEUEr(unit, idx, rqe_scheduler_weight_l1);
    }

    /* LLS Queue Configuration */
    CDK_PBMP_CLEAR(mmu_pbmp);
    CDK_PBMP_ADD(mmu_pbmp, CMIC_PORT);
    CDK_XGSM_BLKTYPE_PBMP_GET(unit, BLKTYPE_MXQPORT, &pbmp);
    CDK_PBMP_OR(mmu_pbmp, pbmp);
    CDK_PBMP_ITER(mmu_pbmp, port) {
        ppport = P2PP(unit, port);
        lport = P2L(unit, port);
        
        LLS_PORT_CONFIGm_CLR(lls_port_cfg);
        LLS_PORT_CONFIGm_L1_LOCK_ON_SEGMENTf_SET(lls_port_cfg, 1);
        LLS_PORT_CONFIGm_L2_LOCK_ON_SEGMENTf_SET(lls_port_cfg, 1);
        LLS_PORT_CONFIGm_P_WRR_IN_USEf_SET(lls_port_cfg, 1);
        LLS_PORT_CONFIGm_P_NUM_SPRIf_SET(lls_port_cfg, 1);
        LLS_PORT_CONFIGm_P_START_SPRIf_SET(lls_port_cfg, (port * 4));
        ioerr += WRITE_LLS_PORT_CONFIGm(unit, lport, lls_port_cfg);
        
        LLS_L0_PARENTm_CLR(l0_parent);
        LLS_L0_PARENTm_C_TYPEf_SET(l0_parent, 1);
        LLS_L0_PARENTm_C_PARENTf_SET(l0_parent, lport);
        ioerr += WRITE_LLS_L0_PARENTm(unit, (port * 4), l0_parent);
        
        LLS_L0_CONFIGm_CLR(l0_config);
        LLS_L0_CONFIGm_P_NUM_SPRIf_SET(l0_config, 1);
        LLS_L0_CONFIGm_P_START_SPRIf_SET(l0_config, (port * 16));
        ioerr += WRITE_LLS_L0_CONFIGm(unit, (port * 4), l0_config);
        
        LLS_L1_PARENTm_CLR(l1_parent);
        LLS_L1_PARENTm_C_PARENTf_SET(l1_parent, (port * 4));
        ioerr += WRITE_LLS_L1_PARENTm(unit, (port * 16), l1_parent);
        
        LLS_L1_CONFIGm_CLR(l1_config);
        LLS_L1_CONFIGm_P_NUM_SPRIf_SET(l1_config, 8);
        if (port == 0) {
            LLS_L1_CONFIGm_P_START_UC_SPRIf_SET(l1_config, 0);
        } else {
            LLS_L1_CONFIGm_P_START_UC_SPRIf_SET(l1_config, (port * 8) + 0x40);
        }
        ioerr += WRITE_LLS_L1_CONFIGm(unit, (port * 16) , l1_config);
        
        LLS_L2_PARENTm_CLR(l2_parent);
        LLS_L2_PARENTm_C_PARENTf_SET(l2_parent, (port * 16));
        for (idx = 0; idx < 8 ; idx++) {
            if (port == 0) {
                ioerr += WRITE_LLS_L2_PARENTm(unit, idx, l2_parent);
            } else {
                ioerr += WRITE_LLS_L2_PARENTm(unit, 
                                    ((port * 8) + 0x40 + idx), l2_parent);
            }
        }
        
        if (ppport >= 0) {
            if (port == CMIC_PORT) {
                q_offset = 0;
                q_limit = 44;
            } else {
                q_offset = 0x40 + (port * 8);
                q_limit = 8;
            }            

            ioerr += READ_ING_COS_MODEr(unit, ppport, &cos_mode);
            ING_COS_MODEr_BASE_QUEUE_NUMf_SET(cos_mode, q_offset);
            ioerr += WRITE_ING_COS_MODEr(unit, ppport, cos_mode);
            
            ioerr += READ_RQE_PP_PORT_CONFIGr(unit, ppport, &rqe_port);
            RQE_PP_PORT_CONFIGr_BASE_QUEUEf_SET(rqe_port, q_offset);
            ioerr += WRITE_RQE_PP_PORT_CONFIGr(unit, ppport, rqe_port);
    
            for (idx = 0; idx < q_limit; idx++) {
                ioerr += READ_EGR_QUEUE_TO_PP_PORT_MAPm(unit, (q_offset + idx), &egr_queue_to_pp_port);
                EGR_QUEUE_TO_PP_PORT_MAPm_PP_PORTf_SET(egr_queue_to_pp_port, ppport);
                ioerr += WRITE_EGR_QUEUE_TO_PP_PORT_MAPm(unit, (q_offset + idx), egr_queue_to_pp_port);
            }
        }
    }

    THDI_BYPASSr_CLR(thdi_bpass);
    THDIRQE_THDI_BYPASSr_CLR(irqe_bpass);
    THDIEMA_THDI_BYPASSr_CLR(iema_bpass);
    THDIEXT_THDI_BYPASSr_CLR(iext_bpass);
    THDIQEN_THDI_BYPASSr_CLR(iqen_bpass);
    THDO_BYPASSr_CLR(thdo_bpass);
    ioerr += (WRITE_THDI_BYPASSr(unit, thdi_bpass));
    ioerr += (WRITE_THDIQEN_THDI_BYPASSr(unit, iqen_bpass));
    ioerr += (WRITE_THDIRQE_THDI_BYPASSr(unit, irqe_bpass));
    ioerr += (WRITE_THDIEXT_THDI_BYPASSr(unit, iext_bpass));
    ioerr += (WRITE_THDIEMA_THDI_BYPASSr(unit, iema_bpass));
    ioerr += (WRITE_THDO_BYPASSr(unit, thdo_bpass));

    CDK_PBMP_ITER(mmu_pbmp, port) {
        mport = P2M(unit, port);
        
        PORT_MAX_PKT_SIZEr_SET(port_max, 49);
        THDIEXT_PORT_MAX_PKT_SIZEr_SET(iext_port_max, 49);
        THDIEMA_PORT_MAX_PKT_SIZEr_SET(iema_port_max, 49);
        THDIRQE_PORT_MAX_PKT_SIZEr_SET(irqe_port_max, 49);
        THDIQEN_PORT_MAX_PKT_SIZEr_SET(iqen_port_max, 49);
        ioerr += (WRITE_PORT_MAX_PKT_SIZEr(unit, mport, port_max));
        ioerr += (WRITE_THDIEXT_PORT_MAX_PKT_SIZEr(unit, mport, iext_port_max));
        ioerr += (WRITE_THDIEMA_PORT_MAX_PKT_SIZEr(unit, mport, iema_port_max));
        ioerr += (WRITE_THDIRQE_PORT_MAX_PKT_SIZEr(unit, mport, irqe_port_max));
        ioerr += (WRITE_THDIQEN_PORT_MAX_PKT_SIZEr(unit, mport, iqen_port_max));

        PORT_PRI_XON_ENABLEr_CLR(port_pri_xon);
        THDIEXT_PORT_PRI_XON_ENABLEr_SET(iext_pri_xon, 0xffff);
        THDIEMA_PORT_PRI_XON_ENABLEr_SET(iema_pri_xon, 0xffff);
        THDIRQE_PORT_PRI_XON_ENABLEr_SET(irqe_pri_xon, 0xffff);
        THDIQEN_PORT_PRI_XON_ENABLEr_SET(iqen_pri_xon, 0xffff);
        ioerr += (WRITE_PORT_PRI_XON_ENABLEr(unit, mport, port_pri_xon));
        ioerr += (WRITE_THDIEXT_PORT_PRI_XON_ENABLEr(unit, mport, iext_pri_xon));
        ioerr += (WRITE_THDIEMA_PORT_PRI_XON_ENABLEr(unit, mport, iema_pri_xon));
        ioerr += (WRITE_THDIRQE_PORT_PRI_XON_ENABLEr(unit, mport, irqe_pri_xon));
        ioerr += (WRITE_THDIQEN_PORT_PRI_XON_ENABLEr(unit, mport, iqen_pri_xon));
    }

    CDK_PBMP_ITER(mmu_pbmp, port) {
        mport = P2M(unit, port);
        
        if ((MXQPORT_SUBPORT(unit, port) == 0 && port <= 40) ||
                    (port == 30 || port == 33 || port == 36 || port == 39)) {
            PORT_PRI_GRPr_SET(port_pri_grp, 0xffffffff);
            THDIEXT_PORT_PRI_GRPr_SET(iext_pri_grp, 0xffffffff);
            THDIEMA_PORT_PRI_GRPr_SET(iema_pri_grp, 0xffffffff);
            THDIRQE_PORT_PRI_GRPr_SET(irqe_pri_grp, 0xffffffff);
            THDIQEN_PORT_PRI_GRPr_SET(iqen_pri_grp, 0xffffffff);
            for (idx = 0; idx < 2; idx++) {
                ioerr += (WRITE_PORT_PRI_GRPr(unit, mport, idx, port_pri_grp));
                ioerr += (WRITE_THDIEXT_PORT_PRI_GRPr(unit, mport, idx, iext_pri_grp));
                ioerr += (WRITE_THDIEMA_PORT_PRI_GRPr(unit, mport, idx, iema_pri_grp));
                ioerr += (WRITE_THDIRQE_PORT_PRI_GRPr(unit, mport, idx, irqe_pri_grp));
                ioerr += (WRITE_THDIQEN_PORT_PRI_GRPr(unit, mport, idx, iqen_pri_grp));
            }
        }
    }

    MMU_ENQ_IP_PRI_TO_PG_PROFILE_0r_SET(pri_to_pg_profile, 0x3fffffff);
    ioerr += (WRITE_MMU_ENQ_IP_PRI_TO_PG_PROFILE_0r(unit, pri_to_pg_profile));

    
    MMU_ENQ_PROFILE_0_PRI_GRPr_CLR(profile_0_pri_gpr);
    MMU_ENQ_PROFILE_1_PRI_GRPr_SET(profile_1_pri_gpr, 0xffffffff);
    MMU_ENQ_PROFILE_2_PRI_GRPr_SET(profile_2_pri_gpr, 0xffffffff);
    MMU_ENQ_PROFILE_3_PRI_GRPr_SET(profile_3_pri_gpr, 0xffffffff);
    for (idx = 0; idx < 2; idx++) {
        ioerr += (WRITE_MMU_ENQ_PROFILE_0_PRI_GRPr(unit, idx, profile_0_pri_gpr));
        ioerr += (WRITE_MMU_ENQ_PROFILE_1_PRI_GRPr(unit, idx, profile_1_pri_gpr));
        ioerr += (WRITE_MMU_ENQ_PROFILE_2_PRI_GRPr(unit, idx, profile_2_pri_gpr));
        ioerr += (WRITE_MMU_ENQ_PROFILE_3_PRI_GRPr(unit, idx, profile_3_pri_gpr));
    }
    
    /* Input port shared space */
    BUFFER_CELL_LIMIT_SP_SHAREDr_CLR(sp_shared);
    THDIEXT_BUFFER_CELL_LIMIT_SP_SHAREDr_CLR(iext_sp_shared);
    THDIEMA_BUFFER_CELL_LIMIT_SP_SHAREDr_CLR(iema_sp_shared);
    THDIRQE_BUFFER_CELL_LIMIT_SP_SHAREDr_CLR(irqe_sp_shared);
    THDIQEN_BUFFER_CELL_LIMIT_SP_SHAREDr_CLR(iqen_sp_shared);
    ioerr += (WRITE_BUFFER_CELL_LIMIT_SP_SHAREDr(unit, sp_shared));
    ioerr += (WRITE_THDIEXT_BUFFER_CELL_LIMIT_SP_SHAREDr(unit, iext_sp_shared));
    ioerr += (WRITE_THDIEMA_BUFFER_CELL_LIMIT_SP_SHAREDr(unit, iema_sp_shared));
    ioerr += (WRITE_THDIRQE_BUFFER_CELL_LIMIT_SP_SHAREDr(unit, irqe_sp_shared));
    ioerr += (WRITE_THDIQEN_BUFFER_CELL_LIMIT_SP_SHAREDr(unit, iqen_sp_shared));

    /* Input port per-device global headroom */
    ioerr += (READ_THDO_MISCCONFIGr(unit, &thd0_misc));
    THDO_MISCCONFIGr_STAT_CLEARf_SET(thd0_misc, 0);
    THDO_MISCCONFIGr_PARITY_CHK_ENf_SET(thd0_misc, 1);
    THDO_MISCCONFIGr_PARITY_GEN_ENf_SET(thd0_misc, 1);
    ioerr += (WRITE_THDO_MISCCONFIGr(unit, thd0_misc));

    ioerr += (READ_OP_THR_CONFIGr(unit, &op_thr));
    OP_THR_CONFIGr_EARLY_E2E_SELECTf_SET(op_thr, 0);
    ioerr += (WRITE_OP_THR_CONFIGr(unit, op_thr));

    CFAPIFULLSETPOINTr_SET(cfapifullsp, 0x2a46);
    ioerr += (WRITE_CFAPIFULLSETPOINTr(unit, cfapifullsp));
    CFAPIFULLRESETPOINTr_SET(cfapifullrp, 0x29e4);
    ioerr += (WRITE_CFAPIFULLRESETPOINTr(unit, cfapifullrp));
    
    /* Input port thresholds */
    COLOR_AWAREr_SET(color_aware, 0);
    ioerr += (WRITE_COLOR_AWAREr(unit, color_aware));

    /* Internal Buffer Ingress Pool */
    GLOBAL_HDRM_LIMITr_SET(glb_hdrm, 0x6c);
    ioerr += (WRITE_GLOBAL_HDRM_LIMITr(unit, glb_hdrm));
    BUFFER_CELL_LIMIT_SPr_SET(buf_cell, 0xa30);
    ioerr += (WRITE_BUFFER_CELL_LIMIT_SPr(unit, 0, buf_cell));
    CELL_RESET_LIMIT_OFFSET_SPr_SET(cell_reset, 0x63);
    ioerr += (WRITE_CELL_RESET_LIMIT_OFFSET_SPr(unit, 0, cell_reset));

    /* RE WQEs */
    THDIRQE_GLOBAL_HDRM_LIMITr_SET(irqe_glb_hdrm, 0);
    ioerr += (WRITE_THDIRQE_GLOBAL_HDRM_LIMITr(unit, irqe_glb_hdrm));
    THDIRQE_BUFFER_CELL_LIMIT_SPr_SET(irqe_buf_cell, 0x50f);
    ioerr += (WRITE_THDIRQE_BUFFER_CELL_LIMIT_SPr(unit, 0, irqe_buf_cell));
    THDIRQE_CELL_RESET_LIMIT_OFFSET_SPr_SET(irqe_cell_reset, 0xb);
    ioerr += (WRITE_THDIRQE_CELL_RESET_LIMIT_OFFSET_SPr(unit, 0, irqe_cell_reset));

    /* EQEs */
    THDIQEN_GLOBAL_HDRM_LIMITr_SET(iqen_glb_hdrm, 0xb);
    ioerr += (WRITE_THDIQEN_GLOBAL_HDRM_LIMITr(unit, iqen_glb_hdrm));
    THDIQEN_BUFFER_CELL_LIMIT_SPr_SET(iqen_buf_cell, 0x1260d);
    ioerr += (WRITE_THDIQEN_BUFFER_CELL_LIMIT_SPr(unit, 0, iqen_buf_cell));
    THDIQEN_CELL_RESET_LIMIT_OFFSET_SPr_SET(iqen_cell_reset, 0x129);
    ioerr += (WRITE_THDIQEN_CELL_RESET_LIMIT_OFFSET_SPr(unit, 0, iqen_cell_reset));

    /* Ouput port thresholds */
    /* Internal buffer Egress pool */
    OP_BUFFER_SHARED_LIMIT_CELLIr_SET(op_shr_cell, 0x2806);
    ioerr += (WRITE_OP_BUFFER_SHARED_LIMIT_CELLIr(unit, op_shr_cell));
    OP_BUFFER_SHARED_LIMIT_RESUME_CELLIr_SET(op_sh_res_cell, 0x27a3);
    ioerr += (WRITE_OP_BUFFER_SHARED_LIMIT_RESUME_CELLIr(unit, op_sh_res_cell));
    OP_BUFFER_LIMIT_YELLOW_CELLIr_SET(op_yel_cell, 0x501);
    ioerr += (WRITE_OP_BUFFER_LIMIT_YELLOW_CELLIr(unit, op_yel_cell));
    OP_BUFFER_LIMIT_RED_CELLIr_SET(op_red_cell, 0x501);
    ioerr += (WRITE_OP_BUFFER_LIMIT_RED_CELLIr(unit, op_red_cell));
    OP_BUFFER_LIMIT_RESUME_YELLOW_CELLIr_SET(op_res_yel_cell, 0x4f5);
    ioerr += (WRITE_OP_BUFFER_LIMIT_RESUME_YELLOW_CELLIr(unit, op_res_yel_cell));
    OP_BUFFER_LIMIT_RESUME_RED_CELLIr_SET(op_res_red_cell, 0x4f5);
    ioerr += (WRITE_OP_BUFFER_LIMIT_RESUME_RED_CELLIr(unit, op_res_red_cell));

    /* RE WQEs*/
    OP_BUFFER_SHARED_LIMIT_THDORQEQr_SET(op_shr_rqeq, 0x1fff);
    ioerr += (WRITE_OP_BUFFER_SHARED_LIMIT_THDORQEQr(unit, op_shr_rqeq));
    OP_BUFFER_SHARED_LIMIT_RESUME_THDORQEQr_SET(op_sh_res_rqeq, 0x1ff5);
    ioerr += (WRITE_OP_BUFFER_SHARED_LIMIT_RESUME_THDORQEQr(unit, op_sh_res_rqeq));
    OP_BUFFER_LIMIT_YELLOW_THDORQEQr_SET(op_yel_rqeq, 0x3ff);
    ioerr += (WRITE_OP_BUFFER_LIMIT_YELLOW_THDORQEQr(unit, op_yel_rqeq));
    OP_BUFFER_LIMIT_RESUME_YELLOW_THDORQEQr_SET(op_res_yel_rqeq, 0x3fe);
    ioerr += (WRITE_OP_BUFFER_LIMIT_RESUME_YELLOW_THDORQEQr(unit, op_res_yel_rqeq));
    OP_BUFFER_LIMIT_RED_THDORQEQr_SET(op_red_rqeq, 0x3ff);
    ioerr += (WRITE_OP_BUFFER_LIMIT_RED_THDORQEQr(unit, op_red_rqeq));
    OP_BUFFER_LIMIT_RESUME_RED_THDORQEQr_SET(op_res_red_rqeq, 0x3fe);
    ioerr += (WRITE_OP_BUFFER_LIMIT_RESUME_RED_THDORQEQr(unit, op_res_red_rqeq));
    OP_BUFFER_SHARED_LIMIT_QENTRYr_SET(op_shr_qentry, 0x3fd78);
    ioerr += (WRITE_OP_BUFFER_SHARED_LIMIT_QENTRYr(unit, op_shr_qentry));
    OP_BUFFER_LIMIT_RED_QENTRYr_SET(op_red_qentry, 0x7faf);
    ioerr += (WRITE_OP_BUFFER_LIMIT_RED_QENTRYr(unit, op_red_qentry));
    OP_BUFFER_LIMIT_YELLOW_QENTRYr_SET(op_yel_qentry, 0x7faf);
    ioerr += (WRITE_OP_BUFFER_LIMIT_YELLOW_QENTRYr(unit, op_yel_qentry));
    OP_BUFFER_SHARED_LIMIT_RESUME_QENTRYr_SET(op_sh_res_qentry, 0x3fc4f);
    ioerr += (WRITE_OP_BUFFER_SHARED_LIMIT_RESUME_QENTRYr(unit, op_sh_res_qentry));
    OP_BUFFER_LIMIT_RESUME_YELLOW_QENTRYr_SET(op_res_yel_qentry, 0x7f8a);
    ioerr += (WRITE_OP_BUFFER_LIMIT_RESUME_YELLOW_QENTRYr(unit, op_res_yel_qentry));
    OP_BUFFER_LIMIT_RESUME_RED_QENTRYr_SET(op_res_red_qentry, 0x7f8a);
    ioerr += (WRITE_OP_BUFFER_LIMIT_RESUME_RED_QENTRYr(unit, op_res_red_qentry));

    /* EP Redirection Packets */
    OP_BUFFER_SHARED_LIMIT_THDORDEQr_SET(op_sh_rdeq, 0x3a8);
    ioerr += (WRITE_OP_BUFFER_SHARED_LIMIT_THDORDEQr(unit, op_sh_rdeq));
    OP_BUFFER_LIMIT_YELLOW_THDORDEQr_SET(op_yel_rdeq, 0x75);
    ioerr += (WRITE_OP_BUFFER_LIMIT_YELLOW_THDORDEQr(unit, op_yel_rdeq));
    OP_BUFFER_LIMIT_RED_THDORDEQr_SET(op_red_rdeq, 0x75);
    ioerr += (WRITE_OP_BUFFER_LIMIT_RED_THDORDEQr(unit, op_red_rdeq));
    OP_BUFFER_LIMIT_RESUME_YELLOW_THDORDEQr_SET(op_res_yel_rdeq, 0x74);
    ioerr += (WRITE_OP_BUFFER_LIMIT_RESUME_YELLOW_THDORDEQr(unit, op_res_yel_rdeq));
    OP_BUFFER_LIMIT_RESUME_RED_THDORDEQr_SET(op_res_red_rdeq, 0x74);
    ioerr += (WRITE_OP_BUFFER_LIMIT_RESUME_RED_THDORDEQr(unit, op_res_red_rdeq));
    OP_BUFFER_SHARED_LIMIT_RESUME_THDORDEQr_SET(op_sh_res_rdeq, 0x3a4);
    ioerr += (WRITE_OP_BUFFER_SHARED_LIMIT_RESUME_THDORDEQr(unit, op_sh_res_rdeq));

    /* Per Port Registers */
    /* ######################## */
    /* 1. Input Port Thresholds */
    /* ######################## */
    /* 1.1 Internal Buffer Ingress Pool */
    CDK_PBMP_ITER(mmu_pbmp, port) {
        mport = P2M(unit, port);
        
        THDI_PORT_SP_CONFIGm_CLR(port_sp_cfg);
        THDI_PORT_SP_CONFIGm_PORT_SP_MAX_LIMITf_SET(port_sp_cfg, 0xa30);
        THDI_PORT_SP_CONFIGm_PORT_SP_RESUME_LIMITf_SET(port_sp_cfg, 0xa1e);
        ioerr += (WRITE_THDI_PORT_SP_CONFIGm(unit, (mport * 4), port_sp_cfg));
        
        ioerr += (READ_PORT_MAX_PKT_SIZEr(unit, mport, &port_max_pkt_size));
        PORT_MAX_PKT_SIZEr_PORT_MAX_PKT_SIZEf_SET(port_max_pkt_size, 0x31);
        ioerr += (WRITE_PORT_MAX_PKT_SIZEr(unit, mport, port_max_pkt_size));

        THDI_PORT_PG_CONFIGm_CLR(port_pg_cfg);
        THDI_PORT_PG_CONFIGm_PG_MIN_LIMITf_SET(port_pg_cfg, 0x31);
        THDI_PORT_PG_CONFIGm_PG_SHARED_LIMITf_SET(port_pg_cfg, 7);
        THDI_PORT_PG_CONFIGm_PG_RESET_OFFSETf_SET(port_pg_cfg, 0x12);
        THDI_PORT_PG_CONFIGm_PG_RESET_FLOORf_SET(port_pg_cfg, 0);
        THDI_PORT_PG_CONFIGm_PG_GBL_HDRM_ENf_SET(port_pg_cfg, 0);
        THDI_PORT_PG_CONFIGm_PG_SHARED_DYNAMICf_SET(port_pg_cfg, 1);
        if (port == CMIC_PORT) {
            THDI_PORT_PG_CONFIGm_PG_HDRM_LIMITf_SET(port_pg_cfg, 0x2e);
        } else if (BMD_PORT_PROPERTIES(unit, port) == BMD_PORT_GE) {
            THDI_PORT_PG_CONFIGm_PG_HDRM_LIMITf_SET(port_pg_cfg, 0x82);
        } else {
            THDI_PORT_PG_CONFIGm_PG_HDRM_LIMITf_SET(port_pg_cfg, 0xb4);
        }
        THDI_PORT_PG_CONFIGm_SP_MIN_PG_ENABLEf_SET(port_pg_cfg, 1);
        THDI_PORT_PG_CONFIGm_SP_SHARED_MAX_ENABLEf_SET(port_pg_cfg, 1);
        ioerr += (WRITE_THDI_PORT_PG_CONFIGm(unit, (mport * 8), port_pg_cfg)); 
        if ((MXQPORT_SUBPORT(unit, port) == 0 && port <= 40) ||
                    (port == 30 || port == 33 || port == 36 || port == 39)) {
            for (idx = 1; idx < 8; idx++) {
                THDI_PORT_PG_CONFIGm_PG_MIN_LIMITf_SET(port_pg_cfg, 1);
                ioerr += (WRITE_THDI_PORT_PG_CONFIGm(unit, ((mport * 8) + idx), port_pg_cfg));
            }
        }
    }

    /* 1.2 External Buffer */
    CDK_PBMP_ITER(mmu_pbmp, port) {
        mport = P2M(unit, port);
        
        THDIEXT_THDI_PORT_PG_CONFIGm_CLR(iext_port_pg_cfg);
        THDIEXT_THDI_PORT_PG_CONFIGm_SP_SHARED_MAX_ENABLEf_SET(iext_port_pg_cfg, 1);
        THDIEXT_THDI_PORT_PG_CONFIGm_SP_MIN_PG_ENABLEf_SET(iext_port_pg_cfg, 1);
        ioerr += (WRITE_THDIEXT_THDI_PORT_PG_CONFIGm(unit, (mport * 8), iext_port_pg_cfg));
        if ((MXQPORT_SUBPORT(unit, port) == 0 && port <= 40) ||
                    (port == 30 || port == 33 || port == 36 || port == 39)) {
            for (idx = 1; idx < 8; idx++) {
                THDIEXT_THDI_PORT_PG_CONFIGm_PG_MIN_LIMITf_SET(iext_port_pg_cfg, 1);
                ioerr += (WRITE_THDIEXT_THDI_PORT_PG_CONFIGm(unit, ((mport * 8) + idx), iext_port_pg_cfg));
            }
        }
    }

    /* 1.3 EMA Pool */
    CDK_PBMP_ITER(mmu_pbmp, port) {
        mport = P2M(unit, port);
        
        THDIEMA_THDI_PORT_PG_CONFIGm_CLR(iema_port_pg_cfg);
        THDIEMA_THDI_PORT_PG_CONFIGm_SP_SHARED_MAX_ENABLEf_SET(iema_port_pg_cfg, 1);
        THDIEMA_THDI_PORT_PG_CONFIGm_SP_MIN_PG_ENABLEf_SET(iema_port_pg_cfg, 1);
        ioerr += (WRITE_THDIEMA_THDI_PORT_PG_CONFIGm(unit, (mport * 8), iema_port_pg_cfg));
        if ((MXQPORT_SUBPORT(unit, port) == 0 && port <= 40) ||
                    (port == 30 || port == 33 || port == 36 || port == 39)) {
            for (idx = 1; idx < 8; idx++) {
                THDIEMA_THDI_PORT_PG_CONFIGm_PG_MIN_LIMITf_SET(iema_port_pg_cfg, 1);
                ioerr += (WRITE_THDIEMA_THDI_PORT_PG_CONFIGm(unit, ((mport * 8) + idx), iema_port_pg_cfg));
            }
        }
    }

    /* 1.4 RE WQEs */
    CDK_PBMP_ITER(mmu_pbmp, port) {
        mport = P2M(unit, port);
        
        THDIRQE_THDI_PORT_SP_CONFIGm_CLR(irqe_port_sp_cfg);
        THDIRQE_THDI_PORT_SP_CONFIGm_PORT_SP_MIN_LIMITf_SET(irqe_port_sp_cfg, 0);
        THDIRQE_THDI_PORT_SP_CONFIGm_PORT_SP_MAX_LIMITf_SET(irqe_port_sp_cfg, 0x50f);
        THDIRQE_THDI_PORT_SP_CONFIGm_PORT_SP_RESUME_LIMITf_SET(irqe_port_sp_cfg, 0x50d);
        ioerr += (WRITE_THDIRQE_THDI_PORT_SP_CONFIGm(unit, (mport * 4), irqe_port_sp_cfg));

        THDIRQE_THDI_PORT_PG_CONFIGm_CLR(irqe_port_pg_cfg);
        THDIRQE_THDI_PORT_PG_CONFIGm_PG_MIN_LIMITf_SET(irqe_port_pg_cfg, 0x31);
        THDIRQE_THDI_PORT_PG_CONFIGm_PG_SHARED_LIMITf_SET(irqe_port_pg_cfg, 7);
        THDIRQE_THDI_PORT_PG_CONFIGm_PG_RESET_OFFSETf_SET(irqe_port_pg_cfg, 2);
        THDIRQE_THDI_PORT_PG_CONFIGm_PG_RESET_FLOORf_SET(irqe_port_pg_cfg, 0);
        THDIRQE_THDI_PORT_PG_CONFIGm_PG_SHARED_DYNAMICf_SET(irqe_port_pg_cfg, 1);
        if (port == CMIC_PORT) {
            THDIRQE_THDI_PORT_PG_CONFIGm_PG_HDRM_LIMITf_SET(irqe_port_pg_cfg, 0x28);
        } else if (BMD_PORT_PROPERTIES(unit, port) == BMD_PORT_GE) {
            THDIRQE_THDI_PORT_PG_CONFIGm_PG_HDRM_LIMITf_SET(irqe_port_pg_cfg, 0x73);
        } else {
            THDIRQE_THDI_PORT_PG_CONFIGm_PG_HDRM_LIMITf_SET(irqe_port_pg_cfg, 0x9f);
        }
        THDIRQE_THDI_PORT_PG_CONFIGm_PG_GBL_HDRM_ENf_SET(irqe_port_pg_cfg, 0);
        THDIRQE_THDI_PORT_PG_CONFIGm_SP_SHARED_MAX_ENABLEf_SET(irqe_port_pg_cfg, 1);
        THDIRQE_THDI_PORT_PG_CONFIGm_SP_MIN_PG_ENABLEf_SET(irqe_port_pg_cfg, 1);
        ioerr += (WRITE_THDIRQE_THDI_PORT_PG_CONFIGm(unit, (mport * 8), irqe_port_pg_cfg));
        
        if ((MXQPORT_SUBPORT(unit, port) == 0 && port <= 40) ||
                    (port == 30 || port == 33 || port == 36 || port == 39)) {
            for (idx = 1; idx < 8; idx++) {
                THDIRQE_THDI_PORT_PG_CONFIGm_PG_MIN_LIMITf_SET(irqe_port_pg_cfg, 1);
                ioerr += (WRITE_THDIRQE_THDI_PORT_PG_CONFIGm(unit, ((mport * 8) + idx), irqe_port_pg_cfg));
            }
        }
    }
    
    /* 1.5 EQEs */
    CDK_PBMP_ITER(mmu_pbmp, port) {
        mport = P2M(unit, port);
        
        THDIQEN_THDI_PORT_SP_CONFIGm_CLR(iqen_port_sp_cfg);
        THDIQEN_THDI_PORT_SP_CONFIGm_PORT_SP_MIN_LIMITf_SET(iqen_port_sp_cfg, 0);
        THDIQEN_THDI_PORT_SP_CONFIGm_PORT_SP_MAX_LIMITf_SET(iqen_port_sp_cfg, 0x1260d);
        THDIQEN_THDI_PORT_SP_CONFIGm_PORT_SP_RESUME_LIMITf_SET(iqen_port_sp_cfg, 0x125d7);
        ioerr += (WRITE_THDIQEN_THDI_PORT_SP_CONFIGm(unit, (mport * 4), iqen_port_sp_cfg));

        THDIQEN_THDI_PORT_PG_CONFIGm_CLR(iqen_port_pg_cfg);
        THDIQEN_THDI_PORT_PG_CONFIGm_PG_MIN_LIMITf_SET(iqen_port_pg_cfg, 0x52b);
        THDIQEN_THDI_PORT_PG_CONFIGm_PG_SHARED_LIMITf_SET(iqen_port_pg_cfg, 7);
        THDIQEN_THDI_PORT_PG_CONFIGm_PG_RESET_OFFSETf_SET(iqen_port_pg_cfg, 0x36);
        THDIQEN_THDI_PORT_PG_CONFIGm_PG_RESET_FLOORf_SET(iqen_port_pg_cfg, 0);
        THDIQEN_THDI_PORT_PG_CONFIGm_PG_SHARED_DYNAMICf_SET(iqen_port_pg_cfg, 1);
        if (mport == CMIC_PORT) {
            THDIQEN_THDI_PORT_PG_CONFIGm_PG_HDRM_LIMITf_SET(iqen_port_pg_cfg, 0x438);
        } else if (BMD_PORT_PROPERTIES(unit, port) == BMD_PORT_GE) {
            THDIQEN_THDI_PORT_PG_CONFIGm_PG_HDRM_LIMITf_SET(iqen_port_pg_cfg, 0xc21);
        } else {
            THDIQEN_THDI_PORT_PG_CONFIGm_PG_HDRM_LIMITf_SET(iqen_port_pg_cfg, 0x10c5);
        }
        THDIQEN_THDI_PORT_PG_CONFIGm_PG_GBL_HDRM_ENf_SET(iqen_port_pg_cfg, 0);
        THDIQEN_THDI_PORT_PG_CONFIGm_SP_SHARED_MAX_ENABLEf_SET(iqen_port_pg_cfg, 1);
        THDIQEN_THDI_PORT_PG_CONFIGm_SP_MIN_PG_ENABLEf_SET(iqen_port_pg_cfg, 1);
        ioerr += (WRITE_THDIQEN_THDI_PORT_PG_CONFIGm(unit, (mport * 8), iqen_port_pg_cfg));
        if ((MXQPORT_SUBPORT(unit, port) == 0 && port <= 40) ||
                    (port == 30 || port == 33 || port == 36 || port == 39)) {
            for (idx = 1; idx < 8; idx++) {
                THDIQEN_THDI_PORT_PG_CONFIGm_PG_MIN_LIMITf_SET(iqen_port_pg_cfg, 1);
                ioerr += (WRITE_THDIQEN_THDI_PORT_PG_CONFIGm(unit, ((mport * 8) + idx), iqen_port_pg_cfg));
            }
        }
    }

    /* ######################## */
    /* 2. Output Port Thresholds */
    /* ######################## */
    /* 2.1 Internal Buffer */
    CDK_PBMP_ITER(mmu_pbmp, port) {
        mport = P2M(unit, port);

        if (mport == CMIC_PORT) {
            op_node = 0;
            ceil = 6;
            q_offset = 0;
            q_limit = 44;
        } else {
            op_node = mport + 8;
            ceil = 1;
            q_offset = 0x40 + (mport * 8);
            q_limit = 8;
        }

        for (idx = 0; idx < ceil; idx++) {
            ioerr += (READ_MMU_THDO_OPNCONFIG_CELLm(unit, (op_node + idx), &opncfg_cell));
            MMU_THDO_OPNCONFIG_CELLm_OPN_SHARED_LIMIT_CELLf_SET(opncfg_cell, 0x2806);
            MMU_THDO_OPNCONFIG_CELLm_OPN_SHARED_RESET_VALUE_CELLf_SET(opncfg_cell, 0x27f4);
            MMU_THDO_OPNCONFIG_CELLm_PORT_LIMIT_ENABLE_CELLf_SET(opncfg_cell, 0);
            ioerr += (WRITE_MMU_THDO_OPNCONFIG_CELLm(unit, (op_node + idx), opncfg_cell));
        }
        
        for (idx = 0; idx < q_limit; idx++) {
            ioerr += (READ_MMU_THDO_QCONFIG_CELLm(unit, (q_offset + idx), &qcfg_cell));
            MMU_THDO_QCONFIG_CELLm_Q_SHARED_LIMIT_CELLf_SET(qcfg_cell, 0x2806);
            MMU_THDO_QCONFIG_CELLm_Q_MIN_CELLf_SET(qcfg_cell, 0);
            MMU_THDO_QCONFIG_CELLm_Q_LIMIT_ENABLE_CELLf_SET(qcfg_cell, 0);
            MMU_THDO_QCONFIG_CELLm_Q_LIMIT_DYNAMIC_CELLf_SET(qcfg_cell, 0);
            MMU_THDO_QCONFIG_CELLm_Q_COLOR_ENABLE_CELLf_SET(qcfg_cell, 0);
            MMU_THDO_QCONFIG_CELLm_Q_COLOR_LIMIT_DYNAMIC_CELLf_SET(qcfg_cell, 0);
            MMU_THDO_QCONFIG_CELLm_LIMIT_YELLOW_CELLf_SET(qcfg_cell, 0x501);
            ioerr += (WRITE_MMU_THDO_QCONFIG_CELLm(unit, (q_offset + idx), qcfg_cell));
        
            ioerr += (READ_MMU_THDO_QOFFSET_CELLm(unit, (q_offset + idx), &qoff_cell));
            MMU_THDO_QOFFSET_CELLm_RESET_OFFSET_CELLf_SET(qoff_cell, 2);
            MMU_THDO_QOFFSET_CELLm_LIMIT_RED_CELLf_SET(qoff_cell, 0x501);
            MMU_THDO_QOFFSET_CELLm_RESET_OFFSET_YELLOW_CELLf_SET(qoff_cell, 2);
            MMU_THDO_QOFFSET_CELLm_RESET_OFFSET_RED_CELLf_SET(qoff_cell, 2);
            ioerr += (WRITE_MMU_THDO_QOFFSET_CELLm(unit, (q_offset + idx), qoff_cell));
        }
    }
    
    /* 2.4 RE WQEs */
    /* 2.4.1 RQEQ */
    for (idx = 0; idx < NUM_RQEQ_COS; idx++) {
        ioerr += (READ_OP_QUEUE_CONFIG1_THDORQEQr(unit, idx, &opq_rqeq1));
        OP_QUEUE_CONFIG1_THDORQEQr_Q_COLOR_ENABLEf_SET(opq_rqeq1, 0);
        OP_QUEUE_CONFIG1_THDORQEQr_Q_COLOR_DYNAMICf_SET(opq_rqeq1, 0);
        OP_QUEUE_CONFIG1_THDORQEQr_Q_MINf_SET(opq_rqeq1, 0);
        ioerr += (WRITE_OP_QUEUE_CONFIG1_THDORQEQr(unit, idx, opq_rqeq1));

        ioerr += (READ_OP_QUEUE_CONFIG_THDORQEQr(unit, idx, &opq_rqeq));
        OP_QUEUE_CONFIG_THDORQEQr_Q_LIMIT_ENABLEf_SET(opq_rqeq, 0);
        OP_QUEUE_CONFIG_THDORQEQr_Q_LIMIT_DYNAMICf_SET(opq_rqeq, 0);
        OP_QUEUE_CONFIG_THDORQEQr_Q_SHARED_LIMITf_SET(opq_rqeq, 0x1fff);
        ioerr += (WRITE_OP_QUEUE_CONFIG_THDORQEQr(unit, idx, opq_rqeq));

        OP_QUEUE_RESET_OFFSET_THDORQEQr_SET(opq_rst_rqeq, 1);
        ioerr += (WRITE_OP_QUEUE_RESET_OFFSET_THDORQEQr(unit, idx, opq_rst_rqeq));

        OP_QUEUE_LIMIT_YELLOW_THDORQEQr_SET(opq_yel_rqeq, 0x3ff);
        ioerr += (WRITE_OP_QUEUE_LIMIT_YELLOW_THDORQEQr(unit, idx, opq_yel_rqeq));

        OP_QUEUE_LIMIT_RED_THDORQEQr_SET(opq_red_rqeq, 0x3ff);
        ioerr += (WRITE_OP_QUEUE_LIMIT_RED_THDORQEQr(unit, idx, opq_red_rqeq));

        OP_QUEUE_RESET_OFFSET_YELLOW_THDORQEQr_SET(opq_rst_yel_rqeq, 1);
        ioerr += (WRITE_OP_QUEUE_RESET_OFFSET_YELLOW_THDORQEQr(unit, idx, opq_rst_yel_rqeq));

        OP_QUEUE_RESET_OFFSET_RED_THDORQEQr_SET(opq_rst_red_rqeq, 1);
        ioerr += (WRITE_OP_QUEUE_RESET_OFFSET_RED_THDORQEQr(unit, idx, opq_rst_red_rqeq));
    }

    /* 2.4.2 RQEI */
    for (idx = 0; idx < NUM_RQEI_COS; idx++) {
        ioerr += (READ_OP_QUEUE_CONFIG1_THDORQEIr(unit, idx, &opq_rqei1));
        OP_QUEUE_CONFIG1_THDORQEIr_Q_COLOR_ENABLEf_SET(opq_rqei1, 0);
        OP_QUEUE_CONFIG1_THDORQEIr_Q_COLOR_DYNAMICf_SET(opq_rqei1, 0);
        OP_QUEUE_CONFIG1_THDORQEIr_Q_MINf_SET(opq_rqei1, 0);
        ioerr += (WRITE_OP_QUEUE_CONFIG1_THDORQEIr(unit, idx, opq_rqei1));

        ioerr += (READ_OP_QUEUE_CONFIG_THDORQEIr(unit, idx, &opq_rqei));
        OP_QUEUE_CONFIG_THDORQEIr_Q_LIMIT_ENABLEf_SET(opq_rqei, 0);
        OP_QUEUE_CONFIG_THDORQEIr_Q_LIMIT_DYNAMICf_SET(opq_rqei, 0);
        OP_QUEUE_CONFIG_THDORQEIr_Q_SHARED_LIMITf_SET(opq_rqei, 0x2806);
        ioerr += (WRITE_OP_QUEUE_CONFIG_THDORQEIr(unit, idx, opq_rqei));

        OP_QUEUE_RESET_OFFSET_THDORQEIr_SET(opq_rst_rqei, 2);
        ioerr += (WRITE_OP_QUEUE_RESET_OFFSET_THDORQEIr(unit, idx, opq_rst_rqei));

        OP_QUEUE_LIMIT_YELLOW_THDORQEIr_SET(opq_yel_rqei, 0x501);
        ioerr += (WRITE_OP_QUEUE_LIMIT_YELLOW_THDORQEIr(unit, idx, opq_yel_rqei));

        OP_QUEUE_LIMIT_RED_THDORQEIr_SET(opq_red_rqei, 0x501);
        ioerr += (WRITE_OP_QUEUE_LIMIT_RED_THDORQEIr(unit, idx, opq_red_rqei));

        OP_QUEUE_RESET_OFFSET_YELLOW_THDORQEIr_SET(opq_rst_yel_rqei, 2);
        ioerr += (WRITE_OP_QUEUE_RESET_OFFSET_YELLOW_THDORQEIr(unit, idx, opq_rst_yel_rqei));

        OP_QUEUE_RESET_OFFSET_RED_THDORQEIr_SET(opq_rst_red_rqei, 2);
        ioerr += (WRITE_OP_QUEUE_RESET_OFFSET_RED_THDORQEIr(unit, idx, opq_rst_red_rqei));
    }

    /* 2.5 EQEs */
    CDK_PBMP_ITER(mmu_pbmp, port) {
        mport = P2M(unit, port);

        if (mport == CMIC_PORT) {
            op_node = 0;
            ceil = 6;
            q_offset = 0;
            q_limit = 44;
        } else {
            op_node = mport + 8;
            ceil = 1;
            q_offset = 0x40 + (mport * 8);
            q_limit = 8;
        }

        for (idx = 0; idx < ceil; idx++) {
            ioerr += (READ_MMU_THDO_OPNCONFIG_QENTRYm(unit, (op_node + idx), &opncfg_qentry));
            MMU_THDO_OPNCONFIG_QENTRYm_OPN_SHARED_LIMIT_QENTRYf_SET(opncfg_qentry, 0x3fd78);
            MMU_THDO_OPNCONFIG_QENTRYm_OPN_SHARED_RESET_VALUE_QENTRYf_SET(opncfg_qentry, 0x3fd76);
            MMU_THDO_OPNCONFIG_QENTRYm_PORT_LIMIT_ENABLE_QENTRYf_SET(opncfg_qentry, 0);
            ioerr += (WRITE_MMU_THDO_OPNCONFIG_QENTRYm(unit, (op_node + idx), opncfg_qentry));
        }
        
        for (idx = 0; idx < q_limit; idx++) {
            ioerr += (READ_MMU_THDO_QCONFIG_QENTRYm(unit, (q_offset + idx), &qcfg_qentry));
            MMU_THDO_QCONFIG_QENTRYm_Q_SHARED_LIMIT_QENTRYf_SET(qcfg_qentry, 0x3fd78);
            MMU_THDO_QCONFIG_QENTRYm_Q_MIN_QENTRYf_SET(qcfg_qentry, 0);
            MMU_THDO_QCONFIG_QENTRYm_Q_LIMIT_ENABLE_QENTRYf_SET(qcfg_qentry, 0);
            MMU_THDO_QCONFIG_QENTRYm_Q_LIMIT_DYNAMIC_QENTRYf_SET(qcfg_qentry, 0);
            MMU_THDO_QCONFIG_QENTRYm_Q_COLOR_ENABLE_QENTRYf_SET(qcfg_qentry, 0);
            MMU_THDO_QCONFIG_QENTRYm_LIMIT_YELLOW_QENTRYf_SET(qcfg_qentry, 0x7faf);
            ioerr += (WRITE_MMU_THDO_QCONFIG_QENTRYm(unit, (q_offset + idx), qcfg_qentry));
        
            ioerr += (READ_MMU_THDO_QOFFSET_QENTRYm(unit, (q_offset + idx), &qoff_qentry));
            MMU_THDO_QOFFSET_QENTRYm_RESET_OFFSET_QENTRYf_SET(qoff_qentry, 1);
            MMU_THDO_QOFFSET_QENTRYm_LIMIT_RED_QENTRYf_SET(qoff_qentry, 0x7faf);
            MMU_THDO_QOFFSET_QENTRYm_RESET_OFFSET_YELLOW_QENTRYf_SET(qoff_qentry, 1);
            MMU_THDO_QOFFSET_QENTRYm_RESET_OFFSET_RED_QENTRYf_SET(qoff_qentry, 1);
            ioerr += (WRITE_MMU_THDO_QOFFSET_QENTRYm(unit, (q_offset + idx), qoff_qentry));
        }
    }
    
    /* 2.5.1 RDEQ */
    for (idx = 0; idx < NUM_RDEQ_COS; idx++) {
        ioerr += (READ_OP_QUEUE_CONFIG1_THDORDEQr(unit, idx, &opq_cfg1_rdeq));
        OP_QUEUE_CONFIG1_THDORDEQr_Q_MINf_SET(opq_cfg1_rdeq, 9);
        OP_QUEUE_CONFIG1_THDORDEQr_Q_COLOR_ENABLEf_SET(opq_cfg1_rdeq, 0);
        OP_QUEUE_CONFIG1_THDORDEQr_Q_COLOR_DYNAMICf_SET(opq_cfg1_rdeq, 1);
        ioerr += (WRITE_OP_QUEUE_CONFIG1_THDORDEQr(unit, idx, opq_cfg1_rdeq));

        ioerr += (READ_OP_QUEUE_CONFIG_THDORDEQr(unit, idx, &opq_cfg_rdeq));
        OP_QUEUE_CONFIG_THDORDEQr_Q_SHARED_LIMITf_SET(opq_cfg_rdeq, 7);
        OP_QUEUE_CONFIG_THDORDEQr_Q_LIMIT_DYNAMICf_SET(opq_cfg_rdeq, 1);
        OP_QUEUE_CONFIG_THDORDEQr_Q_LIMIT_ENABLEf_SET(opq_cfg_rdeq, 0);
        ioerr += (WRITE_OP_QUEUE_CONFIG_THDORDEQr(unit, idx, opq_cfg_rdeq));

        OP_QUEUE_RESET_OFFSET_THDORDEQr_SET(opq_rst_rdeq, 1);
        ioerr += (WRITE_OP_QUEUE_RESET_OFFSET_THDORDEQr(unit, idx, opq_rst_rdeq));

        OP_QUEUE_LIMIT_YELLOW_THDORDEQr_SET(opq_yel_rdeq, 7);
        ioerr += (WRITE_OP_QUEUE_LIMIT_YELLOW_THDORDEQr(unit, idx, opq_yel_rdeq));

        OP_QUEUE_LIMIT_RED_THDORDEQr_SET(opq_red_rdeq, 7);
        ioerr += (WRITE_OP_QUEUE_LIMIT_RED_THDORDEQr(unit, idx, opq_red_rdeq));

        OP_QUEUE_RESET_OFFSET_YELLOW_THDORDEQr_SET(opq_rst_yel_rdeq, 1);
        ioerr += (WRITE_OP_QUEUE_RESET_OFFSET_YELLOW_THDORDEQr(unit, idx, opq_rst_yel_rdeq));

        OP_QUEUE_RESET_OFFSET_RED_THDORDEQr_SET(opq_rst_red_rdeq, 1);
        ioerr += (WRITE_OP_QUEUE_RESET_OFFSET_RED_THDORDEQr(unit, idx, opq_rst_red_rdeq));
    }

    /* 2.5.2 RDEI */
    for (idx = 0; idx < NUM_RDEI_COS; idx++) {
        ioerr += (READ_OP_QUEUE_CONFIG1_THDORDEIr(unit, idx, &opq_cfg1_rdei));
        OP_QUEUE_CONFIG1_THDORDEIr_Q_MINf_SET(opq_cfg1_rdei, 9);
        OP_QUEUE_CONFIG1_THDORDEIr_Q_COLOR_ENABLEf_SET(opq_cfg1_rdei, 0);
        OP_QUEUE_CONFIG1_THDORDEIr_Q_COLOR_DYNAMICf_SET(opq_cfg1_rdei, 1);
        ioerr += (WRITE_OP_QUEUE_CONFIG1_THDORDEIr(unit, idx, opq_cfg1_rdei));

        ioerr += (READ_OP_QUEUE_CONFIG_THDORDEIr(unit, idx, &opq_cfg_rdei));
        OP_QUEUE_CONFIG_THDORDEIr_Q_SHARED_LIMITf_SET(opq_cfg_rdei, 7);
        OP_QUEUE_CONFIG_THDORDEIr_Q_LIMIT_DYNAMICf_SET(opq_cfg_rdei, 1);
        OP_QUEUE_CONFIG_THDORDEIr_Q_LIMIT_ENABLEf_SET(opq_cfg_rdei, 0);
        ioerr += (WRITE_OP_QUEUE_CONFIG_THDORDEIr(unit, idx, opq_cfg_rdei));

        OP_QUEUE_RESET_OFFSET_THDORDEIr_SET(opq_rst_rdei, 2);
        ioerr += (WRITE_OP_QUEUE_RESET_OFFSET_THDORDEIr(unit, idx, opq_rst_rdei));

        OP_QUEUE_LIMIT_YELLOW_THDORDEIr_SET(opq_yel_rdei, 7);
        ioerr += (WRITE_OP_QUEUE_LIMIT_YELLOW_THDORDEIr(unit, idx, opq_yel_rdei));

        OP_QUEUE_LIMIT_RED_THDORDEIr_SET(opq_red_rdei, 7);
        ioerr += (WRITE_OP_QUEUE_LIMIT_RED_THDORDEIr(unit, idx, opq_red_rdei));

        OP_QUEUE_RESET_OFFSET_YELLOW_THDORDEIr_SET(opq_rst_yel_rdei, 2);
        ioerr += (WRITE_OP_QUEUE_RESET_OFFSET_YELLOW_THDORDEIr(unit, idx, opq_rst_yel_rdei));

        OP_QUEUE_RESET_OFFSET_RED_THDORDEIr_SET(opq_rst_red_rdei, 2);
        ioerr += (WRITE_OP_QUEUE_RESET_OFFSET_RED_THDORDEIr(unit, idx, opq_rst_red_rdei));
    }

    THDIEXT_PORT_PAUSE_ENABLE_64r_SET(iext_pause_enable, 0, 0xffffffff);
    THDIEXT_PORT_PAUSE_ENABLE_64r_SET(iext_pause_enable, 1, 0x3ff);
    ioerr += WRITE_THDIEXT_PORT_PAUSE_ENABLE_64r(unit, iext_pause_enable);
    THDIQEN_PORT_PAUSE_ENABLE_64r_SET(iqen_pause_enable, 0, 0xffffffff);
    THDIQEN_PORT_PAUSE_ENABLE_64r_SET(iqen_pause_enable, 1, 0x3ff);
    ioerr += WRITE_THDIQEN_PORT_PAUSE_ENABLE_64r(unit, iqen_pause_enable);
    THDIEMA_PORT_PAUSE_ENABLE_64r_SET(iema_pause_enable, 0, 0xffffffff);
    THDIEMA_PORT_PAUSE_ENABLE_64r_SET(iema_pause_enable, 1, 0x3ff);
    ioerr += WRITE_THDIEMA_PORT_PAUSE_ENABLE_64r(unit, iema_pause_enable);
    PORT_PAUSE_ENABLE_64r_SET(port_pause_enable, 0, 0xffffffff);
    PORT_PAUSE_ENABLE_64r_SET(port_pause_enable, 1, 0x3ff);
    ioerr += WRITE_PORT_PAUSE_ENABLE_64r(unit, port_pause_enable);

    /* Initialize MMU internal/external aging limit memory */
    MMU_AGING_LMT_INTm_CLR(age_int);
    MMU_AGING_LMT_EXTm_CLR(age_ext);
    for (idx = 0; idx < MMU_AGING_LMT_INTm_MAX; idx++) {
        ioerr += (WRITE_MMU_AGING_LMT_INTm(unit, idx, age_int));
    }

    for (idx = 0; idx < MMU_AGING_LMT_EXTm_MAX; idx++) {
        ioerr += (WRITE_MMU_AGING_LMT_EXTm(unit, idx, age_ext));
    }

    /* Enable all ports */
    INPUT_PORT_RX_ENABLE_64r_SET(port_rx_enable, 0, 0xffffffff);
    INPUT_PORT_RX_ENABLE_64r_SET(port_rx_enable, 1, 0x3ff);
    ioerr += WRITE_INPUT_PORT_RX_ENABLE_64r(unit, port_rx_enable);
    THDIEMA_INPUT_PORT_RX_ENABLE_64r_SET(iema_rx_enable, 0, 0xffffffff);
    THDIEMA_INPUT_PORT_RX_ENABLE_64r_SET(iema_rx_enable, 1, 0x3ff);
    ioerr += WRITE_THDIEMA_INPUT_PORT_RX_ENABLE_64r(unit, iema_rx_enable);
    THDIEXT_INPUT_PORT_RX_ENABLE_64r_SET(iext_rx_enable, 0, 0xffffffff);
    THDIEXT_INPUT_PORT_RX_ENABLE_64r_SET(iext_rx_enable, 1, 0x3ff);
    ioerr += WRITE_THDIEXT_INPUT_PORT_RX_ENABLE_64r(unit, iext_rx_enable);
    THDIQEN_INPUT_PORT_RX_ENABLE_64r_SET(iqen_rx_enable, 0, 0xffffffff);
    THDIQEN_INPUT_PORT_RX_ENABLE_64r_SET(iqen_rx_enable, 1, 0x3ff);
    ioerr += WRITE_THDIQEN_INPUT_PORT_RX_ENABLE_64r(unit, iqen_rx_enable);
    THDIRQE_INPUT_PORT_RX_ENABLE_64r_SET(irqe_rx_enable, 0, 0xffffffff);
    THDIRQE_INPUT_PORT_RX_ENABLE_64r_SET(irqe_rx_enable, 1, 0x3ff);
    ioerr += WRITE_THDIRQE_INPUT_PORT_RX_ENABLE_64r(unit, irqe_rx_enable);

    return ioerr;
}

static int
_port_init(int unit, int port)
{
    int ioerr = 0;
    int ppport = P2PP(unit, port);
    int lport = P2L(unit, port);
    PP_PORT_TO_PHYSICAL_PORT_MAPm_t pp_to_phy_port;
    ING_PHYSICAL_PORT_TABLEm_t ing_port;
    EGR_ENABLEm_t egr_enable;
    EGR_PORTm_t egr_port;
    EGR_VLAN_CONTROL_1r_t egr_vlan_ctrl1;
    PORT_TABm_t port_tab;
    
    ioerr += READ_PP_PORT_TO_PHYSICAL_PORT_MAPm(unit, ppport, &pp_to_phy_port);
    PP_PORT_TO_PHYSICAL_PORT_MAPm_DESTINATIONf_SET(pp_to_phy_port, lport);
    ioerr += WRITE_PP_PORT_TO_PHYSICAL_PORT_MAPm(unit, ppport, pp_to_phy_port);

    ioerr += READ_ING_PHYSICAL_PORT_TABLEm(unit, lport, &ing_port);
    ING_PHYSICAL_PORT_TABLEm_PP_PORTf_SET(ing_port, ppport);
    ioerr += WRITE_ING_PHYSICAL_PORT_TABLEm(unit, lport, ing_port);

    /* Default port VLAN and tag action, enable L2 HW learning */
    PORT_TABm_CLR(port_tab);
    PORT_TABm_PORT_VIDf_SET(port_tab, 1);
    PORT_TABm_FILTER_ENABLEf_SET(port_tab, 1);
    PORT_TABm_OUTER_TPID_ENABLEf_SET(port_tab, 1);
    PORT_TABm_CML_FLAGS_NEWf_SET(port_tab, 8);
    PORT_TABm_CML_FLAGS_MOVEf_SET(port_tab, 8);
    ioerr += WRITE_PORT_TABm(unit, ppport, port_tab);

    /* Filter VLAN on egress */
    ioerr += READ_EGR_PORTm(unit, ppport, &egr_port);
    EGR_PORTm_EN_EFILTERf_SET(egr_port, 1);
    ioerr += WRITE_EGR_PORTm(unit, ppport, egr_port);

    /* Configure egress VLAN for backward compatibility */
    ioerr += READ_EGR_VLAN_CONTROL_1r(unit, ppport, &egr_vlan_ctrl1);
    EGR_VLAN_CONTROL_1r_VT_MISS_UNTAGf_SET(egr_vlan_ctrl1, 0);
    EGR_VLAN_CONTROL_1r_REMARK_OUTER_DOT1Pf_SET(egr_vlan_ctrl1, 1);
    ioerr += WRITE_EGR_VLAN_CONTROL_1r(unit, ppport, egr_vlan_ctrl1);

    /* Egress enable */
    ioerr += READ_EGR_ENABLEm(unit, lport, &egr_enable);
    EGR_ENABLEm_PRT_ENABLEf_SET(egr_enable, 1);
    ioerr += WRITE_EGR_ENABLEm(unit, lport, egr_enable);

    return ioerr;
}

int
bcm56450_a0_xport_init(int unit, int port)
{
    int ioerr = 0;
    int blkidx, subport;
    int phy_mode, core_mode, wc_sel = 0;
    int lanes;
    int speed_max;
    int port_enable;
    XPORT_PORT_ENABLEr_t xport_enable;
    XPORT_MODE_REGr_t xport_mode;
    MAC_RSV_MASKr_t mac_rsv_mask;
    TXLP_PORT_ENABLEr_t txlp_enable;
    
    /* Common GPORT initialization */
    ioerr += _port_init(unit, port);  

    /* Update XLPORT mode according to speed */
    blkidx = MXQPORT_BLKIDX(unit, port);
    subport = MXQPORT_SUBPORT(unit, port);
    speed_max = bcm56450_a0_port_speed_max(unit, port);

    lanes = bcm56450_a0_port_num_lanes(unit, port);
    if (lanes == 4) {
        core_mode = 0; /* Single */
    } else if (lanes == 2) {
        core_mode = 1; /* Dual */
    } else {
        core_mode = 2; /* Quad */
    }
    
    /* Get the phy_mode and wc_sel flag */
    ioerr += bcm56450_a0_phy_mode_get(unit, port, speed_max, &phy_mode, &wc_sel);
    
    XPORT_MODE_REGr_CLR(xport_mode);
    XPORT_MODE_REGr_CORE_PORT_MODEf_SET(xport_mode, core_mode);
    XPORT_MODE_REGr_WC_10G_21G_SELf_SET(xport_mode, wc_sel);
    XPORT_MODE_REGr_PHY_PORT_MODEf_SET(xport_mode, phy_mode);
    XPORT_MODE_REGr_PORT_GMII_MII_ENABLEf_SET(xport_mode, ((speed_max >= 10000) ? 0 : 1));
    ioerr += WRITE_XPORT_MODE_REGr(unit, xport_mode, port);
    
    ioerr += READ_MAC_RSV_MASKr(unit, port, &mac_rsv_mask);
    MAC_RSV_MASKr_MASKf_SET(mac_rsv_mask, 0x58);
    ioerr += WRITE_MAC_RSV_MASKr(unit, port, mac_rsv_mask);

    if ((blkidx == 8) || (blkidx == 9)) {
        ioerr += READ_TXLP_PORT_ENABLEr(unit, (blkidx - 8), &txlp_enable);
        port_enable = TXLP_PORT_ENABLEr_PORT_ENABLEf_GET(txlp_enable);
        TXLP_PORT_ENABLEr_PORT_ENABLEf_SET(txlp_enable, (port_enable | (1 << subport)));
        ioerr += WRITE_TXLP_PORT_ENABLEr(unit, (blkidx - 8), txlp_enable);
    }
    
    ioerr += READ_XPORT_PORT_ENABLEr(unit, &xport_enable, port);
    if (subport == 0) {
        XPORT_PORT_ENABLEr_PORT0f_SET(xport_enable, 1);
    } else if (subport == 1) {
        XPORT_PORT_ENABLEr_PORT1f_SET(xport_enable, 1);
    } else if (subport == 2) {
        XPORT_PORT_ENABLEr_PORT2f_SET(xport_enable, 1);
    } else if (subport == 3) {
        XPORT_PORT_ENABLEr_PORT3f_SET(xport_enable, 1);
    }
    ioerr += WRITE_XPORT_PORT_ENABLEr(unit, xport_enable, port);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

int
bcm56450_a0_bmd_init(int unit)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int port, ppport, speed_max;
    int idx, blkidx, subidx;
    int start_addr, end_addr;
    cdk_pbmp_t pbmp;
    ING_HW_RESET_CONTROL_1r_t ing_rst_ctl_1;
    ING_HW_RESET_CONTROL_2r_t ing_rst_ctl_2;
    EGR_HW_RESET_CONTROL_0r_t egr_rst_ctl_0;
    EGR_HW_RESET_CONTROL_1r_t egr_rst_ctl_1;
    EGR_VLAN_CONTROL_1r_t vlan_control;
    PP_PORT_GPP_TRANSLATION_1m_t pp_gpp_tran_1;
    PP_PORT_GPP_TRANSLATION_2m_t pp_gpp_tran_2;
    PP_PORT_GPP_TRANSLATION_3m_t pp_gpp_tran_3;
    PP_PORT_GPP_TRANSLATION_4m_t pp_gpp_tran_4;
    EGR_PP_PORT_GPP_TRANSLATION_1m_t egr_pp_gpp_tran_1;
    EGR_PP_PORT_GPP_TRANSLATION_2m_t egr_pp_gpp_tran_2;
    EGR_IPMC_CFG2r_t ipmc_cfg;
    IARB_TDM_CONTROLr_t iarb_tdm_ctrl;
    MISCCONFIGr_t misc;
    RDBGC0_SELECTr_t rdbgc0_select;
    TDBGC_SELECTr_t tdbgc_select;
    VLAN_PROFILE_TABm_t vlan_profile;
    ING_VLAN_TAG_ACTION_PROFILEm_t vlan_action;
    EGR_VLAN_TAG_ACTION_PROFILEm_t egr_action;
    XPORT_MIB_RESETr_t mib_reset;
    XPORT_XMAC_CONTROLr_t xp_xmac_ctrl;
    TXLP_PORT_ADDR_MAP_TABLEm_t txlp_port_map;

    BMD_CHECK_UNIT(unit);

    /* Reset the IPIPE block */
    ING_HW_RESET_CONTROL_1r_CLR(ing_rst_ctl_1);
    ioerr += WRITE_ING_HW_RESET_CONTROL_1r(unit, ing_rst_ctl_1);
    ING_HW_RESET_CONTROL_2r_CLR(ing_rst_ctl_2);
    ING_HW_RESET_CONTROL_2r_RESET_ALLf_SET(ing_rst_ctl_2, 1);
    ING_HW_RESET_CONTROL_2r_VALIDf_SET(ing_rst_ctl_2, 1);
    ING_HW_RESET_CONTROL_2r_COUNTf_SET(ing_rst_ctl_2, 0x8000);
    ioerr += WRITE_ING_HW_RESET_CONTROL_2r(unit, ing_rst_ctl_2);

    /* Reset the EPIPE block */
    EGR_HW_RESET_CONTROL_0r_CLR(egr_rst_ctl_0);
    ioerr += WRITE_EGR_HW_RESET_CONTROL_0r(unit, egr_rst_ctl_0);

    ioerr += READ_EGR_HW_RESET_CONTROL_1r(unit, &egr_rst_ctl_1);
    EGR_HW_RESET_CONTROL_1r_RESET_ALLf_SET(egr_rst_ctl_1, 1);
    EGR_HW_RESET_CONTROL_1r_VALIDf_SET(egr_rst_ctl_1, 1);
    ioerr += WRITE_EGR_HW_RESET_CONTROL_1r(unit, egr_rst_ctl_1);
    
    for (idx = 0; idx < PIPE_RESET_TIMEOUT_MSEC; idx++) {
        ioerr += READ_ING_HW_RESET_CONTROL_2r(unit, &ing_rst_ctl_2);
        if (ING_HW_RESET_CONTROL_2r_DONEf_GET(ing_rst_ctl_2)) {
            break;
        }
        BMD_SYS_USLEEP(1000);
    }
    if (idx >= PIPE_RESET_TIMEOUT_MSEC) {
        CDK_WARN(("bcm56450_a0_bmd_init[%d]: IPIPE reset timeout\n", unit));
        return ioerr ? CDK_E_IO : CDK_E_TIMEOUT;
    }
        
    for (idx = 0; idx < PIPE_RESET_TIMEOUT_MSEC; idx++) {
        ioerr += READ_EGR_HW_RESET_CONTROL_1r(unit, &egr_rst_ctl_1);
        if (EGR_HW_RESET_CONTROL_1r_DONEf_GET(egr_rst_ctl_1)) {
            break;
        }
        BMD_SYS_USLEEP(1000);
    }
    if (idx >= PIPE_RESET_TIMEOUT_MSEC) {
        CDK_WARN(("bcm56450_a0_bmd_init[%d]: EPIPE reset timeout\n", unit));
        return ioerr ? CDK_E_IO : CDK_E_TIMEOUT;
    }

    /* Clear pipe reset registers */
    ioerr += READ_ING_HW_RESET_CONTROL_2r(unit, &ing_rst_ctl_2);
    ING_HW_RESET_CONTROL_2r_RESET_ALLf_SET(ing_rst_ctl_2, 0);
    ING_HW_RESET_CONTROL_2r_CMIC_REQ_ENABLEf_SET(ing_rst_ctl_2, 1);
    ioerr += WRITE_ING_HW_RESET_CONTROL_2r(unit, ing_rst_ctl_2);
    ioerr += READ_EGR_HW_RESET_CONTROL_1r(unit, &egr_rst_ctl_1);
    EGR_HW_RESET_CONTROL_1r_RESET_ALLf_SET(egr_rst_ctl_1, 0);
    ioerr += WRITE_EGR_HW_RESET_CONTROL_1r(unit, egr_rst_ctl_1);

    ioerr += CDK_XGSM_MEM_CLEAR(unit, RXLP_INTERNAL_STREAM_MAP_PORT_0m);
    ioerr += CDK_XGSM_MEM_CLEAR(unit, RXLP_INTERNAL_STREAM_MAP_PORT_1m);
    ioerr += CDK_XGSM_MEM_CLEAR(unit, RXLP_INTERNAL_STREAM_MAP_PORT_2m);
    ioerr += CDK_XGSM_MEM_CLEAR(unit, RXLP_INTERNAL_STREAM_MAP_PORT_3m);
    ioerr += CDK_XGSM_MEM_CLEAR(unit, TXLP_PORT_STREAM_BITMAP_TABLEm);
    ioerr += CDK_XGSM_MEM_CLEAR(unit, TXLP_INT2EXT_STREAM_MAP_TABLEm);
    ioerr += CDK_XGSM_MEM_CLEAR(unit, DEVICE_STREAM_ID_TO_PP_PORT_MAPm);
    ioerr += CDK_XGSM_MEM_CLEAR(unit, PP_PORT_TO_PHYSICAL_PORT_MAPm);

    /* Some registers are implemented in memory, need to clear them in order
     * to have correct parity value */
    EGR_IPMC_CFG2r_CLR(ipmc_cfg);
    EGR_VLAN_CONTROL_1r_CLR(vlan_control);
    
    bcm56450_a0_mxqport_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        ppport = P2PP(unit, port);
        if (ppport >= 0) {
            ioerr += WRITE_EGR_VLAN_CONTROL_1r(unit, ppport, vlan_control);
            ioerr += WRITE_EGR_IPMC_CFG2r(unit, ppport, ipmc_cfg);
        }
    }
    ioerr += WRITE_EGR_VLAN_CONTROL_1r(unit, CMIC_PORT, vlan_control);
    ioerr += WRITE_EGR_IPMC_CFG2r(unit, CMIC_PORT, ipmc_cfg);

    /* Update the moduleid */
    PP_PORT_GPP_TRANSLATION_1m_CLR(pp_gpp_tran_1);
    PP_PORT_GPP_TRANSLATION_2m_CLR(pp_gpp_tran_2);
    PP_PORT_GPP_TRANSLATION_3m_CLR(pp_gpp_tran_3);
    PP_PORT_GPP_TRANSLATION_4m_CLR(pp_gpp_tran_4);
    EGR_PP_PORT_GPP_TRANSLATION_1m_CLR(egr_pp_gpp_tran_1);
    EGR_PP_PORT_GPP_TRANSLATION_2m_CLR(egr_pp_gpp_tran_2);
    PP_PORT_GPP_TRANSLATION_1m_MODID_0_VALIDf_SET(pp_gpp_tran_1, 1);
    PP_PORT_GPP_TRANSLATION_2m_MODID_0_VALIDf_SET(pp_gpp_tran_2, 1);
    PP_PORT_GPP_TRANSLATION_3m_MODID_0_VALIDf_SET(pp_gpp_tran_3, 1);
    PP_PORT_GPP_TRANSLATION_4m_MODID_0_VALIDf_SET(pp_gpp_tran_4, 1);
    EGR_PP_PORT_GPP_TRANSLATION_1m_MODID_0_VALIDf_SET(egr_pp_gpp_tran_1, 1);
    EGR_PP_PORT_GPP_TRANSLATION_2m_MODID_0_VALIDf_SET(egr_pp_gpp_tran_2, 1);
    ioerr += WRITE_PP_PORT_GPP_TRANSLATION_1m(unit, 0, pp_gpp_tran_1);
    ioerr += WRITE_PP_PORT_GPP_TRANSLATION_2m(unit, 0, pp_gpp_tran_2);
    ioerr += WRITE_PP_PORT_GPP_TRANSLATION_3m(unit, 0, pp_gpp_tran_3);
    ioerr += WRITE_PP_PORT_GPP_TRANSLATION_4m(unit, 0, pp_gpp_tran_4);
    ioerr += WRITE_EGR_PP_PORT_GPP_TRANSLATION_1m(unit, 0, egr_pp_gpp_tran_1);
    ioerr += WRITE_EGR_PP_PORT_GPP_TRANSLATION_2m(unit, 0, egr_pp_gpp_tran_2);
    
    /* Setup main TDM control */
    ioerr += READ_IARB_TDM_CONTROLr(unit, &iarb_tdm_ctrl);
    IARB_TDM_CONTROLr_DISABLEf_SET(iarb_tdm_ctrl, 0);
    IARB_TDM_CONTROLr_TDM_WRAP_PTRf_SET(iarb_tdm_ctrl, 32);
    ioerr += WRITE_IARB_TDM_CONTROLr(unit, iarb_tdm_ctrl);

    /* Enable Field Processor metering clock */
    ioerr += READ_MISCCONFIGr(unit, &misc);
    MISCCONFIGr_METERING_CLK_ENf_SET(misc, 1);
    ioerr += WRITE_MISCCONFIGr(unit, misc);

    /* TXLP_PORT_ADDR_MAP_TABLE */
    for (blkidx = 8; blkidx <= 9; blkidx++) {
        for (subidx = 0, start_addr = 0; subidx < MXQPORTS_PER_BLOCK; subidx++) {
            if ((port = bcm56450_a0_mxqport_from_index(unit, blkidx, subidx)) <= 0) {
                continue;
            }
            speed_max = bcm56450_a0_port_speed_max(unit, port);
            
            if (speed_max <= 2500) {
                end_addr = start_addr + (( 6 * 4) - 1); /* 6 Cells */
            } else if (speed_max <= 10000) {
                end_addr = start_addr + ((12 * 4) - 1); /* 12 Cells */
            } else if (speed_max <= 13000) {
                end_addr = start_addr + ((16 * 4) - 1); /* 16 Cells */
            } else if (speed_max <= 21000) {
                end_addr = start_addr + ((20 * 4) - 1); /* 20 Cells */
            } else {
                return CDK_E_INTERNAL;
            }

            TXLP_PORT_ADDR_MAP_TABLEm_CLR(txlp_port_map);
            TXLP_PORT_ADDR_MAP_TABLEm_START_ADDRf_SET(txlp_port_map, start_addr);
            TXLP_PORT_ADDR_MAP_TABLEm_END_ADDRf_SET(txlp_port_map, end_addr);
            ioerr += WRITE_TXLP_PORT_ADDR_MAP_TABLEm(unit, (blkidx - 8), subidx, txlp_port_map);
            start_addr = end_addr + 1;
        }
    }
            
    /* Configure discard counter */
    RDBGC0_SELECTr_BITMAPf_SET(rdbgc0_select, 0x0400ad11);
    TDBGC_SELECTr_BITMAPf_SET(tdbgc_select, 0xffffffff);
    ioerr += WRITE_RDBGC0_SELECTr(unit, rdbgc0_select);
    ioerr += WRITE_TDBGC_SELECTr(unit, 0, tdbgc_select);
 
    /* Initialize MMU */
    ioerr += _mmu_init(unit);
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

    /* Clear MIB counters in MXQPORT blocks */
    bcm56450_a0_mxqport_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        XPORT_MIB_RESETr_CLR(mib_reset);
        XPORT_MIB_RESETr_CLR_CNTf_SET(mib_reset, 0xf);
        ioerr += WRITE_XPORT_MIB_RESETr(unit, mib_reset, port);
        XPORT_MIB_RESETr_CLR_CNTf_SET(mib_reset, 0);
        ioerr += WRITE_XPORT_MIB_RESETr(unit, mib_reset, port);
    }
    
    /* Configure XQPORTs */
    bcm56450_a0_mxqport_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        blkidx = MXQPORT_BLKIDX(unit, port);
        subidx = MXQPORT_SUBPORT(unit, port);

        if (CDK_SUCCESS(rv)) {
            rv = bmd_phy_probe(unit, port);
        }

        if (((blkidx == 6 || blkidx == 8) && 
             (bcm56450_a0_phy_connection_mode(unit, 6) == PHY_CONN_WARPCORE)) ||
            ((blkidx == 7 || blkidx == 9) &&
             (bcm56450_a0_phy_connection_mode(unit, 7) == PHY_CONN_WARPCORE))) {
            rv = bmd_phy_mode_set(unit, port, "warpcore",
                                        BMD_PHY_MODE_2LANE, 0);
            rv = bmd_phy_mode_set(unit, port, "warpcore",
                                        BMD_PHY_MODE_SERDES, 1);
        } else if (blkidx == 8 || blkidx == 9) {    
            speed_max = bcm56450_a0_port_speed_max(unit, port);
            if (speed_max <= 10000) {
                rv = bmd_phy_mode_set(unit, port, "warpcore",
                                            BMD_PHY_MODE_SERDES, 1);
                rv = bmd_phy_mode_set(unit, port, "warpcore",
                                            BMD_PHY_MODE_2LANE, 0);
            } else {
                rv = bmd_phy_mode_set(unit, port, "warpcore", 
                                            BMD_PHY_MODE_SERDES, 0);
            }    
        }
        
        if (CDK_SUCCESS(rv)) {
            rv = bmd_phy_init(unit, port);
        }
    }
    
    /* Configure XLPORTs */
    bcm56450_a0_mxqport_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        /* Clear MAC hard reset after warpcore is initialized */
        if (MXQPORT_SUBPORT(unit, port) == 0) {
            XPORT_XMAC_CONTROLr_CLR(xp_xmac_ctrl);
            ioerr += WRITE_XPORT_XMAC_CONTROLr(unit, xp_xmac_ctrl, port);
        }
        /* Initialize XLPORTs after XLMAC is out of reset */
        ioerr += bcm56450_a0_xport_init(unit, port);
    }
 
#if BMD_CONFIG_INCLUDE_DMA
    /* Common port initialization for CPU port */
    ioerr += _port_init(unit, CMIC_PORT);
    if (CDK_SUCCESS(rv)) {
        rv = bmd_xgsm_dma_init(unit);
    }

    /* Enable all 48 CPU COS queues for Rx DMA channel */
    if (CDK_SUCCESS(rv)) {
        CMIC_CMC_COS_CTRL_RX_0r_t cos_ctrl_0;
        CMIC_CMC_COS_CTRL_RX_1r_t cos_ctrl_1;
        uint32_t cos_bmp;

        CMIC_CMC_COS_CTRL_RX_0r_CLR(cos_ctrl_0);
        for (idx = 0; idx < 4; idx++) {
            cos_bmp = (idx == XGSM_DMA_RX_CHAN) ? 0xffffffff : 0;
            CMIC_CMC_COS_CTRL_RX_0r_COS_BMPf_SET(cos_ctrl_0, cos_bmp);
            ioerr += WRITE_CMIC_CMC_COS_CTRL_RX_0r(unit, idx, cos_ctrl_0);
        }

        CMIC_CMC_COS_CTRL_RX_1r_CLR(cos_ctrl_1);
        for (idx = 0; idx < 4; idx++) {
            cos_bmp = (idx == XGSM_DMA_RX_CHAN) ? 0xffff : 0;
            CMIC_CMC_COS_CTRL_RX_1r_COS_BMPf_SET(cos_ctrl_1, cos_bmp);
            ioerr += WRITE_CMIC_CMC_COS_CTRL_RX_1r(unit, idx, cos_ctrl_1);
        }

        if (CDK_DEV_FLAGS(unit) & CDK_DEV_MBUS_PCI) {
            CMIC_CMC_HOSTMEM_ADDR_REMAPr_t hostmem_remap;
            uint32_t remap_val[] = { 0x144D2450, 0x19617595, 0x1E75C6DA, 0x1f };

            /* Send DMA data to external host memory when on PCI bus */
            for (idx = 0; idx < COUNTOF(remap_val); idx++) {
                CMIC_CMC_HOSTMEM_ADDR_REMAPr_SET(hostmem_remap, remap_val[idx]);
                ioerr += WRITE_CMIC_CMC_HOSTMEM_ADDR_REMAPr(unit, idx, hostmem_remap);
            }
        }
    }
#endif /* BMD_CONFIG_INCLUDE_DMA */

    return ioerr ? CDK_E_IO : rv;
}
#endif /* CDK_CONFIG_INCLUDE_BCM56450_A0 */
