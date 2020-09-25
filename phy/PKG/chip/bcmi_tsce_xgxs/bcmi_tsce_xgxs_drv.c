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
#include <phy/chip/bcmi_tsce_xgxs_defs.h>
#include <cdk/cdk_debug.h>

#include "tsce/tsce.h"

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
extern cdk_symbols_t bcmi_tsce_xgxs_symbols;
#define SET_SYMBOL_TABLE(_pc) \
    PHY_CTRL_SYMBOLS(_pc) = &bcmi_tsce_xgxs_symbols
#else
#define SET_SYMBOL_TABLE(_pc)
#endif

#define NUM_LANES                       4   /* num of lanes per core */
#define MAX_NUM_LANES                   4   /* max num of lanes per port */
#define LANE_NUM_MASK                   0x3 /* Lane from PHY control instance */

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

/*
 * Function:
 *      _tsce_serdes_lane
 * Purpose:
 *      Retrieve eagle lane number for this PHY instance.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      Lane number or -1 if lane is unknown
 */
static int
_tsce_serdes_lane(phy_ctrl_t *pc)
{
    uint32_t inst = PHY_CTRL_INST(pc);

    if (inst & PHY_INST_VALID) {
        return inst & LANE_NUM_MASK;
    }
    return -1;
}

/*
 * Function:
 *      _tsce_primary_lane
 * Purpose:
 *      Ensure that each tsce is initialized only once.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      TRUE/FALSE
 */
static int
_tsce_primary_lane(phy_ctrl_t *pc)
{
    uint32_t inst = PHY_CTRL_INST(pc);
    
    return (inst & LANE_NUM_MASK) ? FALSE : TRUE;
}

/*
 * Function:
 *      _tsce_phymod_core_t_init
 * Purpose:
 *      Init phymod_core_access structure.
 * Parameters:
 *      pc - PHY control structure
 *      core - PHYMOD core access structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_tsce_phymod_core_t_init(phy_ctrl_t *pc, phymod_core_access_t *core)
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
 *      _tsce_phymod_phy_t_init
 * Purpose:
 *      Init phymod_phy_access structure.
 * Parameters:
 *      pc - PHY control structure
 *      core - PHYMOD phy access structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_tsce_phymod_phy_t_init(phy_ctrl_t *pc, phymod_phy_access_t *phy)
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
    phy->access.lane_mask = lane_map << _tsce_serdes_lane(pc);

    /* Setting the flags for phymod */
    if (PHY_CTRL_FLAGS(pc) & PHY_F_CLAUSE45) {
        PHYMOD_ACC_F_CLAUSE45_CLR(&phy->access);
    }
    
    return CDK_E_NONE;
}

/*
 * Function:
 *      _tsce_firmware_loader
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
_tsce_firmware_loader(const phymod_core_access_t *pm_core,
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
 *      _tsce_serdes_stop
 * Purpose:
 *      Put PHY in or out of reset depending on conditions.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_tsce_serdes_stop(phy_ctrl_t *pc)
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

    _PHY_DBG(pc, ("_tsce_serdes_stop: stop = %d\n", stop));

    if (!stop) { /* Enable */
        tx_control = phymodTxSquelchOff;
        rx_control = phymodRxSquelchOff;
    } else {
        tx_control = phymodTxSquelchOn;
        rx_control = phymodRxSquelchOn;
    }

    _tsce_phymod_phy_t_init(pc, &pm_phy);
    if (CDK_SUCCESS(rv)) {
        rv = tsce_phy_tx_lane_control_set(&pm_phy, tx_control); 
    }
    if (CDK_SUCCESS(rv)) {
        rv = tsce_phy_rx_lane_control_set(&pm_phy, rx_control); 
    }

    return rv;
}

/*
 * Function:
 *      _tsce_ability_set
 * Purpose:
 *      configure phy ability.
 * Parameters:
 *      pc - PHY control structure
 *      ability - ability value
 * Returns:
 *      CDK_E_xxx
 */
