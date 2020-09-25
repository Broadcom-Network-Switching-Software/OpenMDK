/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * PHY driver for Broadcom TSC/Falcon internal SerDes.
 */
/*
 *   LAYERING.
 *   This driver has a layered architecture, including:
 *  
 *      Phy Driver           - this module
 *      Falcon "Tier2"        - Falcon specific driver code.  Phymod uses
 *                              a "dispatch layer", similar to bcm dispatch
 *                              to call Tier2 functions.  This code contains
 *                              device specific logic, that typically consists
 *                              of a sequence of lower level "Tier1" functions.
 *      Falcon "Tier1"        - Falcon specific low level driver code.
 *                              Tier1 code is shared by the DV environment and
 *                              by Phymod, whether it is built as part of a
 *                              standalone PHYMOD driver, or included as part
 *                              of an SDK driver.
 */

#include <phy/phy.h>
#include <phy/phy_drvlist.h>
#include <phymod/phymod.h>
#include <phy/chip/bcmi_tscf_gen3_xgxs_defs.h>

#include "tscf_gen3/tscf_gen3.h"
#include <cdk/cdk_debug.h>

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

#else

#define TX_LANE_MAP_GET(_pc) (0)
#define TX_LANE_MAP_SET(_pc, _val)
#define RX_LANE_MAP_GET(_pc) (0)
#define RX_LANE_MAP_SET(_pc, _val)
#define TX_POLARITY_GET(_pc) (0)
#define TX_POLARITY_SET(_pc, _val)
#define RX_POLARITY_GET(_pc) (0)
#define RX_POLARITY_SET(_pc, _val)

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


#if PHY_CONFIG_INCLUDE_CHIP_SYMBOLS == 1
extern cdk_symbols_t bcmi_tscf_gen3_xgxs_symbols;
#define SET_SYMBOL_TABLE(_pc) \
    PHY_CTRL_SYMBOLS(_pc) = &bcmi_tscf_gen3_xgxs_symbols
#else
#define SET_SYMBOL_TABLE(_pc)
#endif

#define NUM_LANES                       4   /* num of lanes per core */
#define MAX_NUM_LANES                   4   /* max num of lanes per port */
#define LANE_NUM_MASK                   0x3 /* Lane from PHY control instance */

#define TEFMOD_CL73_CL37                0x5
#define TEFMOD_CL73_HPAM                0x4
#define TEFMOD_CL73_HPAM_VS_SW          0x8
#define TEFMOD_CL73_WO_BAM              0x2
#define TEFMOD_CL73_W_BAM               0x1
#define TEFMOD_CL73_DISABLE             0x0

#define TEFMOD_CL37_HR2SPM_W_10G        0x5
#define TEFMOD_CL37_HR2SPM              0x4
#define TEFMOD_CL37_W_10G               0x3
#define TEFMOD_CL37_WO_BAM              0x2
#define TEFMOD_CL37_W_BAM               0x1
#define TEFMOD_CL37_DISABLE             0x0

/* Private PHY flag is used to indicate that firmware is running */
#define PHY_F_FW_RUNNING                PHY_F_PRIVATE

#define PORT_REFCLK_INT                 156

/***********************************************************************
 *
 * HELPER FUNCTIONS
 */
/*
 * Function:
 *      _tscf_gen3_serdes_lane
 * Purpose:
 *      Retrieve falcon lane number for this PHY instance.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      Lane number or -1 if lane is unknown
 */
static int
_tscf_gen3_serdes_lane(phy_ctrl_t *pc)
{
    uint32_t inst = PHY_CTRL_INST(pc);

    if (inst & PHY_INST_VALID) {
        return inst & LANE_NUM_MASK;
    }
    return -1;
}

/*
 * Function:
 *      _tscf_gen3_primary_lane
 * Purpose:
 *      Ensure that each tscf is initialized only once.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      TRUE/FALSE
 */
static int
_tscf_gen3_primary_lane(phy_ctrl_t *pc)
{
    uint32_t inst = PHY_CTRL_INST(pc);
    
    return (inst & LANE_NUM_MASK) ? FALSE : TRUE;
}

/*
 * Function:
 *      _tscf_gen3_phymod_core_t_init
 * Purpose:
 *      Init phymod_core_access structure.
 * Parameters:
 *      pc - PHY control structure
 *      core - PHYMOD core access structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_tscf_gen3_phymod_core_t_init(phy_ctrl_t *pc, phymod_core_access_t *core)
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
 *      _tscf_gen3_phymod_phy_t_init
 * Purpose:
 *      Init phymod_phy_access structure.
 * Parameters:
 *      pc - PHY control structure
 *      core - PHYMOD phy access structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_tscf_gen3_phymod_phy_t_init(phy_ctrl_t *pc, phymod_phy_access_t *phy)
{
    uint32_t lane_map;
    
    if (pc == NULL || phy == NULL) {
        return CDK_E_PARAM;
    }

    CDK_MEMSET(phy, 0, sizeof(*phy));

    /* Use pc->addr_offset to adjust the address for multiple phy access */
    pc->addr_offset = 0;

    /* Use user data pointer to keep whole phy ctrl data */
    phy->access.user_acc = (void *)pc;  

    lane_map = 0xf;
    if (IS_2LANE_PORT(pc)) {
        lane_map = 0x3;
    } else if (IS_1LANE_PORT(pc)) {
        lane_map = 0x1;
    }
    phy->access.lane_mask = lane_map << _tscf_gen3_serdes_lane(pc);

    /* Setting the flags for phymod */
    if (PHY_CTRL_FLAGS(pc) & PHY_F_CLAUSE45) {
        PHYMOD_ACC_F_CLAUSE45_CLR(&phy->access);
    }
    
    return CDK_E_NONE;
}

