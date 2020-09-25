/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * PHY driver for BCM84858.
 */

#include <phy/phy.h>
#include <phy/ge_phy.h>
#include <cdk/cdk_debug.h>

extern unsigned char bcm8485x_ucode[];
extern unsigned int bcm8485x_ucode_len;

#define IS_COPPER_MODE(pc)      (!(PHY_CTRL_FLAGS(pc) & PHY_F_FIBER_MODE))
#define IS_FIBER_MODE(pc)       (PHY_CTRL_FLAGS(pc) & PHY_F_FIBER_MODE)

/* Private PHY flag is used to indicate firmware download method */
#define PHY_F_FW_MDIO_LOAD                      PHY_F_PRIVATE

#define FIRMWARE_BLK_SIZE                       4

#define PHY_RESET_POLL_MAX                      10
#define PHY_LINK_POLL_MAX                       1000
#define PHY_CMD_POLL_MAX                        1000

#define PHY_ID1_REV_MASK                        0x000f

#define BCM84846_CTRL_SPEED_10G                 (MII_CTRL_SS_LSB | MII_CTRL_SS_MSB)
#define BCM84846_CTRL_SPEED_10G_MASK            0x003c

#define BCM84858_PMA_PMD_ID0                    0x600d
#define BCM84858_PMA_PMD_ID1                    0x8560

#define BCM8484X_C45_DEV_PMA_PMD                MII_C45_DEV_PMA_PMD
#define BCM8484X_C45_DEV_PCS                    MII_C45_DEV_PCS
#define BCM8484X_C45_DEV_AN                     MII_C45_DEV_AN
#define BCM8484X_C45_DEV_PHYXS_M                MII_C45_DEV_PHY_XS
#define BCM8484X_C45_DEV_PHYXS_L                0x03
#define BCM8484X_C45_DEV_TOPLVL                 0x1e

#define C45_DEVAD(_a)                           LSHIFT32((_a),16)
#define DEVAD_PMA_PMD                           C45_DEVAD(BCM8484X_C45_DEV_PMA_PMD)
#define DEVAD_PCS                               C45_DEVAD(BCM8484X_C45_DEV_PCS)
#define DEVAD_AN                                C45_DEVAD(BCM8484X_C45_DEV_AN)
#define DEVAD_PHY_XS_M                          C45_DEVAD(BCM8484X_C45_DEV_PHYXS_M)
#define DEVAD_PHY_XS_L                          C45_DEVAD(BCM8484X_C45_DEV_PHYXS_L)
#define DEVAD_TOPLVL                            C45_DEVAD(BCM8484X_C45_DEV_TOPLVL)

/***************************************
 * Top level registers 
 */
#define TOPLVL_AN_CTRL_REG                      (DEVAD_TOPLVL + 0x0000)
#define TOPLVL_MIDO_CMD_REG                     (DEVAD_TOPLVL + 0x4005)
#define TOPLVL_PAIR_SWAP_CTRL_REG               (DEVAD_TOPLVL + 0x4009)
#define TOPLVL_STATUS_REG                       (DEVAD_TOPLVL + 0x400d)

/* Status register: chip revision [15:14] */
#define STATUS_SPIROM_CRC_CHECK_MASK            0xc000
#define STATUS_SPIROM_CRC_CHECK_ALIGN           0
#define STATUS_SPIROM_CRC_CHECK_BITS            2
#define STATUS_SPIROM_CRC_CHECK_SHIFT           14
/* Status register: mac side link [13] */
#define STATUS_MAC_SIDE_LINK_MASK               0x2000
#define STATUS_MAC_SIDE_LINK_ALIGN              0
#define STATUS_MAC_SIDE_LINK_BITS               1
#define STATUS_MAC_SIDE_LINK_SHIFT              13
/* Status register: fiber link [8] */
#define STATUS_FIBER_LINK_MASK                  0x0100
#define STATUS_FIBER_LINK_ALIGN                 0
#define STATUS_FIBER_LINK_BITS                  1
#define STATUS_FIBER_LINK_SHIFT                 8
/* Status register: fiber priority [2] */
#define STATUS_FIBER_PRIORITY_MASK              0x0004
#define STATUS_FIBER_PRIORITY_ALIGN             0
#define STATUS_FIBER_PRIORITY_BITS              1
#define STATUS_FIBER_PRIORITY_SHIFT             2
/* Status register: copper detected [1] */
#define STATUS_COPPER_DETECTED_MASK             0x0002
#define STATUS_COPPER_DETECTED_ALIGN            0
#define STATUS_COPPER_DETECTED_BITS             1
#define STATUS_COPPER_DETECTED_SHIFT            1
/* Status register: fiber detected [0] */
#define STATUS_FIBER_DETECTED_MASK              0x0001
#define STATUS_FIBER_DETECTED_ALIGN             0
#define STATUS_FIBER_DETECTED_BITS              1
#define STATUS_FIBER_DETECTED_SHIFT             0

#define TOPLVL_FIRMWARE_REV_REG                 (DEVAD_TOPLVL + 0x400f)
/* Firmware revision register: chip revision [15:12] */
#define FIRMWARE_REV_CHIP_REVISION_MASK         0xf000
#define FIRMWARE_REV_CHIP_REVISION_ALIGN        0
#define FIRMWARE_REV_CHIP_REVISION_BITS         4
#define FIRMWARE_REV_CHIP_REVISION_SHIFT        12
/* Firmware revision register: main [11:7] */
#define FIRMWARE_REV_MAIN_MASK                  0x0f80
#define FIRMWARE_REV_MAIN_ALIGN                 0
#define FIRMWARE_REV_MAIN_BITS                  5
#define FIRMWARE_REV_MAIN_SHIFT                 7
/* Firmware revision register: branch [6:0] */
#define FIRMWARE_REV_BRANCH_MASK                0x007f
#define FIRMWARE_REV_BRANCH_ALIGN               0
#define FIRMWARE_REV_BRANCH_BITS                7
#define FIRMWARE_REV_BRANCH_SHIFT               0
/* AN Control Register */
#define MGBASE_AN_CTRL_REG               0x0000
#define MGBASE_AN_CTRL_SPEED_5G         (1U << 6)
#define MGBASE_AN_CTRL_SPEED_2P5G       (1U << 5)
#define MGBASE_AN_CTRL_ADV_5G_EEE       (1U << 4)
#define MGBASE_AN_CTRL_ADV_2P5G_EEE     (1U << 3)
#define MGBASE_AN_CTRL_ADV_5G           (1U << 2)
#define MGBASE_AN_CTRL_ADV_2P5G         (1U << 1)
#define MGBASE_AN_CTRL_MG_ENABLE        (1U << 0)
#define MGBASE_AN_CTRL_SPEED_MASK  \
                        (MGBASE_AN_CTRL_SPEED_5G | MGBASE_AN_CTRL_SPEED_2P5G)
#define MGBASE_AN_CTRL_ADV_MASK  \
                        (MGBASE_AN_CTRL_ADV_5G   | MGBASE_AN_CTRL_ADV_2P5G)
                        
#define TOPLVL_FIRMWARE_DATA_REG                (DEVAD_TOPLVL + 0x4010)
#define TOPLVL_CONFIG_STRAP_REG                 (DEVAD_TOPLVL + 0x401a)
/* Config strap pins register: super isolate [15] */
#define CONFIG_STRAP_SUPER_ISOLATE_MASK         0x8000
#define CONFIG_STRAP_SUPER_ISOLATE_ALIGN        0
#define CONFIG_STRAP_SUPER_ISOLATE_BITS         1
#define CONFIG_STRAP_SUPER_ISOLATE_SHIFT        15
/* Config strap pins register: config13 [13] */
#define CONFIG_STRAP_CONFIG13_MASK              0x2000
#define CONFIG_STRAP_CONFIG13_ALIGN             0
#define CONFIG_STRAP_CONFIG13_BITS              1
#define CONFIG_STRAP_CONFIG13_SHIFT             13
/* Config strap pins register: config12 [12] */
#define CONFIG_STRAP_CONFIG12_MASK              0x1000
#define CONFIG_STRAP_CONFIG12_ALIGN             0
#define CONFIG_STRAP_CONFIG12_BITS              1
#define CONFIG_STRAP_CONFIG12_SHIFT             12
/* Config strap pins register: XGPH disabled [7] */
#define CONFIG_STRAP_XGPH_DISABLED_MASK         0x0080
#define CONFIG_STRAP_XGPH_DISABLED_ALIGN        0
#define CONFIG_STRAP_XGPH_DISABLED_BITS         1
#define CONFIG_STRAP_XGPH_DISABLED_SHIFT        7

#define TOPLVL_MDIO_CMD_STATUS_REG              (DEVAD_TOPLVL + 0x4037)
#define TOPLVL_MDIO_CMD_DATA1_REG               (DEVAD_TOPLVL + 0x4038)
#define TOPLVL_MDIO_CMD_DATA2_REG               (DEVAD_TOPLVL + 0x4039)
#define TOPLVL_MDIO_CMD_DATA3_REG               (DEVAD_TOPLVL + 0x403a)
#define TOPLVL_MDIO_CMD_DATA4_REG               (DEVAD_TOPLVL + 0x403b)
#define TOPLVL_MDIO_CMD_DATA5_REG               (DEVAD_TOPLVL + 0x403c)
#define TOPLVL_MDIO_CTRL1_REG                   (DEVAD_TOPLVL + 0x4111)
#define TOPLVL_0X410E_REG                       (DEVAD_TOPLVL + 0x410e)
#define TOPLVL_0X411E_REG                       (DEVAD_TOPLVL + 0x411e)
#define TOPLVL_0X4186_REG                       (DEVAD_TOPLVL + 0x4186)
#define TOPLVL_0X4188_REG                       (DEVAD_TOPLVL + 0x4188)
#define TOPLVL_0X4181_REG                       (DEVAD_TOPLVL + 0x4181)
#define TOPLVL_0X418C_REG                       (DEVAD_TOPLVL + 0x418C)
#define TOPLVL_0X4117_REG                       (DEVAD_TOPLVL + 0x4117)
#define TOPLVL_0X4107_REG                       (DEVAD_TOPLVL + 0x4107)
#define TOPLVL_0X8004_REG                       (DEVAD_TOPLVL + 0x8004)


/***************************************
 * PMA/PMD registers 
 */
#define PMA_PMD_CTRL1_REG                       (DEVAD_PMA_PMD + MII_CTRL_REG)
/* Control register: speed 10g [5:2] */
#define CTRL1_SPEED_10G_MASK                    0x003c
#define CTRL1_SPEED_10G_ALIGN                   0
#define CTRL1_SPEED_10G_BITS                    4
#define CTRL1_SPEED_10G_SHIFT                   2

#define PMA_PMD_ID0_REG                         (DEVAD_PMA_PMD + MII_PHY_ID0_REG)
#define PMA_PMD_ID1_REG                         (DEVAD_PMA_PMD + MII_PHY_ID1_REG)
#define PMA_PMD_DOWNLOAD_CTRL_REG               (DEVAD_PMA_PMD + 0xa817)
/* Download process control: reserved0 [15:6] */
#define DOWNLOAD_CTRL_RESERVED0_MASK            0xffc0
#define DOWNLOAD_CTRL_RESERVED0_ALIGN           0
#define DOWNLOAD_CTRL_RESERVED0_BITS            10
#define DOWNLOAD_CTRL_RESERVED0_SHIFT           6
/* Download process control: SELF_INC_ADDR 5] */
#define DOWNLOAD_CTRL_SELF_INC_ADDR_MASK        0x0020
#define DOWNLOAD_CTRL_SELF_INC_ADDR_ALIGN       0
#define DOWNLOAD_CTRL_SELF_INC_ADDR_BITS        1
#define DOWNLOAD_CTRL_SELF_INC_ADDR_SHIFT       5
/* Download process control: BURST [4] */
#define DOWNLOAD_CTRL_BURST_MASK                0x0010
#define DOWNLOAD_CTRL_BURST_ALIGN               0
#define DOWNLOAD_CTRL_BURST_BITS                1
#define DOWNLOAD_CTRL_BURST_SHIFT               4
/* Download process control: SIZE [3:2] */
#define DOWNLOAD_CTRL_SIZE_MASK                 0x000c
#define DOWNLOAD_CTRL_SIZE_ALIGN                0
#define DOWNLOAD_CTRL_SIZE_BITS                 2
#define DOWNLOAD_CTRL_SIZE_SHIFT                2
/* Download process control: RD [1] */
#define DOWNLOAD_CTRL_RD_MASK                   0x0002
#define DOWNLOAD_CTRL_RD_ALIGN                  0
#define DOWNLOAD_CTRL_RD_BITS                   1
#define DOWNLOAD_CTRL_RD_SHIFT                  1
/* Download process control: WR [0] */
#define DOWNLOAD_CTRL_WR_MASK                   0x0001
#define DOWNLOAD_CTRL_WR_ALIGN                  0
#define DOWNLOAD_CTRL_WR_BITS                   1
#define DOWNLOAD_CTRL_WR_SHIFT                  0