static int
_tsce_ability_set(phy_ctrl_t *pc, uint32_t ability)
{
    int rv = CDK_E_NONE;
    phymod_phy_access_t pm_phy;
    phymod_autoneg_ability_t autoneg_ability;
    phy_ctrl_t *lpc;
    uint32_t an_cap, an_bam37_ability, an_bam73_ability;
    int idx;

    an_cap = 0;
    an_bam73_ability = 0;
    an_bam37_ability = 0;

    /* Set an_cap */
    if (IS_4LANE_PORT(pc)) {
        if (ability & PHY_ABIL_100GB) {
            PHYMOD_AN_CAP_100G_CR10_SET(an_cap);
        }
        if (ability & PHY_ABIL_40GB) {
            if (PHY_CTRL_LINE_INTF(pc) == PHY_IF_CR) {
                PHYMOD_AN_CAP_40G_CR4_SET(an_cap);
            } else {
                PHYMOD_AN_CAP_40G_KR4_SET(an_cap);
            }
        }
        if (ability & PHY_ABIL_20GB) {
            if (PHY_CTRL_LINE_INTF(pc) == PHY_IF_CR) {
                PHYMOD_BAM_CL73_CAP_20G_CR2_SET(an_bam73_ability);
            } else {
                PHYMOD_BAM_CL73_CAP_20G_KR2_SET(an_bam73_ability);
            }
        }
        if (ability & PHY_ABIL_10GB) {
            PHYMOD_AN_CAP_10G_KX4_SET(an_cap);
        }
        if (ability & PHY_ABIL_1000MB_FD) {
            PHYMOD_AN_CAP_1G_KX_SET(an_cap);
        }
    } else if (IS_2LANE_PORT(pc)) {
        if (ability & PHY_ABIL_20GB) {
            if (PHY_CTRL_LINE_INTF(pc) == PHY_IF_CR) {
                PHYMOD_BAM_CL73_CAP_20G_CR2_SET(an_bam73_ability);
            } else {
                PHYMOD_BAM_CL73_CAP_20G_KR2_SET(an_bam73_ability);
            }
        }
        if (ability & PHY_ABIL_10GB) {
            PHYMOD_AN_CAP_10G_KR_SET(an_cap);
        }
        if (ability & PHY_ABIL_1000MB_FD) {
            PHYMOD_AN_CAP_1G_KX_SET(an_cap);
        }
    } else {
        if (ability & PHY_ABIL_10GB) {
            PHYMOD_AN_CAP_10G_KR_SET(an_cap);
        }
        if (ability & PHY_ABIL_1000MB_FD) {
            PHYMOD_AN_CAP_1G_KX_SET(an_cap);
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
        if (ability & PHY_ABIL_2500MB) {
            PHYMOD_BAM_CL37_CAP_2P5G_SET(an_bam37_ability);
        }
    }

    autoneg_ability.an_cap = an_cap;
    autoneg_ability.cl73bam_cap = an_bam73_ability;
    autoneg_ability.cl37bam_cap = an_bam37_ability;
    
    if (ability & PHY_ABIL_PAUSE_TX) {
        PHYMOD_AN_CAP_ASYM_PAUSE_SET(&autoneg_ability);
    }
    if (ability & PHY_ABIL_PAUSE_RX) {
        PHYMOD_AN_CAP_ASYM_PAUSE_SET(&autoneg_ability);
        PHYMOD_AN_CAP_SYMM_PAUSE_SET(&autoneg_ability);
    } 

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

    /* Check if we need to set cl37 attribute */
    autoneg_ability.an_cl72 = 1;
    if (PHY_CTRL_LINE_INTF(pc) == PHY_IF_HIGIG) {
        autoneg_ability.an_hg2 = 1;
    }

    lpc = pc;
    for (idx = 0; idx < pc->num_phys; idx++) {
        if (pc->num_phys == 3 && idx > 0) {
            lpc = pc->slave[idx - 1];
            if (lpc == NULL) {
                continue;
            }
        }
        _tsce_phymod_phy_t_init(lpc, &pm_phy);
        rv = tsce_phy_autoneg_ability_set(&pm_phy, &autoneg_ability);
    }
    
    return rv;
}

/*
 * Function:
 *      _tsce_ability_get
 * Purpose:
 *      Get phy ability.
 * Parameters:
 *      pc - PHY control structure
 *      ability - ability value
 * Returns:
 *      CDK_E_xxx
 */
static int
_tsce_ability_get(phy_ctrl_t *pc, uint32_t *ability)
{
    int rv = CDK_E_NONE;
    phymod_phy_access_t pm_phy;
    phymod_autoneg_ability_t autoneg_ability; 
    int reg37_ability, reg73_ability, reg_ability;

    PHY_CTRL_CHECK(pc);

    _tsce_phymod_phy_t_init(pc, &pm_phy);
    rv = tsce_phy_autoneg_ability_get(&pm_phy, &autoneg_ability);
    if (!CDK_SUCCESS(rv)) {
        return CDK_E_FAIL;
    }

    *ability = 0;

    /* retrieve CL73 abilities */
    reg73_ability = autoneg_ability.an_cap;
    *ability |= PHYMOD_AN_CAP_100G_CR10_GET(reg73_ability) ? PHY_ABIL_100GB : 0;
    *ability |= PHYMOD_AN_CAP_40G_CR4_GET(reg73_ability) ? PHY_ABIL_40GB : 0;
    *ability |= PHYMOD_AN_CAP_40G_KR4_GET(reg73_ability) ? PHY_ABIL_40GB : 0;
    *ability |= PHYMOD_AN_CAP_10G_KR_GET(reg73_ability) ? PHY_ABIL_10GB : 0;
    *ability |= PHYMOD_AN_CAP_10G_KX4_GET(reg73_ability) ? PHY_ABIL_10GB : 0;
    *ability |= PHYMOD_AN_CAP_1G_KX_GET(reg73_ability) ? PHY_ABIL_1000MB_FD : 0;

    /* retrieve CL73bam abilities */
    reg73_ability = autoneg_ability.cl73bam_cap;
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
    *ability |= PHYMOD_BAM_CL37_CAP_15P75G_R2_GET(reg37_ability )? PHY_ABIL_16GB : 0;
    *ability |= PHYMOD_BAM_CL37_CAP_13G_X4_GET(reg37_ability) ? PHY_ABIL_13GB : 0;
    *ability |= PHYMOD_BAM_CL37_CAP_12P7_DXGXS_GET(reg37_ability)? PHY_ABIL_13GB : 0;
    *ability |= PHYMOD_BAM_CL37_CAP_10G_X2_CX4_GET(reg37_ability) ? PHY_ABIL_10GB : 0;
    *ability |= PHYMOD_BAM_CL37_CAP_10G_DXGXS_GET(reg37_ability) ? PHY_ABIL_10GB : 0;
    *ability |= PHYMOD_BAM_CL37_CAP_10G_CX4_GET(reg37_ability) ? PHY_ABIL_10GB : 0;
    *ability |= PHYMOD_BAM_CL37_CAP_10G_HIGIG_GET(reg37_ability) ? PHY_ABIL_10GB : 0;
    *ability |= PHYMOD_BAM_CL37_CAP_2P5G_GET(reg37_ability) ? PHY_ABIL_2500MB : 0;
    *ability |= PHY_ABIL_1000MB_FD ;

    /* retrieve "pause" abilities */
    reg_ability = autoneg_ability.capabilities;
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
_tsce_ref_clk_convert(int port_refclk_int, phymod_ref_clk_t *ref_clk)
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

static int
_tsce_phymod_inf_config_t_multi_core_set(phy_ctrl_t *pc,
                                         phymod_phy_inf_config_t *if_cfg)
{
    uint32_t mc_lane;

    mc_lane = MULTI_CORE_LANE_MAP_GET(pc);

    if (mc_lane == phymodTripleCore244) {
        PHYMOD_INTF_MODES_TC_244_SET(if_cfg);
    } else if (mc_lane == phymodTripleCore343) {
        PHYMOD_INTF_MODES_TC_343_SET(if_cfg);
    } else if (mc_lane == phymodTripleCore442) {
        PHYMOD_INTF_MODES_TC_442_SET(if_cfg);
    } else {
        PHYMOD_INTF_MODES_TC_343_SET(if_cfg);
    }

    PHYMOD_INTF_MODES_TRIPLE_CORE_SET(if_cfg);

    return CDK_E_NONE;
}

static int
_tsce_intf_cfg_set(phy_ctrl_t *pc, uint32_t flags,
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
        _tsce_phymod_phy_t_init(lpc, &pm_phy);
        if (CDK_SUCCESS(rv)) {
            rv = tsce_phy_interface_config_set(&pm_phy, flags, if_cfg);
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
 *      bcmi_tsce_xgxs_probe
 * Purpose:     
 *      Probe for PHY.
 * Parameters:
 *      pc - PHY control structure
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcmi_tsce_xgxs_probe(phy_ctrl_t *pc)
{
    int rv = CDK_E_NONE;
    phymod_core_access_t pm_core;
    uint32_t found;
    
    PHY_CTRL_CHECK(pc);

    PHY_CTRL_FLAGS(pc) |= PHY_F_LANE_CTRL;
    _tsce_phymod_core_t_init(pc, &pm_core);
    
    found = 0;
    rv = tsce_core_identify(&pm_core, 0, &found);
    if (CDK_SUCCESS(rv) && found) {
        /* All lanes are accessed from the same PHY address */
        PHY_CTRL_FLAGS(pc) |= PHY_F_ADDR_SHARE;   
        /* Set CL73 as default */
        PHY_CTRL_FLAGS(pc) &= ~PHY_F_CLAUSE37;
        PHY_CTRL_FLAGS(pc) |= PHY_F_CLAUSE73;
    
        SET_SYMBOL_TABLE(pc);
        return CDK_E_NONE;
    }

    PHY_CTRL_FLAGS(pc) &= ~PHY_F_LANE_CTRL;

    return CDK_E_NOT_FOUND;
}


/*
 * Function:
 *      bcmi_tsce_xgxs_notify
 * Purpose:     
 *      Handle PHY notifications.
 * Parameters:
 *      pc - PHY control structure
 *      event - PHY event
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcmi_tsce_xgxs_notify(phy_ctrl_t *pc, phy_event_t event)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    phymod_phy_inf_config_t interface_config;
    phymod_phy_access_t pm_phy;

    PHY_CTRL_CHECK(pc);

    CDK_MEMSET(&interface_config, 0x0, sizeof(interface_config));
    switch (event) {
    case PhyEvent_ChangeToPassthru:
        PHY_CTRL_FLAGS(pc) |= PHY_F_PASSTHRU;
        PHY_CTRL_FLAGS(pc) &= ~PHY_F_FIBER_MODE;
        /* Put the Serdes in passthru mode */
        rv = _tsce_phymod_phy_t_init(pc, &pm_phy);
        interface_config.data_rate = 1000;
        interface_config.ref_clock = phymodRefClk156Mhz;
        interface_config.interface_type = phymodInterfaceSGMII;
        interface_config.interface_modes &= ~PHYMOD_INTF_MODES_FIBER;
        if (CDK_SUCCESS(rv)) {
            rv = tsce_phy_interface_config_set(&pm_phy,
                            0 /* flags */, &interface_config);
        }
        return CDK_E_NONE;
    case PhyEvent_ChangeToFiber:
         PHY_CTRL_FLAGS(pc) &= ~PHY_F_PASSTHRU;
        PHY_CTRL_FLAGS(pc) |= PHY_F_FIBER_MODE;
        /* Put the Serdes in Fiber mode */
        rv = _tsce_phymod_phy_t_init(pc, &pm_phy);
        interface_config.data_rate = 1000;
        interface_config.ref_clock = phymodRefClk156Mhz;
        interface_config.interface_type = phymodInterface1000X;
        interface_config.interface_modes |= PHYMOD_INTF_MODES_FIBER;
        if (CDK_SUCCESS(rv)) {
            rv = tsce_phy_interface_config_set(&pm_phy,
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
    rv = _tsce_serdes_stop(pc);

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      bcmi_tsce_xgxs_reset
 * Purpose:     
 *      Reset PHY.
 * Parameters:
 *      pc - PHY control structure
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcmi_tsce_xgxs_reset(phy_ctrl_t *pc)
{
    PHY_CTRL_CHECK(pc);

    return CDK_E_NONE;
}

/*
 * Function:
 *      bcmi_tsce_xgxs_init
 * Purpose:     
 *      Initialize PHY driver.
 * Parameters:
 *      pc - PHY control structure
 * Returns:     
 *      CDK_E_NONE
 */
static int
bcmi_tsce_xgxs_init(phy_ctrl_t *pc)
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
    int lane_map_rx, lane_map_tx;
    int lane, idx;
    uint32_t ability;

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
        _tsce_phymod_core_t_init(lpc, &pm_core);
        _tsce_phymod_phy_t_init(lpc, &pm_phy);
            
        if (_tsce_primary_lane(lpc)) {
            CDK_MEMSET(cic, 0, sizeof(*cic));
            /* CORE configuration */
            cic->firmware_load_method = phymodFirmwareLoadMethodInternal;
                
            /* Check for external loader */
            if (PHY_CTRL_FW_HELPER(lpc)) {
                cic->firmware_loader = _tsce_firmware_loader;
                cic->firmware_load_method = phymodFirmwareLoadMethodExternal;
            }
  
            cic->lane_map.num_of_lanes = NUM_LANES;
            PHYMOD_CORE_INIT_F_FIRMWARE_LOAD_VERIFY_SET(cic);

            lane_map_rx = RX_LANE_MAP_GET(lpc);
            if (lane_map_rx == 0) {
                lane_map_rx = 0x3210;
            }
            for (lane = 0; lane < NUM_LANES; lane++) {
                cic->lane_map.lane_map_rx[lane] = (lane_map_rx >> (lane * 4)) & 0xf;
            }
                
            lane_map_tx = TX_LANE_MAP_GET(lpc);
            if (lane_map_tx == 0) {
                lane_map_tx = 0x3210;
            }
            for (lane = 0; lane < NUM_LANES; lane++) {
                cic->lane_map.lane_map_tx[lane] = (lane_map_tx >> (lane * 4)) & 0xf;
            }

            if (pc->num_phys == 3) {
                _tsce_phymod_inf_config_t_multi_core_set(pc, if_cfg);
            }
                
            core_status.pmd_active = 0;
            if (CDK_SUCCESS(rv)) {
                rv = tsce_core_init(&pm_core, cic, &core_status);
            }
        }
            
        /* PHY configuration */
        /* Initialize the tx taps and driver current*/
        CDK_MEMSET(&phy_init_config, 0, sizeof(phy_init_config));
        
        for (lane = 0; lane < NUM_LANES; lane++) {
            phymod_tx_t *tx_cfg = &phy_init_config.tx[lane];

            tx_cfg->pre = 0x0;
            tx_cfg->main = 0x59;
            tx_cfg->post = 0x17;
            tx_cfg->post2 = 0x0;
            tx_cfg->post3 = 0x0;
            tx_cfg->amp = 0x8;
        }
        phy_init_config.polarity.rx_polarity = RX_POLARITY_GET(lpc);
        phy_init_config.polarity.tx_polarity = TX_POLARITY_GET(lpc);
        phy_init_config.cl72_en = TRUE;
        phy_init_config.an_en = TRUE;
    
        if (CDK_SUCCESS(rv)) {
            rv = tsce_phy_init(&pm_phy, &phy_init_config);
        }
    }
    
    /*
     * Must write back the default auto-negotiation capabilities to
     * ensure that they are advertised correctly.
     */
    if (CDK_SUCCESS(rv)) {
        rv = _tsce_ability_get(pc, &ability);
    }
    if (CDK_SUCCESS(rv)) {
        rv = _tsce_ability_set(pc, ability);
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
 *      bcmi_tsce_xgxs_link_get
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
bcmi_tsce_xgxs_link_get(phy_ctrl_t *pc, int *link, int *autoneg_done)
{
    int rv = CDK_E_NONE;
    phymod_phy_access_t pm_phy;
    phymod_autoneg_control_t an;
    int an_done;
    
    PHY_CTRL_CHECK(pc);

    CDK_MEMSET(&an, 0x0, sizeof(an));
    _tsce_phymod_phy_t_init(pc, &pm_phy);
    if (CDK_SUCCESS(rv)) {
        rv = tsce_phy_autoneg_get(&pm_phy, &an, (uint32_t *)&an_done);
    }
    *autoneg_done = 0;
    if (an.enable) {
        *autoneg_done = an_done;
    }
    
    if (CDK_SUCCESS(rv)) {
        _tsce_phymod_phy_t_init(pc, &pm_phy);
        rv = tsce_phy_link_status_get(&pm_phy, (uint32_t *)link);
    }

    return rv;
}

/*
 * Function:    
 *      bcmi_tsce_xgxs_duplex_set
 * Purpose:     
 *      Set the current duplex mode (forced).
 * Parameters:
 *      pc - PHY control structure
 *      duplex - non-zero indicates full duplex, zero indicates half
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcmi_tsce_xgxs_duplex_set(phy_ctrl_t *pc, int duplex)
{
    PHY_CTRL_CHECK(pc);

    return CDK_E_NONE;
}

/*
 * Function:    
 *      bcmi_tsce_xgxs_duplex_get
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
bcmi_tsce_xgxs_duplex_get(phy_ctrl_t *pc, int *duplex)
{
    PHY_CTRL_CHECK(pc);

    *duplex = 1;

    return CDK_E_NONE;
}

/*
 * Function:    
 *      bcmi_tsce_xgxs_speed_set
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
bcmi_tsce_xgxs_speed_set(phy_ctrl_t *pc, uint32_t speed)
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

    if_cfg->data_rate = speed;
    /* Next check if we need to run at 6.25VCO */
    if (PHY_CTRL_FLAGS(pc) & PHY_F_6250_VCO) {
        if_cfg->pll_divider_req = 40;
    }
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
            if (IS_1LANE_PORT(pc)) {
                if (PHY_CTRL_FLAGS(pc) & PHY_F_FIBER_MODE) {
                    if_type = phymodInterface1000X;
                } else {
                    if_type = phymodInterfaceSGMII;
                }
            } else {
                return CDK_E_PARAM;
            }
            break;
        case 2500:
            if (IS_1LANE_PORT(pc)) {
                if_type = phymodInterface1000X;
            } else {
                return CDK_E_PARAM;
            }
            break;
        case 5000: 
            if_type = phymodInterfaceXFI;
            break;
        case 10000:
            if (IS_4LANE_PORT(pc)) {
                if_type = phymodInterfaceXGMII;
            } else if (IS_2LANE_PORT(pc)) {
                if_type = phymodInterfaceRXAUI;
            } else {
                if ((PHY_CTRL_LINE_INTF(pc) == PHY_IF_SFI) ||
                    (PHY_CTRL_LINE_INTF(pc) == PHY_IF_SR)) {
                    if_type = phymodInterfaceSFI;
                } else if (PHY_CTRL_LINE_INTF(pc) == PHY_IF_CR) {
                    if_type = phymodInterfaceSFPDAC;
                } else {
                    if_type = phymodInterfaceXFI;
                }
            }
            break;
        case 11000:
            if_type = phymodInterfaceXFI;
            break;
        case 12000:
            if (IS_4LANE_PORT(pc)) {
                if_type = phymodInterfaceXGMII;
            } else if (IS_2LANE_PORT(pc)) {
                if_type = phymodInterfaceRXAUI;
            } else {
                if_type = phymodInterfaceXFI;
            }
            break ;
        case 13000:
            if_type = phymodInterfaceXGMII;
            break;
        case 15000:
            if_type = phymodInterfaceXGMII;
            break;
        case 16000:
            if (IS_4LANE_PORT(pc)) {
                if_type = phymodInterfaceXGMII;
            } else if (IS_2LANE_PORT(pc)) {
                if_type = phymodInterfaceRXAUI;
            } else {
                return CDK_E_PARAM;
            }
            break ;
        case 20000: 
            if (IS_4LANE_PORT(pc)) {
                if_type = phymodInterfaceXGMII;
            } else if (IS_2LANE_PORT(pc)) {
                if (PHY_CTRL_LINE_INTF(pc) == PHY_IF_HIGIG) {
                    if_type = phymodInterfaceRXAUI;
                } else {
                    if (PHY_CTRL_LINE_INTF(pc) == PHY_IF_KR) {
                        if_type = phymodInterfaceKR2;
                    } else if ((PHY_CTRL_LINE_INTF(pc) == PHY_IF_CR)) {
                        if_type = phymodInterfaceCR2;
                    } else {
                        if_type = phymodInterfaceRXAUI;
                    }
                }
            } else {
                return CDK_E_PARAM;
            }
            break ;
        case 21000:
            if (IS_4LANE_PORT(pc)) {
                if_type = phymodInterfaceXGMII;
            } else if (IS_2LANE_PORT(pc)) {
                if_type = phymodInterfaceRXAUI;
            } else {
                return CDK_E_PARAM;
            }
            break ;
        case 25000:
            if_type = phymodInterfaceXGMII;
            break;
        case 40000:
            if(PHY_CTRL_LINE_INTF(pc) == PHY_IF_HIGIG) {
                if (PHY_CTRL_LINE_INTF(pc) == PHY_IF_KR) {
                    if_type = phymodInterfaceKR4;
                } else {
                    if_type = phymodInterfaceXGMII;
                }
            } else if (PHY_CTRL_FLAGS(pc) & PHY_F_FIBER_MODE) {
                if_cfg->interface_modes |= PHYMOD_INTF_MODES_FIBER;
                if_type = phymodInterfaceKR4;
            } else if (PHY_CTRL_LINE_INTF(pc) == PHY_IF_SR) {
                if_type = phymodInterfaceSR4;
            } else {
                if_type = phymodInterfaceXLAUI;
            }
            break;
        case 42000:
            if (PHY_CTRL_LINE_INTF(pc) == PHY_IF_HIGIG) {
                if_type = phymodInterfaceXGMII;
            } else {
                if_type = phymodInterfaceKR4;
            }
            break;
        case 100000:
        case 106000:
        case 120000:
        case 127000:
            if_type = phymodInterfaceCAUI;
            break;
        default:
            return CDK_E_PARAM;
    }

    if_cfg->interface_type = if_type;
    
    if (pc->num_phys == 3) {
        _tsce_phymod_inf_config_t_multi_core_set(pc, if_cfg);
    }

    rv = _tsce_intf_cfg_set(pc, 0, if_cfg);

    if (pc->num_phys == 3) {
        /* Set CORE_MAP_MODE flag */
        if (CDK_SUCCESS(rv)) {
            rv = _tsce_intf_cfg_set(pc, PHYMOD_INTF_F_SET_CORE_MAP_MODE, if_cfg);
        }
        /* Set SPD_NO_TRIGGER flag */
        if (CDK_SUCCESS(rv)) {
            rv = _tsce_intf_cfg_set(pc, PHYMOD_INTF_F_SET_SPD_NO_TRIGGER, if_cfg);
        }
        /* Set SPD_DISABLE flag */
        if (CDK_SUCCESS(rv)) {
            rv = _tsce_intf_cfg_set(pc, PHYMOD_INTF_F_SET_SPD_DISABLE, if_cfg);
        }
        /* Set SPD_TRIGGER flag */
        if (CDK_SUCCESS(rv)) {
            rv = _tsce_intf_cfg_set(pc, PHYMOD_INTF_F_SET_SPD_TRIGGER, if_cfg);
        }
    }

    return rv;
}

/*
 * Function:    
 *      bcmi_tsce_xgxs_speed_get
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
bcmi_tsce_xgxs_speed_get(phy_ctrl_t *pc, uint32_t *speed)
{
    int rv = CDK_E_NONE;
    phymod_phy_access_t pm_phy;
    phymod_phy_inf_config_t interface_config;
    phymod_ref_clk_t ref_clock;

    PHY_CTRL_CHECK(pc);

    _tsce_phymod_phy_t_init(pc, &pm_phy);
    
    /* Initialize the data structure */
    interface_config.data_rate = 0;
    
    /* Note that the flags have an option to indicate whether it's ok to reprogram the PLL */
    rv = _tsce_ref_clk_convert(PORT_REFCLK_INT, &ref_clock);
    if (CDK_SUCCESS(rv)) {
        rv = tsce_phy_interface_config_get(&pm_phy, 0, ref_clock, 
                                           &interface_config);
    }

    *speed = interface_config.data_rate;
    
    return rv;
}

/*
 * Function:    
 *      bcmi_tsce_xgxs_autoneg_set
 * Purpose:     
 *      Enable or disabled auto-negotiation on the specified port.
 * Parameters:
 *      pc - PHY control structure
 *      autoneg - non-zero enables autoneg, zero disables autoneg
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcmi_tsce_xgxs_autoneg_set(phy_ctrl_t *pc, int autoneg)
{
    int rv = CDK_E_NONE;
    phymod_autoneg_control_t an;
    phymod_phy_access_t pm_phy;
    phy_ctrl_t *lpc;
    uint32_t cl73an, cl37an;
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

    /* By default, disable cl37 */
    cl37an = 0;
    cl73an = TSCE_CL73_WO_BAM;    
    if (PHY_CTRL_LINE_INTF(pc) == PHY_IF_HIGIG) {
        cl73an = 0;
    }
    if (PHY_CTRL_FLAGS(pc) & PHY_F_CLAUSE37) {
        cl37an = TSCE_CL37_WO_BAM;
    } else {
        cl37an = 0;
    }
    if (PHY_CTRL_FLAGS(pc) & PHY_F_CLAUSE73) {
        cl73an = TSCE_CL73_WO_BAM;
    } else {
        cl73an = 0;
    }

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
         if(PHY_CTRL_LINE_INTF(pc) == PHY_IF_HIGIG) {
            an.an_mode = phymod_AN_MODE_CL37BAM;
         } else {        
            if(PHY_CTRL_FLAGS(pc) & PHY_F_FIBER_MODE) {
                an.an_mode = phymod_AN_MODE_CL37;
            } else {
                an.an_mode = phymod_AN_MODE_SGMII;
            }
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
                    _tsce_phymod_phy_t_init(lpc, &pm_phy);
                    rv = tsce_phy_autoneg_set(&pm_phy, &an);
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
        _tsce_phymod_phy_t_init(lpc, &pm_phy);
        rv = tsce_phy_autoneg_set(&pm_phy, &an);
    }
    
    return rv;
}

/*
 * Function:    
 *      bcmi_tsce_xgxs_autoneg_get
 * Purpose:     
 *      Get the current auto-negotiation status (enabled/busy)
 * Parameters:
 *      pc - PHY control structure
 *      autoneg - (OUT) non-zero indicates autoneg enabled
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcmi_tsce_xgxs_autoneg_get(phy_ctrl_t *pc, int *autoneg)
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
    _tsce_phymod_phy_t_init(lpc, &pm_phy);
    rv = tsce_phy_autoneg_get(&pm_phy, &an, (uint32_t *)&an_complete);

    *autoneg = 0;
    if (an.enable) {
        *autoneg = 1;
    }
    
    return rv;
}

/*
 * Function:    
 *      bcmi_tsce_xgxs_loopback_set
 * Purpose:     
 *      Set PHY loopback mode.
 * Parameters:
 *      pc - PHY control structure
 *      enable - non-zero enables PHY loopback
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcmi_tsce_xgxs_loopback_set(phy_ctrl_t *pc, int enable)
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
    
        _tsce_phymod_phy_t_init(lpc, &pm_phy);
           
        if (CDK_SUCCESS(rv)) {
            rv = tsce_phy_loopback_set(&pm_phy, phymodLoopbackGlobal, enable); 
        }
    }

    return rv;
}

/*
 * Function:    
 *      bcmi_tsce_xgxs_loopback_get
 * Purpose:     
 *      Get the current PHY loopback mode.
 * Parameters:
 *      pc - PHY control structure
 *      enable - (OUT) non-zero indicates PHY loopback enabled
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcmi_tsce_xgxs_loopback_get(phy_ctrl_t *pc, int *enable)
{
    int rv = CDK_E_NONE;
    phymod_phy_access_t pm_phy;
    uint32_t lb_enable;

    PHY_CTRL_CHECK(pc);

    *enable = 0;

    _tsce_phymod_phy_t_init(pc, &pm_phy);
    rv = tsce_phy_loopback_get(&pm_phy, phymodLoopbackGlobal, &lb_enable); 
    if (lb_enable) {
        *enable = 1;
    }
     
    return rv;
}

/*
 * Function:    
 *      bcmi_tsce_xgxs_ability_get
 * Purpose:     
 *      Get the abilities of the PHY.
 * Parameters:
 *      pc - PHY control structure
 *      abil - (OUT) ability mask indicating supported options/speeds.
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcmi_tsce_xgxs_ability_get(phy_ctrl_t *pc, uint32_t *abil)
{
    PHY_CTRL_CHECK(pc);

    *abil = PHY_ABIL_10GB | PHY_ABIL_LOOPBACK;
    if (IS_4LANE_PORT(pc)) {
        *abil = (PHY_ABIL_100GB | PHY_ABIL_40GB | PHY_ABIL_25GB | 
                 PHY_ABIL_21GB | PHY_ABIL_20GB |
                 PHY_ABIL_16GB | PHY_ABIL_13GB | PHY_ABIL_10GB |
                 PHY_ABIL_PAUSE | PHY_ABIL_XAUI | PHY_ABIL_XGMII | 
                 PHY_ABIL_AN | PHY_ABIL_LOOPBACK);
    } else if (IS_2LANE_PORT(pc)) {
        *abil = (PHY_ABIL_21GB | PHY_ABIL_21GB | PHY_ABIL_13GB | PHY_ABIL_10GB | 
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
 *      bcmi_tsce_xgxs_config_set
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
bcmi_tsce_xgxs_config_set(phy_ctrl_t *pc, phy_config_t cfg, uint32_t val, void *cd)
{
    int rv = CDK_E_NONE;
    phymod_core_access_t pm_core;
    phymod_phy_access_t pm_phy;
    phy_ctrl_t *lpc;
    int idx;

    PHY_CTRL_CHECK(pc);

    lpc = pc;
    
    switch (cfg) {
    case PhyConfig_Enable:
        return CDK_E_NONE;
    case PhyConfig_RemoteLoopback:
        {
            for (idx = 0; idx < pc->num_phys; idx++) {
                if (pc->num_phys == 3 && idx > 0) {
                    lpc = pc->slave[idx - 1];
                    if (lpc == NULL) {
                        continue;
                    }
                }
                
                _tsce_phymod_phy_t_init(lpc, &pm_phy);
                
                if (CDK_SUCCESS(rv)) {
                    rv = tsce_phy_loopback_set(&pm_phy, phymodLoopbackRemotePMD, val); 
                }
            }
            
            return rv;
        }
#if PHY_CONFIG_INCLUDE_XAUI_TX_LANE_MAP_SET
    case PhyConfig_XauiTxLaneRemap:
        {
            phymod_lane_map_t lane_map;
            
            TX_LANE_MAP_SET(pc, val);

            _tsce_phymod_core_t_init(pc, &pm_core);
            rv = tsce_core_lane_map_get(&pm_core, &lane_map);
            if (CDK_SUCCESS(rv)) {
                for (idx = 0; idx < NUM_LANES; idx++) {
                    /*4 bit per lane*/
                    lane_map.lane_map_tx[idx] = (val >> (idx * 4)) & 0xf;
                }
                rv = tsce_core_lane_map_set(&pm_core, &lane_map);
            }
            
            return rv;
        }
