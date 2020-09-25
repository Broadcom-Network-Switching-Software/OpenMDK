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
#include <bmdi/arch/xgsd_dma.h>

#include <cdk/chip/bcm56960_a0_defs.h>
#include <cdk/cdk_debug.h>
#include <cdk/cdk_util.h>

#include "bcm56960_a0_bmd.h"
#include "bcm56960_a0_internal.h"

#define PIPE_RESET_TIMEOUT_MSEC         50
#define CMIC_NUM_PKT_DMA_CHAN           4

#define JUMBO_MAXSZ                     0x3fe8

#define TDM_LENGTH                      256
#define OVS_GROUP_COUNT                 6
#define OVS_GROUP_TDM_LENGTH            12

#define UPDATE_ALL_PIPES                -1

/* Special port number used by H/W */
#define TH_TDM_OVERSUB_TOKEN            0x22
#define TH_TDM_NULL_TOKEN               0x23
#define TH_TDM_IDL1_TOKEN               0x24
#define TH_TDM_IDL2_TOKEN               0x25
#define TH_TDM_UNUSED_TOKEN             0x3f

#define TH_CELLS_PER_OBM                1012

/* Value hardcoded in set_tdm.c, definition needs to be matched */
#define PORT_STATE_UNUSED               0
#define PORT_STATE_LINERATE             1
#define PORT_STATE_OVERSUB              2
#define PORT_STATE_CONTINUATION         3 /* part of other port */

#define FW_ALIGN_BYTES                  16
#define FW_ALIGN_MASK                   (FW_ALIGN_BYTES - 1)

#define MMU_DEFAULT_PG                  7
#define MMU_CFG_MMU_BYTES_TO_CELLS(_byte, _cellhdr) \
            (((_byte) + (_cellhdr) - 1) / (_cellhdr))

typedef struct _tdm_config_s {
    int speed[TH_NUM_EXT_PORTS];
    int port_state[TH_NUM_EXT_PORTS];
    int *idb_tdm[TH_PIPES_PER_DEV];
    int *idb_ovs_tdm[TH_PIPES_PER_DEV][OVS_GROUP_COUNT];
    int *mmu_tdm[TH_PIPES_PER_DEV];
    int *mmu_ovs_tdm[TH_PIPES_PER_DEV][OVS_GROUP_COUNT];
    int pm_encap_type[TH_NUM_PORT_MODULES];
    th_tdm_globals_t tdm_globals;
    th_tdm_pipes_t tdm_pipe_tables;
    /* Following fields are not arguments to the TDM code */
    int port_ratio[TH_PBLKS_PER_DEV];
    int ovs_ratio_x1000[TH_PIPES_PER_DEV];
}_tdm_config_t;

