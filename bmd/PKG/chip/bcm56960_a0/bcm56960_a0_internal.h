/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef __BCM56960_A0_INTERNAL_H__
#define __BCM56960_A0_INTERNAL_H__

#define NUM_PHYS_PORTS                  136
#define NUM_LOGIC_PORTS                 136
#define NUM_MMU_PORTS                   226

#define TH_MGMT_PORT_0                  129
#define TH_MGMT_PORT_1                  131

#define TH_PORTS_PER_PBLK               4
#define TH_PBLKS_PER_PIPE               8
#define TH_PIPES_PER_DEV                4
#define TH_XPES_PER_DEV                 4

#define TH_PBLKS_PER_DEV                (TH_PBLKS_PER_PIPE * TH_PIPES_PER_DEV)

#define TH_PORTS_PER_PIPE               (TH_PORTS_PER_PBLK * TH_PBLKS_PER_PIPE)
#define TH_PORTS_PER_DEV                (TH_PORTS_PER_PIPE * TH_PIPES_PER_DEV)

#define TH_NUM_PIPES                    4
#define TH_NUM_XPES                     4
#define TH_NUM_SLICES                   2
#define TH_NUM_LAYERS                   2

#define TH_MMU_MAX_PACKET_BYTES         9416  /* bytes */
#define TH_MMU_PACKET_HEADER_BYTES      64    /* bytes */
#define TH_MMU_JUMBO_FRAME_BYTES        9216  /* bytes */
#define TH_MMU_DEFAULT_MTU_BYTES        1536  /* bytes */

#define TH_MMU_PHYSICAL_CELLS_PER_XPE   23040 /* Total Physical cells per XPE */
#define TH_MMU_TOTAL_CELLS_PER_XPE      20165 /* 4MB/XPE = 20165 cells/XPE */
#define TH_MMU_RSVD_CELLS_CFAP_PER_XPE  1080  /* Reserved CFAP cells per XPE */

/* Default number of PM in chip */
#define NUM_TSC                         32

/* Number of physical ports plus CPU and loopback */
#define TH_NUM_EXT_PORTS                136

/* Length of linerate speed groups */
#define TH_LR_GROUP_LEN                 256

/* Length of oversub speed groups */
#define TH_OS_GROUP_LEN                 12

/* Number of port modules */
#define TH_NUM_PORT_MODULES             (NUM_TSC + 1)

#define XPORT_MODE_QUAD                 0x0
#define XPORT_MODE_TRI_012              0x1
#define XPORT_MODE_TRI_023              0x2
#define XPORT_MODE_DUAL                 0x3
#define XPORT_MODE_SINGLE               0x4

#define TH_MMU_MAX_PACKET_BYTES         9416  /* bytes */
#define TH_MMU_PACKET_HEADER_BYTES      64    /* bytes */
#define TH_MMU_JUMBO_FRAME_BYTES        9216  /* bytes */
#define TH_MMU_DEFAULT_MTU_BYTES        1536  /* bytes */

#define TH_MMU_PHYSICAL_CELLS_PER_XPE   23040 /* Total Physical cells per XPE */
#define TH_MMU_TOTAL_CELLS_PER_XPE      20165 /* 4MB/XPE = 20165 cells/XPE */
#define TH_MMU_RSVD_CELLS_CFAP_PER_XPE  1080  /* Reserved CFAP cells per XPE */

#define TH_MMU_TOTAL_CELLS              (TH_MMU_TOTAL_CELLS_PER_XPE * TH_XPES_PER_DEV)
#define TH_MMU_PHYSICAL_CELLS           (TH_MMU_PHYSICAL_CELLS_PER_XPE * TH_XPES_PER_DEV)

#define TH_MMU_BYTES_PER_CELL           208   /* bytes (1664 bits) */
#define TH_MMU_NUM_PG                   8
#define TH_MMU_NUM_POOL                 4
#define TH_MMU_NUM_RQE_QUEUES           11
#define TH_MMU_NUM_INT_PRI              16

#define TH_NUM_UC_QUEUES_PER_PIPE       330 /* Num UC Q's Per pipe 33 * 10 */
#define TH_MMU_MCQ_ENTRY_PER_XPE        8192
#define TH_MMU_RQE_ENTRY_PER_XPE        1024

