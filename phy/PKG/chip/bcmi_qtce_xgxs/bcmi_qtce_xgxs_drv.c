/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * PHY driver for Broadcom QTC/Eagle QSGMII internal SerDes.
 */

#include <phy/phy.h>
#include <phy/phy_drvlist.h>
#include <phymod/phymod.h>
#include <phy/chip/bcmi_qtce_xgxs_defs.h>

#include "qtce/qtce.h"

/* Low level debugging (off by default) */
#ifdef PHY_DEBUG_ENABLE
#define _PHY_DBG(_pc, _stuff) \
    PHY_VERB(_pc, _stuff)
#else
#define _PHY_DBG(_pc, _stuff)
#endif

#define IS_1LANE_PORT(_pc) \
    ((PHY_CTRL_FLAGS(_pc) & (PHY_F_SERDES_MODE | PHY_F_2LANE_MODE)) == PHY_F_SERDES_MODE)
#define IS_4LANE_PORT(_pc) \
    ((PHY_CTRL_FLAGS(_pc) & PHY_F_SERDES_MODE) == 0)

#if PHY_CONFIG_INCLUDE_CHIP_SYMBOLS == 1
extern cdk_symbols_t bcmi_qtce_xgxs_symbols;
#define SET_SYMBOL_TABLE(_pc) \
    PHY_CTRL_SYMBOLS(_pc) = &bcmi_qtce_xgxs_symbols
#else
#define SET_SYMBOL_TABLE(_pc)
#endif

#define NUM_LANES                       4   /* num of lanes per core */
#define MAX_NUM_LANES                   4   /* max num of lanes per port */
#define LANE_NUM_MASK                   0xf /* Lane from PHY control instance */

#define TSCE_CL73_CL37                  0x5
#define TSCE_CL73_HPAM                  0x4
#define TSCE_CL73_HPAM_VS_SW            0x8
#define TSCE_CL73_WO_BAM                0x2
#define TSCE_CL73_W_BAM                 0x1
#define TSCE_CL73_DISABLE               0x0

#define TSCE_CL37_HR2SPM_W_10G          0x5
#define TSCE_CL37_HR2SPM                0x4
#define TSCE_CL37_W_10G                 0x3
#define TSCE_CL37_WO_BAM                0x2
#define TSCE_CL37_W_BAM                 0x1
#define TSCE_CL37_DISABLE               0x0

/* Private PHY flag is used to indicate that firmware is running */
#define PHY_F_FW_RUNNING                PHY_F_PRIVATE

#define PORT_REFCLK_INT                 156

/***********************************************************************
 *
 * HELPER FUNCTIONS
 */
/*
 * Function:
 *      _qtce_serdes_lane
 * Purpose:
 *      Retrieve eagle lane number for this PHY instance.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      Lane number or -1 if lane is unknown
 */
static int
_qtce_serdes_lane(phy_ctrl_t *pc)
{
    uint32_t inst = PHY_CTRL_INST(pc);

    if (inst & PHY_INST_VALID) {
        return inst & LANE_NUM_MASK;
    }
    return -1;
}
/*
 * Function:
 *      _qtce_primary_lane
 * Purpose:
 *      Ensure that each qtc is initialized only once.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      TRUE/FALSE
 */
static int
_qtce_primary_lane(phy_ctrl_t *pc)
{
    uint32_t inst = PHY_CTRL_INST(pc);

    return (inst & 0xf) ? FALSE : TRUE;
}

/*
 * Function:
 *      _qtce_phymod_core_init
 * Purpose:
 *      Init phymod_core_access structure.
 * Parameters:
 *      pc - PHY control structure
 *      core - PHYMOD core access structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_qtce_phymod_core_init(phy_ctrl_t *pc, phymod_core_access_t *core)
{
    if (pc == NULL || core == NULL) {
        return CDK_E_PARAM;
    }

    CDK_MEMSET(core, 0, sizeof(*core));

    /* Use user data pointer to keep whole phy ctrl data */
    core->access.user_acc = (void *)pc;

    /* Setting the flags for phymod */
    if (PHY_CTRL_FLAGS(pc) & PHY_F_CLAUSE45) {
        PHYMOD_ACC_F_CLAUSE45_CLR(&core->access);
    }

    /* Set default to not Qmode */
    PHYMOD_ACC_F_QMODE_CLR(&core->access);
    if (IS_1LANE_PORT(pc)) {    /* QMODE */
        PHYMOD_ACC_F_QMODE_SET(&core->access);
    }

    return CDK_E_NONE;
}

/*
 * Function:
 *      _qtce_phymod_phy_init
 * Purpose:
 *      Init phymod_phy_access structure.
 * Parameters:
 *      pc - PHY control structure
 *      core - PHYMOD phy access structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_qtce_phymod_phy_init(phy_ctrl_t *pc, phymod_phy_access_t *phy, uint32_t phyidx)
{
    uint32_t lane_map = 0x1;

    if (pc == NULL || phy == NULL) {
        return CDK_E_PARAM;
    }

    CDK_MEMSET(phy, 0, sizeof(*phy));

    phy->access.user_acc = (void *)pc;  /* Use user data pointer to keep whole phy ctrl data */
    phy->access.lane_mask = lane_map << _qtce_serdes_lane(pc);

    /* Setting the flags for phymod */
    if (PHY_CTRL_FLAGS(pc) & PHY_F_CLAUSE45) {
        PHYMOD_ACC_F_CLAUSE45_CLR(&phy->access);
    }

    /* Set default to not Qmode */
    PHYMOD_ACC_F_QMODE_CLR(&phy->access);
    if (IS_1LANE_PORT(pc)) { /* QMODE */
        PHYMOD_ACC_F_QMODE_SET(&phy->access);
    }

    return CDK_E_NONE;
}