/*
 * Function:
 *      _tscf_gen3_firmware_loader
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
_tscf_gen3_firmware_loader(const phymod_core_access_t *pm_core,
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
 *      _tscf_gen3_serdes_stop
 * Purpose:
 *      Put PHY in or out of reset depending on conditions.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_tscf_gen3_serdes_stop(phy_ctrl_t *pc)
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

    _PHY_DBG(pc, ("_tscf_gen3_serdes_stop: stop = %d\n", stop));

    if (!stop) { /* Enable */
        tx_control = phymodTxSquelchOff;
        rx_control = phymodRxSquelchOff;
    } else {
        tx_control = phymodTxSquelchOn;
        rx_control = phymodRxSquelchOn;
    }

    _tscf_gen3_phymod_phy_t_init(pc, &pm_phy);
    if (CDK_SUCCESS(rv)) {
        rv = tscf_gen3_phy_tx_lane_control_set(&pm_phy, tx_control); 
    }
    if (CDK_SUCCESS(rv)) {
        rv = tscf_gen3_phy_rx_lane_control_set(&pm_phy, rx_control); 
    }

    return rv;
}

/*
 * Function:
 *      _tscf_gen3_ability_set
 * Purpose:
 *      configure phy ability.
 * Parameters:
 *      pc - PHY control structure
 *      ability - ability value
 * Returns:
 *      CDK_E_xxx
 */
static int
_tscf_gen3_ability_set(phy_ctrl_t *pc, uint32_t ability)
{
    int rv = CDK_E_NONE;
    phymod_phy_access_t pm_phy;
    phymod_autoneg_ability_t autoneg_ability;
    phymod_autoneg_ability_t *an_ablt = &autoneg_ability;
    uint32_t an_cap, an_bam37_ability, an_bam73_ability;

    CDK_MEMSET(an_ablt, 0, sizeof(an_ablt));

    an_cap = 0;
    an_bam73_ability = 0;
    an_bam37_ability = 0;

    /* Set an_cap */
    if (IS_4LANE_PORT(pc)) {
        if (ability & PHY_ABIL_100GB) {
            if (PHY_CTRL_LINE_INTF(pc) == PHY_IF_CR) {
                PHYMOD_AN_CAP_100G_CR4_SET(an_cap);
            } else {
                PHYMOD_AN_CAP_100G_KR4_SET(an_cap);
            }
        }
        if (ability & PHY_ABIL_40GB) {
            if (PHY_CTRL_LINE_INTF(pc) == PHY_IF_CR) {
                PHYMOD_AN_CAP_40G_CR4_SET(an_cap);
            } else {
                PHYMOD_AN_CAP_40G_KR4_SET(an_cap);
            }
        }
        if (ability & PHY_ABIL_10GB) {
            PHYMOD_AN_CAP_10G_KX4_SET(an_cap);
        }
    } else if (IS_2LANE_PORT(pc)) {
        if (ability & PHY_ABIL_20GB) {
            if (PHY_CTRL_LINE_INTF(pc) == PHY_IF_CR) {
                PHYMOD_BAM_CL73_CAP_20G_CR2_SET(an_bam73_ability);
            } else {
                PHYMOD_BAM_CL73_CAP_20G_KR2_SET(an_bam73_ability);
            }
        }
        if (ability & PHY_ABIL_40GB) {
            if (PHY_CTRL_LINE_INTF(pc) == PHY_IF_CR) {
                PHYMOD_BAM_CL73_CAP_40G_CR2_SET(an_bam73_ability);
            } else {
                PHYMOD_BAM_CL73_CAP_40G_KR2_SET(an_bam73_ability);
            }
        }
    } else {
        if (ability & PHY_ABIL_25GB) {
            if (PHY_CTRL_LINE_INTF(pc) == PHY_IF_CR) {
                PHYMOD_BAM_CL73_CAP_25G_CR1_SET(an_bam73_ability);
            } else {
                PHYMOD_BAM_CL73_CAP_25G_KR1_SET(an_bam73_ability);
            }
        }
    }

    /* Set an_bam37_ability */
    if (IS_4LANE_PORT(pc)) {
        if (ability & PHY_ABIL_40GB) { 
            PHYMOD_BAM_CL37_CAP_40G_SET(an_bam37_ability);
        }
        if (ability & PHY_ABIL_30GB) {
            PHYMOD_BAM_CL37_CAP_31P5G_SET(an_bam37_ability);
        }
        if (ability & PHY_ABIL_25GB) {
            PHYMOD_BAM_CL37_CAP_25P455G_SET(an_bam37_ability);
        }
        if (ability & PHY_ABIL_21GB) {
            PHYMOD_BAM_CL37_CAP_21G_X4_SET(an_bam37_ability);
        }
        if (ability & PHY_ABIL_20GB) {
            PHYMOD_BAM_CL37_CAP_20G_X4_SET(an_bam37_ability);
            PHYMOD_BAM_CL37_CAP_20G_X4_CX4_SET(an_bam37_ability);
        }
        if (ability & PHY_ABIL_16GB) {
            PHYMOD_BAM_CL37_CAP_16G_X4_SET(an_bam37_ability);
        }
        if (ability & PHY_ABIL_13GB) {
            PHYMOD_BAM_CL37_CAP_13G_X4_SET(an_bam37_ability);
        }
        if (ability & PHY_ABIL_10GB) {
            PHYMOD_BAM_CL37_CAP_10G_CX4_SET(an_bam37_ability);
        }     
        if (ability & PHY_ABIL_2500MB) {
            PHYMOD_BAM_CL37_CAP_2P5G_SET(an_bam37_ability);
        }
        if (ability & PHY_ABIL_1000MB_FD) {
            PHYMOD_AN_CAP_1G_KX_SET(an_cap);
        }
    } else if (IS_2LANE_PORT(pc)) {
        if (ability & PHY_ABIL_20GB) {
            PHYMOD_BAM_CL37_CAP_20G_X2_SET(an_bam37_ability);
            PHYMOD_BAM_CL37_CAP_20G_X2_CX4_SET(an_bam37_ability);
        }
        if (ability & PHY_ABIL_16GB) {
            PHYMOD_BAM_CL37_CAP_15P75G_R2_SET(an_bam37_ability);
        } 
        if (ability & PHY_ABIL_13GB) {
            PHYMOD_BAM_CL37_CAP_12P7_DXGXS_SET(an_bam37_ability);
        } 
        if (ability & PHY_ABIL_10GB) {
            PHYMOD_BAM_CL37_CAP_10G_X2_CX4_SET(an_bam37_ability);
            PHYMOD_BAM_CL37_CAP_10G_DXGXS_SET(an_bam37_ability);
        }
        if (ability & PHY_ABIL_2500MB) {
            PHYMOD_BAM_CL37_CAP_2P5G_SET(an_bam37_ability);
        }
        if (ability & PHY_ABIL_1000MB_FD) {
            PHYMOD_AN_CAP_1G_KX_SET(an_cap);
        }
    } else {
        if (ability & PHY_ABIL_10GB) {
            PHYMOD_AN_CAP_10G_KR_SET(an_cap);
        }
        if (ability & PHY_ABIL_2500MB) {
            PHYMOD_BAM_CL37_CAP_2P5G_SET(an_bam37_ability);
        }
        if (ability & PHY_ABIL_1000MB_FD) {
            PHYMOD_AN_CAP_1G_KX_SET(an_cap);
        }
    }

    an_ablt->an_cap = an_cap;
    an_ablt->cl73bam_cap = an_bam73_ability;
    an_ablt->cl37bam_cap = an_bam37_ability;
    
    if (ability & PHY_ABIL_PAUSE_TX) {
        PHYMOD_AN_CAP_ASYM_PAUSE_SET(an_ablt);
    }
    if (ability & PHY_ABIL_PAUSE_RX) {
        PHYMOD_AN_CAP_ASYM_PAUSE_SET(an_ablt);
        PHYMOD_AN_CAP_SYMM_PAUSE_SET(an_ablt);
    } 

    /* Set the sgmii speed */
    if (ability & PHY_ABIL_1000MB_FD) {
        PHYMOD_AN_CAP_SGMII_SET(an_ablt);
        an_ablt->sgmii_speed = phymod_CL37_SGMII_1000M;
    } else if(ability & PHY_ABIL_100MB_FD) {
        PHYMOD_AN_CAP_SGMII_SET(an_ablt);
        an_ablt->sgmii_speed = phymod_CL37_SGMII_100M;
    } else if(ability & PHY_ABIL_10MB_FD) {
        PHYMOD_AN_CAP_SGMII_SET(an_ablt);
        an_ablt->sgmii_speed = phymod_CL37_SGMII_10M;
    } else {
        PHYMOD_AN_CAP_SGMII_SET(an_ablt);
        an_ablt->sgmii_speed = phymod_CL37_SGMII_1000M;
    }

    /* Check if we need to set cl37 attribute */
    an_ablt->an_cl72 = 1;
    if (PHY_CTRL_LINE_INTF(pc) == PHY_IF_HIGIG) {
        an_ablt->an_hg2 = 1;
    }
    
    _tscf_gen3_phymod_phy_t_init(pc, &pm_phy);
    rv = tscf_gen3_phy_autoneg_ability_set(&pm_phy, an_ablt);
    
    return rv;
}

