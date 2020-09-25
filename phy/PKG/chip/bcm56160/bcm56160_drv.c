/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * PHY driver for BCM56160 embedded gigabit PHY.
 */

#include <phy/phy.h>
#include <phy/ge_phy.h>
#include <phy/phy_drvlist.h>
#include <phy/chip/bcm54282_defs.h>

#define BCM56160_PHY_ID0                0xAE02
#define BCM56160_PHY_ID1                0x52D0

#define PHY_ID1_REV_MASK                0x000f

/* Lane from PHY control instance */
#define LANE_NUM_MASK                   0x7

/* Default LED control */
#define BCM56160_LED1_SEL(_pc)          0x0
#define BCM56160_LED2_SEL(_pc)          0x1
#define BCM56160_LED3_SEL(_pc)          0x3
#define BCM56160_LED4_SEL(_pc)          0x6
#define BCM56160_LEDCTRL(_pc)           0x8
#define BCM56160_LEDSELECT(_pc)         0x0

/*
 * Function:
 *      bcm56160_phy_probe
 * Purpose:     
 *      Probe for PHY
 * Parameters:
 *      pc - PHY control structure
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm56160_phy_probe(phy_ctrl_t *pc)
{
    uint32_t phyid0, phyid1;
    int ioerr = 0;

    PHY_CTRL_CHECK(pc);

    ioerr += PHY_BUS_READ(pc, MII_PHY_ID0_REG, &phyid0);
    ioerr += PHY_BUS_READ(pc, MII_PHY_ID1_REG, &phyid1);
    if ((phyid0 == BCM56160_PHY_ID0 && 
        (phyid1 & ~PHY_ID1_REV_MASK) == BCM56160_PHY_ID1)) {
        return ioerr ? CDK_E_IO : CDK_E_NONE;
    }
    return CDK_E_NOT_FOUND;
}

/*
 * Function:
 *      bcm56160_phy_notify
 * Purpose:     
 *      Handle PHY notifications
 * Parameters:
 *      pc - PHY control structure
 *      event - PHY event
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm56160_phy_notify(phy_ctrl_t *pc, phy_event_t event)
{
    return bcm54282_drv.pd_notify(pc, event);
}

/*
 * Function:
 *      bcm56160_phy_reset
 * Purpose:     
 *      Reset PHY
 * Parameters:
 *      pc - PHY control structure
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm56160_phy_reset(phy_ctrl_t *pc)
{
    return bcm54282_drv.pd_reset(pc);
}

/*
 * Function:
 *      bcm56160_phy_init
 * Purpose:
 *      Initialize PHY driver
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
bcm56160_phy_init(phy_ctrl_t *pc)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    POWER_MII_CTRLr_t power_mii_ctrl;
    LED_SELECTOR_1r_t led_selector_1;
    LED_SELECTOR_2r_t led_selector_2;
    LED_CTRLr_t led_ctrl;
    EXP_LED_SELECTORr_t exp_led_selector;
    MII_AUX_CTRLr_t mii_aux_ctrl;
    MII_PHY_ECRr_t mii_phy_ecr;
    MII_CTRLr_t mii_ctrl;
    MII_GB_CTRLr_t mii_gb_ctrl;
    MII_ANAr_t mii_ana;
    AUX_1000X_CTRLr_t aux_1000x_ctrl;
    int val;
    
    PHY_CTRL_CHECK(pc);

    /* Default is copper mode */
    PHY_CTRL_FLAGS(pc) &= ~PHY_F_FIBER_MODE;

    /* Set port mode */
    MII_GB_CTRLr_CLR(mii_gb_ctrl);
    MII_GB_CTRLr_SWITCH_DEVf_SET(mii_gb_ctrl, 1);
    MII_GB_CTRLr_CAP_1000BASE_T_FDf_SET(mii_gb_ctrl, 1);
    ioerr += WRITE_MII_GB_CTRLr(pc, mii_gb_ctrl);

    /* Set speed duplex and autoneg */
    MII_CTRLr_CLR(mii_ctrl);
    MII_CTRLr_FULL_DUPLEXf_SET(mii_ctrl, 1);
    MII_CTRLr_SPEED_SEL1f_SET(mii_ctrl, 1);
    MII_CTRLr_AUTONEGf_SET(mii_ctrl, 1);
    MII_CTRLr_RESTART_ANf_SET(mii_ctrl, 1);
    ioerr += WRITE_MII_CTRLr(pc, mii_ctrl);

    /* Disable SuperIsolate */
    ioerr += READ_POWER_MII_CTRLr(pc, &power_mii_ctrl);
    POWER_MII_CTRLr_SUPER_ISOLATEf_SET(power_mii_ctrl, 0);
    ioerr += WRITE_POWER_MII_CTRLr(pc, power_mii_ctrl);

    /* Power up copper interface */
    ioerr += READ_MII_CTRLr(pc, &mii_ctrl);
    MII_CTRLr_POWER_DOWNf_SET(mii_ctrl, 0);
    ioerr += WRITE_MII_CTRLr(pc, mii_ctrl);

    /* Configure Extended Control Register */
    ioerr += READ_MII_PHY_ECRr(pc, &mii_phy_ecr);
    /* Enable LEDs to indicate traffic status */
    MII_PHY_ECRr_LED_TRAFFIC_ENf_SET(mii_phy_ecr, 1);
    ioerr += WRITE_MII_PHY_ECRr(pc, mii_phy_ecr);

    /* Enable extended packet length (4.5k through 25k) */
    ioerr += READ_MII_AUX_CTRLr(pc, &mii_aux_ctrl);
    MII_AUX_CTRLr_EXT_PKT_LENf_SET(mii_aux_ctrl, 1);
    ioerr += WRITE_MII_AUX_CTRLr(pc, mii_aux_ctrl);

    /* Configure LED selectors */
    LED_SELECTOR_1r_SET(led_selector_1,
            BCM56160_LED1_SEL(pc) | (BCM56160_LED2_SEL(pc) << 4));
    ioerr += WRITE_LED_SELECTOR_1r(pc, led_selector_1);

    LED_SELECTOR_2r_SET(led_selector_2,
            BCM56160_LED3_SEL(pc) | (BCM56160_LED4_SEL(pc) << 4));
    ioerr += WRITE_LED_SELECTOR_2r(pc, led_selector_2);

    LED_CTRLr_SET(led_ctrl, BCM56160_LEDCTRL(pc));
    ioerr += WRITE_LED_CTRLr(pc, led_ctrl);

    EXP_LED_SELECTORr_SET(exp_led_selector, BCM56160_LEDSELECT(pc));
    ioerr += WRITE_EXP_LED_SELECTORr(pc, exp_led_selector);

    /*
     * Configure Auxiliary control register to turn off
     * carrier extension.  The Intel 7131 NIC does not accept carrier
     * extension and gets CRC errors.
     */
    /* Disable carrier extension */
    ioerr += READ_AUX_1000X_CTRLr(pc, &aux_1000x_ctrl);
    val = AUX_1000X_CTRLr_GET(aux_1000x_ctrl);
    AUX_1000X_CTRLr_SET(aux_1000x_ctrl, (val | (1 << 6)));
    ioerr += WRITE_AUX_1000X_CTRLr(pc, aux_1000x_ctrl);

    ioerr += READ_MII_AUX_CTRLr(pc, &mii_aux_ctrl);
    MII_AUX_CTRLr_ENABLE_DSP_CLOCKf_SET(mii_aux_ctrl, 1);
    MII_AUX_CTRLr_RESERVEDf_SET(mii_aux_ctrl, 1);
    ioerr += WRITE_MII_AUX_CTRLr(pc, mii_aux_ctrl);

    ioerr += PHY_BUS_WRITE(pc, 0x17, 0x000e);
    ioerr += PHY_BUS_WRITE(pc, 0x15, 0x0752);
    ioerr += PHY_BUS_WRITE(pc, 0x17, 0x000f);
    ioerr += PHY_BUS_WRITE(pc, 0x15, 0xe04e);

    ioerr += READ_MII_AUX_CTRLr(pc, &mii_aux_ctrl);
    MII_AUX_CTRLr_ENABLE_DSP_CLOCKf_SET(mii_aux_ctrl, 0);
    MII_AUX_CTRLr_RESERVEDf_SET(mii_aux_ctrl, 1);
    ioerr += WRITE_MII_AUX_CTRLr(pc, mii_aux_ctrl);

    /* 100BASE-TX initialization change for EEE and autogreen mode */
    ioerr += READ_MII_AUX_CTRLr(pc, &mii_aux_ctrl);
    MII_AUX_CTRLr_ENABLE_DSP_CLOCKf_SET(mii_aux_ctrl, 1);
    MII_AUX_CTRLr_RESERVEDf_SET(mii_aux_ctrl, 1);
    ioerr += WRITE_MII_AUX_CTRLr(pc, mii_aux_ctrl);

    ioerr += PHY_BUS_WRITE(pc, 0x17, 0x4022);
    ioerr += PHY_BUS_WRITE(pc, 0x15, 0x017B);

    ioerr += READ_MII_AUX_CTRLr(pc, &mii_aux_ctrl);
    MII_AUX_CTRLr_ENABLE_DSP_CLOCKf_SET(mii_aux_ctrl, 0);
    MII_AUX_CTRLr_RESERVEDf_SET(mii_aux_ctrl, 1);
    ioerr += WRITE_MII_AUX_CTRLr(pc, mii_aux_ctrl);

    /* Set pause enable */
    ioerr += READ_MII_ANAr(pc, &mii_ana);
    MII_ANAr_PAUSEf_SET(mii_ana, 1);
    ioerr += WRITE_MII_ANAr(pc, mii_ana);

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
 *      bcm56160_phy_link_get
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
bcm56160_phy_link_get(phy_ctrl_t *pc, int *link, int *autoneg_done)
{
    return bcm54282_drv.pd_link_get(pc, link, autoneg_done);
}

