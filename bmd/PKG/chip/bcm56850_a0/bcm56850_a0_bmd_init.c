#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56850_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>
#include <bmd/bmd_device.h>
#include <bmdi/arch/xgsm_dma.h>
#include <cdk/chip/bcm56850_a0_defs.h>
#include <cdk/arch/xgsm_chip.h>
#include <cdk/cdk_util.h>
#include <cdk/cdk_debug.h>

#include "bcm56850_a0_bmd.h"
#include "bcm56850_a0_internal.h"

#define PIPE_RESET_TIMEOUT_MSEC             5
#define JUMBO_MAXSZ                         0x3fe8

#define MMU_MAX_PKT_BYTES                   10240 /* bytes */
#define MMU_PKT_HDR_BYTES                   64    /* bytes */
#define MMU_JUMBO_FRAME_BYTES               9216  /* bytes */
#define MMU_DEFAULT_MTU_BYTES               1536  /* bytes */

#define MMU_TOTAL_CELLS_60K                 61440 /* 60K cells */
#define MMU_TOTAL_CELLS_64K                 65536 /* 64K cells */
#define MMU_TOTAL_CELLS                     MMU_TOTAL_CELLS_60K
#define MMU_CELLS_RSVD_IP                   256
#define MMU_BUFQ_MIN                        8
#define MMU_MCQE_MIN                        8
#define MMU_RQE_MIN                         8
#define MMU_BYTES_PER_CELL                  208   /* bytes (1664 bits) */
#define MMU_TOTAL_MCQE_ENTRIES              49152
#define MMU_TOTAL_RQE_ENTRIES               4096
#define MMU_BYTES_TO_CELLS(_byte_)  \
    (((_byte_) + MMU_BYTES_PER_CELL - 1) / MMU_BYTES_PER_CELL)
#define MMU_NUM_RQE                         11
#define MMU_NUM_PG                          8
#define MMU_NUM_POOL                        4
#define MMU_DEFAULT_PG                      (MMU_NUM_PG - 1)

#define FW_ALIGN_BYTES                      16
#define FW_ALIGN_MASK                       (FW_ALIGN_BYTES - 1)

#define CMIC_NUM_PKT_DMA_CHAN               4

#define PGW_MASTER_COUNT                    4
#define PGW_TDM_LEN                         32
#define MMU_TDM_LEN                         256
#define MMU_OVS_GROUP_COUNT                 4
#define MMU_OVS_GROUP_TDM_LEN               16
#define IARB_TDM_LEN                        512

#define NUM_EXT_PORTS                       130
#define OVS_TOKEN                           (NUM_EXT_PORTS+1)
#define IDL_TOKEN                           (NUM_EXT_PORTS+2)
#define MGM_TOKEN                           (NUM_EXT_PORTS+3)

#define PORT_STATE_UNUSED                   0
#define PORT_STATE_LINERATE                 1
#define PORT_STATE_OVERSUB                  2
#define PORT_STATE_SUBPORT                  3

#define CPU_RESERVED_L0_BASE                261
#define CPU_RESERVED_L0_NUM                 4

static struct tdm_config_s {
    int     speed[NUM_EXT_PORTS];
    int     tdm_bw;
    int     port_state[NUM_EXT_PORTS];
    int     pipe_ovs_state[2];
    int     manage_port_type; /* 0-none, 1-4x1g, 2-4x2.5g, 3-1x10g */
    int     pgw_tdm[PGW_MASTER_COUNT][PGW_TDM_LEN];
    int     pgw_ovs_tdm[PGW_MASTER_COUNT][PGW_TDM_LEN];
    int     pgw_ovs_spacing[PGW_MASTER_COUNT][PGW_TDM_LEN];
    int     mmu_tdm[2][MMU_TDM_LEN + 1];
    int     mmu_ovs_group_tdm[2][MMU_OVS_GROUP_COUNT][MMU_OVS_GROUP_TDM_LEN];
    int     iarb_tdm_wrap_ptr[2];
    int     iarb_tdm[2][IARB_TDM_LEN];
} tcfg;

typedef struct {
    int         level;
    int         node_id;
    int         child_num;
    int         sched_mode;
    uint32_t    uc_mc_map;
} _port_lls_config_t;

typedef struct {
    int         parent;
    int         level;
    int         offset;
    uint32_t    hw_index;
    int         l2_uc;
} _sched_pending_t;

typedef enum {
    SCHED_MODE_UNKNOWN = 0,
    SCHED_MODE_STRICT,
    SCHED_MODE_WRR,
    SCHED_MODE_WDRR
} _sched_mode_e;

typedef enum {
    NODE_LEVEL_ROOT = 0,
    NODE_LEVEL_L0,
    NODE_LEVEL_L1,
    NODE_LEVEL_L2,
    NODE_LEVEL_MAX
} _node_level_e;

static _port_lls_config_t cpu_lls_config[] = {
    { NODE_LEVEL_ROOT, 0, 1, SCHED_MODE_WRR, 0 },
    { NODE_LEVEL_L0,   0, 6, SCHED_MODE_WRR, 0 },
    { NODE_LEVEL_L1,   0, 8, SCHED_MODE_WRR, 0 },
    { NODE_LEVEL_L1,   1, 8, SCHED_MODE_WRR, 0 },
    { NODE_LEVEL_L1,   2, 8, SCHED_MODE_WRR, 0 },
    { NODE_LEVEL_L1,   3, 8, SCHED_MODE_WRR, 0 },
    { NODE_LEVEL_L1,   4, 8, SCHED_MODE_WRR, 0 },
    { NODE_LEVEL_L1,   5, 8, SCHED_MODE_WRR, 0 },
    {            -1,  -1, 0,             -1, 0 }};

static _port_lls_config_t xlport_lls_config[] = {
    { NODE_LEVEL_ROOT, 0, 1, SCHED_MODE_WRR, 0 },
    { NODE_LEVEL_L0,   0,10, SCHED_MODE_WRR, 0 },
    { NODE_LEVEL_L1,   0, 2, SCHED_MODE_WRR, 1 },
    { NODE_LEVEL_L1,   1, 2, SCHED_MODE_WRR, 1 },
    { NODE_LEVEL_L1,   2, 2, SCHED_MODE_WRR, 1 },
    { NODE_LEVEL_L1,   3, 2, SCHED_MODE_WRR, 1 },
    { NODE_LEVEL_L1,   4, 2, SCHED_MODE_WRR, 1 },
    { NODE_LEVEL_L1,   5, 2, SCHED_MODE_WRR, 1 },
    { NODE_LEVEL_L1,   6, 2, SCHED_MODE_WRR, 1 },
    { NODE_LEVEL_L1,   7, 2, SCHED_MODE_WRR, 1 },
    { NODE_LEVEL_L1,   8, 2, SCHED_MODE_WRR, 1 },
    { NODE_LEVEL_L1,   9, 2, SCHED_MODE_WRR, 1 },
    {            -1,   0, 0,             -1, 1 }};

static int _invalid_parent[NODE_LEVEL_MAX] = { -1, 48, 271, 1023 } ;
static int _core_frequency;

static uint32_t
_total_bandwidth(int unit)
{
    cdk_pbmp_t pbmp;
    int port;
    uint32_t total_bw = 0;
    
    bcm56850_a0_xlport_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        total_bw += bcm56850_a0_port_speed_max(unit, port);
    }
    
    return total_bw;
}

static int
_port_map_init(int unit)
{
    int ioerr = 0;
    int port, lport, mport;
    int num_pport = NUM_PHYS_PORTS;
    int num_lport = NUM_LOGIC_PORTS;
    ING_PHYS_TO_LOGIC_MAPm_t ing_p2l;
    IFP_GM_LOGIC_TO_PHYS_MAPr_t ifp_l2p;
    EGR_LOGIC_TO_PHYS_MAPr_t egr_l2p;
    MMU_TO_PHYS_MAPr_t mmu_m2p;
    MMU_TO_LOGIC_MAPr_t mmu_m2l;

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

    /* Ingress logical to physical port mapping */
    for (lport = 0; lport < num_lport; lport++) {
        port = L2P(unit, lport);
        if (port < 0) {
            continue;
        }
        IFP_GM_LOGIC_TO_PHYS_MAPr_VALIDf_SET(ifp_l2p, 1);
        if (PORT_IN_Y_PIPE(port)) {
            IFP_GM_LOGIC_TO_PHYS_MAPr_Y_PIPEf_SET(ifp_l2p, 1);
        }
        IFP_GM_LOGIC_TO_PHYS_MAPr_PHYS_PORTf_SET(ifp_l2p, (P2M(unit, port) & 0x3f));
        ioerr += WRITE_IFP_GM_LOGIC_TO_PHYS_MAPr(unit, lport, ifp_l2p);
    }

    /* Egress logical to physical port mapping */
    for (lport = 0; lport < num_lport; lport++) {
        port = L2P(unit, lport);
        if (port < 0) {
            port = 0xff;
        }
        EGR_LOGIC_TO_PHYS_MAPr_PHYS_PORTf_SET(egr_l2p, port);
        ioerr += WRITE_EGR_LOGIC_TO_PHYS_MAPr(unit, lport, egr_l2p);
    }

    /* MMU to physical port mapping and MMU to logical port mapping */
    for (port = 0; port < num_pport; port++) {
        mport = P2M(unit, port);
        if (mport < 0) {
            continue;
        }
        lport = P2L(unit, port);
        if (lport < 0) {
            lport = 0x7f;
        }
        MMU_TO_PHYS_MAPr_PHYS_PORTf_SET(mmu_m2p, port);
        ioerr += WRITE_MMU_TO_PHYS_MAPr(unit, mport, mmu_m2p);
        MMU_TO_LOGIC_MAPr_LOGIC_PORTf_SET(mmu_m2l, lport);
        ioerr += WRITE_MMU_TO_LOGIC_MAPr(unit, mport, mmu_m2l);
    }

    return ioerr;
}

