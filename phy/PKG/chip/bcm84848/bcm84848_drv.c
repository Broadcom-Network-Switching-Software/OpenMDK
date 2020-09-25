/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * PHY driver for BCM84848.
 */

#include <phy/phy.h>
#include <phy/phy_drvlist.h>
#include <phy/ge_phy.h>

/* Private PHY flag is used to indicate firmware download method */
#define PHY_F_FW_MDIO_LOAD                      PHY_F_PRIVATE

#define PHY_ID1_REV_MASK                        0x000f

#define BCM84848_PMA_PMD_ID0                    0x600d
#define BCM84848_PMA_PMD_ID1                    0x84f0

#define BCM8484X_C45_DEV_PMA_PMD                MII_C45_DEV_PMA_PMD
#define BCM8484X_C45_DEV_PCS                    MII_C45_DEV_PCS
#define BCM8484X_C45_DEV_AN                     MII_C45_DEV_AN
#define BCM8484X_C45_DEV_PHYXS_M                MII_C45_DEV_PHY_XS
#define BCM8484X_C45_DEV_PHYXS_L                0x03
#define BCM8484X_C45_DEV_TOPLVL                 0x1e

#define C45_DEVAD(_a)                           LSHIFT32((_a),16)
#define DEVAD_PMA_PMD                           C45_DEVAD(BCM8484X_C45_DEV_PMA_PMD)
#define DEVAD_PCS                               C45_DEVAD(BCM8484X_C45_DEV_PCS)
#define DEVAD_AN                                C45_DEVAD(BCM8484X_C45_DEV_AN)
#define DEVAD_PHY_XS_M                          C45_DEVAD(BCM8484X_C45_DEV_PHYXS_M)
#define DEVAD_PHY_XS_L                          C45_DEVAD(BCM8484X_C45_DEV_PHYXS_L)
#define DEVAD_TOPLVL                            C45_DEVAD(BCM8484X_C45_DEV_TOPLVL)

/***************************************
 * Top level registers 
 */
#define PMA_PMD_ID0_REG                         (DEVAD_PMA_PMD + MII_PHY_ID0_REG)
#define PMA_PMD_ID1_REG                         (DEVAD_PMA_PMD + MII_PHY_ID1_REG)


/***********************************************************************
 *
 * PHY DRIVER FUNCTIONS
 */

/*
 * Function:
 *      bcm84848_phy_probe
 * Purpose:     
 *      Probe for 84848 PHY
 * Parameters:
 *      pc - PHY control structure
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm84848_phy_probe(phy_ctrl_t *pc)
{
    int ioerr = 0;
    uint32_t phyid0, phyid1;

    PHY_CTRL_CHECK(pc);

    ioerr += PHY_BUS_READ(pc, PMA_PMD_ID0_REG, &phyid0);
    ioerr += PHY_BUS_READ(pc, PMA_PMD_ID1_REG, &phyid1);
    if (ioerr) {
        return CDK_E_IO;
    }

    if ((phyid0 == BCM84848_PMA_PMD_ID0) &&
        ((phyid1 & ~PHY_ID1_REV_MASK) == BCM84848_PMA_PMD_ID1)) {
#if PHY_CONFIG_EXTERNAL_BOOT_ROM == 0
        /* Use MDIO download if external ROM is disabled */
        PHY_CTRL_FLAGS(pc) |= PHY_F_FW_MDIO_LOAD;
#endif
        return ioerr ? CDK_E_IO : CDK_E_NONE;
    }
    return CDK_E_NOT_FOUND;
}

