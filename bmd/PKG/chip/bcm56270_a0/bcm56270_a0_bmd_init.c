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
#include <bmd/bmd_device.h>
#include <cdk/arch/xgsd_chip.h>
#include <cdk/cdk_debug.h>
#include <cdk/cdk_util.h>
#include <cdk/chip/bcm56270_a0_defs.h>
#include <bmd/bmd_phy_ctrl.h>
#include <bmdi/arch/xgsd_dma.h>
#include "bcm56270_a0_bmd.h"
#include "bcm56270_a0_internal.h"

#define PIPE_RESET_TIMEOUT_MSEC         500
#define LLS_RESET_TIMEOUT_MSEC          50

#define NUM_RQEQ_COS                    12
#define NUM_RQEI_COS                    12
#define NUM_RDEQ_COS                    8
#define NUM_RDEI_COS                    8

#define FW_ALIGN_BYTES                  16
#define FW_ALIGN_MASK                   (FW_ALIGN_BYTES - 1)

#define JUMBO_MAXSZ                     0x3fe8
#define TDM_MAX_SIZE                    204

static int
_mmu_tdm_init(int unit)
{
    int ioerr = 0;
    const int *default_tdm_seq;
    int tdm_seq[TDM_MAX_SIZE];
    int idx, tdm_size;
    IARB_TDM_TABLEm_t iarb_tdm_table;
    IARB_TDM_CONTROLr_t iarb_tdm_ctrl;
    LLS_PORT_TDMm_t  lls_tdm;
    LLS_TDM_CAL_CFGr_t cal_cfg;

    /* Get default TDM sequence for this configuration */
    tdm_size = bcm56270_a0_tdm_default(unit, &default_tdm_seq);
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
_mmu_lls_init(int unit)
{
    int ioerr = 0;
    int idx;
    LLS_PORT_CONFIGm_t lls_port_cfg;
    LLS_L0_PARENTm_t l0_parent;
    LLS_L1_PARENTm_t l1_parent;
    LLS_L0_CONFIGm_t l0_config;
    LLS_L1_CONFIGm_t l1_config;
    LLS_L2_PARENTm_t l2_parent;
    int port, ppport;
    cdk_pbmp_t mmu_pbmp, pbmp;
    uint32_t q_offset;
    int q_limit;
    ING_COS_MODEr_t cos_mode;
    RQE_PP_PORT_CONFIGr_t rqe_port;
    EGR_QUEUE_TO_PP_PORT_MAPm_t egrq_pp_map;

    CDK_PBMP_CLEAR(mmu_pbmp);
    CDK_PBMP_ADD(mmu_pbmp, CMIC_PORT);
    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_MXQPORT, &pbmp);
    CDK_PBMP_OR(mmu_pbmp, pbmp);
    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_XLPORT, &pbmp);
    CDK_PBMP_OR(mmu_pbmp, pbmp);

    /*        CMIC_port     port1        port2        port3         ...
        Port: [0]           [1]          [2]          [3]           ...
        L0  : [0]           [5]          [6]          [7]
        L1  : [0]           [0xa]        [0xb]        [0xc]         ...
        L2  : [0]           [0x48..0x4f] [0x50..0x57] [0x58..0x5f]  ...
    */
    /* LLS Queue Configuration */
    CDK_PBMP_ITER(mmu_pbmp, port) {
        LLS_PORT_CONFIGm_CLR(lls_port_cfg);
        if (port == CMIC_PORT) {
            LLS_PORT_CONFIGm_P_NUM_SPRIf_SET(lls_port_cfg, 0xf);
            LLS_PORT_CONFIGm_P_START_SPRIf_SET(lls_port_cfg, port);
        } else {
            LLS_PORT_CONFIGm_P_NUM_SPRIf_SET(lls_port_cfg, 1);
            LLS_PORT_CONFIGm_P_START_SPRIf_SET(lls_port_cfg, port + 4);
        }
        ioerr += WRITE_LLS_PORT_CONFIGm(unit, port, lls_port_cfg);
    }

    CDK_PBMP_ITER(mmu_pbmp, port) {
        LLS_L0_PARENTm_CLR(l0_parent);
        LLS_L0_PARENTm_C_TYPEf_SET(l0_parent, 1);
        if (port == CMIC_PORT) {
            LLS_L0_PARENTm_C_PARENTf_SET(l0_parent, CMIC_PORT);
            ioerr += WRITE_LLS_L0_PARENTm(unit, port, l0_parent);
        } else {
            LLS_L0_PARENTm_C_PARENTf_SET(l0_parent, port);
            ioerr += WRITE_LLS_L0_PARENTm(unit, port + 4, l0_parent);
        }
    }
    CDK_PBMP_ITER(mmu_pbmp, port) {
        LLS_L0_CONFIGm_CLR(l0_config);
        if (port == CMIC_PORT) {
            LLS_L0_CONFIGm_P_NUM_SPRIf_SET(l0_config, 0x1);
            LLS_L0_CONFIGm_P_START_SPRIf_SET(l0_config, CMIC_PORT);
            ioerr += WRITE_LLS_L0_CONFIGm(unit, port, l0_config);
        } else {
            LLS_L0_CONFIGm_P_NUM_SPRIf_SET(l0_config, 0x1);
            LLS_L0_CONFIGm_P_START_SPRIf_SET(l0_config, port + 9);
            ioerr += WRITE_LLS_L0_CONFIGm(unit, port + 4, l0_config);
        }
    }

    CDK_PBMP_ITER(mmu_pbmp, port) {
        LLS_L1_PARENTm_CLR(l1_parent);
        if (port == CMIC_PORT) {
            LLS_L1_PARENTm_C_PARENTf_SET(l1_parent, CMIC_PORT);
            ioerr += WRITE_LLS_L1_PARENTm(unit, port, l1_parent);
        } else {
            LLS_L1_PARENTm_C_PARENTf_SET(l1_parent, port + 4);
            ioerr += WRITE_LLS_L1_PARENTm(unit, port + 9, l1_parent);
        }

        LLS_L1_CONFIGm_CLR(l1_config);
        LLS_L1_CONFIGm_P_NUM_SPRIf_SET(l1_config, 0xf);
        if (port == CMIC_PORT) {
            LLS_L1_CONFIGm_P_START_UC_SPRIf_SET(l1_config, 0);
            ioerr += WRITE_LLS_L1_CONFIGm(unit, port, l1_config);
        } else {
            LLS_L1_CONFIGm_P_START_UC_SPRIf_SET(l1_config, (port * 8) + 0x40);
            ioerr += WRITE_LLS_L1_CONFIGm(unit, port + 9, l1_config);
        }

        LLS_L2_PARENTm_CLR(l2_parent);
        for (idx = 0; idx < 8 ; idx++) {
            if (port == CMIC_PORT) {
                LLS_L2_PARENTm_C_PARENTf_SET(l2_parent, port);
                ioerr += WRITE_LLS_L2_PARENTm(unit, idx, l2_parent);
            } else {
                LLS_L2_PARENTm_C_PARENTf_SET(l2_parent, port + 9);
                ioerr += WRITE_LLS_L2_PARENTm(unit, ((port * 8) + 0x40 + idx), l2_parent);
            }
        }
    }

    CDK_PBMP_ITER(mmu_pbmp, port) {
        /* Assign queues to ports */
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
            RQE_PP_PORT_CONFIGr_COS_MODEf_SET(rqe_port, 0);
            ioerr += WRITE_RQE_PP_PORT_CONFIGr(unit, ppport, rqe_port);

            for (idx = 0; idx < q_limit; idx++) {
                ioerr += READ_EGR_QUEUE_TO_PP_PORT_MAPm(unit,
                                        (q_offset + idx), &egrq_pp_map);
                EGR_QUEUE_TO_PP_PORT_MAPm_PP_PORTf_SET(egrq_pp_map, ppport);
                ioerr += WRITE_EGR_QUEUE_TO_PP_PORT_MAPm(unit,
                                        (q_offset + idx), egrq_pp_map);
            }
        }
    }
    return ioerr;
}

