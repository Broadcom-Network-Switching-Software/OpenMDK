/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * PHY driver for BCM54280.
 */

#include <phy/phy.h>
#include <phy/ge_phy.h>
#include <phy/phy_drvlist.h>
#include <phy/chip/bcm54282_defs.h>

#define BCM54280_PHY_ID0                0x600d
#define BCM54280_PHY_ID1                0x8440

#define BCM54290_PHY_ID0                0x600d
#define BCM54290_PHY_ID1                0x8520

#define PHY_ID1_REV_MASK                0x000f

/* Lane from PHY control instance */
#define LANE_NUM_MASK                   0x7

/* Default LED control */
#define BCM54280_LED1_SEL(_pc)          0x0
#define BCM54280_LED2_SEL(_pc)          0x1
#define BCM54280_LED3_SEL(_pc)          0x3
#define BCM54280_LED4_SEL(_pc)          0x6
#define BCM54280_LEDCTRL(_pc)           0x8
#define BCM54280_LEDSELECT(_pc)         0x0

#define PHY_IS_BCM54280(pc) \
                ((phyid0 == BCM54280_PHY_ID0) && \
                 ((phyid1 & ~PHY_ID1_REV_MASK) == BCM54280_PHY_ID1) && \
                 !((phyid1 & PHY_ID1_REV_MASK) & 0x8))

#define PHY_IS_BCM54290(pc) \
                ((phyid0 == BCM54290_PHY_ID0) && \
                 ((phyid1 & ~PHY_ID1_REV_MASK) == BCM54290_PHY_ID1) && \
                 !((phyid1 & PHY_ID1_REV_MASK) & 0x8))
                 
/***********************************************************************
 *
 * HELPER FUNCTIONS
 */

/*
 * Function:
 *      _bcm54280_inst
 * Purpose:
 *      Retrieve PHY instance.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      PHY instance
 */
static int
_bcm54280_inst(phy_ctrl_t *pc)
{
    int inst = PHY_CTRL_PHY_INST(pc);

    if (inst < 0) {
        uint32_t addr = PHY_CTRL_PHY_ADDR(pc);

        while (addr > 8) {
            addr -= 8;
        }
        inst = addr - 1;
    }
    return inst & LANE_NUM_MASK;
}

/***********************************************************************
 *
 * PHY DRIVER FUNCTIONS
 */

#if PHY_CONFIG_INCLUDE_CHIP_SYMBOLS == 1
extern cdk_symbols_t bcm54282_symbols;
#endif

