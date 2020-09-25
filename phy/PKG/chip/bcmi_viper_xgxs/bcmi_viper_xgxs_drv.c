/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * PHY driver for Broadcom TSC/Eagle internal SerDes.
 */
/*
 *   LAYERING.
 *   This driver has a layered architecture, including:
 *  
 *      Phy Driver           - this module
 *      Eagle "Tier2"        - Eagle specific driver code.  Phymod uses
 *                              a "dispatch layer", similar to bcm dispatch
 *                              to call Tier2 functions.  This code contains
 *                              device specific logic, that typically consists
 *                              of a sequence of lower level "Tier1" functions.
 *      Eagle "Tier1"        - Eagle specific low level driver code.
 *                              Tier1 code is shared by the DV environment and
 *                              by Phymod, whether it is built as part of a
 *                              standalone PHYMOD driver, or included as part
 *                              of an SDK driver.
 */

#include <phy/phy.h>
#include <phy/phy_drvlist.h>
#include <phymod/phymod.h>
#include <phy/chip/bcmi_viper_xgxs_defs.h>
#include <cdk/cdk_debug.h>
#include "viper/viper.h"

#if PHY_CONFIG_PRIVATE_DATA_WORDS > 0

#define PRIV_DATA(_pc) ((_pc)->priv[0])

#define LANE_MAP_COMPRESS(_umap) \
    ((((_umap) & 0x3000) >> 6) | \
     (((_umap) & 0x0300) >> 4) | \
     (((_umap) & 0x0030) >> 2) | \
     (((_umap) & 0x0003) >> 0)) 
#define LANE_MAP_DECOMPRESS(_cmap) \
    ((((_cmap) << 6) & 0x3000) | \
     (((_cmap) << 4) & 0x0300) | \
     (((_cmap) << 2) & 0x0030) | \
     (((_cmap) << 0) & 0x0003))

#define TX_LANE_MAP_GET(_pc) (LANE_MAP_DECOMPRESS(PRIV_DATA(_pc) & 0xff))
#define TX_LANE_MAP_SET(_pc, _val) \
do { \
    PRIV_DATA(_pc) &= ~0xff; \
    PRIV_DATA(_pc) |= LANE_MAP_COMPRESS(_val); \
} while(0)

#define RX_LANE_MAP_GET(_pc) \
    (LANE_MAP_DECOMPRESS((PRIV_DATA(_pc) & 0xff00) >> 8))
#define RX_LANE_MAP_SET(_pc, _val) \
do { \
    PRIV_DATA(_pc) &= ~0xff00; \
    PRIV_DATA(_pc) |= (LANE_MAP_COMPRESS(_val) << 8); \
} while(0)

#define TX_POLARITY_GET(_pc)       ((PRIV_DATA(_pc) & 0xf0000) >> 16)
#define TX_POLARITY_SET(_pc, _val) \
do { \
    PRIV_DATA(_pc) &= ~0xf0000; \
    PRIV_DATA(_pc) |= (((_val) & 0xf) << 16); \
} while(0)

#define RX_POLARITY_GET(_pc)       ((PRIV_DATA(_pc) & 0xf00000) >> 20)
#define RX_POLARITY_SET(_pc, _val) \
do { \
    PRIV_DATA(_pc) &= ~0xf00000; \
    PRIV_DATA(_pc) |= (((_val) & 0xf) << 20); \
} while(0)

#define MULTI_CORE_LANE_MAP_GET(_pc) ((PRIV_DATA(_pc) & 0xf000000) >> 24)
#define MULTI_CORE_LANE_MAP_SET(_pc, _val) \
do { \
    PRIV_DATA(_pc) &= ~0xf000000; \
    PRIV_DATA(_pc) |= (((_val) & 0xf) << 24); \
} while(0)

#else

#define TX_LANE_MAP_GET(_pc) (0)
#define TX_LANE_MAP_SET(_pc, _val)
#define RX_LANE_MAP_GET(_pc) (0)
#define RX_LANE_MAP_SET(_pc, _val)
#define TX_POLARITY_GET(_pc) (0)
#define TX_POLARITY_SET(_pc, _val)
#define RX_POLARITY_GET(_pc) (0)
#define RX_POLARITY_SET(_pc, _val)
#define MULTI_CORE_LANE_MAP_GET(_pc)
#define MULTI_CORE_LANE_MAP_SET(_pc, _val)

#endif /* PHY_CONFIG_PRIVATE_DATA_WORDS */

/* Low level debugging (off by default) */
#ifdef PHY_DEBUG_ENABLE
#define _PHY_DBG(_pc, _stuff) \
    PHY_VERB(_pc, _stuff)
#else
#define _PHY_DBG(_pc, _stuff)
#endif

#define IS_1LANE_PORT(_pc) \
    ((PHY_CTRL_FLAGS(_pc) & (PHY_F_SERDES_MODE | PHY_F_2LANE_MODE)) == PHY_F_SERDES_MODE)
#define IS_2LANE_PORT(_pc) \
    (PHY_CTRL_FLAGS(_pc) & PHY_F_2LANE_MODE)
#define IS_4LANE_PORT(_pc) \
    ((PHY_CTRL_FLAGS(_pc) & PHY_F_SERDES_MODE) == 0)
#define IS_MULTI_CORE_PORT(_pc) \
    (PHY_CTRL_FLAGS(_pc) & PHY_F_MULTI_CORE)

