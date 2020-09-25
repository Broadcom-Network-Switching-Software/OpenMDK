/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * PHY driver for BCM84740.
 */

#include <phy/phy.h>
#include <phy/ge_phy.h>

extern unsigned char bcm84740_ucode[];
extern unsigned int bcm84740_ucode_len;

#define PHY_RESET_POLL_MAX              10
#define PHY_ROM_LOAD_POLL_MAX           500
#define PHY_LANES_POLL_MAX              1000
#define PHY_DOWNLOAD_MSEC               200
#define MDIO_FREQ_MAX_KHZ               24000

#define BCM84740_PMA_PMD_ID0            0x0362
#define BCM84740_PMA_PMD_ID1            0x5fd0
#define BCM84740_CHIP_ID                0x84740

/* Default SPI ROM checksum */
#ifndef BCM84740_SPI_ROM_CHKSUM
#define BCM84740_SPI_ROM_CHKSUM         0x600d
#endif

/* Firmware checksum for MDIO download */
#define BCM84740_MDIO_FW_CHKSUM         0x600d

#define C45_DEVAD(_a)                   LSHIFT32((_a),16)
#define DEVAD_PMA_PMD                   C45_DEVAD(MII_C45_DEV_PMA_PMD)
#define DEVAD_PCS                       C45_DEVAD(MII_C45_DEV_PCS)
#define DEVAD_PHY_XS                    C45_DEVAD(MII_C45_DEV_PHY_XS)
#define DEVAD_AN                        C45_DEVAD(MII_C45_DEV_AN)

/* PMA/PMD registers */
#define PMA_PMD_CTRL_REG                (DEVAD_PMA_PMD + MII_CTRL_REG)
#define PMA_PMD_STAT_REG                (DEVAD_PMA_PMD + MII_STAT_REG)
#define PMA_PMD_ID0_REG                 (DEVAD_PMA_PMD + MII_PHY_ID0_REG)
#define PMA_PMD_ID1_REG                 (DEVAD_PMA_PMD + MII_PHY_ID1_REG)
#define PMA_PMD_SPEED_ABIL              (DEVAD_PMA_PMD + 0x0005)
#define PMA_PMD_DEV_IN_PKG              (DEVAD_PMA_PMD + 0x0006)
#define PMA_PMD_CTRL2_REG               (DEVAD_PMA_PMD + 0x0007)
#define PMA_PMD_STAT2_REG               (DEVAD_PMA_PMD + 0x0008)

#define SPEED_LINK_DETECT_STAT_REG      (DEVAD_PMA_PMD + 0xc820)
#define PMA_PMD_BCST_REG                (DEVAD_PMA_PMD + 0xc8fe)
#define PMA_PMD_CHIP_MODE_REG           (DEVAD_PMA_PMD + 0xc805)
#define PMA_PMD_SPI_CTRL_REG            (DEVAD_PMA_PMD + 0xc848)
#define PMA_PMD_MISC_CTRL1_REG          (DEVAD_PMA_PMD + 0xca85)
#define PMA_PMD_M8051_MSGIN_REG         (DEVAD_PMA_PMD + 0xca12)
#define PMA_PMD_M8051_MSGOUT_REG        (DEVAD_PMA_PMD + 0xca13)
#define PMA_PMD_AER_ADDR_REG            (DEVAD_PMA_PMD + 0xc702)

#define PMA_PMD_GEN_CTRL_STAT_REG       (DEVAD_PMA_PMD + 0xca10)
#define PMA_PMD_GEN_REG_0               (DEVAD_PMA_PMD + 0xca18)
#define PMA_PMD_GEN_REG_1               (DEVAD_PMA_PMD + 0xca19)
#define PMA_PMD_GEN_REG_2               (DEVAD_PMA_PMD + 0xca1a)
#define PMA_PMD_GEN_REG_3               (DEVAD_PMA_PMD + 0xca1b)
#define PMA_PMD_GEN_REG_4               (DEVAD_PMA_PMD + 0xca1c)
#define PMA_PMD_CE00_REG                (DEVAD_PMA_PMD + 0xce00)

#define PMA_PMD_MISC2_REG               (DEVAD_PMA_PMD + 0x8309)
#define PMA_PMD_CD17_REG                (DEVAD_PMA_PMD + 0xcd17)
#define PMA_PMD_0096_REG                (DEVAD_PMA_PMD + 0x0096)
#define PMA_PMD_CD53_REG                (DEVAD_PMA_PMD + 0xcd53)
#define PMA_PMD_C806_REG                (DEVAD_PMA_PMD + 0xc806)
#define PMA_PMD_C8E4_REG                (DEVAD_PMA_PMD + 0xc8e4)

#define PMA_PMD_FFFF_REG                (DEVAD_PMA_PMD + 0xffff)

#define PMA_PMD_CHIP_MODE_MASK          0x3
#define PMA_PMD_DAC_MODE_MASK           0x8
#define PMA_PMD_DAC_MODE                0x8
#define PMA_PMD_MODE_40G                0x1

/* PCS registers */
#define PCS_CTRL_REG                    (DEVAD_PCS + MII_CTRL_REG)
#define PCS_STAT_REG                    (DEVAD_PCS + MII_STAT_REG)
#define PCS_ID0_REG                     (DEVAD_PCS + MII_PHY_ID0_REG)
#define PCS_ID1_REG                     (DEVAD_PCS + MII_PHY_ID1_REG)
#define PCS_SPEED_ABIL                  (DEVAD_PCS + 0x0005)
#define PCS_DEV_IN_PKG                  (DEVAD_PCS + 0x0006)
#define PCS_CTRL2_REG                   (DEVAD_PCS + 0x0007)
#define PCS_STAT2_REG                   (DEVAD_PCS + 0x0008)