/*
 * Function:    
 *      bcm56160_phy_duplex_set
 * Purpose:     
 *      Set the current duplex mode (forced).
 * Parameters:
 *      pc - PHY control structure
 *      duplex - non-zero indicates full duplex, zero indicates half
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm56160_phy_duplex_set(phy_ctrl_t *pc, int duplex)
{
    return bcm54282_drv.pd_duplex_set(pc, duplex);
}

/*
 * Function:    
 *      bcm56160_phy_duplex_get
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
bcm56160_phy_duplex_get(phy_ctrl_t *pc, int *duplex)
{
    return bcm54282_drv.pd_duplex_get(pc, duplex);
}

/*
 * Function:    
 *      bcm56160_phy_speed_set
 * Purpose:     
 *      Set the current operating speed (forced).
 * Parameters:
 *      pc - PHY control structure
 *      speed - new link speed
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm56160_phy_speed_set(phy_ctrl_t *pc, uint32_t speed)
{
    return bcm54282_drv.pd_speed_set(pc, speed);
}

/*
 * Function:    
 *      bcm56160_phy_speed_get
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
bcm56160_phy_speed_get(phy_ctrl_t *pc, uint32_t *speed)
{
    return bcm54282_drv.pd_speed_get(pc, speed);
}

/*
 * Function:    
 *      bcm56160_phy_autoneg_set
 * Purpose:     
 *      Enable or disabled auto-negotiation on the specified port.
 * Parameters:
 *      pc - PHY control structure
 *      autoneg - non-zero enables autoneg, zero disables autoneg
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm56160_phy_autoneg_set(phy_ctrl_t *pc, int autoneg)
{
    return bcm54282_drv.pd_autoneg_set(pc, autoneg);
}

/*
 * Function:    
 *      bcm56160_phy_autoneg_get
 * Purpose:     
 *      Get the current auto-negotiation status (enabled/busy)
 * Parameters:
 *      pc - PHY control structure
 *      autoneg - (OUT) non-zero indicates autoneg enabled
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm56160_phy_autoneg_get(phy_ctrl_t *pc, int *autoneg)
{
    return bcm54282_drv.pd_autoneg_get(pc, autoneg);
}

/*
 * Function:    
 *      bcm56160_phy_loopback_set
 * Purpose:     
 *      Set the internal PHY loopback mode.
 * Parameters:
 *      pc - PHY control structure
 *      enable - non-zero enables PHY loopback
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm56160_phy_loopback_set(phy_ctrl_t *pc, int enable)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t speed, force_link = 0;
    MII_1000X_CTRLr_t mii_1000x_ctrl;
    TEST_1r_t test_1;

    PHY_CTRL_CHECK(pc);

    if (enable) {
        enable = 1;
    }

    if (PHY_CTRL_FLAGS(pc) & PHY_F_FIBER_MODE) {
        ioerr += READ_MII_1000X_CTRLr(pc, &mii_1000x_ctrl);
        MII_1000X_CTRLr_LOOPBACKf_SET(mii_1000x_ctrl, enable);
        ioerr += WRITE_MII_1000X_CTRLr(pc, mii_1000x_ctrl);
    } else {
        rv = ge_phy_loopback_set(pc, enable);

        if (CDK_SUCCESS(rv)) {
            rv = PHY_SPEED_GET(pc, &speed); 
            if (CDK_SUCCESS(rv)) {
                /* When 10/100M mode, force link need to set this register */
                if ((speed == 10) || (speed == 100)) {
                    force_link = enable;
                }
                ioerr += READ_TEST_1r(pc, &test_1);
                TEST_1r_FORCE_LINKf_SET(test_1, force_link);
                ioerr += WRITE_TEST_1r(pc, test_1);
            }
        }
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:    
 *      bcm56160_phy_loopback_get
 * Purpose:     
 *      Get the local PHY loopback mode.
 * Parameters:
 *      pc - PHY control structure
 *      enable - (OUT) non-zero indicates PHY loopback enabled
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm56160_phy_loopback_get(phy_ctrl_t *pc, int *enable)
{
    return bcm54282_drv.pd_loopback_get(pc, enable);
}

