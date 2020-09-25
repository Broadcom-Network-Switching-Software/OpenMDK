#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56112_A0 == 1

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

#include <cdk/chip/bcm56112_a0_defs.h>
#include <cdk/arch/xgs_chip.h>
#include <cdk/cdk_assert.h>

#include "bcm56112_a0_bmd.h"

#if BMD_CONFIG_INCLUDE_PHY == 1

#include <phy/phy_buslist.h>

static phy_bus_t *bcm56112_a0_fport_phy_bus[] = {
#ifdef PHY_BUS_XGS_MIIM_EXT_INSTALLED
    &phy_bus_xgs_miim_ext,
#endif
    NULL
};

static phy_bus_t *bcm56112_a0_gport_phy_bus[] = {
#ifdef PHY_BUS_XGS_MIIM_INT_INSTALLED
    &phy_bus_xgs_miim_int,
#endif
#ifdef PHY_BUS_XGS_MIIM_EXT_INSTALLED
    &phy_bus_xgs_miim_ext,
#endif
    NULL
};

#if BMD_CONFIG_INCLUDE_HIGIG == 1 || BMD_CONFIG_INCLUDE_XE == 1
static phy_bus_t *bcm56112_a0_xport_phy_bus[] = {
#ifdef PHY_BUS_XGS_MIIM_XINT_INSTALLED
    &phy_bus_xgs_miim_xint,
#endif
#ifdef PHY_BUS_BCM56504_MIIM_EXT1_C45_INSTALLED
    &phy_bus_bcm56504_miim_ext1_c45,
#endif
    NULL
};
#endif

#endif

int
bcm56112_a0_bmd_attach(int unit)
{
    int port;
    cdk_pbmp_t pbmp;

    if(!CDK_DEV_EXISTS(unit)) {
        return CDK_E_UNIT;
    }

    CDK_XGS_BLKTYPE_PBMP_GET(unit, BLKTYPE_GPORT, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        CDK_ASSERT(port < BMD_CONFIG_MAX_PORTS);
        BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_FE;
#if BMD_CONFIG_INCLUDE_PHY == 1
        bmd_phy_bus_set(unit, port, bcm56112_a0_fport_phy_bus);
#endif
        if (port >= 24) {
            BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_GE;
#if BMD_CONFIG_INCLUDE_PHY == 1
            bmd_phy_bus_set(unit, port, bcm56112_a0_gport_phy_bus);
#endif
        }
    }

#if BMD_CONFIG_INCLUDE_HIGIG == 1 || BMD_CONFIG_INCLUDE_XE == 1
    CDK_XGS_BLKTYPE_PBMP_GET(unit, BLKTYPE_XPORT, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        CDK_ASSERT(port < BMD_CONFIG_MAX_PORTS);
        BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_XE;
#if BMD_CONFIG_INCLUDE_PHY == 1
        bmd_phy_bus_set(unit, port, bcm56112_a0_xport_phy_bus);
#endif
    }
#endif

    port = CMIC_PORT;
    CDK_ASSERT(port < BMD_CONFIG_MAX_PORTS);
    BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_CPU;

    BMD_DEV_FLAGS(unit) |= BMD_DEV_ATTACHED;

    return 0; 
}

#endif /* CDK_CONFIG_INCLUDE_BCM56112_A0 */
