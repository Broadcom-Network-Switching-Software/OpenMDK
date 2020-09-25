/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * PHY driver for BCM82328.
 */

#include <phy/phy.h>
#include <phy/ge_phy.h>

extern unsigned char bcm82328_ucode[];
extern unsigned int bcm82328_ucode_len;
extern unsigned char bcm82328_b0_ucode[];
extern unsigned int bcm82328_b0_ucode_len;

#define PHY_REGS_SIDE_SYSTEM                    0
#define PHY_REGS_SIDE_LINE                      1

#define PHY_DATAPATH_20_BIT                     0
#define PHY_DATAPATH_4_BIT_1                    1
#define PHY_DATAPATH_4_BIT_2                    2

#define NUM_PHY_LANES                           4
#define PHY_ALL_LANES                           4

#define PHY_POLL_MAX                            100
#define PHY_RESET_POLL_MAX                      10

#define BCM82328_PMA_PMD_ID0                    0xae02
#define BCM82328_PMA_PMD_ID1                    0x5210

#define PHY_ID1_REV_MASK                        0x000f

#define PHY82328_B0_CHIP_REV                    0x00b0
#define PHY82328_A0_CHIP_REV                    0x00a0

/* Firmware checksum for MDIO download */
#define BCM82328_MDIO_FW_CHKSUM                 0x600d

#define C45_DEVAD(_a)                           LSHIFT32((_a),16)
#define DEVAD_PMA_PMD                           C45_DEVAD(MII_C45_DEV_PMA_PMD)
#define DEVAD_AN                                C45_DEVAD(MII_C45_DEV_AN)

/* PMA/PMD registers */
#define PMA_PMD_CTRL_REG                        (DEVAD_PMA_PMD + MII_CTRL_REG)
#define PMA_PMD_STAT_REG                        (DEVAD_PMA_PMD + MII_STAT_REG)
#define PMA_PMD_ID0_REG                         (DEVAD_PMA_PMD + MII_PHY_ID0_REG)
#define PMA_PMD_ID1_REG                         (DEVAD_PMA_PMD + MII_PHY_ID1_REG)
#define PMA_PMD_R_PMD_STATUS_REG                (DEVAD_PMA_PMD + 0x0097)
#define PMA_PMD_TX_ANA_CTRL0_REG                (DEVAD_PMA_PMD + 0xc081)
#define PMA_PMD_TX_ANA_CTRL6_REG                (DEVAD_PMA_PMD + 0xc08a)
#define PMA_PMD_TX_ANA_CTRL7_REG                (DEVAD_PMA_PMD + 0xc08b)
#define PMA_PMD_ANA_RX_STATUS_REG               (DEVAD_PMA_PMD + 0xc0b0)
#define PMA_PMD_ANA_RX_CTRL_REG                 (DEVAD_PMA_PMD + 0xc0b1)
#define PMA_PMD_UC_VERSION_REG                  (DEVAD_PMA_PMD + 0xc161)
#define PMA_PMD_CHIP_REVISION_REG               (DEVAD_PMA_PMD + 0xc801)
#define PMA_PMD_SP_LINK_DETECT_REG              (DEVAD_PMA_PMD + 0xc81f)
#define PMA_PMD_BCST_REG                        (DEVAD_PMA_PMD + 0xc8fe)
#define PMA_PMD_GEN_0_REG                       (DEVAD_PMA_PMD + 0xc840)
#define PMA_PMD_GEN_1_REG                       (DEVAD_PMA_PMD + 0xc841)
#define PMA_PMD_GEN_2_REG                       (DEVAD_PMA_PMD + 0xc842)
#define PMA_PMD_GEN_3_REG                       (DEVAD_PMA_PMD + 0xc843)
#define PMA_PMD_SPA_CTRL_REG                    (DEVAD_PMA_PMD + 0xc848)
#define PMA_PMD_REGUALTOR_CTRL_REG              (DEVAD_PMA_PMD + 0xc850)
#define PMA_PMD_OPTICAL_CONF_REG                (DEVAD_PMA_PMD + 0xc8e4)
#define PMA_PMD_GEN_CTRL_REG                    (DEVAD_PMA_PMD + 0xca10)
#define PMA_PMD_MSG_IN_REG                      (DEVAD_PMA_PMD + 0xca12)
#define PMA_PMD_MSG_OUT_REG                     (DEVAD_PMA_PMD + 0xca13)
#define PMA_PMD_GP_0_REG                        (DEVAD_PMA_PMD + 0xca18)
#define PMA_PMD_GP_1_REG                        (DEVAD_PMA_PMD + 0xca19)
#define PMA_PMD_GP_2_REG                        (DEVAD_PMA_PMD + 0xca1a)
#define PMA_PMD_GP_3_REG                        (DEVAD_PMA_PMD + 0xca1b)
#define PMA_PMD_GP_4_REG                        (DEVAD_PMA_PMD + 0xca1c)
#define PMA_PMD_MISC_CTRL                       (DEVAD_PMA_PMD + 0xca85)
#define PMA_PMD_SINGLE_PMD_CTRL_REG             (DEVAD_PMA_PMD + 0xca86)
#define PMA_PMD_TX_PI_CTRL0_REG                 (DEVAD_PMA_PMD + 0xd070)
#define PMA_PMD_TX_PI_CTRL5_REG                 (DEVAD_PMA_PMD + 0xd075)
#define PMA_PMD_TX_CTRL0_REG                    (DEVAD_PMA_PMD + 0xd0a0)
#define PMA_PMD_TLB_RX_MISC_CTRL_REG            (DEVAD_PMA_PMD + 0xd0d3)
#define PMA_PMD_REMOTE_LB_CTRL_REG              (DEVAD_PMA_PMD + 0xd0e2)
#define PMA_PMD_TLB_TX_MISC_CTRL_REG            (DEVAD_PMA_PMD + 0xd0e3)
#define PMA_PMD_SYSTEM_SIDE_REG                 (DEVAD_PMA_PMD + 0xffff)

/* AN registers */                              
#define AN_CTRL_REG                             (DEVAD_AN + MII_CTRL_REG)
#define AN_STAT_REG                             (DEVAD_AN + MII_STAT_REG)
#define AN_LP_BASE_PAGE_ABILITY_1_REG           (DEVAD_AN + 0x0013)

/* PMA/PMD standard register definitions */
/* Control register */
#define PMA_PMD_CTRL_RESET                      (1L << 15)
#define PMA_PMD_CTRL_SPEED_10G                  (1L << 13)
#define PMA_PMD_CTRL_PMA_LOOPBACK               (1L << 0)

/* PMA/PMD user-defined register definitions */
/* Tx ANA control register */
#define PMA_PMD_TX_ANA_CTRL1_TXPOL_FLIP             (1L << 5)
#define PMA_PMD_TX_ANA_CTRL4_TXPOL_FLIP             (1L << 12)
#define PMA_PMD_TX_ANA_CTRL4_DISABLE_TX             (1L << 13)
#define PMA_PMD_TX_ANA_CTRL6_DISABLE_TX             (1L << 0)
#define PMA_PMD_TX_ANA_CTRL6_REMOTE_LB_ENABLE       (1L << 2)
#define PMA_PMD_TX_ANA_CTRL6_LOWLATENCY_PATH_SEL    (1L << 8)
#define PMA_PMD_TX_ANA_CTRL7_DISABLE_FORCE_EN_SM    (1L << 3)
#define PMA_PMD_TX_ANA_CTRL7_DISABLE_FORCE_VAL_SM   (1L << 2)

/* Select system-side register */
#define PMA_PMD_SYSTEM_SIDE_SELECT              (1L << 0)

/* ANA rx control register */
#define PMA_PMD_ANA_RX_STATUS_SEL               (7L << 0)
#define PMA_PMD_ANA_RX_PMD_LOCK                 (1L << 12)

/* ANA rx status register */
#define PMA_PMD_ANA_RX_STATUS_SIGDET            (1L << 15)
#define PMA_PMD_ANA_RX_STATUS_CDR               (1L << 12)

/* Single PMD control register */
#define PMA_PMD_PMD_CTRL_SINGLE_PMD_MASK        (1L << 7)
#define PMA_PMD_PMD_CTRL_ACCESS_MASK            (0x3 << 4)
#define PMA_PMD_PMD_CTRL_ACCESS_SHIFT           4
#define PMA_PMD_PMD_CTRL_DISABLE_MASK           (0xf << 0)

/* General purpose regigister */
#define PMA_PMD_GEN_SYS_LR_MODE                 (1L << 15)
#define PMA_PMD_GEN_LINE_LR_MODE                (1L << 14)
#define PMA_PMD_GEN_SYS_FORCE_CL72              (1L << 13)
#define PMA_PMD_GEN_LINE_FORCE_CL72             (1L << 12)
#define PMA_PMD_GEN_SYS_CU_TYPE                 (1L << 11)
#define PMA_PMD_GEN_SYS_TYPE                    (1L << 10)
#define PMA_PMD_GEN_LINE_CU_TYPE                (1L << 9)
#define PMA_PMD_GEN_LINE_TYPE                   (1L << 8)
#define PMA_PMD_GEN_FINISH_CHANGE               (1L << 7)
#define PMA_PMD_GEN_ENABLE_LOW_LATENCY_DATAPATH (1L << 6) 
#define PMA_PMD_GEN_ULL_DATAPATH_LATENCY        (1L << 5)
#define PMA_PMD_GEN_SPEED_OR_TYPE_CHANGE        (1L << 4)
#define PMA_PMD_GEN_SPEED_100G                  (1L << 3)
#define PMA_PMD_GEN_SPEED_40G                   (1L << 2)
#define PMA_PMD_GEN_SPEED_10G                   (1L << 1)
#define PMA_PMD_GEN_SPEED_1G                    (1L << 0)

