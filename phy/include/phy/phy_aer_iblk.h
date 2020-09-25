/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef __PHY_AER_IBLK_H__
#define __PHY_AER_IBLK_H__

#include <phy/phy_reg.h>

/*
 * Always set bit 15 in block address register 0x1f to make the
 * register contents more similar to the clause 45 address.
 */
#ifndef PHY_AER_IBLK_DBG_BIT15
#define PHY_AER_IBLK_DBG_BIT15  1
#endif

/* Special lane values for broadcast and dual-lane multicast */
#define PHY_AER_IBLK_BCAST      0x1ff
#define PHY_AER_IBLK_MCAST01    (0x1ff + 1)
#define PHY_AER_IBLK_MCAST23    (0x1ff + 2)


extern int
phy_aer_iblk_read(phy_ctrl_t *pc, uint32_t addr, uint32_t *data);

extern int
phy_aer_iblk_write(phy_ctrl_t *pc, uint32_t addr, uint32_t data);

#endif /* __PHY_AER_IBLK_H__ */
