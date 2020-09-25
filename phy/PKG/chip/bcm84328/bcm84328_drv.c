/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * PHY driver for BCM84328.
 */

#include <phy/phy.h>
#include <phy/ge_phy.h>

extern unsigned char bcm84328_ucode[];
extern unsigned int bcm84328_ucode_len;

#define PHY_REGS_SIDE_SYSTEM                    0
#define PHY_REGS_SIDE_LINE                      1

#define PHY_DATAPATH_20_BIT                     0
#define PHY_DATAPATH_4_BIT                      1

#define NUM_PHY_LANES                           4
#define PHY_ALL_LANES                           4

#define PHY_POLL_MAX                            100
#define PHY_RESET_POLL_MAX                      10

#define BCM84328_PMA_PMD_ID0                    0x600d
#define BCM84328_PMA_PMD_ID1                    0x8500

#define PHY_ID1_REV_MASK                        0x000f

/* Firmware checksum for MDIO download */
#define BCM84328_MDIO_FW_CHKSUM                 0x600d

#define C45_DEVAD(_a)                           LSHIFT32((_a),16)
#define DEVAD_PMA_PMD                           C45_DEVAD(MII_C45_DEV_PMA_PMD)
#define DEVAD_AN                                C45_DEVAD(MII_C45_DEV_AN)

/* PMA/PMD registers */
#define PMA_PMD_CTRL_REG                        (DEVAD_PMA_PMD + MII_CTRL_REG)
#define PMA_PMD_STAT_REG                        (DEVAD_PMA_PMD + MII_STAT_REG)
#define PMA_PMD_ID0_REG                         (DEVAD_PMA_PMD + MII_PHY_ID0_REG)
#define PMA_PMD_ID1_REG                         (DEVAD_PMA_PMD + MII_PHY_ID1_REG)
#define PMA_PMD_R_PMD_STATUS_REG                (DEVAD_PMA_PMD + 0x0097)
#define PMA_PMD_TX_ANA_CTRL1_REG                (DEVAD_PMA_PMD + 0xc061)
#define PMA_PMD_TX_ANA_CTRL4_REG                (DEVAD_PMA_PMD + 0xc068)
#define PMA_PMD_TX_ANA_CTRL6_REG                (DEVAD_PMA_PMD + 0xc06a)
#define PMA_PMD_ANA_RX_STATUS_REG               (DEVAD_PMA_PMD + 0xc0b0)
#define PMA_PMD_ANA_RX_CTRL_REG                 (DEVAD_PMA_PMD + 0xc0b1)
#define PMA_PMD_GP_RX_INV_REG                   (DEVAD_PMA_PMD + 0xc0ba)
#define PMA_PMD_UC_VERSION_REG                  (DEVAD_PMA_PMD + 0xc1f0)
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
#define PMA_PMD_SYSTEM_SIDE_REG                 (DEVAD_PMA_PMD + 0xffff)
#define PMA_PMD_C017_REG                        (DEVAD_PMA_PMD + 0xc017)
#define PMA_PMD_CA7B_REG                        (DEVAD_PMA_PMD + 0xca7b)
#define PMA_PMD_C1FC_REG                        (DEVAD_PMA_PMD + 0xc1fc)

/* AN registers */                              
#define AN_CTRL_REG                             (DEVAD_AN + MII_CTRL_REG)
#define AN_STAT_REG                             (DEVAD_AN + MII_STAT_REG)

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

/* Select system-side register */
#define PMA_PMD_SYSTEM_SIDE_SELECT              (1L << 0)

/* ANA rx control register */
#define PMA_PMD_ANA_RX_STATUS_SEL               (7L << 0)

/* ANA rx status register */
#define PMA_PMD_ANA_RX_STATUS_SIGDET            (1L << 15)
#define PMA_PMD_ANA_RX_STATUS_CDR               (1L << 12)

/* Single PMD control register */
#define PMA_PMD_PMD_CTRL_LANE_SELECTION         (1L << 8)
#define PMA_PMD_PMD_CTRL_SINGLE_PMD_MODE        (1L << 7)
#define PMA_PMD_PMD_CTRL_ACCESS_MASK            0x30
#define PMA_PMD_PMD_CTRL_ACCESS_SHIFT           4
#define PMA_PMD_PMD_CTRL_DISABLE_MASK           0xf

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
#define PMA_PMD_GEN_4BIT_DATAPATH_MODE          (1L << 6)
#define PMA_PMD_GEN_4BIT_FIFO_DEPTH             (1L << 5)
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

/* Rx polarity inversion */
#define PMA_PMD_RX_INV_FORCE                    (1L << 3)
#define PMA_PMD_RX_INV_INVERT                   (1L << 2)

/* SPI port control/status register, 0xC848 */
#define PMA_PMD_SPA_CTRL_SPI_PORT_USED          (1L << 15)
#define PMA_PMD_SPA_CTRL_SPI_BOOT               (1L << 14)
#define PMA_PMD_SPA_CTRL_SPI_DWLD_DONE          (1L << 13)

/* Regulator control register, 0xC850 */
#define PMA_PMD_REGUALTOR_CTRL_PWRDN            (1L << 13)

/* MISC control register, 0xCA85 */
#define PMA_PMD_MISC_CTRL_32K                   (1L << 3)

/* Select broadcast mechanism for MDIO ADDR/Write Operations, 0xC8FE */
#define PMA_PMD_BROADCAT_MODE_ENABLE            (1L << 0)

/* GEN_CTRL register, 0xCA10 */
#define PMA_PMD_GEN_CTRL_URCST                  (1L << 2)

/* Auto-negotiation register definitions */
#define AN_EXT_NXT_PAGE                         (1L << 13)
#define AN_ENABLE                               (1L << 12)
#define AN_RESTART                              (1L << 9)

