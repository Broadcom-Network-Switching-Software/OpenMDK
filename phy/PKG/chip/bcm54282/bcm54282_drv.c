/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * PHY driver for BCM54282.
 */

#include <phy/phy.h>
#include <phy/ge_phy.h>
#include <phy/chip/bcm54282_defs.h>

#define BCM54282_PHY_ID0                0x600d
#define BCM54282_PHY_ID1                0x8450

#define PHY_ID1_REV_MASK                0x000f

/* Lane from PHY control instance */
#define LANE_NUM_MASK                   0x7

/* Default LED control */
#define BCM54282_LED1_SEL(_pc)          0x0
#define BCM54282_LED2_SEL(_pc)          0x1
#define BCM54282_LED3_SEL(_pc)          0x3
#define BCM54282_LED4_SEL(_pc)          0x6
#define BCM54282_LEDCTRL(_pc)           0x8
#define BCM54282_LEDSELECT(_pc)         0x0
                 
/***********************************************************************
 *
 * HELPER FUNCTIONS
 */

/*
 * Function:
 *      _bcm54282_inst
 * Purpose:
 *      Retrieve PHY instance.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      PHY instance
 */
static int
_bcm54282_inst(phy_ctrl_t *pc)
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
 *      bcm54282_phy_probe
 * Purpose:     
 *      Probe for PHY
 * Parameters:
 *      pc - PHY control structure
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm54282_phy_probe(phy_ctrl_t *pc)
{
    uint32_t phyid0, phyid1;
    int ioerr = 0;

    PHY_CTRL_CHECK(pc);

    ioerr += PHY_BUS_READ(pc, MII_PHY_ID0_REG, &phyid0);
    ioerr += PHY_BUS_READ(pc, MII_PHY_ID1_REG, &phyid1);
    if (phyid0 == BCM54282_PHY_ID0 && 
        (phyid1 & ~PHY_ID1_REV_MASK) == BCM54282_PHY_ID1) {
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
 *      bcm54282_phy_notify
 * Purpose:     
 *      Handle PHY notifications
 * Parameters:
 *      pc - PHY control structure
 *      event - PHY event
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm54282_phy_notify(phy_ctrl_t *pc, phy_event_t event)
{
    int rv = CDK_E_NONE;

    PHY_CTRL_CHECK(pc);

    switch (event) {
    case PhyEvent_ChangeToCopper:
        PHY_CTRL_FLAGS(pc) &= ~PHY_F_FIBER_MODE;
        PHY_CTRL_FLAGS(pc) &= ~PHY_F_PASSTHRU;
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
 *      bcm54282_phy_reset
 * Purpose:     
 *      Reset PHY
 * Parameters:
 *      pc - PHY control structure
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm54282_phy_reset(phy_ctrl_t *pc)
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
 *      bcm54282_phy_init
 * Purpose:     
 *      Initialize PHY driver
 * Parameters:
 *      pc - PHY control structure
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm54282_phy_init(phy_ctrl_t *pc)
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
    TOP_MISC_CFGr_t top_misc_cfg;
    MII_AUX_CTRLr_t mii_aux_ctrl;
    EXP_PATT_GEN_STATr_t exp_patt_gen_stat;
    QSGMII_1000X_CONTROL_3r_t qsgmii_1000x_ctrl_3;
    MII_PHY_ECRr_t mii_phy_ecr;
    EEE_803Dr_t eee_803d;
    EEE_ADVr_t eee_adv;
    EEE_STAT_CTRLr_t eee_stat_ctrl;
    MII_BUFFER_CONTROL_0r_t mii_buf_ctrl;
    int orig_inst;
    uint32_t led_val;

    PHY_CTRL_CHECK(pc);

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

    /*
     * The QSGMII MDIO sharing feature saves one MDIO address by
     * accessing the QSGMII via PHYA[4:0]+3 instead of PHYA[4:0]+8.
     * Must always be configured through PHYA[4:0]+0.
     */

    /* Get primary PHY instance */
    orig_inst = _bcm54282_inst(pc);

    /* Switch to instance 0 to enable QSGMII MDIO sharing */
    if (phy_ctrl_change_inst(pc, 0, _bcm54282_inst) < 0) {
        return CDK_E_FAIL;
    }

    /* Enable QSGMII MDIO sharing feature and map QSGMII registers */
    ioerr += READ_TOP_MISC_CFGr(pc, &top_misc_cfg);
    TOP_MISC_CFGr_QSGMII_SELf_SET(top_misc_cfg, 1);
    TOP_MISC_CFGr_QSGMII_PHYAf_SET(top_misc_cfg, 1);
    ioerr += WRITE_TOP_MISC_CFGr(pc, top_misc_cfg);
    
    /* Switch to instance 3 for QSGMII access */
    if (phy_ctrl_change_inst(pc, 3, _bcm54282_inst) < 0) {
        return CDK_E_FAIL;
    }
    
    /* QSGMII FIFO Elasticity */
    QSGMII_1000X_CONTROL_3r_CLR(qsgmii_1000x_ctrl_3);
    QSGMII_1000X_CONTROL_3r_FIFO_ELASTICITY_TX_RXf_SET(qsgmii_1000x_ctrl_3, 3);
    ioerr += WRITELN_QSGMII_1000X_CONTROL_3r(pc, orig_inst, qsgmii_1000x_ctrl_3);
    
    /* Switch to instance 0 to disable QSGMII MDIO sharing */
    if (phy_ctrl_change_inst(pc, 0, _bcm54282_inst) < 0) {
        return CDK_E_FAIL;
    }

    /* Disable QSGMII MDIO sharing feature and map copper registers */
    ioerr += READ_TOP_MISC_CFGr(pc, &top_misc_cfg);
    TOP_MISC_CFGr_QSGMII_SELf_SET(top_misc_cfg, 0);
    TOP_MISC_CFGr_QSGMII_PHYAf_SET(top_misc_cfg, 0);
    ioerr += WRITE_TOP_MISC_CFGr(pc, top_misc_cfg);

    /* Restore instance */
    phy_ctrl_change_inst(pc, orig_inst, _bcm54282_inst);

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
    if (phy_ctrl_change_inst(pc, 0, _bcm54282_inst) < 0) {
        return CDK_E_FAIL;
    }

    /* Disable AutogrEEEn and reset other settings */
    MII_BUFFER_CONTROL_0r_CLR(mii_buf_ctrl);
    MII_BUFFER_CONTROL_0r_DISABLE_LOC_LPI_HALF_DUPLEXf_SET(mii_buf_ctrl, 1);
    ioerr += WRITE_MII_BUFFER_CONTROL_0r(pc, orig_inst, mii_buf_ctrl);

    /* Restore instance */
    phy_ctrl_change_inst(pc, orig_inst, _bcm54282_inst);

    /* Disable LED current mode */
    ioerr += READ_LED_GPIO_CTRLr(pc, &led_gpio_ctrl);
    LED_GPIO_CTRLr_CURRENT_LED_DISABLEf_SET(led_gpio_ctrl, 0xf);
    ioerr += WRITE_LED_GPIO_CTRLr(pc, led_gpio_ctrl);

    /* Configure LED selectors */
    led_val = BCM54282_LED1_SEL(pc) | (BCM54282_LED2_SEL(pc) << 4);
    LED_SELECTOR_1r_SET(led_sel_1, led_val);
    ioerr += WRITE_LED_SELECTOR_1r(pc, led_sel_1);

    led_val = BCM54282_LED3_SEL(pc) | (BCM54282_LED4_SEL(pc) << 4);
    LED_SELECTOR_2r_SET(led_sel_2, led_val);
    ioerr += WRITE_LED_SELECTOR_2r(pc, led_sel_2);

    LED_CTRLr_SET(led_ctrl, BCM54282_LEDCTRL(pc));
    ioerr += WRITE_LED_CTRLr(pc, led_ctrl);

    EXP_LED_SELECTORr_SET(exp_led_selector, BCM54282_LEDSELECT(pc));
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
 *      bcm54282_phy_link_get
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
bcm54282_phy_link_get(phy_ctrl_t *pc, int *link, int *autoneg_done)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    MII_1000X_STATr_t mii_1000x_stat;
    uint32_t speed;

    PHY_CTRL_CHECK(pc);

    if (link == NULL) {
        return CDK_E_PARAM;
    }

    *link = FALSE;

    if (PHY_CTRL_FLAGS(pc) & PHY_F_FIBER_MODE) {

        /* Get fiber link status */
        ioerr += READ_MII_1000X_STATr(pc, &mii_1000x_stat);
        if (MII_1000X_STATr_LINKf_GET(mii_1000x_stat)) {
            *link = TRUE;
        }

        if (autoneg_done) {
            /* Use register value by default */
            *autoneg_done = MII_1000X_STATr_AUTONEG_DONEf_GET(mii_1000x_stat);

            /* Force done for 100FX */
            rv = PHY_SPEED_GET(pc, &speed);
            if (CDK_SUCCESS(rv) && speed == 100) {
                *autoneg_done = 1;
            }
        }
    } else {
        rv = ge_phy_link_get(pc, link, autoneg_done);
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:    
 *      bcm54282_phy_duplex_set
 * Purpose:     
 *      Set the current duplex mode (forced).
 * Parameters:
 *      pc - PHY control structure
 *      duplex - non-zero indicates full duplex, zero indicates half
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm54282_phy_duplex_set(phy_ctrl_t *pc, int duplex)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    SERDES_100FX_CTRLr_t fx100_ctrl;

    PHY_CTRL_CHECK(pc);

    if (duplex) {
        duplex = 1;
    }

    /* Call up the PHY chain */
    rv = PHY_DUPLEX_SET(PHY_CTRL_NEXT(pc), duplex);
    if (CDK_FAILURE(rv)) {
        return rv;
    }

    if (PHY_CTRL_FLAGS(pc) & PHY_F_FIBER_MODE) {
        ioerr += READ_SERDES_100FX_CTRLr(pc, &fx100_ctrl);
        SERDES_100FX_CTRLr_FD_FX_SERDESf_SET(fx100_ctrl, duplex);
        ioerr += WRITE_SERDES_100FX_CTRLr(pc, fx100_ctrl);
    } else {
        rv = ge_phy_duplex_set(pc, duplex);
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:    
 *      bcm54282_phy_duplex_get
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
bcm54282_phy_duplex_get(phy_ctrl_t *pc, int *duplex)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t speed;
    EXP_OPT_MODE_STATr_t exp_opt_mode_stat;

    PHY_CTRL_CHECK(pc);

    if (duplex == NULL) {
        return CDK_E_PARAM;
    }

    *duplex = 1;

    if (PHY_CTRL_FLAGS(pc) & PHY_F_FIBER_MODE) {
        rv = PHY_SPEED_GET(pc, &speed);
        if (CDK_FAILURE(rv)) {
            return rv;
        }
        
        if (speed == 100) {
            ioerr += READ_EXP_OPT_MODE_STATr(pc, &exp_opt_mode_stat);
            *duplex = EXP_OPT_MODE_STATr_FIBER_DUPLEXf_GET(exp_opt_mode_stat);
        }

    } else {
        rv = ge_phy_duplex_get(pc, duplex);
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:    
 *      bcm54282_phy_speed_set
 * Purpose:     
 *      Set the current operating speed (forced).
 * Parameters:
 *      pc - PHY control structure
 *      speed - new link speed
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm54282_phy_speed_set(phy_ctrl_t *pc, uint32_t speed)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int autoneg, lb_enable, fx100;
    MII_1000X_CTRLr_t mii_1000x_ctrl;
    SERDES_100FX_CTRLr_t fx100_ctrl;

    PHY_CTRL_CHECK(pc);

    /* Call up the PHY chain */
    if (CDK_SUCCESS(rv)) {
        rv = PHY_SPEED_SET(PHY_CTRL_NEXT(pc), speed);
    }
    if (CDK_FAILURE(rv)) {
        return rv;
    }

    /* Do not set speed if auto_negotiation is enabled */
    rv = PHY_AUTONEG_GET(pc, &autoneg);
    if (CDK_FAILURE(rv)) {
        return rv;
    }
    if (autoneg) {
        return CDK_E_NONE;
    }

    if (PHY_CTRL_FLAGS(pc) & PHY_F_FIBER_MODE) {
        /* Set fiber speed */
        fx100 = 0;
        switch (speed) {
        case 100:
            fx100 = 1;
            break;
        case 1000:
            break;
        default:
            return CDK_E_UNAVAIL;
        }

        /* Configure FX100 mode */
        ioerr += READ_SERDES_100FX_CTRLr(pc, &fx100_ctrl);
        SERDES_100FX_CTRLr_FX_SERDES_ENf_SET(fx100_ctrl, fx100 ? 1 : 0);
        if (fx100) {
            SERDES_100FX_CTRLr_FD_FX_SERDESf_SET(fx100_ctrl, 0);
        }
        ioerr += WRITE_SERDES_100FX_CTRLr(pc, fx100_ctrl);
        
        /* Configure fiber speed */
        ioerr += READ_MII_1000X_CTRLr(pc, &mii_1000x_ctrl);
        MII_1000X_CTRLr_SS_MSBf_SET(mii_1000x_ctrl, fx100 ? 0 : 1);
        MII_1000X_CTRLr_SS_LSBf_SET(mii_1000x_ctrl, fx100 ? 1 : 0);
        MII_1000X_CTRLr_FULL_DUPLEXf_SET(mii_1000x_ctrl, 1);
        ioerr += WRITE_MII_1000X_CTRLr(pc, mii_1000x_ctrl);
    } else {
        /* Disable loopback while setting copper speed */
        rv = PHY_LOOPBACK_GET(pc, &lb_enable);
        if (CDK_SUCCESS(rv) && lb_enable) {
            rv = PHY_LOOPBACK_SET(pc, 0);
        }

        /* Set copper speed */
        if (CDK_SUCCESS(rv)) {
            rv = ge_phy_speed_set(pc, speed);
        }

        /* Restore loopback setting */
        if (CDK_SUCCESS(rv) && lb_enable) {
            rv = PHY_LOOPBACK_SET(pc, lb_enable);
        }
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:    
 *      bcm54282_phy_speed_get
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
bcm54282_phy_speed_get(phy_ctrl_t *pc, uint32_t *speed)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t sp_mode, ctrl, anc, hcd;
    OPER_MODE_STATUSr_t oper_mode_status;
    MII_AUX_STATUSr_t mii_aux_status;

    PHY_CTRL_CHECK(pc);

    if (PHY_CTRL_FLAGS(pc) & PHY_F_FIBER_MODE) {
        ioerr  += READ_OPER_MODE_STATUSr(pc, &oper_mode_status);
        if (ioerr) {
            return CDK_E_IO;
        }
        sp_mode = OPER_MODE_STATUSr_SERDES_SPEEDf_GET(oper_mode_status);
        switch(sp_mode) {
        case 0: /* SERDES_SPEED_10 */
            *speed = 10;
            break;
        case 1: /* SERDES_SPEED_100 */
            *speed = 100;
            break;
        case 2: /* SERDES_SPEED_1000 */
            *speed = 1000;
            break;
        default:
            return CDK_E_UNAVAIL;
        }
    } else {
        ioerr += PHY_BUS_READ(pc, MII_CTRL_REG, &ctrl);
        ioerr += READ_MII_AUX_STATUSr(pc, &mii_aux_status);
        if (ioerr) {
            return CDK_E_IO;
        }

        if (ctrl & MII_CTRL_AE) {
            /* Auto-negotiation enabled */
            anc = MII_AUX_STATUSr_ANCf_GET(mii_aux_status);
            if (!anc) {
                /* Auto-neg NOT complete */
                *speed = 0;
            } else {
                hcd = MII_AUX_STATUSr_HCDf_GET(mii_aux_status);
                switch(hcd) {
                case 7: /* HCD_FD_1000 */
                case 6: /* HCD_HD_1000 */
                    *speed = 1000;
                    break;
                case 5: /* HCD_FD_100 */
                case 4: /* HCD_T4_100 */
                case 3: /* HCD_HD_100 */
                    *speed = 100;
                    break;
                case 2: /* HCD_FD_10 */
                case 1: /* HCD_HD_10 */
                    *speed = 10;
                    break;
                default:
                    *speed = 0;
                    break;
                }
            }
        } else {
            /* Auto-negotiation disabled. Simply pick up the values we force in CTRL register. */
            switch (MII_CTRL_SS(ctrl)) {
            case MII_CTRL_SS_10:
                *speed = 10;
                break;
            case MII_CTRL_SS_100:
                *speed = 100;
                break;
            case MII_CTRL_SS_1000:
                *speed = 1000;
                break;
            default:
                return CDK_E_UNAVAIL;
            }
        }
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:    
 *      bcm54282_phy_autoneg_set
 * Purpose:     
 *      Enable or disabled auto-negotiation on the specified port.
 * Parameters:
 *      pc - PHY control structure
 *      autoneg - non-zero enables autoneg, zero disables autoneg
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm54282_phy_autoneg_set(phy_ctrl_t *pc, int autoneg)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    SGMII_SLAVEr_t sgmii_slave;
    MII_1000X_CTRLr_t mii_1000x_ctrl;

    PHY_CTRL_CHECK(pc);

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
 *      bcm54282_phy_autoneg_get
 * Purpose:     
 *      Get the current auto-negotiation status (enabled/busy)
 * Parameters:
 *      pc - PHY control structure
 *      autoneg - (OUT) non-zero indicates autoneg enabled
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm54282_phy_autoneg_get(phy_ctrl_t *pc, int *autoneg)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t speed;
    MII_1000X_CTRLr_t mii_1000x_ctrl;

    PHY_CTRL_CHECK(pc);

    if (autoneg == NULL) {
        return CDK_E_PARAM;
    }

    *autoneg = 0;

    if (PHY_CTRL_FLAGS(pc) & PHY_F_FIBER_MODE) {
        rv = PHY_SPEED_GET(pc, &speed);
        if (CDK_FAILURE(rv)) {
            return rv;
        }
        if (speed != 100) {
            ioerr += READ_MII_1000X_CTRLr(pc, &mii_1000x_ctrl);
            *autoneg = MII_1000X_CTRLr_AUTONEGf_GET(mii_1000x_ctrl);
        }
    } else {
        rv = ge_phy_autoneg_get(pc, autoneg);
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:    
 *      bcm54282_phy_loopback_set
 * Purpose:     
 *      Set the internal PHY loopback mode.
 * Parameters:
 *      pc - PHY control structure
 *      enable - non-zero enables PHY loopback
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm54282_phy_loopback_set(phy_ctrl_t *pc, int enable)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int ge_lb, fe_lb;
    uint32_t speed;
    MII_1000X_CTRLr_t mii_1000x_ctrl;
    MII_CTRLr_t mii_ctrl;
    MII_GB_CTRLr_t mii_gb_ctrl;
    MII_AUX_CTRLr_t mii_aux_ctrl;
    COPPER_MISC_TESTr_t copper_misc_test;

    PHY_CTRL_CHECK(pc);

    if (enable) {
        enable = 1;
    }

    if (PHY_CTRL_FLAGS(pc) & PHY_F_FIBER_MODE) {
        ioerr += READ_MII_1000X_CTRLr(pc, &mii_1000x_ctrl);
        MII_1000X_CTRLr_LOOPBACKf_SET(mii_1000x_ctrl, enable);
        ioerr += WRITE_MII_1000X_CTRLr(pc, mii_1000x_ctrl);
    } else {
        /* GE and FE use different loopback registers */
        ge_lb = enable;
        fe_lb = enable;
        ioerr += PHY_SPEED_GET(pc, &speed);
        if (speed == 1000) {
            fe_lb = 0;
        } else {
            ge_lb = 0;
        }
        
        /* Loopback setting for 1000M */
        ioerr += READ_MII_CTRLr(pc, &mii_ctrl);
        MII_CTRLr_LOOPBACKf_SET(mii_ctrl, ge_lb);
        ioerr += WRITE_MII_CTRLr(pc, mii_ctrl);
        
        /* Loopback setting for 100/10M */
        ioerr += READ_MII_GB_CTRLr(pc, &mii_gb_ctrl);
        MII_GB_CTRLr_MASTERf_SET(mii_gb_ctrl, fe_lb);
        MII_GB_CTRLr_MS_MANf_SET(mii_gb_ctrl, fe_lb);
        ioerr += WRITE_MII_GB_CTRLr(pc, mii_gb_ctrl);
                
        ioerr += READ_MII_AUX_CTRLr(pc, &mii_aux_ctrl);
        MII_AUX_CTRLr_EXT_LPBKf_SET(mii_aux_ctrl, fe_lb);
        ioerr += WRITE_MII_AUX_CTRLr(pc, mii_aux_ctrl);
        
        ioerr += READ_COPPER_MISC_TESTr(pc, &copper_misc_test);
        COPPER_MISC_TESTr_SWAP_RXMDIXf_SET(copper_misc_test, fe_lb);
        ioerr += WRITE_COPPER_MISC_TESTr(pc, copper_misc_test);
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:    
 *      bcm54282_phy_loopback_get
 * Purpose:     
 *      Get the local PHY loopback mode.
 * Parameters:
 *      pc - PHY control structure
 *      enable - (OUT) non-zero indicates PHY loopback enabled
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm54282_phy_loopback_get(phy_ctrl_t *pc, int *enable)
{
    int ioerr = 0;
    MII_1000X_CTRLr_t mii_1000x_ctrl;
    COPPER_MISC_TESTr_t copper_misc_test;

    PHY_CTRL_CHECK(pc);

    if (PHY_CTRL_FLAGS(pc) & PHY_F_FIBER_MODE) {
        ioerr += READ_MII_1000X_CTRLr(pc, &mii_1000x_ctrl);
        *enable = MII_1000X_CTRLr_LOOPBACKf_GET(mii_1000x_ctrl) ? 1 : 0;
    } else {
        ioerr += ge_phy_loopback_get(pc, enable);
        
        if (!*enable) {
            ioerr += READ_COPPER_MISC_TESTr(pc, &copper_misc_test);
            *enable = COPPER_MISC_TESTr_SWAP_RXMDIXf_GET(copper_misc_test);
        }
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

/*
 * Function:    
 *      bcm54282_phy_ability_get
 * Purpose:     
 *      Get the abilities of the PHY.
 * Parameters:
 *      pc - PHY control structure
 *      abil - (OUT) ability mask indicating supported options/speeds.
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm54282_phy_ability_get(phy_ctrl_t *pc, uint32_t *abil)
{
    PHY_CTRL_CHECK(pc);

    *abil = (PHY_ABIL_1000MB | PHY_ABIL_100MB | PHY_ABIL_10MB | 
             PHY_ABIL_LOOPBACK | PHY_ABIL_SGMII);
    
    return CDK_E_NONE;
}

/*
 * Function:
 *      bcm54282_phy_config_set
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
bcm54282_phy_config_set(phy_ctrl_t *pc, phy_config_t cfg, uint32_t val, void *cd)
{
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
    case PhyConfig_RemoteLoopback: {
        int ioerr = 0;
        MII_CTRLr_t mii_ctrl;
        COPPER_MISC_TESTr_t copper_misc_test;

        /* Used as field value */
        if (val) {
            val = 1;
        }
        /* Set copper line-side loopback enable */
        ioerr += READ_COPPER_MISC_TESTr(pc, &copper_misc_test);
        COPPER_MISC_TESTr_RMT_LPBK_ENf_SET(copper_misc_test, val);
        ioerr += WRITE_COPPER_MISC_TESTr(pc, copper_misc_test);
        /* Restart auto-negotiation */
        ioerr += READ_MII_CTRLr(pc, &mii_ctrl);
        MII_CTRLr_RESTART_ANf_SET(mii_ctrl, 1);
        ioerr += WRITE_MII_CTRLr(pc, mii_ctrl);
        return ioerr ? CDK_E_IO : CDK_E_NONE;
    }
    case PhyConfig_TempMon: {
        int ioerr = 0;
        int orig_inst, pwrdn;
        VOLT_TEMP_MON_CTRLr_t vt_mon_ctrl;

        if (val) {
            pwrdn = 0;
        } else {
            pwrdn = 1;
        }

        /* Get primary PHY instance */
        orig_inst = _bcm54282_inst(pc);
        /* Switch to instance 0 to enable VTMonitor configuration */
        if (phy_ctrl_change_inst(pc, 0, _bcm54282_inst) < 0) {
            return CDK_E_FAIL;
        }

        ioerr += READ_VOLT_TEMP_MON_CTRLr(pc, &vt_mon_ctrl);
        /* Only support temperature now */
        VOLT_TEMP_MON_CTRLr_VTMON_PWRDNf_SET(vt_mon_ctrl, pwrdn);
        VOLT_TEMP_MON_CTRLr_VTMON_SELf_SET(vt_mon_ctrl, 0);
        VOLT_TEMP_MON_CTRLr_VTMON_VOLT_MODEf_SET(vt_mon_ctrl, 0);
        ioerr += WRITE_VOLT_TEMP_MON_CTRLr(pc, vt_mon_ctrl);

        /* Restore instance */
        (void)phy_ctrl_change_inst(pc, orig_inst, _bcm54282_inst);

        return ioerr ? CDK_E_IO : CDK_E_NONE;
    }
    default:
        break;
    }

    return CDK_E_UNAVAIL;
}

/*
 * Function:
 *      bcm54282_phy_config_get
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
bcm54282_phy_config_get(phy_ctrl_t *pc, phy_config_t cfg, uint32_t *val, void *cd)
{
    PHY_CTRL_CHECK(pc);

    switch (cfg) {
    case PhyConfig_Enable:
        *val = 1;
        return CDK_E_NONE;
    case PhyConfig_PortInterface:
        *val = PHY_IF_GMII;
        return CDK_E_NONE;
    case PhyConfig_RemoteLoopback: {
        int ioerr = 0;
        COPPER_MISC_TESTr_t copper_misc_test;

        /* Get copper line-side loopback enable status */
        ioerr += READ_COPPER_MISC_TESTr(pc, &copper_misc_test);
        *val = COPPER_MISC_TESTr_RMT_LPBK_ENf_GET(copper_misc_test);
        return ioerr ? CDK_E_IO : CDK_E_NONE;
    }
    case PhyConfig_TempMon: {
        int ioerr = 0;
        int orig_inst, pwrdn, vtm_sel;
        VOLT_TEMP_MON_CTRLr_t vt_mon_ctrl;

        /* Get primary PHY instance */
        orig_inst = _bcm54282_inst(pc);
        /* Switch to instance 0 to enable VTMonitor configuration */
        if (phy_ctrl_change_inst(pc, 0, _bcm54282_inst) < 0) {
            return CDK_E_FAIL;
        }
        
        ioerr  += READ_VOLT_TEMP_MON_CTRLr(pc, &vt_mon_ctrl);
        pwrdn   = VOLT_TEMP_MON_CTRLr_VTMON_PWRDNf_GET(vt_mon_ctrl);
        vtm_sel = VOLT_TEMP_MON_CTRLr_VTMON_SELf_GET(vt_mon_ctrl);

        /* Restore instance */
        (void)phy_ctrl_change_inst(pc, orig_inst, _bcm54282_inst);

        /* Only support temperature now */
        if ((pwrdn == 0) && ((vtm_sel == 0) || (vtm_sel == 3))) {
            *val = 1;
        } else {
            *val = 0;
        }

        return ioerr ? CDK_E_IO : CDK_E_NONE;
    }
    default:
        break;
    }

    return CDK_E_UNAVAIL;
}

/*
 * Function:
 *      bcm54282_phy_status_get
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
bcm54282_phy_status_get(phy_ctrl_t *pc, phy_status_t st, uint32_t *val)
{
    PHY_CTRL_CHECK(pc);

    switch (st) {
    case PhyStatus_Temperature: {
        int ioerr = 0;
        int pwrdn, data, orig_inst;
        VOLT_TEMP_MON_CTRLr_t vt_mon_ctrl;
        TEMP_MON_VALUEr_t temp_mon_val;

        /* Get primary PHY instance */
        orig_inst = _bcm54282_inst(pc);
        /* Switch to instance 0 to enable VTMonitor configuration */
        if (phy_ctrl_change_inst(pc, 0, _bcm54282_inst) < 0) {
            return CDK_E_FAIL;
        }
        
        ioerr += READ_VOLT_TEMP_MON_CTRLr(pc, &vt_mon_ctrl);
        pwrdn  = VOLT_TEMP_MON_CTRLr_VTMON_PWRDNf_GET(vt_mon_ctrl);
        ioerr += READ_TEMP_MON_VALUEr(pc, &temp_mon_val);
        data   = TEMP_MON_VALUEr_DATAf_GET(temp_mon_val);

        /* Restore instance */
        (void)phy_ctrl_change_inst(pc, orig_inst, _bcm54282_inst);

        if ((pwrdn == 1) || (data == 0)) {
            *val = 0;
        } else {
            *val = (4180000 - 5556 * data) / 10000;
        }

        return ioerr ? CDK_E_IO : CDK_E_NONE;
    }
    default:
        break;
    }

    return CDK_E_UNAVAIL;
}

/*
 * Variable:    bcm54282_phy drv
 * Purpose:     PHY Driver for BCM54282.
 */
phy_driver_t bcm54282_drv = {
    "bcm54282",
    "BCM54282 Gigabit PHY Driver",  
    0,
    bcm54282_phy_probe,                 /* pd_probe */
    bcm54282_phy_notify,                /* pd_notify */
    bcm54282_phy_reset,                 /* pd_reset */
    bcm54282_phy_init,                  /* pd_init */
    bcm54282_phy_link_get,              /* pd_link_get */
    bcm54282_phy_duplex_set,            /* pd_duplex_set */
    bcm54282_phy_duplex_get,            /* pd_duplex_get */
    bcm54282_phy_speed_set,             /* pd_speed_set */
    bcm54282_phy_speed_get,             /* pd_speed_get */
    bcm54282_phy_autoneg_set,           /* pd_autoneg_set */
    bcm54282_phy_autoneg_get,           /* pd_autoneg_get */
    bcm54282_phy_loopback_set,          /* pd_loopback_set */
    bcm54282_phy_loopback_get,          /* pd_loopback_get */
    bcm54282_phy_ability_get,           /* pd_ability_get */
    bcm54282_phy_config_set,            /* pd_config_set */
    bcm54282_phy_config_get,            /* pd_config_get */
    bcm54282_phy_status_get,            /* pd_status_get */
    NULL                                /* pd_cable_diag */
};
