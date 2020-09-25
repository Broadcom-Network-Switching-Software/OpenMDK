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
#include <bmd/bmd_device.h>
#include <cdk/arch/xgsd_chip.h>
#include <cdk/cdk_debug.h>
#include <cdk/cdk_util.h>
#include <cdk/chip/bcm56260_b0_defs.h>
#include <bmd/bmd_phy_ctrl.h>
#include <bmdi/arch/xgsd_dma.h>
#include "bcm56260_b0_bmd.h"
#include "bcm56260_b0_internal.h"

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

STATIC int
_mmu_tdm_init(int unit)
{
    int ioerr = 0;
    const int *default_tdm_seq;
    int tdm_seq[168];
    int idx, tdm_size;
    IARB_TDM_TABLEm_t iarb_tdm_table;
    IARB_TDM_CONTROLr_t iarb_tdm_ctrl;
    LLS_PORT_TDMm_t  lls_tdm;
    LLS_TDM_CAL_CFGr_t cal_cfg;
    
    /* Get default TDM sequence for this configuration */
    tdm_size = bcm56260_a0_tdm_default(unit, &default_tdm_seq);
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

        if ((idx % 2) == 0) {
            /* Two entries per mem entry */
            LLS_PORT_TDMm_CLR(lls_tdm);
            LLS_PORT_TDMm_PORT_ID_0f_SET(lls_tdm, tdm_seq[idx]);
            LLS_PORT_TDMm_PORT_ID_0_ENABLEf_SET(lls_tdm, 
                                                ((tdm_seq[idx] < 30) ? 1 : 0));
        } else {
            LLS_PORT_TDMm_PORT_ID_1f_SET(lls_tdm, tdm_seq[idx]);
            LLS_PORT_TDMm_PORT_ID_1_ENABLEf_SET(lls_tdm, 
                                                ((tdm_seq[idx] < 30) ? 1 : 0));
            ioerr += WRITE_LLS_PORT_TDMm(unit, (idx / 2), lls_tdm);
        }
    }
    if (tdm_size % 2) {
        ioerr += WRITE_LLS_PORT_TDMm(unit, (idx / 2), lls_tdm);
    }

    ioerr += READ_IARB_TDM_CONTROLr(unit, &iarb_tdm_ctrl);
    IARB_TDM_CONTROLr_DISABLEf_SET(iarb_tdm_ctrl, 0);
    IARB_TDM_CONTROLr_TDM_WRAP_PTRf_SET(iarb_tdm_ctrl, tdm_size - 1);
    IARB_TDM_CONTROLr_IDLE_PORT_NUM_SELf_SET(iarb_tdm_ctrl, 1);
    ioerr += WRITE_IARB_TDM_CONTROLr(unit, iarb_tdm_ctrl);

    LLS_TDM_CAL_CFGr_CLR(cal_cfg);
    LLS_TDM_CAL_CFGr_END_Af_SET(cal_cfg, tdm_size - 1);
    LLS_TDM_CAL_CFGr_END_Bf_SET(cal_cfg, tdm_size - 1);
    LLS_TDM_CAL_CFGr_ENABLEf_SET(cal_cfg, 1);
    ioerr += WRITE_LLS_TDM_CAL_CFGr(unit, cal_cfg);
    
    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

static int
_is_one_pg_port(int mport)
{
    if ((mport > 9 && mport < 13) ||
        (mport > 13 && mport < 17) ||
        (mport > 17 && mport < 21)) {
        return 1;
    }

    return 0;
}