static int
_tdm_init(int unit)
{
    int ioerr = 0;
    int rv;
    PGW_TDM_CONTROLr_t pgw_tdm_ctrl;
    PGW_LR_TDM_REGr_t pgw_lr_tdm;
    PGW_OS_TDM_REGr_t pgw_os_tdm;
    PGW_OS_PORT_SPACING_REGr_t pgw_os_port_spacing;
    ES_PIPE0_TDM_CONFIGr_t es0_tdm_cfg;
    ES_PIPE0_TDM_TABLE_0m_t es0_tdm_tbl0;
    ES_PIPE0_OVR_SUB_GRP_CFGr_t es0_ovr_cfg;
    ES_PIPE0_OVR_SUB_GRP_TBLr_t es0_ovr_tbl;
    ES_PIPE0_OVR_SUB_GRP_WTr_t es0_ovr_wt;
    ES_PIPE1_TDM_CONFIGr_t es1_tdm_cfg;
    ES_PIPE1_TDM_TABLE_0m_t es1_tdm_tbl0;
    ES_PIPE1_OVR_SUB_GRP_CFGr_t es1_ovr_cfg;
    ES_PIPE1_OVR_SUB_GRP_TBLr_t es1_ovr_tbl;
    ES_PIPE1_OVR_SUB_GRP_WTr_t es1_ovr_wt;
    IARB_MAIN_TDM_Xm_t iarb_main_x;
    IARB_MAIN_TDM_Ym_t iarb_main_y;
    IARB_TDM_CONTROLr_t iarb_tdm_ctrl;
    EGR_EDB_XMIT_CTRLm_t egr_xmit_ctrl;
    ENQ_CONFIGr_t enq_cfg;
    cdk_pbmp_t mmu_pbmp, pbmp;
    uint32_t bandwidth, speed, rval;
    int *tdm;
    int port, mport, id, weight;
    int idx, pipe, pgw, group, count, base, slot;
    int oversub;
    
    CDK_MEMSET(&tcfg, 0, sizeof(struct tdm_config_s));

    if (_core_frequency == 608) {
        bandwidth = 720000;
    } else {
        bandwidth = 960000;
    }  

    oversub = (_total_bandwidth(unit) > bandwidth) ? 1 : 0;

    /* Get MMU ports */
    bcm56850_a0_xlport_pbmp_get(unit, &mmu_pbmp);
    CDK_PBMP_PORT_ADD(mmu_pbmp, CMIC_PORT);

    CDK_PBMP_ITER(mmu_pbmp, port) {
        speed = bcm56850_a0_port_speed_max(unit, port);
        if (speed == 0) {
            continue;
        }
        tcfg.speed[port] = speed;
        if (oversub) {
            tcfg.port_state[port] = PORT_STATE_OVERSUB;
        } else {
            tcfg.port_state[port] = PORT_STATE_LINERATE;
        }
        if (tcfg.speed[port] >= 20000) {
            tcfg.port_state[port + 1] = PORT_STATE_SUBPORT;
        }
        if (tcfg.speed[port] >= 40000) {
            tcfg.port_state[port + 2] = PORT_STATE_SUBPORT;
            tcfg.port_state[port + 3] = PORT_STATE_SUBPORT;
        }
    }

    tcfg.speed[0] = 1000;
    tcfg.speed[129] = 1000;
    tcfg.tdm_bw = bandwidth / 1000;
    tcfg.manage_port_type = 0;
    tcfg.mmu_tdm[0][MMU_TDM_LEN] = NUM_EXT_PORTS;
    tcfg.mmu_tdm[1][MMU_TDM_LEN] = NUM_EXT_PORTS;
    if (oversub) {
        tcfg.pipe_ovs_state[0] = 1;
        tcfg.pipe_ovs_state[1] = 1;
    }
    
    CDK_VVERB(("tdm_bw: %dG\n", tcfg.tdm_bw));
    CDK_VVERB(("port speed:"));
    for (idx = 0; idx < NUM_EXT_PORTS; idx++) {
        if (idx % 8 == 0) {
            CDK_VVERB(("\n    "));
        }
        CDK_VVERB((" %6d", tcfg.speed[idx]));
    }
    CDK_VVERB(("\n"));
    CDK_VVERB(("port state map:"));
    for (idx = 0; idx < NUM_EXT_PORTS; idx++) {
        if (idx % 16 == 0) {
            CDK_VVERB(("\n    "));
        }
        if (idx == 0 || idx == (NUM_EXT_PORTS - 1)) {
            CDK_VVERB((" ---"));
        } else {
            CDK_VVERB((" %3d", tcfg.port_state[idx]));
        }
    }
    CDK_VVERB(("\n"));

    rv = bcm56850_a0_set_tdm_tbl(tcfg.speed,
                                 tcfg.tdm_bw,
                                 tcfg.pgw_tdm[0],
                                 tcfg.pgw_ovs_tdm[0],
                                 tcfg.pgw_ovs_spacing[0],
                                 tcfg.pgw_tdm[1],
                                 tcfg.pgw_ovs_tdm[1],
                                 tcfg.pgw_ovs_spacing[1],
                                 tcfg.pgw_tdm[2],
                                 tcfg.pgw_ovs_tdm[2],
                                 tcfg.pgw_ovs_spacing[2],
                                 tcfg.pgw_tdm[3],
                                 tcfg.pgw_ovs_tdm[3],
                                 tcfg.pgw_ovs_spacing[3],
                                 tcfg.mmu_tdm[0],
                                 tcfg.mmu_ovs_group_tdm[0][0],
                                 tcfg.mmu_ovs_group_tdm[0][1],
                                 tcfg.mmu_ovs_group_tdm[0][2],
                                 tcfg.mmu_ovs_group_tdm[0][3],
                                 tcfg.mmu_tdm[1],
                                 tcfg.mmu_ovs_group_tdm[1][0],
                                 tcfg.mmu_ovs_group_tdm[1][1],
                                 tcfg.mmu_ovs_group_tdm[1][2],
                                 tcfg.mmu_ovs_group_tdm[1][3],
                                 &tcfg.port_state[1], /* skip cpu port */
                                 tcfg.iarb_tdm[0],
                                 tcfg.iarb_tdm[1]);
    if (rv == 0) {
        CDK_ERR(("bcm56850_a0_init[%d]: Unable to configure TDM\n", unit));
        return CDK_E_FAIL;
    }

    for (pgw = 0; pgw < PGWS_PER_DEV; pgw += 2) {
        CDK_VVERB(("PGW_CL%d pgw_tdm:", pgw));
        for (idx = 0; idx < PGW_TDM_LEN; idx++) {
            if (idx % 16 == 0) {
                CDK_VVERB(("\n    "));
            }
            CDK_VVERB((" %3d", tcfg.pgw_tdm[pgw / 2][idx]));
        }
        CDK_VVERB(("\n"));
        CDK_VVERB(("PGW_CL%d pgw_ovs_tdm:", pgw));
        for (idx = 0; idx < PGW_TDM_LEN; idx++) {
            if (idx % 16 == 0) {
                CDK_VVERB(("\n    "));
            }
            CDK_VVERB((" %3d", tcfg.pgw_ovs_tdm[pgw / 2][idx]));
        }
        CDK_VVERB(("\n"));
        CDK_VVERB(("PGW_CL%d pgw_ovs_spacing:", pgw));
        for (idx = 0; idx < PGW_TDM_LEN; idx++) {
            if (idx % 16 == 0) {
                CDK_VVERB(("\n    "));
            }
            CDK_VVERB((" %3d",
                      tcfg.pgw_ovs_spacing[pgw / 2][idx]));
        }
        CDK_VVERB(("\n"));
    }
    for (pipe = 0; pipe < PIPES_PER_DEV; pipe++) {
        CDK_VVERB(("Pipe %c mmu_tdm:", pipe ? 'y' : 'x'));
        for (idx = 0; idx < MMU_TDM_LEN; idx++) {
            if (idx % 16 == 0) {
                CDK_VVERB(("\n    "));
            }
            CDK_VVERB((" %3d", tcfg.mmu_tdm[pipe][idx]));
        }
        CDK_VVERB(("\n"));
        for (group = 0; group < MMU_OVS_GROUP_COUNT; group++) {
            CDK_VVERB(("Pipe %c group %d ovs_group_tdm",
                       pipe ? 'y' : 'x', group));
            for (idx = 0; idx < MMU_OVS_GROUP_TDM_LEN; idx++) {
                if (idx % 16 == 0) {
                    CDK_VVERB(("\n    "));
                }
                CDK_VVERB((" %3d",
                           tcfg.mmu_ovs_group_tdm[pipe][group][idx]));
            }
            CDK_VVERB(("\n"));
        }
    }

    CDK_VVERB(("tdm_bw: %dG\n", tcfg.tdm_bw));
    CDK_VVERB(("x pipe ovs state: %d\n", tcfg.pipe_ovs_state[0]));
    CDK_VVERB(("y pipe ovs state: %d\n", tcfg.pipe_ovs_state[1]));
    CDK_VVERB(("manage port type: %d\n", tcfg.manage_port_type));

    rv = bcm56850_a0_set_iarb_tdm_table(tcfg.tdm_bw,
                                        tcfg.pipe_ovs_state[0],
                                        tcfg.pipe_ovs_state[1],
                                        tcfg.manage_port_type == 1,
                                        tcfg.manage_port_type == 2,
                                        tcfg.manage_port_type == 3,
                                        &tcfg.iarb_tdm_wrap_ptr[0],
                                        &tcfg.iarb_tdm_wrap_ptr[1],
                                        tcfg.iarb_tdm[0],
                                        tcfg.iarb_tdm[1]);
    if (rv == 0) {
        CDK_ERR(("bcm56850_a0_init[%d]: Unable to configure IARB TDM\n", unit));
        return CDK_E_FAIL;
    }

    for (pipe = 0; pipe < PIPES_PER_DEV; pipe++) {
        CDK_VVERB(("Pipe %c iarb_tdm: (wrap_ptr %d)",
                   pipe ? 'y' : 'x',
                   tcfg.iarb_tdm_wrap_ptr[pipe]));
        for (idx = 0; idx < IARB_TDM_LEN; idx++) {
            if (idx > tcfg.iarb_tdm_wrap_ptr[pipe]) {
                break;
            }
            if (idx % 16 == 0) {
                CDK_VVERB(("\n    "));
            }
            CDK_VVERB((" %3d", tcfg.iarb_tdm[pipe][idx]));
        }
        CDK_VVERB(("\n"));
    }

    /* Configure PGW TDM, only need to program master PGWs (PGW_CL0/2/4/6) */
    for (pgw = 0; pgw < PGWS_PER_DEV; pgw += 2) {

        ioerr += READ_PGW_TDM_CONTROLr(unit, pgw, &pgw_tdm_ctrl);
        PGW_TDM_CONTROLr_OS_TDM_ENABLEf_SET(pgw_tdm_ctrl, 0);

        /* Configure PGW line rate ports TDM */
        count = 0;
        for (base = 0; base < PGW_TDM_LEN; base += 4) {
            PGW_LR_TDM_REGr_CLR(pgw_lr_tdm);
            for (idx = 0; idx < 4; idx++) {
                slot = base + idx;
                port = tcfg.pgw_tdm[(pgw >> 1)][slot];
                if (port == NUM_EXT_PORTS) {
                    break;
                }
                rval = PGW_LR_TDM_REGr_GET(pgw_lr_tdm, 0);
                rval |= (port << (idx << 3));
                PGW_LR_TDM_REGr_SET(pgw_lr_tdm, 0, rval);
                count++;
            }
            if (idx != 0) {
                ioerr += WRITE_PGW_LR_TDM_REGr(unit, pgw, (base >> 2), pgw_lr_tdm);
            }
            if (idx != 4) {
                break;
            }
        }
        if (count > 0) {
            PGW_TDM_CONTROLr_LR_TDM_WRAP_PTRf_SET(pgw_tdm_ctrl, count - 1);
            PGW_TDM_CONTROLr_LR_TDM_ENABLEf_SET(pgw_tdm_ctrl, 1);
        } else {
            PGW_TDM_CONTROLr_LR_TDM_ENABLEf_SET(pgw_tdm_ctrl, 0);
        }

        /* Configure PGW oversubscription ports TDM */
        count = 0;
        for (base = 0; base < PGW_TDM_LEN; base += 4) {
            PGW_OS_TDM_REGr_CLR(pgw_os_tdm);
            for (idx = 0; idx < 4; idx++) {
                slot = base + idx;
                port = tcfg.pgw_ovs_tdm[pgw >> 1][slot];
                if (port == NUM_EXT_PORTS) {
                    port = 0xff;
                } else {
                    count++;
                }
                rval = PGW_OS_TDM_REGr_GET(pgw_os_tdm, 0);
                rval |= (port << (idx << 3));
                PGW_OS_TDM_REGr_SET(pgw_os_tdm, 0, rval);
            }
            ioerr += WRITE_PGW_OS_TDM_REGr(unit, pgw, (base >> 2), pgw_os_tdm);
        }
        /* OS_TDM_WRAP_PTR is always 31 (reset value) */
        PGW_TDM_CONTROLr_OS_TDM_ENABLEf_SET(pgw_tdm_ctrl, (count ? 1 : 0));

        /* Configure PGW oversubscription port spacing */
        for (base = 0; base < PGW_TDM_LEN; base += 4) {
            PGW_OS_PORT_SPACING_REGr_CLR(pgw_os_port_spacing);
            for (idx = 0; idx < 4; idx++) {
                slot = base + idx;
                port = tcfg.pgw_ovs_spacing[pgw >> 1][slot];

                rval = PGW_OS_PORT_SPACING_REGr_GET(pgw_os_port_spacing, 0);
                rval |= (port << (idx << 3));
                PGW_OS_PORT_SPACING_REGr_SET(pgw_os_port_spacing, 0, rval);
            }
            ioerr += WRITE_PGW_OS_PORT_SPACING_REGr(unit, pgw, (base >> 2), pgw_os_port_spacing);
        }

        ioerr += WRITE_PGW_TDM_CONTROLr(unit, pgw, pgw_tdm_ctrl);
    }

    /* Configure MMU TDM for X-pipe */
    pipe = 0;
    tdm = tcfg.mmu_tdm[pipe];
    ES_PIPE0_TDM_CONFIGr_CLR(es0_tdm_cfg);
    ES_PIPE0_TDM_TABLE_0m_CLR(es0_tdm_tbl0);

    for (slot = 0; slot < MMU_TDM_LEN; slot += 2) {
        port = tdm[slot];
        if (port == OVS_TOKEN) {
            mport = 57; /* opportunist port */
        } else if (port == IDL_TOKEN) {
            mport = 58; /* scheduler will not pick anything */
        } else if (port >= NUM_EXT_PORTS) {
            mport = 0x3f;
        } else {
            mport = P2M(unit, port);
        }
        ES_PIPE0_TDM_TABLE_0m_PORT_NUM_EVENf_SET(es0_tdm_tbl0, (mport & 0x3f));
        port = tdm[slot + 1];
        if (port == OVS_TOKEN) {
            mport = 57; /* opportunist port */
        } else if (port == IDL_TOKEN) {
            mport = 58; /* scheduler will not pick anything */
        } else if (port >= NUM_EXT_PORTS) {
            mport = 0x3f;
        } else {
            mport = P2M(unit, port);
        }
        ES_PIPE0_TDM_TABLE_0m_PORT_NUM_ODDf_SET(es0_tdm_tbl0, (mport & 0x3f));
        ioerr += WRITE_ES_PIPE0_TDM_TABLE_0m(unit, (slot >> 1), es0_tdm_tbl0);

        if (tdm[slot + 2] == NUM_EXT_PORTS) {
            ES_PIPE0_TDM_CONFIGr_CAL0_ENDf_SET(es0_tdm_cfg, (slot >> 1));
            if (tdm[slot + 1] == NUM_EXT_PORTS) {
                ES_PIPE0_TDM_CONFIGr_CAL0_END_SINGLEf_SET(es0_tdm_cfg, 1);
            }
            break;
        }
    }
    ES_PIPE0_TDM_CONFIGr_ENABLEf_SET(es0_tdm_cfg, 1);
    ES_PIPE0_TDM_CONFIGr_OPP_CPULB_ENf_SET(es0_tdm_cfg, 1);
    ioerr += WRITE_ES_PIPE0_TDM_CONFIGr(unit, es0_tdm_cfg);

    if (_core_frequency < 760) {
        for (group = 0; group < MMU_OVS_GROUP_COUNT; group++) {
            port = tcfg.mmu_ovs_group_tdm[pipe][group][0];
            if (port >= NUM_EXT_PORTS) {
                continue;
            }
            speed = bcm56850_a0_port_speed_max(unit, port);
            if (speed < 30000) {
                continue;
            }

            ioerr += READ_ES_PIPE0_OVR_SUB_GRP_CFGr(unit, group, &es0_ovr_cfg);
            ES_PIPE0_OVR_SUB_GRP_CFGr_SAME_SPACINGf_SET(es0_ovr_cfg, ((_core_frequency < 500) ? 6 : 8));
            ioerr += WRITE_ES_PIPE0_OVR_SUB_GRP_CFGr(unit, group, es0_ovr_cfg);
        }
    }
        
    for (group = 0; group < MMU_OVS_GROUP_COUNT; group++) {
        ES_PIPE0_OVR_SUB_GRP_TBLr_CLR(es0_ovr_tbl);
        tdm = tcfg.mmu_ovs_group_tdm[pipe][group];
        for (slot = 0; slot < MMU_OVS_GROUP_TDM_LEN; slot++) {
            port = tdm[slot];
            if (port >= NUM_EXT_PORTS) {
                mport = 0x3f;
                id = 0;
            } else {
                mport = P2M(unit, port);
                id = XLPORT_BLKIDX(port);
            }
            ES_PIPE0_OVR_SUB_GRP_TBLr_PHY_PORT_IDf_SET(es0_ovr_tbl, (id & 0xf));
            ES_PIPE0_OVR_SUB_GRP_TBLr_MMU_PORTf_SET(es0_ovr_tbl, (mport & 0x3f));
            idx = (group * MMU_OVS_GROUP_TDM_LEN) + slot;
            ioerr += WRITE_ES_PIPE0_OVR_SUB_GRP_TBLr(unit, idx, es0_ovr_tbl);
        }
        ES_PIPE0_OVR_SUB_GRP_WTr_CLR(es0_ovr_wt);
        tdm = tcfg.mmu_ovs_group_tdm[pipe][group];
        port = tdm[0];
        if (port >= NUM_EXT_PORTS) {
            weight = 0;
        } else {
            speed = bcm56850_a0_port_speed_max(unit, port);
            /* use 2500M as weight unit */
            weight = (speed > 2500 ? speed : 2500) / 2500;
        }
        for (slot = 0; slot < MMU_OVS_GROUP_TDM_LEN; slot++) {
            ES_PIPE0_OVR_SUB_GRP_WTr_WEIGHTf_SET(es0_ovr_wt,
                                                 weight * (slot + 1));
            idx = (group * MMU_OVS_GROUP_TDM_LEN) + slot;
            ioerr += WRITE_ES_PIPE0_OVR_SUB_GRP_WTr(unit, idx, es0_ovr_wt);
        }
    }

    /* Configure MMU TDM for Y-pipe */
    pipe = 1;
    tdm = tcfg.mmu_tdm[pipe];
    ES_PIPE1_TDM_CONFIGr_CLR(es1_tdm_cfg);
    ES_PIPE1_TDM_TABLE_0m_CLR(es1_tdm_tbl0);
    for (slot = 0; slot < MMU_TDM_LEN; slot += 2) {
        port = tdm[slot];
        if (port == OVS_TOKEN) {
            mport = 57; /* opportunist port */
        } else if (port == IDL_TOKEN) {
            mport = 58; /* scheduler will not pick anything */
        } else if (port >= NUM_EXT_PORTS) {
            mport = 0x3f;
        } else {
            mport = P2M(unit, port);
        }
        ES_PIPE1_TDM_TABLE_0m_PORT_NUM_EVENf_SET(es1_tdm_tbl0, (mport & 0x3f));
        port = tdm[slot + 1];
        if (port == OVS_TOKEN) {
            mport = 57; /* opportunist port */
        } else if (port == IDL_TOKEN) {
            mport = 58; /* scheduler will not pick anything */
        } else if (port >= NUM_EXT_PORTS) {
            mport = 0x3f;
        } else {
            mport = P2M(unit, port);
        }
        ES_PIPE1_TDM_TABLE_0m_PORT_NUM_ODDf_SET(es1_tdm_tbl0, (mport & 0x3f));
        ioerr += WRITE_ES_PIPE1_TDM_TABLE_0m(unit, (slot >> 1), es1_tdm_tbl0);

        if (tdm[slot + 2] == NUM_EXT_PORTS) {
            ES_PIPE1_TDM_CONFIGr_CAL0_ENDf_SET(es1_tdm_cfg, (slot >> 1));
            if (tdm[slot + 1] == NUM_EXT_PORTS) {
                ES_PIPE1_TDM_CONFIGr_CAL0_END_SINGLEf_SET(es1_tdm_cfg, 1);
            }
            break;
        }
    }
    ES_PIPE1_TDM_CONFIGr_ENABLEf_SET(es1_tdm_cfg, 1);
    ES_PIPE1_TDM_CONFIGr_OPP_CPULB_ENf_SET(es1_tdm_cfg, 1);
    ioerr += WRITE_ES_PIPE1_TDM_CONFIGr(unit, es1_tdm_cfg);

    if (_core_frequency < 760) {
        for (group = 0; group < MMU_OVS_GROUP_COUNT; group++) {
            port = tcfg.mmu_ovs_group_tdm[pipe][group][0];
            if (port >= NUM_EXT_PORTS) {
                continue;
            }
            speed = bcm56850_a0_port_speed_max(unit, port);
            if (speed < 30000) {
                continue;
            }

            ioerr += READ_ES_PIPE1_OVR_SUB_GRP_CFGr(unit, group, &es1_ovr_cfg);
            ES_PIPE1_OVR_SUB_GRP_CFGr_SAME_SPACINGf_SET(es1_ovr_cfg, ((_core_frequency < 500) ? 6 : 8));
            ioerr += WRITE_ES_PIPE1_OVR_SUB_GRP_CFGr(unit, group, es1_ovr_cfg);
        }
    }
        
    for (group = 0; group < MMU_OVS_GROUP_COUNT; group++) {
        ES_PIPE1_OVR_SUB_GRP_TBLr_CLR(es1_ovr_tbl);
        tdm = tcfg.mmu_ovs_group_tdm[pipe][group];
        for (slot = 0; slot < MMU_OVS_GROUP_TDM_LEN; slot++) {
            port = tdm[slot];
            if (port >= NUM_EXT_PORTS) {
                mport = 0x3f;
                id = 0;
            } else {
                mport = P2M(unit, port);
                id = XLPORT_BLKIDX(port);
            }
            ES_PIPE1_OVR_SUB_GRP_TBLr_PHY_PORT_IDf_SET(es1_ovr_tbl, (id & 0xf));
            ES_PIPE1_OVR_SUB_GRP_TBLr_MMU_PORTf_SET(es1_ovr_tbl, (mport & 0x3f));
            idx = (group * MMU_OVS_GROUP_TDM_LEN) + slot;
            ioerr += WRITE_ES_PIPE1_OVR_SUB_GRP_TBLr(unit, idx, es1_ovr_tbl);
        }
        ES_PIPE1_OVR_SUB_GRP_WTr_CLR(es1_ovr_wt);
        tdm = tcfg.mmu_ovs_group_tdm[pipe][group];
        port = tdm[0];
        if (port >= NUM_EXT_PORTS) {
            weight = 0;
        } else {
            speed = bcm56850_a0_port_speed_max(unit, port);
            /* use 2500M as weight unit */
            weight = (speed > 2500 ? speed : 2500) / 2500;
        }
        for (slot = 0; slot < MMU_OVS_GROUP_TDM_LEN; slot++) {
            ES_PIPE1_OVR_SUB_GRP_WTr_WEIGHTf_SET(es1_ovr_wt,
                                                 weight * (slot + 1));
            idx = (group * MMU_OVS_GROUP_TDM_LEN) + slot;
            ioerr += WRITE_ES_PIPE1_OVR_SUB_GRP_WTr(unit, idx, es1_ovr_wt);
        }
    }

    /* Configure IARB TDM for X-pipe */
    pipe = 0;
    tdm = tcfg.iarb_tdm[pipe];
    IARB_MAIN_TDM_Xm_CLR(iarb_main_x);
    for (slot = 0; slot < IARB_TDM_LEN; slot++) {
        if (slot > tcfg.iarb_tdm_wrap_ptr[pipe]) {
            break;
        }
        IARB_MAIN_TDM_Xm_TDM_SLOTf_SET(iarb_main_x, tdm[slot]);
        ioerr += WRITE_IARB_MAIN_TDM_Xm(unit, slot, iarb_main_x);
    }

    /* Configure IARB TDM for Y-pipe */
    pipe = 1;
    tdm = tcfg.iarb_tdm[pipe];
    IARB_MAIN_TDM_Ym_CLR(iarb_main_y);
    for (slot = 0; slot < IARB_TDM_LEN; slot++) {
        if (slot > tcfg.iarb_tdm_wrap_ptr[pipe]) {
            break;
        }
        IARB_MAIN_TDM_Ym_TDM_SLOTf_SET(iarb_main_y, tdm[slot]);
        ioerr += WRITE_IARB_MAIN_TDM_Ym(unit, slot, iarb_main_y);
    }

    /* Both pipe are expected to have same IARB TDM table length */
    ioerr += READ_IARB_TDM_CONTROLr(unit, &iarb_tdm_ctrl);
    IARB_TDM_CONTROLr_TDM_WRAP_PTRf_SET(iarb_tdm_ctrl, tcfg.iarb_tdm_wrap_ptr[0]);
    IARB_TDM_CONTROLr_DISABLEf_SET(iarb_tdm_ctrl, 0);
#if BMD_CONFIG_SIMULATION
    IARB_TDM_CONTROLr_DISABLEf_SET(iarb_tdm_ctrl, 1);
#endif
    IARB_TDM_CONTROLr_AUX_CMICM_SLOT_ENf_SET(iarb_tdm_ctrl, 1);
    IARB_TDM_CONTROLr_AUX_EP_LB_SLOT_ENf_SET(iarb_tdm_ctrl, 1);
    ioerr += WRITE_IARB_TDM_CONTROLr(unit, iarb_tdm_ctrl);

    bcm56850_a0_xlport_pbmp_get(unit, &pbmp);
    if (oversub) {
        /* Configure oversubscription port with WAIT_FOR_2ND_MOP */
        CDK_PBMP_ITER(pbmp, port) {
            EGR_EDB_XMIT_CTRLm_CLR(egr_xmit_ctrl);
            EGR_EDB_XMIT_CTRLm_WAIT_FOR_2ND_MOPf_SET(egr_xmit_ctrl, 1);
            EGR_EDB_XMIT_CTRLm_START_CNTf_SET(egr_xmit_ctrl, ((_core_frequency >= 760) ? 12 :
                                                               ((_core_frequency >= 608) ? 14 : 15)));
            ioerr += WRITE_EGR_EDB_XMIT_CTRLm(unit, port, egr_xmit_ctrl);
        }
    } else {
        /* Configure linerate port with WAIT_FOR_MOP */
        CDK_PBMP_ITER(pbmp, port) {
            EGR_EDB_XMIT_CTRLm_CLR(egr_xmit_ctrl);
            EGR_EDB_XMIT_CTRLm_WAIT_FOR_MOPf_SET(egr_xmit_ctrl, 1);
            EGR_EDB_XMIT_CTRLm_START_CNTf_SET(egr_xmit_ctrl, 7);
            if (_core_frequency <= 415) {
                speed = bcm56850_a0_port_speed_max(unit, port);
                EGR_EDB_XMIT_CTRLm_START_CNTf_SET(egr_xmit_ctrl, ((speed > 11000) ? 8 : 7));
            }
            ioerr += WRITE_EGR_EDB_XMIT_CTRLm(unit, port, egr_xmit_ctrl);
        }
    }

    ENQ_CONFIGr_CLR(enq_cfg);
    ENQ_CONFIGr_ASF_ENABLE_HS_PORT_EP_CREDIT_CHKf_SET(enq_cfg, 1);
    ENQ_CONFIGr_ASF_CFAP_FULL_DROP_ENf_SET(enq_cfg, 1);
    ENQ_CONFIGr_ASF_FIFO_THRESHOLDf_SET(enq_cfg, 3);
    ENQ_CONFIGr_ASF_HS_FIFO_THRESHOLDf_SET(enq_cfg, 13);
    ioerr += WRITE_ENQ_CONFIGr(unit, enq_cfg);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

static int
_mmu_config_init(int unit)
{
    int ioerr = 0;
    CFAPFULLTHRESHOLDSETr_t cfap_thr_set;
    CFAPFULLTHRESHOLDRESETr_t cfap_thr_rst;
    CFAP_ARBITER_MASKr_t cfap_arb_mask;
    THDI_GLOBAL_HDRM_LIMIT_PIPEXr_t g_hdrm_limit_x;
    THDI_GLOBAL_HDRM_LIMIT_PIPEYr_t g_hdrm_limit_y;
    THDI_POOL_CONFIGr_t pool_cfg;
    THDI_BUFFER_CELL_LIMIT_SPr_t buf_lim_sp;
    THDI_CELL_RESET_LIMIT_OFFSET_SPr_t rst_lim_off_sp;
    OP_THDU_CONFIGr_t op_cfg;
    MMU_THDM_DB_DEVICE_THR_CONFIGr_t dev_thr_cfg;
    MMU_THDM_DB_POOL_SHARED_LIMITr_t db_sh_lim;
    MMU_THDM_DB_POOL_YELLOW_SHARED_LIMITr_t db_ysh_lim;
    MMU_THDM_DB_POOL_RED_SHARED_LIMITr_t db_rsh_lim;
    MMU_THDM_DB_POOL_RESUME_LIMITr_t db_rsm_lim;
    MMU_THDM_DB_POOL_YELLOW_RESUME_LIMITr_t db_yrsm_lim;
    MMU_THDM_DB_POOL_RED_RESUME_LIMITr_t db_rrsm_lim;
    MMU_THDM_MCQE_POOL_SHARED_LIMITr_t mcq_sh_lim;
    MMU_THDM_MCQE_POOL_YELLOW_SHARED_LIMITr_t mcq_ysh_lim;
    MMU_THDM_MCQE_POOL_RED_SHARED_LIMITr_t mcq_rsh_lim;
    MMU_THDM_MCQE_POOL_RESUME_LIMITr_t mcq_rsm_lim;
    MMU_THDM_MCQE_POOL_YELLOW_RESUME_LIMITr_t mcq_yrsm_lim;
    MMU_THDM_MCQE_POOL_RED_RESUME_LIMITr_t mcq_rrsm_lim;
    MMU_THDR_DB_CONFIGr_t db_cfg;
    MMU_THDR_QE_CONFIGr_t qe_cfg;
    THDI_PORT_SP_CONFIG_Ym_t sp_cfg_y;
    THDI_PORT_SP_CONFIG_Xm_t sp_cfg_x;
    THDI_PORT_PG_CONFIG_Ym_t pg_cfg_y;
    THDI_PORT_PG_CONFIG_Xm_t pg_cfg_x;
    THDI_PORT_MAX_PKT_SIZEr_t max_pkt;
    THDI_INPUT_PORT_XON_ENABLESr_t xon_en;
    THDI_PORT_PRI_GRPr_t port_pri_grp;
    MMU_THDU_XPIPE_CONFIG_QGROUPm_t cfg_qgrp_x;
    MMU_THDU_YPIPE_CONFIG_QGROUPm_t cfg_qgrp_y;
    MMU_THDU_XPIPE_OFFSET_QGROUPm_t off_qgrp_x;
    MMU_THDU_YPIPE_OFFSET_QGROUPm_t off_qgrp_y;
    MMU_THDU_XPIPE_CONFIG_QUEUEm_t cfg_q_x;
    MMU_THDU_YPIPE_CONFIG_QUEUEm_t cfg_q_y;
    MMU_THDU_XPIPE_OFFSET_QUEUEm_t off_q_x;
    MMU_THDU_YPIPE_OFFSET_QUEUEm_t off_q_y;
    MMU_THDU_XPIPE_Q_TO_QGRP_MAPm_t q2qgrp_x;
    MMU_THDU_YPIPE_Q_TO_QGRP_MAPm_t q2qgrp_y;
    MMU_THDU_XPIPE_CONFIG_PORTm_t pcfg_x;
    MMU_THDU_YPIPE_CONFIG_PORTm_t pcfg_y;
    MMU_THDU_XPIPE_RESUME_PORTm_t prsm_x;
    MMU_THDU_YPIPE_RESUME_PORTm_t prsm_y;
    MMU_THDM_DB_QUEUE_CONFIG_0m_t db_q_cfg_x;
    MMU_THDM_DB_QUEUE_CONFIG_1m_t db_q_cfg_y;
    MMU_THDM_DB_QUEUE_OFFSET_0m_t db_q_off_x;
    MMU_THDM_DB_QUEUE_OFFSET_1m_t db_q_off_y;
    MMU_THDM_MCQE_QUEUE_CONFIG_0m_t mcqe_q_cfg_x;
    MMU_THDM_MCQE_QUEUE_CONFIG_1m_t mcqe_q_cfg_y;
    MMU_THDM_MCQE_QUEUE_OFFSET_0m_t mcqe_q_off_x;
    MMU_THDM_MCQE_QUEUE_OFFSET_1m_t mcqe_q_off_y;
    MMU_THDM_DB_QUEUE_RESUME_OFFSET_PROFILE_YELLOWr_t db_yprof;
    MMU_THDM_DB_QUEUE_RESUME_OFFSET_PROFILE_REDr_t db_rprof;
    MMU_THDM_MCQE_QUEUE_RESUME_OFFSET_PROFILE_YELLOWr_t mcqe_yprof;
    MMU_THDM_MCQE_QUEUE_RESUME_OFFSET_PROFILE_REDr_t mcqe_rprof;
    MMU_THDM_DB_PORTSP_CONFIG_0m_t db_psp_cfg_x;
    MMU_THDM_DB_PORTSP_CONFIG_1m_t db_psp_cfg_y;
    MMU_THDM_MCQE_PORTSP_CONFIG_0m_t mcqe_psp_cfg_x;
    MMU_THDM_MCQE_PORTSP_CONFIG_1m_t mcqe_psp_cfg_y;
    MMU_THDR_DB_LIMIT_MIN_PRIQr_t db_min_pq;
    MMU_THDR_DB_CONFIG_PRIQr_t db_cfg_pq;
    MMU_THDR_DB_CONFIG1_PRIQr_t db_cfg1_pq;
    MMU_THDR_DB_LIMIT_COLOR_PRIQr_t db_lim_col_pq;
    MMU_THDR_DB_RESET_OFFSET_COLOR_PRIQr_t db_lcol_pq;
    MMU_THDR_QE_LIMIT_MIN_PRIQr_t qe_min_pq;
    MMU_THDR_QE_CONFIG_PRIQr_t qe_cfg_pq;
    MMU_THDR_QE_CONFIG1_PRIQr_t qe_cfg1_pq;
    MMU_THDR_QE_LIMIT_COLOR_PRIQr_t qe_lim_col_pq;
    MMU_THDR_QE_RESET_OFFSET_COLOR_PRIQr_t qe_lcol_pq;
    MMU_THDR_DB_CONFIG_SPr_t db_cfg_sp;
    MMU_THDR_DB_SP_SHARED_LIMITr_t db_lim_sp;
    MMU_THDR_DB_RESUME_COLOR_LIMIT_SPr_t db_rsmc_sp;
    MMU_THDR_QE_CONFIG_SPr_t qe_cfg_sp;
    MMU_THDR_QE_SHARED_COLOR_LIMIT_SPr_t qe_lim_sp;
    MMU_THDR_QE_RESUME_COLOR_LIMIT_SPr_t qe_rsmc_sp;
    cdk_pbmp_t mmu_pbmp;
    uint32_t rval, fval;
    uint32_t tot_cells, tot_shared, tot_rsvd, tot_hdrm;
    uint32_t tot_mcqe_entries, tot_mcqe_shared, tot_mcqe_rsvd;
    uint32_t tot_rqe_entries, tot_rqe_shared, tot_rqe_rsvd;
    uint32_t pool_resume;
    uint32_t tot_port_min, tot_pg_min;
    uint32_t default_mtu_cells, jumbo_frame_cells;
    int numq_uc, numq_mc, numq_rqe;
    int port, mport, idx;

    /* Get MMU ports */
    bcm56850_a0_xlport_pbmp_get(unit, &mmu_pbmp);
    CDK_PBMP_PORT_ADD(mmu_pbmp, CMIC_PORT);

    default_mtu_cells = MMU_BYTES_TO_CELLS(MMU_DEFAULT_MTU_BYTES +
                                               MMU_PKT_HDR_BYTES);

    jumbo_frame_cells = MMU_BYTES_TO_CELLS(MMU_JUMBO_FRAME_BYTES +
                                               MMU_PKT_HDR_BYTES);

    tot_cells = MMU_TOTAL_CELLS - MMU_CELLS_RSVD_IP;
    tot_mcqe_entries = MMU_TOTAL_MCQE_ENTRIES;
    tot_rqe_entries = MMU_TOTAL_RQE_ENTRIES;
    tot_hdrm = 2 * jumbo_frame_cells;
    tot_port_min = 0;
    tot_pg_min = 0;
    pool_resume = 0;
    numq_uc = 0;
    numq_mc = 0;
    CDK_PBMP_ITER(mmu_pbmp, port) {
        pool_resume += 4;
        numq_uc += bcm56850_a0_mmu_port_uc_queues(unit, port);
        numq_mc += bcm56850_a0_mmu_port_mc_queues(unit, port);
    }
    numq_rqe = MMU_NUM_RQE;
    tot_rsvd = (numq_uc + numq_mc + numq_rqe) * MMU_BUFQ_MIN;
    tot_mcqe_rsvd = (numq_mc * MMU_MCQE_MIN);
    tot_rqe_rsvd = (numq_rqe * MMU_RQE_MIN);

    tot_shared = 0;
    if (tot_cells > tot_rsvd) {
        tot_shared = tot_cells - tot_rsvd;
    }
    CDK_VERB(("MMU total/reserved/shared %"PRIu32"/%"PRIu32"/%"PRIu32"\n",
              tot_cells, tot_rsvd, tot_shared));

    tot_mcqe_shared = 0;
    if (tot_mcqe_entries > tot_mcqe_rsvd) {
        tot_mcqe_shared = tot_mcqe_entries - tot_mcqe_rsvd;
    }
    CDK_VERB(("MCQ total/reserved/shared %"PRIu32"/%"PRIu32"/%"PRIu32"\n",
              tot_mcqe_entries, tot_mcqe_rsvd, tot_mcqe_shared));

    tot_rqe_shared = 0;
    if (tot_rqe_entries > tot_rqe_rsvd) {
        tot_rqe_shared = tot_rqe_entries - tot_rqe_rsvd;
    }
    CDK_VERB(("RQE total/reserved/shared %"PRIu32"/%"PRIu32"/%"PRIu32"\n",
              tot_rqe_entries, tot_rqe_rsvd, tot_rqe_shared));

    fval = tot_cells;
    CFAPFULLTHRESHOLDSETr_CLR(cfap_thr_set);
    CFAPFULLTHRESHOLDSETr_CFAPFULLSETPOINTf_SET(cfap_thr_set, fval);
    ioerr += WRITE_CFAPFULLTHRESHOLDSETr(unit, cfap_thr_set);

    fval -= tot_hdrm;
    CFAPFULLTHRESHOLDRESETr_CLR(cfap_thr_rst);
    CFAPFULLTHRESHOLDRESETr_CFAPFULLRESETPOINTf_SET(cfap_thr_rst, fval);
    ioerr += WRITE_CFAPFULLTHRESHOLDRESETr(unit, cfap_thr_rst);

    CFAP_ARBITER_MASKr_CLR(cfap_arb_mask);
    CFAP_ARBITER_MASKr_MASKf_SET(cfap_arb_mask, 10);
    ioerr += WRITE_CFAP_ARBITER_MASKr(unit, cfap_arb_mask);

    /* Global input thresholds */
    fval = tot_hdrm / 2;
    THDI_GLOBAL_HDRM_LIMIT_PIPEXr_CLR(g_hdrm_limit_x);
    THDI_GLOBAL_HDRM_LIMIT_PIPEXr_GLOBAL_HDRM_LIMITf_SET(g_hdrm_limit_x, fval);
    ioerr += WRITE_THDI_GLOBAL_HDRM_LIMIT_PIPEXr(unit, g_hdrm_limit_x);

    THDI_GLOBAL_HDRM_LIMIT_PIPEYr_CLR(g_hdrm_limit_y);
    THDI_GLOBAL_HDRM_LIMIT_PIPEYr_GLOBAL_HDRM_LIMITf_SET(g_hdrm_limit_y, fval);
    ioerr += WRITE_THDI_GLOBAL_HDRM_LIMIT_PIPEYr(unit, g_hdrm_limit_y);

    THDI_POOL_CONFIGr_CLR(pool_cfg);
    THDI_POOL_CONFIGr_PUBLIC_ENABLEf_SET(pool_cfg, 1);
    ioerr += WRITE_THDI_POOL_CONFIGr(unit, pool_cfg);

    fval = tot_cells - (tot_pg_min + tot_port_min + tot_hdrm);
    THDI_BUFFER_CELL_LIMIT_SPr_CLR(buf_lim_sp);
    THDI_BUFFER_CELL_LIMIT_SPr_LIMITf_SET(buf_lim_sp, fval);
    ioerr += WRITE_THDI_BUFFER_CELL_LIMIT_SPr(unit, 0, buf_lim_sp);
        
    THDI_CELL_RESET_LIMIT_OFFSET_SPr_CLR(rst_lim_off_sp);
    THDI_CELL_RESET_LIMIT_OFFSET_SPr_OFFSETf_SET(rst_lim_off_sp, pool_resume);
    ioerr += WRITE_THDI_CELL_RESET_LIMIT_OFFSET_SPr(unit, 0, rst_lim_off_sp);

    /* Global output thresholds (using only service pool 0) */
    OP_THDU_CONFIGr_CLR(op_cfg);
    OP_THDU_CONFIGr_ENABLE_QUEUE_AND_GROUP_TICKETf_SET(op_cfg, 1);
    OP_THDU_CONFIGr_ENABLE_UPDATE_COLOR_RESUMEf_SET(op_cfg, 1);
    OP_THDU_CONFIGr_MOP_POLICY_1Bf_SET(op_cfg, 1);
    ioerr += WRITE_OP_THDU_CONFIGr(unit, op_cfg);

    ioerr += READ_MMU_THDM_DB_DEVICE_THR_CONFIGr(unit, &dev_thr_cfg);
    MMU_THDM_DB_DEVICE_THR_CONFIGr_MOP_POLICYf_SET(dev_thr_cfg, 1);
    ioerr += WRITE_MMU_THDM_DB_DEVICE_THR_CONFIGr(unit, dev_thr_cfg);

    fval = tot_shared;
    MMU_THDM_DB_POOL_SHARED_LIMITr_SET(db_sh_lim, fval);
    ioerr += WRITE_MMU_THDM_DB_POOL_SHARED_LIMITr(unit, 0, db_sh_lim);

    MMU_THDM_DB_POOL_YELLOW_SHARED_LIMITr_SET(db_ysh_lim, fval/8);
    ioerr += WRITE_MMU_THDM_DB_POOL_YELLOW_SHARED_LIMITr(unit, 0, db_ysh_lim);

    MMU_THDM_DB_POOL_RED_SHARED_LIMITr_SET(db_rsh_lim, fval/8);
    ioerr += WRITE_MMU_THDM_DB_POOL_RED_SHARED_LIMITr(unit, 0, db_rsh_lim);

    MMU_THDM_DB_POOL_RESUME_LIMITr_SET(db_rsm_lim, fval/8);
    ioerr += WRITE_MMU_THDM_DB_POOL_RESUME_LIMITr(unit, 0, db_rsm_lim);

    MMU_THDM_DB_POOL_YELLOW_RESUME_LIMITr_SET(db_yrsm_lim, fval/8);
    ioerr += WRITE_MMU_THDM_DB_POOL_YELLOW_RESUME_LIMITr(unit, 0, db_yrsm_lim);

    MMU_THDM_DB_POOL_RED_RESUME_LIMITr_SET(db_rrsm_lim, fval/8);
    ioerr += WRITE_MMU_THDM_DB_POOL_RED_RESUME_LIMITr(unit, 0, db_rrsm_lim);

    fval = (tot_mcqe_shared + 3) / 4;
    MMU_THDM_MCQE_POOL_SHARED_LIMITr_SET(mcq_sh_lim, fval);
    ioerr += WRITE_MMU_THDM_MCQE_POOL_SHARED_LIMITr(unit, 0, mcq_sh_lim);

    fval = (tot_mcqe_shared + 7) / 8;
    MMU_THDM_MCQE_POOL_YELLOW_SHARED_LIMITr_SET(mcq_ysh_lim, fval);
    ioerr += WRITE_MMU_THDM_MCQE_POOL_YELLOW_SHARED_LIMITr(unit, 0, mcq_ysh_lim);
    MMU_THDM_MCQE_POOL_RED_SHARED_LIMITr_SET(mcq_rsh_lim, fval);
    ioerr += WRITE_MMU_THDM_MCQE_POOL_RED_SHARED_LIMITr(unit, 0, mcq_rsh_lim);
    MMU_THDM_MCQE_POOL_RESUME_LIMITr_SET(mcq_rsm_lim, fval);
    ioerr += WRITE_MMU_THDM_MCQE_POOL_RESUME_LIMITr(unit, 0, mcq_rsm_lim);
    MMU_THDM_MCQE_POOL_YELLOW_RESUME_LIMITr_SET(mcq_yrsm_lim, fval);
    ioerr += WRITE_MMU_THDM_MCQE_POOL_YELLOW_RESUME_LIMITr(unit, 0, mcq_yrsm_lim);
    MMU_THDM_MCQE_POOL_RED_RESUME_LIMITr_SET(mcq_rrsm_lim, fval);
    ioerr += WRITE_MMU_THDM_MCQE_POOL_RED_RESUME_LIMITr(unit, 0, mcq_rrsm_lim);

    MMU_THDR_DB_CONFIGr_CLR(db_cfg);
    MMU_THDR_DB_CONFIGr_CLEAR_DROP_STATE_ON_CONFIG_UPDATEf_SET(db_cfg, 1);
    MMU_THDR_DB_CONFIGr_MOP_POLICY_1Bf_SET(db_cfg, 1);
    ioerr += WRITE_MMU_THDR_DB_CONFIGr(unit, db_cfg);

    MMU_THDR_QE_CONFIGr_CLR(qe_cfg);
    MMU_THDR_QE_CONFIGr_CLEAR_DROP_STATE_ON_CONFIG_UPDATEf_SET(qe_cfg, 1);
    ioerr += WRITE_MMU_THDR_QE_CONFIGr(unit, qe_cfg);

    /* Per-port input thresholds */
    THDI_PORT_SP_CONFIG_Xm_CLR(sp_cfg_x);
    THDI_PORT_SP_CONFIG_Xm_PORT_SP_MIN_LIMITf_SET(sp_cfg_x, 0);
    THDI_PORT_SP_CONFIG_Xm_PORT_SP_MAX_LIMITf_SET(sp_cfg_x, tot_cells);
    fval = tot_cells - (2 * default_mtu_cells);
    THDI_PORT_SP_CONFIG_Xm_PORT_SP_RESUME_LIMITf_SET(sp_cfg_x, fval);
    CDK_MEMCPY(&sp_cfg_y, &sp_cfg_x, sizeof(sp_cfg_y));

    rval = MMU_BYTES_TO_CELLS(MMU_MAX_PKT_BYTES);
    THDI_PORT_MAX_PKT_SIZEr_SET(max_pkt, rval);
    ioerr += WRITE_THDI_PORT_MAX_PKT_SIZEr(unit, max_pkt);

    THDI_INPUT_PORT_XON_ENABLESr_CLR(xon_en);
    THDI_INPUT_PORT_XON_ENABLESr_INPUT_PORT_RX_ENABLEf_SET(xon_en, 1);

    THDI_PORT_PG_CONFIG_Xm_CLR(pg_cfg_x);
    THDI_PORT_PG_CONFIG_Xm_PG_MIN_LIMITf_SET(pg_cfg_x, jumbo_frame_cells);
    THDI_PORT_PG_CONFIG_Xm_PG_SHARED_LIMITf_SET(pg_cfg_x, 7);
    THDI_PORT_PG_CONFIG_Xm_PG_SHARED_DYNAMICf_SET(pg_cfg_x, 1);
    THDI_PORT_PG_CONFIG_Xm_PG_GBL_HDRM_ENf_SET(pg_cfg_x, 1);
    THDI_PORT_PG_CONFIG_Xm_PG_RESET_OFFSETf_SET(pg_cfg_x, default_mtu_cells * 2);
    CDK_MEMCPY(&pg_cfg_y, &pg_cfg_x, sizeof(pg_cfg_y));

    rval = 0;
    for (idx = 0; idx < 8; idx++) {
        /* Three bits per priority */
        rval |= MMU_DEFAULT_PG << (3 * idx);
    }
    THDI_PORT_PRI_GRPr_SET(port_pri_grp, rval);

    CDK_PBMP_ITER(mmu_pbmp, port) {
        mport = P2M(unit, port);
        idx = (mport & 0x3f) * MMU_NUM_POOL;
        if (PORT_IN_Y_PIPE(port)) {
            ioerr += WRITE_THDI_PORT_SP_CONFIG_Ym(unit, idx, sp_cfg_y);
        } else {
            ioerr += WRITE_THDI_PORT_SP_CONFIG_Xm(unit, idx, sp_cfg_x);
        }
        
        idx = (mport & 0x3f) * MMU_NUM_PG + 7;
        if (PORT_IN_Y_PIPE(port)) {
            ioerr += WRITE_THDI_PORT_PG_CONFIG_Ym(unit, idx, pg_cfg_y);
        } else {
            ioerr += WRITE_THDI_PORT_PG_CONFIG_Xm(unit, idx, pg_cfg_x);
        }
        
        ioerr += WRITE_THDI_INPUT_PORT_XON_ENABLESr(unit, mport, xon_en);
        ioerr += WRITE_THDI_PORT_PRI_GRPr(unit, mport, 0, port_pri_grp);
        ioerr += WRITE_THDI_PORT_PRI_GRPr(unit, mport, 1, port_pri_grp);
    }

    /* Per-port queue group output thresholds */
    MMU_THDU_XPIPE_CONFIG_QGROUPm_CLR(cfg_qgrp_x);
    fval = MMU_BUFQ_MIN;
    MMU_THDU_XPIPE_CONFIG_QGROUPm_Q_MIN_LIMIT_CELLf_SET(cfg_qgrp_x, fval);
    MMU_THDU_XPIPE_CONFIG_QGROUPm_Q_SHARED_ALPHA_CELLf_SET(cfg_qgrp_x, 7);
    MMU_THDU_XPIPE_CONFIG_QGROUPm_Q_LIMIT_DYNAMIC_CELLf_SET(cfg_qgrp_x, 1);
    MMU_THDU_XPIPE_CONFIG_QGROUPm_LIMIT_YELLOW_CELLf_SET(cfg_qgrp_x, 7);
    MMU_THDU_XPIPE_CONFIG_QGROUPm_LIMIT_RED_CELLf_SET(cfg_qgrp_x, 7);
    CDK_MEMCPY(&cfg_qgrp_y, &cfg_qgrp_x, sizeof(cfg_qgrp_y));

    MMU_THDU_XPIPE_OFFSET_QGROUPm_CLR(off_qgrp_x);
    MMU_THDU_XPIPE_OFFSET_QGROUPm_RESET_OFFSET_CELLf_SET(off_qgrp_x, 2);
    MMU_THDU_XPIPE_OFFSET_QGROUPm_RESET_OFFSET_YELLOW_CELLf_SET(off_qgrp_x, 2);
    MMU_THDU_XPIPE_OFFSET_QGROUPm_RESET_OFFSET_RED_CELLf_SET(off_qgrp_x, 2);
    CDK_MEMCPY(&off_qgrp_y, &off_qgrp_x, sizeof(off_qgrp_y));

    for (idx = 0; idx < MMU_THDU_XPIPE_CONFIG_QGROUPm_MAX; idx++) {
        ioerr += WRITE_MMU_THDU_XPIPE_CONFIG_QGROUPm(unit, idx, cfg_qgrp_x);
        ioerr += WRITE_MMU_THDU_YPIPE_CONFIG_QGROUPm(unit, idx, cfg_qgrp_y);
        ioerr += WRITE_MMU_THDU_XPIPE_OFFSET_QGROUPm(unit, idx, off_qgrp_x);
        ioerr += WRITE_MMU_THDU_YPIPE_OFFSET_QGROUPm(unit, idx, off_qgrp_y);
    }

    /* Per-port unicast queue output thresholds */
    MMU_THDU_XPIPE_CONFIG_QUEUEm_CLR(cfg_q_x);
    fval = MMU_BUFQ_MIN;
    MMU_THDU_XPIPE_CONFIG_QUEUEm_Q_MIN_LIMIT_CELLf_SET(cfg_q_x, fval);
    MMU_THDU_XPIPE_CONFIG_QUEUEm_Q_SHARED_ALPHA_CELLf_SET(cfg_q_x, 7);
    MMU_THDU_XPIPE_CONFIG_QUEUEm_Q_LIMIT_DYNAMIC_CELLf_SET(cfg_q_x, 1);
    MMU_THDU_XPIPE_CONFIG_QUEUEm_LIMIT_YELLOW_CELLf_SET(cfg_q_x, 7);
    MMU_THDU_XPIPE_CONFIG_QUEUEm_LIMIT_RED_CELLf_SET(cfg_q_x, 7);
    CDK_MEMCPY(&cfg_q_y, &cfg_q_x, sizeof(cfg_q_y));

    MMU_THDU_XPIPE_OFFSET_QUEUEm_CLR(off_q_x);
    MMU_THDU_XPIPE_OFFSET_QUEUEm_RESET_OFFSET_CELLf_SET(off_q_x, 2);
    MMU_THDU_XPIPE_OFFSET_QUEUEm_RESET_OFFSET_YELLOW_CELLf_SET(off_q_x, 2);
    MMU_THDU_XPIPE_OFFSET_QUEUEm_RESET_OFFSET_RED_CELLf_SET(off_q_x, 2);
    CDK_MEMCPY(&off_q_y, &off_q_x, sizeof(off_q_y));

    MMU_THDU_XPIPE_Q_TO_QGRP_MAPm_CLR(q2qgrp_x);
    MMU_THDU_XPIPE_Q_TO_QGRP_MAPm_QGROUP_VALIDf_SET(q2qgrp_x, 1);
    MMU_THDU_XPIPE_Q_TO_QGRP_MAPm_QGROUP_LIMIT_ENABLEf_SET(q2qgrp_x, 1);
    MMU_THDU_XPIPE_Q_TO_QGRP_MAPm_Q_LIMIT_ENABLEf_SET(q2qgrp_x, 1);
    CDK_MEMCPY(&q2qgrp_y, &q2qgrp_x, sizeof(q2qgrp_y));

    for (idx = 0; idx < MMU_THDU_XPIPE_CONFIG_QUEUEm_MAX; idx++) {
        ioerr += WRITE_MMU_THDU_XPIPE_CONFIG_QUEUEm(unit, idx, cfg_q_x);
        ioerr += WRITE_MMU_THDU_YPIPE_CONFIG_QUEUEm(unit, idx, cfg_q_y);
        ioerr += WRITE_MMU_THDU_XPIPE_OFFSET_QUEUEm(unit, idx, off_q_x);
        ioerr += WRITE_MMU_THDU_YPIPE_OFFSET_QUEUEm(unit, idx, off_q_y);
        ioerr += WRITE_MMU_THDU_XPIPE_Q_TO_QGRP_MAPm(unit, idx, q2qgrp_x);
        ioerr += WRITE_MMU_THDU_YPIPE_Q_TO_QGRP_MAPm(unit, idx, q2qgrp_y);
    }

    MMU_THDU_XPIPE_CONFIG_PORTm_CLR(pcfg_x);
    fval = tot_shared;
    MMU_THDU_XPIPE_CONFIG_PORTm_SHARED_LIMITf_SET(pcfg_x, fval);
    fval = (tot_shared + 7) / 8;
    MMU_THDU_XPIPE_CONFIG_PORTm_YELLOW_LIMITf_SET(pcfg_x, fval);
    MMU_THDU_XPIPE_CONFIG_PORTm_RED_LIMITf_SET(pcfg_x, fval);
    CDK_MEMCPY(&pcfg_y, &pcfg_x, sizeof(pcfg_y));

    fval = (tot_shared - (2 * default_mtu_cells) + 7) / 8;
    MMU_THDU_XPIPE_RESUME_PORTm_SHARED_RESUMEf_SET(prsm_x, fval);
    MMU_THDU_XPIPE_RESUME_PORTm_YELLOW_RESUMEf_SET(prsm_x, fval);
    MMU_THDU_XPIPE_RESUME_PORTm_RED_RESUMEf_SET(prsm_x, fval);
    CDK_MEMCPY(&prsm_y, &prsm_x, sizeof(prsm_y));

    for (idx = 0; idx < MMU_THDU_XPIPE_CONFIG_PORTm_MAX; idx += 4) {
        ioerr += WRITE_MMU_THDU_XPIPE_CONFIG_PORTm(unit, idx, pcfg_x);
        ioerr += WRITE_MMU_THDU_YPIPE_CONFIG_PORTm(unit, idx, pcfg_y);
        ioerr += WRITE_MMU_THDU_XPIPE_RESUME_PORTm(unit, idx, prsm_x);
        ioerr += WRITE_MMU_THDU_YPIPE_RESUME_PORTm(unit, idx, prsm_y);
    }

    /* Per-queue multicast output thresholds */
    MMU_THDM_DB_QUEUE_CONFIG_0m_CLR(db_q_cfg_x);
    fval = MMU_BUFQ_MIN;
    MMU_THDM_DB_QUEUE_CONFIG_0m_Q_MIN_LIMITf_SET(db_q_cfg_x, fval);
    MMU_THDM_DB_QUEUE_CONFIG_0m_Q_SHARED_ALPHAf_SET(db_q_cfg_x, 7);
    MMU_THDM_DB_QUEUE_CONFIG_0m_Q_LIMIT_DYNAMICf_SET(db_q_cfg_x, 1);
    MMU_THDM_DB_QUEUE_CONFIG_0m_Q_LIMIT_ENABLEf_SET(db_q_cfg_x, 1);
    MMU_THDM_DB_QUEUE_CONFIG_0m_Q_COLOR_LIMIT_DYNAMICf_SET(db_q_cfg_x, 1);
    MMU_THDM_DB_QUEUE_CONFIG_0m_YELLOW_SHARED_LIMITf_SET(db_q_cfg_x, 7);
    MMU_THDM_DB_QUEUE_CONFIG_0m_RED_SHARED_LIMITf_SET(db_q_cfg_x, 7);
    CDK_MEMCPY(&db_q_cfg_y, &db_q_cfg_x, sizeof(db_q_cfg_y));

    MMU_THDM_DB_QUEUE_OFFSET_0m_CLR(db_q_off_x);
    MMU_THDM_DB_QUEUE_OFFSET_0m_RESUME_OFFSETf_SET(db_q_off_x, 2);
    CDK_MEMCPY(&db_q_off_y, &db_q_off_x, sizeof(db_q_off_y));

    MMU_THDM_MCQE_QUEUE_CONFIG_0m_CLR(mcqe_q_cfg_x);
    fval = MMU_BUFQ_MIN / 4;
    MMU_THDM_MCQE_QUEUE_CONFIG_0m_Q_MIN_LIMITf_SET(mcqe_q_cfg_x, fval);
    MMU_THDM_MCQE_QUEUE_CONFIG_0m_Q_SHARED_ALPHAf_SET(mcqe_q_cfg_x, 7);
    MMU_THDM_MCQE_QUEUE_CONFIG_0m_Q_LIMIT_DYNAMICf_SET(mcqe_q_cfg_x, 1);
    MMU_THDM_MCQE_QUEUE_CONFIG_0m_Q_LIMIT_ENABLEf_SET(mcqe_q_cfg_x, 1);
    MMU_THDM_MCQE_QUEUE_CONFIG_0m_Q_COLOR_LIMIT_DYNAMICf_SET(mcqe_q_cfg_x, 1);
    MMU_THDM_MCQE_QUEUE_CONFIG_0m_YELLOW_SHARED_LIMITf_SET(mcqe_q_cfg_x, 7);
    MMU_THDM_MCQE_QUEUE_CONFIG_0m_RED_SHARED_LIMITf_SET(mcqe_q_cfg_x, 7);
    CDK_MEMCPY(&mcqe_q_cfg_y, &mcqe_q_cfg_x, sizeof(mcqe_q_cfg_y));

    MMU_THDM_MCQE_QUEUE_OFFSET_0m_CLR(mcqe_q_off_x);
    MMU_THDM_MCQE_QUEUE_OFFSET_0m_RESUME_OFFSETf_SET(mcqe_q_off_x, 1);
    CDK_MEMCPY(&mcqe_q_off_y, &mcqe_q_off_x, sizeof(mcqe_q_off_y));

    for (idx = 0; idx < MMU_THDM_DB_QUEUE_CONFIG_0m_MAX; idx++) {
        ioerr += WRITE_MMU_THDM_DB_QUEUE_CONFIG_0m(unit, idx, db_q_cfg_x);
        ioerr += WRITE_MMU_THDM_DB_QUEUE_CONFIG_1m(unit, idx, db_q_cfg_y);
        ioerr += WRITE_MMU_THDM_DB_QUEUE_OFFSET_0m(unit, idx, db_q_off_x);
        ioerr += WRITE_MMU_THDM_DB_QUEUE_OFFSET_1m(unit, idx, db_q_off_y);
        ioerr += WRITE_MMU_THDM_MCQE_QUEUE_CONFIG_0m(unit, idx, mcqe_q_cfg_x);
        ioerr += WRITE_MMU_THDM_MCQE_QUEUE_CONFIG_1m(unit, idx, mcqe_q_cfg_y);
        ioerr += WRITE_MMU_THDM_MCQE_QUEUE_OFFSET_0m(unit, idx, mcqe_q_off_x);
        ioerr += WRITE_MMU_THDM_MCQE_QUEUE_OFFSET_1m(unit, idx, mcqe_q_off_y);
    }

    MMU_THDM_DB_QUEUE_RESUME_OFFSET_PROFILE_YELLOWr_SET(db_yprof, 1);
    ioerr += WRITE_MMU_THDM_DB_QUEUE_RESUME_OFFSET_PROFILE_YELLOWr(unit, 0, db_yprof);

    MMU_THDM_DB_QUEUE_RESUME_OFFSET_PROFILE_REDr_SET(db_rprof, 1);
    ioerr += WRITE_MMU_THDM_DB_QUEUE_RESUME_OFFSET_PROFILE_REDr(unit, 0, db_rprof);

    MMU_THDM_MCQE_QUEUE_RESUME_OFFSET_PROFILE_YELLOWr_SET(mcqe_yprof, 1);
    ioerr += WRITE_MMU_THDM_MCQE_QUEUE_RESUME_OFFSET_PROFILE_YELLOWr(unit, 0, mcqe_yprof);

    MMU_THDM_MCQE_QUEUE_RESUME_OFFSET_PROFILE_REDr_SET(mcqe_rprof, 1);
    ioerr += WRITE_MMU_THDM_MCQE_QUEUE_RESUME_OFFSET_PROFILE_REDr(unit, 0, mcqe_rprof);

    /* Per-port/pool multicast output thresholds */
    MMU_THDM_DB_PORTSP_CONFIG_0m_CLR(db_psp_cfg_x);
    fval = tot_shared;
    MMU_THDM_DB_PORTSP_CONFIG_0m_SHARED_LIMITf_SET(db_psp_cfg_x, fval);
    fval = (tot_shared + 7) / 8;
    MMU_THDM_DB_PORTSP_CONFIG_0m_YELLOW_SHARED_LIMITf_SET(db_psp_cfg_x, fval);
    MMU_THDM_DB_PORTSP_CONFIG_0m_RED_SHARED_LIMITf_SET(db_psp_cfg_x, fval);
    fval = (tot_shared - (2 * default_mtu_cells) + 7) / 8;
    MMU_THDM_DB_PORTSP_CONFIG_0m_SHARED_RESUME_LIMITf_SET(db_psp_cfg_x, fval);
    MMU_THDM_DB_PORTSP_CONFIG_0m_YELLOW_RESUME_LIMITf_SET(db_psp_cfg_x, fval);
    MMU_THDM_DB_PORTSP_CONFIG_0m_RED_RESUME_LIMITf_SET(db_psp_cfg_x, fval);
    MMU_THDM_DB_PORTSP_CONFIG_0m_SHARED_LIMIT_ENABLEf_SET(db_psp_cfg_x, fval);
    CDK_MEMCPY(&db_psp_cfg_y, &db_psp_cfg_x, sizeof(db_psp_cfg_y));

    MMU_THDM_MCQE_PORTSP_CONFIG_0m_CLR(mcqe_psp_cfg_x);
    fval = (tot_mcqe_shared + 3) / 4;
    MMU_THDM_MCQE_PORTSP_CONFIG_0m_SHARED_LIMITf_SET(mcqe_psp_cfg_x, fval);
    fval = (fval / 2) - 1;
    MMU_THDM_MCQE_PORTSP_CONFIG_0m_SHARED_RESUME_LIMITf_SET(mcqe_psp_cfg_x, fval);
    fval = (tot_mcqe_shared + 7) / 8;
    MMU_THDM_MCQE_PORTSP_CONFIG_0m_YELLOW_SHARED_LIMITf_SET(mcqe_psp_cfg_x, fval);
    MMU_THDM_MCQE_PORTSP_CONFIG_0m_RED_SHARED_LIMITf_SET(mcqe_psp_cfg_x, fval);
    fval = fval - 1;
    MMU_THDM_MCQE_PORTSP_CONFIG_0m_YELLOW_RESUME_LIMITf_SET(mcqe_psp_cfg_x, fval);
    MMU_THDM_MCQE_PORTSP_CONFIG_0m_RED_RESUME_LIMITf_SET(mcqe_psp_cfg_x, fval);
    MMU_THDM_MCQE_PORTSP_CONFIG_0m_SHARED_LIMIT_ENABLEf_SET(mcqe_psp_cfg_x, 1);
    CDK_MEMCPY(&mcqe_psp_cfg_y, &mcqe_psp_cfg_x, sizeof(mcqe_psp_cfg_y));

    for (idx = 0; idx < MMU_THDM_DB_PORTSP_CONFIG_0m_MAX; idx++) {
        ioerr += WRITE_MMU_THDM_DB_PORTSP_CONFIG_0m(unit, idx, db_psp_cfg_x);
        ioerr += WRITE_MMU_THDM_DB_PORTSP_CONFIG_1m(unit, idx, db_psp_cfg_y);
        ioerr += WRITE_MMU_THDM_MCQE_PORTSP_CONFIG_0m(unit, idx, mcqe_psp_cfg_x);
        ioerr += WRITE_MMU_THDM_MCQE_PORTSP_CONFIG_1m(unit, idx, mcqe_psp_cfg_y);
    }

    /* Per-queue RQE multicast output thresholds */
    MMU_THDR_DB_LIMIT_MIN_PRIQr_CLR(db_min_pq);
    fval = MMU_BUFQ_MIN;
    MMU_THDR_DB_LIMIT_MIN_PRIQr_MIN_LIMITf_SET(db_min_pq, fval);

    MMU_THDR_DB_CONFIG_PRIQr_CLR(db_cfg_pq);
    MMU_THDR_DB_CONFIG_PRIQr_SHARED_ALPHAf_SET(db_cfg_pq, 7);
    MMU_THDR_DB_CONFIG_PRIQr_RESET_OFFSETf_SET(db_cfg_pq, 2);

    MMU_THDR_DB_CONFIG1_PRIQr_CLR(db_cfg1_pq);
    MMU_THDR_DB_CONFIG1_PRIQr_DYNAMIC_ENABLEf_SET(db_cfg1_pq, 1);
    MMU_THDR_DB_CONFIG1_PRIQr_LIMIT_ENABLEf_SET(db_cfg1_pq, 1);
    MMU_THDR_DB_CONFIG1_PRIQr_COLOR_LIMIT_DYNAMICf_SET(db_cfg1_pq, 1);

    MMU_THDR_DB_LIMIT_COLOR_PRIQr_CLR(db_lim_col_pq);
    MMU_THDR_DB_LIMIT_COLOR_PRIQr_SHARED_YELLOW_LIMITf_SET(db_lim_col_pq, 7);
    MMU_THDR_DB_LIMIT_COLOR_PRIQr_SHARED_RED_LIMITf_SET(db_lim_col_pq, 7);

    MMU_THDR_DB_RESET_OFFSET_COLOR_PRIQr_CLR(db_lcol_pq);
    MMU_THDR_DB_RESET_OFFSET_COLOR_PRIQr_RESET_OFFSET_YELLOWf_SET(db_lcol_pq, 2);
    MMU_THDR_DB_RESET_OFFSET_COLOR_PRIQr_RESET_OFFSET_REDf_SET(db_lcol_pq, 2);

    MMU_THDR_QE_LIMIT_MIN_PRIQr_CLR(qe_min_pq);
    fval = MMU_RQE_MIN / 8;
    MMU_THDR_QE_LIMIT_MIN_PRIQr_MIN_LIMITf_SET(qe_min_pq, fval);

    MMU_THDR_QE_CONFIG_PRIQr_CLR(qe_cfg_pq);
    fval = ((tot_rqe_shared / numq_rqe) + 7) / 8;
    MMU_THDR_QE_CONFIG_PRIQr_SHARED_LIMITf_SET(qe_cfg_pq, fval);
    MMU_THDR_QE_CONFIG_PRIQr_RESET_OFFSETf_SET(qe_cfg_pq, 1);

    MMU_THDR_QE_CONFIG1_PRIQr_CLR(qe_cfg1_pq);
    MMU_THDR_QE_CONFIG1_PRIQr_LIMIT_ENABLEf_SET(qe_cfg1_pq, 1);

    MMU_THDR_QE_LIMIT_COLOR_PRIQr_CLR(qe_lim_col_pq);
    fval = ((tot_rqe_shared / numq_rqe) + 7) / 8;
    MMU_THDR_QE_LIMIT_COLOR_PRIQr_SHARED_YELLOW_LIMITf_SET(qe_lim_col_pq, 7);
    MMU_THDR_QE_LIMIT_COLOR_PRIQr_SHARED_RED_LIMITf_SET(qe_lim_col_pq, 7);

    MMU_THDR_QE_RESET_OFFSET_COLOR_PRIQr_CLR(qe_lcol_pq);
    MMU_THDR_QE_RESET_OFFSET_COLOR_PRIQr_RESET_OFFSET_YELLOWf_SET(qe_lcol_pq, 1);
    MMU_THDR_QE_RESET_OFFSET_COLOR_PRIQr_RESET_OFFSET_REDf_SET(qe_lcol_pq, 1);

    for (idx = 0; idx < MMU_NUM_RQE; idx++) {
        ioerr += WRITE_MMU_THDR_DB_LIMIT_MIN_PRIQr(unit, idx, db_min_pq);
        ioerr += WRITE_MMU_THDR_DB_CONFIG_PRIQr(unit, idx, db_cfg_pq);
        ioerr += WRITE_MMU_THDR_DB_CONFIG1_PRIQr(unit, idx, db_cfg1_pq);
        ioerr += WRITE_MMU_THDR_DB_LIMIT_COLOR_PRIQr(unit, idx, db_lim_col_pq);
        ioerr += WRITE_MMU_THDR_DB_RESET_OFFSET_COLOR_PRIQr(unit, idx, db_lcol_pq);
        ioerr += WRITE_MMU_THDR_QE_LIMIT_MIN_PRIQr(unit, idx, qe_min_pq);
        ioerr += WRITE_MMU_THDR_QE_CONFIG_PRIQr(unit, idx, qe_cfg_pq);
        ioerr += WRITE_MMU_THDR_QE_CONFIG1_PRIQr(unit, idx, qe_cfg1_pq);
        ioerr += WRITE_MMU_THDR_QE_LIMIT_COLOR_PRIQr(unit, idx, qe_lim_col_pq);
        ioerr += WRITE_MMU_THDR_QE_RESET_OFFSET_COLOR_PRIQr(unit, idx, qe_lcol_pq);
    }

    /* Per-pool RQE multicast output thresholds */
    MMU_THDR_DB_CONFIG_SPr_CLR(db_cfg_sp);
    fval = tot_shared;
    MMU_THDR_DB_CONFIG_SPr_SHARED_LIMITf_SET(db_cfg_sp, fval);
    fval = (tot_shared - (2 * default_mtu_cells) + 7) / 8;
    MMU_THDR_DB_CONFIG_SPr_RESUME_LIMITf_SET(db_cfg_sp, fval);
    ioerr += WRITE_MMU_THDR_DB_CONFIG_SPr(unit, 0, db_cfg_sp);

    MMU_THDR_DB_SP_SHARED_LIMITr_CLR(db_lim_sp);
    fval = (tot_shared + 7) / 8;
    MMU_THDR_DB_SP_SHARED_LIMITr_SHARED_YELLOW_LIMITf_SET(db_lim_sp, fval);
    MMU_THDR_DB_SP_SHARED_LIMITr_SHARED_RED_LIMITf_SET(db_lim_sp, fval);
    ioerr += WRITE_MMU_THDR_DB_SP_SHARED_LIMITr(unit, 0, db_lim_sp);

    MMU_THDR_DB_RESUME_COLOR_LIMIT_SPr_CLR(db_rsmc_sp);
    fval = (tot_shared - (2 * default_mtu_cells) + 7) / 8;
    MMU_THDR_DB_RESUME_COLOR_LIMIT_SPr_RESUME_YELLOW_LIMITf_SET(db_rsmc_sp, fval);
    MMU_THDR_DB_RESUME_COLOR_LIMIT_SPr_RESUME_RED_LIMITf_SET(db_rsmc_sp, fval);
    ioerr += WRITE_MMU_THDR_DB_RESUME_COLOR_LIMIT_SPr(unit, 0, db_rsmc_sp);

    MMU_THDR_QE_CONFIG_SPr_CLR(qe_cfg_sp);
    fval = (tot_rqe_shared + 7) / 8;
    MMU_THDR_QE_CONFIG_SPr_SHARED_LIMITf_SET(qe_cfg_sp, fval);
    fval = fval - 1;
    MMU_THDR_QE_CONFIG_SPr_RESUME_LIMITf_SET(qe_cfg_sp, fval);
    ioerr += WRITE_MMU_THDR_QE_CONFIG_SPr(unit, 0, qe_cfg_sp);

    MMU_THDR_QE_SHARED_COLOR_LIMIT_SPr_CLR(qe_lim_sp);
    fval = (tot_rqe_shared + 7) / 8;
    MMU_THDR_QE_SHARED_COLOR_LIMIT_SPr_SHARED_YELLOW_LIMITf_SET(qe_lim_sp, fval);
    MMU_THDR_QE_SHARED_COLOR_LIMIT_SPr_SHARED_RED_LIMITf_SET(qe_lim_sp, fval);
    ioerr += WRITE_MMU_THDR_QE_SHARED_COLOR_LIMIT_SPr(unit, 0, qe_lim_sp);

    MMU_THDR_QE_RESUME_COLOR_LIMIT_SPr_CLR(qe_rsmc_sp);
    fval = fval - 1;
    MMU_THDR_QE_RESUME_COLOR_LIMIT_SPr_RESUME_YELLOW_LIMITf_SET(qe_rsmc_sp, fval);
    MMU_THDR_QE_RESUME_COLOR_LIMIT_SPr_RESUME_RED_LIMITf_SET(qe_rsmc_sp, fval);
    ioerr += WRITE_MMU_THDR_QE_RESUME_COLOR_LIMIT_SPr(unit, 0, qe_rsmc_sp);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

static int
_cosq_set_sched_parent(int unit, int port, int level, 
                       uint32_t hw_index, uint32_t parent_hw_index)
{
    int ioerr = 0;
    ES_PIPE0_LLS_L0_PARENTm_t pipe0_l0_parent;
    ES_PIPE1_LLS_L0_PARENTm_t pipe1_l0_parent;
    ES_PIPE0_LLS_L1_PARENTm_t pipe0_l1_parent;
    ES_PIPE1_LLS_L1_PARENTm_t pipe1_l1_parent;
    ES_PIPE0_LLS_L2_PARENTm_t pipe0_l2_parent;
    ES_PIPE1_LLS_L2_PARENTm_t pipe1_l2_parent;
    
    if (PORT_IN_Y_PIPE(port)) { 
        if (level == NODE_LEVEL_L2) {
            ioerr += READ_ES_PIPE1_LLS_L2_PARENTm(unit, hw_index, &pipe1_l2_parent);
            if (ES_PIPE1_LLS_L2_PARENTm_C_PARENTf_GET(pipe1_l2_parent) != parent_hw_index) {
                ES_PIPE1_LLS_L2_PARENTm_C_PARENTf_SET(pipe1_l2_parent, parent_hw_index);
                ioerr += WRITE_ES_PIPE1_LLS_L2_PARENTm(unit, hw_index, pipe1_l2_parent);
            }
        } else if (level == NODE_LEVEL_L1) {
            ioerr += READ_ES_PIPE1_LLS_L1_PARENTm(unit, hw_index, &pipe1_l1_parent);
            if (ES_PIPE1_LLS_L1_PARENTm_C_PARENTf_GET(pipe1_l1_parent) != parent_hw_index) {
                ES_PIPE1_LLS_L1_PARENTm_C_PARENTf_SET(pipe1_l1_parent, parent_hw_index);
                ioerr += WRITE_ES_PIPE1_LLS_L1_PARENTm(unit, hw_index, pipe1_l1_parent);
            }
        } else if (level == NODE_LEVEL_L0) {
            ioerr += READ_ES_PIPE1_LLS_L0_PARENTm(unit, hw_index, &pipe1_l0_parent);
            if (ES_PIPE1_LLS_L0_PARENTm_C_PARENTf_GET(pipe1_l0_parent) != parent_hw_index) {
                ES_PIPE1_LLS_L0_PARENTm_C_PARENTf_SET(pipe1_l0_parent, parent_hw_index);
                ioerr += WRITE_ES_PIPE1_LLS_L0_PARENTm(unit, hw_index, pipe1_l0_parent);
            }
        }
    } else {
        if (level == NODE_LEVEL_L2) {
            ioerr += READ_ES_PIPE0_LLS_L2_PARENTm(unit, hw_index, &pipe0_l2_parent);
            if (ES_PIPE0_LLS_L2_PARENTm_C_PARENTf_GET(pipe0_l2_parent) != parent_hw_index) {
                ES_PIPE0_LLS_L2_PARENTm_C_PARENTf_SET(pipe0_l2_parent, parent_hw_index);
                ioerr += WRITE_ES_PIPE0_LLS_L2_PARENTm(unit, hw_index, pipe0_l2_parent);
            }
        } else if (level == NODE_LEVEL_L1) {
            ioerr += READ_ES_PIPE0_LLS_L1_PARENTm(unit, hw_index, &pipe0_l1_parent);
            if (ES_PIPE0_LLS_L1_PARENTm_C_PARENTf_GET(pipe0_l1_parent) != parent_hw_index) {
                ES_PIPE0_LLS_L1_PARENTm_C_PARENTf_SET(pipe0_l1_parent, parent_hw_index);
                ioerr += WRITE_ES_PIPE0_LLS_L1_PARENTm(unit, hw_index, pipe0_l1_parent);
            }
        } else if (level == NODE_LEVEL_L0) {
            ioerr += READ_ES_PIPE0_LLS_L0_PARENTm(unit, hw_index, &pipe0_l0_parent);
            if (ES_PIPE0_LLS_L0_PARENTm_C_PARENTf_GET(pipe0_l0_parent) != parent_hw_index) {
                ES_PIPE0_LLS_L0_PARENTm_C_PARENTf_SET(pipe0_l0_parent, parent_hw_index);
                ioerr += WRITE_ES_PIPE0_LLS_L0_PARENTm(unit, hw_index, pipe0_l0_parent);
            }
        }
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

static int
_cosq_set_sched_child(int unit, int port, int level, int idx,
                                int num_spri, int first_sp_child, 
                                int first_sp_mc_child, uint32_t ucmap)
{
    int ioerr = 0;
    ES_PIPE0_LLS_PORT_MEMA_CONFIGm_t pipe0_port_mema_cfg;
    ES_PIPE1_LLS_PORT_MEMA_CONFIGm_t pipe1_port_mema_cfg;
    ES_PIPE0_LLS_L0_MEMA_CONFIGm_t pipe0_l0_mema_cfg;
    ES_PIPE1_LLS_L0_MEMA_CONFIGm_t pipe1_l0_mema_cfg;
    ES_PIPE0_LLS_L1_MEMA_CONFIGm_t pipe0_l1_mema_cfg;
    ES_PIPE1_LLS_L1_MEMA_CONFIGm_t pipe1_l1_mema_cfg;
    
    ES_PIPE0_LLS_PORT_MEMB_CONFIGm_t pipe0_port_memb_cfg;
    ES_PIPE1_LLS_PORT_MEMB_CONFIGm_t pipe1_port_memb_cfg;
    ES_PIPE0_LLS_L0_MEMB_CONFIGm_t pipe0_l0_memb_cfg;
    ES_PIPE1_LLS_L0_MEMB_CONFIGm_t pipe1_l0_memb_cfg;
    ES_PIPE0_LLS_L1_MEMB_CONFIGm_t pipe0_l1_memb_cfg;
    ES_PIPE1_LLS_L1_MEMB_CONFIGm_t pipe1_l1_memb_cfg;

    if (PORT_IN_Y_PIPE(port)) { 
        if (level == NODE_LEVEL_L1) {
            ioerr += READ_ES_PIPE1_LLS_L1_MEMA_CONFIGm(unit, idx, &pipe1_l1_mema_cfg);
            ES_PIPE1_LLS_L1_MEMA_CONFIGm_P_NUM_SPRIf_SET(pipe1_l1_mema_cfg, num_spri);
            ES_PIPE1_LLS_L1_MEMA_CONFIGm_P_START_UC_SPRIf_SET(pipe1_l1_mema_cfg, first_sp_child);
            ES_PIPE1_LLS_L1_MEMA_CONFIGm_P_START_MC_SPRIf_SET(pipe1_l1_mema_cfg, first_sp_mc_child - 1480);
            ES_PIPE1_LLS_L1_MEMA_CONFIGm_P_SPRI_SELECTf_SET(pipe1_l1_mema_cfg, (num_spri > 0) ? ucmap : 0);
            ioerr += WRITE_ES_PIPE1_LLS_L1_MEMA_CONFIGm(unit, idx, pipe1_l1_mema_cfg);
    
            ioerr += READ_ES_PIPE1_LLS_L1_MEMB_CONFIGm(unit, idx, &pipe1_l1_memb_cfg);
            ES_PIPE1_LLS_L1_MEMB_CONFIGm_P_NUM_SPRIf_SET(pipe1_l1_memb_cfg, num_spri);
            ES_PIPE1_LLS_L1_MEMB_CONFIGm_P_START_UC_SPRIf_SET(pipe1_l1_memb_cfg, first_sp_child);
            ES_PIPE1_LLS_L1_MEMB_CONFIGm_P_START_MC_SPRIf_SET(pipe1_l1_memb_cfg, first_sp_mc_child - 1480);
            ES_PIPE1_LLS_L1_MEMB_CONFIGm_P_SPRI_SELECTf_SET(pipe1_l1_memb_cfg, (num_spri > 0) ? ucmap : 0);
            ioerr += WRITE_ES_PIPE1_LLS_L1_MEMB_CONFIGm(unit, idx, pipe1_l1_memb_cfg);
        } else if (level == NODE_LEVEL_L0) {
            ioerr += READ_ES_PIPE1_LLS_L0_MEMA_CONFIGm(unit, idx, &pipe1_l0_mema_cfg);
            ES_PIPE1_LLS_L0_MEMA_CONFIGm_P_NUM_SPRIf_SET(pipe1_l0_mema_cfg, num_spri);
            ES_PIPE1_LLS_L0_MEMA_CONFIGm_P_START_SPRIf_SET(pipe1_l0_mema_cfg, first_sp_child);
            ioerr += WRITE_ES_PIPE1_LLS_L0_MEMA_CONFIGm(unit, idx, pipe1_l0_mema_cfg);
    
            ioerr += READ_ES_PIPE1_LLS_L0_MEMB_CONFIGm(unit, idx, &pipe1_l0_memb_cfg);
            ES_PIPE1_LLS_L0_MEMB_CONFIGm_P_NUM_SPRIf_SET(pipe1_l0_memb_cfg, num_spri);
            ES_PIPE1_LLS_L0_MEMB_CONFIGm_P_START_SPRIf_SET(pipe1_l0_memb_cfg, first_sp_child);
            ioerr += WRITE_ES_PIPE1_LLS_L0_MEMB_CONFIGm(unit, idx, pipe1_l0_memb_cfg);
        } else if (level == NODE_LEVEL_ROOT) {
            ioerr += READ_ES_PIPE1_LLS_PORT_MEMA_CONFIGm(unit, idx, &pipe1_port_mema_cfg);
            ES_PIPE1_LLS_PORT_MEMA_CONFIGm_P_NUM_SPRIf_SET(pipe1_port_mema_cfg, num_spri);
            ES_PIPE1_LLS_PORT_MEMA_CONFIGm_P_START_SPRIf_SET(pipe1_port_mema_cfg, first_sp_child);
            ioerr += WRITE_ES_PIPE1_LLS_PORT_MEMA_CONFIGm(unit, idx, pipe1_port_mema_cfg);
    
            ioerr += READ_ES_PIPE1_LLS_PORT_MEMB_CONFIGm(unit, idx, &pipe1_port_memb_cfg);
            ES_PIPE1_LLS_PORT_MEMB_CONFIGm_P_NUM_SPRIf_SET(pipe1_port_memb_cfg, num_spri);
            ES_PIPE1_LLS_PORT_MEMB_CONFIGm_P_START_SPRIf_SET(pipe1_port_memb_cfg, first_sp_child);
            ioerr += WRITE_ES_PIPE1_LLS_PORT_MEMB_CONFIGm(unit, idx, pipe1_port_memb_cfg);
        }
    } else {
        if (level == NODE_LEVEL_L1) {
            ioerr += READ_ES_PIPE0_LLS_L1_MEMA_CONFIGm(unit, idx, &pipe0_l1_mema_cfg);
            ES_PIPE0_LLS_L1_MEMA_CONFIGm_P_NUM_SPRIf_SET(pipe0_l1_mema_cfg, num_spri);
            ES_PIPE0_LLS_L1_MEMA_CONFIGm_P_START_UC_SPRIf_SET(pipe0_l1_mema_cfg, first_sp_child);
            ES_PIPE0_LLS_L1_MEMA_CONFIGm_P_START_MC_SPRIf_SET(pipe0_l1_mema_cfg, first_sp_mc_child - 1480);
            ES_PIPE0_LLS_L1_MEMA_CONFIGm_P_SPRI_SELECTf_SET(pipe0_l1_mema_cfg, (num_spri > 0) ? ucmap : 0);
            ioerr += WRITE_ES_PIPE0_LLS_L1_MEMA_CONFIGm(unit, idx, pipe0_l1_mema_cfg);
    
            ioerr += READ_ES_PIPE0_LLS_L1_MEMB_CONFIGm(unit, idx, &pipe0_l1_memb_cfg);
            ES_PIPE0_LLS_L1_MEMB_CONFIGm_P_NUM_SPRIf_SET(pipe0_l1_memb_cfg, num_spri);
            ES_PIPE0_LLS_L1_MEMB_CONFIGm_P_START_UC_SPRIf_SET(pipe0_l1_memb_cfg, first_sp_child);
            ES_PIPE0_LLS_L1_MEMB_CONFIGm_P_START_MC_SPRIf_SET(pipe0_l1_memb_cfg, first_sp_mc_child - 1480);
            ES_PIPE0_LLS_L1_MEMB_CONFIGm_P_SPRI_SELECTf_SET(pipe0_l1_memb_cfg, (num_spri > 0) ? ucmap : 0);
            ioerr += WRITE_ES_PIPE0_LLS_L1_MEMB_CONFIGm(unit, idx, pipe0_l1_memb_cfg);
        } else if (level == NODE_LEVEL_L0) {
            ioerr += READ_ES_PIPE0_LLS_L0_MEMA_CONFIGm(unit, idx, &pipe0_l0_mema_cfg);
            ES_PIPE0_LLS_L0_MEMA_CONFIGm_P_NUM_SPRIf_SET(pipe0_l0_mema_cfg, num_spri);
            ES_PIPE0_LLS_L0_MEMA_CONFIGm_P_START_SPRIf_SET(pipe0_l0_mema_cfg, first_sp_child);
            ioerr += WRITE_ES_PIPE0_LLS_L0_MEMA_CONFIGm(unit, idx, pipe0_l0_mema_cfg);
    
            ioerr += READ_ES_PIPE0_LLS_L0_MEMB_CONFIGm(unit, idx, &pipe0_l0_memb_cfg);
            ES_PIPE0_LLS_L0_MEMB_CONFIGm_P_NUM_SPRIf_SET(pipe0_l0_memb_cfg, num_spri);
            ES_PIPE0_LLS_L0_MEMB_CONFIGm_P_START_SPRIf_SET(pipe0_l0_memb_cfg, first_sp_child);
            ioerr += WRITE_ES_PIPE0_LLS_L0_MEMB_CONFIGm(unit, idx, pipe0_l0_memb_cfg);
        } else if (level == NODE_LEVEL_ROOT) {
            ioerr += READ_ES_PIPE0_LLS_PORT_MEMA_CONFIGm(unit, idx, &pipe0_port_mema_cfg);
            ES_PIPE0_LLS_PORT_MEMA_CONFIGm_P_NUM_SPRIf_SET(pipe0_port_mema_cfg, num_spri);
            ES_PIPE0_LLS_PORT_MEMA_CONFIGm_P_START_SPRIf_SET(pipe0_port_mema_cfg, first_sp_child);
            ioerr += WRITE_ES_PIPE0_LLS_PORT_MEMA_CONFIGm(unit, idx, pipe0_port_mema_cfg);
    
            ioerr += READ_ES_PIPE0_LLS_PORT_MEMB_CONFIGm(unit, idx, &pipe0_port_memb_cfg);
            ES_PIPE0_LLS_PORT_MEMB_CONFIGm_P_NUM_SPRIf_SET(pipe0_port_memb_cfg, num_spri);
            ES_PIPE0_LLS_PORT_MEMB_CONFIGm_P_START_SPRIf_SET(pipe0_port_memb_cfg, first_sp_child);
            ioerr += WRITE_ES_PIPE0_LLS_PORT_MEMB_CONFIGm(unit, idx, pipe0_port_memb_cfg);
        }
    }
    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

static int
_cosq_set_sched_mode(int unit, int port, int level, int idx,
                            _sched_mode_e mode, int weight)
{
    int ioerr = 0;
    ES_PIPE0_LLS_L0_CHILD_WEIGHT_CFGm_t pipe0_l0_weight;
    ES_PIPE1_LLS_L0_CHILD_WEIGHT_CFGm_t pipe1_l0_weight;
    ES_PIPE0_LLS_L1_CHILD_WEIGHT_CFGm_t pipe0_l1_weight;
    ES_PIPE1_LLS_L1_CHILD_WEIGHT_CFGm_t pipe1_l1_weight;
    ES_PIPE0_LLS_L2_CHILD_WEIGHT_CFGm_t pipe0_l2_weight;
    ES_PIPE1_LLS_L2_CHILD_WEIGHT_CFGm_t pipe1_l2_weight;
    ES_PIPE0_LLS_PORT_MEMA_CONFIGm_t pipe0_port_mema_cfg;
    ES_PIPE1_LLS_PORT_MEMA_CONFIGm_t pipe1_port_mema_cfg;
    ES_PIPE0_LLS_PORT_MEMB_CONFIGm_t pipe0_port_memb_cfg;
    ES_PIPE1_LLS_PORT_MEMB_CONFIGm_t pipe1_port_memb_cfg;
    
    if (PORT_IN_Y_PIPE(port)) { 
        if (level == NODE_LEVEL_L2) {
            ioerr += READ_ES_PIPE1_LLS_L2_CHILD_WEIGHT_CFGm(unit, idx, &pipe1_l2_weight);
            ES_PIPE1_LLS_L2_CHILD_WEIGHT_CFGm_C_WEIGHTf_SET(pipe1_l2_weight, weight);
            ioerr += WRITE_ES_PIPE1_LLS_L2_CHILD_WEIGHT_CFGm(unit, idx, pipe1_l2_weight);
        } else if (level == NODE_LEVEL_L1) {
            ioerr += READ_ES_PIPE1_LLS_L1_CHILD_WEIGHT_CFGm(unit, idx, &pipe1_l1_weight);
            ES_PIPE1_LLS_L1_CHILD_WEIGHT_CFGm_C_WEIGHTf_SET(pipe1_l1_weight, weight);
            ioerr += WRITE_ES_PIPE1_LLS_L1_CHILD_WEIGHT_CFGm(unit, idx, pipe1_l1_weight);
        } else if (level == NODE_LEVEL_L0) {
            ioerr += READ_ES_PIPE1_LLS_L0_CHILD_WEIGHT_CFGm(unit, idx, &pipe1_l0_weight);
            ES_PIPE1_LLS_L0_CHILD_WEIGHT_CFGm_C_WEIGHTf_SET(pipe1_l0_weight, weight);
            ioerr += WRITE_ES_PIPE1_LLS_L0_CHILD_WEIGHT_CFGm(unit, idx, pipe1_l0_weight);
        }

        ioerr += READ_ES_PIPE1_LLS_PORT_MEMA_CONFIGm(unit, idx, &pipe1_port_mema_cfg);
        ES_PIPE1_LLS_PORT_MEMA_CONFIGm_PACKET_MODE_WRR_ACCOUNTING_ENABLEf_SET(pipe1_port_mema_cfg, 1);
        ioerr += WRITE_ES_PIPE1_LLS_PORT_MEMA_CONFIGm(unit, idx, pipe1_port_mema_cfg);         

        ioerr += READ_ES_PIPE1_LLS_PORT_MEMB_CONFIGm(unit, idx, &pipe1_port_memb_cfg);
        ES_PIPE1_LLS_PORT_MEMB_CONFIGm_PACKET_MODE_WRR_ACCOUNTING_ENABLEf_SET(pipe1_port_memb_cfg, 1);
        ioerr += WRITE_ES_PIPE1_LLS_PORT_MEMB_CONFIGm(unit, idx, pipe1_port_memb_cfg);         
    } else {
        if (level == NODE_LEVEL_L2) {
            ioerr += READ_ES_PIPE0_LLS_L2_CHILD_WEIGHT_CFGm(unit, idx, &pipe0_l2_weight);
            ES_PIPE0_LLS_L2_CHILD_WEIGHT_CFGm_C_WEIGHTf_SET(pipe0_l2_weight, weight);
            ioerr += WRITE_ES_PIPE0_LLS_L2_CHILD_WEIGHT_CFGm(unit, idx, pipe0_l2_weight);
        } else if (level == NODE_LEVEL_L1) {
            ioerr += READ_ES_PIPE0_LLS_L1_CHILD_WEIGHT_CFGm(unit, idx, &pipe0_l1_weight);
            ES_PIPE0_LLS_L1_CHILD_WEIGHT_CFGm_C_WEIGHTf_SET(pipe0_l1_weight, weight);
            ioerr += WRITE_ES_PIPE0_LLS_L1_CHILD_WEIGHT_CFGm(unit, idx, pipe0_l1_weight);
        } else if (level == NODE_LEVEL_L0) {
            ioerr += READ_ES_PIPE0_LLS_L0_CHILD_WEIGHT_CFGm(unit, idx, &pipe0_l0_weight);
            ES_PIPE0_LLS_L0_CHILD_WEIGHT_CFGm_C_WEIGHTf_SET(pipe0_l0_weight, weight);
            ioerr += WRITE_ES_PIPE0_LLS_L0_CHILD_WEIGHT_CFGm(unit, idx, pipe0_l0_weight);
        }

        ioerr += READ_ES_PIPE0_LLS_PORT_MEMA_CONFIGm(unit, idx, &pipe0_port_mema_cfg);
        ES_PIPE0_LLS_PORT_MEMA_CONFIGm_PACKET_MODE_WRR_ACCOUNTING_ENABLEf_SET(pipe0_port_mema_cfg, 1);
        ioerr += WRITE_ES_PIPE0_LLS_PORT_MEMA_CONFIGm(unit, idx, pipe0_port_mema_cfg);         

        ioerr += READ_ES_PIPE0_LLS_PORT_MEMB_CONFIGm(unit, idx, &pipe0_port_memb_cfg);
        ES_PIPE0_LLS_PORT_MEMB_CONFIGm_PACKET_MODE_WRR_ACCOUNTING_ENABLEf_SET(pipe0_port_memb_cfg, 1);
        ioerr += WRITE_ES_PIPE0_LLS_PORT_MEMB_CONFIGm(unit, idx, pipe0_port_memb_cfg);         
    }
    
    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

static _port_lls_config_t *
_get_config_for_level(_port_lls_config_t *cfgtbl, int level,  int offset)
{
    _port_lls_config_t *table = cfgtbl;

    while (table->level != -1) {
        if ((table->level == level) && (table->node_id == offset)) {
            return table;
        }
        table++;
    }

    return NULL;
}

static int
_get_sched_count(int unit, int port, _port_lls_config_t *cfgtbl, int level)
{
    int count = 0;

    while (_get_config_for_level(cfgtbl, level, count)) {
        count++;
    }
    return count;
}

static int
_get_sched_hw_index(int unit, int in_port, _node_level_e level, 
                     int offset, uint32_t *hw_index)
{
    int tlm = -1, max = -1, cpu_tlm;
    int port;
    int base = 0;
    int empty, idx;
    uint32_t sched_bmap[32];
    cdk_pbmp_t pbmp;
        
    CDK_MEMSET(sched_bmap, 0, sizeof(uint32_t)*32);

    if (level == NODE_LEVEL_ROOT) {
        tlm = 1;
        max = 106;
    } else if (level == NODE_LEVEL_L0) {
        tlm = 5; 
        max = 272;
    } else if (level == NODE_LEVEL_L1) {
        tlm = 10;
        max = 1024;
    } else {
        return CDK_E_PARAM;
    }
        
    /* reserve all the scheduler and queus that are prefixed for hsp. */
    bcm56850_a0_all_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        if (PORT_IN_Y_PIPE(in_port) != PORT_IN_Y_PIPE(port)) {
            continue;
        }
    }
    
    if (level == NODE_LEVEL_L0) {
        cpu_tlm = _get_sched_count(unit, in_port, cpu_lls_config, level);
        if (cpu_tlm < CPU_RESERVED_L0_NUM) {
            cpu_tlm = CPU_RESERVED_L0_NUM;
        }
        base = CPU_RESERVED_L0_BASE;
        if (in_port == CMIC_PORT) {
            if (offset >= cpu_tlm) {
                return CDK_E_PARAM;
            }
            base += offset;
            sched_bmap[(base >> 5)] |= 1 << (base & 0x1f);
            *hw_index = base;
            return CDK_E_NONE;
        } else {
            /* reserve the CPU L0 nodes */
            for (idx = 0; idx < cpu_tlm; idx++) {
                sched_bmap[((base + idx) >> 5)] |= 1 << ((base + idx) & 0x1f);
            }
        }
        base = _invalid_parent[level];
        if (base == 0) {
            base = 53;
        }
        sched_bmap[(base >> 5)] |= (1 << (base & 0x1f));
    } else {
        base = _invalid_parent[level];
        sched_bmap[(base >> 5)] |= (1 << (base & 0x1f));
    }

    base = 0;
    bcm56850_a0_all_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        if (PORT_IN_Y_PIPE(in_port) != PORT_IN_Y_PIPE(port)) {
            continue;
        }
        
        tlm = _get_sched_count(unit, port,
                               (port == CMIC_PORT) ? cpu_lls_config : xlport_lls_config,
                               level);
        tlm = (tlm + 3) & ~3;

        /* step to empty resources start. */
        empty = 0;
        while (base < max) {
            if ((sched_bmap[(base >> 5)] & (1 << (base & 0x1f))) == 0) {
                empty += 1;
            } else {
                empty = 0;
            }

            base += 1;
            if (empty == tlm) {
                base -= tlm;
                break;
            }
        }

        if (base == max) {
            return CDK_E_RESOURCE;
        }

        if (port == in_port) {
            if (offset >= tlm) {
                return CDK_E_PARAM;
            }
            *hw_index = base + offset;
            return CDK_E_NONE;
        } else {
            for(idx = 0; idx < tlm; idx++) {
                sched_bmap[(base >> 5)] |= 1 << (base & 0x1f);
                base += 1;
            }
        }
    }
    
    return CDK_E_RESOURCE;
}

static int
_port_lls_init(int unit, int port, _port_lls_config_t *cfgtbl)
{
    _port_lls_config_t *pinfo, *cinfo;
    _sched_pending_t pending_list[64], *ppending;
    uint32_t ucmap;
    uint32_t hw_index;
    int list_size, list_index, uc;
    int l2_level_offset[2] = {0, 0};
    int level_offset[4] = {0, 0, 0, 0};
    int spri, smcpri, num_spri, qnum, numq, cidx;
    int child_num, child_level = 0;
    int child_type[NODE_LEVEL_MAX] = { NODE_LEVEL_L0, NODE_LEVEL_L1, NODE_LEVEL_L2, -1 };
    int mport = P2M(unit, port);
    int rv = CDK_E_NONE;
    
    /* setup port scheduler */
    pending_list[0].parent = -1;
    pending_list[0].level = NODE_LEVEL_ROOT;
    pending_list[0].offset = 0;
    pending_list[0].hw_index = (PORT_IN_Y_PIPE(port)) ? (mport - 64) : mport;

    list_size = 1;
    list_index = 0;
    do {
        ppending = &pending_list[list_index++];
        hw_index = ppending->hw_index;
        if (ppending->parent != -1) {
            /* attach to parent */
            rv = _cosq_set_sched_parent(unit, port, ppending->level, 
                                                hw_index, ppending->parent);
        }

        if (ppending->level == NODE_LEVEL_L2) {
            continue;
        }
        
        pinfo = _get_config_for_level(cfgtbl, 
                                   ppending->level, ppending->offset);
        if (!pinfo) {
            return CDK_E_INTERNAL;
        }
        
        child_level = -1;
        if ((ppending->level >= 0) && (ppending->level < NODE_LEVEL_MAX)) {
            child_level = child_type[ppending->level] ;
        }
        
        ucmap = 0;
        spri = -1;
        smcpri = -1;
        num_spri = 0;
        child_num = pinfo->child_num;
        for (cidx = 0; cidx < child_num; cidx++) {
            pending_list[list_size].parent = hw_index;
            pending_list[list_size].level = child_level;
            pending_list[list_size].offset = level_offset[child_level];
            level_offset[child_level] += 1;
            if (child_level == NODE_LEVEL_L2) {
                uc = 0; /* default MC queue */
                if (port != CMIC_PORT) {
                    uc = (pinfo->uc_mc_map & (1 << cidx)) ? 1 : 0;
                }

                if (uc) {
                    qnum = bcm56850_a0_uc_queue_num(unit, port, 0);
                    numq = bcm56850_a0_mmu_port_uc_queues(unit, port);
                    if ((numq == 0) || (qnum < 0)) {
                        continue;
                    }
                    
                    qnum -= (qnum >= 1480) ? 1480 : 0;
                } else {
                    qnum = bcm56850_a0_mc_queue_num(unit, port, 0);
                    numq = bcm56850_a0_mmu_port_mc_queues(unit, port);
                    if ((numq == 0) || (qnum < 0)) {
                        continue;
                    }
                    
                    qnum -= (qnum >= 568) ? 568 : 0;
                    qnum += 1480;
                }

                pending_list[list_size].hw_index = qnum + l2_level_offset[uc];
                l2_level_offset[uc]++;
                if (uc) {
                    if (spri == -1) {
                        spri = pending_list[list_size].hw_index;
                    }
                } else {
                    if (smcpri == -1) {
                        smcpri = pending_list[list_size].hw_index;
                    }
                }
                
                if ((pinfo->sched_mode == SCHED_MODE_STRICT) &&
                    (port == CMIC_PORT)) {
                    ucmap |= (uc) ? 0 : (1 << cidx);
                    num_spri++;
                }

                rv = _cosq_set_sched_parent(unit, port, NODE_LEVEL_L2, 
                                            pending_list[list_size].hw_index, 
                                            hw_index);

                rv = _cosq_set_sched_mode(unit, port, NODE_LEVEL_L2,
                                          pending_list[list_size].hw_index,
                                          pinfo->sched_mode, 1);
            } else {
                cinfo = _get_config_for_level(cfgtbl, child_level, 
                                          pending_list[list_size].offset);
                if (!cinfo) {
                    return CDK_E_INTERNAL;
                }
                
                if (_get_sched_hw_index(unit, port, child_level, 
                                        pending_list[list_size].offset, 
                                        &pending_list[list_size].hw_index)) {
                    return CDK_E_INTERNAL;
                }
                
                if (spri == -1) {
                    spri = pending_list[list_size].hw_index;
                }
                
                if (cinfo->sched_mode == SCHED_MODE_STRICT) {
                    num_spri++;
                }
                
                rv = _cosq_set_sched_parent(unit, port, child_level, 
                            pending_list[list_size].hw_index,
                            hw_index);

                rv = _cosq_set_sched_mode(unit, port, child_level,
                            pending_list[list_size].hw_index,
                            cinfo->sched_mode, 1);
                list_size++;
            }
        }

        if (spri == -1) {
            spri = 0;
        }
        if (smcpri == -1) {
            smcpri = 1480;
        }

        rv = _cosq_set_sched_child(unit, port, ppending->level, hw_index, 
                                            num_spri, spri, smcpri, ucmap);
    } while (list_index < list_size);

    return rv;
}


static int
_mmu_lls_init(int unit)
{
    int rv = CDK_E_NONE;
    cdk_pbmp_t pbmp;
    int port;

    /* Configure CPU port */
    rv = _port_lls_init(unit, CMIC_PORT, cpu_lls_config);
     
    /* Configure XLPORT and LB_PORT */
    bcm56850_a0_xlport_pbmp_get(unit, &pbmp);
    CDK_PBMP_PORT_ADD(pbmp, LB_PORT);
    CDK_PBMP_ITER(pbmp, port) {
        if (CDK_SUCCESS(rv)) {
            rv = _port_lls_init(unit, port, xlport_lls_config);
        }
    }
  
    return rv;
}

static int
_mmu_init(int unit)
{
    int ioerr = 0;
    int rv;
    ES_PIPE0_LLS_CONFIG0r_t lls_cfg0_x;
    ES_PIPE1_LLS_CONFIG0r_t lls_cfg0_y;
    PRIORITY_CONTROLr_t pri_ctrl;
    IP_TO_CMICM_CREDIT_TRANSFERr_t cred_xfer;
    THDU_OUTPUT_PORT_RX_ENABLE0_64r_t outp_rx_en_x;
    THDU_OUTPUT_PORT_RX_ENABLE1_64r_t outp_rx_en_y;
    MMU_THDM_DB_PORTSP_RX_ENABLE0_64r_t db_rx_en_x;
    MMU_THDM_DB_PORTSP_RX_ENABLE1_64r_t db_rx_en_y;
    MMU_THDM_MCQE_PORTSP_RX_ENABLE0_64r_t mcqe_rx_en_x;
    MMU_THDM_MCQE_PORTSP_RX_ENABLE1_64r_t mcqe_rx_en_y;

    rv = _mmu_config_init(unit);
    if (CDK_FAILURE(rv)) {
        return rv;
    }

    rv = _mmu_lls_init(unit);
    if (CDK_FAILURE(rv)) {
        return rv;
    }

    ES_PIPE0_LLS_CONFIG0r_CLR(lls_cfg0_x);
    ES_PIPE0_LLS_CONFIG0r_ENQUEUE_ENABLEf_SET(lls_cfg0_x, 1);
    ES_PIPE0_LLS_CONFIG0r_DEQUEUE_ENABLEf_SET(lls_cfg0_x, 1);
    CDK_MEMCPY(&lls_cfg0_y, &lls_cfg0_x, sizeof(lls_cfg0_y));
    ioerr += WRITE_ES_PIPE0_LLS_CONFIG0r(unit, lls_cfg0_x);
    ioerr += WRITE_ES_PIPE1_LLS_CONFIG0r(unit, lls_cfg0_y);

    PRIORITY_CONTROLr_CLR(pri_ctrl);
    PRIORITY_CONTROLr_USE_SC_FOR_MH_PRIf_SET(pri_ctrl, 1);
    PRIORITY_CONTROLr_USE_QM_FOR_MH_PRIf_SET(pri_ctrl, 1);
    ioerr += WRITE_PRIORITY_CONTROLr(unit, pri_ctrl);

    /* Enable IP to CMICM credit transfer */
    IP_TO_CMICM_CREDIT_TRANSFERr_CLR(cred_xfer);
    IP_TO_CMICM_CREDIT_TRANSFERr_TRANSFER_ENABLEf_SET(cred_xfer, 1);
    IP_TO_CMICM_CREDIT_TRANSFERr_NUM_OF_CREDITSf_SET(cred_xfer, 32);
    ioerr += WRITE_IP_TO_CMICM_CREDIT_TRANSFERr(unit, cred_xfer);

    /* Enable all ports */
    THDU_OUTPUT_PORT_RX_ENABLE0_64r_SET(outp_rx_en_x, 0, 0xffffffff);
    THDU_OUTPUT_PORT_RX_ENABLE0_64r_SET(outp_rx_en_x, 1, 0x001fffff);
    CDK_MEMCPY(&outp_rx_en_y, &outp_rx_en_x, sizeof(outp_rx_en_y));
    ioerr += WRITE_THDU_OUTPUT_PORT_RX_ENABLE0_64r(unit, outp_rx_en_x);
    ioerr += WRITE_THDU_OUTPUT_PORT_RX_ENABLE1_64r(unit, outp_rx_en_y);

    MMU_THDM_DB_PORTSP_RX_ENABLE0_64r_SET(db_rx_en_x, 0, 0xffffffff);
    MMU_THDM_DB_PORTSP_RX_ENABLE0_64r_SET(db_rx_en_x, 1, 0x001fffff);
    CDK_MEMCPY(&db_rx_en_y, &db_rx_en_x, sizeof(db_rx_en_y));
    ioerr += WRITE_MMU_THDM_DB_PORTSP_RX_ENABLE0_64r(unit, db_rx_en_x);
    ioerr += WRITE_MMU_THDM_DB_PORTSP_RX_ENABLE1_64r(unit, db_rx_en_y);

    MMU_THDM_MCQE_PORTSP_RX_ENABLE0_64r_SET(mcqe_rx_en_x, 0, 0xffffffff);
    MMU_THDM_MCQE_PORTSP_RX_ENABLE0_64r_SET(mcqe_rx_en_x, 1, 0x001fffff);
    CDK_MEMCPY(&mcqe_rx_en_y, &mcqe_rx_en_x, sizeof(mcqe_rx_en_y));
    ioerr += WRITE_MMU_THDM_MCQE_PORTSP_RX_ENABLE0_64r(unit, mcqe_rx_en_x);
    ioerr += WRITE_MMU_THDM_MCQE_PORTSP_RX_ENABLE1_64r(unit, mcqe_rx_en_y);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

static int
_firmware_helper(void *ctx, uint32_t offset, uint32_t size, void *data)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    XLPORT_WC_UCMEM_CTRLr_t ucmem_ctrl;
    XLPORT_WC_UCMEM_DATAm_t ucmem_data;
    int unit, port;
    const char *drv_name;
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

    if (CDK_STRSTR(drv_name, "tsc") == NULL) {
        return CDK_E_UNAVAIL;
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
            XLPORT_WC_UCMEM_DATAm_SET(ucmem_data, (wdx+2)&0x3, wdata);
        }
        WRITE_XLPORT_WC_UCMEM_DATAm(unit, idx >> 4, ucmem_data, port);
    }

    /* Disable parallel bus access */
    XLPORT_WC_UCMEM_CTRLr_ACCESS_MODEf_SET(ucmem_ctrl, 0);
    ioerr += WRITE_XLPORT_WC_UCMEM_CTRLr(unit, ucmem_ctrl, port);

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

int
bcm56850_a0_xport_init(int unit, int port)
{
    int ioerr = 0;
    XLMAC_TX_CTRLr_t tx_ctrl;
    XLMAC_RX_CTRLr_t rx_ctrl;
    XLMAC_RX_MAX_SIZEr_t rx_max_size;
    XLMAC_RX_LSS_CTRLr_t rx_lss_ctrl;
    XLMAC_CTRLr_t mac_ctrl;
    EGR_PORT_CREDIT_RESETm_t egr_port_credit_reset;
    
    /* Common port initialization */
    ioerr += _port_init(unit, port);

    /* Ensure that MAC (Rx) and loopback mode is disabled */
    XLMAC_CTRLr_CLR(mac_ctrl);
    XLMAC_CTRLr_SOFT_RESETf_SET(mac_ctrl, 1);
    ioerr += WRITE_XLMAC_CTRLr(unit, port, mac_ctrl);

    /* Reset EP credit before de-assert SOFT_RESET */
    ioerr += READ_EGR_PORT_CREDIT_RESETm(unit, port, &egr_port_credit_reset);
    EGR_PORT_CREDIT_RESETm_VALUEf_SET(egr_port_credit_reset, 1);
    ioerr += WRITE_EGR_PORT_CREDIT_RESETm(unit, port, egr_port_credit_reset);
    BMD_SYS_USLEEP(1000);
    ioerr += READ_EGR_PORT_CREDIT_RESETm(unit, port, &egr_port_credit_reset);
    EGR_PORT_CREDIT_RESETm_VALUEf_SET(egr_port_credit_reset, 0);
    ioerr += WRITE_EGR_PORT_CREDIT_RESETm(unit, port, egr_port_credit_reset);
 
    XLMAC_CTRLr_SOFT_RESETf_SET(mac_ctrl, 0);
    XLMAC_CTRLr_RX_ENf_SET(mac_ctrl, 0);
    XLMAC_CTRLr_TX_ENf_SET(mac_ctrl, 0);
    XLMAC_CTRLr_XGMII_IPG_CHECK_DISABLEf_SET(mac_ctrl, 
                (BMD_PORT_PROPERTIES(unit, port) == BMD_PORT_HG) ? 1 : 0) ;
    ioerr += WRITE_XLMAC_CTRLr(unit, port, mac_ctrl);

    /* Configure Rx (strip CRC, strict preamble, IEEE header) */
    ioerr += READ_XLMAC_RX_CTRLr(unit, port, &rx_ctrl);
    XLMAC_RX_CTRLr_STRIP_CRCf_SET(rx_ctrl, 0);
    if ((bcm56850_a0_port_speed_max(unit, port) >= 10000) && 
        (BMD_PORT_PROPERTIES(unit, port) == BMD_PORT_XE)) {
        XLMAC_RX_CTRLr_STRICT_PREAMBLEf_SET(rx_ctrl, 1);
    } else {
        XLMAC_RX_CTRLr_STRICT_PREAMBLEf_SET(rx_ctrl, 0);
    }
    ioerr += WRITE_XLMAC_RX_CTRLr(unit, port, rx_ctrl);

    /* Configure Tx (Inter-Packet-Gap, recompute CRC mode, IEEE header) */
    ioerr += READ_XLMAC_TX_CTRLr(unit, port, &tx_ctrl);
    XLMAC_TX_CTRLr_TX_PREAMBLE_LENGTHf_SET(tx_ctrl, 0x8);
    XLMAC_TX_CTRLr_PAD_THRESHOLDf_SET(tx_ctrl, 0x40);
    XLMAC_TX_CTRLr_AVERAGE_IPGf_SET(tx_ctrl, 0xc);
    XLMAC_TX_CTRLr_CRC_MODEf_SET(tx_ctrl, 0x2);
    ioerr += WRITE_XLMAC_TX_CTRLr(unit, port, tx_ctrl);

    /* Set max Rx frame size */
    XLMAC_RX_MAX_SIZEr_CLR(rx_max_size);
    XLMAC_RX_MAX_SIZEr_RX_MAX_SIZEf_SET(rx_max_size, JUMBO_MAXSZ);
    ioerr += WRITE_XLMAC_RX_MAX_SIZEr(unit, port, rx_max_size);

    ioerr += READ_XLMAC_RX_LSS_CTRLr(unit, port, &rx_lss_ctrl);
    XLMAC_RX_LSS_CTRLr_DROP_TX_DATA_ON_LOCAL_FAULTf_SET(rx_lss_ctrl, 1);
    XLMAC_RX_LSS_CTRLr_DROP_TX_DATA_ON_REMOTE_FAULTf_SET(rx_lss_ctrl, 1);
    ioerr += WRITE_XLMAC_RX_LSS_CTRLr(unit, port, rx_lss_ctrl);

    /* Disable loopback and bring XLMAC out of reset */
    XLMAC_CTRLr_LOCAL_LPBKf_SET(mac_ctrl, 0);
    XLMAC_CTRLr_RX_ENf_SET(mac_ctrl, 1);
    XLMAC_CTRLr_TX_ENf_SET(mac_ctrl, 1);
    ioerr += WRITE_XLMAC_CTRLr(unit, port, mac_ctrl);

    return ioerr;
}

int
bcm56850_a0_bmd_init(int unit)
{
    int ioerr = 0;
    int rv;
    ING_HW_RESET_CONTROL_1r_t ing_rst_ctl_1;
    ING_HW_RESET_CONTROL_2r_t ing_rst_ctl_2;
    ING_HW_RESET_CONTROL_2_Xr_t ing_rst_ctl_2_x;
    ING_HW_RESET_CONTROL_2_Yr_t ing_rst_ctl_2_y;
    EGR_HW_RESET_CONTROL_0r_t egr_rst_ctl_0;
    EGR_HW_RESET_CONTROL_1r_t egr_rst_ctl_1;
    EGR_HW_RESET_CONTROL_1_Xr_t egr_rst_ctl_1_x;
    EGR_HW_RESET_CONTROL_1_Yr_t egr_rst_ctl_1_y;
    EGR_VLAN_CONTROL_1r_t egr_vlan_ctrl_1;
    EGR_IPMC_CFG2r_t egr_ipmc_cfg2;
    EGR_PVLAN_EPORT_CONTROLr_t egr_pvlan_ctrl;
    SFLOW_ING_THRESHOLDr_t sflow_ing_threshold;
    STORM_CONTROL_METER_CONFIGr_t storm_ctrl;
    CPU_PBMm_t cpu_pbm;
    CPU_PBM_2m_t cpu_pbm_2;
    MULTIPASS_LOOPBACK_BITMAPm_t mpass_lb_bmap;
    ISBS_PORT_TO_PIPE_MAPPINGm_t isbs_map;
    ESBS_PORT_TO_PIPE_MAPPINGm_t esbs_map;
    EGR_ING_PORTm_t egr_ing_port;
    XLPORT_MODE_REGr_t xlport_mode;
    XLPORT_SOFT_RESETr_t xlport_reset;
    XLPORT_ENABLE_REGr_t xlport_en;
    XLPORT_MAC_CONTROLr_t xlp_mac_ctrl;
    PGW_MIB_RESETr_t mib_reset;
    CELL_ASM_0_CONTROLr_t cell_asm_ctrl;
    MISCCONFIGr_t misc_cfg;
    ING_EN_EFILTER_BITMAPm_t ing_en_efilter;
    L2_TABLE_HASH_CONTROLr_t l2_hash_ctrl;
    L3_TABLE_HASH_CONTROLr_t l3_hash_ctrl;
    L2_ENTRY_CONTROL_6r_t l2_ctrl6;
    SHARED_TABLE_HASH_CONTROLr_t shr_hash_ctrl;
    L2_AGE_DEBUGr_t l2_age_dbg;
    ING_CONFIG_64r_t ing_cfg;
    RTAG7_FLOW_BASED_HASHm_t r7_fb_hash;
    RTAG7_HASH_SELr_t r7_hash_sel;
    CMIC_RATE_ADJUSTr_t rate_adjust;
    CMIC_RATE_ADJUST_INT_MDIOr_t rate_adjust_int;
    RDBGC0_SELECTr_t rdbgc0_select;
    VLAN_PROFILE_TABm_t vlan_profile;
    ING_VLAN_TAG_ACTION_PROFILEm_t vlan_action;
    ING_COS_MODE_64r_t ing_cos_mode;
    cdk_pbmp_t pbmp;
    uint32_t pbm[PBM_PORT_WORDS];
    uint32_t xlport3_bw;
    uint32_t port_field = 0, port_mode, qgp_en;
    int rtag7_field_width[] = { 16, 16, 4, 16, 8, 8, 16, 16 };
    int freq, mdio_div;
    int port, lport, sub_port;
    int idx, pgw, sub_sel, offset, qnum;

    BMD_CHECK_UNIT(unit);

    /* Core clock frequency */
     _core_frequency = (CDK_XGSM_FLAGS(unit) & CHIP_FLAG_BW720G) ? 608 : 760;

    /* Start MMU memory initialization */
    ioerr += READ_MISCCONFIGr(unit, &misc_cfg);
    MISCCONFIGr_INIT_MEMf_SET(misc_cfg, 1);
    ioerr += WRITE_MISCCONFIGr(unit, misc_cfg);

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
    EGR_HW_RESET_CONTROL_1r_CLR(egr_rst_ctl_1);
    ioerr += WRITE_EGR_HW_RESET_CONTROL_0r(unit, egr_rst_ctl_0);
    EGR_HW_RESET_CONTROL_1r_RESET_ALLf_SET(egr_rst_ctl_1, 1);
    EGR_HW_RESET_CONTROL_1r_VALIDf_SET(egr_rst_ctl_1, 1);
    EGR_HW_RESET_CONTROL_1r_COUNTf_SET(egr_rst_ctl_1, 0xc000);
    ioerr += WRITE_EGR_HW_RESET_CONTROL_1r(unit, egr_rst_ctl_1);


    /* Wait for IPIPE memory initialization done. */
    for (idx = 0; idx < PIPE_RESET_TIMEOUT_MSEC; idx++) {
        ioerr += READ_ING_HW_RESET_CONTROL_2_Xr(unit, &ing_rst_ctl_2_x);
        ioerr += READ_ING_HW_RESET_CONTROL_2_Yr(unit, &ing_rst_ctl_2_y);
        if (ING_HW_RESET_CONTROL_2_Xr_DONEf_GET(ing_rst_ctl_2_x) && 
            ING_HW_RESET_CONTROL_2_Yr_DONEf_GET(ing_rst_ctl_2_y)) {
            break;
        }
        BMD_SYS_USLEEP(1000);
    }
    if (idx >= PIPE_RESET_TIMEOUT_MSEC) {
        CDK_WARN(("bcm56850_a0_bmd_init[%d]: ING_HW_RESET timeout\n", unit));
        return ioerr ? CDK_E_IO : CDK_E_TIMEOUT;
    }

    for (; idx < PIPE_RESET_TIMEOUT_MSEC; idx++) {
        ioerr += READ_EGR_HW_RESET_CONTROL_1_Xr(unit, &egr_rst_ctl_1_x);
        ioerr += READ_EGR_HW_RESET_CONTROL_1_Yr(unit, &egr_rst_ctl_1_y);
        if (EGR_HW_RESET_CONTROL_1_Xr_DONEf_GET(egr_rst_ctl_1_x) && 
            EGR_HW_RESET_CONTROL_1_Yr_DONEf_GET(egr_rst_ctl_1_y)) {
            break;
        }
        BMD_SYS_USLEEP(1000);
    }
    if (idx >= PIPE_RESET_TIMEOUT_MSEC) {
        CDK_WARN(("bcm56850_a0_bmd_init[%d]: EGR_HW_RESET timeout\n", unit));
        return ioerr ? CDK_E_IO : CDK_E_TIMEOUT;
    }


    /* Clear pipe reset registers */
    ING_HW_RESET_CONTROL_2r_CLR(ing_rst_ctl_2);
    ioerr += WRITE_ING_HW_RESET_CONTROL_2r(unit, ing_rst_ctl_2);
    EGR_HW_RESET_CONTROL_1r_CLR(egr_rst_ctl_1);
    ioerr += WRITE_EGR_HW_RESET_CONTROL_1r(unit, egr_rst_ctl_1);

    /* Clear registers that are implemented in memory */
    EGR_VLAN_CONTROL_1r_CLR(egr_vlan_ctrl_1);
    EGR_IPMC_CFG2r_CLR(egr_ipmc_cfg2);
    EGR_PVLAN_EPORT_CONTROLr_CLR(egr_pvlan_ctrl);
    STORM_CONTROL_METER_CONFIGr_CLR(storm_ctrl);
    SFLOW_ING_THRESHOLDr_CLR(sflow_ing_threshold);
    bcm56850_a0_xlport_pbmp_get(unit, &pbmp);
    CDK_PBMP_PORT_ADD(pbmp, CMIC_PORT);
    CDK_PBMP_ITER(pbmp, port) {
        lport = P2L(unit, port);
        ioerr += WRITE_EGR_VLAN_CONTROL_1r(unit, lport, egr_vlan_ctrl_1);
        ioerr += WRITE_EGR_IPMC_CFG2r(unit, lport, egr_ipmc_cfg2);
        ioerr += WRITE_EGR_PVLAN_EPORT_CONTROLr(unit, lport, egr_pvlan_ctrl);
        ioerr += WRITE_STORM_CONTROL_METER_CONFIGr(unit, lport, storm_ctrl);
        ioerr += WRITE_SFLOW_ING_THRESHOLDr(unit, lport, sflow_ing_threshold);
    }

    /* Initialize port mappings */
    ioerr += _port_map_init(unit);

    /* Setup TDM within each packet gateway */
    rv = _tdm_init(unit);
    if (CDK_FAILURE(rv)) {
        return rv;
    }

    /* Configure CPU port */
    CPU_PBMm_CLR(cpu_pbm);
    CPU_PBMm_SET(cpu_pbm, 0, 1);
    ioerr += WRITE_CPU_PBMm(unit, 0, cpu_pbm);
    CPU_PBM_2m_CLR(cpu_pbm_2);
    CPU_PBM_2m_SET(cpu_pbm_2, 0, 1);
    ioerr += WRITE_CPU_PBM_2m(unit, 0, cpu_pbm_2);

    /* Configure loopback ports (not used) */
    MULTIPASS_LOOPBACK_BITMAPm_CLR(mpass_lb_bmap);
    ioerr += WRITE_MULTIPASS_LOOPBACK_BITMAPm(unit, 0, mpass_lb_bmap);

    /* Configure logical ports belonging to Y-pipe */
    CDK_MEMSET(pbm, 0, sizeof(pbm));
    for (port = NUM_PHYS_PORTS/2; port < NUM_PHYS_PORTS; port++) {
        lport = P2L(unit, port);
        if (lport >= 0) {
            PBM_PORT_ADD(pbm, lport);
        }
    }
    ISBS_PORT_TO_PIPE_MAPPINGm_CLR(isbs_map);
    ISBS_PORT_TO_PIPE_MAPPINGm_BITMAPf_SET(isbs_map, pbm);
    WRITE_ISBS_PORT_TO_PIPE_MAPPINGm(unit, 0, isbs_map);
    ESBS_PORT_TO_PIPE_MAPPINGm_CLR(esbs_map);
    ESBS_PORT_TO_PIPE_MAPPINGm_BITMAPf_SET(esbs_map, pbm);
    WRITE_ESBS_PORT_TO_PIPE_MAPPINGm(unit, 0, esbs_map);

    /* Configure CPU HiGig ingress */
    EGR_ING_PORTm_CLR(egr_ing_port);
    EGR_ING_PORTm_PORT_TYPEf_SET(egr_ing_port, 1);
    WRITE_EGR_ING_PORTm(unit, CMIC_HG_LPORT, egr_ing_port);

    /* Initialize XLPORTs */
    bcm56850_a0_xlport_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        /* We only need to write once per block */
        if (XLPORT_SUBPORT(port) != 0) {
            continue;
        }
        /* Configure each sub port */
        port_field = 0;
        for (sub_port = 0; sub_port <= 3; sub_port++) {
            if (!CDK_PBMP_MEMBER(pbmp, port + sub_port)) {
                continue;
            }
            port_field |= (1 << sub_port);
        }
        
        /* Assert XLPORT soft reset */
        XLPORT_SOFT_RESETr_CLR(xlport_reset);
        XLPORT_SOFT_RESETr_SET(xlport_reset, port_field);
        ioerr += WRITE_XLPORT_SOFT_RESETr(unit, xlport_reset, port);

        if (port_field == 0xf) {
            port_mode = XPORT_MODE_QUAD;
        } else if (port_field == 0x5) {
            port_mode = XPORT_MODE_DUAL;
        } else if (port_field == 0x7) {
            port_mode = XPORT_MODE_TRI_012;
        } else if (port_field == 0xd) {
            port_mode = XPORT_MODE_TRI_023;
        } else {
            port_mode = XPORT_MODE_SINGLE;
        }

        XLPORT_MODE_REGr_CLR(xlport_mode);
        XLPORT_MODE_REGr_XPORT0_CORE_PORT_MODEf_SET(xlport_mode, port_mode);
        XLPORT_MODE_REGr_XPORT0_PHY_PORT_MODEf_SET(xlport_mode, port_mode);
        ioerr += WRITE_XLPORT_MODE_REGr(unit, xlport_mode, port);

        /* De-assert XLPORT soft reset */
        XLPORT_SOFT_RESETr_CLR(xlport_reset);
        ioerr += WRITE_XLPORT_SOFT_RESETr(unit, xlport_reset, port);

        /* Enable XLPORT */
        XLPORT_ENABLE_REGr_CLR(xlport_en);
        XLPORT_ENABLE_REGr_SET(xlport_en, port_field);
        ioerr += WRITE_XLPORT_ENABLE_REGr(unit, xlport_en, port);
    }

    /* Reset MIB counters in all blocks */
    for (pgw = 0; pgw < 8; pgw++) {
        PGW_MIB_RESETr_CLR(mib_reset);
        PGW_MIB_RESETr_CLR_CNTf_SET(mib_reset, 0xffff);
        ioerr += WRITE_PGW_MIB_RESETr(unit, pgw, mib_reset);
        PGW_MIB_RESETr_CLR_CNTf_SET(mib_reset, 0);
        ioerr += WRITE_PGW_MIB_RESETr(unit, pgw, mib_reset);
    }

    /* Enable QGPORT in XLPORT3 block (if present) */
    qgp_en = 0;
    xlport3_bw = 0;
    CDK_PBMP_ITER(pbmp, port) {
        if (XLPORT_BLKIDX(port) != 3) {
            continue;
        }
        xlport3_bw += bcm56850_a0_port_speed_max(unit, port);
        qgp_en |= (1 << XLPORT_SUBPORT(port));
    }
    if (xlport3_bw > 0 && xlport3_bw <= 10000) {
        CELL_ASM_0_CONTROLr_CLR(cell_asm_ctrl);
        CELL_ASM_0_CONTROLr_QGPORT_ENABLEf_SET(cell_asm_ctrl, qgp_en);
        ioerr += WRITE_CELL_ASM_0_CONTROLr(unit, 3, cell_asm_ctrl);
    }

    /* Enable Field Processor metering clock */
    ioerr += READ_MISCCONFIGr(unit, &misc_cfg);
    MISCCONFIGr_METERING_CLK_ENf_SET(misc_cfg, 1);
    ioerr += WRITE_MISCCONFIGr(unit, misc_cfg);

    /* Ensure that link bitmap is cleared */
    ioerr += CDK_XGSM_MEM_CLEAR(unit, EPC_LINK_BMAPm);

    /* Enable egress VLAN checks for all ports */
    bcm56850_a0_xlport_pbmp_get(unit, &pbmp);
    CDK_PBMP_PORT_ADD(pbmp, CMIC_PORT);
    CDK_MEMSET(pbm, 0, sizeof(pbm));
    CDK_PBMP_ITER(pbmp, port) {
        lport = P2L(unit, port);
        if (lport >= 0) {
            PBM_PORT_ADD(pbm, lport);
        }
    }
    ING_EN_EFILTER_BITMAPm_CLR(ing_en_efilter);
    ING_EN_EFILTER_BITMAPm_BITMAPf_SET(ing_en_efilter, pbm);
    ioerr += WRITE_ING_EN_EFILTER_BITMAPm(unit, 0, ing_en_efilter);

    /* Configure dual hash for L2 dedicated banks */
    ioerr += READ_L2_TABLE_HASH_CONTROLr(unit, &l2_hash_ctrl);
    L2_TABLE_HASH_CONTROLr_MODEf_SET(l2_hash_ctrl, 4);
    L2_TABLE_HASH_CONTROLr_BANK0_HASH_OFFSETf_SET(l2_hash_ctrl, 0);
    L2_TABLE_HASH_CONTROLr_BANK1_HASH_OFFSETf_SET(l2_hash_ctrl, 16);
    ioerr += WRITE_L2_TABLE_HASH_CONTROLr(unit, l2_hash_ctrl);

    /* Configure dual hash for L3 dedicated banks */
    ioerr += READ_L3_TABLE_HASH_CONTROLr(unit, &l3_hash_ctrl);
    L3_TABLE_HASH_CONTROLr_MODEf_SET(l3_hash_ctrl, 4);
    L3_TABLE_HASH_CONTROLr_BANK6_HASH_OFFSETf_SET(l3_hash_ctrl, 0);
    L3_TABLE_HASH_CONTROLr_BANK7_HASH_OFFSETf_SET(l3_hash_ctrl, 8);
    L3_TABLE_HASH_CONTROLr_BANK8_HASH_OFFSETf_SET(l3_hash_ctrl, 16);
    L3_TABLE_HASH_CONTROLr_BANK9_HASH_OFFSETf_SET(l3_hash_ctrl, 32);
    ioerr += WRITE_L3_TABLE_HASH_CONTROLr(unit, l3_hash_ctrl);

    /* Configure dual hash for L2/L3/ALPM shared banks */
    ioerr += READ_SHARED_TABLE_HASH_CONTROLr(unit, &shr_hash_ctrl);
    SHARED_TABLE_HASH_CONTROLr_BANK2_HASH_OFFSETf_SET(shr_hash_ctrl, 4);
    SHARED_TABLE_HASH_CONTROLr_BANK3_HASH_OFFSETf_SET(shr_hash_ctrl, 12);
    SHARED_TABLE_HASH_CONTROLr_BANK4_HASH_OFFSETf_SET(shr_hash_ctrl, 20);
    SHARED_TABLE_HASH_CONTROLr_BANK5_HASH_OFFSETf_SET(shr_hash_ctrl, 24);
    ioerr += WRITE_SHARED_TABLE_HASH_CONTROLr(unit, shr_hash_ctrl);

    /* Enable dual hash tables */
    ioerr += READ_L2_ENTRY_CONTROL_6r(unit, &l2_ctrl6);
    L2_ENTRY_CONTROL_6r_DISABLE_L2_ENTRY_LPf_SET(l2_ctrl6, 1);
    ioerr += WRITE_L2_ENTRY_CONTROL_6r(unit, l2_ctrl6);

    L2_AGE_DEBUGr_CLR(l2_age_dbg);
    L2_AGE_DEBUGr_AGE_COUNTf_SET(l2_age_dbg, L2Xm_MAX);
    ioerr += WRITE_L2_AGE_DEBUGr(unit, l2_age_dbg);

    /* Basic ingress configuration */
    ioerr += READ_ING_CONFIG_64r(unit, &ing_cfg);
    ING_CONFIG_64r_L3SRC_HIT_ENABLEf_SET(ing_cfg, 1);
    ING_CONFIG_64r_L2DST_HIT_ENABLEf_SET(ing_cfg, 1);
    ING_CONFIG_64r_APPLY_EGR_MASK_ON_L2f_SET(ing_cfg, 1);
    ING_CONFIG_64r_APPLY_EGR_MASK_ON_L3f_SET(ing_cfg, 1);
    ING_CONFIG_64r_ARP_RARP_TO_FPf_SET(ing_cfg, 0x3); /* Enable both ARP & RARP */
    ING_CONFIG_64r_ARP_VALIDATION_ENf_SET(ing_cfg, 1);
    ING_CONFIG_64r_IGNORE_HG_HDR_LAG_FAILOVERf_SET(ing_cfg, 1);
    ioerr += WRITE_ING_CONFIG_64r(unit, ing_cfg);

    /* Ensure that link bitmap is cleared */
    ioerr += CDK_XGSM_MEM_CLEAR(unit, EPC_LINK_BMAPm);

    /* Populate and enable RTAG7 Macro flow offset table */
    RTAG7_FLOW_BASED_HASHm_CLR(r7_fb_hash);
    offset = 0;
    sub_sel = 0;
    for (idx = 0; idx < RTAG7_FLOW_BASED_HASHm_MAX; idx++) {
        RTAG7_FLOW_BASED_HASHm_SUB_SEL_ECMPf_SET(r7_fb_hash, sub_sel);
        RTAG7_FLOW_BASED_HASHm_OFFSET_ECMPf_SET(r7_fb_hash, offset);
        ioerr += WRITE_RTAG7_FLOW_BASED_HASHm(unit, idx, r7_fb_hash);
        if (++offset >= rtag7_field_width[sub_sel]) {
            offset = 0;
            if (++sub_sel >= 8) {
                sub_sel = 0;
            }
        }
    }
    
    RTAG7_HASH_SELr_CLR(r7_hash_sel);
    RTAG7_HASH_SELr_USE_FLOW_SEL_ECMPf_SET(r7_hash_sel, 1);
    RTAG7_HASH_SELr_USE_FLOW_SEL_TRILL_ECMPf_SET(r7_hash_sel, 1);
    ioerr += WRITE_RTAG7_HASH_SELr(unit, r7_hash_sel);

    freq = _core_frequency;
    /*
     * Set external MDIO freq to around 6MHz
     * target_freq = core_clock_freq * DIVIDEND / DIVISOR / 2
     */
    mdio_div = (freq + (6 * 2 - 1)) / (6 * 2);
    CMIC_RATE_ADJUSTr_CLR(rate_adjust);
    CMIC_RATE_ADJUSTr_DIVISORf_SET(rate_adjust, mdio_div );
    CMIC_RATE_ADJUSTr_DIVIDENDf_SET(rate_adjust, 1);
    ioerr += WRITE_CMIC_RATE_ADJUSTr(unit, rate_adjust);

    /*
     * Set internal MDIO freq to around 12MHz
     * Valid range is from 2.5MHz to 20MHz
     * target_freq = core_clock_freq * DIVIDEND / DIVISOR / 2
     * or
     * DIVISOR = core_clock_freq * DIVIDENT / (target_freq * 2)
     */
    mdio_div = (freq + (12 * 2 - 1)) / (12 * 2);
    CMIC_RATE_ADJUST_INT_MDIOr_CLR(rate_adjust_int);
    CMIC_RATE_ADJUST_INT_MDIOr_DIVISORf_SET(rate_adjust_int, mdio_div);
    CMIC_RATE_ADJUST_INT_MDIOr_DIVIDENDf_SET(rate_adjust_int, 1);
    ioerr += WRITE_CMIC_RATE_ADJUST_INT_MDIOr(unit, rate_adjust_int);

    /* Configure discard counter */
    RDBGC0_SELECTr_CLR(rdbgc0_select);
    RDBGC0_SELECTr_BITMAPf_SET(rdbgc0_select, 0x0400ad11);
    ioerr += WRITE_RDBGC0_SELECTr(unit, rdbgc0_select);

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
    VLAN_PROFILE_TABm_IPMCV6_L2_ENABLEf_SET(vlan_profile, 1);
    VLAN_PROFILE_TABm_IPMCV4_L2_ENABLEf_SET(vlan_profile, 1);
    ioerr += WRITE_VLAN_PROFILE_TABm(unit, VLAN_PROFILE_TABm_MAX, vlan_profile);

    /* Ensure that all incoming packets get tagged appropriately */
    ING_VLAN_TAG_ACTION_PROFILEm_CLR(vlan_action);
    ING_VLAN_TAG_ACTION_PROFILEm_UT_OTAG_ACTIONf_SET(vlan_action, 1);
    ING_VLAN_TAG_ACTION_PROFILEm_SIT_PITAG_ACTIONf_SET(vlan_action, 3);
    ING_VLAN_TAG_ACTION_PROFILEm_SIT_OTAG_ACTIONf_SET(vlan_action, 1);
    ING_VLAN_TAG_ACTION_PROFILEm_SOT_POTAG_ACTIONf_SET(vlan_action, 2);
    ING_VLAN_TAG_ACTION_PROFILEm_DT_POTAG_ACTIONf_SET(vlan_action, 2);
    ioerr += WRITE_ING_VLAN_TAG_ACTION_PROFILEm(unit, 0, vlan_action);

    /* Probe PHYs */
    bcm56850_a0_xlport_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
    
        if (CDK_SUCCESS(rv)) {
            rv = bmd_phy_probe(unit, port);
        }

        if (XLPORT_SUBPORT(port) == 0) {
            port_field = 0;
            for (sub_port = 0; sub_port <= 3; sub_port++) {
                if (!CDK_PBMP_MEMBER(pbmp, port + sub_port)) {
                    continue;
                }
                port_field |= (1 << sub_port);
            }
        }
        if (port_field == 0xf) {
            rv = bmd_phy_mode_set(unit, port, "tsc", BMD_PHY_MODE_SERDES, 1);      
            rv = bmd_phy_mode_set(unit, port, "tsc", BMD_PHY_MODE_2LANE, 0);
        } else if (port_field == 0x5) {
            rv = bmd_phy_mode_set(unit, port, "tsc", BMD_PHY_MODE_SERDES, 1);
            rv = bmd_phy_mode_set(unit, port, "tsc", BMD_PHY_MODE_2LANE, 1);
        } else {
            rv = bmd_phy_mode_set(unit, port, "tsc", BMD_PHY_MODE_SERDES, 0);
        }

        if (CDK_SUCCESS(rv)) {
            rv = bmd_phy_fw_helper_set(unit, port, _firmware_helper);
        }

        if (CDK_SUCCESS(rv) && (XLPORT_SUBPORT(port) == 0)) {
            rv = bmd_phy_fw_base_set(unit, port, "tsc", 0xfb8);
        }
    }    
    
    if (CDK_SUCCESS(rv)) {
        rv = bmd_phy_staged_init(unit, &pbmp);
    }
    
    /* Configure XLPORTs */
    CDK_PBMP_ITER(pbmp, port) {
        /* Clear MAC hard reset after warpcore is initialized */
        if (XLPORT_SUBPORT(port) == 0) {
            XLPORT_MAC_CONTROLr_CLR(xlp_mac_ctrl);
            ioerr += WRITE_XLPORT_MAC_CONTROLr(unit, xlp_mac_ctrl, port);
        }
        /* Initialize XLPORTs after XLMAC is out of reset */
        ioerr += bcm56850_a0_xport_init(unit, port);
    }
    /* Common port initialization for CPU port */
    ioerr += _port_init(unit, CMIC_PORT);

    /* Assign queues to ports */
    bcm56850_a0_all_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        lport = P2L(unit, port);
        
        qnum = bcm56850_a0_uc_queue_num(unit, port, 0);
	    if (qnum >= 1480) {
	        qnum = (qnum - 1480) + 2048;
	    }
        
        ING_COS_MODE_64r_CLR(ing_cos_mode);
        ING_COS_MODE_64r_BASE_QUEUE_NUM_0f_SET(ing_cos_mode, qnum);
        ING_COS_MODE_64r_BASE_QUEUE_NUM_1f_SET(ing_cos_mode, qnum);
        if (BMD_PORT_PROPERTIES(unit, port) == BMD_PORT_HG) {
            ING_COS_MODE_64r_QUEUE_MODEf_SET(ing_cos_mode, 2);
        }
        ioerr += WRITE_ING_COS_MODE_64r(unit, lport, ing_cos_mode);
    }

#if BMD_CONFIG_INCLUDE_DMA
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
            cos_bmp = (idx == XGSM_DMA_RX_CHAN) ? 0xffff: 0;
            CMIC_CMC_COS_CTRL_RX_1r_COS_BMPf_SET(cos_ctrl_1, cos_bmp);
            ioerr += WRITE_CMIC_CMC_COS_CTRL_RX_1r(unit, idx, cos_ctrl_1);
        }

        if (ioerr) {
            return CDK_E_IO;
        }
    }
#endif /* BMD_CONFIG_INCLUDE_DMA */    

    return ioerr ? CDK_E_IO : rv;
}
#endif /* CDK_CONFIG_INCLUDE_BCM56850_A0 */