#if PHY_CONFIG_INCLUDE_CHIP_SYMBOLS == 1
extern cdk_symbols_t bcmi_viper_xgxs_symbols;
#define SET_SYMBOL_TABLE(_pc) \
    PHY_CTRL_SYMBOLS(_pc) = &bcmi_viper_xgxs_symbols
#else
#define SET_SYMBOL_TABLE(_pc)
#endif

#define NUM_LANES                       4   /* num of lanes per core */
#define MAX_NUM_LANES                   4   /* max num of lanes per port */
#define LANE_NUM_MASK                   0x3 /* Lane from PHY control instance */

#define VIPER_CL37_HR2SPM_W_10G         0x5
#define VIPER_CL37_HR2SPM               0x4
#define VIPER_CL37_W_10G                0x3
#define VIPER_CL37_WO_BAM               0x2
#define VIPER_CL37_W_BAM                0x1
#define VIPER_CL37_DISABLE              0x0

/* Private PHY flag is used to indicate that firmware is running */
#define PHY_F_FW_RUNNING                PHY_F_PRIVATE

static int CL37AN = TRUE;

/*
 * Function:
 *      _viper_serdes_lane
 * Purpose:
 *      Retrieve eagle lane number for this PHY instance.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      Lane number or -1 if lane is unknown
 */
static int
_viper_serdes_lane(phy_ctrl_t *pc)
{
    uint32_t inst = PHY_CTRL_INST(pc);

    if (inst & PHY_INST_VALID) {

        return inst & LANE_NUM_MASK;
    }
    return -1;
}

/*
 * Function:
 *      _viper_primary_lane
 * Purpose:
 *      Ensure that each viper is initialized only once.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      TRUE/FALSE
 */
static int
_viper_primary_lane(phy_ctrl_t *pc)
{
    uint32_t inst = PHY_CTRL_INST(pc);
    
    return (inst & LANE_NUM_MASK) ? FALSE : TRUE;
}

/*
 * Function:
 *      _viper_phymod_core_t_init
 * Purpose:
 *      Init phymod_core_access structure.
 * Parameters:
 *      pc - PHY control structure
 *      core - PHYMOD core access structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_viper_phymod_core_t_init(phy_ctrl_t *pc, phymod_core_access_t *core)
{
    if (pc == NULL || core == NULL) {
        return CDK_E_PARAM;
    }

    CDK_MEMSET(core,0, sizeof(*core));

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
 *      _viper_phymod_phy_t_init
 * Purpose:
 *      Init phymod_phy_access structure.
 * Parameters:
 *      pc - PHY control structure
 *      core - PHYMOD phy access structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_viper_phymod_phy_t_init(phy_ctrl_t *pc, phymod_phy_access_t *phy)
{
    uint32_t lane_map;
    
    if (pc == NULL || phy == NULL) {
        return CDK_E_PARAM;
    }

    CDK_MEMSET(phy, 0, sizeof(*phy));

    /* Use user data pointer to keep whole phy ctrl data */
    phy->access.user_acc = (void *)pc;  

    lane_map = 0xf;
    if (IS_2LANE_PORT(pc)) {
        lane_map = 0x3;
    } else if (IS_1LANE_PORT(pc)) {
        lane_map = 0x1;
    }
    phy->access.lane_mask = lane_map << _viper_serdes_lane(pc);

    /* Setting the flags for phymod */
    if (PHY_CTRL_FLAGS(pc) & PHY_F_CLAUSE45) {
        PHYMOD_ACC_F_CLAUSE45_CLR(&phy->access);
    }
    
    return CDK_E_NONE;
}


/*
 * Function:
 *      _viper_serdes_stop
 * Purpose:
 *      Put PHY in or out of reset depending on conditions.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_viper_serdes_stop(phy_ctrl_t *pc)
{
    int rv = CDK_E_NONE;
    phymod_phy_access_t pm_phy;
    phymod_phy_tx_lane_control_t tx_control;
    phymod_phy_rx_lane_control_t rx_control;
    phymod_phy_power_t power;
    uint32_t f_any = PHY_F_PHY_DISABLE | PHY_F_PORT_DRAIN;
    uint32_t f_copper = PHY_F_MAC_DISABLE | PHY_F_SPEED_CHG | PHY_F_DUPLEX_CHG;
    int stop;

    stop = 0;
    if ((PHY_CTRL_FLAGS(pc) & f_any) ||
        ((PHY_CTRL_FLAGS(pc) & PHY_F_FIBER_MODE) == 0 &&
         (PHY_CTRL_FLAGS(pc) & f_copper))) {
        stop = 1;
    }

    _PHY_DBG(pc, ("_viper_serdes_stop: stop = %d\n", stop));

    if (!stop) { /* Enable */
        tx_control = phymodTxSquelchOff;
        rx_control = phymodRxSquelchOff;
    } else {
        tx_control = phymodTxSquelchOn;
        rx_control = phymodRxSquelchOn;
    }

    _viper_phymod_phy_t_init(pc, &pm_phy);
    power.tx = tx_control;
    power.rx = rx_control;
    rv = viper_phy_power_set(&pm_phy, &power);

    return rv;
}

/*
 * Function:
 *      _viper_ability_set
 * Purpose:
 *      configure phy ability.
 * Parameters:
 *      pc - PHY control structure
 *      ability - ability value
 * Returns:
 *      CDK_E_xxx
 */