/*
 * Function:
 *      _tscf_gen3_ability_get
 * Purpose:
 *      Get phy ability.
 * Parameters:
 *      pc - PHY control structure
 *      ability - ability value
 * Returns:
 *      CDK_E_xxx
 */
static int
_tscf_gen3_ability_get(phy_ctrl_t *pc, uint32_t *ability)
{
    int rv = CDK_E_NONE;
    phymod_phy_access_t pm_phy;
    phymod_autoneg_ability_t autoneg_ability; 
    int reg37_ability, reg73_ability, reg_ability;

    PHY_CTRL_CHECK(pc);

    _tscf_gen3_phymod_phy_t_init(pc, &pm_phy);
    rv = tscf_gen3_phy_autoneg_ability_get(&pm_phy, &autoneg_ability);

    *ability = 0;

    /* retrieve CL73 abilities */
    reg73_ability = autoneg_ability.an_cap;
    *ability |= PHYMOD_AN_CAP_100G_CR4_GET(reg73_ability) ? PHY_ABIL_100GB : 0;
    *ability |= PHYMOD_AN_CAP_100G_KR4_GET(reg73_ability) ? PHY_ABIL_100GB : 0;
    *ability |= PHYMOD_AN_CAP_40G_CR4_GET(reg73_ability) ? PHY_ABIL_40GB : 0;
    *ability |= PHYMOD_AN_CAP_40G_KR4_GET(reg73_ability) ? PHY_ABIL_40GB : 0;
    *ability |= PHYMOD_AN_CAP_10G_KR_GET(reg73_ability) ? PHY_ABIL_10GB : 0;
    *ability |= PHYMOD_AN_CAP_10G_KX4_GET(reg73_ability) ? PHY_ABIL_10GB : 0;
    *ability |= PHYMOD_AN_CAP_1G_KX_GET(reg73_ability) ? PHY_ABIL_1000MB_FD : 0;

    /* retrieve CL73bam abilities */
    reg73_ability = autoneg_ability.cl73bam_cap;
    *ability |= PHYMOD_BAM_CL73_CAP_40G_CR2_GET(reg73_ability) ? PHY_ABIL_40GB : 0;
    *ability |= PHYMOD_BAM_CL73_CAP_40G_KR2_GET(reg73_ability) ? PHY_ABIL_40GB : 0;
    *ability |= PHYMOD_BAM_CL73_CAP_20G_CR2_GET(reg73_ability) ? PHY_ABIL_20GB : 0;
    *ability |= PHYMOD_BAM_CL73_CAP_20G_KR2_GET(reg73_ability) ? PHY_ABIL_20GB : 0;

    /* retrieve CL37 abilities */
    reg37_ability = autoneg_ability.cl37bam_cap;
    *ability |= PHYMOD_BAM_CL37_CAP_40G_GET(reg37_ability) ? PHY_ABIL_40GB : 0;
    *ability |= PHYMOD_BAM_CL37_CAP_31P5G_GET(reg37_ability) ? PHY_ABIL_30GB : 0; 
    *ability |= PHYMOD_BAM_CL37_CAP_25P455G_GET(reg37_ability) ? PHY_ABIL_25GB : 0;
    *ability |= PHYMOD_BAM_CL37_CAP_21G_X4_GET(reg37_ability) ? PHY_ABIL_21GB : 0;
    *ability |= PHYMOD_BAM_CL37_CAP_20G_X2_CX4_GET(reg37_ability) ? PHY_ABIL_20GB : 0;
    *ability |= PHYMOD_BAM_CL37_CAP_20G_X2_GET(reg37_ability) ? PHY_ABIL_20GB : 0;
    *ability |= PHYMOD_BAM_CL37_CAP_20G_X4_GET(reg37_ability) ? PHY_ABIL_20GB : 0;
    *ability |= PHYMOD_BAM_CL37_CAP_20G_X4_CX4_GET(reg37_ability) ? PHY_ABIL_20GB : 0;
    *ability |= PHYMOD_BAM_CL37_CAP_16G_X4_GET(reg37_ability) ? PHY_ABIL_16GB : 0;
    *ability |= PHYMOD_BAM_CL37_CAP_15P75G_R2_GET(reg37_ability) ? PHY_ABIL_16GB : 0;
    *ability |= PHYMOD_BAM_CL37_CAP_13G_X4_GET(reg37_ability) ? PHY_ABIL_13GB : 0;
    *ability |= PHYMOD_BAM_CL37_CAP_12P7_DXGXS_GET(reg37_ability) ? PHY_ABIL_13GB : 0;
    *ability |= PHYMOD_BAM_CL37_CAP_10G_X2_CX4_GET(reg37_ability) ? PHY_ABIL_10GB : 0;
    *ability |= PHYMOD_BAM_CL37_CAP_10G_DXGXS_GET(reg37_ability) ? PHY_ABIL_10GB : 0;
    *ability |= PHYMOD_BAM_CL37_CAP_10G_CX4_GET(reg37_ability) ? PHY_ABIL_10GB : 0;
    *ability |= PHYMOD_BAM_CL37_CAP_10G_HIGIG_GET(reg37_ability) ? PHY_ABIL_10GB : 0; /* 4-lane */
    *ability |= PHYMOD_BAM_CL37_CAP_2P5G_GET(reg37_ability) ? PHY_ABIL_2500MB : 0;
    *ability |= PHY_ABIL_1000MB_FD;

    /* retrieve "pause" abilities */
    reg_ability = autoneg_ability.capabilities;
    if (reg_ability == PHYMOD_AN_CAP_ASYM_PAUSE) {
        *ability |= PHY_ABIL_PAUSE_TX;
    } else if (reg_ability == (PHYMOD_AN_CAP_SYMM_PAUSE|PHYMOD_AN_CAP_ASYM_PAUSE)) {
        *ability |= PHY_ABIL_PAUSE_RX;
    } else if (reg_ability == PHYMOD_AN_CAP_SYMM_PAUSE) {
        *ability |= PHY_ABIL_PAUSE;
    }

    return rv;
}


