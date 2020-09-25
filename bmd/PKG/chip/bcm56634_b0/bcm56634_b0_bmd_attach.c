#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56634_B0 == 1

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

#include <cdk/chip/bcm56634_b0_defs.h>
#include <cdk/arch/xgs_chip.h>
#include <cdk/cdk_assert.h>

#include "bcm56634_b0_bmd.h"

#if BMD_CONFIG_INCLUDE_PHY == 1

#include <phy/phy_buslist.h>

static phy_bus_t *bcm56526_phy_bus[] = {
#ifdef PHY_BUS_BCM56634_MIIM_INT_INSTALLED
    &phy_bus_bcm56634_miim_int,
#endif
#ifdef PHY_BUS_BCM956526K29S_MIIM_EXT_INSTALLED
    &phy_bus_bcm956526k29s_miim_ext,
#endif
    NULL
};

static phy_bus_t *bcm56521_phy_bus[] = {
#ifdef PHY_BUS_BCM56634_MIIM_INT_INSTALLED
    &phy_bus_bcm56634_miim_int,
#endif
#ifdef PHY_BUS_BCM956521K_MIIM_EXT_INSTALLED
    &phy_bus_bcm956521k_miim_ext,
#endif
    NULL
};

static phy_bus_t *bcm56630_phy_bus[] = {
#ifdef PHY_BUS_BCM56634_MIIM_INT_INSTALLED
    &phy_bus_bcm56634_miim_int,
#endif
#ifdef PHY_BUS_BCM956685K24TS_MIIM_EXT_INSTALLED
    &phy_bus_bcm956685k24ts_miim_ext,
#endif
    NULL
};

static phy_bus_t *bcm56634_phy_bus[] = {
#ifdef PHY_BUS_BCM56634_MIIM_INT_INSTALLED
    &phy_bus_bcm56634_miim_int,
#endif
#ifdef PHY_BUS_BCM956634K49S_MIIM_EXT_INSTALLED
    &phy_bus_bcm956634k49s_miim_ext,
#endif
    NULL
};

static phy_bus_t *bcm56636_phy_bus[] = {
#ifdef PHY_BUS_BCM56634_MIIM_INT_INSTALLED
    &phy_bus_bcm56634_miim_int,
#endif
#ifdef PHY_BUS_BCM956636K25S_MIIM_EXT_INSTALLED
    &phy_bus_bcm956636k25s_miim_ext,
#endif
    NULL
};

static phy_bus_t *bcm56638_phy_bus[] = {
#ifdef PHY_BUS_BCM56634_MIIM_INT_INSTALLED
    &phy_bus_bcm56634_miim_int,
#endif
#ifdef PHY_BUS_BCM956638K8XS_MIIM_EXT_INSTALLED
    &phy_bus_bcm956638k8xs_miim_ext,
#endif
    NULL
};

static phy_bus_t *bcm56639_phy_bus[] = {
#ifdef PHY_BUS_BCM56634_MIIM_INT_INSTALLED
    &phy_bus_bcm56634_miim_int,
#endif
#ifdef PHY_BUS_BCM956639K25S_MIIM_EXT_INSTALLED
    &phy_bus_bcm956639k25s_miim_ext,
#endif
    NULL
};

#define PHY_BUS_SET(_u,_p,_b) bmd_phy_bus_set(_u,_p,_b)

#else

#define PHY_BUS_SET(_u,_p,_b)

#endif

int
bcm56634_b0_bmd_attach(int unit)
{
    int port;
    uint32_t port_type_xe = 0;
    cdk_pbmp_t pbmp, pbmp_ge;
#if BMD_CONFIG_INCLUDE_PHY == 1
    phy_bus_t **phy_bus = bcm56634_phy_bus;
#endif

    if(!CDK_DEV_EXISTS(unit)) {
        return CDK_E_UNIT;
    }

    CDK_XGS_BLKTYPE_PBMP_GET(unit, BLKTYPE_SPORT, &pbmp_ge);
    CDK_XGS_BLKTYPE_PBMP_GET(unit, BLKTYPE_GPORT, &pbmp);
    CDK_PBMP_OR(pbmp_ge, pbmp);
    CDK_XGS_BLKTYPE_PBMP_GET(unit, BLKTYPE_XQPORT, &pbmp);
    CDK_PBMP_OR(pbmp_ge, pbmp);

#if BMD_CONFIG_INCLUDE_PHY == 1
    CDK_XGS_BLKTYPE_PBMP_GET(unit, BLKTYPE_GPORT, &pbmp);
    if (CDK_XGS_FLAGS(unit) & CHIP_FLAG_XQ0_XE) {
        phy_bus = bcm56639_phy_bus;
        if (CDK_PBMP_IS_NULL(pbmp)) {
            phy_bus = bcm56638_phy_bus;
        }
    } else if (CDK_XGS_FLAGS(unit) & CHIP_FLAG_XQ4_XE) {
        phy_bus = bcm56526_phy_bus;
    } else if (CDK_XGS_FLAGS(unit) & CHIP_FLAG_XQ3_XE) {
        phy_bus = bcm56636_phy_bus;
    } else if (CDK_XGS_FLAGS(unit) & CHIP_FLAG_NO_ESM) {
        if (CDK_PBMP_MEMBER(pbmp_ge, 2)) {
            phy_bus = bcm56521_phy_bus;
        } else {
            phy_bus = bcm56630_phy_bus;
        }
    }
#endif

    CDK_PBMP_ITER(pbmp_ge, port) {
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
    port_type_xe = BMD_PORT_XE;
#endif

    if (CDK_XGS_FLAGS(unit) & CHIP_FLAG_XQ0_XE) {
        BMD_PORT_PROPERTIES(unit, 30) = port_type_xe;
    }
    if (CDK_XGS_FLAGS(unit) & CHIP_FLAG_XQ2_XE) {
        BMD_PORT_PROPERTIES(unit, 38) = port_type_xe;
    }
    if (CDK_XGS_FLAGS(unit) & CHIP_FLAG_XQ3_XE) {
        BMD_PORT_PROPERTIES(unit, 42) = port_type_xe;
    }
    if (CDK_XGS_FLAGS(unit) & CHIP_FLAG_XQ4_XE) {
        BMD_PORT_PROPERTIES(unit, 46) = port_type_xe;
    }
    if (CDK_XGS_FLAGS(unit) & CHIP_FLAG_XQ5_XE) {
        BMD_PORT_PROPERTIES(unit, 50) = port_type_xe;
    }

    port = CMIC_PORT;
    CDK_ASSERT(port < BMD_CONFIG_MAX_PORTS);
    BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_CPU;

    BMD_DEV_FLAGS(unit) |= BMD_DEV_ATTACHED;

    return 0; 
}
#endif /* CDK_CONFIG_INCLUDE_BCM56634_B0 */
