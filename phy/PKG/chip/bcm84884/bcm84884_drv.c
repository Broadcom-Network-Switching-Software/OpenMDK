/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * PHY driver for BCM84884.
 */

#include <phy/phy.h>
#include <phy/phy_drvlist.h>

#include <phy/ge_phy.h>
#include <cdk/cdk_debug.h>

#include <phy/chip/bcm84888_defs.h>

#define IS_COPPER_MODE(pc)      (!(PHY_CTRL_FLAGS(pc) & PHY_F_FIBER_MODE))
#define IS_FIBER_MODE(pc)       (PHY_CTRL_FLAGS(pc) & PHY_F_FIBER_MODE)

/* Private PHY flag is used to indicate firmware download method */
#define PHY_F_FW_MDIO_LOAD                      PHY_F_PRIVATE

#define PHY_ID1_REV_MASK                        0x000f

#define BCM84884_PMA_PMD_ID0                    0xae02
#define BCM84884_PMA_PMD_ID1                    0x5141

/***********************************************************************
 *
 * HELPER FUNCTIONS
 */

/*
 * Function:
 *      _bcm84884_ability_local_get
 * Purpose:
 *      Get the local abilities of the PHY.
 * Parameters:
 *      pc - PHY control structure
 *      abil - (OUT) ability mask indicating supported options/speeds.
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84884_ability_local_get(phy_ctrl_t *pc, uint32_t *ability)
{
    if (!ability) {
        return CDK_E_PARAM;
    }

    *ability = (PHY_ABIL_100MB_HD | PHY_ABIL_10MB_HD |
                PHY_ABIL_1000MB_FD | PHY_ABIL_1000MB_HD |
                PHY_ABIL_100MB_FD | PHY_ABIL_PAUSE | PHY_ABIL_PAUSE_ASYMM |
                PHY_ABIL_LOOPBACK | PHY_ABIL_AN);

    return CDK_E_NONE;
}


/*
 * Function:
 *      _bcm84884_ability_remote_get
 * Purpose:
 *      Get the current remoteisement for auto-negotiation.
 * Parameters:
 *      pc - PHY control structure
 *      abil - (OUT) ability mask indicating supported options/speeds.
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84884_copper_ability_remote_get(phy_ctrl_t *pc, uint32_t *ability)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    MII_STATr_t mii_stat;
    MII_CTRLr_t mii_ctrl;
    MII_ANPr_t mii_anp;
    MII_GB_STATr_t mii_gb_stat;
    TENG_AN_STATr_t teng_stat;
    uint32_t an_done, link;

    if (!ability) {
        return CDK_E_PARAM;
    }
    *ability = 0;

    ioerr += READ_MII_STATr(pc, &mii_stat);
    ioerr += READ_MII_CTRLr(pc, &mii_ctrl);
    an_done = MII_STATr_AUTONEG_COMPLETEf_GET(mii_stat);
    link = MII_STATr_LINK_STATUSf_GET(mii_stat);
    if (an_done && link) {
        /* Decode remote advertisement only when link is up and autoneg is completed. */
        ioerr += READ_MII_ANPr(pc, &mii_anp);
        if (MII_ANPr_X100BASE_TX_HALF_DUP_CAPf_GET(mii_anp)) {
            *ability |= PHY_ABIL_100MB_HD;
        }
        if (MII_ANPr_X100BASE_TX_FULL_DUP_CAPf_GET(mii_anp)) {
            *ability |= PHY_ABIL_100MB_FD;
        }
        if (MII_ANPr_X10BASE_T_HALF_DUP_CAPf_GET(mii_anp) ||
            MII_ANPr_X10BASE_T_FULL_DUP_CAPf_GET(mii_anp)) {
            rv = CDK_E_PARAM;
        }

        if (MII_ANPr_PAUSE_CAPABLEf_GET(mii_anp) == 1 &&
            MII_ANPr_LINK_PRTNR_ASYM_PAUSEf_GET(mii_anp) == 0) {
            *ability |= (PHY_ABIL_PAUSE_TX | PHY_ABIL_PAUSE_RX);
        } else if (MII_ANPr_PAUSE_CAPABLEf_GET(mii_anp) == 0 &&
            MII_ANPr_LINK_PRTNR_ASYM_PAUSEf_GET(mii_anp) == 1) {
            *ability |= PHY_ABIL_PAUSE_TX;
        } else if (MII_ANPr_PAUSE_CAPABLEf_GET(mii_anp) == 1 &&
            MII_ANPr_LINK_PRTNR_ASYM_PAUSEf_GET(mii_anp) == 1) {
            *ability |= PHY_ABIL_PAUSE_RX;
        }

        /* GE Specific values */
        ioerr += READ_MII_GB_STATr(pc, &mii_gb_stat);
        if (MII_GB_STATr_LNK_PART_HALF_DUP_ABLEf_GET(mii_gb_stat)) {
            *ability |= PHY_ABIL_1000MB_HD;
        }
        if (MII_GB_STATr_LNK_PART_FULL_DUP_ABLEf_GET(mii_gb_stat)) {
            *ability |= PHY_ABIL_1000MB_FD;
        }

        /* 10G Specific values */
        ioerr += READ_TENG_AN_STATr(pc, &teng_stat);
        if (TENG_AN_STATr_LP_10GBTf_GET(teng_stat)) {
            *ability |= PHY_ABIL_10GB;
        } else {
            *ability &= ~PHY_ABIL_10GB;
        }
    }

    return ioerr ? CDK_E_IO : rv;
}