/*
 * Function:
 *      _qtce_firmware_loader
 * Purpose:
 *      Download PHY firmware using fast interface such as S-channel.
 * Parameters:
 *      pa              - (IN)  PHYMOD access oject
 *      fw_len          - (IN)  Number of bytes to download
 *      fw_data         - (IN)  Firmware as byte array
 * Returns:
 *      PHYMOD_E_xxx
 */
static int
_qtce_firmware_loader(const phymod_core_access_t *pm_core,
                      uint32_t fw_size, const uint8_t *fw_data)
{
    phy_ctrl_t *pc;
    int rv = CDK_E_NONE;

    if (pm_core == NULL) {
        return PHYMOD_E_PARAM;
    }

    /* Retrieve PHY control information from PHYMOD object */
    pc = (phy_ctrl_t *)(pm_core->access.user_acc);
    if (pc == NULL) {
        return PHYMOD_E_PARAM;
    }

    /* Invoke external loader */
    rv = PHY_CTRL_FW_HELPER(pc)(pc, 0, fw_size, (uint8_t *)fw_data);

    return (rv == CDK_E_NONE) ? PHYMOD_E_NONE : PHYMOD_E_IO;
}

/*
 * Function:
 *      _qtce_serdes_stop
 * Purpose:
 *      Put PHY in or out of reset depending on conditions.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_qtce_serdes_stop(phy_ctrl_t *pc)
{
    int rv = CDK_E_NONE;
    phymod_phy_access_t pm_phy;
    phymod_phy_tx_lane_control_t tx_control;
    phymod_phy_rx_lane_control_t rx_control;
    uint32_t f_any = PHY_F_PHY_DISABLE | PHY_F_PORT_DRAIN;
    uint32_t f_copper = PHY_F_MAC_DISABLE | PHY_F_SPEED_CHG | PHY_F_DUPLEX_CHG;
    int stop;

    stop = 0;
    if ((PHY_CTRL_FLAGS(pc) & f_any) ||
        ((PHY_CTRL_FLAGS(pc) & PHY_F_FIBER_MODE) == 0 &&
         (PHY_CTRL_FLAGS(pc) & f_copper))) {
        stop = 1;
    }

    _PHY_DBG(pc, ("_qtce_serdes_stop: stop = %d\n", stop));

    if (!stop) {
        tx_control = phymodTxSquelchOff;
        rx_control = phymodRxSquelchOff;
    } else {
        tx_control = phymodTxSquelchOn;
        rx_control = phymodRxSquelchOn;
    }

    rv = _qtce_phymod_phy_init(pc, &pm_phy, 0);
    if (CDK_SUCCESS(rv)) {
        rv = qtce_phy_tx_lane_control_set(&pm_phy, tx_control);
    }
    if (CDK_SUCCESS(rv)) {
        rv = qtce_phy_rx_lane_control_set(&pm_phy, rx_control);
    }

    return rv;
}

/*
 * Function:
 *      _qtce_abiliby_set
 * Purpose:
 *      configure phy ability.
 * Parameters:
 *      pc - PHY control structure
 *      ability - ability value
 * Returns:
 *      CDK_E_xxx
 */
static int
_qtce_abiliby_set(phy_ctrl_t *pc, uint32_t ability)
{
    int rv = CDK_E_NONE;
    phymod_phy_access_t pm_phy;
    phymod_autoneg_ability_t autoneg_ability;

    CDK_MEMSET(&autoneg_ability, 0, sizeof(autoneg_ability));

    switch (ability & (PHY_ABIL_PAUSE_TX | PHY_ABIL_PAUSE_RX)) {
    case PHY_ABIL_PAUSE_TX:
        PHYMOD_AN_CAP_ASYM_PAUSE_SET(&autoneg_ability);
        break;
    case PHY_ABIL_PAUSE_RX:
        PHYMOD_AN_CAP_ASYM_PAUSE_SET(&autoneg_ability);
        PHYMOD_AN_CAP_SYMM_PAUSE_SET(&autoneg_ability);
        break;
    case PHY_ABIL_PAUSE_TX | PHY_ABIL_PAUSE_RX:
        PHYMOD_AN_CAP_SYMM_PAUSE_SET(&autoneg_ability);
        break;
    }

    /* Set the sgmii speed */
    if(ability & PHY_ABIL_1000MB) {
        PHYMOD_AN_CAP_SGMII_SET(&autoneg_ability);
        autoneg_ability.sgmii_speed = phymod_CL37_SGMII_1000M;
    } else if(ability & PHY_ABIL_100MB) {
        PHYMOD_AN_CAP_SGMII_SET(&autoneg_ability);
        autoneg_ability.sgmii_speed = phymod_CL37_SGMII_100M;
    } else if(ability & PHY_ABIL_10MB) {
        PHYMOD_AN_CAP_SGMII_SET(&autoneg_ability);
        autoneg_ability.sgmii_speed = phymod_CL37_SGMII_10M;
    }

    rv = _qtce_phymod_phy_init(pc, &pm_phy, 0);
    if (CDK_SUCCESS(rv)) {
        rv = qtce_phy_autoneg_ability_set(&pm_phy, &autoneg_ability);
    }

    return rv;
}

