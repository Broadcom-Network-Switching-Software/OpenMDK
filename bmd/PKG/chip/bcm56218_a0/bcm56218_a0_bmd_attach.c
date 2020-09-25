#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56218_A0 == 1

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

#include <cdk/chip/bcm56218_a0_defs.h>
#include <cdk/arch/xgs_chip.h>
#include <cdk/cdk_assert.h>

#include "bcm56218_a0_bmd.h"

#if BMD_CONFIG_INCLUDE_PHY == 1

#include <phy/phy_buslist.h>

static phy_bus_t *bcm56218_a0_phy_bus[] = {
#ifdef PHY_BUS_BCM56218_MIIM_INT_INSTALLED
    &phy_bus_bcm56218_miim_int,
#endif
#ifdef PHY_BUS_BCM56218_MIIM_EXT_INSTALLED
    &phy_bus_bcm56218_miim_ext,
#endif
    NULL
};

static phy_bus_t *bcm56214_a0_phy_bus[] = {
#ifdef PHY_BUS_BCM56218_MIIM_INT_INSTALLED
    &phy_bus_bcm56218_miim_int,
#endif
#ifdef PHY_BUS_BCM956214R26T_MIIM_EXT_INSTALLED
    &phy_bus_bcm956214r26t_miim_ext,
#endif
    NULL
};

#endif

int
bcm56218_a0_bmd_attach(int unit)
{
    int port;
    cdk_pbmp_t pbmp;

    if(!CDK_DEV_EXISTS(unit)) {
        return CDK_E_UNIT;
    }

    CDK_XGS_BLKTYPE_PBMP_GET(unit, BLKTYPE_GPORT, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        CDK_ASSERT(port < BMD_CONFIG_MAX_PORTS);
        BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_GE;
        if (port > 5 && (CDK_XGS_FLAGS(unit) & CHIP_FLAG_FE_ONLY)) {
            BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_FE;
        }
#if BMD_CONFIG_INCLUDE_PHY == 1
        bmd_phy_bus_set(unit, port, bcm56218_a0_phy_bus);
        if (!CDK_PBMP_MEMBER(pbmp, 30)) {
            bmd_phy_bus_set(unit, port, bcm56214_a0_phy_bus);
        }
#endif
    }

    port = CMIC_PORT;
    CDK_ASSERT(port < BMD_CONFIG_MAX_PORTS);
    BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_CPU;

    BMD_DEV_FLAGS(unit) |= BMD_DEV_ATTACHED;

    return 0; 
}

#endif /* CDK_CONFIG_INCLUDE_BCM56218_A0 */
