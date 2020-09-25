#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56820_A0 == 1

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

#include <cdk/chip/bcm56820_a0_defs.h>
#include <cdk/arch/xgs_chip.h>
#include <cdk/cdk_assert.h>

#include "bcm56820_a0_internal.h"
#include "bcm56820_a0_bmd.h"

#if BMD_CONFIG_INCLUDE_PHY == 1

#include <phy/phy_buslist.h>

static phy_bus_t *bcm56820_a0_phy_bus[] = {
#ifdef PHY_BUS_BCM56820_MIIM_INT_INSTALLED
    &phy_bus_bcm56820_miim_int,
#endif
#ifdef PHY_BUS_BCM956820K24XG_MIIM_EXT_INSTALLED
    &phy_bus_bcm956820k24xg_miim_ext,
#endif
    NULL
};

#define PHY_BUS_SET(_u,_p,_b) bmd_phy_bus_set(_u,_p,_b)

#else

#define PHY_BUS_SET(_u,_p,_b)

#endif

int
bcm56820_a0_port_speed_max(int unit, int port)
{
    if (BMD_PORT_PROPERTIES(unit, port) & BMD_PORT_GE) {
        return 1000;
    }
    if (port <= 12) {
        if (CDK_XGS_FLAGS(unit) & CHIP_FLAG_TDM3_X) {
            if (port >= 9) {
                return 21000;
            }
            return 16000;
        }
        if (CDK_XGS_FLAGS(unit) & CHIP_FLAG_TDM2_X) {
            if (port <= 4) {
                return 16000;
            }
        }
    } else {
        if (CDK_XGS_FLAGS(unit) & CHIP_FLAG_TDM3_Y) {
            if (port <= 16) {
                return 21000;
            }
            return 16000;
        }
        if (CDK_XGS_FLAGS(unit) & CHIP_FLAG_TDM2_Y) {
            if (port >= 21) {
                return 16000;
            }
        }
    }
    return 10000;
}

int
bcm56820_a0_bmd_attach(int unit)
{
    int port;
    cdk_pbmp_t pbmp;

    if(!CDK_DEV_EXISTS(unit)) {
        return CDK_E_UNIT;
    }

    CDK_XGS_BLKTYPE_PBMP_GET(unit, BLKTYPE_QGPORT, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        CDK_ASSERT(port < BMD_CONFIG_MAX_PORTS);
        BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_GE;
        PHY_BUS_SET(unit, port, bcm56820_a0_phy_bus);
    }

#if BMD_CONFIG_INCLUDE_HIGIG == 1 || BMD_CONFIG_INCLUDE_XE == 1
    CDK_XGS_BLKTYPE_PBMP_GET(unit, BLKTYPE_GXPORT, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        CDK_ASSERT(port < BMD_CONFIG_MAX_PORTS);
        BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_XE;
        PHY_BUS_SET(unit, port, bcm56820_a0_phy_bus);
    }
#endif

    port = CMIC_PORT;
    CDK_ASSERT(port < BMD_CONFIG_MAX_PORTS);
    BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_CPU;

    BMD_DEV_FLAGS(unit) |= BMD_DEV_ATTACHED;

    return 0; 
}
#endif /* CDK_CONFIG_INCLUDE_BCM56820_A0 */