static int
_port_mapping_init(int unit)
{
    int ioerr = 0;
    int port, lport, idb_port, pipe, mport;
    int num_port, num_phy_port;
    ING_IDB_TO_DEV_MAPm_t ing_i2d;
    SYS_PORTMAPm_t sys_pm;
    EGR_DEV_TO_PHYS_MAPr_t egr_d2p;
    MMU_TO_DEV_MAPr_t m2d;
    MMU_TO_PHYS_MAPr_t m2p;
    MMU_TO_SYS_MAPr_t m2s;

    num_port = PORT_TABm_MAX - PORT_TABm_MIN;
    num_phy_port = NUM_PHYS_PORTS;

    /* Ingress physical to device port mapping */
    ING_IDB_TO_DEV_MAPm_CLR(ing_i2d);
    for (port = 0; port < num_phy_port; port++) {
        if (port == 130) {
            /* undefined physical port */
            pipe = 3;
            idb_port = 32;
        } else {
            pipe = bcm56960_a0_port_pipe_get(unit, port);
            idb_port = bcm56960_a0_idbport_get(unit, port);
        }
        if (pipe < 0 || idb_port < 0) {
            continue;
        }
        lport = P2L(unit, port);
        if (lport < 0) {
            lport = 0xff;
        }
        ING_IDB_TO_DEV_MAPm_DEVICE_PORT_NUMBERf_SET(ing_i2d, lport);
        ioerr += WRITE_ING_IDB_TO_DEV_MAPm(unit, pipe, idb_port, ing_i2d);
    }

    /* Ingress GPP port to device port mapping */
    SYS_PORTMAPm_CLR(sys_pm);
    for (port = 0; port < num_port; port ++) {
        SYS_PORTMAPm_DEVICE_PORT_NUMBERf_SET(sys_pm, port);
        ioerr += WRITE_SYS_PORTMAPm(unit, port, sys_pm);
    }

    /* Ingress device port to GPP port mapping
     * PORT_TAB.SRC_SYS_PORT_ID is programmed in the general port config
     * init routine _bcm_fb_port_cfg_init()
     */
    
    /* Egress device port to physical port mapping */
    EGR_DEV_TO_PHYS_MAPr_CLR(egr_d2p);
    for (lport = 0; lport < NUM_LOGIC_PORTS; lport ++) {
        port = L2P(unit, lport);
        if (port < 0) {
            continue;
        }
        EGR_DEV_TO_PHYS_MAPr_PHYSICAL_PORT_NUMBERf_SET(egr_d2p, port);
        ioerr += WRITE_EGR_DEV_TO_PHYS_MAPr(unit, lport, egr_d2p);
    }
    
    /* MMU port to device port mapping */
    MMU_TO_DEV_MAPr_CLR(m2d);
    for (lport = 0; lport < NUM_LOGIC_PORTS; lport ++) {
        MMU_TO_DEV_MAPr_DEVICE_PORTf_SET(m2d, lport);
        mport = P2M(unit, L2P(unit, lport));
        if (mport < 0) {
            continue;
        }
        ioerr += WRITE_MMU_TO_DEV_MAPr(unit, mport, m2d);
    }

    /* MMU port to physical port mapping and MMU port to system port mapping */
    MMU_TO_PHYS_MAPr_CLR(m2p);
    MMU_TO_SYS_MAPr_CLR(m2s);
    for (port = 0; port < num_phy_port; port ++) {
        mport = P2M(unit, port);
        if (mport < 0) {
            continue;
        }
        MMU_TO_PHYS_MAPr_PHY_PORTf_SET(m2p, port);
        ioerr += WRITE_MMU_TO_PHYS_MAPr(unit, mport, m2p);
        lport = P2L(unit, port);
        if (lport < 0) {
            lport = 0xff;
        }
        MMU_TO_SYS_MAPr_SYSTEM_PORTf_SET(m2s, lport);        
        ioerr += WRITE_MMU_TO_SYS_MAPr(unit, mport, m2s);
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

/*
 * #1   single: 100  -   x  -   x  -   x  TH_PORT_RATIO_SINGLE
 * #2   single:  40  -   x  -   x  -   x  TH_PORT_RATIO_TRI_023_2_1_1
 * #3   dual:    50  -   x  -  50  -   x  TH_PORT_RATIO_DUAL_1_1
 * #4   dual:    40  -   x  -  40  -   x  TH_PORT_RATIO_DUAL_1_1
 * #5   dual:    20  -   x  -  20  -   x  TH_PORT_RATIO_DUAL_1_1
 * #6   dual:    40  -   x  -  20  -   x  TH_PORT_RATIO_DUAL_2_1
 * #7   dual:    20  -   x  -  40  -   x  TH_PORT_RATIO_DUAL_1_2
 * #8   tri:     50  -   x  - 25/x - 25/x TH_PORT_RATIO_TRI_023_2_1_1
 * #9   tri:     20  -   x  - 10/x - 10/x TH_PORT_RATIO_TRI_023_2_1_1
 * #10  tri:     40  -   x  - 10/x - 10/x TH_PORT_RATIO_TRI_023_4_1_1
 * #11  tri:    25/x - 25/x -  50  -   x  TH_PORT_RATIO_TRI_012_1_1_2
 * #12  tri:    10/x - 10/x -  20  -   x  TH_PORT_RATIO_TRI_012_1_1_2
 * #13  tri:    10/x - 10/x -  40  -   x  TH_PORT_RATIO_TRI_012_1_1_4
 * #14  quad:   25/x - 25/x - 25/x - 25/x TH_PORT_RATIO_QUAD
 * #15  quad:   10/x - 10/x - 10/x - 10/x TH_PORT_RATIO_QUAD
 */
static void
_port_ratio_get(int unit, int clport, int *mode)
{
    int port, lane;
    int num_lanes[TH_PORTS_PER_PBLK];
    int speed_max[TH_PORTS_PER_PBLK];

    port = 1 + clport * TH_PORTS_PER_PBLK;
    for (lane = 0; lane < TH_PORTS_PER_PBLK; lane += 2) {
        if (P2L(unit, port) < 0 ||
            BMD_PORT_PROPERTIES(unit, port) & BMD_PORT_FLEX) {
            num_lanes[lane] = -1;
            speed_max[lane] = -1;            
        } else {
            num_lanes[lane] = bcm56960_a0_port_lanes_get(unit, port);
            speed_max[lane] = bcm56960_a0_port_speed_max(unit, port);
        }
    }

    if (num_lanes[0] == 4) {
        *mode = PORT_RATIO_SINGLE;
    } else if (num_lanes[0] == 2 && num_lanes[2] == 2) {
        if (speed_max[0] == speed_max[2]) {
            *mode = PORT_RATIO_DUAL_1_1;
        } else if (speed_max[0] > speed_max[2]) {
            *mode = PORT_RATIO_DUAL_2_1;
        } else {
            *mode = PORT_RATIO_DUAL_1_2;
        }
    } else if (num_lanes[0] == 2) {
        if (num_lanes[2] == -1) {
            *mode = PORT_RATIO_DUAL_1_1;
        } else {
            *mode = (speed_max[0] == 20000 || speed_max[0] == 21000 ) ?
                PORT_RATIO_TRI_023_2_1_1 : PORT_RATIO_TRI_023_4_1_1;
        }
    } else if (num_lanes[2] == 2) {
        if (num_lanes[0] == -1) {
            *mode = PORT_RATIO_DUAL_1_1;
        } else {
            *mode = (speed_max[2] == 20000 || speed_max[2] == 21000 ) ?
                PORT_RATIO_TRI_012_1_1_2 : PORT_RATIO_TRI_012_1_1_4;
        }
    } else {
        *mode = PORT_RATIO_QUAD;
    }
}

/*
 * Oversubscription group speed class encoding
 * 0 - 0
 * 1 - 2 (10G)
 * 2 - 4 (20G)
 * 3 - 5 (25G)
 * 4 - 8 (40G)
 * 5 - 10 (50G)
 * 6 - 20 (100G)
 */
static void
_speed_to_ovs_class_mapping(int unit, int speed, int *ovs_class)
{
    if (speed >= 40000) {
        if (speed >= 100000) {
            *ovs_class = 6;
        } else if (speed >= 50000) {
            *ovs_class = 5;
        } else {
            *ovs_class = 4;
        }
    } else {
        if (speed >= 25000) {
            *ovs_class = 3;
        } else if (speed >= 20000) {
            *ovs_class = 2;
        } else {
            *ovs_class = 1;
        }
    }
}

static void
_speed_to_slot_mapping(int unit, int speed, int *slot)
{
    if (speed >= 40000) {
        if (speed >= 100000) {
            *slot = 40;
        } else if (speed >= 50000) {
            *slot = 20;
        } else {
            *slot = 16;
        }
    } else {
        if (speed >= 25000) {
            *slot = 10;
        } else if (speed >= 20000) {
            *slot = 8;
        } else if (speed >= 10000) {
            *slot = 4;
        } else {
            *slot = 1;
        }
    }
}

static int
_tdm_idb_calendar_set(int unit,  int calendar_id, _tdm_config_t *tcfg)
{
    int ioerr = 0;
    uint32_t pipe_map;
    int pipe, slot, id, length;
    int port, idb_port, fval;
    IS_TDM_CONFIGr_t is_tdm_cfg;
    IS_TDM_CALENDAR0m_t is_tdm_cldr0;
    IS_TDM_CALENDAR1m_t is_tdm_cldr1;

    bcm56960_a0_pipe_map_get(unit, &pipe_map);

    for (pipe = 0; pipe < TH_PIPES_PER_DEV; pipe++) {
        if (!(pipe_map & (1 << pipe))) {
            continue;
        }

        for (length = TDM_LENGTH; length > 0; length--) {
            if (tcfg->idb_tdm[pipe][length - 1] != TH_NUM_EXT_PORTS) {
                break;
            }
        }

        if (calendar_id == -1) {
            ioerr += READ_IS_TDM_CONFIGr(unit, pipe, &is_tdm_cfg);
            calendar_id = IS_TDM_CONFIGr_CURR_CALf_GET(is_tdm_cfg) ^ 1;
        }

        IS_TDM_CONFIGr_CLR(is_tdm_cfg);

        if (calendar_id == 0) {
            IS_TDM_CALENDAR0m_CLR(is_tdm_cldr0);
            for (slot = 0; slot < length; slot += 2) {
                port = tcfg->idb_tdm[pipe][slot];
                id = -1;
                if (port == OVSB_TOKEN) {
                    idb_port = TH_TDM_OVERSUB_TOKEN;
                } else if (port == IDL1_TOKEN) {
                    idb_port = TH_TDM_IDL1_TOKEN;
                } else if (port == IDL2_TOKEN) {
                    idb_port = TH_TDM_IDL2_TOKEN;
                } else if (port == NULL_TOKEN) {
                    idb_port = TH_TDM_NULL_TOKEN;
                } else if (port >= TH_NUM_EXT_PORTS) {
                    idb_port = TH_TDM_UNUSED_TOKEN;
                } else {
                    idb_port = bcm56960_a0_idbport_get(unit, port);
                    if (idb_port < 0) {
                        continue;
                    }
                    id = ((port == TH_MGMT_PORT_0) || (port == TH_MGMT_PORT_1)) ? 
                         0xf : bcm56960_a0_port_serdes_get(unit, port);
                }
                IS_TDM_CALENDAR0m_PORT_NUM_EVENf_SET(is_tdm_cldr0, idb_port);
                IS_TDM_CALENDAR0m_PHY_PORT_ID_EVENf_SET(is_tdm_cldr0, id & 0xf);
                port = tcfg->idb_tdm[pipe][slot + 1];
                id = -1;
                if (port == OVSB_TOKEN) {
                    idb_port = TH_TDM_OVERSUB_TOKEN;
                } else if (port == IDL1_TOKEN) {
                    idb_port = TH_TDM_IDL1_TOKEN;
                } else if (port == IDL2_TOKEN) {
                    idb_port = TH_TDM_IDL2_TOKEN;
                } else if (port == NULL_TOKEN) {
                    idb_port = TH_TDM_NULL_TOKEN;
                } else if (port >= TH_NUM_EXT_PORTS) {
                    idb_port = TH_TDM_UNUSED_TOKEN;
                } else {
                    idb_port = bcm56960_a0_idbport_get(unit, port);
                    if (idb_port < 0) {
                        continue;
                    }
                    id = ((port == TH_MGMT_PORT_0) || (port == TH_MGMT_PORT_1)) ? 
                         0xf : bcm56960_a0_port_serdes_get(unit, port);
                }
                IS_TDM_CALENDAR0m_PORT_NUM_ODDf_SET(is_tdm_cldr0, idb_port);
                IS_TDM_CALENDAR0m_PHY_PORT_ID_ODDf_SET(is_tdm_cldr0, id & 0xf);
                ioerr += WRITE_IS_TDM_CALENDAR0m(unit, pipe, slot/2, 
                                                 is_tdm_cldr0);
                if (tcfg->idb_tdm[pipe][slot + 2] == TH_NUM_EXT_PORTS) {
                    fval = (port == TH_NUM_EXT_PORTS) ? slot : slot + 1;
                    IS_TDM_CONFIGr_CAL0_ENDf_SET(is_tdm_cfg, fval);
                    break;
                }
            }
        } else if (calendar_id == 1) {
            IS_TDM_CALENDAR1m_CLR(is_tdm_cldr1);
            for (slot = 0; slot < length; slot += 2) {
                port = tcfg->idb_tdm[pipe][slot];
                id = -1;
                if (port == OVSB_TOKEN) {
                    idb_port = TH_TDM_OVERSUB_TOKEN;
                } else if (port == IDL1_TOKEN) {
                    idb_port = TH_TDM_IDL1_TOKEN;
                } else if (port == IDL2_TOKEN) {
                    idb_port = TH_TDM_IDL2_TOKEN;
                } else if (port == NULL_TOKEN) {
                    idb_port = TH_TDM_NULL_TOKEN;
                } else if (port >= TH_NUM_EXT_PORTS) {
                    idb_port = TH_TDM_UNUSED_TOKEN;
                } else {
                    idb_port = bcm56960_a0_idbport_get(unit, port);
                    if (idb_port < 0) {
                        continue;
                    }
                    id = ((port == TH_MGMT_PORT_0) || (port == TH_MGMT_PORT_1)) ? 
                         0xf : bcm56960_a0_port_serdes_get(unit, port);
                }
                IS_TDM_CALENDAR1m_PORT_NUM_EVENf_SET(is_tdm_cldr1, idb_port);
                IS_TDM_CALENDAR1m_PHY_PORT_ID_EVENf_SET(is_tdm_cldr1, id & 0xf);
                port = tcfg->idb_tdm[pipe][slot + 1];
                id = -1;
                if (port == OVSB_TOKEN) {
                    idb_port = TH_TDM_OVERSUB_TOKEN;
                } else if (port == IDL1_TOKEN) {
                    idb_port = TH_TDM_IDL1_TOKEN;
                } else if (port == IDL2_TOKEN) {
                    idb_port = TH_TDM_IDL2_TOKEN;
                } else if (port == NULL_TOKEN) {
                    idb_port = TH_TDM_NULL_TOKEN;
                } else if (port >= TH_NUM_EXT_PORTS) {
                    idb_port = TH_TDM_UNUSED_TOKEN;
                } else {
                    idb_port = bcm56960_a0_idbport_get(unit, port);
                    if (idb_port < 0) {
                        continue;
                    }
                    id = ((port == TH_MGMT_PORT_0) || (port == TH_MGMT_PORT_1)) ? 
                         0xf : bcm56960_a0_port_serdes_get(unit, port);
                }
                IS_TDM_CALENDAR1m_PORT_NUM_ODDf_SET(is_tdm_cldr1, idb_port);
                IS_TDM_CALENDAR1m_PHY_PORT_ID_ODDf_SET(is_tdm_cldr1, id & 0xf);
                ioerr += WRITE_IS_TDM_CALENDAR1m(unit, pipe, slot/2, 
                                                 is_tdm_cldr1);
                if (tcfg->idb_tdm[pipe][slot + 2] == TH_NUM_EXT_PORTS) {
                    fval = (port == TH_NUM_EXT_PORTS) ? slot : slot + 1;
                    IS_TDM_CONFIGr_CAL1_ENDf_SET(is_tdm_cfg, fval);
                    break;
                }
            }
        }
        IS_TDM_CONFIGr_CURR_CALf_SET(is_tdm_cfg, calendar_id);
        IS_TDM_CONFIGr_ENABLEf_SET(is_tdm_cfg, 1);
        ioerr += WRITE_IS_TDM_CONFIGr(unit, pipe, is_tdm_cfg);
    }
        
    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

static int
_tdm_idb_oversub_group_set(int unit, _tdm_config_t *tcfg)
{
    int ioerr = 0;
    uint32_t pipe_map, speed_max;
    int pipe, group, lane, slot, id, mode, ovs_class;
    int port, idb_port, blk_port, blk_index;
    cdk_pbmp_t pbmp;
    IS_OVR_SUB_GRP0_TBLr_t ovrsub_grp0_tbl; 
    IS_OVR_SUB_GRP1_TBLr_t ovrsub_grp1_tbl;
    IS_OVR_SUB_GRP2_TBLr_t ovrsub_grp2_tbl; 
    IS_OVR_SUB_GRP3_TBLr_t ovrsub_grp3_tbl;
    IS_OVR_SUB_GRP4_TBLr_t ovrsub_grp4_tbl;
    IS_OVR_SUB_GRP5_TBLr_t ovrsub_grp5_tbl;
    IS_OVR_SUB_GRP_CFGr_t ovrsub_grp_cfg;
    IS_PBLK0_CALENDARr_t pblk0_cldr; 
    IS_PBLK1_CALENDARr_t pblk1_cldr;
    IS_PBLK2_CALENDARr_t pblk2_cldr;
    IS_PBLK3_CALENDARr_t pblk3_cldr;
    IS_PBLK4_CALENDARr_t pblk4_cldr;
    IS_PBLK5_CALENDARr_t pblk5_cldr;
    IS_PBLK6_CALENDARr_t pblk6_cldr;
    IS_PBLK7_CALENDARr_t pblk7_cldr;

    static int pblk_slots[PORT_RATIO_COUNT][7] = {
        { 0, -1,  0,  0, -1,  0, -1 }, /* TH_PORT_RATIO_SINGLE */
        { 0, -1,  2,  0, -1,  2, -1 }, /* TH_PORT_RATIO_DUAL_1_1 */
        { 0,  0,  2,  0,  0,  2, -1 }, /* TH_PORT_RATIO_DUAL_2_1 */
        { 0,  2,  2,  0,  2,  2, -1 }, /* TH_PORT_RATIO_DUAL_1_2 */
        { 0, -1,  2,  0, -1,  3, -1 }, /* TH_PORT_RATIO_TRI_023_2_1_1 */
        { 0,  0,  2,  0,  0,  3, -1 }, /* TH_PORT_RATIO_TRI_023_4_1_1 */
        { 0, -1,  2,  1, -1,  2, -1 }, /* TH_PORT_RATIO_TRI_012_1_1_2 */
        { 0,  2,  2,  1,  2,  2, -1 }, /* TH_PORT_RATIO_TRI_012_1_1_4 */
        { 0, -1,  2,  1, -1,  3, -1 }  /* TH_PORT_RATIO_QUAD */
    };

    bcm56960_a0_pipe_map_get(unit, &pipe_map);

    for (pipe = 0; pipe < TH_PIPES_PER_DEV; pipe++) {
        if (!(pipe_map & (1 << pipe))) {
            continue;
        }

        for (group = 0; group < OVS_GROUP_COUNT; group++) {
            switch (group) {
            case 0:
                IS_OVR_SUB_GRP0_TBLr_CLR(ovrsub_grp0_tbl);
                for (slot = 0; slot < OVS_GROUP_TDM_LENGTH; slot++) {
                    port = tcfg->idb_ovs_tdm[pipe][group][slot];
                    if (port >= TH_NUM_EXT_PORTS) {
                        idb_port = 0x3f;
                        id = -1;
                    } else {
                        idb_port = bcm56960_a0_idbport_get(unit, port);
                        if (idb_port < 0) {
                            continue;
                        }
                        id = bcm56960_a0_port_serdes_get(unit, port);
                    }
                    IS_OVR_SUB_GRP0_TBLr_PHY_PORT_IDf_SET(ovrsub_grp0_tbl, 
                                                          id & 0x7);
                    IS_OVR_SUB_GRP0_TBLr_PORT_NUMf_SET(ovrsub_grp0_tbl, 
                                                       idb_port);
                    ioerr += WRITE_IS_OVR_SUB_GRP0_TBLr(unit, pipe, slot, 
                                                        ovrsub_grp0_tbl);
                }
                break;
            case 1:
                IS_OVR_SUB_GRP1_TBLr_CLR(ovrsub_grp1_tbl);
                for (slot = 0; slot < OVS_GROUP_TDM_LENGTH; slot++) {
                    port = tcfg->idb_ovs_tdm[pipe][group][slot];
                    if (port >= TH_NUM_EXT_PORTS) {
                        idb_port = 0x3f;
                        id = -1;
                    } else {
                        idb_port = bcm56960_a0_idbport_get(unit, port);
                        if (idb_port < 0) {
                            continue;
                        }
                        id = bcm56960_a0_port_serdes_get(unit, port);
                    }
                    IS_OVR_SUB_GRP1_TBLr_PHY_PORT_IDf_SET(ovrsub_grp1_tbl, 
                                                          id & 0x7);
                    IS_OVR_SUB_GRP1_TBLr_PORT_NUMf_SET(ovrsub_grp1_tbl, 
                                                       idb_port);
                    ioerr += WRITE_IS_OVR_SUB_GRP1_TBLr(unit, pipe, slot, 
                                                        ovrsub_grp1_tbl);
                }
                break;            
            case 2:
                IS_OVR_SUB_GRP2_TBLr_CLR(ovrsub_grp2_tbl);
                for (slot = 0; slot < OVS_GROUP_TDM_LENGTH; slot++) {
                    port = tcfg->idb_ovs_tdm[pipe][group][slot];
                    if (port >= TH_NUM_EXT_PORTS) {
                        idb_port = 0x3f;
                        id = -1;
                    } else {
                        idb_port = bcm56960_a0_idbport_get(unit, port);
                        if (idb_port < 0) {
                            continue;
                        }
                        id = bcm56960_a0_port_serdes_get(unit, port);
                    }
                    IS_OVR_SUB_GRP2_TBLr_PHY_PORT_IDf_SET(ovrsub_grp2_tbl, 
                                                          id & 0x7);
                    IS_OVR_SUB_GRP2_TBLr_PORT_NUMf_SET(ovrsub_grp2_tbl, 
                                                       idb_port);
                    ioerr += WRITE_IS_OVR_SUB_GRP2_TBLr(unit, pipe, slot, 
                                                        ovrsub_grp2_tbl);
                }
                break;
            case 3:
                IS_OVR_SUB_GRP3_TBLr_CLR(ovrsub_grp3_tbl);
                for (slot = 0; slot < OVS_GROUP_TDM_LENGTH; slot++) {
                    port = tcfg->idb_ovs_tdm[pipe][group][slot];
                    if (port >= TH_NUM_EXT_PORTS) {
                        idb_port = 0x3f;
                        id = -1;
                    } else {
                        idb_port = bcm56960_a0_idbport_get(unit, port);
                        if (idb_port < 0) {
                            continue;
                        }
                        id = bcm56960_a0_port_serdes_get(unit, port);
                    }
                    IS_OVR_SUB_GRP3_TBLr_PHY_PORT_IDf_SET(ovrsub_grp3_tbl, 
                                                          id & 0x7);
                    IS_OVR_SUB_GRP3_TBLr_PORT_NUMf_SET(ovrsub_grp3_tbl, 
                                                       idb_port);
                    ioerr += WRITE_IS_OVR_SUB_GRP3_TBLr(unit, pipe, slot, 
                                                        ovrsub_grp3_tbl);
                }
                break;            
            case 4:
                IS_OVR_SUB_GRP4_TBLr_CLR(ovrsub_grp4_tbl);
                for (slot = 0; slot < OVS_GROUP_TDM_LENGTH; slot++) {
                    port = tcfg->idb_ovs_tdm[pipe][group][slot];
                    if (port >= TH_NUM_EXT_PORTS) {
                        idb_port = 0x3f;
                        id = -1;
                    } else {
                        idb_port = bcm56960_a0_idbport_get(unit, port);
                        if (idb_port < 0) {
                            continue;
                        }
                        id = bcm56960_a0_port_serdes_get(unit, port);
                    }
                    IS_OVR_SUB_GRP4_TBLr_PHY_PORT_IDf_SET(ovrsub_grp4_tbl, 
                                                          id & 0x7);
                    IS_OVR_SUB_GRP4_TBLr_PORT_NUMf_SET(ovrsub_grp4_tbl, 
                                                       idb_port);
                    ioerr += WRITE_IS_OVR_SUB_GRP4_TBLr(unit, pipe, slot, 
                                                        ovrsub_grp4_tbl);
                }
                break;
            case 5:
                IS_OVR_SUB_GRP5_TBLr_CLR(ovrsub_grp5_tbl);
                for (slot = 0; slot < OVS_GROUP_TDM_LENGTH; slot++) {
                    port = tcfg->idb_ovs_tdm[pipe][group][slot];
                    if (port >= TH_NUM_EXT_PORTS) {
                        idb_port = 0x3f;
                        id = -1;
                    } else {
                        idb_port = bcm56960_a0_idbport_get(unit, port);
                        if (idb_port < 0) {
                            continue;
                        }
                        id = bcm56960_a0_port_serdes_get(unit, port);
                    }
                    IS_OVR_SUB_GRP5_TBLr_PHY_PORT_IDf_SET(ovrsub_grp5_tbl, 
                                                          id & 0x7);
                    IS_OVR_SUB_GRP5_TBLr_PORT_NUMf_SET(ovrsub_grp5_tbl, 
                                                       idb_port);
                    ioerr += WRITE_IS_OVR_SUB_GRP5_TBLr(unit, pipe, slot, 
                                                        ovrsub_grp5_tbl);
                }
                break;
            default:
                break;
            }
        }
    }

    for (pipe = 0; pipe < TH_PIPES_PER_DEV; pipe++) {
        if (!(pipe_map & (1 << pipe))) {
            continue;
        }

        for (group = 0; group < OVS_GROUP_COUNT; group++) {
            port = tcfg->idb_ovs_tdm[pipe][group][0];
            if (port >= TH_NUM_EXT_PORTS) {
                continue;
            }
            speed_max = 25000 * bcm56960_a0_port_lanes_get(unit, port);
            if (speed_max > bcm56960_a0_port_speed_max(unit, port)) {
                speed_max = bcm56960_a0_port_speed_max(unit, port);
            }
            _speed_to_ovs_class_mapping(unit, speed_max, &ovs_class);
            IS_OVR_SUB_GRP_CFGr_CLR(ovrsub_grp_cfg);
            IS_OVR_SUB_GRP_CFGr_SAME_SPACINGf_SET(ovrsub_grp_cfg, 4);
            IS_OVR_SUB_GRP_CFGr_SISTER_SPACINGf_SET(ovrsub_grp_cfg, 4);
            IS_OVR_SUB_GRP_CFGr_SPEEDf_SET(ovrsub_grp_cfg, ovs_class);
            ioerr += WRITE_IS_OVR_SUB_GRP_CFGr(unit, pipe, group, 
                                               ovrsub_grp_cfg);
        }
    }

    bcm56960_a0_clport_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        /* CLPORT block iteration */
        blk_port = bcm56960_a0_block_port_get(unit, port, BLKTYPE_CLPORT);
        if (blk_port != 0) {
            continue;
        }
        blk_index = bcm56960_a0_block_index_get(unit, port, BLKTYPE_CLPORT);
        pipe = bcm56960_a0_port_pipe_get(unit, port);

        mode = tcfg->port_ratio[blk_index];
        switch (blk_index & 0x7) {
        case 0:
            for (slot = 0; slot < 7; slot++) {
                IS_PBLK0_CALENDARr_CLR(pblk0_cldr);
                lane = pblk_slots[mode][slot];
                if (lane == -1) {
                    ioerr += WRITE_IS_PBLK0_CALENDARr(unit, pipe, slot, 
                                                      pblk0_cldr);
                    continue;
                }
                idb_port = bcm56960_a0_idbport_get(unit, port + lane);
                if (idb_port < 0) {
                    continue;
                }
                IS_PBLK0_CALENDARr_VALIDf_SET(pblk0_cldr, 1);
                IS_PBLK0_CALENDARr_SPACINGf_SET(pblk0_cldr, 4);
                IS_PBLK0_CALENDARr_PORT_NUMf_SET(pblk0_cldr, idb_port);
                ioerr += WRITE_IS_PBLK0_CALENDARr(unit, pipe, slot, pblk0_cldr);
            }
            break;
        case 1:
            for (slot = 0; slot < 7; slot++) {
                IS_PBLK1_CALENDARr_CLR(pblk1_cldr);
                lane = pblk_slots[mode][slot];
                if (lane == -1) {
                    ioerr += WRITE_IS_PBLK1_CALENDARr(unit, pipe, slot, 
                                                      pblk1_cldr);
                    continue;
                }
                idb_port = bcm56960_a0_idbport_get(unit, port + lane);
                if (idb_port < 0) {
                    continue;
                }
                IS_PBLK1_CALENDARr_VALIDf_SET(pblk1_cldr, 1);
                IS_PBLK1_CALENDARr_SPACINGf_SET(pblk1_cldr, 4);
                IS_PBLK1_CALENDARr_PORT_NUMf_SET(pblk1_cldr, idb_port);
                ioerr += WRITE_IS_PBLK1_CALENDARr(unit, pipe, slot, pblk1_cldr);
            }
            break;        
        case 2:
            for (slot = 0; slot < 7; slot++) {
                IS_PBLK2_CALENDARr_CLR(pblk2_cldr);
                lane = pblk_slots[mode][slot];
                if (lane == -1) {
                    ioerr += WRITE_IS_PBLK2_CALENDARr(unit, pipe, slot, 
                                                      pblk2_cldr);
                    continue;
                }
                idb_port = bcm56960_a0_idbport_get(unit, port + lane);
                if (idb_port < 0) {
                    continue;
                }
                IS_PBLK2_CALENDARr_VALIDf_SET(pblk2_cldr, 1);
                IS_PBLK2_CALENDARr_SPACINGf_SET(pblk2_cldr, 4);
                IS_PBLK2_CALENDARr_PORT_NUMf_SET(pblk2_cldr, idb_port);
                ioerr += WRITE_IS_PBLK2_CALENDARr(unit, pipe, slot, pblk2_cldr);
            }
            break;        
        case 3:
            for (slot = 0; slot < 7; slot++) {
                IS_PBLK3_CALENDARr_CLR(pblk3_cldr);
                lane = pblk_slots[mode][slot];
                if (lane == -1) {
                    ioerr += WRITE_IS_PBLK3_CALENDARr(unit, pipe, slot, 
                                                      pblk3_cldr);
                    continue;
                }
                idb_port = bcm56960_a0_idbport_get(unit, port + lane);
                if (idb_port < 0) {
                    continue;
                }
                IS_PBLK3_CALENDARr_VALIDf_SET(pblk3_cldr, 1);
                IS_PBLK3_CALENDARr_SPACINGf_SET(pblk3_cldr, 4);
                IS_PBLK3_CALENDARr_PORT_NUMf_SET(pblk3_cldr, idb_port);
                ioerr += WRITE_IS_PBLK3_CALENDARr(unit, pipe, slot, pblk3_cldr);
            }
            break;        
        case 4:
            for (slot = 0; slot < 7; slot++) {
                IS_PBLK4_CALENDARr_CLR(pblk4_cldr);
                lane = pblk_slots[mode][slot];
                if (lane == -1) {
                    ioerr += WRITE_IS_PBLK4_CALENDARr(unit, pipe, slot, 
                                                      pblk4_cldr);
                    continue;
                }
                idb_port = bcm56960_a0_idbport_get(unit, port + lane);
                if (idb_port < 0) {
                    continue;
                }
                IS_PBLK4_CALENDARr_VALIDf_SET(pblk4_cldr, 1);
                IS_PBLK4_CALENDARr_SPACINGf_SET(pblk4_cldr, 4);
                IS_PBLK4_CALENDARr_PORT_NUMf_SET(pblk4_cldr, idb_port);
                ioerr += WRITE_IS_PBLK4_CALENDARr(unit, pipe, slot, pblk4_cldr);
            }
            break;        
        case 5:
            for (slot = 0; slot < 7; slot++) {
                IS_PBLK5_CALENDARr_CLR(pblk5_cldr);
                lane = pblk_slots[mode][slot];
                if (lane == -1) {
                    ioerr += WRITE_IS_PBLK5_CALENDARr(unit, pipe, slot, 
                                                      pblk5_cldr);
                    continue;
                }
                idb_port = bcm56960_a0_idbport_get(unit, port + lane);
                if (idb_port < 0) {
                    continue;
                }
                IS_PBLK5_CALENDARr_VALIDf_SET(pblk5_cldr, 1);
                IS_PBLK5_CALENDARr_SPACINGf_SET(pblk5_cldr, 4);
                IS_PBLK5_CALENDARr_PORT_NUMf_SET(pblk5_cldr, idb_port);
                ioerr += WRITE_IS_PBLK5_CALENDARr(unit, pipe, slot, pblk5_cldr);
            }
            break;        
        case 6:
            for (slot = 0; slot < 7; slot++) {
                IS_PBLK6_CALENDARr_CLR(pblk6_cldr);
                lane = pblk_slots[mode][slot];
                if (lane == -1) {
                    ioerr += WRITE_IS_PBLK6_CALENDARr(unit, pipe, slot, 
                                                      pblk6_cldr);
                    continue;
                }
                idb_port = bcm56960_a0_idbport_get(unit, port + lane);
                if (idb_port < 0) {
                    continue;
                }
                IS_PBLK6_CALENDARr_VALIDf_SET(pblk6_cldr, 1);
                IS_PBLK6_CALENDARr_SPACINGf_SET(pblk6_cldr, 4);
                IS_PBLK6_CALENDARr_PORT_NUMf_SET(pblk6_cldr, idb_port);
                ioerr += WRITE_IS_PBLK6_CALENDARr(unit, pipe, slot, pblk6_cldr);
            }
            break;        
        case 7:
            for (slot = 0; slot < 7; slot++) {
                IS_PBLK7_CALENDARr_CLR(pblk7_cldr);
                lane = pblk_slots[mode][slot];
                if (lane == -1) {
                    ioerr += WRITE_IS_PBLK7_CALENDARr(unit, pipe, slot, 
                                                      pblk7_cldr);
                    continue;
                }
                idb_port = bcm56960_a0_idbport_get(unit, port + lane);
                if (idb_port < 0) {
                    continue;
                }
                IS_PBLK7_CALENDARr_VALIDf_SET(pblk7_cldr, 1);
                IS_PBLK7_CALENDARr_SPACINGf_SET(pblk7_cldr, 4);
                IS_PBLK7_CALENDARr_PORT_NUMf_SET(pblk7_cldr, idb_port);
                ioerr += WRITE_IS_PBLK7_CALENDARr(unit, pipe, slot, pblk7_cldr);
            }
            break;        
        }
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

static int
_tdm_idb_opportunistic_set(int unit, int enable)
{
    int rv, ioerr = 0;
    uint32_t pipe_map;
    int pipe;
    IS_CPU_LB_OPP_CFGr_t lb_opp_cfg;
    IS_OPP_SCHED_CFGr_t opp_sched_cfg;

    rv = bcm56960_a0_pipe_map_get(unit, &pipe_map);

    for (pipe = 0; pipe < TH_PIPES_PER_DEV; pipe++) {
        if (!(pipe_map & (1 << pipe))) {
            continue;
        }

        ioerr += READ_IS_CPU_LB_OPP_CFGr(unit, pipe, &lb_opp_cfg);
        IS_CPU_LB_OPP_CFGr_CPU_OPP_ENf_SET(lb_opp_cfg, enable ? 1 : 0);
        IS_CPU_LB_OPP_CFGr_LB_OPP_ENf_SET(lb_opp_cfg, enable ? 1 : 0);
        ioerr += WRITE_IS_CPU_LB_OPP_CFGr(unit, pipe, lb_opp_cfg);

        ioerr += READ_IS_OPP_SCHED_CFGr(unit, pipe, &opp_sched_cfg);
        IS_OPP_SCHED_CFGr_OPP1_PORT_ENf_SET(opp_sched_cfg, enable ? 1 : 0);
        IS_OPP_SCHED_CFGr_OPP2_PORT_ENf_SET(opp_sched_cfg, enable ? 1 : 0);
        IS_OPP_SCHED_CFGr_OPP_OVR_SUB_ENf_SET(opp_sched_cfg, enable ? 1 : 0);
        IS_OPP_SCHED_CFGr_DISABLE_PORT_NUMf_SET(opp_sched_cfg, 35);
        ioerr += WRITE_IS_OPP_SCHED_CFGr(unit, pipe, opp_sched_cfg);
    }

    return ioerr ? CDK_E_IO : rv; 
}

static int
_tdm_idb_hsp_set(int unit, int all_pipes)
{
    int ioerr = 0;
    uint32_t pipe_map;
    int pipe;
    int port, idb_port;
    uint32_t port_map[TH_PIPES_PER_DEV];
    cdk_pbmp_t pbmp;
    IS_TDM_HSPr_t is_tdm_hsp;

    bcm56960_a0_pipe_map_get(unit, &pipe_map);

    for (pipe = 0; pipe < TH_PIPES_PER_DEV; pipe++) {
        port_map[pipe] = 0;
    }

    bcm56960_a0_eq_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        pipe = bcm56960_a0_port_pipe_get(unit, port);
        idb_port = bcm56960_a0_idbport_get(unit, port);
        if (pipe >= 0 && idb_port >= 0) {
            port_map[pipe] |= 1 << idb_port;
        }
    }

    for (pipe = 0; pipe < TH_PIPES_PER_DEV; pipe++) {
        if (!(pipe_map & (1 << pipe))) {
            continue;
        }
        if ((UPDATE_ALL_PIPES == all_pipes) || (pipe == all_pipes)) {
            IS_TDM_HSPr_PORT_BMPf_SET(is_tdm_hsp, port_map[pipe]);
            ioerr += WRITE_IS_TDM_HSPr(unit, pipe, is_tdm_hsp);
        }
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

static int
_tdm_mmu_calendar_set(int unit, int calendar_id, _tdm_config_t *tcfg)
{
    int ioerr = 0;
    uint32_t pipe_map;
    int pipe, slot, id, length;
    int port, mmu_port, fval;
    TDM_CONFIGr_t tdm_cfg;
    TDM_CALENDAR0m_t tdm_cldr0;
    TDM_CALENDAR1m_t tdm_cldr1;

    bcm56960_a0_pipe_map_get(unit, &pipe_map);

    for (pipe = 0; pipe < TH_PIPES_PER_DEV; pipe++) {
        if (!(pipe_map & (1 << pipe))) {
            continue;
        }

        for (length = TDM_LENGTH; length > 0; length--) {
            if (tcfg->mmu_tdm[pipe][length - 1] != TH_NUM_EXT_PORTS) {
                break;
            }
        }

        if (calendar_id == -1) { /* choose "the other one" */
            ioerr += READ_TDM_CONFIGr(unit, pipe, &tdm_cfg);
            calendar_id = TDM_CONFIGr_CURR_CALf_GET(tdm_cfg) ^ 1;
        }

        TDM_CONFIGr_CLR(tdm_cfg);

        if (calendar_id == 0) {
            TDM_CALENDAR0m_CLR(tdm_cldr0);
            for (slot = 0; slot < length; slot += 2) {
                port = tcfg->mmu_tdm[pipe][slot];
                id = -1;
                if (port == OVSB_TOKEN) {
                    mmu_port = TH_TDM_OVERSUB_TOKEN;
                } else if (port == IDL1_TOKEN) {
                    mmu_port = TH_TDM_IDL1_TOKEN;
                } else if (port == IDL2_TOKEN) {
                    mmu_port = TH_TDM_IDL2_TOKEN;
                } else if (port == NULL_TOKEN) {
                    mmu_port = TH_TDM_NULL_TOKEN;
                } else if (port >= TH_NUM_EXT_PORTS) {
                    mmu_port = TH_TDM_UNUSED_TOKEN;
                } else {
                    mmu_port = P2M(unit, port);
                    if (mmu_port < 0) {
                        continue;
                    }
                    id = ((port == TH_MGMT_PORT_0) || (port == TH_MGMT_PORT_1)) ? 
                         0xf : bcm56960_a0_port_serdes_get(unit, port);
                }
                TDM_CALENDAR0m_PORT_NUM_EVENf_SET(tdm_cldr0, mmu_port & 0x3f);
                TDM_CALENDAR0m_PHY_PORT_ID_EVENf_SET(tdm_cldr0, id & 0xf);
                port = tcfg->mmu_tdm[pipe][slot + 1];
                id = -1;
                if (port == OVSB_TOKEN) {
                    mmu_port = TH_TDM_OVERSUB_TOKEN;
                } else if (port == IDL1_TOKEN) {
                    mmu_port = TH_TDM_IDL1_TOKEN;
                } else if (port == IDL2_TOKEN) {
                    mmu_port = TH_TDM_IDL2_TOKEN;
                } else if (port == NULL_TOKEN) {
                    mmu_port = TH_TDM_NULL_TOKEN;
                } else if (port >= TH_NUM_EXT_PORTS) {
                    mmu_port = TH_TDM_UNUSED_TOKEN;
                } else {
                    mmu_port = P2M(unit, port);
                    id = ((port == TH_MGMT_PORT_0) || (port == TH_MGMT_PORT_1)) ? 
                         0xf : bcm56960_a0_port_serdes_get(unit, port);
                }
                TDM_CALENDAR0m_PORT_NUM_ODDf_SET(tdm_cldr0, mmu_port & 0x3f);
                TDM_CALENDAR0m_PHY_PORT_ID_ODDf_SET(tdm_cldr0, id & 0xf);
                ioerr += WRITE_TDM_CALENDAR0m(unit, pipe, slot/2, tdm_cldr0);

                if (tcfg->mmu_tdm[pipe][slot + 2] == TH_NUM_EXT_PORTS) {
                    fval = (port == TH_NUM_EXT_PORTS) ? slot : slot + 1;
                    TDM_CONFIGr_CAL0_ENDf_SET(tdm_cfg, fval);
                    break;
                }                
            }
        } else if (calendar_id == 1) {
            TDM_CALENDAR1m_CLR(tdm_cldr1);
            for (slot = 0; slot < length; slot += 2) {
                port = tcfg->mmu_tdm[pipe][slot];
                id = -1;
                if (port == OVSB_TOKEN) {
                    mmu_port = TH_TDM_OVERSUB_TOKEN;
                } else if (port == IDL1_TOKEN) {
                    mmu_port = TH_TDM_IDL1_TOKEN;
                } else if (port == IDL2_TOKEN) {
                    mmu_port = TH_TDM_IDL2_TOKEN;
                } else if (port == NULL_TOKEN) {
                    mmu_port = TH_TDM_NULL_TOKEN;
                } else if (port >= TH_NUM_EXT_PORTS) {
                    mmu_port = TH_TDM_UNUSED_TOKEN;
                } else {
                    mmu_port = P2M(unit, port);
                    if (mmu_port < 0) {
                        continue;
                    }
                    id = ((port == TH_MGMT_PORT_0) || (port == TH_MGMT_PORT_1)) ? 
                         0xf : bcm56960_a0_port_serdes_get(unit, port);
                }
                TDM_CALENDAR1m_PORT_NUM_EVENf_SET(tdm_cldr1, mmu_port & 0x3f);
                TDM_CALENDAR1m_PHY_PORT_ID_EVENf_SET(tdm_cldr1, id & 0xf);
                port = tcfg->mmu_tdm[pipe][slot + 1];
                id = -1;
                if (port == OVSB_TOKEN) {
                    mmu_port = TH_TDM_OVERSUB_TOKEN;
                } else if (port == IDL1_TOKEN) {
                    mmu_port = TH_TDM_IDL1_TOKEN;
                } else if (port == IDL2_TOKEN) {
                    mmu_port = TH_TDM_IDL2_TOKEN;
                } else if (port == NULL_TOKEN) {
                    mmu_port = TH_TDM_NULL_TOKEN;
                } else if (port >= TH_NUM_EXT_PORTS) {
                    mmu_port = TH_TDM_UNUSED_TOKEN;
                } else {
                    mmu_port = P2M(unit, port);
                    id = ((port == TH_MGMT_PORT_0) || (port == TH_MGMT_PORT_1)) ? 
                         0xf : bcm56960_a0_port_serdes_get(unit, port);
                }
                TDM_CALENDAR1m_PORT_NUM_ODDf_SET(tdm_cldr1, mmu_port & 0x3f);
                TDM_CALENDAR1m_PHY_PORT_ID_ODDf_SET(tdm_cldr1, id & 0xf);
                ioerr += WRITE_TDM_CALENDAR1m(unit, pipe, slot/2, tdm_cldr1);

                if (tcfg->mmu_tdm[pipe][slot + 2] == TH_NUM_EXT_PORTS) {
                    fval = (port == TH_NUM_EXT_PORTS) ? slot : slot + 1;
                    TDM_CONFIGr_CAL1_ENDf_SET(tdm_cfg, fval);
                    break;
                }                
            }
        }
        TDM_CONFIGr_CURR_CALf_SET(tdm_cfg, calendar_id);
        TDM_CONFIGr_ENABLEf_SET(tdm_cfg, 1);
        ioerr += WRITE_TDM_CONFIGr(unit, pipe, tdm_cfg);
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

static int
_tdm_mmu_oversub_group_set(int unit, _tdm_config_t *tcfg)
{
    int ioerr = 0;
    uint32_t pipe_map, speed_max;
    int pipe, group, lane, slot, id, mode, ovs_class;
    int port, mmu_port, blk_port, blk_index;
    cdk_pbmp_t pbmp;
    OVR_SUB_GRP0_TBLr_t grp0_tbl;
    OVR_SUB_GRP1_TBLr_t grp1_tbl;
    OVR_SUB_GRP2_TBLr_t grp2_tbl;
    OVR_SUB_GRP3_TBLr_t grp3_tbl;
    OVR_SUB_GRP4_TBLr_t grp4_tbl;
    OVR_SUB_GRP5_TBLr_t grp5_tbl;
    OVR_SUB_GRP_CFGr_t grp_cfg;
    PBLK0_CALENDARr_t pblk0_cldr;
    PBLK1_CALENDARr_t pblk1_cldr; 
    PBLK2_CALENDARr_t pblk2_cldr;
    PBLK3_CALENDARr_t pblk3_cldr;
    PBLK4_CALENDARr_t pblk4_cldr;
    PBLK5_CALENDARr_t pblk5_cldr;
    PBLK6_CALENDARr_t pblk6_cldr;
    PBLK7_CALENDARr_t pblk7_cldr;

    static const int pblk_slots[PORT_RATIO_COUNT][7] = {
        { 0, -1,  0,  0, -1,  0, -1 }, /* TH_PORT_RATIO_SINGLE */
        { 0, -1,  2,  0, -1,  2, -1 }, /* TH_PORT_RATIO_DUAL_1_1 */
        { 0,  0,  2,  0,  0,  2, -1 }, /* TH_PORT_RATIO_DUAL_2_1 */
        { 0,  2,  2,  0,  2,  2, -1 }, /* TH_PORT_RATIO_DUAL_1_2 */
        { 0, -1,  2,  0, -1,  3, -1 }, /* TH_PORT_RATIO_TRI_023_2_1_1 */
        { 0,  0,  2,  0,  0,  3, -1 }, /* TH_PORT_RATIO_TRI_023_4_1_1 */
        { 0, -1,  2,  1, -1,  2, -1 }, /* TH_PORT_RATIO_TRI_012_1_1_2 */
        { 0,  2,  2,  1,  2,  2, -1 }, /* TH_PORT_RATIO_TRI_012_1_1_4 */
        { 0, -1,  2,  1, -1,  3, -1 }  /* TH_PORT_RATIO_QUAD */
    };

    bcm56960_a0_pipe_map_get(unit, &pipe_map);

    for (pipe = 0; pipe < TH_PIPES_PER_DEV; pipe++) {
        if (!(pipe_map & (1 << pipe))) {
            continue;
        }

        for (group = 0; group < OVS_GROUP_COUNT; group++) {
            switch (group) {
            case 0:
                OVR_SUB_GRP0_TBLr_CLR(grp0_tbl);
                for (slot = 0; slot < OVS_GROUP_TDM_LENGTH; slot++) {
                    port = tcfg->mmu_ovs_tdm[pipe][group][slot];
                    if (port >= TH_NUM_EXT_PORTS) {
                        mmu_port = 0x3f;
                        id = -1;
                    } else {
                        mmu_port = P2M(unit, port);
                        id = bcm56960_a0_port_serdes_get(unit, port);
                    }
                    OVR_SUB_GRP0_TBLr_PHY_PORT_IDf_SET(grp0_tbl, id & 0x7);
                    OVR_SUB_GRP0_TBLr_PORT_NUMf_SET(grp0_tbl, mmu_port & 0x3f);
                    ioerr += WRITE_OVR_SUB_GRP0_TBLr(unit, pipe, slot, 
                                                     grp0_tbl);
                }                
                break;
            case 1:
                OVR_SUB_GRP1_TBLr_CLR(grp1_tbl);
                for (slot = 0; slot < OVS_GROUP_TDM_LENGTH; slot++) {
                    port = tcfg->mmu_ovs_tdm[pipe][group][slot];
                    if (port >= TH_NUM_EXT_PORTS) {
                        mmu_port = 0x3f;
                        id = -1;
                    } else {
                        mmu_port = P2M(unit, port);
                        id = bcm56960_a0_port_serdes_get(unit, port);
                    }
                    OVR_SUB_GRP1_TBLr_PHY_PORT_IDf_SET(grp1_tbl, id & 0x7);
                    OVR_SUB_GRP1_TBLr_PORT_NUMf_SET(grp1_tbl, mmu_port & 0x3f);
                    ioerr += WRITE_OVR_SUB_GRP1_TBLr(unit, pipe, slot, 
                                                     grp1_tbl);
                }                
                break;
            case 2:
                OVR_SUB_GRP2_TBLr_CLR(grp2_tbl);
                for (slot = 0; slot < OVS_GROUP_TDM_LENGTH; slot++) {
                    port = tcfg->mmu_ovs_tdm[pipe][group][slot];
                    if (port >= TH_NUM_EXT_PORTS) {
                        mmu_port = 0x3f;
                        id = -1;
                    } else {
                        mmu_port = P2M(unit, port);
                        id = bcm56960_a0_port_serdes_get(unit, port);
                    }
                    OVR_SUB_GRP2_TBLr_PHY_PORT_IDf_SET(grp2_tbl, id & 0x7);
                    OVR_SUB_GRP2_TBLr_PORT_NUMf_SET(grp2_tbl, mmu_port & 0x3f);
                    ioerr += WRITE_OVR_SUB_GRP2_TBLr(unit, pipe, slot, 
                                                     grp2_tbl);
                }                
                break;
            case 3:
                OVR_SUB_GRP3_TBLr_CLR(grp3_tbl);
                for (slot = 0; slot < OVS_GROUP_TDM_LENGTH; slot++) {
                    port = tcfg->mmu_ovs_tdm[pipe][group][slot];
                    if (port >= TH_NUM_EXT_PORTS) {
                        mmu_port = 0x3f;
                        id = -1;
                    } else {
                        mmu_port = P2M(unit, port);
                        id = bcm56960_a0_port_serdes_get(unit, port);
                    }
                    OVR_SUB_GRP3_TBLr_PHY_PORT_IDf_SET(grp3_tbl, id & 0x7);
                    OVR_SUB_GRP3_TBLr_PORT_NUMf_SET(grp3_tbl, mmu_port & 0x3f);
                    ioerr += WRITE_OVR_SUB_GRP3_TBLr(unit, pipe, slot, 
                                                     grp3_tbl);
                }                
                break;
            case 4:
                OVR_SUB_GRP4_TBLr_CLR(grp4_tbl);
                for (slot = 0; slot < OVS_GROUP_TDM_LENGTH; slot++) {
                    port = tcfg->mmu_ovs_tdm[pipe][group][slot];
                    if (port >= TH_NUM_EXT_PORTS) {
                        mmu_port = 0x3f;
                        id = -1;
                    } else {
                        mmu_port = P2M(unit, port);
                        id = bcm56960_a0_port_serdes_get(unit, port);
                    }
                    OVR_SUB_GRP4_TBLr_PHY_PORT_IDf_SET(grp4_tbl, id & 0x7);
                    OVR_SUB_GRP4_TBLr_PORT_NUMf_SET(grp4_tbl, mmu_port & 0x3f);
                    ioerr += WRITE_OVR_SUB_GRP4_TBLr(unit, pipe, slot, 
                                                     grp4_tbl);
                }                
                break;
            case 5:
                OVR_SUB_GRP5_TBLr_CLR(grp5_tbl);
                for (slot = 0; slot < OVS_GROUP_TDM_LENGTH; slot++) {
                    port = tcfg->mmu_ovs_tdm[pipe][group][slot];
                    if (port >= TH_NUM_EXT_PORTS) {
                        mmu_port = 0x3f;
                        id = -1;
                    } else {
                        mmu_port = P2M(unit, port);
                        id = bcm56960_a0_port_serdes_get(unit, port);
                    }
                    OVR_SUB_GRP5_TBLr_PHY_PORT_IDf_SET(grp5_tbl, id & 0x7);
                    OVR_SUB_GRP5_TBLr_PORT_NUMf_SET(grp5_tbl, mmu_port & 0x3f);
                    ioerr += WRITE_OVR_SUB_GRP5_TBLr(unit, pipe, slot, 
                                                     grp5_tbl);
                }                
                break;
            default:
                break;
            }
        }
    }

    for (pipe = 0; pipe < TH_PIPES_PER_DEV; pipe++) {
        if (!(pipe_map & (1 << pipe))) {
            continue;
        }

        for (group = 0; group < OVS_GROUP_COUNT; group++) {
            port = tcfg->mmu_ovs_tdm[pipe][group][0];
            if (port >= TH_NUM_EXT_PORTS) {
                continue;
            }
            speed_max = 25000 * bcm56960_a0_port_lanes_get(unit, port);
            if (speed_max > bcm56960_a0_port_speed_max(unit, port)) {
                speed_max = bcm56960_a0_port_speed_max(unit, port);
            }
            _speed_to_ovs_class_mapping(unit, speed_max, &ovs_class);
            OVR_SUB_GRP_CFGr_CLR(grp_cfg);
            OVR_SUB_GRP_CFGr_SAME_SPACINGf_SET(grp_cfg, 
                                               speed_max >= 40000 ? 4 : 8);
            OVR_SUB_GRP_CFGr_SISTER_SPACINGf_SET(grp_cfg, 4);
            OVR_SUB_GRP_CFGr_SPEEDf_SET(grp_cfg, ovs_class);
            ioerr += WRITE_OVR_SUB_GRP_CFGr(unit, pipe, group, grp_cfg);
        }
    }

    bcm56960_a0_clport_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        /* CLPORT block iteration */
        blk_port = bcm56960_a0_block_port_get(unit, port, BLKTYPE_CLPORT);
        if (blk_port != 0) {
            continue;
        }
        blk_index = bcm56960_a0_block_index_get(unit, port, BLKTYPE_CLPORT);
        pipe = bcm56960_a0_port_pipe_get(unit, port);

        speed_max = 25000 * bcm56960_a0_port_lanes_get(unit, port);
        if (speed_max > bcm56960_a0_port_speed_max(unit, port)) {
            speed_max = bcm56960_a0_port_speed_max(unit, port);
        }

        mode = tcfg->port_ratio[blk_index];
        
        switch (blk_index & 0x7) {
        case 0:
            for (slot = 0; slot < 7; slot++) {
                PBLK0_CALENDARr_CLR(pblk0_cldr);
                lane = pblk_slots[mode][slot];
                if (lane == -1) {
                    ioerr += WRITE_PBLK0_CALENDARr(unit, pipe, slot, 
                                                   pblk0_cldr);
                    continue;
                }
                mmu_port = P2M(unit, port + lane);
                PBLK0_CALENDARr_VALIDf_SET(pblk0_cldr, 1);
                PBLK0_CALENDARr_SPACINGf_SET(pblk0_cldr, 
                                             speed_max >= 40000 ? 4 : 8);
                PBLK0_CALENDARr_PORT_NUMf_SET(pblk0_cldr, mmu_port & 0x3f);
                ioerr += WRITE_PBLK0_CALENDARr(unit, pipe, slot, pblk0_cldr);
            }
            break;
        case 1:
            for (slot = 0; slot < 7; slot++) {
                PBLK1_CALENDARr_CLR(pblk1_cldr);
                lane = pblk_slots[mode][slot];
                if (lane == -1) {
                    ioerr += WRITE_PBLK1_CALENDARr(unit, pipe, slot, 
                                                   pblk1_cldr);
                    continue;
                }
                mmu_port = P2M(unit, port + lane);
                PBLK1_CALENDARr_VALIDf_SET(pblk1_cldr, 1);
                PBLK1_CALENDARr_SPACINGf_SET(pblk1_cldr, 
                                             speed_max >= 40000 ? 4 : 8);
                PBLK1_CALENDARr_PORT_NUMf_SET(pblk1_cldr, mmu_port & 0x3f);
                ioerr += WRITE_PBLK1_CALENDARr(unit, pipe, slot, pblk1_cldr);
            }
            break;
        case 2:
            for (slot = 0; slot < 7; slot++) {
                PBLK2_CALENDARr_CLR(pblk2_cldr);
                lane = pblk_slots[mode][slot];
                if (lane == -1) {
                    ioerr += WRITE_PBLK2_CALENDARr(unit, pipe, slot, 
                                                   pblk2_cldr);
                    continue;
                }
                mmu_port = P2M(unit, port + lane);
                PBLK2_CALENDARr_VALIDf_SET(pblk2_cldr, 1);
                PBLK2_CALENDARr_SPACINGf_SET(pblk2_cldr, 
                                             speed_max >= 40000 ? 4 : 8);
                PBLK2_CALENDARr_PORT_NUMf_SET(pblk2_cldr, mmu_port & 0x3f);
                ioerr += WRITE_PBLK2_CALENDARr(unit, pipe, slot, pblk2_cldr);
            }
            break;
        case 3:
            for (slot = 0; slot < 7; slot++) {
                PBLK3_CALENDARr_CLR(pblk3_cldr);
                lane = pblk_slots[mode][slot];
                if (lane == -1) {
                    ioerr += WRITE_PBLK3_CALENDARr(unit, pipe, slot, 
                                                   pblk3_cldr);
                    continue;
                }
                mmu_port = P2M(unit, port + lane);
                PBLK3_CALENDARr_VALIDf_SET(pblk3_cldr, 1);
                PBLK3_CALENDARr_SPACINGf_SET(pblk3_cldr, 
                                             speed_max >= 40000 ? 4 : 8);
                PBLK3_CALENDARr_PORT_NUMf_SET(pblk3_cldr, mmu_port & 0x3f);
                ioerr += WRITE_PBLK3_CALENDARr(unit, pipe, slot, pblk3_cldr);
            }
            break;            
        case 4:
            for (slot = 0; slot < 7; slot++) {
                PBLK4_CALENDARr_CLR(pblk4_cldr);
                lane = pblk_slots[mode][slot];
                if (lane == -1) {
                    ioerr += WRITE_PBLK4_CALENDARr(unit, pipe, slot, 
                                                   pblk4_cldr);
                    continue;
                }
                mmu_port = P2M(unit, port + lane);
                PBLK4_CALENDARr_VALIDf_SET(pblk4_cldr, 1);
                PBLK4_CALENDARr_SPACINGf_SET(pblk4_cldr, 
                                             speed_max >= 40000 ? 4 : 8);
                PBLK4_CALENDARr_PORT_NUMf_SET(pblk4_cldr, mmu_port & 0x3f);
                ioerr += WRITE_PBLK4_CALENDARr(unit, pipe, slot, pblk4_cldr);
            }
            break;
        case 5:
            for (slot = 0; slot < 7; slot++) {
                PBLK5_CALENDARr_CLR(pblk5_cldr);
                lane = pblk_slots[mode][slot];
                if (lane == -1) {
                    ioerr += WRITE_PBLK5_CALENDARr(unit, pipe, slot, 
                                                   pblk5_cldr);
                    continue;
                }
                mmu_port = P2M(unit, port + lane);
                PBLK5_CALENDARr_VALIDf_SET(pblk5_cldr, 1);
                PBLK5_CALENDARr_SPACINGf_SET(pblk5_cldr, 
                                             speed_max >= 40000 ? 4 : 8);
                PBLK5_CALENDARr_PORT_NUMf_SET(pblk5_cldr, mmu_port & 0x3f);
                ioerr += WRITE_PBLK5_CALENDARr(unit, pipe, slot, pblk5_cldr);
            }
            break;  
        case 6:
            for (slot = 0; slot < 7; slot++) {
                PBLK6_CALENDARr_CLR(pblk6_cldr);
                lane = pblk_slots[mode][slot];
                if (lane == -1) {
                    ioerr += WRITE_PBLK6_CALENDARr(unit, pipe, slot, 
                                                   pblk6_cldr);
                    continue;
                }
                mmu_port = P2M(unit, port + lane);
                PBLK6_CALENDARr_VALIDf_SET(pblk6_cldr, 1);
                PBLK6_CALENDARr_SPACINGf_SET(pblk6_cldr, 
                                             speed_max >= 40000 ? 4 : 8);
                PBLK6_CALENDARr_PORT_NUMf_SET(pblk6_cldr, mmu_port & 0x3f);
                ioerr += WRITE_PBLK6_CALENDARr(unit, pipe, slot, pblk6_cldr);
            }
            break;
        case 7:
            for (slot = 0; slot < 7; slot++) {
                PBLK7_CALENDARr_CLR(pblk7_cldr);
                lane = pblk_slots[mode][slot];
                if (lane == -1) {
                    ioerr += WRITE_PBLK7_CALENDARr(unit, pipe, slot, 
                                                   pblk7_cldr);
                    continue;
                }
                mmu_port = P2M(unit, port + lane);
                PBLK7_CALENDARr_VALIDf_SET(pblk7_cldr, 1);
                PBLK7_CALENDARr_SPACINGf_SET(pblk7_cldr, 
                                             speed_max >= 40000 ? 4 : 8);
                PBLK7_CALENDARr_PORT_NUMf_SET(pblk7_cldr, mmu_port & 0x3f);
                ioerr += WRITE_PBLK7_CALENDARr(unit, pipe, slot, pblk7_cldr);
            }
            break;            
        }
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

static int
_tdm_mmu_opportunistic_set(int unit, int enable)
{
    int ioerr = 0;
    uint32_t pipe_map;
    int pipe;
    CPU_LB_OPP_CFGr_t opp_cfg;
    OPP_SCHED_CFGr_t sched_cfg;

    bcm56960_a0_pipe_map_get(unit, &pipe_map);

    for (pipe = 0; pipe < TH_PIPES_PER_DEV; pipe++) {
        if (!(pipe_map & (1 << pipe))) {
            continue;
        }

        ioerr += READ_CPU_LB_OPP_CFGr(unit, pipe, &opp_cfg);
        CPU_LB_OPP_CFGr_CPU_OPP_ENf_SET(opp_cfg, enable ? 1 : 0);
        CPU_LB_OPP_CFGr_LB_OPP_ENf_SET(opp_cfg, enable ? 1 : 0);
        ioerr += WRITE_CPU_LB_OPP_CFGr(unit, pipe, opp_cfg);

        ioerr += READ_OPP_SCHED_CFGr(unit, pipe, &sched_cfg);
        OPP_SCHED_CFGr_OPP1_PORT_ENf_SET(sched_cfg, enable ? 1 : 0);
        OPP_SCHED_CFGr_OPP2_PORT_ENf_SET(sched_cfg, enable ? 1 : 0);
        OPP_SCHED_CFGr_OPP_OVR_SUB_ENf_SET(sched_cfg, enable ? 1 : 0);
        OPP_SCHED_CFGr_DISABLE_PORT_NUMf_SET(sched_cfg, 35);
        ioerr += WRITE_OPP_SCHED_CFGr(unit, pipe, sched_cfg);
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

static int
_tdm_mmu_hsp_set(int unit, int all_pipes)
{
    int ioerr = 0;
    uint32_t pipe_map;
    int pipe;
    int port, mmu_port;
    uint32_t port_map[TH_PIPES_PER_DEV];
    cdk_pbmp_t pbmp;
    TDM_HSPr_t tdm_hsp;

    bcm56960_a0_pipe_map_get(unit, &pipe_map);

    for (pipe = 0; pipe < TH_PIPES_PER_DEV; pipe++) {
        port_map[pipe] = 0;
    }

    bcm56960_a0_eq_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        pipe = bcm56960_a0_port_pipe_get(unit, port);
        mmu_port = P2M(unit, port);
        if (pipe >= 0 && mmu_port >= 0) {
            port_map[pipe] |= (1 << (mmu_port & 0x1f));
        }
    }

    for (pipe = 0; pipe < TH_PIPES_PER_DEV; pipe++) {
        if (!(pipe_map & (1 << pipe))) {
            continue;
        }

        if ((UPDATE_ALL_PIPES == all_pipes) || (pipe == all_pipes)) {
            TDM_HSPr_PORT_BMPf_SET(tdm_hsp, port_map[pipe]);
            ioerr += WRITE_TDM_HSPr(unit, pipe, tdm_hsp);
        }
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

static int
_idb_init(int unit, _tdm_config_t *tcfg)
{
    int ioerr = 0;
    int idx, lossless = 1, blk_port, blk_index, port, pipe, obm, lane;
    int fval, oversub_ratio_idx, num_lanes, port_ratio;
    uint32_t uval;
    cdk_pbmp_t pbmp, pbmp_oversub;
    IDB_OBM0_MAX_USAGE_SELECTr_t obm0_max_select;
    IDB_OBM0_CA_CONTROLr_t obm0_ca_ctrl;
    IDB_OBM1_CA_CONTROLr_t obm1_ca_ctrl;
    IDB_OBM2_CA_CONTROLr_t obm2_ca_ctrl;
    IDB_OBM3_CA_CONTROLr_t obm3_ca_ctrl;
    IDB_OBM4_CA_CONTROLr_t obm4_ca_ctrl; 
    IDB_OBM5_CA_CONTROLr_t obm5_ca_ctrl;
    IDB_OBM6_CA_CONTROLr_t obm6_ca_ctrl; 
    IDB_OBM7_CA_CONTROLr_t obm7_ca_ctrl;
    IDB_OBM0_CONTROLr_t obm0_ctrl; 
    IDB_OBM1_CONTROLr_t obm1_ctrl;
    IDB_OBM2_CONTROLr_t obm2_ctrl;
    IDB_OBM3_CONTROLr_t obm3_ctrl;
    IDB_OBM4_CONTROLr_t obm4_ctrl;
    IDB_OBM5_CONTROLr_t obm5_ctrl;
    IDB_OBM6_CONTROLr_t obm6_ctrl;
    IDB_OBM7_CONTROLr_t obm7_ctrl;
    IDB_OBM0_SHARED_CONFIGr_t obm0_shared_cfg;
    IDB_OBM1_SHARED_CONFIGr_t obm1_shared_cfg;
    IDB_OBM2_SHARED_CONFIGr_t obm2_shared_cfg;
    IDB_OBM3_SHARED_CONFIGr_t obm3_shared_cfg;
    IDB_OBM4_SHARED_CONFIGr_t obm4_shared_cfg;
    IDB_OBM5_SHARED_CONFIGr_t obm5_shared_cfg;
    IDB_OBM6_SHARED_CONFIGr_t obm6_shared_cfg;
    IDB_OBM7_SHARED_CONFIGr_t obm7_shared_cfg;
    IDB_OBM0_THRESHOLDr_t obm0_thresh;
    IDB_OBM1_THRESHOLDr_t obm1_thresh;
    IDB_OBM2_THRESHOLDr_t obm2_thresh;
    IDB_OBM3_THRESHOLDr_t obm3_thresh;
    IDB_OBM4_THRESHOLDr_t obm4_thresh;
    IDB_OBM5_THRESHOLDr_t obm5_thresh;
    IDB_OBM6_THRESHOLDr_t obm6_thresh;
    IDB_OBM7_THRESHOLDr_t obm7_thresh;
    IDB_OBM0_FC_THRESHOLDr_t obm0_fc_thresh;
    IDB_OBM1_FC_THRESHOLDr_t obm1_fc_thresh;
    IDB_OBM2_FC_THRESHOLDr_t obm2_fc_thresh;
    IDB_OBM3_FC_THRESHOLDr_t obm3_fc_thresh;
    IDB_OBM4_FC_THRESHOLDr_t obm4_fc_thresh;
    IDB_OBM5_FC_THRESHOLDr_t obm5_fc_thresh;
    IDB_OBM6_FC_THRESHOLDr_t obm6_fc_thresh;
    IDB_OBM7_FC_THRESHOLDr_t obm7_fc_thresh;
    IDB_OBM0_FLOW_CONTROL_CONFIGr_t obm0_flow_ctrl_cfg;
    IDB_OBM1_FLOW_CONTROL_CONFIGr_t obm1_flow_ctrl_cfg;
    IDB_OBM2_FLOW_CONTROL_CONFIGr_t obm2_flow_ctrl_cfg;
    IDB_OBM3_FLOW_CONTROL_CONFIGr_t obm3_flow_ctrl_cfg;
    IDB_OBM4_FLOW_CONTROL_CONFIGr_t obm4_flow_ctrl_cfg;
    IDB_OBM5_FLOW_CONTROL_CONFIGr_t obm5_flow_ctrl_cfg;
    IDB_OBM6_FLOW_CONTROL_CONFIGr_t obm6_flow_ctrl_cfg;
    IDB_OBM7_FLOW_CONTROL_CONFIGr_t obm7_flow_ctrl_cfg;
    IDB_OBM0_PORT_CONFIGr_t obm0_port_cfg; 
    IDB_OBM1_PORT_CONFIGr_t obm1_port_cfg;
    IDB_OBM2_PORT_CONFIGr_t obm2_port_cfg;
    IDB_OBM3_PORT_CONFIGr_t obm3_port_cfg;
    IDB_OBM4_PORT_CONFIGr_t obm4_port_cfg;
    IDB_OBM5_PORT_CONFIGr_t obm5_port_cfg;
    IDB_OBM6_PORT_CONFIGr_t obm6_port_cfg;
    IDB_OBM7_PORT_CONFIGr_t obm7_port_cfg;
    IDB_OBM0_PRI_MAP_PORT0m_t obm0_pri_map_p0;
    IDB_OBM0_PRI_MAP_PORT1m_t obm0_pri_map_p1;
    IDB_OBM0_PRI_MAP_PORT2m_t obm0_pri_map_p2;
    IDB_OBM0_PRI_MAP_PORT3m_t obm0_pri_map_p3;
    IDB_OBM1_PRI_MAP_PORT0m_t obm1_pri_map_p0; 
    IDB_OBM1_PRI_MAP_PORT1m_t obm1_pri_map_p1; 
    IDB_OBM1_PRI_MAP_PORT2m_t obm1_pri_map_p2; 
    IDB_OBM1_PRI_MAP_PORT3m_t obm1_pri_map_p3; 
    IDB_OBM2_PRI_MAP_PORT0m_t obm2_pri_map_p0;  
    IDB_OBM2_PRI_MAP_PORT1m_t obm2_pri_map_p1;  
    IDB_OBM2_PRI_MAP_PORT2m_t obm2_pri_map_p2;  
    IDB_OBM2_PRI_MAP_PORT3m_t obm2_pri_map_p3;  
    IDB_OBM3_PRI_MAP_PORT0m_t obm3_pri_map_p0;
    IDB_OBM3_PRI_MAP_PORT1m_t obm3_pri_map_p1;
    IDB_OBM3_PRI_MAP_PORT2m_t obm3_pri_map_p2;
    IDB_OBM3_PRI_MAP_PORT3m_t obm3_pri_map_p3;
    IDB_OBM4_PRI_MAP_PORT0m_t obm4_pri_map_p0; 
    IDB_OBM4_PRI_MAP_PORT1m_t obm4_pri_map_p1; 
    IDB_OBM4_PRI_MAP_PORT2m_t obm4_pri_map_p2; 
    IDB_OBM4_PRI_MAP_PORT3m_t obm4_pri_map_p3; 
    IDB_OBM5_PRI_MAP_PORT0m_t obm5_pri_map_p0; 
    IDB_OBM5_PRI_MAP_PORT1m_t obm5_pri_map_p1; 
    IDB_OBM5_PRI_MAP_PORT2m_t obm5_pri_map_p2; 
    IDB_OBM5_PRI_MAP_PORT3m_t obm5_pri_map_p3; 
    IDB_OBM6_PRI_MAP_PORT0m_t obm6_pri_map_p0;  
    IDB_OBM6_PRI_MAP_PORT1m_t obm6_pri_map_p1;  
    IDB_OBM6_PRI_MAP_PORT2m_t obm6_pri_map_p2;  
    IDB_OBM6_PRI_MAP_PORT3m_t obm6_pri_map_p3;  
    IDB_OBM7_PRI_MAP_PORT0m_t obm7_pri_map_p0;  
    IDB_OBM7_PRI_MAP_PORT1m_t obm7_pri_map_p1;
    IDB_OBM7_PRI_MAP_PORT2m_t obm7_pri_map_p2;
    IDB_OBM7_PRI_MAP_PORT3m_t obm7_pri_map_p3;
    IDB_OBM0_MAX_USAGE_SELECTr_t obm0_max_us; 
    IDB_OBM1_MAX_USAGE_SELECTr_t obm1_max_us;
    IDB_OBM2_MAX_USAGE_SELECTr_t obm2_max_us;
    IDB_OBM3_MAX_USAGE_SELECTr_t obm3_max_us;
    IDB_OBM4_MAX_USAGE_SELECTr_t obm4_max_us;
    IDB_OBM5_MAX_USAGE_SELECTr_t obm5_max_us;
    IDB_OBM6_MAX_USAGE_SELECTr_t obm6_max_us;
    IDB_OBM7_MAX_USAGE_SELECTr_t obm7_max_us;
    IDB_CA_CPU_CONTROLr_t ca_cpu_ctrl;
    IDB_CA_LPBK_CONTROLr_t ca_lpbk_ctrl;

    static const int hw_mode_values[PORT_RATIO_COUNT] = {
        0, 1, 1, 1, 4, 6, 3, 5, 2
    };

    static const struct _obm_setting_s {
        int discard_limit;
        int lossless_xoff;
        int lossy_only_lossy_low_pri;
        int port_xoff[3];          /* for oversub ratio 1.5, 1.8, and above */
        int lossy_low_pri[3];      /* for oversub ratio 1.5, 1.8, and above */
        int lossy_discard[3];      /* for oversub ratio 1.5, 1.8, and above */
    } obm_settings[] = { /* indexed by number of lanes */
        { /* 0 - invalid  */ 0 },
        { /* 1 lane */
            TH_CELLS_PER_OBM / 4, 45, 157,
            { 129, 87, 67 }, /* port_xoff */
            { 100, 76, 56 }, /* lossy_low_pri */
            { 196, 152, 112 }  /* lossy_discard */
        },
        { /* 2 lanes */
            TH_CELLS_PER_OBM / 2, 108, 410,
            { 312, 248, 215 }, /* port_xoff */
            { 100, 100, 100 }, /* lossy_low_pri */
            { 196, 196, 196 }  /* lossy_discard */
        },
        { /* 3 - invalid  */ 0 },
        { /* 4 lanes */
            TH_CELLS_PER_OBM, 258, 916,
            { 682, 572, 517 }, /* port_xoff */
            { 100, 100, 100 }, /* lossy_low_pri */
            { 196, 196, 196 }  /* lossy_discard */
        }
    };

    IDB_OBM0_MAX_USAGE_SELECTr_CLR(obm0_max_select);
    IDB_OBM0_MAX_USAGE_SELECTr_PRIORITY_SELECTf_SET(obm0_max_select, 2);

    IDB_OBM0_PRI_MAP_PORT0m_CLR(obm0_pri_map_p0);
    uval = 0;
    for (idx = 0; idx < 16; idx++) {
        uval |= (2 << (idx * 2));
    }
    IDB_OBM0_PRI_MAP_PORT0m_SET(obm0_pri_map_p0, uval);

    bcm56960_a0_clport_pbmp_get(unit, &pbmp);
    bcm56960_a0_oversub_map_get(unit, &pbmp_oversub);
    CDK_PBMP_ITER(pbmp, port) {
        /* CLPORT block iteration */
        blk_port = bcm56960_a0_block_port_get(unit, port, BLKTYPE_CLPORT);
        if (blk_port != 0) {
            continue;
        }
        blk_index = bcm56960_a0_block_index_get(unit, port, BLKTYPE_CLPORT);
        pipe = bcm56960_a0_port_pipe_get(unit, port);
        /* obm number is reversed (mirrored) in odd pipe */
        obm = (pipe & 1) ? (7 - (blk_index & 0x7)) : (blk_index & 0x7);

        /* Set cell assembly mode then toggle reset to send initial credit
         * to EP */
        port_ratio = bcm56960_a0_port_ratio_get(unit, blk_index);
        switch(obm) {
        case 0:
            IDB_OBM0_CA_CONTROLr_CLR(obm0_ca_ctrl);
            IDB_OBM0_CA_CONTROLr_PORT_MODEf_SET(obm0_ca_ctrl, 
                                                hw_mode_values[port_ratio]);
            IDB_OBM0_CA_CONTROLr_PORT0_RESETf_SET(obm0_ca_ctrl, 1);
            IDB_OBM0_CA_CONTROLr_PORT1_RESETf_SET(obm0_ca_ctrl, 1);
            IDB_OBM0_CA_CONTROLr_PORT2_RESETf_SET(obm0_ca_ctrl, 1);
            IDB_OBM0_CA_CONTROLr_PORT3_RESETf_SET(obm0_ca_ctrl, 1);
            ioerr += WRITE_IDB_OBM0_CA_CONTROLr(unit, pipe, obm0_ca_ctrl);
            IDB_OBM0_CA_CONTROLr_CLR(obm0_ca_ctrl);
            IDB_OBM0_CA_CONTROLr_PORT_MODEf_SET(obm0_ca_ctrl, 
                                                hw_mode_values[port_ratio]);
            ioerr += WRITE_IDB_OBM0_CA_CONTROLr(unit, pipe, obm0_ca_ctrl);
            break;
        case 1:
            IDB_OBM1_CA_CONTROLr_CLR(obm1_ca_ctrl);
            IDB_OBM1_CA_CONTROLr_PORT_MODEf_SET(obm1_ca_ctrl, 
                                                hw_mode_values[port_ratio]);
            IDB_OBM1_CA_CONTROLr_PORT0_RESETf_SET(obm1_ca_ctrl, 1);
            IDB_OBM1_CA_CONTROLr_PORT1_RESETf_SET(obm1_ca_ctrl, 1);
            IDB_OBM1_CA_CONTROLr_PORT2_RESETf_SET(obm1_ca_ctrl, 1);
            IDB_OBM1_CA_CONTROLr_PORT3_RESETf_SET(obm1_ca_ctrl, 1);
            ioerr += WRITE_IDB_OBM1_CA_CONTROLr(unit, pipe, obm1_ca_ctrl);
            IDB_OBM1_CA_CONTROLr_CLR(obm1_ca_ctrl);
            IDB_OBM1_CA_CONTROLr_PORT_MODEf_SET(obm1_ca_ctrl, 
                                                hw_mode_values[port_ratio]);
            ioerr += WRITE_IDB_OBM1_CA_CONTROLr(unit, pipe, obm1_ca_ctrl);
            break;
        case 2:
            IDB_OBM2_CA_CONTROLr_CLR(obm2_ca_ctrl);
            IDB_OBM2_CA_CONTROLr_PORT_MODEf_SET(obm2_ca_ctrl, 
                                                hw_mode_values[port_ratio]);
            IDB_OBM2_CA_CONTROLr_PORT0_RESETf_SET(obm2_ca_ctrl, 1);
            IDB_OBM2_CA_CONTROLr_PORT1_RESETf_SET(obm2_ca_ctrl, 1);
            IDB_OBM2_CA_CONTROLr_PORT2_RESETf_SET(obm2_ca_ctrl, 1);
            IDB_OBM2_CA_CONTROLr_PORT3_RESETf_SET(obm2_ca_ctrl, 1);
            ioerr += WRITE_IDB_OBM2_CA_CONTROLr(unit, pipe, obm2_ca_ctrl);
            IDB_OBM2_CA_CONTROLr_CLR(obm2_ca_ctrl);
            IDB_OBM2_CA_CONTROLr_PORT_MODEf_SET(obm2_ca_ctrl, 
                                                hw_mode_values[port_ratio]);
            ioerr += WRITE_IDB_OBM2_CA_CONTROLr(unit, pipe, obm2_ca_ctrl);
            break;
        case 3:
            IDB_OBM3_CA_CONTROLr_CLR(obm3_ca_ctrl);
            IDB_OBM3_CA_CONTROLr_PORT_MODEf_SET(obm3_ca_ctrl, 
                                                hw_mode_values[port_ratio]);
            IDB_OBM3_CA_CONTROLr_PORT0_RESETf_SET(obm3_ca_ctrl, 1);
            IDB_OBM3_CA_CONTROLr_PORT1_RESETf_SET(obm3_ca_ctrl, 1);
            IDB_OBM3_CA_CONTROLr_PORT2_RESETf_SET(obm3_ca_ctrl, 1);
            IDB_OBM3_CA_CONTROLr_PORT3_RESETf_SET(obm3_ca_ctrl, 1);
            ioerr += WRITE_IDB_OBM3_CA_CONTROLr(unit, pipe, obm3_ca_ctrl);
            IDB_OBM3_CA_CONTROLr_CLR(obm3_ca_ctrl);
            IDB_OBM3_CA_CONTROLr_PORT_MODEf_SET(obm3_ca_ctrl, 
                                                hw_mode_values[port_ratio]);
            ioerr += WRITE_IDB_OBM3_CA_CONTROLr(unit, pipe, obm3_ca_ctrl);
            break;
        case 4:
            IDB_OBM4_CA_CONTROLr_CLR(obm4_ca_ctrl);
            IDB_OBM4_CA_CONTROLr_PORT_MODEf_SET(obm4_ca_ctrl, 
                                                hw_mode_values[port_ratio]);
            IDB_OBM4_CA_CONTROLr_PORT0_RESETf_SET(obm4_ca_ctrl, 1);
            IDB_OBM4_CA_CONTROLr_PORT1_RESETf_SET(obm4_ca_ctrl, 1);
            IDB_OBM4_CA_CONTROLr_PORT2_RESETf_SET(obm4_ca_ctrl, 1);
            IDB_OBM4_CA_CONTROLr_PORT3_RESETf_SET(obm4_ca_ctrl, 1);
            ioerr += WRITE_IDB_OBM4_CA_CONTROLr(unit, pipe, obm4_ca_ctrl);
            IDB_OBM4_CA_CONTROLr_CLR(obm4_ca_ctrl);
            IDB_OBM4_CA_CONTROLr_PORT_MODEf_SET(obm4_ca_ctrl, 
                                                hw_mode_values[port_ratio]);
            ioerr += WRITE_IDB_OBM4_CA_CONTROLr(unit, pipe, obm4_ca_ctrl);
            break;
        case 5:
            IDB_OBM5_CA_CONTROLr_CLR(obm5_ca_ctrl);
            IDB_OBM5_CA_CONTROLr_PORT_MODEf_SET(obm5_ca_ctrl, 
                                                hw_mode_values[port_ratio]);
            IDB_OBM5_CA_CONTROLr_PORT0_RESETf_SET(obm5_ca_ctrl, 1);
            IDB_OBM5_CA_CONTROLr_PORT1_RESETf_SET(obm5_ca_ctrl, 1);
            IDB_OBM5_CA_CONTROLr_PORT2_RESETf_SET(obm5_ca_ctrl, 1);
            IDB_OBM5_CA_CONTROLr_PORT3_RESETf_SET(obm5_ca_ctrl, 1);
            ioerr += WRITE_IDB_OBM5_CA_CONTROLr(unit, pipe, obm5_ca_ctrl);
            IDB_OBM5_CA_CONTROLr_CLR(obm5_ca_ctrl);
            IDB_OBM5_CA_CONTROLr_PORT_MODEf_SET(obm5_ca_ctrl, 
                                                hw_mode_values[port_ratio]);
            ioerr += WRITE_IDB_OBM5_CA_CONTROLr(unit, pipe, obm5_ca_ctrl);
            break;
        case 6:
            IDB_OBM6_CA_CONTROLr_CLR(obm6_ca_ctrl);
            IDB_OBM6_CA_CONTROLr_PORT_MODEf_SET(obm6_ca_ctrl, 
                                                hw_mode_values[port_ratio]);
            IDB_OBM6_CA_CONTROLr_PORT0_RESETf_SET(obm6_ca_ctrl, 1);
            IDB_OBM6_CA_CONTROLr_PORT1_RESETf_SET(obm6_ca_ctrl, 1);
            IDB_OBM6_CA_CONTROLr_PORT2_RESETf_SET(obm6_ca_ctrl, 1);
            IDB_OBM6_CA_CONTROLr_PORT3_RESETf_SET(obm6_ca_ctrl, 1);
            ioerr += WRITE_IDB_OBM6_CA_CONTROLr(unit, pipe, obm6_ca_ctrl);
            IDB_OBM6_CA_CONTROLr_CLR(obm6_ca_ctrl);
            IDB_OBM6_CA_CONTROLr_PORT_MODEf_SET(obm6_ca_ctrl, 
                                                hw_mode_values[port_ratio]);
            ioerr += WRITE_IDB_OBM6_CA_CONTROLr(unit, pipe, obm6_ca_ctrl);
            break;
        case 7:
            IDB_OBM7_CA_CONTROLr_CLR(obm7_ca_ctrl);
            IDB_OBM7_CA_CONTROLr_PORT_MODEf_SET(obm7_ca_ctrl, 
                                                hw_mode_values[port_ratio]);
            IDB_OBM7_CA_CONTROLr_PORT0_RESETf_SET(obm7_ca_ctrl, 1);
            IDB_OBM7_CA_CONTROLr_PORT1_RESETf_SET(obm7_ca_ctrl, 1);
            IDB_OBM7_CA_CONTROLr_PORT2_RESETf_SET(obm7_ca_ctrl, 1);
            IDB_OBM7_CA_CONTROLr_PORT3_RESETf_SET(obm7_ca_ctrl, 1);
            ioerr += WRITE_IDB_OBM7_CA_CONTROLr(unit, pipe, obm7_ca_ctrl);
            IDB_OBM7_CA_CONTROLr_CLR(obm7_ca_ctrl);
            IDB_OBM7_CA_CONTROLr_PORT_MODEf_SET(obm7_ca_ctrl, 
                                                hw_mode_values[port_ratio]);
            ioerr += WRITE_IDB_OBM7_CA_CONTROLr(unit, pipe, obm7_ca_ctrl);
            break;
        default:
            break;
        }

        if (!CDK_PBMP_MEMBER(pbmp_oversub, port)) {
            continue;
        }

        /* Enable oversub */
        switch(obm) {
        case 0:
            ioerr += READ_IDB_OBM0_CONTROLr(unit, pipe, &obm0_ctrl);
            for (lane = 0; lane < TH_PORTS_PER_PBLK; lane++) {
                if (P2L(unit, port + lane) == -1) {
                    continue;
                }
                if (lane == 0) {
                    IDB_OBM0_CONTROLr_PORT0_BYPASS_ENABLEf_SET(obm0_ctrl, 1);
                    IDB_OBM0_CONTROLr_PORT0_OVERSUB_ENABLEf_SET(obm0_ctrl, 1);
                } else if (lane == 1) {
                    IDB_OBM0_CONTROLr_PORT1_BYPASS_ENABLEf_SET(obm0_ctrl, 1);
                    IDB_OBM0_CONTROLr_PORT1_OVERSUB_ENABLEf_SET(obm0_ctrl, 1);
                } else if (lane == 2) {
                    IDB_OBM0_CONTROLr_PORT2_BYPASS_ENABLEf_SET(obm0_ctrl, 1);
                    IDB_OBM0_CONTROLr_PORT2_OVERSUB_ENABLEf_SET(obm0_ctrl, 1);
                } else if (lane == 3) {
                    IDB_OBM0_CONTROLr_PORT3_BYPASS_ENABLEf_SET(obm0_ctrl, 1);
                    IDB_OBM0_CONTROLr_PORT3_OVERSUB_ENABLEf_SET(obm0_ctrl, 1);
                }
            }
            ioerr += WRITE_IDB_OBM0_CONTROLr(unit, pipe, obm0_ctrl);
            break;
        case 1:
            ioerr += READ_IDB_OBM1_CONTROLr(unit, pipe, &obm1_ctrl);
            for (lane = 0; lane < TH_PORTS_PER_PBLK; lane++) {
                if (P2L(unit, port + lane) == -1) {
                    continue;
                }
                if (lane == 0) {
                    IDB_OBM1_CONTROLr_PORT0_BYPASS_ENABLEf_SET(obm1_ctrl, 1);
                    IDB_OBM1_CONTROLr_PORT0_OVERSUB_ENABLEf_SET(obm1_ctrl, 1);
                } else if (lane == 1) {
                    IDB_OBM1_CONTROLr_PORT1_BYPASS_ENABLEf_SET(obm1_ctrl, 1);
                    IDB_OBM1_CONTROLr_PORT1_OVERSUB_ENABLEf_SET(obm1_ctrl, 1);
                } else if (lane == 2) {
                    IDB_OBM1_CONTROLr_PORT2_BYPASS_ENABLEf_SET(obm1_ctrl, 1);
                    IDB_OBM1_CONTROLr_PORT2_OVERSUB_ENABLEf_SET(obm1_ctrl, 1);
                } else if (lane == 3) {
                    IDB_OBM1_CONTROLr_PORT3_BYPASS_ENABLEf_SET(obm1_ctrl, 1);
                    IDB_OBM1_CONTROLr_PORT3_OVERSUB_ENABLEf_SET(obm1_ctrl, 1);
                }
            }
            ioerr += WRITE_IDB_OBM1_CONTROLr(unit, pipe, obm1_ctrl);
            break;
        case 2:
            ioerr += READ_IDB_OBM2_CONTROLr(unit, pipe, &obm2_ctrl);
            for (lane = 0; lane < TH_PORTS_PER_PBLK; lane++) {
                if (P2L(unit, port + lane) == -1) {
                    continue;
                }
                if (lane == 0) {
                    IDB_OBM2_CONTROLr_PORT0_BYPASS_ENABLEf_SET(obm2_ctrl, 1);
                    IDB_OBM2_CONTROLr_PORT0_OVERSUB_ENABLEf_SET(obm2_ctrl, 1);
                } else if (lane == 1) {
                    IDB_OBM2_CONTROLr_PORT1_BYPASS_ENABLEf_SET(obm2_ctrl, 1);
                    IDB_OBM2_CONTROLr_PORT1_OVERSUB_ENABLEf_SET(obm2_ctrl, 1);
                } else if (lane == 2) {
                    IDB_OBM2_CONTROLr_PORT2_BYPASS_ENABLEf_SET(obm2_ctrl, 1);
                    IDB_OBM2_CONTROLr_PORT2_OVERSUB_ENABLEf_SET(obm2_ctrl, 1);
                } else if (lane == 3) {
                    IDB_OBM2_CONTROLr_PORT3_BYPASS_ENABLEf_SET(obm2_ctrl, 1);
                    IDB_OBM2_CONTROLr_PORT3_OVERSUB_ENABLEf_SET(obm2_ctrl, 1);
                }
            }
            ioerr += WRITE_IDB_OBM2_CONTROLr(unit, pipe, obm2_ctrl);
            break;
        case 3:
            ioerr += READ_IDB_OBM3_CONTROLr(unit, pipe, &obm3_ctrl);
            for (lane = 0; lane < TH_PORTS_PER_PBLK; lane++) {
                if (P2L(unit, port + lane) == -1) {
                    continue;
                }
                if (lane == 0) {
                    IDB_OBM3_CONTROLr_PORT0_BYPASS_ENABLEf_SET(obm3_ctrl, 1);
                    IDB_OBM3_CONTROLr_PORT0_OVERSUB_ENABLEf_SET(obm3_ctrl, 1);
                } else if (lane == 1) {
                    IDB_OBM3_CONTROLr_PORT1_BYPASS_ENABLEf_SET(obm3_ctrl, 1);
                    IDB_OBM3_CONTROLr_PORT1_OVERSUB_ENABLEf_SET(obm3_ctrl, 1);
                } else if (lane == 2) {
                    IDB_OBM3_CONTROLr_PORT2_BYPASS_ENABLEf_SET(obm3_ctrl, 1);
                    IDB_OBM3_CONTROLr_PORT2_OVERSUB_ENABLEf_SET(obm3_ctrl, 1);
                } else if (lane == 3) {
                    IDB_OBM3_CONTROLr_PORT3_BYPASS_ENABLEf_SET(obm3_ctrl, 1);
                    IDB_OBM3_CONTROLr_PORT3_OVERSUB_ENABLEf_SET(obm3_ctrl, 1);
                }
            }
            ioerr += WRITE_IDB_OBM3_CONTROLr(unit, pipe, obm3_ctrl);
            break;
        case 4:
            ioerr += READ_IDB_OBM4_CONTROLr(unit, pipe, &obm4_ctrl);
            for (lane = 0; lane < TH_PORTS_PER_PBLK; lane++) {
                if (P2L(unit, port + lane) == -1) {
                    continue;
                }
                if (lane == 0) {
                    IDB_OBM4_CONTROLr_PORT0_BYPASS_ENABLEf_SET(obm4_ctrl, 1);
                    IDB_OBM4_CONTROLr_PORT0_OVERSUB_ENABLEf_SET(obm4_ctrl, 1);
                } else if (lane == 1) {
                    IDB_OBM4_CONTROLr_PORT1_BYPASS_ENABLEf_SET(obm4_ctrl, 1);
                    IDB_OBM4_CONTROLr_PORT1_OVERSUB_ENABLEf_SET(obm4_ctrl, 1);
                } else if (lane == 2) {
                    IDB_OBM4_CONTROLr_PORT2_BYPASS_ENABLEf_SET(obm4_ctrl, 1);
                    IDB_OBM4_CONTROLr_PORT2_OVERSUB_ENABLEf_SET(obm4_ctrl, 1);
                } else if (lane == 3) {
                    IDB_OBM4_CONTROLr_PORT3_BYPASS_ENABLEf_SET(obm4_ctrl, 1);
                    IDB_OBM4_CONTROLr_PORT3_OVERSUB_ENABLEf_SET(obm4_ctrl, 1);
                }
            }
            ioerr += WRITE_IDB_OBM4_CONTROLr(unit, pipe, obm4_ctrl);
            break;
        case 5:
            ioerr += READ_IDB_OBM5_CONTROLr(unit, pipe, &obm5_ctrl);
            for (lane = 0; lane < TH_PORTS_PER_PBLK; lane++) {
                if (P2L(unit, port + lane) == -1) {
                    continue;
                }
                if (lane == 0) {
                    IDB_OBM5_CONTROLr_PORT0_BYPASS_ENABLEf_SET(obm5_ctrl, 1);
                    IDB_OBM5_CONTROLr_PORT0_OVERSUB_ENABLEf_SET(obm5_ctrl, 1);
                } else if (lane == 1) {
                    IDB_OBM5_CONTROLr_PORT1_BYPASS_ENABLEf_SET(obm5_ctrl, 1);
                    IDB_OBM5_CONTROLr_PORT1_OVERSUB_ENABLEf_SET(obm5_ctrl, 1);
                } else if (lane == 2) {
                    IDB_OBM5_CONTROLr_PORT2_BYPASS_ENABLEf_SET(obm5_ctrl, 1);
                    IDB_OBM5_CONTROLr_PORT2_OVERSUB_ENABLEf_SET(obm5_ctrl, 1);
                } else if (lane == 3) {
                    IDB_OBM5_CONTROLr_PORT3_BYPASS_ENABLEf_SET(obm5_ctrl, 1);
                    IDB_OBM5_CONTROLr_PORT3_OVERSUB_ENABLEf_SET(obm5_ctrl, 1);
                }
            }
            ioerr += WRITE_IDB_OBM5_CONTROLr(unit, pipe, obm5_ctrl);
            break;
        case 6:
            ioerr += READ_IDB_OBM6_CONTROLr(unit, pipe, &obm6_ctrl);
            for (lane = 0; lane < TH_PORTS_PER_PBLK; lane++) {
                if (P2L(unit, port + lane) == -1) {
                    continue;
                }
                if (lane == 0) {
                    IDB_OBM6_CONTROLr_PORT0_BYPASS_ENABLEf_SET(obm6_ctrl, 1);
                    IDB_OBM6_CONTROLr_PORT0_OVERSUB_ENABLEf_SET(obm6_ctrl, 1);
                } else if (lane == 1) {
                    IDB_OBM6_CONTROLr_PORT1_BYPASS_ENABLEf_SET(obm6_ctrl, 1);
                    IDB_OBM6_CONTROLr_PORT1_OVERSUB_ENABLEf_SET(obm6_ctrl, 1);
                } else if (lane == 2) {
                    IDB_OBM6_CONTROLr_PORT2_BYPASS_ENABLEf_SET(obm6_ctrl, 1);
                    IDB_OBM6_CONTROLr_PORT2_OVERSUB_ENABLEf_SET(obm6_ctrl, 1);
                } else if (lane == 3) {
                    IDB_OBM6_CONTROLr_PORT3_BYPASS_ENABLEf_SET(obm6_ctrl, 1);
                    IDB_OBM6_CONTROLr_PORT3_OVERSUB_ENABLEf_SET(obm6_ctrl, 1);
                }
            }
            ioerr += WRITE_IDB_OBM6_CONTROLr(unit, pipe, obm6_ctrl);
            break;
        case 7:
            ioerr += READ_IDB_OBM7_CONTROLr(unit, pipe, &obm7_ctrl);
            for (lane = 0; lane < TH_PORTS_PER_PBLK; lane++) {
                if (P2L(unit, port + lane) == -1) {
                    continue;
                }
                if (lane == 0) {
                    IDB_OBM7_CONTROLr_PORT0_BYPASS_ENABLEf_SET(obm7_ctrl, 1);
                    IDB_OBM7_CONTROLr_PORT0_OVERSUB_ENABLEf_SET(obm7_ctrl, 1);
                } else if (lane == 1) {
                    IDB_OBM7_CONTROLr_PORT1_BYPASS_ENABLEf_SET(obm7_ctrl, 1);
                    IDB_OBM7_CONTROLr_PORT1_OVERSUB_ENABLEf_SET(obm7_ctrl, 1);
                } else if (lane == 2) {
                    IDB_OBM7_CONTROLr_PORT2_BYPASS_ENABLEf_SET(obm7_ctrl, 1);
                    IDB_OBM7_CONTROLr_PORT2_OVERSUB_ENABLEf_SET(obm7_ctrl, 1);
                } else if (lane == 3) {
                    IDB_OBM7_CONTROLr_PORT3_BYPASS_ENABLEf_SET(obm7_ctrl, 1);
                    IDB_OBM7_CONTROLr_PORT3_OVERSUB_ENABLEf_SET(obm7_ctrl, 1);
                }
            }
            ioerr += WRITE_IDB_OBM7_CONTROLr(unit, pipe, obm7_ctrl);
            break;
        default:
            break;
        }

        /* Configure shared config */
        switch(obm) {
        case 0:
            ioerr += READ_IDB_OBM0_SHARED_CONFIGr(unit, pipe, &obm0_shared_cfg);
            IDB_OBM0_SHARED_CONFIGr_DISCARD_THRESHOLDf_SET(obm0_shared_cfg, 
                                                           1023);
            IDB_OBM0_SHARED_CONFIGr_SOP_THRESHOLDf_SET(obm0_shared_cfg, 1023);
            IDB_OBM0_SHARED_CONFIGr_SOP_DISCARD_MODEf_SET(obm0_shared_cfg, 1);
            ioerr += WRITE_IDB_OBM0_SHARED_CONFIGr(unit, pipe, obm0_shared_cfg);
            break;
        case 1:
            ioerr += READ_IDB_OBM1_SHARED_CONFIGr(unit, pipe, &obm1_shared_cfg);
            IDB_OBM1_SHARED_CONFIGr_DISCARD_THRESHOLDf_SET(obm1_shared_cfg, 
                                                           1023);
            IDB_OBM1_SHARED_CONFIGr_SOP_THRESHOLDf_SET(obm1_shared_cfg, 1023);
            IDB_OBM1_SHARED_CONFIGr_SOP_DISCARD_MODEf_SET(obm1_shared_cfg, 1);
            ioerr += WRITE_IDB_OBM1_SHARED_CONFIGr(unit, pipe, obm1_shared_cfg);
            break;
        case 2:
            ioerr += READ_IDB_OBM2_SHARED_CONFIGr(unit, pipe, &obm2_shared_cfg);
            IDB_OBM2_SHARED_CONFIGr_DISCARD_THRESHOLDf_SET(obm2_shared_cfg, 
                                                           1023);
            IDB_OBM2_SHARED_CONFIGr_SOP_THRESHOLDf_SET(obm2_shared_cfg, 1023);
            IDB_OBM2_SHARED_CONFIGr_SOP_DISCARD_MODEf_SET(obm2_shared_cfg, 1);
            ioerr += WRITE_IDB_OBM2_SHARED_CONFIGr(unit, pipe, obm2_shared_cfg);
            break;
        case 3:
            ioerr += READ_IDB_OBM3_SHARED_CONFIGr(unit, pipe, &obm3_shared_cfg);
            IDB_OBM3_SHARED_CONFIGr_DISCARD_THRESHOLDf_SET(obm3_shared_cfg, 
                                                           1023);
            IDB_OBM3_SHARED_CONFIGr_SOP_THRESHOLDf_SET(obm3_shared_cfg, 1023);
            IDB_OBM3_SHARED_CONFIGr_SOP_DISCARD_MODEf_SET(obm3_shared_cfg, 1);
            ioerr += WRITE_IDB_OBM3_SHARED_CONFIGr(unit, pipe, obm3_shared_cfg);
            break;        
        case 4:
            ioerr += READ_IDB_OBM4_SHARED_CONFIGr(unit, pipe, &obm4_shared_cfg);
            IDB_OBM4_SHARED_CONFIGr_DISCARD_THRESHOLDf_SET(obm4_shared_cfg, 
                                                           1023);
            IDB_OBM4_SHARED_CONFIGr_SOP_THRESHOLDf_SET(obm4_shared_cfg, 1023);
            IDB_OBM4_SHARED_CONFIGr_SOP_DISCARD_MODEf_SET(obm4_shared_cfg, 1);
            ioerr += WRITE_IDB_OBM4_SHARED_CONFIGr(unit, pipe, obm4_shared_cfg);
            break;        
        case 5:
            ioerr += READ_IDB_OBM5_SHARED_CONFIGr(unit, pipe, &obm5_shared_cfg);
            IDB_OBM5_SHARED_CONFIGr_DISCARD_THRESHOLDf_SET(obm5_shared_cfg, 
                                                           1023);
            IDB_OBM5_SHARED_CONFIGr_SOP_THRESHOLDf_SET(obm5_shared_cfg, 1023);
            IDB_OBM5_SHARED_CONFIGr_SOP_DISCARD_MODEf_SET(obm5_shared_cfg, 1);
            ioerr += WRITE_IDB_OBM5_SHARED_CONFIGr(unit, pipe, obm5_shared_cfg);
            break;        
        case 6:
            ioerr += READ_IDB_OBM6_SHARED_CONFIGr(unit, pipe, &obm6_shared_cfg);
            IDB_OBM6_SHARED_CONFIGr_DISCARD_THRESHOLDf_SET(obm6_shared_cfg, 
                                                           1023);
            IDB_OBM6_SHARED_CONFIGr_SOP_THRESHOLDf_SET(obm6_shared_cfg, 1023);
            IDB_OBM6_SHARED_CONFIGr_SOP_DISCARD_MODEf_SET(obm6_shared_cfg, 1);
            ioerr += WRITE_IDB_OBM6_SHARED_CONFIGr(unit, pipe, obm6_shared_cfg);
            break;        
        case 7:
            ioerr += READ_IDB_OBM7_SHARED_CONFIGr(unit, pipe, &obm7_shared_cfg);
            IDB_OBM7_SHARED_CONFIGr_DISCARD_THRESHOLDf_SET(obm7_shared_cfg, 
                                                           1023);
            IDB_OBM7_SHARED_CONFIGr_SOP_THRESHOLDf_SET(obm7_shared_cfg, 1023);
            IDB_OBM7_SHARED_CONFIGr_SOP_DISCARD_MODEf_SET(obm7_shared_cfg, 1);
            ioerr += WRITE_IDB_OBM7_SHARED_CONFIGr(unit, pipe, obm7_shared_cfg);
            break;
        default:
            break;
        }

        if (lossless) {
            if (tcfg->ovs_ratio_x1000[pipe] >= 1800) { /* ratio >= 1.8 */
                oversub_ratio_idx = 2;
            } else if (tcfg->ovs_ratio_x1000[pipe] >= 1500) { /* ratio >= 1.5 */
                oversub_ratio_idx = 1;
            } else {
                oversub_ratio_idx = 0;
            }

            for (lane = 0; lane < TH_PORTS_PER_PBLK; lane++) {
                if (P2L(unit, port + lane) == -1) {
                    continue;
                }
                num_lanes = bcm56960_a0_port_lanes_get(unit, port);
                if (num_lanes <= 0) {
                    continue;
                }
                switch (obm) {
                case 0:
                    /* Configure threshold */
                    ioerr += READ_IDB_OBM0_THRESHOLDr(unit, pipe, lane, 
                                                      &obm0_thresh);
                    fval = obm_settings[num_lanes].discard_limit;
                    IDB_OBM0_THRESHOLDr_DISCARD_LIMITf_SET(obm0_thresh, fval);
                    IDB_OBM0_THRESHOLDr_LOSSLESS0_DISCARDf_SET(obm0_thresh, 
                                                               1023);
                    fval = obm_settings[num_lanes].lossy_low_pri[oversub_ratio_idx];
                    IDB_OBM0_THRESHOLDr_LOSSY_LOW_PRIf_SET(obm0_thresh, fval);
                    fval = obm_settings[num_lanes].lossy_discard[oversub_ratio_idx];
                    IDB_OBM0_THRESHOLDr_LOSSY_DISCARDf_SET(obm0_thresh, fval);
                    ioerr += WRITE_IDB_OBM0_THRESHOLDr(unit, pipe, lane, 
                                                       obm0_thresh);

                    /* Configure flow control threshold */
                    ioerr += READ_IDB_OBM0_FC_THRESHOLDr(unit, pipe, lane, 
                                                         &obm0_fc_thresh);
                    fval = obm_settings[num_lanes].port_xoff[oversub_ratio_idx];
                    IDB_OBM0_FC_THRESHOLDr_PORT_XOFFf_SET(obm0_fc_thresh, fval);
                    IDB_OBM0_FC_THRESHOLDr_PORT_XONf_SET(obm0_fc_thresh, 
                                                         fval-10);
                    fval = obm_settings[num_lanes].lossless_xoff;
                    IDB_OBM0_FC_THRESHOLDr_LOSSLESS0_XOFFf_SET(obm0_fc_thresh, 
                                                               fval);
                    IDB_OBM0_FC_THRESHOLDr_LOSSLESS0_XONf_SET(obm0_fc_thresh, 
                                                              fval-10);
                    ioerr += WRITE_IDB_OBM0_FC_THRESHOLDr(unit, pipe, lane, 
                                                          obm0_fc_thresh);

                    /* Configure flow control config */
                    ioerr += READ_IDB_OBM0_FLOW_CONTROL_CONFIGr(unit, pipe, 
                                                     lane, &obm0_flow_ctrl_cfg);
                    IDB_OBM0_FLOW_CONTROL_CONFIGr_PORT_FC_ENf_SET(
                                                         obm0_flow_ctrl_cfg, 1);
                    IDB_OBM0_FLOW_CONTROL_CONFIGr_LOSSLESS0_FC_ENf_SET(
                                                         obm0_flow_ctrl_cfg, 1);
                    IDB_OBM0_FLOW_CONTROL_CONFIGr_LOSSLESS1_FC_ENf_SET(
                                                         obm0_flow_ctrl_cfg, 1);
                    IDB_OBM0_FLOW_CONTROL_CONFIGr_LOSSLESS0_PRIORITY_PROFILEf_SET(
                                                      obm0_flow_ctrl_cfg, 0xff);
                    IDB_OBM0_FLOW_CONTROL_CONFIGr_LOSSLESS1_PRIORITY_PROFILEf_SET(
                                                      obm0_flow_ctrl_cfg, 0xff);
                    ioerr += WRITE_IDB_OBM0_FLOW_CONTROL_CONFIGr(unit, pipe, 
                                                      lane, obm0_flow_ctrl_cfg);

                    /* OBM Port config */
                    IDB_OBM0_PORT_CONFIGr_CLR(obm0_port_cfg);
                    IDB_OBM0_PORT_CONFIGr_PORT_PRIf_SET(obm0_port_cfg, 2);
                    ioerr += WRITE_IDB_OBM0_PORT_CONFIGr(unit, pipe, lane, 
                                                         obm0_port_cfg);

                    if (lane == 0) {
                        IDB_OBM0_PRI_MAP_PORT0m_CLR(obm0_pri_map_p0);
                        ioerr += WRITE_IDB_OBM0_PRI_MAP_PORT0m(unit, pipe, 0, 
                                                               obm0_pri_map_p0);
                    } else if (lane == 1) {
                        IDB_OBM0_PRI_MAP_PORT1m_CLR(obm0_pri_map_p1);
                        ioerr += WRITE_IDB_OBM0_PRI_MAP_PORT1m(unit, pipe, 0, 
                                                               obm0_pri_map_p1);
                    } else if (lane == 2) {
                        IDB_OBM0_PRI_MAP_PORT2m_CLR(obm0_pri_map_p2);
                        ioerr += WRITE_IDB_OBM0_PRI_MAP_PORT2m(unit, pipe, 0, 
                                                               obm0_pri_map_p2);
                    } else if (lane == 3) {
                        IDB_OBM0_PRI_MAP_PORT3m_CLR(obm0_pri_map_p3);
                        ioerr += WRITE_IDB_OBM0_PRI_MAP_PORT3m(unit, pipe, 0, 
                                                               obm0_pri_map_p3);
                    }
                    break;
                case 1:
                    ioerr += READ_IDB_OBM1_THRESHOLDr(unit, pipe, lane, 
                                                      &obm1_thresh);
                    fval = obm_settings[num_lanes].discard_limit;
                    IDB_OBM1_THRESHOLDr_DISCARD_LIMITf_SET(obm1_thresh, fval);
                    IDB_OBM1_THRESHOLDr_LOSSLESS0_DISCARDf_SET(obm1_thresh, 
                                                               1023);
                    fval = obm_settings[num_lanes].lossy_low_pri[oversub_ratio_idx];
                    IDB_OBM1_THRESHOLDr_LOSSY_LOW_PRIf_SET(obm1_thresh, fval);
                    fval = obm_settings[num_lanes].lossy_discard[oversub_ratio_idx];
                    IDB_OBM1_THRESHOLDr_LOSSY_DISCARDf_SET(obm1_thresh, fval);
                    ioerr += WRITE_IDB_OBM1_THRESHOLDr(unit, pipe, lane, 
                                                       obm1_thresh);

                    /* Configure flow control threshold */
                    ioerr += READ_IDB_OBM1_FC_THRESHOLDr(unit, pipe, lane, 
                                                         &obm1_fc_thresh);
                    fval = obm_settings[num_lanes].port_xoff[oversub_ratio_idx];
                    IDB_OBM1_FC_THRESHOLDr_PORT_XOFFf_SET(obm1_fc_thresh, fval);
                    IDB_OBM1_FC_THRESHOLDr_PORT_XONf_SET(obm1_fc_thresh, 
                                                         fval-10);
                    fval = obm_settings[num_lanes].lossless_xoff;
                    IDB_OBM1_FC_THRESHOLDr_LOSSLESS0_XOFFf_SET(obm1_fc_thresh, 
                                                               fval);
                    IDB_OBM1_FC_THRESHOLDr_LOSSLESS0_XONf_SET(obm1_fc_thresh, 
                                                              fval-10);
                    ioerr += WRITE_IDB_OBM1_FC_THRESHOLDr(unit, pipe, lane, 
                                                          obm1_fc_thresh);

                    /* Configure flow control config */
                    ioerr += READ_IDB_OBM1_FLOW_CONTROL_CONFIGr(unit, pipe, 
                                                     lane, &obm1_flow_ctrl_cfg);
                    IDB_OBM1_FLOW_CONTROL_CONFIGr_PORT_FC_ENf_SET(
                                                         obm1_flow_ctrl_cfg, 1);
                    IDB_OBM1_FLOW_CONTROL_CONFIGr_LOSSLESS0_FC_ENf_SET(
                                                         obm1_flow_ctrl_cfg, 1);
                    IDB_OBM1_FLOW_CONTROL_CONFIGr_LOSSLESS1_FC_ENf_SET(
                                                         obm1_flow_ctrl_cfg, 1);
                    IDB_OBM1_FLOW_CONTROL_CONFIGr_LOSSLESS0_PRIORITY_PROFILEf_SET(
                                                      obm1_flow_ctrl_cfg, 0xff);
                    IDB_OBM1_FLOW_CONTROL_CONFIGr_LOSSLESS1_PRIORITY_PROFILEf_SET(
                                                      obm1_flow_ctrl_cfg, 0xff);
                    ioerr += WRITE_IDB_OBM1_FLOW_CONTROL_CONFIGr(unit, pipe, 
                                                      lane, obm1_flow_ctrl_cfg);

                    /* OBM Port config */
                    IDB_OBM1_PORT_CONFIGr_CLR(obm1_port_cfg);
                    IDB_OBM1_PORT_CONFIGr_PORT_PRIf_SET(obm1_port_cfg, 2);
                    ioerr += WRITE_IDB_OBM1_PORT_CONFIGr(unit, pipe, lane, 
                                                         obm1_port_cfg);

                    if (lane == 0) {
                        IDB_OBM1_PRI_MAP_PORT0m_CLR(obm1_pri_map_p0);
                        ioerr += WRITE_IDB_OBM1_PRI_MAP_PORT0m(unit, pipe, 0, 
                                                               obm1_pri_map_p0);
                    } else if (lane == 1) {
                        IDB_OBM1_PRI_MAP_PORT1m_CLR(obm1_pri_map_p1);
                        ioerr += WRITE_IDB_OBM1_PRI_MAP_PORT1m(unit, pipe, 0, 
                                                               obm1_pri_map_p1);
                    } else if (lane == 2) {
                        IDB_OBM1_PRI_MAP_PORT2m_CLR(obm1_pri_map_p2);
                        ioerr += WRITE_IDB_OBM1_PRI_MAP_PORT2m(unit, pipe, 0, 
                                                               obm1_pri_map_p2);
                    } else if (lane == 3) {
                        IDB_OBM1_PRI_MAP_PORT3m_CLR(obm1_pri_map_p3);
                        ioerr += WRITE_IDB_OBM1_PRI_MAP_PORT3m(unit, pipe, 0, 
                                                               obm1_pri_map_p3);
                    }                    
                    break;
                case 2:
                    ioerr += READ_IDB_OBM2_THRESHOLDr(unit, pipe, lane, 
                                                      &obm2_thresh);
                    fval = obm_settings[num_lanes].discard_limit;
                    IDB_OBM2_THRESHOLDr_DISCARD_LIMITf_SET(obm2_thresh, fval);
                    IDB_OBM2_THRESHOLDr_LOSSLESS0_DISCARDf_SET(obm2_thresh, 
                                                               1023);
                    fval = obm_settings[num_lanes].lossy_low_pri[oversub_ratio_idx];
                    IDB_OBM2_THRESHOLDr_LOSSY_LOW_PRIf_SET(obm2_thresh, fval);
                    fval = obm_settings[num_lanes].lossy_discard[oversub_ratio_idx];
                    IDB_OBM2_THRESHOLDr_LOSSY_DISCARDf_SET(obm2_thresh, fval);
                    ioerr += WRITE_IDB_OBM2_THRESHOLDr(unit, pipe, lane, 
                                                       obm2_thresh);

                    /* Configure flow control threshold */
                    ioerr += READ_IDB_OBM2_FC_THRESHOLDr(unit, pipe, lane, 
                                                         &obm2_fc_thresh);
                    fval = obm_settings[num_lanes].port_xoff[oversub_ratio_idx];
                    IDB_OBM2_FC_THRESHOLDr_PORT_XOFFf_SET(obm2_fc_thresh, fval);
                    IDB_OBM2_FC_THRESHOLDr_PORT_XONf_SET(obm2_fc_thresh, 
                                                         fval-10);
                    fval = obm_settings[num_lanes].lossless_xoff;
                    IDB_OBM2_FC_THRESHOLDr_LOSSLESS0_XOFFf_SET(obm2_fc_thresh, 
                                                               fval);
                    IDB_OBM2_FC_THRESHOLDr_LOSSLESS0_XONf_SET(obm2_fc_thresh, 
                                                              fval-10);
                    ioerr += WRITE_IDB_OBM2_FC_THRESHOLDr(unit, pipe, lane, 
                                                          obm2_fc_thresh);

                    /* Configure flow control config */
                    ioerr += READ_IDB_OBM2_FLOW_CONTROL_CONFIGr(unit, pipe, 
                                                     lane, &obm2_flow_ctrl_cfg);
                    IDB_OBM2_FLOW_CONTROL_CONFIGr_PORT_FC_ENf_SET(
                                                         obm2_flow_ctrl_cfg, 1);
                    IDB_OBM2_FLOW_CONTROL_CONFIGr_LOSSLESS0_FC_ENf_SET(
                                                         obm2_flow_ctrl_cfg, 1);
                    IDB_OBM2_FLOW_CONTROL_CONFIGr_LOSSLESS1_FC_ENf_SET(
                                                         obm2_flow_ctrl_cfg, 1);
                    IDB_OBM2_FLOW_CONTROL_CONFIGr_LOSSLESS0_PRIORITY_PROFILEf_SET(
                                                      obm2_flow_ctrl_cfg, 0xff);
                    IDB_OBM2_FLOW_CONTROL_CONFIGr_LOSSLESS1_PRIORITY_PROFILEf_SET(
                                                      obm2_flow_ctrl_cfg, 0xff);
                    ioerr += WRITE_IDB_OBM2_FLOW_CONTROL_CONFIGr(unit, pipe, 
                                                      lane, obm2_flow_ctrl_cfg);

                    /* OBM Port config */
                    IDB_OBM2_PORT_CONFIGr_CLR(obm2_port_cfg);
                    IDB_OBM2_PORT_CONFIGr_PORT_PRIf_SET(obm2_port_cfg, 2);
                    ioerr += WRITE_IDB_OBM2_PORT_CONFIGr(unit, pipe, lane, 
                                                         obm2_port_cfg);

                    if (lane == 0) {
                        IDB_OBM2_PRI_MAP_PORT0m_CLR(obm2_pri_map_p0);
                        ioerr += WRITE_IDB_OBM2_PRI_MAP_PORT0m(unit, pipe, 0, 
                                                               obm2_pri_map_p0);
                    } else if (lane == 1) {
                        IDB_OBM2_PRI_MAP_PORT1m_CLR(obm2_pri_map_p1);
                        ioerr += WRITE_IDB_OBM2_PRI_MAP_PORT1m(unit, pipe, 0, 
                                                               obm2_pri_map_p1);
                    } else if (lane == 2) {
                        IDB_OBM2_PRI_MAP_PORT2m_CLR(obm2_pri_map_p2);
                        ioerr += WRITE_IDB_OBM2_PRI_MAP_PORT2m(unit, pipe, 0, 
                                                               obm2_pri_map_p2);
                    } else if (lane == 3) {
                        IDB_OBM2_PRI_MAP_PORT3m_CLR(obm2_pri_map_p3);
                        ioerr += WRITE_IDB_OBM2_PRI_MAP_PORT3m(unit, pipe, 0, 
                                                               obm2_pri_map_p3);
                    }                    
                    break;
                case 3:
                    ioerr += READ_IDB_OBM3_THRESHOLDr(unit, pipe, lane, 
                                                      &obm3_thresh);
                    fval = obm_settings[num_lanes].discard_limit;
                    IDB_OBM3_THRESHOLDr_DISCARD_LIMITf_SET(obm3_thresh, fval);
                    IDB_OBM3_THRESHOLDr_LOSSLESS0_DISCARDf_SET(obm3_thresh, 
                                                               1023);
                    fval = obm_settings[num_lanes].lossy_low_pri[oversub_ratio_idx];
                    IDB_OBM3_THRESHOLDr_LOSSY_LOW_PRIf_SET(obm3_thresh, fval);
                    fval = obm_settings[num_lanes].lossy_discard[oversub_ratio_idx];
                    IDB_OBM3_THRESHOLDr_LOSSY_DISCARDf_SET(obm3_thresh, fval);
                    ioerr += WRITE_IDB_OBM3_THRESHOLDr(unit, pipe, lane, 
                                                       obm3_thresh);

                    /* Configure flow control threshold */
                    ioerr += READ_IDB_OBM3_FC_THRESHOLDr(unit, pipe, lane, 
                                                         &obm3_fc_thresh);
                    fval = obm_settings[num_lanes].port_xoff[oversub_ratio_idx];
                    IDB_OBM3_FC_THRESHOLDr_PORT_XOFFf_SET(obm3_fc_thresh, fval);
                    IDB_OBM3_FC_THRESHOLDr_PORT_XONf_SET(obm3_fc_thresh, 
                                                         fval-10);
                    fval = obm_settings[num_lanes].lossless_xoff;
                    IDB_OBM3_FC_THRESHOLDr_LOSSLESS0_XOFFf_SET(obm3_fc_thresh, 
                                                               fval);
                    IDB_OBM3_FC_THRESHOLDr_LOSSLESS0_XONf_SET(obm3_fc_thresh, 
                                                              fval-10);
                    ioerr += WRITE_IDB_OBM3_FC_THRESHOLDr(unit, pipe, lane, 
                                                          obm3_fc_thresh);

                    /* Configure flow control config */
                    ioerr += READ_IDB_OBM3_FLOW_CONTROL_CONFIGr(unit, pipe, 
                                                     lane, &obm3_flow_ctrl_cfg);
                    IDB_OBM3_FLOW_CONTROL_CONFIGr_PORT_FC_ENf_SET(
                                                         obm3_flow_ctrl_cfg, 1);
                    IDB_OBM3_FLOW_CONTROL_CONFIGr_LOSSLESS0_FC_ENf_SET(
                                                         obm3_flow_ctrl_cfg, 1);
                    IDB_OBM3_FLOW_CONTROL_CONFIGr_LOSSLESS1_FC_ENf_SET(
                                                         obm3_flow_ctrl_cfg, 1);
                    IDB_OBM3_FLOW_CONTROL_CONFIGr_LOSSLESS0_PRIORITY_PROFILEf_SET(
                                                      obm3_flow_ctrl_cfg, 0xff);
                    IDB_OBM3_FLOW_CONTROL_CONFIGr_LOSSLESS1_PRIORITY_PROFILEf_SET(
                                                      obm3_flow_ctrl_cfg, 0xff);
                    ioerr += WRITE_IDB_OBM3_FLOW_CONTROL_CONFIGr(unit, pipe, 
                                                      lane, obm3_flow_ctrl_cfg);

                    /* OBM Port config */
                    IDB_OBM3_PORT_CONFIGr_CLR(obm3_port_cfg);
                    IDB_OBM3_PORT_CONFIGr_PORT_PRIf_SET(obm3_port_cfg, 2);
                    ioerr += WRITE_IDB_OBM3_PORT_CONFIGr(unit, pipe, lane, 
                                                         obm3_port_cfg);

                    if (lane == 0) {
                        IDB_OBM3_PRI_MAP_PORT0m_CLR(obm3_pri_map_p0);
                        ioerr += WRITE_IDB_OBM3_PRI_MAP_PORT0m(unit, pipe, 0, 
                                                               obm3_pri_map_p0);
                    } else if (lane == 1) {
                        IDB_OBM3_PRI_MAP_PORT1m_CLR(obm3_pri_map_p1);
                        ioerr += WRITE_IDB_OBM3_PRI_MAP_PORT1m(unit, pipe, 0, 
                                                               obm3_pri_map_p1);
                    } else if (lane == 2) {
                        IDB_OBM3_PRI_MAP_PORT2m_CLR(obm3_pri_map_p2);
                        ioerr += WRITE_IDB_OBM3_PRI_MAP_PORT2m(unit, pipe, 0, 
                                                               obm3_pri_map_p2);
                    } else if (lane == 3) {
                        IDB_OBM3_PRI_MAP_PORT3m_CLR(obm3_pri_map_p3);
                        ioerr += WRITE_IDB_OBM3_PRI_MAP_PORT3m(unit, pipe, 0, 
                                                               obm3_pri_map_p3);
                    }                    
                    break;
                case 4:
                    ioerr += READ_IDB_OBM4_THRESHOLDr(unit, pipe, lane, 
                                                      &obm4_thresh);
                    fval = obm_settings[num_lanes].discard_limit;
                    IDB_OBM4_THRESHOLDr_DISCARD_LIMITf_SET(obm4_thresh, fval);
                    IDB_OBM4_THRESHOLDr_LOSSLESS0_DISCARDf_SET(obm4_thresh, 
                                                               1023);
                    fval = obm_settings[num_lanes].lossy_low_pri[oversub_ratio_idx];
                    IDB_OBM4_THRESHOLDr_LOSSY_LOW_PRIf_SET(obm4_thresh, fval);
                    fval = obm_settings[num_lanes].lossy_discard[oversub_ratio_idx];
                    IDB_OBM4_THRESHOLDr_LOSSY_DISCARDf_SET(obm4_thresh, fval);
                    ioerr += WRITE_IDB_OBM4_THRESHOLDr(unit, pipe, lane, 
                                                       obm4_thresh);

                    /* Configure flow control threshold */
                    ioerr += READ_IDB_OBM4_FC_THRESHOLDr(unit, pipe, lane, 
                                                         &obm4_fc_thresh);
                    fval = obm_settings[num_lanes].port_xoff[oversub_ratio_idx];
                    IDB_OBM4_FC_THRESHOLDr_PORT_XOFFf_SET(obm4_fc_thresh, fval);
                    IDB_OBM4_FC_THRESHOLDr_PORT_XONf_SET(obm4_fc_thresh, 
                                                         fval-10);
                    fval = obm_settings[num_lanes].lossless_xoff;
                    IDB_OBM4_FC_THRESHOLDr_LOSSLESS0_XOFFf_SET(obm4_fc_thresh, 
                                                               fval);
                    IDB_OBM4_FC_THRESHOLDr_LOSSLESS0_XONf_SET(obm4_fc_thresh, 
                                                              fval-10);
                    ioerr += WRITE_IDB_OBM4_FC_THRESHOLDr(unit, pipe, lane, 
                                                          obm4_fc_thresh);

                    /* Configure flow control config */
                    ioerr += READ_IDB_OBM4_FLOW_CONTROL_CONFIGr(unit, pipe, 
                                                     lane, &obm4_flow_ctrl_cfg);
                    IDB_OBM4_FLOW_CONTROL_CONFIGr_PORT_FC_ENf_SET(
                                                         obm4_flow_ctrl_cfg, 1);
                    IDB_OBM4_FLOW_CONTROL_CONFIGr_LOSSLESS0_FC_ENf_SET(
                                                         obm4_flow_ctrl_cfg, 1);
                    IDB_OBM4_FLOW_CONTROL_CONFIGr_LOSSLESS1_FC_ENf_SET(
                                                         obm4_flow_ctrl_cfg, 1);
                    IDB_OBM4_FLOW_CONTROL_CONFIGr_LOSSLESS0_PRIORITY_PROFILEf_SET(
                                                      obm4_flow_ctrl_cfg, 0xff);
                    IDB_OBM4_FLOW_CONTROL_CONFIGr_LOSSLESS1_PRIORITY_PROFILEf_SET(
                                                      obm4_flow_ctrl_cfg, 0xff);
                    ioerr += WRITE_IDB_OBM4_FLOW_CONTROL_CONFIGr(unit, pipe, 
                                                      lane, obm4_flow_ctrl_cfg);

                    /* OBM Port config */
                    IDB_OBM4_PORT_CONFIGr_CLR(obm4_port_cfg);
                    IDB_OBM4_PORT_CONFIGr_PORT_PRIf_SET(obm4_port_cfg, 2);
                    ioerr += WRITE_IDB_OBM4_PORT_CONFIGr(unit, pipe, lane, 
                                                         obm4_port_cfg);

                    if (lane == 0) {
                        IDB_OBM4_PRI_MAP_PORT0m_CLR(obm4_pri_map_p0);
                        ioerr += WRITE_IDB_OBM4_PRI_MAP_PORT0m(unit, pipe, 0, 
                                                               obm4_pri_map_p0);
                    } else if (lane == 1) {
                        IDB_OBM4_PRI_MAP_PORT1m_CLR(obm4_pri_map_p1);
                        ioerr += WRITE_IDB_OBM4_PRI_MAP_PORT1m(unit, pipe, 0, 
                                                               obm4_pri_map_p1);
                    } else if (lane == 2) {
                        IDB_OBM4_PRI_MAP_PORT2m_CLR(obm4_pri_map_p2);
                        ioerr += WRITE_IDB_OBM4_PRI_MAP_PORT2m(unit, pipe, 0, 
                                                               obm4_pri_map_p2);
                    } else if (lane == 3) {
                        IDB_OBM4_PRI_MAP_PORT3m_CLR(obm4_pri_map_p3);
                        ioerr += WRITE_IDB_OBM4_PRI_MAP_PORT3m(unit, pipe, 0, 
                                                               obm4_pri_map_p3);
                    }                     
                    break;
                case 5:
                    ioerr += READ_IDB_OBM5_THRESHOLDr(unit, pipe, lane, 
                                                      &obm5_thresh);
                    fval = obm_settings[num_lanes].discard_limit;
                    IDB_OBM5_THRESHOLDr_DISCARD_LIMITf_SET(obm5_thresh, fval);
                    IDB_OBM5_THRESHOLDr_LOSSLESS0_DISCARDf_SET(obm5_thresh, 
                                                               1023);
                    fval = obm_settings[num_lanes].lossy_low_pri[oversub_ratio_idx];
                    IDB_OBM5_THRESHOLDr_LOSSY_LOW_PRIf_SET(obm5_thresh, fval);
                    fval = obm_settings[num_lanes].lossy_discard[oversub_ratio_idx];
                    IDB_OBM5_THRESHOLDr_LOSSY_DISCARDf_SET(obm5_thresh, fval);
                    ioerr += WRITE_IDB_OBM5_THRESHOLDr(unit, pipe, lane, 
                                                       obm5_thresh);

                    /* Configure flow control threshold */
                    ioerr += READ_IDB_OBM5_FC_THRESHOLDr(unit, pipe, lane, 
                                                         &obm5_fc_thresh);
                    fval = obm_settings[num_lanes].port_xoff[oversub_ratio_idx];
                    IDB_OBM5_FC_THRESHOLDr_PORT_XOFFf_SET(obm5_fc_thresh, fval);
                    IDB_OBM5_FC_THRESHOLDr_PORT_XONf_SET(obm5_fc_thresh, 
                                                         fval-10);
                    fval = obm_settings[num_lanes].lossless_xoff;
                    IDB_OBM5_FC_THRESHOLDr_LOSSLESS0_XOFFf_SET(obm5_fc_thresh, 
                                                               fval);
                    IDB_OBM5_FC_THRESHOLDr_LOSSLESS0_XONf_SET(obm5_fc_thresh, 
                                                              fval-10);
                    ioerr += WRITE_IDB_OBM5_FC_THRESHOLDr(unit, pipe, lane, 
                                                          obm5_fc_thresh);

                    /* Configure flow control config */
                    ioerr += READ_IDB_OBM5_FLOW_CONTROL_CONFIGr(unit, pipe, 
                                                     lane, &obm5_flow_ctrl_cfg);
                    IDB_OBM5_FLOW_CONTROL_CONFIGr_PORT_FC_ENf_SET(
                                                         obm5_flow_ctrl_cfg, 1);
                    IDB_OBM5_FLOW_CONTROL_CONFIGr_LOSSLESS0_FC_ENf_SET(
                                                         obm5_flow_ctrl_cfg, 1);
                    IDB_OBM5_FLOW_CONTROL_CONFIGr_LOSSLESS1_FC_ENf_SET(
                                                         obm5_flow_ctrl_cfg, 1);
                    IDB_OBM5_FLOW_CONTROL_CONFIGr_LOSSLESS0_PRIORITY_PROFILEf_SET(
                                                      obm5_flow_ctrl_cfg, 0xff);
                    IDB_OBM5_FLOW_CONTROL_CONFIGr_LOSSLESS1_PRIORITY_PROFILEf_SET(
                                                      obm5_flow_ctrl_cfg, 0xff);
                    ioerr += WRITE_IDB_OBM5_FLOW_CONTROL_CONFIGr(unit, pipe, 
                                                      lane, obm5_flow_ctrl_cfg);

                    /* OBM Port config */
                    IDB_OBM5_PORT_CONFIGr_CLR(obm5_port_cfg);
                    IDB_OBM5_PORT_CONFIGr_PORT_PRIf_SET(obm5_port_cfg, 2);
                    ioerr += WRITE_IDB_OBM5_PORT_CONFIGr(unit, pipe, lane, 
                                                         obm5_port_cfg);

                    if (lane == 0) {
                        IDB_OBM5_PRI_MAP_PORT0m_CLR(obm5_pri_map_p0);
                        ioerr += WRITE_IDB_OBM5_PRI_MAP_PORT0m(unit, pipe, 0, 
                                                               obm5_pri_map_p0);
                    } else if (lane == 1) {
                        IDB_OBM5_PRI_MAP_PORT1m_CLR(obm5_pri_map_p1);
                        ioerr += WRITE_IDB_OBM5_PRI_MAP_PORT1m(unit, pipe, 0, 
                                                               obm5_pri_map_p1);
                    } else if (lane == 2) {
                        IDB_OBM5_PRI_MAP_PORT2m_CLR(obm5_pri_map_p2);
                        ioerr += WRITE_IDB_OBM5_PRI_MAP_PORT2m(unit, pipe, 0, 
                                                               obm5_pri_map_p2);
                    } else if (lane == 3) {
                        IDB_OBM5_PRI_MAP_PORT3m_CLR(obm5_pri_map_p3);
                        ioerr += WRITE_IDB_OBM5_PRI_MAP_PORT3m(unit, pipe, 0, 
                                                               obm5_pri_map_p3);
                    }                     
                    break;
                case 6:
                    ioerr += READ_IDB_OBM6_THRESHOLDr(unit, pipe, lane, 
                                                      &obm6_thresh);
                    fval = obm_settings[num_lanes].discard_limit;
                    IDB_OBM6_THRESHOLDr_DISCARD_LIMITf_SET(obm6_thresh, fval);
                    IDB_OBM6_THRESHOLDr_LOSSLESS0_DISCARDf_SET(obm6_thresh, 
                                                               1023);
                    fval = obm_settings[num_lanes].lossy_low_pri[oversub_ratio_idx];
                    IDB_OBM6_THRESHOLDr_LOSSY_LOW_PRIf_SET(obm6_thresh, fval);
                    fval = obm_settings[num_lanes].lossy_discard[oversub_ratio_idx];
                    IDB_OBM6_THRESHOLDr_LOSSY_DISCARDf_SET(obm6_thresh, fval);
                    ioerr += WRITE_IDB_OBM6_THRESHOLDr(unit, pipe, lane, 
                                                       obm6_thresh);

                    /* Configure flow control threshold */
                    ioerr += READ_IDB_OBM6_FC_THRESHOLDr(unit, pipe, lane, 
                                                         &obm6_fc_thresh);
                    fval = obm_settings[num_lanes].port_xoff[oversub_ratio_idx];
                    IDB_OBM6_FC_THRESHOLDr_PORT_XOFFf_SET(obm6_fc_thresh, fval);
                    IDB_OBM6_FC_THRESHOLDr_PORT_XONf_SET(obm6_fc_thresh, 
                                                         fval-10);
                    fval = obm_settings[num_lanes].lossless_xoff;
                    IDB_OBM6_FC_THRESHOLDr_LOSSLESS0_XOFFf_SET(obm6_fc_thresh, 
                                                               fval);
                    IDB_OBM6_FC_THRESHOLDr_LOSSLESS0_XONf_SET(obm6_fc_thresh, 
                                                              fval-10);
                    ioerr += WRITE_IDB_OBM6_FC_THRESHOLDr(unit, pipe, lane, 
                                                          obm6_fc_thresh);

                    /* Configure flow control config */
                    ioerr += READ_IDB_OBM6_FLOW_CONTROL_CONFIGr(unit, pipe, 
                                                     lane, &obm6_flow_ctrl_cfg);
                    IDB_OBM6_FLOW_CONTROL_CONFIGr_PORT_FC_ENf_SET(
                                                         obm6_flow_ctrl_cfg, 1);
                    IDB_OBM6_FLOW_CONTROL_CONFIGr_LOSSLESS0_FC_ENf_SET(
                                                         obm6_flow_ctrl_cfg, 1);
                    IDB_OBM6_FLOW_CONTROL_CONFIGr_LOSSLESS1_FC_ENf_SET(
                                                         obm6_flow_ctrl_cfg, 1);
                    IDB_OBM6_FLOW_CONTROL_CONFIGr_LOSSLESS0_PRIORITY_PROFILEf_SET(
                                                      obm6_flow_ctrl_cfg, 0xff);
                    IDB_OBM6_FLOW_CONTROL_CONFIGr_LOSSLESS1_PRIORITY_PROFILEf_SET(
                                                      obm6_flow_ctrl_cfg, 0xff);
                    ioerr += WRITE_IDB_OBM6_FLOW_CONTROL_CONFIGr(unit, pipe, 
                                                      lane, obm6_flow_ctrl_cfg);

                    /* OBM Port config */
                    IDB_OBM6_PORT_CONFIGr_CLR(obm6_port_cfg);
                    IDB_OBM6_PORT_CONFIGr_PORT_PRIf_SET(obm6_port_cfg, 2);
                    ioerr += WRITE_IDB_OBM6_PORT_CONFIGr(unit, pipe, lane, 
                                                         obm6_port_cfg);

                    if (lane == 0) {
                        IDB_OBM6_PRI_MAP_PORT0m_CLR(obm6_pri_map_p0);
                        ioerr += WRITE_IDB_OBM6_PRI_MAP_PORT0m(unit, pipe, 0, 
                                                               obm6_pri_map_p0);
                    } else if (lane == 1) {
                        IDB_OBM6_PRI_MAP_PORT1m_CLR(obm6_pri_map_p1);
                        ioerr += WRITE_IDB_OBM6_PRI_MAP_PORT1m(unit, pipe, 0, 
                                                               obm6_pri_map_p1);
                    } else if (lane == 2) {
                        IDB_OBM6_PRI_MAP_PORT2m_CLR(obm6_pri_map_p2);
                        ioerr += WRITE_IDB_OBM6_PRI_MAP_PORT2m(unit, pipe, 0, 
                                                               obm6_pri_map_p2);
                    } else if (lane == 3) {
                        IDB_OBM6_PRI_MAP_PORT3m_CLR(obm6_pri_map_p3);
                        ioerr += WRITE_IDB_OBM6_PRI_MAP_PORT3m(unit, pipe, 0, 
                                                               obm6_pri_map_p3);
                    }                     
                    break;
                case 7:
                    ioerr += READ_IDB_OBM7_THRESHOLDr(unit, pipe, lane, 
                                                      &obm7_thresh);
                    fval = obm_settings[num_lanes].discard_limit;
                    IDB_OBM7_THRESHOLDr_DISCARD_LIMITf_SET(obm7_thresh, fval);
                    IDB_OBM7_THRESHOLDr_LOSSLESS0_DISCARDf_SET(obm7_thresh, 
                                                               1023);
                    fval = obm_settings[num_lanes].lossy_low_pri[oversub_ratio_idx];
                    IDB_OBM7_THRESHOLDr_LOSSY_LOW_PRIf_SET(obm7_thresh, fval);
                    fval = obm_settings[num_lanes].lossy_discard[oversub_ratio_idx];
                    IDB_OBM7_THRESHOLDr_LOSSY_DISCARDf_SET(obm7_thresh, fval);
                    ioerr += WRITE_IDB_OBM7_THRESHOLDr(unit, pipe, lane, 
                                                       obm7_thresh);

                    /* Configure flow control threshold */
                    ioerr += READ_IDB_OBM7_FC_THRESHOLDr(unit, pipe, lane, 
                                                         &obm7_fc_thresh);
                    fval = obm_settings[num_lanes].port_xoff[oversub_ratio_idx];
                    IDB_OBM7_FC_THRESHOLDr_PORT_XOFFf_SET(obm7_fc_thresh, fval);
                    IDB_OBM7_FC_THRESHOLDr_PORT_XONf_SET(obm7_fc_thresh, 
                                                         fval-10);
                    fval = obm_settings[num_lanes].lossless_xoff;
                    IDB_OBM7_FC_THRESHOLDr_LOSSLESS0_XOFFf_SET(obm7_fc_thresh, 
                                                               fval);
                    IDB_OBM7_FC_THRESHOLDr_LOSSLESS0_XONf_SET(obm7_fc_thresh, 
                                                              fval-10);
                    ioerr += WRITE_IDB_OBM7_FC_THRESHOLDr(unit, pipe, lane, 
                                                          obm7_fc_thresh);

                    /* Configure flow control config */
                    ioerr += READ_IDB_OBM7_FLOW_CONTROL_CONFIGr(unit, pipe, 
                                                     lane, &obm7_flow_ctrl_cfg);
                    IDB_OBM7_FLOW_CONTROL_CONFIGr_PORT_FC_ENf_SET(
                                                         obm7_flow_ctrl_cfg, 1);
                    IDB_OBM7_FLOW_CONTROL_CONFIGr_LOSSLESS0_FC_ENf_SET(
                                                         obm7_flow_ctrl_cfg, 1);
                    IDB_OBM7_FLOW_CONTROL_CONFIGr_LOSSLESS1_FC_ENf_SET(
                                                         obm7_flow_ctrl_cfg, 1);
                    IDB_OBM7_FLOW_CONTROL_CONFIGr_LOSSLESS0_PRIORITY_PROFILEf_SET(
                                                      obm7_flow_ctrl_cfg, 0xff);
                    IDB_OBM7_FLOW_CONTROL_CONFIGr_LOSSLESS1_PRIORITY_PROFILEf_SET(
                                                      obm7_flow_ctrl_cfg, 0xff);
                    ioerr += WRITE_IDB_OBM7_FLOW_CONTROL_CONFIGr(unit, pipe, 
                                                      lane, obm7_flow_ctrl_cfg);

                    /* OBM Port config */
                    IDB_OBM7_PORT_CONFIGr_CLR(obm7_port_cfg);
                    IDB_OBM7_PORT_CONFIGr_PORT_PRIf_SET(obm7_port_cfg, 2);
                    ioerr += WRITE_IDB_OBM7_PORT_CONFIGr(unit, pipe, lane, 
                                                         obm7_port_cfg);

                    if (lane == 0) {
                        IDB_OBM7_PRI_MAP_PORT0m_CLR(obm7_pri_map_p0);
                        ioerr += WRITE_IDB_OBM7_PRI_MAP_PORT0m(unit, pipe, 0, 
                                                               obm7_pri_map_p0);
                    } else if (lane == 1) {
                        IDB_OBM7_PRI_MAP_PORT1m_CLR(obm7_pri_map_p1);
                        ioerr += WRITE_IDB_OBM7_PRI_MAP_PORT1m(unit, pipe, 0, 
                                                               obm7_pri_map_p1);
                    } else if (lane == 2) {
                        IDB_OBM7_PRI_MAP_PORT2m_CLR(obm7_pri_map_p2);
                        ioerr += WRITE_IDB_OBM7_PRI_MAP_PORT2m(unit, pipe, 0, 
                                                               obm7_pri_map_p2);
                    } else if (lane == 3) {
                        IDB_OBM7_PRI_MAP_PORT3m_CLR(obm7_pri_map_p3);
                        ioerr += WRITE_IDB_OBM7_PRI_MAP_PORT3m(unit, pipe, 0, 
                                                               obm7_pri_map_p3);
                    }                     
                    break;
                default:
                    break;
                }
            }
            switch (obm) {
            case 0:
                IDB_OBM0_MAX_USAGE_SELECTr_CLR(obm0_max_us);
                IDB_OBM0_MAX_USAGE_SELECTr_PRIORITY_SELECTf_SET(obm0_max_us, 2);
                ioerr += WRITE_IDB_OBM0_MAX_USAGE_SELECTr(unit, pipe, 
                                                          obm0_max_us);
                break;
            case 1:
                IDB_OBM1_MAX_USAGE_SELECTr_CLR(obm1_max_us);
                IDB_OBM1_MAX_USAGE_SELECTr_PRIORITY_SELECTf_SET(obm1_max_us, 2);
                ioerr += WRITE_IDB_OBM1_MAX_USAGE_SELECTr(unit, pipe, 
                                                          obm1_max_us);
                break;
            case 2:
                IDB_OBM2_MAX_USAGE_SELECTr_CLR(obm2_max_us);
                IDB_OBM2_MAX_USAGE_SELECTr_PRIORITY_SELECTf_SET(obm2_max_us, 2);
                ioerr += WRITE_IDB_OBM2_MAX_USAGE_SELECTr(unit, pipe, 
                                                          obm2_max_us);
                break;            
            case 3:
                IDB_OBM3_MAX_USAGE_SELECTr_CLR(obm3_max_us);
                IDB_OBM3_MAX_USAGE_SELECTr_PRIORITY_SELECTf_SET(obm3_max_us, 2);
                ioerr += WRITE_IDB_OBM3_MAX_USAGE_SELECTr(unit, pipe, 
                                                          obm3_max_us);
                break;            
            case 4:
                IDB_OBM4_MAX_USAGE_SELECTr_CLR(obm4_max_us);
                IDB_OBM4_MAX_USAGE_SELECTr_PRIORITY_SELECTf_SET(obm4_max_us, 2);
                ioerr += WRITE_IDB_OBM4_MAX_USAGE_SELECTr(unit, pipe, 
                                                          obm4_max_us);
                break;
            case 5:
                IDB_OBM5_MAX_USAGE_SELECTr_CLR(obm5_max_us);
                IDB_OBM5_MAX_USAGE_SELECTr_PRIORITY_SELECTf_SET(obm5_max_us, 2);
                ioerr += WRITE_IDB_OBM5_MAX_USAGE_SELECTr(unit, pipe, 
                                                          obm5_max_us);
                break;            
            case 6:
                IDB_OBM6_MAX_USAGE_SELECTr_CLR(obm6_max_us);
                IDB_OBM6_MAX_USAGE_SELECTr_PRIORITY_SELECTf_SET(obm6_max_us, 2);
                ioerr += WRITE_IDB_OBM6_MAX_USAGE_SELECTr(unit, pipe, 
                                                          obm6_max_us);
                break;
            case 7:
                IDB_OBM7_MAX_USAGE_SELECTr_CLR(obm7_max_us);
                IDB_OBM7_MAX_USAGE_SELECTr_PRIORITY_SELECTf_SET(obm7_max_us, 2);
                ioerr += WRITE_IDB_OBM7_MAX_USAGE_SELECTr(unit, pipe, 
                                                          obm7_max_us);
                break;            
            }
        } else {
            for (lane = 0; lane < TH_PORTS_PER_PBLK; lane++) {
                if (P2L(unit, port + lane) == -1) {
                    continue;
                }
                num_lanes = bcm56960_a0_port_lanes_get(unit, port);
                if (num_lanes <= 0) {
                    continue;
                }
                switch (obm) {
                case 0:
                    /* Configure threshold */
                    ioerr += READ_IDB_OBM0_THRESHOLDr(unit, pipe, lane, 
                                                      &obm0_thresh);
                    fval = obm_settings[num_lanes].discard_limit;
                    IDB_OBM0_THRESHOLDr_DISCARD_LIMITf_SET(obm0_thresh, fval);
                    /* same as DISCARD_LIMIT setting */
                    IDB_OBM0_THRESHOLDr_LOSSY_DISCARDf_SET(obm0_thresh, fval);
                    IDB_OBM0_THRESHOLDr_LOSSLESS0_DISCARDf_SET(obm0_thresh, 
                                                               1023);
                    IDB_OBM0_THRESHOLDr_LOSSY_LOW_PRIf_SET(obm0_thresh, 1023);
                    ioerr += WRITE_IDB_OBM0_THRESHOLDr(unit, pipe, lane, 
                                                       obm0_thresh);

                    /* Configure flow control threshold */
                    ioerr += READ_IDB_OBM0_FC_THRESHOLDr(unit, pipe, lane, 
                                                         &obm0_fc_thresh);
                    IDB_OBM0_FC_THRESHOLDr_PORT_XOFFf_SET(obm0_fc_thresh, 1023);
                    IDB_OBM0_FC_THRESHOLDr_PORT_XONf_SET(obm0_fc_thresh, 1023);
                    IDB_OBM0_FC_THRESHOLDr_LOSSLESS0_XOFFf_SET(obm0_fc_thresh, 
                                                               1023);
                    IDB_OBM0_FC_THRESHOLDr_LOSSLESS0_XONf_SET(obm0_fc_thresh, 
                                                              1023);
                    ioerr += WRITE_IDB_OBM0_FC_THRESHOLDr(unit, pipe, lane, 
                                                          obm0_fc_thresh);
                    break;
                case 1:
                    /* Configure threshold */
                    ioerr += READ_IDB_OBM1_THRESHOLDr(unit, pipe, lane, 
                                                      &obm1_thresh);
                    fval = obm_settings[num_lanes].discard_limit;
                    IDB_OBM1_THRESHOLDr_DISCARD_LIMITf_SET(obm1_thresh, fval);
                    /* same as DISCARD_LIMIT setting */
                    IDB_OBM1_THRESHOLDr_LOSSY_DISCARDf_SET(obm1_thresh, fval);
                    IDB_OBM1_THRESHOLDr_LOSSLESS0_DISCARDf_SET(obm1_thresh, 
                                                               1023);
                    IDB_OBM1_THRESHOLDr_LOSSY_LOW_PRIf_SET(obm1_thresh, 1023);
                    ioerr += WRITE_IDB_OBM1_THRESHOLDr(unit, pipe, lane, 
                                                       obm1_thresh);

                    /* Configure flow control threshold */
                    ioerr += READ_IDB_OBM1_FC_THRESHOLDr(unit, pipe, lane, 
                                                         &obm1_fc_thresh);
                    IDB_OBM1_FC_THRESHOLDr_PORT_XOFFf_SET(obm1_fc_thresh, 1023);
                    IDB_OBM1_FC_THRESHOLDr_PORT_XONf_SET(obm1_fc_thresh, 1023);
                    IDB_OBM1_FC_THRESHOLDr_LOSSLESS0_XOFFf_SET(obm1_fc_thresh, 
                                                               1023);
                    IDB_OBM1_FC_THRESHOLDr_LOSSLESS0_XONf_SET(obm1_fc_thresh, 
                                                              1023);
                    ioerr += WRITE_IDB_OBM1_FC_THRESHOLDr(unit, pipe, lane, 
                                                          obm1_fc_thresh);
                    break;                
                case 2:
                    /* Configure threshold */
                    ioerr += READ_IDB_OBM2_THRESHOLDr(unit, pipe, lane, 
                                                      &obm2_thresh);
                    fval = obm_settings[num_lanes].discard_limit;
                    IDB_OBM2_THRESHOLDr_DISCARD_LIMITf_SET(obm2_thresh, fval);
                    /* same as DISCARD_LIMIT setting */
                    IDB_OBM2_THRESHOLDr_LOSSY_DISCARDf_SET(obm2_thresh, fval);
                    IDB_OBM2_THRESHOLDr_LOSSLESS0_DISCARDf_SET(obm2_thresh, 
                                                               1023);
                    IDB_OBM2_THRESHOLDr_LOSSY_LOW_PRIf_SET(obm2_thresh, 1023);
                    ioerr += WRITE_IDB_OBM2_THRESHOLDr(unit, pipe, lane, 
                                                       obm2_thresh);

                    /* Configure flow control threshold */
                    ioerr += READ_IDB_OBM2_FC_THRESHOLDr(unit, pipe, lane, 
                                                         &obm2_fc_thresh);
                    IDB_OBM2_FC_THRESHOLDr_PORT_XOFFf_SET(obm2_fc_thresh, 1023);
                    IDB_OBM2_FC_THRESHOLDr_PORT_XONf_SET(obm2_fc_thresh, 1023);
                    IDB_OBM2_FC_THRESHOLDr_LOSSLESS0_XOFFf_SET(obm2_fc_thresh, 
                                                               1023);
                    IDB_OBM2_FC_THRESHOLDr_LOSSLESS0_XONf_SET(obm2_fc_thresh, 
                                                              1023);
                    ioerr += WRITE_IDB_OBM2_FC_THRESHOLDr(unit, pipe, lane, 
                                                          obm2_fc_thresh);
                    break;
                case 3:
                    /* Configure threshold */
                    ioerr += READ_IDB_OBM3_THRESHOLDr(unit, pipe, lane, 
                                                      &obm3_thresh);
                    fval = obm_settings[num_lanes].discard_limit;
                    IDB_OBM3_THRESHOLDr_DISCARD_LIMITf_SET(obm3_thresh, fval);
                    /* same as DISCARD_LIMIT setting */
                    IDB_OBM3_THRESHOLDr_LOSSY_DISCARDf_SET(obm3_thresh, fval);
                    IDB_OBM3_THRESHOLDr_LOSSLESS0_DISCARDf_SET(obm3_thresh, 
                                                               1023);
                    IDB_OBM3_THRESHOLDr_LOSSY_LOW_PRIf_SET(obm3_thresh, 1023);
                    ioerr += WRITE_IDB_OBM3_THRESHOLDr(unit, pipe, lane, 
                                                       obm3_thresh);

                    /* Configure flow control threshold */
                    ioerr += READ_IDB_OBM3_FC_THRESHOLDr(unit, pipe, lane, 
                                                         &obm3_fc_thresh);
                    IDB_OBM3_FC_THRESHOLDr_PORT_XOFFf_SET(obm3_fc_thresh, 1023);
                    IDB_OBM3_FC_THRESHOLDr_PORT_XONf_SET(obm3_fc_thresh, 1023);
                    IDB_OBM3_FC_THRESHOLDr_LOSSLESS0_XOFFf_SET(obm3_fc_thresh, 
                                                               1023);
                    IDB_OBM3_FC_THRESHOLDr_LOSSLESS0_XONf_SET(obm3_fc_thresh, 
                                                              1023);
                    ioerr += WRITE_IDB_OBM3_FC_THRESHOLDr(unit, pipe, lane, 
                                                          obm3_fc_thresh);
                    break;
                case 4:
                    /* Configure threshold */
                    ioerr += READ_IDB_OBM4_THRESHOLDr(unit, pipe, lane, 
                                                      &obm4_thresh);
                    fval = obm_settings[num_lanes].discard_limit;
                    IDB_OBM4_THRESHOLDr_DISCARD_LIMITf_SET(obm4_thresh, fval);
                    /* same as DISCARD_LIMIT setting */
                    IDB_OBM4_THRESHOLDr_LOSSY_DISCARDf_SET(obm4_thresh, fval);
                    IDB_OBM4_THRESHOLDr_LOSSLESS0_DISCARDf_SET(obm4_thresh, 
                                                               1023);
                    IDB_OBM4_THRESHOLDr_LOSSY_LOW_PRIf_SET(obm4_thresh, 1023);
                    ioerr += WRITE_IDB_OBM4_THRESHOLDr(unit, pipe, lane, 
                                                       obm4_thresh);

                    /* Configure flow control threshold */
                    ioerr += READ_IDB_OBM4_FC_THRESHOLDr(unit, pipe, lane, 
                                                         &obm4_fc_thresh);
                    IDB_OBM4_FC_THRESHOLDr_PORT_XOFFf_SET(obm4_fc_thresh, 1023);
                    IDB_OBM4_FC_THRESHOLDr_PORT_XONf_SET(obm4_fc_thresh, 1023);
                    IDB_OBM4_FC_THRESHOLDr_LOSSLESS0_XOFFf_SET(obm4_fc_thresh, 
                                                               1023);
                    IDB_OBM4_FC_THRESHOLDr_LOSSLESS0_XONf_SET(obm4_fc_thresh, 
                                                              1023);
                    ioerr += WRITE_IDB_OBM4_FC_THRESHOLDr(unit, pipe, lane, 
                                                          obm4_fc_thresh);
                    break;
                case 5:
                    /* Configure threshold */
                    ioerr += READ_IDB_OBM5_THRESHOLDr(unit, pipe, lane, 
                                                      &obm5_thresh);
                    fval = obm_settings[num_lanes].discard_limit;
                    IDB_OBM5_THRESHOLDr_DISCARD_LIMITf_SET(obm5_thresh, fval);
                    /* same as DISCARD_LIMIT setting */
                    IDB_OBM5_THRESHOLDr_LOSSY_DISCARDf_SET(obm5_thresh, fval);
                    IDB_OBM5_THRESHOLDr_LOSSLESS0_DISCARDf_SET(obm5_thresh, 
                                                               1023);
                    IDB_OBM5_THRESHOLDr_LOSSY_LOW_PRIf_SET(obm5_thresh, 1023);
                    ioerr += WRITE_IDB_OBM5_THRESHOLDr(unit, pipe, lane, 
                                                       obm5_thresh);

                    /* Configure flow control threshold */
                    ioerr += READ_IDB_OBM5_FC_THRESHOLDr(unit, pipe, lane, 
                                                         &obm5_fc_thresh);
                    IDB_OBM5_FC_THRESHOLDr_PORT_XOFFf_SET(obm5_fc_thresh, 1023);
                    IDB_OBM5_FC_THRESHOLDr_PORT_XONf_SET(obm5_fc_thresh, 1023);
                    IDB_OBM5_FC_THRESHOLDr_LOSSLESS0_XOFFf_SET(obm5_fc_thresh, 
                                                               1023);
                    IDB_OBM5_FC_THRESHOLDr_LOSSLESS0_XONf_SET(obm5_fc_thresh, 
                                                              1023);
                    ioerr += WRITE_IDB_OBM5_FC_THRESHOLDr(unit, pipe, lane, 
                                                          obm5_fc_thresh);
                    break;
                case 6:
                    /* Configure threshold */
                    ioerr += READ_IDB_OBM6_THRESHOLDr(unit, pipe, lane, 
                                                      &obm6_thresh);
                    fval = obm_settings[num_lanes].discard_limit;
                    IDB_OBM6_THRESHOLDr_DISCARD_LIMITf_SET(obm6_thresh, fval);
                    /* same as DISCARD_LIMIT setting */
                    IDB_OBM6_THRESHOLDr_LOSSY_DISCARDf_SET(obm6_thresh, fval);
                    IDB_OBM6_THRESHOLDr_LOSSLESS0_DISCARDf_SET(obm6_thresh, 
                                                               1023);
                    IDB_OBM6_THRESHOLDr_LOSSY_LOW_PRIf_SET(obm6_thresh, 1023);
                    ioerr += WRITE_IDB_OBM6_THRESHOLDr(unit, pipe, lane, 
                                                       obm6_thresh);

                    /* Configure flow control threshold */
                    ioerr += READ_IDB_OBM6_FC_THRESHOLDr(unit, pipe, lane, 
                                                         &obm6_fc_thresh);
                    IDB_OBM6_FC_THRESHOLDr_PORT_XOFFf_SET(obm6_fc_thresh, 1023);
                    IDB_OBM6_FC_THRESHOLDr_PORT_XONf_SET(obm6_fc_thresh, 1023);
                    IDB_OBM6_FC_THRESHOLDr_LOSSLESS0_XOFFf_SET(obm6_fc_thresh, 
                                                               1023);
                    IDB_OBM6_FC_THRESHOLDr_LOSSLESS0_XONf_SET(obm6_fc_thresh, 
                                                              1023);
                    ioerr += WRITE_IDB_OBM6_FC_THRESHOLDr(unit, pipe, lane, 
                                                          obm6_fc_thresh);
                    break;
                case 7:
                    /* Configure threshold */
                    ioerr += READ_IDB_OBM7_THRESHOLDr(unit, pipe, lane, 
                                                      &obm7_thresh);
                    fval = obm_settings[num_lanes].discard_limit;
                    IDB_OBM7_THRESHOLDr_DISCARD_LIMITf_SET(obm7_thresh, fval);
                    /* same as DISCARD_LIMIT setting */
                    IDB_OBM7_THRESHOLDr_LOSSY_DISCARDf_SET(obm7_thresh, fval);
                    IDB_OBM7_THRESHOLDr_LOSSLESS0_DISCARDf_SET(obm7_thresh, 
                                                               1023);
                    IDB_OBM7_THRESHOLDr_LOSSY_LOW_PRIf_SET(obm7_thresh, 1023);
                    ioerr += WRITE_IDB_OBM7_THRESHOLDr(unit, pipe, lane, 
                                                       obm7_thresh);

                    /* Configure flow control threshold */
                    ioerr += READ_IDB_OBM7_FC_THRESHOLDr(unit, pipe, lane, 
                                                         &obm7_fc_thresh);
                    IDB_OBM7_FC_THRESHOLDr_PORT_XOFFf_SET(obm7_fc_thresh, 1023);
                    IDB_OBM7_FC_THRESHOLDr_PORT_XONf_SET(obm7_fc_thresh, 1023);
                    IDB_OBM7_FC_THRESHOLDr_LOSSLESS0_XOFFf_SET(obm7_fc_thresh, 
                                                               1023);
                    IDB_OBM7_FC_THRESHOLDr_LOSSLESS0_XONf_SET(obm7_fc_thresh, 
                                                              1023);
                    ioerr += WRITE_IDB_OBM7_FC_THRESHOLDr(unit, pipe, lane, 
                                                          obm7_fc_thresh);
                    break;
                default:
                    break;
                }
            }
        }
    }

    /* Toggle cpu port cell assembly reset to send initial credit to EP */
    IDB_CA_CPU_CONTROLr_CLR(ca_cpu_ctrl);
    IDB_CA_CPU_CONTROLr_PORT_RESETf_SET(ca_cpu_ctrl, 1);
    ioerr += WRITE_IDB_CA_CPU_CONTROLr(unit, 0, ca_cpu_ctrl);
    IDB_CA_CPU_CONTROLr_CLR(ca_cpu_ctrl);
    ioerr += WRITE_IDB_CA_CPU_CONTROLr(unit, 0, ca_cpu_ctrl);

    /* Toggle loopback port cell assembly reset to send initial credit to EP */
    bcm56960_a0_lpbk_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        pipe = bcm56960_a0_port_pipe_get(unit, port);
        if (pipe < 0) {
            continue;
        }
        IDB_CA_LPBK_CONTROLr_CLR(ca_lpbk_ctrl);
        IDB_CA_LPBK_CONTROLr_PORT_RESETf_SET(ca_lpbk_ctrl, 1);
        ioerr += WRITE_IDB_CA_LPBK_CONTROLr(unit, pipe, ca_lpbk_ctrl);
        IDB_CA_LPBK_CONTROLr_CLR(ca_lpbk_ctrl);
        ioerr += WRITE_IDB_CA_LPBK_CONTROLr(unit, pipe, ca_lpbk_ctrl);                
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE; 
}

static int
_tdm_calculation(int unit, int initTime, struct _tdm_config_s *tdm, 
                 int core_frequency)
{
    int port;
    int pipe, blk_index, blk_port, lane, group, idx, num_lanes;
    int length, ovs_core_slot_count, ovs_io_slot_count, port_slot_count;
    int mode[TH_PIPES_PER_DEV];
    cdk_pbmp_t pbmp, pbmp_port, oversub_pbmp;

    CDK_MEMSET(tdm, 0, sizeof(struct _tdm_config_s));

    bcm56960_a0_all_pbmp_get(unit, &pbmp);
    for (pipe = 0; pipe < TH_PIPES_PER_DEV; pipe++) {
        mode[pipe] = PORT_ETHERNET;
        CDK_PBMP_CLEAR(pbmp_port);
        CDK_PBMP_ITER(pbmp, port) {
            if (pipe == bcm56960_a0_port_pipe_get(unit, port)) {
                if (BMD_PORT_PROPERTIES(unit, port) == BMD_PORT_HG) {
                    CDK_PBMP_ADD(pbmp_port, port);
                }
            }
        }
        bcm56960_a0_oversub_map_get(unit, &oversub_pbmp);
        CDK_PBMP_REMOVE(pbmp_port, oversub_pbmp);
        if (CDK_PBMP_NOT_NULL(pbmp_port)) {
            mode[pipe] = PORT_HIGIG2;
        }
    }

    for (idx = 0; idx < COUNTOF(tdm->pm_encap_type); idx++) {
        tdm->pm_encap_type[idx] = PORT_ETHERNET;
    }

    bcm56960_a0_clport_pbmp_get(unit, &pbmp_port);  
    bcm56960_a0_xlport_pbmp_get(unit, &pbmp);
    CDK_PBMP_OR(pbmp_port, pbmp);

    bcm56960_a0_oversub_map_get(unit, &oversub_pbmp);
    CDK_PBMP_ITER(pbmp_port, port) {
        if (BMD_PORT_PROPERTIES(unit, port) & BMD_PORT_FLEX) {
            continue;
        }
        tdm->port_state[port] = 
            CDK_PBMP_MEMBER(oversub_pbmp, port) ?
            PORT_STATE_OVERSUB : PORT_STATE_LINERATE;

        num_lanes = bcm56960_a0_port_lanes_get(unit, port);
        for (lane = 1; lane < num_lanes; lane++) {
            if (port + lane >= TH_NUM_EXT_PORTS) {
                break;
            }
            tdm->port_state[port + lane] = PORT_STATE_CONTINUATION;
        }
        tdm->speed[port] = bcm56960_a0_port_speed_max(unit, port);
    }

    /* CLPORT */
    bcm56960_a0_clport_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        blk_port = bcm56960_a0_block_port_get(unit, port, BLKTYPE_CLPORT);
        if (blk_port != 0) {
            continue;
        }
        blk_index = bcm56960_a0_block_index_get(unit, port, BLKTYPE_CLPORT);

        _port_ratio_get(unit, blk_index, &tdm->port_ratio[blk_index]);
        pipe = bcm56960_a0_port_pipe_get(unit, port);
        if (pipe < 0) {
            continue;
        }
        tdm->pm_encap_type[blk_index] = mode[pipe];
    }    

    /* Management ports */
    bcm56960_a0_xlport_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        pipe = bcm56960_a0_port_pipe_get(unit, port);
        if (pipe < 0) {
            continue;
        }
        tdm->pm_encap_type[TH_PBLKS_PER_DEV] = mode[pipe];
        tdm->speed[port] = bcm56960_a0_port_speed_max(unit, port);
    }

    CDK_VVERB(("frequency: %dMHz\n", core_frequency)); 
    CDK_VVERB(("port speed:"));
    for (idx = 0; idx < TH_NUM_EXT_PORTS; idx++) {
        if (idx % 8 == 0) {
            CDK_VVERB(("\n    "));
        }
        CDK_VVERB((" %6d", tdm->speed[idx]));
    }
    CDK_VVERB(("\n"));
    CDK_VVERB(("port state map:"));
    for (idx = 0; idx < TH_NUM_EXT_PORTS; idx++) {
        if (idx % 16 == 0) {
            CDK_VVERB(("\n    "));
        }
        if (idx == 0 || idx == (TH_NUM_EXT_PORTS - 1)) {
            CDK_VVERB((" ---"));
        } else {
            CDK_VVERB((" %3d", tdm->port_state[idx]));
        }
    }
    CDK_VVERB(("\n"));
    CDK_VVERB(("pm encap type:"));
    for (idx = 0; idx < TH_NUM_PORT_MODULES; idx++) {
        if (idx % 16 == 0) {
            CDK_VVERB(("\n    "));
        }
        CDK_VVERB((" %3d", tdm->pm_encap_type[idx]));
    }
    CDK_VVERB(("\n"));

    for (idx = 0; idx < TH_NUM_EXT_PORTS; idx++) {
        tdm->tdm_globals.speed[idx] = tdm->speed[idx];
    }
    tdm->tdm_globals.clk_freq = core_frequency;
    for (idx = 1; idx < TH_NUM_EXT_PORTS; idx++) {
        tdm->tdm_globals.port_rates_array[idx-1] = tdm->port_state[idx];
    }
    for (idx = 1; idx < TH_NUM_PORT_MODULES; idx++) {
        tdm->tdm_globals.pm_encap_type[idx] = tdm->pm_encap_type[idx];
    }

    if (bcm56960_a0_set_tdm_tbl(&tdm->tdm_globals, 
                                &tdm->tdm_pipe_tables) == 0) {
        CDK_VVERB(("Unable to configure TDM, please contact your "
                   "Field Applications Engineer or Sales Manager for "
                   "additional assistance.\n"));
        return CDK_E_FAIL;
    } else {
        tdm->idb_tdm[0] = tdm->tdm_pipe_tables.idb_tdm_tbl_0;
        tdm->idb_ovs_tdm[0][0] = tdm->tdm_pipe_tables.idb_tdm_ovs_0_a;
        tdm->idb_ovs_tdm[0][1] = tdm->tdm_pipe_tables.idb_tdm_ovs_0_b;
        tdm->idb_ovs_tdm[0][2] = tdm->tdm_pipe_tables.idb_tdm_ovs_0_c;
        tdm->idb_ovs_tdm[0][3] = tdm->tdm_pipe_tables.idb_tdm_ovs_0_d;
        tdm->idb_ovs_tdm[0][4] = tdm->tdm_pipe_tables.idb_tdm_ovs_0_e;
        tdm->idb_ovs_tdm[0][5] = tdm->tdm_pipe_tables.idb_tdm_ovs_0_f;
        tdm->idb_tdm[1] = tdm->tdm_pipe_tables.idb_tdm_tbl_1;
        tdm->idb_ovs_tdm[1][0] = tdm->tdm_pipe_tables.idb_tdm_ovs_1_a;
        tdm->idb_ovs_tdm[1][1] = tdm->tdm_pipe_tables.idb_tdm_ovs_1_b;
        tdm->idb_ovs_tdm[1][2] = tdm->tdm_pipe_tables.idb_tdm_ovs_1_c;
        tdm->idb_ovs_tdm[1][3] = tdm->tdm_pipe_tables.idb_tdm_ovs_1_d;
        tdm->idb_ovs_tdm[1][4] = tdm->tdm_pipe_tables.idb_tdm_ovs_1_e;
        tdm->idb_ovs_tdm[1][5] = tdm->tdm_pipe_tables.idb_tdm_ovs_1_f;
        tdm->idb_tdm[2] = tdm->tdm_pipe_tables.idb_tdm_tbl_2;
        tdm->idb_ovs_tdm[2][0] = tdm->tdm_pipe_tables.idb_tdm_ovs_2_a;
        tdm->idb_ovs_tdm[2][1] = tdm->tdm_pipe_tables.idb_tdm_ovs_2_b;
        tdm->idb_ovs_tdm[2][2] = tdm->tdm_pipe_tables.idb_tdm_ovs_2_c;
        tdm->idb_ovs_tdm[2][3] = tdm->tdm_pipe_tables.idb_tdm_ovs_2_d;
        tdm->idb_ovs_tdm[2][4] = tdm->tdm_pipe_tables.idb_tdm_ovs_2_e;
        tdm->idb_ovs_tdm[2][5] = tdm->tdm_pipe_tables.idb_tdm_ovs_2_f;
        tdm->idb_tdm[3] = tdm->tdm_pipe_tables.idb_tdm_tbl_3;
        tdm->idb_ovs_tdm[3][0] = tdm->tdm_pipe_tables.idb_tdm_ovs_3_a;
        tdm->idb_ovs_tdm[3][1] = tdm->tdm_pipe_tables.idb_tdm_ovs_3_b;
        tdm->idb_ovs_tdm[3][2] = tdm->tdm_pipe_tables.idb_tdm_ovs_3_c;
        tdm->idb_ovs_tdm[3][3] = tdm->tdm_pipe_tables.idb_tdm_ovs_3_d;
        tdm->idb_ovs_tdm[3][4] = tdm->tdm_pipe_tables.idb_tdm_ovs_3_e;
        tdm->idb_ovs_tdm[3][5] = tdm->tdm_pipe_tables.idb_tdm_ovs_3_f;
        tdm->mmu_tdm[0] = tdm->tdm_pipe_tables.mmu_tdm_tbl_0;
        tdm->mmu_ovs_tdm[0][0] = tdm->tdm_pipe_tables.mmu_tdm_ovs_0_a;
        tdm->mmu_ovs_tdm[0][1] = tdm->tdm_pipe_tables.mmu_tdm_ovs_0_b;
        tdm->mmu_ovs_tdm[0][2] = tdm->tdm_pipe_tables.mmu_tdm_ovs_0_c;
        tdm->mmu_ovs_tdm[0][3] = tdm->tdm_pipe_tables.mmu_tdm_ovs_0_d;
        tdm->mmu_ovs_tdm[0][4] = tdm->tdm_pipe_tables.mmu_tdm_ovs_0_e;
        tdm->mmu_ovs_tdm[0][5] = tdm->tdm_pipe_tables.mmu_tdm_ovs_0_f;
        tdm->mmu_tdm[1] = tdm->tdm_pipe_tables.mmu_tdm_tbl_1;
        tdm->mmu_ovs_tdm[1][0] = tdm->tdm_pipe_tables.mmu_tdm_ovs_1_a;
        tdm->mmu_ovs_tdm[1][1] = tdm->tdm_pipe_tables.mmu_tdm_ovs_1_b;
        tdm->mmu_ovs_tdm[1][2] = tdm->tdm_pipe_tables.mmu_tdm_ovs_1_c;
        tdm->mmu_ovs_tdm[1][3] = tdm->tdm_pipe_tables.mmu_tdm_ovs_1_d;
        tdm->mmu_ovs_tdm[1][4] = tdm->tdm_pipe_tables.mmu_tdm_ovs_1_e;
        tdm->mmu_ovs_tdm[1][5] = tdm->tdm_pipe_tables.mmu_tdm_ovs_1_f;
        tdm->mmu_tdm[2] = tdm->tdm_pipe_tables.mmu_tdm_tbl_2;
        tdm->mmu_ovs_tdm[2][0] = tdm->tdm_pipe_tables.mmu_tdm_ovs_2_a;
        tdm->mmu_ovs_tdm[2][1] = tdm->tdm_pipe_tables.mmu_tdm_ovs_2_b;
        tdm->mmu_ovs_tdm[2][2] = tdm->tdm_pipe_tables.mmu_tdm_ovs_2_c;
        tdm->mmu_ovs_tdm[2][3] = tdm->tdm_pipe_tables.mmu_tdm_ovs_2_d;
        tdm->mmu_ovs_tdm[2][4] = tdm->tdm_pipe_tables.mmu_tdm_ovs_2_e;
        tdm->mmu_ovs_tdm[2][5] = tdm->tdm_pipe_tables.mmu_tdm_ovs_2_f;
        tdm->mmu_tdm[3] = tdm->tdm_pipe_tables.mmu_tdm_tbl_3;
        tdm->mmu_ovs_tdm[3][0] = tdm->tdm_pipe_tables.mmu_tdm_ovs_3_a;
        tdm->mmu_ovs_tdm[3][1] = tdm->tdm_pipe_tables.mmu_tdm_ovs_3_b;
        tdm->mmu_ovs_tdm[3][2] = tdm->tdm_pipe_tables.mmu_tdm_ovs_3_c;
        tdm->mmu_ovs_tdm[3][3] = tdm->tdm_pipe_tables.mmu_tdm_ovs_3_d;
        tdm->mmu_ovs_tdm[3][4] = tdm->tdm_pipe_tables.mmu_tdm_ovs_3_e;
        tdm->mmu_ovs_tdm[3][5] = tdm->tdm_pipe_tables.mmu_tdm_ovs_3_f;
    }

    for (pipe = 0; pipe < TH_PIPES_PER_DEV; pipe++) {
        /* Count number of OVSB_TOKEN assigned by the TDM code */
        for (length = TDM_LENGTH; length > 0; length--) {
            if (tdm->mmu_tdm[pipe][length - 1] != TH_NUM_EXT_PORTS) {
                break;
            }
        }
        ovs_core_slot_count = 0;
        for (idx = 0; idx < length; idx++) {
            if (tdm->mmu_tdm[pipe][idx] == OVSB_TOKEN) {
                ovs_core_slot_count++;
            }
        }

        /* Count number of slot needed for the oversub I/O bandwidth */
        ovs_io_slot_count = 0;
        for (group = 0; group < OVS_GROUP_COUNT; group++) {
            port_slot_count = 0;
            for (idx = 0; idx < OVS_GROUP_TDM_LENGTH; idx++) {
                port = tdm->mmu_ovs_tdm[pipe][group][idx];
                if (port == TH_NUM_EXT_PORTS) {
                    continue;
                }
                if (port_slot_count == 0) {
                    _speed_to_slot_mapping(unit, tdm->speed[port], 
                                           &port_slot_count);
                }
                ovs_io_slot_count += port_slot_count;
            }
        }

        if (ovs_core_slot_count != 0) {
            tdm->ovs_ratio_x1000[pipe] =
                ovs_io_slot_count * 1000 / ovs_core_slot_count;
        }
    }

    for (pipe = 0; pipe < TH_PIPES_PER_DEV; pipe++) {
        CDK_VVERB(("Pipe %d idb_tdm:", pipe));
        for (idx = 0; idx < TDM_LENGTH; idx++) {
            if (idx % 16 == 0) {
                CDK_VVERB(("\n    "));
            }
            CDK_VVERB((" %3d", tdm->idb_tdm[pipe][idx]));
        }
        CDK_VVERB(("\n"));
        for (group = 0; group < OVS_GROUP_COUNT; group++) {
            CDK_VVERB(("Pipe %d group %d idb_ovs_tdm", pipe, group));
            for (idx = 0; idx < OVS_GROUP_TDM_LENGTH; idx++) {
                if (idx % 16 == 0) {
                    CDK_VVERB(("\n    "));
                }
                CDK_VVERB((" %3d", tdm->idb_ovs_tdm[pipe][group][idx]));
            }
            CDK_VVERB(("\n"));
        }

        CDK_VVERB(("Pipe %d mmu_tdm:", pipe));
        for (idx = 0; idx < TDM_LENGTH; idx++) {
            if (idx % 16 == 0) {
                CDK_VVERB(("\n    "));
            }
            CDK_VVERB((" %3d", tdm->mmu_tdm[pipe][idx]));
        }
        CDK_VVERB(("\n"));
        for (group = 0; group < OVS_GROUP_COUNT; group++) {
            CDK_VVERB(("Pipe %d group %d mmu_ovs_tdm", pipe, group));
            for (idx = 0; idx < OVS_GROUP_TDM_LENGTH; idx++) {
                if (idx % 16 == 0) {
                    CDK_VVERB(("\n    "));
                }
                CDK_VVERB((" %3d", tdm->mmu_ovs_tdm[pipe][group][idx]));
            }
            CDK_VVERB(("\n"));
        }
        CDK_VVERB(("Pipe %d oversubscribe ratio %d.%-2d\n", pipe, 
                    tdm->ovs_ratio_x1000[pipe] / 1000,
                    tdm->ovs_ratio_x1000[pipe] % 1000));
    }

    return CDK_E_NONE;
}

static int
_hash_init(int unit)
{
    int ioerr = 0;
    L2_TABLE_HASH_CONTROLr_t l2_hash_ctrl;
    L3_TABLE_HASH_CONTROLr_t l3_hash_ctrl;
    SHARED_TABLE_HASH_CONTROLr_t shr_hash_ctrl;
    VLAN_XLATE_HASH_CONTROLr_t vlan_hash_ctrl;
    EGR_VLAN_XLATE_HASH_CONTROLr_t egr_vlan_hash_ctrl;
    MPLS_ENTRY_HASH_CONTROLr_t mpls_hash_ctrl;

    /* L2 dedicated banks */
    ioerr += READ_L2_TABLE_HASH_CONTROLr(unit, &l2_hash_ctrl);
    L2_TABLE_HASH_CONTROLr_BANK0_HASH_OFFSETf_SET(l2_hash_ctrl, 0);
    L2_TABLE_HASH_CONTROLr_BANK1_HASH_OFFSETf_SET(l2_hash_ctrl, 16);
    ioerr += WRITE_L2_TABLE_HASH_CONTROLr(unit, l2_hash_ctrl);

    /* L3 dedicated banks */
    ioerr += READ_L3_TABLE_HASH_CONTROLr(unit, &l3_hash_ctrl);
    L3_TABLE_HASH_CONTROLr_BANK6_HASH_OFFSETf_SET(l3_hash_ctrl, 0);
    L3_TABLE_HASH_CONTROLr_BANK7_HASH_OFFSETf_SET(l3_hash_ctrl, 8);
    L3_TABLE_HASH_CONTROLr_BANK8_HASH_OFFSETf_SET(l3_hash_ctrl, 16);
    L3_TABLE_HASH_CONTROLr_BANK9_HASH_OFFSETf_SET(l3_hash_ctrl, 32);
    ioerr += WRITE_L3_TABLE_HASH_CONTROLr(unit, l3_hash_ctrl);

    /* L2/L3/ALPM shared banks */
    ioerr += READ_SHARED_TABLE_HASH_CONTROLr(unit, &shr_hash_ctrl);
    SHARED_TABLE_HASH_CONTROLr_BANK2_HASH_OFFSETf_SET(shr_hash_ctrl, 4);
    SHARED_TABLE_HASH_CONTROLr_BANK2_HASH_OFFSETf_SET(shr_hash_ctrl, 12);
    SHARED_TABLE_HASH_CONTROLr_BANK2_HASH_OFFSETf_SET(shr_hash_ctrl, 20);
    SHARED_TABLE_HASH_CONTROLr_BANK2_HASH_OFFSETf_SET(shr_hash_ctrl, 24);
    ioerr += WRITE_SHARED_TABLE_HASH_CONTROLr(unit, shr_hash_ctrl);

    ioerr += READ_VLAN_XLATE_HASH_CONTROLr(unit, &vlan_hash_ctrl);
    VLAN_XLATE_HASH_CONTROLr_ROBUST_HASH_ENf_SET(vlan_hash_ctrl, 0);
    ioerr += WRITE_VLAN_XLATE_HASH_CONTROLr(unit, vlan_hash_ctrl);

    ioerr += READ_EGR_VLAN_XLATE_HASH_CONTROLr(unit, &egr_vlan_hash_ctrl);
    EGR_VLAN_XLATE_HASH_CONTROLr_ROBUST_HASH_ENf_SET(egr_vlan_hash_ctrl, 0);
    ioerr += WRITE_EGR_VLAN_XLATE_HASH_CONTROLr(unit, egr_vlan_hash_ctrl);

    ioerr += READ_MPLS_ENTRY_HASH_CONTROLr(unit, &mpls_hash_ctrl);
    MPLS_ENTRY_HASH_CONTROLr_ROBUST_HASH_ENf_SET(mpls_hash_ctrl, 0);
    ioerr += WRITE_MPLS_ENTRY_HASH_CONTROLr(unit, mpls_hash_ctrl);

    return ioerr;
}

static int
_mc_toq_cfg(int unit, int port, int enable) 
{
    int ioerr = 0;
    uint32_t fval;
    int pipe, mmu_port, baseidx;
    MMU_SCFG_TOQ_MC_CFG1r_t mc_cfg1;

    pipe = bcm56960_a0_port_pipe_get(unit, port);
    mmu_port = P2M(unit, port);

    baseidx = pipe >> 1;
    ioerr += READ_MMU_SCFG_TOQ_MC_CFG1r(unit, baseidx, &mc_cfg1);
    if (pipe & 1) {
        fval = MMU_SCFG_TOQ_MC_CFG1r_IS_MC_T2OQ_PORT1f_GET(mc_cfg1);
    } else {
        fval = MMU_SCFG_TOQ_MC_CFG1r_IS_MC_T2OQ_PORT0f_GET(mc_cfg1);    
    }

    if (enable) {
        fval |= 1 << (mmu_port & 0x0f);
    } else {
        fval &= ~(1 << (mmu_port & 0x0f));
    }

    if (pipe & 1) {
        MMU_SCFG_TOQ_MC_CFG1r_IS_MC_T2OQ_PORT1f_SET(mc_cfg1, fval);
    } else {
        MMU_SCFG_TOQ_MC_CFG1r_IS_MC_T2OQ_PORT0f_SET(mc_cfg1, fval);    
    }

    ioerr += WRITE_MMU_SCFG_TOQ_MC_CFG1r(unit, baseidx, mc_cfg1);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

static int
_default_pg_headroom(int unit, int port, int lossless)
{
    int speed = 1000, hdrm = 0;
    uint8_t port_oversub = 0;
    cdk_pbmp_t oversub_map;

    if (port == CMIC_PORT) {
        return 77;
    } else if (!lossless) {
        return 0;
    } else if (port >= 132 && port <= 135) { /* Loopback port */
        return 150;
    }

    speed = bcm56960_a0_port_speed_max(unit, port);
    bcm56960_a0_oversub_map_get(unit, &oversub_map);
    if (CDK_PBMP_MEMBER(oversub_map, port)) {
        port_oversub = 1;
    }

    if ((speed >= 1000) && (speed < 20000)) {
        hdrm = port_oversub ? 189 : 166;
    } else if ((speed >= 20000) && (speed <= 30000)) {
        hdrm = port_oversub ? 217 : 194;
    } else if ((speed >= 30000) && (speed <= 42000)) {
        hdrm = port_oversub ? 340 : 286;
    } else if ((speed >= 50000) && (speed < 100000)) {
        hdrm = port_oversub ? 358 : 304;
    } else if (speed >= 100000) {
        hdrm = port_oversub ? 648 : 519;
    } else {
        hdrm = port_oversub ? 189 : 166;
    }
    return hdrm;
}

/* Function to get the number of ports present in a given Port Macro */
static int
_ports_per_pm_get(int unit, int pm_id)
{
    int port_base, num_ports = 0;

    if (pm_id >= TH_PBLKS_PER_DEV) {
        return CDK_E_PARAM;
    }

    port_base = 1 + (pm_id * TH_PORTS_PER_PBLK);
    if (P2L(unit, port_base) != 0) {
        num_ports = 1;
        if ((bcm56960_a0_port_lanes_get(unit, port_base)== 2) &&
            (P2L(unit, port_base + 2) != -1)) {
            /* for cases when num_lanes is given in the config (40g).
             */
            num_ports = 2;
        }

        /* When phy_port_base + 1 is valid,
         *      All 4 ports of the PM(port macro) are valid.
         * When phy_port_base + 2 is valid,
         *      The PM is operating in Dual lane mode
         */
        if (P2L(unit, port_base + 1) != -1) {
            num_ports = 4;
            if (P2L(unit, port_base + 2) == -1) {
                num_ports = 2;
            }
        } else if (P2L(unit, port_base + 2) != -1) {
            num_ports = 2;
        }
    }
    return num_ports;
}

static int
_mmu_config_init(int unit)
{
    int ioerr = 0;
    int jumbo_frame_cells, default_mtu_cells, max_packet_cells;
    int idx, baseidx, pipe, port, mport, p, numq, qbase;
    int pool_resume = 0, pool_pg_headroom;
    int prigroup_headroom = 0, prigroup_guarantee = 0, queue_guarantee = 0;
    int limit, buf_pool_size, buf_pool_total, buf_headroom, rqe_queue_guarantee;
    int egr_rsvd = 0, asf_rsvd = 0, lossless = 1;
    int total_pool_size, egr_shared_total, rlimit, rqlen;
    int total_mcq_entry, mcq_entry_reserved = 0;
    int queue_grp_guarantee, rqe_entry_shared_total;
    int ing_rsvd_cells[TH_PIPES_PER_DEV];
    int egr_rsvd_cells[TH_PIPES_PER_DEV];
    int asf_rsvd_cells[TH_PIPES_PER_DEV];
    int min_pm_id, max_pm_id, num_ports;
    int oversub_mode = FALSE;
    uint32_t fval, rval;
    cdk_pbmp_t mmu_pbmp, oversub_pbmp;
    CFAPFULLTHRESHOLDSETr_t cfap_thr_set;
    CFAPFULLTHRESHOLDRESETr_t cfap_thr_rst;
    CFAPBANKFULLr_t cfap_bank_full;
    THDI_PORT_PRI_GRPr_t thdi_pri_grp;
    THDI_GLOBAL_HDRM_LIMITr_t thdi_glb_hrlimit;
    THDI_BUFFER_CELL_LIMIT_SPr_t thdi_buf_limit_sp;
    THDI_CELL_RESET_LIMIT_OFFSET_SPr_t thdi_rst_limit_offset_sp;
    THDI_HDRM_BUFFER_CELL_LIMIT_HPr_t thdi_hdrbuf_limit_hp;
    THDI_BUFFER_CELL_LIMIT_PUBLIC_POOLr_t thdi_buf_limit_pub_pool;
    OP_THDU_CONFIGr_t thdu_cfg;
    MMU_THDM_DB_DEVICE_THR_CONFIGr_t thdm_dev_thr_cfg;
    MMU_THDM_DB_POOL_SHARED_LIMITr_t db_shared_limit;
    MMU_THDM_DB_POOL_YELLOW_SHARED_LIMITr_t db_y_shared_limit;
    MMU_THDM_DB_POOL_RED_SHARED_LIMITr_t db_r_shared_limit;
    MMU_THDM_DB_POOL_RESUME_LIMITr_t db_resume_limit;
    MMU_THDM_DB_POOL_YELLOW_RESUME_LIMITr_t db_y_resume_limit;
    MMU_THDM_DB_POOL_RED_RESUME_LIMITr_t db_r_resume_limit;
    MMU_THDM_MCQE_POOL_SHARED_LIMITr_t mcqe_shared_limit;
    MMU_THDM_MCQE_POOL_YELLOW_SHARED_LIMITr_t mcqe_y_shared_limit;
    MMU_THDM_MCQE_POOL_RED_SHARED_LIMITr_t mcqe_r_shared_limit;
    MMU_THDM_MCQE_POOL_RESUME_LIMITr_t mcqe_resume_limit;
    MMU_THDM_MCQE_POOL_YELLOW_RESUME_LIMITr_t mcqe_y_resume_limit;
    MMU_THDM_MCQE_POOL_RED_RESUME_LIMITr_t mcqe_r_resume_limit;
    MMU_THDU_CONFIG_QGROUPm_t cfg_qgrp;
    MMU_THDU_OFFSET_QGROUPm_t offset_qgrp;
    THDI_PORT_PG_SPIDr_t pg_spid;
    THDI_PORT_SP_CONFIGm_t sp0_cfg, sp123_cfg;
    THDI_INPUT_PORT_XON_ENABLESr_t xon_en;
    THDI_PORT_MAX_PKT_SIZEr_t max_pkt_size;
    THDI_PORT_PG_CONFIGm_t pglt7_cfg, pg7_cfg;
    MMU_THDM_DB_QUEUE_CONFIGm_t db_q_cfg;
    MMU_THDM_DB_QUEUE_OFFSETm_t db_q_offset;
    MMU_THDM_MCQE_QUEUE_CONFIGm_t mcqe_q_cfg;
    MMU_THDM_MCQE_QUEUE_OFFSETm_t mcqe_q_offset;
    MMU_THDM_DB_QUEUE_RESUME_OFFSET_PROFILE_YELLOWr_t db_prf_yellow;
    MMU_THDM_DB_QUEUE_RESUME_OFFSET_PROFILE_REDr_t db_prf_red;
    MMU_THDM_MCQE_QUEUE_RESUME_OFFSET_PROFILE_YELLOWr_t mcqe_prf_yellow;
    MMU_THDM_MCQE_QUEUE_RESUME_OFFSET_PROFILE_REDr_t mcqe_prf_red;
    MMU_THDM_DB_PORTSP_CONFIGm_t db_psp_cfg;
    MMU_THDM_MCQE_PORTSP_CONFIGm_t mcqe_psp_cfg;
    MMU_THDU_CONFIG_QUEUEm_t cfg_q;
    MMU_THDU_OFFSET_QUEUEm_t offset_q;
    MMU_THDU_Q_TO_QGRP_MAPm_t q2qgrp_map;
    MMU_THDU_CONFIG_PORTm_t cfg_port;
    MMU_THDU_RESUME_PORTm_t resume_port;
    MMU_THDR_DB_LIMIT_MIN_PRIQr_t db_min_priq;
    MMU_THDR_DB_CONFIG1_PRIQr_t db_cfg1_priq;
    MMU_THDR_DB_LIMIT_COLOR_PRIQr_t db_clr_priq;
    MMU_THDR_DB_CONFIG_PRIQr_t db_cfg_priq;
    MMU_THDR_DB_RESET_OFFSET_COLOR_PRIQr_t db_rst_offset_clr_priq;
    MMU_THDR_QE_LIMIT_MIN_PRIQr_t qe_min_priq;
    MMU_THDR_QE_CONFIG1_PRIQr_t qe_cfg1_priq;
    MMU_THDR_QE_LIMIT_COLOR_PRIQr_t qe_clr_priq;
    MMU_THDR_QE_CONFIG_PRIQr_t qe_cfg_priq;
    MMU_THDR_QE_RESET_OFFSET_COLOR_PRIQr_t qe_rst_offset_clr_priq;
    MMU_THDR_DB_CONFIG_SPr_t db_cfg_sp;
    MMU_THDR_DB_SP_SHARED_LIMITr_t db_sp_shared_limit;
    MMU_THDR_DB_RESUME_COLOR_LIMIT_SPr_t db_resume_clr_limit;
    MMU_THDR_QE_CONFIG_SPr_t qe_cfg_sp;
    MMU_THDR_QE_SHARED_COLOR_LIMIT_SPr_t qe_shared_clr_limit;
    MMU_THDR_QE_RESUME_COLOR_LIMIT_SPr_t qe_resume_clr_limit;

    bcm56960_a0_oversub_map_get(unit, &oversub_pbmp);
    if (CDK_PBMP_NOT_NULL(oversub_pbmp)) {
        oversub_mode = TRUE;
    }

    for (pipe = 0; pipe < TH_PIPES_PER_DEV; pipe++) {
        ing_rsvd_cells[pipe] = 0;
        egr_rsvd_cells[pipe] = 0;
        asf_rsvd_cells[pipe] = 0;
        min_pm_id = pipe * TH_PBLKS_PER_PIPE;
        max_pm_id = min_pm_id + TH_PBLKS_PER_PIPE;
        for (idx = min_pm_id; idx < max_pm_id; idx++) {
            num_ports = _ports_per_pm_get(unit, idx);
            if (num_ports == 1) {
                ing_rsvd_cells[pipe] += ((0 + 0 + 0) * num_ports);
                egr_rsvd_cells[pipe] += (0 * num_ports);
                if (oversub_mode) {
                    asf_rsvd_cells[pipe] += 48;
                } else {
                    asf_rsvd_cells[pipe] += 20;
                }
            } else if (num_ports == 2) {
                if (oversub_mode) {
                    asf_rsvd_cells[pipe] += 96;
                } else {
                    asf_rsvd_cells[pipe] += 40;
                }
            } else if (num_ports == 4) {
                if (oversub_mode) {
                    asf_rsvd_cells[pipe] += 168;
                } else {
                    asf_rsvd_cells[pipe] += 80;
                }
            } else {
                continue;
            }
        }
    }

    jumbo_frame_cells = MMU_CFG_MMU_BYTES_TO_CELLS(TH_MMU_JUMBO_FRAME_BYTES + 
                                                   TH_MMU_PACKET_HEADER_BYTES,
                                                   TH_MMU_BYTES_PER_CELL);
    default_mtu_cells = MMU_CFG_MMU_BYTES_TO_CELLS(TH_MMU_DEFAULT_MTU_BYTES + 
                                                   TH_MMU_PACKET_HEADER_BYTES,
                                                   TH_MMU_BYTES_PER_CELL);
    pool_resume = 2 * jumbo_frame_cells;

    max_packet_cells = MMU_CFG_MMU_BYTES_TO_CELLS(TH_MMU_MAX_PACKET_BYTES +
                                                  TH_MMU_PACKET_HEADER_BYTES,
                                                  TH_MMU_BYTES_PER_CELL);

    total_pool_size = TH_MMU_TOTAL_CELLS_PER_XPE;
    asf_rsvd = asf_rsvd_cells[0];
    egr_rsvd = egr_rsvd_cells[0];
    egr_shared_total = total_pool_size - ((lossless) ? 88 : 0) - 
                       ((asf_rsvd + egr_rsvd) << 1);
    rqe_entry_shared_total = TH_MMU_RQE_ENTRY_PER_XPE - 88;


    fval = TH_MMU_PHYSICAL_CELLS_PER_XPE - TH_MMU_RSVD_CELLS_CFAP_PER_XPE;
    CFAPFULLTHRESHOLDSETr_CLR(cfap_thr_set);
    CFAPFULLTHRESHOLDSETr_CFAPFULLSETPOINTf_SET(cfap_thr_set, fval);
    for (baseidx = 0; baseidx < TH_NUM_XPES; baseidx++) {
        ioerr += WRITE_CFAPFULLTHRESHOLDSETr(unit, baseidx, cfap_thr_set);
    }

    CFAPFULLTHRESHOLDRESETr_CLR(cfap_thr_rst);
    CFAPFULLTHRESHOLDRESETr_CFAPFULLRESETPOINTf_SET(cfap_thr_rst, 
                                            fval - (2 * jumbo_frame_cells));
    for (baseidx = 0; baseidx < TH_NUM_XPES; baseidx++) {
        ioerr += WRITE_CFAPFULLTHRESHOLDRESETr(unit, baseidx, cfap_thr_rst);
    }

    CFAPBANKFULLr_CLR(cfap_bank_full);
    CFAPBANKFULLr_LIMITf_SET(cfap_bank_full, 767);
    for (idx = 0; idx < 15; idx++) {
        for (baseidx = 0; baseidx < TH_NUM_XPES; baseidx++) {
            ioerr += WRITE_CFAPBANKFULLr(unit, baseidx, idx, cfap_bank_full);
        }
    }

    /* All priorities use the default priority group */
    rval = 0;
    for (idx = 0; idx < 8; idx++) {
        /* Three bits per priority */
        rval |= MMU_DEFAULT_PG << (3 * idx);
    }
    THDI_PORT_PRI_GRPr_SET(thdi_pri_grp, rval);

    /* internal priority to priority group mapping */
    bcm56960_a0_all_pbmp_get(unit, &mmu_pbmp);
    CDK_PBMP_ITER(mmu_pbmp, port) {
        mport = P2M(unit, port);
        if (mport >= 0) {
            for (idx = 0; idx < 2; idx++) {
                ioerr += WRITE_THDI_PORT_PRI_GRPr(unit, mport, idx, 
                                                  thdi_pri_grp);
            }
        }
        prigroup_headroom += _default_pg_headroom(unit, port, 1);
        prigroup_guarantee += default_mtu_cells;
        mcq_entry_reserved += 4 * bcm56960_a0_num_cosq(unit, port);
    }

    /* Input thresholds */
    THDI_GLOBAL_HDRM_LIMITr_CLR(thdi_glb_hrlimit);
    THDI_GLOBAL_HDRM_LIMITr_GLOBAL_HDRM_LIMITf_SET(thdi_glb_hrlimit, 
                                                   max_packet_cells);
    ioerr += WRITE_THDI_GLOBAL_HDRM_LIMITr(unit, 0, thdi_glb_hrlimit);

    /* pool 0 is utilized */
    buf_headroom = 2 * max_packet_cells;
    buf_pool_size = 10000;
    buf_pool_total = buf_pool_size * TH_MMU_TOTAL_CELLS_PER_XPE / 10000;
    rqe_queue_guarantee = 8;
    queue_guarantee = rqe_queue_guarantee * 11;
    total_mcq_entry = (buf_pool_size * TH_MMU_MCQ_ENTRY_PER_XPE) / 10000;
    queue_grp_guarantee = 0;

    pool_pg_headroom = prigroup_headroom / 2;

    limit = buf_pool_total - ((prigroup_guarantee / 2 + pool_pg_headroom 
                               + asf_rsvd) + buf_headroom);
    THDI_BUFFER_CELL_LIMIT_SPr_CLR(thdi_buf_limit_sp);
    THDI_BUFFER_CELL_LIMIT_SPr_LIMITf_SET(thdi_buf_limit_sp, limit);
    for (baseidx = 0; baseidx < TH_NUM_LAYERS; baseidx++) {
        ioerr += WRITE_THDI_BUFFER_CELL_LIMIT_SPr(unit, baseidx, 0, 
                                                  thdi_buf_limit_sp);
    }

    THDI_CELL_RESET_LIMIT_OFFSET_SPr_CLR(thdi_rst_limit_offset_sp);
    THDI_CELL_RESET_LIMIT_OFFSET_SPr_OFFSETf_SET(thdi_rst_limit_offset_sp, 
                                                 pool_resume);
    for (baseidx = 0; baseidx < TH_NUM_LAYERS; baseidx++) {
        ioerr += WRITE_THDI_CELL_RESET_LIMIT_OFFSET_SPr(unit, baseidx, 0, 
                                                     thdi_rst_limit_offset_sp);
    }

    THDI_HDRM_BUFFER_CELL_LIMIT_HPr_CLR(thdi_hdrbuf_limit_hp);
    THDI_HDRM_BUFFER_CELL_LIMIT_HPr_LIMITf_SET(thdi_hdrbuf_limit_hp, 
                                               pool_pg_headroom / 2);
    for (baseidx = 0; baseidx < TH_NUM_LAYERS; baseidx++) {
        ioerr += WRITE_THDI_HDRM_BUFFER_CELL_LIMIT_HPr(unit, baseidx, 0, 
                                                       thdi_hdrbuf_limit_hp);
    }


    THDI_BUFFER_CELL_LIMIT_PUBLIC_POOLr_CLR(thdi_buf_limit_pub_pool);
    for (baseidx = 0; baseidx < TH_NUM_LAYERS; baseidx++) {
        ioerr += WRITE_THDI_BUFFER_CELL_LIMIT_PUBLIC_POOLr(unit, baseidx, 
                                                      thdi_buf_limit_pub_pool);
    }
 
    /* output thresholds */
    ioerr += READ_OP_THDU_CONFIGr(unit, 0, &thdu_cfg);
    OP_THDU_CONFIGr_ENABLE_QUEUE_AND_GROUP_TICKETf_SET(thdu_cfg, 1);
    OP_THDU_CONFIGr_ENABLE_UPDATE_COLOR_RESUMEf_SET(thdu_cfg, 0);
    OP_THDU_CONFIGr_MOP_POLICY_1Bf_SET(thdu_cfg, 1);
    OP_THDU_CONFIGr_MOP_POLICY_1Af_SET(thdu_cfg, 0);
    ioerr += WRITE_OP_THDU_CONFIGr(unit, 0, thdu_cfg);

    ioerr += READ_MMU_THDM_DB_DEVICE_THR_CONFIGr(unit, 0, &thdm_dev_thr_cfg);
    MMU_THDM_DB_DEVICE_THR_CONFIGr_MOP_POLICYf_SET(thdm_dev_thr_cfg, 1);
    ioerr += WRITE_MMU_THDM_DB_DEVICE_THR_CONFIGr(unit, 0, thdm_dev_thr_cfg);
  
    /* per service pool settings, only pool 0 is utilized */ 
    limit = buf_pool_total - ((queue_guarantee >> 1) + asf_rsvd) - 44;
    if (limit <= 0) {
        limit = 0;
    }

    MMU_THDM_DB_POOL_SHARED_LIMITr_CLR(db_shared_limit);
    MMU_THDM_DB_POOL_SHARED_LIMITr_SHARED_LIMITf_SET(db_shared_limit, limit);
    ioerr += WRITE_MMU_THDM_DB_POOL_SHARED_LIMITr(unit, 0, 0, db_shared_limit);

    MMU_THDM_DB_POOL_YELLOW_SHARED_LIMITr_CLR(db_y_shared_limit);
    MMU_THDM_DB_POOL_YELLOW_SHARED_LIMITr_YELLOW_SHARED_LIMITf_SET(
                                                    db_y_shared_limit, limit/8);
    ioerr += WRITE_MMU_THDM_DB_POOL_YELLOW_SHARED_LIMITr(unit, 0, 0,  
                                                         db_y_shared_limit);

    MMU_THDM_DB_POOL_RED_SHARED_LIMITr_CLR(db_r_shared_limit);
    MMU_THDM_DB_POOL_RED_SHARED_LIMITr_RED_SHARED_LIMITf_SET(db_r_shared_limit, 
                                                         limit/8);
    ioerr += WRITE_MMU_THDM_DB_POOL_RED_SHARED_LIMITr(unit, 0, 0, 
                                                      db_r_shared_limit);

    MMU_THDM_DB_POOL_RESUME_LIMITr_CLR(db_resume_limit);
    MMU_THDM_DB_POOL_RESUME_LIMITr_RESUME_LIMITf_SET(db_resume_limit, limit/8);
    ioerr += WRITE_MMU_THDM_DB_POOL_RESUME_LIMITr(unit, 0, 0, db_resume_limit);

    MMU_THDM_DB_POOL_YELLOW_RESUME_LIMITr_CLR(db_y_resume_limit);
    MMU_THDM_DB_POOL_YELLOW_RESUME_LIMITr_YELLOW_RESUME_LIMITf_SET(
                                                    db_y_resume_limit, limit/8);
    ioerr += WRITE_MMU_THDM_DB_POOL_YELLOW_RESUME_LIMITr(unit, 0, 0, 
                                                         db_y_resume_limit);

    MMU_THDM_DB_POOL_RED_RESUME_LIMITr_CLR(db_r_resume_limit);
    MMU_THDM_DB_POOL_RED_RESUME_LIMITr_RED_RESUME_LIMITf_SET(db_r_resume_limit,
                                                             limit/8);
    ioerr += WRITE_MMU_THDM_DB_POOL_RED_RESUME_LIMITr(unit, 0, 0, 
                                                      db_r_resume_limit);
  
    /* mcq entries */
    limit = total_mcq_entry - mcq_entry_reserved;
    MMU_THDM_MCQE_POOL_SHARED_LIMITr_CLR(mcqe_shared_limit);
    MMU_THDM_MCQE_POOL_SHARED_LIMITr_SHARED_LIMITf_SET(mcqe_shared_limit, 
                                                       limit/4);
    ioerr += WRITE_MMU_THDM_MCQE_POOL_SHARED_LIMITr(unit, 0, 0, 
                                                    mcqe_shared_limit);

    MMU_THDM_MCQE_POOL_YELLOW_SHARED_LIMITr_CLR(mcqe_y_shared_limit);
    MMU_THDM_MCQE_POOL_YELLOW_SHARED_LIMITr_YELLOW_SHARED_LIMITf_SET(
                                                  mcqe_y_shared_limit, limit/8);
    ioerr += WRITE_MMU_THDM_MCQE_POOL_YELLOW_SHARED_LIMITr(unit, 0, 0, 
                                                           mcqe_y_shared_limit);

    MMU_THDM_MCQE_POOL_RED_SHARED_LIMITr_CLR(mcqe_r_shared_limit);
    MMU_THDM_MCQE_POOL_RED_SHARED_LIMITr_RED_SHARED_LIMITf_SET(
                                                  mcqe_r_shared_limit, limit/8);
    ioerr += WRITE_MMU_THDM_MCQE_POOL_RED_SHARED_LIMITr(unit, 0, 0, 
                                                        mcqe_r_shared_limit);

    MMU_THDM_MCQE_POOL_RESUME_LIMITr_CLR(mcqe_resume_limit);
    MMU_THDM_MCQE_POOL_RESUME_LIMITr_RESUME_LIMITf_SET(mcqe_resume_limit, 
                                                       limit/8);
    ioerr += WRITE_MMU_THDM_MCQE_POOL_RESUME_LIMITr(unit, 0, 0, 
                                                    mcqe_resume_limit);

    MMU_THDM_MCQE_POOL_YELLOW_RESUME_LIMITr_CLR(mcqe_y_resume_limit);
    MMU_THDM_MCQE_POOL_YELLOW_RESUME_LIMITr_YELLOW_RESUME_LIMITf_SET(
                                                  mcqe_y_resume_limit, limit/8);
    ioerr += WRITE_MMU_THDM_MCQE_POOL_YELLOW_RESUME_LIMITr(unit, 0, 0, 
                                                           mcqe_y_resume_limit);

    MMU_THDM_MCQE_POOL_RED_RESUME_LIMITr_CLR(mcqe_r_resume_limit);
    MMU_THDM_MCQE_POOL_RED_RESUME_LIMITr_RED_RESUME_LIMITf_SET(
                                                  mcqe_r_resume_limit, limit/8);
    ioerr += WRITE_MMU_THDM_MCQE_POOL_RED_RESUME_LIMITr(unit, 0, 0, 
                                                        mcqe_r_resume_limit);
 
    /* configure Q-groups */
    for (baseidx = 0; baseidx < TH_NUM_PIPES; baseidx++) {
        MMU_THDU_CONFIG_QGROUPm_CLR(cfg_qgrp);
        MMU_THDU_CONFIG_QGROUPm_Q_MIN_LIMIT_CELLf_SET(cfg_qgrp, 
                                                      queue_grp_guarantee);
        MMU_THDU_CONFIG_QGROUPm_Q_SHARED_LIMIT_CELLf_SET(cfg_qgrp, 
                                                         egr_shared_total);
        MMU_THDU_CONFIG_QGROUPm_LIMIT_RED_CELLf_SET(cfg_qgrp, 
                                                    egr_shared_total/8);
        MMU_THDU_CONFIG_QGROUPm_LIMIT_YELLOW_CELLf_SET(cfg_qgrp,
                                                       egr_shared_total/8);
        for (idx = 0; idx <= MMU_THDU_CONFIG_QGROUPm_MAX; idx++) {
            ioerr += WRITE_MMU_THDU_CONFIG_QGROUPm(unit, baseidx, idx, 
                                                   cfg_qgrp);
        }

        MMU_THDU_OFFSET_QGROUPm_CLR(offset_qgrp);
        MMU_THDU_OFFSET_QGROUPm_RESET_OFFSET_CELLf_SET(offset_qgrp, 16/8);
        MMU_THDU_OFFSET_QGROUPm_RESET_OFFSET_YELLOW_CELLf_SET(offset_qgrp, 
                                                              16/8);
        MMU_THDU_OFFSET_QGROUPm_RESET_OFFSET_RED_CELLf_SET(offset_qgrp, 16/8);
        for (idx = 0; idx <= MMU_THDU_OFFSET_QGROUPm_MAX; idx++) {
            ioerr += WRITE_MMU_THDU_OFFSET_QGROUPm(unit, baseidx, idx, 
                                                   offset_qgrp);
        }        
    }

    /* Input port per port settings */
    THDI_PORT_PG_SPIDr_CLR(pg_spid);
    
    THDI_PORT_SP_CONFIGm_CLR(sp123_cfg);

    THDI_PORT_SP_CONFIGm_CLR(sp0_cfg);
    THDI_PORT_SP_CONFIGm_PORT_SP_RESUME_LIMITf_SET(sp0_cfg, 
                                                   (total_pool_size - 16)/8);
    THDI_PORT_SP_CONFIGm_PORT_SP_MAX_LIMITf_SET(sp0_cfg, total_pool_size);

    THDI_INPUT_PORT_XON_ENABLESr_CLR(xon_en);
    THDI_INPUT_PORT_XON_ENABLESr_INPUT_PORT_RX_ENABLEf_SET(xon_en, 1);
    THDI_INPUT_PORT_XON_ENABLESr_PORT_PRI_XON_ENABLEf_SET(xon_en, ((1<<16)-1));
    THDI_INPUT_PORT_XON_ENABLESr_PORT_PAUSE_ENABLEf_SET(xon_en, 1);

    THDI_PORT_PG_CONFIGm_CLR(pglt7_cfg);
    
    THDI_PORT_PG_CONFIGm_CLR(pg7_cfg);
    THDI_PORT_PG_CONFIGm_PG_MIN_LIMITf_SET(pg7_cfg, default_mtu_cells);
    THDI_PORT_PG_CONFIGm_PG_SHARED_DYNAMICf_SET(pg7_cfg, 1);
    THDI_PORT_PG_CONFIGm_PG_SHARED_LIMITf_SET(pg7_cfg, 8);
    THDI_PORT_PG_CONFIGm_PG_GBL_HDRM_ENf_SET(pg7_cfg, 1);
    THDI_PORT_PG_CONFIGm_PG_RESET_OFFSETf_SET(pg7_cfg, 16/8);
    THDI_PORT_PG_CONFIGm_PG_RESET_FLOORf_SET(pg7_cfg, 0);

    CDK_PBMP_ITER(mmu_pbmp, port) {
        mport = P2M(unit, port);
        if (mport >= 0) {
            ioerr += WRITE_THDI_PORT_PG_SPIDr(unit, mport, pg_spid);

            pipe = bcm56960_a0_port_pipe_get(unit, port);
            /* Per port per pool settings */
            for (p = 0; p < TH_MMU_NUM_POOL; p++) {
                idx = (mport & 0x3f) * TH_MMU_NUM_POOL + p;
                if (p == 0) {
                    ioerr += WRITE_THDI_PORT_SP_CONFIGm(unit, pipe, idx, 
                                                        sp0_cfg);
                } else {
                    ioerr += WRITE_THDI_PORT_SP_CONFIGm(unit, pipe, idx,
                                                        sp123_cfg);
                }
            }

            ioerr += WRITE_THDI_INPUT_PORT_XON_ENABLESr(unit, mport, xon_en);

            /* Input port per port per priority group settings */
            for (p = 0; p < TH_MMU_NUM_PG; p++) {
                idx = (mport & 0x3f) * TH_MMU_NUM_PG + p;
                if (p == 7) {
                    THDI_PORT_PG_CONFIGm_PG_HDRM_LIMITf_SET(pg7_cfg, 
                                           _default_pg_headroom(unit, port, 1));
                    ioerr += WRITE_THDI_PORT_PG_CONFIGm(unit, pipe, idx, 
                                                        pg7_cfg);
                } else {
                    ioerr += WRITE_THDI_PORT_PG_CONFIGm(unit, pipe, idx, 
                                                        pglt7_cfg);
                }
            }
        }
    }

    THDI_PORT_MAX_PKT_SIZEr_CLR(max_pkt_size);
    THDI_PORT_MAX_PKT_SIZEr_PORT_MAX_PKT_SIZEf_SET(max_pkt_size, 
                                                   max_packet_cells);
    ioerr += WRITE_THDI_PORT_MAX_PKT_SIZEr(unit, 0, max_pkt_size);


    /***********************************
    * THDO
 */
    /* Output port per port per queue setting for regular multicast queue */
    MMU_THDM_DB_QUEUE_CONFIGm_CLR(db_q_cfg);
    MMU_THDM_DB_QUEUE_CONFIGm_Q_MIN_LIMITf_SET(db_q_cfg, 0);
    MMU_THDM_DB_QUEUE_CONFIGm_Q_SHARED_LIMITf_SET(db_q_cfg, egr_shared_total);
    MMU_THDM_DB_QUEUE_CONFIGm_YELLOW_SHARED_LIMITf_SET(db_q_cfg, 
                                                       egr_shared_total/8);
    MMU_THDM_DB_QUEUE_CONFIGm_RED_SHARED_LIMITf_SET(db_q_cfg, 
                                                    egr_shared_total/8);
    MMU_THDM_DB_QUEUE_CONFIGm_Q_SPIDf_SET(db_q_cfg, 0);
    MMU_THDM_DB_QUEUE_OFFSETm_CLR(db_q_offset);
    MMU_THDM_DB_QUEUE_OFFSETm_RESUME_OFFSETf_SET(db_q_offset, 
                                                 (default_mtu_cells * 2)/8);
    MMU_THDM_DB_QUEUE_OFFSETm_YELLOW_RESUME_OFFSET_PROFILE_SELf_SET(db_q_offset,
                                                                    0);
    MMU_THDM_DB_QUEUE_OFFSETm_RED_RESUME_OFFSET_PROFILE_SELf_SET(db_q_offset, 
                                                                 0);
    MMU_THDM_MCQE_QUEUE_CONFIGm_CLR(mcqe_q_cfg);
    MMU_THDM_MCQE_QUEUE_CONFIGm_Q_MIN_LIMITf_SET(mcqe_q_cfg, 4/4);    
    limit = total_mcq_entry - mcq_entry_reserved;
    MMU_THDM_MCQE_QUEUE_CONFIGm_Q_SHARED_LIMITf_SET(mcqe_q_cfg, limit/4);
    MMU_THDM_MCQE_QUEUE_CONFIGm_YELLOW_SHARED_LIMITf_SET(mcqe_q_cfg, limit/8);
    MMU_THDM_MCQE_QUEUE_CONFIGm_RED_SHARED_LIMITf_SET(mcqe_q_cfg, limit/8);
    MMU_THDM_MCQE_QUEUE_CONFIGm_Q_SPIDf_SET(mcqe_q_cfg, 0);
    MMU_THDM_MCQE_QUEUE_OFFSETm_CLR(mcqe_q_offset);
    MMU_THDM_MCQE_QUEUE_OFFSETm_RESUME_OFFSETf_SET(mcqe_q_offset, 1);
    MMU_THDM_MCQE_QUEUE_OFFSETm_YELLOW_RESUME_OFFSET_PROFILE_SELf_SET(
                                                              mcqe_q_offset, 0);
    MMU_THDM_MCQE_QUEUE_OFFSETm_RED_RESUME_OFFSET_PROFILE_SELf_SET(
                                                              mcqe_q_offset, 0);
    MMU_THDM_DB_PORTSP_CONFIGm_CLR(db_psp_cfg);
    limit = total_pool_size;
    rlimit = limit - (default_mtu_cells * 2);
    MMU_THDM_DB_PORTSP_CONFIGm_SHARED_LIMITf_SET(db_psp_cfg, limit);
    MMU_THDM_DB_PORTSP_CONFIGm_RED_SHARED_LIMITf_SET(db_psp_cfg, limit/8);
    MMU_THDM_DB_PORTSP_CONFIGm_YELLOW_SHARED_LIMITf_SET(db_psp_cfg, limit/8);
    MMU_THDM_DB_PORTSP_CONFIGm_SHARED_LIMIT_ENABLEf_SET(db_psp_cfg, !lossless);
    MMU_THDM_DB_PORTSP_CONFIGm_SHARED_RESUME_LIMITf_SET(db_psp_cfg, rlimit/8);
    MMU_THDM_DB_PORTSP_CONFIGm_YELLOW_RESUME_LIMITf_SET(db_psp_cfg, rlimit/8);
    MMU_THDM_DB_PORTSP_CONFIGm_RED_RESUME_LIMITf_SET(db_psp_cfg, rlimit/8);
    MMU_THDM_MCQE_PORTSP_CONFIGm_CLR(mcqe_psp_cfg);
    limit = total_mcq_entry;
    MMU_THDM_MCQE_PORTSP_CONFIGm_SHARED_LIMITf_SET(mcqe_psp_cfg, (limit/4)-1);
    MMU_THDM_MCQE_PORTSP_CONFIGm_YELLOW_SHARED_LIMITf_SET(mcqe_psp_cfg, 
                                                          (limit/8)-1);
    MMU_THDM_MCQE_PORTSP_CONFIGm_RED_SHARED_LIMITf_SET(mcqe_psp_cfg, 
                                                       (limit/8)-1);
    MMU_THDM_MCQE_PORTSP_CONFIGm_SHARED_LIMIT_ENABLEf_SET(mcqe_psp_cfg, 
                                                          !lossless);
    MMU_THDM_MCQE_PORTSP_CONFIGm_SHARED_RESUME_LIMITf_SET(mcqe_psp_cfg, 
                                                          (limit/8)-2);
    MMU_THDM_MCQE_PORTSP_CONFIGm_YELLOW_RESUME_LIMITf_SET(mcqe_psp_cfg,
                                                          (limit/8)-2);
    MMU_THDM_MCQE_PORTSP_CONFIGm_RED_RESUME_LIMITf_SET(mcqe_psp_cfg,
                                                       (limit/8)-2);

    CDK_PBMP_ITER(mmu_pbmp, port) {
        pipe = bcm56960_a0_port_pipe_get(unit, port);
        numq = bcm56960_a0_num_cosq(unit, port);
        qbase = bcm56960_a0_num_cosq_base(unit, port);
        if (numq == 0) {
            continue;
        }
        for (idx = 0; idx < numq; idx++) {
            ioerr += WRITE_MMU_THDM_DB_QUEUE_CONFIGm(unit, pipe, qbase+idx, 
                                                     db_q_cfg);
            ioerr += WRITE_MMU_THDM_DB_QUEUE_OFFSETm(unit, pipe, qbase+idx,
                                                     db_q_offset);
            ioerr += WRITE_MMU_THDM_MCQE_QUEUE_CONFIGm(unit, pipe, qbase+idx,
                                                       mcqe_q_cfg);
            ioerr += WRITE_MMU_THDM_MCQE_QUEUE_OFFSETm(unit, pipe, qbase+idx,
                                                       mcqe_q_offset);
        }

        /* Per port per pool, pool 0 is utilized */
        mport = P2M(unit, port);
        if (mport < 0) {
            continue;
        }
        idx = (mport & 0x3f) + 0 * 34;
        ioerr += WRITE_MMU_THDM_DB_PORTSP_CONFIGm(unit, pipe, idx, db_psp_cfg);
        ioerr += WRITE_MMU_THDM_MCQE_PORTSP_CONFIGm(unit, pipe, idx, 
                                                    mcqe_psp_cfg);
    }

    MMU_THDM_DB_QUEUE_RESUME_OFFSET_PROFILE_YELLOWr_SET(db_prf_yellow, 
                                                       (egr_shared_total-16)/8);
    ioerr += WRITE_MMU_THDM_DB_QUEUE_RESUME_OFFSET_PROFILE_YELLOWr(unit, 0, 0, 
                                                                 db_prf_yellow);

    MMU_THDM_DB_QUEUE_RESUME_OFFSET_PROFILE_REDr_SET(db_prf_red, 
                                                     (egr_shared_total-16)/8);
    ioerr += WRITE_MMU_THDM_DB_QUEUE_RESUME_OFFSET_PROFILE_REDr(unit, 0, 0,  
                                                                db_prf_red);

    MMU_THDM_MCQE_QUEUE_RESUME_OFFSET_PROFILE_YELLOWr_SET(mcqe_prf_yellow,
                                                       (egr_shared_total-16)/8);
    ioerr += WRITE_MMU_THDM_MCQE_QUEUE_RESUME_OFFSET_PROFILE_YELLOWr(unit, 0, 0, 
                                                               mcqe_prf_yellow);

    MMU_THDM_MCQE_QUEUE_RESUME_OFFSET_PROFILE_REDr_SET(mcqe_prf_red,
                                                       (egr_shared_total-16)/8);
    ioerr += WRITE_MMU_THDM_MCQE_QUEUE_RESUME_OFFSET_PROFILE_REDr(unit, 0, 0,  
                                                                  mcqe_prf_red);


    /* Output port per port per queue setting for regular unicast queue */
    MMU_THDU_CONFIG_QUEUEm_CLR(cfg_q);
    MMU_THDU_CONFIG_QUEUEm_Q_MIN_LIMIT_CELLf_SET(cfg_q, 0);
    MMU_THDU_CONFIG_QUEUEm_Q_SHARED_LIMIT_CELLf_SET(cfg_q, egr_shared_total);
    MMU_THDU_CONFIG_QUEUEm_LIMIT_YELLOW_CELLf_SET(cfg_q, 16/8);
    MMU_THDU_CONFIG_QUEUEm_LIMIT_RED_CELLf_SET(cfg_q, 16/8);
    MMU_THDU_OFFSET_QUEUEm_CLR(offset_q);
    MMU_THDU_OFFSET_QUEUEm_RESET_OFFSET_CELLf_SET(offset_q, 16/8);
    MMU_THDU_OFFSET_QUEUEm_RESET_OFFSET_YELLOW_CELLf_SET(offset_q, 16/8);
    MMU_THDU_OFFSET_QUEUEm_RESET_OFFSET_RED_CELLf_SET(offset_q, 16/8);
    MMU_THDU_Q_TO_QGRP_MAPm_CLR(q2qgrp_map);
    MMU_THDU_Q_TO_QGRP_MAPm_Q_SPIDf_SET(q2qgrp_map, 0);
    MMU_THDU_Q_TO_QGRP_MAPm_Q_LIMIT_ENABLEf_SET(q2qgrp_map, 1);
    MMU_THDU_Q_TO_QGRP_MAPm_Q_COLOR_ENABLE_CELLf_SET(q2qgrp_map, 1);
    limit = buf_pool_total;
    MMU_THDU_CONFIG_PORTm_CLR(cfg_port);
    MMU_THDU_CONFIG_PORTm_SHARED_LIMITf_SET(cfg_port, limit);
    MMU_THDU_CONFIG_PORTm_YELLOW_LIMITf_SET(cfg_port, limit/8);
    MMU_THDU_CONFIG_PORTm_RED_LIMITf_SET(cfg_port, limit/8);
    MMU_THDU_RESUME_PORTm_CLR(resume_port);
    MMU_THDU_RESUME_PORTm_SHARED_RESUMEf_SET(resume_port, 
                                             (limit-(default_mtu_cells*2))/8);
    MMU_THDU_RESUME_PORTm_YELLOW_RESUMEf_SET(resume_port, 
                                             (limit-(default_mtu_cells*2))/8);
    MMU_THDU_RESUME_PORTm_RED_RESUMEf_SET(resume_port, 
                                          (limit-(default_mtu_cells*2))/8);   

    CDK_PBMP_ITER(mmu_pbmp, port) {
        pipe = bcm56960_a0_port_pipe_get(unit, port);
        numq = bcm56960_a0_num_uc_cosq(unit, port);
        qbase = bcm56960_a0_num_uc_cosq_base(unit, port);
        if (numq == 0) {
           continue;
        }
        for (idx = 0; idx < numq; idx ++) {
            ioerr += WRITE_MMU_THDU_CONFIG_QUEUEm(unit, pipe, qbase+idx, cfg_q);
            ioerr += WRITE_MMU_THDU_OFFSET_QUEUEm(unit, pipe, qbase+idx, 
                                                  offset_q); 
            ioerr += WRITE_MMU_THDU_Q_TO_QGRP_MAPm(unit, pipe, qbase+idx,
                                                   q2qgrp_map);
        }

        /* Per port per pool unicast, pool 0 is utiized */
        mport = P2M(unit, port);
        if (mport < 0) {
            continue;
        }
        idx = (mport & 0x3f) * 4 + 0;
        ioerr += WRITE_MMU_THDU_CONFIG_PORTm(unit, pipe, idx, cfg_port);
        ioerr += WRITE_MMU_THDU_RESUME_PORTm(unit, pipe, idx, resume_port);
    }

    /* RQE */
    MMU_THDR_DB_LIMIT_MIN_PRIQr_CLR(db_min_priq);
    MMU_THDR_DB_LIMIT_MIN_PRIQr_MIN_LIMITf_SET(db_min_priq, 8);

    MMU_THDR_DB_CONFIG1_PRIQr_CLR(db_cfg1_priq);
    MMU_THDR_DB_CONFIG1_PRIQr_SPIDf_SET(db_cfg1_priq, 0);
    MMU_THDR_DB_CONFIG1_PRIQr_LIMIT_ENABLEf_SET(db_cfg1_priq, 0);

    MMU_THDR_DB_LIMIT_COLOR_PRIQr_CLR(db_clr_priq);
    MMU_THDR_DB_LIMIT_COLOR_PRIQr_SHARED_RED_LIMITf_SET(db_clr_priq, 
                                                        egr_shared_total/8);
    MMU_THDR_DB_LIMIT_COLOR_PRIQr_SHARED_YELLOW_LIMITf_SET(db_clr_priq, 
                                                           egr_shared_total/8);

    MMU_THDR_DB_CONFIG_PRIQr_CLR(db_cfg_priq);
    MMU_THDR_DB_CONFIG_PRIQr_SHARED_LIMITf_SET(db_cfg_priq, egr_shared_total);
    MMU_THDR_DB_CONFIG_PRIQr_RESET_OFFSETf_SET(db_cfg_priq, 2);

    MMU_THDR_DB_RESET_OFFSET_COLOR_PRIQr_CLR(db_rst_offset_clr_priq);
    MMU_THDR_DB_RESET_OFFSET_COLOR_PRIQr_RESET_OFFSET_REDf_SET(
                             db_rst_offset_clr_priq, (default_mtu_cells * 2)/8);
    MMU_THDR_DB_RESET_OFFSET_COLOR_PRIQr_RESET_OFFSET_YELLOWf_SET(
                             db_rst_offset_clr_priq, (default_mtu_cells * 2)/8);

    MMU_THDR_QE_LIMIT_MIN_PRIQr_CLR(qe_min_priq);
    MMU_THDR_QE_LIMIT_MIN_PRIQr_MIN_LIMITf_SET(qe_min_priq, 8/8);

    MMU_THDR_QE_CONFIG1_PRIQr_CLR(qe_cfg1_priq);
    MMU_THDR_QE_CONFIG1_PRIQr_SPIDf_SET(qe_cfg1_priq, 0);
    MMU_THDR_QE_CONFIG1_PRIQr_COLOR_LIMIT_DYNAMICf_SET(qe_cfg1_priq, 0);
    MMU_THDR_QE_CONFIG1_PRIQr_LIMIT_ENABLEf_SET(qe_cfg1_priq, 1);

    fval = (rqe_entry_shared_total + 8 * 11 - 1) / (8 * 11);
    MMU_THDR_QE_LIMIT_COLOR_PRIQr_CLR(qe_clr_priq);
    MMU_THDR_QE_LIMIT_COLOR_PRIQr_SHARED_RED_LIMITf_SET(qe_clr_priq, fval);
    MMU_THDR_QE_LIMIT_COLOR_PRIQr_SHARED_YELLOW_LIMITf_SET(qe_clr_priq, fval);

    MMU_THDR_QE_CONFIG_PRIQr_CLR(qe_cfg_priq);
    MMU_THDR_QE_CONFIG_PRIQr_SHARED_LIMITf_SET(qe_cfg_priq, fval);
    MMU_THDR_QE_CONFIG_PRIQr_RESET_OFFSETf_SET(qe_cfg_priq, 1);

    MMU_THDR_QE_RESET_OFFSET_COLOR_PRIQr_CLR(qe_rst_offset_clr_priq);
    MMU_THDR_QE_RESET_OFFSET_COLOR_PRIQr_RESET_OFFSET_REDf_SET(
                                   qe_rst_offset_clr_priq, default_mtu_cells/8);
    MMU_THDR_QE_RESET_OFFSET_COLOR_PRIQr_RESET_OFFSET_YELLOWf_SET(
                                   qe_rst_offset_clr_priq, default_mtu_cells/8);

    for (idx = 0; idx < TH_MMU_NUM_RQE_QUEUES; idx++) {
        ioerr += WRITE_MMU_THDR_DB_LIMIT_MIN_PRIQr(unit, 0, idx, db_min_priq);
        ioerr += WRITE_MMU_THDR_DB_CONFIG1_PRIQr(unit, 0, idx, db_cfg1_priq);
        ioerr += WRITE_MMU_THDR_DB_CONFIG_PRIQr(unit, 0, idx, db_cfg_priq);
        ioerr += WRITE_MMU_THDR_DB_LIMIT_COLOR_PRIQr(unit, 0, idx, db_clr_priq);
        ioerr += WRITE_MMU_THDR_DB_RESET_OFFSET_COLOR_PRIQr(unit, 0, idx, 
                                                        db_rst_offset_clr_priq);

        /* queue entry */
        ioerr += WRITE_MMU_THDR_QE_LIMIT_MIN_PRIQr(unit, 0, idx, qe_min_priq);
        ioerr += WRITE_MMU_THDR_QE_CONFIG1_PRIQr(unit, 0, idx, qe_cfg1_priq);
        ioerr += WRITE_MMU_THDR_QE_CONFIG_PRIQr(unit, 0, idx, qe_cfg_priq);
        ioerr += WRITE_MMU_THDR_QE_LIMIT_COLOR_PRIQr(unit, 0, idx, qe_clr_priq);
        ioerr += WRITE_MMU_THDR_QE_RESET_OFFSET_COLOR_PRIQr(unit, 0, idx, 
                                                        qe_rst_offset_clr_priq);
    }

    /* per pool RQE settings, pool idx 0 is utilized */
    limit = buf_pool_total - (queue_guarantee/2) - 44;
    MMU_THDR_DB_CONFIG_SPr_CLR(db_cfg_sp);
    MMU_THDR_DB_CONFIG_SPr_SHARED_LIMITf_SET(db_cfg_sp, limit);
    MMU_THDR_DB_CONFIG_SPr_RESUME_LIMITf_SET(db_cfg_sp, 
                                             (limit-(default_mtu_cells*2))/8);

    MMU_THDR_DB_SP_SHARED_LIMITr_CLR(db_sp_shared_limit);
    MMU_THDR_DB_SP_SHARED_LIMITr_SHARED_RED_LIMITf_SET(db_sp_shared_limit, 
                                               (limit-(default_mtu_cells*2))/8);
 
    MMU_THDR_DB_RESUME_COLOR_LIMIT_SPr_CLR(db_resume_clr_limit);
    MMU_THDR_DB_RESUME_COLOR_LIMIT_SPr_RESUME_RED_LIMITf_SET(
                          db_resume_clr_limit, (limit-(default_mtu_cells*2))/8);
    MMU_THDR_DB_RESUME_COLOR_LIMIT_SPr_RESUME_YELLOW_LIMITf_SET(
                          db_resume_clr_limit, (limit-(default_mtu_cells*2))/8);

    rqlen = rqe_entry_shared_total / 8;
    MMU_THDR_QE_CONFIG_SPr_CLR(qe_cfg_sp);
    MMU_THDR_QE_CONFIG_SPr_SHARED_LIMITf_SET(qe_cfg_sp, rqlen);
    MMU_THDR_QE_CONFIG_SPr_RESUME_LIMITf_SET(qe_cfg_sp, rqlen-1);

    MMU_THDR_QE_SHARED_COLOR_LIMIT_SPr_CLR(qe_shared_clr_limit);
    MMU_THDR_QE_SHARED_COLOR_LIMIT_SPr_SHARED_RED_LIMITf_SET(
                                                    qe_shared_clr_limit, rqlen);
    MMU_THDR_QE_SHARED_COLOR_LIMIT_SPr_SHARED_YELLOW_LIMITf_SET(
                                                    qe_shared_clr_limit, rqlen);

    MMU_THDR_QE_RESUME_COLOR_LIMIT_SPr_CLR(qe_resume_clr_limit);
    MMU_THDR_QE_RESUME_COLOR_LIMIT_SPr_RESUME_RED_LIMITf_SET(
                                                  qe_resume_clr_limit, rqlen-1);
    MMU_THDR_QE_RESUME_COLOR_LIMIT_SPr_RESUME_YELLOW_LIMITf_SET(
                                                  qe_resume_clr_limit, rqlen-1);

    ioerr += WRITE_MMU_THDR_DB_CONFIG_SPr(unit, 0, 0, db_cfg_sp);
    ioerr += WRITE_MMU_THDR_DB_SP_SHARED_LIMITr(unit, 0, 0, db_sp_shared_limit);
    ioerr += WRITE_MMU_THDR_DB_RESUME_COLOR_LIMIT_SPr(unit, 0, 0, 
                                                      db_resume_clr_limit);
    ioerr += WRITE_MMU_THDR_QE_CONFIG_SPr(unit, 0, 0, qe_cfg_sp);
    ioerr += WRITE_MMU_THDR_QE_SHARED_COLOR_LIMIT_SPr(unit, 0, 0, 
                                                      qe_shared_clr_limit);
    ioerr += WRITE_MMU_THDR_QE_RESUME_COLOR_LIMIT_SPr(unit, 0, 0, 
                                                      qe_resume_clr_limit); 

    return ioerr ? CDK_E_IO : CDK_E_NONE; 
}

static int
_mmu_init(int unit)
{
    int rv, ioerr = 0;
    int port, pipe, idx, mport;
    int oversub_mode = FALSE;
    uint32_t pmap[TH_PIPES_PER_DEV][2];
    cdk_pbmp_t pbmp;
    uint32_t pipe_map, speed_max, fval;
    MMU_1DBG_Cr_t dbg1_c;
    MMU_1DBG_Ar_t dbg1_a;
    MMU_3DBG_Cr_t dbg3_c;
    THDU_OUTPUT_PORT_RX_ENABLE_64r_t outp_rx_en;
    MMU_THDM_DB_PORT_RX_ENABLE_64r_t dbp_rx_en;
    MMU_THDM_MCQE_PORT_RX_ENABLE_64r_t mcqep_rx_en;

    rv = _mmu_config_init(unit);

    bcm56960_a0_oversub_map_get(unit, &pbmp);
    if (CDK_PBMP_NOT_NULL(pbmp)) {
        oversub_mode = TRUE;
        ioerr += READ_MMU_1DBG_Cr(unit, 0, &dbg1_c);
        MMU_1DBG_Cr_FIELD_Af_SET(dbg1_c, 1);
        for (idx = 0; idx < TH_NUM_PIPES; idx++) {
            ioerr += WRITE_MMU_1DBG_Cr(unit, idx, dbg1_c);
        }
        MMU_1DBG_Ar_SET(dbg1_a, 0xffffffff);
        for (idx = 0; idx < TH_NUM_PIPES; idx++) {
            ioerr += WRITE_MMU_1DBG_Ar(unit, idx, dbg1_a);
        }
    }

    for(pipe = 0; pipe < TH_PIPES_PER_DEV; pipe++) {
        pmap[pipe][0] = 0;
        pmap[pipe][1] = 0;
    }
    bcm56960_a0_all_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        pipe = bcm56960_a0_port_pipe_get(unit, port);
        mport = P2M(unit, port);
        if (pipe < 0 || mport < 0) {
            continue;
        }
        pmap[pipe][(mport & 0x20) >> 5] |= (1 << (mport & 0x1f));
        if (oversub_mode) {
            MMU_3DBG_Cr_CLR(dbg3_c);
            speed_max = bcm56960_a0_port_speed_max(unit, port);
            if (speed_max >= 100000) {
                fval = 140;
            } else if (speed_max >= 40000) {
                fval = 60;
            } else if (speed_max >= 25000) {
                fval = 40;
            } else if (speed_max >= 20000) {
                fval = 30;
            } else {
                fval = 15;
            }
            MMU_3DBG_Cr_SET(dbg3_c, fval+10);
            ioerr += WRITE_MMU_3DBG_Cr(unit, mport, dbg3_c);
        }
    }

    bcm56960_a0_pipe_map_get(unit, &pipe_map);
    for (pipe = 0; pipe < TH_PIPES_PER_DEV; pipe ++) {
        if ((pipe_map & (1 << pipe)) == 0) {
            continue;
        }
        for (idx = 0; idx < 2; idx++) {
            THDU_OUTPUT_PORT_RX_ENABLE_64r_SET(outp_rx_en, idx, 
                                               pmap[pipe][idx]);
            MMU_THDM_DB_PORT_RX_ENABLE_64r_SET(dbp_rx_en, idx, pmap[pipe][idx]);
            MMU_THDM_MCQE_PORT_RX_ENABLE_64r_SET(mcqep_rx_en, idx, 
                                                 pmap[pipe][idx]);
        }
        ioerr += WRITE_THDU_OUTPUT_PORT_RX_ENABLE_64r_ALL(unit, pipe, 
                                                          outp_rx_en);
        ioerr += WRITE_MMU_THDM_DB_PORT_RX_ENABLE_64r(unit, pipe, dbp_rx_en);
        ioerr += WRITE_MMU_THDM_MCQE_PORT_RX_ENABLE_64r(unit, pipe, 
                                                        mcqep_rx_en);
    }

    return ioerr ? CDK_E_IO : rv;
}

static int
_firmware_helper(void *ctx, uint32_t offset, uint32_t size, void *data)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    CLPORT_WC_UCMEM_CTRLr_t cl_ucmem_ctrl;
    CLPORT_WC_UCMEM_DATAm_t cl_ucmem_data;
    XLPORT_WC_UCMEM_CTRLr_t xl_ucmem_ctrl;
    XLPORT_WC_UCMEM_DATAm_t xl_ucmem_data;
    int unit, port, phy_tscf = 0;
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

    if (CDK_STRSTR(drv_name, "tscf") != NULL) {
        phy_tscf = 1;
    } else if (CDK_STRSTR(drv_name, "tsce") == NULL) {
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
    if (phy_tscf) {
        ioerr += READ_CLPORT_WC_UCMEM_CTRLr(unit, &cl_ucmem_ctrl, port);
        CLPORT_WC_UCMEM_CTRLr_ACCESS_MODEf_SET(cl_ucmem_ctrl, 1);
        ioerr += WRITE_CLPORT_WC_UCMEM_CTRLr(unit, cl_ucmem_ctrl, port);
    } else {
        ioerr += READ_XLPORT_WC_UCMEM_CTRLr(unit, &xl_ucmem_ctrl, port);
        XLPORT_WC_UCMEM_CTRLr_ACCESS_MODEf_SET(xl_ucmem_ctrl, 1);
        ioerr += WRITE_XLPORT_WC_UCMEM_CTRLr(unit, xl_ucmem_ctrl, port);
    }

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
            if (phy_tscf) {
                CLPORT_WC_UCMEM_DATAm_SET(cl_ucmem_data, wdx, wdata);
            } else {
                XLPORT_WC_UCMEM_DATAm_SET(xl_ucmem_data, wdx, wdata);
            }
        }
        if (phy_tscf) {
            WRITE_CLPORT_WC_UCMEM_DATAm(unit, idx >> 4, cl_ucmem_data, port);
        } else {
            WRITE_XLPORT_WC_UCMEM_DATAm(unit, idx >> 4, xl_ucmem_data, port);
        }
    }

    /* Disable parallel bus access */
    if (phy_tscf) {
        CLPORT_WC_UCMEM_CTRLr_ACCESS_MODEf_SET(cl_ucmem_ctrl, 0);
        ioerr += WRITE_CLPORT_WC_UCMEM_CTRLr(unit, cl_ucmem_ctrl, port);
    } else {
        XLPORT_WC_UCMEM_CTRLr_ACCESS_MODEf_SET(xl_ucmem_ctrl, 0);
        ioerr += WRITE_XLPORT_WC_UCMEM_CTRLr(unit, xl_ucmem_ctrl, port);
    }

    return ioerr ? CDK_E_IO : rv;
}

static int
_phy_default_ability_set(int unit, int port)
{
    cdk_pbmp_t pbmp;
    int speed_max, ability = 0;

    ability = (BMD_PHY_ABIL_2500MB | BMD_PHY_ABIL_1000MB_FD | 
               BMD_PHY_ABIL_100MB_FD);

    speed_max = bcm56960_a0_port_speed_max(unit, port);    

    bcm56960_a0_clport_pbmp_get(unit, &pbmp);
    if (CDK_PBMP_MEMBER(pbmp, port)) {
        if (speed_max == 10000) {
            ability |= BMD_PHY_ABIL_10GB;
        } else if (speed_max == 20000) {
            ability |= BMD_PHY_ABIL_20GB;
        } else if (speed_max == 25000) {
            ability |= BMD_PHY_ABIL_25GB;
        } else if (speed_max == 40000) {
            ability |= BMD_PHY_ABIL_40GB;
        } else if (speed_max == 100000) {
            ability |= BMD_PHY_ABIL_100GB;
        }
    } else {
        if (speed_max == 10000) {
            ability |= BMD_PHY_ABIL_10GB;
        }
    }

    return bmd_phy_default_ability_set(unit, port, ability);
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
    PORT_TABm_SRC_SYS_PORT_IDf_SET(port_tab, lport);
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
    ioerr += READ_EGR_ENABLEm(unit, lport, &egr_enable);
    EGR_ENABLEm_PRT_ENABLEf_SET(egr_enable, 1);
    ioerr += WRITE_EGR_ENABLEm(unit, lport, egr_enable);

    return ioerr;
}

int
bcm56960_a0_clport_init(int unit, int port)
{
    int ioerr = 0;
    int lport = P2L(unit, port);
    int ipg;
    CLMAC_TX_CTRLr_t tx_ctrl;
    CLMAC_RX_CTRLr_t rx_ctrl;
    CLMAC_RX_MAX_SIZEr_t rx_max_size;
    CLMAC_CTRLr_t mac_ctrl;
    EGR_PORT_CREDIT_RESETm_t egr_port_credit_reset;
    CLMAC_PAUSE_CTRLr_t mac_pause_ctrl;
    CLMAC_PFC_CTRLr_t mac_pfc_ctrl;
    CLMAC_MODEr_t mac_mode;
    
    /* Common port initialization */
    ioerr += _port_init(unit, port);

    /* Ensure that MAC (Rx) and loopback mode is disabled */
    CLMAC_CTRLr_CLR(mac_ctrl);
    CLMAC_CTRLr_SOFT_RESETf_SET(mac_ctrl, 1);
    ioerr += WRITE_CLMAC_CTRLr(unit, port, mac_ctrl);

    /* Reset EP credit before de-assert SOFT_RESET */
    EGR_PORT_CREDIT_RESETm_CLR(egr_port_credit_reset);
    EGR_PORT_CREDIT_RESETm_VALUEf_SET(egr_port_credit_reset, 1);
    ioerr += WRITE_EGR_PORT_CREDIT_RESETm(unit, lport, egr_port_credit_reset);
    BMD_SYS_USLEEP(1000);
    EGR_PORT_CREDIT_RESETm_VALUEf_SET(egr_port_credit_reset, 0);
    ioerr += WRITE_EGR_PORT_CREDIT_RESETm(unit, lport, egr_port_credit_reset);
 
    CLMAC_CTRLr_SOFT_RESETf_SET(mac_ctrl, 0);
    CLMAC_CTRLr_RX_ENf_SET(mac_ctrl, 0);
    CLMAC_CTRLr_TX_ENf_SET(mac_ctrl, 0);
    CLMAC_CTRLr_XGMII_IPG_CHECK_DISABLEf_SET(mac_ctrl, 
                (BMD_PORT_PROPERTIES(unit, port) == BMD_PORT_HG) ? 1 : 0) ;
    ioerr += WRITE_CLMAC_CTRLr(unit, port, mac_ctrl);

    /* Configure Rx (strip CRC, strict preamble, IEEE header) */
    ioerr += READ_CLMAC_RX_CTRLr(unit, port, &rx_ctrl);
    CLMAC_RX_CTRLr_STRIP_CRCf_SET(rx_ctrl, 0);
    ioerr += WRITE_CLMAC_RX_CTRLr(unit, port, rx_ctrl);

    /* Configure Tx (Inter-Packet-Gap, recompute CRC mode, IEEE header) */
    ioerr += READ_CLMAC_TX_CTRLr(unit, port, &tx_ctrl);
    if (BMD_PORT_PROPERTIES(unit, port) == BMD_PORT_HG) {
        ipg = 64;
    } else {
        ipg = 96;
    }
    CLMAC_TX_CTRLr_AVERAGE_IPGf_SET(tx_ctrl, (ipg / 8) & 0x1f);
    CLMAC_TX_CTRLr_CRC_MODEf_SET(tx_ctrl, 0x3);
    ioerr += WRITE_CLMAC_TX_CTRLr(unit, port, tx_ctrl);

    ioerr += READ_CLMAC_PAUSE_CTRLr(unit, port, &mac_pause_ctrl);
    CLMAC_PAUSE_CTRLr_TX_PAUSE_ENf_SET(mac_pause_ctrl, 1);
    CLMAC_PAUSE_CTRLr_RX_PAUSE_ENf_SET(mac_pause_ctrl, 1);
    ioerr += WRITE_CLMAC_PAUSE_CTRLr(unit, port, mac_pause_ctrl);

    ioerr += READ_CLMAC_PFC_CTRLr(unit, port, &mac_pfc_ctrl);
    CLMAC_PFC_CTRLr_PFC_REFRESH_ENf_SET(mac_pfc_ctrl, 1);
    ioerr += WRITE_CLMAC_PFC_CTRLr(unit, port, mac_pfc_ctrl);

    /* Set max Rx frame size */
    CLMAC_RX_MAX_SIZEr_CLR(rx_max_size);
    CLMAC_RX_MAX_SIZEr_RX_MAX_SIZEf_SET(rx_max_size, JUMBO_MAXSZ);
    ioerr += WRITE_CLMAC_RX_MAX_SIZEr(unit, port, rx_max_size);

    ioerr += READ_CLMAC_MODEr(unit, port, &mac_mode);
    if (BMD_PORT_PROPERTIES(unit, port) == BMD_PORT_HG) {
        CLMAC_MODEr_HDR_MODEf_SET(mac_mode, 2);
    }
    ioerr += WRITE_CLMAC_MODEr(unit, port, mac_mode);

    /* Disable loopback and bring CLMAC out of reset */
    CLMAC_CTRLr_LOCAL_LPBKf_SET(mac_ctrl, 0);
    CLMAC_CTRLr_RX_ENf_SET(mac_ctrl, 1);
    CLMAC_CTRLr_TX_ENf_SET(mac_ctrl, 1);
    ioerr += WRITE_CLMAC_CTRLr(unit, port, mac_ctrl);

    return ioerr;
}

int
bcm56960_a0_xlport_init(int unit, int port)
{
    int ioerr = 0;
    int speed, lport = P2L(unit, port);
    XLMAC_TX_CTRLr_t tx_ctrl;
    XLMAC_RX_CTRLr_t rx_ctrl;
    XLMAC_RX_MAX_SIZEr_t rx_max_size;
    XLMAC_RX_LSS_CTRLr_t rx_lss_ctrl;
    XLMAC_CTRLr_t mac_ctrl;
    EGR_PORT_CREDIT_RESETm_t egr_port_credit_reset;
    XLMAC_PAUSE_CTRLr_t mac_pause_ctrl;
    XLMAC_PFC_CTRLr_t mac_pfc_ctrl;
    XLMAC_MODEr_t mac_mode;
    
    /* Common port initialization */
    ioerr += _port_init(unit, port);

    speed = bcm56960_a0_port_speed_max(unit, port);

    /* Ensure that MAC (Rx) and loopback mode is disabled */
    XLMAC_CTRLr_CLR(mac_ctrl);
    XLMAC_CTRLr_SOFT_RESETf_SET(mac_ctrl, 1);
    ioerr += WRITE_XLMAC_CTRLr(unit, port, mac_ctrl);

    /* Reset EP credit before de-assert SOFT_RESET */
    EGR_PORT_CREDIT_RESETm_CLR(egr_port_credit_reset);
    EGR_PORT_CREDIT_RESETm_VALUEf_SET(egr_port_credit_reset, 1);
    ioerr += WRITE_EGR_PORT_CREDIT_RESETm(unit, lport, egr_port_credit_reset);
    BMD_SYS_USLEEP(1000);
    EGR_PORT_CREDIT_RESETm_VALUEf_SET(egr_port_credit_reset, 0);
    ioerr += WRITE_EGR_PORT_CREDIT_RESETm(unit, lport, egr_port_credit_reset);
 
    XLMAC_CTRLr_SOFT_RESETf_SET(mac_ctrl, 0);
    XLMAC_CTRLr_RX_ENf_SET(mac_ctrl, 0);
    XLMAC_CTRLr_TX_ENf_SET(mac_ctrl, 0);
    XLMAC_CTRLr_XGMII_IPG_CHECK_DISABLEf_SET(mac_ctrl, 
                (BMD_PORT_PROPERTIES(unit, port) == BMD_PORT_HG) ? 1 : 0) ;
    ioerr += WRITE_XLMAC_CTRLr(unit, port, mac_ctrl);

    /* Configure Rx (strip CRC, strict preamble, IEEE header) */
    ioerr += READ_XLMAC_RX_CTRLr(unit, port, &rx_ctrl);
    XLMAC_RX_CTRLr_STRIP_CRCf_SET(rx_ctrl, 0);
    if ((speed >= 10000) && 
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

    ioerr += READ_XLMAC_PAUSE_CTRLr(unit, port, &mac_pause_ctrl);
    XLMAC_PAUSE_CTRLr_TX_PAUSE_ENf_SET(mac_pause_ctrl, 1);
    XLMAC_PAUSE_CTRLr_RX_PAUSE_ENf_SET(mac_pause_ctrl, 1);
    ioerr += WRITE_XLMAC_PAUSE_CTRLr(unit, port, mac_pause_ctrl);

    ioerr += READ_XLMAC_PFC_CTRLr(unit, port, &mac_pfc_ctrl);
    XLMAC_PFC_CTRLr_PFC_REFRESH_ENf_SET(mac_pfc_ctrl, 1);
    ioerr += WRITE_XLMAC_PFC_CTRLr(unit, port, mac_pfc_ctrl);

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
    XLMAC_RX_LSS_CTRLr_DROP_TX_DATA_ON_LINK_INTERRUPTf_SET(rx_lss_ctrl, 1);
    ioerr += WRITE_XLMAC_RX_LSS_CTRLr(unit, port, rx_lss_ctrl);

    /* Disable loopback and bring XLMAC out of reset */
    XLMAC_CTRLr_LOCAL_LPBKf_SET(mac_ctrl, 0);
    XLMAC_CTRLr_RX_ENf_SET(mac_ctrl, 1);
    XLMAC_CTRLr_TX_ENf_SET(mac_ctrl, 1);
    ioerr += WRITE_XLMAC_CTRLr(unit, port, mac_ctrl);

    return ioerr;
}

int 
bcm56960_a0_bmd_init(int unit)
{
    int ioerr = 0;
    int rv = 0;
    int core_frequency;
    int idx, count, cmp, fval, class, lanes = 0;
    int port, pipe, sub_port, lport, freq, mdio_div;
    uint32_t pipe_map, in_progress, port_field, port_mode, speed_max, cd;
    cdk_pbmp_t pbmp, cl_pbmp, xl_pbmp, lb_pbmp, eq_pbmp;
    ING_HW_RESET_CONTROL_1r_t ing_hreset_ctrl_1;
    ING_HW_RESET_CONTROL_2r_t ing_hreset_ctrl_2;
    EGR_HW_RESET_CONTROL_0r_t egr_hreset_ctrl_0;
    EGR_HW_RESET_CONTROL_1r_t egr_hreset_ctrl_1;
    IDB_HW_CONTROLr_t idb_hctrl;
    MMU_GCFG_MISCCONFIGr_t mmu_misccfg;
    CPU_PBMm_t cpu_pbm;
    CPU_PBM_2m_t cpu_pbm_2;
    uint32_t pbm[PBM_PORT_WORDS];
    DEVICE_LOOPBACK_PORTS_BITMAPm_t dev_lpbk_pbmp;
    MULTIPASS_LOOPBACK_BITMAPm_t mltpss_lpbk_bmp;
    EGR_ING_PORTm_t egr_ing_port;
    CLPORT_SOFT_RESETr_t clport_reset;
    CLPORT_MODE_REGr_t clport_mode;
    CLPORT_ENABLE_REGr_t clport_en;
    CLPORT_FLOW_CONTROL_CONFIGr_t clport_flowctrl;
    XLPORT_SOFT_RESETr_t xlport_reset;
    XLPORT_MODE_REGr_t xlport_mode;
    XLPORT_ENABLE_REGr_t xlport_en;
    XLPORT_FLOW_CONTROL_CONFIGr_t xlport_flowctrl;
    EGR_MMU_CELL_CREDITr_t egr_mmu_crdt;
    EGR_XMIT_START_COUNTm_t egr_xmt_cnt;
    EGR_ENABLEm_t egr_en;
    CLPORT_MIB_RESETr_t clport_mib_reset;
    XLPORT_MIB_RESETr_t xlport_mib_reset;   
    ING_CONFIG_64r_t ing_cfg_64;
    EGR_CONFIG_1r_t egr_cfg_1;
    EGR_VLAN_CONTROL_1r_t egr_vlan_ctrl_1;
    ING_EN_EFILTER_BITMAPm_t ing_en_efilter;
    EDB_1DBG_Bm_t edb_1dbg_b;
    EDB_1DBG_Ar_t edb_1dbg_a;
    CMIC_RATE_ADJUSTr_t rate_adjust;
    CMIC_RATE_ADJUST_INT_MDIOr_t rate_adjust_int;
    ING_VLAN_TAG_ACTION_PROFILEm_t vlan_action;
    CLPORT_MAC_CONTROLr_t clp_mac_ctrl;
    XLPORT_MAC_CONTROLr_t xlp_mac_ctrl;
    _tdm_config_t tcfg;

    BMD_CHECK_UNIT(unit);

    /* Core clock frequency */
    core_frequency = 850;

    /* Initial IPIPE memory */
    ING_HW_RESET_CONTROL_1r_CLR(ing_hreset_ctrl_1);
    ioerr += WRITE_ING_HW_RESET_CONTROL_1r(unit, ing_hreset_ctrl_1);
    ING_HW_RESET_CONTROL_2r_CLR(ing_hreset_ctrl_2);
    ING_HW_RESET_CONTROL_2r_RESET_ALLf_SET(ing_hreset_ctrl_2, 1);
    ING_HW_RESET_CONTROL_2r_VALIDf_SET(ing_hreset_ctrl_2, 1);
    /* Set count to # entries of largest IPIPE table */
    count = ING_L3_NEXT_HOPm_MAX - ING_L3_NEXT_HOPm_MIN + 1;
    cmp = L2Xm_MAX - L2Xm_MIN + 1;
    if (count < cmp) {
        count = cmp;
    }
    cmp = L3_ENTRY_ONLYm_MAX - L3_ENTRY_ONLYm_MIN + 1;
    if (count < cmp) {
        count = cmp;
    }
    cmp = FPEM_ECCm_MAX - FPEM_ECCm_MIN + 1;
    if (count < cmp) {
        count = cmp;
    }
    cmp = L3_DEFIP_ALPM_IPV4m_MAX - L3_DEFIP_ALPM_IPV4m_MIN + 1;
    if (count < cmp) {
        count = cmp;
    }
    ING_HW_RESET_CONTROL_2r_COUNTf_SET(ing_hreset_ctrl_2, count);
    ioerr += WRITE_ING_HW_RESET_CONTROL_2r_ALL(unit, ing_hreset_ctrl_2);

    /* Initial EPIPE memory */
    EGR_HW_RESET_CONTROL_0r_CLR(egr_hreset_ctrl_0);
    ioerr += WRITE_EGR_HW_RESET_CONTROL_0r(unit, egr_hreset_ctrl_0);
    EGR_HW_RESET_CONTROL_1r_CLR(egr_hreset_ctrl_1);
    EGR_HW_RESET_CONTROL_1r_RESET_ALLf_SET(egr_hreset_ctrl_1, 1);
    EGR_HW_RESET_CONTROL_1r_VALIDf_SET(egr_hreset_ctrl_1, 1);
    /* Set count to # entries in largest EPIPE table (EGR_L3_NEXT_HOP) */
    count = EGR_L3_NEXT_HOPm_MAX - EGR_L3_NEXT_HOPm_MIN + 1;
    EGR_HW_RESET_CONTROL_1r_COUNTf_SET(egr_hreset_ctrl_1, count);
    ioerr += WRITE_EGR_HW_RESET_CONTROL_1r_ALL(unit, egr_hreset_ctrl_1);

    /* Wait for IPIPE memory initialization done. */
    bcm56960_a0_pipe_map_get(unit, &pipe_map);
    in_progress = pipe_map;
    for (idx = 0; idx < PIPE_RESET_TIMEOUT_MSEC; idx++) {
        for (pipe = 0; pipe < TH_PIPES_PER_DEV; pipe++) {            
            if (in_progress & (1 << pipe)) { /* not done yet */
                ioerr += READ_ING_HW_RESET_CONTROL_2r(unit, pipe, 
                                                      &ing_hreset_ctrl_2);
                if (ING_HW_RESET_CONTROL_2r_DONEf_GET(ing_hreset_ctrl_2)) {
                    in_progress ^= (1 << pipe);
                }
            }
        }
        if (in_progress == 0) {
            break;
        }
        BMD_SYS_USLEEP(1000);
    }
    if (idx >= PIPE_RESET_TIMEOUT_MSEC) {
        CDK_WARN(("bcm5696_a0_bmd_init[%d]: ING_HW_RESET timeout. " 
                  "pipe_map=%d, in_progress=%d\n", 
                  unit, (int)pipe_map, (int)in_progress));
        return ioerr ? CDK_E_IO : CDK_E_TIMEOUT;
    }

    /* Wait for EPIPE memory initialization done. */
    in_progress = pipe_map;
    for (idx = 0; idx < PIPE_RESET_TIMEOUT_MSEC; idx ++) {
        for (pipe = 0; pipe < TH_PIPES_PER_DEV; pipe++) {
            if (in_progress & (1 << pipe)) { /* not done yet */
                ioerr += READ_EGR_HW_RESET_CONTROL_1r(unit, pipe, 
                                                      &egr_hreset_ctrl_1);
                if (EGR_HW_RESET_CONTROL_1r_DONEf_GET(egr_hreset_ctrl_1)) {
                    in_progress ^= (1 << pipe);
                }
            }
        }
        if (in_progress == 0) {
            break;
        }
        BMD_SYS_USLEEP(1000);
    }
    if (idx >= PIPE_RESET_TIMEOUT_MSEC) {
        CDK_WARN(("bcm56960_a0_bmd_init[%d]: EGR_HW_RESET timeout\n", unit));
        return ioerr ? CDK_E_IO : CDK_E_TIMEOUT;
    }

    ING_HW_RESET_CONTROL_2r_CLR(ing_hreset_ctrl_2);
    ioerr += WRITE_ING_HW_RESET_CONTROL_2r_ALL(unit, ing_hreset_ctrl_2);
    EGR_HW_RESET_CONTROL_1r_CLR(egr_hreset_ctrl_1);
    ioerr += WRITE_EGR_HW_RESET_CONTROL_1r_ALL(unit, egr_hreset_ctrl_1);

    /* Initialize IDB memory */
    IDB_HW_CONTROLr_CLR(idb_hctrl);
    ioerr += WRITE_IDB_HW_CONTROLr_ALL(unit, idb_hctrl);
    IDB_HW_CONTROLr_IS_MEM_INITf_SET(idb_hctrl, 1);
    ioerr += WRITE_IDB_HW_CONTROLr_ALL(unit, idb_hctrl);
    IDB_HW_CONTROLr_CLR(idb_hctrl);
    ioerr += WRITE_IDB_HW_CONTROLr_ALL(unit, idb_hctrl);

    /* Initialize MMU memory */
    MMU_GCFG_MISCCONFIGr_CLR(mmu_misccfg);
    ioerr += WRITE_MMU_GCFG_MISCCONFIGr(unit, 0, mmu_misccfg);
    MMU_GCFG_MISCCONFIGr_INIT_MEMf_SET(mmu_misccfg, 1);
    ioerr += WRITE_MMU_GCFG_MISCCONFIGr(unit, 0, mmu_misccfg);
    MMU_GCFG_MISCCONFIGr_CLR(mmu_misccfg);
    ioerr += WRITE_MMU_GCFG_MISCCONFIGr(unit, 0, mmu_misccfg);

    /* Initialize port mappings */
    rv = _port_mapping_init(unit);
    if (CDK_FAILURE(rv)) {
        return rv;
    }

    /* Setup TDM */
    rv = _tdm_calculation(unit, TRUE, &tcfg, core_frequency);
    if (CDK_FAILURE(rv)) {
        return rv;
    }
    rv = _tdm_idb_calendar_set(unit, 0, &tcfg);
    if (CDK_FAILURE(rv)) {
        return rv;
    }
    rv = _tdm_idb_oversub_group_set(unit, &tcfg);
    if (CDK_FAILURE(rv)) {
        return rv;
    }
    rv =_tdm_idb_opportunistic_set(unit, TRUE);
    if (CDK_FAILURE(rv)) {
        return rv;
    }
    rv = _tdm_idb_hsp_set(unit, UPDATE_ALL_PIPES);
    if (CDK_FAILURE(rv)) {
        return rv;
    }
    
    rv = _tdm_mmu_calendar_set(unit, 0, &tcfg);
    if (CDK_FAILURE(rv)) {
        return rv;
    }
    rv = _tdm_mmu_oversub_group_set(unit, &tcfg);
    if (CDK_FAILURE(rv)) {
        return rv;
    }
    rv = _tdm_mmu_opportunistic_set(unit, TRUE);
    if (CDK_FAILURE(rv)) {
        return rv;
    }
    rv = _tdm_mmu_hsp_set(unit, UPDATE_ALL_PIPES);
    if (CDK_FAILURE(rv)) {
        return rv;
    }
    rv = _idb_init(unit, &tcfg);
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
    
    /* Configure loopback ports */
    bcm56960_a0_lpbk_pbmp_get(unit, &lb_pbmp);
    CDK_MEMSET(pbm, 0, sizeof(pbm));
    CDK_PBMP_ITER(lb_pbmp, port) {
        PBM_PORT_ADD(pbm, P2L(unit, port));
    }
    DEVICE_LOOPBACK_PORTS_BITMAPm_CLR(dev_lpbk_pbmp);
    DEVICE_LOOPBACK_PORTS_BITMAPm_BITMAPf_SET(dev_lpbk_pbmp, pbm);
    ioerr += WRITE_DEVICE_LOOPBACK_PORTS_BITMAPm(unit, 0, dev_lpbk_pbmp);

    CDK_PBMP_ITER(lb_pbmp, port) {
        pipe = bcm56960_a0_port_pipe_get(unit, port);
        if (pipe < 0) {
            continue;
        }
        CDK_MEMSET(pbm, 0, sizeof(pbm));
        PBM_PORT_ADD(pbm, P2L(unit, port));
        MULTIPASS_LOOPBACK_BITMAPm_CLR(mltpss_lpbk_bmp);
        MULTIPASS_LOOPBACK_BITMAPm_BITMAPf_SET(mltpss_lpbk_bmp, pbm);
        ioerr += WRITE_MULTIPASS_LOOPBACK_BITMAPm(unit, pipe, 0, 
                                                  mltpss_lpbk_bmp);
    }
    
    EGR_ING_PORTm_CLR(egr_ing_port);
    EGR_ING_PORTm_PORT_TYPEf_SET(egr_ing_port, 1);

    bcm56960_a0_all_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        if (BMD_PORT_PROPERTIES(unit, port) == BMD_PORT_HG) {
            ioerr += WRITE_EGR_ING_PORTm(unit, P2L(unit, port), egr_ing_port);
        }
    }
    
    ioerr += WRITE_EGR_ING_PORTm(unit, 136, egr_ing_port); /* cpu_hg_index */

    EGR_ING_PORTm_PORT_TYPEf_SET(egr_ing_port, 2);
    CDK_PBMP_ITER(lb_pbmp, port) {
        ioerr += WRITE_EGR_ING_PORTm(unit, P2L(unit, port), egr_ing_port);
    }

    /* Initialize CLPORTs */
    bcm56960_a0_clport_pbmp_get(unit, &cl_pbmp);
    CDK_PBMP_ITER(cl_pbmp, port) {
        /* We only need to write once per block */
        sub_port = CLPORT_SUBPORT(unit, port);
        if (sub_port != 0) {
            continue;
        }
        /* Configure each sub port */
        port_field = 0;
        for (sub_port = 0; sub_port < TH_PORTS_PER_PBLK; sub_port++) {
            if (P2L(unit, port + sub_port) != -1) {
                port_field |= (1 << sub_port);
            }
        }
        /* Assert CLPORT soft reset */
        CLPORT_SOFT_RESETr_CLR(clport_reset);
        CLPORT_SOFT_RESETr_SET(clport_reset, port_field);
        ioerr += WRITE_CLPORT_SOFT_RESETr(unit, clport_reset, port);

        /* Set port mode */
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
        ioerr += WRITE_CLPORT_MODE_REGr(unit, clport_mode, port);

        /* De-assert CLPORT soft reset */
        CLPORT_SOFT_RESETr_CLR(clport_reset);
        ioerr += WRITE_CLPORT_SOFT_RESETr(unit, clport_reset, port);

        /* Enable CLPORT */
        CLPORT_ENABLE_REGr_CLR(clport_en);
        CLPORT_ENABLE_REGr_SET(clport_en, port_field);
        ioerr += WRITE_CLPORT_ENABLE_REGr(unit, clport_en, port);

        CLPORT_FLOW_CONTROL_CONFIGr_CLR(clport_flowctrl);
        CLPORT_FLOW_CONTROL_CONFIGr_MERGE_MODE_ENf_SET(clport_flowctrl, 1);
        ioerr += WRITE_CLPORT_FLOW_CONTROL_CONFIGr(unit, port, clport_flowctrl);
    }

    /* Initialize XLPORT */
    bcm56960_a0_xlport_pbmp_get(unit, &xl_pbmp);
    CDK_PBMP_ITER(xl_pbmp, port) {
        port_field = 0;
        if (P2L(unit, TH_MGMT_PORT_0) != -1) {
            port_field |= 1;
        }
        if (P2L(unit, TH_MGMT_PORT_1) != -1) {
            port_field |= (1 << 2);
        }  

        XLPORT_SOFT_RESETr_CLR(xlport_reset);
        XLPORT_SOFT_RESETr_SET(xlport_reset, port_field);
        ioerr += WRITE_XLPORT_SOFT_RESETr(unit, xlport_reset, port);

        /* Set port mode based upon lanes config */
        XLPORT_MODE_REGr_CLR(xlport_mode);
        /* Note - this is a special h/w requirement to use dual lane mode */
        port_mode = (bcm56960_a0_port_lanes_get(unit, port) == 4) ? 4 : 3;
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

        XLPORT_FLOW_CONTROL_CONFIGr_CLR(xlport_flowctrl);
        XLPORT_FLOW_CONTROL_CONFIGr_MERGE_MODE_ENf_SET(xlport_flowctrl, 1);
        ioerr += WRITE_XLPORT_FLOW_CONTROL_CONFIGr(unit, port, xlport_flowctrl);

        break;
    }

    /* Reset MIB counters in all blocks */
    CDK_PBMP_ITER(cl_pbmp, port) {
        /* We only need to write once per block */
        if (CLPORT_SUBPORT(unit, port) != 0) {
            continue;
        }
        CLPORT_MIB_RESETr_CLR(clport_mib_reset);
        CLPORT_MIB_RESETr_CLR_CNTf_SET(clport_mib_reset, 0xf);
        ioerr += WRITE_CLPORT_MIB_RESETr(unit, clport_mib_reset, port);
        CLPORT_MIB_RESETr_CLR(clport_mib_reset);
        ioerr += WRITE_CLPORT_MIB_RESETr(unit, clport_mib_reset, port);        
    }

    CDK_PBMP_ITER(xl_pbmp, port) {
        /* We only need to write once per block */
        XLPORT_MIB_RESETr_CLR(xlport_mib_reset);
        XLPORT_MIB_RESETr_CLR_CNTf_SET(xlport_mib_reset, 0xf);
        ioerr += WRITE_XLPORT_MIB_RESETr(unit, xlport_mib_reset, port);
        XLPORT_MIB_RESETr_CLR(xlport_mib_reset);
        ioerr += WRITE_XLPORT_MIB_RESETr(unit, xlport_mib_reset, port);
        break;
    }

    /* Enable Field Processor metering clock */
    ioerr += READ_MMU_GCFG_MISCCONFIGr(unit, 0, &mmu_misccfg);
    MMU_GCFG_MISCCONFIGr_REFRESH_ENf_SET(mmu_misccfg, 1);
    ioerr += WRITE_MMU_GCFG_MISCCONFIGr(unit, 0, mmu_misccfg);

    /* Enable dual hash tables */
    ioerr += _hash_init(unit);

    /* Configure EP credit */
    EGR_MMU_CELL_CREDITr_CLR(egr_mmu_crdt);
    bcm56960_a0_all_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        speed_max = bcm56960_a0_port_speed_max(unit, port);
        if (speed_max >= 100000 || CDK_PBMP_MEMBER(lb_pbmp, port)) {
            fval = 44;
        } else if (speed_max >= 50000) {
            fval = 27;
        } else if (speed_max >= 40000) {
            fval = 25;
        } else if (speed_max >= 25000) {
            fval = 16;
        } else if (speed_max >= 20000) {
            fval = 18;
        } else {
            fval = 13;
        }
        EGR_MMU_CELL_CREDITr_CREDITf_SET(egr_mmu_crdt, fval);
        ioerr += WRITE_EGR_MMU_CELL_CREDITr(unit, P2L(unit, port), 
                                            egr_mmu_crdt);
    }
   
    /* Configure egress transmit start count */
    EGR_XMIT_START_COUNTm_CLR(egr_xmt_cnt);
    EGR_XMIT_START_COUNTm_THRESHOLDf_SET(egr_xmt_cnt, 18);

    bcm56960_a0_all_pbmp_get(unit, &pbmp);
    bcm56960_a0_eq_pbmp_get(unit, &eq_pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        pipe = bcm56960_a0_port_pipe_get(unit, port);
        for (class = 0; class < 13; class++) {
            idx = (P2L(unit, port) % 34) * 16 + class;
            ioerr += WRITE_EGR_XMIT_START_COUNTm(unit, pipe, idx, egr_xmt_cnt);
        }
    
        /* Update HSP port multicast T2OQ setting */
        if (CDK_PBMP_MEMBER(eq_pbmp, port)) {
            _mc_toq_cfg(unit, port, TRUE);
        }
    }

    bcm56960_a0_oversub_map_get(unit, &pbmp);
    if (!CDK_PBMP_IS_NULL(pbmp)) {
        EDB_1DBG_Bm_CLR(edb_1dbg_b);
        bcm56960_a0_all_pbmp_get(unit, &pbmp);
        CDK_PBMP_ITER(pbmp, port) {
            speed_max = bcm56960_a0_port_speed_max(unit, port);
            if (speed_max < 0) {
                continue;
            }
            fval = (speed_max * 11875) / 100000;
            EDB_1DBG_Bm_FIELD_Bf_SET(edb_1dbg_b, fval);
            ioerr += WRITE_EDB_1DBG_Bm(unit, port, edb_1dbg_b);
        }

        bcm56960_a0_pipe_map_get(unit, &pipe_map);
        for (pipe = 0; pipe < TH_NUM_PIPES; pipe++) {
            if (((1 << pipe) & pipe_map) == 0) {
                continue;
            }
            ioerr += READ_EDB_1DBG_Ar(unit, pipe, &edb_1dbg_a);
            cd = 0;
            switch (core_frequency) {
            case 850: cd = 7650;
                break;
            case 765: cd = 6885;
                break;
            case 672: cd = 6048;
                break;
            case 645: cd = 5805;
                break;
            case 545: cd = 4905;
                break;
            default:
                break;
            }
            EDB_1DBG_Ar_FIELD_Af_SET(edb_1dbg_a, core_frequency);
            EDB_1DBG_Ar_FIELD_Bf_SET(edb_1dbg_a, cd);
            EDB_1DBG_Ar_FIELD_Cf_SET(edb_1dbg_a, 8);
            ioerr += WRITE_EDB_1DBG_Ar(unit, pipe, edb_1dbg_a);
        }
    }

    /* Enable egress */
    EGR_ENABLEm_CLR(egr_en);
    EGR_ENABLEm_PRT_ENABLEf_SET(egr_en, 1);
    bcm56960_a0_all_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        lport = P2L(unit, port);
        if (lport >= 0) {
            ioerr += WRITE_EGR_ENABLEm(unit, lport, egr_en);
        }
    }

    ioerr += CDK_XGSD_MEM_CLEAR(unit, EPC_LINK_BMAPm);   

    ioerr += READ_ING_CONFIG_64r(unit, &ing_cfg_64);
    ING_CONFIG_64r_L3SRC_HIT_ENABLEf_SET(ing_cfg_64, 1);
    ING_CONFIG_64r_L2DST_HIT_ENABLEf_SET(ing_cfg_64, 1);
    ING_CONFIG_64r_APPLY_EGR_MASK_ON_L2f_SET(ing_cfg_64, 1);
    ING_CONFIG_64r_APPLY_EGR_MASK_ON_L3f_SET(ing_cfg_64, 1);
    ING_CONFIG_64r_ARP_RARP_TO_FPf_SET(ing_cfg_64, 0x3); /* enable both ARP & RARP */
    ING_CONFIG_64r_ARP_VALIDATION_ENf_SET(ing_cfg_64, 1);
    ING_CONFIG_64r_IGNORE_HG_HDR_LAG_FAILOVERf_SET(ing_cfg_64, 1);
    ioerr += WRITE_ING_CONFIG_64r(unit, ing_cfg_64);

    ioerr += READ_EGR_CONFIG_1r(unit, &egr_cfg_1);
    EGR_CONFIG_1r_RING_MODEf_SET(egr_cfg_1, 1);
    ioerr += WRITE_EGR_CONFIG_1r(unit, egr_cfg_1);
    
    /* The HW defaults for EGR_VLAN_CONTROL_1.VT_MISS_UNTAG == 1, which
     * causes the outer tag to be removed from packets that don't have
     * a hit in the egress vlan tranlation table. Set to 0 to disable this.
     */
    EGR_VLAN_CONTROL_1r_CLR(egr_vlan_ctrl_1);
    EGR_VLAN_CONTROL_1r_VT_MISS_UNTAGf_SET(egr_vlan_ctrl_1, 0);
    /* Enable pri/cfi remarking on egress ports. */
    EGR_VLAN_CONTROL_1r_REMARK_OUTER_DOT1Pf_SET(egr_vlan_ctrl_1, 1);
    bcm56960_a0_all_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        lport = P2L(unit, port);
        ioerr += WRITE_EGR_VLAN_CONTROL_1r(unit, lport, egr_vlan_ctrl_1);
    }

    /* Enable egress VLAN checks for all ports */
    CDK_PBMP_ASSIGN(pbmp, cl_pbmp);
    CDK_PBMP_OR(pbmp, xl_pbmp);
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
    
    freq = core_frequency;
    /*
     * Set external MDIO freq to around 2MHz
     * target_freq = core_clock_freq * DIVIDEND / DIVISOR / 2
     */
    mdio_div = (freq + (2 * 2 - 1)) / (2 * 2);
    CMIC_RATE_ADJUSTr_CLR(rate_adjust);
    CMIC_RATE_ADJUSTr_DIVISORf_SET(rate_adjust, mdio_div);
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

    /* Initialize MMU */
    rv = _mmu_init(unit);
    if (CDK_FAILURE(rv)) {
       return rv;
    }

    /* Ensure that all incoming packets get tagged appropriately */
    ING_VLAN_TAG_ACTION_PROFILEm_CLR(vlan_action);
    ING_VLAN_TAG_ACTION_PROFILEm_UT_OTAG_ACTIONf_SET(vlan_action, 1);
    ING_VLAN_TAG_ACTION_PROFILEm_SIT_PITAG_ACTIONf_SET(vlan_action, 3);
    ING_VLAN_TAG_ACTION_PROFILEm_SIT_OTAG_ACTIONf_SET(vlan_action, 1);
    ING_VLAN_TAG_ACTION_PROFILEm_SOT_POTAG_ACTIONf_SET(vlan_action, 2);
    ING_VLAN_TAG_ACTION_PROFILEm_DT_POTAG_ACTIONf_SET(vlan_action, 2);
    ioerr += WRITE_ING_VLAN_TAG_ACTION_PROFILEm(unit, 0, vlan_action);    


    /* Probe PHYs */
    CDK_PBMP_ITER(cl_pbmp, port) {
    
        if (CDK_SUCCESS(rv)) {
            rv = bmd_phy_probe(unit, port);
        }

        if (CDK_SUCCESS(rv)) {
            lanes = bcm56960_a0_port_lanes_get(unit, port);
        }

        if (lanes == 4) {
            rv = bmd_phy_mode_set(unit, port, "tscf", BMD_PHY_MODE_SERDES, 0);
        } else if (lanes == 2) {
            rv = bmd_phy_mode_set(unit, port, "tscf", BMD_PHY_MODE_SERDES, 1);
            rv = bmd_phy_mode_set(unit, port, "tscf", BMD_PHY_MODE_2LANE, 1);            
        } else {
            rv = bmd_phy_mode_set(unit, port, "tscf", BMD_PHY_MODE_SERDES, 1);
            rv = bmd_phy_mode_set(unit, port, "tscf", BMD_PHY_MODE_2LANE, 0);            
        }
        
        if (CDK_SUCCESS(rv)) {
            rv = bmd_phy_fw_helper_set(unit, port, _firmware_helper);
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
    
    CDK_PBMP_ITER(xl_pbmp, port) {
    
        if (CDK_SUCCESS(rv)) {
            rv = bmd_phy_probe(unit, port);
        }

        if (CDK_SUCCESS(rv)) {
            lanes = bcm56960_a0_port_lanes_get(unit, port);
        }

        if (lanes == 4) {
            rv = bmd_phy_mode_set(unit, port, "tsce", BMD_PHY_MODE_SERDES, 0);
        } else if (lanes == 2) {
            rv = bmd_phy_mode_set(unit, port, "tsce", BMD_PHY_MODE_SERDES, 1);
            rv = bmd_phy_mode_set(unit, port, "tsce", BMD_PHY_MODE_2LANE, 1);            
        } else {
            rv = bmd_phy_mode_set(unit, port, "tsce", BMD_PHY_MODE_SERDES, 1);
            rv = bmd_phy_mode_set(unit, port, "tsce", BMD_PHY_MODE_2LANE, 0);            
        }
        
        if (CDK_SUCCESS(rv)) {
            rv = bmd_phy_fw_helper_set(unit, port, _firmware_helper);
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

    /* Configure CLPORTs */
    CDK_PBMP_ITER(cl_pbmp, port) {
        /* Clear MAC hard reset after phy is initialized */
        if (CLPORT_SUBPORT(unit, port) == 0) {
            CLPORT_MAC_CONTROLr_CLR(clp_mac_ctrl);
            ioerr += WRITE_CLPORT_MAC_CONTROLr(unit, clp_mac_ctrl, port);
        }
        ioerr += bcm56960_a0_clport_init(unit, port);
    }

    /* Configure XLPORTs */
    CDK_PBMP_ITER(xl_pbmp, port) {
        /* Clear MAC hard reset after phy is initialized */
        if (XLPORT_SUBPORT(unit, port) == 0) {
            XLPORT_MAC_CONTROLr_CLR(xlp_mac_ctrl);
            ioerr += WRITE_XLPORT_MAC_CONTROLr(unit, xlp_mac_ctrl, port);
        }
        /* Initialize XLPORTs after XLMAC is out of reset */
        ioerr += bcm56960_a0_xlport_init(unit, port);
    }
    /* Common port initialization for CPU port */
    ioerr += _port_init(unit, CMIC_PORT);

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
            ioerr += WRITE_CMIC_CMC_HOSTMEM_ADDR_REMAPr(unit, idx,
                                                        hostmem_remap);
        }
    }  
#endif /* BMD_CONFIG_INCLUDE_DMA */    

    return ioerr ? CDK_E_IO : rv;
}
#endif /* CDK_CONFIG_INCLUDE_BCM56960_A0 */