static int
_qtce_ref_clk_convert(int port_refclk_int, phymod_ref_clk_t *ref_clk)
{
    switch (port_refclk_int) {
    case 156:
        *ref_clk = phymodRefClk156Mhz;
        break;
    case 125:
        *ref_clk = phymodRefClk125Mhz;
        break;
    default:
        *ref_clk = phymodRefClk156Mhz;
        break;
    }

    return CDK_E_NONE;
}

/***********************************************************************
 *
 * PHY DRIVER FUNCTIONS
 */
/*
 * Function:
 *      bcmi_qtce_xgxs_probe
 * Purpose:
 *      Probe for PHY.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
bcmi_qtce_xgxs_probe(phy_ctrl_t *pc)
{
    int rv = CDK_E_NONE;
    phymod_core_access_t pm_core;
    uint32_t found;

    PHY_CTRL_CHECK(pc);
    PHY_CTRL_FLAGS(pc) |= PHY_F_LANE_CTRL;

    rv = _qtce_phymod_core_init(pc, &pm_core);

    found = 0;
    if (CDK_SUCCESS(rv)) {
        rv = qtce_core_identify(&pm_core, 0, &found);
    }
    if (CDK_SUCCESS(rv) && found) {
        /* All lanes are accessed from the same PHY address */
        PHY_CTRL_FLAGS(pc) |= PHY_F_ADDR_SHARE;    

        SET_SYMBOL_TABLE(pc);
        return CDK_E_NONE;
    }

    PHY_CTRL_FLAGS(pc) &= ~PHY_F_LANE_CTRL;

    return CDK_E_NOT_FOUND;
}


/*
 * Function:
 *      bcmi_qtce_xgxs_notify
 * Purpose:
 *      Handle PHY notifications.
 * Parameters:
 *      pc - PHY control structure
 *      event - PHY event
 * Returns:
 *      CDK_E_xxx
 */
static int
bcmi_qtce_xgxs_notify(phy_ctrl_t *pc, phy_event_t event)
{
    int rv = CDK_E_NONE;
    phymod_phy_inf_config_t interface_config;
    phymod_phy_access_t pm_phy;

    PHY_CTRL_CHECK(pc);

    switch (event) {
    case PhyEvent_ChangeToPassthru:
        PHY_CTRL_FLAGS(pc) |= PHY_F_PASSTHRU;
        PHY_CTRL_FLAGS(pc) &= ~PHY_F_FIBER_MODE;    
        /* Put the Serdes in passthru mode */
        rv = _qtce_phymod_phy_init(pc, &pm_phy, 0);
        interface_config.data_rate = 1000;
        interface_config.ref_clock = phymodRefClk156Mhz;
        interface_config.interface_type = phymodInterfaceSGMII;
        interface_config.interface_modes &= ~PHYMOD_INTF_MODES_FIBER;
        if (CDK_SUCCESS(rv)) {
            rv = qtce_phy_interface_config_set(&pm_phy,
                            0 /* flags */, &interface_config);
        }
        return CDK_E_NONE;
    case PhyEvent_ChangeToFiber:
        PHY_CTRL_FLAGS(pc) &= ~PHY_F_PASSTHRU;
        PHY_CTRL_FLAGS(pc) |= PHY_F_FIBER_MODE;
        /* Put the Serdes in Fiber mode */
        rv = _qtce_phymod_phy_init(pc, &pm_phy, 0);
        interface_config.data_rate = 1000;
        interface_config.ref_clock = phymodRefClk156Mhz;
        interface_config.interface_type = phymodInterface1000X;
        interface_config.interface_modes |= PHYMOD_INTF_MODES_FIBER;
        if (CDK_SUCCESS(rv)) {
            rv = qtce_phy_interface_config_set(&pm_phy,
                            0 /* flags */, &interface_config);
        }
        return CDK_E_NONE;
    case PhyEvent_MacDisable:
        PHY_CTRL_FLAGS(pc) |=  PHY_F_MAC_DISABLE;
        return CDK_E_NONE;
    case PhyEvent_MacEnable:
        PHY_CTRL_FLAGS(pc) &= ~PHY_F_MAC_DISABLE;
        return CDK_E_NONE;
    case PhyEvent_PhyDisable:
        PHY_CTRL_FLAGS(pc) |=  PHY_F_PHY_DISABLE;
        break;
    case PhyEvent_PhyEnable:
        PHY_CTRL_FLAGS(pc) &= ~PHY_F_PHY_DISABLE;
        break;
    case PhyEvent_PortDrainStart:
        PHY_CTRL_FLAGS(pc) |=  PHY_F_PORT_DRAIN;
        break;
    case PhyEvent_PortDrainStop:
        PHY_CTRL_FLAGS(pc) &= ~PHY_F_PORT_DRAIN;
        break;
    default:
        return CDK_E_NONE;
    }

    /* Update power-down state */
    rv = _qtce_serdes_stop(pc);

    return rv;
}

