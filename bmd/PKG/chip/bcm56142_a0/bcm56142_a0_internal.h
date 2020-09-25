/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef __BCM56142_A0_INTERNAL_H__
#define __BCM56142_A0_INTERNAL_H__

#define COMMAND_CONFIG_SPEED_10         0x0
#define COMMAND_CONFIG_SPEED_100        0x1
#define COMMAND_CONFIG_SPEED_1000       0x2
#define COMMAND_CONFIG_SPEED_2500       0x3

#define XPORT_FLAG_GPORT                (1 << BLKTYPE_GPORT)
#define XPORT_FLAG_XQPORT               (1 << BLKTYPE_XQPORT)
#define XPORT_FLAG_ALL                  (XPORT_FLAG_GPORT | XPORT_FLAG_XQPORT)

extern int
bcm56142_a0_xport_reset(int unit, int port);

extern int
bcm56142_a0_xport_init(int unit, int port);

extern int
bcm56142_a0_xport_pbmp_get(int unit, int flag, cdk_pbmp_t *pbmp);

#endif /* __BCM56142_A0_INTERNAL_H__ */
