#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56340_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>
#include <bmd/bmd_device.h>
#include <bmd/bmd_dma.h>

#include <bmdi/arch/xgsm_dma.h>

#include <cdk/chip/bcm56340_a0_defs.h>
#include <cdk/arch/xgsm_chip.h>
#include <cdk/cdk_util.h>
#include <cdk/cdk_debug.h>

#include "bcm56340_a0_bmd.h"
#include "bcm56340_a0_internal.h"

#define PIPE_RESET_TIMEOUT_MSEC         50
#define LLS_RESET_TIMEOUT_MSEC          50

#define PASSTHRU_MPORT                  58

#define JUMBO_MAXSZ                     0x3fe8

#define MMU_CELLS_RSVD_IP               100

#define MMU_MAX_PKT_BYTES               (10 * 1024L) /* bytes */
#define MMU_PKT_HDR_BYTES               64    /* bytes */
#define MMU_JUMBO_FRAME_BYTES           9216  /* bytes */
#define MMU_DEFAULT_MTU_BYTES           1536  /* bytes */

#define MMU_TOTAL_CELLS_24K             (24 * 1024L) /* 24k cells */
#define MMU_TOTAL_CELLS_19K             (19 * 1024L) /* 19k cells */
#define MMU_TOTAL_CELLS_10K                     (10 * 1024L) /* 10k cells */
#define MMU_IN_PG_HDRM_CELLS            162
#define MMU_OUT_PORT_MIN_CELLS          0

#define MMU_CELL_BYTES                  208
#define MMU_NUM_PG                      8
#define MMU_NUM_POOL                    4

#define MMU_PG_PER_PORT                 1
#define MMU_DEFAULT_PG                  (MMU_NUM_PG - 1)

#define ISM_MAX_STAGES                  4
#define ISM_MAX_RAW_BANKS               8
#define ISM_BANKS_PER_STAGE_96          4
#define ISM_BANKS_PER_STAGE_80          4
#define ISM_MAX_BANKS_96                16
#define ISM_MAX_BANKS_80                16

#define ISM_BANK_SIZE_HALF              1
#define ISM_BANK_SIZE_QUARTER           2
#define ISM_BANK_SIZE_DISABLED          3

#define FW_ALIGN_BYTES                  16
#define FW_ALIGN_MASK                   (FW_ALIGN_BYTES - 1)

#define CMIC_NUM_PKT_DMA_CHAN           4

#define ETH_HDR_LEN                     18
#define ETH_CRC_LEN                     4

#if BMD_CONFIG_INCLUDE_DMA == 1
static bmd_pkt_t test_pkt;
#endif /* BMD_CONFIG_INCLUDE_DMA */

typedef struct {
    int count;
    uint8_t index[ISM_MAX_RAW_BANKS];
} _ism_real_bank_map_t;

typedef struct {
    int num_banks;
    const uint8_t *bank_info;
} _ism_tbl_cfg_t;

#define _TBL_CFG(_cfg) { COUNTOF(_cfg), _cfg }

/* Bank info combines 4-bit search stage and 4-bit search bank */
#define ISM_BANK_INFO_STAGE_NO(_bank_info) ((_bank_info) >> 4)
#define ISM_BANK_INFO_BANK_NO(_bank_info) ((_bank_info) & 0xf)
#define ISM_BANK_INFO(_s, _b)   ((_s)<<4 | ((_b)&0xf))

/* ISM table configuration for 96K mode */
/* Bank info for each table, e.g. 0x12 means stage 1 bank 2 */
static const uint8_t _vlan_xlate_96[] = {0x20, 0x22};
static const uint8_t _l2_entry_96[] = {0x00, 0x02, 0x10, 0x12};
static const uint8_t _l3_entry_96[] = {0x01, 0x11, 0x30};
static const uint8_t _ep_vlan_xlate_96[] = {0x03};
static const uint8_t _mpls_96[] = {0x21, 0x32};
static const _ism_tbl_cfg_t _ism_tbl_cfg_96[] = {
    _TBL_CFG(_vlan_xlate_96),
    _TBL_CFG(_l2_entry_96),
    _TBL_CFG(_l3_entry_96),
    _TBL_CFG(_ep_vlan_xlate_96),
    _TBL_CFG(_mpls_96)
};

/* ISM table configuration for 80K mode */
/* Bank info for each table, e.g. 0x12 means stage 1 bank 2 */
static const uint8_t _vlan_xlate_80[] = {0x11, 0x20};
static const uint8_t _l2_entry_80[] = {0x00, 0x01, 0x10};
static const uint8_t _l3_entry_80[] = {0x02, 0x21, 0x30, 0x31};
static const uint8_t _ep_vlan_xlate_80[] = {0x03};
static const uint8_t _mpls_80[] = {0x12, 0x22, 0x32};
static const _ism_tbl_cfg_t _ism_tbl_cfg_80[] = {
    _TBL_CFG(_vlan_xlate_80),
    _TBL_CFG(_l2_entry_80),
    _TBL_CFG(_l3_entry_80),
    _TBL_CFG(_ep_vlan_xlate_80),
    _TBL_CFG(_mpls_80)
};

static const _ism_real_bank_map_t _ism_real_bank_map_96[] = {
        {2, {8, 9}},
        {2, {12, 13}},
        {1, {14}},
        {1, {15}},
        {2, {24, 25}},
        {2, {28, 29}},
        {1, {30}},
        {1, {31}},
        {2, {40, 41}},
        {2, {44, 45}},
        {1, {46}},
        {1, {47}},
        {2, {56, 57}},
        {2, {60, 61}},
        {1, {62}},
        {1, {63}}
};

static const _ism_real_bank_map_t _ism_real_bank_map_80[] = {
        {2, {8, 9}},
        {1, {12}},
        {1, {14}},
        {1, {15}},
        {2, {24, 25}},
        {1, {28}},
        {1, {30}},
        {1, {31}},
        {2, {40, 41}},
        {1, {44}},
        {1, {46}},
        {1, {47}},
        {2, {56, 57}},
        {1, {60}},
        {1, {62}},
        {1, {63}}
};


static int
_port_map_init(int unit)
{
    int ioerr = 0;
    int port, lport, mport;
    int num_pport = NUM_PHYS_PORTS;
    int num_lport = NUM_LOGIC_PORTS;
    int num_mport = NUM_MMU_PORTS;
    cdk_pbmp_t pbmp;
    ING_PHYS_TO_LOGIC_MAPm_t ing_p2l;
    EGR_LOGIC_TO_PHYS_MAPr_t egr_l2p;
    MMU_TO_PHYS_MAPr_t mmu_m2p;
    MMU_TO_LOGIC_MAPr_t mmu_m2l;

    bcm56340_a0_xport_pbmp_get(unit, XPORT_FLAG_ALL, &pbmp);
    CDK_PBMP_PORT_ADD(pbmp, CMIC_PORT);

    /* Ingress physical to logical port mapping */
    ING_PHYS_TO_LOGIC_MAPm_CLR(ing_p2l);
    for (port = 0; port < num_pport; port++) {
        lport = P2L(unit, port);
        if (lport < 0) {
            lport = 0x7f;
        }
        ING_PHYS_TO_LOGIC_MAPm_LOGIC_PORTf_SET(ing_p2l, lport);
        ioerr += WRITE_ING_PHYS_TO_LOGIC_MAPm(unit, port, ing_p2l);
    }

    /* Egress logical to physical port mapping */
    for (lport = 0; lport < num_lport; lport++) {
        port = L2P(unit, lport);
        if (port < 0) {
            port = 0x7f;
        }
        EGR_LOGIC_TO_PHYS_MAPr_PHYS_PORTf_SET(egr_l2p, port);
        ioerr += WRITE_EGR_LOGIC_TO_PHYS_MAPr(unit, lport, egr_l2p);
    }

    /* MMU to physical port mapping and MMU to logical port mapping */
    for (mport = 0; mport < num_mport; mport++) {
        port = M2P(unit, mport);
        if (port < 0) {
            port = 0x7f;
            lport = -1;
        } else {
            lport = P2L(unit, port);
        }
        if (lport < 0) {
            lport = 0x3f;
        }
        MMU_TO_PHYS_MAPr_PHYS_PORTf_SET(mmu_m2p, port);
        ioerr += WRITE_MMU_TO_PHYS_MAPr(unit, mport, mmu_m2p);
        MMU_TO_LOGIC_MAPr_LOGIC_PORTf_SET(mmu_m2l, lport);
        ioerr += WRITE_MMU_TO_LOGIC_MAPr(unit, mport, mmu_m2l);
    }

    return ioerr;
}

static int
_mmu_tdm_init(int unit)
{
    int ioerr = 0;
    IARB_TDM_CONTROLr_t iarb_tdm_ctrl;
    IARB_TDM_TABLEm_t arb_tdm;
    LLS_PORT_TDMm_t lls_tdm;
    LLS_TDM_CAL_CFGr_t tdm_cal_cfg;
    const int *default_tdm_seq;
    int tdm_seq[512];
    int tdm_seq_len, tdm_max, idx, mdx;
    cdk_pbmp_t pbmp;
    int step, sub_port;
    int port, mport;

    /* Get default TDM sequence for this configuration */
    tdm_seq_len = bcm56340_a0_tdm_default(unit, &default_tdm_seq);
    if (tdm_seq_len <= 0 || tdm_seq_len > COUNTOF(tdm_seq)) {
        return CDK_E_INTERNAL;
    }
    tdm_max = tdm_seq_len - 1;

    /* Make local copy of TDM sequence */
    for (idx = 0; idx < tdm_seq_len; idx++) {
        tdm_seq[idx] = default_tdm_seq[idx];
    }

    /* Update TDM sequence for all enabled flex ports */
    bcm56340_a0_xport_pbmp_get(unit, XPORT_FLAG_ALL, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        /* Default TDM sequence only contains base ports */
        if (XPORT_SUBPORT(port) != 0) {
            continue;
        }
        step = 0;
        if (BMD_PORT_PROPERTIES(unit, port) & BMD_PORT_FLEX) {
            if (BMD_PORT_PROPERTIES(unit, port+1) != BMD_PORT_FLEX) {
                /* Sub-port 1 is configured: assume quad mode */
                step = 1;
            } else if (BMD_PORT_PROPERTIES(unit, port+2) != BMD_PORT_FLEX) {
                /* Sub-port 2 is configured: assume dual mode */
                step = 2;
            }
        }
        if (step) {
            sub_port = 0;
            /* Replace base port with enabled sub-ports */
            for (idx = 0; idx < tdm_seq_len; idx++) {
                if (tdm_seq[idx] == port) {
                    tdm_seq[idx] += sub_port;
                    sub_port += step;
                    if (sub_port > 3) {
                        sub_port = 0;
                    }
                }
            }
        }
    }

    /* Disable arbiter while programming TDM tables */
    ioerr += READ_IARB_TDM_CONTROLr(unit, &iarb_tdm_ctrl);
    IARB_TDM_CONTROLr_DISABLEf_SET(iarb_tdm_ctrl, 1);
    IARB_TDM_CONTROLr_TDM_WRAP_PTRf_SET(iarb_tdm_ctrl, tdm_max);
    ioerr += WRITE_IARB_TDM_CONTROLr(unit, iarb_tdm_ctrl);

    IARB_TDM_TABLEm_CLR(arb_tdm);
    for (idx = 0; idx < tdm_seq_len; idx++) {
        port = tdm_seq[idx];
        if (port < 0) {
            port = 0x7f;
            mport = 0x3f;
        } else {
            mport = P2M(unit, port);
        }
        IARB_TDM_TABLEm_PORT_NUMf_SET(arb_tdm, port);
        WRITE_IARB_TDM_TABLEm(unit, idx, arb_tdm);

        if (idx & 1) {
            LLS_PORT_TDMm_PORT_ID_1f_SET(lls_tdm, mport);
            LLS_PORT_TDMm_PORT_ID_1_ENABLEf_SET(lls_tdm, 1);
            mdx = idx >> 1;
            ioerr += WRITE_LLS_PORT_TDMm(unit, mdx, lls_tdm);
            ioerr += WRITE_LLS_PORT_TDMm(unit, mdx + 512, lls_tdm);
        } else {
            LLS_PORT_TDMm_PORT_ID_0f_SET(lls_tdm, mport);
            LLS_PORT_TDMm_PORT_ID_0_ENABLEf_SET(lls_tdm, 1);
        }
        CDK_VERB(("%s", (idx == 0) ? "TDM seq:" : ""));
        CDK_VERB(("%s", (idx & 0xf) == 0 ? "\n" : ""));
        CDK_VERB(("%3d ", tdm_seq[idx]));
    }
    CDK_VERB(("\n"));

    /* Enable arbiter */
    ioerr += READ_IARB_TDM_CONTROLr(unit, &iarb_tdm_ctrl);
    IARB_TDM_CONTROLr_DISABLEf_SET(iarb_tdm_ctrl, 0);
#if BMD_CONFIG_SIMULATION
    IARB_TDM_CONTROLr_DISABLEf_SET(iarb_tdm_ctrl, 1);
#endif
    IARB_TDM_CONTROLr_AUX_CMICM_SLOT_ENf_SET(iarb_tdm_ctrl, 1); 
    IARB_TDM_CONTROLr_AUX_AXP_SLOT_ENf_SET(iarb_tdm_ctrl, 1);
    ioerr += WRITE_IARB_TDM_CONTROLr(unit, iarb_tdm_ctrl);

    LLS_TDM_CAL_CFGr_CLR(tdm_cal_cfg);
    LLS_TDM_CAL_CFGr_END_Af_SET(tdm_cal_cfg, tdm_max);
    LLS_TDM_CAL_CFGr_END_Bf_SET(tdm_cal_cfg, tdm_max);
    LLS_TDM_CAL_CFGr_DEFAULT_PORTf_SET(tdm_cal_cfg, PASSTHRU_MPORT);
    LLS_TDM_CAL_CFGr_ENABLEf_SET(tdm_cal_cfg, 1);
    ioerr += WRITE_LLS_TDM_CAL_CFGr(unit, tdm_cal_cfg);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

static uint32_t
_mmu_port_mc_credits(int unit, int port)
{
    uint32_t speed;
    int mport = P2M(unit, port);

    if (mport == 56) {
        return 2 * 1024;
    }
    if (mport == 58) {
        return 4 * 1024;
    }
    if (mport == 59) {
        return 12 * 1024;
    }
    if (mport == 60 || mport == 61) {
        return 256;
    }

    speed = bcm56340_a0_port_speed_max(unit, port);

    if (speed > 42000) {
        return 20 * 1024;
    }
    if (speed >= 20000) {
        return 4 * 1024;
    }
    if (speed >= 10000) {
        return 1024;
    }
    if (speed >= 1000) {
        return 256;
    }
    return 0;
}

static int
_lls_reset(int unit)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    LLS_SOFT_RESETr_t soft_reset;
    LLS_INITr_t lls_init;
    int idx;

    READ_LLS_SOFT_RESETr(unit, &soft_reset);
    LLS_SOFT_RESETr_SOFT_RESETf_SET(soft_reset, 0);
    ioerr  += WRITE_LLS_SOFT_RESETr(unit, soft_reset);

    READ_LLS_INITr(unit, &lls_init);
    LLS_INITr_INITf_SET(lls_init, 1);
    ioerr  += WRITE_LLS_INITr(unit, lls_init);

    for (idx = 0; idx < LLS_RESET_TIMEOUT_MSEC; idx++) {
        READ_LLS_INITr(unit, &lls_init);
        if (LLS_INITr_INIT_DONEf_GET(lls_init)) {
            break;
        }
        BMD_SYS_USLEEP(1000);
    }
    if (idx >= LLS_RESET_TIMEOUT_MSEC) {
        CDK_WARN(("bcm56340_a0_bmd_init[%d]: LLS reset timeout\n", unit));
        return ioerr ? CDK_E_IO : CDK_E_TIMEOUT;
    }
    return ioerr ? CDK_E_IO : rv;
}