/*
 * Function:
 *      bcmi_qtce_xgxs_reset
 * Purpose:
 *      Reset PHY.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
bcmi_qtce_xgxs_reset(phy_ctrl_t *pc)
{
    PHY_CTRL_CHECK(pc);

    return CDK_E_NONE;
}

/*
 * Function:
 *      bcmi_qtce_xgxs_init
 * Purpose:
 *      Initialize PHY driver.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_NONE
 */
static int
bcmi_qtce_xgxs_init(phy_ctrl_t *pc)
{
    int rv = CDK_E_NONE;
    int lane_map_rx, lane_map_tx;
    int idx, lane;
    phymod_core_access_t pm_core;
    phymod_phy_access_t pm_phy;
    phymod_core_init_config_t core_init_config;
    phymod_core_status_t core_status;
    phymod_phy_init_config_t phy_init_config;
    phymod_autoneg_control_t an;
    phymod_autoneg_ability_t autoneg_ability;
    phymod_phy_inf_config_t interface_config;

    PHY_CTRL_CHECK(pc);

    CDK_MEMSET(&core_init_config, 0, sizeof(core_init_config));
    CDK_MEMSET(&autoneg_ability, 0, sizeof(autoneg_ability));
    CDK_MEMSET(&an, 0, sizeof(phymod_autoneg_control_t));

    pc->num_phys = 1;
    for (idx = 0; idx < pc->num_phys; idx++) {
        rv = _qtce_phymod_core_init(pc, &pm_core);
        if (CDK_SUCCESS(rv)) {
            rv = _qtce_phymod_phy_init(pc, &pm_phy, idx);
        }

        if (_qtce_primary_lane(pc)) {
            /* CORE configuration */
            core_init_config.firmware_load_method = phymodFirmwareLoadMethodInternal;
            core_init_config.firmware_loader = NULL;
            core_init_config.firmware_core_config.CoreConfigFromPCS = 0x0;
            core_init_config.firmware_core_config.VcoRate = 0;
            core_init_config.lane_map.num_of_lanes = NUM_LANES;
            core_init_config.interface.interface_type = phymodInterfaceBypass;

            /* Check for external loader */
            if (PHY_CTRL_FW_HELPER(pc)) {
                core_init_config.firmware_loader = _qtce_firmware_loader;
                core_init_config.firmware_load_method = phymodFirmwareLoadMethodExternal;
            }

            core_init_config.firmware_load_method &= 0xff; /*clear checksum bits*/
            core_init_config.lane_map.num_of_lanes = NUM_LANES;

            lane_map_rx = 0x3210;
            for (lane = 0; lane < NUM_LANES; lane++) {
                core_init_config.lane_map.lane_map_rx[lane] =
                            (lane_map_rx >> (lane * 4 /*4 bit per lane*/)) & 0xf;
            }

            lane_map_tx = 0x3210;
            for (lane = 0; lane < NUM_LANES; lane++) {
                core_init_config.lane_map.lane_map_tx[lane] =
                            (lane_map_tx >> (lane * 4 /*4 bit per lane*/)) & 0xf;
            }

            core_status.pmd_active = 0;
            if (CDK_SUCCESS(rv)) {
                rv = qtce_core_init(&pm_core, &core_init_config, &core_status);
            }
        }
    }

    /* PHY configuration */
    for(lane = 0; lane < PHYMOD_MAX_LANES_PER_CORE; lane++){
        phy_init_config.tx[lane].pre = -1;
        phy_init_config.tx[lane].main = -1;
        phy_init_config.tx[lane].post = -1;
        phy_init_config.tx[lane].post2 = -1;
        phy_init_config.tx[lane].post3 = -1;
        phy_init_config.tx[lane].amp = -1;
    }

    /* Initialize the tx taps and driver current*/
    for (lane = 0; lane < NUM_LANES; lane++) {
        phy_init_config.tx[lane].pre = 0x0;
        phy_init_config.tx[lane].main = 0x70;
        phy_init_config.tx[lane].post = 0x0;
        phy_init_config.tx[lane].post2 = 0x0;
        phy_init_config.tx[lane].post3 = 0x0;
        phy_init_config.tx[lane].amp = 0xc;
    }
    phy_init_config.polarity.rx_polarity = FALSE;
    phy_init_config.polarity.tx_polarity = FALSE;
    phy_init_config.cl72_en = TRUE;
    phy_init_config.an_en = TRUE;

    if (CDK_SUCCESS(rv)) {
        rv = qtce_phy_init(&pm_phy, &phy_init_config);
    }

    PHYMOD_MEMSET(&interface_config, 0, sizeof(phymod_phy_inf_config_t));
    interface_config.data_rate = 1000;
    interface_config.ref_clock = phymodRefClk156Mhz;
    if (PHY_CTRL_FLAGS(pc) & PHY_F_FIBER_MODE) {
        interface_config.interface_type = phymodInterface1000X;
    } else {
        interface_config.interface_type = phymodInterfaceSGMII;
    }

    if (CDK_SUCCESS(rv)) {
        rv = qtce_phy_interface_config_set(&pm_phy,
                        0 /* flags */, &interface_config);
    }

    if (PHY_CTRL_FLAGS(pc) & PHY_F_PASSTHRU) {
        PHY_NOTIFY(pc, PhyEvent_ChangeToPassthru);
    } else {
        PHY_NOTIFY(pc, PhyEvent_ChangeToFiber);
    }

    return rv;
}