/* Speed link detect status register */
#define PMA_PMD_SP_LINK_SYS_AN_COMPLETE         (1L << 13)
#define PMA_PMD_SP_LINK_SYS_PMD_40G             (1L << 12)
#define PMA_PMD_SP_LINK_SYS_PMD_10G             (1L << 11)
#define PMA_PMD_SP_LINK_SYS_PMD_1G              (1L << 10)
#define PMA_PMD_SP_LINK_SYS_PMD_100M            (1L << 9)
#define PMA_PMD_SP_LINK_SYS_PMD_10M             (1L << 8)
#define PMA_PMD_SP_LINK_LN_AN_COMPLETE          (1L << 6)
#define PMA_PMD_SP_LINK_LN_PMD_40G              (1L << 5)
#define PMA_PMD_SP_LINK_LN_PMD_10G              (1L << 4)
#define PMA_PMD_SP_LINK_LN_PMD_1G               (1L << 2)
#define PMA_PMD_SP_LINK_LN_PMD_100M             (1L << 1)
#define PMA_PMD_SP_LINK_LN_PMD_10M              (1L << 0)

/* Single PMD control register */
#define PMA_PMD_SINGLE_PMD_MODE                 (1L << 7)

/* Optical configuration register */
#define PMA_PMD_OPTICAL_CFG_MAN_TXON_EN         (1L << 12)
#define PMA_PMD_OPTICAL_CFG_TXOFFT              (1L << 4)

/* GP_REG_0 register, 0xCA18 */
#define PMA_PMD_GP_0_LINE_3_ENABLE              (0xf << 12)
#define PMA_PMD_GP_0_SYS_3_ENABLE               (0xf << 8)
#define PMA_PMD_GP_0_TX_SQUELCHING_DISABLE      (1L << 7)

/* Rx polarity inversion */
#define PMA_PMD_RX_INV_FORCE                    (1L << 3)
#define PMA_PMD_RX_INV_INVERT                   (1L << 2)

/* SPI port control/status register, 0xC848 */
#define PMA_PMD_SPA_CTRL_SPI_PORT_USED          (1L << 15)
#define PMA_PMD_SPA_CTRL_SPI_BOOT               (1L << 14)
#define PMA_PMD_SPA_CTRL_SPI_DWLD_DONE          (1L << 13)

/* Regulator control register, 0xC850 */
#define PMA_PMD_REGUALTOR_CTRL_PWRDN            (1L << 13)

/* Optical configuration register, 0xC8E4 */
#define PMA_PMD_OPTICAL_CONF_MAN_TXON_ENABLE    (1L << 12)
#define PMA_PMD_OPTICAL_CONF_TXOFFT             (1L << 4)


/* MISC control register, 0xCA85 */
#define PMA_PMD_MISC_CTRL_32K                   (1L << 3)

/* Select broadcast mechanism for MDIO ADDR/Write Operations, 0xC8FE */
#define PMA_PMD_BROADCAT_MODE_ENABLE            (1L << 0)

/* GEN_CTRL register, 0xCA10 */
#define PMA_PMD_GEN_CTRL_URCST                  (1L << 2)

/* TX_PI_CTRL 0 register, 0xD070 */
#define PMA_PMD_TX_PI_CTRL0_EXT_CTRL_ENABLE     (1L << 2)
#define PMA_PMD_TX_PI_CTRL0_LOOP_TIMING_ENT     (1L << 1)
#define PMA_PMD_TX_PI_CTRL0_ENABLE              (1L << 0)
#define PMA_PMD_TX_PI_CTRL5_PASS_THRU_SEL       (1L << 2)

/* TX_CTRL 0 register, 0xD0A0 */
#define PMA_PMD_TX_CTRL_TXPOL_FLIP              (1L << 9)

/* RX_MISC_CTRL register, 0xD0D3 */
#define PMA_PMD_RX_MISC_CTRL_RXPOL_FLIP         (1L << 0)

/* REMOTE_LB_CTRL register, 0xD0E2 */
#define PMA_PMD_REMOTE_LB_CTRL_ENABLE           (1L << 0)

/* TX_MISC_CTRL register, 0xD0E3 */
#define PMA_PMD_TX_MISC_CTRL_TXPOL_FLIP         (1L << 0)

/* Auto-negotiation register definitions */
#define AN_EXT_NXT_PAGE                         (1L << 13)
#define AN_ENABLE                               (1L << 12)
#define AN_RESTART                              (1L << 9)

/* AN LP Base Page Ability 1, 0x0013 */
#define AN_ADVERT_1_ASYM_PAUSE                  (1L << 10)
#define AN_ADVERT_1_PAUSE                       (1L << 11)


#define BCM82328_SINGLE_PORT_MODE(_pc) \
    ((PHY_CTRL_FLAGS(_pc) & PHY_F_SERDES_MODE) == 0)

/* Low level debugging (off by default) */
#ifdef PHY_DEBUG_ENABLE
#define _PHY_DBG(_pc, _stuff) \
    PHY_VERB(_pc, _stuff)
#else
#define _PHY_DBG(_pc, _stuff)
#endif

/***********************************************************************
 *
 * HELPER FUNCTIONS
 */