static int
_mmu_init(int unit)
{
    int rv = 0;
    int ioerr = 0;
    int idx = 0;
    LLS_SOFT_RESETr_t soft_reset;
    LLS_INITr_t lls_init;
    INPUT_PORT_RX_ENABLE_64r_t port_rx_enable;
    DEQ_EFIFO_CFG_COMPLETEr_t deq_efifo_cfg_complete;
    THDIEXT_INPUT_PORT_RX_ENABLE_64r_t iext_rx_enable;
    THDIEMA_INPUT_PORT_RX_ENABLE_64r_t iema_rx_enable;
    THDIRQE_INPUT_PORT_RX_ENABLE_64r_t irqe_rx_enable;
    THDIQEN_INPUT_PORT_RX_ENABLE_64r_t iqen_rx_enable;
    THDI_BYPASSr_t thdi_bpass;
    THDIQEN_THDI_BYPASSr_t iqen_bpass;
    THDIRQE_THDI_BYPASSr_t irqe_bpass;
    THDIEXT_THDI_BYPASSr_t iext_bpass;
    THDIEMA_THDI_BYPASSr_t iema_bpass;
    THDO_BYPASSr_t thdo_bpass;
    int port, mport;
    int subidx;
    int nxtaddr = 0;
    int speed_max;
    DEQ_EFIFO_CFGr_t deq_efifo;
    TOQ_PORT_BW_CTRLr_t toq_port_bw_ctrl;
    TOQ_BW_LIMITING_MIDPKT_ENr_t toq_bw_limiting_midpkt_en;
    TOQ_EXT_MEM_BW_MAP_TABLEr_t toq_ext_mem; 
    RQE_SCHEDULER_WEIGHT_L0_QUEUEr_t rqe_schdw_l0;
    RQE_SCHEDULER_WEIGHT_L1_QUEUEr_t rqe_schdw_l1;
    RQE_SCHEDULER_CONFIGr_t rqe_schd_cfg;
    LLS_CONFIG0r_t lls_config;
    LLS_MAX_REFRESH_ENABLEr_t lls_max_refresh;
    LLS_MIN_REFRESH_ENABLEr_t lls_min_refresh;
    int ppport;
    int lport;
    LLS_L0_PARENTm_t l0_parent;
    LLS_PORT_CONFIGm_t lls_port_cfg;
    LLS_L1_PARENTm_t l1_parent;
    LLS_L0_CONFIGm_t l0_config;
    LLS_L1_CONFIGm_t l1_config;
    LLS_L2_PARENTm_t l2_parent;
    EGR_QUEUE_TO_PP_PORT_MAPm_t egrq_pp_map;
    ING_COS_MODEr_t cos_mode;
    RQE_PP_PORT_CONFIGr_t rqe_port;
    int op_node, ceil, q_offset, q_limit;
    cdk_pbmp_t pbmp;
    cdk_pbmp_t mmu_pbmp;    
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
    MMU_ENQ_IP_PRI_TO_PG_PROFILE_0r_t pri_to_pg_profile;
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
    MMU_ENQ_PACKING_REAS_FIFO_THRESH_PROFILE_1r_t mmu_eprf_thr_prof1;
    MMU_ENQ_PROFILE_3_PRI_GRPr_t mmu_profile_3_pri_gpr;
    THDIEXT_PROFILE3_PRI_GRPr_t thdiext_profile3_pri_gpr;
    THDIEMA_PROFILE3_PRI_GRPr_t thdiema_profile3_pri_gpr;
    PROFILE3_PRI_GRPr_t profile_3_pri_gpr;
    CFAPICONFIGr_t cfapconfig;
    CFAPECONFIGr_t cfapeconfig;
    CFAPEFULLSETPOINTr_t cfapefullsetpoint;
    CFAPEFULLRESETPOINTr_t cfapefullresetpoint;
    MMU_ENQ_FAPCONFIG_0r_t mmu_enq_fapconfig_0;
    MMU_ENQ_FAPFULLSETPOINT_0r_t mmu_enq_FAPFULLSETPOINT_0;
    MMU_ENQ_FAPFULLRESETPOINT_0r_t mmu_enq_fapfullresetpoint_0;
    QSTRUCT_FAPCONFIGr_t qstruct_fapcfg;
    QSTRUCT_FAPFULLSETPOINTr_t qstruct_fapfullsetpoint;
    QSTRUCT_FAPFULLRESETPOINTr_t qstruct_fapfullresetpoint;
    TOP_SW_BOND_OVRD_CTRL1r_t top_sw_bond_ovrd_ctrl1;
    MMU_ENQ_PACKING_REAS_FIFO_PROFILE_THRESHr_t mmu_eprf_prof_thr;
    MMU_ENQ_PACKING_REAS_FIFO_THRESH_PROFILE_0r_t mmu_eprf_thr_prof0;
    WRED_MISCCONFIGr_t wred_misccfg;
    MISCCONFIGr_t misc;
    THDIRQE_PROFILE3_PRI_GRPr_t thdirqe_profile3_pri_gpr;
    THDIQEN_PROFILE3_PRI_GRPr_t thdiqen_profile3_pri_gpr;
    PORT_PROFILE_MAPr_t port_prf_map;
    THDIEXT_PORT_PROFILE_MAPr_t thidext_prf_map;
    THDIEMA_PORT_PROFILE_MAPr_t thidema_prf_map;
    THDIRQE_PORT_PROFILE_MAPr_t thidrqe_prf_map;
    THDIQEN_PORT_PROFILE_MAPr_t thidqen_prf_map;
    TXLP_PORT_ADDR_MAP_TABLEm_t txlp_port_map;
    int start_addr, end_addr;
    cdk_pbmp_t pbmp_all, mxqport_pbmp, xlport_pbmp;
    int priority_group;
    uint32_t fval[2] = {0, 0};
    
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
        CDK_WARN(("bcm56260_b0_bmd_init[%d]: LLS INIT timeout\n", unit));
        return ioerr ? CDK_E_IO : CDK_E_TIMEOUT;
    }

    /* Setup TDM for MMU Arb & LLS */
    rv = _mmu_tdm_init(unit);

    /* Enable all ports */
    INPUT_PORT_RX_ENABLE_64r_SET(port_rx_enable, 0x3FFFFFFF);
    ioerr += WRITE_INPUT_PORT_RX_ENABLE_64r(unit, port_rx_enable);
    
    THDIEMA_INPUT_PORT_RX_ENABLE_64r_SET(iema_rx_enable, 0x3FFFFFFF);
    ioerr += WRITE_THDIEMA_INPUT_PORT_RX_ENABLE_64r(unit, iema_rx_enable);
    THDIEXT_INPUT_PORT_RX_ENABLE_64r_SET(iext_rx_enable, 0x3FFFFFFF);
    ioerr += WRITE_THDIEXT_INPUT_PORT_RX_ENABLE_64r(unit, iext_rx_enable);
    THDIQEN_INPUT_PORT_RX_ENABLE_64r_SET(iqen_rx_enable, 0x3FFFFFFF);
    ioerr += WRITE_THDIQEN_INPUT_PORT_RX_ENABLE_64r(unit, iqen_rx_enable);
    THDIRQE_INPUT_PORT_RX_ENABLE_64r_SET(irqe_rx_enable, 0x3FFFFFFF);
    ioerr += WRITE_THDIRQE_INPUT_PORT_RX_ENABLE_64r(unit, irqe_rx_enable);

    THDI_BYPASSr_CLR(thdi_bpass);
    THDIRQE_THDI_BYPASSr_CLR(irqe_bpass);
    THDIEMA_THDI_BYPASSr_CLR(iema_bpass);
    THDIEXT_THDI_BYPASSr_CLR(iext_bpass);
    THDIQEN_THDI_BYPASSr_CLR(iqen_bpass);
    THDO_BYPASSr_CLR(thdo_bpass);
    ioerr += WRITE_THDI_BYPASSr(unit, thdi_bpass);
    ioerr += WRITE_THDIQEN_THDI_BYPASSr(unit, iqen_bpass);
    ioerr += WRITE_THDIRQE_THDI_BYPASSr(unit, irqe_bpass);
    ioerr += WRITE_THDIEXT_THDI_BYPASSr(unit, iext_bpass);
    ioerr += WRITE_THDIEMA_THDI_BYPASSr(unit, iema_bpass);
    ioerr += WRITE_THDO_BYPASSr(unit, thdo_bpass);

    DEQ_EFIFO_CFG_COMPLETEr_CLR(deq_efifo_cfg_complete);
    ioerr += WRITE_DEQ_EFIFO_CFG_COMPLETEr(unit, deq_efifo_cfg_complete);

    /* Config DEQ_EFIFO_CFG and TOQ_PORT_BW_CTRL */
    /* CMIC PORT */
    ioerr += READ_DEQ_EFIFO_CFGr(unit, CMIC_PORT, &deq_efifo);
    DEQ_EFIFO_CFGr_EGRESS_FIFO_START_ADDRESSf_SET(deq_efifo, nxtaddr);
    DEQ_EFIFO_CFGr_EGRESS_FIFO_XMIT_THRESHOLDf_SET(deq_efifo, 1);
    DEQ_EFIFO_CFGr_EGRESS_FIFO_DEPTHf_SET(deq_efifo, 2);
    ioerr += WRITE_DEQ_EFIFO_CFGr(unit, CMIC_PORT, deq_efifo);

    /* Port BW Ctrl */
    ioerr += READ_TOQ_PORT_BW_CTRLr(unit, CMIC_PORT, &toq_port_bw_ctrl);
    TOQ_PORT_BW_CTRLr_PORT_BWf_SET(toq_port_bw_ctrl, 63);
    TOQ_PORT_BW_CTRLr_START_THRESHOLDf_SET(toq_port_bw_ctrl, 1);
    ioerr += WRITE_TOQ_PORT_BW_CTRLr(unit, CMIC_PORT, toq_port_bw_ctrl);

    nxtaddr += 2;
    bcm56260_a0_mxqport_pbmp_get(unit, &mxqport_pbmp);
    bcm56260_a0_xlport_pbmp_get(unit, &xlport_pbmp);
    pbmp_all = mxqport_pbmp;
    CDK_PBMP_OR(pbmp_all, xlport_pbmp);    
    CDK_PBMP_ITER(pbmp_all, port) {
        speed_max = bcm56260_a0_port_speed_max(unit, port);
        mport = P2M(unit, port);
        
        ioerr += READ_DEQ_EFIFO_CFGr(unit, mport, &deq_efifo);
        DEQ_EFIFO_CFGr_EGRESS_FIFO_START_ADDRESSf_SET(deq_efifo, nxtaddr);
        DEQ_EFIFO_CFGr_EGRESS_FIFO_XMIT_THRESHOLDf_SET(deq_efifo, 1);
        if (speed_max <= 1000) {
            nxtaddr += 10;
            DEQ_EFIFO_CFGr_EGRESS_FIFO_DEPTHf_SET(deq_efifo, 2);
        } else if (speed_max <= 2500) {
            nxtaddr += 19;
            DEQ_EFIFO_CFGr_EGRESS_FIFO_DEPTHf_SET(deq_efifo, 19);
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
            TOQ_PORT_BW_CTRLr_PORT_BWf_SET(toq_port_bw_ctrl, 0x145);
            TOQ_PORT_BW_CTRLr_START_THRESHOLDf_SET(toq_port_bw_ctrl, 0x30);
        } else if (speed_max == 2500) {
            TOQ_PORT_BW_CTRLr_PORT_BWf_SET(toq_port_bw_ctrl, 0x3f);
            TOQ_PORT_BW_CTRLr_START_THRESHOLDf_SET(toq_port_bw_ctrl, 0xe);
        } else {
            TOQ_PORT_BW_CTRLr_PORT_BWf_SET(toq_port_bw_ctrl, 0x3f);
            TOQ_PORT_BW_CTRLr_START_THRESHOLDf_SET(toq_port_bw_ctrl, 0xe);
        }
        ioerr += WRITE_TOQ_PORT_BW_CTRLr(unit, mport, toq_port_bw_ctrl);
    }

    TOQ_BW_LIMITING_MIDPKT_ENr_CLR(toq_bw_limiting_midpkt_en);
    TOQ_BW_LIMITING_MIDPKT_ENr_MIDPKT_SHAPE_ENf_SET(toq_bw_limiting_midpkt_en, 
                                                        0x1FFFFFFE);
    ioerr += WRITE_TOQ_BW_LIMITING_MIDPKT_ENr(unit, toq_bw_limiting_midpkt_en);
    
    DEQ_EFIFO_CFG_COMPLETEr_CLR(deq_efifo_cfg_complete); 
    DEQ_EFIFO_CFG_COMPLETEr_EGRESS_FIFO_CONFIGURATION_COMPLETEf_SET(deq_efifo_cfg_complete, 1);
    ioerr += WRITE_DEQ_EFIFO_CFG_COMPLETEr(unit, deq_efifo_cfg_complete);

    for (idx = 0; idx < 16; idx++) {
        ioerr += READ_TOQ_EXT_MEM_BW_MAP_TABLEr(unit, idx, &toq_ext_mem);
        TOQ_EXT_MEM_BW_MAP_TABLEr_GBL_GUARENTEE_BW_LIMITf_SET(toq_ext_mem,0x7d0);
        TOQ_EXT_MEM_BW_MAP_TABLEr_WR_PHASEf_SET(toq_ext_mem, 0);
        TOQ_EXT_MEM_BW_MAP_TABLEr_RD_PHASEf_SET(toq_ext_mem, 0);
        ioerr += WRITE_TOQ_EXT_MEM_BW_MAP_TABLEr(unit, idx, toq_ext_mem);
    }

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
    RQE_SCHEDULER_CONFIGr_CLR(rqe_schd_cfg);
    RQE_SCHEDULER_CONFIGr_L0_MCM_MODEf_SET(rqe_schd_cfg, 1);
    RQE_SCHEDULER_CONFIGr_L0_CC_MODEf_SET(rqe_schd_cfg, 1);
    RQE_SCHEDULER_CONFIGr_L0_UCM_MODEf_SET(rqe_schd_cfg, 1);
    RQE_SCHEDULER_CONFIGr_L1_MODEf_SET(rqe_schd_cfg, 1);
    ioerr += WRITE_RQE_SCHEDULER_CONFIGr(unit, rqe_schd_cfg);

    RQE_SCHEDULER_WEIGHT_L0_QUEUEr_CLR(rqe_schdw_l0);
    RQE_SCHEDULER_WEIGHT_L0_QUEUEr_WRR_WEIGHTf_SET(rqe_schdw_l0, 1);
    for (idx = 0; idx < 12; idx++) {
        ioerr += WRITE_RQE_SCHEDULER_WEIGHT_L0_QUEUEr(unit, idx, rqe_schdw_l0);
    }

    RQE_SCHEDULER_WEIGHT_L1_QUEUEr_CLR(rqe_schdw_l1);
    RQE_SCHEDULER_WEIGHT_L1_QUEUEr_WRR_WEIGHTf_SET(rqe_schdw_l1, 1);
    for (idx = 0; idx < 3; idx++) {
        ioerr += WRITE_RQE_SCHEDULER_WEIGHT_L1_QUEUEr(unit, idx, rqe_schdw_l1);
    }

    /* LLS Queue Configuration */
    CDK_PBMP_CLEAR(mmu_pbmp);
    CDK_PBMP_ADD(mmu_pbmp, CMIC_PORT);
    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_MXQPORT, &pbmp);
    CDK_PBMP_OR(mmu_pbmp, pbmp);
    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_XLPORT, &pbmp);
    CDK_PBMP_OR(mmu_pbmp, pbmp);    
    CDK_PBMP_ITER(mmu_pbmp, port) {
    
        lport = P2L(unit, port);
        LLS_PORT_CONFIGm_CLR(lls_port_cfg);
        if (port == 0) {
            LLS_PORT_CONFIGm_P_NUM_SPRIf_SET(lls_port_cfg, 9);
            LLS_PORT_CONFIGm_P_START_SPRIf_SET(lls_port_cfg, port);
        } else {
            LLS_PORT_CONFIGm_P_NUM_SPRIf_SET(lls_port_cfg, 1);
            LLS_PORT_CONFIGm_P_START_SPRIf_SET(lls_port_cfg, port + 4);
        }
        ioerr += WRITE_LLS_PORT_CONFIGm(unit, lport, lls_port_cfg);

        LLS_L0_PARENTm_CLR(l0_parent);
        if (port == 0) {
            LLS_L0_PARENTm_C_TYPEf_SET(l0_parent, 1);
            LLS_L0_PARENTm_C_PARENTf_SET(l0_parent, lport);
            ioerr += WRITE_LLS_L0_PARENTm(unit, port, l0_parent);
        } else {
            LLS_L0_PARENTm_C_TYPEf_SET(l0_parent, 1);
            LLS_L0_PARENTm_C_PARENTf_SET(l0_parent, lport);
            ioerr += WRITE_LLS_L0_PARENTm(unit, (4 + port), l0_parent);
        }

        LLS_L0_CONFIGm_CLR(l0_config);
        if (port == 0) {
            LLS_L0_CONFIGm_P_NUM_SPRIf_SET(l0_config, 0xf);
            LLS_L0_CONFIGm_P_START_SPRIf_SET(l0_config, 0);
            ioerr += WRITE_LLS_L0_CONFIGm(unit, port, l0_config);
        } else {
            LLS_L0_CONFIGm_P_NUM_SPRIf_SET(l0_config, 1);
            LLS_L0_CONFIGm_P_START_SPRIf_SET(l0_config, (8 + port));
            ioerr += WRITE_LLS_L0_CONFIGm(unit, (4 + port), l0_config);
        }
        
        LLS_L1_PARENTm_CLR(l1_parent);
        if (port == 0) {
            LLS_L1_PARENTm_C_PARENTf_SET(l1_parent, 0);
            ioerr += WRITE_LLS_L1_PARENTm(unit, port, l1_parent);
        } else {
            LLS_L1_PARENTm_C_PARENTf_SET(l1_parent, (4 + port));
            ioerr += WRITE_LLS_L1_PARENTm(unit, (8 + port), l1_parent);
        }
        
        LLS_L1_CONFIGm_CLR(l1_config);
        LLS_L1_CONFIGm_P_NUM_SPRIf_SET(l1_config, 0xf);
        if (port == 0) {
            LLS_L1_CONFIGm_P_START_UC_SPRIf_SET(l1_config, 0);
            ioerr += WRITE_LLS_L1_CONFIGm(unit, port, l1_config);
        } else {
            LLS_L1_CONFIGm_P_START_UC_SPRIf_SET(l1_config, (port * 8) + 0x40);
            ioerr += WRITE_LLS_L1_CONFIGm(unit, (8 + port), l1_config);
        }
        
        LLS_L2_PARENTm_CLR(l2_parent);
        for (idx = 0; idx < 8 ; idx++) {
            if (port == 0) {
                LLS_L2_PARENTm_C_PARENTf_SET(l2_parent, port);
                ioerr += WRITE_LLS_L2_PARENTm(unit, idx, l2_parent);
            } else {
                LLS_L2_PARENTm_C_PARENTf_SET(l2_parent, (8 + port));
                ioerr += WRITE_LLS_L2_PARENTm(unit, ((port * 8) + 0x40 + idx), l2_parent);
            }
        }
        
        ppport = P2PP(unit, port);
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
                ioerr += READ_EGR_QUEUE_TO_PP_PORT_MAPm(unit, (q_offset + idx), &egrq_pp_map);
                EGR_QUEUE_TO_PP_PORT_MAPm_PP_PORTf_SET(egrq_pp_map, ppport);
                ioerr += WRITE_EGR_QUEUE_TO_PP_PORT_MAPm(unit, (q_offset + idx), egrq_pp_map);
            }
        }
    }

    CDK_PBMP_ITER(mmu_pbmp, port) {
        mport = P2M(unit, port);
        PORT_MAX_PKT_SIZEr_SET(port_max, 0x41);
        THDIEXT_PORT_MAX_PKT_SIZEr_SET(iext_port_max, 0);
        THDIEMA_PORT_MAX_PKT_SIZEr_SET(iema_port_max, 0);
        THDIRQE_PORT_MAX_PKT_SIZEr_SET(irqe_port_max, 0);
        THDIQEN_PORT_MAX_PKT_SIZEr_SET(iqen_port_max, 0);
        ioerr += WRITE_PORT_MAX_PKT_SIZEr(unit, mport, port_max);
        ioerr += WRITE_THDIEXT_PORT_MAX_PKT_SIZEr(unit, mport, iext_port_max);
        ioerr += WRITE_THDIEMA_PORT_MAX_PKT_SIZEr(unit, mport, iema_port_max);
        ioerr += WRITE_THDIRQE_PORT_MAX_PKT_SIZEr(unit, mport, irqe_port_max);
        ioerr += WRITE_THDIQEN_PORT_MAX_PKT_SIZEr(unit, mport, iqen_port_max);
        PORT_PRI_XON_ENABLEr_CLR(port_pri_xon);
        THDIEXT_PORT_PRI_XON_ENABLEr_SET(iext_pri_xon, 0);
        THDIEMA_PORT_PRI_XON_ENABLEr_SET(iema_pri_xon, 0);
        THDIRQE_PORT_PRI_XON_ENABLEr_SET(irqe_pri_xon, 0);
        THDIQEN_PORT_PRI_XON_ENABLEr_SET(iqen_pri_xon, 0);
        ioerr += WRITE_PORT_PRI_XON_ENABLEr(unit, mport, port_pri_xon);
        ioerr += WRITE_THDIEXT_PORT_PRI_XON_ENABLEr(unit, mport, iext_pri_xon);
        ioerr += WRITE_THDIEMA_PORT_PRI_XON_ENABLEr(unit, mport, iema_pri_xon);
        ioerr += WRITE_THDIRQE_PORT_PRI_XON_ENABLEr(unit, mport, irqe_pri_xon);
        ioerr += WRITE_THDIQEN_PORT_PRI_XON_ENABLEr(unit, mport, iqen_pri_xon);
    }

    MMU_ENQ_IP_PRI_TO_PG_PROFILE_0r_SET(pri_to_pg_profile, 0, 0xffffffff);
    MMU_ENQ_IP_PRI_TO_PG_PROFILE_0r_SET(pri_to_pg_profile, 1, 0x000000ff);
    ioerr += WRITE_MMU_ENQ_IP_PRI_TO_PG_PROFILE_0r(unit, pri_to_pg_profile);

    /* Configuring all profile 3 to use all the priority groups 0 to 7 */
    MMU_ENQ_PROFILE_3_PRI_GRPr_SET(mmu_profile_3_pri_gpr, 0xffffff);
    ioerr += WRITE_MMU_ENQ_PROFILE_3_PRI_GRPr(unit, 0, mmu_profile_3_pri_gpr);
    ioerr += WRITE_MMU_ENQ_PROFILE_3_PRI_GRPr(unit, 1, mmu_profile_3_pri_gpr);

    PROFILE3_PRI_GRPr_SET(profile_3_pri_gpr, 0xffffff);
    ioerr += WRITE_PROFILE3_PRI_GRPr(unit, 0, profile_3_pri_gpr);
    ioerr += WRITE_PROFILE3_PRI_GRPr(unit, 1, profile_3_pri_gpr);

    THDIEXT_PROFILE3_PRI_GRPr_SET(thdiext_profile3_pri_gpr, 0xffffff);
    ioerr += WRITE_THDIEXT_PROFILE3_PRI_GRPr(unit, 0, thdiext_profile3_pri_gpr);
    ioerr += WRITE_THDIEXT_PROFILE3_PRI_GRPr(unit, 1, thdiext_profile3_pri_gpr);

    THDIEMA_PROFILE3_PRI_GRPr_SET(thdiema_profile3_pri_gpr, 0xffffff);
    ioerr += WRITE_THDIEMA_PROFILE3_PRI_GRPr(unit, 0, thdiema_profile3_pri_gpr);
    ioerr += WRITE_THDIEMA_PROFILE3_PRI_GRPr(unit, 1, thdiema_profile3_pri_gpr);

    THDIRQE_PROFILE3_PRI_GRPr_SET(thdirqe_profile3_pri_gpr, 0xffffff);
    ioerr += WRITE_THDIRQE_PROFILE3_PRI_GRPr(unit, 0, thdirqe_profile3_pri_gpr);
    ioerr += WRITE_THDIRQE_PROFILE3_PRI_GRPr(unit, 1, thdirqe_profile3_pri_gpr);

    THDIQEN_PROFILE3_PRI_GRPr_SET(thdiqen_profile3_pri_gpr, 0xffffff);
    ioerr += WRITE_THDIQEN_PROFILE3_PRI_GRPr(unit, 0, thdiqen_profile3_pri_gpr);
    ioerr += WRITE_THDIQEN_PROFILE3_PRI_GRPr(unit, 1, thdiqen_profile3_pri_gpr);

    for (port = CMIC_PORT + 1 ; port <= GS_PORT; port++) {
        /* 10-12, 14-16,18-20  ports have 1 PG */
        if (_is_one_pg_port(port)) {
            continue;
        }
        if (port <= 9) {
            fval[0] |= 0x3 << ((port - 1) * 2);
        } else if (port <= 13) {
            fval[0] |= 0x3 << ((port - 4) * 2);
        } else if (port <= 17) {
            fval[0] |= 0x3 << ((port - 7) * 2);
        } else if (port <= 25) {
            fval[0] |= 0x3 << ((port - 10) * 2);
        } else {
            fval[1] |= 0x3 << ((port - 26) * 2);
        }
    }
    PORT_PROFILE_MAPr_SET(port_prf_map, 0, fval[0]);
    THDIEXT_PORT_PROFILE_MAPr_SET(thidext_prf_map, 0, fval[0]);
    THDIEMA_PORT_PROFILE_MAPr_SET(thidema_prf_map, 0, fval[0]);
    THDIRQE_PORT_PROFILE_MAPr_SET(thidrqe_prf_map, 0, fval[0]);
    THDIQEN_PORT_PROFILE_MAPr_SET(thidqen_prf_map, 0, fval[0]);
    PORT_PROFILE_MAPr_SET(port_prf_map, 1, fval[1]);
    THDIEXT_PORT_PROFILE_MAPr_SET(thidext_prf_map, 1, fval[1]);
    THDIEMA_PORT_PROFILE_MAPr_SET(thidema_prf_map, 1, fval[1]);
    THDIRQE_PORT_PROFILE_MAPr_SET(thidrqe_prf_map, 1, fval[1]);
    THDIQEN_PORT_PROFILE_MAPr_SET(thidqen_prf_map, 1, fval[1]);
    WRITE_PORT_PROFILE_MAPr(unit, port_prf_map);
    WRITE_THDIEXT_PORT_PROFILE_MAPr(unit, thidext_prf_map);
    WRITE_THDIEMA_PORT_PROFILE_MAPr(unit, thidema_prf_map);
    WRITE_THDIRQE_PORT_PROFILE_MAPr(unit, thidrqe_prf_map);
    WRITE_THDIQEN_PORT_PROFILE_MAPr(unit, thidqen_prf_map);

    /* Input port shared space */
    BUFFER_CELL_LIMIT_SP_SHAREDr_CLR(sp_shared);
    THDIEXT_BUFFER_CELL_LIMIT_SP_SHAREDr_CLR(iext_sp_shared);
    THDIEMA_BUFFER_CELL_LIMIT_SP_SHAREDr_CLR(iema_sp_shared);
    THDIRQE_BUFFER_CELL_LIMIT_SP_SHAREDr_CLR(irqe_sp_shared);
    THDIQEN_BUFFER_CELL_LIMIT_SP_SHAREDr_CLR(iqen_sp_shared);
    ioerr += WRITE_BUFFER_CELL_LIMIT_SP_SHAREDr(unit, sp_shared);
    ioerr += WRITE_THDIEXT_BUFFER_CELL_LIMIT_SP_SHAREDr(unit, iext_sp_shared);
    ioerr += WRITE_THDIEMA_BUFFER_CELL_LIMIT_SP_SHAREDr(unit, iema_sp_shared);
    ioerr += WRITE_THDIRQE_BUFFER_CELL_LIMIT_SP_SHAREDr(unit, irqe_sp_shared);
    ioerr += WRITE_THDIQEN_BUFFER_CELL_LIMIT_SP_SHAREDr(unit, iqen_sp_shared);

    /* Input port per-device global headroom */
    ioerr += READ_THDO_MISCCONFIGr(unit, &thd0_misc);
    THDO_MISCCONFIGr_STAT_CLEARf_SET(thd0_misc, 0);
    THDO_MISCCONFIGr_PARITY_CHK_ENf_SET(thd0_misc, 1);
    THDO_MISCCONFIGr_PARITY_GEN_ENf_SET(thd0_misc, 1);
    ioerr += WRITE_THDO_MISCCONFIGr(unit, thd0_misc);

    ioerr += READ_OP_THR_CONFIGr(unit, &op_thr);
    OP_THR_CONFIGr_EARLY_E2E_SELECTf_SET(op_thr, 0);
    ioerr += WRITE_OP_THR_CONFIGr(unit, op_thr);

    /* ===================== */
    /* Device Wide Registers */
    /* ===================== */    
    ioerr += READ_CFAPICONFIGr(unit, &cfapconfig);
    CFAPICONFIGr_CFAPIPOOLSIZEf_SET(cfapconfig, 0x2000);
    ioerr += WRITE_CFAPICONFIGr(unit, cfapconfig);
    
    CFAPIFULLRESETPOINTr_SET(cfapifullrp, 0x1f80);
    ioerr += WRITE_CFAPIFULLRESETPOINTr(unit, cfapifullrp);

    ioerr += READ_CFAPECONFIGr(unit, &cfapeconfig);
    CFAPECONFIGr_CFAPEPOOLSIZEf_SET(cfapeconfig, 0xffff);
    ioerr += WRITE_CFAPECONFIGr(unit, cfapeconfig);

    ioerr += READ_CFAPEFULLSETPOINTr(unit, &cfapefullsetpoint);
    CFAPEFULLSETPOINTr_CFAPEFULLSETPOINTf_SET(cfapefullsetpoint, 0xffbf);
    ioerr += WRITE_CFAPEFULLSETPOINTr(unit, cfapefullsetpoint);

    ioerr += READ_CFAPEFULLRESETPOINTr(unit, &cfapefullresetpoint);
    CFAPEFULLRESETPOINTr_CFAPEFULLRESETPOINTf_SET(cfapefullresetpoint, 0xff7f);
    ioerr += WRITE_CFAPEFULLRESETPOINTr(unit, cfapefullresetpoint);

    /* C250: MMU_ENQ_FAPCONFIG_0.FAPPOOLSIZE =C52*1024-1*/
    ioerr += READ_MMU_ENQ_FAPCONFIG_0r(unit, &mmu_enq_fapconfig_0);
    MMU_ENQ_FAPCONFIG_0r_FAPPOOLSIZEf_SET(mmu_enq_fapconfig_0, 0xfff);
    ioerr += WRITE_MMU_ENQ_FAPCONFIG_0r(unit, mmu_enq_fapconfig_0);

    /* C251 MMU_ENQ_FAPFULLSETPOINT_0.FAPFULLSETPOINT =C250-64 */
    ioerr += READ_MMU_ENQ_FAPFULLSETPOINT_0r(unit, &mmu_enq_FAPFULLSETPOINT_0);
    MMU_ENQ_FAPFULLSETPOINT_0r_FAPFULLSETPOINTf_SET(mmu_enq_FAPFULLSETPOINT_0, 0xfbf);
    ioerr += WRITE_MMU_ENQ_FAPFULLSETPOINT_0r(unit, mmu_enq_FAPFULLSETPOINT_0);

    /* C252 MMU_ENQ_FAPFULLRESETPOINT_0.FAPFULLRESETPOINT =C250-128 */
    ioerr += READ_MMU_ENQ_FAPFULLRESETPOINT_0r(unit, &mmu_enq_fapfullresetpoint_0);
    MMU_ENQ_FAPFULLRESETPOINT_0r_FAPFULLRESETPOINTf_SET(mmu_enq_fapfullresetpoint_0, 0xf7f);
    ioerr += WRITE_MMU_ENQ_FAPFULLRESETPOINT_0r(unit, mmu_enq_fapfullresetpoint_0);

    /* C253: QSTRUCT_FAPCONFIG.FAPPOOLSIZE =(C49*1024+1024*6)/4 */
    ioerr += READ_QSTRUCT_FAPCONFIGr(unit, 0, &qstruct_fapcfg);
    QSTRUCT_FAPCONFIGr_FAPPOOLSIZEf_SET(qstruct_fapcfg, 0x4600);
    ioerr += WRITE_QSTRUCT_FAPCONFIGr(unit, 0, qstruct_fapcfg);

    /* C254 QSTRUCT_FAPFULLSETPOINT.FAPFULLSETPOINT */
    ioerr += READ_QSTRUCT_FAPFULLSETPOINTr(unit, 0, &qstruct_fapfullsetpoint);
    QSTRUCT_FAPFULLSETPOINTr_FAPFULLSETPOINTf_SET(qstruct_fapfullsetpoint, 0x45f8); 
    ioerr += WRITE_QSTRUCT_FAPFULLSETPOINTr(unit, 0, qstruct_fapfullsetpoint);

    /* C255 QSTRUCT_FAPFULLRESETPOINT.FAPFULLRESETPOINT */
    ioerr += READ_QSTRUCT_FAPFULLRESETPOINTr(unit, 0, &qstruct_fapfullresetpoint);
    QSTRUCT_FAPFULLRESETPOINTr_FAPFULLRESETPOINTf_SET(qstruct_fapfullresetpoint,
                                                0x45e8);
    ioerr += WRITE_QSTRUCT_FAPFULLRESETPOINTr(unit, 0, qstruct_fapfullresetpoint);

    /* C258: TOP_SW_BOND_OVRD_CTRL1.MMU_PACKING_ENABLE = C12 */
    ioerr += READ_TOP_SW_BOND_OVRD_CTRL1r(unit, &top_sw_bond_ovrd_ctrl1);
    TOP_SW_BOND_OVRD_CTRL1r_MMU_PACKING_ENABLEf_SET(top_sw_bond_ovrd_ctrl1, 0);
    ioerr += WRITE_TOP_SW_BOND_OVRD_CTRL1r(unit, top_sw_bond_ovrd_ctrl1);

    /* C259: MMU_ENQ_PACKING_REAS_FIFO_PROFILE_THRESH.THRESH_PROFILE_[0..3] = C13 */
    ioerr += READ_MMU_ENQ_PACKING_REAS_FIFO_PROFILE_THRESHr(unit, 
                                        &mmu_eprf_prof_thr);
    MMU_ENQ_PACKING_REAS_FIFO_PROFILE_THRESHr_THRESH_PROFILE_0f_SET(mmu_eprf_prof_thr, 0xa);
    MMU_ENQ_PACKING_REAS_FIFO_PROFILE_THRESHr_THRESH_PROFILE_1f_SET(mmu_eprf_prof_thr, 0xa);
    MMU_ENQ_PACKING_REAS_FIFO_PROFILE_THRESHr_THRESH_PROFILE_2f_SET(mmu_eprf_prof_thr, 0xa);
    MMU_ENQ_PACKING_REAS_FIFO_PROFILE_THRESHr_THRESH_PROFILE_3f_SET(mmu_eprf_prof_thr, 0xa);
    ioerr += WRITE_MMU_ENQ_PACKING_REAS_FIFO_PROFILE_THRESHr(unit, mmu_eprf_prof_thr);

    /* C260: MMU_ENQ_PACKING_REAS_FIFO_THRESH_PROFILE_[0/1/2].SRCPORT_[0..41] = 0 */
    MMU_ENQ_PACKING_REAS_FIFO_THRESH_PROFILE_0r_CLR(mmu_eprf_thr_prof0);
    ioerr += WRITE_MMU_ENQ_PACKING_REAS_FIFO_THRESH_PROFILE_0r(unit, mmu_eprf_thr_prof0);
    MMU_ENQ_PACKING_REAS_FIFO_THRESH_PROFILE_1r_CLR(mmu_eprf_thr_prof1);
    ioerr += WRITE_MMU_ENQ_PACKING_REAS_FIFO_THRESH_PROFILE_1r(unit, mmu_eprf_thr_prof1);
    
    /* C261: THDO_MISCCONFIG.UCMC_SEPARATION = 0 */
    ioerr += READ_THDO_MISCCONFIGr(unit, &thd0_misc);
    THDO_MISCCONFIGr_UCMC_SEPARATIONf_SET(thd0_misc, 0);
    ioerr += WRITE_THDO_MISCCONFIGr(unit, thd0_misc);

    /* C262: WRED_MISCCONFIG.UCMC_SEPARATION = 0 */
    ioerr += READ_WRED_MISCCONFIGr(unit, &wred_misccfg);
    WRED_MISCCONFIGr_UCMC_SEPARATIONf_SET(wred_misccfg, 0);
    ioerr += WRITE_WRED_MISCCONFIGr(unit, wred_misccfg);

    /* C263: MISCCONFIG.THDI_ROLLOVER_COUNT = 0xB4 */
    ioerr += READ_MISCCONFIGr(unit, &misc);
    MISCCONFIGr_THDI_ROLLOVER_COUNTf_SET(misc, 0xb4);
    ioerr += WRITE_MISCCONFIGr(unit, misc);
    
    /* C266: COLOR_AWARE.ENABLE = 0 */
    ioerr += READ_COLOR_AWAREr(unit, &color_aware);
    COLOR_AWAREr_SET(color_aware, 0);
    ioerr += WRITE_COLOR_AWAREr(unit, color_aware);

    /* C268: GLOBAL_HDRM_LIMIT.GLOBAL_HDRM_LIMIT = =$C$110 */
    GLOBAL_HDRM_LIMITr_SET(glb_hdrm, 0x82);
    ioerr += WRITE_GLOBAL_HDRM_LIMITr(unit, glb_hdrm);
    BUFFER_CELL_LIMIT_SPr_SET(buf_cell, 0x1e40);
    ioerr += WRITE_BUFFER_CELL_LIMIT_SPr(unit, 0, buf_cell);
    CELL_RESET_LIMIT_OFFSET_SPr_SET(cell_reset, 0x3f);
    ioerr += WRITE_CELL_RESET_LIMIT_OFFSET_SPr(unit, 0, cell_reset);

    /* RE WQEs */
    THDIRQE_GLOBAL_HDRM_LIMITr_SET(irqe_glb_hdrm, 0);
    ioerr += WRITE_THDIRQE_GLOBAL_HDRM_LIMITr(unit, irqe_glb_hdrm);
    THDIRQE_BUFFER_CELL_LIMIT_SPr_SET(irqe_buf_cell, 0x97b);
    ioerr += WRITE_THDIRQE_BUFFER_CELL_LIMIT_SPr(unit, 0, irqe_buf_cell);
    THDIRQE_CELL_RESET_LIMIT_OFFSET_SPr_SET(irqe_cell_reset, 7);
    ioerr += WRITE_THDIRQE_CELL_RESET_LIMIT_OFFSET_SPr(unit, 0, irqe_cell_reset);

    /* EQEs */
    THDIQEN_GLOBAL_HDRM_LIMITr_SET(iqen_glb_hdrm, 0);
    ioerr += WRITE_THDIQEN_GLOBAL_HDRM_LIMITr(unit, iqen_glb_hdrm);
    THDIQEN_BUFFER_CELL_LIMIT_SPr_SET(iqen_buf_cell, 0x9b55);
    ioerr += WRITE_THDIQEN_BUFFER_CELL_LIMIT_SPr(unit, 0, iqen_buf_cell);
    THDIQEN_CELL_RESET_LIMIT_OFFSET_SPr_SET(iqen_cell_reset, 0x46);
    ioerr += WRITE_THDIQEN_CELL_RESET_LIMIT_OFFSET_SPr(unit, 0, iqen_cell_reset);

    /* Ouput port thresholds */
    /* Internal buffer Egress pool */
    /* C289: OP_BUFFER_SHARED_LIMIT_CELLI.OP_BUFFER_SHARED_LIMIT_CELLI = C222 */
    OP_BUFFER_SHARED_LIMIT_CELLIr_SET(op_shr_cell, 0x2a62);
    ioerr += WRITE_OP_BUFFER_SHARED_LIMIT_CELLIr(unit, op_shr_cell);
    OP_BUFFER_SHARED_LIMIT_RESUME_CELLIr_SET(op_sh_res_cell, 0x2a23);
    ioerr += WRITE_OP_BUFFER_SHARED_LIMIT_RESUME_CELLIr(unit, op_sh_res_cell);
    /* C292 OP_BUFFER_LIMIT_RESUME_YELLOW_CELLI.OP_BUFFER_LIMIT_RESUME_YELLOW_CELLI =
       =CEILING(C290/8, 1) */
    OP_BUFFER_LIMIT_RESUME_YELLOW_CELLIr_SET(op_res_yel_cell, 0x545);
    ioerr += WRITE_OP_BUFFER_LIMIT_RESUME_YELLOW_CELLIr(unit, op_res_yel_cell);
    OP_BUFFER_LIMIT_RESUME_RED_CELLIr_SET(op_res_red_cell, 0x545);
    ioerr += WRITE_OP_BUFFER_LIMIT_RESUME_RED_CELLIr(unit, op_res_red_cell);
    OP_BUFFER_LIMIT_YELLOW_CELLIr_SET(op_yel_cell, 0x54d);
    ioerr += WRITE_OP_BUFFER_LIMIT_YELLOW_CELLIr(unit, op_yel_cell);
    OP_BUFFER_LIMIT_RED_CELLIr_SET(op_red_cell, 0x54d);
    ioerr += WRITE_OP_BUFFER_LIMIT_RED_CELLIr(unit, op_red_cell);

    /* C310: 
       OP_BUFFER_SHARED_LIMIT_THDORQEQ.OP_BUFFER_SHARED_LIMIT_CELLE = C226 */
    OP_BUFFER_SHARED_LIMIT_THDORQEQr_SET(op_shr_rqeq, 0xedc);
    ioerr += WRITE_OP_BUFFER_SHARED_LIMIT_THDORQEQr(unit, op_shr_rqeq);
    OP_BUFFER_LIMIT_YELLOW_THDORQEQr_SET(op_yel_rqeq, 0x1dc);
    ioerr += WRITE_OP_BUFFER_LIMIT_YELLOW_THDORQEQr(unit, op_yel_rqeq);
    OP_BUFFER_LIMIT_RED_THDORQEQr_SET(op_red_rqeq, 0x1dc);
    ioerr += WRITE_OP_BUFFER_LIMIT_RED_THDORQEQr(unit, op_red_rqeq);
    OP_BUFFER_SHARED_LIMIT_RESUME_THDORQEQr_SET(op_sh_res_rqeq, 0xed5);
    ioerr += WRITE_OP_BUFFER_SHARED_LIMIT_RESUME_THDORQEQr(unit, op_sh_res_rqeq);
    OP_BUFFER_LIMIT_RESUME_YELLOW_THDORQEQr_SET(op_res_yel_rqeq, 0x1db);
    ioerr += WRITE_OP_BUFFER_LIMIT_RESUME_YELLOW_THDORQEQr(unit, op_res_yel_rqeq);
    OP_BUFFER_LIMIT_RESUME_RED_THDORQEQr_SET(op_res_red_rqeq, 0x1db);
    ioerr += WRITE_OP_BUFFER_LIMIT_RESUME_RED_THDORQEQr(unit, op_res_red_rqeq);
    OP_BUFFER_SHARED_LIMIT_QENTRYr_SET(op_shr_qentry, 0xffff);
    ioerr += WRITE_OP_BUFFER_SHARED_LIMIT_QENTRYr(unit, op_shr_qentry);
    OP_BUFFER_LIMIT_YELLOW_QENTRYr_SET(op_yel_qentry, 0x1fff);
    ioerr += WRITE_OP_BUFFER_LIMIT_YELLOW_QENTRYr(unit, op_yel_qentry);
    OP_BUFFER_LIMIT_RED_QENTRYr_SET(op_red_qentry, 0x1fff);
    ioerr += WRITE_OP_BUFFER_LIMIT_RED_QENTRYr(unit, op_red_qentry);
    OP_BUFFER_SHARED_LIMIT_RESUME_QENTRYr_SET(op_sh_res_qentry, 0xffb9);
    ioerr += WRITE_OP_BUFFER_SHARED_LIMIT_RESUME_QENTRYr(unit, op_sh_res_qentry);
    OP_BUFFER_LIMIT_RESUME_YELLOW_QENTRYr_SET(op_res_yel_qentry, 0x1ff8);
    ioerr += WRITE_OP_BUFFER_LIMIT_RESUME_YELLOW_QENTRYr(unit, op_res_yel_qentry);
    OP_BUFFER_LIMIT_RESUME_RED_QENTRYr_SET(op_res_red_qentry, 0x1ff8);
    ioerr += WRITE_OP_BUFFER_LIMIT_RESUME_RED_QENTRYr(unit, op_res_red_qentry);

    /* C324 : OP_BUFFER_SHARED_LIMIT_THDORDEQ.OP_BUFFER_SHARED_LIMIT = C229 */
    /* EP Redirection Packets */
    OP_BUFFER_SHARED_LIMIT_THDORDEQr_SET(op_sh_rdeq, 0x1a0);
    ioerr += WRITE_OP_BUFFER_SHARED_LIMIT_THDORDEQr(unit, op_sh_rdeq);
    OP_BUFFER_SHARED_LIMIT_RESUME_THDORDEQr_SET(op_sh_res_rdeq, 0x19c);
    ioerr += WRITE_OP_BUFFER_SHARED_LIMIT_RESUME_THDORDEQr(unit, op_sh_res_rdeq);
    OP_BUFFER_LIMIT_YELLOW_THDORDEQr_SET(op_yel_rdeq, 0x34);
    ioerr += WRITE_OP_BUFFER_LIMIT_YELLOW_THDORDEQr(unit, op_yel_rdeq);
    OP_BUFFER_LIMIT_RED_THDORDEQr_SET(op_red_rdeq, 0x34);
    ioerr += WRITE_OP_BUFFER_LIMIT_RED_THDORDEQr(unit, op_red_rdeq);
    OP_BUFFER_LIMIT_RESUME_YELLOW_THDORDEQr_SET(op_res_yel_rdeq, 0x33);
    ioerr += WRITE_OP_BUFFER_LIMIT_RESUME_YELLOW_THDORDEQr(unit, op_res_yel_rdeq);
    OP_BUFFER_LIMIT_RESUME_RED_THDORDEQr_SET(op_res_red_rdeq, 0x33);
    ioerr += WRITE_OP_BUFFER_LIMIT_RESUME_RED_THDORDEQr(unit, op_res_red_rdeq);

    /* Per Port Registers */
    /* ######################## */
    /* 1. Input Port Thresholds */
    /* ######################## */
    /* 1.1 Internal Buffer Ingress Pool */
    CDK_PBMP_ITER(mmu_pbmp, port) {
        mport = P2M(unit, port);

        /* 10-12, 14-16,18-20  ports have 1 PG */
        if (_is_one_pg_port(mport)) {
            priority_group = 0;
        } else {
            priority_group = 7;
        }

        THDI_PORT_SP_CONFIGm_CLR(port_sp_cfg);
        THDI_PORT_SP_CONFIGm_PORT_SP_MAX_LIMITf_SET(port_sp_cfg, 0x1412);
        THDI_PORT_SP_CONFIGm_PORT_SP_RESUME_LIMITf_SET(port_sp_cfg, 0x1400);
        ioerr += WRITE_THDI_PORT_SP_CONFIGm(unit, (mport * 4), port_sp_cfg);
        
        ioerr += READ_PORT_MAX_PKT_SIZEr(unit, mport, &port_max_pkt_size);
        PORT_MAX_PKT_SIZEr_PORT_MAX_PKT_SIZEf_SET(port_max_pkt_size, 0x41);
        ioerr += WRITE_PORT_MAX_PKT_SIZEr(unit, mport, port_max_pkt_size);

        /* C352: THDIEXT_PORT_MAX_PKT_SIZE.PORT_MAX_PKT_SIZE 
               =IF(C17=0, "NA", CEILING(C72/C103, 1)) */
        ioerr += READ_THDIEXT_PORT_MAX_PKT_SIZEr(unit, mport, &iext_port_max);
        THDIEXT_PORT_MAX_PKT_SIZEr_PORT_MAX_PKT_SIZEf_SET(iext_port_max, 0);
        ioerr += WRITE_THDIEXT_PORT_MAX_PKT_SIZEr(unit, mport, iext_port_max);

        /* C367: THDIEMA_PORT_MAX_PKT_SIZE.PORT_MAX_PKT_SIZE 
           =IF(C17=0, "NA", $C$72)*/
        ioerr += READ_THDIEMA_PORT_MAX_PKT_SIZEr(unit, mport, &iema_port_max);
        THDIEMA_PORT_MAX_PKT_SIZEr_PORT_MAX_PKT_SIZEf_SET(iema_port_max, 0);
        ioerr += WRITE_THDIEMA_PORT_MAX_PKT_SIZEr(unit, mport, iema_port_max);

        THDI_PORT_PG_CONFIGm_CLR(port_pg_cfg);
        THDI_PORT_PG_CONFIGm_PG_MIN_LIMITf_SET(port_pg_cfg, 0x31);
        THDI_PORT_PG_CONFIGm_PG_SHARED_LIMITf_SET(port_pg_cfg, 7);
        THDI_PORT_PG_CONFIGm_PG_RESET_OFFSETf_SET(port_pg_cfg, 0x12);
        THDI_PORT_PG_CONFIGm_PG_RESET_FLOORf_SET(port_pg_cfg, 0);
        THDI_PORT_PG_CONFIGm_PG_GBL_HDRM_ENf_SET(port_pg_cfg, 1);
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
        ioerr += WRITE_THDI_PORT_PG_CONFIGm(unit, (mport * 8) + priority_group, port_pg_cfg); 
    }

    /* 1.2 External Buffer */
    CDK_PBMP_ITER(mmu_pbmp, port) {
        mport = P2M(unit, port);
        
        THDIEXT_THDI_PORT_PG_CONFIGm_CLR(iext_port_pg_cfg);
        THDIEXT_THDI_PORT_PG_CONFIGm_SP_SHARED_MAX_ENABLEf_SET(iext_port_pg_cfg, 1);
        THDIEXT_THDI_PORT_PG_CONFIGm_SP_MIN_PG_ENABLEf_SET(iext_port_pg_cfg, 1);
        ioerr += WRITE_THDIEXT_THDI_PORT_PG_CONFIGm(unit, (mport * 8), iext_port_pg_cfg);
        if (MXQPORT_SUBPORT(unit, port) == 0) {
            for (idx = 1; idx < 8; idx++) {
                THDIEXT_THDI_PORT_PG_CONFIGm_PG_MIN_LIMITf_SET(iext_port_pg_cfg, 1);
                ioerr += WRITE_THDIEXT_THDI_PORT_PG_CONFIGm(unit, ((mport * 8) + idx), iext_port_pg_cfg);
            }
        }
    }

    /* 1.3 EMA Pool */
    CDK_PBMP_ITER(mmu_pbmp, port) {
        mport = P2M(unit, port);
        
        THDIEMA_THDI_PORT_PG_CONFIGm_CLR(iema_port_pg_cfg);
        THDIEMA_THDI_PORT_PG_CONFIGm_SP_SHARED_MAX_ENABLEf_SET(iema_port_pg_cfg, 1);
        THDIEMA_THDI_PORT_PG_CONFIGm_SP_MIN_PG_ENABLEf_SET(iema_port_pg_cfg, 1);
        ioerr += WRITE_THDIEMA_THDI_PORT_PG_CONFIGm(unit, (mport * 8), iema_port_pg_cfg);
        if (MXQPORT_SUBPORT(unit, port) == 0) {
            for (idx = 1; idx < 8; idx++) {
                THDIEMA_THDI_PORT_PG_CONFIGm_PG_MIN_LIMITf_SET(iema_port_pg_cfg, 1);
                ioerr += WRITE_THDIEMA_THDI_PORT_PG_CONFIGm(unit, ((mport * 8) + idx), iema_port_pg_cfg);
            }
        }
    }

    /* 1.4 RE WQEs */
    CDK_PBMP_ITER(mmu_pbmp, port) {
        mport = P2M(unit, port);
        
        THDIRQE_THDI_PORT_SP_CONFIGm_CLR(irqe_port_sp_cfg);
        THDIRQE_THDI_PORT_SP_CONFIGm_PORT_SP_MIN_LIMITf_SET(irqe_port_sp_cfg, 0);
        THDIRQE_THDI_PORT_SP_CONFIGm_PORT_SP_MAX_LIMITf_SET(irqe_port_sp_cfg, 0x97b);
        THDIRQE_THDI_PORT_SP_CONFIGm_PORT_SP_RESUME_LIMITf_SET(irqe_port_sp_cfg, 0x979);
        ioerr += WRITE_THDIRQE_THDI_PORT_SP_CONFIGm(unit, (mport * 4), irqe_port_sp_cfg);

        THDIRQE_THDI_PORT_PG_CONFIGm_CLR(irqe_port_pg_cfg);
        THDIRQE_THDI_PORT_PG_CONFIGm_PG_MIN_LIMITf_SET(irqe_port_pg_cfg, 0x31);
        THDIRQE_THDI_PORT_PG_CONFIGm_PG_SHARED_LIMITf_SET(irqe_port_pg_cfg, 7);
        THDIRQE_THDI_PORT_PG_CONFIGm_PG_RESET_OFFSETf_SET(irqe_port_pg_cfg, 2);
        THDIRQE_THDI_PORT_PG_CONFIGm_PG_RESET_FLOORf_SET(irqe_port_pg_cfg, 0);
        THDIRQE_THDI_PORT_PG_CONFIGm_PG_SHARED_DYNAMICf_SET(irqe_port_pg_cfg, 1);
        if (port == CMIC_PORT) {
            THDIRQE_THDI_PORT_PG_CONFIGm_PG_HDRM_LIMITf_SET(irqe_port_pg_cfg, 0x12);
        } else if (BMD_PORT_PROPERTIES(unit, port) == BMD_PORT_GE) {
            THDIRQE_THDI_PORT_PG_CONFIGm_PG_HDRM_LIMITf_SET(irqe_port_pg_cfg, 0x23);
        } else {
            THDIRQE_THDI_PORT_PG_CONFIGm_PG_HDRM_LIMITf_SET(irqe_port_pg_cfg, 0x23);
        }
        THDIRQE_THDI_PORT_PG_CONFIGm_PG_GBL_HDRM_ENf_SET(irqe_port_pg_cfg, 0);
        THDIRQE_THDI_PORT_PG_CONFIGm_SP_SHARED_MAX_ENABLEf_SET(irqe_port_pg_cfg, 1);
        THDIRQE_THDI_PORT_PG_CONFIGm_SP_MIN_PG_ENABLEf_SET(irqe_port_pg_cfg, 1);
        ioerr += WRITE_THDIRQE_THDI_PORT_PG_CONFIGm(unit, (mport * 8) + priority_group, irqe_port_pg_cfg);
    }
    
    /* 1.5 EQEs */
    CDK_PBMP_ITER(mmu_pbmp, port) {
        mport = P2M(unit, port);
        
        THDIQEN_THDI_PORT_SP_CONFIGm_CLR(iqen_port_sp_cfg);
        THDIQEN_THDI_PORT_SP_CONFIGm_PORT_SP_MIN_LIMITf_SET(iqen_port_sp_cfg, 0);
        THDIQEN_THDI_PORT_SP_CONFIGm_PORT_SP_MAX_LIMITf_SET(iqen_port_sp_cfg, 0x9b55);
        THDIQEN_THDI_PORT_SP_CONFIGm_PORT_SP_RESUME_LIMITf_SET(iqen_port_sp_cfg, 0x9b41);
        ioerr += WRITE_THDIQEN_THDI_PORT_SP_CONFIGm(unit, (mport * 4), iqen_port_sp_cfg);

        THDIQEN_THDI_PORT_PG_CONFIGm_CLR(iqen_port_pg_cfg);
        THDIQEN_THDI_PORT_PG_CONFIGm_PG_MIN_LIMITf_SET(iqen_port_pg_cfg, 0x1ea);
        THDIQEN_THDI_PORT_PG_CONFIGm_PG_SHARED_LIMITf_SET(iqen_port_pg_cfg, 7);
        THDIQEN_THDI_PORT_PG_CONFIGm_PG_RESET_OFFSETf_SET(iqen_port_pg_cfg, 0x14);
        THDIQEN_THDI_PORT_PG_CONFIGm_PG_RESET_FLOORf_SET(iqen_port_pg_cfg, 0);
        THDIQEN_THDI_PORT_PG_CONFIGm_PG_SHARED_DYNAMICf_SET(iqen_port_pg_cfg, 1);
        if (mport == CMIC_PORT) {
            THDIQEN_THDI_PORT_PG_CONFIGm_PG_HDRM_LIMITf_SET(iqen_port_pg_cfg, 0xb4);
        } else if (BMD_PORT_PROPERTIES(unit, port) == BMD_PORT_GE) {
            THDIQEN_THDI_PORT_PG_CONFIGm_PG_HDRM_LIMITf_SET(iqen_port_pg_cfg, 0x15e);
        } else {
            THDIQEN_THDI_PORT_PG_CONFIGm_PG_HDRM_LIMITf_SET(iqen_port_pg_cfg, 0x15e);
        }
        THDIQEN_THDI_PORT_PG_CONFIGm_PG_GBL_HDRM_ENf_SET(iqen_port_pg_cfg, 0);
        THDIQEN_THDI_PORT_PG_CONFIGm_SP_SHARED_MAX_ENABLEf_SET(iqen_port_pg_cfg, 1);
        THDIQEN_THDI_PORT_PG_CONFIGm_SP_MIN_PG_ENABLEf_SET(iqen_port_pg_cfg, 1);
        ioerr += WRITE_THDIQEN_THDI_PORT_PG_CONFIGm(unit, (mport * 8) + priority_group, iqen_port_pg_cfg);
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
            ioerr += READ_MMU_THDO_OPNCONFIG_CELLm(unit, (op_node + idx), &opncfg_cell);
            MMU_THDO_OPNCONFIG_CELLm_OPN_SHARED_LIMIT_CELLf_SET(opncfg_cell, 0x2a62);
            MMU_THDO_OPNCONFIG_CELLm_OPN_SHARED_RESET_VALUE_CELLf_SET(opncfg_cell, 0x2a50);
            MMU_THDO_OPNCONFIG_CELLm_PORT_LIMIT_ENABLE_CELLf_SET(opncfg_cell, 0);
            ioerr += WRITE_MMU_THDO_OPNCONFIG_CELLm(unit, (op_node + idx), opncfg_cell);
        }
        
        for (idx = 0; idx < q_limit; idx++) {
            ioerr += READ_MMU_THDO_QCONFIG_CELLm(unit, (q_offset + idx), &qcfg_cell);
            MMU_THDO_QCONFIG_CELLm_Q_SHARED_LIMIT_CELLf_SET(qcfg_cell, 0x2a62);
            MMU_THDO_QCONFIG_CELLm_Q_MIN_CELLf_SET(qcfg_cell, 0);
            MMU_THDO_QCONFIG_CELLm_Q_LIMIT_ENABLE_CELLf_SET(qcfg_cell, 0);
            MMU_THDO_QCONFIG_CELLm_Q_LIMIT_DYNAMIC_CELLf_SET(qcfg_cell, 0);
            MMU_THDO_QCONFIG_CELLm_Q_COLOR_ENABLE_CELLf_SET(qcfg_cell, 0);
            MMU_THDO_QCONFIG_CELLm_Q_COLOR_LIMIT_DYNAMIC_CELLf_SET(qcfg_cell, 0);
            MMU_THDO_QCONFIG_CELLm_LIMIT_YELLOW_CELLf_SET(qcfg_cell, 0);
            ioerr += WRITE_MMU_THDO_QCONFIG_CELLm(unit, (q_offset + idx), qcfg_cell);
        
            ioerr += READ_MMU_THDO_QOFFSET_CELLm(unit, (q_offset + idx), &qoff_cell);
            MMU_THDO_QOFFSET_CELLm_RESET_OFFSET_CELLf_SET(qoff_cell, 2);
            MMU_THDO_QOFFSET_CELLm_LIMIT_RED_CELLf_SET(qoff_cell, 0);
            MMU_THDO_QOFFSET_CELLm_RESET_OFFSET_YELLOW_CELLf_SET(qoff_cell, 2);
            MMU_THDO_QOFFSET_CELLm_RESET_OFFSET_RED_CELLf_SET(qoff_cell, 2);
            ioerr += WRITE_MMU_THDO_QOFFSET_CELLm(unit, (q_offset + idx), qoff_cell);
        }
    }
    
    /* 2.4 RE WQEs */
    /* 2.4.1 RQEQ */
    for (idx = 0; idx < NUM_RQEQ_COS; idx++) {
        ioerr += READ_OP_QUEUE_CONFIG1_THDORQEQr(unit, idx, &opq_rqeq1);
        OP_QUEUE_CONFIG1_THDORQEQr_Q_COLOR_ENABLEf_SET(opq_rqeq1, 0);
        OP_QUEUE_CONFIG1_THDORQEQr_Q_COLOR_DYNAMICf_SET(opq_rqeq1, 0);
        OP_QUEUE_CONFIG1_THDORQEQr_Q_MINf_SET(opq_rqeq1, 0);
        ioerr += WRITE_OP_QUEUE_CONFIG1_THDORQEQr(unit, idx, opq_rqeq1);

        ioerr += READ_OP_QUEUE_CONFIG_THDORQEQr(unit, idx, &opq_rqeq);
        OP_QUEUE_CONFIG_THDORQEQr_Q_LIMIT_ENABLEf_SET(opq_rqeq, 0);
        OP_QUEUE_CONFIG_THDORQEQr_Q_LIMIT_DYNAMICf_SET(opq_rqeq, 0);
        OP_QUEUE_CONFIG_THDORQEQr_Q_SHARED_LIMITf_SET(opq_rqeq, 0xedc);
        ioerr += WRITE_OP_QUEUE_CONFIG_THDORQEQr(unit, idx, opq_rqeq);

        OP_QUEUE_RESET_OFFSET_THDORQEQr_SET(opq_rst_rqeq, 1);
        ioerr += WRITE_OP_QUEUE_RESET_OFFSET_THDORQEQr(unit, idx, opq_rst_rqeq);

        OP_QUEUE_LIMIT_YELLOW_THDORQEQr_SET(opq_yel_rqeq, 0);
        ioerr += WRITE_OP_QUEUE_LIMIT_YELLOW_THDORQEQr(unit, idx, opq_yel_rqeq);

        OP_QUEUE_LIMIT_RED_THDORQEQr_SET(opq_red_rqeq, 0);
        ioerr += WRITE_OP_QUEUE_LIMIT_RED_THDORQEQr(unit, idx, opq_red_rqeq);

        OP_QUEUE_RESET_OFFSET_YELLOW_THDORQEQr_SET(opq_rst_yel_rqeq, 1);
        ioerr += WRITE_OP_QUEUE_RESET_OFFSET_YELLOW_THDORQEQr(unit, idx, opq_rst_yel_rqeq);

        OP_QUEUE_RESET_OFFSET_RED_THDORQEQr_SET(opq_rst_red_rqeq, 1);
        ioerr += WRITE_OP_QUEUE_RESET_OFFSET_RED_THDORQEQr(unit, idx, opq_rst_red_rqeq);
    }

    /* 2.4.2 RQEI */
    for (idx = 0; idx < NUM_RQEI_COS; idx++) {
        ioerr += READ_OP_QUEUE_CONFIG1_THDORQEIr(unit, idx, &opq_rqei1);
        OP_QUEUE_CONFIG1_THDORQEIr_Q_COLOR_ENABLEf_SET(opq_rqei1, 0);
        OP_QUEUE_CONFIG1_THDORQEIr_Q_COLOR_DYNAMICf_SET(opq_rqei1, 0);
        OP_QUEUE_CONFIG1_THDORQEIr_Q_MINf_SET(opq_rqei1, 0);
        ioerr += WRITE_OP_QUEUE_CONFIG1_THDORQEIr(unit, idx, opq_rqei1);

        ioerr += READ_OP_QUEUE_CONFIG_THDORQEIr(unit, idx, &opq_rqei);
        OP_QUEUE_CONFIG_THDORQEIr_Q_LIMIT_ENABLEf_SET(opq_rqei, 0);
        OP_QUEUE_CONFIG_THDORQEIr_Q_LIMIT_DYNAMICf_SET(opq_rqei, 0);
        OP_QUEUE_CONFIG_THDORQEIr_Q_SHARED_LIMITf_SET(opq_rqei, 0x2a62);
        ioerr += WRITE_OP_QUEUE_CONFIG_THDORQEIr(unit, idx, opq_rqei);

        OP_QUEUE_RESET_OFFSET_THDORQEIr_SET(opq_rst_rqei, 2);
        ioerr += WRITE_OP_QUEUE_RESET_OFFSET_THDORQEIr(unit, idx, opq_rst_rqei);

        OP_QUEUE_LIMIT_YELLOW_THDORQEIr_SET(opq_yel_rqei, 0);
        ioerr += WRITE_OP_QUEUE_LIMIT_YELLOW_THDORQEIr(unit, idx, opq_yel_rqei);

        OP_QUEUE_LIMIT_RED_THDORQEIr_SET(opq_red_rqei, 0);
        ioerr += WRITE_OP_QUEUE_LIMIT_RED_THDORQEIr(unit, idx, opq_red_rqei);

        OP_QUEUE_RESET_OFFSET_YELLOW_THDORQEIr_SET(opq_rst_yel_rqei, 2);
        ioerr += WRITE_OP_QUEUE_RESET_OFFSET_YELLOW_THDORQEIr(unit, idx, opq_rst_yel_rqei);

        OP_QUEUE_RESET_OFFSET_RED_THDORQEIr_SET(opq_rst_red_rqei, 2);
        ioerr += WRITE_OP_QUEUE_RESET_OFFSET_RED_THDORQEIr(unit, idx, opq_rst_red_rqei);
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
            ioerr += READ_MMU_THDO_OPNCONFIG_QENTRYm(unit, (op_node + idx), &opncfg_qentry);
            MMU_THDO_OPNCONFIG_QENTRYm_OPN_SHARED_LIMIT_QENTRYf_SET(opncfg_qentry, 0xffff);
            MMU_THDO_OPNCONFIG_QENTRYm_OPN_SHARED_RESET_VALUE_QENTRYf_SET(opncfg_qentry, 0xfffd);
            MMU_THDO_OPNCONFIG_QENTRYm_PORT_LIMIT_ENABLE_QENTRYf_SET(opncfg_qentry, 0);
            ioerr += WRITE_MMU_THDO_OPNCONFIG_QENTRYm(unit, (op_node + idx), opncfg_qentry);
        }
        
        for (idx = 0; idx < q_limit; idx++) {
            ioerr += READ_MMU_THDO_QCONFIG_QENTRYm(unit, (q_offset + idx), &qcfg_qentry);
            MMU_THDO_QCONFIG_QENTRYm_Q_SHARED_LIMIT_QENTRYf_SET(qcfg_qentry, 0xffff);
            MMU_THDO_QCONFIG_QENTRYm_Q_MIN_QENTRYf_SET(qcfg_qentry, 0);
            MMU_THDO_QCONFIG_QENTRYm_Q_LIMIT_ENABLE_QENTRYf_SET(qcfg_qentry, 0);
            MMU_THDO_QCONFIG_QENTRYm_Q_LIMIT_DYNAMIC_QENTRYf_SET(qcfg_qentry, 0);
            MMU_THDO_QCONFIG_QENTRYm_Q_COLOR_ENABLE_QENTRYf_SET(qcfg_qentry, 0);
            MMU_THDO_QCONFIG_QENTRYm_LIMIT_YELLOW_QENTRYf_SET(qcfg_qentry, 0);
            ioerr += WRITE_MMU_THDO_QCONFIG_QENTRYm(unit, (q_offset + idx), qcfg_qentry);
        
            ioerr += READ_MMU_THDO_QOFFSET_QENTRYm(unit, (q_offset + idx), &qoff_qentry);
            MMU_THDO_QOFFSET_QENTRYm_RESET_OFFSET_QENTRYf_SET(qoff_qentry, 1);
            MMU_THDO_QOFFSET_QENTRYm_LIMIT_RED_QENTRYf_SET(qoff_qentry, 0);
            MMU_THDO_QOFFSET_QENTRYm_RESET_OFFSET_YELLOW_QENTRYf_SET(qoff_qentry, 1);
            MMU_THDO_QOFFSET_QENTRYm_RESET_OFFSET_RED_QENTRYf_SET(qoff_qentry, 1);
            ioerr += WRITE_MMU_THDO_QOFFSET_QENTRYm(unit, (q_offset + idx), qoff_qentry);
        }
    }
    
    /* 2.5.1 RDEQ */
    for (idx = 0; idx < NUM_RDEQ_COS; idx++) {
        ioerr += READ_OP_QUEUE_CONFIG1_THDORDEQr(unit, idx, &opq_cfg1_rdeq);
        OP_QUEUE_CONFIG1_THDORDEQr_Q_MINf_SET(opq_cfg1_rdeq, 4);
        OP_QUEUE_CONFIG1_THDORDEQr_Q_COLOR_ENABLEf_SET(opq_cfg1_rdeq, 0);
        OP_QUEUE_CONFIG1_THDORDEQr_Q_COLOR_DYNAMICf_SET(opq_cfg1_rdeq, 1);
        ioerr += WRITE_OP_QUEUE_CONFIG1_THDORDEQr(unit, idx, opq_cfg1_rdeq);

        ioerr += READ_OP_QUEUE_CONFIG_THDORDEQr(unit, idx, &opq_cfg_rdeq);
        OP_QUEUE_CONFIG_THDORDEQr_Q_SHARED_LIMITf_SET(opq_cfg_rdeq, 0x1a0);
        OP_QUEUE_CONFIG_THDORDEQr_Q_LIMIT_DYNAMICf_SET(opq_cfg_rdeq, 0);
        OP_QUEUE_CONFIG_THDORDEQr_Q_LIMIT_ENABLEf_SET(opq_cfg_rdeq, 0);
        ioerr += WRITE_OP_QUEUE_CONFIG_THDORDEQr(unit, idx, opq_cfg_rdeq);

        OP_QUEUE_RESET_OFFSET_THDORDEQr_SET(opq_rst_rdeq, 1);
        ioerr += WRITE_OP_QUEUE_RESET_OFFSET_THDORDEQr(unit, idx, opq_rst_rdeq);

        OP_QUEUE_LIMIT_YELLOW_THDORDEQr_SET(opq_yel_rdeq, 0);
        ioerr += WRITE_OP_QUEUE_LIMIT_YELLOW_THDORDEQr(unit, idx, opq_yel_rdeq);

        OP_QUEUE_LIMIT_RED_THDORDEQr_SET(opq_red_rdeq, 0);
        ioerr += WRITE_OP_QUEUE_LIMIT_RED_THDORDEQr(unit, idx, opq_red_rdeq);

        OP_QUEUE_RESET_OFFSET_YELLOW_THDORDEQr_SET(opq_rst_yel_rdeq, 1);
        ioerr += WRITE_OP_QUEUE_RESET_OFFSET_YELLOW_THDORDEQr(unit, idx, opq_rst_yel_rdeq);

        OP_QUEUE_RESET_OFFSET_RED_THDORDEQr_SET(opq_rst_red_rdeq, 1);
        ioerr += WRITE_OP_QUEUE_RESET_OFFSET_RED_THDORDEQr(unit, idx, opq_rst_red_rdeq);
    }

    /* 2.5.2 RDEI */
    for (idx = 0; idx < NUM_RDEI_COS; idx++) {
        ioerr += READ_OP_QUEUE_CONFIG1_THDORDEIr(unit, idx, &opq_cfg1_rdei);
        OP_QUEUE_CONFIG1_THDORDEIr_Q_MINf_SET(opq_cfg1_rdei, 0);
        OP_QUEUE_CONFIG1_THDORDEIr_Q_COLOR_ENABLEf_SET(opq_cfg1_rdei, 0);
        OP_QUEUE_CONFIG1_THDORDEIr_Q_COLOR_DYNAMICf_SET(opq_cfg1_rdei, 1);
        ioerr += WRITE_OP_QUEUE_CONFIG1_THDORDEIr(unit, idx, opq_cfg1_rdei);

        ioerr += READ_OP_QUEUE_CONFIG_THDORDEIr(unit, idx, &opq_cfg_rdei);
        OP_QUEUE_CONFIG_THDORDEIr_Q_SHARED_LIMITf_SET(opq_cfg_rdei, 0x2a62);
        OP_QUEUE_CONFIG_THDORDEIr_Q_LIMIT_DYNAMICf_SET(opq_cfg_rdei, 0);
        OP_QUEUE_CONFIG_THDORDEIr_Q_LIMIT_ENABLEf_SET(opq_cfg_rdei, 0);
        ioerr += WRITE_OP_QUEUE_CONFIG_THDORDEIr(unit, idx, opq_cfg_rdei);

        OP_QUEUE_RESET_OFFSET_THDORDEIr_SET(opq_rst_rdei, 2);
        ioerr += WRITE_OP_QUEUE_RESET_OFFSET_THDORDEIr(unit, idx, opq_rst_rdei);

        OP_QUEUE_LIMIT_YELLOW_THDORDEIr_SET(opq_yel_rdei, 0);
        ioerr += WRITE_OP_QUEUE_LIMIT_YELLOW_THDORDEIr(unit, idx, opq_yel_rdei);

        OP_QUEUE_LIMIT_RED_THDORDEIr_SET(opq_red_rdei, 0);
        ioerr += WRITE_OP_QUEUE_LIMIT_RED_THDORDEIr(unit, idx, opq_red_rdei);

        OP_QUEUE_RESET_OFFSET_YELLOW_THDORDEIr_SET(opq_rst_yel_rdei, 2);
        ioerr += WRITE_OP_QUEUE_RESET_OFFSET_YELLOW_THDORDEIr(unit, idx, opq_rst_yel_rdei);

        OP_QUEUE_RESET_OFFSET_RED_THDORDEIr_SET(opq_rst_red_rdei, 2);
        ioerr += WRITE_OP_QUEUE_RESET_OFFSET_RED_THDORDEIr(unit, idx, opq_rst_red_rdei);
    }

    THDIEXT_PORT_PAUSE_ENABLE_64r_SET(iext_pause_enable, 0x1ffffffe);
    ioerr += WRITE_THDIEXT_PORT_PAUSE_ENABLE_64r(unit, iext_pause_enable);
    THDIQEN_PORT_PAUSE_ENABLE_64r_SET(iqen_pause_enable, 0x1ffffffe);
    ioerr += WRITE_THDIQEN_PORT_PAUSE_ENABLE_64r(unit, iqen_pause_enable);
    THDIEMA_PORT_PAUSE_ENABLE_64r_SET(iema_pause_enable, 0x1ffffffe);
    ioerr += WRITE_THDIEMA_PORT_PAUSE_ENABLE_64r(unit, iema_pause_enable);
    PORT_PAUSE_ENABLE_64r_SET(port_pause_enable, 0x1ffffffe);
    ioerr += WRITE_PORT_PAUSE_ENABLE_64r(unit, port_pause_enable);

    /* Initialize MMU internal/external aging limit memory */
    MMU_AGING_LMT_INTm_CLR(age_int);
    MMU_AGING_LMT_EXTm_CLR(age_ext);
    for (idx = 0; idx < MMU_AGING_LMT_INTm_MAX; idx++) {
        ioerr += WRITE_MMU_AGING_LMT_INTm(unit, idx, age_int);
    }

    for (idx = 0; idx < MMU_AGING_LMT_EXTm_MAX; idx++) {
        ioerr += WRITE_MMU_AGING_LMT_EXTm(unit, idx, age_ext);
    }
    
    /* post_mmu_init */
    /* TXLP_PORT_ADDR_MAP_TABLE */
    for (subidx = 0, start_addr = 0; subidx < PORTS_PER_BLOCK; subidx++) {
        speed_max = bcm56260_a0_port_speed_max(unit, port);
        if (speed_max <= 2500) {
            end_addr = start_addr + (( 8 * 4) - 1); /* 8 cells */
        } else if (speed_max <= 13000) {
            end_addr = start_addr + ((16 * 4) - 1); /* 16 Cells */
        } else {
            return CDK_E_INTERNAL;
        }

        TXLP_PORT_ADDR_MAP_TABLEm_CLR(txlp_port_map);
        TXLP_PORT_ADDR_MAP_TABLEm_START_ADDRf_SET(txlp_port_map, start_addr);
        TXLP_PORT_ADDR_MAP_TABLEm_END_ADDRf_SET(txlp_port_map, end_addr);
        ioerr += WRITE_TXLP_PORT_ADDR_MAP_TABLEm(unit, subidx, txlp_port_map);
        start_addr = end_addr + 1;
    }
    /* end of post_mmu_init */

    return ioerr ? CDK_E_IO : rv;
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