/*
 * Function:
 *      bcmi_qtce_xgxs_link_get
 * Purpose:
 *      Determine the current link up/down status.
 * Parameters:
 *      pc - PHY control structure
 *      link - (OUT) non-zero indicates link established.
 *      autoneg_done - (OUT) if true, auto-negotiation is complete
 * Returns:
 *      CDK_E_xxx
 */
static int
bcmi_qtce_xgxs_link_get(phy_ctrl_t *pc, int *link, int *autoneg_done)
{
    int rv = CDK_E_NONE;
    int an_done;
    phymod_phy_access_t pm_phy;
    phymod_autoneg_control_t an;
    
    PHY_CTRL_CHECK(pc);

    CDK_MEMSET(&an, 0x0, sizeof(an));

    rv = _qtce_phymod_phy_init(pc, &pm_phy, 0);
    if (CDK_SUCCESS(rv)) {
        rv = qtce_phy_autoneg_get(&pm_phy, &an, (uint32_t *)&an_done);
    }

    *autoneg_done = 0;
    if (an.enable) {
        *autoneg_done = an_done;
    }

    if (CDK_SUCCESS(rv)) {
        rv = qtce_phy_link_status_get(&pm_phy, (uint32_t *)link);
    }
    
    return rv;
}

/*
 * Function:
 *      bcmi_qtce_xgxs_duplex_set
 * Purpose:
 *      Set the current duplex mode (forced).
 * Parameters:
 *      pc - PHY control structure
 *      duplex - non-zero indicates full duplex, zero indicates half
 * Returns:
 *      CDK_E_xxx
 */
static int
bcmi_qtce_xgxs_duplex_set(phy_ctrl_t *pc, int duplex)
{
    PHY_CTRL_CHECK(pc);

    return CDK_E_NONE;
}

/*
 * Function:
 *      bcmi_qtce_xgxs_duplex_get
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
bcmi_qtce_xgxs_duplex_get(phy_ctrl_t *pc, int *duplex)
{
    PHY_CTRL_CHECK(pc);

    *duplex = 1;

    return CDK_E_NONE;
}

/*
 * Function:
 *      bcmi_qtce_xgxs_speed_set
 * Purpose:
 *      Set the current operating speed (forced).
 * Parameters:
 *      pc - PHY control structure
 *      speed - new link speed
 * Returns:
 *      CDK_E_xxx
 * Notes:
 *      The actual speed is controlled elsewhere, so we accept any value.
 */
static int
bcmi_qtce_xgxs_speed_set(phy_ctrl_t *pc, uint32_t speed)
{
    int rv = CDK_E_NONE;
    phymod_phy_access_t pm_phy;
    phymod_phy_inf_config_t interface_config;
    uint32_t cur_speed;

    PHY_CTRL_CHECK(pc);

    /* Leave hardware alone if speed is unchanged */
    rv = PHY_SPEED_GET(pc, &cur_speed);
    if (CDK_SUCCESS(rv) && speed == cur_speed) {
        return CDK_E_NONE;
    }
    
    interface_config.data_rate = speed;
    interface_config.ref_clock = phymodRefClk156Mhz;

    switch (speed) {
    case 10:
        interface_config.interface_type = phymodInterfaceSGMII;
        break;
    case 100:
    case 1000:
        if (PHY_CTRL_FLAGS(pc) & PHY_F_FIBER_MODE) {
            interface_config.interface_type = phymodInterface1000X;
        } else {
            interface_config.interface_type = phymodInterfaceSGMII;
        }
        break;
    case 2500:
        interface_config.interface_type = phymodInterface1000X;
        break;
    default:
        return CDK_E_PARAM;
    }

    rv = _qtce_phymod_phy_init(pc, &pm_phy, 0);
    if (CDK_SUCCESS(rv)) {
        rv = qtce_phy_interface_config_set(&pm_phy,
                        0 /* flags */, &interface_config);
    }

    return rv;
}

/*
 * Function:
 *      bcmi_qtce_xgxs_speed_get
 * Purpose:
 *      Get the current operating speed.
 * Parameters:
 *      pc - PHY control structure
 *      speed - (OUT) current link speed
 * Returns:
 *      CDK_E_xxx
 * Notes:
 *      The actual speed is controlled elsewhere, so always return 10000
 *      for sanity purposes.
 */

static int
bcmi_qtce_xgxs_speed_get(phy_ctrl_t *pc, uint32_t *speed)
{
    int rv = CDK_E_NONE;
    phymod_phy_access_t pm_phy;
    phymod_phy_inf_config_t interface_config;
    phymod_ref_clk_t ref_clock;

    PHY_CTRL_CHECK(pc);

    rv = _qtce_phymod_phy_init(pc, &pm_phy, 0);

    /* Note that the flags have an option to indicate whether it's ok to reprogram the PLL */
    interface_config.data_rate = 0;
    if (CDK_SUCCESS(rv)) {
        rv = _qtce_ref_clk_convert(PORT_REFCLK_INT, &ref_clock);
    }
    if (CDK_SUCCESS(rv)) {
        rv = qtce_phy_interface_config_get(&pm_phy, 0, ref_clock,
                                              &interface_config);
    }

    *speed = interface_config.data_rate;

    return rv;
}