#define PMA_PMD_0XA008_REG                      (DEVAD_PMA_PMD + 0xa008)
#define PMA_PMD_DOWNLOAD_STAT_REG               (DEVAD_PMA_PMD + 0xa818)
#define PMA_PMD_DOWNLOAD_ADDR_LOW_REG           (DEVAD_PMA_PMD + 0xa819)
#define PMA_PMD_DOWNLOAD_ADDR_HIGH_REG          (DEVAD_PMA_PMD + 0xa81a)
#define PMA_PMD_DOWNLOAD_DATA_LOW_REG           (DEVAD_PMA_PMD + 0xa81b)
#define PMA_PMD_DOWNLOAD_DATA_HIGH_REG          (DEVAD_PMA_PMD + 0xa81c)
#define PMA_PMD_LED1_MASK_LOW_REG               (DEVAD_PMA_PMD + 0xa82c)
#define PMA_PMD_LED1_BLINK_REG                  (DEVAD_PMA_PMD + 0xa82e)
#define PMA_PMD_LED2_MASK_LOW_REG               (DEVAD_PMA_PMD + 0xa82f)
#define PMA_PMD_LED2_BLINK_REG                  (DEVAD_PMA_PMD + 0xa831)
#define PMA_PMD_LED3_MASK_LOW_REG               (DEVAD_PMA_PMD + 0xa832)
#define PMA_PMD_LED3_BLINK_REG                  (DEVAD_PMA_PMD + 0xa834)
#define PMA_PMD_LED4_MASK_LOW_REG               (DEVAD_PMA_PMD + 0xa835)
#define PMA_PMD_LED4_BLINK_REG                  (DEVAD_PMA_PMD + 0xa837)
#define PMA_PMD_LED5_MASK_LOW_REG               (DEVAD_PMA_PMD + 0xa838)
#define PMA_PMD_LED5_BLINK_REG                  (DEVAD_PMA_PMD + 0xa83a)
#define PMA_PMD_LED_CTRL_REG                    (DEVAD_PMA_PMD + 0xa83b)
#define PMA_PMD_LED_SOURCE_REG                  (DEVAD_PMA_PMD + 0xa83c)
#define PMA_PMD_AUTOEEE_CTRL_REG                (DEVAD_PMA_PMD + 0xa88a)
#define PMA_PMD_HG2_ENABLE_REG                  (DEVAD_PMA_PMD + 0xa939)

/***************************************
 * PCS registers
 */
#define PCS_CTRL_REG                            (DEVAD_PCS + MII_CTRL_REG)
#define PCS_STAT_REG                            (DEVAD_PCS + MII_STAT_REG)

/***************************************
 * AN registers
 */
#define AN_CTRL_REG                             (DEVAD_AN + MII_CTRL_REG)
#define AN_TENG_CTRL_REG                        (DEVAD_AN + 0x0020)
/* 10G Base-T ability: 10gbaset [12] */
#define TENG_CTRL_10GBASET_MASK                 0x1000
#define TENG_CTRL_10GBASET_ALIGN                0
#define TENG_CTRL_10GBASET_BITS                 1
#define TENG_CTRL_10GBASET_SHIFT                12

#define AN_TENG_STAT_REG                        (DEVAD_AN + 0x0021)
/* 10G Base-T ability: lp 10gbaset [11] */
#define TENG_STATUS_LP_10GBASET_MASK            0x0800
#define TENG_STATUS_LP_10GBASET_ALIGN           0
#define TENG_STATUS_LP_10GBASET_BITS            1
#define TENG_STATUS_LP_10GBASET_SHIFT           11

#define AN_EEE_ADVERT_REG                       (DEVAD_AN + 0x003c)
#define AN_EEE_LP_ADVERT_REG                    (DEVAD_AN + 0x003d)
/* 10G Base-T EEE: 10gbaset [3] */
#define EEE_ADVERT_10GBASET_MASK                0x0008
#define EEE_ADVERT_10GBASET_ALIGN               0
#define EEE_ADVERT_10GBASET_BITS                1
#define EEE_ADVERT_10GBASET_SHIFT               3
/* 1000 Base-T EEE: 1000baset [2] */
#define EEE_ADVERT_1000BASET_MASK               0x0004
#define EEE_ADVERT_1000BASET_ALIGN              0
#define EEE_ADVERT_1000BASET_BITS               1
#define EEE_ADVERT_1000BASET_SHIFT              2
/* 100 Base-T EEE: 100baset [1] */
#define EEE_ADVERT_100BASET_MASK                0x0002
#define EEE_ADVERT_100BASET_ALIGN               0
#define EEE_ADVERT_100BASET_BITS                1
#define EEE_ADVERT_100BASET_SHIFT               1

/**********************************************
 * 1000BASE-T/100BASE-TX registers for DEV AN
 */
#define D7_FFXX_MII_CTRL_REG                    (DEVAD_AN + 0xffe0)
#define D7_FFXX_MII_STAT_REG                    (DEVAD_AN + 0xffe1)
#define D7_FFXX_MII_ANA_REG                     (DEVAD_AN + 0xffe4)
#define D7_FFXX_MII_ANP_REG                     (DEVAD_AN + 0xffe5)
#define D7_FFXX_MII_GB_CTRL_REG                 (DEVAD_AN + 0xffe9)
#define D7_FFXX_MII_GB_STAT_REG                 (DEVAD_AN + 0xffea)
#define D7_FFXX_MII_IEEE_EXTEND_STAT_REG        (DEVAD_AN + 0xffef)
#define D7_FFXX_MII_PHY_EXTEND_STAT_REG         (DEVAD_AN + 0xfff1)

/*****************************************************
 * 1000BASE-T/100BASE-TX registers for DEV PHY_XS_L
 */
#define PHYXS_L_CTRL_REG                        (DEVAD_PHY_XS_L + MII_CTRL_REG)
#define PHYXS_L_GP_STATUS_1000X1_REG            (DEVAD_PHY_XS_L + 0x8124)
/* GP 1000X1 Status: sgmii_mode [0] */
#define GP_STATUS_1000X1_SGMII_MODE_MASK        0x0001
#define GP_STATUS_1000X1_SGMII_MODE_ALIGN       0
#define GP_STATUS_1000X1_SGMII_MODE_BITS        1
#define GP_STATUS_1000X1_SGMII_MODE_SHIFT       0

#define PHYXS_L_GP_STATUS_XGSX3_REG             (DEVAD_PHY_XS_L + 0x8129)
/* GP XGXS3 Status: link [15] */
#define GP_STATUS_XGXS3_LINK_MASK               0x8000
#define GP_STATUS_XGXS3_LINK_ALIGN              0
#define GP_STATUS_XGXS3_LINK_BITS               1
#define GP_STATUS_XGXS3_LINK_SHIFT              15

#define PHYXS_L_GP_STATUS_LP_UP1_REG            (DEVAD_PHY_XS_L + 0x812c)
/* Over1G LP UP1: data rate 10G [4] */
#define LP_UP1_DATARATE_10GCX4_MASK             0x0010
#define LP_UP1_DATARATE_10GCX4_ALIGN            0
#define LP_UP1_DATARATE_10GCX4_BITS             1
#define LP_UP1_DATARATE_10GCX4_SHIFT            4

#define PHYXS_L_OVER1G_UP1_REG                  (DEVAD_PHY_XS_L + 0x8329)
/* Over 1G user page: data rate 10GCX4 [4] */
#define OVER1G_UP1_DATARATE_10GCX4_MASK          0x0010
#define OVER1G_UP1_DATARATE_10GCX4_ALIGN         0
#define OVER1G_UP1_DATARATE_10GCX4_BITS          1
#define OVER1G_UP1_DATARATE_10GCX4_SHIFT         4

#define PHYXS_L_COMBO_IEEE0_MII_CTRL_REG        (DEVAD_PHY_XS_L + 0xffe0)
#define PHYXS_L_COMBO_IEEE0_MII_STAT_REG        (DEVAD_PHY_XS_L + 0xffe1)
#define PHYXS_L_COMBO_IEEE0_MII_ANA_REG         (DEVAD_PHY_XS_L + 0xffe4)
#define PHYXS_L_COMBO_IEEE0_MII_ANP_REG         (DEVAD_PHY_XS_L + 0xffe5)

/***************************************
 * MDIO Command Handler Version 2 
 */
#define BCM8484X_CMD_RECEIVED                           0x0001
#define BCM8484X_CMD_IN_PROGRESS                        0x0002
#define BCM8484X_CMD_COMPLETE_PASS                      0x0004
#define BCM8484X_CMD_COMPLETE_ERROR                     0x0008
#define BCM8484X_CMD_OPEN_FOR_CMDS                      0x0010
#define BCM8484X_CMD_SYSTEM_BOOT                        0x0020
#define BCM8484X_CMD_NOT_OPEN_FOR_CMDS                  0x0040
#define BCM8484X_CMD_CLEAR_COMPLETE                     0x0080
#define BCM8484X_CMD_OPEN_OVERRIDE                      0xA5A5

#define BCM8485X_CMD_SYSTEM_BUSY                        0xBBBB
#define BCM8485X_CMD_OPEN_FOR_CMDS               BCM8484X_CMD_COMPLETE_PASS
#define BCM8485X_CMD_IN_PROGRESS                        0x0002
#define BCM8485X_CMD_COMPLETE_ERROR                     0x0008

#define BCM8484X_DIAG_CMD_NOP                           0x0000
#define BCM8484X_DIAG_CMD_GET_PAIR_SWAP                 0x8000
#define BCM8484X_DIAG_CMD_SET_PAIR_SWAP                 0x8001
#define BCM8484X_DIAG_CMD_GET_MACSEC_ENABLE             0x8002                            
#define BCM8484X_DIAG_CMD_SET_MACSEC_ENABLE             0x8003                            
#define BCM8484X_DIAG_CMD_GET_1588_ENABLE               0x8004                            
#define BCM8484X_DIAG_CMD_SET_1588_ENABLE               0x8005                           
#define BCM8484X_DIAG_CMD_GET_SHORT_REACH_ENABLE        0x8006
#define BCM8484X_DIAG_CMD_SET_SHORT_REACH_ENABLE        0x8007
#define BCM8484X_DIAG_CMD_GET_EEE_MODE                  0x8008
#define BCM8484X_DIAG_CMD_SET_EEE_MODE                  0x8009
#define BCM8484X_DIAG_CMD_GET_EMI_MODE_ENABLE           0x800a
#define BCM8484X_DIAG_CMD_SET_EMI_MODE_ENABLE           0x800b
#define BCM84834_DIAG_CMD_SET_XFI_2P5G_MODE             0x8017
#define BCM8484X_DIAG_CMD_GET_SNR                       0x8030
#define BCM8484X_DIAG_CMD_GET_CURRENT_TEMP              0x8031
#define BCM8484X_DIAG_CMD_SET_UPPER_TEMP_WARNING_LEVEL  0x8032
#define BCM8484X_DIAG_CMD_GET_UPPER_TEMP_WARNING_LEVEL  0x8033
#define BCM8484X_DIAG_CMD_SET_LOWER_TEMP_WARNING_LEVEL  0x8034
#define BCM8484X_DIAG_CMD_GET_LOWER_TEMP_WARNING_LEVEL  0x8035
#define BCM8484X_DIAG_CMD_GET_XAUI_M_REG_VALUES         0x8100
#define BCM8484X_DIAG_CMD_SET_XAUI_M_REG_VALUES         0x8101
#define BCM8484X_DIAG_CMD_GET_XFI_M_REG_VALUES          0x8102
#define BCM8484X_DIAG_CMD_SET_XFI_M_REG_VALUES          0x8103
#define BCM8484X_DIAG_CMD_GET_XAUI_L_REG_VALUES         0x8104
#define BCM8484X_DIAG_CMD_SET_XAUI_L_REG_VALUES         0x8105
#define BCM8484X_DIAG_CMD_GET_XFI_L_REG_VALUES          0x8106
#define BCM8484X_DIAG_CMD_SET_XFI_L_REG_VALUES          0x8107
#define BCM8484X_DIAG_CMD_PEEK_WORD                     0xc000
#define BCM8484X_DIAG_CMD_POKE_WORD                     0xc001
#define BCM8484X_DIAG_CMD_GET_DATA_BUF_ADDRESSES        0xc002

#define PHY8484X_DIAG_CMD_PAIR_SWAP_CHANGE                   2

/* Low level debugging (off by default) */
#ifdef PHY_DEBUG_ENABLE
#define _PHY_DBG(_pc, _stuff) \
    PHY_VERB(_pc, _stuff)
#else
#define _PHY_DBG(_pc, _stuff)
#endif

static int is_phy_repeater = 0;
/***********************************************************************
 *
 * HELPER FUNCTIONS
 */