static int
_bcm84884_ability_remote_get(phy_ctrl_t *pc, uint32_t *ability)
{
    int rv = CDK_E_NONE;

    rv = _bcm84884_copper_ability_remote_get(pc, ability);

    return rv;
}


/*
 * Function:
 *      _bcm84884_copper_ability_advert_set
 * Purpose:
 *      Set the current advertisement for auto-negotiation.
 * Parameters:
 *      pc - PHY control structure
 *      ability - ability mask indicating supported options/speeds.
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84884_copper_ability_advert_set(phy_ctrl_t *pc, uint32_t ability)
{
    int ioerr = 0;
    MII_GB_CTRLr_t gb_ctrl;
    MII_ANAr_t mii_ana;
    TENG_AN_CTRLr_t an_ctrl;
    MGBASE_AN_CTRLr_t mg_an_ctrl;

    ioerr += READ_MII_GB_CTRLr(pc, &gb_ctrl);
    MII_GB_CTRLr_ADV_1000BASE_T_HALF_DUPLEXf_SET(gb_ctrl, 0);
    MII_GB_CTRLr_ADV_1000BASE_T_FULL_DUPLEXf_SET(gb_ctrl, 0);
    MII_GB_CTRLr_REPEATER_DTEf_SET(gb_ctrl, 1);

    ioerr += READ_MII_ANAr(pc, &mii_ana);
    if (ability & PHY_ABIL_100MB_HD) {
        MII_ANAr_X100BASE_TX_HALF_DUP_CAPf_SET(mii_ana, 1);
    }

    if (ability & PHY_ABIL_100MB_FD) {
        MII_ANAr_X100BASE_TX_FULL_DUP_CAPf_SET(mii_ana, 1);
    }

    if (ability & PHY_ABIL_1000MB_HD) {
        MII_GB_CTRLr_ADV_1000BASE_T_HALF_DUPLEXf_SET(gb_ctrl, 1);
    }
    if (ability & PHY_ABIL_1000MB_FD) {
        MII_GB_CTRLr_ADV_1000BASE_T_FULL_DUPLEXf_SET(gb_ctrl, 1);
    }

    ioerr += READ_TENG_AN_CTRLr(pc, &an_ctrl);
    TENG_AN_CTRLr_X10GBTf_SET(an_ctrl, 0);
    TENG_AN_CTRLr_BRSAf_SET(an_ctrl, 0);
    ioerr += WRITE_TENG_AN_CTRLr(pc, an_ctrl);

    ioerr += READ_MGBASE_AN_CTRLr(pc, &mg_an_ctrl);
    MGBASE_AN_CTRLr_ADV_2P5Gf_SET(mg_an_ctrl, 0);
    MGBASE_AN_CTRLr_MG_ENABLEf_SET(mg_an_ctrl, 0);
    ioerr += WRITE_MGBASE_AN_CTRLr(pc, mg_an_ctrl);

    switch (ability & (PHY_ABIL_PAUSE_TX | PHY_ABIL_PAUSE_RX)) {
    case PHY_ABIL_PAUSE_TX:
        MII_ANAr_ASYMMETRIC_PAUSEf_SET(mii_ana, 1);
        break;
    case PHY_ABIL_PAUSE_RX:
        MII_ANAr_PAUSE_CAPABLEf_SET(mii_ana, 1);
        MII_ANAr_ASYMMETRIC_PAUSEf_SET(mii_ana, 1);
        break;
    case (PHY_ABIL_PAUSE_TX | PHY_ABIL_PAUSE_RX):
        MII_ANAr_PAUSE_CAPABLEf_SET(mii_ana, 1);
        break;
    }
    ioerr += WRITE_MII_ANAr(pc, mii_ana);
    ioerr += WRITE_MII_GB_CTRLr(pc, gb_ctrl);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}


static int
_bcm84884_fiber_ability_advert_set(phy_ctrl_t *pc, uint32_t ability)
{
    int ioerr = 0;
    COMBO_IEEE0_ADVr_t combo_adv;
    OVER1G_UP1r_t ovr1g;

    ioerr += READ_COMBO_IEEE0_ADVr(pc, &combo_adv);
    if (ability & PHY_ABIL_1000MB_FD) {
        COMBO_IEEE0_ADVr_C37_FDf_SET(combo_adv, 1);
    }
    switch (ability & (PHY_ABIL_PAUSE_TX | PHY_ABIL_PAUSE_RX)) {
    case PHY_ABIL_PAUSE_TX:
        COMBO_IEEE0_ADVr_C37_ASYM_PAUSEf_SET(combo_adv, 1);
        break;
    case PHY_ABIL_PAUSE_RX:
        COMBO_IEEE0_ADVr_C37_ASYM_PAUSEf_SET(combo_adv, 1);
        COMBO_IEEE0_ADVr_C37_PAUSEf_SET(combo_adv, 1);
        break;
    case PHY_ABIL_PAUSE_TX | PHY_ABIL_PAUSE_RX:
        COMBO_IEEE0_ADVr_C37_PAUSEf_SET(combo_adv, 1);
        break;
    }
    ioerr += WRITE_COMBO_IEEE0_ADVr(pc, combo_adv);

    ioerr += READ_OVER1G_UP1r(pc, &ovr1g);
    OVER1G_UP1r_TENGCX4f_SET(ovr1g, 0);
    ioerr += WRITE_OVER1G_UP1r(pc, ovr1g);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

static int
_bcm84884_ability_advert_set(phy_ctrl_t *pc, uint32_t ability)
{
    int rv = CDK_E_NONE;

    if (IS_COPPER_MODE(pc)) {
        rv = _bcm84884_copper_ability_advert_set(pc, ability);
    } else {
        rv = _bcm84884_fiber_ability_advert_set(pc, ability);
    }

    return rv;
}


/*
 * Function:
 *      _bcm84884_ability_advert_get
 * Purpose:
 *      Get the current advertisement for auto-negotiation.
 * Parameters:
 *      pc - PHY control structure
 *      ability - ability mask indicating supported options/speeds.
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84884_copper_ability_advert_get(phy_ctrl_t *pc, uint32_t *ability)
{
    int ioerr = 0;
    MII_ANAr_t mii_ana;
    MII_GB_CTRLr_t mii_gb_ctrl;

    if (!ability) {
        return CDK_E_PARAM;
    }
    *ability = 0;

    /* Copper AutoNeg Advertisement Reg.(7.0xFFE4) */
    ioerr += READ_MII_ANAr(pc, &mii_ana);

    if (MII_ANAr_X100BASE_TX_HALF_DUP_CAPf_GET(mii_ana)) {
        *ability |= PHY_ABIL_100MB_HD;
    }
    if (MII_ANAr_X100BASE_TX_FULL_DUP_CAPf_GET(mii_ana)) {
        *ability |= PHY_ABIL_100MB_FD;
    }

    if (MII_ANAr_X10BASE_T_HALF_DUP_CAPf_GET(mii_ana) ||
        MII_ANAr_X10BASE_T_FULL_DUP_CAPf_GET(mii_ana)) {
        return CDK_E_PARAM;
    }

    if (MII_ANAr_PAUSE_CAPABLEf_GET(mii_ana) == 1 &&
        MII_ANAr_ASYMMETRIC_PAUSEf_GET(mii_ana) == 0) {
        *ability |= (PHY_ABIL_PAUSE_TX | PHY_ABIL_PAUSE_RX);
    } else if (MII_ANAr_PAUSE_CAPABLEf_GET(mii_ana) == 0 &&
        MII_ANAr_ASYMMETRIC_PAUSEf_GET(mii_ana) == 1) {
        *ability |= PHY_ABIL_PAUSE_TX;
    } else if (MII_ANAr_PAUSE_CAPABLEf_GET(mii_ana) == 1 &&
        MII_ANAr_ASYMMETRIC_PAUSEf_GET(mii_ana) == 1) {
        *ability |= PHY_ABIL_PAUSE_RX;
    }

    /* GE Specific values */
    /* 1000BASE-T Control Reg.(7.0xFFE9) */
    ioerr += READ_MII_GB_CTRLr(pc, &mii_gb_ctrl);
    if (MII_GB_CTRLr_ADV_1000BASE_T_HALF_DUPLEXf_GET(mii_gb_ctrl)) {
        *ability |= PHY_ABIL_1000MB_HD;
    }
    if (MII_GB_CTRLr_ADV_1000BASE_T_FULL_DUPLEXf_GET(mii_gb_ctrl)) {
        *ability |= PHY_ABIL_1000MB_FD;
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

STATIC int
_bcm84884_fiber_ability_advert_get(phy_ctrl_t *pc, uint32_t *ability)
{
    int ioerr = 0;
    COMBO_IEEE0_ADVr_t combo_adv;

    if (!ability) {
        return CDK_E_PARAM;
    }
    *ability = 0;

    ioerr += READ_COMBO_IEEE0_ADVr(pc, &combo_adv);
    if (COMBO_IEEE0_ADVr_C37_ASYM_PAUSEf_GET(combo_adv) == 1 &&
        COMBO_IEEE0_ADVr_C37_PAUSEf_GET(combo_adv) == 0) {
        *ability |= PHY_ABIL_PAUSE_TX;
    } else if (COMBO_IEEE0_ADVr_C37_ASYM_PAUSEf_GET(combo_adv) == 0 &&
        COMBO_IEEE0_ADVr_C37_PAUSEf_GET(combo_adv) == 1) {
        *ability |= (PHY_ABIL_PAUSE_TX | PHY_ABIL_PAUSE_RX);
    } else if (COMBO_IEEE0_ADVr_C37_ASYM_PAUSEf_GET(combo_adv) == 1 &&
        COMBO_IEEE0_ADVr_C37_PAUSEf_GET(combo_adv) == 1) {
        *ability |= PHY_ABIL_PAUSE_RX;
    }

    if (COMBO_IEEE0_ADVr_C37_FDf_GET(combo_adv)) {
        *ability |= PHY_ABIL_1000MB_FD;
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}


/***********************************************************************
 *
 * PHY DRIVER FUNCTIONS
 */

#if PHY_CONFIG_INCLUDE_CHIP_SYMBOLS == 1
extern cdk_symbols_t bcm84888_symbols;
#define SET_SYMBOL_TABLE(_pc) \
    PHY_CTRL_SYMBOLS(_pc) = &bcm84888_symbols
#else
#define SET_SYMBOL_TABLE(_pc)
#endif

/*
 * Function:
 *      bcm84884_phy_probe
 * Purpose:
 *      Probe for 84884 PHY
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
bcm84884_phy_probe(phy_ctrl_t *pc)
{
    int ioerr = 0;
    PMAD_IEEE_DEV_ID0r_t dev_id0;
    PMAD_IEEE_DEV_ID1r_t dev_id1;
    uint32_t phyid0, phyid1;
    uint32_t rev;

    PHY_CTRL_CHECK(pc);

    ioerr += READ_PMAD_IEEE_DEV_ID0r(pc, &dev_id0);
    ioerr += READ_PMAD_IEEE_DEV_ID1r(pc, &dev_id1);
    if (ioerr) {
        return CDK_E_IO;
    }
    phyid0 = PMAD_IEEE_DEV_ID0r_GET(dev_id0);
    phyid1 = PMAD_IEEE_DEV_ID1r_GET(dev_id1) & ~PHY_ID1_REV_MASK;
    rev = PMAD_IEEE_DEV_ID1r_REVf_GET(dev_id1);
    if (phyid0 == BCM84884_PMA_PMD_ID0 &&
       (phyid1 == (BCM84884_PMA_PMD_ID1 & ~PHY_ID1_REV_MASK)) &&
       ((rev == 0x8) || (rev == 0x9)) ) {
#if PHY_CONFIG_EXTERNAL_BOOT_ROM == 0
        /* Use MDIO download if external ROM is disabled */
        PHY_CTRL_FLAGS(pc) |= PHY_F_FW_MDIO_LOAD;
#endif

        SET_SYMBOL_TABLE(pc);
        return ioerr ? CDK_E_IO : CDK_E_NONE;

    }
    return CDK_E_NOT_FOUND;
}