/* AN registers */
#define AN_CTRL_REG                     (DEVAD_AN + MII_CTRL_REG)
#define AN_STAT_REG                     (DEVAD_AN + MII_STAT_REG)
#define AN_MII_CTRL_REG                 (DEVAD_AN + 0xFFE0)
#define AN_MII_STAT_REG                 (DEVAD_AN + 0xFFE1)
#define AN_8309_REG                     (DEVAD_AN + 0x8309)

/* PMA/PMD standard register definitions */

/* Control register */
#define PMA_PMD_CTRL_SPEED_10G          (1L << 13)
#define PMA_PMD_CTRL_PMA_LOOPBACK       (1L << 0)

/* Control 2 register */
#define PMA_PMD_CTRL2_PMA_TYPE_MASK     0xf
#define PMA_PMD_CTRL2_PMA_TYPE_1G_KX    0xd
#define PMA_PMD_CTRL2_PMA_TYPE_10G_KR   0xb
#define PMA_PMD_CTRL2_PMA_TYPE_10G_LRM  0x8

/* PMA/PMD user-defined register definitions */

/* SPI control register */
#define PMA_PMD_SPI_CTRL_SPI_PORT_USED  (1L << 15)
#define PMA_PMD_SPI_CTRL_SPI_BOOT       (1L << 14)
#define PMA_PMD_SPI_CTRL_SPI_DWLD_DONE  (1L << 13)
#define PMA_PMD_SPI_CTRL_SPI_ENABLE     (1L << 2)

/* Misc. control 1 register */
#define PMA_PMD_MISC_CTRL1_SPI_DWLD_32K (1L << 3)
#define PMA_PMD_MISC_CTRL1_SER_BOOT_CTL (1L << 0)

/* Auto-negotiation register definitions */

/* Auto-negotiation control register */
#define AN_EXT_NXT_PAGE                 (1L << 13)
#define AN_ENABLE                       (1L << 12)
#define AN_RESTART                      (1L << 9)

/* Auto-negotiation status register */
#define AN_DONE                         (1L << 5)
#define AN_LINK                         (1L << 2)
#define AN_LP_AN_ABILITY                (1L << 0)

/* Auto-negotiation advertisement register 0 */ 
#define AN_ADVERT_PAUSE_ASYM            (1L << 11)
#define AN_ADVERT_PAUSE                 (1L << 10)

/* Auto-negotiation advertisement register 1 */
#define AN_ADVERT_10G                   (1L << 7)
#define AN_ADVERT_1G                    (1L << 5)

/* Auto-negotiation advertisement register 2 */
#define AN_ADVERT_FEC                   (1L << 15)

#ifndef BCM84740_RXLOS_OVERRIDE_ENABLE
#define BCM84740_RXLOS_OVERRIDE_ENABLE  1
#endif

/* Private PHY flag is used to indicate firmware download method */
#define PHY_F_FW_MDIO_LOAD              PHY_F_PRIVATE

#define BCM84740_SINGLE_PORT_MODE(_pc) \
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
 *      _bcm84740_inst
 * Purpose:
 *      Retrieve PHY instance.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      PHY instance
 */
static int
_bcm84740_inst(phy_ctrl_t *pc)
{
    /* We always use this function on the primary lane */
    return 0;
}

