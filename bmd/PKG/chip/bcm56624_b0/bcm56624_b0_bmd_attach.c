#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56624_B0 == 1

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

#include <cdk/chip/bcm56624_b0_defs.h>
#include <cdk/arch/xgs_chip.h>
#include <cdk/cdk_assert.h>

#include "bcm56624_b0_bmd.h"

#if BMD_CONFIG_INCLUDE_PHY == 1

#include <phy/phy_buslist.h>

static phy_bus_t *bcm56620_phy_bus[] = {
#ifdef PHY_BUS_BCM56624_MIIM_INT_INSTALLED
    &phy_bus_bcm56624_miim_int,
#endif
#ifdef PHY_BUS_BCM956620K24S_MIIM_EXT_INSTALLED
    &phy_bus_bcm956620k24s_miim_ext,
#endif
    NULL
};

static phy_bus_t *bcm56624_phy_bus[] = {
#ifdef PHY_BUS_BCM56624_MIIM_INT_INSTALLED
    &phy_bus_bcm56624_miim_int,
#endif
#ifdef PHY_BUS_BCM956624K49S_MIIM_EXT_INSTALLED
    &phy_bus_bcm956624k49s_miim_ext,
#endif
    NULL
};

static phy_bus_t *bcm56626_phy_bus[] = {
#ifdef PHY_BUS_BCM56624_MIIM_INT_INSTALLED
    &phy_bus_bcm56624_miim_int,
#endif
#ifdef PHY_BUS_BCM956626K25S_MIIM_EXT_INSTALLED
    &phy_bus_bcm956626k25s_miim_ext,
#endif
    NULL
};

static phy_bus_t *bcm56628_phy_bus[] = {
#ifdef PHY_BUS_BCM56624_MIIM_INT_INSTALLED
    &phy_bus_bcm56624_miim_int,
#endif
#ifdef PHY_BUS_BCM956628K8XS_MIIM_EXT_INSTALLED
    &phy_bus_bcm956628k8xs_miim_ext,
#endif
    NULL
};

static phy_bus_t *bcm56629_phy_bus[] = {
#ifdef PHY_BUS_BCM56624_MIIM_INT_INSTALLED
    &phy_bus_bcm56624_miim_int,
#endif
#ifdef PHY_BUS_BCM956629K24S_MIIM_EXT_INSTALLED
    &phy_bus_bcm956629k24s_miim_ext,
#endif
    NULL
};

#define PHY_BUS_SET(_u,_p,_b) bmd_phy_bus_set(_u,_p,_b)

#else

#define PHY_BUS_SET(_u,_p,_b)

#endif

int
bcm56624_b0_bmd_attach(int unit)
{
    int port;
    cdk_pbmp_t pbmp;
#if BMD_CONFIG_INCLUDE_PHY == 1
    phy_bus_t **phy_bus = bcm56624_phy_bus;
#endif

    if(!CDK_DEV_EXISTS(unit)) {
        return CDK_E_UNIT;
    }

#if BMD_CONFIG_INCLUDE_PHY == 1
    if (CDK_XGS_FLAGS(unit) & CHIP_FLAG_NO_ESM) {
        phy_bus = bcm56620_phy_bus;
    }
    if (CDK_XGS_FLAGS(unit) & CHIP_FLAG_XG23_16G) {
        phy_bus = bcm56626_phy_bus;
        if (CDK_XGS_FLAGS(unit) & CHIP_FLAG_XG01_16G) {
            phy_bus = bcm56628_phy_bus;
            if (CDK_XGS_FLAGS(unit) & CHIP_FLAG_XG_MIXED) {
                phy_bus = bcm56629_phy_bus;
            }
        }
    }
#endif

    CDK_XGS_BLKTYPE_PBMP_GET(unit, BLKTYPE_SPORT, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        CDK_ASSERT(port < BMD_CONFIG_MAX_PORTS);
        BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_GE;
        PHY_BUS_SET(unit, port, phy_bus);
    }

    CDK_XGS_BLKTYPE_PBMP_GET(unit, BLKTYPE_XGPORT, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        CDK_ASSERT(port < BMD_CONFIG_MAX_PORTS);
        BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_GE;
        PHY_BUS_SET(unit, port, phy_bus);
    }

#if BMD_CONFIG_INCLUDE_HIGIG == 1 || BMD_CONFIG_INCLUDE_XE == 1
    CDK_XGS_BLKTYPE_PBMP_GET(unit, BLKTYPE_GXPORT, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        CDK_ASSERT(port < BMD_CONFIG_MAX_PORTS);
#if BMD_CONFIG_INCLUDE_HIGIG == 1
        BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_HG;
#else
        BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_XE;
#endif
        PHY_BUS_SET(unit, port, phy_bus);
    }
#endif

#if BMD_CONFIG_INCLUDE_HIGIG == 1 || BMD_CONFIG_INCLUDE_XE == 1
    if (CDK_XGS_FLAGS(unit) & CHIP_FLAG_XG01_16G) {
        BMD_PORT_PROPERTIES(unit, 2) = BMD_PORT_XE;
        BMD_PORT_PROPERTIES(unit, 14) = BMD_PORT_XE;
        PHY_BUS_SET(unit, 2, phy_bus);
        PHY_BUS_SET(unit, 14, phy_bus);
    }
    if (CDK_XGS_FLAGS(unit) & CHIP_FLAG_XG23_16G) {
        BMD_PORT_PROPERTIES(unit, 26) = BMD_PORT_XE;
        BMD_PORT_PROPERTIES(unit, 27) = BMD_PORT_XE;
        PHY_BUS_SET(unit, 26, phy_bus);
        PHY_BUS_SET(unit, 27, phy_bus);
    }
#endif

    port = CMIC_PORT;
    CDK_ASSERT(port < BMD_CONFIG_MAX_PORTS);
    BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_CPU;

    BMD_DEV_FLAGS(unit) |= BMD_DEV_ATTACHED;

    return 0; 
}
#endif /* CDK_CONFIG_INCLUDE_BCM56624_B0 */
