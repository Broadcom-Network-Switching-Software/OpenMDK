/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * PHY driver for BCM54292.
 */

#include <phy/phy.h>
#include <phy/ge_phy.h>
#include <phy/phy_drvlist.h>
#include <phy/chip/bcm54282_defs.h>

#define BCM54292_PHY_ID0                0x600d
#define BCM54292_PHY_ID1                0x8530

#define PHY_ID1_REV_MASK                0x000f

/* Lane from PHY control instance */
#define LANE_NUM_MASK                   0x7

/***********************************************************************
 *
 * PHY DRIVER FUNCTIONS
 */

#if PHY_CONFIG_INCLUDE_CHIP_SYMBOLS == 1
extern cdk_symbols_t bcm54282_symbols;
#endif

/*
 * Function:
 *      bcm54292_phy_probe
 * Purpose:     
 *      Probe for PHY
 * Parameters:
 *      pc - PHY control structure
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm54292_phy_probe(phy_ctrl_t *pc)
{
    uint32_t phyid0, phyid1;
    int ioerr = 0;

    PHY_CTRL_CHECK(pc);

    ioerr += PHY_BUS_READ(pc, MII_PHY_ID0_REG, &phyid0);
    ioerr += PHY_BUS_READ(pc, MII_PHY_ID1_REG, &phyid1);
    if (phyid0 == BCM54292_PHY_ID0 && 
        (phyid1 & ~PHY_ID1_REV_MASK) == BCM54292_PHY_ID1) {
#if PHY_CONFIG_INCLUDE_CHIP_SYMBOLS == 1
        PHY_CTRL_SYMBOLS(pc) = &bcm54282_symbols;
#endif

        /* Default is copper mode */
        PHY_CTRL_FLAGS(pc) &= ~PHY_F_FIBER_MODE;

        return ioerr ? CDK_E_IO : CDK_E_NONE;
    }
    return CDK_E_NOT_FOUND;
}

/*
 * Function:
 *      bcm54292_phy_notify
 * Purpose:     
 *      Handle PHY notifications
 * Parameters:
 *      pc - PHY control structure
 *      event - PHY event
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm54292_phy_notify(phy_ctrl_t *pc, phy_event_t event)
{
    return bcm54282_drv.pd_notify(pc, event);
}

/*
 * Function:
 *      bcm54292_phy_reset
 * Purpose:     
 *      Reset PHY
 * Parameters:
 *      pc - PHY control structure
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm54292_phy_reset(phy_ctrl_t *pc)
{
    return bcm54282_drv.pd_reset(pc);
}

/*
 * Function:
 *      bcm54292_phy_init
 * Purpose:     
 *      Initialize PHY driver
 * Parameters:
 *      pc - PHY control structure
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm54292_phy_init(phy_ctrl_t *pc)
{
    return bcm54282_drv.pd_init(pc);
}

/*
 * Function:    
 *      bcm54292_phy_link_get
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
bcm54292_phy_link_get(phy_ctrl_t *pc, int *link, int *autoneg_done)
{
    return bcm54282_drv.pd_link_get(pc, link, autoneg_done);
}

/*
 * Function:    
 *      bcm54292_phy_duplex_set
 * Purpose:     
 *      Set the current duplex mode (forced).
 * Parameters:
 *      pc - PHY control structure
 *      duplex - non-zero indicates full duplex, zero indicates half
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm54292_phy_duplex_set(phy_ctrl_t *pc, int duplex)
{
    return bcm54282_drv.pd_duplex_set(pc, duplex);
}

/*
 * Function:    
 *      bcm54292_phy_duplex_get
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
bcm54292_phy_duplex_get(phy_ctrl_t *pc, int *duplex)
{
    return bcm54282_drv.pd_duplex_get(pc, duplex);
}

/*
 * Function:    
 *      bcm54292_phy_speed_set
 * Purpose:     
 *      Set the current operating speed (forced).
 * Parameters:
 *      pc - PHY control structure
 *      speed - new link speed
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm54292_phy_speed_set(phy_ctrl_t *pc, uint32_t speed)
{
    return bcm54282_drv.pd_speed_set(pc, speed);
}

/*
 * Function:    
 *      bcm54292_phy_speed_get
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
bcm54292_phy_speed_get(phy_ctrl_t *pc, uint32_t *speed)
{
    return bcm54282_drv.pd_speed_get(pc, speed);
}

/*
 * Function:    
 *      bcm54292_phy_autoneg_set
 * Purpose:     
 *      Enable or disabled auto-negotiation on the specified port.
 * Parameters:
 *      pc - PHY control structure
 *      autoneg - non-zero enables autoneg, zero disables autoneg
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm54292_phy_autoneg_set(phy_ctrl_t *pc, int autoneg)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t next_abil;
    SGMII_SLAVEr_t sgmii_slave;
    MII_1000X_CTRLr_t mii_1000x_ctrl;

    PHY_CTRL_CHECK(pc);
    
    /* Get upstream PHY abilities */
    next_abil = 0;
    PHY_ABILITY_GET(PHY_CTRL_NEXT(pc), &next_abil);
    if (next_abil & PHY_ABIL_AN) {
            /* Call up the PHY chain, set autoneg to '1' to fit
        bcm54292 default system side setting */
        if (CDK_SUCCESS(rv)) {
            rv = PHY_AUTONEG_SET(PHY_CTRL_NEXT(pc), 1);
        }
    }

    if (autoneg) {
        autoneg = 1;
    }

    if (PHY_CTRL_FLAGS(pc) & PHY_F_FIBER_MODE) {
        /*
         * When enabling auto-neg, set the default speed to 1000Mbps first.
         * PHY will not enable auto-neg if the PHY is in 100FX mode.
         */
        if (autoneg) {
            rv = PHY_SPEED_SET(pc, 1000);
            if (CDK_FAILURE(rv)) {
                return rv;
            }
        }

        /* Set auto-detection between SGMII-slave and 1000BASE-X */
        ioerr += READ_SGMII_SLAVEr(pc, &sgmii_slave);
        SGMII_SLAVEr_SGMII_SLAVE_AUTO_DETECTf_SET(sgmii_slave, autoneg);
        ioerr += WRITE_SGMII_SLAVEr(pc, sgmii_slave);

        /* Set auto-neg mode */
        ioerr += READ_MII_1000X_CTRLr(pc, &mii_1000x_ctrl);
        MII_1000X_CTRLr_AUTONEGf_SET(mii_1000x_ctrl, autoneg);
        MII_1000X_CTRLr_RESTART_ANf_SET(mii_1000x_ctrl, autoneg);
        ioerr += WRITE_MII_1000X_CTRLr(pc, mii_1000x_ctrl);
    } else {
        rv = ge_phy_autoneg_set(pc, autoneg);
    }
    
    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:    
 *      bcm54292_phy_autoneg_get
 * Purpose:     
 *      Get the current auto-negotiation status (enabled/busy)
 * Parameters:
 *      pc - PHY control structure
 *      autoneg - (OUT) non-zero indicates autoneg enabled
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm54292_phy_autoneg_get(phy_ctrl_t *pc, int *autoneg)
{
    return bcm54282_drv.pd_autoneg_get(pc, autoneg);
}