#define BCM84328_SINGLE_PORT_MODE(_pc) \
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
 *      _bcm84328_side_regs_select
 * Purpose:
 *      Select side of the PHY register.
 * Parameters:
 *      pc - PHY control structure
 *      side - System side or line side
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84328_side_regs_select(phy_ctrl_t *pc, int side)
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
 *      _bcm84328_channel_select
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
_bcm84328_channel_select(phy_ctrl_t *pc, int lane, int side)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t data;

    if (!(BCM84328_SINGLE_PORT_MODE(pc))) {
        return CDK_E_PARAM;
    }

    if ((lane < 0 || lane >= NUM_PHY_LANES) && (lane != PHY_ALL_LANES)) {
        return CDK_E_PARAM;
    }

    rv = _bcm84328_side_regs_select(pc, PHY_REGS_SIDE_LINE);
    if (CDK_FAILURE(rv)) {
        return rv;
    }
    
    ioerr += PHY_BUS_READ(pc, PMA_PMD_SINGLE_PMD_CTRL_REG, &data);
    data &= ~(PMA_PMD_PMD_CTRL_LANE_SELECTION | PMA_PMD_PMD_CTRL_ACCESS_MASK | 
                                                PMA_PMD_PMD_CTRL_DISABLE_MASK);
    if (lane == PHY_ALL_LANES) {
        data |= PMA_PMD_PMD_CTRL_SINGLE_PMD_MODE;
        data |= PMA_PMD_PMD_CTRL_DISABLE_MASK;
    } else {
        data |= PMA_PMD_PMD_CTRL_LANE_SELECTION;
        data |= (lane << PMA_PMD_PMD_CTRL_ACCESS_SHIFT);
        data |= (1 << lane) & PMA_PMD_PMD_CTRL_DISABLE_MASK;
    }
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_SINGLE_PMD_CTRL_REG, data);
    
    if (side == PHY_REGS_SIDE_SYSTEM) {
        rv = _bcm84328_side_regs_select(pc, PHY_REGS_SIDE_SYSTEM);
        if (CDK_FAILURE(rv)) {
            return CDK_E_FAIL;
        }
    }
    
    return ioerr ? CDK_E_IO : CDK_E_NONE;
}


/*
 * Function:
 *      _bcm84328_interface_update
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
_bcm84328_interface_update(phy_ctrl_t *pc, uint32_t data, uint32_t mask)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t gen1, gen3;
    int cnt;

    rv = _bcm84328_side_regs_select(pc, PHY_REGS_SIDE_LINE);
    if (CDK_FAILURE(rv)) {
        return rv;
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
            PHY_SYS_USLEEP(10); 
        }
        if (cnt == PHY_POLL_MAX) {
            PHY_WARN(pc, ("Polling timeout\n"));
            return CDK_E_TIMEOUT;
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
        PHY_SYS_USLEEP(10); 
    }
    if (cnt == PHY_POLL_MAX) {
        PHY_WARN(pc, ("Polling timeout\n"));
        return CDK_E_TIMEOUT;
    }
    
    /* Cmd active and ucode acked - let ucode know we saw ack */
    ioerr += PHY_BUS_READ(pc, PMA_PMD_GEN_1_REG, &gen1);
    gen1 &= ~PMA_PMD_GEN_FINISH_CHANGE;
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_GEN_1_REG, gen1);
    
    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

/*
 * Function:
 *      _bcm84328_datapath_mode_set
 * Purpose:
 *      Select side of the PHY register.
 * Parameters:
 *      pc - PHY control structure
 *      mode - PHY datapath mode
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84328_datapath_mode_set(phy_ctrl_t *pc, int mode)
{
    uint32_t data, mask;

    data = 0;
    mask = (PMA_PMD_GEN_4BIT_FIFO_DEPTH | PMA_PMD_GEN_4BIT_DATAPATH_MODE);
    if (mode == PHY_DATAPATH_4_BIT) { 
        data = PMA_PMD_GEN_4BIT_DATAPATH_MODE;
    }
    
    return _bcm84328_interface_update(pc, data, mask);
}

/*
 * Function:
 *      _bcm84328_polarity_rx_invert
 * Purpose:
 *      Flip configured rx polarities
 * Parameters:
 *      pc - PHY control structure
 *      value - 
 *      datapath - PHY datapath mode
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84328_polarity_rx_invert(phy_ctrl_t *pc, uint32_t value, int datapath)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t data;
    int lane, flip;

    if (BCM84328_SINGLE_PORT_MODE(pc)) {
        for (lane = 0; lane < NUM_PHY_LANES; lane++) {
            flip = 0;
            if (value & (0xf << (lane * 4))) {
                flip = 1;
            }
        
            if (datapath == PHY_DATAPATH_20_BIT) {
                rv = _bcm84328_channel_select(pc, lane, PHY_REGS_SIDE_LINE);
                if (CDK_FAILURE(rv)) {
                    return CDK_E_FAIL;
                }
                
                ioerr += PHY_BUS_READ(pc, PMA_PMD_GP_RX_INV_REG, &data);
                data &= ~(PMA_PMD_RX_INV_FORCE | PMA_PMD_RX_INV_INVERT) ;
                if (!flip) {
                    data |= (PMA_PMD_RX_INV_FORCE | PMA_PMD_RX_INV_INVERT);
                }
                ioerr += PHY_BUS_WRITE(pc, PMA_PMD_GP_RX_INV_REG, data);
            } else {
                rv = _bcm84328_channel_select(pc, lane, PHY_REGS_SIDE_SYSTEM);
                if (CDK_FAILURE(rv)) {
                    return CDK_E_FAIL;
                }

                ioerr += PHY_BUS_READ(pc, PMA_PMD_TX_ANA_CTRL4_REG, &data);
                data &= ~PMA_PMD_TX_ANA_CTRL4_TXPOL_FLIP;
                if (!flip) {
                    data |= PMA_PMD_TX_ANA_CTRL4_TXPOL_FLIP;
                }
                ioerr += PHY_BUS_WRITE(pc, PMA_PMD_TX_ANA_CTRL4_REG, data);
            }
        }
        /* Restore to default single port register access */
        rv = _bcm84328_channel_select(pc, PHY_ALL_LANES, PHY_REGS_SIDE_LINE);
        if (CDK_FAILURE(rv)) {
            return CDK_E_FAIL;
        }
    } else {
        flip = 0;
        if (value & (0xf << (pc->port & 0x3))) {
            flip = 1;
        }
        if (datapath == PHY_DATAPATH_20_BIT) {
            ioerr += PHY_BUS_READ(pc, PMA_PMD_GP_RX_INV_REG, &data);
            data &= ~(PMA_PMD_RX_INV_FORCE | PMA_PMD_RX_INV_INVERT) ;
            if (flip) {
                data |= (PMA_PMD_RX_INV_FORCE | PMA_PMD_RX_INV_INVERT);
            }
            ioerr += PHY_BUS_WRITE(pc, PMA_PMD_GP_RX_INV_REG, data);
        } else {
            rv = _bcm84328_side_regs_select(pc, PHY_REGS_SIDE_SYSTEM);
            if (CDK_FAILURE(rv)) {
                return CDK_E_FAIL;
            }

            ioerr += PHY_BUS_READ(pc, PMA_PMD_TX_ANA_CTRL4_REG, &data);
            data &= ~PMA_PMD_TX_ANA_CTRL4_TXPOL_FLIP;
            if (!flip) {
                data |= PMA_PMD_TX_ANA_CTRL4_TXPOL_FLIP;
            }
            ioerr += PHY_BUS_WRITE(pc, PMA_PMD_TX_ANA_CTRL4_REG, data);

            rv = _bcm84328_side_regs_select(pc, PHY_REGS_SIDE_LINE);
            if (CDK_FAILURE(rv)) {
                return CDK_E_FAIL;
            }
        }
    }
    
    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