/*
 * Function:
 *      _bcm84858_top_level_cmd_set
 * Purpose:
 *      Implementation for the MDIO command sequence
 * Parameters:
 *      pc - PHY control structure
 *      cmd - Command ID
 *      arg - Command parameters
 *      size - Number of the parameters
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84858_top_level_cmd_set(phy_ctrl_t *pc, uint32_t cmd, uint32_t arg[], int size)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t data, cnt;

    if ((size < 1) || (size > 5)) {
        return CDK_E_PARAM;
    }

    /* Wait for the system is ready to accept commands */
    for (cnt = 0; cnt < PHY_CMD_POLL_MAX; cnt++) {
        ioerr += PHY_BUS_READ(pc, TOPLVL_MDIO_CMD_STATUS_REG, &data);
        if ((data != BCM8485X_CMD_SYSTEM_BUSY) &
             !(data == BCM8485X_CMD_IN_PROGRESS)) {
            break;
        }
        PHY_SYS_USLEEP(10000);
    }
    if (cnt >= PHY_CMD_POLL_MAX) {
        PHY_WARN(pc, ("wait system timeout\n"));
        rv = CDK_E_TIMEOUT;
    }

    if (size-- > 0) {
        if (cmd == BCM8484X_DIAG_CMD_SET_PAIR_SWAP) {
            ioerr += PHY_BUS_WRITE(pc, TOPLVL_MDIO_CMD_DATA2_REG, arg[0]);
        } else {
            ioerr += PHY_BUS_WRITE(pc, TOPLVL_MDIO_CMD_DATA1_REG, arg[0]);
        }
    }
    if (size-- > 0) {
        ioerr += PHY_BUS_WRITE(pc, TOPLVL_MDIO_CMD_DATA2_REG, arg[1]);
    }
    if (size-- > 0) {
        ioerr += PHY_BUS_WRITE(pc, TOPLVL_MDIO_CMD_DATA3_REG, arg[2]);
    }
    if (size-- > 0) {
        ioerr += PHY_BUS_WRITE(pc, TOPLVL_MDIO_CMD_DATA4_REG, arg[3]);
    }
    if (size-- > 0) {
        ioerr += PHY_BUS_WRITE(pc, TOPLVL_MDIO_CMD_DATA5_REG, arg[4]);
    }

    ioerr += PHY_BUS_WRITE(pc, TOPLVL_MIDO_CMD_REG, cmd);

    /* Wait for the system is ready to accept commands */
    for (cnt = 0; cnt < PHY_CMD_POLL_MAX; cnt++) {
        ioerr += PHY_BUS_READ(pc, TOPLVL_MDIO_CMD_STATUS_REG, &data);
        if ((data != BCM8485X_CMD_SYSTEM_BUSY) &
             !(data == BCM8485X_CMD_IN_PROGRESS)) {
            break; 
        }
        PHY_SYS_USLEEP(10000);
    }
    if (cnt >= PHY_CMD_POLL_MAX) {
        PHY_WARN(pc, ("wait system timeout\n"));
        rv = CDK_E_TIMEOUT;
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _bcm84858_top_level_cmd_get
 * Purpose:
 *      Implementation for the MDIO command sequence
 * Parameters:
 *      pc - PHY control structure
 *      cmd - Command ID
 *      arg - Command parameters
 *      size - Number of the parameters
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84858_top_level_cmd_get(phy_ctrl_t *pc, uint32_t cmd, uint32_t arg[], int size)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t data, cnt;

    if ((size < 1) || (size > 5)) {
        return CDK_E_PARAM;
    }

    for (cnt = 0; cnt < PHY_CMD_POLL_MAX; cnt++) {
        ioerr += PHY_BUS_READ(pc, TOPLVL_MDIO_CMD_STATUS_REG, &data);
        if ((data != BCM8484X_CMD_IN_PROGRESS)) {
            break;
        }
        PHY_SYS_USLEEP(10000);
    }
    if (cnt >= PHY_CMD_POLL_MAX) {
        PHY_WARN(pc, ("wait system timeout\n"));
        rv = CDK_E_TIMEOUT;
    }

    ioerr += PHY_BUS_WRITE(pc, TOPLVL_MIDO_CMD_REG, cmd);

    /* Wait for the system is ready to accept commands */
    for (cnt = 0; cnt < PHY_CMD_POLL_MAX; cnt++) {
        ioerr += PHY_BUS_READ(pc, TOPLVL_MDIO_CMD_STATUS_REG, &data);
        if ((data != BCM8485X_CMD_SYSTEM_BUSY) &
           !(data == BCM8485X_CMD_IN_PROGRESS)) {
           break;
        }
        PHY_SYS_USLEEP(10000);
    }
    if (cnt >= PHY_CMD_POLL_MAX) {
        PHY_WARN(pc, ("wait system timeout\n"));
        rv = CDK_E_TIMEOUT;
    }

    if (size-- > 0) {
        if (cmd == BCM8484X_DIAG_CMD_GET_PAIR_SWAP) {
            ioerr += PHY_BUS_READ(pc, TOPLVL_MDIO_CMD_DATA2_REG, &arg[0]);
        } else {
            ioerr += PHY_BUS_READ(pc, TOPLVL_MDIO_CMD_DATA1_REG, &arg[0]);
        }
    }
    if (size-- > 0) {
        ioerr += PHY_BUS_READ(pc, TOPLVL_MDIO_CMD_DATA2_REG, &arg[1]);
    }
    if (size-- > 0) {
        ioerr += PHY_BUS_READ(pc, TOPLVL_MDIO_CMD_DATA3_REG, &arg[2]);
    }
    if (size-- > 0) {
        ioerr += PHY_BUS_READ(pc, TOPLVL_MDIO_CMD_DATA4_REG, &arg[3]);
    }
    if (size-- > 0) {
        ioerr += PHY_BUS_READ(pc, TOPLVL_MDIO_CMD_DATA5_REG, &arg[4]);
    }
    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _bcm84858_halt
 * Purpose:
 *      .
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84858_halt(phy_ctrl_t *pc)
{
    int ioerr = 0;
    uint32_t data;

    /* LED control stuff */
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_LED1_MASK_LOW_REG, 0xffff);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_LED1_BLINK_REG, 0x0000);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_LED2_MASK_LOW_REG, 0x0000);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_LED2_BLINK_REG, 0x0000);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_LED3_MASK_LOW_REG, 0x0000);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_LED3_BLINK_REG, 0x0000);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_LED4_MASK_LOW_REG, 0x0000);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_LED4_BLINK_REG, 0x0000);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_LED5_MASK_LOW_REG, 0x0000);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_LED5_BLINK_REG, 0x0000);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_LED_CTRL_REG, 0xb6db);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_LED_SOURCE_REG, 0xffff);

      
    if (PHY_CTRL_FLAGS(pc) & PHY_F_FW_MDIO_LOAD) {
        ioerr += PHY_BUS_WRITE(pc, TOPLVL_0X4188_REG, 0x0040);
    } else {
        ioerr += PHY_BUS_WRITE(pc, TOPLVL_0X418C_REG, 0x0000);
    }

    /* Enable global reset */
    ioerr += PHY_BUS_WRITE(pc, TOPLVL_0X4186_REG, 0x8000);
    /* Assert reset for the whole ARM system */
    ioerr += PHY_BUS_WRITE(pc, TOPLVL_0X4181_REG, 0x017c);
    /* Deassert reset for the whole ARM system but the ARM processor */
    ioerr += PHY_BUS_WRITE(pc, TOPLVL_0X4181_REG, 0x0040);

    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_ADDR_LOW_REG, 0x0000);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_ADDR_HIGH_REG, 0xc300);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_DATA_LOW_REG, 0x0010);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_DATA_HIGH_REG, 0x0000);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_CTRL_REG, 0x0009);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_ADDR_LOW_REG, 0x0000);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_ADDR_HIGH_REG, 0xffff);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_DATA_LOW_REG, 0x1018);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_DATA_HIGH_REG, 0xe59f);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_CTRL_REG, 0x0009);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_ADDR_LOW_REG, 0x0004);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_ADDR_HIGH_REG, 0xffff);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_DATA_LOW_REG, 0x1f11);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_DATA_HIGH_REG, 0xee09);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_CTRL_REG, 0x0009);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_ADDR_LOW_REG, 0x0008);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_ADDR_HIGH_REG, 0xffff);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_DATA_LOW_REG, 0x0000);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_DATA_HIGH_REG, 0xe3a0);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_CTRL_REG, 0x0009);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_ADDR_LOW_REG, 0x000c);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_ADDR_HIGH_REG, 0xffff);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_DATA_LOW_REG, 0x1806);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_DATA_HIGH_REG, 0xe3a0);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_CTRL_REG, 0x0009);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_ADDR_LOW_REG, 0x0010);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_ADDR_HIGH_REG, 0xffff);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_DATA_LOW_REG, 0x0002);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_DATA_HIGH_REG, 0xe8a0);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_CTRL_REG, 0x0009);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_ADDR_LOW_REG, 0x0014);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_ADDR_HIGH_REG, 0xffff);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_DATA_LOW_REG, 0x0001);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_DATA_HIGH_REG, 0xe150);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_CTRL_REG, 0x0009);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_ADDR_LOW_REG, 0x0018);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_ADDR_HIGH_REG, 0xffff);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_DATA_LOW_REG, 0xfffc);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_DATA_HIGH_REG, 0x3aff);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_CTRL_REG, 0x0009);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_ADDR_LOW_REG, 0x001c);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_ADDR_HIGH_REG, 0xffff);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_DATA_LOW_REG, 0xfffe);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_DATA_HIGH_REG, 0xeaff);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_CTRL_REG, 0x0009);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_ADDR_LOW_REG, 0x0020);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_ADDR_HIGH_REG, 0xffff);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_DATA_LOW_REG, 0x0021);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_DATA_HIGH_REG, 0x0004);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_CTRL_REG, 0x0009);

    ioerr += PHY_BUS_READ(pc, TOPLVL_CONFIG_STRAP_REG, &data);
    if (data & (0x1 << 13)) {
        ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_DATA_LOW_REG, 0x001e);
    }
    
    /* Deassert reset */
    ioerr += PHY_BUS_WRITE(pc, TOPLVL_0X4181_REG, 0x0000);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

/*
 * Function:
 *      _bcm84858_firmware_download
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
_bcm84858_firmware_download(phy_ctrl_t *pc, uint8_t *fw_data, uint32_t fw_size)
{
    int ioerr = 0;
    uint32_t data;
    int n_writes, idx;

    /* Check firmware data */
    if (fw_data == NULL || fw_size == 0) {
        PHY_WARN(pc, ("MDIO download: invalid firmware\n"));
        return CDK_E_NONE;
    }

    data = ((1 << DOWNLOAD_CTRL_SELF_INC_ADDR_SHIFT) |
            (1 << DOWNLOAD_CTRL_BURST_SHIFT) |
            (2 << DOWNLOAD_CTRL_SIZE_SHIFT) |
            (1 << DOWNLOAD_CTRL_WR_SHIFT));
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_CTRL_REG, data);

    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_ADDR_HIGH_REG, 0x0000);
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_ADDR_LOW_REG, 0x0000);

    n_writes = (fw_size + FIRMWARE_BLK_SIZE - 1) / FIRMWARE_BLK_SIZE;
    for (idx = 0; idx < n_writes; idx++ ) {
        data = (((*(fw_data + FIRMWARE_BLK_SIZE / 2 + 1)) << 8) |
                (*(fw_data + FIRMWARE_BLK_SIZE / 2)));
        ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_DATA_HIGH_REG, data);
        
        data = (((*(fw_data + 1)) << 8) | (*(fw_data + 0)));
        ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_DATA_LOW_REG, data);
        
        fw_data += FIRMWARE_BLK_SIZE;
    }
    ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_CTRL_REG, 0x0000);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

