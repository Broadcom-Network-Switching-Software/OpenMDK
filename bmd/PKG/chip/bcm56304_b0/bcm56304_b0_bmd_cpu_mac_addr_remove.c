#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56304_B0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#include <cdk/chip/bcm56304_b0_defs.h>

#include "bcm56304_b0_bmd.h"

int
bcm56304_b0_bmd_cpu_mac_addr_remove(int unit, int vlan, const bmd_mac_addr_t *mac_addr)
{
    return bcm56304_b0_bmd_port_mac_addr_remove(unit, CMIC_PORT, vlan, mac_addr);
}
#endif /* CDK_CONFIG_INCLUDE_BCM56304_B0 */