/*
 *  Linked List Scheduler (LLS)
 *  ===========================
 *
 *   48 CPU (multicast) queues map to single L1/L0 node:
 *
 *   Queue    L1  L0  MMU port
 *  --------------------------
 *   1536      0   0    59
 *   1537      0   0    59
 *       ...
 *
 *   1583      0   0    59
 *
 *
 *   8/10 per-port multicast queues map to 8/10 L1 nodes:
 *
 *   Queue    L1  L0  MMU port
 *  --------------------------
 *   1024      1   1     0
 *   1025      2   1     0
 *       ...
 *
 *   1531      8   1     0
 *   1032      9   2     1
 *   1033     10   2     1
 *       ...
 *
 *   1539     16   2     1
 *       ...
 *
 *
 *   8/10 per-port unicast queues map to 8/10 L1 nodes:
 *   (same L1/L0 nodes as used by multicast)
 *
 *   Queue    L1  L0  MMU port
 *  --------------------------
 *      0      1   1     0
 *      1      2   1     0
 *       ...
 *
 *      7      8   1     0
 *      8      9   2     1
 *      9     10   2     1
 *       ...
 *
 *     15     16   2     1
 *       ...
 */
static int
_lls_init(int unit)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    LLS_L0_CHILD_WEIGHT_CFGm_t l0_weight;
    LLS_L1_CHILD_WEIGHT_CFGm_t l1_weight;
    LLS_L2_CHILD_WEIGHT_CFGm_t l2_weight;
    LLS_PORT_CONFIGm_t lls_pcfg;
    LLS_L0_CONFIGm_t lls_l0cfg;
    LLS_L1_CONFIGm_t lls_l1cfg;
    LLS_L0_PARENTm_t l0_parent;
    LLS_L1_PARENTm_t l1_parent;
    LLS_L2_PARENTm_t l2_parent;
    ING_COS_MODEr_t icos_mode;
    LLS_CONFIG0r_t lls_cfg;
    cdk_pbmp_t mmu_pbmp;
    int port, mport, lport, idx, qx, num_mc_q, num_uc_q;
    int base, lx0, lx1, lx2;

    /* Get front-panel ports */
    bcm56340_a0_xport_pbmp_get(unit, XPORT_FLAG_ALL, &mmu_pbmp);

    lx0 = 0;
    lx1 = 0;

    /* Default entry values */
    LLS_L0_PARENTm_CLR(l0_parent);
    LLS_L1_PARENTm_CLR(l1_parent);
    LLS_L2_PARENTm_CLR(l2_parent);
    LLS_L0_CHILD_WEIGHT_CFGm_CLR(l0_weight);
    LLS_L1_CHILD_WEIGHT_CFGm_CLR(l1_weight);
    LLS_L2_CHILD_WEIGHT_CFGm_CLR(l2_weight);
    LLS_PORT_CONFIGm_CLR(lls_pcfg);
    LLS_PORT_CONFIGm_L0_LOCK_ON_PACKETf_SET(lls_pcfg, 1);
    LLS_PORT_CONFIGm_L1_LOCK_ON_PACKETf_SET(lls_pcfg, 1);
    LLS_PORT_CONFIGm_L2_LOCK_ON_PACKETf_SET(lls_pcfg, 1);
    LLS_L0_CONFIGm_CLR(lls_l0cfg);
    LLS_L0_CONFIGm_P_CFG_EF_PROPAGATEf_SET(lls_l0cfg, 1);
    LLS_L1_CONFIGm_CLR(lls_l1cfg);
    LLS_L1_CONFIGm_P_CFG_EF_PROPAGATEf_SET(lls_l1cfg, 1);

    /* Configure CPU queues */
    mport = P2M(unit, CMIC_PORT);
    lx2 = MMU_QBASE_CPU;
    num_mc_q = 48;
    LLS_L2_PARENTm_C_PARENTf_SET(l2_parent, lx1);
    /* All CPU queues map to a single L1 node with equal weight */
    LLS_L2_CHILD_WEIGHT_CFGm_C_WEIGHTf_SET(l2_weight, 1);
    for (qx = 0; qx < num_mc_q; qx++) {
        idx = lx2 + qx;
        ioerr += WRITE_LLS_L2_PARENTm(unit, idx, l2_parent);
        ioerr += WRITE_LLS_L2_CHILD_WEIGHT_CFGm(unit, idx, l2_weight);
    }
    /* Enable WRR for L1 node */
    LLS_L1_CONFIGm_P_WRR_IN_USEf_SET(lls_l1cfg, 1);
    LLS_L1_CONFIGm_P_NUM_SPRIf_SET(lls_l1cfg, 0);
    ioerr += WRITE_LLS_L1_CONFIGm(unit, lx1, lls_l1cfg);
    /* Map to single L0 node using strict priority */
    LLS_L1_PARENTm_C_PARENTf_SET(l1_parent, lx0);
    ioerr += WRITE_LLS_L1_PARENTm(unit, lx1, l1_parent);
    ioerr += WRITE_LLS_L1_CHILD_WEIGHT_CFGm(unit, lx1, l1_weight);
    LLS_L0_CONFIGm_P_NUM_SPRIf_SET(lls_l0cfg, 1);
    ioerr += WRITE_LLS_L0_CONFIGm(unit, lx0, lls_l0cfg);
    /* Map L0 node to MMU port using strict priority */
    LLS_L0_PARENTm_C_PARENTf_SET(l0_parent, mport);
    ioerr += WRITE_LLS_L0_PARENTm(unit, lx0, l0_parent);
    ioerr += WRITE_LLS_L0_CHILD_WEIGHT_CFGm(unit, lx0, l0_weight);
    LLS_PORT_CONFIGm_P_NUM_SPRIf_SET(lls_pcfg, 1);
    ioerr += WRITE_LLS_PORT_CONFIGm(unit, mport, lls_pcfg);

    /* Configure front-panel queues */
    lx1 = 10;
    lx0 = 3;
    /* All queues (L2 nodes) map to L1 nodes with equal weight */
    LLS_L2_CHILD_WEIGHT_CFGm_C_WEIGHTf_SET(l2_weight, 1);
    /* All L1 nodes map to L0 nodes with equal weight */
    LLS_L1_CHILD_WEIGHT_CFGm_C_WEIGHTf_SET(l1_weight, 1);
    /* All L0 nodes map to ports with equal weight */
    LLS_L0_CHILD_WEIGHT_CFGm_C_WEIGHTf_SET(l0_weight, 1);
    /* Enable WRR for all queues (L2 nodes )*/
    LLS_L1_CONFIGm_P_WRR_IN_USEf_SET(lls_l1cfg, 1);
    LLS_L1_CONFIGm_P_NUM_SPRIf_SET(lls_l1cfg, 0);
    /* Enable WRR for L1 nodes */
    LLS_L0_CONFIGm_P_WRR_IN_USEf_SET(lls_l0cfg, 1);
    LLS_L0_CONFIGm_P_NUM_SPRIf_SET(lls_l0cfg, 0);
    /* Enable WRR for L0 nodes */
    LLS_PORT_CONFIGm_P_WRR_IN_USEf_SET(lls_pcfg, 1);
    LLS_PORT_CONFIGm_P_NUM_SPRIf_SET(lls_pcfg, 0);
    CDK_PBMP_ITER(mmu_pbmp, port) {
        mport = P2M(unit, port);
        lport = P2L(unit, port);
        if (lport < 0) {
            /* Unconfigured flex port */
            continue;
        }
        num_mc_q = bcm56340_a0_mmu_port_mc_queues(unit, port);
        num_uc_q = bcm56340_a0_mmu_port_uc_queues(unit, port);
        /* Get multicast queue base for this port */
        lx2 = bcm56340_a0_mc_queue_num(unit, port, 0);
        /* Get unicast queue base for this port */
        base = bcm56340_a0_uc_queue_num(unit, port, 0);
        CDK_VVERB(("Port %d (MMU %d): mcq=%d ucq=%d mcbase=%d ucbase=%d\n",
                   port, mport, num_mc_q, num_uc_q, lx2, base));
        for (qx = 0; qx < num_mc_q; qx++) {
            /* Queues 8 and above share the L2 parent */
            if (qx > 8) {
                lx1--;
            }
            /* Map multicast queue to L1 node */
            idx = lx2 + qx;
            LLS_L2_PARENTm_C_PARENTf_SET(l2_parent, lx1);
            ioerr += WRITE_LLS_L2_PARENTm(unit, idx, l2_parent);
            ioerr += WRITE_LLS_L2_CHILD_WEIGHT_CFGm(unit, idx, l2_weight);
            /* Unicast queue uses same L1 node */
            if (qx < num_uc_q) {
                idx = base + qx;
                ioerr += WRITE_LLS_L2_PARENTm(unit, idx, l2_parent);
                ioerr += WRITE_LLS_L2_CHILD_WEIGHT_CFGm(unit, idx, l2_weight);
            }
            /* Map L1 node to L0 node */
            LLS_L1_PARENTm_C_PARENTf_SET(l1_parent, lx0);
            ioerr += WRITE_LLS_L1_PARENTm(unit, lx1, l1_parent);
            /* L2 scheduling mode (equal weight WRR) */
            ioerr += WRITE_LLS_L1_CHILD_WEIGHT_CFGm(unit, lx1, l1_weight);
            ioerr += WRITE_LLS_L1_CONFIGm(unit, lx1, lls_l1cfg);
            /* Map L0 node to MMU port */
            LLS_L0_PARENTm_C_PARENTf_SET(l0_parent, mport);
            ioerr += WRITE_LLS_L0_PARENTm(unit, lx0, l0_parent);
            /* L1 scheduling mode (equal weight WRR) */
            ioerr += WRITE_LLS_L0_CHILD_WEIGHT_CFGm(unit, lx0, l0_weight);
            ioerr += WRITE_LLS_L0_CONFIGm(unit, lx0, lls_l0cfg);
            lx1++;
        }
        lx0++;
        /* L0 scheduling mode */
        ioerr += WRITE_LLS_PORT_CONFIGm(unit, mport, lls_pcfg);
        /* Configure base queue for unicast */
        ING_COS_MODEr_CLR(icos_mode);
        ING_COS_MODEr_BASE_QUEUE_NUM_0f_SET(icos_mode, base);
        ING_COS_MODEr_BASE_QUEUE_NUM_1f_SET(icos_mode, base);
        ioerr += WRITE_ING_COS_MODEr(unit, lport, icos_mode);
    }

    /* Enable LLS */
    LLS_CONFIG0r_CLR(lls_cfg);
    LLS_CONFIG0r_DEQUEUE_ENABLEf_SET(lls_cfg, 1);
    LLS_CONFIG0r_ENQUEUE_ENABLEf_SET(lls_cfg, 1);
    LLS_CONFIG0r_PORT_SCHEDULER_ENABLEf_SET(lls_cfg, 1);
    ioerr += WRITE_LLS_CONFIG0r(unit, lls_cfg);

    return ioerr ? CDK_E_IO : rv;
}

