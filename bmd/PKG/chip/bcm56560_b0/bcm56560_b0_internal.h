/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef __BCM56560_B0_INTERNAL_H__
#define __BCM56560_B0_INTERNAL_H__

#define NUM_PHYS_PORTS          75
#define NUM_LOGIC_PORTS         75
#define NUM_MMU_PORTS           75

#define CMIC_LPORT              0
#define CMIC_PPORT              0
#define CMIC_MPORT              72
#define CMIC_HG_LPORT           106

#define LB_LPORT                73
#define LB_PPORT                73
#define LB_MPORT                73

#define RDB_LPORT                74
#define RDB_PPORT                74
#define RDB_MPORT                74

#define PORTS_PER_XLP           4
#define XLPS_PER_PGW            9
#define PGWS_PER_QUAD           1
#define QUADS_PER_PIPE          2
#define PIPES_PER_DEV           1
#define PORTS_PER_TSC           4

#define QUADS_PER_DEV           (QUADS_PER_PIPE * PIPES_PER_DEV)

#define PGWS_PER_PIPE           (PGWS_PER_QUAD * QUADS_PER_PIPE)
#define PGWS_PER_DEV            (PGWS_PER_PIPE * PIPES_PER_DEV)

#define PORTS_PER_PGW           (PORTS_PER_XLP * XLPS_PER_PGW)
#define PORTS_PER_QUAD          (PORTS_PER_PGW * PGWS_PER_QUAD)
#define PORTS_PER_PIPE          75

#define XPORT_MODE_QUAD         0
#define XPORT_MODE_TRI_012      1
#define XPORT_MODE_TRI_023      2
#define XPORT_MODE_DUAL         3
#define XPORT_MODE_SINGLE       4
#define XPORT_MODE_100G_4_4_2   5
#define XPORT_MODE_100G_3_4_3   6
#define XPORT_MODE_100G_2_4_4   7

#define NUM_UC_Q_PER_PIPE       1480
#define NUM_MC_Q_PER_PIPE       568
#define NUM_Q_PER_PIPE          (NUM_UC_Q_PER_PIPE + NUM_MC_Q_PER_PIPE)
#define UC_Q_BASE_CMIC_PORT     876
#define UC_Q_BASE_LB_PORT       864
#define MC_Q_BASE_CMIC_PORT     17114
#define MC_Q_BASE_LB_PORT       17104
#define MC_Q_BASE               16384

#define PORT_IN_Y_PIPE(_p)      ((_p) >= (NUM_PHYS_PORTS >> 1))

/* For manipulating port bitmap memory fields */
#define PBM_PORT_WORDS          ((NUM_PHYS_PORTS / 32) + 1)
#define PBM_LPORT_WORDS         ((NUM_LOGIC_PORTS / 32) + 1)
#define PBM_MEMBER(_pbm, _port) \
     ((_pbm)[(_port) >> 5] & LSHIFT32(1, (_port) & 0x1f))
#define PBM_PORT_ADD(_pbm, _port) \
     ((_pbm)[(_port) >> 5] |= LSHIFT32(1, (_port) & 0x1f))
#define PBM_PORT_REMOVE(_pbm, _port) \
     ((_pbm)[(_port) >> 5] &= ~(LSHIFT32(1, (_port) & 0x1f)))

extern void *
bcm56560_b0_tdm_malloc(int unit, int size);

extern void
bcm56560_b0_tdm_free(int unit, void *addr);

extern int
bcm56560_b0_tsc_xgxs_reset(int unit, int port, int tsc_idx);

extern int
bcm56560_b0_cport_init(int unit, int port);

extern int
bcm56560_b0_xlport_init(int unit, int port);

extern int
bcm56560_b0_clport_init(int unit, int port);

extern int
bcm56560_b0_clg2port_init(int unit, int port);

extern int
bcm56560_b0_get_core_frequency(int unit);

extern int
bcm56560_b0_port_in_eq_bmp(int unit, int port);

extern uint32_t
bcm56560_b0_port_speed_max(int unit, int port);

extern int
bcm56560_b0_port_lanes_get(int unit, int port);

extern int
bcm56560_b0_mmu_port_mc_queues(int unit, int port);

extern int
bcm56560_b0_mmu_port_uc_queues(int unit, int port);

extern int
bcm56560_b0_mmu_port_uc_queue_index(int unit, int port);

extern int
bcm56560_b0_mc_queue_num(int unit, int port, int cosq);

extern int
bcm56560_b0_clport_pbmp_get(int unit, cdk_pbmp_t *pbmp);

extern int
bcm56560_b0_cxxport_pbmp_get(int unit, cdk_pbmp_t *pbmp);

extern int
bcm56560_b0_clg2port_pbmp_get(int unit, cdk_pbmp_t *pbmp);

extern int
bcm56560_b0_xlport_pbmp_get(int unit, cdk_pbmp_t *pbmp);

extern int
bcm56560_b0_all_pbmp_get(int unit, cdk_pbmp_t *pbmp);

extern int
bcm56560_b0_all_front_pbmp_get(int unit, cdk_pbmp_t *pbmp);

void
bcm56560_b0_mmu_pbmp_get(int unit, cdk_pbmp_t *pbmp);

void
bcm56560_b0_ovrs_pbmp_get(int unit, cdk_pbmp_t *pbmp);

extern int
bcm56560_b0_is_ovrs_port(int unit, int port);

extern int
bcm56560_b0_clport_block_index_get(int unit, int port);

extern int
bcm56560_b0_xlport_block_index_get(int unit, int port);

extern int
bcm56560_b0_pgw_clport_block_index_get(int unit, int port, int *obm, int *sub_port);

extern int
bcm56560_b0_pgw_clport_block_number_get(int unit, int port);

extern int
bcm56560_b0_p2l(int unit, int port, int inverse);

extern int
bcm56560_b0_p2m(int unit, int port, int inverse);

extern int
bcm56560_b0_egr_buf_reset(int unit, int port, int reset);

extern int
bcm56560_b0_portctrl_enable_set(int unit, int port, uint32_t flags, int enable);

extern void
bcm56560_b0_xlmac_drain_cell(int unit, int port);

extern void
bcm56560_b0_clmac_drain_cell(int unit, int port);

extern void
bcm56560_b0_clg2mac_drain_cell(int unit, int port);

extern void
bcm56560_b0_idb_buf_reset(int unit, int port, int reset);

extern int
bcm56560_b0_port_is_falcon(int unit, int phy_port);

#define P2L(_u,_p) bcm56560_b0_p2l(_u,_p,0)
#define L2P(_u,_p) bcm56560_b0_p2l(_u,_p,1)

#define P2M(_u,_p) bcm56560_b0_p2m(_u,_p,0)
#define M2P(_u,_p) bcm56560_b0_p2m(_u,_p,1)
#define SPEED_MAX(_u,_p) bcm56560_b0_port_speed_max(_u,_p)

#define IS_100G_DISABLED(_u,clp_blk) (CDK_XGSD_FLAGS(unit) & CHIP_FLAG_CL100G)? 0 : 1

#define XLPORT_BLKIDX(_p) ((_p - 1) >> 2)
#define XLPORT_SUBPORT(_p) ((_p - 1) & 0x3)

#define PGW_BLKIDX(_p) ((_p - 1) >> 4)

#define IS_RDB_PORT(_p) 

#define IS_OVERSUB_PORT(_u,_p) bcm56560_b0_is_ovrs_port(_u,_p)

#endif /* __BCM56560_B0_INTERNAL_H__ */
