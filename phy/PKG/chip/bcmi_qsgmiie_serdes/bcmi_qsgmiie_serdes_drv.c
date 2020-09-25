/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * PHY driver for Broadcom TSC/Eagle QSGMII internal SerDes.
 */

#include <phy/phy.h>
#include <phy/phy_drvlist.h>
#include <phymod/phymod.h>
#include <phy/chip/bcmi_qsgmiie_serdes_defs.h>

#include "qsgmiie/qsgmiie.h"

/* Low level debugging (off by default) */
#ifdef PHY_DEBUG_ENABLE
#define _PHY_DBG(_pc, _stuff) \
    PHY_VERB(_pc, _stuff)
#else
#define _PHY_DBG(_pc, _stuff)
#endif

#if PHY_CONFIG_INCLUDE_CHIP_SYMBOLS == 1
extern cdk_symbols_t bcmi_qsgmiie_serdes_symbols;
#define SET_SYMBOL_TABLE(_pc) \
    PHY_CTRL_SYMBOLS(_pc) = &bcmi_qsgmiie_serdes_symbols
#else
#define SET_SYMBOL_TABLE(_pc)
#endif

#define NUM_LANES                       4   /* num of lanes per core */
#define MAX_NUM_LANES                   4   /* max num of lanes per port */
#define LANE_NUM_MASK                   0xf /* Lane from PHY control instance */

#define TSCE_CL73_CL37                 0x5
#define TSCE_CL73_HPAM                 0x4
#define TSCE_CL73_HPAM_VS_SW           0x8
#define TSCE_CL73_WO_BAM               0x2
#define TSCE_CL73_W_BAM                0x1
#define TSCE_CL73_DISABLE              0x0

#define TSCE_CL37_HR2SPM_W_10G         0x5
#define TSCE_CL37_HR2SPM               0x4
#define TSCE_CL37_W_10G                0x3
#define TSCE_CL37_WO_BAM               0x2
#define TSCE_CL37_W_BAM                0x1
#define TSCE_CL37_DISABLE              0x0

/* Private PHY flag is used to indicate that firmware is running */
#define PHY_F_FW_RUNNING                PHY_F_PRIVATE

#define PORT_REFCLK_INT                156

/***********************************************************************
 *
 * HELPER FUNCTIONS
 */
/*
 * Function:
 *      _qsgmiie_serdes_lane
 * Purpose:
 *      Retrieve eagle lane number for this PHY instance.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      Lane number or -1 if lane is unknown
 */
static int
_qsgmiie_serdes_lane(phy_ctrl_t *pc)
{
    uint32_t inst = PHY_CTRL_INST(pc);

    if (inst & PHY_INST_VALID) {
        return inst & LANE_NUM_MASK;
    }
    return -1;
}

/*
 * Function:
 *      _qsgmiie_primary_lane
 * Purpose:
 *      Ensure that each tsce is initialized only once.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      TRUE/FALSE
 */
static int
_qsgmiie_primary_lane(phy_ctrl_t *pc)
{
    uint32_t inst = PHY_CTRL_INST(pc);
    
    return (inst & LANE_NUM_MASK) ? FALSE : TRUE;
}

/*
 * Function:
 *      _qsgmiie_phymod_core_init
 * Purpose:
 *      Init phymod_core_access structure.
 * Parameters:
 *      pc - PHY control structure
 *      core - PHYMOD core access structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_qsgmiie_phymod_core_init(phy_ctrl_t *pc, phymod_core_access_t *core)
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
    
    return CDK_E_NONE;
}

/*
 * Function:
 *      _qsgmiie_phymod_phy_init
 * Purpose:
 *      Init phymod_phy_access structure.
 * Parameters:
 *      pc - PHY control structure
 *      core - PHYMOD phy access structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_qsgmiie_phymod_phy_init(phy_ctrl_t *pc, phymod_phy_access_t *phy)
{
    uint32_t lane_map = 0x1;
    
    if (pc == NULL || phy == NULL) {
        return CDK_E_PARAM;
    }

    CDK_MEMSET(phy, 0, sizeof(*phy));

    /* Use user data pointer to keep whole phy ctrl data */
    phy->access.user_acc = (void *)pc;
    phy->access.lane_mask = lane_map << _qsgmiie_serdes_lane(pc);

    /* Setting the flags for phymod */
    if (PHY_CTRL_FLAGS(pc) & PHY_F_CLAUSE45) {
        PHYMOD_ACC_F_CLAUSE45_CLR(&phy->access);
    }
    
    return CDK_E_NONE;
}

