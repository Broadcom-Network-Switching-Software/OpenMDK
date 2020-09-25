/*
 *
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef __BCM56260_A0_INTERNAL_H__
#define __BCM56260_A0_INTERNAL_H__

#define NUM_MXQBLOCKS                  6
#define PORTS_PER_BLOCK              4

#define NUM_XLBLOCKS                    1

#define CMIC_LPORT                      0

#define COMMAND_CONFIG_SPEED_10         0x0
#define COMMAND_CONFIG_SPEED_100        0x1
#define COMMAND_CONFIG_SPEED_1000       0x2
#define COMMAND_CONFIG_SPEED_2500       0x3
#define COMMAND_CONFIG_SPEED_10000      0x4

#define XPORT_MODE_QUAD                 0x0
#define XPORT_MODE_TRI_012              0x1
#define XPORT_MODE_TRI_023              0x2
#define XPORT_MODE_DUAL                 0x3
#define XPORT_MODE_SINGLE               0x4


#define XPORT_FLAG_GXPORT       (1 << BLKTYPE_GXPORT)
#define XPORT_FLAG_XLPORT       (1 << BLKTYPE_XLPORT)
#define XPORT_FLAG_ALL          (XPORT_FLAG_GXPORT | XPORT_FLAG_XLPORT)

#define NUM_PHYS_PORTS                    29
#define MAX_VIPER_COUNT                   24

#define GS_PORT                 28
#define GS_MPORT                28
#define GS_LPORT                28

#define DRAIN_WAIT_MSEC                 500

extern int
bcm56260_a0_unicore_phy_init(int unit, int port);

extern int
bcm56260_a0_xlport_reset(int unit, int port);

extern int
bcm56260_a0_soc_port_enable_set(int unit, int port, int enable);

extern int
bcm56260_a0_mac_xl_drain_cells(int unit, int port);

extern int
bcm56260_a0_xlport_init(int unit, int port);

extern int
bcm56260_a0_tsc_block_disable(int unit, uint32_t disable_tsc);

extern int
bcm56260_a0_xlblock_number_get(int unit, int port);

extern int
bcm56260_a0_xlblock_subport_get(int unit, int port);

extern int
bcm56260_a0_xport_pbmp_get(int unit, int flag, cdk_pbmp_t *pbmp);

extern int
bcm56260_a0_p2l(int unit, int port, int inverse);

extern int
bcm56260_a0_p2m(int unit, int port, int inverse);

extern int
bcm56260_a0_xlport_block_index_get(int unit, int port);

extern int
bcm56260_a0_xlport_pbmp_get(int unit, cdk_pbmp_t *pbmp);

extern uint32_t
bcm56260_a0_port_speed_max(int unit, int port);

extern int
bcm56260_a0_mxqport_block_port_get(int unit, int port);

extern int
bcm56260_a0_mxqport_block_index_get(int unit, int port);

extern int
bcm56260_a0_mxqport_pbmp_get(int unit, cdk_pbmp_t *pbmp);

extern int
bcm56260_a0_mxqport_from_index(int unit, int blkidx, int blkport);

extern int
bcm56260_a0_xlport_from_index(int unit, int blkidx, int blkport);

extern int
bcm56260_a0_xlport_pbmp_get(int unit, cdk_pbmp_t *pbmp);

extern int
bcm56260_a0_tdm_default(int unit, const int **tdm_seq);

extern int
bcm56260_a0_port_num_lanes(int unit, int port);

extern int
bcm56260_a0_mxq_phy_mode_get(int unit, int port, int speed, 
                                    int *phy_mode, int *num_lanes);
                                    
extern int
bcm56260_a0_xl_phy_core_port_mode(int unit, int port, int *phy_mode_xl, int *core_mode_xl);
#define P2PP(_u,_p) bcm56260_a0_p2l(_u, _p, 0)
#define PP2P(_u,_p) bcm56260_a0_p2l(_u, _p, 1)
#define P2M(_u,_p) bcm56260_a0_p2l(_u, _p, 0)
#define M2P(_u,_p) bcm56260_a0_p2l(_u, _p, 1)
#define P2L(_u,_p) bcm56260_a0_p2l(_u, _p, 0)
#define L2P(_u,_p) bcm56260_a0_p2l(_u, _p, 1)

#define XLPORT_SUBPORT(_u,_p) bcm56260_a0_xlblock_subport_get(_u,_p)
#define MXQPORT_SUBPORT(_u, _p)  bcm56260_a0_mxqport_block_port_get(_u, _p)

#define MXQPORT_BLKIDX(_u, _p)   bcm56260_a0_mxqport_block_index_get(_u, _p)
#define XLPORT_BLKIDX(_u, _p)   bcm56260_a0_xlport_block_index_get(_u, _p)

#endif /* __BCM56260_A0_INTERNAL_H__ */
