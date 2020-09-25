/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * PHY driver for BCM5464.
 */

#include <phy/phy.h>
#include <phy/ge_phy.h>

#define BCM5464_PHY_ID0                 0x0020
#define BCM5464_PHY_ID1                 0x60b0

#define PHY_ID1_REV_MASK                0x000f

/* Default LED control */
#define BCM5464_LED1_SEL(_pc)           0x0
#define BCM5464_LED2_SEL(_pc)           0x1
#define BCM5464_LED3_SEL(_pc)           0x3
#define BCM5464_LED4_SEL(_pc)           0x6
#define BCM5464_LEDCTRL(_pc)            0x8
#define BCM5464_LEDSELECT(_pc)          0x0

/* Access to shadowed registers at offset 0x18 */
#define REG_18_SEL(_s)                  (((_s) << 12) | 0x7)
#define REG_18_WR(_s,_v)                (((_s) == 7 ? 0x8000 : 0) | (_v) | (_s))

/* Access to shadowed registers at offset 0x1c */
#define REG_1C_SEL(_s)                  ((_s) << 10)
#define REG_1C_WR(_s,_v)                (REG_1C_SEL(_s) | (_v) | 0x8000)

/* Access expansion registers at offset 0x15 */
#define MII_EXP_MAP_REG(_r)             ((_r) | 0x0f00)
#define MII_EXP_UNMAP                   (0)

/*
 * Non-standard MII Registers
 */
#define MII_ECR_REG             0x10 /* MII Extended Control Register */
#define MII_EXP_REG             0x15 /* MII Expansion registers */
#define MII_EXP_SEL             0x17 /* MII Expansion register select */
#define MII_TEST1_REG           0x1e /* MII Test Register 1 */

/***********************************************************************
 *
 * PHY DRIVER FUNCTIONS
 */

#if PHY_CONFIG_INCLUDE_CHIP_SYMBOLS == 1
extern cdk_symbols_t bcm5464_symbols;
#endif

/*
 * Function:
 *      bcm5464_phy_probe
 * Purpose:     
 *      Probe for 5464 PHY
 * Parameters:
 *      pc - PHY control structure
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm5464_phy_probe(phy_ctrl_t *pc)
{
    uint32_t phyid0, phyid1;
    int ioerr = 0;

    PHY_CTRL_CHECK(pc);

    ioerr += PHY_BUS_READ(pc, MII_PHY_ID0_REG, &phyid0);
    ioerr += PHY_BUS_READ(pc, MII_PHY_ID1_REG, &phyid1);

    if (phyid0 == BCM5464_PHY_ID0 && 
        (phyid1 & ~PHY_ID1_REV_MASK) == BCM5464_PHY_ID1) {

#if PHY_CONFIG_INCLUDE_CHIP_SYMBOLS == 1
        PHY_CTRL_SYMBOLS(pc) = &bcm5464_symbols;
#endif
        return ioerr ? CDK_E_IO : CDK_E_NONE;
    }
    return CDK_E_NOT_FOUND;
}

/*
 * Function:
 *      bcm5464_phy_notify
 * Purpose:     
 *      Handle PHY notifications
 * Parameters:
 *      pc - PHY control structure
 *      event - PHY event
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm5464_phy_notify(phy_ctrl_t *pc, phy_event_t event)
{
    int rv = CDK_E_NONE;
    int autoneg;

    PHY_CTRL_CHECK(pc);

    switch (event) {
    case PhyEvent_ChangeToCopper:
        PHY_CTRL_FLAGS(pc) &= ~PHY_F_FIBER_MODE;
        PHY_CTRL_FLAGS(pc) &= ~PHY_F_PASSTHRU;
        /* Upstream PHY should operate in passthru mode */
        event = PhyEvent_ChangeToPassthru;
        break;
    case PhyEvent_ChangeToFiber:
        PHY_CTRL_FLAGS(pc) |= PHY_F_FIBER_MODE;
        PHY_CTRL_FLAGS(pc) |= PHY_F_PASSTHRU;
        break;
    default:
        break;
    }

    /* Call up the PHY chain */
    if (CDK_SUCCESS(rv)) {
        rv = PHY_NOTIFY(PHY_CTRL_NEXT(pc), event);
    }

    /* Update autoneg settings for upstream PHY */
    switch (event) {
    case PhyEvent_ChangeToFiber:
        /* Pass current autoneg setting if fiber mode */
        if (CDK_SUCCESS(rv)) {
            rv = PHY_AUTONEG_GET(pc, &autoneg);
        }
        if (CDK_SUCCESS(rv)) {
            rv = PHY_AUTONEG_SET(PHY_CTRL_NEXT(pc), autoneg);
        }
        break;
    case PhyEvent_ChangeToPassthru:
        /* Disable autoneg if passthru mode */
        if (CDK_SUCCESS(rv)) {
            rv = PHY_AUTONEG_SET(PHY_CTRL_NEXT(pc), 0);
        }
        break;
    default:
        break;
    }

    return rv;
}