#if BMD_CONFIG_INCLUDE_PHY == 1  
static int
_firmware_helper(void *ctx, uint32_t offset, uint32_t size, void *data)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int unit, port;
    const char *drv_name;
    XLPORT_WC_UCMEM_CTRLr_t ucmem_ctrl;
    XLPORT_WC_UCMEM_DATAm_t ucmem_data;
    uint32_t wbuf[4];
    uint32_t wdata;
    uint32_t *fw_data;
    uint32_t *fw_entry;
    uint32_t fw_size;
    uint32_t idx, wdx;
    uint32_t be_host;

    /* Get unit, port and driver name from context */
    bmd_phy_fw_info_get(ctx, &unit, &port, &drv_name);

    if (CDK_STRSTR(drv_name, "tsce") == NULL) {
        return CDK_E_UNAVAIL;
    }

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

    /* Enable parallel bus access */
    ioerr += READ_XLPORT_WC_UCMEM_CTRLr(unit, &ucmem_ctrl, port);
    XLPORT_WC_UCMEM_CTRLr_ACCESS_MODEf_SET(ucmem_ctrl, 1);
    ioerr += WRITE_XLPORT_WC_UCMEM_CTRLr(unit, ucmem_ctrl, port);

    /* We need to byte swap on big endian host */
    be_host = 1;
    if (*((uint8_t *)&be_host) == 1) {
        be_host = 0;
    }

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
            XLPORT_WC_UCMEM_DATAm_SET(ucmem_data, wdx, wdata);
        }
        WRITE_XLPORT_WC_UCMEM_DATAm(unit, idx >> 4, ucmem_data, port);
    }

    /* Disable parallel bus access */
    XLPORT_WC_UCMEM_CTRLr_ACCESS_MODEf_SET(ucmem_ctrl, 0);
    ioerr += WRITE_XLPORT_WC_UCMEM_CTRLr(unit, ucmem_ctrl, port);

    return ioerr ? CDK_E_IO : rv;
}
#endif /* BMD_CONFIG_INCLUDE_PHY == 1 */    