#endif
#if PHY_CONFIG_INCLUDE_XAUI_RX_LANE_MAP_SET
    case PhyConfig_XauiRxLaneRemap:
        {
            phymod_lane_map_t lane_map;

            RX_LANE_MAP_SET(pc, val);
           
            _tsce_phymod_core_t_init(pc, &pm_core);
            rv = tsce_core_lane_map_get(&pm_core, &lane_map);
            if (CDK_SUCCESS(rv)) {
                for (idx = 0; idx < NUM_LANES; idx++) {
                    /*4 bit per lane*/
                    lane_map.lane_map_rx[idx] = (val >> (idx * 4)) & 0xf;
                }
                rv = tsce_core_lane_map_set(&pm_core, &lane_map);
            }

            return rv;
        }
#endif
#if PHY_CONFIG_INCLUDE_XAUI_TX_POLARITY_SET
    case PhyConfig_XauiTxPolInvert:
        {
            phymod_polarity_t polarity;

            TX_POLARITY_SET(pc, val);
        
            _tsce_phymod_phy_t_init(pc, &pm_phy);
            if (CDK_SUCCESS(rv)) {
                rv = tsce_phy_polarity_get(&pm_phy, &polarity);
            }
                    
            polarity.tx_polarity = val;
            if (CDK_SUCCESS(rv)) {
                rv = tsce_phy_polarity_set(&pm_phy, &polarity);
            }

            return rv;
        }
