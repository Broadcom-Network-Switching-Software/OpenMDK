/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef __PHY_DRVLIST_H__
#define __PHY_DRVLIST_H__

#include <phy/phy.h>

#define PHY_DEVLIST_ENTRY(_nm, _bd, _fl, _desc, _r0, _r1) extern phy_driver_t _bd##_drv;
#define PHY_DEVLIST_INCLUDE_ALL
#include <phy/phy_devlist.h>

extern phy_driver_t unknown_drv;

#endif /* __PHY_DRVLIST_H__ */