/* Bring the port down and back up to clear credits */
int bcm56260_b0_soc_port_enable_set(int unit, int port, int enable) 
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int loop;
    uint32_t port_enable;
    cdk_pbmp_t xlport_pbmp;
    TXLP_PORT_ENABLEr_t txlp_enable;
    TXLP_MIN_STARTCNTr_t txlp_min_startcnt;
    IECELL_CONFIGr_t iecell_cfg;
    XPORT_PORT_ENABLEr_t xport_enable;
    XLPORT_ENABLE_REGr_t xlport_enable;
    uint32_t enable_reg_value;
    EGR_ENABLEm_t egr_enable;

    loop = (port-1) % 4;

    ioerr += READ_TXLP_PORT_ENABLEr(unit,  &txlp_enable);
    port_enable = TXLP_PORT_ENABLEr_PORT_ENABLEf_GET(txlp_enable);
    TXLP_PORT_ENABLEr_PORT_ENABLEf_SET(txlp_enable, (port_enable | (1 << loop)));
    ioerr += WRITE_TXLP_PORT_ENABLEr(unit, txlp_enable);

    ioerr += READ_TXLP_MIN_STARTCNTr(unit, &txlp_min_startcnt);
    TXLP_MIN_STARTCNTr_NLP_STARTCNTf_SET(txlp_min_startcnt, 3);
    ioerr += WRITE_TXLP_MIN_STARTCNTr(unit, txlp_min_startcnt);
    bcm56260_a0_xlport_pbmp_get(unit, &xlport_pbmp);
    if (CDK_PBMP_MEMBER(xlport_pbmp, port)) {
        ioerr += READ_IECELL_CONFIGr(unit, XLPORT_SUBPORT(unit, port)+1, &iecell_cfg);
        if (enable) {
            IECELL_CONFIGr_SOFT_RESETf_SET(iecell_cfg, 0);
        } else {
            IECELL_CONFIGr_SOFT_RESETf_SET(iecell_cfg, 1);
        }
        ioerr += WRITE_IECELL_CONFIGr(unit, XLPORT_SUBPORT(unit, port)+1, iecell_cfg);
        ioerr += READ_XLPORT_ENABLE_REGr(unit, &xlport_enable, port);
        enable_reg_value = XLPORT_ENABLE_REGr_GET(xlport_enable);
        if (enable) {
            enable_reg_value |= 0x1 << XLPORT_SUBPORT(unit, port);
            XLPORT_ENABLE_REGr_SET(xlport_enable, enable_reg_value);
        } else {
            enable_reg_value &= ~(0x1 << XLPORT_SUBPORT(unit, port));
            XLPORT_ENABLE_REGr_SET(xlport_enable, enable_reg_value);
        }
        ioerr += WRITE_XLPORT_ENABLE_REGr(unit, xlport_enable, port);
    } else {
        ioerr += READ_XPORT_PORT_ENABLEr(unit, &xport_enable, port);
        enable_reg_value = XPORT_PORT_ENABLEr_GET(xport_enable);
        if (enable) {
            enable_reg_value |= 1 << MXQPORT_SUBPORT(unit, port);
            XPORT_PORT_ENABLEr_SET(xport_enable, enable_reg_value);
        } else {
            enable_reg_value &= ~(1 << MXQPORT_SUBPORT(unit, port));
            XPORT_PORT_ENABLEr_SET(xport_enable, enable_reg_value);
        }
        ioerr += WRITE_XPORT_PORT_ENABLEr(unit, xport_enable, port);
    }

    if (enable) {
        EGR_ENABLEm_PRT_ENABLEf_SET(egr_enable, 1);
    } else {
        EGR_ENABLEm_PRT_ENABLEf_SET(egr_enable, 0);
    }
    ioerr += WRITE_EGR_ENABLEm(unit, port, egr_enable);

    return ioerr ? CDK_E_IO : rv;
}      