static int 
_tscf_gen3_ref_clk_convert(int port_refclk_int, phymod_ref_clk_t *ref_clk)
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
            return CDK_E_PARAM;
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
 *      bcmi_tscf_gen3_xgxs_probe
 * Purpose:     
 *      Probe for PHY.
 * Parameters:
 *      pc - PHY control structure
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcmi_tscf_gen3_xgxs_probe(phy_ctrl_t *pc)
{
    int rv = CDK_E_NONE;
    phymod_core_access_t pm_core;
    uint32_t found;
CDK_PRINTF("\n fn(%s) ln(%d) port(%d)", __FUNCTION__, __LINE__, pc->port);
    PHY_CTRL_CHECK(pc);
CDK_PRINTF("\n fn(%s) ln(%d) port(%d)", __FUNCTION__, __LINE__, pc->port);
    PHY_CTRL_FLAGS(pc) |= PHY_F_LANE_CTRL;
    _tscf_gen3_phymod_core_t_init(pc, &pm_core);
CDK_PRINTF("\n fn(%s) ln(%d) port(%d)", __FUNCTION__, __LINE__, pc->port);
    found = 0;
    rv = tscf_gen3_core_identify(&pm_core, 0, &found);    
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
 *      bcmi_tscf_gen3_xgxs_notify
 * Purpose:     
 *      Handle PHY notifications.
 * Parameters:
 *      pc - PHY control structure
 *      event - PHY event
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcmi_tscf_gen3_xgxs_notify(phy_ctrl_t *pc, phy_event_t event)
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
    rv = _tscf_gen3_serdes_stop(pc);

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      bcmi_tscf_gen3_xgxs_reset
 * Purpose:     
 *      Reset PHY.
 * Parameters:
 *      pc - PHY control structure
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcmi_tscf_gen3_xgxs_reset(phy_ctrl_t *pc)
{
    PHY_CTRL_CHECK(pc);

    return CDK_E_NONE;
}

/*
 * Function:
 *      bcmi_tscf_gen3_xgxs_init
 * Purpose:     
 *      Initialize PHY driver.
 * Parameters:
 *      pc - PHY control structure
 * Returns:     
 *      CDK_E_NONE
 */
static int
bcmi_tscf_gen3_xgxs_init(phy_ctrl_t *pc)
{
    int rv = CDK_E_NONE;
    phymod_core_access_t pm_core;
    phymod_phy_access_t pm_phy;
    phymod_core_init_config_t core_init_config;
    phymod_core_init_config_t *cic;
    phymod_core_status_t core_status;
    phymod_phy_init_config_t phy_init_config;
    int lane_map_rx, lane_map_tx;
    int lane;

    PHY_CTRL_CHECK(pc);

    pc->num_phys = 1;
    cic = &core_init_config;

    _tscf_gen3_phymod_core_t_init(pc, &pm_core);
    _tscf_gen3_phymod_phy_t_init(pc, &pm_phy);
        
    if (_tscf_gen3_primary_lane(pc)) {
        CDK_MEMSET(cic, 0, sizeof(*cic));
        /* CORE configuration */
        cic->firmware_load_method = phymodFirmwareLoadMethodInternal;
            
        /* Check for external loader */
        if (PHY_CTRL_FW_HELPER(pc)) {
            cic->firmware_loader = _tscf_gen3_firmware_loader;
            cic->firmware_load_method = phymodFirmwareLoadMethodExternal;
        }
   
        cic->firmware_load_method &= 0xff; /*clear checksum bits*/
        cic->lane_map.num_of_lanes = NUM_LANES;

        lane_map_rx = RX_LANE_MAP_GET(pc);
        if (lane_map_rx == 0) {
            lane_map_rx = 0x3210;
        }
        for (lane = 0; lane < NUM_LANES; lane++) {
            cic->lane_map.lane_map_rx[lane] = (lane_map_rx >> (lane * 4)) & 0xf;
        }
            
        lane_map_tx = TX_LANE_MAP_GET(pc);
        if (lane_map_tx == 0) {
            lane_map_tx = 0x3210;
        }
        for (lane = 0; lane < NUM_LANES; lane++) {
            cic->lane_map.lane_map_tx[lane] = (lane_map_tx >> (lane * 4)) & 0xf;
        }
            
        core_status.pmd_active = 0;
        if (CDK_SUCCESS(rv)) {
            rv = tscf_gen3_core_init(&pm_core, cic, &core_status);
        }
    }
        
    /* PHY configuration */
    /* Initialize the tx taps and driver current*/
    CDK_MEMSET(&phy_init_config, 0, sizeof(phy_init_config));
    
    for (lane = 0; lane < NUM_LANES; lane++) {
        phymod_tx_t *tx_cfg = &phy_init_config.tx[lane];

        tx_cfg->pre = 0x0;
        tx_cfg->main = 0x64;
        tx_cfg->post = 0xc;
        tx_cfg->post2 = 0x0;
        tx_cfg->post3 = 0x0;
        tx_cfg->amp = 0x8;
    }
    
    phy_init_config.polarity.rx_polarity = RX_POLARITY_GET(pc);
    phy_init_config.polarity.tx_polarity = TX_POLARITY_GET(pc);
    phy_init_config.cl72_en = TRUE;
    phy_init_config.an_en = TRUE;

    if (CDK_SUCCESS(rv)) {
        rv = tscf_gen3_phy_init(&pm_phy, &phy_init_config);
    }

    /* Default mode is fiber */
    PHY_NOTIFY(pc, PhyEvent_ChangeToFiber);

    return rv;
}

/*
 * Function:    
 *      bcmi_tscf_gen3_xgxs_link_get
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
bcmi_tscf_gen3_xgxs_link_get(phy_ctrl_t *pc, int *link, int *autoneg_done)
{
    int rv = CDK_E_NONE;
    phymod_phy_access_t pm_phy;
    phymod_autoneg_control_t an;
    int an_done;
    
    PHY_CTRL_CHECK(pc);

    CDK_MEMSET(&an, 0x0, sizeof(an));

    _tscf_gen3_phymod_phy_t_init(pc, &pm_phy);
    if (CDK_SUCCESS(rv)) {
        rv = tscf_gen3_phy_autoneg_get(&pm_phy, &an, (uint32_t *)&an_done);
    }

    *autoneg_done = 0;
    if (an.enable) {
        *autoneg_done = an_done;
    }
    
    if (CDK_SUCCESS(rv)) {
        _tscf_gen3_phymod_phy_t_init(pc, &pm_phy);
        rv = tscf_gen3_phy_link_status_get(&pm_phy, (uint32_t *)link);
    }

    return rv;
}

/*
 * Function:    
 *      bcmi_tscf_gen3_xgxs_duplex_set
 * Purpose:     
 *      Set the current duplex mode (forced).
 * Parameters:
 *      pc - PHY control structure
 *      duplex - non-zero indicates full duplex, zero indicates half
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcmi_tscf_gen3_xgxs_duplex_set(phy_ctrl_t *pc, int duplex)
{
    PHY_CTRL_CHECK(pc);

    return CDK_E_NONE;
}

/*
 * Function:    
 *      bcmi_tscf_gen3_xgxs_duplex_get
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
bcmi_tscf_gen3_xgxs_duplex_get(phy_ctrl_t *pc, int *duplex)
{
    PHY_CTRL_CHECK(pc);

    *duplex = 1;

    return CDK_E_NONE;
}

/*
 * Function:    
 *      bcmi_tscf_gen3_xgxs_speed_set
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
bcmi_tscf_gen3_xgxs_speed_set(phy_ctrl_t *pc, uint32_t speed)
{
    int rv = CDK_E_NONE;
    phymod_phy_access_t pm_phy;
    phymod_phy_inf_config_t interface_config;
    phymod_phy_inf_config_t *if_cfg;
    int autoneg;
    phymod_interface_t if_type = 0;
    int fiber_pref = 0;

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

    if_cfg->data_rate = speed;
    /* Default setting the ref_clock to 156. */
    if_cfg->ref_clock = phymodRefClk156Mhz; 
    if (PHY_CTRL_LINE_INTF(pc) == PHY_IF_HIGIG) {
        PHYMOD_INTF_MODES_HIGIG_SET(if_cfg);
    }
    if (PHY_CTRL_LINE_INTF(pc) == PHY_IF_SFI || 
        PHY_CTRL_LINE_INTF(pc) == PHY_IF_SR) {
        PHYMOD_INTF_MODES_FIBER_SET(if_cfg);
        fiber_pref = 1;
    } else {
        if (PHY_CTRL_LINE_INTF(pc) == PHY_IF_CR) {
            PHYMOD_INTF_MODES_COPPER_SET(if_cfg);
        } else {
            PHYMOD_INTF_MODES_BACKPLANE_SET(if_cfg);
        }
    }

    switch (speed) {
        case 1000:
            if (fiber_pref) {
                if_type = phymodInterface1000X;
            } else {
                if_type = phymodInterfaceSGMII;
            }
            break;
        case 10000:
            if ((PHY_CTRL_LINE_INTF(pc) == PHY_IF_SFI) || 
                (PHY_CTRL_LINE_INTF(pc) == PHY_IF_SR) || 
                (PHY_CTRL_LINE_INTF(pc) == PHY_IF_CR)) {
                if_type = phymodInterfaceSFI;
            } else {
                if_type = phymodInterfaceXFI;
            }
            break;
        case 11000:
        case 10600:
            if_type = phymodInterfaceXFI;
            break;
        case 20000:
        case 21000:
            if (IS_1LANE_PORT(pc)) {
                if_type = phymodInterfaceXFI;
            } else if (IS_2LANE_PORT(pc)) {
                if_type = phymodInterfaceKR2;
            } else {
                return CDK_E_PARAM;
            }
            break ;
        case 25000:
        case 26500:
        case 27000:
            if_type = phymodInterfaceXFI;
            break;
        case 40000:
        case 42000:
        case 50000:
        case 53000:
            if (IS_4LANE_PORT(pc)) {
                if_type = phymodInterfaceKR4;
            } else {
                if_type = phymodInterfaceKR2;
            }
            break;
        case 100000:
        case 106000:
            if_type = phymodInterfaceKR4;
            break;
        default:
            return CDK_E_PARAM;
    }

    if_cfg->interface_type = if_type;
            
    _tscf_gen3_phymod_phy_t_init(pc, &pm_phy);
    if (CDK_SUCCESS(rv)) {
        rv = tscf_gen3_phy_interface_config_set(&pm_phy, 0, if_cfg);
    }

    return rv;
}

