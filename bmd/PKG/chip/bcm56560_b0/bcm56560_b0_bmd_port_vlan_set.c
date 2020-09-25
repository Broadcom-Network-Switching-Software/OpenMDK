/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56560_B0 == 1

#include <bmd/bmd.h>

#include <cdk/chip/bcm56560_b0_defs.h>

#include "bcm56560_b0_bmd.h"
#include "bcm56560_b0_internal.h"

int 
bcm56560_b0_bmd_port_vlan_set(int unit, int port, int vlan)
{
    int ioerr = 0;
    int lport;
    PORT_TABm_t port_tab;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_VLAN(unit, vlan);
    BMD_CHECK_PORT(unit, port);

    lport = P2L(unit, port);

    ioerr += READ_PORT_TABm(unit, lport, &port_tab);
    PORT_TABm_PORT_VIDf_SET(port_tab, vlan);
    ioerr += WRITE_PORT_TABm(unit, lport, port_tab);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}
#endif /* CDK_CONFIG_INCLUDE_BCM56560_B0 */

