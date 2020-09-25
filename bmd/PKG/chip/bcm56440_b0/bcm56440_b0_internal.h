/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef __BCM56440_B0_INTERNAL_H__
#define __BCM56440_B0_INTERNAL_H__

#include <cdk/chip/bcm56440_b0_defs.h>

#define COMMAND_CONFIG_SPEED_10         0x0
#define COMMAND_CONFIG_SPEED_100        0x1
#define COMMAND_CONFIG_SPEED_1000       0x2
#define COMMAND_CONFIG_SPEED_2500       0x3
#define COMMAND_CONFIG_SPEED_10000      0x4

#define NUM_PHYS_PORTS          34

#define CMIC_LPORT              0
#define CMIC_MPORT              0

#define LB_LPORT                35

extern int
bcm56440_b0_xport_reset(int unit, int port);

extern int
bcm56440_b0_xport_init(int unit, int port);

extern int
bcm56440_b0_xlport_pbmp_get(int unit, cdk_pbmp_t *pbmp);

extern uint32_t
bcm56440_b0_port_speed_max(int unit, int port);

extern int
bcm56440_b0_warpcore_phy_init(int unit, int port);


extern int
bcm56440_b0_p2l(int unit, int port, int inverse);

extern int
bcm56440_b0_p2m(int unit, int port, int inverse);

#define P2L(_u,_p) bcm56440_b0_p2l(_u,_p,0)
#define L2P(_u,_p) bcm56440_b0_p2l(_u,_p,1)

#define P2M(_u,_p) bcm56440_b0_p2m(_u,_p,0)
#define M2P(_u,_p) bcm56440_b0_p2m(_u,_p,1)

#define XLPORT_BLKIDX(_p) ((_p - 1) >> 2)
#define XLPORT_SUBPORT(_p) ((_p - 1) & 0x3)


#endif /* __BCM56440_B0_INTERNAL_H__ */