/*
 * Function:
 *      _bcm84740_fast_load_set
 * Purpose:
 *      Request optimized MDIO clock frequency
 * Parameters:
 *      pc - PHY control structure
 *      enable - turn optimized clock on/off
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84740_fast_load_set(phy_ctrl_t *pc, int enable)
{
    int rv = CDK_E_UNAVAIL;
    int freq_khz;

    if (PHY_CTRL_FW_HELPER(pc)) {
        freq_khz = 0;
        if (enable && (PHY_CTRL_FLAGS(pc) & PHY_F_FAST_LOAD)) {
            freq_khz = MDIO_FREQ_MAX_KHZ;
            PHY_VERB(pc, ("enable fast MDIO clock\n"));
        }
        rv = PHY_CTRL_FW_HELPER(pc)(pc, freq_khz, 0, NULL);
    }
    return rv;
}

/*
 * Function:
 *      _bcm84740_broadcast_set
 * Purpose:
 *      Put all lanes of a PHY in or out of MDIO broadcast mode.
 * Parameters:
 *      pc - PHY control structure
 *      enable - enable MDIO broadcast mode for this core
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84740_broadcast_set(phy_ctrl_t *pc, int enable)
{
    int ioerr = 0;
    int lane;
    uint32_t data;

    data = enable ? 0xffff : 0;

    if (BCM84740_SINGLE_PORT_MODE(pc)) {
        for (lane = 0; lane < 4; lane++) {
            if (phy_ctrl_change_inst(pc, lane, _bcm84740_inst) < 0) {
                return CDK_E_FAIL;
            }
            ioerr += PHY_BUS_WRITE(pc, PMA_PMD_BCST_REG, data);
            if (enable == 0) {
                break;
            }
        }
        if (phy_ctrl_change_inst(pc, 0, _bcm84740_inst) < 0) {
            return CDK_E_FAIL;
        }
    } else {
        ioerr += PHY_BUS_WRITE(pc, PMA_PMD_BCST_REG, data);
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

/*
 * Function:
 *      _bcm84740_soft_reset
 * Purpose:
 *      Perform soft reset and wait for completion.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84740_soft_reset(phy_ctrl_t *pc)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t data;
    int cnt;

    /* Soft reset */
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_CTRL_REG, MII_CTRL_RESET);

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
 *      _bcm84740_single_to_quad_mode
 * Purpose:
 *      Switch PHY from single port mode to quad port mode.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84740_single_to_quad_mode(phy_ctrl_t *pc)
{
    int ioerr = 0;
    int rv;
    uint32_t data;

    _PHY_DBG(pc, ("Single to Quad\n"));

    /* Clear DAC mode (register is broadcast register in single port mode) */
    ioerr += PHY_BUS_READ(pc, PMA_PMD_CHIP_MODE_REG, &data);
    data &= ~PMA_PMD_DAC_MODE_MASK;
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_CHIP_MODE_REG, data);

    /* Configure quad port mode */
    ioerr += PHY_BUS_READ(pc, PMA_PMD_CHIP_MODE_REG, &data);
    data &= ~PMA_PMD_CHIP_MODE_MASK;
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_CHIP_MODE_REG, data);

    /* Enable broadcast mode */
    rv = _bcm84740_broadcast_set(pc, 1);

    /* Do a soft reset to switch mode (clears broadcast mode) */
    if (CDK_SUCCESS(rv)) {
        rv = _bcm84740_soft_reset(pc);
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _bcm84740_quad_to_single_mode
 * Purpose:
 *      Switch PHY from quad port mode to single port mode.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84740_quad_to_single_mode(phy_ctrl_t *pc)
{
    int ioerr = 0;
    int rv;
    uint32_t data;

    _PHY_DBG(pc, ("Quad to Single\n"));

    /* Enable broadcast mode */
    rv = _bcm84740_broadcast_set(pc, 1);

    /* Configure single port mode */
    ioerr += PHY_BUS_READ(pc, PMA_PMD_CHIP_MODE_REG, &data);
    data &= ~PMA_PMD_CHIP_MODE_MASK;
    data |= PMA_PMD_MODE_40G;
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_CHIP_MODE_REG, data);

    /* Configure DAC mode for LR4/SR4 */
    ioerr += PHY_BUS_READ(pc, PMA_PMD_CHIP_MODE_REG, &data);
    data &= ~PMA_PMD_DAC_MODE_MASK;
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_CHIP_MODE_REG, data);

    /* Do a soft reset to switch mode (clears broadcast mode) */
    if (CDK_SUCCESS(rv)) {
        rv = _bcm84740_soft_reset(pc);
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _bcm84740_firmware_download_prep
 * Purpose:
 *      Prepare for firmware download via MDIO.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84740_firmware_download_prep(phy_ctrl_t *pc)
{
    int ioerr = 0;
    int rv;
    uint32_t data;

    /* This step is performed on the broadcast master only */
    if (PHY_CTRL_FLAGS(pc) & PHY_F_STAGED_INIT) {
        if (!(PHY_CTRL_FLAGS(pc) & PHY_F_BCAST_MSTR)) {
            return CDK_E_NONE;
        }
    }

    /* Set download size to 32K */
    ioerr += PHY_BUS_READ(pc, PMA_PMD_MISC_CTRL1_REG, &data);
    data |= PMA_PMD_MISC_CTRL1_SPI_DWLD_32K;
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_MISC_CTRL1_REG, data);

    /* Enable MDIO download and RAM boot */
    ioerr += PHY_BUS_READ(pc, PMA_PMD_SPI_CTRL_REG, &data);
    data &= ~(PMA_PMD_SPI_CTRL_SPI_PORT_USED | PMA_PMD_SPI_CTRL_SPI_DWLD_DONE);
    data |= PMA_PMD_SPI_CTRL_SPI_BOOT;
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_SPI_CTRL_REG, data);

    /* Do a soft reset to start downloading code via MDIO */
    rv = _bcm84740_soft_reset(pc);

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _bcm84740_firmware_download
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
_bcm84740_mdio_firmware_download(phy_ctrl_t *pc,
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

    /* Request fast MDIO clock */
    (void)_bcm84740_fast_load_set(pc, 1);

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

    /* Restore MDIO clock */
    (void)_bcm84740_fast_load_set(pc, 0);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

/*
 * Function:
 *      _bcm84740_mdio_firmware_done
 * Purpose:
 *      MDIO firmware download post-processing.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84740_mdio_firmware_done(phy_ctrl_t *pc)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int lane, num_lanes;
    uint32_t data;
    uint32_t ucode_ver;

    num_lanes = BCM84740_SINGLE_PORT_MODE(pc) ? 4 : 1;

    for (lane = 0; lane < num_lanes; lane++) {

        if (phy_ctrl_change_inst(pc, lane, _bcm84740_inst) < 0) {
            return CDK_E_FAIL;
        }

        /* Read hand-shake message from microcontroller */
        ioerr += PHY_BUS_READ(pc, PMA_PMD_M8051_MSGOUT_REG, &data);
        _PHY_DBG(pc, ("MDIO download done: 0x%04"PRIx32"\n", data));

        /* Clear LASI status */
        ioerr += PHY_BUS_READ(pc, 0x9003, &data);

        /* Wait for LASI to be asserted when M8051 writes checksum to MSG_OUT */
        PHY_SYS_USLEEP(100);
        ioerr += PHY_BUS_READ(pc, PMA_PMD_M8051_MSGOUT_REG, &data);

        /* Verify checksum and print firmware version */
        ioerr += PHY_BUS_READ(pc, PMA_PMD_CE00_REG, &ucode_ver);
        ioerr += PHY_BUS_READ(pc, PMA_PMD_GEN_REG_4, &data);
        if (ioerr) {
            return CDK_E_IO;
        }
        if (data != BCM84740_MDIO_FW_CHKSUM) {
            PHY_WARN(pc, ("MDIO download: bad checksum 0x%04"PRIx32"\n", data));
            return CDK_E_FAIL;
        }
        PHY_VERB(pc, ("MDIO firmware version = 0x%04"PRIx32"\n", ucode_ver));
    }

    if (phy_ctrl_change_inst(pc, 0, _bcm84740_inst) < 0) {
        return CDK_E_FAIL;
    }

    /* Switch to configured port mode AFTER download */
    if (BCM84740_SINGLE_PORT_MODE(pc)) {
        rv = _bcm84740_quad_to_single_mode(pc);
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _bcm84740_rom_firmware_load
 * Purpose:
 *      Load firmware from SPI-ROM.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84740_rom_firmware_load(phy_ctrl_t *pc)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
#if PHY_CONFIG_EXTERNAL_BOOT_ROM == 1
    int lane;
    uint32_t data;

    /* Switch to configured port mode BEFORE download */
    if (BCM84740_SINGLE_PORT_MODE(pc)) {
        rv = _bcm84740_quad_to_single_mode(pc);
        if (CDK_FAILURE(rv)) {
            return rv;
        }
    }

    /* Set download size to 32K */
    if (BCM84740_SINGLE_PORT_MODE(pc)) {
        for (lane = 3; lane >= 0; lane--) {
            ioerr += PHY_BUS_WRITE(pc, PMA_PMD_AER_ADDR_REG, lane);

            ioerr += PHY_BUS_READ(pc, PMA_PMD_MISC_CTRL1_REG, &data);
            data |= PMA_PMD_MISC_CTRL1_SPI_DWLD_32K;
            ioerr += PHY_BUS_WRITE(pc, PMA_PMD_MISC_CTRL1_REG, data);
        }
    } else {
        ioerr += PHY_BUS_READ(pc, PMA_PMD_MISC_CTRL1_REG, &data);
        data |= PMA_PMD_MISC_CTRL1_SPI_DWLD_32K;
        ioerr += PHY_BUS_WRITE(pc, PMA_PMD_MISC_CTRL1_REG, data);
    }

    /* Set up for SPI download and ROM boot */
    ioerr += PHY_BUS_READ(pc, PMA_PMD_SPI_CTRL_REG, &data);
    data |= (PMA_PMD_SPI_CTRL_SPI_PORT_USED | PMA_PMD_SPI_CTRL_SPI_BOOT);
    data &= ~(PMA_PMD_SPI_CTRL_SPI_DWLD_DONE | PMA_PMD_SPI_CTRL_SPI_ENABLE);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_SPI_CTRL_REG, data);

    /* Apply software reset to download code from SPI-ROM */
    rv = _bcm84740_soft_reset(pc);
