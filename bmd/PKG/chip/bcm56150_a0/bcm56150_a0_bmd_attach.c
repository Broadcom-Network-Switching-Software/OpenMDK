/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56150_A0 == 1

#include <bmd/bmd.h>
#include <bmd/bmd_device.h>
#include <bmd/bmd_phy_ctrl.h>

#include <cdk/arch/xgsm_chip.h>
#include <cdk/cdk_assert.h>
#include <cdk/cdk_debug.h>
#include <cdk/chip/bcm56150_a0_defs.h>

#include "bcm56150_a0_bmd.h"
#include "bcm56150_a0_internal.h"

#if BMD_CONFIG_INCLUDE_PHY == 1
#include <phy/phy_buslist.h>

static phy_bus_t *bcm56150_phy_bus[] = {
#ifdef PHY_BUS_BCM56150_MIIM_INT_INSTALLED
    &phy_bus_bcm56150_miim_int,
#endif
#ifdef PHY_BUS_BCM956150K_MIIM_EXT_INSTALLED
    &phy_bus_bcm956150k_miim_ext,
#endif
    NULL
};

static phy_bus_t *bcm56151_phy_bus[] = {
#ifdef PHY_BUS_BCM56151_MIIM_INT_INSTALLED
    &phy_bus_bcm56151_miim_int,
#endif
#ifdef PHY_BUS_BCM956151K_MIIM_EXT_INSTALLED
    &phy_bus_bcm956151k_miim_ext,
#endif
    NULL
};

#define PHY_BUS_SET(_u,_p,_b) bmd_phy_bus_set(_u,_p,_b)

#else

#define PHY_BUS_SET(_u,_p,_b)

#endif

static struct _uplink_map_s {
    int port[8];
} _uplink_map[] = {
    {{ 26, 27, 28, 29, -1, -1, -1, -1 }}, /* 0 */
    {{ -1, -1, -1, -1, 26, 27, 28, 29 }}, /* 1 */
    {{ 26, -1, 27, -1, 28, -1, 29, -1 }}, /* 2 */
    {{ 26, 27, 28, -1, 29, -1, -1, -1 }}, /* 3 */
    {{ 26, -1, -1, -1, 27, 28, 29, -1 }}, /* 4 */
    {{ 26, -1, -1, -1, 27, -1, -1, -1 }}, /* 5 */
    {{ 26, -1, -1, -1, 21, 23, 24, 25 }}, /* 6 */
    {{ 26, 27, 28, 29, 21, 23, 24, 25 }}  /* 7 */
};

int
bcm56150_a0_xlport_pbmp_get(int unit, cdk_pbmp_t *pbmp)
{
    int port;

    CDK_XGSM_BLKTYPE_PBMP_GET(unit, BLKTYPE_XLPORT, pbmp);
    CDK_PBMP_ITER(*pbmp, port) {
        if (BMD_PORT_PROPERTIES(unit, port) == 0) {
            CDK_PBMP_PORT_REMOVE(*pbmp, port);
        }
    }
    return 0;
}

int
bcm56150_a0_p2l(int unit, int port, int inverse)
{
    int pdx, map_idx;
    uint32_t dcfg = CDK_CHIP_CONFIG(unit);

    map_idx = 0;
    if (dcfg & DCFG_TSC_1) {
        map_idx = 1;
    } else if (dcfg & (DCFG_TSC_0_1 | DCFG_HGD_0 | DCFG_HGD_1)) {
        map_idx = 2;
    } else if (dcfg & DCFG_XAUI_1) {
        map_idx = 3;
    } else if (dcfg & DCFG_XAUI_0) {
        map_idx = 4;
    } else if (dcfg & DCFG_XAUI_0_1) {
        map_idx = 5;
    } else if (dcfg & DCFG_XAUI_0_1G) {
        map_idx = 6;
    } else if (CDK_XGSM_FLAGS(unit) & CHIP_FLAG_GE6) {
        map_idx = 7;
    }
    
    if (inverse) {
        for (pdx =0; pdx < 8; pdx++) {
            if (port == _uplink_map[map_idx].port[pdx]) {
                return 26 + pdx;
            }
        }
        return port;
    } else {
        if (port >= 26 && port < 34) {
            return _uplink_map[map_idx].port[port - 26];
        } else {
            return port;
        }
    }
    return port;
}