/*
 * Function:
 *      _bcm82328_side_regs_select
 * Purpose:
 *      Select side of the PHY register.
 * Parameters:
 *      pc - PHY control structure
 *      side - System side or line side
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm82328_side_regs_select(phy_ctrl_t *pc, int side)
{
    int ioerr = 0;
    uint32_t data;

    ioerr += PHY_BUS_READ(pc, PMA_PMD_SYSTEM_SIDE_REG, &data);
    data &= ~PMA_PMD_SYSTEM_SIDE_SELECT;
    if (side == PHY_REGS_SIDE_SYSTEM) {
        data |= PMA_PMD_SYSTEM_SIDE_SELECT;
    }
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_SYSTEM_SIDE_REG, data);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

/*
 * Function:
 *      _bcm82328_side_regs_get
 * Purpose:
 *      Select side of the PHY register.
 * Parameters:
 *      pc - PHY control structure
 *      side - System side or line side
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm82328_side_regs_get(phy_ctrl_t *pc, int *side)
{
    int ioerr = 0;
    uint32_t data;

    *side = PHY_REGS_SIDE_LINE;
    
    ioerr += PHY_BUS_READ(pc, PMA_PMD_SYSTEM_SIDE_REG, &data);
    if (data & PMA_PMD_SYSTEM_SIDE_SELECT) {
        *side = PHY_REGS_SIDE_SYSTEM;
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

/*
 * Function:
 *      _bcm82328_channel_select
 * Purpose:
 *      Select channel to access based on interface side and lane number.
 * Parameters:
 *      pc - PHY control structure
 *      lane - Lane index
 *      side - System side or line side
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm82328_channel_select(phy_ctrl_t *pc, int lane, int side)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t data;

    if (!(BCM82328_SINGLE_PORT_MODE(pc))) {
        return CDK_E_PARAM;
    }

    if ((lane < 0 || lane >= NUM_PHY_LANES) && (lane != PHY_ALL_LANES)) {
        return CDK_E_PARAM;
    }

    rv = _bcm82328_side_regs_select(pc, side);
    if (CDK_FAILURE(rv)) {
        return rv;
    }
    
    ioerr += PHY_BUS_READ(pc, PMA_PMD_SINGLE_PMD_CTRL_REG, &data);
    data &= ~(PMA_PMD_PMD_CTRL_SINGLE_PMD_MASK | PMA_PMD_PMD_CTRL_ACCESS_MASK | 
              PMA_PMD_PMD_CTRL_DISABLE_MASK);
    if (lane == PHY_ALL_LANES) {
        data |= PMA_PMD_PMD_CTRL_SINGLE_PMD_MASK;
        data |= PMA_PMD_PMD_CTRL_DISABLE_MASK;
    } else {
        data |= (lane << PMA_PMD_PMD_CTRL_ACCESS_SHIFT);
        data |= (1 << lane) & PMA_PMD_PMD_CTRL_DISABLE_MASK;
    }
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_SINGLE_PMD_CTRL_REG, data);
    
    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _bcm82328_intf_type_reg_get
 * Purpose:
 *      
 * Parameters:
 *      pc - PHY control structure
 *      type - Interface type
 *      side - System side or line side
 *      data - Register data for setting
 *      mask - Mask for setting
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm82328_intf_type_reg_get(phy_ctrl_t *pc, uint8_t type,
                            int side, uint32_t *data, uint32_t *mask)
{
    int rv = CDK_E_NONE;
    int an;
         
    rv = PHY_AUTONEG_GET(pc, &an);
    if (CDK_FAILURE(rv)) {
        return rv;
    }
            
    *data = 0;
    *mask = 0;
    switch(type) {			
        case PHY_IF_KX:
        case PHY_IF_KR:
            if (side == PHY_REGS_SIDE_LINE) {
                *data = PMA_PMD_GEN_LINE_TYPE;
                *mask = PMA_PMD_GEN_LINE_TYPE;
            } else {
                *data = PMA_PMD_GEN_SYS_TYPE;
                *mask = PMA_PMD_GEN_SYS_TYPE;
            }
            break;
        case PHY_IF_SR:
        case PHY_IF_SFI:
        case PHY_IF_GMII:
        case PHY_IF_SGMII:
            break;
        case PHY_IF_CR:
            if (side == PHY_REGS_SIDE_LINE) {
                *data = PMA_PMD_GEN_LINE_CU_TYPE;
                if (BCM82328_SINGLE_PORT_MODE(pc) && !an) {
                    *data = (PMA_PMD_GEN_LINE_FORCE_CL72 | PMA_PMD_GEN_LINE_CU_TYPE);
                }
            } else {
                *data = (PMA_PMD_GEN_SYS_FORCE_CL72 | PMA_PMD_GEN_SYS_CU_TYPE);
            }
            break;
        case PHY_IF_XFI:
        case PHY_IF_XLAUI:
            if (side == PHY_REGS_SIDE_LINE) {
                *data = (PMA_PMD_GEN_LINE_LR_MODE | PMA_PMD_GEN_LINE_CU_TYPE);
            } else {
                *data = (PMA_PMD_GEN_SYS_LR_MODE | PMA_PMD_GEN_SYS_CU_TYPE);
            }
            break;
        case PHY_IF_LR:
            if (side == PHY_REGS_SIDE_LINE) {
                *data = PMA_PMD_GEN_LINE_LR_MODE;
            } else {
                *data = PMA_PMD_GEN_SYS_LR_MODE;
            }
            break;
        default:
            return CDK_E_UNAVAIL;
    }
    
    *data |= PMA_PMD_GEN_FINISH_CHANGE;
    *mask |= PMA_PMD_GEN_FINISH_CHANGE;
    if (side == PHY_REGS_SIDE_LINE) {
        *mask |= (PMA_PMD_GEN_LINE_LR_MODE | PMA_PMD_GEN_LINE_FORCE_CL72 |
                  PMA_PMD_GEN_LINE_CU_TYPE | PMA_PMD_GEN_LINE_TYPE);
    } else {
        *mask |= (PMA_PMD_GEN_SYS_LR_MODE | PMA_PMD_GEN_SYS_FORCE_CL72 |
                  PMA_PMD_GEN_SYS_CU_TYPE | PMA_PMD_GEN_SYS_TYPE);
    }

    return CDK_E_NONE;	
}

/*
 * Function:
 *      _bcm82328_intf_speed_reg_get
 * Purpose:
 *      
 * Parameters:
 *      pc - PHY control structure
 *      speed - Speed
 *      data - Register data for setting
 *      mask - Mask for setting
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm82328_intf_speed_reg_get(phy_ctrl_t *pc, uint32_t speed, 
                             uint32_t *data, uint32_t *mask)
{
    *data = 0;
    *mask = 0;
    
    switch (speed) {
        case 100000:
            *data = PMA_PMD_GEN_SPEED_100G;
            break;
        case 42000:
            *data = (PMA_PMD_GEN_SPEED_40G | PMA_PMD_GEN_SPEED_10G |
                     PMA_PMD_GEN_SPEED_1G); 
            break;
        case 40000:
            *data = PMA_PMD_GEN_SPEED_40G;
            break;
		case 11000:
            *data = (PMA_PMD_GEN_SPEED_40G | PMA_PMD_GEN_SPEED_1G);
            break;
        case 10000:
            *data = PMA_PMD_GEN_SPEED_10G;
            break;
        case 1000:
		case 100:
		case 10:
            *data = (PMA_PMD_GEN_SPEED_10G | PMA_PMD_GEN_SPEED_1G);
            break;
        default :
            return CDK_E_UNAVAIL;
    }
    
    *data |= PMA_PMD_GEN_FINISH_CHANGE;
    *mask |= (PMA_PMD_GEN_FINISH_CHANGE | PMA_PMD_GEN_SPEED_1G | 
              PMA_PMD_GEN_SPEED_10G | PMA_PMD_GEN_SPEED_40G | 
              PMA_PMD_GEN_SPEED_100G);

    return CDK_E_NONE;
}

/*
 * Function:
 *      _bcm82328_intf_datapath_reg_get
 * Purpose:
 *      
 * Parameters:
 *      pc - PHY control structure
 *      datapath - Datapath type
 *      data - Register data for setting
 *      mask - Mask for setting
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm82328_intf_datapath_reg_get(phy_ctrl_t *pc, uint32_t datapath, 
                                uint32_t *data, uint32_t *mask)
{
    *data = 0;
    *mask = 0;
 
    switch (datapath) {
        case PHY_DATAPATH_20_BIT:
            *data = 0;
            break;
        case PHY_DATAPATH_4_BIT_1:
            *data = PMA_PMD_GEN_ENABLE_LOW_LATENCY_DATAPATH;
            break;
        case PHY_DATAPATH_4_BIT_2:
            *data =	(PMA_PMD_GEN_ENABLE_LOW_LATENCY_DATAPATH |
                     PMA_PMD_GEN_ULL_DATAPATH_LATENCY);
            break;
        default :
            return CDK_E_UNAVAIL;
    }
    *data |= PMA_PMD_GEN_FINISH_CHANGE;
    *mask = (PMA_PMD_GEN_FINISH_CHANGE | PMA_PMD_GEN_ENABLE_LOW_LATENCY_DATAPATH |
	         PMA_PMD_GEN_ULL_DATAPATH_LATENCY);
    
    return CDK_E_NONE;
}

/*
 * Function:
 *      _bcm82328_interface_update
 * Purpose:
 *      Update the PHY interface mode. Updates are done to GP_REGISTER_1 
 *      which are line side registers only.
 * Parameters:
 *      pc - PHY control structure
 *      data - GP_REGISTER_1 register data for setting
 *      mask - Mask for setting
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm82328_interface_update(phy_ctrl_t *pc, uint32_t data, uint32_t mask)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t gen1, gen3;
    int side, cnt;

    rv = _bcm82328_side_regs_get(pc, &side);
    if (CDK_SUCCESS(rv) && (side == PHY_REGS_SIDE_SYSTEM)) {
        rv = _bcm82328_side_regs_select(pc, PHY_REGS_SIDE_LINE);
    }
    
    /* Make sure ucode has acked */
    ioerr += PHY_BUS_READ(pc, PMA_PMD_GEN_3_REG, &gen3);
    if (gen3 & PMA_PMD_GEN_FINISH_CHANGE) {
        /* Cmd active and ucode acked, so let ucode know drv saw ack */
        ioerr += PHY_BUS_READ(pc, PMA_PMD_GEN_1_REG, &gen1);
        gen1 &= ~PMA_PMD_GEN_FINISH_CHANGE;
        ioerr += PHY_BUS_WRITE(pc, PMA_PMD_GEN_1_REG, gen1);
    
        /* Wait for ucode to CTS */
        for (cnt = 0; cnt < PHY_POLL_MAX; cnt++) {
            ioerr += PHY_BUS_READ(pc, PMA_PMD_GEN_3_REG, &gen3);
            if (!(gen3 & PMA_PMD_GEN_FINISH_CHANGE)) {
                break;
            }
            PHY_SYS_USLEEP(100); 
        }
        if (cnt == PHY_POLL_MAX) {
            PHY_WARN(pc, ("Polling timeout\n"));
            rv = CDK_E_TIMEOUT;
            goto fail;
        }
    }

    /* Send command */
    ioerr += PHY_BUS_READ(pc, PMA_PMD_GEN_1_REG, &gen1);
    gen1 &= ~mask;
    gen1 |= (data | PMA_PMD_GEN_FINISH_CHANGE);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_GEN_1_REG, gen1);
    
    /* Handshake with microcode by waiting for ack before moving on */
    for (cnt = 0; cnt < PHY_POLL_MAX; cnt++) {
        ioerr += PHY_BUS_READ(pc, PMA_PMD_GEN_3_REG, &gen3);
        if (gen3 & PMA_PMD_GEN_FINISH_CHANGE) {
            break;
        }
        PHY_SYS_USLEEP(100); 
    }
    if (cnt == PHY_POLL_MAX) {
        PHY_WARN(pc, ("Polling timeout\n"));
        rv = CDK_E_TIMEOUT;
        goto fail;
    }
    
    /* Cmd active and ucode acked - let ucode know we saw ack */
    ioerr += PHY_BUS_READ(pc, PMA_PMD_GEN_1_REG, &gen1);
    gen1 &= ~PMA_PMD_GEN_FINISH_CHANGE;
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_GEN_1_REG, gen1);

