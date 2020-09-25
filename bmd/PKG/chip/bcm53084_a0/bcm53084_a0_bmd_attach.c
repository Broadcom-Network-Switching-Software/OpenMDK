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
#include <bmd/bmd_phy_ctrl.h>

#include <cdk/chip/bcm53084_a0_defs.h>
#include <cdk/arch/robo_chip.h>
#include <cdk/cdk_device.h>
#include <cdk/cdk_assert.h>

#include "bcm53084_a0_bmd.h"

#if BMD_CONFIG_INCLUDE_PHY == 1

#include <phy/phy_buslist.h>

static phy_bus_t *bcm53084_a0_phy_bus[] = {
#ifdef PHY_BUS_ROBO_MIIM_INT_INSTALLED
    &phy_bus_robo_miim_int,
#endif
    NULL
};

#endif

int 
bcm53084_a0_bmd_attach(int unit)
{
    int port;
    cdk_pbmp_t pbmp;
    
    if(!CDK_DEV_EXISTS(unit)) {
        return CDK_E_UNIT;
    }
    
    CDK_ROBO_BLKTYPE_PBMP_GET(unit, BLKTYPE_GPIC, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        CDK_ASSERT(port < BMD_CONFIG_MAX_PORTS);
        BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_GE;
#if BMD_CONFIG_INCLUDE_PHY == 1
        /* port 5 don't have GPHY */
        if (port != 5) {
            bmd_phy_bus_set(unit, port, bcm53084_a0_phy_bus);
        }
#endif
    }

    port = CPIC_PORT;
    CDK_ASSERT(port < BMD_CONFIG_MAX_PORTS);
    BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_CPU;

    BMD_DEV_FLAGS(unit) |= BMD_DEV_ATTACHED;

    return CDK_E_NONE; 

}

#endif /* CDK_CONFIG_INCLUDE_BCM53084_A0 */