/* For manipulating port bitmap memory fields */
#define PBM_PORT_WORDS                  ((NUM_PHYS_PORTS / 32) + 1)
#define PBM_LPORT_WORDS                 ((NUM_LOGIC_PORTS / 32) + 1)
#define PBM_MEMBER(_pbm, _port) \
        ((_pbm)[(_port) >> 5] & LSHIFT32(1, (_port) & 0x1f))
#define PBM_PORT_ADD(_pbm, _port) \
        ((_pbm)[(_port) >> 5] |= LSHIFT32(1, (_port) & 0x1f))
#define PBM_PORT_REMOVE(_pbm, _port) \
        ((_pbm)[(_port) >> 5] &= ~(LSHIFT32(1, (_port) & 0x1f)))

enum port_speed_e {
    SPEED_0=0, SPEED_10M=10, SPEED_20M=20, SPEED_25M=25,
    SPEED_33M=33, SPEED_40M=40, SPEED_50M=50, SPEED_100M=100,
    SPEED_100M_FX=101, SPEED_120M=120,SPEED_400M=400,
    SPEED_1G=1000, SPEED_1G_FX=1001, SPEED_1p2G=1200,
    SPEED_2G=2000, SPEED_2p5G=2500, SPEED_4G=4000,
    SPEED_5G=5000, SPEED_7p5G=7500, SPEED_10G=10000,
    SPEED_10G_DUAL=10001, SPEED_10G_XAUI=10002, SPEED_11G=11000,
    SPEED_12G=12000, SPEED_12p5G=12500, SPEED_13G=13000,
    SPEED_15G=15000, SPEED_16G=16000, SPEED_20G=20000,
    SPEED_21G=21000, SPEED_21G_DUAL=21010, SPEED_24G=24000,
    SPEED_25G=25000, SPEED_27G=27000, SPEED_30G=30000,
    SPEED_40G=40000, SPEED_42G=40005, SPEED_42G_HG2=42000,
    SPEED_50G=50000, SPEED_53G=53000, SPEED_75G=75000,
    SPEED_82G=82000, SPEED_100G=100000, SPEED_106G=106000,
    SPEED_120G=120000, SPEED_126G=126000
};

enum th_port_ratio_e {
    PORT_RATIO_SINGLE,
    PORT_RATIO_DUAL_1_1,
    PORT_RATIO_DUAL_2_1,
    PORT_RATIO_DUAL_1_2,
    PORT_RATIO_TRI_023_2_1_1,
    PORT_RATIO_TRI_023_4_1_1,
    PORT_RATIO_TRI_012_1_1_2,
    PORT_RATIO_TRI_012_1_1_4,
    PORT_RATIO_QUAD,
    PORT_RATIO_COUNT
};

/* Types of ports based on traffic */
#define PORT_ETHERNET                   998
#define PORT_HIGIG2                     999

/* IDB: Idle slot guaranteed for refresh */
/* Oversub token */
#define OVSB_TOKEN                      250
/* IDB: Idle slot for memreset, L2 management, sbus, other */
/* MMU: No explicit definition */
#define IDL1_TOKEN                      251
/* MMU: Purge */
#define IDL2_TOKEN                      252
/* IDB: NULL slot, no pick, no opportunistic */
/* MMU: NULL slot, no pick, no opportunistic */
#define NULL_TOKEN                      253