/*
 * Function:
 *      _qsgmiie_serdes_stop
 * Purpose:
 *      Put PHY in or out of reset depending on conditions.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_qsgmiie_serdes_stop(phy_ctrl_t *pc)
{
    int rv = CDK_E_NONE;
    phymod_phy_access_t pm_phy;
    phymod_phy_power_t phy_power;
    uint32_t f_any = PHY_F_PHY_DISABLE | PHY_F_PORT_DRAIN;
    uint32_t f_copper = PHY_F_MAC_DISABLE | PHY_F_SPEED_CHG | PHY_F_DUPLEX_CHG;
    int stop;

    stop = 0;
    if ((PHY_CTRL_FLAGS(pc) & f_any) ||
        ((PHY_CTRL_FLAGS(pc) & PHY_F_FIBER_MODE) == 0 &&
         (PHY_CTRL_FLAGS(pc) & f_copper))) {
        stop = 1;
    }

    _PHY_DBG(pc, ("_qsgmiie_serdes_stop: stop = %d\n", stop));

    if (stop) {
        phy_power.tx = phymodPowerOff;
        phy_power.rx = phymodPowerOff;
    } else {
        phy_power.tx = phymodPowerOn;
        phy_power.rx = phymodPowerOn;
    }

    _qsgmiie_phymod_phy_init(pc, &pm_phy);
    if (CDK_SUCCESS(rv)) {
        rv = qsgmiie_phy_power_set(&pm_phy, &phy_power); 
    }

    return rv;
}

/*
 * Function:
 *      _qsgmiie_abiliby_set
 * Purpose:
 *      configure phy ability.
 * Parameters:
 *      pc - PHY control structure
 *      ability - ability value
 * Returns:
 *      CDK_E_xxx
 */
static int
_qsgmiie_abiliby_set(phy_ctrl_t *pc, uint32_t ability)
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

    _qsgmiie_phymod_phy_init(pc, &pm_phy);
    rv = qsgmiie_phy_autoneg_ability_set(&pm_phy, &autoneg_ability);
    
    return rv;
}

