/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * PHY driver for BCM54684.
 */

#include <phy/phy.h>
#include <phy/ge_phy.h>

#define BCM54684_PHY_ID0                0x0143
#define BCM54684_PHY_ID1                0xbef0

#define PHY_ID1_REV_MASK                0x000f

/* Default LED control */
#define BCM54684_LED1_SEL(_pc)          0x0
#define BCM54684_LED2_SEL(_pc)          0x1
#define BCM54684_LED3_SEL(_pc)          0x3
#define BCM54684_LED4_SEL(_pc)          0x6
#define BCM54684_LEDCTRL(_pc)           0x8
#define BCM54684_LEDSELECT(_pc)         0x0

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

/***********************************************************************
 *
 * PHY DRIVER FUNCTIONS
 */

#if PHY_CONFIG_INCLUDE_CHIP_SYMBOLS == 1
extern cdk_symbols_t bcm54684_symbols;
#endif

/*
 * Function:
 *      bcm54684_phy_probe
 * Purpose:     
 *      Probe for PHY
 * Parameters:
 *      pc - PHY control structure
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm54684_phy_probe(phy_ctrl_t *pc)
{
    uint32_t phyid0, phyid1;
    int ioerr = 0;

    PHY_CTRL_CHECK(pc);

    ioerr += PHY_BUS_READ(pc, MII_PHY_ID0_REG, &phyid0);
    ioerr += PHY_BUS_READ(pc, MII_PHY_ID1_REG, &phyid1);

    if (phyid0 == BCM54684_PHY_ID0 && 
        (phyid1 & ~PHY_ID1_REV_MASK) == BCM54684_PHY_ID1) {

#if PHY_CONFIG_INCLUDE_CHIP_SYMBOLS == 1
        PHY_CTRL_SYMBOLS(pc) = &bcm54684_symbols;
#endif
        return ioerr ? CDK_E_IO : CDK_E_NONE;
    }
    return CDK_E_NOT_FOUND;
}

/*
 * Function:
 *      bcm54684_phy_notify
 * Purpose:     
 *      Handle PHY notifications
 * Parameters:
 *      pc - PHY control structure
 *      event - PHY event
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm54684_phy_notify(phy_ctrl_t *pc, phy_event_t event)
{
    int rv = CDK_E_NONE;

    PHY_CTRL_CHECK(pc);

    switch (event) {
    case PhyEvent_ChangeToCopper:
        event = PhyEvent_ChangeToPassthru;
        break;
    default:
        break;
    }

    /* Call up the PHY chain */
    if (CDK_SUCCESS(rv)) {
        rv = PHY_NOTIFY(PHY_CTRL_NEXT(pc), event);
    }

    return rv;
}

/*
 * Function:
 *      bcm54684_phy_reset
 * Purpose:     
 *      Reset PHY
 * Parameters:
 *      pc - PHY control structure
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm54684_phy_reset(phy_ctrl_t *pc)
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
 *      bcm54684_phy_init
 * Purpose:     
 *      Initialize PHY driver
 * Parameters:
 *      pc - PHY control structure
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm54684_phy_init(phy_ctrl_t *pc)
{
    uint32_t ctrl, tmp;
    int ioerr = 0;
    int rv = CDK_E_NONE;

    PHY_CTRL_CHECK(pc);

    /* Reset PHY */
    if (CDK_SUCCESS(rv)) {
        rv =  PHY_RESET(pc);
    }

    /* Select copper registers and QSGMII copper only */
    ioerr += PHY_BUS_WRITE(pc, 0x1c, REG_1C_WR(0x1f, 0));

    /* Power up copper interface */
    ioerr += PHY_BUS_READ(pc, MII_CTRL_REG, &ctrl);
    ctrl &= ~MII_CTRL_PD;
    ioerr += PHY_BUS_WRITE(pc, MII_CTRL_REG, ctrl);

    /* Set port mode */
    ioerr += PHY_BUS_READ(pc, MII_GB_CTRL_REG, &ctrl);
    ctrl |= MII_GB_CTRL_PT;
    ioerr += PHY_BUS_WRITE(pc, MII_GB_CTRL_REG, ctrl);

    /* Enable link speed LED mode */
    ioerr += PHY_BUS_WRITE(pc, 0x1c, REG_1C_SEL(0x2));
    ioerr += PHY_BUS_READ(pc, 0x1c, &tmp);
    tmp |= 0x0004;
    ioerr += PHY_BUS_WRITE(pc, 0x1c, REG_1C_WR(0x2, tmp));

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
                           REG_1C_WR(0x0d, BCM54684_LED1_SEL(pc) |
                                     (BCM54684_LED2_SEL(pc) << 4)));
    ioerr += PHY_BUS_WRITE(pc, 0x1c,
                           REG_1C_WR(0x0e, BCM54684_LED3_SEL(pc) |
                                     (BCM54684_LED4_SEL(pc) << 4)));
    ioerr += PHY_BUS_WRITE(pc, 0x1c,
                           REG_1C_WR(0x09, BCM54684_LEDCTRL(pc)));
    ioerr += PHY_BUS_WRITE(pc, MII_EXP_SEL, MII_EXP_MAP_REG(0x4));
    ioerr += PHY_BUS_WRITE(pc, MII_EXP_REG, BCM54684_LEDSELECT(pc));
    ioerr += PHY_BUS_WRITE(pc, MII_EXP_SEL, MII_EXP_UNMAP);
    /* If using LED link/activity mode, disable LED traffic mode */
    if ((BCM54684_LEDCTRL(pc) & 0x10) || BCM54684_LEDSELECT(pc) == 0x01) {
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
 *      bcm54684_phy_link_get
 * Purpose:     
 *      Determine the current link up/down status
 * Parameters:
 *      pc - PHY control structure
 *      link - (OUT) non-zero indicates link established.
 *      autoneg_done - (OUT) if true, auto-negotiation is complete
 * Returns:
 *      CDK_E_xxx
 */
static int
bcm54684_phy_link_get(phy_ctrl_t *pc, int *link, int *autoneg_done)
{
    return ge_phy_link_get(pc, link, autoneg_done);
}

/*
 * Function:    
 *      bcm54684_phy_duplex_set
 * Purpose:     
 *      Set the current duplex mode (forced).
 * Parameters:
 *      pc - PHY control structure
 *      duplex - non-zero indicates full duplex, zero indicates half
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm54684_phy_duplex_set(phy_ctrl_t *pc, int duplex)
{
    int rv = CDK_E_NONE;

    PHY_CTRL_CHECK(pc);

    /* Call up the PHY chain */
    if (CDK_SUCCESS(rv)) {
        rv = PHY_DUPLEX_SET(PHY_CTRL_NEXT(pc), duplex);
    }

    if (CDK_SUCCESS(rv)) {
        rv = ge_phy_duplex_set(pc, duplex);
    }

    return rv;
}