/*
 * Function:
 *      bcmi_qtce_xgxs_autoneg_set
 * Purpose:
 *      Enable or disabled auto-negotiation on the specified port.
 * Parameters:
 *      pc - PHY control structure
 *      autoneg - non-zero enables autoneg, zero disables autoneg
 * Returns:
 *      CDK_E_xxx
 */
static int
bcmi_qtce_xgxs_autoneg_set(phy_ctrl_t *pc, int autoneg)
{
    int rv = CDK_E_NONE;
    phymod_autoneg_control_t an;
    phymod_phy_access_t pm_phy;
    uint32_t cl37an;

    PHY_CTRL_CHECK(pc);

    an.flags = 0;
    an.enable = autoneg;
    an.num_lane_adv = 1;
    an.an_mode = phymod_AN_MODE_NONE;
    cl37an = 0;

    /*first check if qsgmii mode */
    if(PHY_CTRL_FLAGS(pc) & PHY_F_FIBER_MODE) {
        an.an_mode = phymod_AN_MODE_CL37;
    } else {
        an.an_mode = phymod_AN_MODE_SGMII;
    }

    if (cl37an ==  TSCE_CL37_W_BAM) {
        an.an_mode = phymod_AN_MODE_CL37BAM;
    }

    rv = _qtce_phymod_phy_init(pc, &pm_phy, 0);
    if (CDK_SUCCESS(rv)) {
        rv = qtce_phy_autoneg_set(&pm_phy, &an);
    }

    return rv;
}

/*
 * Function:
 *      bcmi_qtce_xgxs_autoneg_get
 * Purpose:
 *      Get the current auto-negotiation status (enabled/busy)
 * Parameters:
 *      pc - PHY control structure
 *      autoneg - (OUT) non-zero indicates autoneg enabled
 * Returns:
 *      CDK_E_xxx
 */
static int
bcmi_qtce_xgxs_autoneg_get(phy_ctrl_t *pc, int *autoneg)
{
    int rv = CDK_E_NONE;
    phymod_phy_access_t pm_phy;
    phymod_autoneg_control_t an_control;
    int an_complete;

    PHY_CTRL_CHECK(pc);

    CDK_MEMSET(&an_control, 0x0, sizeof(an_control));

    rv = _qtce_phymod_phy_init(pc, &pm_phy, 0);
    if (CDK_SUCCESS(rv)) {
        rv = qtce_phy_autoneg_get(&pm_phy, &an_control, (uint32_t *)&an_complete);
    }

    if (an_control.enable) {
        *autoneg = 1;
    } else {
        *autoneg = 0;
    }

    return rv;
}

/*
 * Function:
 *      bcmi_qtce_xgxs_loopback_set
 * Purpose:
 *      Set PHY loopback mode.
 * Parameters:
 *      pc - PHY control structure
 *      enable - non-zero enables PHY loopback
 * Returns:
 *      CDK_E_xxx
 */
static int
bcmi_qtce_xgxs_loopback_set(phy_ctrl_t *pc, int enable)
{
    int rv = CDK_E_NONE;
    phymod_phy_access_t pm_phy;

    PHY_CTRL_CHECK(pc);

    rv = _qtce_phymod_phy_init(pc, &pm_phy, 0);
    if (CDK_SUCCESS(rv)) {
        rv = qtce_phy_loopback_set(&pm_phy, phymodLoopbackGlobal, enable);
    }

    return rv;
}

/*
 * Function:
 *      bcmi_qtce_xgxs_loopback_get
 * Purpose:
 *      Get the current PHY loopback mode.
 * Parameters:
 *      pc - PHY control structure
 *      enable - (OUT) non-zero indicates PHY loopback enabled
 * Returns:
 *      CDK_E_xxx
 */
static int
bcmi_qtce_xgxs_loopback_get(phy_ctrl_t *pc, int *enable)
{
    int rv = CDK_E_NONE;
    phymod_phy_access_t pm_phy;
    uint32_t lb_enable;

    PHY_CTRL_CHECK(pc);

    *enable = 0;

    rv = _qtce_phymod_phy_init(pc, &pm_phy, 0);
    if (CDK_SUCCESS(rv)) {
        rv = qtce_phy_loopback_get(&pm_phy, phymodLoopbackGlobal, &lb_enable);
    }

    if (lb_enable) {
        *enable = 1;
    }

    return rv;
}

/*
 * Function:
 *      bcmi_qtce_xgxs_ability_get
 * Purpose:
 *      Get the abilities of the PHY.
 * Parameters:
 *      pc - PHY control structure
 *      abil - (OUT) ability mask indicating supported options/speeds.
 * Returns:
 *      CDK_E_xxx
 */
static int
bcmi_qtce_xgxs_ability_get(phy_ctrl_t *pc, uint32_t *abil)
{
    PHY_CTRL_CHECK(pc);

    *abil = (PHY_ABIL_2500MB | PHY_ABIL_1000MB |
             PHY_ABIL_100MB | PHY_ABIL_10MB | PHY_ABIL_SERDES |
             PHY_ABIL_PAUSE | PHY_ABIL_SGMII | PHY_ABIL_GMII |
             PHY_ABIL_AN | PHY_ABIL_LOOPBACK);

    return CDK_E_NONE;
}