fail:
    if (side == PHY_REGS_SIDE_SYSTEM) {
        _bcm82328_side_regs_select(pc, PHY_REGS_SIDE_SYSTEM);
    }
    
    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _bcm82328_datapath_mode_get
 * Purpose:
 *      Get current datapath mode configuration.
 * Parameters:
 *      pc - PHY control structure
 *      datapath - PHY datapath mode
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm82328_datapath_mode_get(phy_ctrl_t *pc, int *datapath)
{
    int ioerr = 0;
    uint32_t data;

    if (!datapath) {
        return CDK_E_PARAM;
    }
    
    ioerr += PHY_BUS_READ(pc, PMA_PMD_GEN_1_REG, &data);
    if (data & PMA_PMD_GEN_ENABLE_LOW_LATENCY_DATAPATH) {
        if (data & PMA_PMD_GEN_ULL_DATAPATH_LATENCY) {
            *datapath = PHY_DATAPATH_4_BIT_2;
        } else {
            *datapath = PHY_DATAPATH_4_BIT_1;
        }
    } else {
        *datapath = PHY_DATAPATH_20_BIT;
    }
    
    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

/*
 * Function:
 *      _bcm82328_datapath_mode_set
 * Purpose:
 *      Select side of the PHY register.
 * Parameters:
 *      pc - PHY control structure
 *      datapath - PHY datapath mode
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm82328_datapath_mode_set(phy_ctrl_t *pc, int datapath)
{
    int rv = CDK_E_NONE;
    uint32_t reg_data, reg_mask;

    rv = _bcm82328_intf_datapath_reg_get(pc, datapath, &reg_data, &reg_mask);
    if (CDK_FAILURE(rv)) {
        return rv;
    }
    
    return _bcm82328_interface_update(pc, reg_data, reg_mask);
}

/*
 * Function:
 *      _bcm82328_broadcast_setup
 * Purpose:
 *      Put all lanes of a PHY in or out of MDIO broadcast mode.
 * Parameters:
 *      pc - PHY control structure
 *      enable - enable MDIO broadcast mode for this core
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm82328_broadcast_setup(phy_ctrl_t *pc, int enable)
{
    int ioerr = 0;
    uint32_t data;

    ioerr += PHY_BUS_READ(pc, PMA_PMD_BCST_REG, &data);
    data &= ~PMA_PMD_BROADCAT_MODE_ENABLE;
    if (enable) {
        data |= PMA_PMD_BROADCAT_MODE_ENABLE;
    }
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_BCST_REG, data);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

/*
 * Function:
 *      _bcm82328_soft_reset
 * Purpose:
 *      Perform soft reset and wait for completion.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm82328_soft_reset(phy_ctrl_t *pc)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t data;

    ioerr += PHY_BUS_READ(pc, PMA_PMD_CTRL_REG, &data);
    data |= PMA_PMD_CTRL_RESET;
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_CTRL_REG, data);

    PHY_SYS_USLEEP(3000);

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _bcm82328_firmware_download_prep
 * Purpose:
 *      Prepare for firmware download via MDIO.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm82328_firmware_download_prep(phy_ctrl_t *pc)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t data;

    /* Soft reset */
    ioerr += PHY_BUS_READ(pc, PMA_PMD_GEN_CTRL_REG, &data);
    data |= PMA_PMD_GEN_CTRL_URCST;
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_GEN_CTRL_REG, data);

    /* Apply reset to start download code from MDIO */
    rv = _bcm82328_soft_reset(pc);

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _bcm82328_firmware_download
 * Purpose:
 *      Download firmware via MDIO.
 * Parameters:
 *      pc - PHY control structure
 *      fw_data - firmware data
 *      fw_size - size of firmware data (in bytes)
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm82328_mdio_firmware_download(phy_ctrl_t *pc,
                                 uint8_t *fw_data, uint32_t fw_size)
{
    int ioerr = 0;
    uint32_t data;
    uint32_t idx;
    
    /* Check firmware data */
    if (fw_data == NULL || fw_size == 0) {
        PHY_WARN(pc, ("MDIO download: invalid firmware\n"));
        return CDK_E_NONE;
    }
    
    /*Select MDIO FOR ucontroller download */
    ioerr += PHY_BUS_READ(pc, PMA_PMD_SPA_CTRL_REG, &data);
    data &= ~PMA_PMD_SPA_CTRL_SPI_PORT_USED;
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_SPA_CTRL_REG, data);
    
    /* Clear Download Done */
    ioerr += PHY_BUS_READ(pc, PMA_PMD_SPA_CTRL_REG, &data);
    data &= ~PMA_PMD_SPA_CTRL_SPI_DWLD_DONE;
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_SPA_CTRL_REG, data);
    
    ioerr += PHY_BUS_READ(pc, PMA_PMD_SPA_CTRL_REG, &data);
    data |= PMA_PMD_SPA_CTRL_SPI_BOOT;
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_SPA_CTRL_REG, data);
    
    /* Set Dload size to 32K */
    ioerr += PHY_BUS_READ(pc, PMA_PMD_MISC_CTRL, &data);
    data |= PMA_PMD_MISC_CTRL_32K;
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_MISC_CTRL, data);
    
    /* uController reset */
    ioerr += PHY_BUS_READ(pc, PMA_PMD_GEN_CTRL_REG, &data);
    data &= ~PMA_PMD_GEN_CTRL_URCST;
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_GEN_CTRL_REG, data);
    
    /* Wait for 3ms to initialize the RAM*/
    PHY_SYS_USLEEP(3000);
    
    /* Write starting Address, where the code will reside in SRAM */
    data = 0x8000;
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_MSG_IN_REG, data);
    
    /* make sure address word is read by the micro */
    PHY_SYS_USLEEP(10);
    
    /* Write SPI SRAM Count Size */
    data = (fw_size) / 2;
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_MSG_IN_REG, data);
    
    /* make sure read by the micro */
    PHY_SYS_USLEEP(10);
    
    /* Write firmware to SRAM */
    for (idx = 0; idx < (fw_size - 1); idx += 2) {
        /* Write data as 16-bit words */
        data = (fw_data[idx] << 8) | fw_data[idx + 1];
        ioerr += PHY_BUS_WRITE(pc, PMA_PMD_MSG_IN_REG, data);
    
        /* Make sure the word is read by the microcontroller */
        PHY_SYS_USLEEP(10);
    }
    
    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

/*
 * Function:
 *      _bcm82328_mdio_firmware_done
 * Purpose:
 *      MDIO firmware download post-processing.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm82328_mdio_firmware_done(phy_ctrl_t *pc)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t data;
    
    /* make sure last code word is read by the micro */
    PHY_SYS_USLEEP(20);
    
    /* Read Hand-Shake message (Done) from Micro */
    ioerr += PHY_BUS_READ(pc, PMA_PMD_MSG_OUT_REG, &data);
    PHY_VERB(pc, ("MDIO firmware download done message:0x%"PRIx32": port:%d\n",
                  data, pc->port));
    
    PHY_SYS_USLEEP(4000);
    
    /* Need to check if checksum is correct */
    ioerr += PHY_BUS_READ(pc, PMA_PMD_GP_4_REG, &data);
    PHY_VERB(pc, ("BCM82328 firmware load done, port:%d, checksum:0x%"PRIx32"\n",
                  pc->port, data));
    if (data == BCM82328_MDIO_FW_CHKSUM) {
        ioerr += PHY_BUS_READ(pc, PMA_PMD_UC_VERSION_REG, &data);
        PHY_VERB(pc, ("Firmware revID:0x%"PRIx32": port:%d\n",
                      data, pc->port));
    }
    
    /* Disable broadcast mode */
    rv = _bcm82328_broadcast_setup(pc, 0);
    if (CDK_FAILURE(rv)) {
        return rv;
    }
    
    /* Go back to proper PMD mode */
    ioerr += PHY_BUS_READ(pc, PMA_PMD_SINGLE_PMD_CTRL_REG, &data);
    data &= ~PMA_PMD_SINGLE_PMD_MODE;
    if (BCM82328_SINGLE_PORT_MODE(pc)) {
        data |= PMA_PMD_SINGLE_PMD_MODE;
    } 
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_SINGLE_PMD_CTRL_REG, data);
    
    /* Make sure micro completes its initialization */
    PHY_SYS_USLEEP(5000);
    
    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _bcm82328_init_stage_0
 * Purpose:
 *      PHY init.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm82328_init_stage_0(phy_ctrl_t *pc)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t data;
    
    _PHY_DBG(pc, ("init_stage_0\n"));

    if (PHY_CTRL_NEXT(pc)) {
        /* Adopt operating mode from upstream PHY */
        if (PHY_CTRL_FLAGS(PHY_CTRL_NEXT(pc)) & PHY_F_SERDES_MODE) {
            /* Enable quad port mode (single lane mode) */
            PHY_CTRL_FLAGS(pc) |= PHY_F_SERDES_MODE;
        }
    }

    PHY_CTRL_FLAGS(pc) |= (PHY_F_FIBER_MODE | PHY_F_CLAUSE45);
    
    rv = _bcm82328_side_regs_select(pc, PHY_REGS_SIDE_LINE);
    if (CDK_FAILURE(rv)) {
        return rv;
    }
    
    ioerr += PHY_BUS_READ(pc, PMA_PMD_SINGLE_PMD_CTRL_REG, &data);
    data &= ~PMA_PMD_SINGLE_PMD_MODE;
    if (BCM82328_SINGLE_PORT_MODE(pc)) {
        data |= PMA_PMD_SINGLE_PMD_MODE;
    }
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_SINGLE_PMD_CTRL_REG, data);

    /* Setup broadcast mode */
    rv = _bcm82328_broadcast_setup(pc, 1);

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _bcm82328_init_stage_1
 * Purpose:
 *      PHY init.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm82328_init_stage_1(phy_ctrl_t *pc)
{
    int rv = CDK_E_NONE;

    /* Prepare for firmware download */
    /* This step is performed on the broadcast master only */
    if (PHY_CTRL_FLAGS(pc) & PHY_F_STAGED_INIT) {
        if (!(PHY_CTRL_FLAGS(pc) & PHY_F_BCAST_MSTR)) {
            return CDK_E_NONE;
        }
    }

    rv = _bcm82328_firmware_download_prep(pc);  
    
    return rv;
}

/*
 * Function:
 *      _bcm82328_init_stage_2
 * Purpose:
 *      PHY init.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm82328_init_stage_2(phy_ctrl_t *pc)
{
    int rv = CDK_E_NONE;

    /* Broadcast setup for first encounterd devices */
    rv = _bcm82328_broadcast_setup(pc, 1);
    
    return rv;
}

