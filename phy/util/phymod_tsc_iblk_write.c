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
phymod_tsc_iblk_write(const phymod_access_t *pa, uint32_t addr, uint32_t data)
{
    int ioerr = 0;
    phy_ctrl_t *pc;
    uint32_t lane = 0, lane_map;
    uint32_t wr_mask, rdata;
    uint32_t pll_index;

    pc = (phy_ctrl_t *)(pa->user_acc);

    if (addr & PHYMOD_REG_ACC_AER_IBLK_FORCE_LANE) {
        /* Forcing lane overrides default behavior */
        lane = (addr >> PHY_REG_ACCESS_FLAGS_SHIFT) & 0x7;
    } else {
        lane_map = PHYMOD_ACC_LANE_MASK(pa) & 0xffff;
        if (lane_map) {
            if (lane_map == 0x3) {
                lane = PHY_TSC_IBLK_MCAST01;
            } else if (lane_map == 0xc) {
                lane = PHY_TSC_IBLK_MCAST23;
            } else if (lane_map == 0xf) {
                lane = PHY_TSC_IBLK_BCAST;
            } else {
                lane = -1;
                while (lane_map) {
                    lane++;
                    lane_map >>= 1;
                }
            }
        }
    }

    pll_index = PHYMOD_ACC_PLLIDX(pa) & 0x3;
    addr |= (pll_index << PHY_REG_ACCESS_FLAGS_SHIFT);
    
    /* Check if write mask is specified */
    wr_mask = (data >> 16);
    if (wr_mask) {
        /* Read register if bus driver does not support write mask */
        if (!(PHY_BUS_CAP(pc) & PHY_BUS_CAP_WR_MASK)) {
            ioerr += phymod_tsc_iblk_read(pa, addr, &rdata);
            data = (rdata & ~wr_mask) | (data & wr_mask);
            data &= 0xffff;
        }
    }

    PHY_CTRL_LANE(pc) = lane;
    PHY_CTRL_LANE(pc) |= PHY_LANE_VALID;

    ioerr += phy_tsc_sbus_mdio_write(pc, addr, data);
    
    /* Clear LANE to zero */
    PHY_CTRL_LANE(pc) = 0;
    
    return ioerr;
}
