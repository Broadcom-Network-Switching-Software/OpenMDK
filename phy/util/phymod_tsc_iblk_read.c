/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * XGS internal PHY read function with AER support.
 *
 * This function accepts a 32-bit PHY register address and will
 * properly configure clause 45 DEVAD and XAUI lane access.
 * Please see phymod_reg.h for additional information.
 */

#include <phy/phy_tsc_iblk.h>
#include <phy/phy_tsc_sbus.h>
#include <phymod/acc/phymod_tsc_iblk.h>

int
phymod_tsc_iblk_read(const phymod_access_t *pa, uint32_t addr, uint32_t *data)
{
    int ioerr;
    phy_ctrl_t *pc;
    uint32_t lane, lane_map;
    uint32_t pll_index;

    pc = (phy_ctrl_t *)(pa->user_acc);

    pll_index = PHYMOD_ACC_PLLIDX(pa) & 0x3;
    addr |= (pll_index << PHY_REG_ACCESS_FLAGS_SHIFT);

    lane_map = PHYMOD_ACC_LANE_MASK(pa);
    if (lane_map) {
        if (lane_map & 0x1) {
            PHY_CTRL_LANE(pc) = 0;
        } else if (lane_map & 0x2) {
            PHY_CTRL_LANE(pc) = 1;
        } else if (lane_map & 0x4) {
            PHY_CTRL_LANE(pc) = 2;
        } else if (lane_map & 0x8) {
            PHY_CTRL_LANE(pc) = 3;
        } else if (lane_map & 0xfff0) {
            lane = -1;
            while (lane_map) {
                lane++;
                lane_map >>= 1;
            }
            PHY_CTRL_LANE(pc) = lane;
        }
    }
    
    PHY_CTRL_LANE(pc) |= PHY_LANE_VALID;
    
    ioerr = phy_tsc_sbus_mdio_read(pc, addr, data);

    /* Clear LANE to zero */
    PHY_CTRL_LANE(pc) = 0;
    
    return ioerr;
}

