/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * PHY driver for BCM84756, BCM84757 and BCM84759.
 */

#include <phy/phy.h>
#include <phy/ge_phy.h>

#include <cdk/cdk_debug.h>

extern unsigned char bcm84756_ucode[];
extern unsigned int bcm84756_ucode_len;
extern unsigned char bcm84756_b0_ucode[];
extern unsigned int bcm84756_b0_ucode_len;

#define PHY_REGS_SIDE_SYSTEM                    0
#define PHY_REGS_SIDE_LINE                      1

#define PHY_ROM_LOAD_POLL_MAX                   10
#define PHY_DOWNLOAD_MSEC                       200

#define BCM84756_PMA_PMD_ID0                    0x600d
#define BCM84756_PMA_PMD_ID1                    0x8670
#define PHY_ID1_REV_MASK                        0x000f

#define BCM84756_CHIP_ID                        0x00084756
#define BCM84757_CHIP_ID                        0x00084757
#define BCM84759_CHIP_ID                        0x00084759

/* Checksum value for firmeare*/
#define BCM84756_SPI_ROM_CHKSUM                 0x600d
#define BCM84756_MDIO_FW_CHKSUM                 0x600d


#define C45_DEVAD(_a)                           LSHIFT32((_a),16)
#define DEVAD_PMA_PMD                           C45_DEVAD(MII_C45_DEV_PMA_PMD)
#define DEVAD_PCS                               C45_DEVAD(MII_C45_DEV_PCS)
#define DEVAD_AN                                C45_DEVAD(MII_C45_DEV_AN)

/* PMA/PMD registers */
#define PMA_PMD_CTRL_REG                        (DEVAD_PMA_PMD + MII_CTRL_REG)
#define PMA_PMD_STAT_REG                        (DEVAD_PMA_PMD + MII_STAT_REG)
#define PMA_PMD_ID0_REG                         (DEVAD_PMA_PMD + MII_PHY_ID0_REG)
#define PMA_PMD_ID1_REG                         (DEVAD_PMA_PMD + MII_PHY_ID1_REG)
#define PMA_PMD_SPEED_ABIL_REG                  (DEVAD_PMA_PMD + 0x0004)
#define PMA_PMD_DEV_IN_PKG1_REG                 (DEVAD_PMA_PMD + 0x0005)
#define PMA_PMD_DEV_IN_PKG2_REG                 (DEVAD_PMA_PMD + 0x0006)
#define PMA_PMD_CTRL2_REG                       (DEVAD_PMA_PMD + 0x0007)
#define PMA_PMD_STAT2_REG                       (DEVAD_PMA_PMD + 0x0008)
#define PMA_PMD_PMD_TX_DISABLE_REG              (DEVAD_PMA_PMD + 0x0009)
#define PMA_PMD_RX_ALARM_STATUS_REG             (DEVAD_PMA_PMD + 0x9003)
#define PMA_PMD_CHIP_VERSION_REG                (DEVAD_PMA_PMD + 0xc801)
#define PMA_PMD_SPEED_LINK_DETECT_STAT_REG      (DEVAD_PMA_PMD + 0xc820)
#define PMA_PMD_BCST_REG                        (DEVAD_PMA_PMD + 0xc8fe)
#define PMA_PMD_CHIP_MODE_REG                   (DEVAD_PMA_PMD + 0xc805)
#define PMA_PMD_BYPASS_MACSEC_REG               (DEVAD_PMA_PMD + 0xc8f0)
#define PMA_PMD_SPI_CTRL_REG                    (DEVAD_PMA_PMD + 0xc848)
#define PMA_PMD_MISC_CTRL1_REG                  (DEVAD_PMA_PMD + 0xca85)
#define PMA_PMD_M8051_MSGIN_REG                 (DEVAD_PMA_PMD + 0xca12)
#define PMA_PMD_M8051_MSGOUT_REG                (DEVAD_PMA_PMD + 0xca13)

#define PMA_PMD_CHIP_ID_LSB_REG                 (DEVAD_PMA_PMD + 0xc802)
#define PMA_PMD_CHIP_ID_MSB_REG                 (DEVAD_PMA_PMD + 0xc803)
#define PMA_PMD_DATAPATH_CDR_SELECT_REG         (DEVAD_PMA_PMD + 0xc806)
#define PMA_PMD_LINK_DETECT_STAT_REG            (DEVAD_PMA_PMD + 0xc81f)
#define PMA_PMD_GEN_REG_0                       (DEVAD_PMA_PMD + 0xca18)
#define PMA_PMD_GEN_REG_1                       (DEVAD_PMA_PMD + 0xca19)
#define PMA_PMD_GEN_REG_2                       (DEVAD_PMA_PMD + 0xca1a)
#define PMA_PMD_GEN_REG_3                       (DEVAD_PMA_PMD + 0xca1b)
#define PMA_PMD_GEN_REG_4                       (DEVAD_PMA_PMD + 0xca1c)
#define PMA_PMD_SYSTEM_SIDE_REG                 (DEVAD_PMA_PMD + 0xffff)

/* PCS registers */
#define PCS_CTRL_REG                            (DEVAD_PCS + MII_CTRL_REG)
#define PCS_STAT_REG                            (DEVAD_PCS + MII_STAT_REG)
#define PCS_ID0_REG                             (DEVAD_PCS + MII_PHY_ID0_REG)
#define PCS_ID1_REG                             (DEVAD_PCS + MII_PHY_ID1_REG)

/* AN registers */
#define AN_CTRL_REG                             (DEVAD_AN + MII_CTRL_REG)
#define AN_STAT_REG                             (DEVAD_AN + MII_STAT_REG)
#define AN_0X8000_REG                           (DEVAD_AN + 0x8000)
#define AN_BASE1000X_CTRL1_REG                  (DEVAD_AN + 0x8300)
#define AN_BASE1000X_CTRL2_REG                  (DEVAD_AN + 0x8301)
#define AN_BASE1000X_STAT1_REG                  (DEVAD_AN + 0x8304)
#define AN_MISC1_REG                            (DEVAD_AN + 0x8308)
#define AN_MISC2_REG                            (DEVAD_AN + 0x8309)
#define AN_0X835C_REG                           (DEVAD_AN + 0x835c)
#define AN_MII_CTRL_REG                         (DEVAD_AN + 0xffe0)
#define AN_MII_STAT_REG                         (DEVAD_AN + 0xffe1)
#define AN_AUTONEG_LP_ABILITY_REG               (DEVAD_AN + 0xffe5)

/* PMA/PMD standard register definitions */
/* PMD control register, 0x0000 */
#define PMA_PMD_CTRL_RESET                      (1L << 15)
#define PMA_PMD_CTRL_SS_LSB                     (1L << 13)
#define PMA_PMD_CTRL_PD                         (1L << 11)
#define PMA_PMD_CTRL_SS_MSB                     (1L << 6)
#define PMA_PMD_CTRL_SS                         (0xf << 2)
#define PMA_PMD_CTRL_PMS_LOOPBACK               (1L << 0)
#define PMA_PMD_CTRL_SS_SPEED_SEL   (PMA_PMD_CTRL_SS_LSB | PMA_PMD_CTRL_SS_MSB)
#define PMA_PMD_CTRL_SS_MASK        (PMA_PMD_CTRL_SS_LSB | PMA_PMD_CTRL_SS_MSB)
#define PMA_PMD_CTRL_10GSS_MASK     (PMA_PMD_CTRL_SS_LSB | \
                                     PMA_PMD_CTRL_SS_MSB | \
                                    (0xf << 2))
#define PMA_PMD_CTRL_SS_10          0
#define PMA_PMD_CTRL_SS_100         (PMA_PMD_CTRL_SS_LSB)
#define PMA_PMD_CTRL_SS_1000        (PMA_PMD_CTRL_SS_MSB)
#define PMA_PMD_CTRL_SS_10000       (PMA_PMD_CTRL_SS_SPEED_SEL)


/* PMD control 2 register, 0x0007 */
#define PMA_PMD_CTRL2_1GT_PMD_TYPE              (0xc)
#define PMA_PMD_CTRL2_10GLRM_PMD_TYPE           (0x8)
#define PMA_PMD_TYPE_SELECT_MASK                (0x3f << 0)

/* PMD transmit disable register, 0x0009 */
#define PMA_PMD_TX_DISABLE_GLOBAL_DISABLE       (1L << 0)

/* Chip mode register, 0xc805 */
#define PMA_PMD_CHIP_MODE_QUAD_10G              (0x0)
#define PMA_PMD_CHIP_MODE_40G                   (0x1)
#define PMA_PMD_CHIP_MODE_MASK                  (0x3 << 0)

/* Datapath and CDR selection register, 0xc806 */
#define PMA_PMD_DP_CDR_SELECT_TX_REPEATER_MASK  (1L << 2)
#define PMA_PMD_DP_CDR_SELECT_RX_REPEATER_MASK  (1L << 1)

