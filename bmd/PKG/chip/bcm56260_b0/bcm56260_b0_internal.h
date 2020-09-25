/*
 *
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef __BCM56260_B0_INTERNAL_H__
#define __BCM56260_B0_INTERNAL_H__

#include "../bcm56260_a0/bcm56260_a0_internal.h"

extern int
bcm56260_b0_xlport_init(int unit, int port);

extern int
bcm56260_b0_soc_port_enable_set(int unit, int port, int enable);

extern int
bcm56260_b0_mac_xl_drain_cells(int unit, int port);

#endif /* __BCM56260_B0_INTERNAL_H__ */
