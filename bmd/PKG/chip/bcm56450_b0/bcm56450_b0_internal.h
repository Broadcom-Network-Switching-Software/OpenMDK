/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef __BCM56450_B0_INTERNAL_H__
#define __BCM56450_B0_INTERNAL_H__

#include <cdk/chip/bcm56450_b0_defs.h>

#define NUM_MXQBLOCKS                  11
#define MXQPORTS_PER_BLOCK              4

#define XMAC_MODE_SPEED_10              0x0
#define XMAC_MODE_SPEED_100             0x1
#define XMAC_MODE_SPEED_1000            0x2
#define XMAC_MODE_SPEED_2500            0x3
#define XMAC_MODE_SPEED_10000           0x4

#define PHY_CONN_WARPCORE               0
#define PHY_CONN_UNICORE                1

#define NUM_PHYS_PORTS          43
#define CMIC_LPORT              0

#define GS_PORT                 42
#define GS_MPORT                40
#define GS_LPORT                40

extern int 
bcm56450_b0_mxqport_pbmp_get(int unit, cdk_pbmp_t *pbmp);

extern int
bcm56450_b0_p2l(int unit, int port, int inverse);

#define P2PP(_u,_p) bcm56450_b0_p2l(_u, _p, 0)
#define PP2P(_u,_p) bcm56450_b0_p2l(_u, _p, 1)
#define P2M(_u,_p) bcm56450_b0_p2l(_u, _p, 0)
#define M2P(_u,_p) bcm56450_b0_p2l(_u, _p, 1)
#define P2L(_u,_p) bcm56450_b0_p2l(_u, _p, 0)
#define L2P(_u,_p) bcm56450_b0_p2l(_u, _p, 1)


#endif /* __BCM56450_B0_INTERNAL_H__ */