/* Speed Link status register defination, 0xc81f */
#define PMA_PMD_SPEED_LD_STAT_SYS_RX_SD             (1 << 15)
#define PMA_PMD_SPEED_LD_STAT_SYS_AN_COMPLETE       (1 << 14)
#define PMA_PMD_SPEED_LD_STAT_SYS_PDM_SPEED_10G     (1 << 12)
#define PMA_PMD_SPEED_LD_STAT_SYS_PDM_SPEED_2P5G    (1 << 11)
#define PMA_PMD_SPEED_LD_STAT_SYS_PDM_SPEED_1G      (1 << 10)
#define PMA_PMD_SPEED_LD_STAT_SYS_PDM_SPEED_100M    (1 << 9)
#define PMA_PMD_SPEED_LD_STAT_SYS_PDM_SPEED_10M     (1 << 8)
#define PMA_PMD_SPEED_LD_STAT_LINE_RX_SD            (1 << 7)
#define PMA_PMD_SPEED_LD_STAT_LINE_AN_COMPLETE      (1 << 6)
#define PMA_PMD_SPEED_LD_STAT_LINE_PDM_SPEED_10G    (1 << 4)
#define PMA_PMD_SPEED_LD_STAT_LINE_PDM_SPEED_2P5G   (1 << 3)
#define PMA_PMD_SPEED_LD_STAT_LINE_PDM_SPEED_1G     (1 << 2)
#define PMA_PMD_SPEED_LD_STAT_LINE_PDM_SPEED_100M   (1 << 1)
#define PMA_PMD_SPEED_LD_STAT_LINE_PDM_SPEED_10M    (1 << 0)
#define PMA_PMD_SPEED_LD_STAT_SYS_PDM_SPEED_MASK                \
                    (PMA_PMD_SPEED_LD_STAT_SYS_PDM_SPEED_10M |  \
                     PMA_PMD_SPEED_LD_STAT_SYS_PDM_SPEED_100M | \
                     PMA_PMD_SPEED_LD_STAT_SYS_PDM_SPEED_1G |   \
                     PMA_PMD_SPEED_LD_STAT_SYS_PDM_SPEED_2P5G | \
                     PMA_PMD_SPEED_LD_STAT_SYS_PDM_SPEED_10G)
#define PMA_PMD_SPEED_LD_STAT_LINE_PDM_SPEED_MASK                \
                    (PMA_PMD_SPEED_LD_STAT_LINE_PDM_SPEED_10M |  \
                     PMA_PMD_SPEED_LD_STAT_LINE_PDM_SPEED_100M | \
                     PMA_PMD_SPEED_LD_STAT_LINE_PDM_SPEED_1G |   \
                     PMA_PMD_SPEED_LD_STAT_LINE_PDM_SPEED_2P5G | \
                     PMA_PMD_SPEED_LD_STAT_LINE_PDM_SPEED_10G)

/* Speed link detect status register, 0xc820 */
#define PMA_PMD_SPEED_LINK_DETECT_PCS_10G_MASK  (1L << 2)
#define PMA_PMD_SPEED_LINK_DETECT_PCS_1G_MASK   (1L << 0)

/* Bypass macsec code register, 0xc8f0 */
#define PMA_PMD_MACSEC_UDSW_RESET_MACSEC_MASK   (1L << 15)
#define PMA_PMD_MACSEC_UDSW_PWRDW_MACSEC_MASK   (1L << 11)
#define PMA_PMD_MACSEC_ENABLE_PWRDN_MACSEC_MASK (1L << 8)
#define PMA_PMD_MACSEC_BYPASS_MODE_MASK         (1L << 0)

/* Select register type register, 0xffff */
#define PMA_PMD_SYSTEM_SIDE_SEL_MASK            (1L << 0)

/* SPI control register, 0xc848 */
#define PMA_PMD_SPI_CTRL_SPI_PORT_USED_MASK     (1L << 15)
#define PMA_PMD_SPI_CTRL_SPI_BOOT_MASK          (1L << 14)
#define PMA_PMD_SPI_CTRL_SPI_DWLD_DONE_MASK     (1L << 13)
#define PMA_PMD_SPI_CTRL_SPI_ENABLE_MASK        (1L << 2)

/* Misc. control 1 register */
#define PMA_PMD_MISC_CTRL1_SPI_DWLD_32K_MASK    (1L << 3)
#define PMA_PMD_MISC_CTRL1_SER_BOOT_CTL_MASK    (1L << 0)

/* Auto-negotiation register definitions */
/* AN 1000x control 1 register, 0x8300 */
#define AN_BASE1000X_CTRL1_DIS_PLL_PWRDN        (1L << 6)
#define AN_BASE1000X_CTRL1_SGMII_MASTER         (1L << 5)
#define AN_BASE1000X_CTRL1_AUTODETECT_EN        (1L << 4)
#define AN_BASE1000X_CTRL1_INVERT_SD            (1L << 3)
#define AN_BASE1000X_CTRL1_SD_ENABLE            (1L << 2)
#define AN_BASE1000X_CTRL1_TBI_INT              (1L << 1)
#define AN_BASE1000X_CTRL1_FIBER_MODE           (1L << 0)

/* AN base 1000x control 2 register, 0x8301 */
#define AN_BASE1000X_CTRL2_PDET_MASK            (1L << 0)

/* AN auto-negotiation link partner ability register, 0xffe5 */
#define AN_AUTONEG_LP_ABILITY_C37_ASYM_PAUSE    (1L << 8)
#define AN_AUTONEG_LP_ABILITY_C37_PAUSE         (1L << 7)
#define AN_AUTONEG_LP_ABILITY_C37_HD            (1L << 6)
#define AN_AUTONEG_LP_ABILITY_C37_FD            (1L << 5)

#define CHIP_IS_BCM84756_FAMILY(_chipid)    \
           (((_chipid) == BCM84756_CHIP_ID) || \
            ((_chipid) == BCM84757_CHIP_ID) || \
            ((_chipid) == BCM84759_CHIP_ID))

#define CHIP_VERSION_IS_A0(_data)           (_data == 0xa0a0)
#define CHIP_VERSION_IS_B0(_data)           (_data == 0xb0b0)
#define CHIP_VERSION_IS_C0(_data)           (_data == 0xc0c0)

#define PHY_COPPER_MODE(pc)      (!(PHY_CTRL_FLAGS(pc) & PHY_F_FIBER_MODE))
#define PHY_FIBER_MODE(pc)       (PHY_CTRL_FLAGS(pc) & PHY_F_FIBER_MODE)

/* Private PHY flag is used to indicate firmware download method */
#define PHY_F_FW_MDIO_LOAD              PHY_F_PRIVATE

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
 *      _bcm84756_side_regs_select
 * Purpose:
 *      Select side of the PHY register.
 * Parameters:
 *      pc - PHY control structure
 *      side - System side or line side
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84756_side_regs_select(phy_ctrl_t *pc, int side)
{
    int ioerr = 0;
    uint32_t data;

    ioerr += PHY_BUS_READ(pc, PMA_PMD_SYSTEM_SIDE_REG, &data);
    data &= ~PMA_PMD_SYSTEM_SIDE_SEL_MASK;
    if (side == PHY_REGS_SIDE_SYSTEM) {
        data |= PMA_PMD_SYSTEM_SIDE_SEL_MASK;
    }
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_SYSTEM_SIDE_REG, data);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

/*
 * Function:     
 *    _bcm84756_line_mode_get
 * Purpose:    
 *    Get the Line side mode
 * Parameters:
 *    pc - PHY control structure 
 *    mode - Desired mode (PHY_IF_SGMII/PHY_IF_TBI/PHY_IF_SFI)
 * Returns:    
 */