/*
 * Function:
 *      bcm84884_phy_notify
 * Purpose:
 *      Handle PHY notifications
 * Parameters:
 *      pc - PHY control structure
 *      event - PHY event
 * Returns:
 *      CDK_E_xxx
 */
static int
bcm84884_phy_notify(phy_ctrl_t *pc, phy_event_t event)
{
    PHY_CTRL_CHECK(pc);

    return bcm84888_drv.pd_notify(pc, event);
}

/*
 * Function:
 *      bcm84884_phy_reset
 * Purpose:
 *      Reset 84884 PHY
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
bcm84884_phy_reset(phy_ctrl_t *pc)
{
    PHY_CTRL_CHECK(pc);

    return bcm84888_drv.pd_reset(pc);
}

/*
 * Function:
 *      bcm84884_phy_init
 * Purpose:
 *      Initialize 84884 PHY driver
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
bcm84884_phy_init(phy_ctrl_t *pc)
{
    int rv = CDK_E_NONE;
    uint32_t ability;

    PHY_CTRL_CHECK(pc);

    rv = bcm84888_drv.pd_init(pc);

    /* Update the ability advertisement */
    rv = _bcm84884_ability_local_get(pc, &ability);

    if (CDK_SUCCESS(rv)) {
        rv = _bcm84884_ability_advert_set(pc, ability);
    }

    return rv;
}