typedef struct _tdm_pipes_s {
    int idb_tdm_tbl_0[TH_LR_GROUP_LEN];
    int idb_tdm_ovs_0_a[TH_OS_GROUP_LEN];
    int idb_tdm_ovs_0_b[TH_OS_GROUP_LEN];
    int idb_tdm_ovs_0_c[TH_OS_GROUP_LEN];
    int idb_tdm_ovs_0_d[TH_OS_GROUP_LEN];
    int idb_tdm_ovs_0_e[TH_OS_GROUP_LEN];
    int idb_tdm_ovs_0_f[TH_OS_GROUP_LEN];
    int idb_tdm_ovs_0_g[TH_OS_GROUP_LEN];
    int idb_tdm_ovs_0_h[TH_OS_GROUP_LEN];
    int idb_tdm_tbl_1[TH_LR_GROUP_LEN];
    int idb_tdm_ovs_1_a[TH_OS_GROUP_LEN];
    int idb_tdm_ovs_1_b[TH_OS_GROUP_LEN];
    int idb_tdm_ovs_1_c[TH_OS_GROUP_LEN];
    int idb_tdm_ovs_1_d[TH_OS_GROUP_LEN];
    int idb_tdm_ovs_1_e[TH_OS_GROUP_LEN];
    int idb_tdm_ovs_1_f[TH_OS_GROUP_LEN];
    int idb_tdm_ovs_1_g[TH_OS_GROUP_LEN];
    int idb_tdm_ovs_1_h[TH_OS_GROUP_LEN];
    int idb_tdm_tbl_2[TH_LR_GROUP_LEN];
    int idb_tdm_ovs_2_a[TH_OS_GROUP_LEN];
    int idb_tdm_ovs_2_b[TH_OS_GROUP_LEN];
    int idb_tdm_ovs_2_c[TH_OS_GROUP_LEN];
    int idb_tdm_ovs_2_d[TH_OS_GROUP_LEN];
    int idb_tdm_ovs_2_e[TH_OS_GROUP_LEN];
    int idb_tdm_ovs_2_f[TH_OS_GROUP_LEN];
    int idb_tdm_ovs_2_g[TH_OS_GROUP_LEN];
    int idb_tdm_ovs_2_h[TH_OS_GROUP_LEN];
    int idb_tdm_tbl_3[TH_LR_GROUP_LEN];
    int idb_tdm_ovs_3_a[TH_OS_GROUP_LEN];
    int idb_tdm_ovs_3_b[TH_OS_GROUP_LEN];
    int idb_tdm_ovs_3_c[TH_OS_GROUP_LEN];
    int idb_tdm_ovs_3_d[TH_OS_GROUP_LEN];
    int idb_tdm_ovs_3_e[TH_OS_GROUP_LEN];
    int idb_tdm_ovs_3_f[TH_OS_GROUP_LEN];
    int idb_tdm_ovs_3_g[TH_OS_GROUP_LEN];
    int idb_tdm_ovs_3_h[TH_OS_GROUP_LEN];
    int mmu_tdm_tbl_0[TH_LR_GROUP_LEN];
    int mmu_tdm_ovs_0_a[TH_OS_GROUP_LEN];
    int mmu_tdm_ovs_0_b[TH_OS_GROUP_LEN];
    int mmu_tdm_ovs_0_c[TH_OS_GROUP_LEN];
    int mmu_tdm_ovs_0_d[TH_OS_GROUP_LEN];
    int mmu_tdm_ovs_0_e[TH_OS_GROUP_LEN];
    int mmu_tdm_ovs_0_f[TH_OS_GROUP_LEN];
    int mmu_tdm_ovs_0_g[TH_OS_GROUP_LEN];
    int mmu_tdm_ovs_0_h[TH_OS_GROUP_LEN];
    int mmu_tdm_tbl_1[TH_LR_GROUP_LEN];
    int mmu_tdm_ovs_1_a[TH_OS_GROUP_LEN];
    int mmu_tdm_ovs_1_b[TH_OS_GROUP_LEN];
    int mmu_tdm_ovs_1_c[TH_OS_GROUP_LEN];
    int mmu_tdm_ovs_1_d[TH_OS_GROUP_LEN];
    int mmu_tdm_ovs_1_e[TH_OS_GROUP_LEN];
    int mmu_tdm_ovs_1_f[TH_OS_GROUP_LEN];
    int mmu_tdm_ovs_1_g[TH_OS_GROUP_LEN];
    int mmu_tdm_ovs_1_h[TH_OS_GROUP_LEN];
    int mmu_tdm_tbl_2[TH_LR_GROUP_LEN];
    int mmu_tdm_ovs_2_a[TH_OS_GROUP_LEN];
    int mmu_tdm_ovs_2_b[TH_OS_GROUP_LEN];
    int mmu_tdm_ovs_2_c[TH_OS_GROUP_LEN];
    int mmu_tdm_ovs_2_d[TH_OS_GROUP_LEN];
    int mmu_tdm_ovs_2_e[TH_OS_GROUP_LEN];
    int mmu_tdm_ovs_2_f[TH_OS_GROUP_LEN];
    int mmu_tdm_ovs_2_g[TH_OS_GROUP_LEN];
    int mmu_tdm_ovs_2_h[TH_OS_GROUP_LEN];
    int mmu_tdm_tbl_3[TH_LR_GROUP_LEN];
    int mmu_tdm_ovs_3_a[TH_OS_GROUP_LEN];
    int mmu_tdm_ovs_3_b[TH_OS_GROUP_LEN];
    int mmu_tdm_ovs_3_c[TH_OS_GROUP_LEN];
    int mmu_tdm_ovs_3_d[TH_OS_GROUP_LEN];
    int mmu_tdm_ovs_3_e[TH_OS_GROUP_LEN];
    int mmu_tdm_ovs_3_f[TH_OS_GROUP_LEN];
    int mmu_tdm_ovs_3_g[TH_OS_GROUP_LEN];
    int mmu_tdm_ovs_3_h[TH_OS_GROUP_LEN];
} th_tdm_pipes_t;

