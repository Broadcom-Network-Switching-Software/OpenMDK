#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53262_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#include <cdk/chip/bcm53262_a0_defs.h>

#include "bcm53262_a0_bmd.h"

int
bcm53262_a0_bmd_port_vlan_set(int unit, int port, int vlan)
{
    int ioerr = 0;
    DEFAULT_1Q_TAGr_t default_tag;
    uint32_t	reg_value;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);

    ioerr += READ_DEFAULT_1Q_TAGr(unit, port, &default_tag);
    reg_value = DEFAULT_1Q_TAGr_DEF_TAGf_GET(default_tag);
    reg_value = (vlan | (reg_value & 0xf000));
    DEFAULT_1Q_TAGr_DEF_TAGf_SET(default_tag, reg_value);
    ioerr += WRITE_DEFAULT_1Q_TAGr(unit, port, default_tag);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}
#endif /* CDK_CONFIG_INCLUDE_BCM53262_A0 */
