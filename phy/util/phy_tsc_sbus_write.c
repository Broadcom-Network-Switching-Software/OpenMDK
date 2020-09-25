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

#include <phy/phy_tsc_sbus.h>

#define FORCE_CL45              0x20

#define MDIO_UC_MAILBOXr        0x0010ffc8
#define RAMWORDr                0x0010ffc0
#define ADDRESSr                0x0010ffc1
#define WRDATAr                 0x0010ffc3

int
phy_tsc_sbus_mdio_write(phy_ctrl_t *pc, uint32_t addr, uint32_t data)
{
    int ioerr = 0;
    uint32_t devad = (addr >> 16) & 0xf;
    uint32_t reg_copies = (addr >> 20) & 0xf;
    uint32_t aer;
    uint32_t pll_idx;

    aer = 0;
    if (PHY_CTRL_LANE(pc) & PHY_LANE_VALID) {
        /* Setting lane value overrides default behavior */
        aer = PHY_CTRL_LANE(pc) & ~PHY_LANE_VALID;
    } else if (PHY_CTRL_FLAGS(pc) & PHY_F_SERDES_MODE) {
        /* Set lane if independent lanes share PHY address */
        if (PHY_CTRL_FLAGS(pc) & PHY_F_ADDR_SHARE) {
            aer = PHY_CTRL_INST(pc) & 0x3;
        }
        /* Fall back if not per-lane register */
        if (reg_copies == 1) {
            aer = 0;
        } else if (reg_copies == 2) {
            aer &= ~0x1;
        }
        /* By default we write both lanes in 2-lane mode */
        if ((PHY_CTRL_FLAGS(pc) & PHY_F_2LANE_MODE) && reg_copies != 1) {
            if ((PHY_CTRL_INST(pc) & 0x3) == 0) {
                /* Multicast lanes 0 and 1 */
                aer = PHY_TSC_SBUS_MCAST01;
            } else {
                /* Multicast lanes 2 and 3 */
                aer = PHY_TSC_SBUS_MCAST23;
            }
        }
    }
    
    pll_idx = PHY_REG_ACC_TSC_SBUS_PLL(addr);
    aer |= (pll_idx << 8);

    /* If bus driver supports lane control, then we are done */
    if (PHY_CTRL_FLAGS(pc) & PHY_F_LANE_CTRL) {
        addr &= 0xffff;

        aer |= (devad << 11);
        ioerr += PHY_BUS_WRITE(pc, addr | (aer << 16), data);
        return ioerr;
    }

    addr &= 0xffff;
    /* DEVAD 0x20 forces clause 45 access on DEVAD 0 */
    if (devad == 0) {
        devad = FORCE_CL45;
    }
    ioerr += PHY_BUS_WRITE(pc, 0xffde | (devad << 16), aer);
    ioerr += PHY_BUS_WRITE(pc, addr | (devad << 16), data);
    return ioerr;
}

int
phy_tsc_sbus_write(phy_ctrl_t *pc, uint32_t addr, uint32_t data)
{
    return phy_tsc_sbus_mdio_write(pc, addr, data);
}