/*
 * Function:
 *      bcm54280_phy_probe
 * Purpose:     
 *      Probe for PHY
 * Parameters:
 *      pc - PHY control structure
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm54280_phy_probe(phy_ctrl_t *pc)
{
    uint32_t phyid0, phyid1;
    int ioerr = 0;

    PHY_CTRL_CHECK(pc);

    ioerr += PHY_BUS_READ(pc, MII_PHY_ID0_REG, &phyid0);
    ioerr += PHY_BUS_READ(pc, MII_PHY_ID1_REG, &phyid1);
    if (PHY_IS_BCM54280(pc) || PHY_IS_BCM54290(pc)) {
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
 *      bcm54280_phy_notify
 * Purpose:     
 *      Handle PHY notifications
 * Parameters:
 *      pc - PHY control structure
 *      event - PHY event
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm54280_phy_notify(phy_ctrl_t *pc, phy_event_t event)
{
    return bcm54282_drv.pd_notify(pc, event);
}

/*
 * Function:
 *      bcm54280_phy_reset
 * Purpose:     
 *      Reset PHY
 * Parameters:
 *      pc - PHY control structure
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm54280_phy_reset(phy_ctrl_t *pc)
{
     return bcm54282_drv.pd_reset(pc);
}

/*
 * Function:
 *      bcm54280_phy_init
 * Purpose:     
 *      Initialize PHY driver
 * Parameters:
 *      pc - PHY control structure
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm54280_phy_init(phy_ctrl_t *pc)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    TOP_MISC_GLOBAL_RESETr_t top_misc_global_reset;
    AFE_VDACCTRL_2r_t afe_vdacc_ctrl_2;
    DSP_TAP10r_t dsp_tap10;
    POWER_MII_CTRLr_t power_mii_ctrl;
    LED_GPIO_CTRLr_t led_gpio_ctrl;
    LED_SELECTOR_1r_t led_sel_1;
    LED_SELECTOR_2r_t led_sel_2;
    LED_CTRLr_t led_ctrl;
    EXP_LED_SELECTORr_t exp_led_selector;
    SPARE_CTRLr_t spare_ctrl;
    MII_AUX_CTRLr_t mii_aux_ctrl;
    EXP_PATT_GEN_STATr_t exp_patt_gen_stat;
    MII_PHY_ECRr_t mii_phy_ecr;
    EEE_803Dr_t eee_803d;
    EEE_ADVr_t eee_adv;
    EEE_STAT_CTRLr_t eee_stat_ctrl;
    MII_BUFFER_CONTROL_0r_t mii_buf_ctrl;
    int orig_inst;
    uint32_t led_val;

    PHY_CTRL_CHECK(pc);

    /* Get primary PHY instance */
    orig_inst = _bcm54280_inst(pc);

    /* Reset Top level register block*/
    TOP_MISC_GLOBAL_RESETr_CLR(top_misc_global_reset);
    TOP_MISC_GLOBAL_RESETr_TOP_MII_REG_SOFT_RSTf_SET(top_misc_global_reset, 1);
    TOP_MISC_GLOBAL_RESETr_RESET_1588f_SET(top_misc_global_reset, 1);
    ioerr += WRITE_TOP_MISC_GLOBAL_RESETr(pc, top_misc_global_reset);

    AFE_VDACCTRL_2r_SET(afe_vdacc_ctrl_2, 0x7);
    ioerr += WRITE_AFE_VDACCTRL_2r(pc, afe_vdacc_ctrl_2);

    /* Recommended DSP_TAP10 setting for all the revisions */
    MII_AUX_CTRLr_CLR(mii_aux_ctrl);
    MII_AUX_CTRLr_ENABLE_DSP_CLOCKf_SET(mii_aux_ctrl, 1);
    MII_AUX_CTRLr_RESERVEDf_SET(mii_aux_ctrl, 1);
    ioerr += WRITE_MII_AUX_CTRLr(pc, mii_aux_ctrl);

    DSP_TAP10r_SET(dsp_tap10, 0x011B);
    ioerr += WRITE_DSP_TAP10r(pc, dsp_tap10);

    /* Disable SuperIsolate */
    ioerr += READ_POWER_MII_CTRLr(pc, &power_mii_ctrl);
    POWER_MII_CTRLr_SUPER_ISOLATEf_SET(power_mii_ctrl, 0);
    ioerr += WRITE_POWER_MII_CTRLr(pc, power_mii_ctrl);

    /* Enable current mode LED */
    ioerr += READ_LED_GPIO_CTRLr(pc, &led_gpio_ctrl);
    LED_GPIO_CTRLr_CURRENT_LED_DISABLEf_SET(led_gpio_ctrl, 0);
    ioerr += WRITE_LED_GPIO_CTRLr(pc, led_gpio_ctrl);

    /* Enable extended packet length (4.5k through 25k) */
    ioerr += READ_MII_AUX_CTRLr(pc, &mii_aux_ctrl);
    MII_AUX_CTRLr_EXT_PKT_LENf_SET(mii_aux_ctrl, 1);
    ioerr += WRITE_MII_AUX_CTRLr(pc, mii_aux_ctrl);

    ioerr += READ_EXP_PATT_GEN_STATr(pc, &exp_patt_gen_stat);
    EXP_PATT_GEN_STATr_GMII_RGMII_FIFO_ELASTICITY_1f_SET(exp_patt_gen_stat, 1);
    ioerr += WRITE_EXP_PATT_GEN_STATr(pc, exp_patt_gen_stat);

    /* Enable traffic LED mode and set FIFO elasticity to 10K (jumbo frames) */
    ioerr += READ_MII_PHY_ECRr(pc, &mii_phy_ecr);
    MII_PHY_ECRr_LED_TRAFFIC_ENf_SET(mii_phy_ecr, 1);
    MII_PHY_ECRr_PCS_TX_FIFO_ELASTf_SET(mii_phy_ecr, 1);
    ioerr += WRITE_MII_PHY_ECRr(pc, mii_phy_ecr);

    /* Reset EEE to default state (LPI disabled) */
    ioerr += READ_EEE_803Dr(pc, &eee_803d);
    EEE_803Dr_LPI_FEATURE_ENABLEf_SET(eee_803d, 0);
    EEE_803Dr_EEE_CAPABILITY_USING_QSGMII_ANf_SET(eee_803d, 0);
    ioerr += WRITE_EEE_803Dr(pc, eee_803d);

    /* Do not advertise EEE capability */   
    ioerr += READ_EEE_ADVr(pc, &eee_adv);
    EEE_ADVr_EEE_1000BASE_Tf_SET(eee_adv, 0);
    EEE_ADVr_EEE_100BASE_TXf_SET(eee_adv, 0);
    ioerr += WRITE_EEE_ADVr(pc, eee_adv);
    
    /* Reset counters and other settings : EEE_STAT_CTRLr */   
    ioerr += READ_EEE_STAT_CTRLr(pc, &eee_stat_ctrl);
    EEE_STAT_CTRLr_STATISTIC_SATURATE_MODEf_SET(eee_stat_ctrl, 0);
    EEE_STAT_CTRLr_RESERVED1f_SET(eee_stat_ctrl, 0);
    EEE_STAT_CTRLr_RESERVED0f_SET(eee_stat_ctrl, 0);
    EEE_STAT_CTRLr_STATISTIC_COUNTERS_RESETf_SET(eee_stat_ctrl, 0);
    EEE_STAT_CTRLr_STATISTIC_COUNTERS_ENABLEf_SET(eee_stat_ctrl, 0);
    ioerr += WRITE_EEE_STAT_CTRLr(pc, eee_stat_ctrl);
    
    /* Switch to instance 0 for AutogrEEEn configuration */
    if (phy_ctrl_change_inst(pc, 0, _bcm54280_inst) < 0) {
        return CDK_E_FAIL;
    }

    /* Disable AutogrEEEn and reset other settings */
    MII_BUFFER_CONTROL_0r_CLR(mii_buf_ctrl);
    MII_BUFFER_CONTROL_0r_DISABLE_LOC_LPI_HALF_DUPLEXf_SET(mii_buf_ctrl, 1);
    ioerr += WRITE_MII_BUFFER_CONTROL_0r(pc, orig_inst, mii_buf_ctrl);

    /* Restore instance */
    phy_ctrl_change_inst(pc, orig_inst, _bcm54280_inst);

    /* Disable LED current mode */
    ioerr += READ_LED_GPIO_CTRLr(pc, &led_gpio_ctrl);
    LED_GPIO_CTRLr_CURRENT_LED_DISABLEf_SET(led_gpio_ctrl, 0xf);
    ioerr += WRITE_LED_GPIO_CTRLr(pc, led_gpio_ctrl);

    /* Configure LED selectors */
    led_val = BCM54280_LED1_SEL(pc) | (BCM54280_LED2_SEL(pc) << 4);
    LED_SELECTOR_1r_SET(led_sel_1, led_val);
    ioerr += WRITE_LED_SELECTOR_1r(pc, led_sel_1);

    led_val = BCM54280_LED3_SEL(pc) | (BCM54280_LED4_SEL(pc) << 4);
    LED_SELECTOR_2r_SET(led_sel_2, led_val);
    ioerr += WRITE_LED_SELECTOR_2r(pc, led_sel_2);

    LED_CTRLr_SET(led_ctrl, BCM54280_LEDCTRL(pc));
    ioerr += WRITE_LED_CTRLr(pc, led_ctrl);

    EXP_LED_SELECTORr_SET(exp_led_selector, BCM54280_LEDSELECT(pc));
    ioerr += WRITE_EXP_LED_SELECTORr(pc, exp_led_selector);

    /* Change link speed LED mode */
    ioerr += READ_SPARE_CTRLr(pc, &spare_ctrl);
    SPARE_CTRLr_LINK_SPEED_LED_MODEf_SET(spare_ctrl, 0);
    SPARE_CTRLr_LINK_LED_MODEf_SET(spare_ctrl, 0);
    ioerr += WRITE_SPARE_CTRLr(pc, spare_ctrl);

    /* Enable PHY temperature monitor */
    if (CDK_SUCCESS(rv)) {
        rv = PHY_CONFIG_SET(pc, PhyConfig_TempMon, 1, NULL);
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
 *      bcm54280_phy_link_get
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
bcm54280_phy_link_get(phy_ctrl_t *pc, int *link, int *autoneg_done)
{
     return bcm54282_drv.pd_link_get(pc, link, autoneg_done);
}

/*
 * Function:    
 *      bcm54280_phy_duplex_set
 * Purpose:     
 *      Set the current duplex mode (forced).
 * Parameters:
 *      pc - PHY control structure
 *      duplex - non-zero indicates full duplex, zero indicates half
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm54280_phy_duplex_set(phy_ctrl_t *pc, int duplex)
{
    return bcm54282_drv.pd_duplex_set(pc, duplex);
}

/*
 * Function:    
 *      bcm54280_phy_duplex_get
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
bcm54280_phy_duplex_get(phy_ctrl_t *pc, int *duplex)
{
     return bcm54282_drv.pd_duplex_get(pc, duplex);
}

/*
 * Function:    
 *      bcm54280_phy_speed_set
 * Purpose:     
 *      Set the current operating speed (forced).
 * Parameters:
 *      pc - PHY control structure
 *      speed - new link speed
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm54280_phy_speed_set(phy_ctrl_t *pc, uint32_t speed)
{
    return bcm54282_drv.pd_speed_set(pc, speed);
}

/*
 * Function:    
 *      bcm54280_phy_speed_get
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
bcm54280_phy_speed_get(phy_ctrl_t *pc, uint32_t *speed)
{
    return bcm54282_drv.pd_speed_get(pc, speed);
}

/*
 * Function:    
 *      bcm54280_phy_autoneg_set
 * Purpose:     
 *      Enable or disabled auto-negotiation on the specified port.
 * Parameters:
 *      pc - PHY control structure
 *      autoneg - non-zero enables autoneg, zero disables autoneg
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm54280_phy_autoneg_set(phy_ctrl_t *pc, int autoneg)
{
    return bcm54282_drv.pd_autoneg_set(pc, autoneg);
}

/*
 * Function:    
 *      bcm54280_phy_autoneg_get
 * Purpose:     
 *      Get the current auto-negotiation status (enabled/busy)
 * Parameters:
 *      pc - PHY control structure
 *      autoneg - (OUT) non-zero indicates autoneg enabled
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm54280_phy_autoneg_get(phy_ctrl_t *pc, int *autoneg)
{
    return bcm54282_drv.pd_autoneg_get(pc, autoneg);
}

/*
 * Function:    
 *      bcm54280_phy_loopback_set
 * Purpose:     
 *      Set the internal PHY loopback mode.
 * Parameters:
 *      pc - PHY control structure
 *      enable - non-zero enables PHY loopback
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm54280_phy_loopback_set(phy_ctrl_t *pc, int enable)
{
    return bcm54282_drv.pd_loopback_set(pc, enable);
}

/*
 * Function:    
 *      bcm54280_phy_loopback_get
 * Purpose:     
 *      Get the local PHY loopback mode.
 * Parameters:
 *      pc - PHY control structure
 *      enable - (OUT) non-zero indicates PHY loopback enabled
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm54280_phy_loopback_get(phy_ctrl_t *pc, int *enable)
{
    return bcm54282_drv.pd_loopback_get(pc, enable);
}

/*
 * Function:    
 *      bcm54280_phy_ability_get
 * Purpose:     
 *      Get the abilities of the PHY.
 * Parameters:
 *      pc - PHY control structure
 *      abil - (OUT) ability mask indicating supported options/speeds.
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm54280_phy_ability_get(phy_ctrl_t *pc, uint32_t *abil)
{
    PHY_CTRL_CHECK(pc);

    *abil = (PHY_ABIL_1000MB | PHY_ABIL_100MB | PHY_ABIL_10MB | 
             PHY_ABIL_LOOPBACK | PHY_ABIL_SGMII);
    
    return CDK_E_NONE;
}

/*
 * Function:
 *      bcm54280_phy_config_set
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
bcm54280_phy_config_set(phy_ctrl_t *pc, phy_config_t cfg, uint32_t val, void *cd)
{
    return bcm54282_drv.pd_config_set(pc, cfg, val, cd);
}

/*
 * Function:
 *      bcm54280_phy_config_get
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
bcm54280_phy_config_get(phy_ctrl_t *pc, phy_config_t cfg, uint32_t *val, void *cd)
{
    return bcm54282_drv.pd_config_get(pc, cfg, val, cd);
}

/*
 * Function:
 *      bcm54280_phy_status_get
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
bcm54280_phy_status_get(phy_ctrl_t *pc, phy_status_t st, uint32_t *val)
{
    return bcm54282_drv.pd_status_get(pc, st, val);
}

/*
 * Variable:    bcm54280_phy drv
 * Purpose:     PHY Driver for BCM54280.
 */
phy_driver_t bcm54280_drv = {
    "bcm5428(9)0",
    "BCM5428(9)0 Gigabit PHY Driver",  
    0,
    bcm54280_phy_probe,                 /* pd_probe */
    bcm54280_phy_notify,                /* pd_notify */
    bcm54280_phy_reset,                 /* pd_reset */
    bcm54280_phy_init,                  /* pd_init */
    bcm54280_phy_link_get,              /* pd_link_get */
    bcm54280_phy_duplex_set,            /* pd_duplex_set */
    bcm54280_phy_duplex_get,            /* pd_duplex_get */
    bcm54280_phy_speed_set,             /* pd_speed_set */
    bcm54280_phy_speed_get,             /* pd_speed_get */
    bcm54280_phy_autoneg_set,           /* pd_autoneg_set */
    bcm54280_phy_autoneg_get,           /* pd_autoneg_get */
    bcm54280_phy_loopback_set,          /* pd_loopback_set */
    bcm54280_phy_loopback_get,          /* pd_loopback_get */
    bcm54280_phy_ability_get,           /* pd_ability_get */
    bcm54280_phy_config_set,            /* pd_config_set */
    bcm54280_phy_config_get,            /* pd_config_get */
    bcm54280_phy_status_get,            /* pd_status_get */
    NULL                                /* pd_cable_diag */
};
