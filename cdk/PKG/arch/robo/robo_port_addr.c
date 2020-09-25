/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Common ROBO chip functions.
 */

#include <cdk/arch/robo_chip.h>

/*
 * Calculate address of port-based register
 */
uint32_t
cdk_robo_port_addr(int unit, int port, int size, uint32_t offset)
{
    /* The translation can be specified per-chip if necessary */
    if (CDK_ROBO_INFO(unit)->port_addr) {
        return CDK_ROBO_INFO(unit)->port_addr(port, size, offset); 
    }

    /* Default address calculation */
    if (port < 0) {
        port = 0;
    }
    port -= (offset >> 16);
    return (offset & 0xffff) + (port * size); 
}