/*
 * Function:
 *      _bcm84328_polarity_tx_invert
 * Purpose:
 *      Flip configured tx polarities
 * Parameters:
 *      pc - PHY control structure
 *      value - 
 *      datapath - PHY datapath mode
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84328_polarity_tx_invert(phy_ctrl_t *pc, uint32_t value, int datapath)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t data;
    int lane, flip;

    if (BCM84328_SINGLE_PORT_MODE(pc)) {
        for (lane = 0; lane < NUM_PHY_LANES; lane++) {
            flip = 0;
            if (value & (0xf << (lane * 4))) {
                flip = 1;
            }
        
            rv = _bcm84328_channel_select(pc, lane, PHY_REGS_SIDE_LINE);
            if (CDK_FAILURE(rv)) {
                return CDK_E_FAIL;
            }
            
            if (datapath == PHY_DATAPATH_20_BIT) {
                ioerr += PHY_BUS_READ(pc, PMA_PMD_TX_ANA_CTRL1_REG, &data);
                data &= ~PMA_PMD_TX_ANA_CTRL1_TXPOL_FLIP;
                if (!flip) {
                    data |= PMA_PMD_TX_ANA_CTRL1_TXPOL_FLIP;
                }
                ioerr += PHY_BUS_WRITE(pc, PMA_PMD_TX_ANA_CTRL1_REG, data);
            } else {
                ioerr += PHY_BUS_READ(pc, PMA_PMD_TX_ANA_CTRL4_REG, &data);
                data &= ~PMA_PMD_TX_ANA_CTRL4_TXPOL_FLIP;
                if (!flip) {
                    data |= PMA_PMD_TX_ANA_CTRL4_TXPOL_FLIP;
                }
                ioerr += PHY_BUS_WRITE(pc, PMA_PMD_TX_ANA_CTRL4_REG, data);
            }
        }
        /* Restore to default single port register access */
        rv = _bcm84328_channel_select(pc, PHY_ALL_LANES, PHY_REGS_SIDE_LINE);
        if (CDK_FAILURE(rv)) {
            return CDK_E_FAIL;
        }
    } else {
        flip = 0;
        if (value & (0xf << (pc->port & 0x3))) {
            flip = 1;
        }
        if (datapath == PHY_DATAPATH_20_BIT) {
            ioerr += PHY_BUS_READ(pc, PMA_PMD_TX_ANA_CTRL1_REG, &data);
            data &= ~PMA_PMD_TX_ANA_CTRL1_TXPOL_FLIP;
            if (!flip) {
                data |= PMA_PMD_TX_ANA_CTRL1_TXPOL_FLIP;
            }
            ioerr += PHY_BUS_WRITE(pc, PMA_PMD_TX_ANA_CTRL1_REG, data);
        } else {
            ioerr += PHY_BUS_READ(pc, PMA_PMD_TX_ANA_CTRL4_REG, &data);
            data &= ~PMA_PMD_TX_ANA_CTRL4_TXPOL_FLIP;
            if (!flip) {
                data |= PMA_PMD_TX_ANA_CTRL4_TXPOL_FLIP;
            }
            ioerr += PHY_BUS_WRITE(pc, PMA_PMD_TX_ANA_CTRL4_REG, data);
        }
    }
    
    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