/*
 * Function:    
 *      bcm54684_phy_duplex_get
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
bcm54684_phy_duplex_get(phy_ctrl_t *pc, int *duplex)
{
    return ge_phy_duplex_get(pc, duplex);
}

/*
 * Function:    
 *      bcm54684_phy_speed_set
 * Purpose:     
 *      Set the current operating speed (forced).
 * Parameters:
 *      pc - PHY control structure
 *      speed - new link speed
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm54684_phy_speed_set(phy_ctrl_t *pc, uint32_t speed)
{
    int rv = CDK_E_NONE;

    PHY_CTRL_CHECK(pc);

    /* Call up the PHY chain */
    if (CDK_SUCCESS(rv)) {
        rv = PHY_SPEED_SET(PHY_CTRL_NEXT(pc), speed);
    }

    if (CDK_SUCCESS(rv)) {
        rv = ge_phy_speed_set(pc, speed);
    }

    return rv;
}

/*
 * Function:    
 *      bcm54684_phy_speed_get
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
bcm54684_phy_speed_get(phy_ctrl_t *pc, uint32_t *speed)
{
    return ge_phy_speed_get(pc, speed);
}

/*
 * Function:    
 *      bcm54684_phy_autoneg_set
 * Purpose:     
 *      Enable or disabled auto-negotiation on the specified port.
 * Parameters:
 *      pc - PHY control structure
 *      autoneg - non-zero enables autoneg, zero disables autoneg
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm54684_phy_autoneg_set(phy_ctrl_t *pc, int autoneg)
{
    return ge_phy_autoneg_set(pc, autoneg);
}

/*
 * Function:    
 *      bcm54684_phy_autoneg_get
 * Purpose:     
 *      Get the current auto-negotiation status (enabled/busy)
 * Parameters:
 *      pc - PHY control structure
 *      autoneg - (OUT) non-zero indicates autoneg enabled
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm54684_phy_autoneg_get(phy_ctrl_t *pc, int *autoneg)
{
    return ge_phy_autoneg_get(pc, autoneg);
}

/*
 * Function:    
 *      bcm54684_phy_loopback_set
 * Purpose:     
 *      Set the internal PHY loopback mode.
 * Parameters:
 *      pc - PHY control structure
 *      enable - non-zero enables PHY loopback
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm54684_phy_loopback_set(phy_ctrl_t *pc, int enable)
{
    return ge_phy_loopback_set(pc, enable);
}

/*
 * Function:    
 *      bcm54684_phy_loopback_get
 * Purpose:     
 *      Get the local PHY loopback mode.
 * Parameters:
 *      pc - PHY control structure
 *      enable - (OUT) non-zero indicates PHY loopback enabled
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm54684_phy_loopback_get(phy_ctrl_t *pc, int *enable)
{
    return ge_phy_loopback_get(pc, enable);
}

/*
 * Function:    
 *      bcm54684_phy_ability_get
 * Purpose:     
 *      Get the abilities of the PHY.
 * Parameters:
 *      pc - PHY control structure
 *      abil - (OUT) ability mask indicating supported options/speeds.
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm54684_phy_ability_get(phy_ctrl_t *pc, uint32_t *abil)
{
    PHY_CTRL_CHECK(pc);

    *abil = (PHY_ABIL_1000MB | PHY_ABIL_100MB | PHY_ABIL_10MB | 
             PHY_ABIL_LOOPBACK | PHY_ABIL_SGMII);
    
    return CDK_E_NONE;
}

/*
 * Function:
 *      bcm54684_phy_config_set
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
bcm54684_phy_config_set(phy_ctrl_t *pc, phy_config_t cfg, uint32_t val, void *cd)
{
    int ioerr = 0;
    uint32_t sgmii_lb, ana, gb_ctrl, ctrl;

    PHY_CTRL_CHECK(pc);

    switch (cfg) {
    case PhyConfig_Enable:
        return CDK_E_NONE;
    case PhyConfig_PortInterface:
        switch (val) {
        case PHY_IF_SGMII:
            return CDK_E_NONE;
        default:
            break;
        }
        break;
    case PhyConfig_RemoteLoopback:
        /* Read SGMII loopback setting from expansion register 0x44 */
        ioerr += PHY_BUS_WRITE(pc, 0x17, 0x0f44);
        ioerr += PHY_BUS_READ(pc, 0x15, &sgmii_lb);
        /* Leave autoneg untouched if disable and already disabled */
        if (!val && !sgmii_lb) {
            return ioerr ? CDK_E_IO : CDK_E_NONE;
        }
        /* Save current configuration */
        ioerr += PHY_BUS_READ(pc, MII_ANA_REG, &ana);
        ioerr += PHY_BUS_READ(pc, MII_GB_CTRL_REG, &gb_ctrl);
        ioerr += PHY_BUS_READ(pc, MII_CTRL_REG, &ctrl);
        /* Force link down by doing autoneg with no abilities */
        ioerr += PHY_BUS_WRITE(pc, MII_ANA_REG, 0x0001);
        ioerr += PHY_BUS_WRITE(pc, MII_GB_CTRL_REG, 0x0000);
        ioerr += PHY_BUS_WRITE(pc, MII_CTRL_REG, MII_CTRL_AE | MII_CTRL_RAN);
        /* Set SGMII loopback with Rx suppress in expansion register 0x44 */
        ioerr += PHY_BUS_WRITE(pc, 0x15, val ? 0x000b : 0x0000);
        /* Restore configuration and restart autoneg */
        ioerr += PHY_BUS_WRITE(pc, MII_ANA_REG, ana);
        ioerr += PHY_BUS_WRITE(pc, MII_GB_CTRL_REG, gb_ctrl);
        ioerr += PHY_BUS_WRITE(pc, MII_CTRL_REG, ctrl | MII_CTRL_RAN);
        return ioerr ? CDK_E_IO : CDK_E_NONE;
    default:
        break;
    }

    return CDK_E_UNAVAIL;
}