/*
 * Function:
 *      _bcm84858_auto_negotiate_gcd
 * Purpose:
 *      Get the information of auto negotiation.
 * Parameters:
 *      pc - PHY control structure
 *      speed - (OUT) Speed
 *      duplex - (OUT) Duplex mode
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84858_auto_negotiate_gcd(phy_ctrl_t *pc, int *speed, int *duplex)
{
    int ioerr = 0;
    int t_speed, t_duplex;
    uint32_t mii_stat, mii_ana, mii_anp;
    uint32_t mii_gb_stat, mii_gb_ctrl;
    uint32_t data;
    
    t_speed = 0;
    t_duplex = 0;
    mii_gb_stat = 0;
    mii_gb_ctrl = 0;

    ioerr += PHY_BUS_READ(pc, D7_FFXX_MII_STAT_REG, &mii_stat);
    if (mii_stat & MII_STAT_ES) {   /* Supports extended status */
        /*
         * If the PHY supports extended status, check if it is 1000MB
         * capable.  If it is, check the 1000Base status register to see
         * if 1000MB negotiated.
         */
        ioerr += PHY_BUS_READ(pc, D7_FFXX_MII_IEEE_EXTEND_STAT_REG, &data);
        if (data & (MII_ESR_1000_X_FD | MII_ESR_1000_X_HD | 
                    MII_ESR_1000_T_FD | MII_ESR_1000_T_HD)) {
            ioerr += PHY_BUS_READ(pc, D7_FFXX_MII_GB_STAT_REG, &mii_gb_stat);
            ioerr += PHY_BUS_READ(pc, D7_FFXX_MII_GB_CTRL_REG, &mii_gb_ctrl);
        }
    }

    /*
     * At this point, if we did not see Gig status, one of mii_gb_stat or 
     * mii_gb_ctrl will be 0. This will cause the first 2 cases below to 
     * fail and fall into the default 10/100 cases.
     */
    ioerr += PHY_BUS_READ(pc, D7_FFXX_MII_ANA_REG, &mii_ana);
    ioerr += PHY_BUS_READ(pc, D7_FFXX_MII_ANP_REG, &mii_anp);
    mii_ana &= mii_anp;

    if ((mii_gb_ctrl & MII_GB_CTRL_ADV_1000FD) &&
        (mii_gb_stat & MII_GB_STAT_LP_1000FD)) {
        t_speed  = 1000;
        t_duplex = 1;
    } else if ((mii_gb_ctrl & MII_GB_CTRL_ADV_1000HD) &&
               (mii_gb_stat & MII_GB_STAT_LP_1000HD)) {
        t_speed  = 1000;
        t_duplex = 0;
    } else if (mii_ana & MII_ANA_FD_100) {
        t_speed = 100;
        t_duplex = 1;
    } else if (mii_ana & MII_ANA_T4) {
        t_speed = 100;
        t_duplex = 0;
    } else if (mii_ana & MII_ANA_HD_100) {
        t_speed = 100;
        t_duplex = 0;
    } else if (mii_ana & MII_ANA_FD_10) {
        t_speed = 10;
        t_duplex = 1 ;
    } else if (mii_ana & MII_ANA_HD_10) {
        t_speed = 10;
        t_duplex = 0;
    }

    if (speed)  {
        *speed  = t_speed;
    }
    if (duplex) {
        *duplex = t_duplex;
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

/*
 * Function:
 *      _bcm84858_mdi_pair_remap
 * Purpose:
 *      Change MDI pair order
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84858_mdi_pair_remap(phy_ctrl_t *pc, uint32_t mapping)
{
    int ioerr = 0;
    uint16_t value;
    uint32_t args[5];
    
    value = (((mapping & (0x3 << 12)) >> 6) | ((mapping & (0x3 << 8)) >> 4) | 
             ((mapping & (0x3 << 4)) >> 2) | (mapping & 0x3));
    
    args[0] = (uint32_t)value;
    ioerr += _bcm84858_top_level_cmd_set(pc, 
                BCM8484X_DIAG_CMD_SET_PAIR_SWAP, args, 1);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

/*
 * Function:
 *      _bcm84858_eee_mode_set
 * Purpose:
 *      Set EEE mode
 * Parameters:
 *      pc - PHY control structure
 *      mode - EEE mode
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84858_eee_mode_set(phy_ctrl_t *pc, uint32_t mode)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t args[5];
            
    if (mode == PHY_EEE_802_3) {
        args[0] = 5;
        args[1] = 0x0000;
        args[2] = 0x0000;
        args[3] = 0x0000;
        
        rv = _bcm84858_top_level_cmd_set(pc, 
                    BCM8484X_DIAG_CMD_SET_EEE_MODE, args, 4);
                    
        if (CDK_SUCCESS(rv)) {
            rv = PHY_AUTONEG_SET(pc, 1);
        }
        
        if (CDK_SUCCESS(rv)) {
            rv = _bcm84858_top_level_cmd_get(pc, 
                        BCM8484X_DIAG_CMD_GET_EEE_MODE, args, 4);
        }
    } else if (mode == PHY_EEE_AUTO) {
        args[0] = 0xa;
        args[1] = 0x0000;
        args[2] = 0x3d09;
        args[3] = 0x047e;
        
        rv = _bcm84858_top_level_cmd_set(pc, 
                    BCM8484X_DIAG_CMD_SET_EEE_MODE, args, 4);
        
        if (CDK_SUCCESS(rv)) {
            rv = PHY_AUTONEG_SET(pc, 1);
        }

        if (CDK_SUCCESS(rv)) {
            rv = _bcm84858_top_level_cmd_get(pc, 
                        BCM8484X_DIAG_CMD_GET_EEE_MODE, args, 4);
        }
    } else if (mode == PHY_EEE_NONE) {
        args[0] = 0;
        args[1] = 0x0000;
        args[2] = 0x0000;
        args[3] = 0x0000;
    
        rv = _bcm84858_top_level_cmd_set(pc, 
                    BCM8484X_DIAG_CMD_SET_EEE_MODE, args, 4);
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _bcm84858_eee_mode_get
 * Purpose:
 *      Set EEE mode
 * Parameters:
 *      pc - PHY control structure
 *      mode - EEE mode
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84858_eee_mode_get(phy_ctrl_t *pc, uint32_t *mode)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t args[5];
 
    rv = _bcm84858_top_level_cmd_get(pc, 
                    BCM8484X_DIAG_CMD_GET_EEE_MODE, args, 4);
    if (CDK_SUCCESS(rv)) {
        if (args[0] == 1) {
           *mode = PHY_EEE_802_3;
        } else if (args[0] == 2) {
           *mode = PHY_EEE_AUTO;
        } else {
           *mode = PHY_EEE_NONE;
        }
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _bcm84858_master_set
 * Purpose:
 *      Set the master mode for the PHY.
 * Parameters:
 *      pc - PHY control structure
 *      master - PHY_MS_xxx
 * Returns:     
 *      CDK_E_xxx
 */

static int
_bcm84858_master_set(phy_ctrl_t *pc, uint32_t master)
{
    int ioerr = 0;
    uint32_t mii_gb_ctrl;

    ioerr += PHY_BUS_READ(pc, D7_FFXX_MII_GB_CTRL_REG, &mii_gb_ctrl);

    switch (master) {
    case PHY_MS_SLAVE:
        mii_gb_ctrl |= MII_GB_CTRL_MS_MAN;
        mii_gb_ctrl &= ~MII_GB_CTRL_MS;
        break;
    case PHY_MS_MASTER:
        mii_gb_ctrl |= MII_GB_CTRL_MS_MAN;
        mii_gb_ctrl |= MII_GB_CTRL_MS;
        break;
    case PHY_MS_AUTO:
        mii_gb_ctrl &= ~MII_GB_CTRL_MS_MAN;
        break;
    }

    ioerr += PHY_BUS_WRITE(pc, D7_FFXX_MII_GB_CTRL_REG, mii_gb_ctrl);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

/*
 * Function:
 *      _bcm84858_master_get
 * Purpose:
 *      Get the master mode for the PHY.  If mode is forced, then
 *      forced mode is returned; otherwise operating mode is returned.
 * Parameters:
 *      pc - PHY control structure
 *      master - (OUT) PHY_MS_xxx
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84858_master_get(phy_ctrl_t *pc, uint32_t *master)
{
    int ioerr = 0;
    uint32_t mii_gb_stat, mii_gb_ctrl;

    if (!master) {
        return CDK_E_PARAM;
    }

    ioerr += PHY_BUS_READ(pc, D7_FFXX_MII_GB_CTRL_REG, &mii_gb_ctrl);
    if (!(mii_gb_ctrl & MII_GB_CTRL_MS_MAN)) {
         *master = PHY_MS_AUTO;
    } else {
        ioerr += PHY_BUS_READ(pc, D7_FFXX_MII_GB_STAT_REG, &mii_gb_stat);
        if (mii_gb_stat & MII_GB_STAT_MS_FAULT) {
            *master = PHY_MS_NONE;
        } else if (mii_gb_stat & MII_GB_STAT_MS) {
            *master = PHY_MS_MASTER;
        } else {
            *master = PHY_MS_SLAVE;
        }
    }
    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

/*
 * Function:    
 *      _bcm84858_ability_local_get
 * Purpose:     
 *      Get the local abilities of the PHY.
 * Parameters:
 *      pc - PHY control structure
 *      abil - (OUT) ability mask indicating supported options/speeds.
 * Returns:     
 *      CDK_E_xxx
 */
static int
_bcm84858_ability_local_get(phy_ctrl_t *pc, uint32_t *ability, int *idx)
{
    *idx = 0;
    
    if (!ability) {
        return CDK_E_PARAM;
    }
    *idx = *idx + 1;
    if (IS_COPPER_MODE(pc)) {
        *ability = (PHY_ABIL_100MB_HD | PHY_ABIL_100MB_FD | PHY_ABIL_1000MB_FD | 
                    PHY_ABIL_10GB | PHY_ABIL_PAUSE | PHY_ABIL_PAUSE_ASYMM | 
                    PHY_ABIL_XGMII | PHY_ABIL_LOOPBACK);
    } else {
        *ability = (PHY_ABIL_1000MB_FD | PHY_ABIL_10GB | PHY_ABIL_PAUSE |
                    PHY_ABIL_PAUSE_ASYMM | PHY_ABIL_XGMII | PHY_ABIL_LOOPBACK | 
                    PHY_ABIL_AN);
    }
    
    return CDK_E_NONE;
}

/*
 * Function:    
 *      _bcm84858_ability_remote_get
 * Purpose:     
 *      Get the current remoteisement for auto-negotiation.
 * Parameters:
 *      pc - PHY control structure
 *      abil - (OUT) ability mask indicating supported options/speeds.
 * Returns:     
 *      CDK_E_xxx
 */
static int
_bcm84858_copper_ability_remote_get(phy_ctrl_t *pc, uint32_t *ability)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t an, an_done, link;
    uint32_t mii_stat, mii_ctrl;
    uint32_t data;
    int *arr_idx = 0;
    if (!ability) {
        return CDK_E_PARAM;
    }
    *ability = 0;

    ioerr += PHY_BUS_READ(pc, D7_FFXX_MII_STAT_REG, &mii_stat);
    ioerr += PHY_BUS_READ(pc, D7_FFXX_MII_CTRL_REG, &mii_ctrl);
    an = (mii_ctrl & MII_CTRL_AE) ? TRUE : FALSE;
    an_done = (mii_stat & MII_STAT_AN_DONE) ? TRUE : FALSE;
    link = (mii_stat & MII_STAT_LA) ? TRUE : FALSE;
    if (an && an_done && link) {
        /* Decode remote advertisement only when link is up and autoneg is completed. */
        ioerr += PHY_BUS_READ(pc, D7_FFXX_MII_ANP_REG, &data);
        if (data & MII_ANA_HD_10) {
            *ability |= PHY_ABIL_10MB_HD;
        }
        if (data & MII_ANA_HD_100) { 
            *ability |= PHY_ABIL_100MB_HD;
        }
        if (data & MII_ANA_FD_10) {
            *ability |= PHY_ABIL_10MB_FD;
        }
        if (data & MII_ANA_FD_100) {
            *ability |= PHY_ABIL_100MB_FD;
        }
        
        switch (data & (MII_ANA_PAUSE | MII_ANA_ASYM_PAUSE)) {
            case MII_ANA_PAUSE:
                *ability |= (PHY_ABIL_PAUSE_TX | PHY_ABIL_PAUSE_RX);
                break;
            case MII_ANA_ASYM_PAUSE:
                *ability |= PHY_ABIL_PAUSE_TX;
                break;
            case (MII_ANA_PAUSE | MII_ANA_ASYM_PAUSE):
                *ability |= PHY_ABIL_PAUSE_RX;
                break;
        }

        /* GE Specific values */
        ioerr += PHY_BUS_READ(pc, D7_FFXX_MII_GB_STAT_REG, &data);
        if (data & MII_GB_STAT_LP_1000HD) {
            *ability |= PHY_ABIL_1000MB_HD;
        }
        if (data & MII_GB_STAT_LP_1000FD) {
            *ability |= PHY_ABIL_1000MB_FD;
        }
    
        /* 10G Specific values */
        ioerr += PHY_BUS_READ(pc, AN_TENG_STAT_REG, &data);
        if (data & TENG_STATUS_LP_10GBASET_MASK) {
            *ability |= PHY_ABIL_10GB;
        }
    } else {
        /* Simply return local abilities */
        rv = _bcm84858_ability_local_get(pc, ability, arr_idx);
    }

    return ioerr ? CDK_E_IO : rv;
}

static int
_bcm84858_fiber_ability_remote_get(phy_ctrl_t *pc, uint32_t *ability)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t mii_stat;
    uint32_t data;
    int *arr_idx = 0;

    if (!ability) {
        return CDK_E_PARAM;
    }
    *ability = 0;

    ioerr += PHY_BUS_READ(pc, PHYXS_L_COMBO_IEEE0_MII_STAT_REG, &mii_stat);
    ioerr += PHY_BUS_READ(pc, PHYXS_L_GP_STATUS_XGSX3_REG, &data);
    if ((data & GP_STATUS_XGXS3_LINK_MASK) && (mii_stat & MII_STAT_AN_DONE)) {
        /* Decode remote advertisement only when link is up and autoneg is
         * completed.
         */
        ioerr += PHY_BUS_READ(pc, PHYXS_L_GP_STATUS_LP_UP1_REG, &data);
        if (data & LP_UP1_DATARATE_10GCX4_MASK) {
            *ability |= PHY_ABIL_10GB;
        }
        
        ioerr += PHY_BUS_READ(pc, PHYXS_L_COMBO_IEEE0_MII_ANP_REG, &data);
        if (data & MII_ANP_C37_FD) {
            *ability |= PHY_ABIL_1000MB_FD;
        }

        switch (data & (MII_ANP_C37_PAUSE | MII_ANP_C37_ASYM_PAUSE)) {
            case MII_ANP_C37_PAUSE:
                *ability |= PHY_ABIL_PAUSE_TX | PHY_ABIL_PAUSE_RX;
                break;
            case MII_ANP_C37_ASYM_PAUSE:
                *ability |= PHY_ABIL_PAUSE_TX;
                break;
            case MII_ANP_C37_PAUSE | MII_ANP_C37_ASYM_PAUSE:
                *ability |= PHY_ABIL_PAUSE_RX;
                break;
        }
    } else {
        /* Simply return local abilities */
        rv = _bcm84858_ability_local_get(pc, ability, arr_idx);
    }
        
    return ioerr ? CDK_E_IO : rv;
}

