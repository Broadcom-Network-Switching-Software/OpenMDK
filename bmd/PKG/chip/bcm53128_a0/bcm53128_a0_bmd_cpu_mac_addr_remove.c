#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53128_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#include <cdk/chip/bcm53128_a0_defs.h>

#include "bcm53128_a0_bmd.h"

int
bcm53128_a0_bmd_cpu_mac_addr_remove(int unit, int vlan, const bmd_mac_addr_t *mac_addr)
{
    return bcm53128_a0_bmd_port_mac_addr_remove(unit, CPIC_PORT, vlan, mac_addr);
}
#endif /* CDK_CONFIG_INCLUDE_BCM53128_A0 */