/*
 * Function:    
 *      bcm54292_phy_loopback_set
 * Purpose:     
 *      Set the internal PHY loopback mode.
 * Parameters:
 *      pc - PHY control structure
 *      enable - non-zero enables PHY loopback
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm54292_phy_loopback_set(phy_ctrl_t *pc, int enable)
{
    return bcm54282_drv.pd_loopback_set(pc, enable);
}

/*
 * Function:    
 *      bcm54292_phy_loopback_get
 * Purpose:     
 *      Get the local PHY loopback mode.
 * Parameters:
 *      pc - PHY control structure
 *      enable - (OUT) non-zero indicates PHY loopback enabled
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm54292_phy_loopback_get(phy_ctrl_t *pc, int *enable)
{
    return bcm54282_drv.pd_loopback_get(pc, enable);
}

/*
 * Function:    
 *      bcm54292_phy_ability_get
 * Purpose:     
 *      Get the abilities of the PHY.
 * Parameters:
 *      pc - PHY control structure
 *      abil - (OUT) ability mask indicating supported options/speeds.
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm54292_phy_ability_get(phy_ctrl_t *pc, uint32_t *abil)
{
    return bcm54282_drv.pd_ability_get(pc, abil);
}

/*
 * Function:
 *      bcm54292_phy_config_set
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
bcm54292_phy_config_set(phy_ctrl_t *pc, phy_config_t cfg, uint32_t val, void *cd)
{
    return bcm54282_drv.pd_config_set(pc, cfg, val, cd);
}

/*
 * Function:
 *      bcm54292_phy_config_get
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
bcm54292_phy_config_get(phy_ctrl_t *pc, phy_config_t cfg, uint32_t *val, void *cd)
{
    return bcm54282_drv.pd_config_get(pc, cfg, val, cd);
}

/*
 * Function:
 *      bcm54292_phy_status_get
 * Purpose:
 *      Get PHY status value.
 * Parameters:
 *      pc - PHY control structure
 *      st - Status parameter
 *      val - (OUT) Status value
 * Returns:
 *      CDK_E_xxx
 */
static int
bcm54292_phy_status_get(phy_ctrl_t *pc, phy_status_t st, uint32_t *val)
{
    return bcm54282_drv.pd_status_get(pc, st, val);
}

/*
 * Variable:    bcm54292_phy drv
 * Purpose:     PHY Driver for BCM54292.
 */
phy_driver_t bcm54292_drv = {
    "bcm54292",
    "BCM54292 Gigabit PHY Driver",  
    0,
    bcm54292_phy_probe,                 /* pd_probe */
    bcm54292_phy_notify,                /* pd_notify */
    bcm54292_phy_reset,                 /* pd_reset */
    bcm54292_phy_init,                  /* pd_init */
    bcm54292_phy_link_get,              /* pd_link_get */
    bcm54292_phy_duplex_set,            /* pd_duplex_set */
    bcm54292_phy_duplex_get,            /* pd_duplex_get */
    bcm54292_phy_speed_set,             /* pd_speed_set */
    bcm54292_phy_speed_get,             /* pd_speed_get */
    bcm54292_phy_autoneg_set,           /* pd_autoneg_set */
    bcm54292_phy_autoneg_get,           /* pd_autoneg_get */
    bcm54292_phy_loopback_set,          /* pd_loopback_set */
    bcm54292_phy_loopback_get,          /* pd_loopback_get */
    bcm54292_phy_ability_get,           /* pd_ability_get */
    bcm54292_phy_config_set,            /* pd_config_set */
    bcm54292_phy_config_get,            /* pd_config_get */
    bcm54292_phy_status_get,            /* pd_status_get */
    NULL                                /* pd_cable_diag */
};
