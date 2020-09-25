/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef __BCM56150_A0_INTERNAL_H__
#define __BCM56150_A0_INTERNAL_H__

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

#define NUM_PHYS_PORTS                  34

extern int
bcm56150_a0_xlport_reset(int unit, int port);

extern int
bcm56150_a0_xlport_init(int unit, int port);

extern int
bcm56150_a0_p2l(int unit, int port, int inverse);

extern int
bcm56150_a0_p2m(int unit, int port, int inverse);

extern int
bcm56150_a0_xlport_pbmp_get(int unit, cdk_pbmp_t *pbmp);

extern uint32_t
bcm56150_a0_port_speed_max(int unit, int port);

extern int
bcm56150_a0_port_num_lanes(int unit, int port);

#define P2L(_u,_p) bcm56150_a0_p2l(_u,_p,0)
#define L2P(_u,_p) bcm56150_a0_p2l(_u,_p,1)

#define P2M(_u,_p) bcm56150_a0_p2m(_u,_p,0)
#define M2P(_u,_p) bcm56150_a0_p2m(_u,_p,1)

#define XLPORT_SUBPORT(_p) ((_p - 2) & 0x3)

#endif /* __BCM56150_A0_INTERNAL_H__ */