int
bcm56260_b0_mac_xl_drain_cells(int unit, int port)
{
    int ioerr = 0;
    int pause_rx, rx_pfc_en, llfc_rx;
    XLMAC_PAUSE_CTRLr_t xlmac_pause_ctrl;
    XLMAC_PFC_CTRLr_t xlmac_pfc_ctrl;
    XLMAC_LLFC_CTRLr_t xlmac_llfc_ctrl;
    XLMAC_CTRLr_t xlmac_ctrl;
    XLMAC_TX_CTRLr_t xlmac_tx_ctrl;
    XLMAC_TXFIFO_CELL_CNTr_t xlmac_cell_cnt;
    int cnt;

    /* Disable pause/pfc function */
    ioerr += READ_XLMAC_PAUSE_CTRLr(unit, port, &xlmac_pause_ctrl);
    pause_rx = XLMAC_PAUSE_CTRLr_RX_PAUSE_ENf_GET(xlmac_pause_ctrl);
    XLMAC_PAUSE_CTRLr_RX_PAUSE_ENf_SET(xlmac_pause_ctrl, 0);
    ioerr += WRITE_XLMAC_PAUSE_CTRLr(unit, port, xlmac_pause_ctrl);
    
    ioerr += READ_XLMAC_PFC_CTRLr(unit, port, &xlmac_pfc_ctrl);
    rx_pfc_en = XLMAC_PFC_CTRLr_RX_PFC_ENf_GET(xlmac_pfc_ctrl);
    XLMAC_PFC_CTRLr_RX_PFC_ENf_SET(xlmac_pfc_ctrl, 0);
    ioerr += WRITE_XLMAC_PFC_CTRLr(unit, port, xlmac_pfc_ctrl);

    ioerr += READ_XLMAC_LLFC_CTRLr(unit, port, &xlmac_llfc_ctrl);
    llfc_rx = XLMAC_LLFC_CTRLr_RX_LLFC_ENf_GET(xlmac_llfc_ctrl);
    XLMAC_LLFC_CTRLr_RX_LLFC_ENf_SET(xlmac_llfc_ctrl, 0);
    ioerr += WRITE_XLMAC_LLFC_CTRLr(unit, port, xlmac_llfc_ctrl);

    /* Assert SOFT_RESET before DISCARD just in case there is no credit left */
    ioerr += READ_XLMAC_CTRLr(unit, port, &xlmac_ctrl);
    XLMAC_CTRLr_SOFT_RESETf_SET(xlmac_ctrl, 1);
    ioerr += WRITE_XLMAC_CTRLr(unit, port, xlmac_ctrl);

    /* Drain data in TX FIFO without egressing at packet boundary */
    ioerr += READ_XLMAC_TX_CTRLr(unit, port, &xlmac_tx_ctrl);
    XLMAC_TX_CTRLr_DISCARDf_SET(xlmac_tx_ctrl, 1);
    XLMAC_TX_CTRLr_EP_DISCARDf_SET(xlmac_tx_ctrl, 1);
    ioerr += WRITE_XLMAC_TX_CTRLr(unit, port, xlmac_tx_ctrl);
    
    /* Reset EP credit before de-assert SOFT_RESET */
    ioerr += bcm56260_b0_soc_port_enable_set(unit, port, 0);
    ioerr += bcm56260_b0_soc_port_enable_set(unit, port, 1);
    
    /* De-assert SOFT_RESET to let the drain start */
    XLMAC_CTRLr_SOFT_RESETf_SET(xlmac_ctrl, 0);
    ioerr += WRITE_XLMAC_CTRLr(unit, port, xlmac_ctrl);

    /* Wait until TX fifo cell count is 0 */
    cnt = DRAIN_WAIT_MSEC / 10;

    while (--cnt >= 0) {
        ioerr += READ_XLMAC_TXFIFO_CELL_CNTr(unit, port, &xlmac_cell_cnt);
        if (XLMAC_TXFIFO_CELL_CNTr_CELL_CNTf_GET(xlmac_cell_cnt) == 0) {
            break;
        }
        BMD_SYS_USLEEP(10000);
    }
    if (cnt < 0) {
        CDK_WARN(("bcm56260_b0_bmd_port_mode_set[%d]: "
                "drain failed on port %d\n", unit, port));
    }

    /* Stop TX FIFO drainging */
    ioerr += READ_XLMAC_TX_CTRLr(unit, port, &xlmac_tx_ctrl);
    XLMAC_TX_CTRLr_DISCARDf_SET(xlmac_tx_ctrl, 0);
    XLMAC_TX_CTRLr_EP_DISCARDf_SET(xlmac_tx_ctrl, 0);
    ioerr += WRITE_XLMAC_TX_CTRLr(unit, port, xlmac_tx_ctrl);

    /* Restore original pause/pfc/llfc configuration */
    if (pause_rx) {
        ioerr += READ_XLMAC_PAUSE_CTRLr(unit, port, &xlmac_pause_ctrl);
        XLMAC_PAUSE_CTRLr_RX_PAUSE_ENf_SET(xlmac_pause_ctrl, 1);
        ioerr += WRITE_XLMAC_PAUSE_CTRLr(unit, port, xlmac_pause_ctrl);
    }
    
    if (rx_pfc_en) {
        ioerr += READ_XLMAC_PFC_CTRLr(unit, port, &xlmac_pfc_ctrl);
        XLMAC_PFC_CTRLr_RX_PFC_ENf_SET(xlmac_pfc_ctrl, 1);
        ioerr += WRITE_XLMAC_PFC_CTRLr(unit, port, xlmac_pfc_ctrl);
    }

    if (llfc_rx) {
        ioerr += READ_XLMAC_LLFC_CTRLr(unit, port, &xlmac_llfc_ctrl);
        XLMAC_LLFC_CTRLr_RX_LLFC_ENf_SET(xlmac_llfc_ctrl, 1);
        ioerr += WRITE_XLMAC_LLFC_CTRLr(unit, port, xlmac_llfc_ctrl);
    }
    
    return ioerr;
}