static int
_viper_ability_set(phy_ctrl_t *pc, uint32_t ability)
{
    int rv = CDK_E_NONE;
    phymod_phy_access_t pm_phy;
    phymod_autoneg_ability_t autoneg_ability;
    phy_ctrl_t *lpc;
    int idx;
    
    if (ability & PHY_ABIL_PAUSE_TX) {
        PHYMOD_AN_CAP_ASYM_PAUSE_SET(&autoneg_ability);
    }
    if (ability & PHY_ABIL_PAUSE_RX) {
        PHYMOD_AN_CAP_ASYM_PAUSE_SET(&autoneg_ability);
        PHYMOD_AN_CAP_SYMM_PAUSE_SET(&autoneg_ability);
    } 

    /* Set 1000MB CL37*/
    if (CL37AN){
        if(ability & PHY_ABIL_1000MB_FD) {
            PHYMOD_AN_CAP_CL37_SET(&autoneg_ability);
        }
    } else {    
        /* Set the sgmii speed */
        if(ability & PHY_ABIL_1000MB_FD) {
            PHYMOD_AN_CAP_SGMII_SET(&autoneg_ability);
            autoneg_ability.sgmii_speed = phymod_CL37_SGMII_1000M;
        } else if(ability & PHY_ABIL_100MB_FD) {
            PHYMOD_AN_CAP_SGMII_SET(&autoneg_ability);
            autoneg_ability.sgmii_speed = phymod_CL37_SGMII_100M;
        } else if(ability & PHY_ABIL_10MB_FD) {
            PHYMOD_AN_CAP_SGMII_SET(&autoneg_ability);
            autoneg_ability.sgmii_speed = phymod_CL37_SGMII_10M;
        } else {
            PHYMOD_AN_CAP_SGMII_SET(&autoneg_ability);
            autoneg_ability.sgmii_speed = phymod_CL37_SGMII_1000M;
        }
    }

    lpc = pc;
    for (idx = 0; idx < pc->num_phys; idx++) {
        if (pc->num_phys == 3 && idx > 0) {
            lpc = pc->slave[idx - 1];
            if (lpc == NULL) {
                continue;
            }
        }
        _viper_phymod_phy_t_init(lpc, &pm_phy);
        rv = viper_phy_autoneg_ability_set(&pm_phy, &autoneg_ability);
    }
    
    return rv;
}

/*
 * Function:
 *      _viper_ability_get
 * Purpose:
 *      Get phy ability.
 * Parameters:
 *      pc - PHY control structure
 *      ability - ability value
 * Returns:
 *      CDK_E_xxx
 */
static int
_viper_ability_get(phy_ctrl_t *pc, uint32_t *ability)
{
    int rv = CDK_E_NONE;
    phymod_phy_access_t pm_phy;
    phymod_autoneg_ability_t phymod_autoneg_ability; 
    int reg_ability;
    phymod_cl37_sgmii_speed_t cl37_sgmii_speed;
    
    PHY_CTRL_CHECK(pc);

    _viper_phymod_phy_t_init(pc, &pm_phy);
    rv = viper_phy_autoneg_ability_get(&pm_phy, &phymod_autoneg_ability);

    cl37_sgmii_speed = phymod_autoneg_ability.sgmii_speed;
    reg_ability = phymod_autoneg_ability.capabilities;

    *ability = 0;
    
    /* retrieve CL37 abilities */
    if (reg_ability == PHYMOD_AN_CAP_CL37){
        *ability |= PHY_ABIL_1000MB;
    }
    
    /* retrieve sgmii speed */
    if (cl37_sgmii_speed == phymod_CL37_SGMII_1000M){
        *ability |= PHY_ABIL_1000MB;
    } else if (cl37_sgmii_speed == phymod_CL37_SGMII_100M){
        *ability |= PHY_ABIL_100MB;
    } else if (cl37_sgmii_speed == phymod_CL37_SGMII_10M){
        *ability |= PHY_ABIL_10MB;
    } else {
        *ability |= 0;
    }

    /* retrieve "pause" abilities */
    if (reg_ability == PHYMOD_AN_CAP_ASYM_PAUSE) {
        *ability |= PHY_ABIL_PAUSE_TX;
    } else if (reg_ability == (PHYMOD_AN_CAP_SYMM_PAUSE | PHYMOD_AN_CAP_ASYM_PAUSE)) {
        *ability |= PHY_ABIL_PAUSE_RX;
    } else if (reg_ability == PHYMOD_AN_CAP_SYMM_PAUSE) {
        *ability |= PHY_ABIL_PAUSE;
    }

    return rv;
}

static int
_viper_phymod_inf_config_t_multi_core_set(phy_ctrl_t *pc,
                                         phymod_phy_inf_config_t *if_cfg)
{
    PHYMOD_INTF_MODES_TRIPLE_CORE_SET(if_cfg);

    return CDK_E_NONE;
}

static int
_viper_intf_cfg_set(phy_ctrl_t *pc, uint32_t flags,
                   phymod_phy_inf_config_t *if_cfg)
{
    int rv = CDK_E_NONE;
    int idx;
    phymod_phy_access_t pm_phy;
    phy_ctrl_t *lpc;

    lpc = pc;
    for (idx = 0; idx < pc->num_phys; idx++) {
        if (pc->num_phys == 3 && idx > 0) {
            lpc = pc->slave[idx - 1];
            if (lpc == NULL) {
                continue;
            }
        }
        _viper_phymod_phy_t_init(lpc, &pm_phy);
        if (CDK_SUCCESS(rv)) {
            rv = viper_phy_interface_config_set(&pm_phy, flags, if_cfg);
        }
    }

    return rv;
}