/*
 * Function:
 *      bcm84884_phy_link_get
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
bcm84884_phy_link_get(phy_ctrl_t *pc, int *link, int *autoneg_done)
{
    PHY_CTRL_CHECK(pc);

    return bcm84888_drv.pd_link_get(pc, link, autoneg_done);
}

/*
 * Function:
 *      bcm84884_phy_duplex_set
 * Purpose:
 *      Set the current duplex mode (forced).
 * Parameters:
 *      pc - PHY control structure
 *      duplex - non-zero indicates full duplex, zero indicates half
 * Returns:
 *      CDK_E_xxx
 */
static int
bcm84884_phy_duplex_set(phy_ctrl_t *pc, int duplex)
{
    PHY_CTRL_CHECK(pc);

    return bcm84888_drv.pd_duplex_set(pc, duplex);
}

/*
 * Function:
 *      bcm84884_phy_duplex_get
 * Purpose:
 *      Get the current operating duplex mode.
 * Parameters:
 *      pc - PHY control structure
 *      duplex - (OUT) non-zero indicates full duplex, zero indicates half
 * Returns:
 *      CDK_E_xxx
 */
static int
bcm84884_phy_duplex_get(phy_ctrl_t *pc, int *duplex)
{
    PHY_CTRL_CHECK(pc);

    return bcm84888_drv.pd_duplex_get(pc, duplex);
}