static int
_fifo_init(int unit)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    MCQ_FIFO_BASE_REG_32_35r_t mcq_base32;
    MCQ_FIFO_BASE_REG_36_39r_t mcq_base36;
    MCQ_FIFO_BASE_REGr_t mcq_base;
    MCQ_FIFO_BASE_REG_48_55r_t mcq_base48;
    MCQ_FIFO_BASE_REG_56r_t mcq_base56;
    MCQ_FIFO_BASE_REG_PASSTHRUr_t mcq_base58;
    MCQ_FIFO_BASE_REG_CPUr_t mcq_base59;
    OVQ_MCQ_CREDITSr_t ovq_cred;
    MMU_INTFO_CONGST_STr_t cng_st;
    MCFIFO_CONFIGr_t mcfifo_cfg;
    cdk_pbmp_t mmu_pbmp;
    uint32_t credits, fifo_base;
    uint32_t mode_combine;
    int num_q, total_q;
    int port, mport, idx;

    /* Get MMU ports */
    bcm56340_a0_xport_pbmp_get(unit, XPORT_FLAG_ALL, &mmu_pbmp);
    CDK_PBMP_PORT_ADD(mmu_pbmp, CMIC_PORT);

    /* Configure multicast FIFO credits */
    CDK_PBMP_ITER(mmu_pbmp, port) {
        mport = P2M(unit, port);
        total_q = (mport >= 40 && mport <= 55) ? 10 : 8;
        num_q = bcm56340_a0_mmu_port_mc_queues(unit, port);
        if (num_q == 0) {
            continue;
        }
        credits = _mmu_port_mc_credits(unit, port) / num_q;
        fifo_base = (credits < 2048) ? credits : 0;
        
        if (mport >= 32 && mport <= 35) {
            for (idx = 0; idx < total_q; idx++) {
                MCQ_FIFO_BASE_REG_32_35r_SET(mcq_base32, idx * fifo_base);
                ioerr += WRITE_MCQ_FIFO_BASE_REG_32_35r(unit, mport,
                                                        idx, mcq_base32);
            }
        } else if (mport >= 36 && mport <= 39) {
            for (idx = 0; idx < total_q; idx++) {
                MCQ_FIFO_BASE_REG_36_39r_SET(mcq_base36, idx * fifo_base);
                ioerr += WRITE_MCQ_FIFO_BASE_REG_36_39r(unit, mport,
                                                        idx, mcq_base36);
            }
        } else if (mport >= 40 && mport <= 47) {
            for (idx = 0; idx < total_q; idx++) {
                MCQ_FIFO_BASE_REGr_SET(mcq_base, idx * fifo_base);
                ioerr += WRITE_MCQ_FIFO_BASE_REGr(unit, mport,
                                                  idx, mcq_base);
            }
        } else if (mport >= 48 && mport <= 55) {
            for (idx = 0; idx < total_q; idx++) {
                MCQ_FIFO_BASE_REG_48_55r_SET(mcq_base48, idx * fifo_base);
                ioerr += WRITE_MCQ_FIFO_BASE_REG_48_55r(unit, mport,
                                                        idx, mcq_base48);
            }
        } else if (mport == 56) {
            for (idx = 0; idx < total_q; idx++) {
                MCQ_FIFO_BASE_REG_56r_SET(mcq_base56, idx * fifo_base);
                ioerr += WRITE_MCQ_FIFO_BASE_REG_56r(unit, mport,
                                                     idx, mcq_base56);
            }
        } else if (mport == 58) {
            for (idx = 0; idx < total_q; idx++) {
                MCQ_FIFO_BASE_REG_PASSTHRUr_SET(mcq_base58, idx * fifo_base);
                ioerr += WRITE_MCQ_FIFO_BASE_REG_PASSTHRUr(unit, mport,
                                                           idx, mcq_base58);
            }
        } else if (mport == 59) {
            for (idx = 0; idx < total_q; idx++) {
                MCQ_FIFO_BASE_REG_CPUr_SET(mcq_base59, idx * fifo_base);
                ioerr += WRITE_MCQ_FIFO_BASE_REG_CPUr(unit, mport,
                                                      idx, mcq_base59);
            }
        }

        for (idx = 0; idx < total_q; idx++) {
            OVQ_MCQ_CREDITSr_SET(ovq_cred, credits);
            if (idx > num_q) {
                OVQ_MCQ_CREDITSr_CLR(ovq_cred);
            }
            ioerr += WRITE_OVQ_MCQ_CREDITSr(unit, mport, idx, ovq_cred);
        }

        MMU_INTFO_CONGST_STr_CLR(cng_st);
        MMU_INTFO_CONGST_STr_ENf_SET(cng_st, 1);
        ioerr += WRITE_MMU_INTFO_CONGST_STr(unit, mport, cng_st);
    }

    /* Configure multicast FIFO mode */
    mode_combine = 0;
    CDK_PBMP_ITER(mmu_pbmp, port) {
        mport = P2M(unit, port);
        if (mport >= 32 && mport < 48) {
            if (bcm56340_a0_port_speed_max(unit, port) > 1000) {
                mode_combine |= LSHIFT32(1, mport - 32);
            }
        }
    }
    MCFIFO_CONFIGr_CLR(mcfifo_cfg);
    MCFIFO_CONFIGr_MODE_COMBINEf_SET(mcfifo_cfg, mode_combine);
    ioerr += WRITE_MCFIFO_CONFIGr(unit, mcfifo_cfg);

    return ioerr ? CDK_E_IO : rv;
}