static int
_bcm84756_line_mode_get(phy_ctrl_t *pc, uint32_t *mode)
{
    int ioerr = 0;
    uint32_t data;
    
    if (PHY_COPPER_MODE(pc)) {
        *mode = PHY_IF_SGMII;
    } else {
        *mode = PHY_IF_TBI;
        
        ioerr += PHY_BUS_READ(pc, PMA_PMD_CTRL_REG, &data);
        if ((data & PMA_PMD_CTRL_SS_SPEED_SEL) == PMA_PMD_CTRL_SS_10000) {
            *mode = PHY_IF_SFI;
        }
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

/*
 * Function:     
 *    _bcm84756_line_mode_set
 * Purpose:    
 *    Set the Line side mode
 * Parameters:
 *    pc - PHY control structure 
 *    mode - Desired mode (PHY_IF_SGMII/PHY_IF_TBI/PHY_IF_SFI)
 * Returns:    
 */
static int
_bcm84756_line_mode_set(phy_ctrl_t *pc, uint32_t mode)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t data;
    int event;

    /* Disable Autodetect */
    ioerr += PHY_BUS_READ(pc, AN_BASE1000X_CTRL1_REG, &data);
    data &= ~AN_BASE1000X_CTRL1_AUTODETECT_EN;
    ioerr += PHY_BUS_WRITE(pc, AN_BASE1000X_CTRL1_REG, data);

    switch (mode) {
        case PHY_IF_SGMII :
            /* Clear fiber mode and SGMII master mode */
            ioerr += PHY_BUS_WRITE(pc, AN_BASE1000X_CTRL1_REG, 0x0184);
            
            /* Disable force speed */
            ioerr += PHY_BUS_READ(pc, AN_MISC1_REG, &data);
            data &= ~(0x1f);
            ioerr += PHY_BUS_WRITE(pc, AN_MISC1_REG, data);

            /* Disable PMA_PMD force speed */
            ioerr += PHY_BUS_READ(pc, AN_MISC2_REG, &data);
            data &= ~(0x6020);
            data |= 0x6000;
            ioerr += PHY_BUS_WRITE(pc, AN_MISC2_REG, data);

            /* Set linkup in basepage */
            ioerr += PHY_BUS_WRITE(pc, AN_0X835C_REG, 0x0801);
            
            /* Set autoneg enable */
            ioerr += PHY_BUS_WRITE(pc, AN_MII_CTRL_REG, 0x1140);
            break;

        case PHY_IF_TBI :
            ioerr += PHY_BUS_READ(pc, AN_BASE1000X_CTRL1_REG, &data);
            data |= AN_BASE1000X_CTRL1_FIBER_MODE;
            ioerr += PHY_BUS_WRITE(pc, AN_BASE1000X_CTRL1_REG, data);
            break;

        case PHY_IF_SFI :
            ioerr += PHY_BUS_READ(pc, AN_BASE1000X_CTRL1_REG, &data);
            data |= AN_BASE1000X_CTRL1_FIBER_MODE;
            ioerr += PHY_BUS_WRITE(pc, AN_BASE1000X_CTRL1_REG, data);
        
            /* Set PMA type */
            ioerr += PHY_BUS_READ(pc, PMA_PMD_CTRL2_REG, &data);
            data &= ~(PMA_PMD_TYPE_SELECT_MASK);
            data |= PMA_PMD_CTRL2_10GLRM_PMD_TYPE;
            ioerr += PHY_BUS_WRITE(pc, PMA_PMD_CTRL2_REG, data);
            break;

        default :
            rv = CDK_E_PARAM;
            break;
    }

    event = PhyEvent_ChangeToFiber;
    if (mode == PHY_IF_SGMII) {
        event = PhyEvent_ChangeToCopper;
    }
    if (CDK_SUCCESS(rv)) {
        rv = PHY_NOTIFY(pc, event);
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:     
 *    _bcm84756_sys_mode_set
 * Purpose:    
 *    Set the system side mode
 * Parameters:
 *    pc - PHY control structure 
 *    mode - Desired mode (PHY_IF_SGMII/PHY_IF_TBI/PHY_IF_XFI)
 * Returns:    
 */
static int
_bcm84756_sys_mode_set(phy_ctrl_t *pc, uint32_t mode)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t data;

    rv = _bcm84756_side_regs_select(pc, PHY_REGS_SIDE_SYSTEM);

    /* Disable Autodetect */
    ioerr += PHY_BUS_READ(pc, AN_BASE1000X_CTRL1_REG, &data);
    data &= ~AN_BASE1000X_CTRL1_AUTODETECT_EN;
    ioerr += PHY_BUS_WRITE(pc, AN_BASE1000X_CTRL1_REG, data);

    switch (mode) {
        case PHY_IF_SGMII :
            /* Disable PLL start sequence */
            ioerr += PHY_BUS_READ(pc, AN_0X8000_REG, &data);
            data &= ~0x2000;
            ioerr += PHY_BUS_WRITE(pc, AN_0X8000_REG, data);
            
            /* Clear fiber mode, set SGMII master mode */
            ioerr += PHY_BUS_WRITE(pc, AN_BASE1000X_CTRL1_REG, 0x01a0);

            /* Disable force speed */
            ioerr += PHY_BUS_READ(pc, AN_MISC1_REG, &data);
            data &= ~(0x1f);
            ioerr += PHY_BUS_WRITE(pc, AN_MISC1_REG, data);

            /* Disable PMA_PMD force speed */
            ioerr += PHY_BUS_READ(pc, AN_MISC2_REG, &data);
            data &= ~(0x6020);
            data |= 0x6000;
            ioerr += PHY_BUS_WRITE(pc, AN_MISC2_REG, data);

            /* Clear linkup bit in basepage */
            ioerr += PHY_BUS_WRITE(pc, AN_0X835C_REG, 0x0001);
            
            /* Clear autoneg enable */
            ioerr += PHY_BUS_WRITE(pc, AN_MII_CTRL_REG, 0x0140);

            /* Enable PLL start sequence */
            ioerr += PHY_BUS_READ(pc, AN_0X8000_REG, &data);
            data |= 0x2000;
            ioerr += PHY_BUS_WRITE(pc, AN_0X8000_REG, data);
            break;
            
        case PHY_IF_TBI :
            ioerr += PHY_BUS_READ(pc, AN_BASE1000X_CTRL1_REG, &data);
            data |= AN_BASE1000X_CTRL1_FIBER_MODE;
            ioerr += PHY_BUS_WRITE(pc, AN_BASE1000X_CTRL1_REG, data);
            break;
        
        case PHY_IF_XFI :
            break;
    
        default :
            _bcm84756_side_regs_select(pc, PHY_REGS_SIDE_LINE);
            return CDK_E_PARAM;
    }

    _bcm84756_side_regs_select(pc, PHY_REGS_SIDE_LINE);
    
    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _bcm84756_broadcast_set
 * Purpose:
 *      Put all lanes of a PHY in or out of MDIO broadcast mode.
 * Parameters:
 *      pc - PHY control structure
 *      enable - enable MDIO broadcast mode for this core
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84756_broadcast_set(phy_ctrl_t *pc, int enable)
{
    int ioerr = 0;
    uint32_t data;

    data = enable ? 0xffff : 0;
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_BCST_REG, data);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

/*
 * Function:
 *      _bcm84756_soft_reset
 * Purpose:
 *      Perform soft reset and wait for completion.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84756_soft_reset(phy_ctrl_t *pc)
{
    int ioerr = 0;
    uint32_t data;

    /* Reset the PHY **/
    ioerr += PHY_BUS_READ(pc, PMA_PMD_CTRL_REG, &data);
    data |= PMA_PMD_CTRL_RESET;
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_CTRL_REG, data);

    /* Needs minimum of 5us to complete the reset */
    PHY_SYS_USLEEP(30);

    ioerr += PHY_BUS_READ(pc, PMA_PMD_CTRL_REG, &data);
    if (data & PMA_PMD_CTRL_RESET) {
        data &= ~PMA_PMD_CTRL_RESET;
        ioerr += PHY_BUS_WRITE(pc, PMA_PMD_CTRL_REG, data);
        return CDK_E_IO;
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

/*
 * Function:    
 *      _bcm84756_ability_local_get
 * Purpose:     
 *      Get the local abilities of the PHY.
 * Parameters:
 *      pc - PHY control structure
 *      ability - (OUT) ability mask indicating supported options/speeds.
 * Returns:     
 *      CDK_E_xxx
 */
static int
_bcm84756_ability_local_get(phy_ctrl_t *pc, uint32_t *ability)
{
    if (!ability) {
        return CDK_E_PARAM;
    }

    return PHY_ABILITY_GET(pc, ability);
}

/*
 * Function:    
 *      _bcm84756_ability_remote_get
 * Purpose:     
 *      Get the current remoteisement for auto-negotiation.
 * Parameters:
 *      pc - PHY control structure
 *      ability - (OUT) ability mask indicating supported options/speeds.
 * Returns:     
 *      CDK_E_xxx
 */
static int
_bcm84756_ability_remote_get(phy_ctrl_t *pc, uint32_t *ability)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t line_mode;
    uint32_t data;

    if (!ability) {
        return CDK_E_PARAM;
    }
    *ability = 0;

    rv = _bcm84756_line_mode_get(pc, &line_mode);
    if (line_mode == PHY_IF_TBI) {
        ioerr += PHY_BUS_READ(pc, AN_AUTONEG_LP_ABILITY_REG, &data);
        if (data & AN_AUTONEG_LP_ABILITY_C37_FD) {
            *ability |= PHY_ABIL_1000MB_FD;
        } 
        
        switch (data & (AN_AUTONEG_LP_ABILITY_C37_PAUSE |
                        AN_AUTONEG_LP_ABILITY_C37_ASYM_PAUSE)) {
            case AN_AUTONEG_LP_ABILITY_C37_PAUSE:
                *ability |= PHY_ABIL_PAUSE;
                break;
            case AN_AUTONEG_LP_ABILITY_C37_ASYM_PAUSE:
                *ability |= PHY_ABIL_PAUSE_TX;
                break;
            case (AN_AUTONEG_LP_ABILITY_C37_PAUSE | 
                  AN_AUTONEG_LP_ABILITY_C37_ASYM_PAUSE):
                *ability |= PHY_ABIL_PAUSE_RX;
                break;
        }
    } else if (line_mode == PHY_IF_SGMII) {
        rv = _bcm84756_ability_local_get(pc, ability);
    } else {
        return CDK_E_UNAVAIL;
    }

    ioerr += PHY_BUS_READ(pc, PMA_PMD_DATAPATH_CDR_SELECT_REG, &data);
    if (PHY_CTRL_NEXT(pc)) {
        if ((data & PMA_PMD_DP_CDR_SELECT_TX_REPEATER_MASK) &&
            (data & PMA_PMD_DP_CDR_SELECT_RX_REPEATER_MASK)) {
            if (CDK_SUCCESS(rv)) {
                rv = PHY_CONFIG_GET(PHY_CTRL_NEXT(pc), 
                                    PhyConfig_AdvRemote, ability, NULL);
            }
        }
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _bcm84756_firmware_download_prep
 * Purpose:
 *      Prepare for firmware download via MDIO.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84756_firmware_download_prep(phy_ctrl_t *pc)
{
    int ioerr = 0;
    int rv;
    uint32_t data;

    /* Enable MDIO download and RAM boot */
    ioerr += PHY_BUS_READ(pc, PMA_PMD_SPI_CTRL_REG, &data);
    data &= ~(PMA_PMD_SPI_CTRL_SPI_PORT_USED_MASK | 
              PMA_PMD_SPI_CTRL_SPI_DWLD_DONE_MASK);
    data |= PMA_PMD_SPI_CTRL_SPI_BOOT_MASK;
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_SPI_CTRL_REG, data);

    /* Set download size to 32K */
    ioerr += PHY_BUS_READ(pc, PMA_PMD_MISC_CTRL1_REG, &data);
    data |= PMA_PMD_MISC_CTRL1_SPI_DWLD_32K_MASK;
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_MISC_CTRL1_REG, data);

    /* Apply reset to start downloading code from MDIO */
    rv = _bcm84756_soft_reset(pc);

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _bcm84756_firmware_download
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
_bcm84756_mdio_firmware_download(phy_ctrl_t *pc,
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

    /* Wait for 2ms for M8051 start and 5ms to initialize the RAM */
    PHY_SYS_USLEEP(10000);

    /* Write start address (where the code will reside in SRAM) */
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_M8051_MSGIN_REG, 0x8000);

    /* Make sure address word is read by the microcontroller */
    PHY_SYS_USLEEP(10);

    /* Write SPI SRAM size in 16-bit words */
    data = (fw_size / 2);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_M8051_MSGIN_REG, data);

    /* Make sure SRAM size is read by the microcontroller */
    PHY_SYS_USLEEP(10);

    /* Write firmware to SRAM */
    for (idx = 0; idx < fw_size; idx += 2) {
        /* Write data as 16-bit words */
        data = (fw_data[idx] << 8) | fw_data[idx+1];
        ioerr += PHY_BUS_WRITE(pc, PMA_PMD_M8051_MSGIN_REG, data);

        /* Make sure the word is read by the microcontroller */
        PHY_SYS_USLEEP(10);
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

/*
 * Function:
 *      _bcm84756_mdio_firmware_done
 * Purpose:
 *      MDIO firmware download post-processing.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84756_mdio_firmware_done(phy_ctrl_t *pc)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t data;

    /* Read hand-shake message from microcontroller */
    ioerr += PHY_BUS_READ(pc, PMA_PMD_M8051_MSGOUT_REG, &data);
    _PHY_DBG(pc, ("MDIO download done: 0x%04"PRIx32"\n", data));
    
    /* Clear LASI status */
    ioerr += PHY_BUS_READ(pc, PMA_PMD_RX_ALARM_STATUS_REG, &data);
    
    /* Wait for LASI to be asserted when M8051 writes checksum to MSG_OUT */
    PHY_SYS_USLEEP(100);
    ioerr += PHY_BUS_READ(pc, PMA_PMD_M8051_MSGOUT_REG, &data);
    
    /* Need to check if checksum is correct */
    ioerr += PHY_BUS_READ(pc, PMA_PMD_GEN_REG_4, &data);
    if (data != BCM84756_MDIO_FW_CHKSUM) {
        PHY_WARN(pc, ("MDIO download: bad checksum 0x%04"PRIx32"\n", data));
        return CDK_E_FAIL;
    }
        
    /* Read Rev-ID */
    ioerr += PHY_BUS_READ(pc, PMA_PMD_GEN_REG_2, &data);
    PHY_VERB(pc, ("MDIO firmware version = 0x%04"PRIx32"\n", data));

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _bcm84756_rom_firmware_download
 * Purpose:
 *      Load firmware from SPI-ROM.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84756_rom_firmware_download(phy_ctrl_t *pc)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
#if PHY_CONFIG_EXTERNAL_BOOT_ROM == 1
    uint32_t data;
    int idx;
    
    /* Set SPI-ROM downloading to RAM, set SPI-ROM downloading not done */
    /* Eanble serial boot and SPI port */ 
    ioerr += PHY_BUS_READ(pc, PMA_PMD_SPI_CTRL_REG, &data);
    data &= ~(PMA_PMD_SPI_CTRL_SPI_DWLD_DONE_MASK | 
              PMA_PMD_SPI_CTRL_SPI_ENABLE_MASK);
    data |= (PMA_PMD_SPI_CTRL_SPI_PORT_USED_MASK | 
             PMA_PMD_SPI_CTRL_SPI_BOOT_MASK);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_SPI_CTRL_REG, data);

    /* Apply software reset to download code from SPI-ROM */
    ioerr += PHY_BUS_READ(pc, PMA_PMD_CTRL_REG, &data);
    data |= PMA_PMD_CTRL_RESET;
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_CTRL_REG, data);

    for (idx = 0; idx < PHY_ROM_LOAD_POLL_MAX; idx++) {
        PHY_SYS_USLEEP(100000);
        
        ioerr += PHY_BUS_READ(pc, PMA_PMD_SPI_CTRL_REG, &data);
        if (data & PMA_PMD_SPI_CTRL_SPI_DWLD_DONE_MASK) {
            /* Need to check if checksum is correct */
            ioerr += PHY_BUS_READ(pc, PMA_PMD_GEN_REG_4, &data);
            if (data != BCM84756_SPI_ROM_CHKSUM) {
                PHY_WARN(pc, ("SPI download: bad checksum 0x%04"PRIx32"\n", data));
                return CDK_E_FAIL;
            }
            break;
        }
    }
    if (idx >= PHY_ROM_LOAD_POLL_MAX) { 
        PHY_WARN(pc, ("SPI-Download Firmware download failure\n"));
        return CDK_E_TIMEOUT;
    }

    /* Read Rev-ID */
    ioerr += PHY_BUS_READ(pc, PMA_PMD_GEN_REG_2, &data);
    PHY_VERB(pc, ("SPI firmware version = 0x%04"PRIx32"\n", data));
#endif /* PHY_CONFIG_EXTERNAL_BOOT_ROM */

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _bcm84756_init_stage_0
 * Purpose:
 *      PHY init.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84756_init_stage_0(phy_ctrl_t *pc)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t data;
    
    PHY_VERB(pc, ("BCM84756 init stage 0, port:%d\n", pc->port));

    /* By default select ethernet chip mode */
    ioerr += PHY_BUS_READ(pc, PMA_PMD_CHIP_MODE_REG, &data);
    data &= ~PMA_PMD_CHIP_MODE_MASK;
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_CHIP_MODE_REG, data);

    /* Reset */    
    rv = _bcm84756_soft_reset(pc);

    ioerr += PHY_BUS_READ(pc, AN_BASE1000X_CTRL1_REG, &data);
    data |= AN_BASE1000X_CTRL1_AUTODETECT_EN;
    ioerr += PHY_BUS_WRITE(pc, AN_BASE1000X_CTRL1_REG, data);
    
    /* Set the default is fiber mode */
    PHY_CTRL_FLAGS(pc) |= PHY_F_FIBER_MODE;

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _bcm84756_init_stage_1
 * Purpose:
 *      PHY init.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84756_init_stage_1(phy_ctrl_t *pc)
{
    int rv = CDK_E_NONE;

    if (PHY_CTRL_FLAGS(pc) & PHY_F_FW_MDIO_LOAD) {
        PHY_VERB(pc, ("BCM84756 init stage 1, port:%d\n", pc->port));
        
        rv = _bcm84756_broadcast_set(pc, 1);
    }

    return rv;
}