/*
 * Function:
 *      _bcm82328_init_stage_3
 * Purpose:
 *      PHY init.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm82328_init_stage_3(phy_ctrl_t *pc)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t data;

    /* Broadcast firmware to MDIO bus, for first encounterd device */
    /* This step is performed on the broadcast master only */
    if (PHY_CTRL_FLAGS(pc) & PHY_F_STAGED_INIT) {
        if (!(PHY_CTRL_FLAGS(pc) & PHY_F_BCAST_MSTR)) {
            return CDK_E_NONE;
        }
    }

    ioerr += PHY_BUS_READ(pc, PMA_PMD_CHIP_REVISION_REG, &data);
    if ((data & 0xFF) == PHY82328_B0_CHIP_REV) {
        rv = _bcm82328_mdio_firmware_download(pc, bcm82328_b0_ucode, 
                                                  bcm82328_b0_ucode_len);
    } else { 
        rv = _bcm82328_mdio_firmware_download(pc, bcm82328_ucode, 
                                                  bcm82328_ucode_len);
    }
    
    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _bcm82328_init_stage_4
 * Purpose:
 *      PHY init.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm82328_init_stage_4(phy_ctrl_t *pc)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t data;
    int speed;
    
    _PHY_DBG(pc, ("init_stage_4\n"));

    /* MDIO firmware download post-processing */
    rv = _bcm82328_mdio_firmware_done(pc);
    if (CDK_FAILURE(rv)) {
        return rv;
    }

    /*
     * Initialize main firmware control register
     * - First quiesce firmware and clear default settings
     * - Enable all channels as there are race conditions if not all channels are 
     *   enabled when changing to quad-channel mode
     * - If tx_gpio0 is set, firmware drives line side tx enable/disable based on
     *   the system side cdr state
     */
    ioerr += PHY_BUS_READ(pc, PMA_PMD_GP_0_REG, &data);
    data |= (PMA_PMD_GP_0_LINE_3_ENABLE | PMA_PMD_GP_0_SYS_3_ENABLE);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_GP_0_REG, data);

    /* Initialize system and line side interfaces */
    speed = 10000;
    if (BCM82328_SINGLE_PORT_MODE(pc)) {
        speed = 40000;
    }
    rv = PHY_SPEED_SET(pc, speed);
 
    /* Default datapath is 20 bit mode */
    _bcm82328_datapath_mode_set(pc, PHY_DATAPATH_20_BIT);
   
    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _bcm82328_init_stage_5
 * Purpose:
 *      PHY init.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm82328_init_stage_5(phy_ctrl_t *pc)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t data;

    _PHY_DBG(pc, ("init_stage_5\n"));

    if (PHY_CTRL_NEXT(pc)) {
        PHY_NOTIFY(PHY_CTRL_NEXT(pc), PhyEvent_PhyEnable);
    }

    /* Logical lane0 used for auto-negotiation in 40G CR4 */
    if (BCM82328_SINGLE_PORT_MODE(pc)) {
        ioerr += PHY_BUS_READ(pc, PMA_PMD_SINGLE_PMD_CTRL_REG, &data);
        data &= ~PMA_PMD_PMD_CTRL_ACCESS_MASK;
        ioerr += PHY_BUS_WRITE(pc, PMA_PMD_SINGLE_PMD_CTRL_REG, data);
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _bcm82328_init_stage
 * Purpose:
 *      Execute specified init stage.
 * Parameters:
 *      pc - PHY control structure
 *      stage - init stage
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm82328_init_stage(phy_ctrl_t *pc, int stage)
{
    switch (stage) {
    case 0:
        return _bcm82328_init_stage_0(pc);
    case 1:
        return _bcm82328_init_stage_1(pc);
    case 2:
        return _bcm82328_init_stage_2(pc);
    case 3:
        return _bcm82328_init_stage_3(pc);
    case 4:
        return _bcm82328_init_stage_4(pc);
    case 5:
        return _bcm82328_init_stage_5(pc);
    default:
        break;
    }
    return CDK_E_UNAVAIL;
}

/*
 * Function:    
 *      _bcm82328_ability_local_get
 * Purpose:     
 *      Get the local abilities of the PHY.
 * Parameters:
 *      pc - PHY control structure
 *      abil - (OUT) ability mask indicating supported options/speeds.
 * Returns:     
 *      CDK_E_xxx
 */
static int
_bcm82328_ability_local_get(phy_ctrl_t *pc, uint32_t *ability)
{
    if (!ability) {
        return CDK_E_PARAM;
    }

    return PHY_ABILITY_GET(pc, ability);
}

/*
 * Function:    
 *      _bcm82328_ability_remote_get
 * Purpose:     
 *      Get the current remoteisement for auto-negotiation.
 * Parameters:
 *      pc - PHY control structure
 *      abil - (OUT) ability mask indicating supported options/speeds.
 * Returns:     
 *      CDK_E_xxx
 */
static int
_bcm82328_ability_remote_get(phy_ctrl_t *pc, uint32_t *ability)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int an, an_done, link;
    uint32_t data;

    if (!ability) {
        return CDK_E_PARAM;
    }
    *ability = 0;

    rv = PHY_AUTONEG_GET(pc, &an);
    if (CDK_SUCCESS(rv)) {
        rv = PHY_LINK_GET(pc, &an_done, &link);
    }
    if (CDK_FAILURE(rv)) {
        return rv;
    }
    
    if (an && an_done && link) {
        if (PHY_CTRL_FLAGS(pc) & PHY_F_PASSTHRU) {
            if (PHY_CTRL_NEXT(pc)) {
                rv = PHY_CONFIG_GET(PHY_CTRL_NEXT(pc), 
                                    PhyConfig_AdvRemote, ability, NULL);
            }
        } else {
            if (BCM82328_SINGLE_PORT_MODE(pc)) {
                *ability |= PHY_ABIL_40GB;
            } else {
                *ability |= PHY_ABIL_10GB;
            }

            ioerr += PHY_BUS_READ(pc, AN_LP_BASE_PAGE_ABILITY_1_REG, &data);
            switch (data & (AN_ADVERT_1_PAUSE | AN_ADVERT_1_ASYM_PAUSE)) {
                case AN_ADVERT_1_ASYM_PAUSE:
			        *ability = PHY_ABIL_PAUSE_TX;		
                    break;
                case AN_ADVERT_1_PAUSE:
                    *ability = PHY_ABIL_PAUSE_TX | PHY_ABIL_PAUSE_RX;
                    break;		
                case (AN_ADVERT_1_PAUSE | AN_ADVERT_1_ASYM_PAUSE):
                    *ability = PHY_ABIL_PAUSE_RX;
                    break;		
            }            
        }
    } else {
        /* Simply return local abilities */
        rv = _bcm82328_ability_local_get(pc, ability);
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:    
 *      _bcm82328_tx_polarity_invert
 * Purpose:     
 *      Set the TX polarity in line side.
 * Parameters:
 *      pc - PHY control structure
 *      polarity - tx polarity.
 * Returns:     
 *      CDK_E_xxx
 */
static int 
_bcm82328_polarity_flip_tx_set(phy_ctrl_t *pc, uint32_t flip)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int datapath;
    uint32_t data;

    rv = _bcm82328_datapath_mode_get(pc, &datapath);

    if (CDK_SUCCESS(rv)) {
        if (datapath == PHY_DATAPATH_20_BIT) {
            ioerr += PHY_BUS_READ(pc, PMA_PMD_TLB_TX_MISC_CTRL_REG, &data);
            data &= ~PMA_PMD_TX_MISC_CTRL_TXPOL_FLIP;
            if (flip) {
                data |= PMA_PMD_TX_MISC_CTRL_TXPOL_FLIP;
            }
            ioerr += PHY_BUS_WRITE(pc, PMA_PMD_TLB_TX_MISC_CTRL_REG, data);
        } else {
            ioerr += PHY_BUS_READ(pc, PMA_PMD_TX_CTRL0_REG, &data);
            data &= ~PMA_PMD_TX_CTRL_TXPOL_FLIP;
            if (flip) {
                data |= PMA_PMD_TX_CTRL_TXPOL_FLIP;
            }
            ioerr += PHY_BUS_WRITE(pc, PMA_PMD_TX_CTRL0_REG, data);
        }
    }
    return ioerr ? CDK_E_IO : rv;
}

static int
_bcm82328_tx_polarity_invert(phy_ctrl_t *pc, uint32_t polarity)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int flip, lane, side;
   
    rv = _bcm82328_side_regs_get(pc, &side);
    if (CDK_SUCCESS(rv) && (side == PHY_REGS_SIDE_SYSTEM)) {
        rv = _bcm82328_side_regs_select(pc, PHY_REGS_SIDE_LINE);
    }
    
    if (BCM82328_SINGLE_PORT_MODE(pc)) {
        for (lane = 0; lane < NUM_PHY_LANES; lane++) {
            flip = 0;
            if (polarity & (0x1 << lane)) {
                flip = 1;
            }
            
		    /* Select the lane on the line side */
		    if (CDK_SUCCESS(rv)) {
		        rv = _bcm82328_channel_select(pc, lane, PHY_REGS_SIDE_LINE);
		    }
		    if (CDK_SUCCESS(rv)) {
		        rv = _bcm82328_polarity_flip_tx_set(pc, flip);
		    }
        }
        if (CDK_SUCCESS(rv)) {
            rv = _bcm82328_channel_select(pc, PHY_ALL_LANES, PHY_REGS_SIDE_LINE);
        }
    } else {
        flip = 0;
        if (polarity) {
            flip = 1;
        }
        
		if (CDK_SUCCESS(rv)) {
		    rv = _bcm82328_polarity_flip_tx_set(pc, flip);
		}
    }

    if (side == PHY_REGS_SIDE_SYSTEM) {
        _bcm82328_side_regs_select(pc, PHY_REGS_SIDE_SYSTEM);
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:    
 *      _bcm82328_rx_polarity_invert
 * Purpose:     
 *      Set the RX polarity in line side.
 * Parameters:
 *      pc - PHY control structure
 *      polarity - rx polarity.
 * Returns:     
 *      CDK_E_xxx
 */
static int 
_bcm82328_polarity_flip_rx_set(phy_ctrl_t *pc, uint32_t flip)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int datapath;
    uint32_t data;

    rv = _bcm82328_datapath_mode_get(pc, &datapath);

    if (CDK_SUCCESS(rv)) {
        /* Only support 20-bit polarity handled on line side */
        if (datapath == PHY_DATAPATH_20_BIT) {
            ioerr += PHY_BUS_READ(pc, PMA_PMD_TLB_RX_MISC_CTRL_REG, &data);
            data &= ~PMA_PMD_RX_MISC_CTRL_RXPOL_FLIP;
            if (flip) {
                data |= PMA_PMD_RX_MISC_CTRL_RXPOL_FLIP;
            }
            ioerr += PHY_BUS_WRITE(pc, PMA_PMD_TLB_RX_MISC_CTRL_REG, data);
        } 
    }
    return ioerr ? CDK_E_IO : rv;
}

static int
_bcm82328_rx_polarity_invert(phy_ctrl_t *pc, uint32_t polarity)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int flip, lane, side;
   
    rv = _bcm82328_side_regs_get(pc, &side);
    if (CDK_SUCCESS(rv) && (side == PHY_REGS_SIDE_SYSTEM)) {
        rv = _bcm82328_side_regs_select(pc, PHY_REGS_SIDE_LINE);
    }
    
    if (BCM82328_SINGLE_PORT_MODE(pc)) {
        for (lane = 0; lane < NUM_PHY_LANES; lane++) {
            flip = 0;
            if (polarity & (0x1 << lane)) {
                flip = 1;
            }
            
		    /* Select the lane on the line side */
		    if (CDK_SUCCESS(rv)) {
		        rv = _bcm82328_channel_select(pc, lane, PHY_REGS_SIDE_LINE);
		    }
		    if (CDK_SUCCESS(rv)) {
		        rv = _bcm82328_polarity_flip_rx_set(pc, flip);
		    }
        }
        if (CDK_SUCCESS(rv)) {
            rv = _bcm82328_channel_select(pc, PHY_ALL_LANES, PHY_REGS_SIDE_LINE);
        }
    } else {
        flip = 0;
        if (polarity) {
            flip = 1;
        }
        
		if (CDK_SUCCESS(rv)) {
		    rv = _bcm82328_polarity_flip_rx_set(pc, flip);
		}
    }

    if (side == PHY_REGS_SIDE_SYSTEM) {
        _bcm82328_side_regs_select(pc, PHY_REGS_SIDE_SYSTEM);
    }

    return ioerr ? CDK_E_IO : rv;
}


