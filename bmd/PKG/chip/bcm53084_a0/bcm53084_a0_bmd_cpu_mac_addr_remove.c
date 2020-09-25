#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53084_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#include <cdk/cdk_device.h>
#include <cdk/chip/bcm53084_a0_defs.h>

#include "bcm53084_a0_bmd.h"

int 
bcm53084_a0_bmd_cpu_mac_addr_remove(
    int unit, int vlan, const bmd_mac_addr_t *mac_addr)
{
    int port;

    /* External CPU is port 8 if the interface is secured USB access */
    if (CDK_CHIP_CONFIG(unit) & DCFG_MBUS_SEC_USB) {
        port = CPIC_PORT;
    } else {
        /* External CPU is port 5 */
        port = 5;
    }

    return bcm53084_a0_bmd_port_mac_addr_remove(unit, port, vlan, mac_addr);
}

#endif /* CDK_CONFIG_INCLUDE_BCM53084_A0 */