/*
 * Function:
 *      bcm5464_phy_reset
 * Purpose:     
 *      Reset 5464 PHY
 * Parameters:
 *      pc - PHY control structure
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm5464_phy_reset(phy_ctrl_t *pc)
{
    int rv;

    rv = ge_phy_reset(pc);

    /* Call up the PHY chain */
    if (CDK_SUCCESS(rv)) {
        rv = PHY_RESET(PHY_CTRL_NEXT(pc));
    }

    return rv;
}

/*
 * Function:
 *      bcm5464_phy_init
 * Purpose:     
 *      Initialize 5464 PHY driver
 * Parameters:
 *      pc - PHY control structure
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm5464_phy_init(phy_ctrl_t *pc)
{
    uint32_t ctrl, next_abil, tmp, phyid1, sgmii;
    int ioerr = 0;
    int rv = CDK_E_NONE;

    PHY_CTRL_CHECK(pc);

    /* Set port mode */
    ioerr += PHY_BUS_READ(pc, MII_GB_CTRL_REG, &ctrl);
    ctrl |= MII_GB_CTRL_PT;
    ioerr += PHY_BUS_WRITE(pc, MII_GB_CTRL_REG, ctrl);

    /* Get upstream PHY abilities */
    next_abil = 0;
    PHY_ABILITY_GET(PHY_CTRL_NEXT(pc), &next_abil);

    /* Leave mode registers untouched if no SerDes */
    sgmii = 0;
    if (next_abil & PHY_ABIL_SERDES) {
        sgmii = 0x4;

        /* Select fiber registers */
        ioerr += PHY_BUS_WRITE(pc, 0x1c, REG_1C_WR(0x1f, sgmii | 0x01));
 
        /* Power up fiber interface if upstream PHY has SerDes support */
        ioerr += PHY_BUS_READ(pc, MII_CTRL_REG, &ctrl);
        if (next_abil & PHY_ABIL_SERDES) {
            ctrl &= ~MII_CTRL_PD;
        } else {
            ctrl |= MII_CTRL_PD;
        }
        if (next_abil & PHY_ABIL_AN_SGMII) {
            ctrl |= MII_CTRL_AE;
        } else {
            ctrl &= ~MII_CTRL_AE;
        }
        ioerr += PHY_BUS_WRITE(pc, MII_CTRL_REG, ctrl);

        /* Select copper registers */
        ioerr += PHY_BUS_WRITE(pc, 0x1c, REG_1C_WR(0x1f, sgmii));

        /* Power up copper interface */
        ioerr += PHY_BUS_READ(pc, MII_CTRL_REG, &ctrl);
        ctrl &= ~MII_CTRL_PD;
        ioerr += PHY_BUS_WRITE(pc, MII_CTRL_REG, ctrl);

        /* Configure auto-medium detect */
        ioerr += PHY_BUS_WRITE(pc, 0x1c, REG_1C_SEL(0x1e));
        ioerr += PHY_BUS_READ(pc, 0x1c, &tmp);
        tmp &= ~0x001f;
        ioerr += PHY_BUS_READ(pc, MII_PHY_ID1_REG, &phyid1);
        if (phyid1 != BCM5464_PHY_ID1) {
            /* Enable auto-medium detect if not rev A0 */
            tmp |= 0x0001;
        }
        tmp |= 0x0004; /* Prefer SerDes if no medium active */
        ioerr += PHY_BUS_WRITE(pc, 0x1c, REG_1C_WR(0x1e, tmp));
    }

    /* Disable carrier extension (to prevent Intel 7131 NIC CRC errors) */
    ioerr += PHY_BUS_WRITE(pc, 0x1c, REG_1C_SEL(0x1b));
    ioerr += PHY_BUS_READ(pc, 0x1c, &tmp);
    tmp |= 0x0040;
    ioerr += PHY_BUS_WRITE(pc, 0x1c, REG_1C_WR(0x1b, tmp));

    /* Configure Extended Control Register */
    ioerr += PHY_BUS_READ(pc, MII_ECR_REG, &tmp);
    /* Enable LEDs to indicate traffic status */
    tmp |= 0x0020;
    /* Set high FIFO elasticity to support jumbo frames */
    tmp |= 0x0001;
    ioerr += PHY_BUS_WRITE(pc, MII_ECR_REG, tmp);

    /* Enable extended packet length (4.5k through 25k) */
    ioerr += PHY_BUS_WRITE(pc, 0x18, 0x0007);
    ioerr += PHY_BUS_READ(pc, 0x18, &tmp);
    tmp |= 0x4000;
    ioerr += PHY_BUS_WRITE(pc, 0x18, tmp);

    /* Configure LED selectors */
    ioerr += PHY_BUS_WRITE(pc, 0x1c,
                           REG_1C_WR(0x0d, BCM5464_LED1_SEL(pc) |
                                     (BCM5464_LED2_SEL(pc) << 4)));
    ioerr += PHY_BUS_WRITE(pc, 0x1c,
                           REG_1C_WR(0x0e, BCM5464_LED3_SEL(pc) |
                                     (BCM5464_LED4_SEL(pc) << 4)));
    ioerr += PHY_BUS_WRITE(pc, 0x1c,
                           REG_1C_WR(0x09, BCM5464_LEDCTRL(pc)));
    ioerr += PHY_BUS_WRITE(pc, MII_EXP_SEL, MII_EXP_MAP_REG(0x4));
    ioerr += PHY_BUS_WRITE(pc, MII_EXP_REG, BCM5464_LEDSELECT(pc));
    ioerr += PHY_BUS_WRITE(pc, MII_EXP_SEL, MII_EXP_UNMAP);
    /* If using LED link/activity mode, disable LED traffic mode */
    if ((BCM5464_LEDCTRL(pc) & 0x10) || BCM5464_LEDSELECT(pc) == 0x01) {
        ioerr += PHY_BUS_READ(pc, MII_ECR_REG, &tmp);
        tmp &= ~0x0020;
        ioerr += PHY_BUS_WRITE(pc, MII_ECR_REG, tmp);
    }

    /* Call up the PHY chain */
    if (CDK_SUCCESS(rv)) {
        rv = PHY_INIT(PHY_CTRL_NEXT(pc));
    }

    /* Set default medium */
    if (CDK_SUCCESS(rv)) {
        PHY_NOTIFY(pc, PhyEvent_ChangeToCopper);
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:    
 *      bcm5464_phy_link_get
 * Purpose:     
 *      Determine the current link up/down status
 * Parameters:
 *      pc - PHY control structure
 *      link - (OUT) non-zero indicates link established.
 *      autoneg_done - (OUT) if true, auto-negotiation is complete
 * Returns:
 *      CDK_E_xxx
 * Notes:
 *      MII_STATUS bit 2 reflects link state.
 */
static int
bcm5464_phy_link_get(phy_ctrl_t *pc, int *link, int *autoneg_done)
{
    return ge_phy_link_get(pc, link, autoneg_done);
}

/*
 * Function:    
 *      bcm5464_phy_duplex_set
 * Purpose:     
 *      Set the current duplex mode (forced).
 * Parameters:
 *      pc - PHY control structure
 *      duplex - non-zero indicates full duplex, zero indicates half
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm5464_phy_duplex_set(phy_ctrl_t *pc, int duplex)
{
    int rv = CDK_E_NONE;

    PHY_CTRL_CHECK(pc);

    /* Call up the PHY chain */
    if (CDK_SUCCESS(rv)) {
        rv = PHY_DUPLEX_SET(PHY_CTRL_NEXT(pc), duplex);
    }

    if (CDK_SUCCESS(rv)) {
        if ((PHY_CTRL_FLAGS(pc) & PHY_F_FIBER_MODE) == 0) {
            rv = ge_phy_duplex_set(pc, duplex);
        }
    }

    return rv;
}

