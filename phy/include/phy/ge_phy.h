/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef __GE_PHY_H__
#define __GE_PHY_H__

#include <phy/phy.h>

extern int
ge_phy_reset(phy_ctrl_t *pc);

extern int
ge_phy_autoneg_gcd(phy_ctrl_t *pc, uint32_t *speed, int *duplex);

extern int
ge_phy_link_get(phy_ctrl_t *pc, int *link_up, int *autoneg_done);

extern int
ge_phy_duplex_set(phy_ctrl_t *pc, int duplex);

extern int
ge_phy_duplex_get(phy_ctrl_t *pc, int *duplex);

extern int
ge_phy_speed_set(phy_ctrl_t *pc, uint32_t speed);

extern int
ge_phy_speed_get(phy_ctrl_t *pc, uint32_t *speed);

extern int
ge_phy_loopback_set(phy_ctrl_t *pc, int enable);

extern int
ge_phy_loopback_get(phy_ctrl_t *pc, int *enable);

extern int 
ge_phy_adv_local_set(phy_ctrl_t *pc, uint32_t abil);

extern int 
ge_phy_adv_local_get(phy_ctrl_t *pc, uint32_t *abil);

extern int
ge_phy_autoneg_set(phy_ctrl_t *pc, int autoneg);

extern int
ge_phy_autoneg_get(phy_ctrl_t *pc, int *autoneg);

#endif /* __GE_PHY_H__ */
