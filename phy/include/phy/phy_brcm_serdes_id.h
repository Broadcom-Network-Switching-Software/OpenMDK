/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef __PHY_BRCM_SERDES_ID_H__
#define __PHY_BRCM_SERDES_ID_H__

#include <phy/phy.h>

/* Serdes ID registers that will map block both as [3:0] and [14:4] */
#define SERDES_ID0              0x031110
#define SERDES_ID1              0x031111
#define SERDES_ID2              0x031112

extern int
phy_brcm_serdes_id(phy_ctrl_t *pc, uint32_t *phyid0, uint32_t *phyid1);

#endif /* __PHY_BRCM_SERDES_ID_H__ */
