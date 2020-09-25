/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef __PHY_XGS_IBLK_H__
#define __PHY_XGS_IBLK_H__

#include <phy/phy_reg.h>

/* Transform register address from software API to datasheet format */
#define PHY_XGS_IBLK_TO_C45(_a) \
    (((_a) & 0xf) | (((_a) >> 8) & 0x7ff0) | (((_a) << 11) & 0x8000));

/* Transform register address from datasheet to software API format */
#define PHY_XGS_C45_TO_IBLK(_a) \
    ((((_a) & 0x7ff0) << 8) | (((_a) & 0x8000) >> 11) | ((_a) & 0xf))

/*
 * Always set bit 15 in block address register 0x1f to make the
 * register contents more similar to the clause 45 address.
 */
#ifndef PHY_XGS_IBLK_DBG_BIT15
#define PHY_XGS_IBLK_DBG_BIT15  1
#endif


extern int
phy_xgs_iblk_read(phy_ctrl_t *pc, uint32_t addr, uint32_t *data);

extern int
phy_xgs_iblk_write(phy_ctrl_t *pc, uint32_t addr, uint32_t data);

extern int
phy_xgs_iblk_map_ieee(phy_ctrl_t *pc);

#endif /* __PHY_XGS_IBLK_H__ */