/*
 * Function:    
 *      bcm56160_phy_ability_get
 * Purpose:     
 *      Get the abilities of the PHY.
 * Parameters:
 *      pc - PHY control structure
 *      abil - (OUT) ability mask indicating supported options/speeds.
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm56160_phy_ability_get(phy_ctrl_t *pc, uint32_t *abil)
{
    return bcm54282_drv.pd_ability_get(pc, abil);
}

/*
 * Function:
 *      bcm56160_phy_config_set
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
bcm56160_phy_config_set(phy_ctrl_t *pc, phy_config_t cfg, uint32_t val, void *cd)
{
    return bcm54282_drv.pd_config_set(pc, cfg, val, cd);
}

/*
 * Function:
 *      bcm56160_phy_config_get
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
bcm56160_phy_config_get(phy_ctrl_t *pc, phy_config_t cfg, uint32_t *val, void *cd)
{
    return bcm54282_drv.pd_config_get(pc, cfg, val, cd);
}

/*
 * Variable:    bcm56160_phy drv
 * Purpose:     PHY Driver for BCM56160.
 */
phy_driver_t bcm56160_drv = {
    "bcm56160",
    "Internal Embedded Gigabit PHY Driver",  
    PHY_DRIVER_F_INTERNAL,
    bcm56160_phy_probe,                 /* pd_probe */
    bcm56160_phy_notify,                /* pd_notify */
    bcm56160_phy_reset,                 /* pd_reset */
    bcm56160_phy_init,                  /* pd_init */
    bcm56160_phy_link_get,              /* pd_link_get */
    bcm56160_phy_duplex_set,            /* pd_duplex_set */
    bcm56160_phy_duplex_get,            /* pd_duplex_get */
    bcm56160_phy_speed_set,             /* pd_speed_set */
    bcm56160_phy_speed_get,             /* pd_speed_get */
    bcm56160_phy_autoneg_set,           /* pd_autoneg_set */
    bcm56160_phy_autoneg_get,           /* pd_autoneg_get */
    bcm56160_phy_loopback_set,          /* pd_loopback_set */
    bcm56160_phy_loopback_get,          /* pd_loopback_get */
    bcm56160_phy_ability_get,           /* pd_ability_get */
    bcm56160_phy_config_set,            /* pd_config_set */
    bcm56160_phy_config_get,            /* pd_config_get */
    NULL,                               /* pd_status_get */
    NULL                                /* pd_cable_diag */
};