/*
 * Function:
 *      _bcm84756_init_stage_2
 * Purpose:
 *      PHY init.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84756_init_stage_2(phy_ctrl_t *pc)
{
    int rv = CDK_E_NONE;

    if (PHY_CTRL_FLAGS(pc) & PHY_F_FW_MDIO_LOAD) {
        PHY_VERB(pc, ("BCM84756 init stage 2, port:%d\n", pc->port));
        
        /* This step is performed on the broadcast master only */
        if (PHY_CTRL_FLAGS(pc) & PHY_F_STAGED_INIT) {
            if (!(PHY_CTRL_FLAGS(pc) & PHY_F_BCAST_MSTR)) {
                return CDK_E_NONE;
            }
        }

        rv = _bcm84756_firmware_download_prep(pc);
    }

    return rv;
}

/*
 * Function:
 *      _bcm84756_init_stage_3
 * Purpose:
 *      PHY init.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84756_init_stage_3(phy_ctrl_t *pc)
{
    int rv = CDK_E_NONE;

    if (PHY_CTRL_FLAGS(pc) & PHY_F_FW_MDIO_LOAD) {
        PHY_VERB(pc, ("BCM84756 init stage 3, port:%d\n", pc->port));

        rv = _bcm84756_broadcast_set(pc, 1);
    }

    return rv;
}

/*
 * Function:
 *      _bcm84756_init_stage_4
 * Purpose:
 *      PHY init.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84756_init_stage_4(phy_ctrl_t *pc)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t data;
    uint8_t *fw_data;
    uint32_t fw_size;

    PHY_VERB(pc, ("BCM84756 init stage 4, port:%d\n", pc->port));

    if (PHY_CTRL_FLAGS(pc) & PHY_F_FW_MDIO_LOAD) {
        /* This step is performed on the broadcast master only */
        if (PHY_CTRL_FLAGS(pc) & PHY_F_STAGED_INIT) {
            if (!(PHY_CTRL_FLAGS(pc) & PHY_F_BCAST_MSTR)) {
                return CDK_E_NONE;
            }
        }

        ioerr += PHY_BUS_READ(pc, PMA_PMD_CHIP_VERSION_REG, &data);
        if (CHIP_VERSION_IS_A0(data)) {
            fw_data = bcm84756_ucode;
            fw_size = bcm84756_ucode_len;
        } else {
            fw_data = bcm84756_b0_ucode;
            fw_size = bcm84756_b0_ucode_len;
        }
        rv = _bcm84756_mdio_firmware_download(pc, fw_data, fw_size);
    } else {
        rv = _bcm84756_rom_firmware_download(pc);
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _bcm84756_init_stage_5
 * Purpose:
 *      PHY init.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84756_init_stage_5(phy_ctrl_t *pc)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;

    if (PHY_CTRL_FLAGS(pc) & PHY_F_FW_MDIO_LOAD) {
        PHY_VERB(pc, ("BCM84756 init stage 5, port:%d\n", pc->port));

        /* Disable broadcast mode */
        rv = _bcm84756_broadcast_set(pc, 0);
        
        if (CDK_SUCCESS(rv)) {
            rv = _bcm84756_mdio_firmware_done(pc);
        }
    } 

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _bcm84756_init_stage_6
 * Purpose:
 *      PHY init.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84756_init_stage_6(phy_ctrl_t *pc)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t data;
    
    PHY_VERB(pc, ("BCM84756 init stage 6, port:%d\n", pc->port));

    /* Enable Power - PMA/PMD, PCS, AN */
    ioerr += PHY_BUS_READ(pc, PMA_PMD_CTRL_REG, &data);
    data &= ~PMA_PMD_CTRL_PD;
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_CTRL_REG, data);
    
    ioerr += PHY_BUS_READ(pc, PCS_CTRL_REG, &data);
    data &= ~MII_CTRL_PD;
    ioerr += PHY_BUS_WRITE(pc, PCS_CTRL_REG, data);

    ioerr += PHY_BUS_READ(pc, AN_CTRL_REG, &data);
    data &= ~MII_CTRL_PD;
    ioerr += PHY_BUS_WRITE(pc, AN_CTRL_REG, data);

    /* Disable MACSEC */
    ioerr += PHY_BUS_READ(pc, PMA_PMD_BYPASS_MACSEC_REG, &data);
    data &= ~(PMA_PMD_MACSEC_BYPASS_MODE_MASK |
              PMA_PMD_MACSEC_ENABLE_PWRDN_MACSEC_MASK |
              PMA_PMD_MACSEC_UDSW_PWRDW_MACSEC_MASK |
              PMA_PMD_MACSEC_UDSW_RESET_MACSEC_MASK);
    data |= (PMA_PMD_MACSEC_BYPASS_MODE_MASK |
             PMA_PMD_MACSEC_ENABLE_PWRDN_MACSEC_MASK |
             PMA_PMD_MACSEC_UDSW_PWRDW_MACSEC_MASK);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_BYPASS_MACSEC_REG, data);
    
    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _bcm84756_init_stage
 * Purpose:
 *      Execute specified init stage.
 * Parameters:
 *      pc - PHY control structure
 *      stage - init stage
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84756_init_stage(phy_ctrl_t *pc, int stage)
{
    switch (stage) {
    case 0:
        return _bcm84756_init_stage_0(pc);
    case 1:
        return _bcm84756_init_stage_1(pc);
    case 2:
        return _bcm84756_init_stage_2(pc);
    case 3:
        return _bcm84756_init_stage_3(pc);
    case 4:
        return _bcm84756_init_stage_4(pc);
    case 5:
        return _bcm84756_init_stage_5(pc);
    case 6:
        return _bcm84756_init_stage_6(pc);
    default:
        break;
    }

    return CDK_E_UNAVAIL;
}