/*
 * Function:
 *      bcmi_qtce_xgxs_config_set
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
bcmi_qtce_xgxs_config_set(phy_ctrl_t *pc, phy_config_t cfg, uint32_t val, void *cd)
{
    int rv = CDK_E_NONE;
    phymod_core_access_t pm_core;
    phymod_phy_access_t pm_phy;
    int idx;

    PHY_CTRL_CHECK(pc);

    switch (cfg) {
    case PhyConfig_EEE:
        return CDK_E_NONE;
    case PhyConfig_Enable:
        PHY_CTRL_FLAGS(pc) |= PHY_F_PHY_DISABLE;
        if (val) {
            PHY_CTRL_FLAGS(pc) &= ~PHY_F_PHY_DISABLE;
        }
        return _qtce_serdes_stop(pc);
    case PhyConfig_RemoteLoopback:
        {
            rv = _qtce_phymod_phy_init(pc, &pm_phy, 0);
            if (CDK_SUCCESS(rv)) {
                rv = qtce_phy_loopback_set(&pm_phy,
                                   phymodLoopbackRemotePMD, val);
            }
            return rv;
        }
#if PHY_CONFIG_INCLUDE_XAUI_TX_LANE_MAP_SET
    case PhyConfig_XauiTxLaneRemap:
        {
            phymod_lane_map_t lane_map;

            rv = _qtce_phymod_core_init(pc, &pm_core);
            if (CDK_SUCCESS(rv)) {
                rv = qtce_core_lane_map_get(&pm_core, &lane_map);
            }
            if (CDK_SUCCESS(rv)) {
                for (idx = 0; idx < NUM_LANES; idx++) {
                    lane_map.lane_map_tx[idx] =
                            (val >> (16 + idx * 4 /*4 bit per lane*/)) & 0xf;
                }
                rv = qtce_core_lane_map_set(&pm_core, &lane_map);
            }
            return rv;
        }
#endif
#if PHY_CONFIG_INCLUDE_XAUI_RX_LANE_MAP_SET
    case PhyConfig_XauiRxLaneRemap:
        {
            phymod_lane_map_t lane_map;

            rv = _qtce_phymod_core_init(pc, &pm_core);
            if (CDK_SUCCESS(rv)) {
                rv = qtce_core_lane_map_get(&pm_core, &lane_map);
            }
            if (CDK_SUCCESS(rv)) {
                for (idx = 0; idx < NUM_LANES; idx++) {
                    lane_map.lane_map_rx[idx] =
                                 (val >> (idx * 4 /*4 bit per lane*/)) & 0xf;
                }
                rv = qtce_core_lane_map_set(&pm_core, &lane_map);
            }
            return rv;
        }
#endif
#if PHY_CONFIG_INCLUDE_XAUI_TX_POLARITY_SET
    case PhyConfig_XauiTxPolInvert:
        {
            phymod_polarity_t polarity;

            rv = _qtce_phymod_phy_init(pc, &pm_phy, 0);
            if (CDK_SUCCESS(rv)) {
                rv = qtce_phy_polarity_get(&pm_phy, &polarity);
            }

            polarity.tx_polarity = val;
            if (CDK_SUCCESS(rv)) {
                rv = qtce_phy_polarity_set(&pm_phy, &polarity);
            }
            return rv;
        }
#endif
#if PHY_CONFIG_INCLUDE_XAUI_RX_POLARITY_SET
    case PhyConfig_XauiRxPolInvert:
        {
            phymod_polarity_t polarity;

            rv = _qtce_phymod_phy_init(pc, &pm_phy, 0);
            if (CDK_SUCCESS(rv)) {
                rv = qtce_phy_polarity_get(&pm_phy, &polarity);
            }

            polarity.rx_polarity = val;
            if (CDK_SUCCESS(rv)) {
                rv = qtce_phy_polarity_set(&pm_phy, &polarity);
            }

            return rv;
        }
#endif
#if PHY_CONFIG_INCLUDE_LINK_ABILITIES
    case PhyConfig_AdvLocal:
        rv = _qtce_abiliby_set(pc, val);
        return rv;
#endif
    default:
        break;
    }

    return CDK_E_UNAVAIL;
}

