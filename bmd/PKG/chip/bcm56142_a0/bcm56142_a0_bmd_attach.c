/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56142_A0 == 1

#include <bmd/bmd.h>
#include <bmd/bmd_device.h>
#include <bmd/bmd_phy_ctrl.h>
#include <cdk/arch/xgs_chip.h>
#include <cdk/cdk_assert.h>
#include <cdk/chip/bcm56142_a0_defs.h>
#include "bcm56142_a0_bmd.h"
#include "bcm56142_a0_internal.h"

#if BMD_CONFIG_INCLUDE_PHY == 1
#include <phy/phy_buslist.h>

static phy_bus_t *bcm56142_phy_bus[] = {
#ifdef PHY_BUS_BCM56142_MIIM_INT_INSTALLED
    &phy_bus_bcm56142_miim_int,
#endif
#ifdef PHY_BUS_BCM956142K_MIIM_EXT_INSTALLED
    &phy_bus_bcm956142k_miim_ext,
#endif
    NULL
};
#endif /* BMD_CONFIG_INCLUDE_PHY */

int
bcm56142_a0_xport_pbmp_get(int unit, int flag, cdk_pbmp_t *pbmp)
{
    int port;
    cdk_pbmp_t gbmp, xqbmp;

    CDK_PBMP_CLEAR(*pbmp);
    CDK_PBMP_CLEAR(gbmp);
    CDK_PBMP_CLEAR(xqbmp);

    if (flag & XPORT_FLAG_GPORT) {
         CDK_XGS_BLKTYPE_PBMP_GET(unit, BLKTYPE_GPORT, &gbmp);
         CDK_PBMP_OR(*pbmp, gbmp);
    }
    
    if (flag & XPORT_FLAG_XQPORT) {
         CDK_XGS_BLKTYPE_PBMP_GET(unit, BLKTYPE_XQPORT, &xqbmp);
         CDK_PBMP_OR(*pbmp, xqbmp);
    }

    CDK_PBMP_ITER(*pbmp, port) {
        if (BMD_PORT_PROPERTIES(unit, port) == 0) {
            CDK_PBMP_PORT_REMOVE(*pbmp, port);
        }
    }
    
    return 0;
}

int 
bcm56142_a0_bmd_attach(int unit)
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
        if (CDK_XGS_FLAGS(unit) & CHIP_FLAG_FE_ONLY) {
            BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_FE;
        }
#if BMD_CONFIG_INCLUDE_PHY == 1
        bmd_phy_bus_set(unit, port, bcm56142_phy_bus);
#endif
    }

#if BMD_CONFIG_INCLUDE_HIGIG == 1 || BMD_CONFIG_INCLUDE_XE == 1
    CDK_XGS_BLKTYPE_PBMP_GET(unit, BLKTYPE_XQPORT, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        CDK_ASSERT(port < BMD_CONFIG_MAX_PORTS);

#if BMD_CONFIG_INCLUDE_HIGIG == 1
        BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_HG;
#else
        BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_XE;
#endif

        if (CDK_XGS_FLAGS(unit) & CHIP_FLAG_HD25_HD127) {
            if ((port == 28) || (port == 29)) {
                BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_GE;
            }
        } else if ((CDK_XGS_FLAGS(unit) & CHIP_FLAG_FE_ONLY) || 
                   (CDK_XGS_FLAGS(unit) & CHIP_FLAG_HD25_ONLY)) {
            BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_GE;
        } else {
            if (!CDK_CHIP_CONFIG(unit) ||
                (CDK_CHIP_CONFIG(unit) & DCFG_GE) ||
                ((CDK_CHIP_CONFIG(unit) & DCFG_HG_GE) && 
                 ((port == 28) || (port == 29)))) {
                BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_GE;
            } 
            
            if (((CDK_CHIP_CONFIG(unit) & DCFG_HG) && 
                 ((port == 27) || (port == 29))) ||
                ((CDK_CHIP_CONFIG(unit) & DCFG_HG_GE) && (port == 27)) ||
                ((CDK_CHIP_CONFIG(unit) & DCFG_HG_HGD) && (port == 27))) {
                BMD_PORT_PROPERTIES(unit, port) = 0;
            }
        }
        
#if BMD_CONFIG_INCLUDE_PHY == 1
        bmd_phy_bus_set(unit, port, bcm56142_phy_bus);
#endif
    }
#endif

    port = CMIC_PORT;
    CDK_ASSERT(port < BMD_CONFIG_MAX_PORTS);
    BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_CPU;

    BMD_DEV_FLAGS(unit) |= BMD_DEV_ATTACHED;

    return 0; 
}

#endif /* CDK_CONFIG_INCLUDE_BCM56142_A0 */