static int
_mmu_set_limits(int unit)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    CFAPFULLTHRESHOLDr_t cfap_full_th;
    PORT_MAX_PKT_SIZEr_t max_pkt_sz;
    PORT_PRI_GRPr_t port_pri_grp;
    PORT_PRI_XON_ENABLEr_t xon_enable;
    THDI_PORT_SP_CONFIGm_t port_sp_config;
    THDI_PORT_PG_CONFIGm_t port_pg_config;
    USE_SP_SHAREDr_t use_sp_shared;
    BUFFER_CELL_LIMIT_SPr_t buf_cell_limit;
    CELL_RESET_LIMIT_OFFSET_SPr_t cell_reset_limit;
    GLOBAL_HDRM_LIMITr_t global_hdrm_limit;
    MMU_THDO_CONFIG_QGROUPm_t cfg_qgrp;
    OP_QUEUE_CONFIG_CELLr_t opq_cfg_cell;
    OP_QUEUE_LIMIT_COLOR_CELLr_t opq_lcol_cell;
    OP_QUEUE_RESET_OFFSET_CELLr_t opq_rsto_cell;
    MMU_THDO_CONFIG_QUEUEm_t cfg_queue;
    MMU_THDO_OFFSET_QUEUEm_t off_queue;
    OP_BUFFER_SHARED_LIMIT_CELLr_t opb_shl_cell;
    OP_BUFFER_LIMIT_YELLOW_CELLr_t opb_lim_y_cell;
    OP_BUFFER_LIMIT_RED_CELLr_t opb_lim_r_cell;
    OP_BUFFER_SHARED_LIMIT_RESUME_CELLr_t opb_shl_rsm_cell;
    OP_BUFFER_LIMIT_RESUME_YELLOW_CELLr_t opb_lim_y_rsm_cell;
    OP_BUFFER_LIMIT_RESUME_RED_CELLr_t opb_lim_r_rsm_cell;
    MMU_THDO_CONFIG_PORTm_t cfg_port;
    OP_PORT_CONFIG_CELLr_t op_cfg_cell;
    OP_PORT_LIMIT_COLOR_CELLr_t op_lim_col_cell;
    OP_PORT_LIMIT_RESUME_COLOR_CELLr_t op_rsm_col_cell;
    cdk_pbmp_t mmu_pbmp;
    uint32_t pg_pbm;
    uint32_t rval, fval;
    uint32_t max_packet_cells, jumbo_frame_cells, default_mtu_cells;
    uint32_t total_cells, in_reserved_cells, out_reserved_cells;
    int num_port, num_q;
    int port, mport, base, idx;

    /* Get front-panel ports */
    bcm56340_a0_xport_pbmp_get(unit, XPORT_FLAG_ALL, &mmu_pbmp);

    /* Number of front-panel ports */
    num_port = 0;
    CDK_PBMP_ITER(mmu_pbmp, port) {
        num_port++;
    }

    /* Add CPU port to MMU ports */
    CDK_PBMP_PORT_ADD(mmu_pbmp, CMIC_PORT);

    /* Number of output queues */
    num_q = 0;
    CDK_PBMP_ITER(mmu_pbmp, port) {
        if (port == CMIC_PORT) {
            continue;
        }
        mport = P2M(unit, port);
        num_q += bcm56340_a0_mmu_port_mc_queues(unit, port);
        num_q += bcm56340_a0_mmu_port_uc_queues(unit, port);
    }

    max_packet_cells =
        (MMU_MAX_PKT_BYTES + MMU_PKT_HDR_BYTES + MMU_CELL_BYTES - 1) /
        MMU_CELL_BYTES;
    jumbo_frame_cells =
        (MMU_JUMBO_FRAME_BYTES + MMU_PKT_HDR_BYTES + MMU_CELL_BYTES - 1) /
        MMU_CELL_BYTES;
    default_mtu_cells =
        (MMU_DEFAULT_MTU_BYTES + MMU_PKT_HDR_BYTES + MMU_CELL_BYTES - 1) /
        MMU_CELL_BYTES;

    /*
     * Input port pool allocation precedence:
     *   reserved space: per-port per-PG minimum space
     *   reserved space: per-port minimum space (include cpu port)
     *   shared space = total - input port reserved - output port reserved
     *   reserved space: per-port per-PG headroom
     *   reserved space: per-device global headroom
     * Output port:
     *   reserved space: per-port per-queue minimum space
     *   shared space = total - output port reserved
     */
    if(CDK_XGSM_FLAGS(unit) & CHIP_FLAG_MMU10) {
        total_cells = MMU_TOTAL_CELLS_10K;
    } else {
        total_cells = MMU_TOTAL_CELLS_19K;
    }

    total_cells -= MMU_CELLS_RSVD_IP;
    in_reserved_cells = (num_port + 1) * jumbo_frame_cells
        + num_port * MMU_PG_PER_PORT * MMU_IN_PG_HDRM_CELLS
        + num_port * default_mtu_cells;
    out_reserved_cells = num_q * MMU_OUT_PORT_MIN_CELLS
        + 2 * jumbo_frame_cells;

    pg_pbm = 0;
    for (idx = 8 - MMU_PG_PER_PORT; idx < 8; idx++) {
        pg_pbm |= LSHIFT32(1, idx);
    }

    CFAPFULLTHRESHOLDr_CLR(cfap_full_th);
    CFAPFULLTHRESHOLDr_CFAPFULLSETPOINTf_SET(cfap_full_th, total_cells);
    fval = total_cells - out_reserved_cells;
    CFAPFULLTHRESHOLDr_CFAPFULLRESETPOINTf_SET(cfap_full_th, fval);
    ioerr += WRITE_CFAPFULLTHRESHOLDr(unit, cfap_full_th);

    /* Input port misc per-port setting */
    PORT_MAX_PKT_SIZEr_CLR(max_pkt_sz);
    PORT_MAX_PKT_SIZEr_PORT_MAX_PKT_SIZEf_SET(max_pkt_sz, max_packet_cells);

    /* All priorities use the default priority group */
    rval = 0;
    for (idx = 0; idx < 8; idx++) {
        /* Three bits per priority */
        rval |= MMU_DEFAULT_PG << (3 * idx);
    }
    PORT_PRI_GRPr_SET(port_pri_grp, rval);

    PORT_PRI_XON_ENABLEr_SET(xon_enable, 0xffff);

    CDK_PBMP_ITER(mmu_pbmp, port) {
        mport = P2M(unit, port);
        ioerr += WRITE_PORT_MAX_PKT_SIZEr(unit, mport, max_pkt_sz);
        ioerr += WRITE_PORT_PRI_GRPr(unit, mport, 0, port_pri_grp);
        ioerr += WRITE_PORT_PRI_GRPr(unit, mport, 1, port_pri_grp);
        if (port == CMIC_PORT) {
            continue;
        }
        ioerr += WRITE_PORT_PRI_XON_ENABLEr(unit, mport, xon_enable);
    }

    /* Input port per-port limits */
    THDI_PORT_SP_CONFIGm_CLR(port_sp_config);
    THDI_PORT_SP_CONFIGm_PORT_SP_MAX_LIMITf_SET(port_sp_config, total_cells);
    fval = total_cells - (2 * default_mtu_cells);
    THDI_PORT_SP_CONFIGm_PORT_SP_RESUME_LIMITf_SET(port_sp_config, fval);
    CDK_PBMP_ITER(mmu_pbmp, port) {
        mport = P2M(unit, port);
        idx = mport * MMU_NUM_POOL;
        ioerr += WRITE_THDI_PORT_SP_CONFIGm(unit, idx, port_sp_config);
    }

    /* Input port per-port per-PG limits */
    THDI_PORT_PG_CONFIGm_CLR(port_pg_config);
    THDI_PORT_PG_CONFIGm_PG_RESET_OFFSETf_SET(port_pg_config, 16);
    THDI_PORT_PG_CONFIGm_PG_MIN_LIMITf_SET(port_pg_config, jumbo_frame_cells);
    THDI_PORT_PG_CONFIGm_PG_SHARED_LIMITf_SET(port_pg_config, 7);
    THDI_PORT_PG_CONFIGm_PG_SHARED_DYNAMICf_SET(port_pg_config, 1);
    THDI_PORT_PG_CONFIGm_PG_GBL_HDRM_ENf_SET(port_pg_config, 1);
    THDI_PORT_PG_CONFIGm_PG_HDRM_LIMITf_SET(port_pg_config,
                                            MMU_IN_PG_HDRM_CELLS);
    CDK_PBMP_ITER(mmu_pbmp, port) {
        mport = P2M(unit, port);
        base = mport * MMU_NUM_PG;
        for (idx = base; idx < (base + MMU_NUM_PG); idx++) {
            if (idx < (base + (MMU_NUM_PG - MMU_PG_PER_PORT))) {
                continue;
            }
            ioerr += WRITE_THDI_PORT_PG_CONFIGm(unit, idx, port_pg_config);
        }
    }

    /* Input port shared space (use service pool 0 only) */
    USE_SP_SHAREDr_CLR(use_sp_shared);
    USE_SP_SHAREDr_ENABLEf_SET(use_sp_shared, 1);
    ioerr += WRITE_USE_SP_SHAREDr(unit, use_sp_shared);

    BUFFER_CELL_LIMIT_SPr_CLR(buf_cell_limit);
    fval = total_cells - in_reserved_cells - out_reserved_cells;
    BUFFER_CELL_LIMIT_SPr_LIMITf_SET(buf_cell_limit, fval);
    ioerr += WRITE_BUFFER_CELL_LIMIT_SPr(unit, 0, buf_cell_limit);

    CELL_RESET_LIMIT_OFFSET_SPr_CLR(cell_reset_limit);
    fval = 30 * default_mtu_cells;
    CELL_RESET_LIMIT_OFFSET_SPr_OFFSETf_SET(cell_reset_limit, fval);
    ioerr += WRITE_CELL_RESET_LIMIT_OFFSET_SPr(unit, 0, cell_reset_limit);

    /* Input port per-device global headroom */
    GLOBAL_HDRM_LIMITr_CLR(global_hdrm_limit);
    fval = 2 * jumbo_frame_cells;
    GLOBAL_HDRM_LIMITr_GLOBAL_HDRM_LIMITf_SET(global_hdrm_limit, fval);
    ioerr += WRITE_GLOBAL_HDRM_LIMITr(unit, global_hdrm_limit);

    /* Output Q-groups */
    MMU_THDO_CONFIG_QGROUPm_CLR(cfg_qgrp);
    MMU_THDO_CONFIG_QGROUPm_Q_SHARED_LIMIT_CELLf_SET(cfg_qgrp, total_cells);
    MMU_THDO_CONFIG_QGROUPm_Q_COLOR_LIMIT_DYNAMIC_CELLf_SET(cfg_qgrp, 1);
    fval = (total_cells * 125) / 1000;
    MMU_THDO_CONFIG_QGROUPm_LIMIT_YELLOW_CELLf_SET(cfg_qgrp, fval);
    MMU_THDO_CONFIG_QGROUPm_LIMIT_RED_CELLf_SET(cfg_qgrp, fval);
    for (idx = 0; idx <= MMU_THDO_CONFIG_QGROUPm_MAX; idx++) {
        ioerr += WRITE_MMU_THDO_CONFIG_QGROUPm(unit, idx, cfg_qgrp);
    }

    /* Output Q-groups are off by default */
    ioerr += CDK_XGSM_MEM_CLEAR(unit, MMU_THDO_Q_TO_QGRP_MAPm);

    /* Output multicast queues */
    fval = total_cells - out_reserved_cells;
    OP_QUEUE_CONFIG_CELLr_CLR(opq_cfg_cell);
    OP_QUEUE_CONFIG_CELLr_Q_SHARED_LIMIT_CELLf_SET(opq_cfg_cell, fval);
    fval = total_cells / 8;
    OP_QUEUE_LIMIT_COLOR_CELLr_CLR(opq_lcol_cell);
    OP_QUEUE_LIMIT_COLOR_CELLr_REDf_SET(opq_lcol_cell, fval);
    OP_QUEUE_RESET_OFFSET_CELLr_CLR(opq_rsto_cell);
    OP_QUEUE_RESET_OFFSET_CELLr_Q_RESET_OFFSET_CELLf_SET(opq_rsto_cell, 2);
    CDK_PBMP_ITER(mmu_pbmp, port) {
        mport = P2M(unit, port);
        num_q = bcm56340_a0_mmu_port_mc_queues(unit, port);
        for (idx = 0; idx <= num_q; idx++) {
            ioerr += WRITE_OP_QUEUE_CONFIG_CELLr(unit, mport, idx,
                                                 opq_cfg_cell);
            ioerr += WRITE_OP_QUEUE_LIMIT_COLOR_CELLr(unit, mport, idx,
                                                      opq_lcol_cell);
            ioerr += WRITE_OP_QUEUE_RESET_OFFSET_CELLr(unit, mport, idx,
                                                       opq_rsto_cell);
        }
    }

    /* Output unicast queues */
    MMU_THDO_CONFIG_QUEUEm_CLR(cfg_queue);
    MMU_THDO_CONFIG_QUEUEm_Q_SHARED_LIMIT_CELLf_SET(cfg_queue, total_cells);
    MMU_THDO_CONFIG_QUEUEm_Q_COLOR_LIMIT_DYNAMIC_CELLf_SET(cfg_queue, 1);
    fval = (total_cells * 125) / 1000;
    MMU_THDO_CONFIG_QUEUEm_LIMIT_YELLOW_CELLf_SET(cfg_queue, fval);
    MMU_THDO_CONFIG_QUEUEm_LIMIT_RED_CELLf_SET(cfg_queue, fval);
    MMU_THDO_OFFSET_QUEUEm_CLR(off_queue);
    MMU_THDO_OFFSET_QUEUEm_RESET_OFFSET_CELLf_SET(off_queue, 2);
    MMU_THDO_OFFSET_QUEUEm_RESET_OFFSET_YELLOW_CELLf_SET(off_queue, 2);
    MMU_THDO_OFFSET_QUEUEm_RESET_OFFSET_RED_CELLf_SET(off_queue, 2);
    CDK_PBMP_ITER(mmu_pbmp, port) {
        if (port == CMIC_PORT) {
            continue;
        }
        mport = P2M(unit, port);
        num_q = bcm56340_a0_mmu_port_uc_queues(unit, port);
        for (idx = 0; idx <= num_q; idx++) {
            ioerr += WRITE_MMU_THDO_CONFIG_QUEUEm(unit, idx, cfg_queue);
            ioerr += WRITE_MMU_THDO_OFFSET_QUEUEm(unit, idx, off_queue);
        }
    }

    /* Global limits for service pool 0 */
    fval = total_cells - out_reserved_cells;
    OP_BUFFER_SHARED_LIMIT_CELLr_CLR(opb_shl_cell);
    OP_BUFFER_SHARED_LIMIT_CELLr_SET(opb_shl_cell, fval);
    ioerr += WRITE_OP_BUFFER_SHARED_LIMIT_CELLr(unit, 0, opb_shl_cell);
    OP_BUFFER_SHARED_LIMIT_RESUME_CELLr_CLR(opb_shl_rsm_cell);
    OP_BUFFER_SHARED_LIMIT_RESUME_CELLr_SET(opb_shl_rsm_cell, fval);
    ioerr += WRITE_OP_BUFFER_SHARED_LIMIT_RESUME_CELLr(unit, 0, opb_shl_rsm_cell);
    fval = fval / 8;
    OP_BUFFER_LIMIT_YELLOW_CELLr_CLR(opb_lim_y_cell);
    OP_BUFFER_LIMIT_YELLOW_CELLr_SET(opb_lim_y_cell, fval);
    ioerr += WRITE_OP_BUFFER_LIMIT_YELLOW_CELLr(unit, 0, opb_lim_y_cell);
    OP_BUFFER_LIMIT_RED_CELLr_CLR(opb_lim_r_cell);
    OP_BUFFER_LIMIT_RED_CELLr_SET(opb_lim_r_cell, fval);
    ioerr += WRITE_OP_BUFFER_LIMIT_RED_CELLr(unit, 0, opb_lim_r_cell);
    OP_BUFFER_LIMIT_RESUME_YELLOW_CELLr_CLR(opb_lim_y_rsm_cell);
    OP_BUFFER_LIMIT_RESUME_YELLOW_CELLr_SET(opb_lim_y_rsm_cell, fval);
    ioerr += WRITE_OP_BUFFER_LIMIT_RESUME_YELLOW_CELLr(unit, 0, opb_lim_y_rsm_cell);
    OP_BUFFER_LIMIT_RESUME_RED_CELLr_CLR(opb_lim_r_rsm_cell);
    OP_BUFFER_LIMIT_RESUME_RED_CELLr_SET(opb_lim_r_rsm_cell, fval);
    ioerr += WRITE_OP_BUFFER_LIMIT_RESUME_RED_CELLr(unit, 0, opb_lim_r_rsm_cell);

    /* Per-port multicast limits for service pool 0 */
    fval = total_cells - out_reserved_cells;
    OP_PORT_CONFIG_CELLr_CLR(op_cfg_cell);
    OP_PORT_CONFIG_CELLr_OP_SHARED_LIMIT_CELLf_SET(op_cfg_cell, fval);
    OP_PORT_CONFIG_CELLr_OP_SHARED_RESET_VALUE_CELLf_SET(op_cfg_cell, fval - 16);
    fval = fval / 8;
    OP_PORT_LIMIT_COLOR_CELLr_CLR(op_lim_col_cell);
    OP_PORT_LIMIT_COLOR_CELLr_REDf_SET(op_lim_col_cell, fval);
    OP_PORT_LIMIT_RESUME_COLOR_CELLr_CLR(op_rsm_col_cell);
    OP_PORT_LIMIT_RESUME_COLOR_CELLr_REDf_SET(op_rsm_col_cell, fval - 2);

    CDK_PBMP_ITER(mmu_pbmp, port) {
        mport = P2M(unit, port);
        ioerr += WRITE_OP_PORT_CONFIG_CELLr(unit, mport, 0,
                                            op_cfg_cell);
        ioerr += WRITE_OP_PORT_LIMIT_COLOR_CELLr(unit, mport, 0,
                                                 op_lim_col_cell);
        ioerr += WRITE_OP_PORT_LIMIT_RESUME_COLOR_CELLr(unit, mport, 0,
                                                        op_rsm_col_cell);
    }

    /* Per-port unicast limits for service pool 0 */
    fval = total_cells - out_reserved_cells;
    MMU_THDO_CONFIG_PORTm_CLR(cfg_port);
    MMU_THDO_CONFIG_PORTm_SHARED_LIMITf_SET(cfg_port, fval);
    MMU_THDO_CONFIG_PORTm_SHARED_RESUMEf_SET(cfg_port, fval - 16);
    fval = fval / 8;
    MMU_THDO_CONFIG_PORTm_YELLOW_LIMITf_SET(cfg_port, fval);
    MMU_THDO_CONFIG_PORTm_YELLOW_RESUMEf_SET(cfg_port, fval - 2);
    MMU_THDO_CONFIG_PORTm_RED_LIMITf_SET(cfg_port, fval);
    MMU_THDO_CONFIG_PORTm_RED_RESUMEf_SET(cfg_port, fval - 2);
    CDK_PBMP_ITER(mmu_pbmp, port) {
        mport = P2M(unit, port);
        ioerr += WRITE_MMU_THDO_CONFIG_PORTm(unit, mport * 4, cfg_port);
    }

    return ioerr ? CDK_E_IO : rv;
}

static int
_mmu_init(int unit)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    IP_TO_CMICM_CREDIT_TRANSFERr_t cmic_cred_xfer;
    EGR_EDB_XMIT_CTRLm_t xmit_ctrl;
    OVQ_DROP_THRESHOLD0r_t ovq_drop_thr;
    OVQ_DROP_THRESHOLD_RESET_LIMITr_t ovq_drop_lim;
    OP_THR_CONFIGr_t op_thr_cfg;
    INPUT_PORT_RX_ENABLE_64r_t in_rx_en;
    OUTPUT_PORT_RX_ENABLE0_64r_t out_rx_en;
    PORT_PAUSE_ENABLE_64r_t pause_en;
    int idx, start_cnt;

    /* Reset linked-list scheduler */
    if (CDK_SUCCESS(rv)) {
        rv = _lls_reset(unit);
    }

    /* Setup TDM for MMU */
    if (CDK_SUCCESS(rv)) {
        rv = _mmu_tdm_init(unit);
    }

    /* Configure MMU limits and guarantees */
    if (CDK_SUCCESS(rv)) {
        rv = _mmu_set_limits(unit);
    }

    /* Configure linked-list scheduler */
    if (CDK_SUCCESS(rv)) {
        rv = _lls_init(unit);
    }

    /* Configure multicast FIFO */
    if (CDK_SUCCESS(rv)) {
        rv = _fifo_init(unit);
    }

    /* Enable IP to CMICM credit transfer */
    IP_TO_CMICM_CREDIT_TRANSFERr_CLR(cmic_cred_xfer);
    IP_TO_CMICM_CREDIT_TRANSFERr_TRANSFER_ENABLEf_SET(cmic_cred_xfer, 1);
    IP_TO_CMICM_CREDIT_TRANSFERr_NUM_OF_CREDITSf_SET(cmic_cred_xfer, 32);
    ioerr += WRITE_IP_TO_CMICM_CREDIT_TRANSFERr(unit, cmic_cred_xfer);

    /* Transmit start thresholds */
    EGR_EDB_XMIT_CTRLm_CLR(xmit_ctrl);
    for (idx = 0; idx <= EGR_EDB_XMIT_CTRLm_MAX; idx++) {
        start_cnt = (idx < 53) ? 7 : 2;
        EGR_EDB_XMIT_CTRLm_START_CNTf_SET(xmit_ctrl, start_cnt);
        ioerr += WRITE_EGR_EDB_XMIT_CTRLm(unit, idx, xmit_ctrl);
    }

    /* OVQ settings */
    OVQ_DROP_THRESHOLD0r_SET(ovq_drop_thr, 0x17cf);
    ioerr += WRITE_OVQ_DROP_THRESHOLD0r(unit, ovq_drop_thr);
    OVQ_DROP_THRESHOLD_RESET_LIMITr_SET(ovq_drop_lim, 0x1700);
    ioerr += WRITE_OVQ_DROP_THRESHOLD_RESET_LIMITr(unit, ovq_drop_lim);

    /* Egress policies */
    OP_THR_CONFIGr_CLR(op_thr_cfg);
    OP_THR_CONFIGr_MOP_POLICYf_SET(op_thr_cfg, 7);
    OP_THR_CONFIGr_YELLOW_CELL_DS_SELECTf_SET(op_thr_cfg, 1);
    ioerr += WRITE_OP_THR_CONFIGr(unit, op_thr_cfg);

    /* Enable all ports */
    INPUT_PORT_RX_ENABLE_64r_SET(in_rx_en, 0, 0xffffffff);
    INPUT_PORT_RX_ENABLE_64r_SET(in_rx_en, 1, 0x7fffffff);
    ioerr += WRITE_INPUT_PORT_RX_ENABLE_64r(unit, in_rx_en);
    OUTPUT_PORT_RX_ENABLE0_64r_SET(out_rx_en, 0, 0xffffffff);
    OUTPUT_PORT_RX_ENABLE0_64r_SET(out_rx_en, 1, 0x7fffffff);
    ioerr += WRITE_OUTPUT_PORT_RX_ENABLE0_64r(unit, out_rx_en);

    /* Input port pause enable */
    PORT_PAUSE_ENABLE_64r_SET(pause_en, 0, 0xffffffff);
    PORT_PAUSE_ENABLE_64r_SET(pause_en, 1, 0x7fffffff);
    ioerr += WRITE_PORT_PAUSE_ENABLE_64r(unit, pause_en);

    return ioerr ? CDK_E_IO : rv;
}

