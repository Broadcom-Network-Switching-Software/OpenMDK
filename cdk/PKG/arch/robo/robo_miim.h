/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * MIIM access.
 */

#ifndef __ROBO_MIIM_H__
#define __ROBO_MIIM_H__

/* MII write access. */
extern int 
cdk_robo_miim_write(int unit, uint32_t phy_addr, uint32_t reg, uint32_t val); 

/* MII read access. */
extern int
cdk_robo_miim_read(int unit, uint32_t phy_addr, uint32_t reg, uint32_t *val); 

extern int 
cdk_robo_miim_ext_mdio_write(int unit, uint32_t phy_addr, uint32_t reg, uint32_t val);

extern int
cdk_robo_miim_ext_mdio_read(int unit, uint32_t phy_addr, uint32_t reg, uint32_t *val);
#endif /* __ROBO_MIIM_H__ */
