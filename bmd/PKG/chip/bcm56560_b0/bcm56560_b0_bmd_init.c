/*
 *
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56560_B0 == 1

#include <bmd/bmd.h>
#include <bmd/bmd_dma.h>
#include <bmdi/arch/xgsd_dma.h>

#include <cdk/chip/bcm56560_b0_defs.h>
#include <cdk/cdk_util.h>
#include <cdk/cdk_debug.h>

#include "bcm56560_b0_bmd.h"
#include "bcm56560_b0_internal.h"
#include "bcm56560_b0_tdm.h"

#define PIPE_RESET_TIMEOUT_MSEC             5
#define JUMBO_MAXSZ                         0x3fe8
#define MMU_APACHE_MAX_PACKET_BYTES         12228 /* bytes */
#define MMU_MAVERICK_MAX_PACKET_BYTES       9416 /* bytes */
#define MMU_PKT_HDR_BYTES                   64    /* bytes */
#define MMU_JUMBO_FRAME_BYTES               9216  /* bytes */
#define MMU_DEFAULT_MTU_BYTES               1536  /* bytes */
#define MMU_BYTES_PER_CELL                  208   /* bytes (1664 bits) */
#define MMU_BYTES_TO_CELLS(_byte_)  \
        (((_byte_) + MMU_BYTES_PER_CELL - 1) / MMU_BYTES_PER_CELL)
#define MMU_NUM_RQE                         11
#define MMU_TOTAL_CELLS                     60495
#define MMU_GLOBAL_HEADROOM_CELL_PER_PIPE   50
#define MMU_MCQ_ENTRY                       31540
#define MMU_NUM_POOL                        4
#define MMU_NUM_PG                          8
#define MMU_BUFQ_MIN                        8
#define MMU_OVS_WT_GROUP_COUNT              4
#define MMU_CFG_QGROUP_MAX                  128
#define FW_ALIGN_BYTES                      16
#define FW_ALIGN_MASK                       (FW_ALIGN_BYTES - 1)
#define CMIC_NUM_PKT_DMA_CHAN               4
#define PGW_MASTER_COUNT                    4
#define PGW_TDM_SLOTS_PER_REG               4
#define PGW_TDM_LEN                         64
#define PGW_TDM_OVS_SIZE                    40
#define MMU_TDM_LEN                         512
#define MMU_OVS_GROUP_COUNT                 8
#define MMU_OVS_GROUP_TDM_LEN               16
#define IARB_TDM_LEN                        512
#define MAX_MMU_PORTS_PER_PIPE              52
#define MMU_XMIT_START_CNT_LINERATE(_freq, _speed) \
        ((_speed) > 42000 ? 10 : \
        (((_freq) <= 415) && ((_speed) > 21000) ? 8 : 7))

#define NUM_EXT_PORTS                       130
#define PORT_STATE_UNUSED                   0
#define PORT_STATE_LINERATE                 1
#define PORT_STATE_OVERSUB                  2
#define PORT_STATE_SUBPORT                  3

#define OVS_WT_GROUP_SPEED_NA               0
#define OVS_WT_GROUP_SPEED_5G               2
#define OVS_WT_GROUP_SPEED_10G              4
#define OVS_WT_GROUP_SPEED_20G              8
#define OVS_WT_GROUP_SPEED_25G             10
#define OVS_WT_GROUP_SPEED_40G             16
#define OVS_WT_GROUP_SPEED_50G             20
#define OVS_WT_GROUP_SPEED_100G            40

#define APACHE_PORT_CT_SPEED_NONE          0
#define APACHE_PORT_CT_SPEED_10M_FULL      1
#define APACHE_PORT_CT_SPEED_100M_FULL     2
#define APACHE_PORT_CT_SPEED_1000M_FULL    3
#define APACHE_PORT_CT_SPEED_2500M_FULL    4
#define APACHE_PORT_CT_SPEED_5000M_FULL    5
#define APACHE_PORT_CT_SPEED_10000M_FULL   6
#define APACHE_PORT_CT_SPEED_11000M_FULL   7
#define APACHE_PORT_CT_SPEED_20000M_FULL   8
#define APACHE_PORT_CT_SPEED_21000M_FULL   9
#define APACHE_PORT_CT_SPEED_25000M_FULL  10
#define APACHE_PORT_CT_SPEED_27000M_FULL  11
#define APACHE_PORT_CT_SPEED_40000M_FULL  12
#define APACHE_PORT_CT_SPEED_42000M_FULL  13
#define APACHE_PORT_CT_SPEED_50000M_FULL  14
#define APACHE_PORT_CT_SPEED_53000M_FULL  15
#define APACHE_PORT_CT_SPEED_100000M_FULL 16
#define APACHE_PORT_CT_SPEED_106000M_FULL 17

typedef struct tdm_b0_config_s {
    int     speed[NUM_EXT_PORTS];
    int     tdm_b0_bw;
    int     port_state[NUM_EXT_PORTS];
    int     pipe_ovs_state[2];
    int     manage_port_type; /* 0-none, 1-4x1g, 2-4x2.5g, 3-1x10g */
    int     pgw_tdm[PGW_MASTER_COUNT][PGW_TDM_LEN];
    int     pgw_ovs_tdm[PGW_MASTER_COUNT][PGW_TDM_OVS_SIZE];
    int     pgw_ovs_spacing[PGW_MASTER_COUNT][PGW_TDM_OVS_SIZE];
    int     mmu_tdm[MMU_TDM_LEN + 1];
    int     mmu_ovs_group_tdm[MMU_OVS_GROUP_COUNT][MMU_OVS_GROUP_TDM_LEN];
    int     iarb_tdm_b0_wrap_ptr;
    int     iarb_tdm[2][IARB_TDM_LEN];
} tdm_b0_config_t;

typedef enum {
    NODE_LVL_ROOT = 0,
    NODE_LVL_S1,
    NODE_LVL_L0,
    NODE_LVL_L1,
    NODE_LVL_L2,
    NODE_LVL_MAX
} _node_level_e;

static uint32_t
_total_bandwidth(int unit)
{
    cdk_pbmp_t pbmp;
    int port;
    uint32_t total_bw = 0;

    bcm56560_b0_all_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        total_bw += bcm56560_b0_port_speed_max(unit, port);
    }

    return total_bw;
}

static uint32_t
_core_bandwidth(int unit)
{
    uint32_t core_bandwidth;
    int core_freq = bcm56560_b0_get_core_frequency(unit);

    switch (core_freq) {
    case 841:
        core_bandwidth = 567000;
        break;
    case 793:
        core_bandwidth = 480000;
        break;
    case 575:
        core_bandwidth = 340000;
        break;
    default:
        core_bandwidth = 480000;
        break;
    }

    return core_bandwidth;
}

static int
_port_map_init(int unit)
{
    int ioerr = 0;
    int port, lport, mport;
    int num_pport = NUM_PHYS_PORTS;
    int num_lport = NUM_LOGIC_PORTS;
    ING_PHYS_TO_LOGIC_MAPm_t ing_p2l;
    IFP_GM_LOGICAL_TO_PHYSICAL_MAPPINGr_t ifp_l2p;
    EGR_LOGIC_TO_PHYS_MAPr_t egr_l2p;
    MMU_PORT_TO_PHY_PORT_MAPPINGr_t mmu_m2p;
    MMU_PORT_TO_LOGIC_PORT_MAPPINGr_t mmu_m2l;

    /* Ingress physical to logical port mapping */
    ING_PHYS_TO_LOGIC_MAPm_CLR(ing_p2l);
    for (port = 0; port < num_pport; port++) {
        lport = P2L(unit, port);
        if (lport < 0) {
            lport = 0x7f;
        }
        ING_PHYS_TO_LOGIC_MAPm_LOGICAL_PORT_NUMBERf_SET(ing_p2l, lport);
        ioerr += WRITE_ING_PHYS_TO_LOGIC_MAPm(unit, port, ing_p2l);
    }

    /* Ingress logical to physical port mapping */
    for (lport = 0; lport < num_lport; lport++) {
        port = L2P(unit, lport);
        if (port < 0) {
            continue;
        }
        mport = P2M(unit, port) & 0x7f;
        IFP_GM_LOGICAL_TO_PHYSICAL_MAPPINGr_CLR(ifp_l2p);
        IFP_GM_LOGICAL_TO_PHYSICAL_MAPPINGr_VALIDf_SET(ifp_l2p, 1);
        IFP_GM_LOGICAL_TO_PHYSICAL_MAPPINGr_PHYSICAL_PORT_NUMf_SET(ifp_l2p, mport);
        ioerr += WRITE_IFP_GM_LOGICAL_TO_PHYSICAL_MAPPINGr(unit, lport, ifp_l2p);
    }

    /* Egress logical to physical port mapping */
    EGR_LOGIC_TO_PHYS_MAPr_CLR(egr_l2p);
    for (lport = 0; lport < num_lport; lport++) {
        port = L2P(unit, lport);
        if (port < 0) {
            port = 0x7f;
        }
        EGR_LOGIC_TO_PHYS_MAPr_PHYSICAL_PORT_NUMBERf_SET(egr_l2p, port);
        ioerr += WRITE_EGR_LOGIC_TO_PHYS_MAPr(unit, lport, egr_l2p);
    }

    /* MMU to physical port mapping and MMU to logical port mapping */
    MMU_PORT_TO_PHY_PORT_MAPPINGr_CLR(mmu_m2p);
    MMU_PORT_TO_LOGIC_PORT_MAPPINGr_CLR(mmu_m2l);
    for (port = 0; port < num_pport; port++) {
        mport = P2M(unit, port);
        if (mport < 0) {
            continue;
        }
        lport = P2L(unit, port);
        if (lport < 0) {
            lport = 0x7f;
        }
        MMU_PORT_TO_PHY_PORT_MAPPINGr_PHY_PORTf_SET(mmu_m2p, port);
        ioerr += WRITE_MMU_PORT_TO_PHY_PORT_MAPPINGr(unit, mport, mmu_m2p);
        MMU_PORT_TO_LOGIC_PORT_MAPPINGr_LOGIC_PORTf_SET(mmu_m2l, lport);
        ioerr += WRITE_MMU_PORT_TO_LOGIC_PORT_MAPPINGr(unit, mport, mmu_m2l);
    }

    return ioerr;
}

int
bcm56560_b0_port_is_falcon(int unit, int port)
{

    if ((port >= 29 && port <= 36) ||
        (port >= 65 && port <= 72)) {
        return TRUE;
    }

    return FALSE;
}

static void
_mmu_ovs_speed_class_map_get(int unit, int speed, int *spg, uint32_t *sp_spacing)
{
    switch (speed) {
        case 5000:
            *sp_spacing = 0x14;
            *spg = OVS_WT_GROUP_SPEED_5G;
            break;
        case 10000:
            *sp_spacing = 0x14;
            *spg = OVS_WT_GROUP_SPEED_10G;
            break;
        case 20000:
            *sp_spacing = 0x0a;
            *spg = OVS_WT_GROUP_SPEED_20G;
            break;
        case 25000:
            *sp_spacing = 0x0a;
            *spg = OVS_WT_GROUP_SPEED_25G;
            break;
        case 40000:
            *sp_spacing = 0x0a;
            *spg = OVS_WT_GROUP_SPEED_40G;
            break;
        case 50000:
            *sp_spacing = 0x0a;
            *spg = OVS_WT_GROUP_SPEED_50G;
            break;
        case 100000:
            *sp_spacing = 0x04;
            *spg = OVS_WT_GROUP_SPEED_100G;
            break;
        default:
            *sp_spacing = 0x00;
            *spg = OVS_WT_GROUP_SPEED_NA;
            break;
    }

    return;
}

static int
_tdm_b0_speed_adjust(int unit, int speed)
{
    /* Round up low speeds to IEEE speeds */
    if (speed < 1000) {
        return 1000;
    }
    if (speed < 10000) {
        return 10000;
    }

    /* HiGig2 to IEEE bandwidth conversion */
    switch (speed) {
    case 11000:
        return 10000;
    case 21000:
        return 20000;
    case 27000:
        return 25000;
    case 42000:
    case 254500:
        return 40000;
    case 53000:
        return 50000;
    case 106000:
        return 100000;
    case 127000:
        return 120000;
    default:
        break;
    }

    /* Return remaining speeds unchanged */
    return speed;
}