#endif
#if PHY_CONFIG_INCLUDE_XAUI_RX_POLARITY_SET
    case PhyConfig_XauiRxPolInvert:
        {
            phymod_polarity_t polarity;

            RX_POLARITY_SET(pc, val);
        
            _tsce_phymod_phy_t_init(pc, &pm_phy);
            if (CDK_SUCCESS(rv)) {
                rv = tsce_phy_polarity_get(&pm_phy, &polarity);
            }
                    
            polarity.rx_polarity = val;
            if (CDK_SUCCESS(rv)) {
                rv = tsce_phy_polarity_set(&pm_phy, &polarity);
            }

            return rv;
        }
#endif
#if PHY_CONFIG_INCLUDE_LINK_ABILITIES
    case PhyConfig_AdvLocal: 
        rv = _tsce_ability_set(pc, val);
        return rv;
#endif
    case PhyConfig_EEE:
        {
            for (idx = 0; idx < pc->num_phys; idx++) {
                if (pc->num_phys == 3 && idx > 0) {
                    lpc = pc->slave[idx - 1];
                    if (lpc == NULL) {
                        continue;
                    }
                }
                _tsce_phymod_phy_t_init(lpc, &pm_phy);
                if (CDK_SUCCESS(rv)) {
                    rv = tsce_phy_eee_set(&pm_phy, val);
                }
            }
            
            return rv;
        }
    case PhyConfig_TxPreemp:
    case PhyConfig_TxPost2:
    case PhyConfig_TxIDrv:
    case PhyConfig_TxPreIDrv:
        return CDK_E_NONE;
    case PhyConfig_MultiCoreLaneMap:
        MULTI_CORE_LANE_MAP_SET(pc, val);
        return rv;
    case PhyConfig_Clause37En:
        if (val) {
            PHY_CTRL_FLAGS(pc) |= PHY_F_CLAUSE37;
        } else {
            PHY_CTRL_FLAGS(pc) &= ~PHY_F_CLAUSE37;
        }
        return CDK_E_NONE;
    case PhyConfig_Clause73En:
        if (val) {
            PHY_CTRL_FLAGS(pc) |= PHY_F_CLAUSE73;
        } else {
            PHY_CTRL_FLAGS(pc) &= ~PHY_F_CLAUSE73;
        }
        return CDK_E_NONE;
    default:
        break;
    }

    return CDK_E_UNAVAIL;
}