#endif

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _bcm84740_rom_firmware_wait
 * Purpose:
 *      SPI-ROM firmware load post-processing.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84740_rom_firmware_wait(phy_ctrl_t *pc)
{
    int ioerr = 0;
#if PHY_CONFIG_EXTERNAL_BOOT_ROM == 1
    int lane;
    int cnt;
    uint32_t data;
    uint32_t ucode_ver;

    for (lane = 3; lane >= 0; lane--) {

        if (BCM84740_SINGLE_PORT_MODE(pc)) {
            ioerr += PHY_BUS_WRITE(pc, PMA_PMD_AER_ADDR_REG, lane);
        }

        for (cnt = 0; cnt < PHY_DOWNLOAD_MSEC; cnt++) {
            ioerr += PHY_BUS_READ(pc, PMA_PMD_M8051_MSGOUT_REG, &data);
            if (data != 0) {
                break;
            }
            PHY_SYS_USLEEP(1000);
        }
        if (cnt >= PHY_DOWNLOAD_MSEC) {
            PHY_WARN(pc, ("download timeout\n"));
        }

        _PHY_DBG(pc, ("SPI-ROM download done msg 0x%"PRIx32"\n", data));

        /* Verify checksum and print firmware version */
        ioerr += PHY_BUS_READ(pc, PMA_PMD_CE00_REG, &ucode_ver);
        ioerr += PHY_BUS_READ(pc, PMA_PMD_GEN_REG_4, &data);
        if (ioerr) {
            return CDK_E_IO;
        }
        if (data != BCM84740_SPI_ROM_CHKSUM) {
            PHY_WARN(pc, ("SPI-ROM load: bad checksum 0x%04"PRIx32"\n", data));
            return CDK_E_FAIL;
        }
        PHY_VERB(pc, ("SPI-ROM firmware version = 0x%04"PRIx32"\n", ucode_ver));

        if (!BCM84740_SINGLE_PORT_MODE(pc)) {
            break;
        }
    }
#endif

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

/*
 * Function:
 *      _bcm84740_init_stage_0
 * Purpose:
 *      PHY init.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84740_init_stage_0(phy_ctrl_t *pc)
{
    int rv = CDK_E_NONE;

    _PHY_DBG(pc, ("init_stage_0\n"));

    if (PHY_CTRL_NEXT(pc)) {
        /* Adopt operating mode from upstream PHY */
        if (PHY_CTRL_FLAGS(PHY_CTRL_NEXT(pc)) & PHY_F_SERDES_MODE) {
            /* Enable quad port mode (single lane mode) */
            PHY_CTRL_FLAGS(pc) |= PHY_F_SERDES_MODE;
        }
        /* Request KR4 mode for 40G speeds */
        PHY_CTRL_LINE_INTF(PHY_CTRL_NEXT(pc)) = PHY_IF_KR;
    }

    PHY_CTRL_FLAGS(pc) |= PHY_F_FIBER_MODE;

    /* Enable broadcast mode if firmware download via MDIO */
    if (PHY_CTRL_FLAGS(pc) & PHY_F_FW_MDIO_LOAD) {
        rv = _bcm84740_broadcast_set(pc, 1);
    }

    return rv;
}

/*
 * Function:
 *      _bcm84740_init_stage_1
 * Purpose:
 *      PHY init.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84740_init_stage_1(phy_ctrl_t *pc)
{
    int rv = CDK_E_NONE;

    _PHY_DBG(pc, ("init_stage_1\n"));

    if (PHY_CTRL_FLAGS(pc) & PHY_F_FW_MDIO_LOAD) {
        rv = _bcm84740_firmware_download_prep(pc);
    } else {
        rv = _bcm84740_rom_firmware_load(pc);
    }

    return rv;
}

/*
 * Function:
 *      _bcm84740_init_stage_2
 * Purpose:
 *      PHY init.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84740_init_stage_2(phy_ctrl_t *pc)
{
    int rv = CDK_E_NONE;

    _PHY_DBG(pc, ("init_stage_2\n"));

    /* Enable broadcast mode if firmware download via MDIO */
    if (PHY_CTRL_FLAGS(pc) & PHY_F_FW_MDIO_LOAD) {
        rv = _bcm84740_broadcast_set(pc, 1);
    }

    return rv;
}