static int
_tdm_b0_init(int unit)
{
    int ioerr = 0;
    int rv;
    cdk_pbmp_t all_pbmp;
    int phy_port;
    uint32_t speed;
    int idx;
    tdm_b0_soc_t tdm_b0_soc, *chip_pkg = &tdm_b0_soc;
    int chip_state[NUM_EXT_PORTS], chip_speed[NUM_EXT_PORTS];
    int slot;
    tdm_mod_t tdm_b0_mod, *tdm_b0_pkg = &tdm_b0_mod;
    int pgw, group;
    int pipe;
    int pgw_master;
    PGW_TDM_CONTROLr_t pgw_tdm_b0_ctrl;
    int count, base, base_idx;
    PGW_LR_TDM_REGr_t pgw_lr_tdm;
    PGW_OS_TDM_REGr_t pgw_os_tdm;
    PGW_OS_PORT_SPACING_REGr_t pgw_os_port_spacing;
    PGW_HSP_CONFIGr_t pgw_hsp_cfg;
    int length;
    ES_PIPE0_TDM_TABLE_0m_t es0_tdm_b0_tbl0;
    ES_PIPE0_TDM_TABLE_1m_t es0_tdm_b0_tbl1;
    int mmu_idle_ports[] = {76, 78, 79, 80}; /* IDLE MMU ports in Apache */
    int mmu_idle_port_ix = 0;
    int core_freq = bcm56560_b0_get_core_frequency(unit);
    int cur_cal = 0;
    ES_PIPE0_TDM_CONFIGr_t es0_tdm_b0_cfg;
    ES_PIPE0_OVR_SUB_GRP_TBLr_t es0_ovr_tbl;
    int speed_group;
    uint32_t speed_spacing;
    ES_PIPE0_OVR_SUB_GRP_CFGr_t es0_ovr_sub_grp_cfg;
    uint32_t wt_group;
    ES_PIPE0_OVR_SUB_GRP_WT0r_t es0_ovr_wt;
    int weight;
    ES_PIPE0_GRP_WT_SELECTr_t es0_grp_wt_sel;
    IARB_MAIN_TDMm_t iarb_main;
    int *tdm;
    IARB_TDM_CONTROLr_t iarb_tdm_b0_ctrl;
    IARB_OPP_SCH_CONTROLr_t iarb_opp_sch_ctrl;
    ENQ_CONFIGr_t enq_cfg;
    EGR_FORCE_SAF_CONFIGr_t egr_force_saf_cfg;
    ES_PIPE0_MMU_1DBG_Cr_t es0_mmu_1dbg_c;
    ES_PIPE0_MMU_DI_THRr_t es0_mmu_di_thr;
    ES_PIPE0_MMU_2DBG_C_1r_t es0_mmu_2dbg_c1;
    ES_PIPE0_MMU_2DBG_C_0r_t es0_mmu_2dbg_c0;
    ES_PIPE0_MMU_3DBG_Cr_t esp_mmu_3dbg_c;
    EDB_1DBG_Bm_t edb_1dbg_b;
    ES_PIPE0_MMU_1DBG_A_LOr_t es0_mmu_1dbg_a_lo;
    ES_PIPE0_MMU_1DBG_A_MIDr_t es0_mmu_1dbg_a_mid;
    ES_PIPE0_MMU_1DBG_A_HIr_t es0_mmu_1dbg_a_hi;
    uint32_t values[4];
    cdk_pbmp_t pbmp;
    int port, mport;
    int oversub;
    int val;
    uint32_t rval;
    tdm_b0_config_t tdm_b0_config, *tcfg = &tdm_b0_config;
    uint32_t bandwidth;
    int pgw_hsp_config_index[2] = {0, 0};    /* for each pgw */
    int pgw_hsp_ports[2][XLPS_PER_PGW] = {{0}, {0}};
    int ix;
    int pgw_cl;

    CDK_MEMSET(tcfg, 0, sizeof(*tcfg));
    bandwidth = _core_bandwidth(unit);
    oversub = (_total_bandwidth(unit) > bandwidth) ? 1 : 0;

    bcm56560_b0_all_pbmp_get(unit, &all_pbmp);
    CDK_PBMP_ITER(all_pbmp, port) {
        phy_port = port;

        speed = bcm56560_b0_port_speed_max(unit, phy_port);
        if (speed == 0) {
            continue;
        }

        tcfg->speed[phy_port] = speed;
        tcfg->speed[phy_port] = _tdm_b0_speed_adjust(unit, speed);
        if (oversub) {
            tcfg->port_state[phy_port] = PORT_STATE_OVERSUB;
        } else {
            tcfg->port_state[phy_port] = PORT_STATE_LINERATE;
        }

        if (bcm56560_b0_port_is_falcon(unit, phy_port)) { /*  Falcons  */
            if (tcfg->speed[phy_port] > 25000) {
                tcfg->port_state[phy_port + 1] = PORT_STATE_SUBPORT;
            }
            if (tcfg->speed[phy_port] > 50000) {
                tcfg->port_state[phy_port + 2] = PORT_STATE_SUBPORT;
                tcfg->port_state[phy_port + 3] = PORT_STATE_SUBPORT;
            }
        } else {                                        /*  Eagles  */
            if (tcfg->speed[phy_port] > 10000) {
                tcfg->port_state[phy_port + 1] = PORT_STATE_SUBPORT;
            }
            if (tcfg->speed[phy_port] > 20000) {
                tcfg->port_state[phy_port + 2] = PORT_STATE_SUBPORT;
                tcfg->port_state[phy_port + 3] = PORT_STATE_SUBPORT;
            }
            if (tcfg->speed[phy_port] > 40000) {
                tcfg->port_state[phy_port + 4] = PORT_STATE_SUBPORT;
                tcfg->port_state[phy_port + 5] = PORT_STATE_SUBPORT;
                tcfg->port_state[phy_port + 6] = PORT_STATE_SUBPORT;
                tcfg->port_state[phy_port + 7] = PORT_STATE_SUBPORT;
                tcfg->port_state[phy_port + 8] = PORT_STATE_SUBPORT;
                tcfg->port_state[phy_port + 9] = PORT_STATE_SUBPORT;
            }
        }
    }

    tcfg->speed[0] = 1000;
    tcfg->speed[73] = 1000;
    tcfg->speed[74] = 1000;
    tcfg->tdm_b0_bw = bandwidth / 1000;
    for (idx = 0; idx <= MMU_TDM_LEN; idx++) {
        tcfg->mmu_tdm[idx] = NUM_EXT_PORTS;
    }
    CDK_VVERB(("tdm_b0_bw: %dG\n", tcfg->tdm_b0_bw));
    CDK_VVERB(("port speed:"));
    for (idx = 0; idx < NUM_EXT_PORTS; idx++) {
        if (idx % 8 == 0) {
            CDK_VVERB(("\n    "));
        }
        CDK_VVERB((" %6d", tcfg->speed[idx]));
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
            CDK_VVERB((" %3d", tcfg->port_state[idx]));
        }
    }
    CDK_VVERB(("\n"));

    chip_pkg->unit = unit;
    chip_pkg->num_ext_ports = NUM_EXT_PORTS;
    chip_pkg->state = (enum port_state_e *)chip_state;
    chip_pkg->speed = (enum port_speed_e *)chip_speed;
    for (idx = 1; idx < NUM_EXT_PORTS; idx++) {
        chip_pkg->state[idx] = tcfg->port_state[idx];
    }
    chip_pkg->state[CMIC_LPORT] = 1; /* enable cpu port */
    chip_pkg->state[LB_LPORT] = 1; /* enable loopback port */
    chip_pkg->state[RDB_LPORT] = 1; /* enable RDB port */
    for (idx = 0; idx < NUM_EXT_PORTS; idx ++) {
        chip_pkg->speed[idx] = tcfg->speed[idx];
    }

    for (slot = 0; slot <= MMU_TDM_LEN; slot++) {
        tcfg->mmu_tdm[slot] = NUM_EXT_PORTS;
    }
    chip_pkg->clk_freq = (core_freq == 841) ? 840 : core_freq;
    bcm56560_b0_sel_tdm(chip_pkg, tdm_b0_pkg);
    if (NULL == bcm56560_b0_set_tdm_b0_tbl(tdm_b0_pkg)) {
        CDK_ERR(("bcm56560_b0_init[%d]: Unable to configure TDM\n", unit));
        return CDK_E_FAIL;
    }

    CDK_MEMCPY(tcfg->pgw_tdm[0], tdm_b0_pkg->_chip_data.cal_0.cal_main,
               sizeof(int) * PGW_TDM_LEN);
    CDK_MEMCPY(tcfg->pgw_ovs_tdm[0], tdm_b0_pkg->_chip_data.cal_0.cal_grp[0],
               sizeof(int) * PGW_TDM_OVS_SIZE);
    CDK_MEMCPY(tcfg->pgw_ovs_spacing[0], tdm_b0_pkg->_chip_data.cal_0.cal_grp[1],
               sizeof(int) * PGW_TDM_OVS_SIZE);
    CDK_MEMCPY(tcfg->pgw_tdm[1], tdm_b0_pkg->_chip_data.cal_1.cal_main,
               sizeof(int) * PGW_TDM_LEN);
    CDK_MEMCPY(tcfg->pgw_ovs_tdm[1], tdm_b0_pkg->_chip_data.cal_1.cal_grp[0],
               sizeof(int) * PGW_TDM_OVS_SIZE);
    CDK_MEMCPY(tcfg->pgw_ovs_spacing[1], tdm_b0_pkg->_chip_data.cal_1.cal_grp[1],
               sizeof(int) * PGW_TDM_OVS_SIZE);
    CDK_MEMCPY(tcfg->mmu_tdm, tdm_b0_pkg->_chip_data.cal_2.cal_main,
               sizeof(int) * MMU_TDM_LEN);
    for (idx = 0; idx < MMU_OVS_GROUP_COUNT; idx++) {
        CDK_MEMCPY(tcfg->mmu_ovs_group_tdm[idx],
                   tdm_b0_pkg->_chip_data.cal_2.cal_grp[idx],
                   sizeof(int) * MMU_OVS_GROUP_TDM_LEN);
    }

    for (pgw = 0; pgw < PGWS_PER_DEV; pgw += PGWS_PER_QUAD) {
        CDK_VVERB(("PGW_CL%d pgw_tdm:", pgw));
        for (idx = 0; idx < PGW_TDM_LEN; idx++) {
            if (idx % 16 == 0) {
                CDK_VVERB(("\n    "));
            }
            CDK_VVERB((" %3d", tcfg->pgw_tdm[pgw][idx]));
        }
        CDK_VVERB(("\n"));
        CDK_VVERB(("PGW_CL%d pgw_ovs_tdm:", pgw));
        for (idx = 0; idx < PGW_TDM_OVS_SIZE; idx++) {
            if (idx % 16 == 0) {
                CDK_VVERB(("\n    "));
            }
            CDK_VVERB((" %3d", tcfg->pgw_ovs_tdm[pgw][idx]));
        }
        CDK_VVERB(("\n"));
        CDK_VVERB(("PGW_CL%d pgw_ovs_spacing:", pgw));
        for (idx = 0; idx < PGW_TDM_OVS_SIZE; idx++) {
            if (idx % 16 == 0) {
                CDK_VVERB(("\n    "));
            }
            CDK_VVERB((" %3d",
                      tcfg->pgw_ovs_spacing[pgw][idx]));
        }
        CDK_VVERB(("\n"));
    }
    CDK_VVERB(("mmu_tdm:"));
    for (idx = 0; idx < MMU_TDM_LEN; idx++) {
        if (idx % 16 == 0) {
            CDK_VVERB(("\n    "));
        }
        CDK_VVERB((" %3d", tcfg->mmu_tdm[idx]));
    }
    CDK_VVERB(("\n"));
    for (group = 0; group < MMU_OVS_GROUP_COUNT; group++) {
        CDK_VVERB(("group %d ovs_group_tdm"));
        for (idx = 0; idx < MMU_OVS_GROUP_TDM_LEN; idx++) {
            if (idx % 16 == 0) {
                CDK_VVERB(("\n    "));
            }
            CDK_VVERB((" %3d",
                       tcfg->mmu_ovs_group_tdm[group][idx]));
        }
        CDK_VVERB(("\n"));
    }

    CDK_VVERB(("tdm_b0_bw: %dG\n", tcfg->tdm_b0_bw));
    CDK_VVERB(("ovs state: %d\n", tcfg->pipe_ovs_state[0]));

    rv = bcm56560_b0_set_iarb_tdm_b0_table(tcfg->tdm_b0_bw,
                                           tcfg->pipe_ovs_state[0],
                                           &tcfg->iarb_tdm_b0_wrap_ptr,
                                           tcfg->iarb_tdm[0]);
    if (rv == 0) {
        CDK_ERR(("bcm56560_b0_init[%d]: Unable to configure IARB TDM\n", unit));
        return CDK_E_FAIL;
    }

    for (pipe = 0; pipe < PIPES_PER_DEV; pipe++) {
        CDK_VVERB(("Pipe %c iarb_tdm: (wrap_ptr %d)",
                   pipe ? 'y' : 'x',
                   tcfg->iarb_tdm_b0_wrap_ptr));
        for (idx = 0; idx < IARB_TDM_LEN; idx++) {
            if (idx > tcfg->iarb_tdm_b0_wrap_ptr) {
                break;
            }
            if (idx % 16 == 0) {
                CDK_VVERB(("\n    "));
            }
            CDK_VVERB((" %3d", tcfg->iarb_tdm[pipe][idx]));
        }
        CDK_VVERB(("\n"));
    }

    /* Configure PGW TDM, only need to program master PGWs*/
    for (pgw = 0; pgw < PGWS_PER_DEV; pgw += PGWS_PER_QUAD) {

        pgw_master = pgw;

        ioerr += READ_PGW_TDM_CONTROLr(unit, pgw_master, &pgw_tdm_b0_ctrl);
        /* Configure PGW line rate ports TDM */
        count = 0;
        for (base = 0; base < PGW_TDM_LEN; base += PGW_TDM_SLOTS_PER_REG) {
            PGW_LR_TDM_REGr_CLR(pgw_lr_tdm);
            base_idx = base / PGW_TDM_SLOTS_PER_REG;
            for (idx = 0; idx < PGW_TDM_SLOTS_PER_REG; idx++) {
                slot = base + idx;
                port = tcfg->pgw_tdm[pgw_master][slot];
                if (port == NUM_EXT_PORTS) {
                    port = 0xff;
                    if (base % PGW_TDM_SLOTS_PER_REG) {
                        break;
                    }
                } else {
                    if (port == 75) {
                        port = 0x4b;
                    }
                }
                rval = PGW_LR_TDM_REGr_GET(pgw_lr_tdm, base_idx);
                rval |= (port << (idx << 3));
                PGW_LR_TDM_REGr_SET(pgw_lr_tdm, 0, rval);
                count++;
            }
            if (((slot + 1) % PGW_TDM_SLOTS_PER_REG) == 0) {
                ioerr += WRITE_PGW_LR_TDM_REGr(unit, pgw_master, base_idx,
                                               pgw_lr_tdm);
            }
        }

        if (count > 0) {
            if (!cur_cal) {
                PGW_TDM_CONTROLr_LR_TDM_WRAP_PTRf_SET(pgw_tdm_b0_ctrl, count - 1);
            } else {
                PGW_TDM_CONTROLr_LR_TDM2_WRAP_PTRf_SET(pgw_tdm_b0_ctrl, count - 1);
            }
        }
        PGW_TDM_CONTROLr_LR_TDM_SELf_SET(pgw_tdm_b0_ctrl, cur_cal);
        /* Always set both LR_TDM_ENABLE and LR_TDM2_ENABLE.
         * Even when switching, we need to make sure the the current
         * calendar is active so that it services the slots until the
         * end of the calendar when it actually switches to the new one.
         */
        PGW_TDM_CONTROLr_LR_TDM_ENABLEf_SET(pgw_tdm_b0_ctrl, 1);
        PGW_TDM_CONTROLr_LR_TDM2_ENABLEf_SET(pgw_tdm_b0_ctrl, 1);

        /* Configure PGW oversubscription ports TDM */
        count = 0;
        for (base = 0; base < PGW_TDM_OVS_SIZE; base += PGW_TDM_SLOTS_PER_REG) {
            PGW_OS_TDM_REGr_CLR(pgw_os_tdm);
            base_idx = base / PGW_TDM_SLOTS_PER_REG;
            for (idx = 0; idx < PGW_TDM_SLOTS_PER_REG; idx++) {
                slot = base + idx;
                port = tcfg->pgw_ovs_tdm[pgw_master][slot];

                if (port == NUM_EXT_PORTS) {
                    port = 0xff;
                } else {
                    count++;
                }
                rval = PGW_OS_TDM_REGr_GET(pgw_os_tdm, 0);
                rval |= (port << (idx << 3));
                PGW_OS_TDM_REGr_SET(pgw_os_tdm, 0, rval);
            }
            if (((slot + 1) % PGW_TDM_SLOTS_PER_REG) == 0) {
                ioerr += WRITE_PGW_OS_TDM_REGr(unit, pgw_master, base_idx,
                                           pgw_os_tdm);
            }
            if (bcm56560_b0_port_speed_max(unit, port) >= 40000) {
                for (ix = 0; ix < pgw_hsp_config_index[pgw]; ix++) {
                    if (pgw_hsp_ports[pgw][ix] == port) {
                        break;
                    }
                }
                if (ix < pgw_hsp_config_index[pgw]) {
                    continue;
                }
                for (pgw_cl = 0; pgw_cl < 2; pgw_cl++) {
                    ioerr += READ_PGW_HSP_CONFIGr(unit, pgw_cl,
                        pgw_hsp_config_index[pgw], &pgw_hsp_cfg);
                    PGW_HSP_CONFIGr_PHY_PORT_NO_0f_SET(pgw_hsp_cfg, port);
                    PGW_HSP_CONFIGr_ENTRIES_IN_CALf_SET(pgw_hsp_cfg,
                        bcm56560_b0_port_speed_max(unit, port)/1000);
                    ioerr += WRITE_PGW_HSP_CONFIGr(unit, pgw_cl,
                        pgw_hsp_config_index[pgw], pgw_hsp_cfg);
                }
                pgw_hsp_config_index[pgw] += 1;
            }
        }

        /* Enable os_skip_cnt_arb_sel by default in Apache */
        PGW_TDM_CONTROLr_OS_SKIP_CNT_ARB_SELf_SET(pgw_tdm_b0_ctrl, 1);
        while (pgw_hsp_config_index[pgw] < XLPS_PER_PGW) {
            for (pgw_cl = 0; pgw_cl < 2; pgw_cl++) {
                ioerr += READ_PGW_HSP_CONFIGr(unit, pgw_cl,
                    pgw_hsp_config_index[pgw], &pgw_hsp_cfg);
                PGW_HSP_CONFIGr_PHY_PORT_NO_0f_SET(pgw_hsp_cfg, 0);
                PGW_HSP_CONFIGr_ENTRIES_IN_CALf_SET(pgw_hsp_cfg, 0);
                ioerr += WRITE_PGW_HSP_CONFIGr(unit, pgw_cl,
                    pgw_hsp_config_index[pgw], pgw_hsp_cfg);
            }
            pgw_hsp_config_index[pgw] += 1;
        }

        /* OS_TDM_WRAP_PTR and OS_TDM_ENABLE programming */
        PGW_TDM_CONTROLr_OS_TDM_WRAP_PTRf_SET(pgw_tdm_b0_ctrl, (count - 1));
        PGW_TDM_CONTROLr_OS_TDM_ENABLEf_SET(pgw_tdm_b0_ctrl, (count ? 1 : 0));

        /* Configure PGW oversubscription port spacing */
        for (base = 0; base < PGW_TDM_OVS_SIZE; base += PGW_TDM_SLOTS_PER_REG) {
            PGW_OS_PORT_SPACING_REGr_CLR(pgw_os_port_spacing);
            base_idx = base / PGW_TDM_SLOTS_PER_REG;
            for (idx = 0; idx < PGW_TDM_SLOTS_PER_REG; idx++) {
                slot = base + idx;
                port = tcfg->pgw_ovs_spacing[pgw_master][slot];

                rval = PGW_OS_PORT_SPACING_REGr_GET(pgw_os_port_spacing, 0);
                rval |= (port << (idx << 3));
                PGW_OS_PORT_SPACING_REGr_SET(pgw_os_port_spacing, 0, rval);
            }
            if (((slot + 1) % PGW_TDM_SLOTS_PER_REG) == 0) {
                ioerr += WRITE_PGW_OS_PORT_SPACING_REGr(unit, pgw_master, base_idx,
                                                    pgw_os_port_spacing);
            }
        }
        ioerr += WRITE_PGW_TDM_CONTROLr(unit, pgw_master, pgw_tdm_b0_ctrl);
    }

    /* Configure MMU TDM */
    tdm = tcfg->mmu_tdm;
    if (!cur_cal) {
        ES_PIPE0_TDM_TABLE_0m_CLR(es0_tdm_b0_tbl0);
    } else {
        ES_PIPE0_TDM_TABLE_1m_CLR(es0_tdm_b0_tbl1);
    }
    length = MMU_TDM_LEN;
    while ((tdm[length] == NUM_EXT_PORTS)) {
        length--;
    }

    for (slot = 0; slot <= length; slot ++) {
        port = tdm[slot];
        if (port == 94) {
            mport = 81; /* Dedicated token for guaranteed SBUS ops */
        } else if (port == 93) {
            mport = 77; /* Dedicated ANC slot */
        } else if (port == 91) {
            /* iter over all available internal ports to avoid 1:4 violation */
            mport = mmu_idle_ports[mmu_idle_port_ix];
            mmu_idle_port_ix = (mmu_idle_port_ix + 1) % 4;
        } else if (port == 90) {
            mport = 75; /* Oversub port */
        } else if (port >= NUM_EXT_PORTS) {
            mport = 0x7f; /* Invalid Port */
        } else {
            mport = P2M(unit, port);
        }

        if (!cur_cal) {
            if (slot % 2) {
                ES_PIPE0_TDM_TABLE_0m_PORT_NUM_ODDf_SET(es0_tdm_b0_tbl0, mport);
            } else {
                ES_PIPE0_TDM_TABLE_0m_PORT_NUM_EVENf_SET(es0_tdm_b0_tbl0, mport);
            }
            /* two slots in each entry; length can be odd too */
            if ((slot % 2) || (slot == MMU_TDM_LEN)) {
                ioerr += WRITE_ES_PIPE0_TDM_TABLE_0m(unit, (slot >> 1), es0_tdm_b0_tbl0);
            }
        } else {
            if (slot % 2) {
                ES_PIPE0_TDM_TABLE_1m_PORT_NUM_ODDf_SET(es0_tdm_b0_tbl1, mport);
            } else {
                ES_PIPE0_TDM_TABLE_1m_PORT_NUM_EVENf_SET(es0_tdm_b0_tbl1, mport);
            }
            /* two slots in each entry; length can be odd too */
            if ((slot % 2) || (slot == MMU_TDM_LEN)) {
                ioerr += WRITE_ES_PIPE0_TDM_TABLE_1m(unit, (slot >> 1), es0_tdm_b0_tbl1);
            }
        }
    }

    if (!cur_cal) {
        ES_PIPE0_TDM_CONFIGr_CAL0_ENDf_SET(es0_tdm_b0_cfg,
                            (slot % 2) ? (slot / 2): ((slot / 2) - 1));
        if (slot % 2) {
            ES_PIPE0_TDM_CONFIGr_CAL0_END_SINGLEf_SET(es0_tdm_b0_cfg, 1);
        }
    } else {
        ES_PIPE0_TDM_CONFIGr_CAL1_ENDf_SET(es0_tdm_b0_cfg,
                            (slot % 2) ? (slot / 2): ((slot / 2) - 1));
        if (slot % 2) {
            ES_PIPE0_TDM_CONFIGr_CAL1_END_SINGLEf_SET(es0_tdm_b0_cfg, 1);
        }
    }
    ES_PIPE0_TDM_CONFIGr_OPP_CPULB_ENf_SET(es0_tdm_b0_cfg, 1);
    ES_PIPE0_TDM_CONFIGr_ENABLEf_SET(es0_tdm_b0_cfg, 1);
    ES_PIPE0_TDM_CONFIGr_CURR_CALf_SET(es0_tdm_b0_cfg, cur_cal);
    ioerr += WRITE_ES_PIPE0_TDM_CONFIGr(unit, es0_tdm_b0_cfg);

    ES_PIPE0_GRP_WT_SELECTr_CLR(es0_grp_wt_sel);
    for (group = 0; group < MMU_OVS_GROUP_COUNT; group++) {
        ES_PIPE0_OVR_SUB_GRP_TBLr_CLR(es0_ovr_tbl);
        tdm = tcfg->mmu_ovs_group_tdm[group];
        for (slot = 0; slot < MMU_OVS_GROUP_TDM_LEN; slot++) {
            port = tdm[slot];
            if (port >= NUM_EXT_PORTS) {
                mport = 0x7f;
            } else {
                mport = P2M(unit, port);
            }
            ES_PIPE0_OVR_SUB_GRP_TBLr_MMU_PORTf_SET(es0_ovr_tbl,
                                                    (mport & 0x7f));
            idx = (group * MMU_OVS_GROUP_TDM_LEN) + slot;

            ioerr += WRITE_ES_PIPE0_OVR_SUB_GRP_TBLr(unit, idx, es0_ovr_tbl);
        }

        speed = bcm56560_b0_port_speed_max(unit, port);
        _mmu_ovs_speed_class_map_get(unit, speed, &speed_group, &speed_spacing);
        if (speed_group == OVS_WT_GROUP_SPEED_NA) {
            continue;
        }

        ioerr += READ_ES_PIPE0_OVR_SUB_GRP_CFGr(unit, 0, &es0_ovr_sub_grp_cfg);
        ES_PIPE0_OVR_SUB_GRP_CFGr_SAME_SPACINGf_SET(es0_ovr_sub_grp_cfg, speed_spacing);
        ioerr += WRITE_ES_PIPE0_OVR_SUB_GRP_CFGr(unit, 0, es0_ovr_sub_grp_cfg);
        wt_group = 0;
        switch(group) {
        case 0:
            ES_PIPE0_GRP_WT_SELECTr_GRP0f_SET(es0_grp_wt_sel, wt_group);
            break;
        case 1:
            ES_PIPE0_GRP_WT_SELECTr_GRP1f_SET(es0_grp_wt_sel, wt_group);
            break;
        case 2:
            ES_PIPE0_GRP_WT_SELECTr_GRP2f_SET(es0_grp_wt_sel, wt_group);
            break;
        case 3:
            ES_PIPE0_GRP_WT_SELECTr_GRP3f_SET(es0_grp_wt_sel, wt_group);
            break;
        case 4:
            ES_PIPE0_GRP_WT_SELECTr_GRP4f_SET(es0_grp_wt_sel, wt_group);
            break;
        case 5:
            ES_PIPE0_GRP_WT_SELECTr_GRP5f_SET(es0_grp_wt_sel, wt_group);
            break;
        case 6:
            ES_PIPE0_GRP_WT_SELECTr_GRP6f_SET(es0_grp_wt_sel, wt_group);
            break;
        case 7:
            ES_PIPE0_GRP_WT_SELECTr_GRP7f_SET(es0_grp_wt_sel, wt_group);
            break;
        default:
            break;
        }
    }

    for (wt_group = 0; wt_group < MMU_OVS_WT_GROUP_COUNT; wt_group++) {
        tdm = tcfg->mmu_ovs_group_tdm[wt_group];

        ioerr += READ_ES_PIPE0_OVR_SUB_GRP_WT0r(unit, 0, &es0_ovr_wt);
        for (idx = 0; idx < MMU_OVS_GROUP_TDM_LEN; idx++) {
            port = tdm[idx];
            speed = SPEED_MAX(unit, port);
            /* use 2500M as weight unit */
            weight = (speed > 2500 ? speed : 2500) / 2500;
            ES_PIPE0_OVR_SUB_GRP_WT0r_WEIGHTf_SET(es0_ovr_wt,
                                                 weight * (idx + 1));
            ioerr += WRITE_ES_PIPE0_OVR_SUB_GRP_WT0r(unit, idx, es0_ovr_wt);
        }
        rval = ES_PIPE0_GRP_WT_SELECTr_GET(es0_grp_wt_sel);
        rval |= (wt_group << (group << 1));
        ES_PIPE0_GRP_WT_SELECTr_SET(es0_grp_wt_sel, rval);
    }
    ioerr += WRITE_ES_PIPE0_GRP_WT_SELECTr(unit, es0_grp_wt_sel);

    /* Configure IARB TDM */
    IARB_MAIN_TDMm_CLR(iarb_main);
    tdm = tcfg->iarb_tdm[0];
    for (slot = 0; slot < IARB_TDM_LEN; slot++) {
        if (slot > tcfg->iarb_tdm_b0_wrap_ptr) {
            break;
        }
        IARB_MAIN_TDMm_TDM_SLOTf_SET(iarb_main, tdm[slot]);
        ioerr += WRITE_IARB_MAIN_TDMm(unit, slot, iarb_main);
    }

    /* IARB_TDM_CONTROL */
    ioerr += READ_IARB_TDM_CONTROLr(unit, &iarb_tdm_b0_ctrl);
    IARB_TDM_CONTROLr_TDM_WRAP_PTRf_SET(iarb_tdm_b0_ctrl,
                                        tcfg->iarb_tdm_b0_wrap_ptr);
    IARB_TDM_CONTROLr_DISABLEf_SET(iarb_tdm_b0_ctrl, 0);
    ioerr += WRITE_IARB_TDM_CONTROLr(unit, iarb_tdm_b0_ctrl);

    /* IARB_OPP_SCH_CONTROL */
    ioerr += READ_IARB_OPP_SCH_CONTROLr(unit, &iarb_opp_sch_ctrl);
    IARB_OPP_SCH_CONTROLr_RDB_OPP_ENf_SET(iarb_opp_sch_ctrl, 1);
    IARB_OPP_SCH_CONTROLr_LB_OPP_ENf_SET(iarb_opp_sch_ctrl, 1);
    ioerr += WRITE_IARB_OPP_SCH_CONTROLr(unit, iarb_opp_sch_ctrl);

    ENQ_CONFIGr_CLR(enq_cfg);
    ENQ_CONFIGr_ASF_CFAP_FULL_DROP_ENf_SET(enq_cfg, 1);
    ENQ_CONFIGr_ASF_FIFO_THRESHOLDf_SET(enq_cfg, 3);
    ENQ_CONFIGr_ASF_HS_FIFO_THRESHOLDf_SET(enq_cfg, 13);
    ioerr += WRITE_ENQ_CONFIGr(unit, enq_cfg);

    ioerr += READ_EGR_FORCE_SAF_CONFIGr(unit, &egr_force_saf_cfg);
    EGR_FORCE_SAF_CONFIGr_FIELD_Af_SET(egr_force_saf_cfg, 1023);
    EGR_FORCE_SAF_CONFIGr_FIELD_Bf_SET(egr_force_saf_cfg, 9215);
    EGR_FORCE_SAF_CONFIGr_FIELD_Cf_SET(egr_force_saf_cfg, 16);
    ioerr += WRITE_EGR_FORCE_SAF_CONFIGr(unit, egr_force_saf_cfg);

    ioerr += READ_ES_PIPE0_MMU_1DBG_Cr(unit, &es0_mmu_1dbg_c);
    ES_PIPE0_MMU_1DBG_Cr_FIELD_Af_SET(es0_mmu_1dbg_c, 1);
    ES_PIPE0_MMU_1DBG_Cr_DI_ACT_THRf_SET(es0_mmu_1dbg_c, 20);
    ioerr += WRITE_ES_PIPE0_MMU_1DBG_Cr(unit, es0_mmu_1dbg_c);

    ioerr += READ_ES_PIPE0_MMU_DI_THRr(unit, &es0_mmu_di_thr);
    ES_PIPE0_MMU_DI_THRr_OS_SLT_LMTf_SET(es0_mmu_di_thr, 1000);
    ioerr += WRITE_ES_PIPE0_MMU_DI_THRr(unit, es0_mmu_di_thr);

    ioerr += READ_ES_PIPE0_MMU_2DBG_C_1r(unit, &es0_mmu_2dbg_c1);
    ES_PIPE0_MMU_2DBG_C_1r_FIELD_Bf_SET(es0_mmu_2dbg_c1, 0);
    ioerr += WRITE_ES_PIPE0_MMU_2DBG_C_1r(unit, es0_mmu_2dbg_c1);

    ioerr += READ_ES_PIPE0_MMU_2DBG_C_0r(unit, &es0_mmu_2dbg_c0);
    ES_PIPE0_MMU_2DBG_C_0r_FIELD_Af_SET(es0_mmu_2dbg_c0, (core_freq * 200));
    ioerr += WRITE_ES_PIPE0_MMU_2DBG_C_0r(unit, es0_mmu_2dbg_c0);

    core_freq = bcm56560_b0_get_core_frequency(unit);
    /* values array for filling mmu_1dbg_a regs  */
    values[0] = values[1] = values[2] = 0;

    bcm56560_b0_all_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        mport = P2M(unit, port);
        if (port == -1 || mport == -1) {
            continue;
        }

        ioerr += READ_ES_PIPE0_MMU_3DBG_Cr(unit, port, &esp_mmu_3dbg_c);
        if ((oversub) && (SPEED_MAX(unit, port) >= 20000)) {
            /* val = 120 + (sal_rand() % 20); */
            val = 120 + (5 % 20);
        } else {
            /* linerate port or OS with < 20G */
            val = 0;
        }
        ES_PIPE0_MMU_3DBG_Cr_FIELD_Af_SET(esp_mmu_3dbg_c, val);
        ioerr += WRITE_ES_PIPE0_MMU_3DBG_Cr(unit, port, esp_mmu_3dbg_c);

        if (oversub) {
            rval = ((SPEED_MAX(unit, port) / 1000) *
                    (12148125 / (core_freq * 100)));
            ioerr += READ_EDB_1DBG_Bm(unit, port, &edb_1dbg_b);
            EDB_1DBG_Bm_FIELD_Bf_SET(edb_1dbg_b, rval);
            ioerr += WRITE_EDB_1DBG_Bm(unit, port, edb_1dbg_b);

            if (mport < 32) {
                values[0] |= (1 << mport);
            } else if (mport < 64) {
                values[1] |= (1 << (mport - 32));
            } else {
                values[2] |= (1 << (mport - 64));
            }
        }
    }

    ioerr += READ_ES_PIPE0_MMU_1DBG_A_LOr(unit, &es0_mmu_1dbg_a_lo);
    ES_PIPE0_MMU_1DBG_A_LOr_FIELD_Af_SET(es0_mmu_1dbg_a_lo, values[0]);
    ioerr += WRITE_ES_PIPE0_MMU_1DBG_A_LOr(unit, es0_mmu_1dbg_a_lo);
    ioerr += READ_ES_PIPE0_MMU_1DBG_A_MIDr(unit, &es0_mmu_1dbg_a_mid);
    ES_PIPE0_MMU_1DBG_A_MIDr_FIELD_Af_SET(es0_mmu_1dbg_a_mid, values[1]);
    ioerr += WRITE_ES_PIPE0_MMU_1DBG_A_MIDr(unit, es0_mmu_1dbg_a_mid);
    ioerr += READ_ES_PIPE0_MMU_1DBG_A_HIr(unit, &es0_mmu_1dbg_a_hi);
    ES_PIPE0_MMU_1DBG_A_HIr_FIELD_Af_SET(es0_mmu_1dbg_a_hi, values[2]);
    ioerr += WRITE_ES_PIPE0_MMU_1DBG_A_HIr(unit, es0_mmu_1dbg_a_hi);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

/*
 * SINGLE   => {VALUE => 0, DESC => "Port 0 valid.  10G/20G/25G/40G/50G/100G."},
 * DUAL     => {VALUE => 1, DESC => "Port 0 and 2 valid.  10G/20G/25G/40G/50G."},
 * QUAD     => {VALUE => 2, DESC => "Port 0, 1, 2, and 3 valid.  10G/20G/25G."},
 * TRI211   => {VALUE => 3, DESC => "Port 0 is 20G, port 2 10G, and port 3 10G."},
 * TRI112   => {VALUE => 4, DESC => "Port 0 is 10G, port 1 10G, and port 2 20G."},
 * TRI411   => {VALUE => 5, DESC => "Port 0 is 40G, port 2 10G, and port 3 10G."},
 * TRI114   => {VALUE => 6, DESC => "Port 0 is 10G, port 1 10G, and port 2 40G."},
 */
static int
_port_ca_mode_get(int unit, int flexport, int xlp, int *mode)
{
    int port, phy_port_base, lane, lport;
    int num_lanes[PORTS_PER_XLP];
    int speed_max[PORTS_PER_XLP];

    *mode = 2;
    /* SOC_AP_PORT_CA_MODE_QUAD */

    phy_port_base = 1 + (xlp * PORTS_PER_XLP);
    for (lane = 0; lane < PORTS_PER_XLP; lane++) {
        port = phy_port_base + lane;
        lport = P2L(unit, port);
        if (lport == -1) {
            num_lanes[lane] = -1;
            speed_max[lane] = -1;
        } else {
            num_lanes[lane] = bcm56560_b0_port_lanes_get(unit, port);
            speed_max[lane] = SPEED_MAX(unit, port);
        }
    }

    if (num_lanes[0] >= 4) {
        *mode = 0;
        /*SOC_AP_PORT_CA_MODE_SINGLE */
    } else if ((num_lanes[0] == 2) && (num_lanes[2] == 2)) {
        *mode = 1;
        /* SOC_AP_PORT_CA_MODE_DUAL */
    } else if ((num_lanes[0] == 1) && (num_lanes[1] == 1) &&
               (num_lanes[2] == 1) && (num_lanes[3] == 1)) {
        *mode = 2;
        /* SOC_AP_PORT_CA_MODE_QUAD */
    } else if (num_lanes[0] == 2) {
        *mode = (speed_max[0] >= 40000) ? 5 : 3;
    } else if (num_lanes[2] == 2) {
        *mode = (speed_max[2] >= 40000) ? 6 : 4;
    }

    return CDK_E_NONE;
}