/*
 * Function:
 *      _bcm84328_broadcast_setup
 * Purpose:
 *      Put all lanes of a PHY in or out of MDIO broadcast mode.
 * Parameters:
 *      pc - PHY control structure
 *      enable - enable MDIO broadcast mode for this core
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84328_broadcast_setup(phy_ctrl_t *pc, int enable)
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
 *      _bcm84328_soft_reset
 * Purpose:
 *      Perform soft reset and wait for completion.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84328_soft_reset(phy_ctrl_t *pc)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t data;
    int cnt;

    /* Soft reset */
    ioerr += PHY_BUS_READ(pc, PMA_PMD_GEN_CTRL_REG, &data);
    data |= PMA_PMD_GEN_CTRL_URCST;
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_GEN_CTRL_REG, data);

    /* Clear GP_REG registers */
    data = 0;
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_GP_0_REG, data);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_GP_1_REG, data);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_GP_2_REG, data);

    ioerr += PHY_BUS_READ(pc, PMA_PMD_CTRL_REG, &data);
    data |= PMA_PMD_CTRL_RESET;
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_CTRL_REG, data);

    /* Wait for 2.8 msec for 8051 to start */
    PHY_SYS_USLEEP(2800);

    /* Wait for reset completion */
    for (cnt = 0; cnt < PHY_RESET_POLL_MAX; cnt++) {
        ioerr += PHY_BUS_READ(pc, PMA_PMD_CTRL_REG, &data);
        if ((data & MII_CTRL_RESET) == 0) {
            break;
        }
    }
    if (cnt >= PHY_RESET_POLL_MAX) {
        PHY_WARN(pc, ("reset timeout\n"));
        rv = CDK_E_TIMEOUT;
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _bcm84328_firmware_download_prep
 * Purpose:
 *      Prepare for firmware download via MDIO.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84328_firmware_download_prep(phy_ctrl_t *pc)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;

    /* This step is performed on the broadcast master only */
    if (PHY_CTRL_FLAGS(pc) & PHY_F_STAGED_INIT) {
        if (!(PHY_CTRL_FLAGS(pc) & PHY_F_BCAST_MSTR)) {
            return CDK_E_NONE;
        }
    }

    /* Apply bcst Reset to start download code from MDIO */
    rv = _bcm84328_soft_reset(pc);

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _bcm84328_firmware_download
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
_bcm84328_mdio_firmware_download(phy_ctrl_t *pc,
                                 uint8_t *fw_data, uint32_t fw_size)
{
    int ioerr = 0;
    uint32_t data;
    uint32_t idx;
    
    /* This step is performed on the broadcast master only */
    if (PHY_CTRL_FLAGS(pc) & PHY_F_STAGED_INIT) {
        if (!(PHY_CTRL_FLAGS(pc) & PHY_F_BCAST_MSTR)) {
            return CDK_E_NONE;
        }
    }
    
    /* Check firmware data */
    if (fw_data == NULL || fw_size == 0) {
        PHY_WARN(pc, ("MDIO download: invalid firmware\n"));
        return CDK_E_NONE;
    }
    
    /* 
     * clear SPA ctrl reg bit 15 and bit 13.
     * bit 15, 0-use MDIO download to SRAM, 1 SPI-ROM download to SRAM
     * bit 13, 0 clear download done status, 1 skip download
     */
    ioerr += PHY_BUS_READ(pc, PMA_PMD_SPA_CTRL_REG, &data);
    data &= ~(PMA_PMD_SPA_CTRL_SPI_PORT_USED | PMA_PMD_SPA_CTRL_SPI_DWLD_DONE);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_SPA_CTRL_REG, data);
    
    /* set SPA ctrl reg bit 14, 1 RAM boot, 0 internal ROM boot */
    ioerr += PHY_BUS_READ(pc, PMA_PMD_SPA_CTRL_REG, &data);
    data |= PMA_PMD_SPA_CTRL_SPI_BOOT;
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_SPA_CTRL_REG, data);
    
    /* misc_ctrl1 reg bit 3 to 1 for 32k downloading size */
    ioerr += PHY_BUS_READ(pc, PMA_PMD_MISC_CTRL, &data);
    data |= PMA_PMD_MISC_CTRL_32K;
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_MISC_CTRL, data);
    
    /* deassert ucrst 1.ca10 bit 2=0 */
    ioerr += PHY_BUS_READ(pc, PMA_PMD_GEN_CTRL_REG, &data);
    data &= ~PMA_PMD_GEN_CTRL_URCST;
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_GEN_CTRL_REG, data);
    
    /* wait for 2ms for M8051 start and 5ms to initialize the RAM */
    PHY_SYS_USLEEP(10000);
    
    /* Write Starting Address, where the Code will reside in SRAM */
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
 *      _bcm84328_mdio_firmware_done
 * Purpose:
 *      MDIO firmware download post-processing.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84328_mdio_firmware_done(phy_ctrl_t *pc)
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
    
    PHY_SYS_USLEEP(100); /* Wait for 100 usecs */
    
    /* 
     * Read checksum for Micro. The expected value is 0x0300, where 
     * upper_byte=03 means mdio download is done and lower byte=00 means 
     * no checksum error. 
     */
    ioerr += PHY_BUS_READ(pc, PMA_PMD_MSG_OUT_REG, &data);
    PHY_VERB(pc, ("MDIO firmware download status message state:0x%"PRIx32", "
                  "checksum:0x%"PRIx32", port:%d\n",
                  ((data >> 8) & 0xff), (data & 0xff), pc->port));
    
    /* Need to check if checksum is correct */
    ioerr += PHY_BUS_READ(pc, PMA_PMD_GP_4_REG, &data);
    if (data == BCM84328_MDIO_FW_CHKSUM) {
        ioerr += PHY_BUS_READ(pc, PMA_PMD_UC_VERSION_REG, &data);
        PHY_VERB(pc, ("Firmware revID:0x%"PRIx32": port:%d\n",
                      data, pc->port));
    }
    
    /* Disable broadcast mode */
    rv = _bcm84328_broadcast_setup(pc, 0);
    if (CDK_FAILURE(rv)) {
        return rv;
    }
    
    /* Go back to proper PMD mode */
    ioerr += PHY_BUS_READ(pc, PMA_PMD_SINGLE_PMD_CTRL_REG, &data);
    data &= ~PMA_PMD_SINGLE_PMD_MODE;
    if (BCM84328_SINGLE_PORT_MODE(pc)) {
        data |= PMA_PMD_SINGLE_PMD_MODE;
    } 
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_SINGLE_PMD_CTRL_REG, data);
    
    /* Make sure micro completes its initialization */
    PHY_SYS_USLEEP(5000);
    
    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _bcm84328_init_stage_0
 * Purpose:
 *      PHY init.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84328_init_stage_0(phy_ctrl_t *pc)
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
    
    rv = _bcm84328_side_regs_select(pc, PHY_REGS_SIDE_LINE);
    if (CDK_FAILURE(rv)) {
        return rv;
    }
    
    ioerr += PHY_BUS_READ(pc, PMA_PMD_SINGLE_PMD_CTRL_REG, &data);
    if (BCM84328_SINGLE_PORT_MODE(pc)) {
        data |= PMA_PMD_SINGLE_PMD_MODE;
    } else {
        data &= ~PMA_PMD_SINGLE_PMD_MODE;
    }
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_SINGLE_PMD_CTRL_REG, data);

    /* Setup broadcast mode */
    rv = _bcm84328_broadcast_setup(pc, 1);

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _bcm84328_init_stage_1
 * Purpose:
 *      PHY init.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84328_init_stage_1(phy_ctrl_t *pc)
{
    int rv = CDK_E_NONE;

    /* Prepare for firmware download */
    rv = _bcm84328_firmware_download_prep(pc);  
    
    return rv;
}

/*
 * Function:
 *      _bcm84328_init_stage_2
 * Purpose:
 *      PHY init.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84328_init_stage_2(phy_ctrl_t *pc)
{
    int rv = CDK_E_NONE;

    /* Broadcast setup for first encounterd devices */
    rv = _bcm84328_broadcast_setup(pc, 1);
    
    return rv;
}