/*
 * Function:    
 *      bcm5464_phy_duplex_get
 * Purpose:     
 *      Get the current operating duplex mode. If autoneg is enabled, 
 *      then operating mode is returned, otherwise forced mode is returned.
 * Parameters:
 *      pc - PHY control structure
 *      duplex - (OUT) non-zero indicates full duplex, zero indicates half
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm5464_phy_duplex_get(phy_ctrl_t *pc, int *duplex)
{
    PHY_CTRL_CHECK(pc);

    if (PHY_CTRL_FLAGS(pc) & PHY_F_FIBER_MODE) {
        *duplex = TRUE;
        return CDK_E_NONE;
    }

    return ge_phy_duplex_get(pc, duplex);
}

/*
 * Function:    
 *      bcm5464_phy_speed_set
 * Purpose:     
 *      Set the current operating speed (forced).
 * Parameters:
 *      pc - PHY control structure
 *      speed - new link speed
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm5464_phy_speed_set(phy_ctrl_t *pc, uint32_t speed)
{
    int rv = CDK_E_NONE;

    PHY_CTRL_CHECK(pc);

    /* Call up the PHY chain */
    if (CDK_SUCCESS(rv)) {
        rv = PHY_SPEED_SET(PHY_CTRL_NEXT(pc), speed);
    }

    if (CDK_SUCCESS(rv)) {
        if ((PHY_CTRL_FLAGS(pc) & PHY_F_FIBER_MODE) == 0) {
            rv = ge_phy_speed_set(pc, speed);
        }
    }

    return rv;
}