static int
_port_blk_ca_mode_set(int unit, int flexport, int xlp)
{
    int ioerr = 0;
    int ca_port_mode = 0;
    IDB_OBM_CA_CONTROLr_t idb_obm_ca_ctrl;
    int blk, obm, subport;

    /* Calculate the mode of the Cell Assembly for the port block */
    ioerr += _port_ca_mode_get(unit, flexport, xlp, &ca_port_mode);
    blk = bcm56560_b0_pgw_clport_block_index_get(unit, xlp, &obm, &subport);

    ioerr += READ_IDB_OBM_CA_CONTROLr(unit, blk, obm, &idb_obm_ca_ctrl);
    IDB_OBM_CA_CONTROLr_PORT_MODEf_SET(idb_obm_ca_ctrl, ca_port_mode);
    if (!flexport) {
        if (subport == 0) {
            IDB_OBM_CA_CONTROLr_PORT0_RESETf_SET(idb_obm_ca_ctrl, 1);
        } else if (subport == 1) {
            IDB_OBM_CA_CONTROLr_PORT1_RESETf_SET(idb_obm_ca_ctrl, 1);
        } else if (subport == 2) {
            IDB_OBM_CA_CONTROLr_PORT2_RESETf_SET(idb_obm_ca_ctrl, 1);
        } else {
            IDB_OBM_CA_CONTROLr_PORT3_RESETf_SET(idb_obm_ca_ctrl, 1);
        }
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

static int
_idb_init(int unit)
{
    int ioerr = 0;
    cdk_pbmp_t all_pbmp;
    int port;

    /* Per-port settings */
    bcm56560_b0_all_front_pbmp_get(unit, &all_pbmp);
    CDK_PBMP_ITER(all_pbmp, port) {
        ioerr += _port_blk_ca_mode_set(unit, FALSE, port);
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

int
bcm56560_b0_egr_buf_reset(int unit, int port, int reset)
{
    int ioerr = 0;
    EGR_PER_PORT_BUFFER_SFT_RESETm_t port_buf_reset;

    ioerr += READ_EGR_PER_PORT_BUFFER_SFT_RESETm(unit, port, &port_buf_reset);
    EGR_PER_PORT_BUFFER_SFT_RESETm_ENABLEf_SET(port_buf_reset, reset);
    ioerr += WRITE_EGR_PER_PORT_BUFFER_SFT_RESETm(unit, port, port_buf_reset);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

static int
_mc_toq_cfg(int unit, int port, int enable)
{
    int ioerr = 0;
    TOQ_MC_CFG1r_t toq_mc_cfg1;
    int mport = P2M(unit, port);
    uint32_t fval;

    ioerr += READ_TOQ_MC_CFG1r(unit, &toq_mc_cfg1);

    fval = TOQ_MC_CFG1r_IS_MC_T2OQ_PORT0f_GET(toq_mc_cfg1);
    if (enable) {
        fval |= 1 << (mport & 0x0f);
    } else {
        fval &= ~(1 << (mport & 0x0f));
    }
    TOQ_MC_CFG1r_IS_MC_T2OQ_PORT0f_SET(toq_mc_cfg1, fval);

    ioerr += WRITE_TOQ_MC_CFG1r(unit, toq_mc_cfg1);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

static int
_mmu_delay_insertion_set(int unit, int port, int speed)
{
    int ioerr = 0;
    ES_PIPE0_MMU_3DBG_Cr_t mmu_3dbg_x;
    int mport;
    int oversub = 0;
    uint32_t fval = 0;

    mport = P2M(unit, port) & 0x3f;
    if (_total_bandwidth(unit) > _core_bandwidth(unit)) {
        oversub = 1;
    }

    if (mport < MAX_MMU_PORTS_PER_PIPE) {
        if (oversub) {
            if (speed <= 10000) {
                fval = 15;
            } else if (speed <= 20000) {
                fval = 30;
            } else if (speed <= 25000) {
                fval = 40;
            } else if (speed <= 40000) {
                fval = 60;
            }
        }
        ioerr += READ_ES_PIPE0_MMU_3DBG_Cr(unit, mport, &mmu_3dbg_x);
        ES_PIPE0_MMU_3DBG_Cr_FIELD_Af_SET(mmu_3dbg_x, fval);
        ioerr += WRITE_ES_PIPE0_MMU_3DBG_Cr(unit, mport, mmu_3dbg_x);
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

static int
_port_speed_update(int unit, int port, int speed)
{
    int ioerr = 0;
    EGR_EDB_XMIT_CTRLm_t egr_xmit_ctrl;
    TOQ_MC_CFG1r_t toq_mc_cfg1;
    MMU_MCQDB0m_t mcqdb0;
    int oversub, idx, qnum, cosq;
    int mport = P2M(unit, port);
    uint32_t fval;
    int core_freq;

    core_freq = bcm56560_b0_get_core_frequency(unit);

    /* Update Edatabuf transmit start count */
    oversub = (_total_bandwidth(unit) > _core_bandwidth(unit)) ? 1 : 0;
    if (core_freq == 415 && !oversub) {
        ioerr += READ_EGR_EDB_XMIT_CTRLm(unit, port, &egr_xmit_ctrl);
        EGR_EDB_XMIT_CTRLm_START_CNTf_SET(egr_xmit_ctrl,
                                MMU_XMIT_START_CNT_LINERATE(core_freq, speed));
        ioerr += WRITE_EGR_EDB_XMIT_CTRLm(unit, port, egr_xmit_ctrl);
    }

    /* Update VBS (HSP) port multicast T2OQ setting */
    if (bcm56560_b0_port_in_eq_bmp(unit, port) && core_freq < 500) {
        ioerr += READ_TOQ_MC_CFG1r(unit, &toq_mc_cfg1);
        fval = TOQ_MC_CFG1r_IS_MC_T2OQ_PORT0f_GET(toq_mc_cfg1);
        if (speed >= 30000) {
            fval |= 1 << (mport & 0x3f);
        } else {
            fval &= ~(1 << (mport & 0x3f));
        }
        TOQ_MC_CFG1r_IS_MC_T2OQ_PORT0f_SET(toq_mc_cfg1, fval);
        ioerr += WRITE_TOQ_MC_CFG1r(unit, toq_mc_cfg1);

        /* write 0 to Q_NOT_EMPTY field of sister entry locations in MMU_MCQDBx
         */
        if (speed < 30000) {
            qnum = bcm56560_b0_mmu_port_mc_queues(unit, port);
            for (idx = 0; idx < qnum; idx++) {
                cosq = bcm56560_b0_mc_queue_num(unit, port, idx);
                cosq = cosq - NUM_UC_Q_PER_PIPE + 360;
                ioerr += READ_MMU_MCQDB0m(unit, cosq, &mcqdb0);
                MMU_MCQDB0m_Q_NOT_EMPTYf_SET(mcqdb0, 0);
                ioerr += WRITE_MMU_MCQDB0m(unit, cosq, mcqdb0);
            }
        }
    }

    if (bcm56560_b0_port_in_eq_bmp(unit, port)) {
        _mc_toq_cfg(unit, port, (speed >= 40000) ? TRUE : FALSE);
    }

    _mmu_delay_insertion_set(unit, port, speed);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

static void
_default_lossless_pg_headroom(int unit, int port, int *headroom, int *guarantee)
{
    cdk_pbmp_t oversub_pbm;

    if (port == RDB_MPORT) {
        *headroom = 0;
        *guarantee = 90;
        return;
    }

    if (port == CMIC_MPORT) {
        *headroom = 50;
        *guarantee = 8;
        return;
    }

    if (port == LB_MPORT) {
        *headroom = 162;
        *guarantee = 8;
        return;
    }

    bcm56560_b0_ovrs_pbmp_get(unit, &oversub_pbm);
    if (CDK_PBMP_MEMBER(oversub_pbm, port)) {
        if (SPEED_MAX(unit, port) >= 100000) {
            *headroom = 687;
        }else if (SPEED_MAX(unit, port) >= 500000) {
             *headroom = 352;
        } else if (SPEED_MAX(unit, port) >= 40000) {
            *headroom = 338;
        } else if (SPEED_MAX(unit, port) >= 25000) {
            *headroom = 214;
        } else if (SPEED_MAX(unit, port) >= 20000) {
            *headroom = 206;
        } else {
            *headroom = 185;
        }
    } else {
        if (SPEED_MAX(unit, port) >= 100000) {
            *headroom = 558;
        }else if (SPEED_MAX(unit, port) >= 500000) {
             *headroom = 298;
        } else if (SPEED_MAX(unit, port) >= 40000) {
            *headroom = 284;
        } else if (SPEED_MAX(unit, port) >= 25000) {
            *headroom = 191;
        } else if (SPEED_MAX(unit, port) >= 20000) {
            *headroom = 183;
        } else {
            *headroom = 162;
        }
    }
    *guarantee = 8;

    return;
}

/* input: port is mmu port */
static int
_port_numq(int unit, int port)
{
    if (port == RDB_MPORT) {
        return 0;
    } else if (port == CMIC_MPORT) {
        return 48;
    } else if (port == LB_MPORT) {
        return 9;
    } else {
        return 10;
    }
}

#define MMU_RQE_ENTRY 3936
#define CFAPFULLSETPOINT 61376
static int
_mmu_config_init(int unit, int input_port)
{
    int ioerr = 0;
    THDI_PORT_PRI_GRPr_t port_pri_grp;
    CFAPFULLSETPOINTr_t cfap_set_point;
    CFAPFULLRESETPOINTr_t cfap_reset_point;
    THDI_GLOBAL_HDRM_LIMIT_PIPEXr_t g_hdrm_limit_x;
    THDI_BYPASSr_t thdi_bypass;
    THDI_POOL_CONFIGr_t pool_cfg;
    THDI_BUFFER_CELL_LIMIT_SPr_t buf_lim_sp;
    THDI_CELL_RESET_LIMIT_OFFSET_SPr_t rst_lim_off_sp;
    THDI_BUFFER_CELL_LIMIT_PUBLIC_POOLr_t buf_lim_pub_pool;
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
    MMU_THDU_XPIPE_CONFIG_QGROUPm_t cfg_qgrp_x;
    MMU_THDU_XPIPE_OFFSET_QGROUPm_t off_qgrp_x;
    THDI_PORT_PG_SPIDr_t thdi_port_pg_spid;
    THDI_PORT_SP_CONFIG_Xm_t sp_cfg_x;
    THDI_INPUT_PORT_XON_ENABLESr_t xon_en;
    THDI_PORT_MAX_PKT_SIZEr_t max_pkt;
    THDI_PORT_PG_CONFIG_Xm_t pg_cfg_x;
    MMU_THDU_XPIPE_CONFIG_QUEUEm_t cfg_q_x;
    MMU_THDU_XPIPE_OFFSET_QUEUEm_t off_q_x;
    MMU_THDU_XPIPE_Q_TO_QGRP_MAPm_t q2qgrp_x;
    MMU_THDU_XPIPE_CONFIG_PORTm_t pcfg_x;
    MMU_THDU_XPIPE_RESUME_PORTm_t prsm_x;
    MMU_THDM_DB_QUEUE_CONFIG_0m_t db_q_cfg_x;
    MMU_THDM_DB_QUEUE_OFFSET_0m_t db_q_off_x;
    MMU_THDM_MCQE_QUEUE_CONFIG_0m_t mcqe_q_cfg_x;
    MMU_THDM_MCQE_QUEUE_OFFSET_0m_t mcqe_q_off_x;
    MMU_THDM_DB_QUEUE_RESUME_OFFSET_PROFILE_YELLOWr_t db_yprof;
    MMU_THDM_DB_QUEUE_RESUME_OFFSET_PROFILE_REDr_t db_rprof;
    MMU_THDM_MCQE_QUEUE_RESUME_OFFSET_PROFILE_YELLOWr_t mcqe_yprof;
    MMU_THDM_MCQE_QUEUE_RESUME_OFFSET_PROFILE_REDr_t mcqe_rprof;
    MMU_THDM_DB_PORTSP_CONFIG_0m_t db_psp_cfg_x;
    MMU_THDM_MCQE_PORTSP_CONFIG_0m_t mcqe_psp_cfg_x;
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
    uint32_t default_mtu_cells, jumbo_frame_cells;
    cdk_pbmp_t all_pbmp;
    int port;
    uint32_t mmu_total_cell;
    int fifo_entry_count = 0;
    int port_count = 0;
    uint32_t rval, fval;
    int idx, idx_int;
    int headroom = 0, guarantee = 0, prigroup_headroom = 0, prigroup_guarantee = 0;
    int temp_headroom, temp_guarantee;
    uint32_t ing_rsvd;
    uint32_t queue_guarantee;
    uint32_t limit;
    uint32_t pool_resume;
    uint32_t total_rsvd;
    uint32_t pool_limit;
    uint32_t total_mcq_entry = MMU_MCQ_ENTRY;
    uint32_t pkt_size;
    int total_rqe_entry;
    int base;
    int numq;
    uint32_t rlimit;
    uint32_t min_resume_limit;
    uint32_t mcqe_limit;
    int mport;

    default_mtu_cells = MMU_BYTES_TO_CELLS(MMU_DEFAULT_MTU_BYTES +
                                           MMU_PKT_HDR_BYTES);

    jumbo_frame_cells = MMU_BYTES_TO_CELLS(MMU_JUMBO_FRAME_BYTES +
                                           MMU_PKT_HDR_BYTES);
    bcm56560_b0_all_front_pbmp_get(unit, &all_pbmp);
    CDK_PBMP_ITER(all_pbmp, port) {
        if (SPEED_MAX(unit, port) != 100000){
            port_count++;
        }
        fifo_entry_count += SPEED_MAX(unit, port) <= 11000 ? 4 : 16;
    }
    mmu_total_cell = MMU_TOTAL_CELLS - fifo_entry_count - port_count  - 10;

    CFAPFULLSETPOINTr_CLR(cfap_set_point);
    CFAPFULLSETPOINTr_CFAPFULLSETPOINTf_SET(cfap_set_point, CFAPFULLSETPOINT);
    ioerr += WRITE_CFAPFULLSETPOINTr(unit, cfap_set_point);

    fval = CFAPFULLSETPOINT - 2 * jumbo_frame_cells;
    CFAPFULLRESETPOINTr_CLR(cfap_reset_point);
    CFAPFULLRESETPOINTr_CFAPFULLRESETPOINTf_SET(cfap_reset_point, fval);
    ioerr += WRITE_CFAPFULLRESETPOINTr(unit, cfap_reset_point);

     /* internal priority to priority group mapping */
    rval = 0;
    for (idx_int = 0; idx_int < 8; idx_int++) {
        rval |= (7 << (idx_int * 3));
    }
    THDI_PORT_PRI_GRPr_SET(port_pri_grp, rval);

    CDK_PBMP_PORT_ADD(all_pbmp, CMIC_LPORT);
    CDK_PBMP_PORT_ADD(all_pbmp, RDB_LPORT);
    CDK_PBMP_PORT_ADD(all_pbmp, LB_LPORT);

    CDK_PBMP_ITER(all_pbmp, port) {
        mport = P2M(unit, port);

        for (idx = 0; idx < 2; idx++) {
            if ((mport == RDB_MPORT) && (idx == 0)) {
                for (idx_int = 0; idx_int < 8; idx_int++) {
                    if (idx_int <= 5) {
                        rval |= (idx_int + 2) << (idx_int * 3);
                    } else {
                        rval |= 7 << (idx_int * 3);
                    }
                }
            }
            ioerr += WRITE_THDI_PORT_PRI_GRPr(unit, mport, idx, port_pri_grp);
        }
    }

    /* Input thresholds */
    headroom = 2 * MMU_GLOBAL_HEADROOM_CELL_PER_PIPE;
    THDI_GLOBAL_HDRM_LIMIT_PIPEXr_CLR(g_hdrm_limit_x);
    THDI_GLOBAL_HDRM_LIMIT_PIPEXr_GLOBAL_HDRM_LIMITf_SET(g_hdrm_limit_x, headroom / 2);
    ioerr += WRITE_THDI_GLOBAL_HDRM_LIMIT_PIPEXr(unit, g_hdrm_limit_x);

    THDI_BYPASSr_CLR(thdi_bypass);
    ioerr += WRITE_THDI_BYPASSr(unit, thdi_bypass);

    THDI_POOL_CONFIGr_CLR(pool_cfg);
    THDI_POOL_CONFIGr_PUBLIC_ENABLEf_SET(pool_cfg, 1);
    ioerr += WRITE_THDI_POOL_CONFIGr(unit, pool_cfg);

    CDK_PBMP_ITER(all_pbmp, port) {
        mport = P2M(unit, port);

        _default_lossless_pg_headroom(unit, mport, &temp_headroom, &temp_guarantee);
        prigroup_headroom += temp_headroom;
        prigroup_guarantee += temp_guarantee;
    }
    ing_rsvd = prigroup_headroom + prigroup_guarantee;

    queue_guarantee = 0;
    for (idx = 0; idx < MMU_NUM_RQE; idx++) {
        queue_guarantee += 8;
    }

    limit = mmu_total_cell - (ing_rsvd+ (headroom / 2) + queue_guarantee);
    port_count = 0;
    CDK_PBMP_ITER(all_pbmp, port) {
        port_count++;
    }
    port_count -= 1;
    pool_resume = (port_count / 2) *  default_mtu_cells;

    THDI_BUFFER_CELL_LIMIT_SPr_CLR(buf_lim_sp);
    THDI_BUFFER_CELL_LIMIT_SPr_LIMITf_SET(buf_lim_sp, limit);
    ioerr += WRITE_THDI_BUFFER_CELL_LIMIT_SPr(unit, 0, buf_lim_sp);

    THDI_CELL_RESET_LIMIT_OFFSET_SPr_CLR(rst_lim_off_sp);
    THDI_CELL_RESET_LIMIT_OFFSET_SPr_OFFSETf_SET(rst_lim_off_sp, pool_resume);
    ioerr += WRITE_THDI_CELL_RESET_LIMIT_OFFSET_SPr(unit, 0, rst_lim_off_sp);

    THDI_BUFFER_CELL_LIMIT_PUBLIC_POOLr_CLR(buf_lim_pub_pool);
    ioerr += WRITE_THDI_BUFFER_CELL_LIMIT_PUBLIC_POOLr(unit, buf_lim_pub_pool);

    /* output thresholds */
    /* Global output thresholds (using only service pool 0) */
    OP_THDU_CONFIGr_CLR(op_cfg);
    OP_THDU_CONFIGr_ENABLE_QUEUE_AND_GROUP_TICKETf_SET(op_cfg, 1);
    OP_THDU_CONFIGr_ENABLE_UPDATE_COLOR_RESUMEf_SET(op_cfg, 1);
    OP_THDU_CONFIGr_MOP_POLICY_1Bf_SET(op_cfg, 1);
    ioerr += WRITE_OP_THDU_CONFIGr(unit, op_cfg);

    ioerr += READ_MMU_THDM_DB_DEVICE_THR_CONFIGr(unit, &dev_thr_cfg);
    MMU_THDM_DB_DEVICE_THR_CONFIGr_MOP_POLICYf_SET(dev_thr_cfg, 1);
    ioerr += WRITE_MMU_THDM_DB_DEVICE_THR_CONFIGr(unit, dev_thr_cfg);

    total_rsvd = queue_guarantee;
    pool_limit = mmu_total_cell - total_rsvd;
    MMU_THDM_DB_POOL_SHARED_LIMITr_CLR(db_sh_lim);
    MMU_THDM_DB_POOL_SHARED_LIMITr_SHARED_LIMITf_SET(db_sh_lim, pool_limit);
    ioerr += WRITE_MMU_THDM_DB_POOL_SHARED_LIMITr(unit, 0, db_sh_lim);

    fval = (pool_limit + 7) / 8;
    MMU_THDM_DB_POOL_YELLOW_SHARED_LIMITr_SET(db_ysh_lim, fval);
    ioerr += WRITE_MMU_THDM_DB_POOL_YELLOW_SHARED_LIMITr(unit, 0, db_ysh_lim);

    MMU_THDM_DB_POOL_RED_SHARED_LIMITr_SET(db_rsh_lim, fval);
    ioerr += WRITE_MMU_THDM_DB_POOL_RED_SHARED_LIMITr(unit, 0, db_rsh_lim);

    MMU_THDM_DB_POOL_RESUME_LIMITr_SET(db_rsm_lim, fval);
    ioerr += WRITE_MMU_THDM_DB_POOL_RESUME_LIMITr(unit, 0, db_rsm_lim);

    MMU_THDM_DB_POOL_YELLOW_RESUME_LIMITr_SET(db_yrsm_lim, fval);
    ioerr += WRITE_MMU_THDM_DB_POOL_YELLOW_RESUME_LIMITr(unit, 0, db_yrsm_lim);

    MMU_THDM_DB_POOL_RED_RESUME_LIMITr_SET(db_rrsm_lim, fval);
    ioerr += WRITE_MMU_THDM_DB_POOL_RED_RESUME_LIMITr(unit, 0, db_rrsm_lim);

    total_rsvd = 0;
    CDK_PBMP_ITER(all_pbmp, port) {
        mport = P2M(unit, port);

        total_rsvd += _port_numq(unit, mport) * 4;
        if (mport == LB_MPORT) {
            total_rsvd -= 4;
        }
    }
    limit = total_mcq_entry - total_rsvd;
    MMU_THDM_MCQE_POOL_SHARED_LIMITr_SET(mcq_sh_lim, limit/4);
    ioerr += WRITE_MMU_THDM_MCQE_POOL_SHARED_LIMITr(unit, 0, mcq_sh_lim);

    fval = (limit + 7)/ 8;
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

    /* configure Q-groups */
    fval = (pool_limit + 7) / 8;
    MMU_THDU_XPIPE_CONFIG_QGROUPm_CLR(cfg_qgrp_x);
    MMU_THDU_XPIPE_CONFIG_QGROUPm_Q_SHARED_LIMIT_CELLf_SET(cfg_qgrp_x, pool_limit);
    MMU_THDU_XPIPE_CONFIG_QGROUPm_LIMIT_YELLOW_CELLf_SET(cfg_qgrp_x, fval);
    MMU_THDU_XPIPE_CONFIG_QGROUPm_LIMIT_RED_CELLf_SET(cfg_qgrp_x, fval);

    MMU_THDU_XPIPE_OFFSET_QGROUPm_CLR(off_qgrp_x);
    pool_resume = default_mtu_cells * 2;
    fval = pool_resume / 8;
    MMU_THDU_XPIPE_OFFSET_QGROUPm_RESET_OFFSET_CELLf_SET(off_qgrp_x, fval);
    MMU_THDU_XPIPE_OFFSET_QGROUPm_RESET_OFFSET_YELLOW_CELLf_SET(off_qgrp_x, fval);
    MMU_THDU_XPIPE_OFFSET_QGROUPm_RESET_OFFSET_RED_CELLf_SET(off_qgrp_x, fval);

    for (idx = 0; idx < MMU_CFG_QGROUP_MAX; idx++) {
        ioerr += WRITE_MMU_THDU_XPIPE_CONFIG_QGROUPm(unit, idx, cfg_qgrp_x);
        ioerr += WRITE_MMU_THDU_XPIPE_OFFSET_QGROUPm(unit, idx, off_qgrp_x);
    }

    /* Input port per port settings */
    CDK_PBMP_ITER(all_pbmp, port) {
        mport = P2M(unit, port);

        idx = (mport & 0x7f) * MMU_NUM_POOL;

        THDI_PORT_PG_SPIDr_CLR(thdi_port_pg_spid);
        ioerr += WRITE_THDI_PORT_PG_SPIDr(unit, mport, thdi_port_pg_spid);

        THDI_PORT_SP_CONFIG_Xm_CLR(sp_cfg_x);
        THDI_PORT_SP_CONFIG_Xm_PORT_SP_MIN_LIMITf_SET(sp_cfg_x, 0);
        THDI_PORT_SP_CONFIG_Xm_PORT_SP_MAX_LIMITf_SET(sp_cfg_x, mmu_total_cell);
        fval = mmu_total_cell - 2 * default_mtu_cells;
        THDI_PORT_SP_CONFIG_Xm_PORT_SP_RESUME_LIMITf_SET(sp_cfg_x, fval);
        ioerr += WRITE_THDI_PORT_SP_CONFIG_Xm(unit, idx, sp_cfg_x);
        THDI_INPUT_PORT_XON_ENABLESr_CLR(xon_en);
        THDI_INPUT_PORT_XON_ENABLESr_INPUT_PORT_RX_ENABLEf_SET(xon_en, 1);
        if (mport != RDB_MPORT) {
            THDI_INPUT_PORT_XON_ENABLESr_PORT_PRI_XON_ENABLEf_SET(xon_en, 0x3ffff);
            THDI_INPUT_PORT_XON_ENABLESr_PORT_PAUSE_ENABLEf_SET(xon_en, 1);
        }
        ioerr += WRITE_THDI_INPUT_PORT_XON_ENABLESr(unit, mport, xon_en);

        if (CDK_XGSD_FLAGS(unit) & CHIP_FLAG_HQOS_FOUR) {
            pkt_size = MMU_BYTES_TO_CELLS(MMU_MAVERICK_MAX_PACKET_BYTES);
        } else {
            pkt_size = MMU_BYTES_TO_CELLS(MMU_APACHE_MAX_PACKET_BYTES);
        }
        THDI_PORT_MAX_PKT_SIZEr_SET(max_pkt, pkt_size);
        ioerr += WRITE_THDI_PORT_MAX_PKT_SIZEr(unit, max_pkt);
    }

    CDK_PBMP_ITER(all_pbmp, port) {
        mport = P2M(unit, port);

        THDI_PORT_PG_CONFIG_Xm_CLR(pg_cfg_x);
        if (mport != RDB_MPORT) {
            THDI_PORT_PG_CONFIG_Xm_PG_MIN_LIMITf_SET(pg_cfg_x, 8);
        }
        THDI_PORT_PG_CONFIG_Xm_PG_SHARED_DYNAMICf_SET(pg_cfg_x, 1);
        THDI_PORT_PG_CONFIG_Xm_PG_SHARED_LIMITf_SET(pg_cfg_x, 8);
        THDI_PORT_PG_CONFIG_Xm_PG_GBL_HDRM_ENf_SET(pg_cfg_x, 1);

        _default_lossless_pg_headroom(unit, mport, &headroom, &guarantee);
        THDI_PORT_PG_CONFIG_Xm_PG_HDRM_LIMITf_SET(pg_cfg_x, headroom);
        THDI_PORT_PG_CONFIG_Xm_PG_RESET_OFFSETf_SET(pg_cfg_x, pool_resume);
        if (mport != RDB_MPORT) {
            idx = (mport & 0x7f) * MMU_NUM_PG + 7;
            ioerr += WRITE_THDI_PORT_PG_CONFIG_Xm(unit, idx, pg_cfg_x);
        } else {
            base = ((mport - 1) & 0x7f) * MMU_NUM_PG + 7 + 1;
            for (idx = 0; idx < 8; idx ++) {
                ioerr += WRITE_THDI_PORT_PG_CONFIG_Xm(unit, base + idx, pg_cfg_x);
            }
        }
    }

    /***********************************
     * THDO
 */
    CDK_PBMP_ITER(all_pbmp, port) {
        mport = P2M(unit, port);

        if (mport == RDB_MPORT) {
            continue;
        }
        if (mport == CMIC_MPORT) {
            base = 0;
        } else if (mport == LB_MPORT) {
            base = 48 + 10 * (LB_MPORT - 1);
        } else {
            base = 48 + 10 * (mport - 1);
        }

        numq = _port_numq(unit, mport);
        for (idx = 0; idx < numq; idx++) {
            fval = (pool_limit + 7) / 8;
            /* Output port per port per queue setting for regular multicast queue */
            MMU_THDM_DB_QUEUE_CONFIG_0m_CLR(db_q_cfg_x);
            MMU_THDM_DB_QUEUE_CONFIG_0m_Q_SHARED_LIMITf_SET(db_q_cfg_x, pool_limit);
            MMU_THDM_DB_QUEUE_CONFIG_0m_YELLOW_SHARED_LIMITf_SET(db_q_cfg_x, fval);
            MMU_THDM_DB_QUEUE_CONFIG_0m_RED_SHARED_LIMITf_SET(db_q_cfg_x, fval);
            MMU_THDM_DB_QUEUE_CONFIG_0m_Q_SPIDf_SET(db_q_cfg_x, 0);

            fval = (default_mtu_cells * 2) / 8;
            MMU_THDM_DB_QUEUE_OFFSET_0m_CLR(db_q_off_x);
            MMU_THDM_DB_QUEUE_OFFSET_0m_RESUME_OFFSETf_SET(db_q_off_x, fval);

            MMU_THDM_DB_QUEUE_RESUME_OFFSET_PROFILE_YELLOWr_SET(db_yprof, 2);
            ioerr += WRITE_MMU_THDM_DB_QUEUE_RESUME_OFFSET_PROFILE_YELLOWr(unit,
                                                                    0, db_yprof);

            MMU_THDM_DB_QUEUE_RESUME_OFFSET_PROFILE_REDr_SET(db_rprof, 2);
            ioerr += WRITE_MMU_THDM_DB_QUEUE_RESUME_OFFSET_PROFILE_REDr(unit,
                                                                    0, db_rprof);

            fval = (mcqe_limit + 7) / 8;
            MMU_THDM_MCQE_QUEUE_CONFIG_0m_CLR(mcqe_q_cfg_x);
            mcqe_limit = total_mcq_entry - total_rsvd;
            MMU_THDM_MCQE_QUEUE_CONFIG_0m_Q_SHARED_LIMITf_SET(mcqe_q_cfg_x,
                                                                mcqe_limit / 4);
            MMU_THDM_MCQE_QUEUE_CONFIG_0m_YELLOW_SHARED_LIMITf_SET(mcqe_q_cfg_x, fval);
            MMU_THDM_MCQE_QUEUE_CONFIG_0m_RED_SHARED_LIMITf_SET(mcqe_q_cfg_x, fval);

            MMU_THDM_MCQE_QUEUE_OFFSET_0m_CLR(mcqe_q_off_x);
            MMU_THDM_MCQE_QUEUE_OFFSET_0m_RESUME_OFFSETf_SET(mcqe_q_off_x, 1);

            ioerr += WRITE_MMU_THDM_DB_QUEUE_CONFIG_0m(unit, base + idx, db_q_cfg_x);
            ioerr += WRITE_MMU_THDM_DB_QUEUE_OFFSET_0m(unit, base + idx, db_q_off_x);
            ioerr += WRITE_MMU_THDM_MCQE_QUEUE_CONFIG_0m(unit, base + idx, mcqe_q_cfg_x);
            ioerr += WRITE_MMU_THDM_MCQE_QUEUE_OFFSET_0m(unit, base + idx, mcqe_q_off_x);

            MMU_THDM_MCQE_QUEUE_RESUME_OFFSET_PROFILE_YELLOWr_SET(mcqe_yprof, 1);
            ioerr += WRITE_MMU_THDM_MCQE_QUEUE_RESUME_OFFSET_PROFILE_YELLOWr(unit,
                                                                0, mcqe_yprof);

            MMU_THDM_MCQE_QUEUE_RESUME_OFFSET_PROFILE_REDr_SET(mcqe_rprof, 1);
            ioerr += WRITE_MMU_THDM_MCQE_QUEUE_RESUME_OFFSET_PROFILE_REDr(unit,
                                                                0, mcqe_rprof);
        }
        /* Per port per pool */
        CDK_PBMP_ITER(all_pbmp, port) {
            mport = P2M(unit, port);

            if (mport == RDB_MPORT) {
                continue;
            }
            limit = pool_limit;
            limit += 10;
            min_resume_limit = 8;
            rlimit = ((limit - 2 * default_mtu_cells) < min_resume_limit) ?
                    min_resume_limit : limit - 2 * default_mtu_cells;

            MMU_THDM_DB_PORTSP_CONFIG_0m_CLR(db_psp_cfg_x);
            MMU_THDM_DB_PORTSP_CONFIG_0m_SHARED_LIMITf_SET(db_psp_cfg_x, limit);
            fval = (limit + 7) / 8;
            MMU_THDM_DB_PORTSP_CONFIG_0m_YELLOW_SHARED_LIMITf_SET(db_psp_cfg_x, fval);
            MMU_THDM_DB_PORTSP_CONFIG_0m_RED_SHARED_LIMITf_SET(db_psp_cfg_x, fval);
            MMU_THDM_DB_PORTSP_CONFIG_0m_SHARED_LIMIT_ENABLEf_SET(db_psp_cfg_x, 0);
            fval = (rlimit + 7) / 8;
            MMU_THDM_DB_PORTSP_CONFIG_0m_SHARED_RESUME_LIMITf_SET(db_psp_cfg_x, fval);
            MMU_THDM_DB_PORTSP_CONFIG_0m_YELLOW_RESUME_LIMITf_SET(db_psp_cfg_x, fval);
            MMU_THDM_DB_PORTSP_CONFIG_0m_RED_RESUME_LIMITf_SET(db_psp_cfg_x, fval);
            WRITE_MMU_THDM_DB_PORTSP_CONFIG_0m(unit, mport, db_psp_cfg_x);

            limit = total_mcq_entry - total_rsvd;
            rlimit = (limit < min_resume_limit) ? min_resume_limit : limit;
            MMU_THDM_MCQE_PORTSP_CONFIG_0m_CLR(mcqe_psp_cfg_x);
            fval = (limit + 3)/4;
            MMU_THDM_MCQE_PORTSP_CONFIG_0m_SHARED_LIMITf_SET(mcqe_psp_cfg_x, fval);
            fval = (rlimit / 8) - 1;
            MMU_THDM_MCQE_PORTSP_CONFIG_0m_SHARED_RESUME_LIMITf_SET(mcqe_psp_cfg_x, fval);
            MMU_THDM_MCQE_PORTSP_CONFIG_0m_SHARED_LIMIT_ENABLEf_SET(mcqe_psp_cfg_x, 0);
            fval = (limit + 7) / 8;
            MMU_THDM_MCQE_PORTSP_CONFIG_0m_RED_SHARED_LIMITf_SET(mcqe_psp_cfg_x, fval);
            MMU_THDM_MCQE_PORTSP_CONFIG_0m_YELLOW_SHARED_LIMITf_SET(mcqe_psp_cfg_x, fval);
            fval = fval - 1;
            MMU_THDM_MCQE_PORTSP_CONFIG_0m_YELLOW_RESUME_LIMITf_SET(mcqe_psp_cfg_x, fval);
            MMU_THDM_MCQE_PORTSP_CONFIG_0m_RED_RESUME_LIMITf_SET(mcqe_psp_cfg_x, fval);
            ioerr += WRITE_MMU_THDM_MCQE_PORTSP_CONFIG_0m(unit, mport, mcqe_psp_cfg_x);
        }
    }


    /* Output port per port per queue setting for regular unicast queue */
    CDK_PBMP_ITER(all_pbmp, port) {
        mport = P2M(unit, port);

        if (mport == RDB_MPORT) {
            continue;
        }
        if (mport == CMIC_MPORT) {
            base = 0;
        } else if (mport == LB_MPORT) {
            base = 48 + 10 * (port_count - 2);
        } else {
            base = 48 + 10 * (mport - 1);
        }

        numq = _port_numq(unit, mport);

        MMU_THDU_XPIPE_CONFIG_QUEUEm_CLR(cfg_q_x);
        fval = MMU_BUFQ_MIN;
        MMU_THDU_XPIPE_CONFIG_QUEUEm_Q_MIN_LIMIT_CELLf_SET(cfg_q_x, fval);
        MMU_THDU_XPIPE_CONFIG_QUEUEm_Q_SHARED_LIMIT_CELLf_SET(cfg_q_x, pool_limit);

        fval = (pool_limit + 7) / 8;
        MMU_THDU_XPIPE_CONFIG_QUEUEm_LIMIT_RED_CELLf_SET(cfg_q_x, fval);
        MMU_THDU_XPIPE_CONFIG_QUEUEm_LIMIT_YELLOW_CELLf_SET(cfg_q_x, fval);

        fval = pool_resume / 8;
        MMU_THDU_XPIPE_OFFSET_QUEUEm_CLR(off_q_x);
        MMU_THDU_XPIPE_OFFSET_QUEUEm_RESET_OFFSET_CELLf_SET(off_q_x, fval);
        MMU_THDU_XPIPE_OFFSET_QUEUEm_RESET_OFFSET_YELLOW_CELLf_SET(off_q_x, fval);
        MMU_THDU_XPIPE_OFFSET_QUEUEm_RESET_OFFSET_RED_CELLf_SET(off_q_x, fval);

        MMU_THDU_XPIPE_Q_TO_QGRP_MAPm_CLR(q2qgrp_x);
        port_count = 72;
        for (idx = 0; idx < (12 * port_count) ; idx++) {
            ioerr += WRITE_MMU_THDU_XPIPE_CONFIG_QUEUEm(unit, base + idx, cfg_q_x);
            ioerr += WRITE_MMU_THDU_XPIPE_OFFSET_QUEUEm(unit, base + idx, off_q_x);
            ioerr += WRITE_MMU_THDU_XPIPE_Q_TO_QGRP_MAPm(unit, base + idx, q2qgrp_x);
        }

        /* Per  port per pool unicast */
        limit = pool_limit;
        MMU_THDU_XPIPE_CONFIG_PORTm_CLR(pcfg_x);
        MMU_THDU_XPIPE_CONFIG_PORTm_SHARED_LIMITf_SET(pcfg_x, limit);
        fval = (limit + 7) / 8;
        MMU_THDU_XPIPE_CONFIG_PORTm_YELLOW_LIMITf_SET(pcfg_x, fval);
        MMU_THDU_XPIPE_CONFIG_PORTm_RED_LIMITf_SET(pcfg_x, fval);

        fval = ((limit - (default_mtu_cells * 2)) + 7) / 8;
        MMU_THDU_XPIPE_RESUME_PORTm_SHARED_RESUMEf_SET(prsm_x, fval);
        MMU_THDU_XPIPE_RESUME_PORTm_YELLOW_RESUMEf_SET(prsm_x, fval);
        MMU_THDU_XPIPE_RESUME_PORTm_RED_RESUMEf_SET(prsm_x, fval);

        for (idx = 0; idx < (port_count * 4); idx += MMU_NUM_POOL) {
            ioerr += WRITE_MMU_THDU_XPIPE_CONFIG_PORTm(unit, idx, pcfg_x);
            ioerr += WRITE_MMU_THDU_XPIPE_RESUME_PORTm(unit, idx, prsm_x);
        }
    }

    /* Per-queue RQE multicast output thresholds */
    MMU_THDR_DB_LIMIT_MIN_PRIQr_SET(db_min_pq, 8);

    MMU_THDR_DB_CONFIG1_PRIQr_CLR(db_cfg1_pq);

    fval = (pool_limit + 7) / 8;
    MMU_THDR_DB_LIMIT_COLOR_PRIQr_CLR(db_lim_col_pq);
    MMU_THDR_DB_LIMIT_COLOR_PRIQr_SHARED_YELLOW_LIMITf_SET(db_lim_col_pq, fval);
    MMU_THDR_DB_LIMIT_COLOR_PRIQr_SHARED_RED_LIMITf_SET(db_lim_col_pq, fval);

    MMU_THDR_DB_CONFIG_PRIQr_CLR(db_cfg_pq);
    MMU_THDR_DB_CONFIG_PRIQr_SHARED_LIMITf_SET(db_cfg_pq, pool_limit);
    MMU_THDR_DB_CONFIG_PRIQr_RESET_OFFSETf_SET(db_cfg_pq, 2);

    fval = (default_mtu_cells * 2) / 8;
    MMU_THDR_DB_RESET_OFFSET_COLOR_PRIQr_CLR(db_lcol_pq);
    MMU_THDR_DB_RESET_OFFSET_COLOR_PRIQr_RESET_OFFSET_YELLOWf_SET(db_lcol_pq, fval);
    MMU_THDR_DB_RESET_OFFSET_COLOR_PRIQr_RESET_OFFSET_REDf_SET(db_lcol_pq, fval);

    /* queue entry */
    MMU_THDR_QE_LIMIT_MIN_PRIQr_CLR(qe_min_pq);
    fval = 1;
    MMU_THDR_QE_LIMIT_MIN_PRIQr_MIN_LIMITf_SET(qe_min_pq, fval);

    MMU_THDR_QE_CONFIG1_PRIQr_CLR(qe_cfg1_pq);

    MMU_THDR_QE_LIMIT_COLOR_PRIQr_CLR(qe_lim_col_pq);
    total_rqe_entry = MMU_RQE_ENTRY - 88;
    fval = (total_rqe_entry + 7) / 8;
    MMU_THDR_QE_LIMIT_COLOR_PRIQr_SHARED_YELLOW_LIMITf_SET(qe_lim_col_pq, fval);
    MMU_THDR_QE_LIMIT_COLOR_PRIQr_SHARED_RED_LIMITf_SET(qe_lim_col_pq, fval);

    MMU_THDR_QE_CONFIG_PRIQr_CLR(qe_cfg_pq);
    fval = (total_rqe_entry + 7) / 8;
    MMU_THDR_QE_CONFIG_PRIQr_SHARED_LIMITf_SET(qe_cfg_pq, fval);
    MMU_THDR_QE_CONFIG_PRIQr_RESET_OFFSETf_SET(qe_cfg_pq, 1);

    MMU_THDR_QE_RESET_OFFSET_COLOR_PRIQr_CLR(qe_lcol_pq);
    MMU_THDR_QE_RESET_OFFSET_COLOR_PRIQr_RESET_OFFSET_YELLOWf_SET(qe_lcol_pq, 1);
    MMU_THDR_QE_RESET_OFFSET_COLOR_PRIQr_RESET_OFFSET_REDf_SET(qe_lcol_pq, 1);

    for (idx = 0; idx < MMU_NUM_RQE; idx++) {
        ioerr += WRITE_MMU_THDR_DB_CONFIG_PRIQr(unit, idx, db_cfg_pq);
        ioerr += WRITE_MMU_THDR_DB_LIMIT_COLOR_PRIQr(unit, idx, db_lim_col_pq);
        ioerr += WRITE_MMU_THDR_DB_LIMIT_MIN_PRIQr(unit, idx, db_min_pq);
        ioerr += WRITE_MMU_THDR_DB_CONFIG1_PRIQr(unit, idx, db_cfg1_pq);
        ioerr += WRITE_MMU_THDR_DB_RESET_OFFSET_COLOR_PRIQr(unit, idx, db_lcol_pq);
        ioerr += WRITE_MMU_THDR_QE_LIMIT_MIN_PRIQr(unit, idx, qe_min_pq);
        ioerr += WRITE_MMU_THDR_QE_CONFIG1_PRIQr(unit, idx, qe_cfg1_pq);
        ioerr += WRITE_MMU_THDR_QE_LIMIT_COLOR_PRIQr(unit, idx, qe_lim_col_pq);
        ioerr += WRITE_MMU_THDR_QE_CONFIG_PRIQr(unit, idx, qe_cfg_pq);
        ioerr += WRITE_MMU_THDR_QE_RESET_OFFSET_COLOR_PRIQr(unit, idx, qe_lcol_pq);
    }

    /* Per-pool RQE multicast output thresholds */
    MMU_THDR_DB_CONFIG_SPr_CLR(db_cfg_sp);
    fval = pool_limit;
    MMU_THDR_DB_CONFIG_SPr_SHARED_LIMITf_SET(db_cfg_sp, fval);
    fval = (pool_limit - (default_mtu_cells * 2) + 7) / 8;
    MMU_THDR_DB_CONFIG_SPr_RESUME_LIMITf_SET(db_cfg_sp, fval);
    ioerr += WRITE_MMU_THDR_DB_CONFIG_SPr(unit, 0, db_cfg_sp);

    MMU_THDR_DB_SP_SHARED_LIMITr_CLR(db_lim_sp);
    fval = (pool_limit + 7) / 8;
    MMU_THDR_DB_SP_SHARED_LIMITr_SHARED_YELLOW_LIMITf_SET(db_lim_sp, fval);
    MMU_THDR_DB_SP_SHARED_LIMITr_SHARED_RED_LIMITf_SET(db_lim_sp, fval);
    ioerr += WRITE_MMU_THDR_DB_SP_SHARED_LIMITr(unit, 0, db_lim_sp);

    MMU_THDR_DB_RESUME_COLOR_LIMIT_SPr_CLR(db_rsmc_sp);
    fval = (pool_limit - (default_mtu_cells * 2) + 7) / 8;
    MMU_THDR_DB_RESUME_COLOR_LIMIT_SPr_RESUME_YELLOW_LIMITf_SET(db_rsmc_sp, fval);
    MMU_THDR_DB_RESUME_COLOR_LIMIT_SPr_RESUME_RED_LIMITf_SET(db_rsmc_sp, fval);
    ioerr += WRITE_MMU_THDR_DB_RESUME_COLOR_LIMIT_SPr(unit, 0, db_rsmc_sp);

    MMU_THDR_QE_CONFIG_SPr_CLR(qe_cfg_sp);
    fval = (total_rqe_entry + 7) / 8 - 1;
    MMU_THDR_QE_CONFIG_SPr_SHARED_LIMITf_SET(qe_cfg_sp, fval);
    fval = fval - 1;
    MMU_THDR_QE_CONFIG_SPr_RESUME_LIMITf_SET(qe_cfg_sp, fval);
    ioerr += WRITE_MMU_THDR_QE_CONFIG_SPr(unit, 0, qe_cfg_sp);

    MMU_THDR_QE_SHARED_COLOR_LIMIT_SPr_CLR(qe_lim_sp);
    fval = (total_rqe_entry + 7) / 8 - 1;
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

#define LLS_RESET_TIMEOUT_MSEC          50
#define CMIC_L0_IDX                   0x3fa
#define LLS_NUM_L2UC                    12
static int
_mmu_lls_init(int unit)
{
    int ioerr = 0;
    int level, index_max, idx_0, idx, sub;
    LLS_S1_PARENTm_t s1_parent;
    LLS_S1_CONFIGm_t s1_config;
    LLS_L0_PARENTm_t l0_parent;
    LLS_L0_CONFIGm_t l0_config;
    LLS_L1_PARENTm_t l1_parent;
    LLS_L1_CONFIGm_t l1_config;
    LLS_L2_PARENTm_t l2_parent;
    /*  Initialize LLS tree to reset state */
    /* Init LLS  */
    LLS_INITr_t lls_init;
    /* Use front port 1 to get each fron port number of queue */
    int port_numq = _port_numq(unit, 1);
    /* Get CMIC port number of queue */
    int cpu_port_numq = _port_numq(unit, CMIC_MPORT);
    /* L1 node front port starting entry */
    int start_l1_idx = 8;
    int txq;
    int lport, mport;
    uint32_t fval;

    ioerr += READ_LLS_INITr(unit, &lls_init);
    LLS_INITr_INITf_SET(lls_init, 1);
    ioerr += WRITE_LLS_INITr(unit, lls_init);

    /* Check INIT_DONE in LLS block */
    BMD_SYS_USLEEP(1000);
    for (idx = 0; idx < LLS_RESET_TIMEOUT_MSEC; idx++) {
        ioerr += READ_LLS_INITr(unit, &lls_init);
        if (LLS_INITr_INIT_DONEf_GET(lls_init)) {
            break;
        }
        BMD_SYS_USLEEP(1000);
    }
    if (idx >= LLS_RESET_TIMEOUT_MSEC) {
        CDK_WARN(("bcm56560_b0_bmd_init[%d]: LLS INIT timeout\n", unit));
        return ioerr ? CDK_E_IO : CDK_E_TIMEOUT;
    }

    /* Build the tree from S1-> L0 -> L1 -> L2(unicast and multicast) */
    /* Map S1's parent node to MMU port, index is logical port */
    /* MMU_port: [0x48], [mport1],  [mport2],  [mport3], ...,
       S1 :        [0],     [1],       [2],       [3]...,
       L0 :   [CMIC_L0_IDX],[0],       [4],       [8]...,
       L1 :        [0],     [8],       [9],       [10]...,
       L2(UC) :    [0],[mport1*queue#],[mport2*queue#],[mport3*queue#],...,
         (MC):  [0x42da]  [0x4028]
     */
    for (level = NODE_LVL_S1; level <= NODE_LVL_L2; level++) {
        if (level == NODE_LVL_S1) {
            index_max = LLS_S1_PARENTm_MAX;
            for (idx_0 = 0; idx_0 <= index_max; idx_0++) {
                ioerr += READ_LLS_S1_PARENTm(unit, idx_0, &s1_parent);
                if (idx_0 == 0) {
                    /* CMIC */
                    LLS_S1_PARENTm_P_PORTf_SET(s1_parent, CMIC_MPORT);
                } else if (idx_0 == LB_MPORT) {
                    LLS_S1_PARENTm_P_PORTf_SET(s1_parent, LB_MPORT);
                } else if (idx_0 < LB_MPORT) {
                    LLS_S1_PARENTm_P_PORTf_SET(s1_parent, P2M(unit, L2P(unit, idx_0)));
                } else {
                    LLS_S1_PARENTm_P_PORTf_SET(s1_parent, 0x7f);
                }
                ioerr += WRITE_LLS_S1_PARENTm(unit, idx_0, s1_parent);

                ioerr += READ_LLS_S1_CONFIGm(unit, idx_0, &s1_config);
                if (idx_0 > LB_MPORT){
                    LLS_S1_CONFIGm_PACKET_MODE_WRR_ACCOUNTING_ENABLEf_SET(s1_config, 1);
                    LLS_S1_CONFIGm_P_START_SPRIf_SET(s1_config, 0);
                } else if (idx_0 == 0) {
                    /* CMIC port */
                    LLS_S1_CONFIGm_PACKET_MODE_WRR_ACCOUNTING_ENABLEf_SET(s1_config, 1);
                    LLS_S1_CONFIGm_P_START_SPRIf_SET(s1_config, CMIC_L0_IDX);
                } else {
                    LLS_S1_CONFIGm_PACKET_MODE_WRR_ACCOUNTING_ENABLEf_SET(s1_config, 1);
                    LLS_S1_CONFIGm_P_START_SPRIf_SET(s1_config, ((idx_0  - 1) * 4));
                }
                ioerr += WRITE_LLS_S1_CONFIGm(unit, idx_0, s1_config);
            }
        } else if (level == NODE_LVL_L0) {
            index_max = LLS_L0_PARENTm_MAX;

            for (idx_0 = 0; idx_0 <= index_max; idx_0++) {
                ioerr += READ_LLS_L0_PARENTm(unit, idx_0, &l0_parent);
                LLS_L0_PARENTm_C_PARENTf_SET(l0_parent, LLS_S1_PARENTm_MAX);
                ioerr += WRITE_LLS_L0_PARENTm(unit, idx_0, l0_parent);
            }
            for (idx_0 = 0; idx_0 <= index_max; idx_0++) {
                ioerr += READ_LLS_L0_PARENTm(unit, idx_0, &l0_parent);
                if (idx_0 == CMIC_L0_IDX) {
                    /* CMIC port */
                    LLS_L0_PARENTm_C_PARENTf_SET(l0_parent, 0);
                } else if (((idx_0 % 4) == 0) && ((idx_0 / 4) < LB_MPORT)) {
                    LLS_L0_PARENTm_C_PARENTf_SET(l0_parent, (idx_0 / 4) + 1);
                }
                ioerr += WRITE_LLS_L0_PARENTm(unit, idx_0, l0_parent);
            }
            for (idx_0 = 0; idx_0 <= index_max; idx_0++) {
                ioerr += READ_LLS_L0_CONFIGm(unit, idx_0, &l0_config);
                lport = idx_0 / 4 + 1;
                if (idx_0 == CMIC_L0_IDX) {
                    LLS_L0_CONFIGm_P_START_SPRIf_SET(l0_config, 0);
                } else if (((idx_0 % 4) == 0) && ((idx_0 / 4) < LB_MPORT)) {
                    fval = start_l1_idx + (lport - 1);
                    LLS_L0_CONFIGm_P_START_SPRIf_SET(l0_config, fval);
                } else {
                    LLS_L0_CONFIGm_P_START_SPRIf_SET(l0_config, 0);
                }
                ioerr += WRITE_LLS_L0_CONFIGm(unit, idx_0, l0_config);
            }
        } else if (level == NODE_LVL_L1) {
            index_max = LLS_L1_PARENTm_MAX;
            for (idx_0 = 0; idx_0 <= index_max; idx_0++) {
                ioerr += READ_LLS_L1_PARENTm(unit, idx_0, &l1_parent);
                LLS_L1_PARENTm_C_PARENTf_SET(l1_parent, LLS_L0_PARENTm_MAX);
                ioerr += WRITE_LLS_L1_PARENTm(unit, idx_0, l1_parent);
            }
            for (idx_0 = 0; idx_0 <= index_max; idx_0++) {
                ioerr += READ_LLS_L1_PARENTm(unit, idx_0, &l1_parent);
                if (idx_0 < start_l1_idx) {
                    /* CMIC port */
                    LLS_L1_PARENTm_C_PARENTf_SET(l1_parent, CMIC_L0_IDX);
                } else {
                    idx = (idx_0 - start_l1_idx) / LLS_NUM_L2UC;
                    sub = (idx_0 - start_l1_idx) % LLS_NUM_L2UC;

                    if ((idx < LB_MPORT) &&
                        (idx_0 >= start_l1_idx)) {
                        LLS_L1_PARENTm_C_PARENTf_SET(l1_parent, (idx_0 - 8) * 4);
                    } else {
                        LLS_L1_PARENTm_C_PARENTf_SET(l1_parent, LLS_L1_PARENTm_MAX);
                    }
                }
                ioerr += WRITE_LLS_L1_PARENTm(unit, idx_0, l1_parent);

                ioerr += READ_LLS_L1_CONFIGm(unit, idx_0, &l1_config);
                if (idx_0 <= 5) {
                    /* CMIC port */
                    LLS_L1_CONFIGm_P_START_UC_SPRIf_SET(l1_config, 0);
                    LLS_L1_CONFIGm_P_START_MC_SPRIf_SET(l1_config,
                        MC_Q_BASE_CMIC_PORT + 8 * idx_0);
                } else {
                    if ((idx < LB_MPORT) && (idx_0 >= 8)) {
                        lport = idx_0 - 8 + 1;
                        mport = P2M(unit, L2P(unit, lport));
                        txq = bcm56560_b0_mmu_port_mc_queues(unit, L2P(unit, lport)) * mport;
                        LLS_L1_CONFIGm_P_START_UC_SPRIf_SET(l1_config,
                            bcm56560_b0_mmu_port_uc_queue_index(unit, L2P(unit, lport)));
                        LLS_L1_CONFIGm_P_START_MC_SPRIf_SET(l1_config, MC_Q_BASE + txq);
                        LLS_L1_CONFIGm_P_NUM_SPRIf_SET(l1_config, port_numq);
                    }
                }
                ioerr += WRITE_LLS_L1_CONFIGm(unit, idx_0, l1_config);
            }
        } else {
            index_max = LLS_L2_PARENTm_MAX;
            for (idx_0 = 0; idx_0 <= index_max; idx_0++) {
                ioerr += READ_LLS_L2_PARENTm(unit, idx_0, &l2_parent);
                LLS_L2_PARENTm_C_PARENTf_SET(l2_parent, LLS_L1_PARENTm_MAX);
                ioerr += WRITE_LLS_L2_PARENTm(unit, idx_0, l2_parent);
            }
            for (idx_0 = 0; idx_0 <= index_max; idx_0++) {
                ioerr += READ_LLS_L2_PARENTm(unit, idx_0, &l2_parent);
                if ((idx_0 >= MC_Q_BASE_CMIC_PORT) &&
                    (idx_0 <= (MC_Q_BASE_CMIC_PORT + cpu_port_numq - 1))) {

                    /* CMIC port */
                    idx = (idx_0 - MC_Q_BASE_CMIC_PORT) / 8;
                    LLS_L2_PARENTm_C_PARENTf_SET(l2_parent, idx);
                    ioerr += WRITE_LLS_L2_PARENTm(unit, idx_0, l2_parent);
                } else {
                    if (idx_0 < MC_Q_BASE) {
                        /* unicast entries */
                        sub = idx_0 % LLS_NUM_L2UC;
                        /* Get logical port1's txq for all ports */
                        txq = bcm56560_b0_mmu_port_uc_queues(unit, 1);
                        mport = idx_0 / txq;
                        if (idx_0 < (LB_MPORT * txq + 1)) {
                            fval = P2L(unit, M2P(unit, mport)) + (8 - 1);
                            LLS_L2_PARENTm_C_PARENTf_SET(l2_parent, fval);
                            ioerr += WRITE_LLS_L2_PARENTm(unit, idx_0, l2_parent);
                        }
                    } else {
                        /* multicast entries */
                        idx = (idx_0 - MC_Q_BASE);
                        sub = (idx_0 - MC_Q_BASE) % port_numq;
                        txq = bcm56560_b0_mmu_port_mc_queues(unit, 1);
                        mport = idx / txq;
                        if ((sub < port_numq) && ((idx / port_numq) <= CMIC_MPORT)) {
                            fval = P2L(unit, M2P(unit, mport)) + (8 - 1);
                            LLS_L2_PARENTm_C_PARENTf_SET(l2_parent, fval);
                            ioerr += WRITE_LLS_L2_PARENTm(unit, idx_0, l2_parent);
                        }
                    }
                }
            }
        }
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

static void
_fc_config_init(int unit)
{
    int ioerr = 0;
    int port;
    cdk_pbmp_t pbmp;
    CLPORT_FLOW_CONTROL_CONFIGr_t clport_ctrl;
    CLG2PORT_FLOW_CONTROL_CONFIGr_t clgport_ctrl;
    XLPORT_FLOW_CONTROL_CONFIGr_t xlport_ctrl;

    /* Per-port settings */
    bcm56560_b0_clport_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        ioerr += READ_CLPORT_FLOW_CONTROL_CONFIGr(unit, port, &clport_ctrl);
        CLPORT_FLOW_CONTROL_CONFIGr_MERGE_MODE_ENf_SET(clport_ctrl, 1);
        CLPORT_FLOW_CONTROL_CONFIGr_PARALLEL_FC_ENf_SET(clport_ctrl, 1);
        ioerr += WRITE_CLPORT_FLOW_CONTROL_CONFIGr(unit, port, clport_ctrl);
    }

    bcm56560_b0_clg2port_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        ioerr += READ_CLG2PORT_FLOW_CONTROL_CONFIGr(unit, port, &clgport_ctrl);
        CLG2PORT_FLOW_CONTROL_CONFIGr_MERGE_MODE_ENf_SET(clgport_ctrl, 1);
        CLG2PORT_FLOW_CONTROL_CONFIGr_PARALLEL_FC_ENf_SET(clgport_ctrl, 1);
        ioerr += WRITE_CLG2PORT_FLOW_CONTROL_CONFIGr(unit, port, clgport_ctrl);
    }

    bcm56560_b0_xlport_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        ioerr += READ_XLPORT_FLOW_CONTROL_CONFIGr(unit, port, &xlport_ctrl);
        XLPORT_FLOW_CONTROL_CONFIGr_MERGE_MODE_ENf_SET(xlport_ctrl, 1);
        XLPORT_FLOW_CONTROL_CONFIGr_PARALLEL_FC_ENf_SET(xlport_ctrl, 1);
        ioerr += WRITE_XLPORT_FLOW_CONTROL_CONFIGr(unit, port, xlport_ctrl);
    }
}

/*Clear MMU_CBPDATA for APACHE mmu_init*/
static void
_mmu_cbpdata_clear(int unit)
{
    int idx;
    MMU_CBPDATA0m_t cbpdata0;
    MMU_CBPDATA1m_t cbpdata1;
    MMU_CBPDATA2m_t cbpdata2;
    MMU_CBPDATA3m_t cbpdata3;

    MMU_CBPDATA0m_CLR(cbpdata0);
    for (idx = MMU_CBPDATA0m_MIN; idx <= MMU_CBPDATA0m_MAX; idx++) {
        WRITE_MMU_CBPDATA0m(unit, idx, cbpdata0);
    }
    MMU_CBPDATA1m_CLR(cbpdata1);
    for (idx = MMU_CBPDATA1m_MIN; idx <= MMU_CBPDATA1m_MAX; idx++) {
        WRITE_MMU_CBPDATA1m(unit, idx, cbpdata1);
    }
    MMU_CBPDATA2m_CLR(cbpdata2);
    for (idx = MMU_CBPDATA2m_MIN; idx <= MMU_CBPDATA2m_MAX; idx++) {
        WRITE_MMU_CBPDATA2m(unit, idx, cbpdata2);
    }
    MMU_CBPDATA3m_CLR(cbpdata3);
    for (idx = MMU_CBPDATA3m_MIN; idx <= MMU_CBPDATA3m_MAX; idx++) {
        WRITE_MMU_CBPDATA3m(unit, idx, cbpdata3);
    }
}

static int
_mmu_init(int unit)
{
    int ioerr = 0;
    LLS_CONFIG0r_t lls_cfg0;
    PRIORITY_CONTROLr_t pri_ctrl;
    IP_TO_CMICM_CREDIT_TRANSFERr_t cred_xfer;
    THDU_OUTPUT_PORT_RX_ENABLE0_64r_t outp_rx_en_0;
    THDU_OUTPUT_PORT_RX_ENABLE1_64r_t outp_rx_en_1;
    MMU_THDM_DB_PORTSP_RX_ENABLE0_64r_t db_rx_en_0;
    MMU_THDM_DB_PORTSP_RX_ENABLE1_64r_t db_rx_en_1;
    MMU_THDM_MCQE_PORTSP_RX_ENABLE0_64r_t mcqe_rx_en_0;
    MMU_THDM_MCQE_PORTSP_RX_ENABLE1_64r_t mcqe_rx_en_1;
    WRED_REFRESH_CONTROLr_t wred_ref_ctrl;
    int rv;
    int total_entries, clock_period, refresh_time_val;
    int cpu_slots;
    int msbus_idle_enable;
    int core_freq;

    core_freq = bcm56560_b0_get_core_frequency(unit);

    _fc_config_init(unit);

    _mmu_config_init(unit, -1);

    LLS_CONFIG0r_CLR(lls_cfg0);
    LLS_CONFIG0r_FC_ENABLEf_SET(lls_cfg0, 1);
    LLS_CONFIG0r_ENQUEUE_ENABLEf_SET(lls_cfg0, 1);
    LLS_CONFIG0r_DEQUEUE_ENABLEf_SET(lls_cfg0, 1);
    LLS_CONFIG0r_PORT_SCHEDULER_ENABLEf_SET(lls_cfg0, 1);
    ioerr += WRITE_LLS_CONFIG0r(unit, lls_cfg0);

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
    THDU_OUTPUT_PORT_RX_ENABLE0_64r_SET(outp_rx_en_0, 0, 0xffffffff);
    THDU_OUTPUT_PORT_RX_ENABLE0_64r_SET(outp_rx_en_0, 1, 0xffffffff);
    ioerr += WRITE_THDU_OUTPUT_PORT_RX_ENABLE0_64r(unit, outp_rx_en_0);
    MMU_THDM_DB_PORTSP_RX_ENABLE0_64r_SET(db_rx_en_0, 0, 0xffffffff);
    MMU_THDM_DB_PORTSP_RX_ENABLE0_64r_SET(db_rx_en_0, 1, 0xffffffff);
    ioerr += WRITE_MMU_THDM_DB_PORTSP_RX_ENABLE0_64r(unit, db_rx_en_0);
    MMU_THDM_MCQE_PORTSP_RX_ENABLE0_64r_SET(mcqe_rx_en_0, 0, 0xffffffff);
    MMU_THDM_MCQE_PORTSP_RX_ENABLE0_64r_SET(mcqe_rx_en_0, 1, 0xffffffff);
    ioerr += WRITE_MMU_THDM_MCQE_PORTSP_RX_ENABLE0_64r(unit, mcqe_rx_en_0);

    THDU_OUTPUT_PORT_RX_ENABLE1_64r_SET(outp_rx_en_1, 0x3ff);
    ioerr += WRITE_THDU_OUTPUT_PORT_RX_ENABLE1_64r(unit, outp_rx_en_1);
    MMU_THDM_DB_PORTSP_RX_ENABLE1_64r_SET(db_rx_en_1, 0x3ff);
    ioerr += WRITE_MMU_THDM_DB_PORTSP_RX_ENABLE1_64r(unit, db_rx_en_1);
    MMU_THDM_MCQE_PORTSP_RX_ENABLE1_64r_SET(mcqe_rx_en_1, 0x3ff);
    ioerr += WRITE_MMU_THDM_MCQE_PORTSP_RX_ENABLE1_64r(unit, mcqe_rx_en_1);

    /*
     * Refresh Value =
     *  ((total no of entities to be refreshed) X Core clk period in ns)/Wred tick period 50 ns
     */

    total_entries = 4096 + 1024;
    cpu_slots = (total_entries * 2) / 16; /* 2 slots for every 16 entries refreshed */
    clock_period = (1 * 1000 * 1000) / core_freq; /*clock period in microseconds */

    ioerr += READ_WRED_REFRESH_CONTROLr(unit, &wred_ref_ctrl);
    msbus_idle_enable = WRED_REFRESH_CONTROLr_MSBUS_IDLE_ENABLEf_GET(wred_ref_ctrl);

    if (msbus_idle_enable) {
        refresh_time_val = ((total_entries + cpu_slots) * clock_period) / 50;
    } else {
        refresh_time_val = (total_entries * clock_period) / 50;
    }
    refresh_time_val = refresh_time_val / 1000;  /* micro to nano seconds */

    WRED_REFRESH_CONTROLr_REFRESH_TIME_VALf_SET(wred_ref_ctrl, refresh_time_val);
    WRED_REFRESH_CONTROLr_REFRESH_DISABLEf_SET(wred_ref_ctrl, 0);
    ioerr += WRITE_WRED_REFRESH_CONTROLr(unit, wred_ref_ctrl);
    /* Clear MMU_CBPData */
    _mmu_cbpdata_clear(unit);

    rv = _mmu_lls_init(unit);
    if (CDK_FAILURE(rv)) {
        return rv;
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

static int
_xl_firmware_helper(void *ctx, uint32_t offset, uint32_t size, void *data)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    XLPORT_WC_UCMEM_CTRLr_t xl_ucmem_ctrl;
    XLPORT_WC_UCMEM_DATAm_t xl_ucmem_data;
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
    ioerr += READ_XLPORT_WC_UCMEM_CTRLr(unit, &xl_ucmem_ctrl, port);
    XLPORT_WC_UCMEM_CTRLr_ACCESS_MODEf_SET(xl_ucmem_ctrl, 1);
    ioerr += WRITE_XLPORT_WC_UCMEM_CTRLr(unit, xl_ucmem_ctrl, port);

    /* We need to byte swap on little endian host */
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
            XLPORT_WC_UCMEM_DATAm_SET(xl_ucmem_data, wdx, wdata);
        }
        WRITE_XLPORT_WC_UCMEM_DATAm(unit, (idx >> 4), xl_ucmem_data, port);
    }

    /* Disable parallel bus access */
    XLPORT_WC_UCMEM_CTRLr_ACCESS_MODEf_SET(xl_ucmem_ctrl, 0);
    ioerr += WRITE_XLPORT_WC_UCMEM_CTRLr(unit, xl_ucmem_ctrl, port);

    return ioerr ? CDK_E_IO : rv;
}

static int
_wcport(int port)
{
    return (port <= 28) ? 17 : 53;
}

static int
_wcinst(int port)
{
    port -= _wcport(port);
    return port / 4;
}

static int
_cxx_firmware_helper(void *ctx, uint32_t offset, uint32_t size, void *data)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    CXXPORT_WC_UCMEM_CTRLr_t cxx_ucmem_ctrl;
    CXXPORT_WC_UCMEM_DATA0m_t cxx_ucmem_data0;
    CXXPORT_WC_UCMEM_DATA1m_t cxx_ucmem_data1;
    CXXPORT_WC_UCMEM_DATA2m_t cxx_ucmem_data2;
    int unit, port;
    const char *drv_name;
    uint32_t wbuf[4];
    uint32_t wdata;
    uint32_t *fw_data;
    uint32_t *fw_entry;
    uint32_t fw_size;
    uint32_t idx, wdx;
    uint32_t le_host;
    int wcport, wcinst;

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

    wcport = _wcport(port);
    wcinst = _wcinst(port);
    ioerr += READ_CXXPORT_WC_UCMEM_CTRLr(unit, wcport, wcinst, &cxx_ucmem_ctrl);
    CXXPORT_WC_UCMEM_CTRLr_ACCESS_MODEf_SET(cxx_ucmem_ctrl, 1);
    ioerr += WRITE_CXXPORT_WC_UCMEM_CTRLr(unit, wcport, wcinst, cxx_ucmem_ctrl);

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
        for (wdx = 0; wdx <= 3; wdx++) {
            wdata = fw_entry[3 - wdx];

            wdata = ((wdata & 0xff000000) >> 24) | ((wdata & 0x00ff0000) >> 8)
                | ((wdata & 0x0000ff00) << 8) | ((wdata & 0x000000ff) << 24);
            if (le_host) {
                wdata = cdk_util_swap32(wdata);
            }

            if (wcinst == 0) {
                CXXPORT_WC_UCMEM_DATA0m_SET(cxx_ucmem_data0, wdx^3, wdata);
            } else if (wcinst == 1) {
                CXXPORT_WC_UCMEM_DATA1m_SET(cxx_ucmem_data1, wdx^3, wdata);
            } else {
                CXXPORT_WC_UCMEM_DATA2m_SET(cxx_ucmem_data2, wdx^3, wdata);
            }
        }

        if (wcinst == 0) {
            WRITE_CXXPORT_WC_UCMEM_DATA0m(unit, idx >> 4, cxx_ucmem_data0, wcport);
        } else if (wcinst == 1) {
            WRITE_CXXPORT_WC_UCMEM_DATA1m(unit, idx >> 4, cxx_ucmem_data1, wcport);
        } else {
            WRITE_CXXPORT_WC_UCMEM_DATA2m(unit, idx >> 4, cxx_ucmem_data2, wcport);
        }
    }

    /* Disable parallel bus access */
    CXXPORT_WC_UCMEM_CTRLr_ACCESS_MODEf_SET(cxx_ucmem_ctrl, 0);
    ioerr += WRITE_CXXPORT_WC_UCMEM_CTRLr(unit, wcport, wcinst, cxx_ucmem_ctrl);
    
    return ioerr ? CDK_E_IO : rv;
}

static int
_clg_firmware_helper(void *ctx, uint32_t offset, uint32_t size, void *data)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    CLG2PORT_WC_UCMEM_CTRLr_t clg_ucmem_ctrl;
    CLG2PORT_WC_UCMEM_DATAm_t clg_ucmem_data;
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

    if (CDK_STRSTR(drv_name, "tscf") == NULL) {
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
    ioerr += READ_CLG2PORT_WC_UCMEM_CTRLr(unit, &clg_ucmem_ctrl, port);
    CLG2PORT_WC_UCMEM_CTRLr_ACCESS_MODEf_SET(clg_ucmem_ctrl, 1);
    ioerr += WRITE_CLG2PORT_WC_UCMEM_CTRLr(unit, clg_ucmem_ctrl, port);

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
            CLG2PORT_WC_UCMEM_DATAm_SET(clg_ucmem_data, wdx, wdata);
        }
        WRITE_CLG2PORT_WC_UCMEM_DATAm(unit, (idx >> 4), clg_ucmem_data, port);
    }

    /* Disable parallel bus access */
    CLG2PORT_WC_UCMEM_CTRLr_ACCESS_MODEf_SET(clg_ucmem_ctrl, 0);
    ioerr += WRITE_CLG2PORT_WC_UCMEM_CTRLr(unit, clg_ucmem_ctrl, port);

    return ioerr ? CDK_E_IO : rv;
}

static int
_phy_default_ability_set(int unit, int port)
{
    int rv = 0;
    int speed_max, ability = 0;

    ability = (BMD_PHY_ABIL_2500MB | BMD_PHY_ABIL_1000MB_FD |
               BMD_PHY_ABIL_100MB_FD);

    speed_max = bcm56560_b0_port_speed_max(unit, port);
    if (speed_max == 10000) {
        ability |= BMD_PHY_ABIL_10GB;
    } else if (speed_max == 20000) {
        ability |= BMD_PHY_ABIL_20GB;
    } else if (speed_max == 40000) {
        ability |= BMD_PHY_ABIL_40GB;
    } else if (speed_max == 100000) {
        ability |= BMD_PHY_ABIL_100GB;
    }
    rv = bmd_phy_default_ability_set(unit, port, ability);

    return rv;
}

static int
_port_init(int unit, int port)
{
    int ioerr = 0;
    int lport = P2L(unit, port);
    EGR_VLAN_CONTROL_1m_t egr_vlan_ctrl1;
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
    EGR_VLAN_CONTROL_1m_CLR(egr_vlan_ctrl1);
    EGR_VLAN_CONTROL_1m_VT_MISS_UNTAGf_SET(egr_vlan_ctrl1, 0);
    EGR_VLAN_CONTROL_1m_REMARK_OUTER_DOT1Pf_SET(egr_vlan_ctrl1, 1);
    ioerr += WRITE_EGR_VLAN_CONTROL_1m(unit, lport, egr_vlan_ctrl1);

    /* Egress enable */
    ioerr += READ_EGR_ENABLEm(unit, port, &egr_enable);
    EGR_ENABLEm_PRT_ENABLEf_SET(egr_enable, 1);
    ioerr += WRITE_EGR_ENABLEm(unit, port, egr_enable);

    return ioerr;
}

int
bcm56560_b0_clport_init(int unit, int port)
{
    int ioerr = 0;
    int speed;
    uint32_t val;
    CLPORT_SOFT_RESETr_t cp_soft_reset;
    CLPORT_ENABLE_REGr_t cp_en;
    CLMAC_TX_CTRLr_t tx_ctrl;
    CLMAC_RX_CTRLr_t rx_ctrl;
    CLMAC_RX_MAX_SIZEr_t rx_max_size;
    CLMAC_RX_LSS_CTRLr_t rx_lss_ctrl;
    CLMAC_CTRLr_t mac_ctrl;
    CLMAC_MODEr_t mac_mode;
    EGR_PORT_CREDIT_RESETm_t egr_port_credit_reset;

    /* Common port initialization */
    ioerr += _port_init(unit, port);
    speed = SPEED_MAX(unit, port);

    /* Soft reset */
    ioerr += READ_CLPORT_SOFT_RESETr(unit, &cp_soft_reset, port);
    val = CLPORT_SOFT_RESETr_GET(cp_soft_reset);
    val |= 1 << ((port - 1) & 3);
    CLPORT_SOFT_RESETr_SET(cp_soft_reset, val);
    ioerr += WRITE_CLPORT_SOFT_RESETr(unit, cp_soft_reset, port);
    val &= ~(1 << ((port - 1) & 3));
    CLPORT_SOFT_RESETr_SET(cp_soft_reset, val);
    ioerr += WRITE_CLPORT_SOFT_RESETr(unit, cp_soft_reset, port);

    /* Port enable */
    ioerr += READ_CLPORT_ENABLE_REGr(unit, &cp_en, port);
    val = CLPORT_ENABLE_REGr_GET(cp_en);
    val |= 1 << ((port - 1) & 3);
    CLPORT_ENABLE_REGr_SET(cp_en, val);
    ioerr += WRITE_CLPORT_ENABLE_REGr(unit, cp_en, port);

    /* Init MAC */
    /* Ensure that MAC (Rx) and loopback mode is disabled */
    CLMAC_CTRLr_CLR(mac_ctrl);
    CLMAC_CTRLr_SOFT_RESETf_SET(mac_ctrl, 1);
    ioerr += WRITE_CLMAC_CTRLr(unit, port, mac_ctrl);

    /* Reset EP credit before de-assert SOFT_RESET */
    ioerr += READ_EGR_PORT_CREDIT_RESETm(unit, port, &egr_port_credit_reset);
    EGR_PORT_CREDIT_RESETm_VALUEf_SET(egr_port_credit_reset, 1);
    ioerr += WRITE_EGR_PORT_CREDIT_RESETm(unit, port, egr_port_credit_reset);
    BMD_SYS_USLEEP(1000);
    ioerr += READ_EGR_PORT_CREDIT_RESETm(unit, port, &egr_port_credit_reset);
    EGR_PORT_CREDIT_RESETm_VALUEf_SET(egr_port_credit_reset, 0);
    ioerr += WRITE_EGR_PORT_CREDIT_RESETm(unit, port, egr_port_credit_reset);

    CLMAC_CTRLr_SOFT_RESETf_SET(mac_ctrl, 0);
    CLMAC_CTRLr_RX_ENf_SET(mac_ctrl, 0);
    CLMAC_CTRLr_TX_ENf_SET(mac_ctrl, 0);
    val = (BMD_PORT_PROPERTIES(unit, port) == BMD_PORT_HG) ? 1 : 0;
    CLMAC_CTRLr_XGMII_IPG_CHECK_DISABLEf_SET(mac_ctrl, val);
    ioerr += WRITE_CLMAC_CTRLr(unit, port, mac_ctrl);

    /* Configure Rx (strip CRC, strict preamble, IEEE header) */
    ioerr += READ_CLMAC_RX_CTRLr(unit, port, &rx_ctrl);
    CLMAC_RX_CTRLr_STRIP_CRCf_SET(rx_ctrl, 0);
    if (speed >= 10000 && BMD_PORT_PROPERTIES(unit, port) == BMD_PORT_XE) {
        CLMAC_RX_CTRLr_STRICT_PREAMBLEf_SET(rx_ctrl, 1);
    } else {
        CLMAC_RX_CTRLr_STRICT_PREAMBLEf_SET(rx_ctrl, 0);
    }
    ioerr += WRITE_CLMAC_RX_CTRLr(unit, port, rx_ctrl);

    /* Configure Tx (Inter-Packet-Gap, recompute CRC mode, IEEE header) */
    ioerr += READ_CLMAC_TX_CTRLr(unit, port, &tx_ctrl);
    CLMAC_TX_CTRLr_TX_PREAMBLE_LENGTHf_SET(tx_ctrl, 0x8);
    CLMAC_TX_CTRLr_PAD_THRESHOLDf_SET(tx_ctrl, 0x40);
    CLMAC_TX_CTRLr_AVERAGE_IPGf_SET(tx_ctrl, 0xc);
    CLMAC_TX_CTRLr_CRC_MODEf_SET(tx_ctrl, 0x2);
    ioerr += WRITE_CLMAC_TX_CTRLr(unit, port, tx_ctrl);

    /* Set max Rx frame size */
    CLMAC_RX_MAX_SIZEr_CLR(rx_max_size);
    CLMAC_RX_MAX_SIZEr_RX_MAX_SIZEf_SET(rx_max_size, JUMBO_MAXSZ);
    ioerr += WRITE_CLMAC_RX_MAX_SIZEr(unit, port, rx_max_size);

    CLMAC_MODEr_CLR(mac_mode);
    CLMAC_MODEr_HDR_MODEf_SET(mac_mode, 0);
    CLMAC_MODEr_SPEED_MODEf_SET(mac_mode, 4);
    if (speed == 1000) {
        CLMAC_MODEr_SPEED_MODEf_SET(mac_mode, 2);
    } else if (speed == 2500) {
        CLMAC_MODEr_SPEED_MODEf_SET(mac_mode, 3);
    }
    ioerr += WRITE_CLMAC_MODEr(unit, port, mac_mode);

    ioerr += READ_CLMAC_RX_LSS_CTRLr(unit, port, &rx_lss_ctrl);
    CLMAC_RX_LSS_CTRLr_DROP_TX_DATA_ON_LOCAL_FAULTf_SET(rx_lss_ctrl, 1);
    CLMAC_RX_LSS_CTRLr_DROP_TX_DATA_ON_REMOTE_FAULTf_SET(rx_lss_ctrl, 1);
    ioerr += WRITE_CLMAC_RX_LSS_CTRLr(unit, port, rx_lss_ctrl);

    /* Disable loopback and bring XLMAC out of reset */
    CLMAC_CTRLr_LOCAL_LPBKf_SET(mac_ctrl, 0);
    CLMAC_CTRLr_RX_ENf_SET(mac_ctrl, 1);
    CLMAC_CTRLr_TX_ENf_SET(mac_ctrl, 1);
    ioerr += WRITE_CLMAC_CTRLr(unit, port, mac_ctrl);

    return ioerr;
}

int
bcm56560_b0_clg2port_init(int unit, int port)
{
    int ioerr = 0;
    int speed;
    uint32_t val;
    CLG2PORT_SOFT_RESETr_t cp_soft_reset;
    CLG2PORT_ENABLE_REGr_t cp_en;
    CLG2MAC_TX_CTRLr_t tx_ctrl;
    CLG2MAC_RX_CTRLr_t rx_ctrl;
    CLG2MAC_RX_MAX_SIZEr_t rx_max_size;
    CLG2MAC_RX_LSS_CTRLr_t rx_lss_ctrl;
    CLG2MAC_CTRLr_t mac_ctrl;
    CLG2MAC_MODEr_t mac_mode;
    EGR_PORT_CREDIT_RESETm_t egr_port_credit_reset;

    /* Common port initialization */
    ioerr += _port_init(unit, port);

    speed = SPEED_MAX(unit, port);

    /* Soft reset */
    ioerr += READ_CLG2PORT_SOFT_RESETr(unit, &cp_soft_reset, port);
    val = CLG2PORT_SOFT_RESETr_GET(cp_soft_reset);
    val |= 1 << ((port - 1) & 3);
    CLG2PORT_SOFT_RESETr_SET(cp_soft_reset, val);
    ioerr += WRITE_CLG2PORT_SOFT_RESETr(unit, cp_soft_reset, port);
    val &= ~(1 << ((port - 1) & 3));
    CLG2PORT_SOFT_RESETr_SET(cp_soft_reset, val);
    ioerr += WRITE_CLG2PORT_SOFT_RESETr(unit, cp_soft_reset, port);

    /* Port enable */
    ioerr += READ_CLG2PORT_ENABLE_REGr(unit, &cp_en, port);
    val = CLG2PORT_ENABLE_REGr_GET(cp_en);
    val |= 1 << ((port - 1) & 3);
    CLG2PORT_ENABLE_REGr_SET(cp_en, val);
    ioerr += WRITE_CLG2PORT_ENABLE_REGr(unit, cp_en, port);

    /* Init MAC */
    /* Ensure that MAC (Rx) and loopback mode is disabled */
    CLG2MAC_CTRLr_CLR(mac_ctrl);
    CLG2MAC_CTRLr_SOFT_RESETf_SET(mac_ctrl, 1);
    ioerr += WRITE_CLG2MAC_CTRLr(unit, port, mac_ctrl);

    /* Reset EP credit before de-assert SOFT_RESET */
    ioerr += READ_EGR_PORT_CREDIT_RESETm(unit, port, &egr_port_credit_reset);
    EGR_PORT_CREDIT_RESETm_VALUEf_SET(egr_port_credit_reset, 1);
    ioerr += WRITE_EGR_PORT_CREDIT_RESETm(unit, port, egr_port_credit_reset);
    BMD_SYS_USLEEP(1000);
    ioerr += READ_EGR_PORT_CREDIT_RESETm(unit, port, &egr_port_credit_reset);
    EGR_PORT_CREDIT_RESETm_VALUEf_SET(egr_port_credit_reset, 0);
    ioerr += WRITE_EGR_PORT_CREDIT_RESETm(unit, port, egr_port_credit_reset);

    CLG2MAC_CTRLr_SOFT_RESETf_SET(mac_ctrl, 0);
    CLG2MAC_CTRLr_RX_ENf_SET(mac_ctrl, 0);
    CLG2MAC_CTRLr_TX_ENf_SET(mac_ctrl, 0);
    val = (BMD_PORT_PROPERTIES(unit, port) == BMD_PORT_HG) ? 1 : 0;
    CLG2MAC_CTRLr_XGMII_IPG_CHECK_DISABLEf_SET(mac_ctrl, val);
    ioerr += WRITE_CLG2MAC_CTRLr(unit, port, mac_ctrl);

    /* Configure Rx (strip CRC, strict preamble, IEEE header) */
    ioerr += READ_CLG2MAC_RX_CTRLr(unit, port, &rx_ctrl);
    CLG2MAC_RX_CTRLr_STRIP_CRCf_SET(rx_ctrl, 0);
    if (speed >= 10000 && BMD_PORT_PROPERTIES(unit, port) == BMD_PORT_XE) {
        CLG2MAC_RX_CTRLr_STRICT_PREAMBLEf_SET(rx_ctrl, 1);
    } else {
        CLG2MAC_RX_CTRLr_STRICT_PREAMBLEf_SET(rx_ctrl, 0);
    }
    ioerr += WRITE_CLG2MAC_RX_CTRLr(unit, port, rx_ctrl);

    /* Configure Tx (Inter-Packet-Gap, recompute CRC mode, IEEE header) */
    ioerr += READ_CLG2MAC_TX_CTRLr(unit, port, &tx_ctrl);
    CLG2MAC_TX_CTRLr_TX_PREAMBLE_LENGTHf_SET(tx_ctrl, 0x8);
    CLG2MAC_TX_CTRLr_PAD_THRESHOLDf_SET(tx_ctrl, 0x40);
    CLG2MAC_TX_CTRLr_AVERAGE_IPGf_SET(tx_ctrl, 0xc);
    CLG2MAC_TX_CTRLr_CRC_MODEf_SET(tx_ctrl, 0x2);
    ioerr += WRITE_CLG2MAC_TX_CTRLr(unit, port, tx_ctrl);

    /* Set max Rx frame size */
    CLG2MAC_RX_MAX_SIZEr_CLR(rx_max_size);
    CLG2MAC_RX_MAX_SIZEr_RX_MAX_SIZEf_SET(rx_max_size, JUMBO_MAXSZ);
    ioerr += WRITE_CLG2MAC_RX_MAX_SIZEr(unit, port, rx_max_size);

    CLG2MAC_MODEr_CLR(mac_mode);
    CLG2MAC_MODEr_HDR_MODEf_SET(mac_mode, 0);
    CLG2MAC_MODEr_SPEED_MODEf_SET(mac_mode, 4);
    if (speed == 1000) {
        CLG2MAC_MODEr_SPEED_MODEf_SET(mac_mode, 2);
    } else if (speed == 2500) {
        CLG2MAC_MODEr_SPEED_MODEf_SET(mac_mode, 3);
    }
    ioerr += WRITE_CLG2MAC_MODEr(unit, port, mac_mode);

    ioerr += READ_CLG2MAC_RX_LSS_CTRLr(unit, port, &rx_lss_ctrl);
    CLG2MAC_RX_LSS_CTRLr_DROP_TX_DATA_ON_LOCAL_FAULTf_SET(rx_lss_ctrl, 1);
    CLG2MAC_RX_LSS_CTRLr_DROP_TX_DATA_ON_REMOTE_FAULTf_SET(rx_lss_ctrl, 1);
    ioerr += WRITE_CLG2MAC_RX_LSS_CTRLr(unit, port, rx_lss_ctrl);

    /* Disable loopback and bring XLMAC out of reset */
    CLG2MAC_CTRLr_LOCAL_LPBKf_SET(mac_ctrl, 0);
    CLG2MAC_CTRLr_RX_ENf_SET(mac_ctrl, 1);
    CLG2MAC_CTRLr_TX_ENf_SET(mac_ctrl, 1);
    ioerr += WRITE_CLG2MAC_CTRLr(unit, port, mac_ctrl);

    return ioerr;
}

int
bcm56560_b0_xlport_init(int unit, int port)
{
    int ioerr = 0;
    int speed;
    uint32_t val;
    XLPORT_SOFT_RESETr_t xlp_soft_reset;
    XLPORT_ENABLE_REGr_t xlp_en;
    XLMAC_TX_CTRLr_t tx_ctrl;
    XLMAC_RX_CTRLr_t rx_ctrl;
    XLMAC_RX_MAX_SIZEr_t rx_max_size;
    XLMAC_RX_LSS_CTRLr_t rx_lss_ctrl;
    XLMAC_CTRLr_t mac_ctrl;
    XLMAC_MODEr_t mac_mode;
    EGR_PORT_CREDIT_RESETm_t egr_port_credit_reset;
    cdk_pbmp_t xlpbmp;

    bcm56560_b0_xlport_pbmp_get(unit, &xlpbmp);

    /* Common port initialization */
    ioerr += _port_init(unit, port);
    speed = SPEED_MAX(unit, port);

    /* Soft reset */
    ioerr += READ_XLPORT_SOFT_RESETr(unit, &xlp_soft_reset, port);
    val = XLPORT_SOFT_RESETr_GET(xlp_soft_reset);
    val |= 1 << ((port - 1) & 3);
    XLPORT_SOFT_RESETr_SET(xlp_soft_reset, val);
    ioerr += WRITE_XLPORT_SOFT_RESETr(unit, xlp_soft_reset, port);
    val &= ~(1 << ((port - 1) & 3));
    XLPORT_SOFT_RESETr_SET(xlp_soft_reset, val);
    ioerr += WRITE_XLPORT_SOFT_RESETr(unit, xlp_soft_reset, port);

    /* Port enable */
    ioerr += READ_XLPORT_ENABLE_REGr(unit, &xlp_en, port);
    val = XLPORT_ENABLE_REGr_GET(xlp_en);
    val |= 1 << ((port - 1) & 3);
    XLPORT_ENABLE_REGr_SET(xlp_en, val);
    ioerr += WRITE_XLPORT_ENABLE_REGr(unit, xlp_en, port);

    ioerr += READ_XLPORT_ENABLE_REGr(unit, &xlp_en, port);
    val = XLPORT_ENABLE_REGr_GET(xlp_en);

    /* Init MAC */
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
    val = (BMD_PORT_PROPERTIES(unit, port) == BMD_PORT_HG) ? 1 : 0;
    XLMAC_CTRLr_XGMII_IPG_CHECK_DISABLEf_SET(mac_ctrl, val);
    ioerr += WRITE_XLMAC_CTRLr(unit, port, mac_ctrl);

    /* Configure Rx (strip CRC, strict preamble, IEEE header) */
    ioerr += READ_XLMAC_RX_CTRLr(unit, port, &rx_ctrl);
    XLMAC_RX_CTRLr_STRIP_CRCf_SET(rx_ctrl, 0);
    if (speed >= 10000 && BMD_PORT_PROPERTIES(unit, port) == BMD_PORT_XE) {
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

    XLMAC_MODEr_CLR(mac_mode);
    XLMAC_MODEr_HDR_MODEf_SET(mac_mode, 0);
    XLMAC_MODEr_SPEED_MODEf_SET(mac_mode, 4);
    if (speed == 1000) {
        XLMAC_MODEr_SPEED_MODEf_SET(mac_mode, 2);
    } else if (speed == 2500) {
        XLMAC_MODEr_SPEED_MODEf_SET(mac_mode, 3);
    }
    ioerr += WRITE_XLMAC_MODEr(unit, port, mac_mode);

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

void *
bcm56560_b0_tdm_malloc(int unit, int size)
{
#if BMD_CONFIG_INCLUDE_DMA == 1
    /* Not real DMA memory is needed actually, but the DMA memory allocate
     * function is the only API in MDK for dynamic memory allocation usage.
     */
    dma_addr_t dma_addr;

    return bmd_dma_alloc_coherent(unit, size, &dma_addr);
#else
    CDK_WARN(("bcm56560_b0_tdm_malloc[%d]: BMD_CONFIG_INCLUDE_DMA "
              "must be defined in bcm56560 to support "
              "bcm56560_b0_tdm_malloc.\n", unit));
    return NULL;
#endif
}

void
bcm56560_b0_tdm_free(int unit, void *addr)
{
#if BMD_CONFIG_INCLUDE_DMA == 1
    bmd_dma_free_coherent(unit, 0, addr, 0);
#else
    return;
#endif
}

static int
_ports_edb_config(int unit, int asf, cdk_pbmp_t pbm)
{
#define PGW_NUM_XLP 7
#define PGW_NUM_CLP 2
    int ioerr = 0;
    cdk_pbmp_t oversub_pbm;
    uint32_t osct_pbm;
    int port, phy_port, mmu_port;
    int pgw;
    int pm_id, linerate;
    EGR_EDB_XMIT_CTRLm_t egr_edb_xmit_ctrl;
    uint32_t edb_values[3] = {0};
    CELL_ASM_CUT_THRU_THRESHOLD0r_t cell_asm_cut_thru_th0;
    ENQ_ASF_HS_OVERSUB_ENr_t enq_asf_hs_en;
    ENQ_ASF_LRCT_OVERSUB_ENr_t enq_asf_lrct_en;
    uint32_t enq_asf_lrct_ovr_en[2];
    int asm_ct_thresh;

    /* Remove extraneous ports from the input list */
    osct_pbm = 0;
    bcm56560_b0_ovrs_pbmp_get(unit, &oversub_pbm);

    CDK_PBMP_ITER(pbm, port) {

        phy_port = port;
        mmu_port = P2M(unit, phy_port);
        pgw = (phy_port - 1) / PORTS_PER_PGW;
        linerate = (!CDK_PBMP_MEMBER(oversub_pbm, phy_port));

        pm_id = ((phy_port - 1) / PORTS_PER_XLP) % XLPS_PER_PGW;
        if (pm_id < PGW_NUM_XLP) { /* Eagle Cores */
            if (SPEED_MAX(unit, phy_port) <= 21000) {  /* 0-21G */
                edb_values[0] = (linerate) ? (asf ? 7 : 0) : (asf ? 0 : 5);
                edb_values[1] = (linerate) ? (asf ? 0 : 1) : 0;
                edb_values[2] = 0; /* linerate/oversub and cut-thru or no cut-thru */

                asm_ct_thresh = 6;
            } else if (SPEED_MAX(unit, phy_port) <= 42000) {  /* 21G-42G */
                edb_values[0] = (linerate) ? (asf ? 7 : 0) : (asf ? 11 : 9);
                edb_values[1] = (linerate) ? (asf ? 0 : 1) : 0;
                edb_values[2] = 0; /* linerate/oversub and cut-thru or no cut-thru */

                asm_ct_thresh = 6;
            } else { /* 42G or more (100G)  */
                edb_values[0] = (linerate) ? (asf ? 7 : 0) : (asf ? 0 : 4);
                edb_values[1] = (linerate) ? (asf ? 0 : 1) : 0;
                edb_values[2] = 0; /* linerate/oversub and cut-thru or no cut-thru */

                asm_ct_thresh = 6;
            }
        } else { /*  Flacon Cores  */
            if (SPEED_MAX(unit, phy_port) <= 11000) {  /* 0-11G */
                edb_values[0] = (linerate) ? (asf ? 3 : 0) : (asf ? 0 : 4);
                edb_values[1] = (linerate) ? (asf ? 0 : 1) : 0;
                edb_values[2] = 0; /* linerate/oversub and cut-thru or no cut-thru */

                asm_ct_thresh = 6;
            } else if (SPEED_MAX(unit, phy_port) <= 27000) {  /* 11G-27G */
                edb_values[0] = (linerate) ? (asf ? 3 : 0) : (asf ? 0 : 4);
                edb_values[1] = (linerate) ? (asf ? 0 : 1) : 0;
                edb_values[2] = 0; /* linerate/oversub and cut-thru or no cut-thru */

                asm_ct_thresh = 6;
            } else if (SPEED_MAX(unit, phy_port) <= 42000) {  /* 27G-42G */
                edb_values[0] = (linerate) ? (asf ? 3 : 0) : (asf ? 5 : 4);
                edb_values[1] = (linerate) ? (asf ? 0 : 1) : 0;
                edb_values[2] = 0; /* linerate/oversub and cut-thru or no cut-thru */

                asm_ct_thresh = 6;
            } else { /* 42G or more (50G, 53G, 100G, 106G)  */
                edb_values[0] = (linerate) ? (asf ? 7 : 0) : (asf ? 0 : 4);
                edb_values[1] = (linerate) ? (asf ? 0 : 1) : 0;
                edb_values[2] = 0; /* linerate/oversub and cut-thru or no cut-thru */

                asm_ct_thresh = 6;
            }
        }

        ioerr += READ_EGR_EDB_XMIT_CTRLm(unit, phy_port, &egr_edb_xmit_ctrl);
        EGR_EDB_XMIT_CTRLm_START_CNTf_SET(egr_edb_xmit_ctrl, edb_values[0]);
        EGR_EDB_XMIT_CTRLm_WAIT_FOR_MOPf_SET(egr_edb_xmit_ctrl, edb_values[1]);
        EGR_EDB_XMIT_CTRLm_WAIT_FOR_2ND_MOPf_SET(egr_edb_xmit_ctrl, edb_values[2]);
        ioerr += WRITE_EGR_EDB_XMIT_CTRLm(unit, phy_port, egr_edb_xmit_ctrl);

        ioerr += READ_CELL_ASM_CUT_THRU_THRESHOLD0r(unit,  pgw, &cell_asm_cut_thru_th0);
        CELL_ASM_CUT_THRU_THRESHOLD0r_XLP0f_SET(cell_asm_cut_thru_th0, asm_ct_thresh);
        CELL_ASM_CUT_THRU_THRESHOLD0r_XLP1f_SET(cell_asm_cut_thru_th0, asm_ct_thresh);
        CELL_ASM_CUT_THRU_THRESHOLD0r_XLP2f_SET(cell_asm_cut_thru_th0, asm_ct_thresh);
        CELL_ASM_CUT_THRU_THRESHOLD0r_XLP3f_SET(cell_asm_cut_thru_th0, asm_ct_thresh);
        CELL_ASM_CUT_THRU_THRESHOLD0r_XLP4f_SET(cell_asm_cut_thru_th0, asm_ct_thresh);
        CELL_ASM_CUT_THRU_THRESHOLD0r_XLP5f_SET(cell_asm_cut_thru_th0, asm_ct_thresh);
        CELL_ASM_CUT_THRU_THRESHOLD0r_XLP6f_SET(cell_asm_cut_thru_th0, asm_ct_thresh);
        CELL_ASM_CUT_THRU_THRESHOLD0r_CLP0f_SET(cell_asm_cut_thru_th0, asm_ct_thresh);
        CELL_ASM_CUT_THRU_THRESHOLD0r_CLP1f_SET(cell_asm_cut_thru_th0, asm_ct_thresh);

        if (CDK_PBMP_MEMBER(oversub_pbm, phy_port)) {
            if ((SPEED_MAX(unit, phy_port) == 40000) ||
                (SPEED_MAX(unit, phy_port) == 42000)) {
                osct_pbm |= 1 << mmu_port;    /* mmu ports: [0, 17] */
            }
        } else {
            enq_asf_lrct_ovr_en[0] = (0x0 << (mmu_port - 18)) |
                                (0x1>> (32 - (mmu_port - 18)));
            enq_asf_lrct_ovr_en[1] = 0x1 << (mmu_port - 18);
            /* mmu ports: [18, 71] */
        }
    }

    ENQ_ASF_HS_OVERSUB_ENr_SET(enq_asf_hs_en, osct_pbm);
    ioerr += WRITE_ENQ_ASF_HS_OVERSUB_ENr(unit, enq_asf_hs_en);
    ENQ_ASF_LRCT_OVERSUB_ENr_SET(enq_asf_lrct_en, 0, enq_asf_lrct_ovr_en[0]);
    ENQ_ASF_LRCT_OVERSUB_ENr_SET(enq_asf_lrct_en, 1, enq_asf_lrct_ovr_en[1]);
    ioerr += WRITE_ENQ_ASF_LRCT_OVERSUB_ENr(unit, enq_asf_lrct_en);

    return CDK_E_NONE;
}

static int
_edb_init(int unit)
{
    cdk_pbmp_t pbm;

    bcm56560_b0_all_front_pbmp_get(unit, &pbm);
    _ports_edb_config(unit, 0, pbm);

    return CDK_E_NONE;
}

static int
_port_asf_speed_set(int unit, int port, int speed)
{
    int ioerr = 0;
    int asf_speed_mode;
    ASF_PORT_CFGr_t asf_port_cfg;

    ioerr += READ_ASF_PORT_CFGr(unit, port, &asf_port_cfg);

    switch (speed) {
        case 1000:
            asf_speed_mode = APACHE_PORT_CT_SPEED_1000M_FULL;
            break;
        case 2500:
            asf_speed_mode = APACHE_PORT_CT_SPEED_2500M_FULL;
            break;
        case 5000:
            asf_speed_mode = APACHE_PORT_CT_SPEED_5000M_FULL;
            break;
        case 10000:
            asf_speed_mode = APACHE_PORT_CT_SPEED_10000M_FULL;
            break;
        case 11000:
            asf_speed_mode = APACHE_PORT_CT_SPEED_11000M_FULL;
            break;
        case 20000:
            asf_speed_mode = APACHE_PORT_CT_SPEED_20000M_FULL;
            break;
        case 21000:
            asf_speed_mode = APACHE_PORT_CT_SPEED_21000M_FULL;
            break;
        case 25000:
            asf_speed_mode = APACHE_PORT_CT_SPEED_25000M_FULL;
            break;
        case 27000:
            asf_speed_mode = APACHE_PORT_CT_SPEED_27000M_FULL;
            break;
        case 40000:
            asf_speed_mode = APACHE_PORT_CT_SPEED_40000M_FULL;
            break;
        case 42000:
            asf_speed_mode = APACHE_PORT_CT_SPEED_42000M_FULL;
            break;
        case 50000:
            asf_speed_mode = APACHE_PORT_CT_SPEED_50000M_FULL;
            break;
        case 53000:
            asf_speed_mode = APACHE_PORT_CT_SPEED_53000M_FULL;
            break;
        case 100000:
            asf_speed_mode = APACHE_PORT_CT_SPEED_100000M_FULL;
            break;
        case 106000:
            asf_speed_mode = APACHE_PORT_CT_SPEED_106000M_FULL;
            break;
        case 0:
            return CDK_E_NONE;
        default:
            return CDK_E_PARAM;
    }

    ASF_PORT_CFGr_ASF_PORT_SPEEDf_SET(asf_port_cfg, asf_speed_mode);
    ioerr += WRITE_ASF_PORT_CFGr(unit, port, asf_port_cfg);

    return CDK_E_NONE;
}

static int
_port_asf_set(int unit, int port, int speed)
{
    int ioerr = 0;
    int mmu_port, phy_port;
    int asf_credit_thresh_lo, asf_credit_thresh_hi;
    int ep_credit_size = 0;

    ES_PIPE0_ASF_CREDIT_THRESH_LOr_t ep_pipe0_credit_lo;
    ES_PIPE0_ASF_CREDIT_THRESH_HIr_t ep_pipe0_credit_hi;

    phy_port = port;
    mmu_port = P2M(unit, phy_port);

    if (speed <= 11000) {
        asf_credit_thresh_lo = 4;
        asf_credit_thresh_hi = 9;
    } else if (speed <= 21000) {
        asf_credit_thresh_lo = 8;
        asf_credit_thresh_hi = 18;
    } else if (speed <= 42000) {
        asf_credit_thresh_lo = 18;
        asf_credit_thresh_hi = 36;
    } else {
        asf_credit_thresh_lo = 30;
        asf_credit_thresh_hi = 72;
    }
    ep_credit_size = ep_credit_size - 1;

    ES_PIPE0_ASF_CREDIT_THRESH_LOr_CLR(ep_pipe0_credit_lo);
    ES_PIPE0_ASF_CREDIT_THRESH_LOr_THRESHf_SET(ep_pipe0_credit_lo, asf_credit_thresh_lo);
    ioerr += WRITE_ES_PIPE0_ASF_CREDIT_THRESH_LOr(unit, mmu_port, ep_pipe0_credit_lo);

    ES_PIPE0_ASF_CREDIT_THRESH_HIr_CLR(ep_pipe0_credit_hi);
    ES_PIPE0_ASF_CREDIT_THRESH_HIr_THRESHf_SET(ep_pipe0_credit_hi, asf_credit_thresh_hi);
    ioerr += WRITE_ES_PIPE0_ASF_CREDIT_THRESH_HIr(unit, mmu_port, ep_pipe0_credit_hi);

    _port_asf_speed_set(unit, mmu_port, speed);

    return CDK_E_NONE;
}

static int
_portctrl_port_init(int unit)
{
    int ioerr = 0;
    int port;
    int pgw;
    cdk_pbmp_t pbmp;
    PGW_ETM_MODEr_t pgw_etm_mode;

    bcm56560_b0_cxxport_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {

        /*  If this is a 100G port, reset egress port buffer */
        if (SPEED_MAX(unit, port) >= 100000) {
            bcm56560_b0_egr_buf_reset(unit, port, 1);

            _port_speed_update(unit, port, SPEED_MAX(unit, port));

            bcm56560_b0_egr_buf_reset(unit, port, 0);

            pgw = port / PORTS_PER_PGW;
            ioerr += READ_PGW_ETM_MODEr(unit, pgw, &pgw_etm_mode);
            PGW_ETM_MODEr_ETM_PORT_EN_CONFIGf_SET(pgw_etm_mode, 1);
            ioerr += WRITE_PGW_ETM_MODEr(unit, pgw, pgw_etm_mode);
        }

    }

    /* Cut through settings for all ports */
    bcm56560_b0_all_front_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        _port_asf_set(unit, port, SPEED_MAX(unit, port));
    }

    return CDK_E_NONE;
}

void
bcm56560_b0_idb_buf_reset(int unit, int port, int reset)
{

    int obm, subport, pgw;
    IDB_OBM0_USAGEr_t idb_obm0_us;
    IDB_OBM1_USAGEr_t idb_obm1_us;
    IDB_OBM2_USAGEr_t idb_obm2_us;
    IDB_OBM3_USAGEr_t idb_obm3_us;
    IDB_OBM4_USAGEr_t idb_obm4_us;
    IDB_OBM5_USAGEr_t idb_obm5_us;
    IDB_OBM6_USAGEr_t idb_obm6_us;
    IDB_OBM7_USAGEr_t idb_obm7_us;
    IDB_OBM8_USAGEr_t idb_obm8_us;
    PGW_BOD_STATUS1r_t bod_sts;
    IDB_OBM0_CONTROLr_t idb_obm0_ctl;
    IDB_OBM1_CONTROLr_t idb_obm1_ctl;
    IDB_OBM2_CONTROLr_t idb_obm2_ctl;
    IDB_OBM3_CONTROLr_t idb_obm3_ctl;
    IDB_OBM4_CONTROLr_t idb_obm4_ctl;
    IDB_OBM5_CONTROLr_t idb_obm5_ctl;
    IDB_OBM6_CONTROLr_t idb_obm6_ctl;
    IDB_OBM7_CONTROLr_t idb_obm7_ctl;
    IDB_OBM8_CONTROLr_t idb_obm8_ctl;
    IDB_OBM_CA_CONTROLr_t idb_obm_ca_ctrl;

    pgw = bcm56560_b0_pgw_clport_block_index_get(unit, port, &obm, &subport);
    if (obm == 0) {
        READ_IDB_OBM0_USAGEr(unit, pgw, subport, &idb_obm0_us);
        if (IDB_OBM0_USAGEr_TOTAL_COUNTf_GET(idb_obm0_us) != 0) {
            CDK_PRINTF("\n port(%d) count error ", port);
        }
        READ_PGW_BOD_STATUS1r(unit, port, &bod_sts);
        if (PGW_BOD_STATUS1r_PGW_BOD_FIFO_EMPTYf_GET(bod_sts) != 1) {
            CDK_PRINTF("\n port(%d) fifo error ", port);
        }
        READ_IDB_OBM0_CONTROLr(unit, pgw, &idb_obm0_ctl);
        if (subport == 0) {
            IDB_OBM0_CONTROLr_PORT0_RESETf_SET(idb_obm0_ctl, reset);
        } else if (subport == 1) {
            IDB_OBM0_CONTROLr_PORT1_RESETf_SET(idb_obm0_ctl, reset);
        } else if (subport == 2) {
            IDB_OBM0_CONTROLr_PORT2_RESETf_SET(idb_obm0_ctl, reset);
        } else {
            IDB_OBM0_CONTROLr_PORT3_RESETf_SET(idb_obm0_ctl, reset);
        }
        WRITE_IDB_OBM0_CONTROLr(unit, pgw, idb_obm0_ctl);

    } else if (obm == 1) {
        READ_IDB_OBM1_USAGEr(unit, pgw, subport, &idb_obm1_us);
        if (IDB_OBM1_USAGEr_TOTAL_COUNTf_GET(idb_obm1_us) != 0) {
            CDK_PRINTF("\n port(%d) count error ", port);
        }
        READ_PGW_BOD_STATUS1r(unit, port, &bod_sts);
        if (PGW_BOD_STATUS1r_PGW_BOD_FIFO_EMPTYf_GET(bod_sts) != 1) {
            CDK_PRINTF("\n port(%d) fifo error ", port);
        }

        READ_IDB_OBM1_CONTROLr(unit, pgw, &idb_obm1_ctl);
        if (subport == 0) {
            IDB_OBM1_CONTROLr_PORT0_RESETf_SET(idb_obm1_ctl, reset);
        } else if (subport == 1) {
            IDB_OBM1_CONTROLr_PORT1_RESETf_SET(idb_obm1_ctl, reset);
        } else if (subport == 2) {
            IDB_OBM1_CONTROLr_PORT2_RESETf_SET(idb_obm1_ctl, reset);
        } else {
            IDB_OBM1_CONTROLr_PORT3_RESETf_SET(idb_obm1_ctl, reset);
        }
        WRITE_IDB_OBM1_CONTROLr(unit, pgw, idb_obm1_ctl);
    } else if (obm == 2) {
        READ_IDB_OBM2_USAGEr(unit, pgw, subport, &idb_obm2_us);
        if (IDB_OBM2_USAGEr_TOTAL_COUNTf_GET(idb_obm2_us) != 0) {
            CDK_PRINTF("\n port(%d) count error ", port);
        }
        READ_PGW_BOD_STATUS1r(unit, port, &bod_sts);
        if (PGW_BOD_STATUS1r_PGW_BOD_FIFO_EMPTYf_GET(bod_sts) != 1) {
            CDK_PRINTF("\n port(%d) fifo error ", port);
        }

        READ_IDB_OBM2_CONTROLr(unit, pgw, &idb_obm2_ctl);
        if (subport == 0) {
            IDB_OBM2_CONTROLr_PORT0_RESETf_SET(idb_obm2_ctl, reset);
        } else if (subport == 1) {
            IDB_OBM2_CONTROLr_PORT1_RESETf_SET(idb_obm2_ctl, reset);
        } else if (subport == 2) {
            IDB_OBM2_CONTROLr_PORT2_RESETf_SET(idb_obm2_ctl, reset);
        } else {
            IDB_OBM2_CONTROLr_PORT3_RESETf_SET(idb_obm2_ctl, reset);
        }
        WRITE_IDB_OBM2_CONTROLr(unit, pgw, idb_obm2_ctl);
    } else if (obm == 3) {
        READ_IDB_OBM3_USAGEr(unit, pgw, subport, &idb_obm3_us);
        if (IDB_OBM3_USAGEr_TOTAL_COUNTf_GET(idb_obm3_us) != 0) {
            CDK_PRINTF("\n port(%d) count error ", port);
        }
        READ_PGW_BOD_STATUS1r(unit, port, &bod_sts);
        if (PGW_BOD_STATUS1r_PGW_BOD_FIFO_EMPTYf_GET(bod_sts) != 1) {
            CDK_PRINTF("\n port(%d) fifo error ", port);
        }

        READ_IDB_OBM3_CONTROLr(unit, pgw, &idb_obm3_ctl);
        if (subport == 0) {
            IDB_OBM3_CONTROLr_PORT0_RESETf_SET(idb_obm3_ctl, reset);
        } else if (subport == 1) {
            IDB_OBM3_CONTROLr_PORT1_RESETf_SET(idb_obm3_ctl, reset);
        } else if (subport == 2) {
            IDB_OBM3_CONTROLr_PORT2_RESETf_SET(idb_obm3_ctl, reset);
        } else {
            IDB_OBM3_CONTROLr_PORT3_RESETf_SET(idb_obm3_ctl, reset);
        }
        WRITE_IDB_OBM3_CONTROLr(unit, pgw, idb_obm3_ctl);
    } else if (obm == 4) {
        READ_IDB_OBM4_USAGEr(unit, pgw, subport, &idb_obm4_us);
        if (IDB_OBM4_USAGEr_TOTAL_COUNTf_GET(idb_obm4_us) != 0) {
            CDK_PRINTF("\n port(%d) count error ", port);
        }
        READ_PGW_BOD_STATUS1r(unit, port, &bod_sts);
        if (PGW_BOD_STATUS1r_PGW_BOD_FIFO_EMPTYf_GET(bod_sts) != 1) {
            CDK_PRINTF("\n port(%d) fifo error ", port);
        }

        READ_IDB_OBM4_CONTROLr(unit, pgw, &idb_obm4_ctl);
        if (subport == 0) {
            IDB_OBM4_CONTROLr_PORT0_RESETf_SET(idb_obm4_ctl, reset);
        } else if (subport == 1) {
            IDB_OBM4_CONTROLr_PORT1_RESETf_SET(idb_obm4_ctl, reset);
        } else if (subport == 2) {
            IDB_OBM4_CONTROLr_PORT2_RESETf_SET(idb_obm4_ctl, reset);
        } else {
            IDB_OBM4_CONTROLr_PORT3_RESETf_SET(idb_obm4_ctl, reset);
        }
        WRITE_IDB_OBM4_CONTROLr(unit, pgw, idb_obm4_ctl);
    } else if (obm == 5) {
        READ_IDB_OBM5_USAGEr(unit, pgw, subport, &idb_obm5_us);
        if (IDB_OBM5_USAGEr_TOTAL_COUNTf_GET(idb_obm5_us) != 0) {
            CDK_PRINTF("\n port(%d) count error ", port);
        }
        READ_PGW_BOD_STATUS1r(unit, port, &bod_sts);
        if (PGW_BOD_STATUS1r_PGW_BOD_FIFO_EMPTYf_GET(bod_sts) != 1) {
            CDK_PRINTF("\n port(%d) fifo error ", port);
        }

        READ_IDB_OBM5_CONTROLr(unit, pgw, &idb_obm5_ctl);
        if (subport == 0) {
            IDB_OBM5_CONTROLr_PORT0_RESETf_SET(idb_obm5_ctl, reset);
        } else if (subport == 1) {
            IDB_OBM5_CONTROLr_PORT1_RESETf_SET(idb_obm5_ctl, reset);
        } else if (subport == 2) {
            IDB_OBM5_CONTROLr_PORT2_RESETf_SET(idb_obm5_ctl, reset);
        } else {
            IDB_OBM5_CONTROLr_PORT3_RESETf_SET(idb_obm5_ctl, reset);
        }
        WRITE_IDB_OBM5_CONTROLr(unit, pgw, idb_obm5_ctl);
    } else  if (obm == 6) {
        READ_IDB_OBM6_USAGEr(unit, pgw, subport, &idb_obm6_us);
        if (IDB_OBM6_USAGEr_TOTAL_COUNTf_GET(idb_obm6_us) != 0) {
            CDK_PRINTF("\n port(%d) count error ", port);
        }
        READ_PGW_BOD_STATUS1r(unit, port, &bod_sts);
        if (PGW_BOD_STATUS1r_PGW_BOD_FIFO_EMPTYf_GET(bod_sts) != 1) {
            CDK_PRINTF("\n port(%d) fifo error ", port);
        }

        READ_IDB_OBM6_CONTROLr(unit, pgw, &idb_obm6_ctl);
        if (subport == 0) {
            IDB_OBM6_CONTROLr_PORT0_RESETf_SET(idb_obm6_ctl, reset);
        } else if (subport == 1) {
            IDB_OBM6_CONTROLr_PORT1_RESETf_SET(idb_obm6_ctl, reset);
        } else if (subport == 2) {
            IDB_OBM6_CONTROLr_PORT2_RESETf_SET(idb_obm6_ctl, reset);
        } else {
            IDB_OBM6_CONTROLr_PORT3_RESETf_SET(idb_obm6_ctl, reset);
        }
        WRITE_IDB_OBM6_CONTROLr(unit, pgw, idb_obm6_ctl);
    } else if (obm == 7) {
        READ_IDB_OBM7_USAGEr(unit, pgw, subport, &idb_obm7_us);
        if (IDB_OBM7_USAGEr_TOTAL_COUNTf_GET(idb_obm7_us) != 0) {
            CDK_PRINTF("\n port(%d) count error ", port);
        }
        READ_PGW_BOD_STATUS1r(unit, port, &bod_sts);
        if (PGW_BOD_STATUS1r_PGW_BOD_FIFO_EMPTYf_GET(bod_sts) != 1) {
            CDK_PRINTF("\n port(%d) fifo error ", port);
        }

        READ_IDB_OBM7_CONTROLr(unit, pgw, &idb_obm7_ctl);
        if (subport == 0) {
            IDB_OBM7_CONTROLr_PORT0_RESETf_SET(idb_obm7_ctl, reset);
        } else if (subport == 1) {
            IDB_OBM7_CONTROLr_PORT1_RESETf_SET(idb_obm7_ctl, reset);
        } else if (subport == 2) {
            IDB_OBM7_CONTROLr_PORT2_RESETf_SET(idb_obm7_ctl, reset);
        } else {
            IDB_OBM7_CONTROLr_PORT3_RESETf_SET(idb_obm7_ctl, reset);
        }
        WRITE_IDB_OBM7_CONTROLr(unit, pgw, idb_obm7_ctl);
    } else {
        READ_IDB_OBM8_USAGEr(unit, pgw, subport, &idb_obm8_us);
        if (IDB_OBM8_USAGEr_TOTAL_COUNTf_GET(idb_obm8_us) != 0) {
            CDK_PRINTF("\n port(%d) count error ", port);
        }
        READ_PGW_BOD_STATUS1r(unit, port, &bod_sts);
        if (PGW_BOD_STATUS1r_PGW_BOD_FIFO_EMPTYf_GET(bod_sts) != 1) {
            CDK_PRINTF("\n port(%d) fifo error ", port);
        }

        READ_IDB_OBM8_CONTROLr(unit, pgw, &idb_obm8_ctl);
        if (subport == 0) {
            IDB_OBM8_CONTROLr_PORT0_RESETf_SET(idb_obm8_ctl, reset);
        } else if (subport == 1) {
            IDB_OBM8_CONTROLr_PORT1_RESETf_SET(idb_obm8_ctl, reset);
        } else if (subport == 2) {
            IDB_OBM8_CONTROLr_PORT2_RESETf_SET(idb_obm8_ctl, reset);
        } else {
            IDB_OBM8_CONTROLr_PORT3_RESETf_SET(idb_obm8_ctl, reset);
        }
        WRITE_IDB_OBM8_CONTROLr(unit, pgw, idb_obm8_ctl);
    }
    READ_IDB_OBM_CA_CONTROLr(unit, pgw, obm, &idb_obm_ca_ctrl);
    if (subport == 0) {
        IDB_OBM_CA_CONTROLr_PORT0_RESETf_SET(idb_obm_ca_ctrl, reset);
    } else if (subport == 1) {
        IDB_OBM_CA_CONTROLr_PORT1_RESETf_SET(idb_obm_ca_ctrl, reset);
    } else if (subport == 2) {
        IDB_OBM_CA_CONTROLr_PORT2_RESETf_SET(idb_obm_ca_ctrl, reset);
    } else {
        IDB_OBM_CA_CONTROLr_PORT3_RESETf_SET(idb_obm_ca_ctrl, reset);
    }
    WRITE_IDB_OBM_CA_CONTROLr(unit, pgw, obm, idb_obm_ca_ctrl);
}

static int
_p2l_value(int unit, int port, int up1_off, int skip)
{
    if (P2L(unit, port) == -1) {
        return 0;
    } else {
        return (port - up1_off - skip);
    }
}

static int
_ledup_init(int unit)
{
    int ioerr = 0;
    int ix, idx, skip_count = 0;
    int uP1_off;
    CMIC_LEDUP0_CTRLr_t ledup0_ctrl;
    CMIC_LEDUP1_CTRLr_t ledup1_ctrl;
    CMIC_LEDUP0_PORT_ORDER_REMAPr_t ledup0_order_remap;
    CMIC_LEDUP1_PORT_ORDER_REMAPr_t ledup1_order_remap;
    int skip_slot[72];
    CMIC_LEDUP0_DATA_RAMr_t ledup0_data_ram;
    CMIC_LEDUP1_DATA_RAMr_t ledup1_data_ram;
    uint32_t fval;

    /* configure the LED scan delay cycles */
    ioerr += READ_CMIC_LEDUP0_CTRLr(unit, &ledup0_ctrl);
    CMIC_LEDUP0_CTRLr_LEDUP_SCAN_START_DELAYf_SET(ledup0_ctrl, 0xd);
    CMIC_LEDUP0_CTRLr_LEDUP_SCAN_INTRA_PORT_DELAYf_SET(ledup0_ctrl, 4);
    ioerr += WRITE_CMIC_LEDUP0_CTRLr(unit, ledup0_ctrl);

    ioerr += READ_CMIC_LEDUP1_CTRLr(unit, &ledup1_ctrl);
    CMIC_LEDUP1_CTRLr_LEDUP_SCAN_START_DELAYf_SET(ledup1_ctrl, 0xb);
    CMIC_LEDUP1_CTRLr_LEDUP_SCAN_INTRA_PORT_DELAYf_SET(ledup1_ctrl, 4);
    ioerr += WRITE_CMIC_LEDUP1_CTRLr(unit, ledup1_ctrl);

    /* Initialize LED Port remap registers */
    CMIC_LEDUP0_PORT_ORDER_REMAPr_CLR(ledup0_order_remap);
    for (idx = 0; idx < 9; idx++) {
        ioerr += WRITE_CMIC_LEDUP0_PORT_ORDER_REMAPr(unit, idx, ledup0_order_remap);
    }

    CMIC_LEDUP1_PORT_ORDER_REMAPr_CLR(ledup1_order_remap);
    for (idx = 0; idx < 9; idx++) {
        ioerr += WRITE_CMIC_LEDUP1_PORT_ORDER_REMAPr(unit, idx, ledup1_order_remap);
    }

    for (ix = 1; ix <= 72; ix++) {
        if (P2L(unit, ix) == -1) {
            skip_count++;
        }
        if (ix == 37) {
            skip_count = 0;
        }
        skip_slot[ix] = skip_count;
    }

    /* Configure LED Port remap registers */
    ix = 1;
    uP1_off = 0;
    CMIC_LEDUP0_PORT_ORDER_REMAPr_CLR(ledup0_order_remap);
    for (idx = 0; idx < 9; idx++) {
        fval = _p2l_value(unit, ix, uP1_off, skip_slot[ix]);
        CMIC_LEDUP0_PORT_ORDER_REMAPr_REMAP_PORT_0f_SET(ledup0_order_remap, fval);
        ++ix;
        fval = _p2l_value(unit, ix, uP1_off, skip_slot[ix]);
        CMIC_LEDUP0_PORT_ORDER_REMAPr_REMAP_PORT_1f_SET(ledup0_order_remap, fval);
        ++ix;
        fval = _p2l_value(unit, ix, uP1_off, skip_slot[ix]);
        CMIC_LEDUP0_PORT_ORDER_REMAPr_REMAP_PORT_2f_SET(ledup0_order_remap, fval);
        ++ix;
        fval = _p2l_value(unit, ix, uP1_off, skip_slot[ix]);
        CMIC_LEDUP0_PORT_ORDER_REMAPr_REMAP_PORT_3f_SET(ledup0_order_remap, fval);
        ++ix;
        fval = _p2l_value(unit, ix, uP1_off, skip_slot[ix]);
        ioerr += WRITE_CMIC_LEDUP0_PORT_ORDER_REMAPr(unit, idx, ledup0_order_remap);
    }

    /* Configure LED Port remap registers - LED uP1*/

    uP1_off = 36;

    CMIC_LEDUP1_PORT_ORDER_REMAPr_CLR(ledup1_order_remap);
    for (idx = 0; idx < 9; idx++) {
        CMIC_LEDUP1_PORT_ORDER_REMAPr_REMAP_PORT_0f_SET(ledup1_order_remap, _p2l_value(unit, ix, uP1_off, skip_slot[ix]));
        ++ix;
        CMIC_LEDUP1_PORT_ORDER_REMAPr_REMAP_PORT_1f_SET(ledup1_order_remap, _p2l_value(unit, ix, uP1_off, skip_slot[ix]));
        ++ix;
        CMIC_LEDUP1_PORT_ORDER_REMAPr_REMAP_PORT_2f_SET(ledup1_order_remap, _p2l_value(unit, ix, uP1_off, skip_slot[ix]));
        ++ix;
        CMIC_LEDUP1_PORT_ORDER_REMAPr_REMAP_PORT_3f_SET(ledup1_order_remap, _p2l_value(unit, ix, uP1_off, skip_slot[ix]));
        ++ix;
        ioerr += WRITE_CMIC_LEDUP1_PORT_ORDER_REMAPr(unit, idx, ledup1_order_remap);
    }

    /* initialize the UP0, UP1 data ram */
    for (ix = 0; ix < 256; ix++) {
        CMIC_LEDUP0_DATA_RAMr_CLR(ledup0_data_ram);
        ioerr += WRITE_CMIC_LEDUP0_DATA_RAMr(unit, ix, ledup0_data_ram);
        CMIC_LEDUP1_DATA_RAMr_CLR(ledup1_data_ram);
        ioerr += WRITE_CMIC_LEDUP1_DATA_RAMr(unit, ix, ledup1_data_ram);
    }

    return CDK_E_NONE;
}

/* Get number of banks for the hash table according to the config */
static int
_hash_bank_count_get(int unit, BCM56560_B0_ENUM_t mem, int *num_banks)
{
    int count;

    switch (mem) {
    case L2Xm_ENUM:
        /*
         * 8k entries in each of 2 dedicated L2 bank
         * 64k entries in each shared bank
         */
        count = L2Xm_MAX - L2Xm_MIN + 1;;
        *num_banks = 2 + (count - 2 * 8 * 1024) / (64 * 1024);
        break;
    case L3_ENTRY_ONLYm_ENUM:
    case L3_ENTRY_IPV4_UNICASTm_ENUM:
    case L3_ENTRY_IPV4_MULTICASTm_ENUM:
    case L3_ENTRY_IPV6_UNICASTm_ENUM:
    case L3_ENTRY_IPV6_MULTICASTm_ENUM:
        /*
         * 1k entries in each of 4 dedicated L3 bank
         * 64k entries in each shared bank
         */
        count = L3_ENTRY_ONLYm_MAX - L3_ENTRY_ONLYm_MIN + 1;
        *num_banks = 4 + (count - 4 * 1 * 1024) / (64 * 1024);
        break;
    case MPLS_ENTRYm_ENUM:
    case VLAN_XLATEm_ENUM:
    case VLAN_MACm_ENUM:
    case EGR_VLAN_XLATEm_ENUM:
    case ING_VP_VLAN_MEMBERSHIPm_ENUM:
    case EGR_VP_VLAN_MEMBERSHIPm_ENUM:
    case ING_DNAT_ADDRESS_TYPEm_ENUM:
    case L2_ENDPOINT_IDm_ENUM:
    case ENDPOINT_QUEUE_MAPm_ENUM:
        *num_banks = 2;
        break;
    default:
        return CDK_E_INTERNAL;
    }

    return CDK_E_NONE;
}

static int
_hash_bank_init(int unit)
{
    int ioerr = 0;
    int idx;
    int num_shared_l2_banks, num_shared_l3_banks, num_banks;
    int shared_bank_offset = 0;
    int shb;
    ISS_LOG_TO_PHY_BANK_MAP_2r_t iss_bank_map_2;
    LPM_LOG_TO_PHY_BANK_MAPr_t lpm_bank_map;
    L2_ISS_BANK_CONFIGr_t l2_bank_cfg;
    L3_ISS_BANK_CONFIGr_t l3_bank_cfg;
    L2_ISS_LOG_TO_PHY_BANK_MAPr_t l2_iss_bank_map;
    ISS_MEMORY_CONTROL_84r_t iss_mem_ctrl_84;
    L3_ISS_LOG_TO_PHY_BANK_MAPr_t l3_iss_bank_map;
    ENHANCED_HASHING_CONTROLr_t en_hash_ctrl;

    ioerr += _hash_bank_count_get(unit, L2Xm_ENUM, &num_banks);
    num_shared_l2_banks = num_banks - 2;
    ioerr += _hash_bank_count_get(unit, L3_ENTRY_ONLYm_ENUM, &num_banks);
    num_shared_l3_banks = num_banks - 4;

    if (L3_DEFIP_ALPM_IPV4m_MAX > L3_DEFIP_ALPM_IPV4m_MIN) {
        int non_alpm = num_shared_l3_banks + num_shared_l2_banks;

        ISS_LOG_TO_PHY_BANK_MAP_2r_CLR(iss_bank_map_2);
        LPM_LOG_TO_PHY_BANK_MAPr_CLR(lpm_bank_map);
        ISS_MEMORY_CONTROL_84r_CLR(iss_mem_ctrl_84);

        if ((non_alpm) && (non_alpm <= 2)) {
            /* If Shared banks are used between ALPM and non-ALPM memories,
             * then ALPM uses Shared Bank B2, B3 and non-ALPM uses B4, B5 banks
             */
            ISS_LOG_TO_PHY_BANK_MAP_2r_ALPM_BANK_MODEf_SET(iss_bank_map_2, 1);
            ioerr += WRITE_ISS_LOG_TO_PHY_BANK_MAP_2r(unit, iss_bank_map_2);
            LPM_LOG_TO_PHY_BANK_MAPr_ALPM_BANK_MODEf_SET(lpm_bank_map, 1);
            ioerr += WRITE_LPM_LOG_TO_PHY_BANK_MAPr(unit, lpm_bank_map);
            ISS_MEMORY_CONTROL_84r_BYPASS_ISS_MEMORY_LPf_SET(iss_mem_ctrl_84,
                0x3);
            ioerr += WRITE_ISS_MEMORY_CONTROL_84r(unit, iss_mem_ctrl_84);

            shared_bank_offset = 2;
        } else {
            ISS_LOG_TO_PHY_BANK_MAP_2r_ALPM_BANK_MODEf_SET(iss_bank_map_2, 0);
            ioerr += WRITE_ISS_LOG_TO_PHY_BANK_MAP_2r(unit, iss_bank_map_2);
            LPM_LOG_TO_PHY_BANK_MAPr_ALPM_BANK_MODEf_SET(lpm_bank_map, 0);
            ioerr += WRITE_LPM_LOG_TO_PHY_BANK_MAPr(unit, lpm_bank_map);
            ISS_MEMORY_CONTROL_84r_BYPASS_ISS_MEMORY_LPf_SET(iss_mem_ctrl_84,
                0xf);
            ioerr += WRITE_ISS_MEMORY_CONTROL_84r(unit, iss_mem_ctrl_84);
        }
    }

    shared_bank_offset += num_shared_l2_banks;
    L2_ISS_BANK_CONFIGr_L2_ENTRY_BANK_CONFIGf_SET(l2_bank_cfg,
                        ((1 << num_shared_l2_banks) - 1) << shared_bank_offset);
    for (idx = 0; idx < num_shared_l2_banks; idx++) {
        shb = idx + shared_bank_offset;
        L2_ISS_LOG_TO_PHY_BANK_MAPr_L2_ENTRY_BANK_2f_SET(l2_iss_bank_map, shb);
        L2_ISS_LOG_TO_PHY_BANK_MAPr_L2_ENTRY_BANK_3f_SET(l2_iss_bank_map, shb);
        L2_ISS_LOG_TO_PHY_BANK_MAPr_L2_ENTRY_BANK_4f_SET(l2_iss_bank_map, shb);
        L2_ISS_LOG_TO_PHY_BANK_MAPr_L2_ENTRY_BANK_5f_SET(l2_iss_bank_map, shb);
    }

    shared_bank_offset += num_shared_l2_banks;
    L3_ISS_BANK_CONFIGr_L3_ENTRY_BANK_CONFIGf_SET(l3_bank_cfg,
                        ((1 << num_shared_l3_banks) - 1) << shared_bank_offset);
    for (idx = 0; idx < num_shared_l3_banks; idx++) {
        shb = idx + shared_bank_offset;
        L3_ISS_LOG_TO_PHY_BANK_MAPr_L3_ENTRY_BANK_4f_SET(l3_iss_bank_map, shb);
        L3_ISS_LOG_TO_PHY_BANK_MAPr_L3_ENTRY_BANK_5f_SET(l3_iss_bank_map, shb);
        L3_ISS_LOG_TO_PHY_BANK_MAPr_L3_ENTRY_BANK_6f_SET(l3_iss_bank_map, shb);
        L3_ISS_LOG_TO_PHY_BANK_MAPr_L3_ENTRY_BANK_7f_SET(l3_iss_bank_map, shb);
    }

    ioerr += READ_ENHANCED_HASHING_CONTROLr(unit, &en_hash_ctrl);
    ENHANCED_HASHING_CONTROLr_RH_FLOWSET_TABLE_CONFIG_ENCODINGf_SET(en_hash_ctrl,
        0);
    ioerr += WRITE_ENHANCED_HASHING_CONTROLr(unit, en_hash_ctrl);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

static int
_misc_init(int unit)
{
    int rv, ioerr = 0;
    MISCCONFIGr_t misccfg;
    MMU_CCP_EN_COR_ERR_RPTr_t mmu_ccp_en_cor_err_rpt;
    PGW_ING_HW_RESET_CONTROL_2r_t pgw_ing_ctrl2;
    int blk_idx;
    ING_HW_RESET_CONTROL_1r_t ing_hw_reset_ctrl1;
    ING_HW_RESET_CONTROL_2r_t ing_hw_reset_ctrl2;
    int count, cmp;
    EGR_HW_RESET_CONTROL_0r_t egr_hw_reset_ctrl0;
    EGR_HW_RESET_CONTROL_1r_t egr_hw_reset_ctrl1;
    ING_HW_RESET_CONTROL_1r_t ing_rst_ctl_1;
    ING_HW_RESET_CONTROL_2r_t ing_rst_ctl_2;
    EGR_HW_RESET_CONTROL_0r_t egr_rst_ctl_0;
    EGR_HW_RESET_CONTROL_1r_t egr_rst_ctl_1;
    int idx;
    EGR_VLAN_CONTROL_1m_t egr_vlan_ctrl_1;
    EGR_VLAN_CONTROL_2m_t egr_vlan_ctrl_2;
    EGR_VLAN_CONTROL_3m_t egr_vlan_ctrl_3;
    EGR_SF_SRC_MODID_CHECKr_t egr_sf_chk;
    EGR_PVLAN_EPORT_CONTROLr_t egr_pvlan_ctrl;
    EGR_1588_LINK_DELAY_64r_t egr_1588_link_delay;
    EGR_FCOE_INVALID_CRC_FRAMESr_t egr_fcoe_inv_crc;
    EGR_FCOE_DELIMITER_ERROR_FRAMESr_t egr_fcoe_errf;
    ING_TRILL_ADJACENCYr_t ing_trill_adj;
    SFLOW_ING_THRESHOLDr_t sflow_ing_threshold;
    CPU_PBMm_t cpu_pbm;
    CPU_PBM_2m_t cpu_pbm_2;
    MULTIPASS_LOOPBACK_BITMAPm_t mpass_lb_bmap;
    cdk_pbmp_t all_pbmp, xlpbmp, clpbmp, clgpbmp;
    int port, lport;
    CLPORT_MIB_RESETr_t clp_mib_reset;
    CLG2PORT_MIB_RESETr_t clg2p_mib_reset;
    XLPORT_MIB_RESETr_t xlp_mib_reset;
    EGR_ING_PORTm_t egr_ing_port;
    uint32_t port_field = 0, port_mode;
    int sub_port;
    int inst_nums;
    XLPORT_MAC_CONTROLr_t xlp_mac_ctrl;
    CLPORT_MAC_CONTROLr_t clp_mac_ctrl;
    CLG2PORT_MAC_CONTROLr_t clg2p_mac_ctrl;
    int cfg_port;
    CLG2PORT_MAC_RSV_MASKr_t clg2p_mac_rsv_mask;
    CLG2PORT_MODE_REGr_t clg2port_mode;
    CLG2PORT_SOFT_RESETr_t clg2port_reset;
    L2_TABLE_HASH_CONTROLr_t l2_hash_ctrl;
    SHARED_TABLE_HASH_CONTROLr_t sh_hash_ctrl;
    L2_ENTRY_CONTROL_6r_t l2_ctrl_6;
    XLPORT_SOFT_RESETr_t xlport_reset;
    XLPORT_MODE_REGr_t xlport_mode;
    uint32_t pbm[PBM_PORT_WORDS];
    ING_DEST_PORT_ENABLEm_t ing_dest_port_en;
    MODPORT_MAP_SUBPORTm_t mod_subport;
    MY_MODID_SET_2_64r_t modid_2;
    ING_CONFIG_64r_t ing_cfg;
    EGR_CONFIG_1r_t egr_cfg1;
    TOQ_MC_CFG1r_t toq_mc_cfg1;
    ING_EN_EFILTER_BITMAPm_t ing_en_efilter;
    int mdio_div;
    int core_freq;
    L2_AGE_DEBUGr_t l2_age_dbg;
    EGR_ENABLEm_t egr_enable;
    SW2_FP_DST_ACTION_CONTROLr_t sw2_fp_act_ctrl;
    CMIC_RATE_ADJUSTr_t rate_adjust;
    CMIC_RATE_ADJUST_INT_MDIOr_t rate_adjust_int;
    EP_REDIR_CONTROLr_t ep_redir_ctrl;
    RDB_ENABLEr_t rdb_enable;
    RDB_CREDIT_ENABLEr_t rdb_credit_enable;
    XLPORT_MAC_RSV_MASKr_t xlp_mac_rsv_mask;
    CLPORT_MAC_RSV_MASKr_t clp_mac_rsv_mask;
    CLPORT_MODE_REGr_t clport_mode;
    CLPORT_SOFT_RESETr_t clport_reset;

    bcm56560_b0_all_pbmp_get(unit, &all_pbmp);
    bcm56560_b0_xlport_pbmp_get(unit, &xlpbmp);
    bcm56560_b0_clport_pbmp_get(unit, &clpbmp);
    bcm56560_b0_clg2port_pbmp_get(unit, &clgpbmp);

    /* Start MMU memory initialization */
    ioerr += READ_MISCCONFIGr(unit, &misccfg);
    MISCCONFIGr_INIT_MEMf_SET(misccfg, 0);
    ioerr += WRITE_MISCCONFIGr(unit, misccfg);

    ioerr += READ_MMU_CCP_EN_COR_ERR_RPTr(unit, &mmu_ccp_en_cor_err_rpt);
    MMU_CCP_EN_COR_ERR_RPTr_CCP0_RESEQf_SET(mmu_ccp_en_cor_err_rpt, 0);
    ioerr += WRITE_MMU_CCP_EN_COR_ERR_RPTr(unit, mmu_ccp_en_cor_err_rpt);

    MISCCONFIGr_INIT_MEMf_SET(misccfg, 1);
    ioerr += WRITE_MISCCONFIGr(unit, misccfg);
    BMD_SYS_USLEEP(10); /* Allow things to stabalize */

    MMU_CCP_EN_COR_ERR_RPTr_CCP0_RESEQf_SET(mmu_ccp_en_cor_err_rpt, 1);
    ioerr += WRITE_MMU_CCP_EN_COR_ERR_RPTr(unit, mmu_ccp_en_cor_err_rpt);

    /*
     * Reset the PGW, IPIPE and EPIPE block
     */
    PGW_ING_HW_RESET_CONTROL_2r_CLR(pgw_ing_ctrl2);
    PGW_ING_HW_RESET_CONTROL_2r_RESET_ALLf_SET(pgw_ing_ctrl2, 1);
    PGW_ING_HW_RESET_CONTROL_2r_VALIDf_SET(pgw_ing_ctrl2, 1);
    for (blk_idx = 0; blk_idx < 2; blk_idx++) {
        ioerr += WRITE_PGW_ING_HW_RESET_CONTROL_2r(unit, blk_idx, pgw_ing_ctrl2);
    }

    ING_HW_RESET_CONTROL_1r_CLR(ing_hw_reset_ctrl1);
    ioerr += WRITE_ING_HW_RESET_CONTROL_1r(unit, ing_hw_reset_ctrl1);
    ING_HW_RESET_CONTROL_2r_CLR(ing_hw_reset_ctrl2);
    ING_HW_RESET_CONTROL_2r_RESET_ALLf_SET(ing_hw_reset_ctrl2, 1);
    ING_HW_RESET_CONTROL_2r_VALIDf_SET(ing_hw_reset_ctrl2, 1);

    /* Set count to # entries of largest IPIPE table */
    count = RH_HGT_FLOWSETm_MAX - RH_HGT_FLOWSETm_MIN + 1;
    cmp = L2Xm_MAX - L2Xm_MIN + 1;
    if (count < cmp) {
        count = cmp;
    }
    cmp = L3_ENTRY_ONLYm_MAX - L3_ENTRY_ONLYm_MIN + 1;
    if (count < cmp) {
        count = cmp;
    }
    cmp = L3_DEFIP_ALPM_IPV4m_MAX - L3_DEFIP_ALPM_IPV4m_MIN + 1;
    if (count < cmp) {
        count = cmp;
    }
    ING_HW_RESET_CONTROL_2r_COUNTf_SET(ing_hw_reset_ctrl2, count);

    EGR_HW_RESET_CONTROL_0r_CLR(egr_hw_reset_ctrl0);
    ioerr += WRITE_EGR_HW_RESET_CONTROL_0r(unit, egr_hw_reset_ctrl0);

    EGR_HW_RESET_CONTROL_1r_CLR(egr_hw_reset_ctrl1);
    EGR_HW_RESET_CONTROL_1r_RESET_ALLf_SET(egr_hw_reset_ctrl1, 1);
    EGR_HW_RESET_CONTROL_1r_VALIDf_SET(egr_hw_reset_ctrl1, 1);
    /* Set count to # entries of largest EPIPE table (EGR_L3_NEXT_HOP) */
    count = EGR_L3_NEXT_HOPm_MAX - EGR_L3_NEXT_HOPm_MIN + 1;
    EGR_HW_RESET_CONTROL_1r_COUNTf_SET(egr_hw_reset_ctrl1, count);
    ioerr += WRITE_EGR_HW_RESET_CONTROL_1r(unit, egr_hw_reset_ctrl1);

    BMD_SYS_USLEEP(1000);

    EGR_HW_RESET_CONTROL_0r_CLR(egr_hw_reset_ctrl0);
    EGR_HW_RESET_CONTROL_0r_STAGE_NUMBERf_SET(egr_hw_reset_ctrl0, 1);
    EGR_HW_RESET_CONTROL_0r_START_ADDRESSf_SET(egr_hw_reset_ctrl0, 0x80000);
    ioerr += WRITE_EGR_HW_RESET_CONTROL_0r(unit, egr_hw_reset_ctrl0);

    EGR_HW_RESET_CONTROL_1r_CLR(egr_hw_reset_ctrl1);
    EGR_HW_RESET_CONTROL_1r_VALIDf_SET(egr_hw_reset_ctrl1, 1);
    EGR_HW_RESET_CONTROL_1r_COUNTf_SET(egr_hw_reset_ctrl1, count);
    ioerr += WRITE_EGR_HW_RESET_CONTROL_1r(unit, egr_hw_reset_ctrl1);

    BMD_SYS_USLEEP(1000);

    /* Clear HW reset control registers */
    PGW_ING_HW_RESET_CONTROL_2r_CLR(pgw_ing_ctrl2);
    for (blk_idx = 0; blk_idx < 2; blk_idx++) {
        ioerr += WRITE_PGW_ING_HW_RESET_CONTROL_2r(unit, blk_idx, pgw_ing_ctrl2);
    }

    ING_HW_RESET_CONTROL_2r_CLR(ing_hw_reset_ctrl2);
    EGR_HW_RESET_CONTROL_1r_CLR(egr_hw_reset_ctrl1);
    WRITE_EGR_HW_RESET_CONTROL_1r(unit, egr_hw_reset_ctrl1);
    WRITE_ING_HW_RESET_CONTROL_2r(unit, ing_hw_reset_ctrl2);

    /* Reset the IPIPE block */
    ING_HW_RESET_CONTROL_1r_CLR(ing_rst_ctl_1);
    ioerr += WRITE_ING_HW_RESET_CONTROL_1r(unit, ing_rst_ctl_1);
    ING_HW_RESET_CONTROL_2r_CLR(ing_rst_ctl_2);
    ING_HW_RESET_CONTROL_2r_RESET_ALLf_SET(ing_rst_ctl_2, 1);
    ING_HW_RESET_CONTROL_2r_VALIDf_SET(ing_rst_ctl_2, 1);
    ING_HW_RESET_CONTROL_2r_COUNTf_SET(ing_rst_ctl_2, 0x88000);
    ioerr += WRITE_ING_HW_RESET_CONTROL_2r(unit, ing_rst_ctl_2);

    /* Reset the EPIPE block */
    EGR_HW_RESET_CONTROL_0r_CLR(egr_rst_ctl_0);
    ioerr += WRITE_EGR_HW_RESET_CONTROL_0r(unit, egr_rst_ctl_0);
    EGR_HW_RESET_CONTROL_1r_CLR(egr_rst_ctl_1);
    EGR_HW_RESET_CONTROL_1r_RESET_ALLf_SET(egr_rst_ctl_1, 1);
    EGR_HW_RESET_CONTROL_1r_VALIDf_SET(egr_rst_ctl_1, 1);
    EGR_HW_RESET_CONTROL_1r_COUNTf_SET(egr_rst_ctl_1, 0xc000);
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
        CDK_WARN(("bcm56560_b0_bmd_init[%d]: ING_HW_RESET timeout\n", unit));
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
        CDK_WARN(("bcm56560_b0_bmd_init[%d]: EGR_HW_RESET timeout\n", unit));
        return ioerr ? CDK_E_IO : CDK_E_TIMEOUT;
    }

    /* Clear pipe reset registers */
    ING_HW_RESET_CONTROL_2r_CLR(ing_rst_ctl_2);
    ioerr += WRITE_ING_HW_RESET_CONTROL_2r(unit, ing_rst_ctl_2);
    EGR_HW_RESET_CONTROL_1r_CLR(egr_rst_ctl_1);
    ioerr += WRITE_EGR_HW_RESET_CONTROL_1r(unit, egr_rst_ctl_1);

    rv = _hash_bank_init(unit);

    /* Clear registers that are implemented in memory */
    EGR_VLAN_CONTROL_1m_CLR(egr_vlan_ctrl_1);
    EGR_VLAN_CONTROL_2m_CLR(egr_vlan_ctrl_2);
    EGR_VLAN_CONTROL_3m_CLR(egr_vlan_ctrl_3);
    EGR_SF_SRC_MODID_CHECKr_CLR(egr_sf_chk);
    EGR_PVLAN_EPORT_CONTROLr_CLR(egr_pvlan_ctrl);
    EGR_1588_LINK_DELAY_64r_CLR(egr_1588_link_delay);
    EGR_FCOE_INVALID_CRC_FRAMESr_CLR(egr_fcoe_inv_crc);
    EGR_FCOE_DELIMITER_ERROR_FRAMESr_CLR(egr_fcoe_errf);
    ING_TRILL_ADJACENCYr_CLR(ing_trill_adj);
    SFLOW_ING_THRESHOLDr_CLR(sflow_ing_threshold);
    CDK_PBMP_ITER(all_pbmp, port) {
        lport = P2L(unit, port);
        if (lport == -1) {
            continue;
        }
        ioerr += WRITE_EGR_VLAN_CONTROL_1m(unit, lport, egr_vlan_ctrl_1);
        ioerr += WRITE_EGR_VLAN_CONTROL_2m(unit, lport, egr_vlan_ctrl_2);
        ioerr += WRITE_EGR_VLAN_CONTROL_3m(unit, lport, egr_vlan_ctrl_3);
        ioerr += WRITE_EGR_PVLAN_EPORT_CONTROLr(unit, lport, egr_pvlan_ctrl);
        ioerr += WRITE_EGR_SF_SRC_MODID_CHECKr(unit, lport, egr_sf_chk);
        ioerr += WRITE_EGR_1588_LINK_DELAY_64r(unit, lport, egr_1588_link_delay);
        ioerr += WRITE_EGR_FCOE_INVALID_CRC_FRAMESr(unit, lport, egr_fcoe_inv_crc);
        ioerr += WRITE_EGR_FCOE_DELIMITER_ERROR_FRAMESr(unit, lport, egr_fcoe_errf);
        ioerr += WRITE_ING_TRILL_ADJACENCYr(unit, lport, ing_trill_adj);
        ioerr += WRITE_SFLOW_ING_THRESHOLDr(unit, lport, sflow_ing_threshold);
    }

    /* Initialize port mappings */
    ioerr += _port_map_init(unit);

    rv = _tdm_b0_init(unit);
    if (CDK_FAILURE(rv)) {
        return rv;
    }

    rv = _idb_init(unit);
    if (CDK_FAILURE(rv)) {
        return rv;
    }

    rv = _edb_init(unit);
    if (CDK_FAILURE(rv)) {
        return rv;
    }

    /* Configure CPU port */
    CPU_PBMm_CLR(cpu_pbm);
    CPU_PBMm_SET(cpu_pbm, 0, 1 << CMIC_PORT);
    ioerr += WRITE_CPU_PBMm(unit, 0, cpu_pbm);
    CPU_PBM_2m_CLR(cpu_pbm_2);
    CPU_PBM_2m_SET(cpu_pbm_2, 0, 1 << CMIC_PORT);
    ioerr += WRITE_CPU_PBM_2m(unit, 0, cpu_pbm_2);

    /* Configure loopback ports (not used) */
    MULTIPASS_LOOPBACK_BITMAPm_CLR(mpass_lb_bmap);
    ioerr += WRITE_MULTIPASS_LOOPBACK_BITMAPm(unit, 0, mpass_lb_bmap);

    CDK_PBMP_ITER(all_pbmp, port) {
        lport = P2L(unit, port);
        EGR_ING_PORTm_CLR(egr_ing_port);
        if (BMD_PORT_PROPERTIES(unit, port) & BMD_PORT_HG) {
            EGR_ING_PORTm_PORT_TYPEf_SET(egr_ing_port, 1);
            ioerr += WRITE_EGR_ING_PORTm(unit, lport, egr_ing_port);
        }
        EGR_ING_PORTm_PORT_TYPEf_SET(egr_ing_port, 2);
        ioerr += WRITE_EGR_ING_PORTm(unit, LB_LPORT, egr_ing_port);
    }

    _portctrl_port_init(unit);

    /* Initialize XLPORTs */
    CDK_PBMP_ITER(xlpbmp, port) {
        /* We only need to write once per block */
        if (((port - 1) % 4) != 0) {
            continue;
        }

        /* Configure each sub port */
        port_field = 0;
        for (sub_port = 0; sub_port <= 3; sub_port++) {;
            if (!CDK_PBMP_MEMBER(xlpbmp, port + sub_port)) {
                continue;
            }
            port_field |= (1 << sub_port);
        }

        inst_nums = 1;
        if (bcm56560_b0_port_speed_max(unit, port >= 100000)) {
            port_field = 0xf;
            inst_nums = 3;
        }

        for (idx = 0; idx < inst_nums; idx++) {
            cfg_port = port + idx;

            /* Assert XLPORT soft reset */
            XLPORT_SOFT_RESETr_CLR(xlport_reset);
            XLPORT_SOFT_RESETr_SET(xlport_reset, port_field);
            ioerr += WRITE_XLPORT_SOFT_RESETr(unit, xlport_reset, cfg_port);

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
            ioerr += WRITE_XLPORT_MODE_REGr(unit, xlport_mode, cfg_port);

            /* Bring MAC out of reset */
            ioerr += READ_XLPORT_MAC_CONTROLr(unit, &xlp_mac_ctrl, cfg_port);
            XLPORT_MAC_CONTROLr_XMAC0_RESETf_SET(xlp_mac_ctrl, 0);
            ioerr += WRITE_XLPORT_MAC_CONTROLr(unit, xlp_mac_ctrl, cfg_port);
        }
    }

    CDK_PBMP_ITER(xlpbmp, port) {
        /* RSV Mask */
        XLPORT_MAC_RSV_MASKr_CLR(xlp_mac_rsv_mask);
        XLPORT_MAC_RSV_MASKr_MASKf_SET(xlp_mac_rsv_mask, 0x20058);
        ioerr += WRITE_XLPORT_MAC_RSV_MASKr(unit, port, xlp_mac_rsv_mask);
    }

    CDK_PBMP_ITER(xlpbmp, port) {
        /* We only need to write once per block */
        if (XLPORT_SUBPORT(port) != 0) {
            continue;
        }
        XLPORT_MIB_RESETr_CLR(xlp_mib_reset);
        XLPORT_MIB_RESETr_CLR_CNTf_SET(xlp_mib_reset, 0xf);
        ioerr += WRITE_XLPORT_MIB_RESETr(unit, xlp_mib_reset, port);
        XLPORT_MIB_RESETr_CLR(xlp_mib_reset);
        ioerr += WRITE_XLPORT_MIB_RESETr(unit, xlp_mib_reset, port);
    }

    CDK_PBMP_ITER(clpbmp, port) {
        /* We only need to write once per block */
        if (((port - 1) % 4) != 0) {
            continue;
        }
        /* Configure each sub port */
        port_field = 0;
        for (sub_port = 0; sub_port <= 3; sub_port++) {;
            if (!CDK_PBMP_MEMBER(clpbmp, port + sub_port)) {
                continue;
            }
            port_field |= (1 << sub_port);
        }

        inst_nums = 1;
        if (bcm56560_b0_port_speed_max(unit, port >= 100000)) {
            port_field = 0xf;
            inst_nums = 3;
        }

        for (idx = 0; idx < inst_nums; idx++) {
            cfg_port = port + idx;

            /* Assert XLPORT soft reset */
            CLPORT_SOFT_RESETr_CLR(clport_reset);
            CLPORT_SOFT_RESETr_SET(clport_reset, port_field);
            ioerr += WRITE_CLPORT_SOFT_RESETr(unit, clport_reset, cfg_port);

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

            CLPORT_MODE_REGr_CLR(clport_mode);
            CLPORT_MODE_REGr_XPORT0_CORE_PORT_MODEf_SET(clport_mode, port_mode);
            CLPORT_MODE_REGr_XPORT0_PHY_PORT_MODEf_SET(clport_mode, port_mode);
            ioerr += WRITE_CLPORT_MODE_REGr(unit, clport_mode, cfg_port);

            /* Bring MAC out of reset */
            ioerr += READ_CLPORT_MAC_CONTROLr(unit, &clp_mac_ctrl, cfg_port);
            CLPORT_MAC_CONTROLr_XMAC0_RESETf_SET(clp_mac_ctrl, 0);
            ioerr += WRITE_CLPORT_MAC_CONTROLr(unit, clp_mac_ctrl, cfg_port);
        }
    }

    CDK_PBMP_ITER(clpbmp, port) {
        /* RSV Mask */
        CLPORT_MAC_RSV_MASKr_CLR(clp_mac_rsv_mask);
        CLPORT_MAC_RSV_MASKr_MASKf_SET(clp_mac_rsv_mask, 0x20058);
        ioerr += WRITE_CLPORT_MAC_RSV_MASKr(unit, port, clp_mac_rsv_mask);
    }

    CDK_PBMP_ITER(clpbmp, port) {
        /* We only need to write once per block */
        if (((port - 1) % 4) != 0) {
            continue;
        }

        CLPORT_MIB_RESETr_CLR(clp_mib_reset);
        CLPORT_MIB_RESETr_CLR_CNTf_SET(clp_mib_reset, 0xf);
        ioerr += WRITE_CLPORT_MIB_RESETr(unit, clp_mib_reset, port);
        CLPORT_MIB_RESETr_CLR(clp_mib_reset);
        ioerr += WRITE_CLPORT_MIB_RESETr(unit, clp_mib_reset, port);
    }

    CDK_PBMP_ITER(clgpbmp, port) {
        /* We only need to write once per block */
        if (((port - 1) % 4) != 0) {
            continue;
        }
        /* Configure each sub port */
        port_field = 0;
        for (sub_port = 0; sub_port <= 3; sub_port++) {;
            if (!CDK_PBMP_MEMBER(clgpbmp, port + sub_port)) {
                continue;
            }
            port_field |= (1 << sub_port);
        }

        inst_nums = 1;
        if (bcm56560_b0_port_speed_max(unit, port >= 100000)) {
            port_field = 0xf;
            inst_nums = 3;
        }

        for (idx = 0; idx < inst_nums; idx++) {
            cfg_port = port + idx;

            /* Assert XLPORT soft reset */
            CLG2PORT_SOFT_RESETr_CLR(clg2port_reset);
            CLG2PORT_SOFT_RESETr_SET(clg2port_reset, port_field);
            ioerr += WRITE_CLG2PORT_SOFT_RESETr(unit, clg2port_reset, cfg_port);

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

            CLG2PORT_MODE_REGr_CLR(clg2port_mode);
            CLG2PORT_MODE_REGr_XPORT0_CORE_PORT_MODEf_SET(clg2port_mode, port_mode);
            CLG2PORT_MODE_REGr_XPORT0_PHY_PORT_MODEf_SET(clg2port_mode, port_mode);
            ioerr += WRITE_CLG2PORT_MODE_REGr(unit, clg2port_mode, cfg_port);

            /* Bring MAC out of reset */
            ioerr += READ_CLG2PORT_MAC_CONTROLr(unit, &clg2p_mac_ctrl, cfg_port);
            CLG2PORT_MAC_CONTROLr_XMAC0_RESETf_SET(clg2p_mac_ctrl, 0);
            ioerr += WRITE_CLG2PORT_MAC_CONTROLr(unit, clg2p_mac_ctrl, cfg_port);
        }
    }

    CDK_PBMP_ITER(clgpbmp, port) {
        /* RSV Mask */
        CLG2PORT_MAC_RSV_MASKr_CLR(clg2p_mac_rsv_mask);
        CLG2PORT_MAC_RSV_MASKr_MASKf_SET(clg2p_mac_rsv_mask, 0x20058);
        ioerr += WRITE_CLG2PORT_MAC_RSV_MASKr(unit, port, clg2p_mac_rsv_mask);
    }

    CDK_PBMP_ITER(clgpbmp, port) {
        /* We only need to write once per block */
        if (((port - 1) % 4) != 0) {
            continue;
        }

        CLG2PORT_MIB_RESETr_CLR(clg2p_mib_reset);
        CLG2PORT_MIB_RESETr_CLR_CNTf_SET(clg2p_mib_reset, 0xf);
        ioerr += WRITE_CLG2PORT_MIB_RESETr(unit, clg2p_mib_reset, port);
        CLG2PORT_MIB_RESETr_CLR(clg2p_mib_reset);
        ioerr += WRITE_CLG2PORT_MIB_RESETr(unit, clg2p_mib_reset, port);
    }

    /* Enable Field Processor metering clock */
    ioerr += READ_MISCCONFIGr(unit, &misccfg);
    MISCCONFIGr_METERING_CLK_ENf_SET(misccfg, 1);
    ioerr += WRITE_MISCCONFIGr(unit, misccfg);

    /* Enable dual hash tables */
    READ_L2_TABLE_HASH_CONTROLr(unit, &l2_hash_ctrl);
    L2_TABLE_HASH_CONTROLr_BANK0_HASH_OFFSETf_SET(l2_hash_ctrl, 0);
    L2_TABLE_HASH_CONTROLr_BANK1_HASH_OFFSETf_SET(l2_hash_ctrl, 16);
    WRITE_L2_TABLE_HASH_CONTROLr(unit, l2_hash_ctrl);

    READ_SHARED_TABLE_HASH_CONTROLr(unit, &sh_hash_ctrl);
    SHARED_TABLE_HASH_CONTROLr_BANK2_HASH_OFFSETf_SET(sh_hash_ctrl, 4);
    SHARED_TABLE_HASH_CONTROLr_BANK3_HASH_OFFSETf_SET(sh_hash_ctrl, 12);
    SHARED_TABLE_HASH_CONTROLr_BANK4_HASH_OFFSETf_SET(sh_hash_ctrl, 20);
    SHARED_TABLE_HASH_CONTROLr_BANK5_HASH_OFFSETf_SET(sh_hash_ctrl, 24);
    WRITE_SHARED_TABLE_HASH_CONTROLr(unit, sh_hash_ctrl);

    ioerr += READ_L2_ENTRY_CONTROL_6r(unit, &l2_ctrl_6);
    L2_ENTRY_CONTROL_6r_DISABLE_L2_ENTRY_LPf_SET(l2_ctrl_6, 3);
    ioerr += WRITE_L2_ENTRY_CONTROL_6r(unit, l2_ctrl_6);

    L2_AGE_DEBUGr_CLR(l2_age_dbg);
    L2_AGE_DEBUGr_AGE_COUNTf_SET(l2_age_dbg, L2Xm_MAX);
    ioerr += WRITE_L2_AGE_DEBUGr(unit, l2_age_dbg);

    /*
     * Egress Enable (for non-serdes ports).
     * For PM ports, it is set during port enable/disable sequence.
     */
    CDK_PBMP_ITER(all_pbmp, port) {
        ioerr += READ_EGR_ENABLEm(unit, port, &egr_enable);
        EGR_ENABLEm_PRT_ENABLEf_SET(egr_enable, 1);
        ioerr += WRITE_EGR_ENABLEm(unit, port, egr_enable);
    }

    ioerr += READ_EGR_ENABLEm(unit, CMIC_PORT, &egr_enable);
    EGR_ENABLEm_PRT_ENABLEf_SET(egr_enable, 1);
    ioerr += WRITE_EGR_ENABLEm(unit, CMIC_PORT, egr_enable);

    /* Ensure that link bitmap is cleared */
    ioerr += CDK_XGSD_MEM_CLEAR(unit, EPC_LINK_BMAPm);

    /* Enable all ports */
    CDK_MEMSET(pbm, 0, sizeof(pbm));
    CDK_PBMP_ITER(all_pbmp, port) {
        lport = P2L(unit, port);
        if (lport >= 0) {
            PBM_PORT_ADD(pbm, lport);
        }
    }
    ING_DEST_PORT_ENABLEm_CLR(ing_dest_port_en);
    ING_DEST_PORT_ENABLEm_PORT_BITMAPf_SET(ing_dest_port_en, pbm);
    ioerr += WRITE_ING_DEST_PORT_ENABLEm(unit, 0, ing_dest_port_en);

    MODPORT_MAP_SUBPORTm_CLR(mod_subport);
    MODPORT_MAP_SUBPORTm_ENABLEf_SET(mod_subport, 1);

    /* my_modid and other modid related initialization */
    CDK_PBMP_ITER(all_pbmp, port) {
        /* configure logical port numbers */
        lport = P2L(unit, port);
        if (lport >= 0) {
            MODPORT_MAP_SUBPORTm_DESTf_SET(mod_subport, lport);
            ioerr += WRITE_MODPORT_MAP_SUBPORTm(unit, lport, mod_subport);
        }
    }

    /* setting up my_modid */
    ioerr += READ_MY_MODID_SET_2_64r(unit, &modid_2);
    MY_MODID_SET_2_64r_MODID_0_VALIDf_SET(modid_2, 1);
    MY_MODID_SET_2_64r_MODID_0f_SET(modid_2, 0);
    ioerr += WRITE_MY_MODID_SET_2_64r(unit, modid_2);

    /* Basic ingress configuration */
    ioerr += READ_ING_CONFIG_64r(unit, &ing_cfg);
    ING_CONFIG_64r_L3SRC_HIT_ENABLEf_SET(ing_cfg, 1);
    ING_CONFIG_64r_L2DST_HIT_ENABLEf_SET(ing_cfg, 1);
    ING_CONFIG_64r_APPLY_EGR_MASK_ON_L2f_SET(ing_cfg, 1);
    ING_CONFIG_64r_APPLY_EGR_MASK_ON_L3f_SET(ing_cfg, 1);
    /* Enable both ARP & RARP */
    ING_CONFIG_64r_ARP_RARP_TO_FPf_SET(ing_cfg, 0x3);
    ING_CONFIG_64r_ARP_VALIDATION_ENf_SET(ing_cfg, 1);
    ING_CONFIG_64r_USE_MY_STATION1_FOR_NON_TUNNELSf_SET(ing_cfg, 1);
    ioerr += WRITE_ING_CONFIG_64r(unit, ing_cfg);

    ioerr += READ_EGR_CONFIG_1r(unit, &egr_cfg1);
    EGR_CONFIG_1r_RING_MODEf_SET(egr_cfg1, 1);
    ioerr += WRITE_EGR_CONFIG_1r(unit, egr_cfg1);

    /* T2OQ needs to be setup for all ports that require 1 in 9 spacing or less
     *  In Apache, it is safe to program all mmu ports 0-15 in T2OQ (whether in
     *  high speed mode or not). In TD2+, part of the MCQDB memory was re-used
     *  for T2OQ; Apache has separate 160 deep MCQDB memory */
    ioerr += READ_TOQ_MC_CFG1r(unit, &toq_mc_cfg1);
    TOQ_MC_CFG1r_IS_MC_T2OQ_PORT0f_SET(toq_mc_cfg1, 0xffff);
    ioerr += WRITE_TOQ_MC_CFG1r(unit, toq_mc_cfg1);

    /* The HW defaults for EGR_VLAN_CONTROL_1.VT_MISS_UNTAG == 1, which
     * causes the outer tag to be removed from packets that don't have
     * a hit in the egress vlan tranlation table. Set to 0 to disable this.
     */
    EGR_VLAN_CONTROL_1m_CLR(egr_vlan_ctrl_1);
    EGR_VLAN_CONTROL_1m_VT_MISS_UNTAGf_SET(egr_vlan_ctrl_1, 0);
    EGR_VLAN_CONTROL_1m_REMARK_OUTER_DOT1Pf_SET(egr_vlan_ctrl_1, 1);
    CDK_PBMP_ITER(all_pbmp, port) {
        lport = P2L(unit, port);
        ioerr += WRITE_EGR_VLAN_CONTROL_1m(unit, lport, egr_vlan_ctrl_1);
    }
    ioerr += WRITE_EGR_VLAN_CONTROL_1m(unit, CMIC_LPORT, egr_vlan_ctrl_1);

    /* Enable egress VLAN checks for all ports */
    CDK_MEMSET(pbm, 0, sizeof(pbm));
    CDK_PBMP_ITER(all_pbmp, port) {
        lport = P2L(unit, port);
        if (lport >= 0) {
            PBM_PORT_ADD(pbm, lport);
        }
    }
    ING_EN_EFILTER_BITMAPm_CLR(ing_en_efilter);
    ING_EN_EFILTER_BITMAPm_BITMAPf_SET(ing_en_efilter, pbm);
    ioerr += WRITE_ING_EN_EFILTER_BITMAPm(unit, 0, ing_en_efilter);

    /* Setup SW2_FP_DST_ACTION_CONTROL */
    ioerr += READ_SW2_FP_DST_ACTION_CONTROLr(unit, &sw2_fp_act_ctrl);
    SW2_FP_DST_ACTION_CONTROLr_HGTRUNK_RES_ENf_SET(sw2_fp_act_ctrl, 1);
    SW2_FP_DST_ACTION_CONTROLr_LAG_RES_ENf_SET(sw2_fp_act_ctrl, 1);
    ioerr += WRITE_SW2_FP_DST_ACTION_CONTROLr(unit, sw2_fp_act_ctrl);

    /* Core clock frequency */
    core_freq = bcm56560_b0_get_core_frequency(unit);
    /*
     * Set external MDIO freq to around 6MHz
     * target_freq = core_clock_freq * DIVIDEND / DIVISOR / 2
     */
    mdio_div = (core_freq + (6 * 2 - 1)) / (6 * 2);
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
    mdio_div = (core_freq + (12 * 2 - 1)) / (12 * 2);
    CMIC_RATE_ADJUST_INT_MDIOr_CLR(rate_adjust_int);
    CMIC_RATE_ADJUST_INT_MDIOr_DIVISORf_SET(rate_adjust_int, mdio_div);
    CMIC_RATE_ADJUST_INT_MDIOr_DIVIDENDf_SET(rate_adjust_int, 1);
    ioerr += WRITE_CMIC_RATE_ADJUST_INT_MDIOr(unit, rate_adjust_int);

    _ledup_init(unit);

    /* Setup config needed for EP-Redirection */
    /* Enable re-direction */
    EP_REDIR_CONTROLr_CLR(ep_redir_ctrl);
    EP_REDIR_CONTROLr_ENABLEf_SET(ep_redir_ctrl, 1);
    ioerr += WRITE_EP_REDIR_CONTROLr(unit, ep_redir_ctrl);

    /* Enable RX/TX */
    RDB_ENABLEr_CLR(rdb_enable);
    RDB_ENABLEr_RX_ENABLEf_SET(rdb_enable, 1);
    RDB_ENABLEr_TX_ENABLEf_SET(rdb_enable, 1);
    RDB_ENABLEr_CTRL_INITf_SET(rdb_enable, 1);
    ioerr += WRITE_RDB_ENABLEr(unit, rdb_enable);

    /* Enable Credit */
    RDB_CREDIT_ENABLEr_CLR(rdb_credit_enable);
    RDB_CREDIT_ENABLEr_ENABLEf_SET(rdb_credit_enable, 1);
    ioerr += WRITE_RDB_CREDIT_ENABLEr(unit, rdb_credit_enable);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

#define SOC_REG_ADDR_INSTANCE_MASK 0x80000000
int
bcm56560_b0_bmd_init(int unit)
{
    int ioerr = 0;
    int rv;
    int port, lport;
    cdk_pbmp_t all_pbmp, front_pbmp;
    cdk_pbmp_t clpbmp, clgpbmp;
    cdk_pbmp_t cxxpbmp;
    XLPORT_MAC_CONTROLr_t xlp_mac_ctrl;
    CLPORT_MAC_CONTROLr_t clp_mac_ctrl;
    CLG2PORT_MAC_CONTROLr_t clg2p_mac_ctrl;
    RDBGC0_SELECTr_t rdbgc0_select;
    VLAN_PROFILE_TABm_t vlan_profile;
    ING_VLAN_TAG_ACTION_PROFILEm_t vlan_action;
    char *name;
    int lanes;
    int qnum;
    ING_COS_MODE_64r_t ing_cos_mode;
    int idx;

    bcm56560_b0_clport_pbmp_get(unit, &clpbmp);
    bcm56560_b0_clg2port_pbmp_get(unit, &clgpbmp);
    bcm56560_b0_all_pbmp_get(unit, &all_pbmp);
    bcm56560_b0_all_front_pbmp_get(unit, &front_pbmp);
    bcm56560_b0_cxxport_pbmp_get(unit, &cxxpbmp);

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

    /* Configure discard counter */
    RDBGC0_SELECTr_CLR(rdbgc0_select);
    RDBGC0_SELECTr_BITMAPf_SET(rdbgc0_select, 0x0400ad11);
    ioerr += WRITE_RDBGC0_SELECTr(unit, rdbgc0_select);

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

    /* Configure PORTs */
    CDK_PBMP_ITER(front_pbmp, port) {
        if (P2L(unit, port) == -1) {
            continue;
        }

        if ((SPEED_MAX(unit, port) >= 100000) && CDK_PBMP_MEMBER(cxxpbmp, port)) {
            /* 12X10 */
            for (idx=0; idx < 3; idx++) {
                XLPORT_MAC_CONTROLr_CLR(xlp_mac_ctrl);
                ioerr += WRITE_XLPORT_MAC_CONTROLr(unit, xlp_mac_ctrl,
                    port + idx * 4);
            }
            /* Clear MAC hard reset after warpcore is initialized */
            CLPORT_MAC_CONTROLr_CLR(clp_mac_ctrl);
            ioerr += WRITE_CLPORT_MAC_CONTROLr(unit, clp_mac_ctrl, port);
            /* Initialize CLPORTs after CLMAC is out of reset */
            ioerr += bcm56560_b0_clport_init(unit, port);
        } else if (CDK_PBMP_MEMBER(clpbmp, port)) {
            /* 4X25 */
            /* Clear MAC hard reset after warpcore is initialized */
            CLPORT_MAC_CONTROLr_CLR(clp_mac_ctrl);
            ioerr += WRITE_CLPORT_MAC_CONTROLr(unit, clp_mac_ctrl, port);
            /* Initialize CLPORTs after CLMAC is out of reset */
            ioerr += bcm56560_b0_clport_init(unit, port);
        } else if (CDK_PBMP_MEMBER(clgpbmp, port)) {
            /* 4X25 */
            /* Clear MAC hard reset after warpcore is initialized */
            CLG2PORT_MAC_CONTROLr_CLR(clg2p_mac_ctrl);
            ioerr += WRITE_CLG2PORT_MAC_CONTROLr(unit, clg2p_mac_ctrl, port);

            /* Initialize CLG2PORTs after CLMAC is out of reset */
            ioerr += bcm56560_b0_clg2port_init(unit, port);
        } else {
            /* 4x10 */
            /* Clear MAC hard reset after warpcore is initialized */
            if (XLPORT_SUBPORT(port) == 0) {
                XLPORT_MAC_CONTROLr_CLR(xlp_mac_ctrl);
                ioerr += WRITE_XLPORT_MAC_CONTROLr(unit, xlp_mac_ctrl, port);
            }
            /* Initialize XLPORTs after XLMAC is out of reset */
            ioerr += bcm56560_b0_xlport_init(unit, port);
        }
    }

    /* Probe PHYs */
    CDK_PBMP_ITER(front_pbmp, port) {
        if (P2L(unit, port) == -1) {
            continue;
        }
        rv = bmd_phy_probe(unit, port);
        if (CDK_FAILURE(rv)) {
            return rv;
        }

        if (((port >= 29) && (port <= 36)) ||
            ((port >= 65) && (port <= 72))) {
            name = "tscf";
        } else {
            name = "tsce";
        }

        lanes = bcm56560_b0_port_lanes_get(unit, port);
        if (lanes == 4) {
            rv = bmd_phy_mode_set(unit, port, name, BMD_PHY_MODE_SERDES, 0);
        } else if (lanes == 2) {
            rv = bmd_phy_mode_set(unit, port, name, BMD_PHY_MODE_SERDES, 1);
            rv = bmd_phy_mode_set(unit, port, name, BMD_PHY_MODE_2LANE, 1);
        } else if (lanes == 1) {
            rv = bmd_phy_mode_set(unit, port, name, BMD_PHY_MODE_SERDES, 1);
            rv = bmd_phy_mode_set(unit, port, name, BMD_PHY_MODE_2LANE, 0);
        } else {
            CDK_ERR(("Unsupported lanes number : %d\n", lanes));
        }

        if (((port >= 29) && (port <= 36)) ||
            ((port >= 65) && (port <= 72))) {
            if (CDK_SUCCESS(rv)) {
                rv = bmd_phy_fw_helper_set(unit, port, _clg_firmware_helper);
            }
        } else if (((port >= 17) && (port <= 28)) ||
            ((port >= 53) && (port <= 64))) {
            if (CDK_SUCCESS(rv)) {
                rv = bmd_phy_fw_helper_set(unit, port, _cxx_firmware_helper);
            }
        } else {
            if (CDK_SUCCESS(rv)) {
                rv = bmd_phy_fw_helper_set(unit, port, _xl_firmware_helper);
            }
        }

        /* Set the phy default ability to be cached locally,
         * it would then be set in bmd_phy_int function.
         */
        if (CDK_SUCCESS(rv)) {
            rv = _phy_default_ability_set(unit, port);
        }

        if (CDK_SUCCESS(rv)) {
            rv = bmd_phy_init(unit, port);
        }
    }

    /* Common port initialization for CPU port */
    ioerr += _port_init(unit, CMIC_PORT);

    /* Assign queues to ports */
    CDK_PBMP_ITER(all_pbmp, port) {
        lport = P2L(unit, port);
        if (lport == -1) {
            continue;
        }
        qnum = bcm56560_b0_mmu_port_uc_queue_index(unit, port);

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
        rv = bmd_xgsd_dma_init(unit);
    }

    /* Additional configuration required when on PCI bus */
    if (CDK_DEV_FLAGS(unit) & CDK_DEV_MBUS_PCI) {
        CMIC_CMC_HOSTMEM_ADDR_REMAPr_t hostmem_remap;
        uint32_t remap_val[] = { 0x144D2450, 0x19617595, 0x1E75C6DA, 0x1f };

        /* Send DMA data to external host memory when on PCI bus */
        for (idx = 0; idx < COUNTOF(remap_val); idx++) {
            CMIC_CMC_HOSTMEM_ADDR_REMAPr_SET(hostmem_remap, remap_val[idx]);
            ioerr += WRITE_CMIC_CMC_HOSTMEM_ADDR_REMAPr(unit, idx, hostmem_remap);
        }
    }

    /* Enable all 48 CPU COS queues for Rx DMA channel */
    if (CDK_SUCCESS(rv)) {
        CMIC_CMC_COS_CTRL_RX_0r_t cos_ctrl_0;
        CMIC_CMC_COS_CTRL_RX_1r_t cos_ctrl_1;
        uint32_t cos_bmp;

        CMIC_CMC_COS_CTRL_RX_0r_CLR(cos_ctrl_0);
        for (idx = 0; idx < CMIC_NUM_PKT_DMA_CHAN; idx++) {
            cos_bmp = (idx == XGSD_DMA_RX_CHAN) ? 0xffffffff : 0;
            CMIC_CMC_COS_CTRL_RX_0r_COS_BMPf_SET(cos_ctrl_0, cos_bmp);
            ioerr += WRITE_CMIC_CMC_COS_CTRL_RX_0r(unit, idx, cos_ctrl_0);
        }

        CMIC_CMC_COS_CTRL_RX_1r_CLR(cos_ctrl_1);
        for (idx = 0; idx < CMIC_NUM_PKT_DMA_CHAN; idx++) {
            cos_bmp = (idx == XGSD_DMA_RX_CHAN) ? 0xffff: 0;
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
#endif /* CDK_CONFIG_INCLUDE_BCM56560_B0 */