/***********************************************************************
 *
 * PHY DRIVER FUNCTIONS
 */
/*
 * Function:
 *      bcmi_viper_xgxs_probe
 * Purpose:     
 *      Probe for PHY.
 * Parameters:
 *      pc - PHY control structure
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcmi_viper_xgxs_probe(phy_ctrl_t *pc)
{
    int rv = CDK_E_NONE;
    phymod_core_access_t pm_core;
    uint32_t found;
    
    PHY_CTRL_CHECK(pc);
    
    _viper_phymod_core_t_init(pc, &pm_core);
    
    found = 0;
    rv = viper_core_identify(&pm_core, 0, &found);
    if (CDK_SUCCESS(rv) && found) {
        SET_SYMBOL_TABLE(pc);
        return CDK_E_NONE;
    }

    PHY_CTRL_FLAGS(pc) &= ~PHY_F_LANE_CTRL;
    return CDK_E_NOT_FOUND;
}


/*
 * Function:
 *      bcmi_viper_xgxs_notify
 * Purpose:     
 *      Handle PHY notifications.
 * Parameters:
 *      pc - PHY control structure
 *      event - PHY event
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcmi_viper_xgxs_notify(phy_ctrl_t *pc, phy_event_t event)
{
    int ioerr = 0;
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
    rv = _viper_serdes_stop(pc);

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      bcmi_viper_xgxs_reset
 * Purpose:     
 *      Reset PHY.
 * Parameters:
 *      pc - PHY control structure
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcmi_viper_xgxs_reset(phy_ctrl_t *pc)
{
    PHY_CTRL_CHECK(pc);

    return CDK_E_NONE;
}

/*
 * Function:
 *      bcmi_viper_xgxs_init
 * Purpose:     
 *      Initialize PHY driver.
 * Parameters:
 *      pc - PHY control structure
 * Returns:     
 *      CDK_E_NONE
 */
static int
bcmi_viper_xgxs_init(phy_ctrl_t *pc)
{
    int rv = CDK_E_NONE;
    phymod_core_access_t pm_core;
    phymod_phy_access_t pm_phy;
    phymod_core_init_config_t core_init_config;
    phymod_core_init_config_t *cic;
    phymod_core_status_t core_status;
    phymod_phy_init_config_t phy_init_config;
    phymod_phy_inf_config_t *if_cfg;
    phy_ctrl_t *lpc;
    int lane_map_rx = 0, lane_map_tx = 0;
    int lane, idx;

    PHY_CTRL_CHECK(pc);

    pc->num_phys = 1;
    if (IS_MULTI_CORE_PORT(pc)) {
        /* Support triple core */
        pc->num_phys = 3;
    }

    cic = &core_init_config;
    if_cfg = &cic->interface;

    lpc = pc;
    for (idx = 0; idx < pc->num_phys; idx++) {
        if (pc->num_phys == 3 && idx != 0) {
            lpc = pc->slave[idx - 1];
            if (lpc == NULL) {
                continue;
            }
        }
        _viper_phymod_core_t_init(lpc, &pm_core);
        _viper_phymod_phy_t_init(lpc, &pm_phy);
            
        if (_viper_primary_lane(lpc)) {
            CDK_MEMSET(cic, 0, sizeof(*cic));
            /* CORE configuration */
            cic->firmware_load_method = phymodFirmwareLoadMethodInternal;
  
            cic->lane_map.num_of_lanes = NUM_LANES;
            PHYMOD_CORE_INIT_F_FIRMWARE_LOAD_VERIFY_SET(cic);

            lane_map_rx = RX_LANE_MAP_GET(lpc);
            for (lane = 0; lane < NUM_LANES; lane++) {
                cic->lane_map.lane_map_rx[lane] = (lane_map_rx >> (lane * 4)) & 0xf;
            }
                
            lane_map_tx = TX_LANE_MAP_GET(lpc);
            for (lane = 0; lane < NUM_LANES; lane++) {
                cic->lane_map.lane_map_tx[lane] = (lane_map_tx >> (lane * 4)) & 0xf;
            }

            if (pc->num_phys == 3) {
                _viper_phymod_inf_config_t_multi_core_set(pc, if_cfg);
            }
                
            core_status.pmd_active = 0;
            if (CDK_SUCCESS(rv)) {
                rv = viper_core_init(&pm_core, cic, &core_status);
            }
        }
          
        /* PHY configuration */
        /* Initialize the tx taps and driver current*/
        CDK_MEMSET(&phy_init_config, 0, sizeof(phy_init_config));
        
        for (lane = 0; lane < NUM_LANES; lane++) {
            phymod_tx_t *tx_cfg = &phy_init_config.tx[lane];

            tx_cfg->pre = 0xc;
            tx_cfg->main = 0x66;
            tx_cfg->post = 0x0;
            tx_cfg->amp = 0xc;
        }
        phy_init_config.polarity.rx_polarity = RX_POLARITY_GET(lpc);
        phy_init_config.polarity.tx_polarity = TX_POLARITY_GET(lpc);
        phy_init_config.an_en = TRUE;

        if (CDK_SUCCESS(rv)) {
            rv = viper_phy_init(&pm_phy, &phy_init_config);
        }

    }
    {
    phymod_phy_inf_config_t interface_config;
    phymod_interface_t if_type = 0;
    

    if_cfg = &interface_config;
    CDK_MEMSET(if_cfg, 0, sizeof(*if_cfg));
    
    if_cfg = &interface_config;
    if_type = phymodInterfaceSGMII;
    /* Default setting the ref_clock to 156. */
    if_cfg->ref_clock = phymodRefClk156Mhz; 
    if_cfg->interface_type = if_type;
    if_cfg->data_rate = 1000;
    rv = _viper_intf_cfg_set(pc, 0, if_cfg);    
    }
    /* Default mode is fiber 
    PHY_NOTIFY(pc, PhyEvent_ChangeToFiber);
    */

    return rv;
}

