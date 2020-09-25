#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53101_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>
#include <bmd/bmd_device.h>
#include <bmd/bmd_phy_ctrl.h>

#include <cdk/chip/bcm53101_a0_defs.h>
#include <cdk/arch/robo_chip.h>
#include <cdk/cdk_assert.h>

#include "bcm53101_a0_bmd.h"
#include "bcm53101_a0_internal.h"

#if BMD_CONFIG_INCLUDE_PHY == 1

#include <phy/phy_buslist.h>

static phy_bus_t *bcm53101_a0_phy_bus[] = {
#ifdef PHY_BUS_ROBO_MIIM_INT_INSTALLED
    &phy_bus_robo_miim_int,
#endif
    NULL
};

static phy_bus_t *bcm53101_a0_ext_phy_bus[] = {
#ifdef PHY_BUS_BCM53101_MIIM_EXT_INSTALLED
    &phy_bus_bcm53101_miim_ext,
#endif
    NULL
};

#endif
int
bcm53101_a0_bmd_attach(int unit)
{
    int port;
    cdk_pbmp_t pbmp;

    if(!CDK_DEV_EXISTS(unit)) {
        return CDK_E_UNIT;
    }

    CDK_ROBO_BLKTYPE_PBMP_GET(unit, BLKTYPE_EPIC, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        CDK_ASSERT(port < BMD_CONFIG_MAX_PORTS);
        BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_FE;
#if BMD_CONFIG_INCLUDE_PHY == 1
        if (port == 5) {
            bmd_phy_bus_set(unit, port, bcm53101_a0_ext_phy_bus);
        } else {
            bmd_phy_bus_set(unit, port, bcm53101_a0_phy_bus);
        }
#endif
    }

    port = CPIC_PORT;
    CDK_ASSERT(port < BMD_CONFIG_MAX_PORTS);
    BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_CPU;

    BMD_DEV_FLAGS(unit) |= BMD_DEV_ATTACHED;

    return 0; 

}

#endif /* CDK_CONFIG_INCLUDE_BCM53101_A0 */