/*
 * Function:    
 *      bcmi_tscf_gen3_xgxs_speed_get
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
bcmi_tscf_gen3_xgxs_speed_get(phy_ctrl_t *pc, uint32_t *speed)
{
    int rv = CDK_E_NONE;
    phymod_phy_access_t pm_phy;
    phymod_phy_inf_config_t interface_config;
    phymod_ref_clk_t ref_clock;

    PHY_CTRL_CHECK(pc);

    _tscf_gen3_phymod_phy_t_init(pc, &pm_phy);
    
    /* Initialize the data structure */
    interface_config.data_rate = 0;

    /* Note that the flags have an option to indicate whether it's ok to reprogram the PLL */
    rv = _tscf_gen3_ref_clk_convert(PORT_REFCLK_INT, &ref_clock);
    if (CDK_SUCCESS(rv)) {
        rv = tscf_gen3_phy_interface_config_get(&pm_phy,
                                           0, ref_clock, &interface_config);
    }

    *speed = interface_config.data_rate;
    
    return rv;
}

/*
 * Function:    
 *      bcmi_tscf_gen3_xgxs_autoneg_set
 * Purpose:     
 *      Enable or disabled auto-negotiation on the specified port.
 * Parameters:
 *      pc - PHY control structure
 *      autoneg - non-zero enables autoneg, zero disables autoneg
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcmi_tscf_gen3_xgxs_autoneg_set(phy_ctrl_t *pc, int autoneg)
{
    int rv = CDK_E_NONE;
    phymod_autoneg_control_t an;
    phymod_phy_access_t pm_phy;
    uint32_t cl73an;
    
    PHY_CTRL_CHECK(pc);

    an.flags = 0;
    an.enable = autoneg;
    an.num_lane_adv = 1;
    if (IS_2LANE_PORT(pc)) {
        an.num_lane_adv = 2;
    } else if (IS_4LANE_PORT(pc)) {
        an.num_lane_adv = 4;
    }
    an.an_mode = phymod_AN_MODE_NONE;

    cl73an = PHYMOD_AN_CAP_CL73;
    if (PHY_CTRL_LINE_INTF(pc) == PHY_IF_HIGIG) {
        cl73an = 0;
    }
    
    if(cl73an && PHY_CTRL_LINE_INTF(pc) != PHY_IF_HIGIG) {
        if (IS_2LANE_PORT(pc)) {
            an.an_mode = phymod_AN_MODE_CL73BAM; /* default mode */
        } else {
            switch (cl73an) {
            case TEFMOD_CL73_W_BAM:
                an.an_mode = phymod_AN_MODE_CL73BAM;
                break;
            case TEFMOD_CL73_WO_BAM:
                an.an_mode = phymod_AN_MODE_CL73;
                break;
            case TEFMOD_CL73_HPAM_VS_SW:
                an.an_mode = phymod_AN_MODE_HPAM;
                break;
            case TEFMOD_CL73_HPAM:
                an.an_mode = phymod_AN_MODE_HPAM;
                break;
            case TEFMOD_CL73_CL37:
                an.an_mode = phymod_AN_MODE_CL73;
                break;
            default:
                break;
            }
        }
    } else {
        an.an_mode = phymod_AN_MODE_CL73BAM; /* default mode */
    }

    if (CDK_SUCCESS(rv)) {
        _tscf_gen3_phymod_phy_t_init(pc, &pm_phy);
        rv = tscf_gen3_phy_autoneg_set(&pm_phy, &an);
    }
    
    return rv;
}