int
bcm56260_b0_xlport_init(int unit, int port)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    XLMAC_CTRLr_t xlmac_ctrl;
    XLMAC_RX_CTRLr_t xlmac_rx_ctrl;
    XLMAC_TX_CTRLr_t xlmac_tx_ctrl;
    XLMAC_PAUSE_CTRLr_t xlmac_pause_ctrl;
    XLMAC_RX_MAX_SIZEr_t xlmac_rx_max_size;
    XLMAC_MODEr_t xlmac_mode;
    XLMAC_RX_LSS_CTRLr_t xlmac_rx_lss_ctrl;
    XLMAC_PFC_CTRLr_t xlmac_pfc_ctrl;
    int speed;
    cdk_pbmp_t mxqport_pbmp;
    X_GPORT_SGNDET_EARLYCRSr_t sgn_crs;

    ioerr += _port_init(unit, port);

    speed = bcm56260_a0_port_speed_max(unit, port);
    /* MAC init */
    /* Disable Tx/Rx, assume that MAC is stable (or out of reset) */
    ioerr += READ_XLMAC_CTRLr(unit, port, &xlmac_ctrl);

    /* Reset EP credit before de-assert SOFT_RESET */
    if (XLMAC_CTRLr_SOFT_RESETf_GET(xlmac_ctrl)) {
        /* credit_reset */
        /* Bring the port down and back up to clear credits */
        ioerr += bcm56260_b0_soc_port_enable_set(unit, port, 0);
        ioerr += bcm56260_b0_soc_port_enable_set(unit, port, 1);
        /* end of credit_reset */
    }

    XLMAC_CTRLr_SOFT_RESETf_SET(xlmac_ctrl, 0);
    XLMAC_CTRLr_RX_ENf_SET(xlmac_ctrl, 0);
    XLMAC_CTRLr_TX_ENf_SET(xlmac_ctrl, 0);
    if (BMD_PORT_PROPERTIES(unit, port) & BMD_PORT_HG) {
        XLMAC_CTRLr_XGMII_IPG_CHECK_DISABLEf_SET(xlmac_ctrl, 1);
    } else {
        XLMAC_CTRLr_XGMII_IPG_CHECK_DISABLEf_SET(xlmac_ctrl, 0);
    }
    ioerr += WRITE_XLMAC_CTRLr(unit, port, xlmac_ctrl);
    BMD_SYS_USLEEP(1000);

    ioerr += READ_XLMAC_RX_CTRLr(unit, port, &xlmac_rx_ctrl);
    XLMAC_RX_CTRLr_STRIP_CRCf_SET(xlmac_rx_ctrl, 0);
    XLMAC_RX_CTRLr_STRICT_PREAMBLEf_SET(xlmac_rx_ctrl, 0);
    if (speed >= 10000) {
        XLMAC_RX_CTRLr_STRICT_PREAMBLEf_SET(xlmac_rx_ctrl, 1);
    }
    ioerr += WRITE_XLMAC_RX_CTRLr(unit, port, xlmac_rx_ctrl);

    ioerr += READ_XLMAC_TX_CTRLr(unit, port, &xlmac_tx_ctrl);
    if (BMD_PORT_PROPERTIES(unit, port) & BMD_PORT_HG) {
        XLMAC_TX_CTRLr_AVERAGE_IPGf_SET(xlmac_tx_ctrl, 0xc);
    } else {
        XLMAC_TX_CTRLr_AVERAGE_IPGf_SET(xlmac_tx_ctrl, 0x9);
    }
    XLMAC_TX_CTRLr_CRC_MODEf_SET(xlmac_tx_ctrl, 3);
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

    /* Setup header mode */
    ioerr += READ_XLMAC_MODEr(unit, port, &xlmac_mode);
    if (BMD_PORT_PROPERTIES(unit, port) & BMD_PORT_HG) {
        XLMAC_MODEr_HDR_MODEf_SET(xlmac_mode, 1);
    } else {
        XLMAC_MODEr_HDR_MODEf_SET(xlmac_mode, 0);
    }
    
    XLMAC_MODEr_SPEED_MODEf_SET(xlmac_mode, 4);
    if (speed == 1000) {
        XLMAC_MODEr_SPEED_MODEf_SET(xlmac_mode, 2);
    } else if (speed == 2500) {
        XLMAC_MODEr_SPEED_MODEf_SET(xlmac_mode, 3);
    }
    ioerr += WRITE_XLMAC_MODEr(unit, port, xlmac_mode);

    ioerr += READ_XLMAC_RX_LSS_CTRLr(unit, port, &xlmac_rx_lss_ctrl);
    XLMAC_RX_LSS_CTRLr_DROP_TX_DATA_ON_LOCAL_FAULTf_SET(xlmac_rx_lss_ctrl, 1);
    XLMAC_RX_LSS_CTRLr_DROP_TX_DATA_ON_REMOTE_FAULTf_SET(xlmac_rx_lss_ctrl, 1);
    XLMAC_RX_LSS_CTRLr_DROP_TX_DATA_ON_LINK_INTERRUPTf_SET(xlmac_rx_lss_ctrl, 1);
    ioerr += WRITE_XLMAC_RX_LSS_CTRLr(unit, port, xlmac_rx_lss_ctrl);

    ioerr += READ_XLMAC_CTRLr(unit, port, &xlmac_ctrl);
    XLMAC_CTRLr_EXTENDED_HIG2_ENf_SET(xlmac_ctrl, 1);
    XLMAC_CTRLr_SW_LINK_STATUSf_SET(xlmac_ctrl, 1);
    WRITE_XLMAC_CTRLr(unit, port, xlmac_ctrl);

    bcm56260_a0_mxqport_pbmp_get(unit, &mxqport_pbmp);
    if (CDK_PBMP_MEMBER(mxqport_pbmp, port)) {
        ioerr += READ_X_GPORT_SGNDET_EARLYCRSr(unit, port, &sgn_crs);
        X_GPORT_SGNDET_EARLYCRSr_SGN_DETf_SET(sgn_crs, 0);
        ioerr += WRITE_X_GPORT_SGNDET_EARLYCRSr(unit, port, sgn_crs);
    }

    /* Disable loopback and bring XLMAC out of reset */
    ioerr += READ_XLMAC_CTRLr(unit, port, &xlmac_ctrl);
    XLMAC_CTRLr_RX_ENf_SET(xlmac_ctrl, 1);
    XLMAC_CTRLr_TX_ENf_SET(xlmac_ctrl, 1);
    ioerr += WRITE_XLMAC_CTRLr(unit, port, xlmac_ctrl);
    /* End of MAC init */

    return ioerr ? CDK_E_IO : rv;
}


