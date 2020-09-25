#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM89500_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#include <cdk/chip/bcm89500_a0_defs.h>

#include "bcm89500_a0_bmd.h"

int
bcm89500_a0_bmd_port_vlan_get(int unit, int port, int *vlan)
{
    int ioerr = 0;
    DEFAULT_1Q_TAGr_t default_tag;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);

    ioerr += READ_DEFAULT_1Q_TAGr(unit, port, &default_tag);
    *vlan = DEFAULT_1Q_TAGr_VIDf_GET(default_tag);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}
#endif /* CDK_CONFIG_INCLUDE_BCM89500_A0 */