static int 
_qsgmiie_ref_clk_convert(int port_refclk_int, phymod_ref_clk_t *ref_clk)
{
    switch (port_refclk_int)
    {
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
 *      bcmi_qsgmiie_serdes_probe
 * Purpose:     
 *      Probe for PHY.
 * Parameters:
 *      pc - PHY control structure
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcmi_qsgmiie_serdes_probe(phy_ctrl_t *pc)
{
    int rv = CDK_E_NONE;
    phymod_core_access_t pm_core;
    uint32_t found;
    
    PHY_CTRL_CHECK(pc);

    PHY_CTRL_FLAGS(pc) |= PHY_F_LANE_CTRL;
    _qsgmiie_phymod_core_init(pc, &pm_core);
    found = 0;
    rv = qsgmiie_core_identify(&pm_core, 0, &found);
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
 *      bcmi_qsgmiie_serdes_notify
 * Purpose:     
 *      Handle PHY notifications.
 * Parameters:
 *      pc - PHY control structure
 *      event - PHY event
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcmi_qsgmiie_serdes_notify(phy_ctrl_t *pc, phy_event_t event)
{
    int rv = CDK_E_NONE;

    PHY_CTRL_CHECK(pc);

    switch (event) {
    case PhyEvent_ChangeToPassthru:
        PHY_CTRL_FLAGS(pc) &= ~PHY_F_FIBER_MODE;
        PHY_CTRL_FLAGS(pc) |= PHY_F_PASSTHRU;
        return CDK_E_NONE;
    case PhyEvent_ChangeToFiber:
        PHY_CTRL_FLAGS(pc) |= PHY_F_FIBER_MODE;
        PHY_CTRL_FLAGS(pc) &= ~PHY_F_PASSTHRU;
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
    rv = _qsgmiie_serdes_stop(pc);
    
    return rv;
}

/*
 * Function:
 *      bcmi_qsgmiie_serdes_reset
 * Purpose:     
 *      Reset PHY.
 * Parameters:
 *      pc - PHY control structure
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcmi_qsgmiie_serdes_reset(phy_ctrl_t *pc)
{
    PHY_CTRL_CHECK(pc);

    return CDK_E_NONE;
}

/*
 * Function:
 *      bcmi_qsgmiie_serdes_init
 * Purpose:     
 *      Initialize PHY driver.
 * Parameters:
 *      pc - PHY control structure
 * Returns:     
 *      CDK_E_NONE
 */
static int
bcmi_qsgmiie_serdes_init(phy_ctrl_t *pc)
{
    int rv = CDK_E_NONE;
    phymod_core_access_t pm_core;
    phymod_phy_access_t pm_phy;
    phymod_core_init_config_t core_init_config;
    phymod_core_status_t core_status;
    phymod_phy_init_config_t phy_init_config;
    int lane_map_rx, lane_map_tx;
    int lane;

    PHY_CTRL_CHECK(pc);

    pc->num_phys = 1;

    _qsgmiie_phymod_core_init(pc, &pm_core);
    _qsgmiie_phymod_phy_init(pc, &pm_phy);
    
    if (_qsgmiie_primary_lane(pc)) {
        /* CORE configuration */
        core_init_config.firmware_load_method = phymodFirmwareLoadMethodInternal;
        core_init_config.firmware_loader = NULL;
        core_init_config.firmware_core_config.CoreConfigFromPCS = 0x0;
        core_init_config.firmware_core_config.VcoRate = 0;
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
            rv = qsgmiie_core_init(&pm_core, &core_init_config, &core_status);
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
        rv = qsgmiie_phy_init(&pm_phy, &phy_init_config);
    }

    /* Default mode is fiber */
    PHY_NOTIFY(pc, PhyEvent_ChangeToFiber);
    return rv;
}

/*
 * Function:    
 *      bcmi_qsgmiie_serdes_link_get
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
bcmi_qsgmiie_serdes_link_get(phy_ctrl_t *pc, int *link, int *autoneg_done)
{
    int rv = CDK_E_NONE;
    phymod_phy_access_t pm_phy;
    phymod_autoneg_control_t an;
    int an_done;
    
    PHY_CTRL_CHECK(pc);

    CDK_MEMSET(&an, 0x0, sizeof(an));
    _qsgmiie_phymod_phy_init(pc, &pm_phy);
    
    if (CDK_SUCCESS(rv)) {
        rv = qsgmiie_phy_autoneg_get(&pm_phy, &an, (uint32_t *)&an_done);
    }   
    
    *autoneg_done = 0;
    if (an.enable) {
        *autoneg_done = an_done;
    }
    
    if (CDK_SUCCESS(rv)) {
        rv = qsgmiie_phy_link_status_get(&pm_phy, (uint32_t *)link);
    }
    return rv;
}

/*
 * Function:    
 *      bcmi_qsgmiie_serdes_duplex_set
 * Purpose:     
 *      Set the current duplex mode (forced).
 * Parameters:
 *      pc - PHY control structure
 *      duplex - non-zero indicates full duplex, zero indicates half
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcmi_qsgmiie_serdes_duplex_set(phy_ctrl_t *pc, int duplex)
{
    PHY_CTRL_CHECK(pc);

    return CDK_E_NONE;
}

/*
 * Function:    
 *      bcmi_qsgmiie_serdes_duplex_get
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
bcmi_qsgmiie_serdes_duplex_get(phy_ctrl_t *pc, int *duplex)
{
    PHY_CTRL_CHECK(pc);

    *duplex = 1;

    return CDK_E_NONE;
}

/*
 * Function:    
 *      bcmi_qsgmiie_serdes_speed_set
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
bcmi_qsgmiie_serdes_speed_set(phy_ctrl_t *pc, uint32_t speed)
{
    int rv = CDK_E_NONE;
    phymod_phy_access_t pm_phy;
    phymod_phy_inf_config_t interface_config;
    int autoneg;

    PHY_CTRL_CHECK(pc);

    /* Do not set speed if auto-negotiation is enabled */
    rv = PHY_AUTONEG_GET(pc, &autoneg);
    if (CDK_FAILURE(rv)) {
        return rv;
    }
    if (autoneg) {
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
            
    _qsgmiie_phymod_phy_init(pc, &pm_phy);
    rv = qsgmiie_phy_interface_config_set(&pm_phy, 
                            0 /* flags */, &interface_config);
    return rv;
}

/*
 * Function:    
 *      bcmi_qsgmiie_serdes_speed_get
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
bcmi_qsgmiie_serdes_speed_get(phy_ctrl_t *pc, uint32_t *speed)
{
    int rv = CDK_E_NONE;
    phymod_phy_access_t pm_phy;
    phymod_phy_inf_config_t interface_config;
    phymod_ref_clk_t ref_clock;

    PHY_CTRL_CHECK(pc);

    _qsgmiie_phymod_phy_init(pc, &pm_phy);
    
    /* Note that the flags have an option to indicate whether it's ok to reprogram the PLL */
    interface_config.data_rate = 0;
    rv = _qsgmiie_ref_clk_convert(PORT_REFCLK_INT, &ref_clock);
    if (CDK_SUCCESS(rv)) {
        rv = qsgmiie_phy_interface_config_get(&pm_phy, 0, ref_clock,  
                                              &interface_config);
    }
    
    *speed = interface_config.data_rate;
    
    return rv;
}

/*
 * Function:    
 *      bcmi_qsgmiie_serdes_autoneg_set
 * Purpose:     
 *      Enable or disabled auto-negotiation on the specified port.
 * Parameters:
 *      pc - PHY control structure
 *      autoneg - non-zero enables autoneg, zero disables autoneg
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcmi_qsgmiie_serdes_autoneg_set(phy_ctrl_t *pc, int autoneg)
{
    int rv = CDK_E_NONE;
    phymod_autoneg_control_t an;
    phymod_phy_access_t pm_phy;
    uint32_t cl73an, cl37an;
    
    PHY_CTRL_CHECK(pc);

    an.flags = 0;
    an.enable = autoneg;
    an.num_lane_adv = 1;
    an.an_mode = phymod_AN_MODE_NONE;

    /* By default, disable cl37 */
    cl37an = 0;
    cl73an = PHYMOD_AN_CAP_CL73;
    if(cl73an) {
        switch (cl73an) {
            case TSCE_CL73_W_BAM:  
                an.an_mode = phymod_AN_MODE_CL73BAM;  
                break ;
            case TSCE_CL73_WO_BAM:
                an.an_mode = phymod_AN_MODE_CL73;  
                break ;
            case TSCE_CL73_HPAM_VS_SW:    
                an.an_mode = phymod_AN_MODE_HPAM;  
                break ;
            case TSCE_CL73_HPAM:
                an.an_mode = phymod_AN_MODE_HPAM;  
                break ;
            case TSCE_CL73_CL37:    
                an.an_mode = phymod_AN_MODE_CL73;  
                break ;
            default:
                break;
        }
    } else if (cl37an) {
        switch (cl37an) {
            case TSCE_CL37_WO_BAM:
                an.an_mode = phymod_AN_MODE_CL37;
                break;
            case TSCE_CL37_W_BAM:
                an.an_mode = phymod_AN_MODE_CL37BAM;
                break;
            default:
                break;
        } 
    } else {
        if(PHY_CTRL_FLAGS(pc) & PHY_F_FIBER_MODE) {
            an.an_mode = phymod_AN_MODE_CL37;
        } else {
            an.an_mode = phymod_AN_MODE_SGMII;
        }
    }

    _qsgmiie_phymod_phy_init(pc, &pm_phy);
    rv = qsgmiie_phy_autoneg_set(&pm_phy, &an);

    return rv;
}

/*
 * Function:    
 *      bcmi_qsgmiie_serdes_autoneg_get
 * Purpose:     
 *      Get the current auto-negotiation status (enabled/busy)
 * Parameters:
 *      pc - PHY control structure
 *      autoneg - (OUT) non-zero indicates autoneg enabled
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcmi_qsgmiie_serdes_autoneg_get(phy_ctrl_t *pc, int *autoneg)
{
    int rv = CDK_E_NONE;
    phymod_autoneg_control_t an;
    phymod_phy_access_t pm_phy;
    int an_complete;

    PHY_CTRL_CHECK(pc);

    CDK_MEMSET(&an, 0x0, sizeof(an));

    _qsgmiie_phymod_phy_init(pc, &pm_phy);
    rv = qsgmiie_phy_autoneg_get(&pm_phy, &an, (uint32_t *)&an_complete);

    *autoneg = 0;
    if (an.enable) {
        *autoneg = 1;
    }
    return rv;
}

/*
 * Function:    
 *      bcmi_qsgmiie_serdes_loopback_set
 * Purpose:     
 *      Set PHY loopback mode.
 * Parameters:
 *      pc - PHY control structure
 *      enable - non-zero enables PHY loopback
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcmi_qsgmiie_serdes_loopback_set(phy_ctrl_t *pc, int enable)
{
    int rv = CDK_E_NONE;
    phymod_phy_access_t pm_phy;

    PHY_CTRL_CHECK(pc);

    _qsgmiie_phymod_phy_init(pc, &pm_phy);
    rv = qsgmiie_phy_loopback_set(&pm_phy, phymodLoopbackGlobalPMD, enable); 
    
    return rv;
}

/*
 * Function:    
 *      bcmi_qsgmiie_serdes_loopback_get
 * Purpose:     
 *      Get the current PHY loopback mode.
 * Parameters:
 *      pc - PHY control structure
 *      enable - (OUT) non-zero indicates PHY loopback enabled
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcmi_qsgmiie_serdes_loopback_get(phy_ctrl_t *pc, int *enable)
{
    int rv = CDK_E_NONE;
    phymod_phy_access_t pm_phy;
    uint32_t lb_enable;

    PHY_CTRL_CHECK(pc);

    *enable = 0;
    _qsgmiie_phymod_phy_init(pc, &pm_phy);
    
    rv = qsgmiie_phy_loopback_get(&pm_phy, phymodLoopbackGlobalPMD, &lb_enable); 
    if (lb_enable) {
        *enable = 1;
    }
    return rv;
}

/*
 * Function:    
 *      bcmi_qsgmiie_serdes_ability_get
 * Purpose:     
 *      Get the abilities of the PHY.
 * Parameters:
 *      pc - PHY control structure
 *      abil - (OUT) ability mask indicating supported options/speeds.
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcmi_qsgmiie_serdes_ability_get(phy_ctrl_t *pc, uint32_t *abil)
{
    PHY_CTRL_CHECK(pc);

    *abil = (PHY_ABIL_2500MB | PHY_ABIL_1000MB | 
             PHY_ABIL_100MB | PHY_ABIL_10MB | PHY_ABIL_SERDES | 
             PHY_ABIL_PAUSE | PHY_ABIL_SGMII | PHY_ABIL_GMII | 
             PHY_ABIL_LOOPBACK);

    return CDK_E_NONE;
}

/*
 * Function:
 *      bcmi_qsgmiie_serdes_config_set
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
bcmi_qsgmiie_serdes_config_set(phy_ctrl_t *pc, phy_config_t cfg, uint32_t val, void *cd)
{
    int rv = CDK_E_NONE;
    phymod_core_access_t pm_core;
    phymod_phy_access_t pm_phy;
    int idx;

    PHY_CTRL_CHECK(pc);

    switch (cfg) {
    case PhyConfig_Enable:
        return CDK_E_NONE;
    case PhyConfig_RemoteLoopback:
        {
            _qsgmiie_phymod_phy_init(pc, &pm_phy);
            rv = qsgmiie_phy_loopback_set(&pm_phy, phymodLoopbackRemotePMD, val);
            return rv;
        }
#if PHY_CONFIG_INCLUDE_XAUI_TX_LANE_MAP_SET
    case PhyConfig_XauiTxLaneRemap:
        {
            phymod_lane_map_t lane_map;
           
            _qsgmiie_phymod_core_init(pc, &pm_core);
            rv = qsgmiie_core_lane_map_get(&pm_core, &lane_map);
            if (CDK_SUCCESS(rv)) {
                for (idx = 0; idx < NUM_LANES; idx++) {
                    lane_map.lane_map_tx[idx] = 
                                (val >> (16 + idx * 4 /*4 bit per lane*/)) & 0xf;
                }
                rv = qsgmiie_core_lane_map_set(&pm_core, &lane_map);
            }
            return rv;
        }