static int
_bcm84858_ability_remote_get(phy_ctrl_t *pc, uint32_t *ability)
{
    int rv = CDK_E_NONE;
    
    if (IS_COPPER_MODE(pc)) {
        rv = _bcm84858_copper_ability_remote_get(pc, ability);
    } else {
        rv = _bcm84858_fiber_ability_remote_get(pc, ability);
    }
    
    return rv;
}

/*
 * Function:    
 *      _bcm84858_ability_eee_remote_get
 * Purpose:     
 *      Get the current remote EEE abilities.
 * Parameters:
 *      pc - PHY control structure
 *      abil - (OUT) ability mask indicating supported options/speeds.
 * Returns:     
 *      CDK_E_xxx
 */
static int
_bcm84858_ability_eee_remote_get(phy_ctrl_t *pc, uint32_t *ability)
{
    int ioerr = 0;
    uint32_t an, an_done, link;
    uint32_t mii_stat, mii_ctrl;
    uint32_t data;

    if (!ability) {
        return CDK_E_PARAM;
    }
    *ability = 0;

    if (IS_COPPER_MODE(pc)) {
        ioerr += PHY_BUS_READ(pc, D7_FFXX_MII_STAT_REG, &mii_stat);
        ioerr += PHY_BUS_READ(pc, D7_FFXX_MII_CTRL_REG, &mii_ctrl);
        an = (mii_ctrl & MII_CTRL_AE) ? TRUE : FALSE;
        an_done = (mii_stat & MII_STAT_AN_DONE) ? TRUE : FALSE;
        link = (mii_stat & MII_STAT_LA) ? TRUE : FALSE;
        if (an && an_done && link) {
            ioerr += PHY_BUS_READ(pc, AN_EEE_LP_ADVERT_REG, &data);
        } else {
            /* Simply return local abilities */
            ioerr += PHY_BUS_READ(pc, AN_EEE_ADVERT_REG, &data);
        }
        if (data & EEE_ADVERT_10GBASET_MASK) {
            *ability |= PHY_ABIL_EEE_10GB;
        } 
        if (data & EEE_ADVERT_1000BASET_MASK) {
            *ability |= PHY_ABIL_EEE_1GB;
        } 
        if (data & EEE_ADVERT_100BASET_MASK) {
            *ability |= PHY_ABIL_EEE_100MB;
        }
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

/*
 * Function:    
 *      _bcm84858_ability_advert_set
 * Purpose:     
 *      Set the current advertisement for auto-negotiation.
 * Parameters:
 *      pc - PHY control structure
 *      ability - ability mask indicating supported options/speeds.
 * Returns:     
 *      CDK_E_xxx
 */
static int
_bcm84858_copper_ability_advert_set(phy_ctrl_t *pc, uint32_t ability[], int arr_idx)
{
    int ioerr = 0;
    uint32_t data;
    ioerr += PHY_BUS_READ(pc, D7_FFXX_MII_GB_CTRL_REG, &data);
    data &= ~(MII_GB_CTRL_ADV_1000HD | MII_GB_CTRL_ADV_1000FD);
    data |= MII_GB_CTRL_PT;
    
    if (ability[0] & PHY_ABIL_1000MB_HD) {
        data |= MII_GB_CTRL_ADV_1000HD;
    } 
    if (ability[0] & PHY_ABIL_1000MB_FD) {
        data |= MII_GB_CTRL_ADV_1000FD;
    } 
    ioerr += PHY_BUS_WRITE(pc, D7_FFXX_MII_GB_CTRL_REG, data);

    ioerr += PHY_BUS_READ(pc, D7_FFXX_MII_ANA_REG, &data);
    data &= ~(MII_ANA_HD_10 | MII_ANA_FD_10 | MII_ANA_HD_100 | MII_ANA_FD_100 | 
              MII_ANA_PAUSE | MII_ANA_ASYM_PAUSE);
    if (ability[0] & PHY_ABIL_10MB_HD)  {
        data |= MII_ANA_HD_10;
    }
    if (ability[0] & PHY_ABIL_100MB_HD) {
        data |= MII_ANA_HD_100;
    }
    if (ability[0] & PHY_ABIL_10MB_FD)  {
        data |= MII_ANA_FD_10;
    }
    if (ability[0] & PHY_ABIL_100MB_FD) {
        data |= MII_ANA_FD_100;
    }

    switch (ability[0] & (PHY_ABIL_PAUSE_TX | PHY_ABIL_PAUSE_RX)) {
    case PHY_ABIL_PAUSE_TX:
        data |= MII_ANA_ASYM_PAUSE;
        break;
    case PHY_ABIL_PAUSE_RX:
        data |= (MII_ANA_PAUSE | MII_ANA_ASYM_PAUSE);
        break;
    case (PHY_ABIL_PAUSE_TX | PHY_ABIL_PAUSE_RX):
        data |= MII_ANA_PAUSE;
        break;
    }
    ioerr += PHY_BUS_WRITE(pc, D7_FFXX_MII_ANA_REG, data);

    ioerr += PHY_BUS_READ(pc, AN_TENG_CTRL_REG, &data);
    data &= ~TENG_CTRL_10GBASET_MASK;
    if (ability[0] & PHY_ABIL_10GB) {
        data |= TENG_CTRL_10GBASET_MASK;
    }
    ioerr += PHY_BUS_WRITE(pc, AN_TENG_CTRL_REG, data);
    
    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

static int
_bcm84858_fiber_ability_advert_set(phy_ctrl_t *pc, uint32_t ability)
{
    int ioerr = 0;
    uint32_t data;

    data = 0;
    if (ability & PHY_ABIL_1000MB_FD) {
        data |= MII_ANP_C37_FD;
    }
    switch (ability & (PHY_ABIL_PAUSE_TX | PHY_ABIL_PAUSE_RX)) {
    case PHY_ABIL_PAUSE_TX:
        data |= MII_ANP_C37_ASYM_PAUSE;
        break;
    case PHY_ABIL_PAUSE_RX:
        data |= MII_ANP_C37_ASYM_PAUSE | MII_ANP_C37_PAUSE;
        break;
    case PHY_ABIL_PAUSE_TX | PHY_ABIL_PAUSE_RX:
        data |= MII_ANP_C37_PAUSE;
        break;
    }
    ioerr += PHY_BUS_WRITE(pc, PHYXS_L_COMBO_IEEE0_MII_ANA_REG, data);

    data = 0;
    if (ability & PHY_ABIL_10GB) {
        data |= OVER1G_UP1_DATARATE_10GCX4_MASK;
    }
    ioerr += PHY_BUS_WRITE(pc, PHYXS_L_OVER1G_UP1_REG, data);
    
    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

static int
_bcm84858_ability_advert_set(phy_ctrl_t *pc, uint32_t ability[], int arr_idx)
{
    int rv = CDK_E_NONE;
    
    if (IS_COPPER_MODE(pc)) {
        rv = _bcm84858_copper_ability_advert_set(pc, ability, arr_idx);
    } else {
        rv = _bcm84858_fiber_ability_advert_set(pc, ability[0]);
    }
    
    return rv;
}


/*
 * Function:    
 *      _bcm84858_ability_advert_get
 * Purpose:     
 *      Get the current advertisement for auto-negotiation.
 * Parameters:
 *      pc - PHY control structure
 *      ability - ability mask indicating supported options/speeds.
 * Returns:     
 *      CDK_E_xxx
 */
static int
_bcm84858_copper_ability_advert_get(phy_ctrl_t *pc, uint32_t *ability)
{
    int ioerr = 0;
    uint32_t data;

    if (!ability) {
        return CDK_E_PARAM;
    }
    *ability = 0;
    
    ioerr += PHY_BUS_READ(pc, D7_FFXX_MII_ANA_REG, &data);
    if (data & MII_ANA_HD_10) {
        *ability |= PHY_ABIL_10MB_HD;
    }
    if (data & MII_ANA_HD_100) { 
        *ability |= PHY_ABIL_100MB_HD;
    }
    if (data & MII_ANA_FD_10) {
        *ability |= PHY_ABIL_10MB_FD;
    }
    if (data & MII_ANA_FD_100) {
        *ability |= PHY_ABIL_100MB_FD;
    }

    switch (data & (MII_ANA_PAUSE | MII_ANA_ASYM_PAUSE)) {
        case MII_ANA_PAUSE:
            *ability |= (PHY_ABIL_PAUSE_TX | PHY_ABIL_PAUSE_RX);
            break;
        case MII_ANA_ASYM_PAUSE:
            *ability |= PHY_ABIL_PAUSE_TX;
            break;
        case (MII_ANA_PAUSE | MII_ANA_ASYM_PAUSE):
            *ability |= PHY_ABIL_PAUSE_RX;
            break;
    }

    /* GE Specific values */
    ioerr += PHY_BUS_READ(pc, D7_FFXX_MII_GB_CTRL_REG, &data);
    if (data & MII_GB_CTRL_ADV_1000HD) {
        *ability |= PHY_ABIL_1000MB_HD;
    }
    if (data & MII_GB_CTRL_ADV_1000FD) {
        *ability |= PHY_ABIL_1000MB_FD;
    }

    /* 10G Specific values */
    ioerr += PHY_BUS_READ(pc, AN_TENG_CTRL_REG, &data);
    if (data & TENG_CTRL_10GBASET_MASK) {
        *ability |= PHY_ABIL_10GB;
    }
    
    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

STATIC int
_bcm84858_fiber_ability_advert_get(phy_ctrl_t *pc, uint32_t *ability)
{
    int ioerr = 0;
    uint32_t data;

    if (!ability) {
        return CDK_E_PARAM;
    }
    *ability = 0;

    ioerr += PHY_BUS_READ(pc, PHYXS_L_COMBO_IEEE0_MII_ANA_REG, &data);
    switch (data & (MII_ANP_C37_ASYM_PAUSE | MII_ANP_C37_PAUSE)) {
    case MII_ANP_C37_ASYM_PAUSE:
        *ability |= PHY_ABIL_PAUSE_TX;
        break;
    case (MII_ANP_C37_ASYM_PAUSE | MII_ANP_C37_PAUSE):
        *ability |= PHY_ABIL_PAUSE_RX;
        break;
    case MII_ANP_C37_PAUSE:
        *ability |= PHY_ABIL_PAUSE_TX | PHY_ABIL_PAUSE_RX;
        break;
    }

    if (data & MII_ANP_C37_FD) {
        *ability |= PHY_ABIL_1000MB_FD;
    }

    ioerr += PHY_BUS_READ(pc, PHYXS_L_OVER1G_UP1_REG, &data);
    if (data & OVER1G_UP1_DATARATE_10GCX4_MASK) {
        *ability |= PHY_ABIL_10GB;
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

/*
 * Function:    
 *      _bcm84858_medium_change
 * Purpose:     
 *      Check the link medium.
 * Parameters:
 *      pc - PHY control structure
 * Returns:     
 *      CDK_E_xxx
 */
static int
_bcm84858_medium_change(phy_ctrl_t *pc)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int medium, changed = 0;
    uint32_t data;
    uint32_t ability;
    int arr_idx;

    /* Check the connected medium */
    ioerr += PHY_BUS_READ(pc, TOPLVL_STATUS_REG, &data);
    if (data & STATUS_FIBER_PRIORITY_MASK) {
        medium = PHY_MEDIUM_FIBER;
        if (!(data & STATUS_FIBER_DETECTED_MASK) && 
             (data & STATUS_COPPER_DETECTED_MASK)) {
            medium = PHY_MEDIUM_COPPER;
        }
    } else {
        medium = PHY_MEDIUM_COPPER;
        if (!(data & STATUS_COPPER_DETECTED_MASK) && 
             (data & STATUS_FIBER_DETECTED_MASK)) {
            medium = PHY_MEDIUM_FIBER;
        }
    }
    
    if (IS_FIBER_MODE(pc) && (medium == PHY_MEDIUM_COPPER)) {
        PHY_CTRL_FLAGS(pc) &= ~PHY_F_FIBER_MODE;
        changed = 1;
    } else if (IS_COPPER_MODE(pc) && (medium == PHY_MEDIUM_FIBER)) {
        PHY_CTRL_FLAGS(pc) |= PHY_F_FIBER_MODE;
        changed = 1;
    }

    if (changed) {
        /* Select register map */
        if (IS_COPPER_MODE(pc)) {
            ioerr += PHY_BUS_WRITE(pc, TOPLVL_MDIO_CTRL1_REG, 0x0003);
        } else {
            ioerr += PHY_BUS_WRITE(pc, TOPLVL_MDIO_CTRL1_REG, 0x2004);
        }
    
        /* Update the ability advertisement */
        rv = _bcm84858_ability_local_get(pc, &ability, &arr_idx);
        if (CDK_SUCCESS(rv)) {
            rv = _bcm84858_ability_advert_set(pc, &ability, arr_idx);
        }
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _bcm84858_init_stage_0
 * Purpose:
 *      PHY init.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84858_init_stage_0(phy_ctrl_t *pc)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t data;
    int cnt;
    
    _PHY_DBG(pc, ("init_stage_0\n"));

    PHY_CTRL_FLAGS(pc) |= PHY_F_CLAUSE45;
    PHY_CTRL_FLAGS(pc) &= ~PHY_F_FIBER_MODE;
#if 0
    /* Reset the GPHY core */
    ioerr += PHY_BUS_READ(pc, D7_FFXX_MII_CTRL_REG, &data);
    data |= MII_CTRL_RESET;
    ioerr += PHY_BUS_WRITE(pc, D7_FFXX_MII_CTRL_REG, data);

    /* Wait for reset completion */
    for (cnt = 0; cnt < PHY_RESET_POLL_MAX; cnt++) {
        ioerr += PHY_BUS_READ(pc, D7_FFXX_MII_CTRL_REG, &data);
        if ((data & MII_CTRL_RESET) == 0) {
            break;
        }
    }
    if (cnt >= PHY_RESET_POLL_MAX) {
        PHY_WARN(pc, ("1 reset timeout\n"));
        rv = CDK_E_TIMEOUT;
    }
#endif    
    /* disable bit7 for 10G, need to clean it */
    ioerr += PHY_BUS_READ(pc, TOPLVL_CONFIG_STRAP_REG, &data);
    data &= ~(CONFIG_STRAP_XGPH_DISABLED_MASK);
    ioerr += PHY_BUS_WRITE(pc, TOPLVL_CONFIG_STRAP_REG, data);
        
    ioerr += PHY_BUS_READ(pc, TOPLVL_FIRMWARE_REV_REG, &data);
    if (!data && !(PHY_CTRL_FLAGS(pc) & PHY_F_FW_MDIO_LOAD)) {
        /* Reset the device PMA/PMD and PCS */
        ioerr += PHY_BUS_READ(pc, PMA_PMD_CTRL1_REG, &data);
        data |= MII_CTRL_RESET;
        ioerr += PHY_BUS_WRITE(pc, PMA_PMD_CTRL1_REG, data);

        PHY_SYS_USLEEP(300000);
    
        /* Wait for reset completion */
        for (cnt = 0; cnt < PHY_RESET_POLL_MAX; cnt++) {
            ioerr += PHY_BUS_READ(pc, PMA_PMD_CTRL1_REG, &data);
            if ((data & MII_CTRL_RESET) == 0) {
                break;
            }
        }
        if (cnt >= PHY_RESET_POLL_MAX) {
            PHY_WARN(pc, ("2 reset timeout\n"));
            rv = CDK_E_TIMEOUT;
        }
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _bcm84858_init_stage_1
 * Purpose:
 *      PHY init.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84858_init_stage_1(phy_ctrl_t *pc)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;

    _PHY_DBG(pc, ("init_stage_1\n"));

    if (PHY_CTRL_FLAGS(pc) & PHY_F_FW_MDIO_LOAD) {
        /* Turn on broadcast mode */ 
        ioerr += PHY_BUS_WRITE(pc, TOPLVL_0X4117_REG, 0xf003);
        ioerr += PHY_BUS_WRITE(pc, TOPLVL_0X4107_REG, 0x0401);
        
        rv = _bcm84858_halt(pc);
    }
     
    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _bcm84858_init_stage_2
 * Purpose:
 *      PHY init.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84858_init_stage_2(phy_ctrl_t *pc)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t data, save_phy_addr;
  
    _PHY_DBG(pc, ("init_stage_2\n"));

    if (PHY_CTRL_FLAGS(pc) & PHY_F_FW_MDIO_LOAD) {
        if ((PHY_CTRL_FLAGS(pc) & PHY_F_STAGED_INIT) &&
            !(PHY_CTRL_FLAGS(pc) & PHY_F_BCAST_MSTR)) {
            return CDK_E_NONE;
        }

        save_phy_addr = PHY_CTRL_ADDR(pc);
        PHY_CTRL_ADDR(pc) &= ~0x1f;
        
        rv = _bcm84858_firmware_download(pc, bcm8485x_ucode, bcm8485x_ucode_len);
        
        if (CDK_SUCCESS(rv)) {
            ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_ADDR_LOW_REG, 0x0000);
            ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_ADDR_HIGH_REG, 0xc300);
            ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_DATA_LOW_REG, 0x0000);
            ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_DATA_HIGH_REG, 0x0000);
            data = ((0x2 << DOWNLOAD_CTRL_SIZE_SHIFT) | 
                    (0x1 << DOWNLOAD_CTRL_WR_SHIFT));
            ioerr += PHY_BUS_WRITE(pc, PMA_PMD_DOWNLOAD_CTRL_REG, data);
            
            /* Reset the processor to start execution of the code in the on-chip memory */
            ioerr += PHY_BUS_WRITE(pc, TOPLVL_0X4186_REG, 0x0004);
            ioerr += PHY_BUS_WRITE(pc, TOPLVL_0X4186_REG, 0x0000);
        }
        
        PHY_CTRL_ADDR(pc) = save_phy_addr;
    }
    
    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _bcm84858_init_stage_3
 * Purpose:
 *      PHY init.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84858_init_stage_3(phy_ctrl_t *pc)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t data;

    _PHY_DBG(pc, ("init_stage_3\n"));

    if (PHY_CTRL_FLAGS(pc) & PHY_F_FW_MDIO_LOAD) {
        /* Turn off broadcast mode */
        /* Before reset the processor, add OSF setting for all ports  */
        ioerr += PHY_BUS_WRITE(pc, PMA_PMD_0XA008_REG, 0x0000);
        ioerr += PHY_BUS_WRITE(pc, TOPLVL_0X8004_REG, 0x5555);
        ioerr += PHY_BUS_WRITE(pc, PMA_PMD_CTRL1_REG, 0x8000);
        ioerr += PHY_BUS_WRITE(pc, TOPLVL_0X4107_REG, 0x0000);

        /* Clear f/w ver. regs. */
        ioerr += PHY_BUS_WRITE(pc, TOPLVL_FIRMWARE_REV_REG, 0x0000);
        ioerr += PHY_BUS_WRITE(pc, TOPLVL_FIRMWARE_DATA_REG, 0x0000);
        
        ioerr += PHY_BUS_READ(pc, TOPLVL_CONFIG_STRAP_REG, &data);
        if (data & (0x1 << 13)) {
            /* Now reset only the ARM core */
            ioerr += PHY_BUS_WRITE(pc, TOPLVL_0X4181_REG, 0x0040);
            ioerr += PHY_BUS_WRITE(pc, TOPLVL_0X4181_REG, 0x0000);
        } else {
            /* Halt CPU if doing PMA/PMD reset. */ 
            ioerr += PHY_BUS_READ(pc, PMA_PMD_CTRL1_REG, &data);
            data |= MII_CTRL_RESET;
            ioerr += PHY_BUS_WRITE(pc, PMA_PMD_CTRL1_REG, data);
        }
        PHY_SYS_USLEEP(300000);
        /* disable bit7 for 10G, need to clean it */
        ioerr += PHY_BUS_READ(pc, TOPLVL_CONFIG_STRAP_REG, &data);
        data &= ~(CONFIG_STRAP_XGPH_DISABLED_MASK);
        ioerr += PHY_BUS_WRITE(pc, TOPLVL_CONFIG_STRAP_REG, data);
    }
    
    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _bcm84858_init_stage_4
 * Purpose:
 *      PHY init.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84858_init_stage_4(phy_ctrl_t *pc)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint8_t chip_rev, main_ver, branch, crc_status;
    uint32_t data;
    
    _PHY_DBG(pc, ("init_stage_4\n"));

    /* Check the firmware version */
    ioerr += PHY_BUS_READ(pc, TOPLVL_FIRMWARE_REV_REG, &data);
    chip_rev = (data & FIRMWARE_REV_CHIP_REVISION_MASK) >> FIRMWARE_REV_CHIP_REVISION_SHIFT;
    main_ver = (data & FIRMWARE_REV_MAIN_MASK) >> FIRMWARE_REV_MAIN_SHIFT;
    branch = (data & FIRMWARE_REV_BRANCH_MASK) >> FIRMWARE_REV_BRANCH_SHIFT;
    PHY_VERB(pc, ("BCM84858: port: %d, chip rev:%d, version:%d.%d\n", 
                                        pc->port, chip_rev, main_ver, branch));
                                        
    /* Check the SPIROM CRC status */
    ioerr += PHY_BUS_READ(pc, TOPLVL_STATUS_REG, &data);
    crc_status = (data & STATUS_SPIROM_CRC_CHECK_MASK) >> STATUS_SPIROM_CRC_CHECK_SHIFT;
    PHY_VERB(pc, ("BCM84858: port: %d, SPIROM CRC check status:%d\n", 
                                        pc->port, crc_status));

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _bcm84858_init_stage_5
 * Purpose:
 *      PHY init.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84858_init_stage_5(phy_ctrl_t *pc)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;

    uint32_t data;
    uint32_t ability;
    int arr_idx=0;
    
    _PHY_DBG(pc, ("init_stage_5\n"));

    if (PHY_CTRL_NEXT(pc)) {
        PHY_NOTIFY(PHY_CTRL_NEXT(pc), PhyEvent_PhyEnable);
    }

    /* Update the ability advertisement */
    rv = _bcm84858_ability_local_get(pc, &ability, &arr_idx);

    if (CDK_SUCCESS(rv)) {
        rv = _bcm84858_ability_advert_set(pc, &ability, arr_idx);
    }
        
    if (!(is_phy_repeater)) {
        /* for 2500base-X, the adv max speed is 2500, force not to use 10G */
        ioerr += PHY_BUS_READ(pc, AN_TENG_CTRL_REG, &data);
        data &= ~TENG_CTRL_10GBASET_MASK;
        ioerr += PHY_BUS_WRITE(pc, AN_TENG_CTRL_REG, data);
    }
    
    /* Remove super isolate */
    ioerr += PHY_BUS_READ(pc, TOPLVL_CONFIG_STRAP_REG, &data);
    data &= ~CONFIG_STRAP_SUPER_ISOLATE_MASK;
    ioerr += PHY_BUS_WRITE(pc, TOPLVL_CONFIG_STRAP_REG, data);

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _bcm84858_init_stage
 * Purpose:
 *      Execute specified init stage.
 * Parameters:
 *      pc - PHY control structure
 *      stage - init stage
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84858_init_stage(phy_ctrl_t *pc, int stage)
{
    switch (stage) {
    case 0:
        return _bcm84858_init_stage_0(pc);
    case 1:
        return _bcm84858_init_stage_1(pc);
    case 2:
        return _bcm84858_init_stage_2(pc);
    case 3:
        return _bcm84858_init_stage_3(pc);
    case 4:
        return _bcm84858_init_stage_4(pc);
    case 5:
        return _bcm84858_init_stage_5(pc);
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
 *      bcm84858_phy_probe
 * Purpose:     
 *      Probe for 84858 PHY
 * Parameters:
 *      pc - PHY control structure
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm84858_phy_probe(phy_ctrl_t *pc)
{
    int ioerr = 0;
    uint32_t phyid0, phyid1;

    PHY_CTRL_CHECK(pc);

    ioerr += PHY_BUS_READ(pc, PMA_PMD_ID0_REG, &phyid0);
    ioerr += PHY_BUS_READ(pc, PMA_PMD_ID1_REG, &phyid1);
    if (ioerr) {
        return CDK_E_IO;
    }

    if ((phyid0 == BCM84858_PMA_PMD_ID0) &&
        ((phyid1 & ~PHY_ID1_REV_MASK) == (BCM84858_PMA_PMD_ID1 & ~PHY_ID1_REV_MASK))) {
#if PHY_CONFIG_EXTERNAL_BOOT_ROM == 0

        /* Use MDIO download if external ROM is disabled */
        PHY_CTRL_FLAGS(pc) |= PHY_F_FW_MDIO_LOAD;
#endif
        return ioerr ? CDK_E_IO : CDK_E_NONE;
        
    }
    return CDK_E_NOT_FOUND;
}

/*
 * Function:
 *      bcm84858_phy_notify
 * Purpose:     
 *      Handle PHY notifications
 * Parameters:
 *      pc - PHY control structure
 *      event - PHY event
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm84858_phy_notify(phy_ctrl_t *pc, phy_event_t event)
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
 *      bcm84858_phy_reset
 * Purpose:     
 *      Reset 84858 PHY
 * Parameters:
 *      pc - PHY control structure
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm84858_phy_reset(phy_ctrl_t *pc)
{
    PHY_CTRL_CHECK(pc);

    return CDK_E_NONE;
}

/*
 * Function:
 *      bcm84858_phy_init
 * Purpose:     
 *      Initialize 84858 PHY driver
 * Parameters:
 *      pc - PHY control structure
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm84858_phy_init(phy_ctrl_t *pc)
{
    int rv = CDK_E_NONE;
    int stage;

    PHY_CTRL_CHECK(pc);

    if (PHY_CTRL_FLAGS(pc) & PHY_F_STAGED_INIT) {
        PHY_CTRL_FLAGS(pc) &= ~PHY_F_STAGED_INIT;
    }

    for (stage = 0; CDK_SUCCESS(rv); stage++) {
        rv = _bcm84858_init_stage(pc, stage);
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
 *      bcm84858_phy_link_get
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
bcm84858_phy_link_get(phy_ctrl_t *pc, int *link, int *autoneg_done)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t mii_stat, mii_ctrl, pcs_mii_stat;
    uint32_t data;
    uint32_t speed, autoneg;

    PHY_CTRL_CHECK(pc);
    
    if (!link || !autoneg_done) {
        return CDK_E_PARAM;
    }
    *link = 0;
    *autoneg_done = 0;

    if (IS_COPPER_MODE(pc)) { /* Connected at copper mode */
        ioerr += PHY_BUS_READ(pc, D7_FFXX_MII_STAT_REG, &mii_stat);
        ioerr += PHY_BUS_READ(pc, D7_FFXX_MII_CTRL_REG, &mii_ctrl);
        autoneg = (mii_ctrl & MII_CTRL_AE) ? TRUE : FALSE;
        if (autoneg) {
            *autoneg_done = (mii_stat & MII_STAT_AN_DONE) ? TRUE : FALSE;
            
            if (*autoneg_done == FALSE) {
                *link = FALSE;
                return CDK_E_NONE;
            }
        }
    
        /* Check is to handle removable PHY daughter cards */
        ioerr += PHY_BUS_READ(pc, PCS_STAT_REG, &pcs_mii_stat);

        if (pcs_mii_stat == 0xffff) {
            *link = FALSE;
            return CDK_E_NONE;
        } 
    
        /* Check if the current speed is set to 10G */
        speed = 0;
        if (autoneg) {
            ioerr += PHY_BUS_READ(pc, AN_TENG_CTRL_REG, &data);
            if ((data & TENG_CTRL_10GBASET_MASK) && (pcs_mii_stat & MII_STAT_LA)) {
                speed = 10000;
            }
        } else {
            ioerr += PHY_BUS_READ(pc, PMA_PMD_CTRL1_REG, &data);
            if ((data & (CTRL1_SPEED_10G_MASK | MII_CTRL_SS_MASK)) == 
                                                BCM84846_CTRL_SPEED_10G) {
                speed = 10000;
            }
        }
    
        if (speed == 10000) { 
            *link = (pcs_mii_stat & MII_STAT_LA) ?  TRUE : FALSE;
        } else {
            *link = (mii_stat & MII_STAT_LA) ? TRUE : FALSE;
        }
        
        if (autoneg && *link) {
            if (PHY_CTRL_NEXT(pc)) {
                rv = PHY_SPEED_GET(pc, &speed);
                if (CDK_SUCCESS(rv) && (speed == 10000)) {
                    rv = PHY_NOTIFY(PHY_CTRL_NEXT(pc), PhyEvent_ChangeToFiber);
                } else {
                    rv = PHY_NOTIFY(PHY_CTRL_NEXT(pc), PhyEvent_ChangeToPassthru);
                }
            }
        }
    } else { /* Connected at fiber mode */
        ioerr += PHY_BUS_READ(pc, TOPLVL_STATUS_REG, &data);
        *link = FALSE;
        if ((data != 0xffff) && (data & STATUS_FIBER_LINK_MASK)) {
            *link = TRUE;
        }
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:    
 *      bcm84858_phy_duplex_set
 * Purpose:     
 *      Set the current duplex mode (forced).
 * Parameters:
 *      pc - PHY control structure
 *      duplex - non-zero indicates full duplex, zero indicates half
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm84858_phy_duplex_set(phy_ctrl_t *pc, int duplex)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t speed, mii_ctrl;

    PHY_CTRL_CHECK(pc);
    
    if (IS_COPPER_MODE(pc)) { /* Connected at copper mode */
        rv = PHY_SPEED_GET(pc, &speed);     
        if (CDK_SUCCESS(rv)) {
            if ((speed == 100) || (speed == 1000) || (speed == 2500)
              || (speed == 5000) || (speed == 10000)) {
                ioerr += PHY_BUS_READ(pc, D7_FFXX_MII_CTRL_REG, &mii_ctrl);
                mii_ctrl &= ~MII_CTRL_FD;
                if (duplex) {
                    mii_ctrl |= MII_CTRL_FD;
                }
                ioerr += PHY_BUS_WRITE(pc, D7_FFXX_MII_CTRL_REG, mii_ctrl);
            } 
        }
    } else { /* Connected at fiber mode */
        ioerr += PHY_BUS_READ(pc, PHYXS_L_COMBO_IEEE0_MII_CTRL_REG, &mii_ctrl);
        mii_ctrl &= ~MII_CTRL_FD;
        if (duplex) {
            mii_ctrl |= MII_CTRL_FD;
        }
        ioerr += PHY_BUS_WRITE(pc, PHYXS_L_COMBO_IEEE0_MII_CTRL_REG, mii_ctrl);
    }
    
    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:    
 *      bcm84858_phy_duplex_get
 * Purpose:     
 *      Get the current operating duplex mode.
 * Parameters:
 *      pc - PHY control structure
 *      duplex - (OUT) non-zero indicates full duplex, zero indicates half
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm84858_phy_duplex_get(phy_ctrl_t *pc, int *duplex)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t mii_stat, mii_ctrl;
    uint32_t data, speed;

    PHY_CTRL_CHECK(pc);
    
    if (!duplex) {
        return CDK_E_PARAM;
    }

    if (IS_COPPER_MODE(pc)) { /* Connected at copper mode */
        rv = PHY_SPEED_GET(pc, &speed);
        if (CDK_SUCCESS(rv)) {
            if ((speed == 10000) || (speed == 0)) {
                *duplex =  TRUE;
                return CDK_E_NONE;
            }
            if ((speed == 5000) || (speed == 2500)) {
                *duplex = TRUE;
                return CDK_E_NONE;
            }
        }
    
        ioerr += PHY_BUS_READ(pc, D7_FFXX_MII_STAT_REG, &mii_stat);
        ioerr += PHY_BUS_READ(pc, D7_FFXX_MII_CTRL_REG, &mii_ctrl);
        if (mii_ctrl & MII_CTRL_AE) {     /* Auto-negotiation enabled */
            if (!(mii_stat & MII_STAT_AN_DONE)) { /* Auto-neg NOT complete */
                *duplex = FALSE;
            } else {
                rv = _bcm84858_auto_negotiate_gcd(pc, NULL, duplex);
            }
        } else {                /* Auto-negotiation disabled */
            *duplex = (mii_ctrl & MII_CTRL_FD) ? TRUE : FALSE;
        }
    } else { /* Connected at fiber mode */
        *duplex = TRUE;
        
        ioerr += PHY_BUS_READ(pc, PHYXS_L_GP_STATUS_1000X1_REG, &data);
        if (data & GP_STATUS_1000X1_SGMII_MODE_MASK) {
            /* Retrieve the duplex setting in SGMII mode */
            ioerr += PHY_BUS_READ(pc, PHYXS_L_COMBO_IEEE0_MII_CTRL_REG, &mii_ctrl);
            *duplex = (mii_ctrl & MII_CTRL_FD) ? TRUE : FALSE;
            if (mii_ctrl & MII_CTRL_AE) {
                ioerr += PHY_BUS_READ(pc, PHYXS_L_COMBO_IEEE0_MII_ANP_REG, &data);
            
                /* Make sure link partner is also in SGMII mode
                 * otherwise fall through to use the FD bit in MII_CTRL reg
                 */
                if (data & MII_ANP_SGMII_MODE) {
                    *duplex = (data & MII_ANP_SGMII_FD) ? TRUE : FALSE;
                }
            }
        }
    }
    
    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:    
 *      bcm84858_phy_speed_set
 * Purpose:     
 *      Set the current operating speed (forced).
 * Parameters:
 *      pc - PHY control structure
 *      speed - new link speed
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm84858_phy_speed_set(phy_ctrl_t *pc, uint32_t speed)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t speed_c45, speed_c22, data;
    int an, lb_mode;

    PHY_CTRL_CHECK(pc);

    rv = PHY_AUTONEG_GET(pc, &an);
    if (CDK_FAILURE(rv) || an) {
        return rv;
    }

    if (IS_COPPER_MODE(pc)) { /* Connected at copper mode */
        switch (speed) {
            case 10000:
                speed_c45 = BCM84846_CTRL_SPEED_10G;
                speed_c22 = MII_CTRL_SS_MASK;
                break;
    
            case 1000:
                speed_c45 = 0;
                speed_c22 = MII_CTRL_SS_1000;
                break;
    
            case 100:
                speed_c45 = 0;
                speed_c22 = MII_CTRL_SS_100;
                break;
    
            default:
                return CDK_E_PARAM;
        }
    
        /* If the current loopback mode is enabled, disable the loopback first. */
        rv = PHY_LOOPBACK_GET(pc, &lb_mode);
        if (CDK_SUCCESS(rv) && lb_mode) {
            rv = PHY_LOOPBACK_SET(pc, 0);
        }
        if (CDK_FAILURE(rv)) {
            return rv;
        }

        ioerr += PHY_BUS_READ(pc, PMA_PMD_CTRL1_REG, &data);
        data &= ~(MII_CTRL_SS_MASK | BCM84846_CTRL_SPEED_10G_MASK);
        data |= (speed_c45 | speed_c22);
        ioerr += PHY_BUS_WRITE(pc, PMA_PMD_CTRL1_REG, data);
       
        ioerr += PHY_BUS_READ(pc, D7_FFXX_MII_CTRL_REG, &data);
        data &= ~MII_CTRL_SS_MASK;
        data |= speed_c22;
        ioerr += PHY_BUS_WRITE(pc, D7_FFXX_MII_CTRL_REG, data);
    
        if (PHY_CTRL_NEXT(pc)) {
            if (speed == 10000) {
                rv = PHY_NOTIFY(PHY_CTRL_NEXT(pc), PhyEvent_ChangeToFiber);
            } else {
                rv = PHY_NOTIFY(PHY_CTRL_NEXT(pc), PhyEvent_ChangeToPassthru);
            }
            
            if (CDK_SUCCESS(rv)) {
                rv = PHY_SPEED_SET(PHY_CTRL_NEXT(pc), speed);
            }
        }
    
        /* Restore the loopback if necessary */
        if (CDK_SUCCESS(rv) && lb_mode) {
            PHY_SYS_USLEEP(1000000);
            rv = PHY_LOOPBACK_SET(pc, 1);
        }
    } else { /* Connected at fiber mode */
        ioerr += PHY_BUS_READ(pc, TOPLVL_CONFIG_STRAP_REG, &data);
        data &= ~CONFIG_STRAP_CONFIG12_MASK;
        if (speed == 1000) {
            data |= CONFIG_STRAP_CONFIG12_MASK;
        }
        ioerr += PHY_BUS_WRITE(pc, TOPLVL_CONFIG_STRAP_REG, data);
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:    
 *      bcm84858_phy_speed_get
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
bcm84858_phy_speed_get(phy_ctrl_t *pc, uint32_t *speed)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t teng_stat, teng_ctrl;
    uint32_t mii_stat, mii_ctrl;
    uint32_t an, an_done;
    uint32_t data;
    PHY_CTRL_CHECK(pc);

    if (!speed) {
        return CDK_E_PARAM;
    }
    
    if (IS_COPPER_MODE(pc)) { /* Connected at copper mode */
        ioerr += PHY_BUS_READ(pc, D7_FFXX_MII_STAT_REG, &mii_stat);
        ioerr += PHY_BUS_READ(pc, D7_FFXX_MII_CTRL_REG, &mii_ctrl);
        an = (mii_ctrl & MII_CTRL_AE) ? TRUE : FALSE;
        if (an) {
            an_done = (mii_stat & MII_STAT_AN_DONE) ? TRUE : FALSE;
            if (an_done) {
                ioerr += PHY_BUS_READ(pc, AN_TENG_CTRL_REG, &teng_ctrl);
                ioerr += PHY_BUS_READ(pc, AN_TENG_STAT_REG, &teng_stat);
                if ((teng_ctrl & TENG_CTRL_10GBASET_MASK) && 
                    (teng_stat & TENG_STATUS_LP_10GBASET_MASK)) {
                    *speed = 10000;
                } else {
                    /* Look at the CL22 regs and determine the gcd */
                    rv = _bcm84858_auto_negotiate_gcd(pc, (int *)speed, NULL);
                }
            } else {
                *speed = 0;
                return CDK_E_NONE;
            }
        } else {
            ioerr += PHY_BUS_READ(pc, PMA_PMD_CTRL1_REG, &data);
            switch(MII_CTRL_SS(data)) {
            case BCM84846_CTRL_SPEED_10G:
                *speed = 10000;
                break;
            case MII_CTRL_SS_1000:
                *speed = 1000;
                break;
            case MII_CTRL_SS_100:
                *speed = 100;
                break;
            default:
                *speed = 0;
                break;
            }
        }
    } else { /* Connected at fiber mode */
        ioerr += PHY_BUS_READ(pc, TOPLVL_CONFIG_STRAP_REG, &data);
        *speed = 10000;
        if (data & CONFIG_STRAP_CONFIG12_MASK) {
            *speed = 1000;
        }
    }
    
    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:    
 *      bcm84858_phy_autoneg_set
 * Purpose:     
 *      Enable or disable auto-negotiation on the specified port.
 * Parameters:
 *      pc - PHY control structure
 *      autoneg - non-zero enables autoneg, zero disables autoneg
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm84858_phy_autoneg_set(phy_ctrl_t *pc, int autoneg)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t mii_ctrl, an_ctrl;

    PHY_CTRL_CHECK(pc);

    rv = _bcm84858_medium_change(pc);
    if (CDK_FAILURE(rv)) {
        return rv;
    }

    if (IS_COPPER_MODE(pc)) { /* Connected at copper mode */
        if (PHY_CTRL_NEXT(pc)) {
            rv = PHY_AUTONEG_SET(PHY_CTRL_NEXT(pc), 0);
        }
        
        ioerr += PHY_BUS_READ(pc, D7_FFXX_MII_CTRL_REG, &mii_ctrl);
        mii_ctrl &= ~(MII_CTRL_AE | MII_CTRL_RAN) ;
        if (autoneg) {
            mii_ctrl |= (MII_CTRL_AE | MII_CTRL_RAN);
        }
        ioerr += PHY_BUS_WRITE(pc, D7_FFXX_MII_CTRL_REG, mii_ctrl);
    
        ioerr += PHY_BUS_READ(pc, AN_CTRL_REG, &an_ctrl);
        an_ctrl &= ~(MII_CTRL_AE | MII_CTRL_RAN) ;
        if (autoneg) {
            an_ctrl |= (MII_CTRL_AE | MII_CTRL_RAN);
        }
        ioerr += PHY_BUS_WRITE(pc, AN_CTRL_REG, an_ctrl);
    }
    
    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:    
 *      bcm84858_phy_autoneg_get
 * Purpose:     
 *      Get the current auto-negotiation setting.
 * Parameters:
 *      pc - PHY control structure
 *      autoneg - (OUT) non-zero indicates autoneg enabled
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm84858_phy_autoneg_get(phy_ctrl_t *pc, int *autoneg)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t mii_ctrl;

    PHY_CTRL_CHECK(pc);

    if (IS_COPPER_MODE(pc)) { /* Connected at copper mode */
        if (!autoneg) {
            return CDK_E_PARAM;
        }
    
        ioerr += PHY_BUS_READ(pc, D7_FFXX_MII_CTRL_REG, &mii_ctrl);
        *autoneg = (mii_ctrl & MII_CTRL_AE) ? TRUE : FALSE;
    } else { /* Connected at fiber mode */
        *autoneg = FALSE;
    }
    
    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:    
 *      bcm84858_phy_loopback_set
 * Purpose:     
 *      Set the internal PHY loopback mode.
 * Parameters:
 *      pc - PHY control structure
 *      enable - non-zero enables PHY loopback
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm84858_phy_loopback_set(phy_ctrl_t *pc, int enable)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t speed, data, cnt;

    PHY_CTRL_CHECK(pc);

    if (IS_COPPER_MODE(pc)) { /* Connected at copper mode */
        if (enable) {
            rv = PHY_SPEED_GET(pc, &speed);
            if (CDK_SUCCESS(rv)) {
                if (speed >= 2500) {
                    ioerr += PHY_BUS_READ(pc, PCS_CTRL_REG, &data);
                    data |= MII_CTRL_LE;
                    ioerr += PHY_BUS_WRITE(pc, PCS_CTRL_REG, data);
                } else {
                    ioerr += PHY_BUS_READ(pc, D7_FFXX_MII_CTRL_REG, &data);
                    data |= MII_CTRL_LE;
                    ioerr += PHY_BUS_WRITE(pc, D7_FFXX_MII_CTRL_REG, data);
                }
            }
            PHY_SYS_USLEEP(1000);
            for (cnt = 0; cnt < PHY_LINK_POLL_MAX; cnt++) {
                ioerr += PHY_BUS_READ(pc, TOPLVL_STATUS_REG, &data);
                if (data & STATUS_MAC_SIDE_LINK_MASK) {
                    break;
                }
                PHY_SYS_USLEEP(3000);
            }
            if (cnt >= PHY_LINK_POLL_MAX) {
                PHY_VERB(pc, ("MAC side link is down\n"));
            } else {
                PHY_VERB(pc, ("MAC side link is up\n"));
            }
            if (PHY_CTRL_NEXT(pc)) {
                rv = PHY_SPEED_SET(PHY_CTRL_NEXT(pc), speed);
                rv = PHY_AUTONEG_SET(PHY_CTRL_NEXT(pc), 0);
            }            
        } else {
            ioerr += PHY_BUS_READ(pc, PCS_CTRL_REG, &data);
            if (data & MII_CTRL_LE) {
                data &= ~MII_CTRL_LE;
                ioerr += PHY_BUS_WRITE(pc, PCS_CTRL_REG, data);
            }
    
            ioerr += PHY_BUS_READ(pc, D7_FFXX_MII_CTRL_REG, &data);
            if (data & MII_CTRL_LE) {
                data &= ~MII_CTRL_LE;
                ioerr += PHY_BUS_WRITE(pc, D7_FFXX_MII_CTRL_REG, data);
            }
        }
    } else { /* Connected at fiber mode */
        ioerr += PHY_BUS_READ(pc, PHYXS_L_CTRL_REG, &data);
        data &= ~MII_CTRL_LE;
        if (enable) {
            data |= MII_CTRL_LE;
        }
        ioerr += PHY_BUS_WRITE(pc, PHYXS_L_CTRL_REG, data);
        
        /* Configure Loopback in SerDes */
        ioerr += PHY_BUS_READ(pc, PHYXS_L_COMBO_IEEE0_MII_CTRL_REG, &data);
        data &= ~MII_CTRL_LE;
        if (enable) {
            data |= MII_CTRL_LE;
        }
        ioerr += PHY_BUS_WRITE(pc, PHYXS_L_COMBO_IEEE0_MII_CTRL_REG, data);
    }
    
    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:    
 *      bcm84858_phy_loopback_get
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
bcm84858_phy_loopback_get(phy_ctrl_t *pc, int *enable)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t speed, data;

    PHY_CTRL_CHECK(pc);

    if (!enable) {
        return CDK_E_PARAM;
    }

    *enable = FALSE;
    if (IS_COPPER_MODE(pc)) { /* Connected at copper mode */
        rv = PHY_SPEED_GET(pc, &speed);
        if (CDK_SUCCESS(rv)) {
            if (speed == 10000) {
                ioerr += PHY_BUS_READ(pc, PCS_CTRL_REG, &data);
                if ((data != 0xffff) && (data & MII_CTRL_LE)) {
                    *enable = TRUE;
                }
            } else {
                ioerr += PHY_BUS_READ(pc, D7_FFXX_MII_CTRL_REG, &data);
                if ((data != 0xffff) && (data & MII_CTRL_LE)) {
                    *enable = TRUE;
                }
            }
        }
    } else { /* Connected at fiber mode */
        ioerr += PHY_BUS_READ(pc, PHYXS_L_COMBO_IEEE0_MII_CTRL_REG, &data);
        if ((data != 0xffff) && (data & MII_CTRL_LE)) {
            *enable = TRUE;
        }
    }
    
    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:    
 *      bcm84858_phy_ability_get
 * Purpose:     
 *      Get the abilities of the PHY.
 * Parameters:
 *      pc - PHY control structure
 *      ability - (OUT) ability mask indicating supported options/speeds.
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcm84858_phy_ability_get(phy_ctrl_t *pc, uint32_t *ability)
{
    int rv = CDK_E_NONE;

    PHY_CTRL_CHECK(pc);

    if (IS_COPPER_MODE(pc)) { /* Connected at copper mode */
        rv = _bcm84858_copper_ability_advert_get(pc, ability);
    } else { /* Connected at fiber mode */
        rv = _bcm84858_fiber_ability_advert_get(pc, ability);
    }

    return rv;
}

/*
 * Function:
 *      _phy8486x_xfi_2p5g_mode_set
 * Purpose:
 *      Set Mac-side mode
 * Parameters:
 *      pc - PHY control structure
 *      mode - XFI: 0 
 *            2500: 1 
 * Returns:
 *      CDK_E_xxx
 */
static int
_phy8486x_xfi_2p5g_mode_set(phy_ctrl_t *pc, uint32_t mode)
{
/*
#define    PHY84834_DIAG_CMD_XFI_2P5G_MODE_XFI                       0
#define    PHY84834_DIAG_CMD_XFI_2P5G_MODE_2500X                     1
*/
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t args[2];
            
    if (mode == 1) {
        /* 2500X set to '1' */
        args[0] = 1;
        is_phy_repeater = 0;
    } else {
        /* XFI set to '0' */
        args[0] = 0;
        is_phy_repeater = 1;
    }
    args[1] = 0x0000;

    rv = _bcm84858_top_level_cmd_set(pc, 
                    BCM84834_DIAG_CMD_SET_XFI_2P5G_MODE, args, 1);

    return ioerr ? CDK_E_IO : rv;
}


/*
 * Function:
 *      bcm84858_phy_config_set
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
bcm84858_phy_config_set(phy_ctrl_t *pc, phy_config_t cfg, uint32_t val, void *cd)
{
    int rv = CDK_E_NONE;
    
    PHY_CTRL_CHECK(pc);

    switch (cfg) {
    case PhyConfig_MdiPairRemap:
        rv = _bcm84858_mdi_pair_remap(pc, val);
        break;
    case PhyConfig_InitStage:
        if (PHY_CTRL_FLAGS(pc) & PHY_F_STAGED_INIT) {
            rv = _bcm84858_init_stage(pc, val);
        }
        break;
    case PhyConfig_EEE:
        rv = _bcm84858_eee_mode_set(pc, val);
        break;
    case PhyConfig_Master:
        rv = _bcm84858_master_set(pc, val);
        break;
    case PhyConfig_PHY_SERDES:
        rv = _phy8486x_xfi_2p5g_mode_set(pc, val);
        return rv;        
    default:
        rv = CDK_E_UNAVAIL;
        break;
    }

    return rv;
}

/*
 * Function:
 *      bcm84858_phy_config_get
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
bcm84858_phy_config_get(phy_ctrl_t *pc, phy_config_t cfg, uint32_t *val, void *cd)
{
    int rv = CDK_E_NONE;
    PHY_CTRL_CHECK(pc);

    switch (cfg) {
    case PhyConfig_Enable:
        *val = 1;
        return CDK_E_NONE;
    case PhyConfig_Clause45Devs:
        *val = 0x4000009a;
        return CDK_E_NONE;
    case PhyConfig_BcastAddr:
        *val = PHY_CTRL_BUS_ADDR(pc) & ~0x1f;
        return CDK_E_NONE;
    case PhyConfig_EEE:
        rv = _bcm84858_eee_mode_get(pc, val);
        return rv;
    case PhyConfig_Master:
        rv = _bcm84858_master_get(pc, val);
        return rv;
    case PhyConfig_AdvLocal:
        rv = _bcm84858_ability_local_get(pc, val, (void *)cd);
        return rv;
    case PhyConfig_AdvRemote:
        rv = _bcm84858_ability_remote_get(pc, val);
        return rv;
    case PhyConfig_AdvEEERemote:
        rv = _bcm84858_ability_eee_remote_get(pc, val);
        return rv;
    default:
        break;
    }

    return CDK_E_UNAVAIL;
}

/*
 * Function:
 *      bcm84858_phy_status_get
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
bcm84858_phy_status_get(phy_ctrl_t *pc, phy_status_t st, uint32_t *val)
{
    int ioerr = 0;
    uint32_t data;
    
    PHY_CTRL_CHECK(pc);

    switch (st) {
    case PhyStatus_PortMDIX: 
        ioerr += PHY_BUS_READ(pc, D7_FFXX_MII_PHY_EXTEND_STAT_REG, &data);
        if (data & 0x2000) {
            *val = PHY_MDIX_XOVER;
        } else {
            *val = PHY_MDIX_NORMAL;
        }
        return ioerr ? CDK_E_IO : CDK_E_NONE;
    case PhyStatus_Medium: 
        *val = PHY_MEDIUM_COPPER;
        return ioerr ? CDK_E_IO : CDK_E_NONE;
    default:
        break;
    }

    return CDK_E_UNAVAIL;
}

/*
 * Variable:    bcm84858_drv
 * Purpose:     PHY Driver for BCM84858.
 */
phy_driver_t bcm84858_drv = {
    "bcm84858",
    "BCM84858 10GbE PHY Driver",  
    0,
    bcm84858_phy_probe,                  /* pd_probe */
    bcm84858_phy_notify,                 /* pd_notify */
    bcm84858_phy_reset,                  /* pd_reset */
    bcm84858_phy_init,                   /* pd_init */
    bcm84858_phy_link_get,               /* pd_link_get */
    bcm84858_phy_duplex_set,             /* pd_duplex_set */
    bcm84858_phy_duplex_get,             /* pd_duplex_get */
    bcm84858_phy_speed_set,              /* pd_speed_set */
    bcm84858_phy_speed_get,              /* pd_speed_get */
    bcm84858_phy_autoneg_set,            /* pd_autoneg_set */
    bcm84858_phy_autoneg_get,            /* pd_autoneg_get */
    bcm84858_phy_loopback_set,           /* pd_loopback_set */
    bcm84858_phy_loopback_get,           /* pd_loopback_get */
    bcm84858_phy_ability_get,            /* pd_ability_get */
    bcm84858_phy_config_set,             /* pd_config_set */
    bcm84858_phy_config_get,             /* pd_config_get */
    bcm84858_phy_status_get,             /* pd_status_get */
    NULL                                 /* pd_cable_diag */
};