typedef struct _tdm_globals_s {
    enum port_speed_e speed[TH_NUM_EXT_PORTS];
    int clk_freq;
    int port_rates_array[TH_NUM_EXT_PORTS];
    int pm_encap_type[TH_NUM_PORT_MODULES];
} th_tdm_globals_t;

extern int
bcm56960_a0_p2l(int unit, int port, int inverse);

extern int
bcm56960_a0_p2m(int unit, int port, int inverse);

extern uint32_t
bcm56960_a0_port_speed_max(int unit, int port);

extern int
bcm56960_a0_xlport_pbmp_get(int unit, cdk_pbmp_t *pbmp);

extern int
bcm56960_a0_clport_pbmp_get(int unit, cdk_pbmp_t *pbmp);

extern int
bcm56960_a0_all_pbmp_get(int unit, cdk_pbmp_t *pbmp);

extern int
bcm56960_a0_eq_pbmp_get(int unit, cdk_pbmp_t *pbmp);

extern int
bcm56960_a0_lpbk_pbmp_get(int unit,cdk_pbmp_t * pbmp);

extern int
bcm56960_a0_oversub_map_get(int unit, cdk_pbmp_t *oversub_map);

extern int
bcm56960_a0_block_port_get(int unit, int port, int blktype);

extern int
bcm56960_a0_block_index_get(int unit, int port, int blktype);

extern int
bcm56960_a0_port_pipe_get(int unit, int port);

extern int 
bcm56960_a0_idbport_get(int unit, int port);

extern int
bcm56960_a0_port_lanes_get(int unit, int port);

extern int
bcm56960_a0_pipe_map_get(int unit, uint32_t *pipe_map);

extern int
bcm56960_a0_port_serdes_get(int unit, int port);

extern int
bcm56960_a0_port_ratio_get(int unit, int clport);

extern int 
bcm56960_a0_num_cosq(int unit, int port);

extern int 
bcm56960_a0_num_cosq_base(int unit, int port);

extern int 
bcm56960_a0_num_uc_cosq(int unit, int port);

extern int 
bcm56960_a0_num_uc_cosq_base(int unit, int port);

extern int 
bcm56960_a0_set_tdm_tbl(th_tdm_globals_t *tdm_globals, 
                               th_tdm_pipes_t *tdm_pipe_tables);

extern int
bcm56960_a0_clport_init(int unit, int port);

extern int
bcm56960_a0_xlport_init(int unit, int port);


#define P2L(_u,_p) bcm56960_a0_p2l(_u,_p,0)
#define L2P(_u,_p) bcm56960_a0_p2l(_u,_p,1)

#define P2M(_u,_p) bcm56960_a0_p2m(_u,_p,0)
#define M2P(_u,_p) bcm56960_a0_p2m(_u,_p,1)

#define CLPORT_SUBPORT(_u,_p) bcm56960_a0_block_port_get(_u,_p, BLKTYPE_CLPORT)
#define XLPORT_SUBPORT(_u,_p) bcm56960_a0_block_port_get(_u,_p, BLKTYPE_XLPORT)

#endif /* __BCM56960_A0_INTERNAL_H__ */