static int
_ism_init(int unit)
{
    int ioerr = 0;
    STAGE_BANK_SIZEr_t bank_size;
    TABLE_BANK_CONFIGr_t bank_cfg;
    TABLE0_LOG_TO_PHY_MAPm_t tbl0_map;
    TABLE1_LOG_TO_PHY_MAPm_t tbl1_map;
    TABLE2_LOG_TO_PHY_MAPm_t tbl2_map;
    TABLE3_LOG_TO_PHY_MAPm_t tbl3_map;
    TABLE4_LOG_TO_PHY_MAPm_t tbl4_map;
    STAGE_HASH_OFFSETr_t sho[ISM_MAX_STAGES];
    const _ism_tbl_cfg_t *tbl_cfg;
    const _ism_real_bank_map_t *real_bank_map;
    uint32_t bank_info;
    uint32_t bank_mask[ISM_MAX_STAGES];
    int num_tbl_cfg;
    int bank, stage, offset;
    int idx, tdx, bdx, mdx, rdx, rbdx;
    int max_bank, bank_per_stage ;

    if (CDK_XGSM_FLAGS(unit) & CHIP_FLAG_ISM80) {
        tbl_cfg = _ism_tbl_cfg_80;
        num_tbl_cfg = COUNTOF(_ism_tbl_cfg_80);
        real_bank_map = _ism_real_bank_map_80;
        max_bank = ISM_MAX_BANKS_80 ;
        bank_per_stage = ISM_BANKS_PER_STAGE_80 ;
        offset = 1;
    } else {
        tbl_cfg = _ism_tbl_cfg_96;
        num_tbl_cfg = COUNTOF(_ism_tbl_cfg_96);
        real_bank_map = _ism_real_bank_map_96;
        max_bank = ISM_MAX_BANKS_96 ;
        bank_per_stage = ISM_BANKS_PER_STAGE_96 ;
        offset = 1;
    }

    STAGE_BANK_SIZEr_CLR(bank_size);
    if (CDK_XGSM_FLAGS(unit) & CHIP_FLAG_ISM80) {
        STAGE_BANK_SIZEr_BANK0_SIZE_LIMITf_SET(bank_size, ISM_BANK_SIZE_DISABLED);
        STAGE_BANK_SIZEr_BANK1_SIZE_LIMITf_SET(bank_size, ISM_BANK_SIZE_QUARTER);
        STAGE_BANK_SIZEr_BANK2_SIZE_LIMITf_SET(bank_size, ISM_BANK_SIZE_QUARTER);
        STAGE_BANK_SIZEr_BANK3_SIZE_LIMITf_SET(bank_size, ISM_BANK_SIZE_HALF);
        STAGE_BANK_SIZEr_BANK4_SIZE_LIMITf_SET(bank_size, ISM_BANK_SIZE_HALF);
    } else { /* CHIP_FLAG_ISM96 */
        STAGE_BANK_SIZEr_BANK0_SIZE_LIMITf_SET(bank_size, ISM_BANK_SIZE_DISABLED);
        STAGE_BANK_SIZEr_BANK1_SIZE_LIMITf_SET(bank_size, ISM_BANK_SIZE_QUARTER);
        STAGE_BANK_SIZEr_BANK2_SIZE_LIMITf_SET(bank_size, ISM_BANK_SIZE_HALF);
        STAGE_BANK_SIZEr_BANK3_SIZE_LIMITf_SET(bank_size, ISM_BANK_SIZE_HALF);
        STAGE_BANK_SIZEr_BANK4_SIZE_LIMITf_SET(bank_size, ISM_BANK_SIZE_HALF);
    }
    /* All stages uses same configuration */
    for (bdx = 0; bdx < ISM_MAX_STAGES; bdx++) {
        ioerr += WRITE_STAGE_BANK_SIZEr(unit, bdx, bank_size);
    }

    /* Read hash offset registers */
    for (stage = 0; stage < ISM_MAX_STAGES; stage++) {
        ioerr += READ_STAGE_HASH_OFFSETr(unit, stage, &sho[stage]);
    }

    for (tdx = 0; tdx < num_tbl_cfg; tdx++, tbl_cfg++) {
        /* Configure which banks that are used for this table */
        CDK_MEMSET(bank_mask, 0, sizeof(bank_mask));
        for (bdx = 0; bdx < tbl_cfg->num_banks; bdx++) {
            bank_info = tbl_cfg->bank_info[bdx];
            stage = ISM_BANK_INFO_STAGE_NO(bank_info);
            bank = ISM_BANK_INFO_BANK_NO(bank_info);
            bank_mask[stage] |= (1 << (bank+offset));
        }
        TABLE_BANK_CONFIGr_CLR(bank_cfg);
        TABLE_BANK_CONFIGr_STAGE0_BANKSf_SET(bank_cfg, bank_mask[0]);
        TABLE_BANK_CONFIGr_STAGE1_BANKSf_SET(bank_cfg, bank_mask[1]);
        TABLE_BANK_CONFIGr_STAGE2_BANKSf_SET(bank_cfg, bank_mask[2]);
        TABLE_BANK_CONFIGr_STAGE3_BANKSf_SET(bank_cfg, bank_mask[3]);
        TABLE_BANK_CONFIGr_HASH_ZERO_OR_LSBf_SET(bank_cfg, 1);
        TABLE_BANK_CONFIGr_MAPPING_MODEf_SET(bank_cfg, 1);
        ioerr += WRITE_TABLE_BANK_CONFIGr(unit, tdx, bank_cfg);

        mdx = 0;
        for (idx = 0; idx < max_bank; idx++) {
            stage = idx % ISM_MAX_STAGES;
            bank = idx / ISM_MAX_STAGES;
            for (bdx = 0; bdx < tbl_cfg->num_banks; bdx++) {
                if (ISM_BANK_INFO(stage, bank) == tbl_cfg->bank_info[bdx]) {
                    rbdx = stage * bank_per_stage + bank;
                    for (rdx = 0; rdx < real_bank_map[rbdx].count; rdx++) {
                        if (tdx == 0) {
                            TABLE0_LOG_TO_PHY_MAPm_SET(tbl0_map, real_bank_map[rbdx].index[rdx]);
                            ioerr += WRITE_TABLE0_LOG_TO_PHY_MAPm(unit, mdx, tbl0_map);
                        } else if (tdx == 1) {
                            TABLE1_LOG_TO_PHY_MAPm_SET(tbl1_map, real_bank_map[rbdx].index[rdx]);
                            ioerr += WRITE_TABLE1_LOG_TO_PHY_MAPm(unit, mdx, tbl1_map);
                        } else if (tdx == 2) {
                            TABLE2_LOG_TO_PHY_MAPm_SET(tbl2_map, real_bank_map[rbdx].index[rdx]);
                            ioerr += WRITE_TABLE2_LOG_TO_PHY_MAPm(unit, mdx, tbl2_map);
                        } else if (tdx == 3) {
                            TABLE3_LOG_TO_PHY_MAPm_SET(tbl3_map, real_bank_map[rbdx].index[rdx]);
                            ioerr += WRITE_TABLE3_LOG_TO_PHY_MAPm(unit, mdx, tbl3_map);
                        } else if (tdx == 4) {
                            TABLE4_LOG_TO_PHY_MAPm_SET(tbl4_map, real_bank_map[rbdx].index[rdx]);
                            ioerr += WRITE_TABLE4_LOG_TO_PHY_MAPm(unit, mdx, tbl4_map);
                        }
                        mdx++;
                    }
                }
                            
            }
        }

        /* Update hash offset register for this bank */
        for (bdx = 0; bdx < tbl_cfg->num_banks; bdx++) {
            stage = ISM_BANK_INFO_STAGE_NO(tbl_cfg->bank_info[bdx]);
            bank = ISM_BANK_INFO_BANK_NO(tbl_cfg->bank_info[bdx]);
            if (bank == 0) {
                STAGE_HASH_OFFSETr_BANK0_HASH_OFFSETf_SET(sho[stage], bdx * 4);
            } else if (bank == 1) {
                STAGE_HASH_OFFSETr_BANK1_HASH_OFFSETf_SET(sho[stage], bdx * 4);
            } else if (bank == 2) {
                STAGE_HASH_OFFSETr_BANK2_HASH_OFFSETf_SET(sho[stage], bdx * 4);
            } else if (bank == 3) {
                STAGE_HASH_OFFSETr_BANK3_HASH_OFFSETf_SET(sho[stage], bdx * 4);
            } else if (bank == 4) {
                STAGE_HASH_OFFSETr_BANK4_HASH_OFFSETf_SET(sho[stage], bdx * 4);
            }
        }
    }
    /* Write hash offset registers */
    for (stage = 0; stage < ISM_MAX_STAGES; stage++) {
        ioerr += WRITE_STAGE_HASH_OFFSETr(unit, stage, sho[stage]);
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

static int
_firmware_helper(void *ctx, uint32_t offset, uint32_t size, void *data)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    PORT_WC_UCMEM_CTRLr_t ucmem_ctrl;
    PORT_WC_UCMEM_DATAm_t ucmem_data;
    int unit, port, inst, bcast;
    const char *drv_name;
    uint32_t wbuf[4];
    uint32_t wdata;
    uint32_t *fw_data;
    uint32_t *fw_entry;
    uint32_t fw_size;
    uint32_t speed;
    uint32_t idx, wdx;
    uint32_t le_host;

    /* Get unit, port and driver name from context */
    bmd_phy_fw_info_get(ctx, &unit, &port, &drv_name);

    /* Check if PHY driver requests optimized MDIO clock */
    if (data == NULL) {
        CMIC_RATE_ADJUSTr_t rate_adjust;
        uint32_t val = 1;

        /* Offset value is MDIO clock freq in kHz (or zero to restore) */
        if (offset) {
            val = offset / 9375;
        }
        ioerr += READ_CMIC_RATE_ADJUSTr(unit, &rate_adjust);
        CMIC_RATE_ADJUSTr_DIVIDENDf_SET(rate_adjust, val);
        ioerr += WRITE_CMIC_RATE_ADJUSTr(unit, rate_adjust);

        return ioerr ? CDK_E_IO : CDK_E_NONE;
    }

    if (CDK_STRSTR(drv_name, "warpcore") == NULL) {
        return CDK_E_UNAVAIL;
    }

    if (size == 0) {
        return CDK_E_INTERNAL;
    }

    /* Aligned firmware size */
    fw_size = (size + FW_ALIGN_MASK) & ~FW_ALIGN_MASK;

    /* Get XPORT instance within port block */
    inst = bcm56340_a0_xport_inst(unit, port);

    /* Check if broadcast can be used */
    bcast = 0;
    speed = bcm56340_a0_port_speed_max(unit, port);
    if (speed >= 100000) {
        bcast = 1;
    }

    /* Enable parallel bus access and select instance(s) */
    ioerr += READ_PORT_WC_UCMEM_CTRLr(unit, &ucmem_ctrl, port);
    PORT_WC_UCMEM_CTRLr_ACCESS_MODEf_SET(ucmem_ctrl, 1);
    PORT_WC_UCMEM_CTRLr_INST_SELECTf_SET(ucmem_ctrl, inst);
    PORT_WC_UCMEM_CTRLr_WR_BROADCASTf_SET(ucmem_ctrl, bcast);
    ioerr += WRITE_PORT_WC_UCMEM_CTRLr(unit, ucmem_ctrl, port);

    /* We need to byte swap on little endian host */
    le_host = 1;
    if (*((uint8_t *)&le_host) == 0) {
        le_host = 0;
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
            if (le_host) {
                wdata = cdk_util_swap32(wdata);
            }
            PORT_WC_UCMEM_DATAm_SET(ucmem_data, wdx^3, wdata);
        }
        WRITE_PORT_WC_UCMEM_DATAm(unit, idx >> 4, ucmem_data, port);
    }

    /* Disable parallel bus access */
    PORT_WC_UCMEM_CTRLr_ACCESS_MODEf_SET(ucmem_ctrl, 0);
    ioerr += WRITE_PORT_WC_UCMEM_CTRLr(unit, ucmem_ctrl, port);

    return ioerr ? CDK_E_IO : rv;
}