/*
 * Function:
 *      _bcm84328_init_stage_3
 * Purpose:
 *      PHY init.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84328_init_stage_3(phy_ctrl_t *pc)
{
    int rv = CDK_E_NONE;

    /* Broadcast firmware to MDIO bus, for first encounterd device */
    rv = _bcm84328_mdio_firmware_download(pc, bcm84328_ucode,
                                              bcm84328_ucode_len);
    return rv;
}

/*
 * Function:
 *      _bcm84328_init_stage_4
 * Purpose:
 *      PHY init.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84328_init_stage_4(phy_ctrl_t *pc)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t data;
    int speed;
    
    _PHY_DBG(pc, ("init_stage_4\n"));

    /* MDIO firmware download post-processing */
    rv = _bcm84328_mdio_firmware_done(pc);
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

    /* Reset does not clear lane control2 */
    ioerr += PHY_BUS_READ(pc, PMA_PMD_C017_REG, &data);
    data &= ~(0xf << 4);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_C017_REG, data);

    rv = _bcm84328_side_regs_select(pc, PHY_REGS_SIDE_SYSTEM);
    if (CDK_FAILURE(rv)) {
        return rv;
    }
    ioerr += PHY_BUS_READ(pc, PMA_PMD_C017_REG, &data);
    data &= ~(0xf << 4);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_C017_REG, data);

    /* Make sure accessing line side registers and 1:1 logical channel mappings */
    rv = _bcm84328_side_regs_select(pc, PHY_REGS_SIDE_LINE);
    if (CDK_FAILURE(rv)) {
        return rv;
    }
    
    ioerr += PHY_BUS_READ(pc, PMA_PMD_CA7B_REG, &data);
    data &= ~(0xff);
    data |= 0xe4;
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_CA7B_REG, data);

    rv = _bcm84328_polarity_tx_invert(pc, 0, PHY_DATAPATH_20_BIT);
    if (CDK_SUCCESS(rv)) {
        rv = _bcm84328_polarity_rx_invert(pc, 0, PHY_DATAPATH_20_BIT);
    }
    if (CDK_SUCCESS(rv)) {
        rv = _bcm84328_polarity_tx_invert(pc, 0, PHY_DATAPATH_4_BIT);
    }
    if (CDK_SUCCESS(rv)) {
        rv = _bcm84328_polarity_rx_invert(pc, 0, PHY_DATAPATH_4_BIT);
    }
    if (CDK_FAILURE(rv)) {
        return rv;
    }
    
    /* Bypass steady state tuning */
    rv = _bcm84328_side_regs_select(pc, PHY_REGS_SIDE_SYSTEM);
    if (CDK_FAILURE(rv)) {
        return rv;
    }
    ioerr += PHY_BUS_READ(pc, PMA_PMD_C1FC_REG, &data);
    data |= (1 << 8);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_C1FC_REG, data);
    
    rv = _bcm84328_side_regs_select(pc, PHY_REGS_SIDE_LINE);
    if (CDK_FAILURE(rv)) {
        return rv;
    }
   
    /* Initialize system and line side interfaces */
    speed = 10000;
    if (BCM84328_SINGLE_PORT_MODE(pc)) {
        speed = 40000;
    }
    rv = PHY_SPEED_SET(pc, speed);
    
    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _bcm84328_init_stage_5
 * Purpose:
 *      PHY init.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84328_init_stage_5(phy_ctrl_t *pc)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t data;

    _PHY_DBG(pc, ("init_stage_5\n"));

    if (PHY_CTRL_NEXT(pc)) {
        PHY_NOTIFY(PHY_CTRL_NEXT(pc), PhyEvent_PhyEnable);
    }

    ioerr += PHY_BUS_READ(pc, PMA_PMD_REGUALTOR_CTRL_REG, &data);
    data &= ~PMA_PMD_REGUALTOR_CTRL_PWRDN;
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_REGUALTOR_CTRL_REG, data);

    /* Logical lane0 used for auto-negotiation in 40G CR4 */
    if (BCM84328_SINGLE_PORT_MODE(pc)) {
        ioerr += PHY_BUS_READ(pc, PMA_PMD_SINGLE_PMD_CTRL_REG, &data);
        data &= ~PMA_PMD_PMD_CTRL_ACCESS_MASK;
        ioerr += PHY_BUS_WRITE(pc, PMA_PMD_SINGLE_PMD_CTRL_REG, data);
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _bcm84328_init_stage
 * Purpose:
 *      Execute specified init stage.
 * Parameters:
 *      pc - PHY control structure
 *      stage - init stage
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84328_init_stage(phy_ctrl_t *pc, int stage)
{
    switch (stage) {
    case 0:
        return _bcm84328_init_stage_0(pc);
    case 1:
        return _bcm84328_init_stage_1(pc);
    case 2:
        return _bcm84328_init_stage_2(pc);
    case 3:
        return _bcm84328_init_stage_3(pc);
    case 4:
        return _bcm84328_init_stage_4(pc);
    case 5:
        return _bcm84328_init_stage_5(pc);
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
 *      bcm84328_phy_probe
 * Purpose:     
 *      Probe for 84328 PHY
 * Parameters:
 *      pc - PHY control structure
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm84328_phy_probe(phy_ctrl_t *pc)
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

    if ((phyid0 == BCM84328_PMA_PMD_ID0) &&
        ((phyid1 & ~PHY_ID1_REV_MASK) == BCM84328_PMA_PMD_ID1)) {

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
 *      bcm84328_phy_notify
 * Purpose:     
 *      Handle PHY notifications
 * Parameters:
 *      pc - PHY control structure
 *      event - PHY event
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm84328_phy_notify(phy_ctrl_t *pc, phy_event_t event)
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
 *      bcm84328_phy_reset
 * Purpose:     
 *      Reset 84328 PHY
 * Parameters:
 *      pc - PHY control structure
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm84328_phy_reset(phy_ctrl_t *pc)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;

    PHY_CTRL_CHECK(pc);

    rv = _bcm84328_soft_reset(pc);

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
 *      bcm84328_phy_init
 * Purpose:     
 *      Initialize 84328 PHY driver
 * Parameters:
 *      pc - PHY control structure
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm84328_phy_init(phy_ctrl_t *pc)
{
    int rv = CDK_E_NONE;
    int stage;

    PHY_CTRL_CHECK(pc);

    if (PHY_CTRL_FLAGS(pc) & PHY_F_STAGED_INIT) {
        PHY_CTRL_FLAGS(pc) &= ~PHY_F_STAGED_INIT;
    }

    for (stage = 0; CDK_SUCCESS(rv); stage++) {
        rv = _bcm84328_init_stage(pc, stage);
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
 *      bcm84328_phy_link_get
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
bcm84328_phy_link_get(phy_ctrl_t *pc, int *link, int *autoneg_done)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t data, mask;
    int autoneg;

    PHY_CTRL_CHECK(pc);
    
    if (!link || !autoneg_done) {
        return CDK_E_PARAM;
    }
        
    *link = 0;
    *autoneg_done = 0;

    /* PCS from the internal PHY is used to determine link. */
    if (PHY_CTRL_NEXT(pc)) {
        rv = PHY_LINK_GET(PHY_CTRL_NEXT(pc), link, autoneg_done);
        if (CDK_FAILURE(rv)) {
            return rv;
        }
    }

    if (PHY_CTRL_FLAGS(pc) & PHY_F_PASSTHRU) {
        if (PHY_CTRL_NEXT(pc)) {
            rv = PHY_AUTONEG_GET(PHY_CTRL_NEXT(pc), &autoneg);
        }
    } else {
        ioerr += PHY_BUS_READ(pc, AN_CTRL_REG, &data);
        autoneg = (data & AN_ENABLE);
        
        if (autoneg_done) {
            ioerr += PHY_BUS_READ(pc, PMA_PMD_R_PMD_STATUS_REG, &data);
        
            /* Check autoneg status before link status */
            mask = 0x0001;
            if (BCM84328_SINGLE_PORT_MODE(pc)) {
                mask = 0x1111;
            }
            *autoneg_done = ((data & mask) == mask);
        }
    }

    if ((autoneg == TRUE) && (autoneg_done == FALSE)) {
        *link = FALSE;
        return CDK_E_NONE;
    }
    
    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

/*
 * Function:    
 *      bcm84328_phy_duplex_set
 * Purpose:     
 *      Set the current duplex mode (forced).
 * Parameters:
 *      pc - PHY control structure
 *      duplex - non-zero indicates full duplex, zero indicates half
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm84328_phy_duplex_set(phy_ctrl_t *pc, int duplex)
{
    return (duplex != 0) ? CDK_E_NONE : CDK_E_PARAM;
}

/*
 * Function:    
 *      bcm84328_phy_duplex_get
 * Purpose:     
 *      Get the current operating duplex mode.
 * Parameters:
 *      pc - PHY control structure
 *      duplex - (OUT) non-zero indicates full duplex, zero indicates half
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm84328_phy_duplex_get(phy_ctrl_t *pc, int *duplex)
{
    *duplex = 1;

    return CDK_E_NONE;
}

/*
 * Function:    
 *      bcm84328_phy_speed_set
 * Purpose:     
 *      Set the current operating speed (forced).
 * Parameters:
 *      pc - PHY control structure
 *      speed - new link speed
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm84328_phy_speed_set(phy_ctrl_t *pc, uint32_t speed)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t new_type, cur_type, cur_speed;
    uint32_t data, mask;
    int an, lb, datapath;

    PHY_CTRL_CHECK(pc);

    /* Do not set speed if auto-negotiation is enabled */
    rv = PHY_AUTONEG_GET(pc, &an);
    if (CDK_FAILURE(rv) || an) {
        return rv;
    }

    /* Enable the auto-negotiation if the interface is KR type */
    if (PHY_CTRL_LINE_INTF(pc) == PHY_IF_KR) {
        return PHY_AUTONEG_SET(pc, 1);
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
    if (BCM84328_SINGLE_PORT_MODE(pc)) {
        switch (speed) {
        case 42000:
        case 40000:
            if (!PHY_CTRL_LINE_INTF(pc)) {
                PHY_CTRL_LINE_INTF(pc) = PHY_IF_CR;
            } 
            break;
        default:
            return CDK_E_PARAM;
        }
    } else {
        switch (speed) {
        case 10000:
            if (!PHY_CTRL_LINE_INTF(pc)) {
                PHY_CTRL_LINE_INTF(pc) = PHY_IF_XFI;
            } 
            break;
        case 1000:
        case 100:
        case 10:
            PHY_CTRL_FLAGS(pc) |= PHY_F_PASSTHRU;
            break;
        default:
            return CDK_E_PARAM;
        }
    }

    /* Set the datapath to 20 bit mode if the speed < 10000 */
    datapath = PHY_DATAPATH_4_BIT;
    if (speed < 10000) { 
        datapath = PHY_DATAPATH_20_BIT; 
    }
    _bcm84328_datapath_mode_set(pc, datapath);

    if (PHY_CTRL_NEXT(pc)) {
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
            new_type = PHY_CTRL_LINE_INTF(pc);
            if (BCM84328_SINGLE_PORT_MODE(pc) && (new_type == PHY_IF_CR)) {
                new_type = PHY_IF_XLAUI;
            }
                
            cur_type = PHY_CTRL_LINE_INTF(PHY_CTRL_NEXT(pc));
            if (cur_type != new_type) {
              PHY_CTRL_LINE_INTF(PHY_CTRL_NEXT(pc)) = new_type;
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

    /* Update the interface */
    data = 0;
    mask = (PMA_PMD_GEN_SYS_LR_MODE | PMA_PMD_GEN_LINE_LR_MODE |
            PMA_PMD_GEN_SYS_FORCE_CL72 | PMA_PMD_GEN_LINE_FORCE_CL72 |
            PMA_PMD_GEN_SYS_CU_TYPE | PMA_PMD_GEN_LINE_CU_TYPE |
            PMA_PMD_GEN_SYS_TYPE | PMA_PMD_GEN_LINE_TYPE |
            PMA_PMD_GEN_SPEED_100G | PMA_PMD_GEN_SPEED_40G | 
            PMA_PMD_GEN_SPEED_10G | PMA_PMD_GEN_SPEED_1G);
    if ((PHY_CTRL_LINE_INTF(pc) == PHY_IF_XFI) || 
        (PHY_CTRL_LINE_INTF(pc) == PHY_IF_SFI)) {
        data |= (PMA_PMD_GEN_SYS_LR_MODE | 
                 PMA_PMD_GEN_LINE_LR_MODE | 
                 PMA_PMD_GEN_SYS_CU_TYPE | 
                 PMA_PMD_GEN_LINE_CU_TYPE);
    } else if (PHY_CTRL_LINE_INTF(pc) == PHY_IF_CR) {
        if (BCM84328_SINGLE_PORT_MODE(pc)) {
            data |= (PMA_PMD_GEN_SYS_LR_MODE | 
                     PMA_PMD_GEN_SYS_CU_TYPE | 
                     PMA_PMD_GEN_LINE_FORCE_CL72 | 
                     PMA_PMD_GEN_LINE_CU_TYPE);
        } else {
            data |= (PMA_PMD_GEN_SYS_FORCE_CL72 | 
                     PMA_PMD_GEN_SYS_CU_TYPE | 
                     PMA_PMD_GEN_LINE_CU_TYPE);
        }
    } 
    
    if (speed == 42000) {
        data |= (PMA_PMD_GEN_SPEED_40G | PMA_PMD_GEN_SPEED_1G);
    } else if (speed == 40000) {
        data |= PMA_PMD_GEN_SPEED_40G;
    } else if (speed == 10000) {
        data |= PMA_PMD_GEN_SPEED_10G;
    } else {
        data |= (PMA_PMD_GEN_SPEED_10G | PMA_PMD_GEN_SPEED_1G);
    }

    /* Disable the internal serdes before update the phy interface */
    if (PHY_CTRL_NEXT(pc)) {
        PHY_NOTIFY(PHY_CTRL_NEXT(pc), PhyEvent_PhyDisable);
    }
    
    rv = _bcm84328_interface_update(pc, data, mask);
    if (CDK_FAILURE(rv)) {
        return rv;
    }
 
    /* Enable the internal serdes */
    if (PHY_CTRL_NEXT(pc)) {
        PHY_SYS_USLEEP(10000); 
        PHY_NOTIFY(PHY_CTRL_NEXT(pc), PhyEvent_PhyEnable);
    }

    /* Restore the loopback if necessary */
    if (lb) {
        rv = PHY_LOOPBACK_SET(pc, 1);
    }
    
    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:    
 *      bcm84328_phy_speed_get
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
bcm84328_phy_speed_get(phy_ctrl_t *pc, uint32_t *speed)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t data;

    PHY_CTRL_CHECK(pc);
    
    if (speed) {
        ioerr += PHY_BUS_READ(pc, PMA_PMD_GEN_1_REG, &data);
        if (data & PMA_PMD_GEN_SPEED_1G) {
            if (data & PMA_PMD_GEN_SPEED_40G) {
                *speed = 42000;
            } else if (data & PMA_PMD_GEN_SPEED_10G) {
                if (PHY_CTRL_NEXT(pc)) {
                    rv = PHY_SPEED_GET(PHY_CTRL_NEXT(pc), speed);
                }
            }
        } else if (data & PMA_PMD_GEN_SPEED_10G) {
            *speed = 10000;
        } else if (data & PMA_PMD_GEN_SPEED_40G) {
            *speed = 40000;
        }
    }
    
    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:    
 *      bcm84328_phy_autoneg_set
 * Purpose:     
 *      Enable or disable auto-negotiation on the specified port.
 * Parameters:
 *      pc - PHY control structure
 *      autoneg - non-zero enables autoneg, zero disables autoneg
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm84328_phy_autoneg_set(phy_ctrl_t *pc, int autoneg)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t data, mask;

    PHY_CTRL_CHECK(pc);

    if (PHY_CTRL_NEXT(pc)) {
        if (PHY_CTRL_FLAGS(pc) & PHY_F_PASSTHRU) {
            rv = PHY_AUTONEG_SET(PHY_CTRL_NEXT(pc), autoneg);            
            if (CDK_FAILURE(rv)) {
                return rv;
            }
        }
    }

    data = 0;
    mask = (PMA_PMD_GEN_LINE_LR_MODE | PMA_PMD_GEN_LINE_FORCE_CL72 | 
            PMA_PMD_GEN_LINE_CU_TYPE | PMA_PMD_GEN_LINE_TYPE);
    if (BCM84328_SINGLE_PORT_MODE(pc)) {
        if (PHY_CTRL_LINE_INTF(pc) == PHY_IF_CR) {
            mask |= (PMA_PMD_GEN_4BIT_FIFO_DEPTH | 
                     PMA_PMD_GEN_4BIT_DATAPATH_MODE);
            if (!autoneg) {
                data = (PMA_PMD_GEN_4BIT_DATAPATH_MODE | 
                        PMA_PMD_GEN_LINE_FORCE_CL72 | 
                        PMA_PMD_GEN_LINE_CU_TYPE);
            } else {
                data = PMA_PMD_GEN_LINE_CU_TYPE;
            }
        } else {
            if (autoneg) {
                data = PMA_PMD_GEN_LINE_TYPE;
            }
        }
    } 
    
    if (!data) {
        if (autoneg) {
            /* Set to KR mode for AN enable */
            data = PMA_PMD_GEN_LINE_TYPE;
        } else {
            /* Set to XFI mode for AN disable */
            data = (PMA_PMD_GEN_LINE_LR_MODE | PMA_PMD_GEN_LINE_CU_TYPE);
        }
    }
    
    rv = _bcm84328_interface_update(pc, data, mask);
    if (CDK_FAILURE(rv)) {
        return rv;
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:    
 *      bcm84328_phy_autoneg_get
 * Purpose:     
 *      Get the current auto-negotiation setting.
 * Parameters:
 *      pc - PHY control structure
 *      autoneg - (OUT) non-zero indicates autoneg enabled
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm84328_phy_autoneg_get(phy_ctrl_t *pc, int *autoneg)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t data;

    PHY_CTRL_CHECK(pc);

    if (autoneg) {
        if (PHY_CTRL_FLAGS(pc) & PHY_F_PASSTHRU) {
            if (PHY_CTRL_NEXT(pc)) {
                rv = PHY_AUTONEG_GET(PHY_CTRL_NEXT(pc), autoneg);
            }
        } else {
            if (autoneg) {
                ioerr += PHY_BUS_READ(pc, AN_CTRL_REG, &data);
                *autoneg = (data & AN_ENABLE);
            }
        }
    }
    
    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:    
 *      bcm84328_phy_loopback_set
 * Purpose:     
 *      Set the internal PHY loopback mode.
 * Parameters:
 *      pc - PHY control structure
 *      enable - non-zero enables PHY loopback
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm84328_phy_loopback_set(phy_ctrl_t *pc, int enable)
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
    if (CDK_FAILURE(rv)) {
        return rv;
    }

    rv = PHY_SPEED_GET(pc, &cur_speed);
    if (CDK_FAILURE(rv)) {
        return rv;
    }
    
    /* Loopback requires 20-bit datapath */
    datapath = PHY_DATAPATH_4_BIT;
    if ((cur_speed < 10000) || enable) { 
        datapath = PHY_DATAPATH_20_BIT; 
    }
    _bcm84328_datapath_mode_set(pc, datapath);

    PHY_SYS_USLEEP(1000); 

    /* Change to the system side */
    rv = _bcm84328_side_regs_select(pc, PHY_REGS_SIDE_SYSTEM);
    if (CDK_FAILURE(rv)) {
        return rv;
    }

    /* Read loopback control registers */
    ioerr += PHY_BUS_READ(pc, PMA_PMD_TX_ANA_CTRL6_REG, &data);
    if (enable) {
        data |= PMA_PMD_TX_ANA_CTRL6_REMOTE_LB_ENABLE;
        data &= ~PMA_PMD_TX_ANA_CTRL6_LOWLATENCY_PATH_SEL;
    } else {
        data &= ~PMA_PMD_TX_ANA_CTRL6_REMOTE_LB_ENABLE;
        data |= PMA_PMD_TX_ANA_CTRL6_LOWLATENCY_PATH_SEL;
    }
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_TX_ANA_CTRL6_REG, data);

    /* Change back to the line side */
    rv = _bcm84328_side_regs_select(pc, PHY_REGS_SIDE_LINE);
    if (CDK_FAILURE(rv)) {
        return rv;
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:    
 *      bcm84328_phy_loopback_get
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
bcm84328_phy_loopback_get(phy_ctrl_t *pc, int *enable)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t data;

    PHY_CTRL_CHECK(pc);

    if (enable) {
        /* Get loopback of upstream PHY */
        rv = PHY_LOOPBACK_GET(PHY_CTRL_NEXT(pc), enable);
        if (CDK_FAILURE(rv)) {
            return rv;
        }
    
        if (*enable == 0) {
            /* Change to the system side */
            rv = _bcm84328_side_regs_select(pc, PHY_REGS_SIDE_SYSTEM);
            if (CDK_FAILURE(rv)) {
                return rv;
            }
            
            /* Read loopback control registers */
            ioerr += PHY_BUS_READ(pc, PMA_PMD_TX_ANA_CTRL6_REG, &data);
            if (data & PMA_PMD_TX_ANA_CTRL6_REMOTE_LB_ENABLE) {
                *enable = 1;
            }
    
            /* Change back to the line side */
            rv = _bcm84328_side_regs_select(pc, PHY_REGS_SIDE_LINE);
            if (CDK_FAILURE(rv)) {
                return rv;
            }
        }
    }
    
    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:    
 *      bcm84328_phy_ability_get
 * Purpose:     
 *      Get the abilities of the PHY.
 * Parameters:
 *      pc - PHY control structure
 *      abil - (OUT) ability mask indicating supported options/speeds.
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm84328_phy_ability_get(phy_ctrl_t *pc, uint32_t *abil)
{
    PHY_CTRL_CHECK(pc);

    *abil = (PHY_ABIL_40GB | PHY_ABIL_10GB | PHY_ABIL_1000MB_FD | 
             PHY_ABIL_100MB_FD | PHY_ABIL_10MB_FD | PHY_ABIL_PAUSE_ASYMM | 
             PHY_ABIL_XGMII | PHY_ABIL_LOOPBACK | PHY_ABIL_AN);

    return CDK_E_NONE;
}

/*
 * Function:
 *      bcm84328_phy_config_set
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
bcm84328_phy_config_set(phy_ctrl_t *pc, phy_config_t cfg, uint32_t val, void *cd)
{
    PHY_CTRL_CHECK(pc);

    switch (cfg) {
    case PhyConfig_InitStage:
        if (PHY_CTRL_FLAGS(pc) & PHY_F_STAGED_INIT) {
            return _bcm84328_init_stage(pc, val);
        }
        break;
    default:
        break;
    }

    return CDK_E_UNAVAIL;
}

/*
 * Function:
 *      bcm84328_phy_config_get
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
bcm84328_phy_config_get(phy_ctrl_t *pc, phy_config_t cfg, uint32_t *val, void *cd)
{
    PHY_CTRL_CHECK(pc);

    switch (cfg) {
    case PhyConfig_Enable:
        *val = 1;
        return CDK_E_NONE;
    case PhyConfig_Clause45Devs:
        *val = 0x82;
        return CDK_E_NONE;
    case PhyConfig_BcastAddr:
        *val = PHY_CTRL_BUS_ADDR(pc) & ~0x1f;
        return CDK_E_NONE;
    default:
        break;
    }

    return CDK_E_UNAVAIL;
}

/*
 * Variable:    bcm84328_drv
 * Purpose:     PHY Driver for BCM84328.
 */
phy_driver_t bcm84328_drv = {
    "bcm84328",
    "BCM84328 40GbE/10GbE PHY Driver",  
    0,
    bcm84328_phy_probe,                  /* pd_probe */
    bcm84328_phy_notify,                 /* pd_notify */
    bcm84328_phy_reset,                  /* pd_reset */
    bcm84328_phy_init,                   /* pd_init */
    bcm84328_phy_link_get,               /* pd_link_get */
    bcm84328_phy_duplex_set,             /* pd_duplex_set */
    bcm84328_phy_duplex_get,             /* pd_duplex_get */
    bcm84328_phy_speed_set,              /* pd_speed_set */
    bcm84328_phy_speed_get,              /* pd_speed_get */
    bcm84328_phy_autoneg_set,            /* pd_autoneg_set */
    bcm84328_phy_autoneg_get,            /* pd_autoneg_get */
    bcm84328_phy_loopback_set,           /* pd_loopback_set */
    bcm84328_phy_loopback_get,           /* pd_loopback_get */
    bcm84328_phy_ability_get,            /* pd_ability_get */
    bcm84328_phy_config_set,             /* pd_config_set */
    bcm84328_phy_config_get,             /* pd_config_get */
    NULL,                                /* pd_status_get */
    NULL                                 /* pd_cable_diag */
};