/*
 * Function:
 *      bcm54684_phy_config_get
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
bcm54684_phy_config_get(phy_ctrl_t *pc, phy_config_t cfg, uint32_t *val, void *cd)
{
    int ioerr = 0;
    uint32_t sgmii_lb;

    PHY_CTRL_CHECK(pc);

    switch (cfg) {
    case PhyConfig_Enable:
        *val = 1;
        return CDK_E_NONE;
    case PhyConfig_PortInterface:
        *val = PHY_IF_GMII;
        return CDK_E_NONE;
    case PhyConfig_RemoteLoopback:
        /* Read SGMII loopback setting from expansion register 0x44 */
        ioerr += PHY_BUS_WRITE(pc, 0x17, 0x0f44);
        ioerr += PHY_BUS_READ(pc, 0x15, &sgmii_lb);
        *val = (sgmii_lb) ? 1 : 0;
        return ioerr ? CDK_E_IO : CDK_E_NONE;
    default:
        break;
    }

    return CDK_E_UNAVAIL;
}

/*
 * Variable:    bcm54684_phy drv
 * Purpose:     PHY Driver for BCM54684.
 */
phy_driver_t bcm54684_drv = {
    "bcm54684",
    "BCM54684 Gigabit PHY Driver",  
    0,
    bcm54684_phy_probe,                 /* pd_probe */
    bcm54684_phy_notify,                /* pd_notify */
    bcm54684_phy_reset,                 /* pd_reset */
    bcm54684_phy_init,                  /* pd_init */
    bcm54684_phy_link_get,              /* pd_link_get */
    bcm54684_phy_duplex_set,            /* pd_duplex_set */
    bcm54684_phy_duplex_get,            /* pd_duplex_get */
    bcm54684_phy_speed_set,             /* pd_speed_set */
    bcm54684_phy_speed_get,             /* pd_speed_get */
    bcm54684_phy_autoneg_set,           /* pd_autoneg_set */
    bcm54684_phy_autoneg_get,           /* pd_autoneg_get */
    bcm54684_phy_loopback_set,          /* pd_loopback_set */
    bcm54684_phy_loopback_get,          /* pd_loopback_get */
    bcm54684_phy_ability_get,           /* pd_ability_get */
    bcm54684_phy_config_set,            /* pd_config_set */
    bcm54684_phy_config_get,            /* pd_config_get */
    NULL,                               /* pd_status_get */
    NULL                                /* pd_cable_diag */
};
