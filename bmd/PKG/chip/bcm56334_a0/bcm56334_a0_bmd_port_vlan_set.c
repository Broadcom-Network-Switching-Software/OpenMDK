#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56334_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#include <cdk/chip/bcm56334_a0_defs.h>

#include "bcm56334_a0_bmd.h"

int
bcm56334_a0_bmd_port_vlan_set(int unit, int port, int vlan)
{
    int ioerr = 0;
    PORT_TABm_t port_tab;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_VLAN(unit, vlan);
    BMD_CHECK_PORT(unit, port);

    ioerr += READ_PORT_TABm(unit, port, &port_tab);
    PORT_TABm_PORT_VIDf_SET(port_tab, vlan);
    ioerr += WRITE_PORT_TABm(unit, port, port_tab);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

#endif /* CDK_CONFIG_INCLUDE_BCM56334_A0 */