/***********************************************************************
 *
 * PHY DRIVER FUNCTIONS
 */

/*
 * Function:
 *      bcm84756_phy_probe
 * Purpose:     
 *      Probe for 84756 PHY
 * Parameters:
 *      pc - PHY control structure
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm84756_phy_probe(phy_ctrl_t *pc)
{
    uint32_t phyid0, phyid1;
    uint32_t chipid, chipid_lsb, chipid_msb;
    int ioerr = 0;

    PHY_CTRL_CHECK(pc);

    ioerr += PHY_BUS_READ(pc, PMA_PMD_ID0_REG, &phyid0);
    ioerr += PHY_BUS_READ(pc, PMA_PMD_ID1_REG, &phyid1);
    ioerr += PHY_BUS_READ(pc, PMA_PMD_CHIP_ID_LSB_REG, &chipid_lsb);
    ioerr += PHY_BUS_READ(pc, PMA_PMD_CHIP_ID_MSB_REG, &chipid_msb);
    if (ioerr) {
        return CDK_E_IO;
    }

    chipid = ((chipid_msb & 0xf) << 16) + chipid_lsb;

    if ((phyid0 == BCM84756_PMA_PMD_ID0) &&
        ((phyid1 & ~PHY_ID1_REV_MASK) == BCM84756_PMA_PMD_ID1)) {
        if (CHIP_IS_BCM84756_FAMILY(chipid)) {
#if PHY_CONFIG_EXTERNAL_BOOT_ROM == 0
            /* Use MDIO download if external ROM is disabled */
            PHY_CTRL_FLAGS(pc) |= PHY_F_FW_MDIO_LOAD;
#endif
            PHY_CTRL_FLAGS(pc) |= PHY_F_CLAUSE45;
            
            return CDK_E_NONE;
        }
    }

    return CDK_E_NOT_FOUND;
}

/*
 * Function:
 *      bcm84756_phy_notify
 * Purpose:     
 *      Handle PHY notifications
 * Parameters:
 *      pc - PHY control structure
 *      event - PHY event
 * Returns:
 *      CDK_E_xxx
 */
static int
bcm84756_phy_notify(phy_ctrl_t *pc, phy_event_t event)
{
    int rv = CDK_E_NONE;

    PHY_CTRL_CHECK(pc);

    switch (event) {
        case PhyEvent_ChangeToCopper:
            PHY_CTRL_FLAGS(pc) &= ~PHY_F_FIBER_MODE;
            event = PhyEvent_ChangeToPassthru;
            break;
        case PhyEvent_ChangeToFiber:
            PHY_CTRL_FLAGS(pc) |= PHY_F_FIBER_MODE;
            break;
        default:
            break;
    }

    /* Call up the PHY chain */
    if (CDK_SUCCESS(rv)) {
        if (PHY_CTRL_NEXT(pc)) {
            rv = PHY_NOTIFY(PHY_CTRL_NEXT(pc), event);
        }
    }

    return rv;
}