static int
_misc_init(int unit)
{
    int ioerr = 0;
    RXLP_HW_RESET_CONTROLr_t rxlp_hw_rset_ctrl;
    int idx;
    TXLP_HW_RESET_CONTROL_1r_t txlp_hw_rset_ctrl_1;
    EGR_VLAN_CONTROL_1r_t egr_vlan_control_1;
    EGR_IPMC_CFG2r_t ipmc_cfg;
    cdk_pbmp_t pbmp_all, mxqport_pbmp, xlport_pbmp;
    int port;
    PP_PORT_GPP_TRANSLATION_1m_t pp_gpp_tran_1;
    PP_PORT_GPP_TRANSLATION_2m_t pp_gpp_tran_2;
    PP_PORT_GPP_TRANSLATION_3m_t pp_gpp_tran_3;
    PP_PORT_GPP_TRANSLATION_4m_t pp_gpp_tran_4;
    EGR_PP_PORT_GPP_TRANSLATION_1m_t egr_pp_gpp_tran_1;
    EGR_PP_PORT_GPP_TRANSLATION_2m_t egr_pp_gpp_tran_2;
    TOP_MISC_CONTROL_1r_t top_misc_ctrl_1;
    MISCCONFIGr_t misc;
    DEQ_EFIFO_CFG_COMPLETEr_t deq_efifo_cfg_complete;
    int speed_max;
    int phy_mode, num_lanes = 0;    
    XPORT_MODE_REGr_t xport_mode;
    XPORT_MIB_RESETr_t mib_reset;
    XLPORT_CONFIGr_t xlport_cfg;
    XLPORT_MODE_REGr_t xlport_mode_port;
    XLPORT_SOFT_RESETr_t xlport_soft_reset;
    int phy_mode_xl, core_mode_xl;
    XLPORT_MIB_RESETr_t xlport_mib_reset;
    XLPORT_ECC_CONTROLr_t xlport_ecc_ctrl;
    XLPORT_CNTMAXSIZEr_t xlport_cntmaxsize;
    X_GPORT_CNTMAXSIZEr_t x_gport_cntmaxsize;
    XLPORT_FLOW_CONTROL_CONFIGr_t xlport_flow_ctrl_cfg;

    /* saber2 related hw reset control */
    READ_RXLP_HW_RESET_CONTROLr(unit, &rxlp_hw_rset_ctrl);
    RXLP_HW_RESET_CONTROLr_VALIDf_SET(rxlp_hw_rset_ctrl, 1);
    RXLP_HW_RESET_CONTROLr_START_ADDRESSf_SET(rxlp_hw_rset_ctrl, 0);
    RXLP_HW_RESET_CONTROLr_COUNTf_SET(rxlp_hw_rset_ctrl, 0x7D0);
    WRITE_RXLP_HW_RESET_CONTROLr(unit, rxlp_hw_rset_ctrl);
    /* Now wait for HW to set DONEf */
    for (idx = 0; idx < PIPE_RESET_TIMEOUT_MSEC; idx++) {
        ioerr += READ_RXLP_HW_RESET_CONTROLr(unit, &rxlp_hw_rset_ctrl);
        if (RXLP_HW_RESET_CONTROLr_DONEf_GET(rxlp_hw_rset_ctrl)) {
            break;
        }
        BMD_SYS_USLEEP(1000);
    }
    if (idx >= PIPE_RESET_TIMEOUT_MSEC) {
        CDK_WARN(("RXLP HW RESET Failed\n"));
        return ioerr ? CDK_E_IO : CDK_E_TIMEOUT;
    }    

    READ_TXLP_HW_RESET_CONTROL_1r(unit, &txlp_hw_rset_ctrl_1);
    TXLP_HW_RESET_CONTROL_1r_VALIDf_SET(txlp_hw_rset_ctrl_1, 1);
    TXLP_HW_RESET_CONTROL_1r_COUNTf_SET(txlp_hw_rset_ctrl_1, 0x84);
    TXLP_HW_RESET_CONTROL_1r_RESET_ALLf_SET(txlp_hw_rset_ctrl_1, 0x1);
    WRITE_TXLP_HW_RESET_CONTROL_1r(unit, txlp_hw_rset_ctrl_1);
    for (idx = 0; idx < PIPE_RESET_TIMEOUT_MSEC; idx++) {
        ioerr += READ_TXLP_HW_RESET_CONTROL_1r(unit, &txlp_hw_rset_ctrl_1);
        if (TXLP_HW_RESET_CONTROL_1r_DONEf_GET(txlp_hw_rset_ctrl_1)) {
            break;
        }
        BMD_SYS_USLEEP(1000);
    }
    if (idx >= PIPE_RESET_TIMEOUT_MSEC) {
        CDK_WARN(("TXLP HW RESET Failed\n"));
        return ioerr ? CDK_E_IO : CDK_E_TIMEOUT;
    }  

    ioerr += CDK_XGSD_MEM_CLEAR(unit, RXLP_INTERNAL_STREAM_MAP_PORT_0m);
    ioerr += CDK_XGSD_MEM_CLEAR(unit, RXLP_INTERNAL_STREAM_MAP_PORT_1m);
    ioerr += CDK_XGSD_MEM_CLEAR(unit, RXLP_INTERNAL_STREAM_MAP_PORT_2m);
    ioerr += CDK_XGSD_MEM_CLEAR(unit, RXLP_INTERNAL_STREAM_MAP_PORT_3m);
    ioerr += CDK_XGSD_MEM_CLEAR(unit, TXLP_PORT_STREAM_BITMAP_TABLEm);
    ioerr += CDK_XGSD_MEM_CLEAR(unit, TXLP_INT2EXT_STREAM_MAP_TABLEm);
    ioerr += CDK_XGSD_MEM_CLEAR(unit, DEVICE_STREAM_ID_TO_PP_PORT_MAPm);
    ioerr += CDK_XGSD_MEM_CLEAR(unit, PP_PORT_TO_PHYSICAL_PORT_MAPm);
    /* end of saber2 related hw reset control */
    
    {
    /* Set ser to disable */
    }
    
    /* Some registers are implemented in memory, need to clear them in order
     * to have correct parity value */
    EGR_VLAN_CONTROL_1r_CLR(egr_vlan_control_1);
    EGR_IPMC_CFG2r_CLR(ipmc_cfg);
    bcm56260_a0_mxqport_pbmp_get(unit, &mxqport_pbmp);
    bcm56260_a0_xlport_pbmp_get(unit, &xlport_pbmp);
    pbmp_all = mxqport_pbmp;
    CDK_PBMP_OR(pbmp_all, xlport_pbmp);    
    CDK_PBMP_ITER(pbmp_all, port) {
        if (port >= 0) {
            ioerr += WRITE_EGR_VLAN_CONTROL_1r(unit, port, egr_vlan_control_1);
            ioerr += WRITE_EGR_IPMC_CFG2r(unit, port, ipmc_cfg);
        }
    }
    ioerr += WRITE_EGR_VLAN_CONTROL_1r(unit, CMIC_PORT, egr_vlan_control_1);
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

    ioerr += READ_TOP_MISC_CONTROL_1r(unit, &top_misc_ctrl_1);
    TOP_MISC_CONTROL_1r_DDR3_PHY0_IDDQ_ENf_SET(top_misc_ctrl_1, 0);
    ioerr += WRITE_TOP_MISC_CONTROL_1r(unit, top_misc_ctrl_1);
    
    /* Enable Field Processor metering clock */
    ioerr += READ_MISCCONFIGr(unit, &misc);
    MISCCONFIGr_METERING_CLK_ENf_SET(misc, 1);
    ioerr += WRITE_MISCCONFIGr(unit, misc);

    ioerr += READ_DEQ_EFIFO_CFG_COMPLETEr(unit, &deq_efifo_cfg_complete);
    DEQ_EFIFO_CFG_COMPLETEr_EGRESS_FIFO_CONFIGURATION_COMPLETEf_SET(deq_efifo_cfg_complete, 0);
    ioerr += WRITE_DEQ_EFIFO_CFG_COMPLETEr(unit, deq_efifo_cfg_complete);

    /* The HW defaults for EGR_VLAN_CONTROL_1.VT_MISS_UNTAG == 1, which
     * causes the outer tag to be removed from packets that don't have
     * a hit in the egress vlan tranlation table. Set to 0 to disable this.
     */
    EGR_VLAN_CONTROL_1r_CLR(egr_vlan_control_1);
    EGR_VLAN_CONTROL_1r_VT_MISS_UNTAGf_SET(egr_vlan_control_1, 0);
    EGR_VLAN_CONTROL_1r_REMARK_OUTER_DOT1Pf_SET(egr_vlan_control_1, 1);
    CDK_PBMP_ITER(pbmp_all, port) {
        ioerr += WRITE_EGR_VLAN_CONTROL_1r(unit, port, egr_vlan_control_1);
    }
    
    /* soc_saber2_port_init_config */
    CDK_PBMP_ITER(pbmp_all, port) {
        speed_max = bcm56260_a0_port_speed_max(unit, port);
        if (CDK_PBMP_MEMBER(mxqport_pbmp, port)) {
            ioerr += bcm56260_a0_mxq_phy_mode_get(unit, port, speed_max, &phy_mode, &num_lanes);
            
            ioerr += READ_XPORT_MODE_REGr(unit, &xport_mode, port);
            XPORT_MODE_REGr_PHY_PORT_MODEf_SET(xport_mode, phy_mode);
            XPORT_MODE_REGr_PORT_GMII_MII_ENABLEf_SET(xport_mode, ((speed_max < 10000) ? 1 : 0));
            ioerr += WRITE_XPORT_MODE_REGr(unit, xport_mode, port);

            XPORT_MIB_RESETr_SET(mib_reset, 0Xf);
            ioerr += WRITE_XPORT_MIB_RESETr(unit, mib_reset, port);
            BMD_SYS_USLEEP(1000);
            XPORT_MIB_RESETr_CLR(mib_reset);
            ioerr += WRITE_XPORT_MIB_RESETr(unit, mib_reset, port);
        }
        
        if (CDK_PBMP_MEMBER(xlport_pbmp, port)) {
            ioerr += READ_XLPORT_CONFIGr(unit, port, &xlport_cfg);
            XLPORT_CONFIGr_HIGIG_MODEf_SET(xlport_cfg, 1);
            XLPORT_CONFIGr_HIGIG2_MODEf_SET(xlport_cfg, 0);
            XLPORT_CONFIGr_MY_MODIDf_SET(xlport_cfg, 0);
            ioerr += WRITE_XLPORT_CONFIGr(unit, port, xlport_cfg);

            XLPORT_MODE_REGr_CLR(xlport_mode_port);
            ioerr += WRITE_XLPORT_MODE_REGr(unit, xlport_mode_port, port);   
            
            XLPORT_SOFT_RESETr_SET(xlport_soft_reset, 0xf);
            ioerr += WRITE_XLPORT_SOFT_RESETr(unit, xlport_soft_reset, port);            

            bcm56260_a0_xl_phy_core_port_mode(unit, port, &phy_mode_xl, &core_mode_xl);
            
            XLPORT_MODE_REGr_CLR(xlport_mode_port);
            XLPORT_MODE_REGr_EGR_1588_TIMESTAMPING_CMIC_48_ENf_SET(xlport_mode_port, 1);
            XLPORT_MODE_REGr_XPORT0_CORE_PORT_MODEf_SET(xlport_mode_port,core_mode_xl); 
            XLPORT_MODE_REGr_XPORT0_PHY_PORT_MODEf_SET(xlport_mode_port, phy_mode_xl);
            ioerr += WRITE_XLPORT_MODE_REGr(unit, xlport_mode_port, port);

            XLPORT_SOFT_RESETr_SET(xlport_soft_reset, 0);
            ioerr += WRITE_XLPORT_SOFT_RESETr(unit, xlport_soft_reset, port);
    
            XLPORT_MIB_RESETr_SET(xlport_mib_reset, 0xf);
            ioerr += WRITE_XLPORT_MIB_RESETr(unit, xlport_mib_reset, port);
            BMD_SYS_USLEEP(1000);
            XLPORT_MIB_RESETr_SET(xlport_mib_reset, 0x0);
            ioerr += WRITE_XLPORT_MIB_RESETr(unit, xlport_mib_reset, port); 
    
            XLPORT_ECC_CONTROLr_CLR(xlport_ecc_ctrl);
            XLPORT_ECC_CONTROLr_MIB_TSC_MEM_ENf_SET(xlport_ecc_ctrl, 1);
            ioerr += WRITE_XLPORT_ECC_CONTROLr(unit, xlport_ecc_ctrl, port);
    
            XLPORT_CNTMAXSIZEr_CLR(xlport_cntmaxsize);
            XLPORT_CNTMAXSIZEr_CNTMAXSIZEf_SET(xlport_cntmaxsize, 0x5f2);
            ioerr += WRITE_XLPORT_CNTMAXSIZEr(unit, port, xlport_cntmaxsize);
        }
        
        if (CDK_PBMP_MEMBER(mxqport_pbmp, port)) {
            X_GPORT_CNTMAXSIZEr_CLR(x_gport_cntmaxsize);
            X_GPORT_CNTMAXSIZEr_CNTMAXSIZEf_SET(x_gport_cntmaxsize, 0x5f2);
            ioerr += WRITE_X_GPORT_CNTMAXSIZEr(unit, port, x_gport_cntmaxsize);
        }
        
        if (CDK_PBMP_MEMBER(xlport_pbmp, port)) {
            /* enable flow control for port macro ports */
            ioerr += READ_XLPORT_FLOW_CONTROL_CONFIGr(unit, port, &xlport_flow_ctrl_cfg);
            XLPORT_FLOW_CONTROL_CONFIGr_MERGE_MODE_ENf_SET(xlport_flow_ctrl_cfg, 1);
            ioerr += WRITE_XLPORT_FLOW_CONTROL_CONFIGr(unit, port, xlport_flow_ctrl_cfg);
        }
    } /* end of soc_saber2_port_init_config */
    
    return ioerr ? CDK_E_IO : CDK_E_NONE;
}        