/*
 * Function:    
 *      bcm5464_phy_speed_get
 * Purpose:     
 *      Get the current operating speed. If autoneg is enabled, 
 *      then operating mode is returned, otherwise forced mode is returned.
 * Parameters:
 *      pc - PHY control structure
 *      speed - (OUT) current link speed
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm5464_phy_speed_get(phy_ctrl_t *pc, uint32_t *speed)
{
    PHY_CTRL_CHECK(pc);

    if (PHY_CTRL_FLAGS(pc) & PHY_F_FIBER_MODE) {
        *speed = 1000;
        return CDK_E_NONE;
    }

    return ge_phy_speed_get(pc, speed);
}

/*
 * Function:    
 *      bcm5464_phy_autoneg_set
 * Purpose:     
 *      Enable or disabled auto-negotiation on the specified port.
 * Parameters:
 *      pc - PHY control structure
 *      autoneg - non-zero enables autoneg, zero disables autoneg
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm5464_phy_autoneg_set(phy_ctrl_t *pc, int autoneg)
{
    int rv = CDK_E_NONE;

    PHY_CTRL_CHECK(pc);

    /* Call up the PHY chain if passthru mode */
    if (PHY_CTRL_FLAGS(pc) & PHY_F_PASSTHRU) {
        rv = PHY_AUTONEG_SET(PHY_CTRL_NEXT(pc), autoneg);
    }

    if (CDK_SUCCESS(rv)) {
        rv = ge_phy_autoneg_set(pc, autoneg);
    }

    return rv;
}