/*
 * Function:    
 *      bcmi_viper_xgxs_link_get
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
bcmi_viper_xgxs_link_get(phy_ctrl_t *pc, int *link, int *autoneg_done)
{
    int rv = CDK_E_NONE;
    phymod_phy_access_t pm_phy;
    phymod_autoneg_control_t an;
    int an_done;
    
    PHY_CTRL_CHECK(pc);

    CDK_MEMSET(&an, 0x0, sizeof(an));
    _viper_phymod_phy_t_init(pc, &pm_phy);
    if (CDK_SUCCESS(rv)) {
        rv = viper_phy_autoneg_get(&pm_phy, &an, (uint32_t *)&an_done);
    }
    *autoneg_done = 0;
    if (an.enable) {
        *autoneg_done = an_done;
    }
    
    if (CDK_SUCCESS(rv)) {
        _viper_phymod_phy_t_init(pc, &pm_phy);
        rv = viper_phy_link_status_get(&pm_phy, (uint32_t *)link);
    }

    return rv;
}

/*
 * Function:    
 *      bcmi_viper_xgxs_duplex_set
 * Purpose:     
 *      Set the current duplex mode (forced).
 * Parameters:
 *      pc - PHY control structure
 *      duplex - non-zero indicates full duplex, zero indicates half
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcmi_viper_xgxs_duplex_set(phy_ctrl_t *pc, int duplex)
{
    PHY_CTRL_CHECK(pc);

    return CDK_E_NONE;
}

/*
 * Function:    
 *      bcmi_viper_xgxs_duplex_get
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
bcmi_viper_xgxs_duplex_get(phy_ctrl_t *pc, int *duplex)
{
    PHY_CTRL_CHECK(pc);

    *duplex = 1;

    return CDK_E_NONE;
}

/*
 * Function:    
 *      bcmi_viper_xgxs_speed_set
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
bcmi_viper_xgxs_speed_set(phy_ctrl_t *pc, uint32_t speed)
{
    int rv = CDK_E_NONE;
    phymod_phy_inf_config_t interface_config;
    phymod_phy_inf_config_t *if_cfg;
    int autoneg;
    phymod_interface_t if_type = 0;

    PHY_CTRL_CHECK(pc);

    /* Do not set speed if auto-negotiation is enabled */
    rv = PHY_AUTONEG_GET(pc, &autoneg);
    if (CDK_FAILURE(rv)) {
        return rv;
    }
    if (autoneg) {
        return CDK_E_NONE;
    }

    if_cfg = &interface_config;

    CDK_MEMSET(if_cfg, 0, sizeof(*if_cfg));
    
    if_cfg->pll_divider_req = 40;
    
    if_cfg->data_rate = speed;
    
    /* Default setting the ref_clock to 156. */
    if_cfg->ref_clock = phymodRefClk156Mhz; 
    if (PHY_CTRL_LINE_INTF(pc) == PHY_IF_HIGIG) {
        if_cfg->interface_modes |= PHYMOD_INTF_MODES_HIGIG;
    }    

    switch (speed) {
        case 10:
            if (IS_1LANE_PORT(pc)) {
                if_type = phymodInterfaceSGMII;
            } else {
                return CDK_E_PARAM;
            }
            break;
        case 100:
        case 1000:
#if 1        
            if (IS_1LANE_PORT(pc)) {
                if (PHY_CTRL_FLAGS(pc) & PHY_F_FIBER_MODE) {
                    if_type = phymodInterface1000X;
                } else {
                    if_type = phymodInterfaceSGMII;
                }
            } else {    
                return CDK_E_PARAM;
            }
#else
/* Test for from sdk, only do port update */
                    if_type = phymodInterfaceSGMII;
#endif            
            break;
        case 2500:
            if (IS_1LANE_PORT(pc)) {
                if_type = phymodInterface1000X;
            } else {
                return CDK_E_PARAM;
            }
            break;
        case 10000:
            if_type = phymodInterfaceXGMII;
            break;
        default:

            return CDK_E_PARAM;
    }

    if_cfg->interface_type = if_type;
    rv = _viper_intf_cfg_set(pc, 0, if_cfg);

    return rv;
}

/*
 * Function:    
 *      bcmi_viper_xgxs_speed_get
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
bcmi_viper_xgxs_speed_get(phy_ctrl_t *pc, uint32_t *speed)
{
    int rv = CDK_E_NONE;
    phymod_phy_access_t pm_phy;
    phymod_phy_inf_config_t interface_config;
    phymod_ref_clk_t ref_clock = phymodRefClk156Mhz;

    PHY_CTRL_CHECK(pc);

    _viper_phymod_phy_t_init(pc, &pm_phy);
    
    /* Initialize the data structure */
    interface_config.data_rate = 0;
    
    /* Note that the flags have an option to indicate whether it's ok to reprogram the PLL */
    if (CDK_SUCCESS(rv)) {
        rv = viper_phy_interface_config_get(&pm_phy, 0, ref_clock, 
                                           &interface_config);
    }

    *speed = interface_config.data_rate;
    
    return rv;
}