#endif
#if PHY_CONFIG_INCLUDE_XAUI_RX_LANE_MAP_SET
    case PhyConfig_XauiRxLaneRemap:
        {
            phymod_lane_map_t lane_map;
           
            _qsgmiie_phymod_core_init(pc, &pm_core);
            rv = qsgmiie_core_lane_map_get(&pm_core, &lane_map);
            if (CDK_SUCCESS(rv)) {
                for (idx = 0; idx < NUM_LANES; idx++) {
                    lane_map.lane_map_rx[idx] = 
                                 (val >> (idx * 4 /*4 bit per lane*/)) & 0xf;
                }
                rv = qsgmiie_core_lane_map_set(&pm_core, &lane_map);
            }
            return rv;
        }
#endif
#if PHY_CONFIG_INCLUDE_XAUI_TX_POLARITY_SET
    case PhyConfig_XauiTxPolInvert:
        {
            phymod_polarity_t polarity;
        
            _qsgmiie_phymod_phy_init(pc, &pm_phy);
            if (CDK_SUCCESS(rv)) {
                rv = qsgmiie_phy_polarity_get(&pm_phy, &polarity);
            }
                
            polarity.tx_polarity = val;
            if (CDK_SUCCESS(rv)) {
                rv = qsgmiie_phy_polarity_set(&pm_phy, &polarity);
            }
            return rv;
        }