/*
 * Function:
 *      bcm84848_phy_notify
 * Purpose:     
 *      Handle PHY notifications
 * Parameters:
 *      pc - PHY control structure
 *      event - PHY event
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm84848_phy_notify(phy_ctrl_t *pc, phy_event_t event)
{
    PHY_CTRL_CHECK(pc);

    return bcm84846_drv.pd_notify(pc, event);
}

/*
 * Function:
 *      bcm84848_phy_reset
 * Purpose:     
 *      Reset 84848 PHY
 * Parameters:
 *      pc - PHY control structure
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm84848_phy_reset(phy_ctrl_t *pc)
{
    PHY_CTRL_CHECK(pc);

    return bcm84846_drv.pd_reset(pc);
}

/*
 * Function:
 *      bcm84848_phy_init
 * Purpose:     
 *      Initialize 84848 PHY driver
 * Parameters:
 *      pc - PHY control structure
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm84848_phy_init(phy_ctrl_t *pc)
{
    PHY_CTRL_CHECK(pc);

    return bcm84846_drv.pd_init(pc);
}

/*
 * Function:    
 *      bcm84848_phy_link_get
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
bcm84848_phy_link_get(phy_ctrl_t *pc, int *link, int *autoneg_done)
{
    PHY_CTRL_CHECK(pc);

    return bcm84846_drv.pd_link_get(pc, link, autoneg_done);
}

/*
 * Function:    
 *      bcm84848_phy_duplex_set
 * Purpose:     
 *      Set the current duplex mode (forced).
 * Parameters:
 *      pc - PHY control structure
 *      duplex - non-zero indicates full duplex, zero indicates half
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm84848_phy_duplex_set(phy_ctrl_t *pc, int duplex)
{
    PHY_CTRL_CHECK(pc);

    return bcm84846_drv.pd_duplex_set(pc, duplex);
}

/*
 * Function:    
 *      bcm84848_phy_duplex_get
 * Purpose:     
 *      Get the current operating duplex mode.
 * Parameters:
 *      pc - PHY control structure
 *      duplex - (OUT) non-zero indicates full duplex, zero indicates half
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm84848_phy_duplex_get(phy_ctrl_t *pc, int *duplex)
{
    PHY_CTRL_CHECK(pc);

    return bcm84846_drv.pd_duplex_get(pc, duplex);
}

/*
 * Function:    
 *      bcm84848_phy_speed_set
 * Purpose:     
 *      Set the current operating speed (forced).
 * Parameters:
 *      pc - PHY control structure
 *      speed - new link speed
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm84848_phy_speed_set(phy_ctrl_t *pc, uint32_t speed)
{
    PHY_CTRL_CHECK(pc);

    return bcm84846_drv.pd_speed_set(pc, speed);
}

/*
 * Function:    
 *      bcm84848_phy_speed_get
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
bcm84848_phy_speed_get(phy_ctrl_t *pc, uint32_t *speed)
{
    PHY_CTRL_CHECK(pc);

    return bcm84846_drv.pd_speed_get(pc, speed);
}

/*
 * Function:    
 *      bcm84848_phy_autoneg_set
 * Purpose:     
 *      Enable or disable auto-negotiation on the specified port.
 * Parameters:
 *      pc - PHY control structure
 *      autoneg - non-zero enables autoneg, zero disables autoneg
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm84848_phy_autoneg_set(phy_ctrl_t *pc, int autoneg)
{
    PHY_CTRL_CHECK(pc);

    return bcm84846_drv.pd_autoneg_set(pc, autoneg);
}

/*
 * Function:    
 *      bcm84848_phy_autoneg_get
 * Purpose:     
 *      Get the current auto-negotiation setting.
 * Parameters:
 *      pc - PHY control structure
 *      autoneg - (OUT) non-zero indicates autoneg enabled
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm84848_phy_autoneg_get(phy_ctrl_t *pc, int *autoneg)
{
    PHY_CTRL_CHECK(pc);

    return bcm84846_drv.pd_autoneg_get(pc, autoneg);
}

/*
 * Function:    
 *      bcm84848_phy_loopback_set
 * Purpose:     
 *      Set the internal PHY loopback mode.
 * Parameters:
 *      pc - PHY control structure
 *      enable - non-zero enables PHY loopback
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm84848_phy_loopback_set(phy_ctrl_t *pc, int enable)
{
    PHY_CTRL_CHECK(pc);

    return bcm84846_drv.pd_loopback_set(pc, enable);
}

/*
 * Function:    
 *      bcm84848_phy_loopback_get
 * Purpose:     
 *      Get the local PHY loopback mode.
 * Parameters:
 *      pc - PHY control structure
 *      enable - (OUT) non-zero indicates PHY loopback enabled
 * Returns:     
 *      CDK_E_xxx
 * Notes:
 *      Return correct value independently of passthru flag.
 */
static int
bcm84848_phy_loopback_get(phy_ctrl_t *pc, int *enable)
{
    PHY_CTRL_CHECK(pc);

    return bcm84846_drv.pd_loopback_get(pc, enable);
}

/*
 * Function:    
 *      bcm84848_phy_ability_get
 * Purpose:     
 *      Get the abilities of the PHY.
 * Parameters:
 *      pc - PHY control structure
 *      abil - (OUT) ability mask indicating supported options/speeds.
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm84848_phy_ability_get(phy_ctrl_t *pc, uint32_t *abil)
{
    PHY_CTRL_CHECK(pc);

    return bcm84846_drv.pd_ability_get(pc, abil);
}

/*
 * Function:
 *      bcm84848_phy_config_set
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
bcm84848_phy_config_set(phy_ctrl_t *pc, phy_config_t cfg, uint32_t val, void *cd)
{
    PHY_CTRL_CHECK(pc);

    return bcm84846_drv.pd_config_set(pc, cfg, val, cd);
}

/*
 * Function:
 *      bcm84848_phy_config_get
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
bcm84848_phy_config_get(phy_ctrl_t *pc, phy_config_t cfg, uint32_t *val, void *cd)
{
    PHY_CTRL_CHECK(pc);

    return bcm84846_drv.pd_config_get(pc, cfg, val, cd);
}

/*
 * Function:
 *      bcm84848_phy_status_get
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
bcm84848_phy_status_get(phy_ctrl_t *pc, phy_status_t st, uint32_t *val)
{
    PHY_CTRL_CHECK(pc);

    return bcm84846_drv.pd_status_get(pc, st, val);
}

/*
 * Variable:    bcm84848_drv
 * Purpose:     PHY Driver for BCM84848.
 */
phy_driver_t bcm84848_drv = {
    "bcm84848",
    "BCM84848 10GbE PHY Driver",  
    0,
    bcm84848_phy_probe,                  /* pd_probe */
    bcm84848_phy_notify,                 /* pd_notify */
    bcm84848_phy_reset,                  /* pd_reset */
    bcm84848_phy_init,                   /* pd_init */
    bcm84848_phy_link_get,               /* pd_link_get */
    bcm84848_phy_duplex_set,             /* pd_duplex_set */
    bcm84848_phy_duplex_get,             /* pd_duplex_get */
    bcm84848_phy_speed_set,              /* pd_speed_set */
    bcm84848_phy_speed_get,              /* pd_speed_get */
    bcm84848_phy_autoneg_set,            /* pd_autoneg_set */
    bcm84848_phy_autoneg_get,            /* pd_autoneg_get */
    bcm84848_phy_loopback_set,           /* pd_loopback_set */
    bcm84848_phy_loopback_get,           /* pd_loopback_get */
    bcm84848_phy_ability_get,            /* pd_ability_get */
    bcm84848_phy_config_set,             /* pd_config_set */
    bcm84848_phy_config_get,             /* pd_config_get */
    bcm84848_phy_status_get,             /* pd_status_get */
    NULL                                 /* pd_cable_diag */
};
