/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * XGS internal PHY write function with AER support.
 *
 * This function accepts a 32-bit PHY register address and will
 * properly configure clause 45 DEVAD and XAUI lane access.
 * Please see phy_reg.h for additional information.
 */

#include <phy/phy_aer_iblk.h>

int
phy_aer_iblk_write(phy_ctrl_t *pc, uint32_t addr, uint32_t data)
{
    int ioerr = 0;
    uint32_t devad = (addr >> 16) & 0xf;
    uint32_t reg_copies = (addr >> 20) & 0xf;
    uint32_t blkaddr, regaddr;
    uint32_t aer, lane_mask;

    /* Assume 4 lanes if lane_mask not set */
    lane_mask = PHY_CTRL_LANE_MASK(pc);
    if (lane_mask == 0) {
        lane_mask = 0x3;
    }

    aer = 0;
    if (PHY_CTRL_LANE(pc) & PHY_LANE_VALID) {
        /* Setting lane value overrides default behavior */
        aer = PHY_CTRL_LANE(pc) & ~PHY_LANE_VALID;
    } else if (addr & PHY_REG_ACC_AER_IBLK_FORCE_LANE) {
        /* Forcing lane overrides default behavior */
        aer = (addr >> PHY_REG_ACCESS_FLAGS_SHIFT) & 0x7;
    } else if (addr & PHY_REG_ACC_AER_IBLK_BCAST) {
        /* Broadcast to all lanes if multiple copies */
        if (reg_copies != 1) {
            aer = PHY_AER_IBLK_BCAST;
        }
    } else if (PHY_CTRL_FLAGS(pc) & PHY_F_SERDES_MODE) {
        /* Set lane if independent lanes share PHY address */
        if (PHY_CTRL_FLAGS(pc) & PHY_F_ADDR_SHARE) {
            aer = PHY_CTRL_INST(pc) & lane_mask;
        }
        /* Fall back if not per-lane register */
        if (reg_copies == 1) {
            aer = 0;
        } else if (reg_copies == 2) {
            aer &= ~0x1;
        }
        /* By default we write both lanes in 2-lane mode */
        if ((PHY_CTRL_FLAGS(pc) & PHY_F_2LANE_MODE) && reg_copies != 1) {
            if ((PHY_CTRL_INST(pc) & lane_mask) == 0) {
                /* Multicast lanes 0 and 1 */
                aer = PHY_AER_IBLK_MCAST01;
            } else {
                /* Multicast lanes 2 and 3 */
                aer = PHY_AER_IBLK_MCAST23;
            }
        }
    } else {
        /* Broadcast to all lanes if multiple copies */
        if (reg_copies != 1) {
            aer = PHY_AER_IBLK_BCAST;
        }
    }

    if (PHY_CTRL_FLAGS(pc) & PHY_F_CLAUSE45) {
        addr &= 0xffff;
        /* DEVAD 0 is not supported, so use DEVAD 1 instead */
        if (devad == 0) {
            devad = 1;
        }
        ioerr += PHY_BUS_WRITE(pc, 0xffde | (devad << 16), aer);
        ioerr += PHY_BUS_WRITE(pc, addr | (devad << 16), data);
        return ioerr;
    }

    /* Select device if non-zero */
    if (devad) {
        aer |= (devad << 11);
    }
    ioerr += PHY_BUS_WRITE(pc, 0x1f, 0xffd0);
    ioerr += PHY_BUS_WRITE(pc, 0x1e, aer);

    /* Select block */
#if PHY_AER_IBLK_DBG_BIT15
    blkaddr = addr & 0xfff0;
#else
    blkaddr = addr & 0x7ff0;
#endif
    ioerr += PHY_BUS_WRITE(pc, 0x1f, blkaddr);

    /* Write register value */
    regaddr = addr & 0xf;
    if (addr & 0x8000) {
        regaddr |= 0x10;
    }
    ioerr += PHY_BUS_WRITE(pc, regaddr, data);

    return ioerr;
}