int
bcm56260_b0_bmd_init(int unit)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int port;
    cdk_pbmp_t mxqport_pbmp, xlport_pbmp, pbmp_all;
    XLPORT_MAC_RSV_MASKr_t xlmac_mac_rsv_mask;
    MAC_RSV_MASKr_t mac_rsv_mask;
    RDBGC0_SELECTr_t rdbgc0_select;
    TDBGC_SELECTr_t tdbgc_select;
    VLAN_PROFILE_TABm_t vlan_profile;
    ING_VLAN_TAG_ACTION_PROFILEm_t vlan_action;
    EGR_VLAN_TAG_ACTION_PROFILEm_t egr_action;
    EGR_BYPASS_CTRLr_t egr_bypass_ctrl;
    int speed_max;
    
    BMD_CHECK_UNIT(unit);

    rv = _misc_init(unit);

    /* Initialize MMU */
    if (CDK_SUCCESS(rv)) {
        rv = _mmu_init(unit);
    }

    /* Configure discard counter */
    RDBGC0_SELECTr_BITMAPf_SET(rdbgc0_select, 0x0400ad11);
    TDBGC_SELECTr_BITMAPf_SET(tdbgc_select, 0xffffffff);
    ioerr += WRITE_RDBGC0_SELECTr(unit, rdbgc0_select);
    ioerr += WRITE_TDBGC_SELECTr(unit, 0, tdbgc_select);
    
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

    ioerr += READ_EGR_BYPASS_CTRLr(unit, &egr_bypass_ctrl);
    EGR_BYPASS_CTRLr_EFP_SW_ENC_DEC_TCAMf_SET(egr_bypass_ctrl, 1);
    ioerr += WRITE_EGR_BYPASS_CTRLr(unit, egr_bypass_ctrl);
    
    bcm56260_a0_mxqport_pbmp_get(unit, &mxqport_pbmp);
    bcm56260_a0_xlport_pbmp_get(unit, &xlport_pbmp);
    pbmp_all = mxqport_pbmp;
    CDK_PBMP_OR(pbmp_all, xlport_pbmp);  
        
    CDK_PBMP_ITER(pbmp_all, port) {
        if (CDK_PBMP_MEMBER(xlport_pbmp, port)) {
            ioerr += READ_XLPORT_MAC_RSV_MASKr(unit, port, &xlmac_mac_rsv_mask);
            XLPORT_MAC_RSV_MASKr_SET(xlmac_mac_rsv_mask, 0x58);
            ioerr += WRITE_XLPORT_MAC_RSV_MASKr(unit, port, xlmac_mac_rsv_mask);
        } else {
            ioerr += READ_MAC_RSV_MASKr(unit, port, &mac_rsv_mask);
            MAC_RSV_MASKr_SET(mac_rsv_mask, 0x58);
            ioerr += WRITE_MAC_RSV_MASKr(unit, port, mac_rsv_mask);
        }
    }
    
    speed_max = bcm56260_a0_port_speed_max(unit, port);
    CDK_PBMP_ITER(mxqport_pbmp, port) {
#if BMD_CONFIG_INCLUDE_PHY == 1      
        phy_ctrl_t *pc;
        uint32_t ability= 0x0;
        int lanes;    
        
        /* PHY init */
        if (CDK_SUCCESS(rv)) {
            rv = bmd_phy_probe(unit, port);
        }
        lanes = bcm56260_a0_port_num_lanes(unit, port);
        if (CDK_SUCCESS(rv)) {
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
        }        
        
        ability = (BMD_PHY_ABIL_1000MB_FD | BMD_PHY_ABIL_100MB_FD);
        if (speed_max == 1000) {
            ability |= BMD_PHY_ABIL_10MB_FD;
        } else if (speed_max == 2500) {
            ability |= BMD_PHY_ABIL_2500MB;
        } else if (speed_max == 10000) {
            ability |= (BMD_PHY_ABIL_10GB | BMD_PHY_ABIL_2500MB);
        }
        if (CDK_SUCCESS(rv)) {
            rv = bmd_phy_default_ability_set(unit, port, ability);
        }
        
        if (CDK_SUCCESS(rv)) {
            rv = bmd_phy_init(unit, port);
        }
        
        /* Set default as not fiber mode */
        pc = BMD_PORT_PHY_CTRL(unit, port);
        if (pc) {
            PHY_CTRL_FLAGS(pc) &= ~PHY_F_FIBER_MODE;
        }
#endif /* BMD_CONFIG_INCLUDE_PHY == 1 */
    } /* end of MXQPORT PHY init */

    CDK_PBMP_ITER(xlport_pbmp, port) {
#if BMD_CONFIG_INCLUDE_PHY == 1   
        uint32_t ability= 0x0;
        int lanes;    
        
        if (CDK_SUCCESS(rv)) {
            rv = bmd_phy_probe(unit, port);
        }
        lanes = bcm56260_a0_port_num_lanes(unit, port);
        if (CDK_SUCCESS(rv)) {
            if (lanes == 4) {
                rv = bmd_phy_mode_set(unit, port, "tsce",
                                            BMD_PHY_MODE_SERDES, 0);
            } else if (lanes == 2) {
                rv = bmd_phy_mode_set(unit, port, "tsce",
                                            BMD_PHY_MODE_SERDES, 1);
                rv = bmd_phy_mode_set(unit, port, "tsce", 
                                            BMD_PHY_MODE_2LANE, 1);
            } else { /* lanes = 1 */
                rv = bmd_phy_mode_set(unit, port, "tsce",
                                            BMD_PHY_MODE_SERDES, 1);
                rv = bmd_phy_mode_set(unit, port, "tsce", 
                                            BMD_PHY_MODE_2LANE, 0);
            }
        }
        ability = (BMD_PHY_ABIL_1000MB_FD | BMD_PHY_ABIL_100MB_FD);
        if (speed_max == 1000) {
            ability |= BMD_PHY_ABIL_10MB_FD;
        } else if (speed_max == 2500) {
            ability |= BMD_PHY_ABIL_2500MB;
        } else if (speed_max == 10000) {
            ability |= (BMD_PHY_ABIL_10GB | BMD_PHY_ABIL_2500MB);
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
#endif /* BMD_CONFIG_INCLUDE_PHY == 1 */    
    } /* end of XLPORT PHY init */

    CDK_PBMP_ITER(pbmp_all, port) {
        ioerr += bcm56260_b0_xlport_init(unit, port);    
        ioerr += _port_init(unit, port);  
    }

#if BMD_CONFIG_INCLUDE_DMA

    /* Common port initialization for CPU port */
    ioerr += _port_init(unit, CMIC_PORT);

    if (CDK_SUCCESS(rv)) {
        rv = bmd_xgsd_dma_init(unit);
    }

    /* Enable all 48 CPU COS queues for Rx DMA channel */
    if (CDK_SUCCESS(rv)) {
        int idx;
        THDO_QUEUE_DISABLE_CFG2r_t thd0_queue_disable_cfg2;
        THDO_QUEUE_DISABLE_CFG1r_t thd0_queue_disable_cfg1;
        CMIC_CMC_COS_CTRL_RX_0r_t cos_ctrl_0;
        CMIC_CMC_COS_CTRL_RX_1r_t cos_ctrl_1;
        uint32_t cos_bmp;
        uint32_t regval = 0;;

        CMIC_CMC_COS_CTRL_RX_0r_CLR(cos_ctrl_0);
        for (idx = 0; idx < 4; idx++) {
            cos_bmp = (idx == XGSD_DMA_RX_CHAN) ? 0xffffffff : 0;
            CMIC_CMC_COS_CTRL_RX_0r_COS_BMPf_SET(cos_ctrl_0, cos_bmp);
            ioerr += WRITE_CMIC_CMC_COS_CTRL_RX_0r(unit, idx, cos_ctrl_0);
        }
        
        for (idx = 0; idx < 2; idx++) {
            THDO_QUEUE_DISABLE_CFG2r_CLR(thd0_queue_disable_cfg2);
            THDO_QUEUE_DISABLE_CFG2r_QUEUE_NUMf_SET(thd0_queue_disable_cfg2, regval);
            regval ++;
            WRITE_THDO_QUEUE_DISABLE_CFG2r(unit, thd0_queue_disable_cfg2);
            THDO_QUEUE_DISABLE_CFG1r_CLR(thd0_queue_disable_cfg1);
            THDO_QUEUE_DISABLE_CFG1r_QUEUE_WRf_SET(thd0_queue_disable_cfg1, 1);
            WRITE_THDO_QUEUE_DISABLE_CFG1r(unit, thd0_queue_disable_cfg1);
        }

        CMIC_CMC_COS_CTRL_RX_0r_CLR(cos_ctrl_0);
        for (idx = 0; idx < 4; idx++) {
            cos_bmp = (idx == XGSD_DMA_RX_CHAN) ? 0xffffffff : 0;
            CMIC_CMC_COS_CTRL_RX_0r_COS_BMPf_SET(cos_ctrl_0, cos_bmp);
            ioerr += WRITE_CMIC_CMC_COS_CTRL_RX_0r(unit, idx, cos_ctrl_0);
        }

        CMIC_CMC_COS_CTRL_RX_1r_CLR(cos_ctrl_1);
        for (idx = 0; idx < 4; idx++) {
            cos_bmp = (idx == XGSD_DMA_RX_CHAN) ? 0x1ffff : 0;
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
#endif /* CDK_CONFIG_INCLUDE_BCM56260_B0 */