/*
 * Function:    
 *      bcmi_viper_xgxs_autoneg_set
 * Purpose:     
 *      Enable or disabled auto-negotiation on the specified port.
 * Parameters:
 *      pc - PHY control structure
 *      autoneg - non-zero enables autoneg, zero disables autoneg
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcmi_viper_xgxs_autoneg_set(phy_ctrl_t *pc, int autoneg)
{
    int rv = CDK_E_NONE;
    phymod_autoneg_control_t an;
    phymod_phy_access_t pm_phy;
    phy_ctrl_t *lpc;
    uint32_t cl37an;
    int idx;

    PHY_CTRL_CHECK(pc);

    an.flags = 0;
    an.enable = autoneg;
    an.num_lane_adv = 1;
    if (pc->num_phys == 3) {
        an.num_lane_adv = 10;
    } else if (IS_2LANE_PORT(pc)) {
        an.num_lane_adv = 2;
    } else if (IS_4LANE_PORT(pc)) {
        an.num_lane_adv = 4;
    }
    an.an_mode = phymod_AN_MODE_NONE;

    cl37an = CL37AN;

    if (cl37an) {
        an.an_mode = phymod_AN_MODE_CL37;
    } else {
        if(PHY_CTRL_FLAGS(pc) & PHY_F_FIBER_MODE) {
            an.an_mode = phymod_AN_MODE_CL37;
        } else {
            an.an_mode = phymod_AN_MODE_SGMII;
        }
    }
    
    if (pc->num_phys == 3) {
        if (autoneg) {
            an.flags |= PHYMOD_AN_F_SET_PRIOR_ENABLE;
            lpc = pc;
            for (idx = 0; idx < pc->num_phys; idx++) {
                if (pc->num_phys == 3 && idx != 0) {
                    lpc = pc->slave[idx - 1];
                    if (lpc == NULL) {
                        continue;
                    }
                }
                if (CDK_SUCCESS(rv)) {
                    _viper_phymod_phy_t_init(lpc, &pm_phy);
                    rv = viper_phy_autoneg_set(&pm_phy, &an);
                }
            }
            an.flags &= ~PHYMOD_AN_F_SET_PRIOR_ENABLE;            
        }
    }

    lpc = pc;
    if (pc->num_phys == 3) {
        /* For tripe-PHY, set autoneg to the most significant PHY only */
        lpc = pc->slave[1];
        if (lpc == NULL) {
            lpc = pc;
        }
    }

    if (CDK_SUCCESS(rv)) {
        _viper_phymod_phy_t_init(lpc, &pm_phy);
        rv = viper_phy_autoneg_set(&pm_phy, &an);
    }

    return rv;
}

/*
 * Function:    
 *      bcmi_viper_xgxs_autoneg_get
 * Purpose:     
 *      Get the current auto-negotiation status (enabled/busy)
 * Parameters:
 *      pc - PHY control structure
 *      autoneg - (OUT) non-zero indicates autoneg enabled
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcmi_viper_xgxs_autoneg_get(phy_ctrl_t *pc, int *autoneg)
{
    int rv = CDK_E_NONE;
    phymod_autoneg_control_t an;
    phymod_phy_access_t pm_phy;
    phy_ctrl_t *lpc;
    int an_complete;

    PHY_CTRL_CHECK(pc);

    CDK_MEMSET(&an, 0x0, sizeof(an));

    lpc = pc;
    if (pc->num_phys == 3) {
        /* For tripe-PHY, get autoneg from the most significant PHY only */
        lpc = pc->slave[1];
        if (lpc == NULL) {
            lpc = pc;
        }
    }
    _viper_phymod_phy_t_init(lpc, &pm_phy);
    rv = viper_phy_autoneg_get(&pm_phy, &an, (uint32_t *)&an_complete);

    *autoneg = 0;
    if (an.enable) {
        *autoneg = 1;
    }
    
    return rv;
}

