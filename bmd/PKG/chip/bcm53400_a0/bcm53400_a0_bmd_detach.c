/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53400_A0 == 1

#include <bmd/bmd.h>
#include <bmd/bmd_phy.h>
#include <cdk/cdk_device.h>
#include <cdk/cdk_error.h>
#include "bcm53400_a0_bmd.h"

int 
bcm53400_a0_bmd_detach(int unit)
{
    int port;

    BMD_CHECK_UNIT(unit);

    for (port = 0; port < BMD_CONFIG_MAX_PORTS; port++) {
        bmd_phy_detach(unit, port);
    }
    BMD_DEV_FLAGS(unit) &= ~BMD_DEV_ATTACHED;

    return CDK_E_NONE; 
}

#endif /* CDK_CONFIG_INCLUDE_BCM53400_A0 */