#endif
#if PHY_CONFIG_INCLUDE_XAUI_RX_POLARITY_SET
    case PhyConfig_XauiRxPolInvert:
        {
            phymod_polarity_t polarity;
        
            _qsgmiie_phymod_phy_init(pc, &pm_phy);
            if (CDK_SUCCESS(rv)) {
                rv = qsgmiie_phy_polarity_get(&pm_phy, &polarity);
            }
            
            polarity.rx_polarity = val;
            if (CDK_SUCCESS(rv)) {
                rv = qsgmiie_phy_polarity_set(&pm_phy, &polarity);
            }
            
            return rv;
        }
#endif
#if PHY_CONFIG_INCLUDE_LINK_ABILITIES
    case PhyConfig_AdvLocal: 
        rv = _qsgmiie_abiliby_set(pc, val);
        return rv;
#endif
    case PhyConfig_TxPreemp:
    case PhyConfig_TxPost2:
    case PhyConfig_TxIDrv:
    case PhyConfig_TxPreIDrv:
    case PhyConfig_InitStage:
        return CDK_E_NONE;
    default:
        break;
    }

    return CDK_E_UNAVAIL;
}

/*
 * Function:
 *      bcmi_qsgmiie_serdes_config_get
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
bcmi_qsgmiie_serdes_config_get(phy_ctrl_t *pc, phy_config_t cfg, uint32_t *val, void *cd)
{
    int rv = CDK_E_NONE;
    phymod_core_access_t pm_core;
    phymod_phy_access_t pm_phy;
    int idx;

    PHY_CTRL_CHECK(pc);

    switch (cfg) {
    case PhyConfig_Enable:
        return CDK_E_NONE;
    case PhyConfig_RemoteLoopback:
        {
            _qsgmiie_phymod_phy_init(pc, &pm_phy);
            rv = qsgmiie_phy_loopback_get(&pm_phy, phymodLoopbackRemotePMD, val);
            return rv;
        }
#if PHY_CONFIG_INCLUDE_XAUI_TX_LANE_MAP_SET
    case PhyConfig_XauiTxLaneRemap:
        {
            phymod_lane_map_t lane_map;
           
            _qsgmiie_phymod_core_init(pc, &pm_core);
            rv = qsgmiie_core_lane_map_get(&pm_core, &lane_map);
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
           
            _qsgmiie_phymod_core_init(pc, &pm_core);
            rv = qsgmiie_core_lane_map_get(&pm_core, &lane_map);
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
        
            _qsgmiie_phymod_phy_init(pc, &pm_phy);
            if (CDK_SUCCESS(rv)) {
                rv = qsgmiie_phy_polarity_get(&pm_phy, &polarity);
            }
            *val = polarity.tx_polarity;

            return rv;
        }
#endif
#if PHY_CONFIG_INCLUDE_XAUI_RX_POLARITY_SET
    case PhyConfig_XauiRxPolInvert:
        {
            phymod_polarity_t polarity;
        
            _qsgmiie_phymod_phy_init(pc, &pm_phy);
            if (CDK_SUCCESS(rv)) {
                rv = qsgmiie_phy_polarity_get(&pm_phy, &polarity);
            }
            *val = polarity.rx_polarity;

            return rv;
        }
#endif
    case PhyConfig_TxPreemp:
    case PhyConfig_TxPost2:
    case PhyConfig_TxIDrv:
    case PhyConfig_TxPreIDrv:
    case PhyConfig_InitStage:
        return CDK_E_NONE;
    default:
        break;
    }

    return CDK_E_UNAVAIL;
}

/*
 * Function:
 *      bcmi_qsgmiie_serdes_status_get
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
bcmi_qsgmiie_serdes_status_get(phy_ctrl_t *pc, phy_status_t stat, uint32_t *val)
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
            
            _qsgmiie_phymod_phy_init(pc, &pm_phy);
            rv = _qsgmiie_ref_clk_convert(PORT_REFCLK_INT, &ref_clock);
            if (CDK_SUCCESS(rv)) {
                rv = qsgmiie_phy_interface_config_get(&pm_phy, 0, ref_clock, 
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
phy_driver_t bcmi_qsgmiie_serdes_drv = {
    "bcmi_qsgmiie_serdes", 
    "Internal TSC/Eagle QSGMII SerDes PHY Driver",  
    PHY_DRIVER_F_INTERNAL,
    bcmi_qsgmiie_serdes_probe,              /* pd_probe */
    bcmi_qsgmiie_serdes_notify,             /* pd_notify */
    bcmi_qsgmiie_serdes_reset,              /* pd_reset */
    bcmi_qsgmiie_serdes_init,               /* pd_init */
    bcmi_qsgmiie_serdes_link_get,           /* pd_link_get */
    bcmi_qsgmiie_serdes_duplex_set,         /* pd_duplex_set */
    bcmi_qsgmiie_serdes_duplex_get,         /* pd_duplex_get */
    bcmi_qsgmiie_serdes_speed_set,          /* pd_speed_set */
    bcmi_qsgmiie_serdes_speed_get,          /* pd_speed_get */
    bcmi_qsgmiie_serdes_autoneg_set,        /* pd_autoneg_set */
    bcmi_qsgmiie_serdes_autoneg_get,        /* pd_autoneg_get */
    bcmi_qsgmiie_serdes_loopback_set,       /* pd_loopback_set */
    bcmi_qsgmiie_serdes_loopback_get,       /* pd_loopback_get */
    bcmi_qsgmiie_serdes_ability_get,        /* pd_ability_get */
    bcmi_qsgmiie_serdes_config_set,         /* pd_config_set */
    bcmi_qsgmiie_serdes_config_get,         /* pd_config_get */
    bcmi_qsgmiie_serdes_status_get,         /* pd_status_get */
    NULL                                    /* pd_cable_diag */
};
