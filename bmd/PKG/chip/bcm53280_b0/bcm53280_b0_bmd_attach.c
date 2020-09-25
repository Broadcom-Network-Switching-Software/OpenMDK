#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53280_B0 == 1
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

#include <cdk/chip/bcm53280_b0_defs.h>
#include <cdk/arch/robo_chip.h>
#include <cdk/cdk_assert.h>

#include "bcm53280_b0_bmd.h"
#include "bcm53280_b0_internal.h"
#include <cdk/cdk_debug.h>

#if BMD_CONFIG_INCLUDE_PHY == 1

#include <phy/phy_buslist.h>

static phy_bus_t *bcm53280_b0_fport_phy_bus[] = {
#ifdef PHY_BUS_BCM53280_MIIM_INT_INSTALLED
    &phy_bus_bcm53280_miim_int,
#endif        
    NULL
};

static phy_bus_t *bcm53280_b0_gport_phy_bus[] = {
#ifdef PHY_BUS_BCM53280_MIIM_INT_INSTALLED
    &phy_bus_bcm53280_miim_int,
#endif        
#ifdef PHY_BUS_BCM53280_MIIM_EXT_INSTALLED
    &phy_bus_bcm53280_miim_ext,
#endif
    NULL
};

#endif

int 
bcm53280_b0_bmd_attach(
    int unit)
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
        bmd_phy_bus_set(unit, port, bcm53280_b0_fport_phy_bus);
#endif
    }

    CDK_ROBO_BLKTYPE_PBMP_GET(unit, BLKTYPE_GPIC, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        CDK_ASSERT(port < BMD_CONFIG_MAX_PORTS);
        BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_GE;
#if BMD_CONFIG_INCLUDE_PHY == 1
        bmd_phy_bus_set(unit, port, bcm53280_b0_gport_phy_bus);
#endif
    }

    port = CPIC_PORT;
    CDK_ASSERT(port < BMD_CONFIG_MAX_PORTS);
    BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_CPU;

    BMD_DEV_FLAGS(unit) |= BMD_DEV_ATTACHED;

    return 0; 
}

#endif /* CDK_CONFIG_INCLUDE_BCM53280_B0 */
