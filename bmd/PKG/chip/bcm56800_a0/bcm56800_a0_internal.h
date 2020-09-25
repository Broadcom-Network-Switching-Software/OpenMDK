/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef __BCM56800_A0_INTERNAL_H__
#define __BCM56800_A0_INTERNAL_H__

extern int
bcm56800_a0_port_speed_max(int unit, int port);

extern int
bcm56800_a0_port_ethernet(int unit, int port);

extern int
bcm56800_a0_gxport_reset(int unit, int port);

extern int
bcm56800_a0_gxport_init(int unit, int port);

#endif /* __BCM56800_A0_INTERNAL_H__ */