/***********************************************************************
 *
 * PHY DRIVER FUNCTIONS
 */
/*
 * Function:
 *      bcm82328_phy_probe
 * Purpose:     
 *      Probe for 82328 PHY
 * Parameters:
 *      pc - PHY control structure
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm82328_phy_probe(phy_ctrl_t *pc)
{
    int ioerr = 0;
    uint32_t phyid0, phyid1;
    uint32_t data;

    PHY_CTRL_CHECK(pc);

    ioerr += PHY_BUS_READ(pc, PMA_PMD_ID0_REG, &phyid0);
    ioerr += PHY_BUS_READ(pc, PMA_PMD_ID1_REG, &phyid1);
    if (ioerr) {
        return CDK_E_IO;
    }

    if ((phyid0 == BCM82328_PMA_PMD_ID0) &&
        ((phyid1 & ~PHY_ID1_REV_MASK) == BCM82328_PMA_PMD_ID1)) {

        /* Select quad mode for correct probing of subsequent lanes */
        ioerr += PHY_BUS_READ(pc, PMA_PMD_SINGLE_PMD_CTRL_REG, &data);
        data &= ~PMA_PMD_SINGLE_PMD_MODE;
        ioerr += PHY_BUS_WRITE(pc, PMA_PMD_SINGLE_PMD_CTRL_REG, data);

        return ioerr ? CDK_E_IO : CDK_E_NONE;
    }
    return CDK_E_NOT_FOUND;
}

/*
 * Function:
 *      bcm82328_phy_notify
 * Purpose:     
 *      Handle PHY notifications
 * Parameters:
 *      pc - PHY control structure
 *      event - PHY event
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm82328_phy_notify(phy_ctrl_t *pc, phy_event_t event)
{
    int rv = CDK_E_NONE;

    PHY_CTRL_CHECK(pc);

    /* Call up the PHY chain */
    if (PHY_CTRL_NEXT(pc)) {
        rv = PHY_NOTIFY(PHY_CTRL_NEXT(pc), event);
    }

    return rv;
}