static int
_port_init(int unit, int port)
{
    int ioerr = 0;
    int lport = P2L(unit, port);
    EGR_VLAN_CONTROL_1r_t egr_vlan_ctrl1;
    PORT_TABm_t port_tab;
    EGR_PORTm_t egr_port;
    EGR_ENABLEm_t egr_enable;

    if (lport < 0) {
        /* Unconfigured flex port */
        return 0;
    }

    /* Default port VLAN and tag action, enable L2 HW learning */
    PORT_TABm_CLR(port_tab);
    PORT_TABm_PORT_VIDf_SET(port_tab, 1);
    PORT_TABm_FILTER_ENABLEf_SET(port_tab, 1);
    PORT_TABm_OUTER_TPID_ENABLEf_SET(port_tab, 1);
    PORT_TABm_CML_FLAGS_NEWf_SET(port_tab, 8);
    PORT_TABm_CML_FLAGS_MOVEf_SET(port_tab, 8);
    ioerr += WRITE_PORT_TABm(unit, lport, port_tab);

    /* Filter VLAN on egress */
    EGR_PORTm_CLR(egr_port);
    EGR_PORTm_EN_EFILTERf_SET(egr_port, 1);
    ioerr += WRITE_EGR_PORTm(unit, lport, egr_port);

    /* Configure egress VLAN for backward compatibility */
    ioerr += READ_EGR_VLAN_CONTROL_1r(unit, lport, &egr_vlan_ctrl1);
    EGR_VLAN_CONTROL_1r_VT_MISS_UNTAGf_SET(egr_vlan_ctrl1, 0);
    EGR_VLAN_CONTROL_1r_REMARK_OUTER_DOT1Pf_SET(egr_vlan_ctrl1, 1);
    ioerr += WRITE_EGR_VLAN_CONTROL_1r(unit, lport, egr_vlan_ctrl1);

    /* Egress enable */
    ioerr += READ_EGR_ENABLEm(unit, port, &egr_enable);
    EGR_ENABLEm_PRT_ENABLEf_SET(egr_enable, 1);
    ioerr += WRITE_EGR_ENABLEm(unit, port, egr_enable);

    return ioerr;
}

static int
_xlport_reset(int unit, int port)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    cdk_pbmp_t pbmp;
    uint32_t port_en_mask;
    PORT_MAC_CONTROLr_t mac_ctrl;
    EGR_PORT_BUFFER_SFT_RESETr_t egr_buf_reset;
    PORT_SOFT_RESETr_t port_soft_reset;
    PORT_ENABLE_REGr_t port_en;
    LLS_PORT_CREDITr_t lls_port_crecit;
    PORT_MODE_REGr_t port_mode;
    EGR_ENABLEm_t egr_enable;
    
    bcm56340_a0_xport_pbmp_get(unit, XPORT_FLAG_XLPORT, &pbmp);
    if (!CDK_PBMP_MEMBER(pbmp, port)) {
        return CDK_E_PORT;
    }
        
    /* Put the port in reset */
    ioerr += READ_PORT_MAC_CONTROLr(unit, &mac_ctrl, port);
    PORT_MAC_CONTROLr_XMAC0_RESETf_SET(mac_ctrl, 1);
    ioerr += WRITE_PORT_MAC_CONTROLr(unit, mac_ctrl, port);
        
    /* Reset egress buffers */
    ioerr += READ_EGR_PORT_BUFFER_SFT_RESETr(unit, &egr_buf_reset);
    EGR_PORT_BUFFER_SFT_RESETr_XLP0_RESETf_SET(egr_buf_reset, 1);
    ioerr += WRITE_EGR_PORT_BUFFER_SFT_RESETr(unit, egr_buf_reset);
    
    /* Do port soft reset */
    ioerr += READ_PORT_SOFT_RESETr(unit, &port_soft_reset, port);
    PORT_SOFT_RESETr_XPORT_CORE0f_SET(port_soft_reset, 1);
    ioerr += WRITE_PORT_SOFT_RESETr(unit, port_soft_reset, port);
    
    /* Disable port */
    ioerr += READ_PORT_ENABLE_REGr(unit, &port_en, port);
    port_en_mask = PORT_ENABLE_REGr_GET(port_en);
    port_en_mask &= ~0xf;
    PORT_ENABLE_REGr_SET(port_en, port_en_mask);
    ioerr += WRITE_PORT_ENABLE_REGr(unit, port_en, port);
    
    /* Clear ports MMU link list credit */
    LLS_PORT_CREDITr_CLR(lls_port_crecit);
    ioerr += WRITE_LLS_PORT_CREDITr(unit, P2M(unit, port), lls_port_crecit);
    
    /* Setup the port modes */
    ioerr += READ_PORT_MODE_REGr(unit, &port_mode, port);
    PORT_MODE_REGr_XPORT0_PHY_PORT_MODEf_SET(port_mode, PHY_MODE_QUAD);
    PORT_MODE_REGr_XPORT0_CORE_PORT_MODEf_SET(port_mode, CORE_MODE_QUAD);
    PORT_MODE_REGr_XPC0_GMII_MII_ENABLEf_SET(port_mode, 0);
    ioerr += WRITE_PORT_MODE_REGr(unit, port_mode, port);
    
    /* Configure multicast FIFO */
    rv = _fifo_init(unit);
    
    /* Port reinit */
    ioerr += READ_PORT_MAC_CONTROLr(unit, &mac_ctrl, port);
    PORT_MAC_CONTROLr_XMAC0_RESETf_SET(mac_ctrl, 0);
    ioerr += WRITE_PORT_MAC_CONTROLr(unit, mac_ctrl, port);
    
    /* Release port reset */
    ioerr += READ_PORT_SOFT_RESETr(unit, &port_soft_reset, port);
    PORT_SOFT_RESETr_XPORT_CORE0f_SET(port_soft_reset, 0);
    ioerr += WRITE_PORT_SOFT_RESETr(unit, port_soft_reset, port);
    
    /* Release egress buffer reset */
    ioerr += READ_EGR_PORT_BUFFER_SFT_RESETr(unit, &egr_buf_reset);
    EGR_PORT_BUFFER_SFT_RESETr_XLP0_RESETf_SET(egr_buf_reset, 0);
    ioerr += WRITE_EGR_PORT_BUFFER_SFT_RESETr(unit, egr_buf_reset);
    
    /* Enable new subports */
    ioerr += READ_PORT_ENABLE_REGr(unit, &port_en, port);
    port_en_mask = PORT_ENABLE_REGr_GET(port_en);
    port_en_mask |= 0xf;
    PORT_ENABLE_REGr_SET(port_en, port_en_mask);
    ioerr += WRITE_PORT_ENABLE_REGr(unit, port_en, port);
    
    /* Enable egress cell request generation */
    ioerr += READ_EGR_ENABLEm(unit, port, &egr_enable);
    EGR_ENABLEm_PRT_ENABLEf_SET(egr_enable, 1);
    ioerr += WRITE_EGR_ENABLEm(unit, port, egr_enable);
    
    return ioerr ? CDK_E_IO : rv;
}

static int
_xlport_xmac_fifo_check(int unit, int port, int *txfifo_flag)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
#if BMD_CONFIG_INCLUDE_DMA == 1
    bmd_pkt_t *pkt;
    XMAC_MACSEC_CTRLr_t macsec_ctrl;
    PORT_TXFIFO_CELL_REQ_CNTr_t cell_req_cnt;
    PORT_TXFIFO_CELL_CNTr_t cell_cnt;
    PORT_MIB_RESETr_t mib_reset;
    XMAC_CTRLr_t xmac_ctrl;
    XMAC_MODEr_t xmac_mode;
    int bdx = 0;
    int data_len = 9200;
    uint8_t eth_hdr[ETH_HDR_LEN] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 
                                    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 
                                    0x81, 0x00, 0x00, 0x01, 0x23, 0xF0}; 
    
    CDK_ASSERT(txfifo_flag);
    
    pkt = &test_pkt;
    pkt->flags = BMD_PKT_F_CRC_REGEN;
    pkt->size = data_len + ETH_HDR_LEN + ETH_CRC_LEN;
    pkt->port = port;
    pkt->data = bmd_dma_alloc_coherent(unit, pkt->size, &pkt->baddr);
    CDK_ASSERT(pkt->data);

    CDK_MEMCPY(&pkt->data[bdx], eth_hdr, ETH_HDR_LEN); 
    bdx += ETH_HDR_LEN;

    for (; bdx < (pkt->size - ETH_CRC_LEN); bdx++) {
        pkt->data[bdx] = bdx ^ 0xff;
    }
    for (; bdx < pkt->size; bdx++) {
        pkt->data[bdx] = 0;
    }

    BMD_DMA_CACHE_FLUSH(pkt->data, pkt->size);

    if (CDK_SUCCESS(rv)) {
        rv = bcm56340_a0_bmd_port_mode_set(unit, port, bmdPortMode1000fd, 
                                                BMD_PORT_MODE_F_MAC_LOOPBACK);
    }
    
    ioerr += READ_XMAC_CTRLr(unit, port, &xmac_ctrl);
    XMAC_CTRLr_TX_ENf_SET(xmac_ctrl, 1);
    XMAC_CTRLr_RX_ENf_SET(xmac_ctrl, 0);
    XMAC_CTRLr_LINE_LOCAL_LPBKf_SET(xmac_ctrl, 1);
    XMAC_CTRLr_CORE_LOCAL_LPBKf_SET(xmac_ctrl, 0);
    ioerr += WRITE_XMAC_CTRLr(unit, port, xmac_ctrl);
    BMD_SYS_USLEEP(10);

    ioerr += READ_XMAC_MODEr(unit, port, &xmac_mode);
    XMAC_MODEr_SPEED_MODEf_SET(xmac_mode, 4);
    ioerr += WRITE_XMAC_MODEr(unit, port, xmac_mode);

    ioerr += READ_XMAC_MACSEC_CTRLr(unit, port, &macsec_ctrl);
    XMAC_MACSEC_CTRLr_MACSEC_TX_LAUNCH_ENf_SET(macsec_ctrl, 1);
    ioerr += WRITE_XMAC_MACSEC_CTRLr(unit, port, macsec_ctrl);

    /* Transmit the test packet */
    if (CDK_SUCCESS(rv)) {
        rv = bmd_tx(unit, pkt);
    }

    /* 
     * Wait for a short period of time to allow the packet 
     * to reach the end of the EP 
     */
    BMD_SYS_USLEEP(1000);

    /* Read the request count value */
    READ_PORT_TXFIFO_CELL_CNTr(unit, port, &cell_cnt);
    READ_PORT_TXFIFO_CELL_REQ_CNTr(unit, port, &cell_req_cnt);
    
    *txfifo_flag = 0;
    if (PORT_TXFIFO_CELL_REQ_CNTr_GET(cell_req_cnt)) {
        *txfifo_flag = 1;
    }

    ioerr += READ_XMAC_MODEr(unit, port, &xmac_mode);
    XMAC_MODEr_SPEED_MODEf_SET(xmac_mode, 2);
    ioerr += WRITE_XMAC_MODEr(unit, port, xmac_mode);

    ioerr += READ_XMAC_MACSEC_CTRLr(unit, port, &macsec_ctrl);
    XMAC_MACSEC_CTRLr_MACSEC_TX_LAUNCH_ENf_SET(macsec_ctrl, 0);
    ioerr += WRITE_XMAC_MACSEC_CTRLr(unit, port, macsec_ctrl);

    if (CDK_SUCCESS(rv)) {
        rv = bcm56340_a0_bmd_port_mode_set(unit, port, bmdPortModeDisabled, 0);
    }

    /* Reset MIB counters */
    PORT_MIB_RESETr_CLR(mib_reset);
    PORT_MIB_RESETr_CLR_CNTf_SET(mib_reset, 0xfff);
    ioerr += WRITE_PORT_MIB_RESETr(unit, mib_reset, port);
    PORT_MIB_RESETr_CLR_CNTf_SET(mib_reset, 0);
    ioerr += WRITE_PORT_MIB_RESETr(unit, mib_reset, port);

    bmd_dma_free_coherent(unit, pkt->size, pkt->data, pkt->baddr);

#endif /* BMD_CONFIG_INCLUDE_DMA */
    return ioerr ? CDK_E_IO : rv;
}

static int
_xlport_txfifo_check(int unit)
{
    int rv = CDK_E_NONE;
    int retry, port;
    int fifo_underrun, issue_overcome, txfifo_flag = 0;
    cdk_pbmp_t pbmp;
    PORT_TXFIFO_CELL_REQ_CNTr_t cell_req_cnt;
    PORT_TXFIFO_CELL_CNTr_t cell_cnt;
    
    bcm56340_a0_xport_pbmp_get(unit, XPORT_FLAG_XLPORT, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        retry = 10;
        fifo_underrun = FALSE;
        issue_overcome = FALSE;
        
        while (retry--) {
            rv = _xlport_xmac_fifo_check(unit, port, &txfifo_flag);
            if (CDK_FAILURE(rv)) {
                break ;
            }
            
            if (txfifo_flag) { /* FIFO underrun happened */
                fifo_underrun = TRUE;
                
                /* Reset the XLPORTs */
                rv = _xlport_reset(unit, port);
                if (CDK_FAILURE(rv)) {
                    break;
                }
     
                /* Initialize XLPORTs after the port is out of reset */
                if (bcm56340_a0_xport_init(unit, port) != 0) {
                    rv = CDK_E_IO;
                    break;
                }
       
                READ_PORT_TXFIFO_CELL_REQ_CNTr(unit, port, &cell_req_cnt);
                READ_PORT_TXFIFO_CELL_CNTr(unit, port, &cell_cnt);
                if ((PORT_TXFIFO_CELL_REQ_CNTr_GET(cell_req_cnt) != 4) ||
                    (PORT_TXFIFO_CELL_CNTr_GET(cell_cnt) != 0)) {
                    rv = CDK_E_FAIL;
                    break;
                }
            } else {
                if (fifo_underrun) {
                    issue_overcome = TRUE;
                }
                break;
            } 
        }

        if (CDK_SUCCESS(rv)) {
            if (fifo_underrun) {
                if (issue_overcome) {
                    CDK_WARN(("bcm56340_a0_bmd_init[%d]: port %d, " 
                        "recovered from XLPORT underrun\n", unit, port));
                } else {
                    CDK_WARN(("bcm56340_a0_bmd_init[%d]: port %d, " 
                        "did not recover from XLPORT underrun\n", unit, port));
                }
            }
        } else {
            CDK_WARN(("bcm56340_a0_bmd_init[%d]: port %d, "
                "failed while recovering XLPORT underrun\n", unit, port));
        }
    }
    
    return rv;
}