/*
 * Function:
 *      bcmi_qtce_xgxs_config_get
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
bcmi_qtce_xgxs_config_get(phy_ctrl_t *pc, phy_config_t cfg, uint32_t *val, void *cd)
{
    int rv = CDK_E_NONE;
    phymod_core_access_t pm_core;
    phymod_phy_access_t pm_phy;
    int idx;

    PHY_CTRL_CHECK(pc);

    switch (cfg) {
    case PhyConfig_Enable:
        *val = (PHY_CTRL_FLAGS(pc) & PHY_F_PHY_DISABLE) ? 0 : 1;
        return CDK_E_NONE;
    case PhyConfig_RemoteLoopback:
        {
            rv = _qtce_phymod_phy_init(pc, &pm_phy, 0);
            if (CDK_SUCCESS(rv)) {
                rv = qtce_phy_loopback_get(&pm_phy,
                                    phymodLoopbackRemotePMD, val);
            }
            return rv;
        }
#if PHY_CONFIG_INCLUDE_XAUI_TX_LANE_MAP_SET
    case PhyConfig_XauiTxLaneRemap:
        {
            phymod_lane_map_t lane_map;

            rv = _qtce_phymod_core_init(pc, &pm_core);
            if (CDK_SUCCESS(rv)) {
                rv = qtce_core_lane_map_get(&pm_core, &lane_map);
            }
            if (CDK_SUCCESS(rv)) {
                *val = 0;
                for (idx = 0; idx < NUM_LANES; idx++) {
                    *val += (lane_map.lane_map_tx[idx]<< (idx * 4 + 16));
                }
            }
            return rv;
        }
#endif
#if PHY_CONFIG_INCLUDE_XAUI_RX_LANE_MAP_SET
    case PhyConfig_XauiRxLaneRemap:
        {
            phymod_lane_map_t lane_map;

            rv = _qtce_phymod_core_init(pc, &pm_core);
            if (CDK_SUCCESS(rv)) {
                rv = qtce_core_lane_map_get(&pm_core, &lane_map);
            }
            if (CDK_SUCCESS(rv)) {
                *val = 0;
                for (idx = 0; idx < NUM_LANES; idx++) {
                    *val += (lane_map.lane_map_rx[idx] << (idx * 4));
                }
            }
            return rv;
        }
#endif
#if PHY_CONFIG_INCLUDE_XAUI_TX_POLARITY_SET
    case PhyConfig_XauiTxPolInvert:
        {
            phymod_polarity_t polarity;

            rv = _qtce_phymod_phy_init(pc, &pm_phy, 0);
            if (CDK_SUCCESS(rv)) {
                rv = qtce_phy_polarity_get(&pm_phy, &polarity);
            }
            *val = polarity.tx_polarity;

            return rv;
        }
#endif
#if PHY_CONFIG_INCLUDE_XAUI_RX_POLARITY_SET
    case PhyConfig_XauiRxPolInvert:
        {
            phymod_polarity_t polarity;

            rv = _qtce_phymod_phy_init(pc, &pm_phy, 0);
            if (CDK_SUCCESS(rv)) {
                rv = qtce_phy_polarity_get(&pm_phy, &polarity);
            }
            *val = polarity.rx_polarity;

            return rv;
        }
#endif
    default:
        break;
    }

    return CDK_E_UNAVAIL;
}

/*
 * Function:
 *      bcmi_qtce_xgxs_status_get
 * Purpose:
 *      Get PHY status value.
 * Parameters:
 *      pc - PHY control structure
 *      stat - status parameter
 *      val - (OUT) status value
 * Returns:
 *      CDK_E_xxx
 */
static int
bcmi_qtce_xgxs_status_get(phy_ctrl_t *pc, phy_status_t stat, uint32_t *val)
{
    int rv = CDK_E_NONE;
    phymod_phy_access_t pm_phy;
    phymod_ref_clk_t ref_clock;

    PHY_CTRL_CHECK(pc);

    switch (stat) {
    case PhyStatus_LineInterface:
        {
            phymod_phy_inf_config_t interface_config;

            CDK_MEMSET(&interface_config, 0x0, sizeof(interface_config));

            rv = _qtce_phymod_phy_init(pc, &pm_phy, 0);
            if (CDK_SUCCESS(rv)) {
                rv = _qtce_ref_clk_convert(PORT_REFCLK_INT, &ref_clock);
            }
            if (CDK_SUCCESS(rv)) {
                rv = qtce_phy_interface_config_get(&pm_phy, 0, ref_clock,
                                                      &interface_config);
            }

            switch (interface_config.interface_type) {
            case phymodInterface1000X:
                *val = PHY_IF_GMII;
                break;
            case phymodInterfaceSGMII:
                *val = PHY_IF_SGMII;
                break;
            default:
                *val = PHY_IF_GMII;
                break;
            }
            return rv;
        }
    default:
        break;
    }

    return rv;
}

/* Public PHY Driver Structure */
phy_driver_t bcmi_qtce_xgxs_drv = {
    "bcmi_qtce_xgxs",
    "Internal QTC/Eagle QSGMII SerDes PHY Driver",
    PHY_DRIVER_F_INTERNAL,
    bcmi_qtce_xgxs_probe,              /* pd_probe */
    bcmi_qtce_xgxs_notify,             /* pd_notify */
    bcmi_qtce_xgxs_reset,              /* pd_reset */
    bcmi_qtce_xgxs_init,               /* pd_init */
    bcmi_qtce_xgxs_link_get,           /* pd_link_get */
    bcmi_qtce_xgxs_duplex_set,         /* pd_duplex_set */
    bcmi_qtce_xgxs_duplex_get,         /* pd_duplex_get */
    bcmi_qtce_xgxs_speed_set,          /* pd_speed_set */
    bcmi_qtce_xgxs_speed_get,          /* pd_speed_get */
    bcmi_qtce_xgxs_autoneg_set,        /* pd_autoneg_set */
    bcmi_qtce_xgxs_autoneg_get,        /* pd_autoneg_get */
    bcmi_qtce_xgxs_loopback_set,       /* pd_loopback_set */
    bcmi_qtce_xgxs_loopback_get,       /* pd_loopback_get */
    bcmi_qtce_xgxs_ability_get,        /* pd_ability_get */
    bcmi_qtce_xgxs_config_set,         /* pd_config_set */
    bcmi_qtce_xgxs_config_get,         /* pd_config_get */
    bcmi_qtce_xgxs_status_get,         /* pd_status_get */
    NULL                                    /* pd_cable_diag */
};