/*
 * Function:    
 *      bcmi_tscf_gen3_xgxs_autoneg_get
 * Purpose:     
 *      Get the current auto-negotiation status (enabled/busy)
 * Parameters:
 *      pc - PHY control structure
 *      autoneg - (OUT) non-zero indicates autoneg enabled
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcmi_tscf_gen3_xgxs_autoneg_get(phy_ctrl_t *pc, int *autoneg)
{
    int rv = CDK_E_NONE;
    phymod_autoneg_control_t an;
    phymod_phy_access_t pm_phy;
    int an_complete;

    PHY_CTRL_CHECK(pc);

    CDK_MEMSET(&an, 0x0, sizeof(an));

    _tscf_gen3_phymod_phy_t_init(pc, &pm_phy);
    rv = tscf_gen3_phy_autoneg_get(&pm_phy, &an, (uint32_t *)&an_complete);

    *autoneg = 0;
    if (an.enable) {
        *autoneg = 1;
    }
    
    return rv;
}

/*
 * Function:    
 *      bcmi_tscf_gen3_xgxs_loopback_set
 * Purpose:     
 *      Set PHY loopback mode.
 * Parameters:
 *      pc - PHY control structure
 *      enable - non-zero enables PHY loopback
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcmi_tscf_gen3_xgxs_loopback_set(phy_ctrl_t *pc, int enable)
{
    int rv = CDK_E_NONE;
    phymod_phy_access_t pm_phy;

    PHY_CTRL_CHECK(pc);

    _tscf_gen3_phymod_phy_t_init(pc, &pm_phy);
        
    if (CDK_SUCCESS(rv)) {
        rv = tscf_gen3_phy_loopback_set(&pm_phy, phymodLoopbackGlobal, enable); 
    }

    return rv;
}

/*
 * Function:    
 *      bcmi_tscf_gen3_xgxs_loopback_get
 * Purpose:     
 *      Get the current PHY loopback mode.
 * Parameters:
 *      pc - PHY control structure
 *      enable - (OUT) non-zero indicates PHY loopback enabled
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcmi_tscf_gen3_xgxs_loopback_get(phy_ctrl_t *pc, int *enable)
{
    int rv = CDK_E_NONE;
    phymod_phy_access_t pm_phy;
    uint32_t lb_enable;

    PHY_CTRL_CHECK(pc);

    *enable = 0;

    _tscf_gen3_phymod_phy_t_init(pc, &pm_phy);
    rv = tscf_gen3_phy_loopback_get(&pm_phy, phymodLoopbackGlobal, &lb_enable); 
    if (lb_enable) {
        *enable = 1;
    }
     
    return rv;
}

/*
 * Function:    
 *      bcmi_tscf_gen3_xgxs_ability_get
 * Purpose:     
 *      Get the abilities of the PHY.
 * Parameters:
 *      pc - PHY control structure
 *      abil - (OUT) ability mask indicating supported options/speeds.
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcmi_tscf_gen3_xgxs_ability_get(phy_ctrl_t *pc, uint32_t *abil)
{
    PHY_CTRL_CHECK(pc);

    *abil = PHY_ABIL_10GB | PHY_ABIL_LOOPBACK;
    if (IS_4LANE_PORT(pc)) {
        *abil = (PHY_ABIL_40GB | PHY_ABIL_25GB | PHY_ABIL_21GB |
                 PHY_ABIL_16GB | PHY_ABIL_13GB | PHY_ABIL_10GB |
                 PHY_ABIL_PAUSE | PHY_ABIL_XAUI | PHY_ABIL_XGMII | 
                 PHY_ABIL_AN | PHY_ABIL_LOOPBACK);
    } else if (IS_2LANE_PORT(pc)) {
        *abil = (PHY_ABIL_21GB | PHY_ABIL_13GB | PHY_ABIL_10GB | 
                 PHY_ABIL_XAUI | PHY_ABIL_XGMII | PHY_ABIL_LOOPBACK);
    } else {
        *abil = (PHY_ABIL_10GB | PHY_ABIL_2500MB | PHY_ABIL_1000MB | 
                 PHY_ABIL_100MB | PHY_ABIL_10MB | PHY_ABIL_SERDES | 
                 PHY_ABIL_PAUSE | PHY_ABIL_SGMII | PHY_ABIL_GMII | 
                 PHY_ABIL_AN | PHY_ABIL_LOOPBACK);
    }

    return CDK_E_NONE;
}

/*
 * Function:
 *      bcmi_tscf_gen3_xgxs_config_set
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
bcmi_tscf_gen3_xgxs_config_set(phy_ctrl_t *pc, phy_config_t cfg, uint32_t val, void *cd)
{
    int rv = CDK_E_NONE;
    phymod_phy_access_t pm_phy;

    PHY_CTRL_CHECK(pc);

    switch (cfg) {
    case PhyConfig_Enable:
        return CDK_E_NONE;
    case PhyConfig_RemoteLoopback:
        {
            _tscf_gen3_phymod_phy_t_init(pc, &pm_phy);
                
            if (CDK_SUCCESS(rv)) {
                rv = tscf_gen3_phy_loopback_set(&pm_phy, phymodLoopbackRemotePMD, val); 
            }
            return rv;
        }
#if PHY_CONFIG_INCLUDE_XAUI_TX_LANE_MAP_SET
    case PhyConfig_XauiTxLaneRemap:
        {
            TX_LANE_MAP_SET(pc, val);
            return rv;
        }
#endif
#if PHY_CONFIG_INCLUDE_XAUI_RX_LANE_MAP_SET
    case PhyConfig_XauiRxLaneRemap:
        {
            RX_LANE_MAP_SET(pc, val);
            return rv;
        }
#endif
#if PHY_CONFIG_INCLUDE_XAUI_TX_POLARITY_SET
    case PhyConfig_XauiTxPolInvert:
        {
            TX_POLARITY_SET(pc, val);
            return rv;
        }
#endif
#if PHY_CONFIG_INCLUDE_XAUI_RX_POLARITY_SET
    case PhyConfig_XauiRxPolInvert:
        {
            RX_POLARITY_SET(pc, val);
            return rv;
        }
#endif
#if PHY_CONFIG_INCLUDE_LINK_ABILITIES
    case PhyConfig_AdvLocal: 
        rv = _tscf_gen3_ability_set(pc, val);
        return rv;
#endif
    case PhyConfig_EEE:
        {
            _tscf_gen3_phymod_phy_t_init(pc, &pm_phy);
            if (CDK_SUCCESS(rv)) {
                rv = tscf_gen3_phy_eee_set(&pm_phy, val);
            }
            return rv;
        }
    case PhyConfig_TxPreemp:
    case PhyConfig_TxPost2:
    case PhyConfig_TxIDrv:
    case PhyConfig_TxPreIDrv:
        return CDK_E_NONE;
    default:
        break;
    }

    return CDK_E_UNAVAIL;
}

/*
 * Function:
 *      bcmi_tscf_gen3_xgxs_config_get
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
bcmi_tscf_gen3_xgxs_config_get(phy_ctrl_t *pc, phy_config_t cfg, uint32_t *val, void *cd)
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
            _tscf_gen3_phymod_phy_t_init(pc, &pm_phy);
            rv = tscf_gen3_phy_loopback_get(&pm_phy, phymodLoopbackRemotePMD, val);
            return rv;
        }
#if PHY_CONFIG_INCLUDE_XAUI_TX_LANE_MAP_SET
    case PhyConfig_XauiTxLaneRemap:
        {
            phymod_lane_map_t lane_map;
           
            _tscf_gen3_phymod_core_t_init(pc, &pm_core);
            rv = tscf_gen3_core_lane_map_get(&pm_core, &lane_map);
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
           
            _tscf_gen3_phymod_core_t_init(pc, &pm_core);
            rv = tscf_gen3_core_lane_map_get(&pm_core, &lane_map);
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
        
            _tscf_gen3_phymod_phy_t_init(pc, &pm_phy);
            if (CDK_SUCCESS(rv)) {
                rv = tscf_gen3_phy_polarity_get(&pm_phy, &polarity);
            }
            *val = polarity.tx_polarity;

            return rv;
        }
#endif
#if PHY_CONFIG_INCLUDE_XAUI_RX_POLARITY_SET
    case PhyConfig_XauiRxPolInvert:
        {
            phymod_polarity_t polarity;
        
            _tscf_gen3_phymod_phy_t_init(pc, &pm_phy);
            if (CDK_SUCCESS(rv)) {
                rv = tscf_gen3_phy_polarity_get(&pm_phy, &polarity);
            }
            *val = polarity.rx_polarity;

            return rv;
        }
#endif
#if PHY_CONFIG_INCLUDE_LINK_ABILITIES
    case PhyConfig_AdvLocal: 
        rv = _tscf_gen3_ability_get(pc, val);
        return rv;
#endif
    case PhyConfig_EEE:
        {
            _tscf_gen3_phymod_phy_t_init(pc, &pm_phy);
            if (CDK_SUCCESS(rv)) {
                rv = tscf_gen3_phy_eee_get(&pm_phy, val);
            }
            return rv;
        }
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
 *      bcmi_tscf_gen3_xgxs_status_get
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
bcmi_tscf_gen3_xgxs_status_get(phy_ctrl_t *pc, phy_status_t stat, uint32_t *val)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    phymod_phy_access_t pm_phy;
    phymod_ref_clk_t ref_clock;
    
    PHY_CTRL_CHECK(pc);

    switch (stat) {
    case PhyStatus_LineInterface:
        {
            phymod_phy_inf_config_t interface_config;
            phymod_phy_inf_config_t *if_cfg = &interface_config;
            
            CDK_MEMSET(&interface_config, 0x0, sizeof(*if_cfg));
            
            _tscf_gen3_phymod_phy_t_init(pc, &pm_phy);
            rv = _tscf_gen3_ref_clk_convert(PORT_REFCLK_INT, &ref_clock);
            if (CDK_SUCCESS(rv)) {
                rv = tscf_gen3_phy_interface_config_get(&pm_phy, 0, ref_clock, 
                                                   if_cfg);
            }
            
            switch (if_cfg->interface_type) {
            case phymodInterfaceXFI:
                if (PHYMOD_INTF_MODES_COPPER_GET(if_cfg)) {
                    *val = PHY_IF_CR;
                } else {
                    *val = PHY_IF_XFI;
                }
                break;
            case phymodInterfaceSFI:
                *val = PHY_IF_SFI;
                break;
            case phymodInterfaceXLAUI:
                *val = PHY_IF_XLAUI;
                break;
            case phymodInterfaceKX:
                *val = PHY_IF_KX;
                break;
            case phymodInterfaceRXAUI:
                *val = PHY_IF_RXAUI;
                break;
            case phymodInterfaceXGMII:
                *val = PHY_IF_XGMII;
                break;
            case phymodInterfaceCR:
            case phymodInterfaceCR2:
            case phymodInterfaceCR4:
                *val = PHY_IF_CR;
                break;
            case phymodInterfaceKR2:
            case phymodInterfaceKR4:
            case phymodInterfaceKR:
                *val = PHY_IF_KR;
                break;
            case phymodInterfaceSR:
            case phymodInterfaceSR2:
            case phymodInterfaceSR4:
                *val = PHY_IF_SR;
                break;
            default:
                *val = PHY_IF_XGMII;
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
phy_driver_t bcmi_tscf_gen3_xgxs_drv = {
    "bcmi_tscf_gen3_xgxs", 
    "Internal TSC Falcon 40G SerDes PHY Driver",  
    PHY_DRIVER_F_INTERNAL,
    bcmi_tscf_gen3_xgxs_probe,                /* pd_probe */
    bcmi_tscf_gen3_xgxs_notify,               /* pd_notify */
    bcmi_tscf_gen3_xgxs_reset,                /* pd_reset */
    bcmi_tscf_gen3_xgxs_init,                 /* pd_init */
    bcmi_tscf_gen3_xgxs_link_get,             /* pd_link_get */
    bcmi_tscf_gen3_xgxs_duplex_set,           /* pd_duplex_set */
    bcmi_tscf_gen3_xgxs_duplex_get,           /* pd_duplex_get */
    bcmi_tscf_gen3_xgxs_speed_set,            /* pd_speed_set */
    bcmi_tscf_gen3_xgxs_speed_get,            /* pd_speed_get */
    bcmi_tscf_gen3_xgxs_autoneg_set,          /* pd_autoneg_set */
    bcmi_tscf_gen3_xgxs_autoneg_get,          /* pd_autoneg_get */
    bcmi_tscf_gen3_xgxs_loopback_set,         /* pd_loopback_set */
    bcmi_tscf_gen3_xgxs_loopback_get,         /* pd_loopback_get */
    bcmi_tscf_gen3_xgxs_ability_get,          /* pd_ability_get */
    bcmi_tscf_gen3_xgxs_config_set,           /* pd_config_set */
    bcmi_tscf_gen3_xgxs_config_get,           /* pd_config_get */
    bcmi_tscf_gen3_xgxs_status_get,           /* pd_status_get */
    NULL                                 /* pd_cable_diag */
};