/*
 * Function:
 *      _bcm84740_init_stage_3
 * Purpose:
 *      PHY init.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84740_init_stage_3(phy_ctrl_t *pc)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;

    _PHY_DBG(pc, ("init_stage_3\n"));

    if (PHY_CTRL_FLAGS(pc) & PHY_F_FW_MDIO_LOAD) {
        rv = _bcm84740_mdio_firmware_download(pc, bcm84740_ucode,
                                              bcm84740_ucode_len);
        /* Disable broadcast mode */
        rv = _bcm84740_broadcast_set(pc, 0);
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _bcm84740_init_stage_4
 * Purpose:
 *      PHY init.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84740_init_stage_4(phy_ctrl_t *pc)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t data;

    _PHY_DBG(pc, ("init_stage_4\n"));

    if (PHY_CTRL_FLAGS(pc) & PHY_F_FW_MDIO_LOAD) {
        /* Verify MDIO firmware download */
        rv = _bcm84740_mdio_firmware_done(pc);
    } else {
        /* Wait for SPI-ROM load to finish */
        rv = _bcm84740_rom_firmware_wait(pc);
    }

    /* Set default DAC mode as SR4/LR4 */
    ioerr += PHY_BUS_READ(pc, PMA_PMD_CHIP_MODE_REG, &data);
    data &= ~PMA_PMD_DAC_MODE_MASK;
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_CHIP_MODE_REG, data);

    if (!(BCM84740_SINGLE_PORT_MODE(pc))) {
        /* Clear 1.0xcd17 to enable the PCS */
        ioerr += PHY_BUS_WRITE(pc, PMA_PMD_CD17_REG, 0x0);
    } else {
        /*
         * EDC mode for SR4/LR4 is set by firmware (not needed for CR4)
         * Force 40G and disable clause 72.
         */
        ioerr += PHY_BUS_WRITE(pc, PMA_PMD_0096_REG, 0x0);

        /* Disable auto-negotiation */
        ioerr += PHY_BUS_WRITE(pc, AN_CTRL_REG, 0x0);

    }

#if BCM84740_RXLOS_OVERRIDE_ENABLE
    if (BCM84740_SINGLE_PORT_MODE(pc)) {
        int lane;

        for (lane = 0; lane < 4; lane++) {
            ioerr += PHY_BUS_WRITE(pc, PMA_PMD_AER_ADDR_REG, lane);
            /* XXX temp 0xc0c0: RXLOS override: 0x0808 MOD_ABS override */
            ioerr += PHY_BUS_WRITE(pc, PMA_PMD_C8E4_REG, 0xc8c8);
        }
    } else {
        /* XXX temp 0xc0c0: RXLOS override: 0x0808 MOD_ABS override */
        ioerr += PHY_BUS_WRITE(pc, PMA_PMD_C8E4_REG, 0xc8c8);
    }
