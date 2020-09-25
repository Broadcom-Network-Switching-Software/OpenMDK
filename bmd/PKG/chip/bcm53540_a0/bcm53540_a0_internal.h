/*
 *
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef __BCM53540_A0_INTERNAL_H__
#define __BCM53540_A0_INTERNAL_H__

#define CMIC_LPORT                      0
#define NUM_PHYS_PORTS                  38
#define PHY_GPORT4_BASE                 34

#define QGPHY_MAX_BLK_COUNT             6

#define COMMAND_CONFIG_SPEED_10         0x0
#define COMMAND_CONFIG_SPEED_100        0x1
#define COMMAND_CONFIG_SPEED_1000       0x2
#define COMMAND_CONFIG_SPEED_2500       0x3

#define OPT_CONFIG_MASK                 0xff

extern int
bcm53540_a0_p2l(int unit, int port, int inverse);

extern int
bcm53540_a0_p2m(int unit, int port, int inverse);

extern int
bcm53540_a0_all_pbmp_get(int unit, cdk_pbmp_t *pbmp);

extern int
bcm53540_a0_gport_pbmp_get(int unit, cdk_pbmp_t *pbmp);

extern int
bcm53540_a0_block_index_get(int unit, int blk_type, int port);

extern int
bcm53540_a0_block_number_get(int unit, int blk_type, int port);

extern int
bcm53540_a0_block_subport_get(int unit, int blk_type, int port);

extern uint32_t
bcm53540_a0_port_speed_max(int unit, int port);

extern int
bcm53540_a0_tdm_default(int unit, const int **tdm_seq);

extern int
bcm53540_a0_core_clock(int unit);

extern uint32_t
bcm53540_a0_qgphy_core_map(int unit);

extern uint32_t
bcm53540_a0_qgphy5_lane(int unit);

extern uint32_t
bcm53540_a0_sgmii_4p0_lane(int unit);

extern int
bcm53540_a0_gphy_get(int unit, int port);

extern int
bcm53540_a0_sku_option_get(int unit);

#define P2L(_u,_p) bcm53540_a0_p2l(_u,_p,0)
#define L2P(_u,_p) bcm53540_a0_p2l(_u,_p,1)

#define P2M(_u,_p) bcm53540_a0_p2m(_u,_p,0)
#define M2P(_u,_p) bcm53540_a0_p2m(_u,_p,1)

#define SPEED_MAX(_u,_p) bcm53540_a0_port_speed_max(_u,_p)
#define CORE_CLK(_u) bcm53540_a0_core_clock(_u)
#define QGPHY_CORE(_u) bcm53540_a0_qgphy_core_map(_u)

#define QGPHY5_LANE(_u) bcm53540_a0_qgphy5_lane(_u)

#define SGMII_4P0_LANE(_u) bcm53540_a0_sgmii_4p0_lane(_u)

#define IS_GPHY(_u,_p) bcm53540_a0_gphy_get(_u, _p)

/* PGW_GE or PMQ block number: 0:port2-13, 1:port18-29, 2: port34 */
#define PMQ_BLKNUM(_p) ((_p - 2) >> 4)

#define SUBPORT(_u,_b,_p)    bcm53540_a0_block_subport_get(_u, _b, _p)

#endif /* __BCM53540_A0_INTERNAL_H__ */