/*
 * Function:
 *      bcm82328_phy_reset
 * Purpose:     
 *      Reset 82328 PHY
 * Parameters:
 *      pc - PHY control structure
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm82328_phy_reset(phy_ctrl_t *pc)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;

    PHY_CTRL_CHECK(pc);

    rv = _bcm82328_soft_reset(pc);

    /* Call up the PHY chain */
    if (CDK_SUCCESS(rv)) {
        if (PHY_CTRL_NEXT(pc)) {
            rv = PHY_RESET(PHY_CTRL_NEXT(pc));
        }
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      bcm82328_phy_init
 * Purpose:     
 *      Initialize 82328 PHY driver
 * Parameters:
 *      pc - PHY control structure
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm82328_phy_init(phy_ctrl_t *pc)
{
    int rv = CDK_E_NONE;
    int stage;

    PHY_CTRL_CHECK(pc);

    if (PHY_CTRL_FLAGS(pc) & PHY_F_STAGED_INIT) {
        PHY_CTRL_FLAGS(pc) &= ~PHY_F_STAGED_INIT;
    }

    for (stage = 0; CDK_SUCCESS(rv); stage++) {
        rv = _bcm82328_init_stage(pc, stage);
    }

    if (rv == CDK_E_UNAVAIL) {
        /* Successfully completed all stages */
        rv = CDK_E_NONE;
    }

    /* Call up the PHY chain */
    if (CDK_SUCCESS(rv)) {
        if (PHY_CTRL_NEXT(pc)) {
            rv = PHY_INIT(PHY_CTRL_NEXT(pc));
        }
    }

    return rv;
}

/*
 * Function:    
 *      bcm82328_phy_link_get
 * Purpose:     
 *      Determine the current link up/down status
 * Parameters:
 *      pc - PHY control structure
 *      link - (OUT) non-zero indicates link established.
 * Returns:
 *      CDK_E_xxx
 * Notes:
 *      MII_STATUS bit 2 reflects link state.
 */
static int
bcm82328_phy_link_get(phy_ctrl_t *pc, int *link, int *autoneg_done)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t data;
    int autoneg, max_lane, lane;

    PHY_CTRL_CHECK(pc);
    
    if (!link || !autoneg_done) {
        return CDK_E_PARAM;
    }
        
    *link = 0;
    *autoneg_done = 0;
    
    /* PCS from the internal PHY is used to determine link. */
    if (PHY_CTRL_NEXT(pc)) {
        rv = PHY_LINK_GET(PHY_CTRL_NEXT(pc), link, autoneg_done);
    }

    max_lane = 1;
    if (BCM82328_SINGLE_PORT_MODE(pc)) {
        max_lane = 4;
    }
    
    for (lane = 0; lane < max_lane; lane++) {
        if (BCM82328_SINGLE_PORT_MODE(pc)) {
            if (CDK_SUCCESS(rv)) {
                rv = _bcm82328_channel_select(pc, lane, PHY_REGS_SIDE_LINE);
            }
        }
        
        if (CDK_SUCCESS(rv)) {
            rv = PHY_AUTONEG_GET(pc, &autoneg);
        }
        
        if (CDK_SUCCESS(rv)) {
            if (autoneg) {
                ioerr += PHY_BUS_READ(pc, AN_STAT_REG, &data);
                if (data & MII_STAT_LA) {
                    *link &= 1;
                } else {
                   *link = 0; 
                }
                
                if (data & MII_STAT_AN_DONE) {
                    *autoneg_done &= 1;
                } else {
                    *autoneg_done = 0;
                }
            } else {
                ioerr += PHY_BUS_READ(pc, PMA_PMD_ANA_RX_CTRL_REG, &data);
                data &= ~PMA_PMD_ANA_RX_STATUS_SEL;
                ioerr += PHY_BUS_WRITE(pc, PMA_PMD_ANA_RX_CTRL_REG, data);
                
                ioerr += PHY_BUS_READ(pc, PMA_PMD_ANA_RX_STATUS_REG, &data);
                if (data & PMA_PMD_ANA_RX_PMD_LOCK) {
                    *link &= 1;
                } else {
                   *link = 0; 
                }
            }
        }
    }

    if (BCM82328_SINGLE_PORT_MODE(pc)) {
        if (CDK_SUCCESS(rv)) {
            rv = _bcm82328_channel_select(pc, PHY_ALL_LANES, PHY_REGS_SIDE_LINE);
        }
    }
    
    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:    
 *      bcm82328_phy_duplex_set
 * Purpose:     
 *      Set the current duplex mode (forced).
 * Parameters:
 *      pc - PHY control structure
 *      duplex - non-zero indicates full duplex, zero indicates half
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm82328_phy_duplex_set(phy_ctrl_t *pc, int duplex)
{
    return (duplex != 0) ? CDK_E_NONE : CDK_E_PARAM;
}

/*
 * Function:    
 *      bcm82328_phy_duplex_get
 * Purpose:     
 *      Get the current operating duplex mode.
 * Parameters:
 *      pc - PHY control structure
 *      duplex - (OUT) non-zero indicates full duplex, zero indicates half
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm82328_phy_duplex_get(phy_ctrl_t *pc, int *duplex)
{
    *duplex = 1;

    return CDK_E_NONE;
}

/*
 * Function:    
 *      bcm82328_phy_speed_set
 * Purpose:     
 *      Set the current operating speed (forced).
 * Parameters:
 *      pc - PHY control structure
 *      speed - new link speed
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm82328_phy_speed_set(phy_ctrl_t *pc, uint32_t speed)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t cur_type, cur_speed;
    uint32_t reg_data, reg_mask, data, mask;
    uint8_t sys_intf = 0;
    int an, lb;

    PHY_CTRL_CHECK(pc);

    /* Do not set speed if auto-negotiation is enabled */
    rv = PHY_AUTONEG_GET(pc, &an);
    if (CDK_FAILURE(rv) || an) {
        return rv;
    }

    /* Disable the loopback before setting the speed */
    rv = PHY_LOOPBACK_GET(pc, &lb);
    if (CDK_SUCCESS(rv) && lb) {
        rv = PHY_LOOPBACK_SET(pc, 0);
    }
    if (CDK_FAILURE(rv)) {
        return rv;
    }

    PHY_CTRL_FLAGS(pc) &= ~PHY_F_PASSTHRU;
    if (BCM82328_SINGLE_PORT_MODE(pc)) {
        switch (speed) {
        case 42000:
        case 40000:
            if (!PHY_CTRL_LINE_INTF(pc)) {
                PHY_CTRL_LINE_INTF(pc) = PHY_IF_SR;
            }
            sys_intf = PHY_IF_XLAUI;
            break;
        default:
            return CDK_E_PARAM;
        }
    } else {
        switch (speed) {
        case 10000:
            rv = PHY_SPEED_GET(pc, &cur_speed);
            if (CDK_FAILURE(rv)) {
                return rv;
            }
            if (cur_speed <= 1000 || !PHY_CTRL_LINE_INTF(pc)) {
                PHY_CTRL_LINE_INTF(pc) = PHY_IF_SR;
            } 
            sys_intf = PHY_IF_XFI;
            break;
        case 1000:
            if (!PHY_CTRL_LINE_INTF(pc)) {
                PHY_CTRL_LINE_INTF(pc) = PHY_IF_SGMII;
            } 
            PHY_CTRL_FLAGS(pc) |= PHY_F_PASSTHRU;
            break;
        case 100:
        case 10:
            if (PHY_CTRL_LINE_INTF(pc) != PHY_IF_SGMII) {
                PHY_CTRL_LINE_INTF(pc) = PHY_IF_SGMII;
            }
            PHY_CTRL_FLAGS(pc) |= PHY_F_PASSTHRU;
            break;
        default:
            return CDK_E_PARAM;
        }
    }

    if (!sys_intf) {
        sys_intf = PHY_CTRL_LINE_INTF(pc);
    }
    
    if (PHY_CTRL_NEXT(pc)) {
        PHY_CTRL_LINE_INTF(PHY_CTRL_NEXT(pc)) = sys_intf;
        
        if (PHY_CTRL_FLAGS(pc) & PHY_F_PASSTHRU) {
            rv = PHY_SPEED_SET(PHY_CTRL_NEXT(pc), speed);
            if (CDK_FAILURE(rv)) {
                return rv;
            }
        } else {
            /*
             * When setting system side 40G DAC/XLAUI-DFE, tell internal SerDes to match
             * with XLAUI.
             */
            if (BCM82328_SINGLE_PORT_MODE(pc)) {
                cur_type = PHY_CTRL_LINE_INTF(PHY_CTRL_NEXT(pc));
                if (cur_type == PHY_IF_CR) {
                    PHY_CTRL_LINE_INTF(PHY_CTRL_NEXT(pc)) = PHY_IF_XLAUI;
                    sys_intf = PHY_IF_XLAUI;
                }
            }
             
            /* Check if it must change to forced */
            rv = PHY_AUTONEG_GET(PHY_CTRL_NEXT(pc), &an);
            if (CDK_SUCCESS(rv) && an) {
                rv = PHY_AUTONEG_SET(PHY_CTRL_NEXT(pc), 0);
            }
            if (CDK_FAILURE(rv)) {
                return rv;
            }
             
            rv = PHY_SPEED_GET(PHY_CTRL_NEXT(pc), &cur_speed);
            if (CDK_SUCCESS(rv) && (cur_speed != speed)) {
                rv = PHY_SPEED_SET(PHY_CTRL_NEXT(pc), speed);
            }
            if (CDK_FAILURE(rv)) {
                return rv;
            }
        }
    }

    /* Update interface type/speed */
    reg_data = 0;
    reg_mask = 0;
    rv = _bcm82328_intf_type_reg_get(pc, PHY_CTRL_LINE_INTF(pc), 
                                        PHY_REGS_SIDE_LINE, &data, &mask);
    if (CDK_FAILURE(rv)) {
        return rv;
    }
    reg_data |= data;
    reg_mask |= mask;
    
    rv = _bcm82328_intf_type_reg_get(pc, sys_intf, 
                                PHY_REGS_SIDE_SYSTEM, &data, &mask);
    if (CDK_FAILURE(rv)) {
        return rv;
    }
    reg_data |= data;
    reg_mask |= mask;
    
    rv = _bcm82328_intf_speed_reg_get(pc, speed, &data, &mask);
    if (CDK_FAILURE(rv)) {
        return rv;
    }
    reg_data |= data;
    reg_mask |= mask;
    
    /* Disable the internal serdes before update the phy interface */
    if (PHY_CTRL_NEXT(pc)) {
        PHY_NOTIFY(PHY_CTRL_NEXT(pc), PhyEvent_PhyDisable);
    }
    
    if (CDK_SUCCESS(rv)) {
        rv = _bcm82328_interface_update(pc, reg_data, reg_mask);
    }
 
    /* Enable the internal serdes */
    if (PHY_CTRL_NEXT(pc)) {
        PHY_SYS_USLEEP(100000); 
        PHY_NOTIFY(PHY_CTRL_NEXT(pc), PhyEvent_PhyEnable);
    }

    /* Restore the loopback if necessary */
    if (CDK_SUCCESS(rv) && lb) {
        PHY_SYS_USLEEP(100000); 
        rv = PHY_LOOPBACK_SET(pc, 1);
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:    
 *      bcm82328_phy_speed_get
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
bcm82328_phy_speed_get(phy_ctrl_t *pc, uint32_t *speed)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t data;

    PHY_CTRL_CHECK(pc);

    if (!speed) {
        return CDK_E_PARAM;
    }
    
    ioerr += PHY_BUS_READ(pc, PMA_PMD_GEN_1_REG, &data);
    if (data & PMA_PMD_GEN_SPEED_1G) {
        if ((data & PMA_PMD_GEN_SPEED_10G) && (data & PMA_PMD_GEN_SPEED_40G)) {
            *speed = 42000;
        } else if (data & PMA_PMD_GEN_SPEED_10G) {
            if (PHY_CTRL_NEXT(pc)) {
                rv = PHY_SPEED_GET(PHY_CTRL_NEXT(pc), speed);
            }
        } else if (data & PMA_PMD_GEN_SPEED_40G) {
            *speed = 11000;
        }
    } else if (data & PMA_PMD_GEN_SPEED_10G) {
        *speed = 10000;
    } else if (data & PMA_PMD_GEN_SPEED_40G) {
        *speed = 40000;
    } else if (data & PMA_PMD_GEN_SPEED_100G) {
        *speed = 100000;
    }
    
    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:    
 *      bcm82328_phy_autoneg_set
 * Purpose:     
 *      Enable or disable auto-negotiation on the specified port.
 * Parameters:
 *      pc - PHY control structure
 *      autoneg - non-zero enables autoneg, zero disables autoneg
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm82328_phy_autoneg_set(phy_ctrl_t *pc, int autoneg)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t reg_data, reg_mask;
    uint8_t line_intf;
    int curr_mode;

    PHY_CTRL_CHECK(pc);

    if (PHY_CTRL_NEXT(pc)) {
        if (PHY_CTRL_FLAGS(pc) & PHY_F_PASSTHRU) {
            if (CDK_SUCCESS(rv)) {
                rv = PHY_AUTONEG_SET(PHY_CTRL_NEXT(pc), autoneg);            
            }
        }
    }

    if (BCM82328_SINGLE_PORT_MODE(pc)) {
        if (PHY_CTRL_LINE_INTF(pc) == PHY_IF_CR) {
            if (CDK_SUCCESS(rv) && autoneg) {
                rv = _bcm82328_datapath_mode_set(pc, PHY_DATAPATH_20_BIT);
            }
        }
    } 

    if (PHY_CTRL_FLAGS(pc) & PHY_F_PASSTHRU) {
        line_intf = PHY_IF_SGMII;
    } else {
        if (autoneg) {
            line_intf = PHY_IF_KR;
        } else {
            line_intf = PHY_IF_SR;
        }
    }
    
    reg_data = 0;
    reg_mask = 0;
    if (CDK_SUCCESS(rv) && line_intf) {
        rv = _bcm82328_intf_type_reg_get(pc, line_intf, 
                            PHY_REGS_SIDE_LINE, &reg_data, &reg_mask);
    }

    if (CDK_SUCCESS(rv) && reg_mask) {
        rv = _bcm82328_interface_update(pc, reg_data, reg_mask);
    }

    /* If not passtrhu an, make sure firmware has enabled an */
    if (!(PHY_CTRL_FLAGS(pc) & PHY_F_PASSTHRU)) {
        rv = PHY_AUTONEG_GET(pc, &curr_mode);
        if (autoneg != curr_mode) {
            PHY_WARN(pc, ("82328 device autonegotiation mismatch: port=%d an=%d\n",
                       pc->port, autoneg));
        }
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:    
 *      bcm82328_phy_autoneg_get
 * Purpose:     
 *      Get the current auto-negotiation setting.
 * Parameters:
 *      pc - PHY control structure
 *      autoneg - (OUT) non-zero indicates autoneg enabled
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm82328_phy_autoneg_get(phy_ctrl_t *pc, int *autoneg)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t data;

    PHY_CTRL_CHECK(pc);

    if (!autoneg) {
        return CDK_E_PARAM;
    }
 
    *autoneg = 0;
    if (PHY_CTRL_FLAGS(pc) & PHY_F_PASSTHRU) {
        if (PHY_CTRL_NEXT(pc)) {
            rv = PHY_AUTONEG_GET(PHY_CTRL_NEXT(pc), autoneg);
        }
    } else {
        ioerr += PHY_BUS_READ(pc, AN_CTRL_REG, &data);
        if (data & AN_ENABLE) {
            *autoneg = 1;
        }
        *autoneg = (data & AN_ENABLE);
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:    
 *      bcm82328_phy_loopback_set
 * Purpose:     
 *      Set the internal PHY loopback mode.
 * Parameters:
 *      pc - PHY control structure
 *      enable - non-zero enables PHY loopback
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm82328_phy_loopback_set(phy_ctrl_t *pc, int enable)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t data, cur_speed;
    int datapath, next_lb;

    PHY_CTRL_CHECK(pc);

    next_lb = 0;
    if (PHY_CTRL_FLAGS(pc) & PHY_F_PASSTHRU) {
        next_lb = enable;
    }

    /* Set loopback on upstream PHY */
    rv = PHY_LOOPBACK_SET(PHY_CTRL_NEXT(pc), next_lb);

    if (CDK_SUCCESS(rv)) {
        rv = PHY_SPEED_GET(pc, &cur_speed);
    }
    
    /* Loopback requires 20-bit datapath */
    _bcm82328_datapath_mode_get(pc, &datapath);
    if (datapath != PHY_DATAPATH_20_BIT) {
        _bcm82328_datapath_mode_set(pc, PHY_DATAPATH_20_BIT);
    }
    
    PHY_SYS_USLEEP(1000); 

    /* Change to the line side */
    if (CDK_SUCCESS(rv)) {
        rv = _bcm82328_side_regs_select(pc, PHY_REGS_SIDE_LINE);
    }

    ioerr += PHY_BUS_READ(pc, PMA_PMD_GP_0_REG, &data);
    data &= ~PMA_PMD_GP_0_TX_SQUELCHING_DISABLE;
    if (enable) {
        data |= PMA_PMD_GP_0_TX_SQUELCHING_DISABLE;
    }
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_GP_0_REG, data);

    /* Change to the system side */
    if (CDK_SUCCESS(rv)) {
        rv = _bcm82328_side_regs_select(pc, PHY_REGS_SIDE_SYSTEM);
    }

    if (enable) {
        ioerr += PHY_BUS_READ(pc, PMA_PMD_TX_ANA_CTRL7_REG, &data);
        data &= ~(PMA_PMD_TX_ANA_CTRL7_DISABLE_FORCE_EN_SM | 
                  PMA_PMD_TX_ANA_CTRL7_DISABLE_FORCE_VAL_SM);
        ioerr += PHY_BUS_WRITE(pc, PMA_PMD_TX_ANA_CTRL7_REG, data);
        
        ioerr += PHY_BUS_READ(pc, PMA_PMD_TX_PI_CTRL0_REG, &data);
        data |= (PMA_PMD_TX_PI_CTRL0_LOOP_TIMING_ENT | 
                 PMA_PMD_TX_PI_CTRL0_ENABLE);
        ioerr += PHY_BUS_WRITE(pc, PMA_PMD_TX_PI_CTRL0_REG, data);
        
        ioerr += PHY_BUS_READ(pc, PMA_PMD_REMOTE_LB_CTRL_REG, &data);
        data |= PMA_PMD_REMOTE_LB_CTRL_ENABLE;
        ioerr += PHY_BUS_WRITE(pc, PMA_PMD_REMOTE_LB_CTRL_REG, data);
        
        PHY_SYS_USLEEP(100); 

        ioerr += PHY_BUS_READ(pc, PMA_PMD_TX_PI_CTRL5_REG, &data);
        data &= ~PMA_PMD_TX_PI_CTRL5_PASS_THRU_SEL;
        ioerr += PHY_BUS_WRITE(pc, PMA_PMD_TX_PI_CTRL5_REG, data);

        ioerr += PHY_BUS_READ(pc, PMA_PMD_TX_PI_CTRL0_REG, &data);
        data |= PMA_PMD_TX_PI_CTRL0_EXT_CTRL_ENABLE;
        ioerr += PHY_BUS_WRITE(pc, PMA_PMD_TX_PI_CTRL0_REG, data);
    } else {
        ioerr += PHY_BUS_READ(pc, PMA_PMD_REMOTE_LB_CTRL_REG, &data);
        data &= ~PMA_PMD_REMOTE_LB_CTRL_ENABLE;
        ioerr += PHY_BUS_WRITE(pc, PMA_PMD_REMOTE_LB_CTRL_REG, data);

        ioerr += PHY_BUS_READ(pc, PMA_PMD_TX_PI_CTRL5_REG, &data);
        data |= PMA_PMD_TX_PI_CTRL5_PASS_THRU_SEL;
        ioerr += PHY_BUS_WRITE(pc, PMA_PMD_TX_PI_CTRL5_REG, data);

        PHY_SYS_USLEEP(100); 
        
        ioerr += PHY_BUS_READ(pc, PMA_PMD_TX_PI_CTRL0_REG, &data);
        data &= ~PMA_PMD_TX_PI_CTRL0_EXT_CTRL_ENABLE;
        ioerr += PHY_BUS_WRITE(pc, PMA_PMD_TX_PI_CTRL0_REG, data);
    }

    /* Change back to the line side */
    if (CDK_SUCCESS(rv)) {
        rv = _bcm82328_side_regs_select(pc, PHY_REGS_SIDE_LINE);
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:    
 *      bcm82328_phy_loopback_get
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
bcm82328_phy_loopback_get(phy_ctrl_t *pc, int *enable)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t data;
    
    PHY_CTRL_CHECK(pc);

    if (!enable) {
        return CDK_E_PARAM;
    }
    *enable = 0;
    
    /* Change to the system side */
    rv = _bcm82328_side_regs_select(pc, PHY_REGS_SIDE_SYSTEM);

    ioerr += PHY_BUS_READ(pc, PMA_PMD_REMOTE_LB_CTRL_REG, &data);
    if (data & PMA_PMD_REMOTE_LB_CTRL_ENABLE) {
        *enable = 1;
    }

    /* Change back to the line side */
    if (CDK_SUCCESS(rv)) {
        rv = _bcm82328_side_regs_select(pc, PHY_REGS_SIDE_LINE);
    }
    
    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:    
 *      bcm82328_phy_ability_get
 * Purpose:     
 *      Get the abilities of the PHY.
 * Parameters:
 *      pc - PHY control structure
 *      abil - (OUT) ability mask indicating supported options/speeds.
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm82328_phy_ability_get(phy_ctrl_t *pc, uint32_t *ability)
{
    PHY_CTRL_CHECK(pc);

    if (!ability) {
        return CDK_E_PARAM;
    }

    *ability = 0;
    if (BCM82328_SINGLE_PORT_MODE(pc)) {
        *ability |= PHY_ABIL_40GB;
    } else {
        *ability |= (PHY_ABIL_10GB | PHY_ABIL_1000MB_FD | PHY_ABIL_100MB_FD | 
                     PHY_ABIL_10MB_FD);
    }
    *ability |= (PHY_ABIL_PAUSE | PHY_ABIL_PAUSE_ASYMM | PHY_ABIL_XGMII |
                 PHY_ABIL_LOOPBACK);

    return CDK_E_NONE;
}

/*
 * Function:
 *      bcm82328_phy_config_set
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
bcm82328_phy_config_set(phy_ctrl_t *pc, phy_config_t cfg, uint32_t val, void *cd)
{
    PHY_CTRL_CHECK(pc);

    switch (cfg) {
    case PhyConfig_InitStage:
        if (PHY_CTRL_FLAGS(pc) & PHY_F_STAGED_INIT) {
            return _bcm82328_init_stage(pc, val);
        }
        break;
#if PHY_CONFIG_INCLUDE_XAUI_TX_POLARITY_SET
    case PhyConfig_XauiTxPolInvert:
        return _bcm82328_tx_polarity_invert(pc,val);
#endif
#if PHY_CONFIG_INCLUDE_XAUI_RX_POLARITY_SET
    case PhyConfig_XauiRxPolInvert:
        return _bcm82328_rx_polarity_invert(pc,val);
#endif
    default:
        break;
    }

    return CDK_E_UNAVAIL;
}

/*
 * Function:
 *      bcm82328_phy_config_get
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
bcm82328_phy_config_get(phy_ctrl_t *pc, phy_config_t cfg, uint32_t *val, void *cd)
{
    int rv = CDK_E_NONE;

    PHY_CTRL_CHECK(pc);

    switch (cfg) {
    case PhyConfig_Enable:
        *val = 1;
        break;
    case PhyConfig_Clause45Devs:
        *val = 0x82;
        break;
    case PhyConfig_BcastAddr:
        *val = PHY_CTRL_BUS_ADDR(pc) & ~0x1f;
        break;
    case PhyConfig_AdvLocal:
        rv = _bcm82328_ability_local_get(pc, val);
        break;
    case PhyConfig_AdvRemote:
        rv = _bcm82328_ability_remote_get(pc, val);
        break;
    default:
        return CDK_E_UNAVAIL;
    }

    return rv;
}

/*
 * Variable:    bcm82328_drv
 * Purpose:     PHY Driver for BCM82328.
 */
phy_driver_t bcm82328_drv = {
    "bcm82328",
    "BCM82328 40GbE/10GbE PHY Driver",  
    0,
    bcm82328_phy_probe,                  /* pd_probe */
    bcm82328_phy_notify,                 /* pd_notify */
    bcm82328_phy_reset,                  /* pd_reset */
    bcm82328_phy_init,                   /* pd_init */
    bcm82328_phy_link_get,               /* pd_link_get */
    bcm82328_phy_duplex_set,             /* pd_duplex_set */
    bcm82328_phy_duplex_get,             /* pd_duplex_get */
    bcm82328_phy_speed_set,              /* pd_speed_set */
    bcm82328_phy_speed_get,              /* pd_speed_get */
    bcm82328_phy_autoneg_set,            /* pd_autoneg_set */
    bcm82328_phy_autoneg_get,            /* pd_autoneg_get */
    bcm82328_phy_loopback_set,           /* pd_loopback_set */
    bcm82328_phy_loopback_get,           /* pd_loopback_get */
    bcm82328_phy_ability_get,            /* pd_ability_get */
    bcm82328_phy_config_set,             /* pd_config_set */
    bcm82328_phy_config_get,             /* pd_config_get */
    NULL,                                /* pd_status_get */
    NULL                                 /* pd_cable_diag */
};
