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
 * Please see phy_reg.h for additional information.
 */

#include <phy/phy_tsc_iblk.h>

#define FORCE_CL45              0x20

#define MDIO_UC_MAILBOXr        0x0010ffc8
#define RAMWORDr                0x0010ffc0
#define ADDRESSr                0x0010ffc1
#define WRDATAr                 0x0010ffc3

int
phy_tsc_iblk_mdio_read(phy_ctrl_t *pc, uint32_t addr, uint32_t *data)
{
    int ioerr = 0;
    uint32_t devad = (addr >> 16) & 0xf;
    uint32_t reg_copies = (addr >> 20) & 0x7;
    uint32_t blkaddr, regaddr;
    uint32_t aer;

    /* Do not attempt to read write-only registers */
    if (addr & PHY_REG_ACC_TSC_IBLK_WR_ONLY) {
        *data = 0;
        return  CDK_E_NONE;
    }

    aer = 0;
    if (PHY_CTRL_LANE(pc) & PHY_LANE_VALID) {
        /* Setting lane value overrides default behavior */
        aer = PHY_CTRL_LANE(pc) & ~PHY_LANE_VALID;
    } else if (addr & PHY_REG_ACC_TSC_IBLK_FORCE_LANE) {
        /* Forcing lane overrides default behavior */
        aer = (addr >> PHY_REG_ACCESS_FLAGS_SHIFT) & 0x7;
    } else if (PHY_CTRL_FLAGS(pc) & PHY_F_SERDES_MODE) {
        /* Set lane if independent lanes share PHY address */
        if (PHY_CTRL_FLAGS(pc) & PHY_F_ADDR_SHARE) {
            aer = PHY_CTRL_INST(pc) & 0x3;
        }
        if (reg_copies == 1) {
            aer = 0;
        } else if (reg_copies == 2) {
            aer &= ~0x1;
        }
    } else {
        /* Optional lane 0 override */
        aer = (addr >> PHY_REG_ACCESS_FLAGS_SHIFT) & 0x3;
    }

    /* If bus driver supports lane control, then we are done */
    if (PHY_CTRL_FLAGS(pc) & PHY_F_LANE_CTRL) {
        addr &= 0xffff;

        aer |= (devad << 11);
        ioerr += PHY_BUS_READ(pc, addr | (aer << 16), data);
        return ioerr;
    }

    if (PHY_CTRL_FLAGS(pc) & PHY_F_CLAUSE45) {
        addr &= 0xffff;
        /* DEVAD 0x20 forces clause 45 access on DEVAD 0 */
        if (devad == 0) {
            devad = FORCE_CL45;
        }
        ioerr += PHY_BUS_WRITE(pc, 0xffde | (devad << 16), aer);
        ioerr += PHY_BUS_READ(pc, addr | (devad << 16), data);
        return ioerr;
    }

    /* Select device if non-zero */
    if (devad) {
        aer |= (devad << 11);
    }
    ioerr += PHY_BUS_WRITE(pc, 0x1f, 0xffd0);
    ioerr += PHY_BUS_WRITE(pc, 0x1e, aer);

    /* Select block */
#if PHY_TSC_IBLK_DBG_BIT15
    blkaddr = addr & 0xfff0;
#else
    blkaddr = addr & 0x7ff0;
#endif
    ioerr += PHY_BUS_WRITE(pc, 0x1f, blkaddr);

    /* Read register value */
    regaddr = addr & 0xf;
    if (addr & 0x8000) {
        regaddr |= 0x10;
    }
    ioerr += PHY_BUS_READ(pc, regaddr, data);

    return ioerr;
}

int
phy_tsc_iblk_proxy_read(phy_ctrl_t *pc, uint32_t addr, uint32_t *data)
{
    int ioerr = 0;
    int cnt, done;
    uint32_t aer, cmd, data_rd;
    uint32_t regaddr = addr & 0xffff;
    uint32_t reg_copies = (addr >> 20) & 0x7;

    /* Do not attempt to read write-only registers */
    if (addr & PHY_REG_ACC_TSC_IBLK_WR_ONLY) {
        *data = 0;
        return CDK_E_NONE;
    }

    aer = 0;
    if (PHY_CTRL_LANE(pc) & PHY_LANE_VALID) {
        /* Setting lane value overrides default behavior */
        aer = PHY_CTRL_LANE(pc) & ~PHY_LANE_VALID;
    } else if (addr & PHY_REG_ACC_TSC_IBLK_FORCE_LANE) {
        /* Forcing lane overrides default behavior */
        aer = (addr >> PHY_REG_ACCESS_FLAGS_SHIFT) & 0x7;
    } else if (PHY_CTRL_FLAGS(pc) & PHY_F_SERDES_MODE) {
        /* Set lane if independent lanes share PHY address */
        if (PHY_CTRL_FLAGS(pc) & PHY_F_ADDR_SHARE) {
            aer = PHY_CTRL_INST(pc) & 0x3;
        }
        if (reg_copies == 1) {
            aer = 0;
        } else if (reg_copies == 2) {
            aer &= ~0x1;
        }
    } else {
        /* Optional lane 0 override */
        aer = (addr >> PHY_REG_ACCESS_FLAGS_SHIFT) & 0x3;
    }

    /* Clear external station to uC cmd */ 
    ioerr += phy_tsc_iblk_mdio_write(pc, MDIO_UC_MAILBOXr, 0x00000000);

    /* Write register address */
    ioerr += phy_tsc_iblk_mdio_write(pc, RAMWORDr, regaddr);

    /* Write lane number */
    ioerr += phy_tsc_iblk_mdio_write(pc, ADDRESSr, aer);

    /* Write command to mailbox */
    cmd = TSC_UC_SYNC_CMD_REQ;
    if (reg_copies == 3) {
        cmd |= TSC_UC_SYNC_CMD_RAM;
    }
    ioerr += phy_tsc_iblk_mdio_write(pc, MDIO_UC_MAILBOXr, cmd);

    /* Wait for command completion */
    done = 0;
    for (cnt = 0; cnt < TSC_UC_PROXY_WAIT_TIME; cnt++) {
        ioerr += phy_tsc_iblk_mdio_read(pc, MDIO_UC_MAILBOXr, &data_rd);
        if (ioerr) {
            return ioerr;
        }
        done = data_rd & TSC_UC_SYNC_CMD_DONE;
        if (done) {
            break;
        }
    }

    if (done == 0) {
        PHY_VERB(pc, ("TSC proxy read did not complete\n"));
        return CDK_E_NONE;
    }

    /* Read the data */
    ioerr += phy_tsc_iblk_mdio_read(pc, WRDATAr, &data_rd);
    *data = data_rd;

    /* Clear external station to uC cmd */ 
    ioerr += phy_tsc_iblk_mdio_write(pc, MDIO_UC_MAILBOXr, 0x00000000);

    return ioerr;
}

int
phy_tsc_iblk_read(phy_ctrl_t *pc, uint32_t addr, uint32_t *data)
{
    if (PHY_CTRL_ADDR_TYPE(pc) && PHY_CTRL_ADDR_TYPE(pc)(pc, addr) != 0) {
        return phy_tsc_iblk_proxy_read(pc, addr, data);
    }
    return phy_tsc_iblk_mdio_read(pc, addr, data);
}