/*
 * Function:
 *      bcm84884_phy_speed_set
 * Purpose:
 *      Set the current operating speed (forced).
 * Parameters:
 *      pc - PHY control structure
 *      speed - new link speed
 * Returns:
 *      CDK_E_xxx
 */
static int
bcm84884_phy_speed_set(phy_ctrl_t *pc, uint32_t speed)
{
    PHY_CTRL_CHECK(pc);

    return bcm84888_drv.pd_speed_set(pc, speed);
}

/*
 * Function:
 *      bcm84884_phy_speed_get
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
bcm84884_phy_speed_get(phy_ctrl_t *pc, uint32_t *speed)
{
    PHY_CTRL_CHECK(pc);

    return bcm84888_drv.pd_speed_get(pc, speed);}

/*
 * Function:
 *      bcm84884_phy_autoneg_set
 * Purpose:
 *      Enable or disable auto-negotiation on the specified port.
 * Parameters:
 *      pc - PHY control structure
 *      autoneg - non-zero enables autoneg, zero disables autoneg
 * Returns:
 *      CDK_E_xxx
 */
static int
bcm84884_phy_autoneg_set(phy_ctrl_t *pc, int autoneg)
{
    PHY_CTRL_CHECK(pc);

    return bcm84888_drv.pd_autoneg_set(pc, autoneg);
}

/*
 * Function:
 *      bcm84884_phy_autoneg_get
 * Purpose:
 *      Get the current auto-negotiation setting.
 * Parameters:
 *      pc - PHY control structure
 *      autoneg - (OUT) non-zero indicates autoneg enabled
 * Returns:
 *      CDK_E_xxx
 */
