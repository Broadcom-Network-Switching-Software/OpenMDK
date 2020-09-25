#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53118_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#include <cdk/cdk_device.h>

#include "bcm53118_a0_internal.h"
#include "bcm53118_a0_bmd.h"

int
bcm53118_a0_bmd_port_mac_addr_remove(int unit, int port, int vlan, const bmd_mac_addr_t *mac_addr)
{
    BMD_CHECK_UNIT(unit);
    BMD_CHECK_VLAN(unit, vlan);
    BMD_CHECK_PORT(unit, port);

    return bcm53118_a0_arl_write(unit, -1, vlan, mac_addr);
}

#endif /* CDK_CONFIG_INCLUDE_BCM53118_A0 */