/*
 * Function:    
 *      bcmi_viper_xgxs_loopback_set
 * Purpose:     
 *      Set PHY loopback mode.
 * Parameters:
 *      pc - PHY control structure
 *      enable - non-zero enables PHY loopback
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcmi_viper_xgxs_loopback_set(phy_ctrl_t *pc, int enable)
{
    int rv = CDK_E_NONE;
    phymod_phy_access_t pm_phy;
    phy_ctrl_t *lpc;
    int idx;

    PHY_CTRL_CHECK(pc);

    lpc = pc;
    for (idx = 0; idx < pc->num_phys; idx++) {
        if (pc->num_phys == 3 && idx > 0) {
            lpc = pc->slave[idx - 1];
            if (lpc == NULL) {
                continue;
            }
        }
    
        _viper_phymod_phy_t_init(lpc, &pm_phy);
           
        if (CDK_SUCCESS(rv)) {
            rv = viper_phy_loopback_set(&pm_phy, phymodLoopbackGlobal, enable); 
        }
    }

    return rv;
}

/*
 * Function:    
 *      bcmi_viper_xgxs_loopback_get
 * Purpose:     
 *      Get the current PHY loopback mode.
 * Parameters:
 *      pc - PHY control structure
 *      enable - (OUT) non-zero indicates PHY loopback enabled
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcmi_viper_xgxs_loopback_get(phy_ctrl_t *pc, int *enable)
{
    int rv = CDK_E_NONE;
    phymod_phy_access_t pm_phy;
    uint32_t lb_enable;

    PHY_CTRL_CHECK(pc);

    *enable = 0;

    _viper_phymod_phy_t_init(pc, &pm_phy);
    rv = viper_phy_loopback_get(&pm_phy, phymodLoopbackGlobal, &lb_enable); 
    if (lb_enable) {
        *enable = 1;
    }
     
    return rv;
}

/*
 * Function:    
 *      bcmi_viper_xgxs_ability_get
 * Purpose:     
 *      Get the abilities of the PHY.
 * Parameters:
 *      pc - PHY control structure
 *      abil - (OUT) ability mask indicating supported options/speeds.
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcmi_viper_xgxs_ability_get(phy_ctrl_t *pc, uint32_t *abil)
{
    PHY_CTRL_CHECK(pc);

    *abil = PHY_ABIL_10MB | PHY_ABIL_100MB | PHY_ABIL_1000MB |
            PHY_ABIL_10GB | PHY_ABIL_LOOPBACK | PHY_ABIL_SERDES | 
            PHY_ABIL_SERDES | PHY_ABIL_SGMII | PHY_ABIL_AN;
            
    return CDK_E_NONE;
}

/*
 * Function:
 *      bcmi_viper_xgxs_config_set
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
bcmi_viper_xgxs_config_set(phy_ctrl_t *pc, phy_config_t cfg, uint32_t val, void *cd)
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
        return CDK_E_NONE;
#if PHY_CONFIG_INCLUDE_XAUI_TX_LANE_MAP_SET
    case PhyConfig_XauiTxLaneRemap:
        {
            phymod_lane_map_t lane_map;
            
            TX_LANE_MAP_SET(pc, val);

            _viper_phymod_core_t_init(pc, &pm_core);
            rv = viper_core_lane_map_get(&pm_core, &lane_map);
            if (CDK_SUCCESS(rv)) {
                for (idx = 0; idx < NUM_LANES; idx++) {
                    /*4 bit per lane*/
                    lane_map.lane_map_tx[idx] = (val >> (idx * 4)) & 0xf;
                }
                rv = viper_core_lane_map_set(&pm_core, &lane_map);
            }
            
            return rv;
        }
#endif
#if PHY_CONFIG_INCLUDE_XAUI_RX_LANE_MAP_SET
    case PhyConfig_XauiRxLaneRemap:
        {
            phymod_lane_map_t lane_map;

            RX_LANE_MAP_SET(pc, val);
           
            _viper_phymod_core_t_init(pc, &pm_core);
            rv = viper_core_lane_map_get(&pm_core, &lane_map);
            if (CDK_SUCCESS(rv)) {
                for (idx = 0; idx < NUM_LANES; idx++) {
                    /*4 bit per lane*/
                    lane_map.lane_map_rx[idx] = (val >> (idx * 4)) & 0xf;
                }
                rv = viper_core_lane_map_set(&pm_core, &lane_map);
            }

            return rv;
        }
#endif
#if PHY_CONFIG_INCLUDE_XAUI_TX_POLARITY_SET
    case PhyConfig_XauiTxPolInvert:
        {
            phymod_polarity_t polarity;

            TX_POLARITY_SET(pc, val);
        
            _viper_phymod_phy_t_init(pc, &pm_phy);
            if (CDK_SUCCESS(rv)) {
                rv = viper_phy_polarity_get(&pm_phy, &polarity);
            }
                    
            polarity.tx_polarity = val;
            if (CDK_SUCCESS(rv)) {
                rv = viper_phy_polarity_set(&pm_phy, &polarity);
            }

            return rv;
        }
#endif
#if PHY_CONFIG_INCLUDE_XAUI_RX_POLARITY_SET
    case PhyConfig_XauiRxPolInvert:
        {
            phymod_polarity_t polarity;

            RX_POLARITY_SET(pc, val);
        
            _viper_phymod_phy_t_init(pc, &pm_phy);
            if (CDK_SUCCESS(rv)) {
                rv = viper_phy_polarity_get(&pm_phy, &polarity);
            }
                    
            polarity.rx_polarity = val;
            if (CDK_SUCCESS(rv)) {
                rv = viper_phy_polarity_set(&pm_phy, &polarity);
            }

            return rv;
        }
#endif
#if PHY_CONFIG_INCLUDE_LINK_ABILITIES
    case PhyConfig_AdvLocal: 
        rv = _viper_ability_set(pc, val);
        return rv;
#endif
    case PhyConfig_EEE:
        return CDK_E_NONE;
    case PhyConfig_TxPreemp:
    case PhyConfig_TxPost2:
    case PhyConfig_TxIDrv:
    case PhyConfig_TxPreIDrv:
        return CDK_E_NONE;
    case PhyConfig_MultiCoreLaneMap:
        MULTI_CORE_LANE_MAP_SET(pc, val);
        return rv;
    default:
        break;
    }

    return CDK_E_UNAVAIL;
}