static int
bcm84884_phy_autoneg_get(phy_ctrl_t *pc, int *autoneg)
{
    PHY_CTRL_CHECK(pc);

    return bcm84888_drv.pd_autoneg_get(pc, autoneg);
}

/*
 * Function:
 *      bcm84884_phy_loopback_set
 * Purpose:
 *      Set the internal PHY loopback mode.
 * Parameters:
 *      pc - PHY control structure
 *      enable - non-zero enables PHY loopback
 * Returns:
 *      CDK_E_xxx
 */
static int
bcm84884_phy_loopback_set(phy_ctrl_t *pc, int enable)
{
    PHY_CTRL_CHECK(pc);

    return bcm84888_drv.pd_loopback_set(pc, enable);}

/*
 * Function:
 *      bcm84884_phy_loopback_get
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
bcm84884_phy_loopback_get(phy_ctrl_t *pc, int *enable)
{
    PHY_CTRL_CHECK(pc);

    return bcm84888_drv.pd_loopback_get(pc, enable);}

/*
 * Function:
 *      bcm84884_phy_ability_get
 * Purpose:
 *      Get the abilities of the PHY.
 * Parameters:
 *      pc - PHY control structure
 *      ability - (OUT) ability mask indicating supported options/speeds.
 * Returns:
 *      CDK_E_xxx
 */
static int
bcm84884_phy_ability_get(phy_ctrl_t *pc, uint32_t *ability)
{
    int rv = CDK_E_NONE;

    PHY_CTRL_CHECK(pc);

    if (IS_COPPER_MODE(pc)) { /* Connected at copper mode */
        rv = _bcm84884_copper_ability_advert_get(pc, ability);
    } else { /* Connected at fiber mode */
        rv = _bcm84884_fiber_ability_advert_get(pc, ability);
    }

    return rv;
}

/*
 * Function:
 *      bcm84884_phy_config_set
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
bcm84884_phy_config_set(phy_ctrl_t *pc, phy_config_t cfg, uint32_t val, void *cd)
{
    PHY_CTRL_CHECK(pc);

    return bcm84888_drv.pd_config_set(pc, cfg, val, cd);
}

/*
 * Function:
 *      bcm84884_phy_config_get
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
bcm84884_phy_config_get(phy_ctrl_t *pc, phy_config_t cfg, uint32_t *val, void *cd)
{
    PHY_CTRL_CHECK(pc);

    switch (cfg) {
    case PhyConfig_AdvLocal:
        return _bcm84884_ability_local_get(pc, val);
    case PhyConfig_AdvRemote:
        return _bcm84884_ability_remote_get(pc, val);
    default:
        break;
    }

    return bcm84888_drv.pd_config_get(pc, cfg, val, cd);
}

/*
 * Function:
 *      bcm84884_phy_status_get
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
bcm84884_phy_status_get(phy_ctrl_t *pc, phy_status_t st, uint32_t *val)
{
    PHY_CTRL_CHECK(pc);

    return bcm84888_drv.pd_status_get(pc, st, val);
}

/*
 * Variable:    bcm84884_drv
 * Purpose:     PHY Driver for BCM84884.
 */
phy_driver_t bcm84884_drv = {
    "bcm84884",
    "BCM84884 1GbE PHY Driver",
    0,
    bcm84884_phy_probe,                  /* pd_probe */
    bcm84884_phy_notify,                 /* pd_notify */
    bcm84884_phy_reset,                  /* pd_reset */
    bcm84884_phy_init,                   /* pd_init */
    bcm84884_phy_link_get,               /* pd_link_get */
    bcm84884_phy_duplex_set,             /* pd_duplex_set */
    bcm84884_phy_duplex_get,             /* pd_duplex_get */
    bcm84884_phy_speed_set,              /* pd_speed_set */
    bcm84884_phy_speed_get,              /* pd_speed_get */
    bcm84884_phy_autoneg_set,            /* pd_autoneg_set */
    bcm84884_phy_autoneg_get,            /* pd_autoneg_get */
    bcm84884_phy_loopback_set,           /* pd_loopback_set */
    bcm84884_phy_loopback_get,           /* pd_loopback_get */
    bcm84884_phy_ability_get,            /* pd_ability_get */
    bcm84884_phy_config_set,             /* pd_config_set */
    bcm84884_phy_config_get,             /* pd_config_get */
    bcm84884_phy_status_get,             /* pd_status_get */
    NULL                                 /* pd_cable_diag */
};