static void
_mmu_buffer_pools_init(int unit)
{
    int ioerr = 0;
    cdk_pbmp_t mmu_pbmp, pbmp;
    int port, mport;
    int idx, op_node, ceil, q_limit;
    uint32_t q_offset;
    CFAPICONFIGr_t cfapconfig;
    uint32_t mmu_int_buf_size = 10922;
    uint32_t int_size_after_limitation = 10880;
    CFAPIFULLSETPOINTr_t cfapfsp;
    CFAPIFULLRESETPOINTr_t cfapifullrp;
    MMU_ENQ_FAPCONFIG_0r_t mmu_enq_fapconfig_0;
    MMU_ENQ_FAPFULLSETPOINT_0r_t mmu_enq_fapfspoint_0;
    MMU_ENQ_FAPFULLRESETPOINT_0r_t mmu_enq_fapfrspoint_0;
    QSTRUCT_FAPCONFIGr_t qstruct_fapcfg;
    QSTRUCT_FAPFULLSETPOINTr_t qstruct_fapfspoint;
    QSTRUCT_FAPFULLRESETPOINTr_t qstruct_fapfrspoint;
    THDO_MISCCONFIGr_t thd0_misc;
    WRED_MISCCONFIGr_t wred_misccfg;
    COLOR_AWAREr_t color_aware;
    GLOBAL_HDRM_LIMITr_t glb_hdrm;
    uint32_t total_shared_ing_buff_pool = 0x26c7;
    BUFFER_CELL_LIMIT_SPr_t buf_cell;
    CELL_RESET_LIMIT_OFFSET_SPr_t cell_reset;
    THDIRQE_GLOBAL_HDRM_LIMITr_t irqe_glb_hdrm;
    uint32_t total_shared_re_w_buff = 0x34b;
    THDIRQE_CELL_RESET_LIMIT_OFFSET_SPr_t irqe_cell_reset;
    THDIQEN_GLOBAL_HDRM_LIMITr_t iqen_glb_hdrm;
    uint32_t eqe_buff = 0x2838;
    THDIQEN_BUFFER_CELL_LIMIT_SPr_t iqen_buf_cell;
    THDIRQE_BUFFER_CELL_LIMIT_SPr_t irqe_buf_cell;
    THDIQEN_CELL_RESET_LIMIT_OFFSET_SPr_t iqen_cell_reset;
    OP_BUFFER_SHARED_LIMIT_CELLIr_t op_shr_cell;
    OP_BUFFER_SHARED_LIMIT_RESUME_CELLIr_t op_sh_res_cell;
    OP_BUFFER_LIMIT_RESUME_YELLOW_CELLIr_t op_res_yel_cell;
    OP_BUFFER_LIMIT_RESUME_RED_CELLIr_t op_res_red_cell;
    OP_BUFFER_LIMIT_YELLOW_CELLIr_t op_yel_cell;
    OP_BUFFER_LIMIT_RED_CELLIr_t op_red_cell;
    OP_BUFFER_SHARED_LIMIT_THDORQEQr_t op_shr_rqeq;
    OP_BUFFER_LIMIT_YELLOW_THDORQEQr_t op_yel_rqeq;
    OP_BUFFER_LIMIT_RED_THDORQEQr_t op_red_rqeq;
    OP_BUFFER_SHARED_LIMIT_RESUME_THDORQEQr_t op_sh_res_rqeq;
    OP_BUFFER_LIMIT_RESUME_YELLOW_THDORQEQr_t op_res_yel_rqeq;
    OP_BUFFER_LIMIT_RESUME_RED_THDORQEQr_t op_res_red_rqeq;
    OP_BUFFER_SHARED_LIMIT_QENTRYr_t op_shr_qentry;
    OP_BUFFER_LIMIT_YELLOW_QENTRYr_t op_yel_qentry;
    OP_BUFFER_LIMIT_RED_QENTRYr_t op_red_qentry;
    OP_BUFFER_SHARED_LIMIT_RESUME_QENTRYr_t op_sh_res_qentry;
    OP_BUFFER_LIMIT_RESUME_YELLOW_QENTRYr_t op_res_yel_qentry;
    OP_BUFFER_LIMIT_RESUME_RED_QENTRYr_t op_res_red_qentry;
    OP_BUFFER_SHARED_LIMIT_THDORDEQr_t op_sh_rdeq;
    OP_BUFFER_SHARED_LIMIT_RESUME_THDORDEQr_t op_sh_res_rdeq;
    OP_BUFFER_LIMIT_YELLOW_THDORDEQr_t op_yel_rdeq;
    OP_BUFFER_LIMIT_RED_THDORDEQr_t op_red_rdeq;
    OP_BUFFER_LIMIT_RESUME_YELLOW_THDORDEQr_t op_res_yel_rdeq;
    OP_BUFFER_LIMIT_RESUME_RED_THDORDEQr_t op_res_red_rdeq;
    THDI_PORT_SP_CONFIGm_t port_sp_cfg;
    THDI_PORT_PG_CONFIGm_t port_pg_cfg;
    THDIRQE_THDI_PORT_SP_CONFIGm_t irqe_port_sp_cfg;
    THDIRQE_THDI_PORT_PG_CONFIGm_t irqe_port_pg_cfg;
    THDIQEN_THDI_PORT_SP_CONFIGm_t iqen_port_sp_cfg;
    THDIQEN_THDI_PORT_PG_CONFIGm_t iqen_port_pg_cfg;
    PORT_MAX_PKT_SIZEr_t port_max_pkt_size;
    MMU_THDO_OPNCONFIG_CELLm_t opncfg_cell;
    MMU_THDO_QCONFIG_CELLm_t qcfg_cell;
    MMU_THDO_QOFFSET_CELLm_t qoff_cell;
    MMU_THDO_OPNCONFIG_QENTRYm_t opncfg_qentry;
    MMU_THDO_QCONFIG_QENTRYm_t qcfg_qentry;
    MMU_THDO_QOFFSET_QENTRYm_t qoff_qentry;
    OP_QUEUE_CONFIG_THDORQEIr_t opq_rqei;
    OP_QUEUE_CONFIG_THDORQEQr_t opq_rqeq;
    OP_QUEUE_CONFIG1_THDORQEIr_t opq_rqei1;
    OP_QUEUE_CONFIG1_THDORQEQr_t opq_rqeq1;
    OP_QUEUE_RESET_OFFSET_YELLOW_THDORQEIr_t opq_rst_yel_rqei;
    OP_QUEUE_RESET_OFFSET_RED_THDORQEIr_t opq_rst_red_rqei;
    OP_QUEUE_RESET_OFFSET_YELLOW_THDORQEQr_t opq_rst_yel_rqeq;
    OP_QUEUE_RESET_OFFSET_RED_THDORQEQr_t opq_rst_red_rqeq;
    OP_QUEUE_RESET_OFFSET_THDORQEQr_t opq_rst_rqeq;
    OP_QUEUE_LIMIT_YELLOW_THDORQEQr_t opq_yel_rqeq;
    OP_QUEUE_RESET_OFFSET_THDORQEIr_t opq_rst_rqei;
    OP_QUEUE_LIMIT_YELLOW_THDORQEIr_t opq_yel_rqei;
    OP_QUEUE_LIMIT_RED_THDORQEIr_t opq_red_rqei;
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
    uint32_t temp_val;
    uint32_t ml_port_mapping[14][2] =
        { {0,0}, {1,15}, {2,23}, {3, 31}, {4, 39}, {5,47}, {6, 55},
          {7,63}, {8, 71}, {9, 79}, {21, 175}, {22, 183}, {23, 191}, {29,239}};
    uint32_t total_egr_base_queues_in_device = 168;

    CDK_PBMP_CLEAR(mmu_pbmp);
    CDK_PBMP_ADD(mmu_pbmp, CMIC_PORT);
    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_MXQPORT, &pbmp);
    CDK_PBMP_OR(mmu_pbmp, pbmp);
    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_XLPORT, &pbmp);
    CDK_PBMP_OR(mmu_pbmp, pbmp);

    /* ===================== */
    /* Device Wide Registers */
    /* ===================== */
    ioerr += READ_CFAPICONFIGr(unit, &cfapconfig);
    CFAPICONFIGr_CFAPIPOOLSIZEf_SET(cfapconfig, mmu_int_buf_size + 22);
    ioerr += WRITE_CFAPICONFIGr(unit, cfapconfig);

    ioerr += READ_CFAPIFULLSETPOINTr(unit, &cfapfsp);
    CFAPIFULLSETPOINTr_CFAPIFULLSETPOINTf_SET(cfapfsp, int_size_after_limitation);
    ioerr += WRITE_CFAPIFULLSETPOINTr(unit, cfapfsp);

    ioerr += READ_CFAPIFULLRESETPOINTr(unit, &cfapifullrp);
    CFAPIFULLRESETPOINTr_SET(cfapifullrp, mmu_int_buf_size + 22 - 128);
    ioerr += WRITE_CFAPIFULLRESETPOINTr(unit, cfapifullrp);

    ioerr += READ_MMU_ENQ_FAPCONFIG_0r(unit, &mmu_enq_fapconfig_0);
    MMU_ENQ_FAPCONFIG_0r_FAPPOOLSIZEf_SET(mmu_enq_fapconfig_0, 1344);
    ioerr += WRITE_MMU_ENQ_FAPCONFIG_0r(unit, mmu_enq_fapconfig_0);

    temp_val = 1344;
    ioerr += READ_MMU_ENQ_FAPFULLSETPOINT_0r(unit, &mmu_enq_fapfspoint_0);
    MMU_ENQ_FAPFULLSETPOINT_0r_FAPFULLSETPOINTf_SET
                                        (mmu_enq_fapfspoint_0, temp_val - 64);
    ioerr += WRITE_MMU_ENQ_FAPFULLSETPOINT_0r(unit, mmu_enq_fapfspoint_0);

    ioerr += READ_MMU_ENQ_FAPFULLRESETPOINT_0r(unit, &mmu_enq_fapfrspoint_0);
    MMU_ENQ_FAPFULLRESETPOINT_0r_FAPFULLRESETPOINTf_SET
                                        (mmu_enq_fapfrspoint_0, temp_val - 128);
    ioerr += WRITE_MMU_ENQ_FAPFULLRESETPOINT_0r(unit, mmu_enq_fapfrspoint_0);

    temp_val = ((12 * 1024 ) + (1024 *3))/4;
    ioerr += READ_QSTRUCT_FAPCONFIGr(unit, 0, &qstruct_fapcfg);
    QSTRUCT_FAPCONFIGr_FAPPOOLSIZEf_SET(qstruct_fapcfg, temp_val);
    ioerr += WRITE_QSTRUCT_FAPCONFIGr(unit, 0, qstruct_fapcfg);

    temp_val = temp_val - 8;
    ioerr += READ_QSTRUCT_FAPFULLSETPOINTr(unit, 0, &qstruct_fapfspoint);
    QSTRUCT_FAPFULLSETPOINTr_FAPFULLSETPOINTf_SET(qstruct_fapfspoint, temp_val);
    ioerr += WRITE_QSTRUCT_FAPFULLSETPOINTr(unit, 0, qstruct_fapfspoint);

    temp_val = temp_val -16;
    ioerr += READ_QSTRUCT_FAPFULLRESETPOINTr(unit, 0, &qstruct_fapfrspoint);
    QSTRUCT_FAPFULLRESETPOINTr_FAPFULLRESETPOINTf_SET(qstruct_fapfrspoint,
                                                temp_val);
    ioerr += WRITE_QSTRUCT_FAPFULLRESETPOINTr(unit, 0, qstruct_fapfrspoint);

    ioerr += READ_THDO_MISCCONFIGr(unit, &thd0_misc);
    THDO_MISCCONFIGr_UCMC_SEPARATIONf_SET(thd0_misc, 0);
    ioerr += WRITE_THDO_MISCCONFIGr(unit, thd0_misc);

    ioerr += READ_WRED_MISCCONFIGr(unit, &wred_misccfg);
    WRED_MISCCONFIGr_UCMC_SEPARATIONf_SET(wred_misccfg, 0);
    ioerr += WRITE_WRED_MISCCONFIGr(unit, wred_misccfg);

    ioerr += READ_COLOR_AWAREr(unit, &color_aware);
    COLOR_AWAREr_ENABLEf_SET(color_aware, 0);
    ioerr += WRITE_COLOR_AWAREr(unit, color_aware);

    GLOBAL_HDRM_LIMITr_SET(glb_hdrm, 130);
    ioerr += WRITE_GLOBAL_HDRM_LIMITr(unit, glb_hdrm);

    BUFFER_CELL_LIMIT_SPr_SET(buf_cell, total_shared_ing_buff_pool);
    ioerr += WRITE_BUFFER_CELL_LIMIT_SPr(unit, 0, buf_cell);

    CELL_RESET_LIMIT_OFFSET_SPr_SET(cell_reset, 0xf00);
    ioerr += WRITE_CELL_RESET_LIMIT_OFFSET_SPr(unit, 0, cell_reset);

    THDIRQE_GLOBAL_HDRM_LIMITr_SET(irqe_glb_hdrm, 0);
    ioerr += WRITE_THDIRQE_GLOBAL_HDRM_LIMITr(unit, irqe_glb_hdrm);

    THDIRQE_BUFFER_CELL_LIMIT_SPr_SET(irqe_buf_cell, total_shared_re_w_buff);
    ioerr += WRITE_THDIRQE_BUFFER_CELL_LIMIT_SPr(unit, 0, irqe_buf_cell);

    THDIRQE_CELL_RESET_LIMIT_OFFSET_SPr_SET(irqe_cell_reset, 3);
    ioerr += WRITE_THDIRQE_CELL_RESET_LIMIT_OFFSET_SPr(unit, 0, irqe_cell_reset);

    THDIQEN_GLOBAL_HDRM_LIMITr_SET(iqen_glb_hdrm, 0);
    ioerr += WRITE_THDIQEN_GLOBAL_HDRM_LIMITr(unit, iqen_glb_hdrm);

    THDIQEN_BUFFER_CELL_LIMIT_SPr_SET(iqen_buf_cell, eqe_buff);
    ioerr += WRITE_THDIQEN_BUFFER_CELL_LIMIT_SPr(unit, 0, iqen_buf_cell);

    THDIQEN_CELL_RESET_LIMIT_OFFSET_SPr_SET(iqen_cell_reset, 0x1b);
    ioerr += WRITE_THDIQEN_CELL_RESET_LIMIT_OFFSET_SPr(unit, 0, iqen_cell_reset);
    OP_BUFFER_SHARED_LIMIT_CELLIr_SET(op_shr_cell, 0x242c);
    ioerr += WRITE_OP_BUFFER_SHARED_LIMIT_CELLIr(unit, op_shr_cell);

    OP_BUFFER_SHARED_LIMIT_RESUME_CELLIr_SET(op_sh_res_cell, 0x242c);
    ioerr += WRITE_OP_BUFFER_SHARED_LIMIT_RESUME_CELLIr(unit, op_sh_res_cell);

    OP_BUFFER_LIMIT_RESUME_YELLOW_CELLIr_SET(op_res_yel_cell, 0x483);
    ioerr += WRITE_OP_BUFFER_LIMIT_RESUME_YELLOW_CELLIr(unit, op_res_yel_cell);

    OP_BUFFER_LIMIT_RESUME_RED_CELLIr_SET(op_res_red_cell, 0x483);
    ioerr += WRITE_OP_BUFFER_LIMIT_RESUME_RED_CELLIr(unit, op_res_red_cell);

    OP_BUFFER_LIMIT_YELLOW_CELLIr_SET(op_yel_cell, 0x486);
    ioerr += WRITE_OP_BUFFER_LIMIT_YELLOW_CELLIr(unit, op_yel_cell);

    OP_BUFFER_LIMIT_RED_CELLIr_SET(op_red_cell, 0x486);
    ioerr += WRITE_OP_BUFFER_LIMIT_RED_CELLIr(unit, op_red_cell);

    OP_BUFFER_SHARED_LIMIT_THDORQEQr_SET(op_shr_rqeq, 0x242c);
    ioerr += WRITE_OP_BUFFER_SHARED_LIMIT_THDORQEQr(unit, op_shr_rqeq);

    OP_BUFFER_LIMIT_YELLOW_THDORQEQr_SET(op_yel_rqeq, 0x73);
    ioerr += WRITE_OP_BUFFER_LIMIT_YELLOW_THDORQEQr(unit, op_yel_rqeq);

    OP_BUFFER_LIMIT_RED_THDORQEQr_SET(op_red_rqeq, 0x73);
    ioerr += WRITE_OP_BUFFER_LIMIT_RED_THDORQEQr(unit, op_red_rqeq);

    OP_BUFFER_SHARED_LIMIT_RESUME_THDORQEQr_SET(op_sh_res_rqeq, 0x391);
    ioerr += WRITE_OP_BUFFER_SHARED_LIMIT_RESUME_THDORQEQr(unit, op_sh_res_rqeq);

    OP_BUFFER_LIMIT_RESUME_YELLOW_THDORQEQr_SET(op_res_yel_rqeq, 0x72);
    ioerr += WRITE_OP_BUFFER_LIMIT_RESUME_YELLOW_THDORQEQr(unit, op_res_yel_rqeq);

    OP_BUFFER_LIMIT_RESUME_RED_THDORQEQr_SET(op_res_red_rqeq, 0x72);
    ioerr += WRITE_OP_BUFFER_LIMIT_RESUME_RED_THDORQEQr(unit, op_res_red_rqeq);

    OP_BUFFER_SHARED_LIMIT_QENTRYr_SET(op_shr_qentry, 0xffff);
    ioerr += WRITE_OP_BUFFER_SHARED_LIMIT_QENTRYr(unit, op_shr_qentry);

    OP_BUFFER_LIMIT_YELLOW_QENTRYr_SET(op_yel_qentry, 0x543);
    ioerr += WRITE_OP_BUFFER_LIMIT_YELLOW_QENTRYr(unit, op_yel_qentry);

    OP_BUFFER_LIMIT_RED_QENTRYr_SET(op_red_qentry, 0x543);
    ioerr += WRITE_OP_BUFFER_LIMIT_RED_QENTRYr(unit, op_red_qentry);

    OP_BUFFER_SHARED_LIMIT_RESUME_QENTRYr_SET(op_sh_res_qentry, 0x29f6);
    ioerr += WRITE_OP_BUFFER_SHARED_LIMIT_RESUME_QENTRYr(unit, op_sh_res_qentry);

    OP_BUFFER_LIMIT_RESUME_YELLOW_QENTRYr_SET(op_res_yel_qentry, 0x1ff8);
    ioerr += WRITE_OP_BUFFER_LIMIT_RESUME_YELLOW_QENTRYr(unit, op_res_yel_qentry);

    OP_BUFFER_LIMIT_RESUME_RED_QENTRYr_SET(op_res_red_qentry, 0x53f);
    ioerr += (WRITE_OP_BUFFER_LIMIT_RESUME_RED_QENTRYr(unit, op_res_red_qentry));

    OP_BUFFER_SHARED_LIMIT_THDORDEQr_SET(op_sh_rdeq, 0x1c0);
    ioerr += WRITE_OP_BUFFER_SHARED_LIMIT_THDORDEQr(unit, op_sh_rdeq);

    OP_BUFFER_SHARED_LIMIT_RESUME_THDORDEQr_SET(op_sh_res_rdeq, 0x1c0 - 4);
    ioerr += WRITE_OP_BUFFER_SHARED_LIMIT_RESUME_THDORDEQr(unit, op_sh_res_rdeq);

    OP_BUFFER_LIMIT_YELLOW_THDORDEQr_SET(op_yel_rdeq, 0x38);
    ioerr += WRITE_OP_BUFFER_LIMIT_YELLOW_THDORDEQr(unit, op_yel_rdeq);

    OP_BUFFER_LIMIT_RED_THDORDEQr_SET(op_red_rdeq, 0x38);
    ioerr += WRITE_OP_BUFFER_LIMIT_RED_THDORDEQr(unit, op_red_rdeq);

    OP_BUFFER_LIMIT_RESUME_YELLOW_THDORDEQr_SET(op_res_yel_rdeq, 0x38 - 1);
    ioerr += WRITE_OP_BUFFER_LIMIT_RESUME_YELLOW_THDORDEQr(unit, op_res_yel_rdeq);

    OP_BUFFER_LIMIT_RESUME_RED_THDORDEQr_SET(op_res_red_rdeq, 0x38 - 1);
    ioerr += WRITE_OP_BUFFER_LIMIT_RESUME_RED_THDORDEQr(unit, op_res_red_rdeq);

    /* Per Port Registers */
    /* ######################## */
    /* 1. Input Port Thresholds */
    /* ######################## */
    /* 1.1 Internal Buffer Ingress Pool */
    CDK_PBMP_ITER(mmu_pbmp, port) {
        mport = P2M(unit, ml_port_mapping[port][0]);

        THDI_PORT_SP_CONFIGm_CLR(port_sp_cfg);
        THDI_PORT_SP_CONFIGm_PORT_SP_MAX_LIMITf_SET(port_sp_cfg,
                                            total_shared_ing_buff_pool);
        THDI_PORT_SP_CONFIGm_PORT_SP_RESUME_LIMITf_SET(port_sp_cfg,
                                            total_shared_ing_buff_pool - 2 * 0x9);
        ioerr += WRITE_THDI_PORT_SP_CONFIGm(unit, (mport * 4), port_sp_cfg);

        ioerr += READ_PORT_MAX_PKT_SIZEr(unit, port, &port_max_pkt_size);
        PORT_MAX_PKT_SIZEr_PORT_MAX_PKT_SIZEf_SET(port_max_pkt_size, 0x41);
        ioerr += WRITE_PORT_MAX_PKT_SIZEr(unit, port, port_max_pkt_size);

        THDI_PORT_PG_CONFIGm_CLR(port_pg_cfg);
        THDI_PORT_PG_CONFIGm_PG_MIN_LIMITf_SET(port_pg_cfg, 0x31);
        THDI_PORT_PG_CONFIGm_PG_SHARED_LIMITf_SET
                                    (port_pg_cfg, total_shared_ing_buff_pool);
        THDI_PORT_PG_CONFIGm_PG_RESET_OFFSETf_SET(port_pg_cfg, 2 * 0x9);
        if (port == CMIC_PORT) {
            THDI_PORT_PG_CONFIGm_PG_HDRM_LIMITf_SET(port_pg_cfg, 0x2c);
        } else if (BMD_PORT_PROPERTIES(unit, port) == BMD_PORT_GE) {
            THDI_PORT_PG_CONFIGm_PG_HDRM_LIMITf_SET(port_pg_cfg, 0);
        } else {
            THDI_PORT_PG_CONFIGm_PG_HDRM_LIMITf_SET(port_pg_cfg, 0);
        }
        THDI_PORT_PG_CONFIGm_PG_GBL_HDRM_ENf_SET(port_pg_cfg, 1);
        THDI_PORT_PG_CONFIGm_SP_SHARED_MAX_ENABLEf_SET(port_pg_cfg, 1);
        THDI_PORT_PG_CONFIGm_SP_MIN_PG_ENABLEf_SET(port_pg_cfg, 1);
        mport = P2M(unit, ml_port_mapping[port][1]);
        ioerr += WRITE_THDI_PORT_PG_CONFIGm(unit, mport, port_pg_cfg);

        mport = P2M(unit, ml_port_mapping[port][0]);
        THDIRQE_THDI_PORT_SP_CONFIGm_CLR(irqe_port_sp_cfg);
        THDIRQE_THDI_PORT_SP_CONFIGm_PORT_SP_MAX_LIMITf_SET
                                                        (irqe_port_sp_cfg, 0x34b);
        THDIRQE_THDI_PORT_SP_CONFIGm_PORT_SP_RESUME_LIMITf_SET
                                                    (irqe_port_sp_cfg, 0x34b - 1);
        ioerr += WRITE_THDIRQE_THDI_PORT_SP_CONFIGm
                                            (unit, (mport * 4), irqe_port_sp_cfg);

        mport = P2M(unit, ml_port_mapping[port][1]);
        THDIRQE_THDI_PORT_PG_CONFIGm_CLR(irqe_port_pg_cfg);
        THDIRQE_THDI_PORT_PG_CONFIGm_PG_MIN_LIMITf_SET(irqe_port_pg_cfg, 0x2c);
        THDIRQE_THDI_PORT_PG_CONFIGm_PG_SHARED_LIMITf_SET(irqe_port_pg_cfg, 0x9);
        THDIRQE_THDI_PORT_PG_CONFIGm_PG_RESET_OFFSETf_SET(irqe_port_pg_cfg, 2);
        THDIRQE_THDI_PORT_PG_CONFIGm_PG_SHARED_DYNAMICf_SET(irqe_port_pg_cfg, 0);
        if (port == CMIC_PORT) {
            THDIRQE_THDI_PORT_PG_CONFIGm_PG_HDRM_LIMITf_SET
                                                        (irqe_port_pg_cfg, 0x17);
        } else if (BMD_PORT_PROPERTIES(unit, port) == BMD_PORT_GE) {
            THDIRQE_THDI_PORT_PG_CONFIGm_PG_HDRM_LIMITf_SET(irqe_port_pg_cfg, 0);
        } else {
            THDIRQE_THDI_PORT_PG_CONFIGm_PG_HDRM_LIMITf_SET(irqe_port_pg_cfg, 0);
        }
        THDIRQE_THDI_PORT_PG_CONFIGm_PG_GBL_HDRM_ENf_SET(irqe_port_pg_cfg, 0);
        THDIRQE_THDI_PORT_PG_CONFIGm_SP_SHARED_MAX_ENABLEf_SET
                                                        (irqe_port_pg_cfg, 1);
        THDIRQE_THDI_PORT_PG_CONFIGm_SP_MIN_PG_ENABLEf_SET(irqe_port_pg_cfg, 1);
        ioerr += WRITE_THDIRQE_THDI_PORT_PG_CONFIGm(unit, mport, irqe_port_pg_cfg);

        mport = P2M(unit, ml_port_mapping[port][0]);
        THDIQEN_THDI_PORT_SP_CONFIGm_CLR(iqen_port_sp_cfg);
        THDIQEN_THDI_PORT_SP_CONFIGm_PORT_SP_MAX_LIMITf_SET
                                                    (iqen_port_sp_cfg, 0x2838);
        THDIQEN_THDI_PORT_SP_CONFIGm_PORT_SP_RESUME_LIMITf_SET
                                                    (iqen_port_sp_cfg, 0x2822);
        ioerr += WRITE_THDIQEN_THDI_PORT_SP_CONFIGm
                                            (unit, (mport * 4), iqen_port_sp_cfg);

        mport = P2M(unit, ml_port_mapping[port][1]);
        THDIQEN_THDI_PORT_PG_CONFIGm_CLR(iqen_port_pg_cfg);
        THDIQEN_THDI_PORT_PG_CONFIGm_PG_MIN_LIMITf_SET(iqen_port_pg_cfg, 0x63);
        THDIQEN_THDI_PORT_PG_CONFIGm_PG_SHARED_LIMITf_SET(iqen_port_pg_cfg, 0);
        THDIQEN_THDI_PORT_PG_CONFIGm_PG_RESET_OFFSETf_SET(iqen_port_pg_cfg, 0x16);
        THDIQEN_THDI_PORT_PG_CONFIGm_PG_RESET_FLOORf_SET(iqen_port_pg_cfg, 0);
        THDIQEN_THDI_PORT_PG_CONFIGm_PG_SHARED_DYNAMICf_SET(iqen_port_pg_cfg, 0);
        if (mport == CMIC_PORT) {
            THDIQEN_THDI_PORT_PG_CONFIGm_PG_HDRM_LIMITf_SET(iqen_port_pg_cfg, 0xfd);
        } else if (BMD_PORT_PROPERTIES(unit, port) == BMD_PORT_GE) {
            THDIQEN_THDI_PORT_PG_CONFIGm_PG_HDRM_LIMITf_SET(iqen_port_pg_cfg, 0);
        } else {
            THDIQEN_THDI_PORT_PG_CONFIGm_PG_HDRM_LIMITf_SET(iqen_port_pg_cfg, 0);
        }
        THDIQEN_THDI_PORT_PG_CONFIGm_PG_GBL_HDRM_ENf_SET(iqen_port_pg_cfg, 0);
        THDIQEN_THDI_PORT_PG_CONFIGm_SP_SHARED_MAX_ENABLEf_SET(iqen_port_pg_cfg, 1);
        THDIQEN_THDI_PORT_PG_CONFIGm_SP_MIN_PG_ENABLEf_SET(iqen_port_pg_cfg, 1);
        ioerr += WRITE_THDIQEN_THDI_PORT_PG_CONFIGm(unit, mport, iqen_port_pg_cfg);

        if (mport == CMIC_PORT) {
            op_node = 0;
            ceil = 6;
            q_offset = UC_COSQ_BASE(unit, CMIC_PORT);
            q_limit = NUM_UC_COSQ(unit, CMIC_PORT);
        } else {
            op_node = mport + 8;
            ceil = 1;
            q_offset = UC_COSQ_BASE(unit, port);
            q_limit = NUM_UC_COSQ(unit, port);
        }

        for (idx = 0; idx < ceil; idx++) {
            ioerr += READ_MMU_THDO_OPNCONFIG_CELLm
                                            (unit, (op_node + idx), &opncfg_cell);
            MMU_THDO_OPNCONFIG_CELLm_OPN_SHARED_LIMIT_CELLf_SET
                                                        (opncfg_cell, 0x242c);
            MMU_THDO_OPNCONFIG_CELLm_OPN_SHARED_RESET_VALUE_CELLf_SET
                                                        (opncfg_cell, 0x2a50);
            MMU_THDO_OPNCONFIG_CELLm_PORT_LIMIT_ENABLE_CELLf_SET(opncfg_cell, 0);
            ioerr += WRITE_MMU_THDO_OPNCONFIG_CELLm
                                            (unit, (op_node + idx), opncfg_cell);
        }

        for (idx = 0; idx < q_limit; idx++) {
            ioerr += READ_MMU_THDO_QCONFIG_CELLm
                                            (unit, (q_offset + idx), &qcfg_cell);
            if ((q_offset + idx) < total_egr_base_queues_in_device) {
                MMU_THDO_QCONFIG_CELLm_Q_MIN_CELLf_SET(qcfg_cell, 9);
            } else {
                MMU_THDO_QCONFIG_CELLm_Q_MIN_CELLf_SET(qcfg_cell, 0);
            }
            MMU_THDO_QCONFIG_CELLm_Q_SHARED_LIMIT_CELLf_SET(qcfg_cell, 0x241a);
            MMU_THDO_QCONFIG_CELLm_Q_LIMIT_DYNAMIC_CELLf_SET(qcfg_cell, 1);
            MMU_THDO_QCONFIG_CELLm_Q_LIMIT_ENABLE_CELLf_SET(qcfg_cell, 1);
            MMU_THDO_QCONFIG_CELLm_Q_COLOR_ENABLE_CELLf_SET(qcfg_cell, 0);
            MMU_THDO_QCONFIG_CELLm_Q_COLOR_LIMIT_DYNAMIC_CELLf_SET(qcfg_cell, 1);
            MMU_THDO_QCONFIG_CELLm_LIMIT_YELLOW_CELLf_SET(qcfg_cell, 0);
            ioerr += WRITE_MMU_THDO_QCONFIG_CELLm
                                            (unit, (q_offset + idx), qcfg_cell);

            ioerr += READ_MMU_THDO_QOFFSET_CELLm
                                            (unit, (q_offset + idx), &qoff_cell);
            MMU_THDO_QOFFSET_CELLm_RESET_OFFSET_CELLf_SET(qoff_cell, 2);
            MMU_THDO_QOFFSET_CELLm_LIMIT_RED_CELLf_SET(qoff_cell, 0);
            MMU_THDO_QOFFSET_CELLm_RESET_OFFSET_YELLOW_CELLf_SET(qoff_cell, 2);
            MMU_THDO_QOFFSET_CELLm_RESET_OFFSET_RED_CELLf_SET(qoff_cell, 2);
            ioerr += WRITE_MMU_THDO_QOFFSET_CELLm
                                            (unit, (q_offset + idx), qoff_cell);
        }

        for (idx = 0; idx < ceil; idx++) {
            ioerr += READ_MMU_THDO_OPNCONFIG_QENTRYm
                                        (unit, (op_node + idx), &opncfg_qentry);
            MMU_THDO_OPNCONFIG_QENTRYm_OPN_SHARED_LIMIT_QENTRYf_SET
                                                        (opncfg_qentry, 0x2a17);
            MMU_THDO_OPNCONFIG_QENTRYm_OPN_SHARED_RESET_VALUE_QENTRYf_SET
                                                        (opncfg_qentry, 0x7);
            MMU_THDO_OPNCONFIG_QENTRYm_PORT_LIMIT_ENABLE_QENTRYf_SET
                                                        (opncfg_qentry, 0);
            MMU_THDO_OPNCONFIG_QENTRYm_PIDf_SET(opncfg_qentry, mport);
            ioerr += WRITE_MMU_THDO_OPNCONFIG_QENTRYm
                                        (unit, (op_node + idx), opncfg_qentry);
        }

        for (idx = 0; idx < q_limit; idx++) {
            ioerr += READ_MMU_THDO_QCONFIG_QENTRYm
                                        (unit, (q_offset + idx), &qcfg_qentry);
            if ((q_offset + idx) < total_egr_base_queues_in_device) {
                MMU_THDO_QCONFIG_QENTRYm_Q_MIN_QENTRYf_SET(qcfg_qentry, 0x9);
            } else {
                MMU_THDO_QCONFIG_QENTRYm_Q_MIN_QENTRYf_SET(qcfg_qentry, 0);
            }
            MMU_THDO_QCONFIG_QENTRYm_Q_SHARED_LIMIT_QENTRYf_SET
                                                        (qcfg_qentry, 0x2a15);
            MMU_THDO_QCONFIG_QENTRYm_Q_LIMIT_DYNAMIC_QENTRYf_SET(qcfg_qentry, 1);
            MMU_THDO_QCONFIG_QENTRYm_Q_LIMIT_ENABLE_QENTRYf_SET(qcfg_qentry, 1);
            MMU_THDO_QCONFIG_QENTRYm_Q_COLOR_ENABLE_QENTRYf_SET(qcfg_qentry, 0);
            MMU_THDO_QCONFIG_QENTRYm_Q_COLOR_LIMIT_DYNAMIC_QENTRYf_SET
                                                                (qcfg_qentry, 1);
            MMU_THDO_QCONFIG_QENTRYm_LIMIT_YELLOW_QENTRYf_SET(qcfg_qentry, 0);
            ioerr += WRITE_MMU_THDO_QCONFIG_QENTRYm
                                        (unit, (q_offset + idx), qcfg_qentry);

            ioerr += READ_MMU_THDO_QOFFSET_QENTRYm
                                        (unit, (q_offset + idx), &qoff_qentry);
            MMU_THDO_QOFFSET_QENTRYm_RESET_OFFSET_QENTRYf_SET(qoff_qentry, 1);
            MMU_THDO_QOFFSET_QENTRYm_LIMIT_RED_QENTRYf_SET(qoff_qentry, 0);
            MMU_THDO_QOFFSET_QENTRYm_RESET_OFFSET_YELLOW_QENTRYf_SET(qoff_qentry, 1);
            MMU_THDO_QOFFSET_QENTRYm_RESET_OFFSET_RED_QENTRYf_SET(qoff_qentry, 1);
            ioerr += WRITE_MMU_THDO_QOFFSET_QENTRYm
                                        (unit, (q_offset + idx), qoff_qentry);
        }
    }

    for (idx = 0; idx < NUM_RQEQ_COS; idx++) {
        ioerr += READ_OP_QUEUE_CONFIG1_THDORQEQr(unit, idx, &opq_rqeq1);
        OP_QUEUE_CONFIG1_THDORQEQr_Q_MINf_SET(opq_rqeq1, 0x9);
        OP_QUEUE_CONFIG1_THDORQEQr_Q_COLOR_ENABLEf_SET(opq_rqeq1, 0);
        OP_QUEUE_CONFIG1_THDORQEQr_Q_COLOR_DYNAMICf_SET(opq_rqeq1, 1);
        ioerr += WRITE_OP_QUEUE_CONFIG1_THDORQEQr(unit, idx, opq_rqeq1);

        ioerr += READ_OP_QUEUE_CONFIG_THDORQEQr(unit, idx, &opq_rqeq);
        OP_QUEUE_CONFIG_THDORQEQr_Q_SHARED_LIMITf_SET(opq_rqeq, 0x7);
        OP_QUEUE_CONFIG_THDORQEQr_Q_LIMIT_DYNAMICf_SET(opq_rqeq, 1);
        OP_QUEUE_CONFIG_THDORQEQr_Q_LIMIT_ENABLEf_SET(opq_rqeq, 1);
        ioerr += WRITE_OP_QUEUE_CONFIG_THDORQEQr(unit, idx, opq_rqeq);

        OP_QUEUE_RESET_OFFSET_THDORQEQr_SET(opq_rst_rqeq, 1);
        ioerr += WRITE_OP_QUEUE_RESET_OFFSET_THDORQEQr(unit, idx, opq_rst_rqeq);

        OP_QUEUE_LIMIT_YELLOW_THDORQEQr_SET(opq_yel_rqeq, 0);
        ioerr += WRITE_OP_QUEUE_LIMIT_YELLOW_THDORQEQr(unit, idx, opq_yel_rqeq);

        OP_QUEUE_LIMIT_RED_THDORQEQr_SET(opq_red_rqeq, 0);
        ioerr += WRITE_OP_QUEUE_LIMIT_RED_THDORQEQr(unit, idx, opq_red_rqeq);

        OP_QUEUE_RESET_OFFSET_YELLOW_THDORQEQr_SET(opq_rst_yel_rqeq, 1);
        ioerr += WRITE_OP_QUEUE_RESET_OFFSET_YELLOW_THDORQEQr
                                                    (unit, idx, opq_rst_yel_rqeq);

        OP_QUEUE_RESET_OFFSET_RED_THDORQEQr_SET(opq_rst_red_rqeq, 1);
        ioerr += WRITE_OP_QUEUE_RESET_OFFSET_RED_THDORQEQr
                                                    (unit, idx, opq_rst_red_rqeq);
    }

    /* 2.4.2 RQEI */
    for (idx = 0; idx < NUM_RQEI_COS; idx++) {
        ioerr += READ_OP_QUEUE_CONFIG1_THDORQEIr(unit, idx, &opq_rqei1);
        OP_QUEUE_CONFIG1_THDORQEIr_Q_MINf_SET(opq_rqei1, 0);
        OP_QUEUE_CONFIG1_THDORQEIr_Q_COLOR_ENABLEf_SET(opq_rqei1, 0);
        OP_QUEUE_CONFIG1_THDORQEIr_Q_COLOR_DYNAMICf_SET(opq_rqei1, 1);
        ioerr += WRITE_OP_QUEUE_CONFIG1_THDORQEIr(unit, idx, opq_rqei1);

        ioerr += READ_OP_QUEUE_CONFIG_THDORQEIr(unit, idx, &opq_rqei);
        OP_QUEUE_CONFIG_THDORQEIr_Q_SHARED_LIMITf_SET(opq_rqei, 0x7);
        OP_QUEUE_CONFIG_THDORQEIr_Q_LIMIT_DYNAMICf_SET(opq_rqei, 1);
        OP_QUEUE_CONFIG_THDORQEIr_Q_LIMIT_ENABLEf_SET(opq_rqei, 1);
        ioerr += WRITE_OP_QUEUE_CONFIG_THDORQEIr(unit, idx, opq_rqei);

        OP_QUEUE_RESET_OFFSET_THDORQEIr_SET(opq_rst_rqei, 2);
        ioerr += WRITE_OP_QUEUE_RESET_OFFSET_THDORQEIr(unit, idx, opq_rst_rqei);

        OP_QUEUE_LIMIT_YELLOW_THDORQEIr_SET(opq_yel_rqei, 0);
        ioerr += WRITE_OP_QUEUE_LIMIT_YELLOW_THDORQEIr(unit, idx, opq_yel_rqei);

        OP_QUEUE_LIMIT_RED_THDORQEIr_SET(opq_red_rqei, 0);
        ioerr += WRITE_OP_QUEUE_LIMIT_RED_THDORQEIr(unit, idx, opq_red_rqei);

        OP_QUEUE_RESET_OFFSET_YELLOW_THDORQEIr_SET(opq_rst_yel_rqei, 2);
        ioerr += WRITE_OP_QUEUE_RESET_OFFSET_YELLOW_THDORQEIr
                                                (unit, idx, opq_rst_yel_rqei);

        OP_QUEUE_RESET_OFFSET_RED_THDORQEIr_SET(opq_rst_red_rqei, 2);
        ioerr += WRITE_OP_QUEUE_RESET_OFFSET_RED_THDORQEIr
                                                (unit, idx, opq_rst_red_rqei);
    }

    for (idx = 0; idx < NUM_RDEQ_COS; idx++) {
        ioerr += READ_OP_QUEUE_CONFIG1_THDORDEQr(unit, idx, &opq_cfg1_rdeq);
        OP_QUEUE_CONFIG1_THDORDEQr_Q_MINf_SET(opq_cfg1_rdeq, 4);
        OP_QUEUE_CONFIG1_THDORDEQr_Q_COLOR_ENABLEf_SET(opq_cfg1_rdeq, 0);
        OP_QUEUE_CONFIG1_THDORDEQr_Q_COLOR_DYNAMICf_SET(opq_cfg1_rdeq, 1);
        ioerr += WRITE_OP_QUEUE_CONFIG1_THDORDEQr(unit, idx, opq_cfg1_rdeq);

        ioerr += READ_OP_QUEUE_CONFIG_THDORDEQr(unit, idx, &opq_cfg_rdeq);
        OP_QUEUE_CONFIG_THDORDEQr_Q_SHARED_LIMITf_SET(opq_cfg_rdeq, 0x7);
        OP_QUEUE_CONFIG_THDORDEQr_Q_LIMIT_DYNAMICf_SET(opq_cfg_rdeq, 1);
        OP_QUEUE_CONFIG_THDORDEQr_Q_LIMIT_ENABLEf_SET(opq_cfg_rdeq, 1);
        ioerr += WRITE_OP_QUEUE_CONFIG_THDORDEQr(unit, idx, opq_cfg_rdeq);

        OP_QUEUE_RESET_OFFSET_THDORDEQr_SET(opq_rst_rdeq, 1);
        ioerr += WRITE_OP_QUEUE_RESET_OFFSET_THDORDEQr(unit, idx, opq_rst_rdeq);

        OP_QUEUE_LIMIT_YELLOW_THDORDEQr_SET(opq_yel_rdeq, 0);
        ioerr += WRITE_OP_QUEUE_LIMIT_YELLOW_THDORDEQr(unit, idx, opq_yel_rdeq);

        OP_QUEUE_LIMIT_RED_THDORDEQr_SET(opq_red_rdeq, 0);
        ioerr += WRITE_OP_QUEUE_LIMIT_RED_THDORDEQr(unit, idx, opq_red_rdeq);

        OP_QUEUE_RESET_OFFSET_YELLOW_THDORDEQr_SET(opq_rst_yel_rdeq, 1);
        ioerr += WRITE_OP_QUEUE_RESET_OFFSET_YELLOW_THDORDEQr
                                                (unit, idx, opq_rst_yel_rdeq);

        OP_QUEUE_RESET_OFFSET_RED_THDORDEQr_SET(opq_rst_red_rdeq, 1);
        ioerr += WRITE_OP_QUEUE_RESET_OFFSET_RED_THDORDEQr
                                                (unit, idx, opq_rst_red_rdeq);
    }

    for (idx = 0; idx < NUM_RDEI_COS; idx++) {
        ioerr += READ_OP_QUEUE_CONFIG1_THDORDEIr(unit, idx, &opq_cfg1_rdei);
        OP_QUEUE_CONFIG1_THDORDEIr_Q_MINf_SET(opq_cfg1_rdei, 0x9);
        OP_QUEUE_CONFIG1_THDORDEIr_Q_COLOR_ENABLEf_SET(opq_cfg1_rdei, 0);
        OP_QUEUE_CONFIG1_THDORDEIr_Q_COLOR_DYNAMICf_SET(opq_cfg1_rdei, 1);
        ioerr += WRITE_OP_QUEUE_CONFIG1_THDORDEIr(unit, idx, opq_cfg1_rdei);

        ioerr += READ_OP_QUEUE_CONFIG_THDORDEIr(unit, idx, &opq_cfg_rdei);
        OP_QUEUE_CONFIG_THDORDEIr_Q_SHARED_LIMITf_SET(opq_cfg_rdei, 0x7);
        OP_QUEUE_CONFIG_THDORDEIr_Q_LIMIT_DYNAMICf_SET(opq_cfg_rdei, 1);
        OP_QUEUE_CONFIG_THDORDEIr_Q_LIMIT_ENABLEf_SET(opq_cfg_rdei, 1);
        ioerr += WRITE_OP_QUEUE_CONFIG_THDORDEIr(unit, idx, opq_cfg_rdei);

        OP_QUEUE_RESET_OFFSET_THDORDEIr_SET(opq_rst_rdei, 2);
        ioerr += WRITE_OP_QUEUE_RESET_OFFSET_THDORDEIr(unit, idx, opq_rst_rdei);

        OP_QUEUE_LIMIT_YELLOW_THDORDEIr_SET(opq_yel_rdei, 0);
        ioerr += WRITE_OP_QUEUE_LIMIT_YELLOW_THDORDEIr(unit, idx, opq_yel_rdei);

        OP_QUEUE_LIMIT_RED_THDORDEIr_SET(opq_red_rdei, 0);
        ioerr += WRITE_OP_QUEUE_LIMIT_RED_THDORDEIr(unit, idx, opq_red_rdei);

        OP_QUEUE_RESET_OFFSET_YELLOW_THDORDEIr_SET(opq_rst_yel_rdei, 2);
        ioerr += WRITE_OP_QUEUE_RESET_OFFSET_YELLOW_THDORDEIr
                                                    (unit, idx, opq_rst_yel_rdei);

        OP_QUEUE_RESET_OFFSET_RED_THDORDEIr_SET(opq_rst_red_rdei, 2);
        ioerr += WRITE_OP_QUEUE_RESET_OFFSET_RED_THDORDEIr
                                                    (unit, idx, opq_rst_red_rdei);
    }

    /* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
    /* OpNode related config for remaining OpNodes                 */
    /* (assuming internal memory settings)                         */
    /* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
    for (op_node = MMU_THDO_OPNCONFIG_CELLm_MIN;
                        op_node <= MMU_THDO_OPNCONFIG_CELLm_MAX; op_node++) {
        ioerr += READ_MMU_THDO_OPNCONFIG_CELLm(unit, op_node, &opncfg_cell);
        MMU_THDO_OPNCONFIG_CELLm_OPN_SHARED_LIMIT_CELLf_SET
                                                        (opncfg_cell, 0x242c);
        MMU_THDO_OPNCONFIG_CELLm_OPN_SHARED_RESET_VALUE_CELLf_SET
                                                        (opncfg_cell, 0x241a);
        MMU_THDO_OPNCONFIG_CELLm_PORT_LIMIT_ENABLE_CELLf_SET(opncfg_cell, 0);
        ioerr += WRITE_MMU_THDO_OPNCONFIG_CELLm(unit, op_node, opncfg_cell);
    }

    for (op_node = MMU_THDO_OPNCONFIG_QENTRYm_MIN;
                        op_node <= MMU_THDO_OPNCONFIG_QENTRYm_MAX; op_node++) {
        ioerr += READ_MMU_THDO_OPNCONFIG_QENTRYm(unit, op_node, &opncfg_qentry);
        MMU_THDO_OPNCONFIG_QENTRYm_OPN_SHARED_LIMIT_QENTRYf_SET
                                                        (opncfg_qentry, 0x2a17);
        MMU_THDO_OPNCONFIG_QENTRYm_OPN_SHARED_RESET_VALUE_QENTRYf_SET
                                                    (opncfg_qentry, 0x2a17 - 2);
        MMU_THDO_OPNCONFIG_QENTRYm_PORT_LIMIT_ENABLE_QENTRYf_SET(opncfg_qentry, 0);
        ioerr += WRITE_MMU_THDO_OPNCONFIG_QENTRYm(unit, op_node, opncfg_qentry);
    }

/* end of init_helper */
}

