/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef __BCM56850_A0_INTERNAL_H__
#define __BCM56850_A0_INTERNAL_H__

#define NUM_PHYS_PORTS          130
#define NUM_LOGIC_PORTS         106
#define NUM_MMU_PORTS           108

#define CMIC_LPORT              0
#define CMIC_MPORT              52
#define CMIC_HG_LPORT           106

#define LB_LPORT                105
#define LB_MPORT                116

#define XLPS_PER_PGW            4
#define PGWS_PER_PIPE           4
#define PIPES_PER_DEV           2
#define PORTS_PER_XLP           4
#define PGWS_PER_DEV            (PIPES_PER_DEV * PGWS_PER_PIPE) 
#define PORTS_PER_PGW           (PORTS_PER_XLP * XLPS_PER_PGW)
#define PORTS_PER_PIPE          (PORTS_PER_PGW * PGWS_PER_PIPE) 

#define XPORT_MODE_QUAD         0
#define XPORT_MODE_TRI_012      1
#define XPORT_MODE_TRI_023      2
#define XPORT_MODE_DUAL         3
#define XPORT_MODE_SINGLE       4

#define XMAC_MODE_10M           0
#define XMAC_MODE_100M          1
#define XMAC_MODE_1G            2
#define XMAC_MODE_2G5           3
#define XMAC_MODE_10G_PLUS      4

#define PORT_IN_Y_PIPE(_p)      ((_p) >= (NUM_PHYS_PORTS>>1))

/* For manipulating port bitmap memory fields */
#define PBM_PORT_WORDS          ((NUM_PHYS_PORTS / 32) + 1)
#define PBM_LPORT_WORDS         ((NUM_LOGIC_PORTS / 32) + 1)
#define PBM_MEMBER(_pbm, _port) \
     ((_pbm)[(_port) >> 5] & LSHIFT32(1, (_port) & 0x1f))
#define PBM_PORT_ADD(_pbm, _port) \
     ((_pbm)[(_port) >> 5] |= LSHIFT32(1, (_port) & 0x1f))
#define PBM_PORT_REMOVE(_pbm, _port) \
     ((_pbm)[(_port) >> 5] &= ~(LSHIFT32(1, (_port) & 0x1f)))

extern uint32_t
bcm56850_a0_port_speed_max(int unit, int port);

extern int
bcm56850_a0_mmu_port_mc_queues(int unit, int port);

extern int
bcm56850_a0_mmu_port_uc_queues(int unit, int port);

extern int
bcm56850_a0_mc_queue_num(int unit, int port, int cosq);

extern int
bcm56850_a0_uc_queue_num(int unit, int port, int cosq);

extern int
bcm56850_a0_warpcore_phy_init(int unit, int port);

extern int
bcm56850_a0_wait_for_tsc_lock(int unit, int port);

extern int
bcm56850_a0_xport_reset(int unit, int port);

extern int
bcm56850_a0_xport_init(int unit, int port);

extern int
bcm56850_a0_xlport_pbmp_get(int unit, cdk_pbmp_t *pbmp);

extern int
bcm56850_a0_all_pbmp_get(int unit, cdk_pbmp_t *pbmp);

extern int
bcm56850_a0_p2l(int unit, int port, int inverse);

extern int
bcm56850_a0_p2m(int unit, int port, int inverse);

extern int
bcm56850_a0_port_enable_set(int unit, int port, int enable);

extern int
bcm56850_a0_set_tdm_tbl(
    int speed[130],
    int tdm_bw, 
    int pgw_tdm_tbl_x0[32],
    int ovs_tdm_tbl_x0[32],
    int ovs_spacing_x0[32],
    int pgw_tdm_tbl_x1[32],
    int ovs_tdm_tbl_x1[32],
    int ovs_spacing_x1[32],
    int pgw_tdm_tbl_y0[32],
    int ovs_tdm_tbl_y0[32],
    int ovs_spacing_y0[32],
    int pgw_tdm_tbl_y1[32],
    int ovs_tdm_tbl_y1[32],
    int ovs_spacing_y1[32],
    int mmu_tdm_tbl_x[256],
    int mmu_tdm_ovs_x_1[16],
    int mmu_tdm_ovs_x_2[16],
    int mmu_tdm_ovs_x_3[16],
    int mmu_tdm_ovs_x_4[16],
    int mmu_tdm_tbl_y[256],
    int mmu_tdm_ovs_y_1[16],
    int mmu_tdm_ovs_y_2[16],
    int mmu_tdm_ovs_y_3[16],
    int mmu_tdm_ovs_y_4[16],
    int port_state_map[128],
    int iarb_tdm_tbl_x[512],
    int iarb_tdm_tbl_y[512]);

extern int
bcm56850_a0_set_iarb_tdm_table(
    int core_bw,
    int is_x_ovs,
    int is_y_ovs,
    int mgm4x1,
    int mgm4x2p5,
    int mgm1x10,
    int *iarb_tdm_wrap_ptr_x,
    int *iarb_tdm_wrap_ptr_y,
    int iarb_tdm_tbl_x[512],
    int iarb_tdm_tbl_y[512]);

#define P2L(_u,_p) bcm56850_a0_p2l(_u,_p,0)
#define L2P(_u,_p) bcm56850_a0_p2l(_u,_p,1)

#define P2M(_u,_p) bcm56850_a0_p2m(_u,_p,0)
#define M2P(_u,_p) bcm56850_a0_p2m(_u,_p,1)

#define XLPORT_BLKIDX(_p) ((_p - 1) >> 2)
#define XLPORT_SUBPORT(_p) ((_p - 1) & 0x3)

#endif /* __BCM56850_A0_INTERNAL_H__ */