/*
 * Function:    
 *      bcm5464_phy_autoneg_get
 * Purpose:     
 *      Get the current auto-negotiation status (enabled/busy)
 * Parameters:
 *      pc - PHY control structure
 *      autoneg - (OUT) non-zero indicates autoneg enabled
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm5464_phy_autoneg_get(phy_ctrl_t *pc, int *autoneg)
{
    return ge_phy_autoneg_get(pc, autoneg);
}

/*
 * Function:    
 *      bcm5464_phy_loopback_set
 * Purpose:     
 *      Set the internal PHY loopback mode.
 * Parameters:
 *      pc - PHY control structure
 *      enable - non-zero enables PHY loopback
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm5464_phy_loopback_set(phy_ctrl_t *pc, int enable)
{
    int ioerr = 0;
    int rv;
    uint32_t tmp;

    PHY_CTRL_CHECK(pc);

    if (PHY_CTRL_FLAGS(pc) & PHY_F_PASSTHRU) {
        return PHY_LOOPBACK_SET(PHY_CTRL_NEXT(pc), enable);
    }

    rv = ge_phy_loopback_set(pc, enable);

    /* Force link in loopback mode (required by some MACs) */
    ioerr += PHY_BUS_READ(pc, MII_TEST1_REG, &tmp);
    tmp &= ~0x1000;
    if (enable) {
        tmp |= 0x1000;
    }
    ioerr += PHY_BUS_WRITE(pc, MII_TEST1_REG, tmp);

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:    
 *      bcm5464_phy_loopback_get
 * Purpose:     
 *      Get the local PHY loopback mode.
 * Parameters:
 *      pc - PHY control structure
 *      enable - (OUT) non-zero indicates PHY loopback enabled
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm5464_phy_loopback_get(phy_ctrl_t *pc, int *enable)
{
    PHY_CTRL_CHECK(pc);

    if (PHY_CTRL_FLAGS(pc) & PHY_F_PASSTHRU) {
        return PHY_LOOPBACK_GET(PHY_CTRL_NEXT(pc), enable);
    }

    return ge_phy_loopback_get(pc, enable);
}

/*
 * Function:    
 *      bcm5464_phy_ability_get
 * Purpose:     
 *      Get the abilities of the PHY.
 * Parameters:
 *      pc - PHY control structure
 *      abil - (OUT) ability mask indicating supported options/speeds.
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm5464_phy_ability_get(phy_ctrl_t *pc, uint32_t *abil)
{
    PHY_CTRL_CHECK(pc);

    *abil = (PHY_ABIL_1000MB | PHY_ABIL_100MB | PHY_ABIL_10MB | 
             PHY_ABIL_LOOPBACK | PHY_ABIL_GMII);
    
    return CDK_E_NONE;
}

/*
 * Function:
 *      bcm5464_phy_config_set
 * Purpose:
 *      Modify PHY configuration value.
 * Parameters:
 *      pc - PHY control structure
 *      cfg - Configuration parameter
 *      val - Configuration value
 *      cd - Additional configuration data (if any)
 * Returns:
 *      CDK_E_xxx
 */
