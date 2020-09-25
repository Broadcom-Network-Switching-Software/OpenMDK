/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef __BCM56160_A0_INTERNAL_H__
#define __BCM56160_A0_INTERNAL_H__

#define CMIC_LPORT                      0
#define NUM_PHYS_PORTS                  42

#define TSC_MAX_BLK_COUNT               2
#define QTC_MAX_BLK_COUNT               2
#define QGPHY_MAX_BLK_COUNT             2
#define PORT_COUNT_PER_TSC              4
#define PORT_COUNT_PER_QTC              16
#define QGPHY_COUNT_PER_QTC             8

#define COMMAND_CONFIG_SPEED_10         0x0
#define COMMAND_CONFIG_SPEED_100        0x1
#define COMMAND_CONFIG_SPEED_1000       0x2
#define COMMAND_CONFIG_SPEED_2500       0x3
#define COMMAND_CONFIG_SPEED_10000      0x4

#define XLPORT_MODE_QUAD                0x0
#define XLPORT_MODE_TRI_012             0x1
#define XLPORT_MODE_TRI_023             0x2
#define XLPORT_MODE_DUAL                0x3
#define XLPORT_MODE_SINGLE              0x4

enum block_port_ratio_e {
    BLOCK_PORT_RATIO_SINGLE,
    BLOCK_PORT_RATIO_DUAL_1_1,
    BLOCK_PORT_RATIO_DUAL_2_2,
    BLOCK_PORT_RATIO_TRI_1_1_2,
    BLOCK_PORT_RATIO_QUAD,
    BLOCK_PORT_RATIO_OCTAL,
    BLOCK_PORT_RATIO_NONE
};

extern int
bcm56160_a0_xlport_reset(int unit, int port);

extern int
bcm56160_a0_gport_reset(int unit, int blk);

extern int
bcm56160_a0_xlport_init(int unit, int port);

extern int
bcm56160_a0_block_disable(int unit, uint32_t disable_tsc, uint32_t disable_qtc);

extern int
bcm56160_a0_p2l(int unit, int port, int inverse);

extern int
bcm56160_a0_p2m(int unit, int port, int inverse);

extern int
bcm56160_a0_qgphy_pbmp_get(int unit, cdk_pbmp_t *pbmp);

extern int
bcm56160_a0_gport_pbmp_get(int unit, cdk_pbmp_t *pbmp);

extern int
bcm56160_a0_xlport_pbmp_get(int unit, cdk_pbmp_t *pbmp);

extern int
bcm56160_a0_block_index_get(int unit, int blk_type, int port);

extern int
bcm56160_a0_block_number_get(int unit, int blk_type, int port);

extern int
bcm56160_a0_block_subport_get(int unit, int blk_type, int port);

extern uint32_t
bcm56160_a0_port_speed_max(int unit, int port);

extern int
bcm56160_a0_tdm_default(int unit, const int **tdm_seq);

extern int
bcm56160_a0_qtcport_ratio(int unit, int port);

extern int
bcm56160_a0_xlport_ratio(int unit, int port);

extern int
bcm56160_a0_port_num_lanes(int unit, int port);

#define P2L(_u,_p) bcm56160_a0_p2l(_u,_p,0)
#define L2P(_u,_p) bcm56160_a0_p2l(_u,_p,1)

#define P2M(_u,_p) bcm56160_a0_p2m(_u,_p,0)
#define M2P(_u,_p) bcm56160_a0_p2m(_u,_p,1)

#define XLPORT_SUBPORT(_u, _p) ((_p - 2) & 0x3)
#define XPORT_BLKIDX(_u, _b, _p)     bcm56160_a0_block_index_get(_u, _b, _p)
#define XPORT_BLKNUM(_u, _b, _p)     bcm56160_a0_block_number_get(_u, _b, _p)
#define XPORT_SUBPORT(_u, _b, _p)    bcm56160_a0_block_subport_get(_u, _b, _p)

#endif /* __BCM56160_A0_INTERNAL_H__ */