int
bcm56150_a0_p2m(int unit, int port, int inverse)
{
    /* MMU port map is same as logical port map */
    return bcm56150_a0_p2l(unit, port, inverse);
}

uint32_t
bcm56150_a0_port_speed_max(int unit, int port)
{
    if (P2L(unit, port) <= 0) {
        return 0;
    }
    if (port >= 26 && port < 30) {
        if (CDK_CHIP_CONFIG(unit) & DCFG_HGD_0) {
            return 13000;
        } else if (CDK_XGSM_FLAGS(unit) & CHIP_FLAG_TSC_1G) {
            return 1000;
        } 
        return 10000;
    } 
    if (port >= 30 && port < 34) {
        if (CDK_CHIP_CONFIG(unit) & DCFG_HGD_1) {
            return 13000;
        } else if (CDK_CHIP_CONFIG(unit) & DCFG_XAUI_0) {
            return 10000;
        } else if (CDK_XGSM_FLAGS(unit) & CHIP_FLAG_TSC_10G) {
            return 10000;
        } 
        return 1000;
    }
    return 1000;
}

int
bcm56150_a0_port_num_lanes(int unit, int port)
{
    uint32_t dcfg = CDK_CHIP_CONFIG(unit);
    int lanes;
    
    lanes = 1;
    if (port >= 26 && port < 30) {
        if (dcfg & (DCFG_XAUI_0 | DCFG_XAUI_0_1G | DCFG_XAUI_0_1)) {
            lanes = 4;
        } else if (dcfg & DCFG_HGD_0) {
            lanes = 2;
        }
    } else if (port >= 30 && port <= 34) {
        if (dcfg & (DCFG_XAUI_1 | DCFG_XAUI_0_1)) {
            lanes = 4;
        } else if (dcfg & DCFG_HGD_1) {
            lanes = 2;
        }
    } 
        
    return lanes;
}

int 
bcm56150_a0_bmd_attach(int unit)
{
    int port;
    cdk_pbmp_t pbmp;
    uint32_t speed_max;
#if BMD_CONFIG_INCLUDE_PHY == 1
    phy_bus_t **phy_bus;
#endif

    if(!CDK_DEV_EXISTS(unit)) {
        return CDK_E_UNIT;
    }

    /* Only one bit can be set at a time */
    if (CDK_CHIP_CONFIG(unit) & (CDK_CHIP_CONFIG(unit) - 1)) {
        return CDK_E_CONFIG;
    }

#if BMD_CONFIG_INCLUDE_PHY == 1
    if (CDK_XGSM_FLAGS(unit) & CHIP_FLAG_NO_PHY) {
        phy_bus = bcm56151_phy_bus;
    } else {
        phy_bus = bcm56150_phy_bus;
    }   
#endif

    CDK_XGSM_BLKTYPE_PBMP_GET(unit, BLKTYPE_GPORT, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        CDK_ASSERT(port < BMD_CONFIG_MAX_PORTS);
        speed_max = bcm56150_a0_port_speed_max(unit, port);
        if (speed_max > 0) {
            BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_GE;
        }

        PHY_BUS_SET(unit, port, phy_bus);
    }

#if BMD_CONFIG_INCLUDE_HIGIG == 1 || BMD_CONFIG_INCLUDE_XE == 1
    CDK_XGSM_BLKTYPE_PBMP_GET(unit,BLKTYPE_XLPORT, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        CDK_ASSERT(port < BMD_CONFIG_MAX_PORTS);
        speed_max = bcm56150_a0_port_speed_max(unit, port);
        if (speed_max >= 10000) {
            BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_XE;
        } else if (speed_max > 0) {
            BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_GE;
        }
#if BMD_CONFIG_INCLUDE_HIGIG == 1
        if (speed_max > 10000) {
            BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_HG;
        }
#endif
        
        PHY_BUS_SET(unit, port, phy_bus);
    }
#endif

    port = CMIC_PORT;
    CDK_ASSERT(port < BMD_CONFIG_MAX_PORTS);
    BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_CPU;

    /* Initialize debug functions */
    BMD_PORT_SPEED_MAX(unit) = bcm56150_a0_port_speed_max;
    BMD_PORT_P2L(unit) = bcm56150_a0_p2l;
    BMD_PORT_P2M(unit) = bcm56150_a0_p2m;

    BMD_DEV_FLAGS(unit) |= BMD_DEV_ATTACHED;

    return 0; 
}

#endif /* CDK_CONFIG_INCLUDE_BCM56150_A0 */