int
bcm56340_a0_xmac_reset_set(int unit, int port, int reset)
{
    int ioerr = 0;
    PORT_MAC_CONTROLr_t port_mac_ctrl;
    int inst;

    inst = bcm56340_a0_xport_inst(unit, port);

    ioerr += READ_PORT_MAC_CONTROLr(unit, &port_mac_ctrl, port);
    if (inst == 2) {
        PORT_MAC_CONTROLr_XMAC2_RESETf_SET(port_mac_ctrl, reset);
    } else if (inst == 1) {
        PORT_MAC_CONTROLr_XMAC1_RESETf_SET(port_mac_ctrl, reset);
    } else {
        PORT_MAC_CONTROLr_XMAC0_RESETf_SET(port_mac_ctrl, reset);
    }
    ioerr += WRITE_PORT_MAC_CONTROLr(unit, port_mac_ctrl, port);

    return ioerr;
}

int
bcm56340_a0_xport_init(int unit, int port)
{
    int ioerr = 0;
    XMAC_TX_CTRLr_t txctrl;
    XMAC_RX_CTRLr_t rxctrl;
    XMAC_RX_MAX_SIZEr_t rxmaxsz;
    XMAC_CTRLr_t mac_ctrl;
    PORT_CNTMAXSIZEr_t cntmaxsz;

    /* Common port initialization */
    ioerr += _port_init(unit, port);

    /* Ensure that MAC (Rx) and loopback mode is disabled */
    XMAC_CTRLr_CLR(mac_ctrl);
    XMAC_CTRLr_SOFT_RESETf_SET(mac_ctrl, 1);
    ioerr += WRITE_XMAC_CTRLr(unit, port, mac_ctrl);

    XMAC_CTRLr_TX_ENf_SET(mac_ctrl, 1);
    if (bcm56340_a0_port_speed_max(unit, port) == 40000) {
        XMAC_CTRLr_XLGMII_ALIGN_ENBf_SET(mac_ctrl, 1);
    }
    ioerr += WRITE_XMAC_CTRLr(unit, port, mac_ctrl);

    /* Configure Tx (Inter-Packet-Gap, recompute CRC mode, IEEE header) */
    XMAC_TX_CTRLr_CLR(txctrl);
    XMAC_TX_CTRLr_TX_PREAMBLE_LENGTHf_SET(txctrl, 8);
    XMAC_TX_CTRLr_PAD_THRESHOLDf_SET(txctrl, 0x40);
    XMAC_TX_CTRLr_AVERAGE_IPGf_SET(txctrl, 0xc);
    XMAC_TX_CTRLr_CRC_MODEf_SET(txctrl, 0x3);
    ioerr += WRITE_XMAC_TX_CTRLr(unit, port, txctrl);

    /* Configure Rx (strip CRC, strict preamble, IEEE header) */
    XMAC_RX_CTRLr_CLR(rxctrl);
    XMAC_RX_CTRLr_STRICT_PREAMBLEf_SET(rxctrl, 1);
    if (bcm56340_a0_port_speed_max(unit, port) == 1000) {
        XMAC_RX_CTRLr_STRICT_PREAMBLEf_SET(rxctrl, 0);
        XMAC_RX_CTRLr_PROCESS_VARIABLE_PREAMBLEf_SET(rxctrl, 1);
    }
    XMAC_RX_CTRLr_RUNT_THRESHOLDf_SET(rxctrl, 0x40);
    ioerr += WRITE_XMAC_RX_CTRLr(unit, port, rxctrl);

    /* Set max Rx frame size */
    XMAC_RX_MAX_SIZEr_CLR(rxmaxsz);
    XMAC_RX_MAX_SIZEr_RX_MAX_SIZEf_SET(rxmaxsz, JUMBO_MAXSZ);
    ioerr += WRITE_XMAC_RX_MAX_SIZEr(unit, port, rxmaxsz);

    /* Set max MIB frame size */
    PORT_CNTMAXSIZEr_SET(cntmaxsz, JUMBO_MAXSZ);
    ioerr += WRITE_PORT_CNTMAXSIZEr(unit, port, cntmaxsz);

    return ioerr;
}

uint32_t
bcm56340_a0_port_num_lanes(int unit, int port)
{
    uint32_t speed, lanes = 0;
    
    speed = bcm56340_a0_port_speed_max(unit, port);
    port = ((port - 1) & ~0x3) + 1;

    if (speed >= 40000) {
        lanes = 4;
    } else if ((speed >= 21000) && (speed < 40000)) {
        if (P2L(unit, (port + 2)) >= 0) {
            lanes = 2;
        } else {
            lanes = 4;
        }
    } else if ((speed >= 10000) && (speed < 21000)) {
        lanes = 1;
    } else if (speed > 0) {
        lanes = 1;
    }
    
    return lanes;
}

