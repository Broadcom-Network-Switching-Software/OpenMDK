/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * ROBO MIIM access functions for integrated PHYs.
 */

#include <cdk/cdk_device.h>
#include <cdk/cdk_assert.h>
#include <cdk/cdk_debug.h>

#include <cdk/arch/robo_chip.h>
#include <cdk/arch/robo_reg.h>
#include <cdk/arch/robo_miim.h>

/* Convert standard PHY address and register into a ROBO register address */
#define ROBO_REG_ADDR(_pa, _r)    (((_pa) << 8) + (2 * (_r)))
#define EXT_MDIO_REG_ADDR(_pa, _r)   (CDK_DEV_ADDR_EXT_PHY_BUS_MDIO | (((_pa) << 8) + (_r)))

int 
cdk_robo_miim_write(int unit, uint32_t phy_addr, uint32_t reg, uint32_t val)
{
    CDK_ASSERT(CDK_DEV_EXISTS(unit)); 
    
    CDK_DEBUG_MIIM
        (("cdk_robo_miim_write[%d]: phy_addr=0x%08"PRIx32" reg_addr=%08"PRIx32" data: 0x%08"PRIx32"\n",
          unit, phy_addr, reg, val));

    return cdk_robo_reg_write(unit, ROBO_REG_ADDR(phy_addr, reg), &val, 2);
}

int
cdk_robo_miim_read(int unit, uint32_t phy_addr, uint32_t reg, uint32_t *val)
{
    int rv;

    CDK_ASSERT(CDK_DEV_EXISTS(unit)); 
    
    rv = cdk_robo_reg_read(unit, ROBO_REG_ADDR(phy_addr, reg), val, 2);
    
    if (rv >= 0) {
        CDK_DEBUG_MIIM
            (("cdk_robo_miim_read[%d]: phy_addr=0x%08"PRIx32" reg_addr=%08"PRIx32" data: 0x%08"PRIx32"\n",
              unit, phy_addr, reg, *val));
    }
    return rv;
}

int 
cdk_robo_miim_ext_mdio_write(int unit, uint32_t phy_addr, uint32_t reg, uint32_t val)
{
    CDK_ASSERT(CDK_DEV_EXISTS(unit)); 
    
    CDK_DEBUG_MIIM
        (("cdk_robo_miim_write[%d]: phy_addr=0x%08"PRIx32" reg_addr=%08"PRIx32" data: 0x%08"PRIx32"\n",
          unit, phy_addr, reg, val));

    if (phy_addr == 0xffffffff) {
        /* skip attempting access of not supported port */
        return 0;
    } else {
        return cdk_robo_reg_write(unit, EXT_MDIO_REG_ADDR(phy_addr, reg), &val, 2);
    }
}

int
cdk_robo_miim_ext_mdio_read(int unit, uint32_t phy_addr, uint32_t reg, uint32_t *val)
{
    int rv;

    CDK_ASSERT(CDK_DEV_EXISTS(unit)); 

    if (phy_addr == 0xffffffff) {
        /* skip attempting access of not supported port */
        *val = 0xffff;
        rv = 0;
    } else {
        rv = cdk_robo_reg_read(unit, EXT_MDIO_REG_ADDR(phy_addr, reg), val, 2);
    }

    
    if (rv >= 0) {
        CDK_DEBUG_MIIM
            (("cdk_robo_miim_ext_mdio_read[%d]: phy_addr=0x%08"PRIx32" reg_addr=%08"PRIx32" data: 0x%08"PRIx32"\n",
              unit, phy_addr, reg, *val));
    }
    return rv;
}