/*
 * Function:
 *      bcmi_tsce_xgxs_config_get
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
bcmi_tsce_xgxs_config_get(phy_ctrl_t *pc, phy_config_t cfg, uint32_t *val, void *cd)
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
            _tsce_phymod_phy_t_init(pc, &pm_phy);
            rv = tsce_phy_loopback_get(&pm_phy, phymodLoopbackRemotePMD, val);
            return rv;
        }
#if PHY_CONFIG_INCLUDE_XAUI_TX_LANE_MAP_SET
    case PhyConfig_XauiTxLaneRemap:
        {
            phymod_lane_map_t lane_map;
           
            _tsce_phymod_core_t_init(pc, &pm_core);
            rv = tsce_core_lane_map_get(&pm_core, &lane_map);
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
           
            _tsce_phymod_core_t_init(pc, &pm_core);
            rv = tsce_core_lane_map_get(&pm_core, &lane_map);
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
        
            _tsce_phymod_phy_t_init(pc, &pm_phy);
            if (CDK_SUCCESS(rv)) {
                rv = tsce_phy_polarity_get(&pm_phy, &polarity);
            }
            *val = polarity.tx_polarity;

            return rv;
        }
#endif
#if PHY_CONFIG_INCLUDE_XAUI_RX_POLARITY_SET
    case PhyConfig_XauiRxPolInvert:
        {
            phymod_polarity_t polarity;
        
            _tsce_phymod_phy_t_init(pc, &pm_phy);
            if (CDK_SUCCESS(rv)) {
                rv = tsce_phy_polarity_get(&pm_phy, &polarity);
            }
            *val = polarity.rx_polarity;

            return rv;
        }
#endif
#if PHY_CONFIG_INCLUDE_LINK_ABILITIES
    case PhyConfig_AdvLocal: 
        rv = _tsce_ability_get(pc, val);
        return rv;
#endif
    case PhyConfig_EEE:
        {
            _tsce_phymod_phy_t_init(pc, &pm_phy);
            if (CDK_SUCCESS(rv)) {
                rv = tsce_phy_eee_get(&pm_phy, val);
            }
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
    case PhyConfig_Clause37En:
        if (PHY_CTRL_FLAGS(pc) & PHY_F_CLAUSE37) {
            *val = 1;
        } else {
            *val = 0;
        }
        return CDK_E_NONE;
    case PhyConfig_Clause73En:
        if (PHY_CTRL_FLAGS(pc) & PHY_F_CLAUSE73) {
            *val = 1;
        } else {
            *val = 0;
        }
        return CDK_E_NONE;
    default:
        break;
    }

    return CDK_E_UNAVAIL;
}

/*
 * Function:
 *      bcmi_tsce_xgxs_status_get
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
bcmi_tsce_xgxs_status_get(phy_ctrl_t *pc, phy_status_t stat, uint32_t *val)
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
            
            CDK_MEMSET(&interface_config, 0x0, sizeof(interface_config));
            
            _tsce_phymod_phy_t_init(pc, &pm_phy);
            rv = _tsce_ref_clk_convert(PORT_REFCLK_INT, &ref_clock);
            if (CDK_SUCCESS(rv)) {
                rv = tsce_phy_interface_config_get(&pm_phy, 0, ref_clock, 
                                                   &interface_config);
            }
            
            switch (interface_config.interface_type) {
            case phymodInterfaceXFI:
                *val = PHY_IF_XFI;
                break;
            case phymodInterfaceSFI:
                *val = PHY_IF_SFI;
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
            case phymodInterfaceRXAUI:
                *val = PHY_IF_RXAUI;
                break;
            case phymodInterfaceXGMII:
                *val = PHY_IF_XGMII;
                break;
            case phymodInterfaceKR4:
                *val = PHY_IF_KR;
                break;
            case phymodInterfaceKR:
            {
                int an = 0;
                bcmi_tsce_xgxs_autoneg_get(pc, &an);
                if (an) {
                    *val = PHY_IF_KR;
                } else {
                    if (PHY_CTRL_FLAGS(pc) & PHY_F_FIBER_MODE) {
                        *val = PHY_IF_SFI;
                    } else {
                        *val = PHY_IF_XFI;
                    }
                }
                break;
            }
            case phymodInterfaceCR4:
            case phymodInterfaceCR10:
                *val = PHY_IF_CR;
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
phy_driver_t bcmi_tsce_xgxs_drv = {
    "bcmi_tsce_xgxs", 
    "Internal TSC Eagle 40G SerDes PHY Driver",  
    PHY_DRIVER_F_INTERNAL,
    bcmi_tsce_xgxs_probe,                /* pd_probe */
    bcmi_tsce_xgxs_notify,               /* pd_notify */
    bcmi_tsce_xgxs_reset,                /* pd_reset */
    bcmi_tsce_xgxs_init,                 /* pd_init */
    bcmi_tsce_xgxs_link_get,             /* pd_link_get */
    bcmi_tsce_xgxs_duplex_set,           /* pd_duplex_set */
    bcmi_tsce_xgxs_duplex_get,           /* pd_duplex_get */
    bcmi_tsce_xgxs_speed_set,            /* pd_speed_set */
    bcmi_tsce_xgxs_speed_get,            /* pd_speed_get */
    bcmi_tsce_xgxs_autoneg_set,          /* pd_autoneg_set */
    bcmi_tsce_xgxs_autoneg_get,          /* pd_autoneg_get */
    bcmi_tsce_xgxs_loopback_set,         /* pd_loopback_set */
    bcmi_tsce_xgxs_loopback_get,         /* pd_loopback_get */
    bcmi_tsce_xgxs_ability_get,          /* pd_ability_get */
    bcmi_tsce_xgxs_config_set,           /* pd_config_set */
    bcmi_tsce_xgxs_config_get,           /* pd_config_get */
    bcmi_tsce_xgxs_status_get,           /* pd_status_get */
    NULL                                 /* pd_cable_diag */
};