#endif

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _bcm84740_init_stage
 * Purpose:
 *      Execute specified init stage.
 * Parameters:
 *      pc - PHY control structure
 *      stage - init stage
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84740_init_stage(phy_ctrl_t *pc, int stage)
{
    switch (stage) {
    case 0:
        return _bcm84740_init_stage_0(pc);
    case 1:
        return _bcm84740_init_stage_1(pc);
    case 2:
        return _bcm84740_init_stage_2(pc);
    case 3:
        return _bcm84740_init_stage_3(pc);
    case 4:
        return _bcm84740_init_stage_4(pc);
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
 *      bcm84740_phy_probe
 * Purpose:     
 *      Probe for 84740 PHY
 * Parameters:
 *      pc - PHY control structure
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm84740_phy_probe(phy_ctrl_t *pc)
{
    uint32_t phyid0, phyid1;
    int ioerr = 0;

    PHY_CTRL_CHECK(pc);

    ioerr += PHY_BUS_READ(pc, PMA_PMD_ID0_REG, &phyid0);
    ioerr += PHY_BUS_READ(pc, PMA_PMD_ID1_REG, &phyid1);

    if (ioerr) {
        return CDK_E_IO;
    }

    if ((phyid0 == BCM84740_PMA_PMD_ID0) &&
        ((phyid1 & ~0xf) == (BCM84740_PMA_PMD_ID1 & ~0xf))) {
#if PHY_CONFIG_EXTERNAL_BOOT_ROM == 0
        /* Use MDIO download if external ROM is disabled */
        PHY_CTRL_FLAGS(pc) |= PHY_F_FW_MDIO_LOAD;
#endif
        /* Select quad mode for correct probing of subsequent lanes */
        return _bcm84740_single_to_quad_mode(pc);
    }

    return CDK_E_NOT_FOUND;
}

/*
 * Function:
 *      bcm84740_phy_notify
 * Purpose:     
 *      Handle PHY notifications
 * Parameters:
 *      pc - PHY control structure
 *      event - PHY event
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm84740_phy_notify(phy_ctrl_t *pc, phy_event_t event)
{
    int rv = CDK_E_NONE;

    PHY_CTRL_CHECK(pc);

    /* Call up the PHY chain */
    if (CDK_SUCCESS(rv)) {
        rv = PHY_NOTIFY(PHY_CTRL_NEXT(pc), event);
    }

    return rv;
}

/*
 * Function:
 *      bcm84740_phy_reset
 * Purpose:     
 *      Reset 84740 PHY
 * Parameters:
 *      pc - PHY control structure
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm84740_phy_reset(phy_ctrl_t *pc)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;

    PHY_CTRL_CHECK(pc);

    rv = _bcm84740_soft_reset(pc);

    /* Call up the PHY chain */
    if (CDK_SUCCESS(rv)) {
        rv = PHY_RESET(PHY_CTRL_NEXT(pc));
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      bcm84740_phy_init
 * Purpose:     
 *      Initialize 84740 PHY driver
 * Parameters:
 *      pc - PHY control structure
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm84740_phy_init(phy_ctrl_t *pc)
{
    int rv = CDK_E_NONE;
    int stage;

    PHY_CTRL_CHECK(pc);

    if (PHY_CTRL_FLAGS(pc) & PHY_F_STAGED_INIT) {
        PHY_CTRL_FLAGS(pc) &= ~PHY_F_STAGED_INIT;
    }

    for (stage = 0; CDK_SUCCESS(rv); stage++) {
        rv = _bcm84740_init_stage(pc, stage);
    }

    if (rv == CDK_E_UNAVAIL) {
        /* Successfully completed all stages */
        rv = CDK_E_NONE;
    }

    /* Call up the PHY chain */
    if (CDK_SUCCESS(rv)) {
        rv = PHY_INIT(PHY_CTRL_NEXT(pc));
    }

    return rv;
}

/*
 * Function:    
 *      bcm84740_phy_link_get
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
bcm84740_phy_link_get(phy_ctrl_t *pc, int *link, int *autoneg_done)
{
    uint32_t ctrl, stat, speed_val;
    int ioerr = 0, rv;
    int cur_speed, autoneg;

    PHY_CTRL_CHECK(pc);
    cur_speed = 0;
    *link = 0;

    ioerr += PHY_BUS_READ(pc, AN_MII_CTRL_REG, &ctrl);

    autoneg = (ctrl & AN_ENABLE);

    /* Check autoneg status before link status */
    if (autoneg_done) {
        ioerr += PHY_BUS_READ(pc, AN_MII_STAT_REG, &stat);
        *autoneg_done = (stat & AN_DONE);
    }

    if (BCM84740_SINGLE_PORT_MODE(pc)) {
        int serdes_link;
        rv = PHY_LINK_GET(PHY_CTRL_NEXT(pc), &serdes_link, NULL);
        if (CDK_FAILURE(rv)) {
            return CDK_E_FAIL;
        }
        ioerr += PHY_BUS_READ(pc, PMA_PMD_STAT_REG, &stat);
        *link = ((stat & MII_STAT_LA) && serdes_link) ? TRUE : FALSE;
        return CDK_E_NONE;
    }

    /* Return link false if in the middle of autoneg */
    if (autoneg == TRUE && autoneg_done == FALSE) {
        *link = FALSE;
        return CDK_E_NONE;
    }

    if (autoneg) {
        cur_speed = 1000;
    } else {
        ioerr += PHY_BUS_READ(pc, PMA_PMD_CTRL2_REG, &speed_val);
        speed_val &= PMA_PMD_CTRL2_PMA_TYPE_MASK;
        if (speed_val != PMA_PMD_CTRL2_PMA_TYPE_1G_KX) {
            cur_speed = 10000;
        }
    }

    if (cur_speed == 10000) {
        /* 10G link must be up in PMA/PMD and PCS */
        *link = 0;
        ioerr += PHY_BUS_READ(pc, PMA_PMD_STAT_REG, &stat);
        if (stat & MII_STAT_LA) {
            ioerr += PHY_BUS_READ(pc, PCS_STAT_REG, &stat);
            if (stat & MII_STAT_LA) {
                *link = 1;
            }
        }
    } else {
        /* Check 1G link only if no 10G link */
        if (*link == 0) {
            ioerr += PHY_BUS_READ(pc, AN_MII_STAT_REG, &stat);
            *link = ((stat & AN_LINK) != 0);
        }
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

/*
 * Function:    
 *      bcm84740_phy_duplex_set
 * Purpose:     
 *      Set the current duplex mode (forced).
 * Parameters:
 *      pc - PHY control structure
 *      duplex - non-zero indicates full duplex, zero indicates half
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm84740_phy_duplex_set(phy_ctrl_t *pc, int duplex)
{
    return (duplex != 0) ? CDK_E_NONE : CDK_E_PARAM;
}

/*
 * Function:    
 *      bcm84740_phy_duplex_get
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
bcm84740_phy_duplex_get(phy_ctrl_t *pc, int *duplex)
{
    *duplex = 1;

    return CDK_E_NONE;
}

/*
 * Function:    
 *      bcm84740_phy_speed_set
 * Purpose:     
 *      Set the current operating speed (forced).
 * Parameters:
 *      pc - PHY control structure
 *      speed - new link speed
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm84740_phy_speed_set(phy_ctrl_t *pc, uint32_t speed)
{
    uint32_t pma_pmd_ctrl, an_ctrl;
    uint32_t cur_speed;
    int lb, an;
    int ioerr = 0;
    int rv;

    PHY_CTRL_CHECK(pc);

    /* Check valid port speed */
    if (BCM84740_SINGLE_PORT_MODE(pc)) {
        switch (speed) {
        case 40000:
            PHY_CTRL_FLAGS(pc) |= PHY_F_PASSTHRU;
            break;
        default:
            return CDK_E_PARAM;
        }
    } else {
        switch (speed) {
        case 10000:
            PHY_CTRL_FLAGS(pc) &= ~PHY_F_PASSTHRU;
            break;
        case 1000:
            PHY_CTRL_FLAGS(pc) |= PHY_F_PASSTHRU;
            break;
        default:
            return CDK_E_PARAM;
        }
           
    }

    /* Call up the PHY chain */
    rv = PHY_SPEED_SET(PHY_CTRL_NEXT(pc), speed);
    if (CDK_FAILURE(rv)) {
        return rv;
    }

    /* Update loopback according to passthru mode */
    lb = 0;
    rv = PHY_LOOPBACK_GET(pc, &lb);
    if (CDK_SUCCESS(rv) && lb) {
        rv = PHY_LOOPBACK_SET(pc, 1);
    }
    if (CDK_FAILURE(rv)) {
        return rv;
    }

    /* Leave hardware alone if speed is unchanged */
    rv = PHY_SPEED_GET(pc, &cur_speed);
    if (CDK_SUCCESS(rv) && speed == cur_speed) {
        return CDK_E_NONE;
    }

    if (CDK_SUCCESS(rv)) {
        rv = PHY_AUTONEG_GET(pc, &an);
    }

    /* Leave hardware alone if auto-neg is enabled */
    if (CDK_SUCCESS(rv) && an == 0) {

        switch (speed) {
        case 40000:
            /* Single port mode */
            break;

        case 10000:
            /* Select 10G mode */
            ioerr += PHY_BUS_READ(pc, PMA_PMD_CTRL_REG, &pma_pmd_ctrl);
            pma_pmd_ctrl |= PMA_PMD_CTRL_SPEED_10G;
            ioerr += PHY_BUS_WRITE(pc, PMA_PMD_CTRL_REG, pma_pmd_ctrl);

            ioerr += PHY_BUS_WRITE(pc, PMA_PMD_CTRL2_REG,
                                   PMA_PMD_CTRL2_PMA_TYPE_10G_LRM);

            /* Restart auto-neg and wait */
            ioerr += PHY_BUS_READ(pc, AN_CTRL_REG, &an_ctrl);
            ioerr += PHY_BUS_WRITE(pc, AN_CTRL_REG,
                                   AN_ENABLE | AN_RESTART);
            PHY_SYS_USLEEP(40000);

            /* Restore auto-neg setting */
            ioerr += PHY_BUS_WRITE(pc, AN_CTRL_REG, an_ctrl);

            break;

        case 1000:
            /* Select 1G by-pass mode */
            ioerr += PHY_BUS_READ(pc, PMA_PMD_CTRL_REG, &pma_pmd_ctrl);
            pma_pmd_ctrl &= ~PMA_PMD_CTRL_SPEED_10G;
            ioerr += PHY_BUS_WRITE(pc, PMA_PMD_CTRL_REG, pma_pmd_ctrl);

            ioerr += PHY_BUS_WRITE(pc, PMA_PMD_CTRL2_REG,
                                   PMA_PMD_CTRL2_PMA_TYPE_1G_KX);
            break;
        default:
            break;
        }
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:    
 *      bcm84740_phy_speed_get
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
bcm84740_phy_speed_get(phy_ctrl_t *pc, uint32_t *speed)
{
    uint32_t pma_pmd_ctrl2, pma_type, link_stat;
    int an;
    int ioerr = 0;
    int rv;

    PHY_CTRL_CHECK(pc);

    *speed = 0;

    if (BCM84740_SINGLE_PORT_MODE(pc)) {
        /* Always 40G in single port mode */
        *speed = 40000;
        return CDK_E_NONE;
    }

    rv = PHY_AUTONEG_GET(pc, &an);
    if (CDK_SUCCESS(rv)) {
        if (an) {
            ioerr += PHY_BUS_READ(pc, AN_MII_STAT_REG, &link_stat);
            if (link_stat & AN_LINK) {
                *speed = 1000;
            }
        } else {
            ioerr += PHY_BUS_READ(pc, PMA_PMD_CTRL2_REG, &pma_pmd_ctrl2);
            pma_type = (pma_pmd_ctrl2 & PMA_PMD_CTRL2_PMA_TYPE_MASK);
            if (pma_type== PMA_PMD_CTRL2_PMA_TYPE_1G_KX) {
                *speed = 1000;
            } else {
                *speed = 10000;
            }
        }
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:    
 *      bcm84740_phy_autoneg_set
 * Purpose:     
 *      Enable or disable auto-negotiation on the specified port.
 * Parameters:
 *      pc - PHY control structure
 *      autoneg - non-zero enables autoneg, zero disables autoneg
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm84740_phy_autoneg_set(phy_ctrl_t *pc, int autoneg)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t mode, temp;

    rv = PHY_AUTONEG_SET(PHY_CTRL_NEXT(pc), FALSE);
    if (BCM84740_SINGLE_PORT_MODE(pc)) {
        ioerr += PHY_BUS_READ(pc, PMA_PMD_CHIP_MODE_REG, &mode);
        /* No auto-negotiation if in SR4/LR4 mode */
        if (!(mode & PMA_PMD_DAC_MODE_MASK)) {
            autoneg = 0;
        }
        rv = PHY_AUTONEG_SET(PHY_CTRL_NEXT(pc), autoneg);
        ioerr += PHY_BUS_WRITE(pc, PMA_PMD_0096_REG, autoneg ? 2:0);
        ioerr += PHY_BUS_WRITE(pc, AN_CTRL_REG, autoneg ? (AN_ENABLE | AN_RESTART) : 0);

        return ioerr ? CDK_E_IO : rv;
    }

    if (autoneg) {
        ioerr += PHY_BUS_WRITE(pc, AN_MII_CTRL_REG, MII_CTRL_AE | MII_CTRL_RAN);
        ioerr += PHY_BUS_READ(pc, AN_8309_REG, &temp);
        temp &= ~(1U << 5);
        ioerr += PHY_BUS_WRITE(pc, AN_8309_REG, temp);
    } else {
        ioerr += PHY_BUS_WRITE(pc, AN_MII_CTRL_REG, 0);
        ioerr += PHY_BUS_READ(pc, AN_8309_REG, &temp);
        temp |= (1U << 5);
        ioerr += PHY_BUS_WRITE(pc, AN_8309_REG, temp);
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:    
 *      bcm84740_phy_autoneg_get
 * Purpose:     
 *      Get the current auto-negotiation setting.
 * Parameters:
 *      pc - PHY control structure
 *      autoneg - (OUT) non-zero indicates autoneg enabled
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm84740_phy_autoneg_get(phy_ctrl_t *pc, int *autoneg)
{
    uint32_t ctrl;
    int ioerr = 0;
    int rv = CDK_E_NONE;

    if (BCM84740_SINGLE_PORT_MODE(pc)) {
        if (autoneg) {
            ioerr += PHY_BUS_READ(pc, AN_CTRL_REG, &ctrl);
            *autoneg = (ctrl & AN_ENABLE);
        }
    } else {
        if (autoneg) {
            ioerr += PHY_BUS_READ(pc, AN_MII_CTRL_REG, &ctrl);
            *autoneg = (ctrl & AN_ENABLE);
        }
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:    
 *      bcm84740_phy_loopback_set
 * Purpose:     
 *      Set the internal PHY loopback mode.
 * Parameters:
 *      pc - PHY control structure
 *      enable - non-zero enables PHY loopback
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm84740_phy_loopback_set(phy_ctrl_t *pc, int enable)
{
    uint32_t pma_pmd_ctrl;
    int ioerr = 0;
    int rv;
    int next_lb;

    next_lb = 0;
    if (PHY_CTRL_FLAGS(pc) & PHY_F_PASSTHRU) {
        next_lb = enable;
        enable = 0;
    }

    /* Set loopback on upstream PHY */
    rv = PHY_LOOPBACK_SET(PHY_CTRL_NEXT(pc), next_lb);

    /* Read loopback control registers */
    ioerr += PHY_BUS_READ(pc, PMA_PMD_CTRL_REG, &pma_pmd_ctrl);

    pma_pmd_ctrl &= ~PMA_PMD_CTRL_PMA_LOOPBACK;
    if (enable) {
        pma_pmd_ctrl |= PMA_PMD_CTRL_PMA_LOOPBACK;
    }

    /* Write updated loopback control registers */
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_CTRL_REG, pma_pmd_ctrl);

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:    
 *      bcm84740_phy_loopback_get
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
bcm84740_phy_loopback_get(phy_ctrl_t *pc, int *enable)
{
    uint32_t pma_pmd_ctrl;
    int ioerr = 0;
    int rv;

    *enable = 0;

    /* Get loopback of upstream PHY */
    rv = PHY_LOOPBACK_GET(PHY_CTRL_NEXT(pc), enable);

    if (*enable == 0) {
        /* Read loopback control registers */
        ioerr += PHY_BUS_READ(pc, PMA_PMD_CTRL_REG, &pma_pmd_ctrl);
        if (pma_pmd_ctrl & (1U << 0)) {
            *enable = 1;
        }
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:    
 *      bcm84740_phy_ability_get
 * Purpose:     
 *      Get the abilities of the PHY.
 * Parameters:
 *      pc - PHY control structure
 *      abil - (OUT) ability mask indicating supported options/speeds.
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm84740_phy_ability_get(phy_ctrl_t *pc, uint32_t *abil)
{
    PHY_CTRL_CHECK(pc);

    *abil = (PHY_ABIL_40GB | PHY_ABIL_10GB |
             PHY_ABIL_LOOPBACK | PHY_ABIL_1000MB_FD);

    return CDK_E_NONE;
}

/*
 * Function:
 *      bcm84740_phy_config_set
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
bcm84740_phy_config_set(phy_ctrl_t *pc, phy_config_t cfg, uint32_t val, void *cd)
{
    PHY_CTRL_CHECK(pc);

    switch (cfg) {
    case PhyConfig_Enable:
        return CDK_E_NONE;
    case PhyConfig_PortInterface:
        switch (val) {
        case PHY_IF_XFI:
            return CDK_E_NONE;
        default:
            break;
        }
        break;
    case PhyConfig_Mode:
        if (val == 0) {
            return CDK_E_NONE;
        }
        break;
    case PhyConfig_InitStage:
        if (PHY_CTRL_FLAGS(pc) & PHY_F_STAGED_INIT) {
            return _bcm84740_init_stage(pc, val);
        }
        break;
    case PhyConfig_Firmware:
        PHY_CTRL_FLAGS(pc) |= PHY_F_FW_MDIO_LOAD;
        return CDK_E_NONE;
    default:
        break;
    }

    return CDK_E_UNAVAIL;
}

/*
 * Function:
 *      bcm84740_phy_config_get
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
bcm84740_phy_config_get(phy_ctrl_t *pc, phy_config_t cfg, uint32_t *val, void *cd)
{
    PHY_CTRL_CHECK(pc);

    switch (cfg) {
    case PhyConfig_Enable:
        *val = 1;
        return CDK_E_NONE;
    case PhyConfig_PortInterface:
        *val = PHY_IF_KR;
        return CDK_E_NONE;
    case PhyConfig_Mode:
        *val = PHY_MODE_LAN;
        return CDK_E_NONE;
    case PhyConfig_Clause45Devs:
        *val = 0x9a;
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
 * Variable:    bcm84740_drv
 * Purpose:     PHY Driver for BCM84740.
 */
phy_driver_t bcm84740_drv = {
    "bcm84740",
    "BCM84740 40-Gigabit PHY Driver",  
    0,
    bcm84740_phy_probe,                  /* pd_probe */
    bcm84740_phy_notify,                 /* pd_notify */
    bcm84740_phy_reset,                  /* pd_reset */
    bcm84740_phy_init,                   /* pd_init */
    bcm84740_phy_link_get,               /* pd_link_get */
    bcm84740_phy_duplex_set,             /* pd_duplex_set */
    bcm84740_phy_duplex_get,             /* pd_duplex_get */
    bcm84740_phy_speed_set,              /* pd_speed_set */
    bcm84740_phy_speed_get,              /* pd_speed_get */
    bcm84740_phy_autoneg_set,            /* pd_autoneg_set */
    bcm84740_phy_autoneg_get,            /* pd_autoneg_get */
    bcm84740_phy_loopback_set,           /* pd_loopback_set */
    bcm84740_phy_loopback_get,           /* pd_loopback_get */
    bcm84740_phy_ability_get,            /* pd_ability_get */
    bcm84740_phy_config_set,             /* pd_config_set */
    bcm84740_phy_config_get,             /* pd_config_get */
    NULL,                                /* pd_status_get */
    NULL                                 /* pd_cable_diag */
};
