/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef __BCM53400_A0_INTERNAL_H__
#define __BCM53400_A0_INTERNAL_H__

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

#define NUM_PHYS_PORTS                  38
#define MAX_TSC_COUNT                   6

extern int
bcm53400_a0_xlport_reset(int unit, int port);

extern int
bcm53400_a0_xlport_init(int unit, int port);

extern int
bcm53400_a0_tsc_block_disable(int unit, uint32_t disable_tsc);

extern int
bcm53400_a0_xlblock_number_get(int unit, int port);

extern int
bcm53400_a0_xlblock_subport_get(int unit, int port);

extern int
bcm53400_a0_xport_pbmp_get(int unit, int flag, cdk_pbmp_t *pbmp);

extern int
bcm53400_a0_p2l(int unit, int port, int inverse);

extern int
bcm53400_a0_p2m(int unit, int port, int inverse);

extern uint32_t
bcm53400_a0_port_speed_max(int unit, int port);

extern int
bcm53400_a0_tdm_default(int unit, const int **tdm_seq);

extern int
bcm53400_a0_port_num_lanes(int unit, int port);

#define P2L(_u,_p) bcm53400_a0_p2l(_u,_p,0)
#define L2P(_u,_p) bcm53400_a0_p2l(_u,_p,1)

#define P2M(_u,_p) bcm53400_a0_p2m(_u,_p,0)
#define M2P(_u,_p) bcm53400_a0_p2m(_u,_p,1)

#define XLPORT_SUBPORT(_u,_p) bcm53400_a0_xlblock_subport_get(_u,_p)

#endif /* __BCM53400_A0_INTERNAL_H__ */