/*
 * Function:
 *      bcm84756_phy_reset
 * Purpose:     
 *      Reset 84756 PHY
 * Parameters:
 *      pc - PHY control structure
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm84756_phy_reset(phy_ctrl_t *pc)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;

    PHY_CTRL_CHECK(pc);

    rv = _bcm84756_soft_reset(pc);

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
 *      bcm84756_phy_init
 * Purpose:     
 *      Initialize 84756 PHY driver
 * Parameters:
 *      pc - PHY control structure
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm84756_phy_init(phy_ctrl_t *pc)
{
    int rv = CDK_E_NONE;
    int stage;

    PHY_CTRL_CHECK(pc);

    if (PHY_CTRL_FLAGS(pc) & PHY_F_STAGED_INIT) {
        PHY_CTRL_FLAGS(pc) &= ~PHY_F_STAGED_INIT;
    }

    for (stage = 0; CDK_SUCCESS(rv); stage++) {
        rv = _bcm84756_init_stage(pc, stage);
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
 *      bcm84756_phy_link_get
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
bcm84756_phy_link_get(phy_ctrl_t *pc, int *link, int *autoneg_done)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t an_mii_stat, pcs_mii_stat, pma_mii_stat, data;
    uint32_t line_mode, speed;
    int autoneg;

    PHY_CTRL_CHECK(pc);
    
    *link = 0;
    *autoneg_done = 0;
    autoneg = 0;
    
    /* Read AN MII STAT once per link get period */
    ioerr += PHY_BUS_READ(pc, AN_MII_STAT_REG, &an_mii_stat);
    if (an_mii_stat == 0xffff) {
        return CDK_E_NONE;
    }
    
    rv = PHY_AUTONEG_GET(pc, &autoneg);
    if (autoneg) {
        *autoneg_done = 0;
        if (an_mii_stat & MII_STAT_AN_DONE) {
            *autoneg_done = 1;
        }
    }

    if (autoneg && !autoneg_done) {
        /* Autoneg in progess, return no link */
        if (CDK_SUCCESS(rv)) {
            rv = PHY_SPEED_GET(pc, &speed);
        }    

        if(speed == 1000) {
            ioerr += PHY_BUS_READ(pc, AN_BASE1000X_CTRL2_REG, &data);
            if (data & AN_BASE1000X_CTRL2_PDET_MASK) {
                ioerr += PHY_BUS_READ(pc, AN_BASE1000X_STAT1_REG, &data);
                if (data & (1L << 1)) {
                    *link = 1;
                }
            }
        }
        return ioerr ? CDK_E_IO : CDK_E_NONE;
    }

    if (CDK_SUCCESS(rv)) {
        rv = _bcm84756_line_mode_get(pc, &line_mode);
    }
    if (line_mode == PHY_IF_SFI) {
        ioerr += PHY_BUS_READ(pc, PMA_PMD_STAT_REG, &pma_mii_stat);
        ioerr += PHY_BUS_READ(pc, PCS_STAT_REG, &pcs_mii_stat);
        if ((pma_mii_stat & MII_STAT_LA) && 
            (pcs_mii_stat & MII_STAT_LA)) {
            *link = 1;
        }
    } else {
        /* SGMII should have AN enabled to declare link up */
        if (line_mode == PHY_IF_SGMII && !autoneg) {
            *link = 0;
        } else if (!(an_mii_stat & MII_STAT_LA)) {
            *link = 0;
        } else {
            ioerr += PHY_BUS_READ(pc, PMA_PMD_LINK_DETECT_STAT_REG, &data);
            /* System side and line side rx signal detect OK */
            if ((data & PMA_PMD_SPEED_LD_STAT_SYS_RX_SD) &&
                (data & PMA_PMD_SPEED_LD_STAT_LINE_RX_SD)) {
                /* System side and line side link up */
                if ((data & PMA_PMD_SPEED_LD_STAT_SYS_PDM_SPEED_MASK) &&
                    (data & PMA_PMD_SPEED_LD_STAT_LINE_PDM_SPEED_MASK)) {
                    *link = 1;
                }
            }
        }
    }

    ioerr += PHY_BUS_READ(pc, PMA_PMD_DATAPATH_CDR_SELECT_REG, &data);
    if (PHY_CTRL_NEXT(pc)) {
        if ((data & PMA_PMD_DP_CDR_SELECT_TX_REPEATER_MASK) &&
            (data & PMA_PMD_DP_CDR_SELECT_RX_REPEATER_MASK)) {
            if (CDK_SUCCESS(rv)) {
                rv = PHY_LINK_GET(PHY_CTRL_NEXT(pc), link, autoneg_done);
            }
        }
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

/*
 * Function:    
 *      bcm84756_phy_duplex_set
 * Purpose:     
 *      Set the current duplex mode (forced).
 * Parameters:
 *      pc - PHY control structure
 *      duplex - non-zero indicates full duplex, zero indicates half
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm84756_phy_duplex_set(phy_ctrl_t *pc, int duplex)
{
    int ioerr = 0;
    uint32_t data;

    PHY_CTRL_CHECK(pc);

    if (PHY_FIBER_MODE(pc)) {
        if (duplex == 1) {
            return CDK_E_NONE;
        } else {
            return CDK_E_UNAVAIL;
        }
    } else {
        ioerr += PHY_BUS_READ(pc, AN_MII_CTRL_REG, &data);
        data &= ~MII_CTRL_FD;
        if (duplex == 1) {
            data |= MII_CTRL_FD;
        }
        ioerr += PHY_BUS_WRITE(pc, AN_MII_CTRL_REG, data);
    }
    
    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

/*
 * Function:    
 *      bcm84756_phy_duplex_get
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
bcm84756_phy_duplex_get(phy_ctrl_t *pc, int *duplex)
{
    int ioerr = 0;
    uint32_t data;

    PHY_CTRL_CHECK(pc);

    if (!duplex) {
        return CDK_E_PARAM;
    }
    *duplex = 0;
    
    if (PHY_FIBER_MODE(pc)) {
        *duplex = 1;
    } else {
        ioerr += PHY_BUS_READ(pc, AN_BASE1000X_STAT1_REG, &data);
        if (data & (1L << 2)) {
            *duplex = 1;
        }
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

/*
 * Function:    
 *      bcm84756_phy_speed_set
 * Purpose:     
 *      Set the current operating speed (forced).
 * Parameters:
 *      pc - PHY control structure
 *      speed - new link speed
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm84756_phy_speed_set(phy_ctrl_t *pc, uint32_t speed)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t speed_val;
    uint32_t line_intf, sys_intf;
    uint32_t data;
    int lb, an;

    PHY_CTRL_CHECK(pc);

    rv = PHY_AUTONEG_GET(pc, &an);
    if (CDK_FAILURE(rv) || an) {
        return rv;
    }

    rv = PHY_LOOPBACK_GET(pc, &lb);
    if (CDK_SUCCESS(rv) && lb) {
        rv = PHY_LOOPBACK_SET(pc, 0);
    }

    if (PHY_FIBER_MODE(pc)) {
        switch (speed) {
            case 10000:
                line_intf = PHY_IF_SFI;
                sys_intf = PHY_IF_XFI;
                speed_val = PMA_PMD_CTRL_SS_10000;
                break;
            case 1000:
                line_intf = PHY_IF_TBI;
                sys_intf = PHY_IF_TBI;
                speed_val = PMA_PMD_CTRL_SS_1000;
                break;
            default:
                return CDK_E_PARAM;
        }
        
        /* Disable Clause 37 Autoneg */
        ioerr += PHY_BUS_READ(pc, AN_MII_CTRL_REG, &data);
        data &= ~(MII_CTRL_SS_LSB | MII_CTRL_SS_MSB | MII_CTRL_AE);
        ioerr += PHY_BUS_WRITE(pc, AN_MII_CTRL_REG, data);
        
        ioerr += PHY_BUS_READ(pc, AN_BASE1000X_CTRL1_REG, &data);
        data |= AN_BASE1000X_CTRL1_FIBER_MODE;
        ioerr += PHY_BUS_WRITE(pc, AN_BASE1000X_CTRL1_REG, data);

        ioerr += PHY_BUS_READ(pc, AN_MISC2_REG, &data);
        data |= 0x2020;
        ioerr += PHY_BUS_WRITE(pc, AN_MISC2_REG, data);

        if (CDK_SUCCESS(rv)) {
            /* Set the register side to system side */
            rv = _bcm84756_side_regs_select(pc, PHY_REGS_SIDE_SYSTEM);
        }

        /* Disable Clause 37 Autoneg */
        ioerr += PHY_BUS_READ(pc, AN_MII_CTRL_REG, &data);
        data &= ~(MII_CTRL_SS_LSB | MII_CTRL_SS_MSB | MII_CTRL_AE);
        ioerr += PHY_BUS_WRITE(pc, AN_MII_CTRL_REG, data);
        
        ioerr += PHY_BUS_READ(pc, AN_BASE1000X_CTRL1_REG, &data);
        data |= AN_BASE1000X_CTRL1_FIBER_MODE;
        ioerr += PHY_BUS_WRITE(pc, AN_BASE1000X_CTRL1_REG, data);

        /* Set back the register side to line side */
        _bcm84756_side_regs_select(pc, PHY_REGS_SIDE_LINE);
        
        /* Set Line mode */
        if (CDK_SUCCESS(rv)) {
            rv = _bcm84756_line_mode_set(pc, line_intf);
        }

        /* Set System side to match Line side */
        if (CDK_SUCCESS(rv)) {
            rv = _bcm84756_sys_mode_set(pc, sys_intf);
        }

        /* Force Speed in PMD */
        ioerr += PHY_BUS_READ(pc, PMA_PMD_CTRL_REG, &data);
        data &= ~(PMA_PMD_CTRL_10GSS_MASK);
        data |= speed_val;
        ioerr += PHY_BUS_WRITE(pc, PMA_PMD_CTRL_REG, data);
        
        /* Select 10G-LRM PMA */
        ioerr += PHY_BUS_READ(pc, PMA_PMD_CTRL2_REG, &data);
        data &= ~(PMA_PMD_TYPE_SELECT_MASK);
        data |= PMA_PMD_CTRL2_10GLRM_PMD_TYPE;
        ioerr += PHY_BUS_WRITE(pc, PMA_PMD_CTRL2_REG, data);
        
        if (CDK_SUCCESS(rv)) {
            /* Set the register side to system side */
            rv = _bcm84756_side_regs_select(pc, PHY_REGS_SIDE_SYSTEM);
        }
        
        /* Force Speed in PMD */
        ioerr += PHY_BUS_READ(pc, PMA_PMD_CTRL_REG, &data);
        data &= ~(PMA_PMD_CTRL_10GSS_MASK);
        data |= speed_val;
        ioerr += PHY_BUS_WRITE(pc, PMA_PMD_CTRL_REG, data);
        
        /* Select 10G-LRM PMA */
        ioerr += PHY_BUS_READ(pc, PMA_PMD_CTRL2_REG, &data);
        data &= ~(PMA_PMD_TYPE_SELECT_MASK);
        data |= PMA_PMD_CTRL2_10GLRM_PMD_TYPE;
        ioerr += PHY_BUS_WRITE(pc, PMA_PMD_CTRL2_REG, data);

        /* Set back the register side to line side */
        _bcm84756_side_regs_select(pc, PHY_REGS_SIDE_LINE);
    } else {
        switch (speed) {
            case 1000:
                speed_val = PMA_PMD_CTRL_SS_1000;
                break;
            case 100:
                speed_val = PMA_PMD_CTRL_SS_100;
                break;
            case 10:
                speed_val = PMA_PMD_CTRL_SS_10;
                break;
            default:
                return CDK_E_PARAM;
        }
        
        line_intf = PHY_IF_SGMII;
        sys_intf = PHY_IF_SGMII;
        
        /* Set Line mode */
        if (CDK_SUCCESS(rv)) {
            rv = _bcm84756_line_mode_set(pc, line_intf);
        }

        /* Set system side to match Line side */
        if (CDK_SUCCESS(rv)) {
            rv = _bcm84756_sys_mode_set(pc, sys_intf);
        }

        /* Disable Clause 37 Autoneg */
        ioerr += PHY_BUS_READ(pc, AN_MII_CTRL_REG, &data);
        data &= ~(MII_CTRL_SS_LSB | MII_CTRL_SS_MSB | MII_CTRL_AE);
        ioerr += PHY_BUS_WRITE(pc, AN_MII_CTRL_REG, data);
        
        ioerr += PHY_BUS_READ(pc, AN_BASE1000X_CTRL1_REG, &data);
        data &= ~AN_BASE1000X_CTRL1_FIBER_MODE;
        ioerr += PHY_BUS_WRITE(pc, AN_BASE1000X_CTRL1_REG, data);

        ioerr += PHY_BUS_READ(pc, AN_MISC2_REG, &data);
        data |= 0x2020;
        ioerr += PHY_BUS_WRITE(pc, AN_MISC2_REG, data);

        /* Force Speed in PMD */
        ioerr += PHY_BUS_READ(pc, PMA_PMD_CTRL_REG, &data);
        data &= ~(PMA_PMD_CTRL_10GSS_MASK);
        data |= speed_val;
        ioerr += PHY_BUS_WRITE(pc, PMA_PMD_CTRL_REG, data);

        if (CDK_SUCCESS(rv)) {
            /* Set the register side to system side */
            rv = _bcm84756_side_regs_select(pc, PHY_REGS_SIDE_SYSTEM);
        }

        /* Disable Clause 37 Autoneg */
        ioerr += PHY_BUS_READ(pc, AN_MII_CTRL_REG, &data);
        data &= ~(MII_CTRL_SS_LSB | MII_CTRL_SS_MSB | MII_CTRL_AE);
        ioerr += PHY_BUS_WRITE(pc, AN_MII_CTRL_REG, data);
        
        ioerr += PHY_BUS_READ(pc, AN_BASE1000X_CTRL1_REG, &data);
        data &= ~AN_BASE1000X_CTRL1_FIBER_MODE;
        ioerr += PHY_BUS_WRITE(pc, AN_BASE1000X_CTRL1_REG, data);

        ioerr += PHY_BUS_READ(pc, AN_MISC2_REG, &data);
        data |= 0x2020;
        ioerr += PHY_BUS_WRITE(pc, AN_MISC2_REG, data);

        /* Force Speed in PMD */
        ioerr += PHY_BUS_READ(pc, PMA_PMD_CTRL_REG, &data);
        data &= ~(PMA_PMD_CTRL_10GSS_MASK);
        data |= speed_val;
        ioerr += PHY_BUS_WRITE(pc, PMA_PMD_CTRL_REG, data);

        /* Set back the register side to line side */
        _bcm84756_side_regs_select(pc, PHY_REGS_SIDE_LINE);
    }

    if (PHY_CTRL_NEXT(pc)) {
        if(CDK_SUCCESS(rv)) {
            rv = PHY_AUTONEG_SET(PHY_CTRL_NEXT(pc), 0);
        }
        if(CDK_SUCCESS(rv)) {
            PHY_CTRL_LINE_INTF(PHY_CTRL_NEXT(pc)) = sys_intf;
            rv = PHY_SPEED_SET(PHY_CTRL_NEXT(pc), speed);
        }
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
 *      bcm84756_phy_speed_get
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
bcm84756_phy_speed_get(phy_ctrl_t *pc, uint32_t *speed)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t data;
    uint32_t line_mode;

    PHY_CTRL_CHECK(pc);

    if (!speed) {
        return CDK_E_PARAM;
    }
    *speed = 0;

    rv = _bcm84756_line_mode_get(pc, &line_mode);
    if (line_mode == PHY_IF_SFI) {
        *speed = 10000;
    } else if (line_mode == PHY_IF_TBI) {
        *speed = 1000;
    } else { /* PHY_IF_SGMII */
        ioerr += PHY_BUS_READ(pc, PMA_PMD_LINK_DETECT_STAT_REG, &data);
        switch(data & PMA_PMD_SPEED_LD_STAT_LINE_PDM_SPEED_MASK) {
            case PMA_PMD_SPEED_LD_STAT_LINE_PDM_SPEED_1G:
                *speed = 1000;
                break;
            case PMA_PMD_SPEED_LD_STAT_LINE_PDM_SPEED_100M:
                *speed = 100;
                break;
            case PMA_PMD_SPEED_LD_STAT_LINE_PDM_SPEED_10M:
                *speed = 10;
                break;
        }
    }

    ioerr += PHY_BUS_READ(pc, PMA_PMD_DATAPATH_CDR_SELECT_REG, &data);
    if (PHY_CTRL_NEXT(pc)) {
        if ((data & PMA_PMD_DP_CDR_SELECT_TX_REPEATER_MASK) &&
            (data & PMA_PMD_DP_CDR_SELECT_RX_REPEATER_MASK)) {
            if (CDK_SUCCESS(rv)) {
                rv = PHY_SPEED_GET(PHY_CTRL_NEXT(pc), speed);
            }
        }
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:    
 *      bcm84756_phy_autoneg_set
 * Purpose:     
 *      Enable or disable auto-negotiation on the specified port.
 * Parameters:
 *      pc - PHY control structure
 *      autoneg - non-zero enables autoneg, zero disables autoneg
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm84756_phy_autoneg_set(phy_ctrl_t *pc, int autoneg)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t data;
    uint32_t line_mode;

    PHY_CTRL_CHECK(pc);

    rv = _bcm84756_line_mode_get(pc, &line_mode);
    if (line_mode == PHY_IF_SFI) {
        autoneg = 0;
    }

    if (line_mode != PHY_IF_SFI) {
        ioerr += PHY_BUS_READ(pc, PMA_PMD_CTRL_REG, &data);
        data &= ~(PMA_PMD_CTRL_SS_SPEED_SEL);
        data |= PMA_PMD_CTRL_SS_MSB;
        if (autoneg) {
            data |= PMA_PMD_CTRL_SS_LSB;
        }
        ioerr += PHY_BUS_WRITE(pc, PMA_PMD_CTRL_REG, data);
    }

    ioerr += PHY_BUS_READ(pc, AN_MISC2_REG, &data);
    data &= ~(0x0030);
    data |= 0x0010;
    if (!autoneg) {
        data |= 0x0020;
    }
    ioerr += PHY_BUS_WRITE(pc, AN_MISC2_REG, data);

    ioerr += PHY_BUS_READ(pc, AN_MII_CTRL_REG, &data);
    data &= ~(MII_CTRL_AE | MII_CTRL_FD | MII_CTRL_RAN);
    data |= MII_CTRL_FD;
    if (autoneg) {
        data |= (MII_CTRL_AE | MII_CTRL_RAN);
    }
    ioerr += PHY_BUS_WRITE(pc, AN_MII_CTRL_REG, data);

    ioerr += PHY_BUS_READ(pc, PMA_PMD_DATAPATH_CDR_SELECT_REG, &data);
    if (PHY_CTRL_NEXT(pc)) {
        if ((data & PMA_PMD_DP_CDR_SELECT_TX_REPEATER_MASK) &&
            (data & PMA_PMD_DP_CDR_SELECT_RX_REPEATER_MASK)) {
            if (CDK_SUCCESS(rv)) {
                rv = PHY_AUTONEG_SET(PHY_CTRL_NEXT(pc), autoneg);
            }
        } else {
            if (CDK_SUCCESS(rv)) {
                rv = PHY_AUTONEG_SET(PHY_CTRL_NEXT(pc), FALSE);
            }
        }
    }

    if(autoneg == 0) {
        ioerr += PHY_BUS_READ(pc, PMA_PMD_PMD_TX_DISABLE_REG, &data);
        data |= PMA_PMD_TX_DISABLE_GLOBAL_DISABLE;
        ioerr += PHY_BUS_WRITE(pc, PMA_PMD_PMD_TX_DISABLE_REG, data);
        
        PHY_SYS_USLEEP(50000);

        ioerr += PHY_BUS_READ(pc, PMA_PMD_PMD_TX_DISABLE_REG, &data);
        data &= ~PMA_PMD_TX_DISABLE_GLOBAL_DISABLE;
        ioerr += PHY_BUS_WRITE(pc, PMA_PMD_PMD_TX_DISABLE_REG, data);
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:    
 *      bcm84756_phy_autoneg_get
 * Purpose:     
 *      Get the current auto-negotiation setting.
 * Parameters:
 *      pc - PHY control structure
 *      autoneg - (OUT) non-zero indicates autoneg enabled
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm84756_phy_autoneg_get(phy_ctrl_t *pc, int *autoneg)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t data;
    uint32_t line_mode;

    PHY_CTRL_CHECK(pc);

    if (!autoneg) {
        return CDK_E_PARAM;
    }
    *autoneg = 0;

    rv = _bcm84756_line_mode_get(pc, &line_mode);
    if (line_mode != PHY_IF_SFI) {
        ioerr += PHY_BUS_READ(pc, AN_MII_CTRL_REG, &data);
        if (data & MII_CTRL_AE) {
            *autoneg = 1;
        }
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:    
 *      bcm84756_phy_loopback_set
 * Purpose:     
 *      Set the internal PHY loopback mode.
 * Parameters:
 *      pc - PHY control structure
 *      enable - non-zero enables PHY loopback
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm84756_phy_loopback_set(phy_ctrl_t *pc, int enable)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t data;
    uint32_t line_mode;

    PHY_CTRL_CHECK(pc);

    rv = _bcm84756_line_mode_get(pc, &line_mode);
    if (line_mode == PHY_IF_SFI) {
        ioerr += PHY_BUS_READ(pc, PMA_PMD_CTRL_REG, &data);
        data &= ~PMA_PMD_CTRL_PMS_LOOPBACK;
        if (enable) {
            data |= PMA_PMD_CTRL_PMS_LOOPBACK;
        }
        ioerr += PHY_BUS_WRITE(pc, PMA_PMD_CTRL_REG, data);
    } else {
        ioerr += PHY_BUS_READ(pc, AN_MII_CTRL_REG, &data);
        data &= ~MII_CTRL_LE;
        if (enable) {
            data |= MII_CTRL_LE;
        }
        ioerr += PHY_BUS_WRITE(pc, AN_MII_CTRL_REG, data);
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:    
 *      bcm84756_phy_loopback_get
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
bcm84756_phy_loopback_get(phy_ctrl_t *pc, int *enable)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t data;
    uint32_t line_mode;

    PHY_CTRL_CHECK(pc);

    if (!enable) {
        return CDK_E_PARAM;
    }
    *enable = 0;

    rv = _bcm84756_line_mode_get(pc, &line_mode);
    if (line_mode == PHY_IF_SFI) {
        ioerr += PHY_BUS_READ(pc, PMA_PMD_CTRL_REG, &data);
        if ((data != 0xffff) && (data & PMA_PMD_CTRL_PMS_LOOPBACK)) {
            *enable = 1;
        }
    } else {
        ioerr += PHY_BUS_READ(pc, AN_MII_CTRL_REG, &data);
        if ((data != 0xffff) && (data & MII_CTRL_LE)) {
            *enable = 1;
        }
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:    
 *      bcm84756_phy_ability_get
 * Purpose:     
 *      Get the abilities of the PHY.
 * Parameters:
 *      pc - PHY control structure
 *      abil - (OUT) ability mask indicating supported options/speeds.
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm84756_phy_ability_get(phy_ctrl_t *pc, uint32_t *abil)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t data;
    
    PHY_CTRL_CHECK(pc);

    if (!abil) {
        return CDK_E_PARAM;
    }
    *abil = 0;

    if (PHY_COPPER_MODE(pc)) {
        *abil |= (PHY_ABIL_AN | PHY_ABIL_10MB | PHY_ABIL_100MB | 
                  PHY_ABIL_1000MB);
    } else {
        *abil |= (PHY_ABIL_XGMII | PHY_ABIL_1000MB_FD | PHY_ABIL_10GB);
    }
    
    *abil = (PHY_ABIL_LOOPBACK | PHY_ABIL_PAUSE | PHY_ABIL_SGMII);

    ioerr += PHY_BUS_READ(pc, PMA_PMD_DATAPATH_CDR_SELECT_REG, &data);
    if (PHY_CTRL_NEXT(pc)) {
        if ((data & PMA_PMD_DP_CDR_SELECT_TX_REPEATER_MASK) &&
            (data & PMA_PMD_DP_CDR_SELECT_RX_REPEATER_MASK)) {
            if (CDK_SUCCESS(rv)) {
                rv = PHY_ABILITY_GET(PHY_CTRL_NEXT(pc), abil);
            }
        }
    }

    return rv;
}

/*
 * Function:
 *      bcm84756_phy_config_set
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
bcm84756_phy_config_set(phy_ctrl_t *pc, phy_config_t cfg, uint32_t val, void *cd)
{
    PHY_CTRL_CHECK(pc);

    switch (cfg) {
    case PhyConfig_Enable:
        return CDK_E_NONE;
    case PhyConfig_InitStage:
        if (PHY_CTRL_FLAGS(pc) & PHY_F_STAGED_INIT) {
            return _bcm84756_init_stage(pc, val);
        }
        break;
    default:
        break;
    }

    return CDK_E_UNAVAIL;
}

/*
 * Function:
 *      bcm84756_phy_config_get
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
bcm84756_phy_config_get(phy_ctrl_t *pc, phy_config_t cfg, uint32_t *val, void *cd)
{
    int rv = CDK_E_NONE;

    PHY_CTRL_CHECK(pc);

    switch (cfg) {
    case PhyConfig_Enable:
        *val = 1;
        return CDK_E_NONE;
    case PhyConfig_PortInterface:
        rv = _bcm84756_line_mode_get(pc, val);
        return rv;
    case PhyConfig_Clause45Devs:
        *val = 0x8a;
        return CDK_E_NONE;
    case PhyConfig_BcastAddr:
        *val = PHY_CTRL_BUS_ADDR(pc) & ~0x1f;
        return CDK_E_NONE;
    case PhyConfig_AdvLocal:
        rv = _bcm84756_ability_local_get(pc, val);
        break;
    case PhyConfig_AdvRemote:
        rv = _bcm84756_ability_remote_get(pc, val);
        break;
    default:
        break;
    }

    return CDK_E_UNAVAIL;
}

/*
 * Variable:    bcm84756_drv
 * Purpose:     PHY Driver for BCM84756.
 */
phy_driver_t bcm84756_drv = {
    "bcm84756",
    "BCM84756 10-Gigabit PHY Driver",  
    0,
    bcm84756_phy_probe,                  /* pd_probe */
    bcm84756_phy_notify,                 /* pd_notify */
    bcm84756_phy_reset,                  /* pd_reset */
    bcm84756_phy_init,                   /* pd_init */
    bcm84756_phy_link_get,               /* pd_link_get */
    bcm84756_phy_duplex_set,             /* pd_duplex_set */
    bcm84756_phy_duplex_get,             /* pd_duplex_get */
    bcm84756_phy_speed_set,              /* pd_speed_set */
    bcm84756_phy_speed_get,              /* pd_speed_get */
    bcm84756_phy_autoneg_set,            /* pd_autoneg_set */
    bcm84756_phy_autoneg_get,            /* pd_autoneg_get */
    bcm84756_phy_loopback_set,           /* pd_loopback_set */
    bcm84756_phy_loopback_get,           /* pd_loopback_get */
    bcm84756_phy_ability_get,            /* pd_ability_get */
    bcm84756_phy_config_set,             /* pd_config_set */
    bcm84756_phy_config_get,             /* pd_config_get */
    NULL,                                /* pd_status_get */
    NULL                                 /* pd_cable_diag */
};
