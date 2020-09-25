#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56800_A0 == 1

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

#include <cdk/chip/bcm56800_a0_defs.h>
#include <cdk/arch/xgs_chip.h>
#include <cdk/cdk_assert.h>

#include "bcm56800_a0_internal.h"
#include "bcm56800_a0_bmd.h"

#if BMD_CONFIG_INCLUDE_PHY == 1

#include <phy/phy_buslist.h>

static phy_bus_t *bcm56800_a0_gxport_phy_bus[] = {
#ifdef PHY_BUS_BCM56800_MIIM_INT_INSTALLED
    &phy_bus_bcm56800_miim_int,
#endif
#ifdef PHY_BUS_XGS_MIIM_EXT_INSTALLED
    &phy_bus_xgs_miim_ext,
#endif
    NULL
};

static phy_bus_t *bcm56580_a0_gxport_phy_bus[] = {
#ifdef PHY_BUS_BCM56580_MIIM_INT_INSTALLED
    &phy_bus_bcm56580_miim_int,
#endif
#ifdef PHY_BUS_XGS_MIIM_EXT_INSTALLED
    &phy_bus_xgs_miim_ext,
#endif
    NULL
};

#endif

int
bcm56800_a0_port_speed_max(int unit, int port)
{
    if ((CDK_XGS_FLAGS(unit) & CHIP_FLAG_56580) && port < 16) {
        return 2500;
    }
    if (port < 10) {
        if (CDK_XGS_FLAGS(unit) & CHIP_FLAG_TDM8_X) {
            return 13000;
        }
    } else {
        if (CDK_XGS_FLAGS(unit) & CHIP_FLAG_TDM8_Y) {
            return 13000;
        }
    }
    return 10000;
}

int
bcm56800_a0_port_ethernet(int unit, int port)
{
    if (port < 10) {
        if (CDK_XGS_FLAGS(unit) & CHIP_FLAG_NOETH_X) {
            return 0;
        }
    } else {
        if (CDK_XGS_FLAGS(unit) & CHIP_FLAG_NOETH_Y) {
            return 0;
        }
    }
    return 1;
}

int
bcm56800_a0_bmd_attach(int unit)
{
    int port;
    cdk_pbmp_t pbmp;

    if(!CDK_DEV_EXISTS(unit)) {
        return CDK_E_UNIT;
    }

    CDK_XGS_BLKTYPE_PBMP_GET(unit, BLKTYPE_GXPORT, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        CDK_ASSERT(port < BMD_CONFIG_MAX_PORTS);
        if ((CDK_XGS_FLAGS(unit) & CHIP_FLAG_56580) && port < 16) {
            BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_GE;
        } else if (((CDK_XGS_FLAGS(unit) & CHIP_FLAG_TDM8_X) && port < 10) ||
                   ((CDK_XGS_FLAGS(unit) & CHIP_FLAG_TDM8_Y) && port >= 10)) {
            BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_HG;
        } else {
            BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_XE;
        }
#if BMD_CONFIG_INCLUDE_PHY == 1
        if (CDK_XGS_FLAGS(unit) & CHIP_FLAG_56580) {
            bmd_phy_bus_set(unit, port, bcm56580_a0_gxport_phy_bus);
        } else {
            bmd_phy_bus_set(unit, port, bcm56800_a0_gxport_phy_bus);
        }
#endif
    }

    port = CMIC_PORT;
    CDK_ASSERT(port < BMD_CONFIG_MAX_PORTS);
    BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_CPU;

    BMD_DEV_FLAGS(unit) |= BMD_DEV_ATTACHED;

    return 0; 
}
#endif /* CDK_CONFIG_INCLUDE_BCM56800_A0 */