static void
_post_mmu_init(int unit)
{
    int subidx, port;
    TXLP_HW_RESET_CONTROL_1r_t txlp_hw_rset_ctrl_1;
    TXLP_PORT_ADDR_MAP_TABLEm_t txlp_port_map;
    int start_addr, end_addr;
    int speed_max;

    READ_TXLP_HW_RESET_CONTROL_1r(unit, &txlp_hw_rset_ctrl_1);
    TXLP_HW_RESET_CONTROL_1r_VALIDf_SET(txlp_hw_rset_ctrl_1, 1);
    TXLP_HW_RESET_CONTROL_1r_COUNTf_SET(txlp_hw_rset_ctrl_1, 256);
    TXLP_HW_RESET_CONTROL_1r_RESET_ALLf_SET(txlp_hw_rset_ctrl_1, 0x1);
    WRITE_TXLP_HW_RESET_CONTROL_1r(unit, txlp_hw_rset_ctrl_1);

    /* TXLP_PORT_ADDR_MAP_TABLE */
    for (subidx = 0, start_addr = 0, port = 1; subidx < PORTS_PER_BLOCK; subidx++, port++) {
        speed_max = bcm56270_a0_port_speed_max(unit, port);
        if (speed_max <= 2500) {
            end_addr = start_addr + (( 8 * 4) - 1); /* 8 cells */
        } else if (speed_max <= 13000) {
            end_addr = start_addr + ((16 * 4) - 1); /* 16 Cells */
        }

        TXLP_PORT_ADDR_MAP_TABLEm_CLR(txlp_port_map);
        TXLP_PORT_ADDR_MAP_TABLEm_START_ADDRf_SET(txlp_port_map, start_addr);
        TXLP_PORT_ADDR_MAP_TABLEm_END_ADDRf_SET(txlp_port_map, end_addr);
        WRITE_TXLP_PORT_ADDR_MAP_TABLEm(unit, subidx, txlp_port_map);
        start_addr = end_addr + 1;
    }
    /* end of post_mmu_init */

}