static int
bcm5464_phy_config_set(phy_ctrl_t *pc, phy_config_t cfg, uint32_t val, void *cd)
{
    int ioerr = 0;
    uint32_t next_abil;

    PHY_CTRL_CHECK(pc);

    switch (cfg) {
    case PhyConfig_Enable:
        return CDK_E_NONE;
    case PhyConfig_PortInterface:
        switch (val) {
        case PHY_IF_GMII:
        case PHY_IF_SGMII:
            return CDK_E_NONE;
        default:
            break;
        }
        break;
    case PhyConfig_RemoteLoopback:
        /* Get upstream PHY abilities */
        next_abil = 0;
        PHY_ABILITY_GET(PHY_CTRL_NEXT(pc), &next_abil);
        if (val) {
            /* Select copper mode and copper registers */
            if (next_abil & PHY_ABIL_SERDES) {
                ioerr += PHY_BUS_WRITE(pc, 0x1c, REG_1C_WR(0x1f, 0x00));
            }
            /* Enable remote loopback in register 0x18 shadow 100b */
            ioerr += PHY_BUS_WRITE(pc, 0x18, REG_18_WR(0x4, 0x8800));
        } else {
            /* Select SGMII mode if SerDes */
            if (next_abil & PHY_ABIL_SERDES) {
                ioerr += PHY_BUS_WRITE(pc, 0x1c, REG_1C_WR(0x1f, 0x04));
            }
            /* Disable remote loopback in register 0x18 shadow 100b */
            ioerr += PHY_BUS_WRITE(pc, 0x18, REG_18_WR(0x4, 0));
        }
        return ioerr ? CDK_E_IO : CDK_E_NONE;
    default:
        break;
    }

    return CDK_E_UNAVAIL;
}

/*
 * Function:
 *      bcm5464_phy_config_get
 * Purpose:
 *      Get PHY configuration value.
 * Parameters:
 *      pc - PHY control structure
 *      cfg - Configuration parameter
 *      val - (OUT) Configuration value
 *      cd - (OUT) Additional configuration data (if any)
 * Returns:
 *      CDK_E_xxx
 */
static int
bcm5464_phy_config_get(phy_ctrl_t *pc, phy_config_t cfg, uint32_t *val, void *cd)
{
    int ioerr = 0;
    uint32_t misc_test;

    PHY_CTRL_CHECK(pc);

    switch (cfg) {
    case PhyConfig_Enable:
        *val = 1;
        return CDK_E_NONE;
    case PhyConfig_PortInterface:
        *val = PHY_IF_GMII;
        return CDK_E_NONE;
    case PhyConfig_RemoteLoopback:
        /* Read remote loopback from register 0x18 shadow 100b */
        ioerr += PHY_BUS_WRITE(pc, 0x18, REG_18_SEL(0x4));
        ioerr += PHY_BUS_READ(pc, 0x18, &misc_test);
        *val = (misc_test & 0x8000) ? 1 : 0;
        return ioerr ? CDK_E_IO : CDK_E_NONE;
    default:
        break;
    }

    return CDK_E_UNAVAIL;
}

/*
 * Variable:    bcm5464_phy drv
 * Purpose:     PHY Driver for BCM5464.
 */
phy_driver_t bcm5464_drv = {
    "bcm5464",
    "BCM5464 Gigabit PHY Driver",  
    0,
    bcm5464_phy_probe,                  /* pd_probe */
    bcm5464_phy_notify,                 /* pd_notify */
    bcm5464_phy_reset,                  /* pd_reset */
    bcm5464_phy_init,                   /* pd_init */
    bcm5464_phy_link_get,               /* pd_link_get */
    bcm5464_phy_duplex_set,             /* pd_duplex_set */
    bcm5464_phy_duplex_get,             /* pd_duplex_get */
    bcm5464_phy_speed_set,              /* pd_speed_set */
    bcm5464_phy_speed_get,              /* pd_speed_get */
    bcm5464_phy_autoneg_set,            /* pd_autoneg_set */
    bcm5464_phy_autoneg_get,            /* pd_autoneg_get */
    bcm5464_phy_loopback_set,           /* pd_loopback_set */
    bcm5464_phy_loopback_get,           /* pd_loopback_get */
    bcm5464_phy_ability_get,            /* pd_ability_get */
    bcm5464_phy_config_set,             /* pd_config_set */
    bcm5464_phy_config_get,             /* pd_config_get */
    NULL,                               /* pd_status_get */
    NULL                                /* pd_cable_diag */
};