int
bcm56340_a0_bmd_init(int unit)
{
    int ioerr = 0;
    int rv;
    ING_HW_RESET_CONTROL_1r_t ing_rst_ctl_1;
    ING_HW_RESET_CONTROL_2r_t ing_rst_ctl_2;
    EGR_HW_RESET_CONTROL_0r_t egr_rst_ctl_0;
    EGR_HW_RESET_CONTROL_1r_t egr_rst_ctl_1;
    ISM_HW_RESET_CONTROL_0r_t ism_rst_ctl_0;
    ISM_HW_RESET_CONTROL_1r_t ism_rst_ctl_1;
    AXP_WRX_MEMORY_BULK_RESETr_t axp_wrx_rst;
    AXP_WTX_MEMORY_BULK_RESETr_t axp_wtx_rst;
    AXP_SM_MEMORY_BULK_RESETr_t axp_sm_rst;
    TOP_XG_PLL_CTRL_1r_t xg_pll_ctrl;
    CPU_PBMm_t cpu_pbm;
    CPU_PBM_2m_t cpu_pbm_2;
    PORT_MODE_REGr_t port_mode;
    PORT_ENABLE_REGr_t port_en;
    PORT_MAC_CONTROLr_t port_mac_ctrl;
    PORT_MIB_RESETr_t mib_reset;
    MISCCONFIGr_t misc_cfg;
    ING_EN_EFILTER_BITMAPm_t ing_en_efilter;
    CMIC_RATE_ADJUSTr_t rate_adjust;
    CMIC_RATE_ADJUST_INT_MDIOr_t rate_adjust_int_mdio;
    RDBGC_SELECTr_t rdbgc_select;
    VLAN_PROFILE_TABm_t vlan_profile;
    ING_VLAN_TAG_ACTION_PROFILEm_t vlan_action;
    EGR_VLAN_TAG_ACTION_PROFILEm_t egr_action;
    cdk_pbmp_t pbmp, xlpbmp;
    uint32_t port_en_mask, lane_en, lanes;
    int mac_mode = MAC_MODE_INDEP, phy_mode, core_mode;
    int port;
    int idx, inst;
    int stop_clk_sync;

    BMD_CHECK_UNIT(unit);

    /* Reset the IPIPE block */
    ING_HW_RESET_CONTROL_1r_CLR(ing_rst_ctl_1);
    ioerr += WRITE_ING_HW_RESET_CONTROL_1r(unit, ing_rst_ctl_1);
    ING_HW_RESET_CONTROL_2r_CLR(ing_rst_ctl_2);
    ING_HW_RESET_CONTROL_2r_RESET_ALLf_SET(ing_rst_ctl_2, 1);
    ING_HW_RESET_CONTROL_2r_VALIDf_SET(ing_rst_ctl_2, 1);
    ING_HW_RESET_CONTROL_2r_COUNTf_SET(ing_rst_ctl_2, 0x10000);
    ioerr += WRITE_ING_HW_RESET_CONTROL_2r(unit, ing_rst_ctl_2);

    /* Reset the EPIPE block */
    EGR_HW_RESET_CONTROL_0r_CLR(egr_rst_ctl_0);
    ioerr += WRITE_EGR_HW_RESET_CONTROL_0r(unit, egr_rst_ctl_0);
    EGR_HW_RESET_CONTROL_1r_CLR(egr_rst_ctl_1);
    EGR_HW_RESET_CONTROL_1r_RESET_ALLf_SET(egr_rst_ctl_1, 1);
    EGR_HW_RESET_CONTROL_1r_VALIDf_SET(egr_rst_ctl_1, 1);
    EGR_HW_RESET_CONTROL_1r_COUNTf_SET(egr_rst_ctl_1, 0x10000);
    ioerr += WRITE_EGR_HW_RESET_CONTROL_1r(unit, egr_rst_ctl_1);

    /* Reset the ISM block */
    ISM_HW_RESET_CONTROL_0r_CLR(ism_rst_ctl_0);
    ioerr += WRITE_ISM_HW_RESET_CONTROL_0r(unit, ism_rst_ctl_0);
    ISM_HW_RESET_CONTROL_1r_CLR(ism_rst_ctl_1);
    ISM_HW_RESET_CONTROL_1r_RESET_ALLf_SET(ism_rst_ctl_1, 1);
    ISM_HW_RESET_CONTROL_1r_VALIDf_SET(ism_rst_ctl_1, 1);
    ISM_HW_RESET_CONTROL_1r_COUNTf_SET(ism_rst_ctl_1, 0x20000);
    ioerr += WRITE_ISM_HW_RESET_CONTROL_1r(unit, ism_rst_ctl_1);

    /* Clear AXP block memories */
    AXP_WRX_MEMORY_BULK_RESETr_CLR(axp_wrx_rst);
    AXP_WRX_MEMORY_BULK_RESETr_START_RESETf_SET(axp_wrx_rst, 1);
    ioerr += WRITE_AXP_WRX_MEMORY_BULK_RESETr(unit, axp_wrx_rst);
    AXP_WTX_MEMORY_BULK_RESETr_CLR(axp_wtx_rst);
    AXP_WTX_MEMORY_BULK_RESETr_START_RESETf_SET(axp_wtx_rst, 1);
    ioerr += WRITE_AXP_WTX_MEMORY_BULK_RESETr(unit, axp_wtx_rst);
    AXP_SM_MEMORY_BULK_RESETr_CLR(axp_sm_rst);
    AXP_SM_MEMORY_BULK_RESETr_START_RESETf_SET(axp_sm_rst, 1);
    ioerr += WRITE_AXP_SM_MEMORY_BULK_RESETr(unit, axp_sm_rst);

    for (idx = 0; idx < PIPE_RESET_TIMEOUT_MSEC; idx++) {
        ioerr += READ_ING_HW_RESET_CONTROL_2r(unit, &ing_rst_ctl_2);
        if (ING_HW_RESET_CONTROL_2r_DONEf_GET(ing_rst_ctl_2)) {
            break;
        }
        BMD_SYS_USLEEP(1000);
    }
    if (idx >= PIPE_RESET_TIMEOUT_MSEC) {
        CDK_WARN(("bcm56340_a0_bmd_init[%d]: IPIPE reset timeout\n", unit));
        return ioerr ? CDK_E_IO : CDK_E_TIMEOUT;
    }
        
    for (; idx < PIPE_RESET_TIMEOUT_MSEC; idx++) {
        ioerr += READ_EGR_HW_RESET_CONTROL_1r(unit, &egr_rst_ctl_1);
        if (EGR_HW_RESET_CONTROL_1r_DONEf_GET(egr_rst_ctl_1)) {
            break;
        }
        BMD_SYS_USLEEP(1000);
    }
    if (idx >= PIPE_RESET_TIMEOUT_MSEC) {
        CDK_WARN(("bcm56340_a0_bmd_init[%d]: EPIPE reset timeout\n", unit));
        return ioerr ? CDK_E_IO : CDK_E_TIMEOUT;
    }

    for (; idx < PIPE_RESET_TIMEOUT_MSEC; idx++) {
        ioerr += READ_ISM_HW_RESET_CONTROL_1r(unit, &ism_rst_ctl_1);
        if (ISM_HW_RESET_CONTROL_1r_DONEf_GET(ism_rst_ctl_1)) {
            break;
        }
        BMD_SYS_USLEEP(1000);
    }
    if (idx >= PIPE_RESET_TIMEOUT_MSEC) {
        CDK_WARN(("bcm56340_a0_bmd_init[%d]: ISM reset timeout\n", unit));
        return ioerr ? CDK_E_IO : CDK_E_TIMEOUT;
    }

    for (; idx < PIPE_RESET_TIMEOUT_MSEC; idx++) {
        ioerr += READ_AXP_WRX_MEMORY_BULK_RESETr(unit, &axp_wrx_rst);
        if (AXP_WRX_MEMORY_BULK_RESETr_RESET_DONEf_GET(axp_wrx_rst)) {
            break;
        }
        BMD_SYS_USLEEP(1000);
    }
    if (idx >= PIPE_RESET_TIMEOUT_MSEC) {
        CDK_WARN(("bcm56340_a0_bmd_init[%d]: AXP WRX reset timeout\n", unit));
        return ioerr ? CDK_E_IO : CDK_E_TIMEOUT;
    }

    for (; idx < PIPE_RESET_TIMEOUT_MSEC; idx++) {
        ioerr += READ_AXP_WTX_MEMORY_BULK_RESETr(unit, &axp_wtx_rst);
        if (AXP_WTX_MEMORY_BULK_RESETr_RESET_DONEf_GET(axp_wtx_rst)) {
            break;
        }
        BMD_SYS_USLEEP(1000);
    }
    if (idx >= PIPE_RESET_TIMEOUT_MSEC) {
        CDK_WARN(("bcm56340_a0_bmd_init[%d]: AXP WTX reset timeout\n", unit));
        return ioerr ? CDK_E_IO : CDK_E_TIMEOUT;
    }

    for (; idx < PIPE_RESET_TIMEOUT_MSEC; idx++) {
        if (CDK_XGSM_FLAGS(unit) & CHIP_FLAG_NO_DPI) {
            /* DPI-SM memories disabled */
            break;
        }
        ioerr += READ_AXP_SM_MEMORY_BULK_RESETr(unit, &axp_sm_rst);
        if (AXP_SM_MEMORY_BULK_RESETr_RESET_DONEf_GET(axp_sm_rst)) {
            break;
        }
        BMD_SYS_USLEEP(1000);
    }
    if (idx >= PIPE_RESET_TIMEOUT_MSEC) {
        CDK_WARN(("bcm56340_a0_bmd_init[%d]: AXP SM reset timeout\n", unit));
        return ioerr ? CDK_E_IO : CDK_E_TIMEOUT;
    }

    /* Clear pipe reset registers */
    ING_HW_RESET_CONTROL_2r_CLR(ing_rst_ctl_2);
    ioerr += WRITE_ING_HW_RESET_CONTROL_2r(unit, ing_rst_ctl_2);
    EGR_HW_RESET_CONTROL_1r_CLR(egr_rst_ctl_1);
    ioerr += WRITE_EGR_HW_RESET_CONTROL_1r(unit, egr_rst_ctl_1);
    ISM_HW_RESET_CONTROL_1r_CLR(ism_rst_ctl_1);
    ioerr += WRITE_ISM_HW_RESET_CONTROL_1r(unit, ism_rst_ctl_1);
    AXP_WRX_MEMORY_BULK_RESETr_CLR(axp_wrx_rst);
    ioerr += WRITE_AXP_WRX_MEMORY_BULK_RESETr(unit, axp_wrx_rst);
    AXP_WTX_MEMORY_BULK_RESETr_CLR(axp_wtx_rst);
    ioerr += WRITE_AXP_WTX_MEMORY_BULK_RESETr(unit, axp_wtx_rst);
    AXP_SM_MEMORY_BULK_RESETr_CLR(axp_sm_rst);
    ioerr += WRITE_AXP_SM_MEMORY_BULK_RESETr(unit, axp_sm_rst);

    /* Initialize port mappings */
    ioerr += _port_map_init(unit);

    /* Configure CPU port */
    CPU_PBMm_CLR(cpu_pbm);
    CPU_PBMm_BITMAP_W0f_SET(cpu_pbm, 1);
    ioerr += WRITE_CPU_PBMm(unit, 0, cpu_pbm);
    CPU_PBM_2m_CLR(cpu_pbm_2);
    CPU_PBM_2m_BITMAP_W0f_SET(cpu_pbm_2, 1);
    ioerr += WRITE_CPU_PBM_2m(unit, 0, cpu_pbm_2);

    /* Initialize XLPORTs */
    bcm56340_a0_xport_pbmp_get(unit, XPORT_FLAG_XLPORT, &xlpbmp);
    bcm56340_a0_xport_pbmp_get(unit, XPORT_FLAG_ALL, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        if (XPORT_SUBPORT(port) == 0) {
            inst = bcm56340_a0_xport_inst(unit, port);
            lanes = bcm56340_a0_port_num_lanes(unit, port);
            lane_en = 0;
            phy_mode = PHY_MODE_SINGLE;
            
            if (lanes == 4) {
                core_mode = CORE_MODE_SINGLE;
                phy_mode = PHY_MODE_SINGLE;
                lane_en = 0x1;
            } else if (lanes == 2) {
                core_mode = CORE_MODE_DUAL;
                phy_mode = PHY_MODE_DUAL;
                lane_en = 0x5;
            } else if (lanes == 1) {
                core_mode = CORE_MODE_QUAD;
                phy_mode = PHY_MODE_QUAD;
                lane_en = 0xf;
                if (port == 49) {
                    lane_en = 0x1;
                }
            } else {
                core_mode = CORE_MODE_NOTDM;
            }
            CDK_VVERB(("Port %d: Core Mode:%d, PHY Mode:%d\n", port, core_mode, phy_mode));

            /* Set port mode */
            ioerr += READ_PORT_MODE_REGr(unit, &port_mode, port);
            if (inst == 2) {
                PORT_MODE_REGr_XPORT2_PHY_PORT_MODEf_SET(port_mode, phy_mode);
                PORT_MODE_REGr_XPORT2_CORE_PORT_MODEf_SET(port_mode, core_mode);
                PORT_MODE_REGr_XPC2_GMII_MII_ENABLEf_SET(port_mode, 0);
            } else if (inst == 1) {
                PORT_MODE_REGr_XPORT1_PHY_PORT_MODEf_SET(port_mode, phy_mode);
                PORT_MODE_REGr_XPORT1_CORE_PORT_MODEf_SET(port_mode, core_mode);
                PORT_MODE_REGr_XPC1_GMII_MII_ENABLEf_SET(port_mode, 0);
            } else {
                PORT_MODE_REGr_XPORT0_PHY_PORT_MODEf_SET(port_mode, phy_mode);
                PORT_MODE_REGr_XPORT0_CORE_PORT_MODEf_SET(port_mode, core_mode);
                PORT_MODE_REGr_XPC0_GMII_MII_ENABLEf_SET(port_mode, 0);
            }
            PORT_MODE_REGr_MAC_MODEf_SET(port_mode, mac_mode);
            /* Keep port in reset while updating port mode */
            bcm56340_a0_xmac_reset_set(unit, port, 1);
            ioerr += WRITE_PORT_MODE_REGr(unit, port_mode, port);
            bcm56340_a0_xmac_reset_set(unit, port, 0);

            stop_clk_sync = 1;
            if (CDK_PBMP_MEMBER(xlpbmp, port)) {
                stop_clk_sync = 0;
            }
            
            if (stop_clk_sync) {
                /* Stop the clock sync */
                ioerr += READ_TOP_XG_PLL_CTRL_1r(unit, 0, &xg_pll_ctrl);
                TOP_XG_PLL_CTRL_1r_HOLD_CHf_SET(xg_pll_ctrl, 0x10);
                ioerr += WRITE_TOP_XG_PLL_CTRL_1r(unit, 0, xg_pll_ctrl);
            }
            
            /* Set port enable for 4 lanes */
            ioerr += READ_PORT_ENABLE_REGr(unit, &port_en, port);
            port_en_mask = PORT_ENABLE_REGr_GET(port_en);
            port_en_mask &= ~(0xf << (inst << 2));
            port_en_mask |= (lane_en << (inst << 2));
            PORT_ENABLE_REGr_SET(port_en, port_en_mask);
            ioerr += WRITE_PORT_ENABLE_REGr(unit, port_en, port);
            
            if (stop_clk_sync) {
                /* Start the clock sync */
                ioerr += READ_TOP_XG_PLL_CTRL_1r(unit, 0, &xg_pll_ctrl);
                TOP_XG_PLL_CTRL_1r_HOLD_CHf_SET(xg_pll_ctrl, 0);
                ioerr += WRITE_TOP_XG_PLL_CTRL_1r(unit, 0, xg_pll_ctrl);
            }
        }
    }

    /* Reset MIB counters in all blocks */
    CDK_PBMP_ITER(pbmp, port) {
        PORT_MIB_RESETr_CLR(mib_reset);
        PORT_MIB_RESETr_CLR_CNTf_SET(mib_reset, 0xfff);
        ioerr += WRITE_PORT_MIB_RESETr(unit, mib_reset, port);
        PORT_MIB_RESETr_CLR_CNTf_SET(mib_reset, 0);
        ioerr += WRITE_PORT_MIB_RESETr(unit, mib_reset, port);
    }

    /* Enable Field Processor metering clock */
    ioerr += READ_MISCCONFIGr(unit, &misc_cfg);
    MISCCONFIGr_METERING_CLK_ENf_SET(misc_cfg, 1);
    ioerr += WRITE_MISCCONFIGr(unit, misc_cfg);

    /* Ensure that link bitmap is cleared */
    ioerr += CDK_XGSM_MEM_CLEAR(unit, EPC_LINK_BMAPm);

    /* Enable egress VLAN checks for all ports */
    bcm56340_a0_xport_pbmp_get(unit, XPORT_FLAG_ALL, &pbmp);
    CDK_PBMP_PORT_ADD(pbmp, CMIC_PORT);
    ING_EN_EFILTER_BITMAPm_CLR(ing_en_efilter);
    ING_EN_EFILTER_BITMAPm_BITMAP_W0f_SET(ing_en_efilter,
                                          CDK_PBMP_WORD_GET(pbmp, 0));
    ING_EN_EFILTER_BITMAPm_BITMAP_W1f_SET(ing_en_efilter,
                                          CDK_PBMP_WORD_GET(pbmp, 1));
    ioerr += WRITE_ING_EN_EFILTER_BITMAPm(unit, 0, ing_en_efilter);

    /*
     * Set external MDIO freq to
     * 9.38MHz (450MHz), 8.65MHz (415MHz), or 6.56MHz (315 MHz)
     * target_freq = core_clock_freq * DIVIDEND / DIVISOR / 2
     */
    CMIC_RATE_ADJUSTr_CLR(rate_adjust);
    CMIC_RATE_ADJUSTr_DIVISORf_SET(rate_adjust, 24);
    CMIC_RATE_ADJUSTr_DIVIDENDf_SET(rate_adjust, 1);
    ioerr += WRITE_CMIC_RATE_ADJUSTr(unit, rate_adjust);

    /*
     * Set internal MDIO freq to
     * 37.5MHz (450MHz), 34.58MHz (415MHz), or 26.25MHz (315 MHz)
     * Valid range is from 2.5MHz to 40MHz
     * target_freq = core_clock_freq * DIVIDEND / DIVISOR / 2
     */
    CMIC_RATE_ADJUST_INT_MDIOr_CLR(rate_adjust_int_mdio);
    CMIC_RATE_ADJUST_INT_MDIOr_DIVISORf_SET(rate_adjust_int_mdio, 18);
    CMIC_RATE_ADJUST_INT_MDIOr_DIVIDENDf_SET(rate_adjust_int_mdio, 1);
    ioerr += WRITE_CMIC_RATE_ADJUST_INT_MDIOr(unit, rate_adjust_int_mdio);

    /* Configure discard counter */
    RDBGC_SELECTr_CLR(rdbgc_select);
    RDBGC_SELECTr_BITMAPf_SET(rdbgc_select, 0x0400ad11);
    ioerr += WRITE_RDBGC_SELECTr(unit, 0, rdbgc_select);

    /* Initialize MMU */
    rv = _mmu_init(unit);
    if (CDK_FAILURE(rv)) {
        return rv;
    }

    /* Initialize ISM-based hash tables */
    rv = _ism_init(unit);
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

    /* Probe PHYs */
    bcm56340_a0_xport_pbmp_get(unit, (XPORT_FLAG_XLPORT | 
                                      XPORT_FLAG_XTPORT), &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        if (CDK_SUCCESS(rv)) {
            rv = bmd_phy_probe(unit, port);
        }
    }

    bcm56340_a0_xport_pbmp_get(unit, XPORT_FLAG_XWPORT, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        if (CDK_SUCCESS(rv)) {
            rv = bmd_phy_probe(unit, port);
        }
        if (CDK_SUCCESS(rv)) {
            lanes = bcm56340_a0_port_num_lanes(unit, port);
            if (lanes == 1) {
                rv = bmd_phy_mode_set(unit, port, "warpcore",
                                      BMD_PHY_MODE_SERDES, 1);
            } else if (lanes == 2) {
                rv = bmd_phy_mode_set(unit, port, "warpcore",
                                      BMD_PHY_MODE_2LANE, 1);
            }
        }
        if (CDK_SUCCESS(rv)) {
            rv = bmd_phy_fw_helper_set(unit, port, _firmware_helper);
        }
    }

    /* Configure XPORTs */
    bcm56340_a0_xport_pbmp_get(unit, XPORT_FLAG_ALL, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        ioerr += bmd_phy_init(unit, port);
        /* Clear MAC hard reset after warpcore is initialized */
        if (XPORT_SUBPORT(port) == 0) {
            PORT_MAC_CONTROLr_CLR(port_mac_ctrl);
            ioerr += WRITE_PORT_MAC_CONTROLr(unit, port_mac_ctrl, port);
        }
        /* Initialize XLPORTs after XMAC is out of reset */
        ioerr += bcm56340_a0_xport_init(unit, port);
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
        for (idx = 0; idx < CMIC_NUM_PKT_DMA_CHAN; idx++) {
            cos_bmp = (idx == XGSM_DMA_RX_CHAN) ? 0xffffffff : 0;
            CMIC_CMC_COS_CTRL_RX_0r_COS_BMPf_SET(cos_ctrl_0, cos_bmp);
            ioerr += WRITE_CMIC_CMC_COS_CTRL_RX_0r(unit, idx, cos_ctrl_0);
        }

        CMIC_CMC_COS_CTRL_RX_1r_CLR(cos_ctrl_1);
        for (idx = 0; idx < CMIC_NUM_PKT_DMA_CHAN; idx++) {
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

        if (ioerr) {
            return CDK_E_IO;
        }
    }
#endif

    /* XLPORT XMAC FIFO underrun handling */
    if (CDK_SUCCESS(rv)) {
        rv = _xlport_txfifo_check(unit);
    }
    
    return ioerr ? CDK_E_IO : rv;
}
#endif /* CDK_CONFIG_INCLUDE_BCM56340_A0 */