static int
_mmu_init(int unit)
{
    int rv = 0;
    int ioerr = 0;

    int idx = 0;

    INPUT_PORT_RX_ENABLE_64r_t port_rx_enable;
    THDIRQE_INPUT_PORT_RX_ENABLE_64r_t irqe_rx_enable;
    THDIQEN_INPUT_PORT_RX_ENABLE_64r_t iqen_rx_enable;
    THDI_BYPASSr_t thdi_bpass;
    THDIQEN_THDI_BYPASSr_t iqen_bpass;
    THDIRQE_THDI_BYPASSr_t irqe_bpass;
    THDO_BYPASSr_t thdo_bpass;
    int nxtaddr = 0;
    RQE_SCHEDULER_WEIGHT_L0_QUEUEr_t rqe_schdw_l0;
    RQE_SCHEDULER_WEIGHT_L1_QUEUEr_t rqe_schdw_l1;
    RQE_SCHEDULER_CONFIGr_t rqe_schd_cfg;
    LLS_CONFIG0r_t lls_config;
    LLS_MAX_REFRESH_ENABLEr_t lls_max_refresh;
    LLS_MIN_REFRESH_ENABLEr_t lls_min_refresh;
    MMU_ENQ_IP_PRI_TO_PG_PROFILE_0r_t pri_to_pg_profile;
    PORT_PROFILE_MAPr_t port_pro_map;
    THDIRQE_PORT_PROFILE_MAPr_t thdirqe_port_pro_map;
    THDIQEN_PORT_PROFILE_MAPr_t thdiqen_port_pro_map;
    BUFFER_CELL_LIMIT_SP_SHAREDr_t sp_shared;
    THDIRQE_BUFFER_CELL_LIMIT_SP_SHAREDr_t irqe_sp_shared;
    THDIQEN_BUFFER_CELL_LIMIT_SP_SHAREDr_t iqen_sp_shared;
    THDO_MISCCONFIGr_t thd0_misc;
    MISCCONFIGr_t misc;
    OP_THR_CONFIGr_t op_thr;
    MMU_AGING_LMT_INTm_t age_int;
    MMU_ENQ_PROFILE_0_PRI_GRPr_t mmu_profile_0_pri_gpr;
    PROFILE0_PRI_GRPr_t profile_0_pri_gpr;
    THDIRQE_PROFILE0_PRI_GRPr_t thdirqe_profile0_pri_gpr;
    THDIQEN_PROFILE0_PRI_GRPr_t thdiqen_profile0_pri_gpr;
    cdk_pbmp_t pbmp_all, mxqport_pbmp, xlport_pbmp;
    LLS_SOFT_RESETr_t soft_reset;
    LLS_INITr_t lls_init;


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
        CDK_WARN(("bcm56270_a0_bmd_init[%d]: LLS INIT timeout\n", unit));
        return ioerr ? CDK_E_IO : CDK_E_TIMEOUT;
    }

    /* Setup TDM for MMU Arb & LLS */
    rv = _mmu_tdm_init(unit);

    /* Enable all ports */
    INPUT_PORT_RX_ENABLE_64r_SET(port_rx_enable, 0x3FFFFFFF);
    ioerr += WRITE_INPUT_PORT_RX_ENABLE_64r(unit, port_rx_enable);

    THDIQEN_INPUT_PORT_RX_ENABLE_64r_SET(iqen_rx_enable, 0x3FFFFFFF);
    ioerr += WRITE_THDIQEN_INPUT_PORT_RX_ENABLE_64r(unit, iqen_rx_enable);
    THDIRQE_INPUT_PORT_RX_ENABLE_64r_SET(irqe_rx_enable, 0x3FFFFFFF);
    ioerr += WRITE_THDIRQE_INPUT_PORT_RX_ENABLE_64r(unit, irqe_rx_enable);

    THDI_BYPASSr_CLR(thdi_bpass);
    THDIRQE_THDI_BYPASSr_CLR(irqe_bpass);
    THDIQEN_THDI_BYPASSr_CLR(iqen_bpass);
    THDO_BYPASSr_CLR(thdo_bpass);
    ioerr += WRITE_THDI_BYPASSr(unit, thdi_bpass);
    ioerr += WRITE_THDIQEN_THDI_BYPASSr(unit, iqen_bpass);
    ioerr += WRITE_THDIRQE_THDI_BYPASSr(unit, irqe_bpass);
    ioerr += WRITE_THDO_BYPASSr(unit, thdo_bpass);

    nxtaddr += 2;
    bcm56270_a0_mxqport_pbmp_get(unit, &mxqport_pbmp);
    bcm56270_a0_xlport_pbmp_get(unit, &xlport_pbmp);
    pbmp_all = mxqport_pbmp;
    CDK_PBMP_OR(pbmp_all, xlport_pbmp);

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

    READ_MISCCONFIGr(unit, &misc);
    MISCCONFIGr_REFRESH_ENf_SET(misc, 1);
    WRITE_MISCCONFIGr(unit, misc);

    /* Configuring all profile 0 to use all the priority groups 0 to 7 */
    MMU_ENQ_PROFILE_0_PRI_GRPr_SET(mmu_profile_0_pri_gpr, 0xffffff);
    ioerr += WRITE_MMU_ENQ_PROFILE_0_PRI_GRPr(unit, 0, mmu_profile_0_pri_gpr);
    ioerr += WRITE_MMU_ENQ_PROFILE_0_PRI_GRPr(unit, 1, mmu_profile_0_pri_gpr);

    PROFILE0_PRI_GRPr_SET(profile_0_pri_gpr, 0xffffff);
    ioerr += WRITE_PROFILE0_PRI_GRPr(unit, 0, profile_0_pri_gpr);
    ioerr += WRITE_PROFILE0_PRI_GRPr(unit, 1, profile_0_pri_gpr);

    THDIRQE_PROFILE0_PRI_GRPr_SET(thdirqe_profile0_pri_gpr, 0xffffff);
    ioerr += WRITE_THDIRQE_PROFILE0_PRI_GRPr(unit, 0, thdirqe_profile0_pri_gpr);
    ioerr += WRITE_THDIRQE_PROFILE0_PRI_GRPr(unit, 1, thdirqe_profile0_pri_gpr);

    THDIQEN_PROFILE0_PRI_GRPr_SET(thdiqen_profile0_pri_gpr, 0xffffff);
    ioerr += WRITE_THDIQEN_PROFILE0_PRI_GRPr(unit, 0, thdiqen_profile0_pri_gpr);
    ioerr += WRITE_THDIQEN_PROFILE0_PRI_GRPr(unit, 1, thdiqen_profile0_pri_gpr);

    MMU_ENQ_IP_PRI_TO_PG_PROFILE_0r_CLR(pri_to_pg_profile);
    ioerr += WRITE_MMU_ENQ_IP_PRI_TO_PG_PROFILE_0r(unit, pri_to_pg_profile);

    PORT_PROFILE_MAPr_CLR(port_pro_map);
    WRITE_PORT_PROFILE_MAPr(unit, port_pro_map);
    THDIRQE_PORT_PROFILE_MAPr_CLR(thdirqe_port_pro_map);
    WRITE_THDIRQE_PORT_PROFILE_MAPr(unit, thdirqe_port_pro_map);
    THDIQEN_PORT_PROFILE_MAPr_CLR(thdiqen_port_pro_map);
    WRITE_THDIQEN_PORT_PROFILE_MAPr(unit, thdiqen_port_pro_map);

    /* Input port shared space */
    BUFFER_CELL_LIMIT_SP_SHAREDr_CLR(sp_shared);
    THDIRQE_BUFFER_CELL_LIMIT_SP_SHAREDr_CLR(irqe_sp_shared);
    THDIQEN_BUFFER_CELL_LIMIT_SP_SHAREDr_CLR(iqen_sp_shared);
    ioerr += WRITE_BUFFER_CELL_LIMIT_SP_SHAREDr(unit, sp_shared);
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

    _mmu_lls_init(unit);

    _mmu_buffer_pools_init(unit);

    /* Initialize MMU internal/external aging limit memory */
    MMU_AGING_LMT_INTm_CLR(age_int);
    for (idx = 0; idx < MMU_AGING_LMT_INTm_MAX; idx++) {
        ioerr += WRITE_MMU_AGING_LMT_INTm(unit, idx, age_int);
    }

    _post_mmu_init(unit);

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
int
bcm56270_a0_soc_port_enable_set(int unit, int port, int enable)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int loop;
    uint32_t port_enable;
    cdk_pbmp_t xlport_pbmp, mxq_pbmp;
    TXLP_PORT_ENABLEr_t txlp_enable;
    TXLP_MIN_STARTCNTr_t txlp_min_startcnt;
    IECELL_CONFIGr_t iecell_cfg;
    XPORT_PORT_ENABLEr_t xport_enable;
    XLPORT_ENABLE_REGr_t xlport_enable;
    uint32_t enable_reg_value;
    EGR_ENABLEm_t egr_enable;

    loop = (port-1) % 4;

    bcm56270_a0_mxqport_pbmp_get(unit, &mxq_pbmp);
    bcm56270_a0_xlport_pbmp_get(unit, &xlport_pbmp);

    ioerr += READ_TXLP_PORT_ENABLEr(unit,  &txlp_enable);
    port_enable = TXLP_PORT_ENABLEr_PORT_ENABLEf_GET(txlp_enable);
    TXLP_PORT_ENABLEr_PORT_ENABLEf_SET(txlp_enable, (port_enable | (1 << loop)));
    ioerr += WRITE_TXLP_PORT_ENABLEr(unit, txlp_enable);

    ioerr += READ_TXLP_MIN_STARTCNTr(unit, &txlp_min_startcnt);
    TXLP_MIN_STARTCNTr_NLP_STARTCNTf_SET(txlp_min_startcnt, 3);
    ioerr += WRITE_TXLP_MIN_STARTCNTr(unit, txlp_min_startcnt);

    if (CDK_PBMP_MEMBER(xlport_pbmp, port)) {
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
        ioerr += READ_IECELL_CONFIGr(unit, port - 4, &iecell_cfg);
        if (enable) {
            IECELL_CONFIGr_SOFT_RESETf_SET(iecell_cfg, 0);
        } else {
            IECELL_CONFIGr_SOFT_RESETf_SET(iecell_cfg, 1);
        }
        ioerr += WRITE_IECELL_CONFIGr(unit, port - 4, iecell_cfg);
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
bcm56270_a0_mac_xl_drain_cells(int unit, int port)
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
    ioerr += bcm56270_a0_soc_port_enable_set(unit, port, 0);
    ioerr += bcm56270_a0_soc_port_enable_set(unit, port, 1);

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
        CDK_WARN(("bcm56270_a0_bmd_port_mode_set[%d]: "
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
bcm56270_a0_xlport_init(int unit, int port)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    XLMAC_CTRLr_t xlmac_ctrl;
    XLMAC_RX_CTRLr_t xlmac_rx_ctrl;
    XLMAC_TX_CTRLr_t xlmac_tx_ctrl;
    EGR_MTUr_t egr_mtu;
    XLMAC_PAUSE_CTRLr_t xlmac_pause_ctrl;
    XLMAC_RX_MAX_SIZEr_t xlmac_rx_max_size;
    XLMAC_MODEr_t xlmac_mode;
    XLMAC_RX_LSS_CTRLr_t xlmac_rx_lss_ctrl;
    XLMAC_PFC_CTRLr_t xlmac_pfc_ctrl;
    int speed;
    cdk_pbmp_t mxqport_pbmp;
    X_GPORT_SGNDET_EARLYCRSr_t sgn_crs;

    ioerr += _port_init(unit, port);

    speed = bcm56270_a0_port_speed_max(unit, port);
    /* MAC init */
    /* Disable Tx/Rx, assume that MAC is stable (or out of reset) */
    ioerr += READ_XLMAC_CTRLr(unit, port, &xlmac_ctrl);

    /* Reset EP credit before de-assert SOFT_RESET */
    if (XLMAC_CTRLr_SOFT_RESETf_GET(xlmac_ctrl)) {
        /* credit_reset */
        /* Bring the port down and back up to clear credits */
        ioerr += bcm56270_a0_soc_port_enable_set(unit, port, 0);
        ioerr += bcm56270_a0_soc_port_enable_set(unit, port, 1);
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

    ioerr += READ_XLMAC_RX_CTRLr(unit, port, &xlmac_rx_ctrl);
    XLMAC_RX_CTRLr_STRIP_CRCf_SET(xlmac_rx_ctrl, 0);
    XLMAC_RX_CTRLr_STRICT_PREAMBLEf_SET(xlmac_rx_ctrl, 0);
    if (speed >= 10000) {
        XLMAC_RX_CTRLr_STRICT_PREAMBLEf_SET(xlmac_rx_ctrl, 1);
    }
    ioerr += WRITE_XLMAC_RX_CTRLr(unit, port, xlmac_rx_ctrl);

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

    ioerr += READ_EGR_MTUr(unit, port, &egr_mtu);
    EGR_MTUr_MTU_SIZEf_SET(egr_mtu, JUMBO_MAXSZ);
    EGR_MTUr_MTU_ENABLEf_SET(egr_mtu, 1);
    ioerr += WRITE_EGR_MTUr(unit, port, egr_mtu);

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

    ioerr += READ_XLMAC_TX_CTRLr(unit, port, &xlmac_tx_ctrl);
    if (BMD_PORT_PROPERTIES(unit, port) & BMD_PORT_HG) {
        XLMAC_TX_CTRLr_AVERAGE_IPGf_SET(xlmac_tx_ctrl, 0xc);
    } else {
        XLMAC_TX_CTRLr_AVERAGE_IPGf_SET(xlmac_tx_ctrl, 0x9);
    }
    XLMAC_TX_CTRLr_CRC_MODEf_SET(xlmac_tx_ctrl, 3);
    ioerr += WRITE_XLMAC_TX_CTRLr(unit, port, xlmac_tx_ctrl);

    ioerr += READ_XLMAC_RX_LSS_CTRLr(unit, port, &xlmac_rx_lss_ctrl);
    XLMAC_RX_LSS_CTRLr_DROP_TX_DATA_ON_LOCAL_FAULTf_SET(xlmac_rx_lss_ctrl, 1);
    XLMAC_RX_LSS_CTRLr_DROP_TX_DATA_ON_REMOTE_FAULTf_SET(xlmac_rx_lss_ctrl, 1);
    XLMAC_RX_LSS_CTRLr_DROP_TX_DATA_ON_LINK_INTERRUPTf_SET(xlmac_rx_lss_ctrl, 1);
    ioerr += WRITE_XLMAC_RX_LSS_CTRLr(unit, port, xlmac_rx_lss_ctrl);

    ioerr += READ_XLMAC_CTRLr(unit, port, &xlmac_ctrl);
    XLMAC_CTRLr_EXTENDED_HIG2_ENf_SET(xlmac_ctrl, 1);
    XLMAC_CTRLr_SW_LINK_STATUSf_SET(xlmac_ctrl, 1);
    WRITE_XLMAC_CTRLr(unit, port, xlmac_ctrl);

    bcm56270_a0_mxqport_pbmp_get(unit, &mxqport_pbmp);
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
    WRED_MISCCONFIGr_t wred_misccfg;
    int idx;
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
    MISCCONFIGr_t misc;
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

    bcm56270_a0_mxqport_pbmp_get(unit, &mxqport_pbmp);
    bcm56270_a0_xlport_pbmp_get(unit, &xlport_pbmp);

    CDK_PBMP_ITER(pbmp_all, port) {
        if (CDK_PBMP_MEMBER(mxqport_pbmp, port) && ((port - 1) % 4) == 0) {
            /* Bring XMAC out of reset */
            READ_XPORT_MODE_REGr(unit, &xport_mode , port);
            XPORT_MODE_REGr_PORT_GMII_MII_ENABLEf_SET(xport_mode, 0x1);
            XPORT_MODE_REGr_PHY_PORT_MODEf_SET(xport_mode, 0x2);
            WRITE_XPORT_MODE_REGr(unit, xport_mode, port);
        }
    }

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

    ioerr += READ_WRED_MISCCONFIGr(unit, &wred_misccfg);
    if (CDK_XGSD_FLAGS(unit) & CHIP_FLAG_FREQ180) {
        WRED_MISCCONFIGr_BASE_UPDATE_INTERVALf_SET(wred_misccfg, 3);
    } else if (CDK_XGSD_FLAGS(unit) & CHIP_FLAG_FREQ125) {
        WRED_MISCCONFIGr_BASE_UPDATE_INTERVALf_SET(wred_misccfg, 4);
    } else if (CDK_XGSD_FLAGS(unit) & CHIP_FLAG_FREQ30) {
        WRED_MISCCONFIGr_BASE_UPDATE_INTERVALf_SET(wred_misccfg, 12);
    }
    ioerr += WRITE_WRED_MISCCONFIGr(unit, wred_misccfg);

    ioerr += CDK_XGSD_MEM_CLEAR(unit, RXLP_INTERNAL_STREAM_MAP_PORT_0m);
    ioerr += CDK_XGSD_MEM_CLEAR(unit, RXLP_INTERNAL_STREAM_MAP_PORT_1m);
    ioerr += CDK_XGSD_MEM_CLEAR(unit, RXLP_INTERNAL_STREAM_MAP_PORT_2m);
    ioerr += CDK_XGSD_MEM_CLEAR(unit, RXLP_INTERNAL_STREAM_MAP_PORT_3m);
    ioerr += CDK_XGSD_MEM_CLEAR(unit, TXLP_PORT_STREAM_BITMAP_TABLEm);
    ioerr += CDK_XGSD_MEM_CLEAR(unit, TXLP_INT2EXT_STREAM_MAP_TABLEm);
    ioerr += CDK_XGSD_MEM_CLEAR(unit, DEVICE_STREAM_ID_TO_PP_PORT_MAPm);
    ioerr += CDK_XGSD_MEM_CLEAR(unit, PP_PORT_TO_PHYSICAL_PORT_MAPm);
    /* end of saber2 related hw reset control */

    /* Some registers are implemented in memory, need to clear them in order
     * to have correct parity value */
    EGR_VLAN_CONTROL_1r_CLR(egr_vlan_control_1);
    EGR_IPMC_CFG2r_CLR(ipmc_cfg);
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

    /* Enable Field Processor metering clock */
    ioerr += READ_MISCCONFIGr(unit, &misc);
    MISCCONFIGr_METERING_CLK_ENf_SET(misc, 1);
    ioerr += WRITE_MISCCONFIGr(unit, misc);

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
        speed_max = bcm56270_a0_port_speed_max(unit, port);
        if (CDK_PBMP_MEMBER(mxqport_pbmp, port)) {
            ioerr += bcm56270_a0_mxq_phy_mode_get(unit, port, speed_max, &phy_mode, &num_lanes);

            ioerr += READ_XPORT_MODE_REGr(unit, &xport_mode, port);
            XPORT_MODE_REGr_PHY_PORT_MODEf_SET(xport_mode, phy_mode);
            XPORT_MODE_REGr_PORT_GMII_MII_ENABLEf_SET(xport_mode, ((speed_max < 10000) ? 1 : 0));
            ioerr += WRITE_XPORT_MODE_REGr(unit, xport_mode, port);

            XPORT_MIB_RESETr_SET(mib_reset, 0XF);
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
        }
        if (CDK_PBMP_MEMBER(xlport_pbmp, port)) {
            bcm56270_a0_xl_phy_core_port_mode(unit, port, &phy_mode_xl, &core_mode_xl);

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
            XLPORT_CNTMAXSIZEr_CNTMAXSIZEf_SET(xlport_cntmaxsize, 0x5F2);
            ioerr += WRITE_XLPORT_CNTMAXSIZEr(unit, port, xlport_cntmaxsize);
        }

        if (CDK_PBMP_MEMBER(mxqport_pbmp, port)) {
            X_GPORT_CNTMAXSIZEr_CLR(x_gport_cntmaxsize);
            X_GPORT_CNTMAXSIZEr_CNTMAXSIZEf_SET(x_gport_cntmaxsize, 0x5F2);
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
bcm56270_a0_bmd_init(int unit)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    cdk_pbmp_t mxqport_pbmp, xlport_pbmp, pbmp_all;
    int port;
    XLPORT_MAC_RSV_MASKr_t xlmac_mac_rsv_mask;
    MAC_RSV_MASKr_t mac_rsv_mask;
    RDBGC0_SELECTr_t rdbgc0_select;
    TDBGC_SELECTr_t tdbgc_select;
    VLAN_PROFILE_TABm_t vlan_profile;
    ING_VLAN_TAG_ACTION_PROFILEm_t vlan_action;
    EGR_VLAN_TAG_ACTION_PROFILEm_t egr_action;
    EGR_BYPASS_CTRLr_t egr_bypass_ctrl;
#if BMD_CONFIG_INCLUDE_PHY == 1
    phy_ctrl_t *pc;
    uint32_t ability= 0x0;
    int lanes;
    int speed_max;
#endif 

    BMD_CHECK_UNIT(unit);

    bcm56270_a0_mxqport_pbmp_get(unit, &mxqport_pbmp);
    bcm56270_a0_xlport_pbmp_get(unit, &xlport_pbmp);
    pbmp_all = mxqport_pbmp;
    CDK_PBMP_OR(pbmp_all, xlport_pbmp);

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

#if BMD_CONFIG_INCLUDE_PHY == 1
    CDK_PBMP_ITER(mxqport_pbmp, port) {
        speed_max = bcm56270_a0_port_speed_max(unit, port);
        
        /* PHY init */
        if (CDK_SUCCESS(rv)) {
            rv = bmd_phy_probe(unit, port);
        }
        lanes = bcm56270_a0_port_num_lanes(unit, port);
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
    } /* end of MXQPORT PHY init */
#endif /* BMD_CONFIG_INCLUDE_PHY == 1 */

#if BMD_CONFIG_INCLUDE_PHY == 1
    CDK_PBMP_ITER(xlport_pbmp, port) {
        speed_max = bcm56270_a0_port_speed_max(unit, port);

        if (CDK_SUCCESS(rv)) {
            rv = bmd_phy_probe(unit, port);
        }

        lanes = bcm56270_a0_port_num_lanes(unit, port);
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
    } /* end of XLPORT PHY init */
#endif /* BMD_CONFIG_INCLUDE_PHY == 1 */

    CDK_PBMP_ITER(pbmp_all, port) {
        ioerr += bcm56270_a0_xlport_init(unit, port);
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
        CMIC_CMC_COS_CTRL_RX_0r_t cos_ctrl_0;
        CMIC_CMC_COS_CTRL_RX_1r_t cos_ctrl_1;
        uint32_t cos_bmp;

        CMIC_CMC_COS_CTRL_RX_0r_CLR(cos_ctrl_0);
        for (idx = 0; idx < 4; idx++) {
            cos_bmp = (idx == XGSD_DMA_RX_CHAN) ? 0xffffffff : 0;
            CMIC_CMC_COS_CTRL_RX_0r_COS_BMPf_SET(cos_ctrl_0, cos_bmp);
            ioerr += WRITE_CMIC_CMC_COS_CTRL_RX_0r(unit, idx, cos_ctrl_0);
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
#endif /* CDK_CONFIG_INCLUDE_BCM56270_A0 */
