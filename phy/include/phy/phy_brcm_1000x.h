/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef __PHY_BRCM_1000X_H__
#define __PHY_BRCM_1000X_H__

#include <phy/phy_reg.h>

extern int
phy_brcm_1000x_read(phy_ctrl_t *pc, uint32_t addr, uint32_t *data);

extern int
phy_brcm_1000x_write(phy_ctrl_t *pc, uint32_t addr, uint32_t data);

#endif /* __PHY_BRCM_1000X_H__ */