/*
 * Function:
 *      bcmi_viper_xgxs_config_get
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
bcmi_viper_xgxs_config_get(phy_ctrl_t *pc, phy_config_t cfg, uint32_t *val, void *cd)
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
            _viper_phymod_phy_t_init(pc, &pm_phy);
            rv = viper_phy_loopback_get(&pm_phy, phymodLoopbackRemotePMD, val);
            return rv;
        }
#if PHY_CONFIG_INCLUDE_XAUI_TX_LANE_MAP_SET
    case PhyConfig_XauiTxLaneRemap:
        {
            phymod_lane_map_t lane_map;
           
            _viper_phymod_core_t_init(pc, &pm_core);
            rv = viper_core_lane_map_get(&pm_core, &lane_map);
            if (CDK_SUCCESS(rv)) {
                *val = 0;
                for (idx = 0; idx < NUM_LANES; idx++) {
                    *val += (lane_map.lane_map_tx[idx] << (idx * 4 + 16));
                }
            }
            return rv;
        }
#endif
#if PHY_CONFIG_INCLUDE_XAUI_RX_LANE_MAP_SET
    case PhyConfig_XauiRxLaneRemap:
        {
            phymod_lane_map_t lane_map;
           
            _viper_phymod_core_t_init(pc, &pm_core);
            rv = viper_core_lane_map_get(&pm_core, &lane_map);
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
        
            _viper_phymod_phy_t_init(pc, &pm_phy);
            if (CDK_SUCCESS(rv)) {
                rv = viper_phy_polarity_get(&pm_phy, &polarity);
            }
            *val = polarity.tx_polarity;

            return rv;
        }
#endif
#if PHY_CONFIG_INCLUDE_XAUI_RX_POLARITY_SET
    case PhyConfig_XauiRxPolInvert:
        {
            phymod_polarity_t polarity;
        
            _viper_phymod_phy_t_init(pc, &pm_phy);
            if (CDK_SUCCESS(rv)) {
                rv = viper_phy_polarity_get(&pm_phy, &polarity);
            }
            *val = polarity.rx_polarity;

            return rv;
        }
#endif
#if PHY_CONFIG_INCLUDE_LINK_ABILITIES
    case PhyConfig_AdvLocal: 
        rv = _viper_ability_get(pc, val);
        return rv;
#endif
    case PhyConfig_EEE:
        {
            *val = 0;
            return rv;
        }
    case PhyConfig_TxPreemp:
    case PhyConfig_TxPost2:
    case PhyConfig_TxIDrv:
    case PhyConfig_TxPreIDrv:
    case PhyConfig_InitStage:
        return CDK_E_NONE;
    case PhyConfig_MultiCoreLaneMap:
        *val = MULTI_CORE_LANE_MAP_GET(pc);
        return rv;
    default:
        break;
    }

    return CDK_E_UNAVAIL;
}

/*
 * Function:
 *      bcmi_viper_xgxs_status_get
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
bcmi_viper_xgxs_status_get(phy_ctrl_t *pc, phy_status_t stat, uint32_t *val)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    phymod_phy_access_t pm_phy;
    phymod_ref_clk_t ref_clock = phymodRefClk156Mhz;
    
    PHY_CTRL_CHECK(pc);

    switch (stat) {
    case PhyStatus_LineInterface:
        {
            phymod_phy_inf_config_t interface_config;
            
            CDK_MEMSET(&interface_config, 0x0, sizeof(interface_config));
            
            _viper_phymod_phy_t_init(pc, &pm_phy);
            if (CDK_SUCCESS(rv)) {
                rv = viper_phy_interface_config_get(&pm_phy, 0, ref_clock, 
                                                   &interface_config);
            }
            
            switch (interface_config.interface_type) {
            case phymodInterfaceXGMII:
                *val = PHY_IF_XGMII;
                break;
            case phymodInterfaceXLAUI:
                *val = PHY_IF_XLAUI;
                break;
            case phymodInterface1000X:
                *val = PHY_IF_GMII;
                break;
            case phymodInterfaceKX:
                *val = PHY_IF_KX;
                break;
            case phymodInterfaceSGMII:
                *val = PHY_IF_SGMII;
                break;
            case phymodInterfaceKR4:
                *val = PHY_IF_KR;
                break;
            default:
                *val = PHY_IF_SGMII;
                break;
            }
            return rv;
        }
    default:
        break;
    }

    return ioerr ? CDK_E_IO : rv;
}

/* Public PHY Driver Structure */
phy_driver_t bcmi_viper_xgxs_drv = {
    "bcmi_viper_xgxs", 
    "Internal VIPER SerDes PHY Driver",  
    PHY_DRIVER_F_INTERNAL,
    bcmi_viper_xgxs_probe,                /* pd_probe */
    bcmi_viper_xgxs_notify,               /* pd_notify */
    bcmi_viper_xgxs_reset,                /* pd_reset */
    bcmi_viper_xgxs_init,                 /* pd_init */
    bcmi_viper_xgxs_link_get,             /* pd_link_get */
    bcmi_viper_xgxs_duplex_set,           /* pd_duplex_set */
    bcmi_viper_xgxs_duplex_get,           /* pd_duplex_get */
    bcmi_viper_xgxs_speed_set,            /* pd_speed_set */
    bcmi_viper_xgxs_speed_get,            /* pd_speed_get */
    bcmi_viper_xgxs_autoneg_set,          /* pd_autoneg_set */
    bcmi_viper_xgxs_autoneg_get,          /* pd_autoneg_get */
    bcmi_viper_xgxs_loopback_set,         /* pd_loopback_set */
    bcmi_viper_xgxs_loopback_get,         /* pd_loopback_get */
    bcmi_viper_xgxs_ability_get,          /* pd_ability_get */
    bcmi_viper_xgxs_config_set,           /* pd_config_set */
    bcmi_viper_xgxs_config_get,           /* pd_config_get */
    bcmi_viper_xgxs_status_get,           /* pd_status_get */
    NULL                                 /* pd_cable_diag */
};
