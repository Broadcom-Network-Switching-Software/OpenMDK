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

#include <cdk/chip/bcm53084_a0_defs.h>

#include "bcm53084_a0_bmd.h"

int 
bcm53084_a0_bmd_port_vlan_set(int unit, int port, int vlan)
{
    int ioerr = 0;
    DEFAULT_1Q_TAGr_t default_tag;
    DEFAULT_1Q_TAG_P5r_t default_tag_p5;
    DEFAULT_1Q_TAG_IMPr_t default_tag_imp;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);
    BMD_CHECK_VLAN(unit, vlan);

    if (port == 5) {
        ioerr += READ_DEFAULT_1Q_TAG_P5r(unit, &default_tag_p5);
        DEFAULT_1Q_TAG_P5r_VIDf_SET(default_tag_p5, vlan);
        ioerr += WRITE_DEFAULT_1Q_TAG_P5r(unit, default_tag_p5);

    } else if (port == 8) {
        ioerr += READ_DEFAULT_1Q_TAG_IMPr(unit, &default_tag_imp);
        DEFAULT_1Q_TAG_IMPr_VIDf_SET(default_tag_imp, vlan);
        ioerr += WRITE_DEFAULT_1Q_TAG_IMPr(unit, default_tag_imp);

    } else {
        ioerr += READ_DEFAULT_1Q_TAGr(unit, port, &default_tag);
        DEFAULT_1Q_TAGr_VIDf_SET(default_tag, vlan);
        ioerr += WRITE_DEFAULT_1Q_TAGr(unit, port, default_tag);
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

#endif /* CDK_CONFIG_INCLUDE_BCM53084_A0 */
