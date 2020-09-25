/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * PHY driver for BCM84888.
 */
#include <phy/chip/bcm84888_defs.h>

#include <phy/phy.h>
#include <phy/ge_phy.h>
#include <cdk/cdk_debug.h>

extern unsigned char bcm_84888A0_firmware[];
extern int bcm_84888A0_firmware_size;

extern unsigned char bcm_84888B0_firmware[];
extern int bcm_84888B0_firmware_size;

#define IS_COPPER_MODE(pc)      (!(PHY_CTRL_FLAGS(pc) & PHY_F_FIBER_MODE))
#define IS_FIBER_MODE(pc)       (PHY_CTRL_FLAGS(pc) & PHY_F_FIBER_MODE)

/* Private PHY flag is used to indicate firmware download method */
#define PHY_F_FW_MDIO_LOAD                      PHY_F_PRIVATE

#define FIRMWARE_BLK_SIZE                       4

#define PHY_RESET_POLL_MAX                      10
#define PHY_LINK_POLL_MAX                       1000
#define PHY_CMD_POLL_MAX                        1000

#define PHY_ID1_REV_MASK                        0x000f

#define BCM84888_PMA_PMD_ID0                    0xae02
#define BCM84888_PMA_PMD_ID1                    0x5141

/***************************************
 * MDIO Command Handler Version 2
 */
#define BCM8485X_CMD_OPEN_FOR_CMDS                      0x0004
#define BCM8485X_CMD_COMPLETE_ERROR                     0x0008

#define BCM8485X_CMD_SYSTEM_BUSY                        0xBBBB
#define BCM8485X_CMD_IN_PROGRESS                        0x0002

#define BCM84834_DIAG_CMD_SET_PAIR_SWAP_V2              0x8001
#define BCM84834_DIAG_CMD_SET_PAIR_SWAP_GENERIC         BCM84834_DIAG_CMD_SET_PAIR_SWAP_V2
#define BCM8484X_DIAG_CMD_GET_EEE_MODE                  0x8008
#define BCM84834_DIAG_CMD_SET_EEE_MODE_V2               0x8009
#define BCM84834_DIAG_CMD_SET_EEE_MODE_GENERIC          BCM84834_DIAG_CMD_SET_EEE_MODE_V2
#define BCM8485X_DIAG_CMD_WRITE_SHADOW_REG_V2           0x8015
#define BCM84834_DIAG_CMD_SET_XFI_2P5G_MODE_V2          0x8017
#define BCM848x_DIAG_CMD_SET_JUMBO_FRAME_MODE           0x801C

#define BCM8481_CMD_COMPLETE_PASS                       BCM8485X_CMD_OPEN_FOR_CMDS
/* Low level debugging (off by default) */
#ifdef PHY_DEBUG_ENABLE
#define _PHY_DBG(_pc, _stuff) \
    PHY_VERB(_pc, _stuff)
#else
#define _PHY_DBG(_pc, _stuff)
#endif

#if PHY_CONFIG_PRIVATE_DATA_WORDS > 0
#define PRIV_DATA(_pc) ((_pc)->priv[0])
#define LB_SPEED_GET(_pc) (PRIV_DATA(_pc))
#define LB_SPEED_SET(_pc,_val) \
do { \
    PRIV_DATA(_pc) = (_val); \
} while (0)

#else

#define LB_SPEED_GET(_pc) (0)
#define LB_SPEED_SET(_pc,_val)

#endif /* PHY_CONFIG_PRIVATE_DATA_WORDS */

/* Config sys-side is 2500 or 5000 needs to set repeater as '0' */
static int is_phy_repeater = 1;
/***********************************************************************
 *
 * HELPER FUNCTIONS
 */
/*
 * Function:
 *      _bcm84888_top_level_cmd_set_v2
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
_bcm84888_top_level_cmd_set_v2(phy_ctrl_t *pc, uint32_t cmd, uint32_t arg[], int size)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t cnt;
    TOP_CONFIG_SCRATCH_26r_t top26;
    TOP_CONFIG_SCRATCH_27r_t top27;
    TOP_CONFIG_SCRATCH_28r_t top28;
    TOP_CONFIG_SCRATCH_29r_t top29;
    TOP_CONFIG_SCRATCH_30r_t top30;
    TOP_CONFIG_SCRATCH_31r_t top31;
    TOP_CONFIG_SCRATCH_0r_t top0;
    uint32_t sts;

    if ((size < 1) || (size > 5)) {
        return CDK_E_PARAM;
    }

    /* Wait for the system is ready to accept commands */
    for (cnt = 0; cnt < PHY_CMD_POLL_MAX; cnt++) {
        ioerr += READ_TOP_CONFIG_SCRATCH_26r(pc, &top26);
        sts = TOP_CONFIG_SCRATCH_26r_GET(top26);
        if ((sts != BCM8485X_CMD_SYSTEM_BUSY) &&
           !(sts == BCM8485X_CMD_IN_PROGRESS)) {
            break;
        }
        PHY_SYS_USLEEP(10000);
    }
    if (cnt >= PHY_CMD_POLL_MAX) {
        PHY_WARN(pc, ("wait system timeout\n"));
        rv = CDK_E_TIMEOUT;
    }
    if (!(sts == BCM8485X_CMD_OPEN_FOR_CMDS || sts == BCM8485X_CMD_COMPLETE_ERROR)) {
        _PHY_DBG(pc, ("PHY84834_TOP_LEVEL_CMD_SET failed\n"));
        return (CDK_E_FAIL);
    }

    if (size-- > 0) {
        if (cmd == BCM84834_DIAG_CMD_SET_PAIR_SWAP_V2) {
            TOP_CONFIG_SCRATCH_28r_SET(top28, arg[0]);
            ioerr += WRITE_TOP_CONFIG_SCRATCH_28r(pc, top28);
        } else {
            TOP_CONFIG_SCRATCH_27r_SET(top27, arg[0]);
            ioerr += WRITE_TOP_CONFIG_SCRATCH_27r(pc, top27);
        }
    }
    if (size-- > 0) {
        TOP_CONFIG_SCRATCH_28r_SET(top28, arg[1]);
        ioerr += WRITE_TOP_CONFIG_SCRATCH_28r(pc, top28);
    }
    if (size-- > 0) {
        TOP_CONFIG_SCRATCH_29r_SET(top29, arg[2]);
        ioerr += WRITE_TOP_CONFIG_SCRATCH_29r(pc, top29);
    }
    if (size-- > 0) {
        TOP_CONFIG_SCRATCH_30r_SET(top30, arg[3]);
        ioerr += WRITE_TOP_CONFIG_SCRATCH_30r(pc, top30);
    }
    if (size-- > 0) {
        TOP_CONFIG_SCRATCH_31r_SET(top31, arg[4]);
        ioerr += WRITE_TOP_CONFIG_SCRATCH_31r(pc, top31);
    }

    TOP_CONFIG_SCRATCH_0r_SET(top0, cmd);
    ioerr += WRITE_TOP_CONFIG_SCRATCH_0r(pc, top0);

    /* Wait for the system is ready to accept commands */
    for (cnt = 0; cnt < PHY_CMD_POLL_MAX; cnt++) {
        ioerr += READ_TOP_CONFIG_SCRATCH_26r(pc, &top26);
        sts = TOP_CONFIG_SCRATCH_26r_GET(top26);
        if ((sts != BCM8485X_CMD_SYSTEM_BUSY) &&
           !(sts == BCM8485X_CMD_IN_PROGRESS)) {
            break;
        }
        PHY_SYS_USLEEP(10000);
    }
    if (cnt >= PHY_CMD_POLL_MAX) {
        PHY_WARN(pc, ("wait system timeout\n"));
        rv = CDK_E_TIMEOUT;
    }
    if (sts != BCM8481_CMD_COMPLETE_PASS) {
        _PHY_DBG(pc, ("PHY84834_TOP_LEVEL_CMD_SET failed\n"));
        return (CDK_E_FAIL);
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _bcm84888_top_level_cmd_get_v2
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
_bcm84888_top_level_cmd_get_v2(phy_ctrl_t *pc, uint32_t cmd, uint32_t arg[], int size)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    TOP_CONFIG_SCRATCH_26r_t top26;
    TOP_CONFIG_SCRATCH_27r_t top27;
    TOP_CONFIG_SCRATCH_28r_t top28;
    TOP_CONFIG_SCRATCH_29r_t top29;
    TOP_CONFIG_SCRATCH_30r_t top30;
    TOP_CONFIG_SCRATCH_31r_t top31;
    TOP_CONFIG_SCRATCH_0r_t top0;
    uint32_t cnt;
    uint32_t sts;

    if ((size < 1) || (size > 5)) {
        return CDK_E_PARAM;
    }

    for (cnt = 0; cnt < PHY_CMD_POLL_MAX; cnt++) {
        ioerr += READ_TOP_CONFIG_SCRATCH_26r(pc, &top26);
        sts = TOP_CONFIG_SCRATCH_26r_GET(top26);
        if ((sts != BCM8485X_CMD_SYSTEM_BUSY) &&
           !(sts == BCM8485X_CMD_IN_PROGRESS)) {
            break;
        }
        PHY_SYS_USLEEP(10000);
    }
    if (cnt >= PHY_CMD_POLL_MAX) {
        PHY_WARN(pc, ("wait system timeout\n"));
        rv = CDK_E_TIMEOUT;
    }
    if (!(sts == BCM8485X_CMD_OPEN_FOR_CMDS || sts == BCM8485X_CMD_COMPLETE_ERROR)) {
        _PHY_DBG(pc, ("PHY84834_TOP_LEVEL_CMD_GET failed\n"));
        return (CDK_E_FAIL);
    }

    TOP_CONFIG_SCRATCH_0r_SET(top0, cmd);
    ioerr += WRITE_TOP_CONFIG_SCRATCH_0r(pc, top0);

    /* Wait for the system is ready to accept commands */
    /* Wait for the system is ready to accept commands */
    for (cnt = 0; cnt < PHY_CMD_POLL_MAX; cnt++) {
        ioerr += READ_TOP_CONFIG_SCRATCH_26r(pc, &top26);
        sts = TOP_CONFIG_SCRATCH_26r_GET(top26);
        if ((sts != BCM8485X_CMD_SYSTEM_BUSY) &&
           !(sts == BCM8485X_CMD_IN_PROGRESS)) {
            break;
        }
        PHY_SYS_USLEEP(10000);
    }
    if (cnt >= PHY_CMD_POLL_MAX) {
        PHY_WARN(pc, ("wait system timeout\n"));
        rv = CDK_E_TIMEOUT;
    }
    if (sts != BCM8481_CMD_COMPLETE_PASS) {
        _PHY_DBG(pc, ("PHY84834_TOP_LEVEL_CMD_GET failed\n"));
        return (CDK_E_FAIL);
    }

    if (size-- > 0) {
        ioerr += READ_TOP_CONFIG_SCRATCH_27r(pc, &top27);
        arg[0] = TOP_CONFIG_SCRATCH_27r_GET(top27);
    }
    if (size-- > 0) {
        ioerr += READ_TOP_CONFIG_SCRATCH_28r(pc, &top28);
        arg[1] = TOP_CONFIG_SCRATCH_28r_GET(top28);
    }
    if (size-- > 0) {
        ioerr += READ_TOP_CONFIG_SCRATCH_29r(pc, &top29);
        arg[2] = TOP_CONFIG_SCRATCH_29r_GET(top29);
    }
    if (size-- > 0) {
        ioerr += READ_TOP_CONFIG_SCRATCH_30r(pc, &top30);
        arg[3] = TOP_CONFIG_SCRATCH_30r_GET(top30);
    }
    if (size-- > 0) {
        ioerr += READ_TOP_CONFIG_SCRATCH_31r(pc, &top31);
        arg[4] = TOP_CONFIG_SCRATCH_31r_GET(top31);
    }
    return ioerr ? CDK_E_IO : rv;
}

static int
_phy84888_shadow_reg_write(phy_ctrl_t *pc, uint32_t reg_group, uint32_t reg_addr, uint32_t bit_mask, uint32_t bit_shift, uint32_t value)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t args[5];
    uint32_t cmd;

    args[0] = reg_group;
    args[1] = reg_addr;
    args[2] = bit_mask;
    args[3] = bit_shift;
    args[4] = value;

    cmd = BCM8485X_DIAG_CMD_WRITE_SHADOW_REG_V2;
    rv = _bcm84888_top_level_cmd_set_v2(pc, cmd, args, 5);

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _bcm84888_halt
 * Purpose:
 *      .
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84888_halt(phy_ctrl_t *pc)
{
    int ioerr = 0;
    MGBASE_LED1_MASK_LOWr_t led1_lo;
    MGBASE_LED1_CTL1r_t led1_ctl;
    MGBASE_LED1_BLNKr_t led1_blnk;
    MGBASE_LED2_MASK_LOWr_t led2_lo;
    MGBASE_LED2_CTL1r_t led2_ctl;
    MGBASE_LED2_BLNKr_t led2_blnk;
    MGBASE_LED3_MASK_LOWr_t led3_lo;
    MGBASE_LED3_CTL1r_t led3_ctl;
    MGBASE_LED3_BLNKr_t led3_blnk;
    MGBASE_LED4_MASK_LOWr_t led4_lo;
    MGBASE_LED4_CTL1r_t led4_ctl;
    MGBASE_LED4_BLNKr_t led4_blnk;
    MGBASE_LED5_MASK_LOWr_t led5_lo;
    MGBASE_LED5_CTL1r_t led5_ctl;
    MGBASE_LED5_BLNKr_t led5_blnk;
    MGBASE_LED_CTLr_t led_ctl;
    MGBASE_LED_SRCr_t led_src;
    MGBASE_LED_SRC_HIr_t led_src_hi;
    TOP_SCRATCH_3r_t top3;
    TOP_SCRATCH_4r_t top4;
    TOP_SCRATCH_5r_t top5;
    TOP_SCRATCH_6r_t top6;
    MDIO2ARM_ADDR_LOWr_t adr_low;
    MDIO2ARM_ADDR_HIGHr_t adr_hi;
    MDIO2ARM_DATA_LOWr_t data_lo;
    MDIO2ARM_DATA_HIGHr_t data_hi;
    MDIO2ARM_CTLr_t load_ctl;
    TOP_XGPHY_STRAP1r_t strap1;

    /* LED control stuff */
    MGBASE_LED1_MASK_LOWr_SET(led1_lo, 0xffff);
    ioerr += WRITE_MGBASE_LED1_MASK_LOWr(pc, led1_lo);
    MGBASE_LED1_CTL1r_SET(led1_ctl, 0x0000);
    ioerr += WRITE_MGBASE_LED1_CTL1r(pc, led1_ctl);
    MGBASE_LED1_BLNKr_SET(led1_blnk, 0x0000);
    ioerr += WRITE_MGBASE_LED1_BLNKr(pc, led1_blnk);
    MGBASE_LED2_MASK_LOWr_SET(led2_lo, 0x0000);
    ioerr += WRITE_MGBASE_LED2_MASK_LOWr(pc, led2_lo);
    MGBASE_LED2_CTL1r_SET(led2_ctl, 0x0000);
    ioerr += WRITE_MGBASE_LED2_CTL1r(pc, led2_ctl);
    MGBASE_LED2_BLNKr_SET(led2_blnk, 0x0000);
    ioerr += WRITE_MGBASE_LED2_BLNKr(pc, led2_blnk);
    MGBASE_LED3_MASK_LOWr_SET(led3_lo, 0x0000);
    ioerr += WRITE_MGBASE_LED3_MASK_LOWr(pc, led3_lo);
    MGBASE_LED3_CTL1r_SET(led3_ctl, 0x0000);
    ioerr += WRITE_MGBASE_LED3_CTL1r(pc, led3_ctl);
    MGBASE_LED3_BLNKr_SET(led3_blnk, 0x0000);
    ioerr += WRITE_MGBASE_LED3_BLNKr(pc, led3_blnk);
    MGBASE_LED4_MASK_LOWr_SET(led4_lo, 0x0000);
    ioerr += WRITE_MGBASE_LED4_MASK_LOWr(pc, led4_lo);
    MGBASE_LED4_CTL1r_SET(led4_ctl, 0x0000);
    ioerr += WRITE_MGBASE_LED4_CTL1r(pc, led4_ctl);
    MGBASE_LED4_BLNKr_SET(led4_blnk, 0x0000);
    ioerr += WRITE_MGBASE_LED4_BLNKr(pc, led4_blnk);
    MGBASE_LED5_MASK_LOWr_SET(led5_lo, 0x0000);
    ioerr += WRITE_MGBASE_LED5_MASK_LOWr(pc, led5_lo);
    MGBASE_LED5_CTL1r_SET(led5_ctl, 0x0000);
    ioerr += WRITE_MGBASE_LED5_CTL1r(pc, led5_ctl);
    MGBASE_LED5_BLNKr_SET(led5_blnk, 0x0000);
    ioerr += WRITE_MGBASE_LED5_BLNKr(pc, led5_blnk);
    MGBASE_LED_CTLr_SET(led_ctl, 0xb6db);
    ioerr += WRITE_MGBASE_LED_CTLr(pc, led_ctl);
    MGBASE_LED_SRCr_SET(led_src, 0xffff);
    ioerr += WRITE_MGBASE_LED_SRCr(pc, led_src);
    MGBASE_LED_SRC_HIr_SET(led_src_hi, 0x0000);
    ioerr += WRITE_MGBASE_LED_SRC_HIr(pc, led_src_hi);

    if (PHY_CTRL_FLAGS(pc) & PHY_F_FW_MDIO_LOAD) {
        /* MDIO firmware download */
        TOP_SCRATCH_5r_SET(top5, 0x48f0);
        ioerr += WRITE_TOP_SCRATCH_5r(pc, top5);
    } else {
        /* SPI ROM firmware download */
        TOP_SCRATCH_6r_CLR(top6);
        ioerr += WRITE_TOP_SCRATCH_6r(pc, top6);
    }

    /* Enable global reset */
    TOP_SCRATCH_4r_SET(top4, 0x8000);
    ioerr += WRITE_TOP_SCRATCH_4r(pc, top4);
    /* Assert reset for the whole ARM system */
    TOP_SCRATCH_3r_SET(top3, 0x017c);
    ioerr += WRITE_TOP_SCRATCH_3r(pc, top3);
    /* Deassert reset for the whole ARM system but the ARM processor */
    TOP_SCRATCH_3r_SET(top3, 0x0040);
    ioerr += WRITE_TOP_SCRATCH_3r(pc, top3);

    MDIO2ARM_ADDR_LOWr_SET(adr_low, 0x0000);
    ioerr += WRITE_MDIO2ARM_ADDR_LOWr(pc, adr_low);
    MDIO2ARM_ADDR_HIGHr_SET(adr_hi, 0xc300);
    ioerr += WRITE_MDIO2ARM_ADDR_HIGHr(pc, adr_hi);
    MDIO2ARM_DATA_LOWr_SET(data_lo, 0x0010);
    ioerr += WRITE_MDIO2ARM_DATA_LOWr(pc, data_lo);
    MDIO2ARM_DATA_HIGHr_SET(data_hi, 0x0000);
    ioerr += WRITE_MDIO2ARM_DATA_HIGHr(pc, data_hi);
    MDIO2ARM_CTLr_SET(load_ctl, 0x0009);
    ioerr += WRITE_MDIO2ARM_CTLr(pc, load_ctl);
    MDIO2ARM_ADDR_LOWr_SET(adr_low, 0x0000);
    ioerr += WRITE_MDIO2ARM_ADDR_LOWr(pc, adr_low);
    MDIO2ARM_ADDR_HIGHr_SET(adr_hi, 0xffff);
    ioerr += WRITE_MDIO2ARM_ADDR_HIGHr(pc, adr_hi);
    MDIO2ARM_DATA_LOWr_SET(data_lo, 0x1018);
    ioerr += WRITE_MDIO2ARM_DATA_LOWr(pc, data_lo);
    MDIO2ARM_DATA_HIGHr_SET(data_hi, 0xe59f);
    ioerr += WRITE_MDIO2ARM_DATA_HIGHr(pc, data_hi);
    MDIO2ARM_CTLr_SET(load_ctl, 0x0009);
    ioerr += WRITE_MDIO2ARM_CTLr(pc, load_ctl);
    MDIO2ARM_ADDR_LOWr_SET(adr_low, 0x0004);
    ioerr += WRITE_MDIO2ARM_ADDR_LOWr(pc, adr_low);
    MDIO2ARM_ADDR_HIGHr_SET(adr_hi, 0xffff);
    ioerr += WRITE_MDIO2ARM_ADDR_HIGHr(pc, adr_hi);
    MDIO2ARM_DATA_LOWr_SET(data_lo, 0x1f11);
    ioerr += WRITE_MDIO2ARM_DATA_LOWr(pc, data_lo);
    MDIO2ARM_DATA_HIGHr_SET(data_hi, 0xee09);
    ioerr += WRITE_MDIO2ARM_DATA_HIGHr(pc, data_hi);
    MDIO2ARM_CTLr_SET(load_ctl, 0x0009);
    ioerr += WRITE_MDIO2ARM_CTLr(pc, load_ctl);
    MDIO2ARM_ADDR_LOWr_SET(adr_low, 0x0008);
    ioerr += WRITE_MDIO2ARM_ADDR_LOWr(pc, adr_low);
    MDIO2ARM_ADDR_HIGHr_SET(adr_hi, 0xffff);
    ioerr += WRITE_MDIO2ARM_ADDR_HIGHr(pc, adr_hi);
    MDIO2ARM_DATA_LOWr_SET(data_lo, 0x0000);
    ioerr += WRITE_MDIO2ARM_DATA_LOWr(pc, data_lo);
    MDIO2ARM_DATA_HIGHr_SET(data_hi, 0xe3a0);
    ioerr += WRITE_MDIO2ARM_DATA_HIGHr(pc, data_hi);
    MDIO2ARM_CTLr_SET(load_ctl, 0x0009);
    ioerr += WRITE_MDIO2ARM_CTLr(pc, load_ctl);

    MDIO2ARM_ADDR_LOWr_SET(adr_low, 0x000c);
    ioerr += WRITE_MDIO2ARM_ADDR_LOWr(pc, adr_low);
    MDIO2ARM_ADDR_HIGHr_SET(adr_hi, 0xffff);
    ioerr += WRITE_MDIO2ARM_ADDR_HIGHr(pc, adr_hi);
    MDIO2ARM_DATA_LOWr_SET(data_lo, 0x1806);
    ioerr += WRITE_MDIO2ARM_DATA_LOWr(pc, data_lo);
    MDIO2ARM_DATA_HIGHr_SET(data_hi, 0xe3a0);
    ioerr += WRITE_MDIO2ARM_DATA_HIGHr(pc, data_hi);
    MDIO2ARM_CTLr_SET(load_ctl, 0x0009);
    ioerr += WRITE_MDIO2ARM_CTLr(pc, load_ctl);

    MDIO2ARM_ADDR_LOWr_SET(adr_low, 0x0010);
    ioerr += WRITE_MDIO2ARM_ADDR_LOWr(pc, adr_low);
    MDIO2ARM_ADDR_HIGHr_SET(adr_hi, 0xffff);
    ioerr += WRITE_MDIO2ARM_ADDR_HIGHr(pc, adr_hi);
    MDIO2ARM_DATA_LOWr_SET(data_lo, 0x0002);
    ioerr += WRITE_MDIO2ARM_DATA_LOWr(pc, data_lo);
    MDIO2ARM_DATA_HIGHr_SET(data_hi, 0xe8a0);
    ioerr += WRITE_MDIO2ARM_DATA_HIGHr(pc, data_hi);
    MDIO2ARM_CTLr_SET(load_ctl, 0x0009);
    ioerr += WRITE_MDIO2ARM_CTLr(pc, load_ctl);

    MDIO2ARM_ADDR_LOWr_SET(adr_low, 0x0014);
    ioerr += WRITE_MDIO2ARM_ADDR_LOWr(pc, adr_low);
    MDIO2ARM_ADDR_HIGHr_SET(adr_hi, 0xffff);
    ioerr += WRITE_MDIO2ARM_ADDR_HIGHr(pc, adr_hi);
    MDIO2ARM_DATA_LOWr_SET(data_lo, 0x0001);
    ioerr += WRITE_MDIO2ARM_DATA_LOWr(pc, data_lo);
    MDIO2ARM_DATA_HIGHr_SET(data_hi, 0xe150);
    ioerr += WRITE_MDIO2ARM_DATA_HIGHr(pc, data_hi);
    MDIO2ARM_CTLr_SET(load_ctl, 0x0009);
    ioerr += WRITE_MDIO2ARM_CTLr(pc, load_ctl);

    MDIO2ARM_ADDR_LOWr_SET(adr_low, 0x0018);
    ioerr += WRITE_MDIO2ARM_ADDR_LOWr(pc, adr_low);
    MDIO2ARM_ADDR_HIGHr_SET(adr_hi, 0xffff);
    ioerr += WRITE_MDIO2ARM_ADDR_HIGHr(pc, adr_hi);
    MDIO2ARM_DATA_LOWr_SET(data_lo, 0xfffc);
    ioerr += WRITE_MDIO2ARM_DATA_LOWr(pc, data_lo);
    MDIO2ARM_DATA_HIGHr_SET(data_hi, 0x3aff);
    ioerr += WRITE_MDIO2ARM_DATA_HIGHr(pc, data_hi);
    MDIO2ARM_CTLr_SET(load_ctl, 0x0009);
    ioerr += WRITE_MDIO2ARM_CTLr(pc, load_ctl);

    MDIO2ARM_ADDR_LOWr_SET(adr_low, 0x001c);
    ioerr += WRITE_MDIO2ARM_ADDR_LOWr(pc, adr_low);
    MDIO2ARM_ADDR_HIGHr_SET(adr_hi, 0xffff);
    ioerr += WRITE_MDIO2ARM_ADDR_HIGHr(pc, adr_hi);
    MDIO2ARM_DATA_LOWr_SET(data_lo, 0xfffe);
    ioerr += WRITE_MDIO2ARM_DATA_LOWr(pc, data_lo);
    MDIO2ARM_DATA_HIGHr_SET(data_hi, 0xeaff);
    ioerr += WRITE_MDIO2ARM_DATA_HIGHr(pc, data_hi);
    MDIO2ARM_CTLr_SET(load_ctl, 0x0009);
    ioerr += WRITE_MDIO2ARM_CTLr(pc, load_ctl);

    MDIO2ARM_ADDR_LOWr_SET(adr_low, 0x0020);
    ioerr += WRITE_MDIO2ARM_ADDR_LOWr(pc, adr_low);
    MDIO2ARM_ADDR_HIGHr_SET(adr_hi, 0xffff);
    ioerr += WRITE_MDIO2ARM_ADDR_HIGHr(pc, adr_hi);
    MDIO2ARM_DATA_LOWr_SET(data_lo, 0x0021);
    ioerr += WRITE_MDIO2ARM_DATA_LOWr(pc, data_lo);
    MDIO2ARM_DATA_HIGHr_SET(data_hi, 0x0004);
    ioerr += WRITE_MDIO2ARM_DATA_HIGHr(pc, data_hi);
    MDIO2ARM_CTLr_SET(load_ctl, 0x0009);
    ioerr += WRITE_MDIO2ARM_CTLr(pc, load_ctl);

    ioerr += READ_TOP_XGPHY_STRAP1r(pc, &strap1);
    if (TOP_XGPHY_STRAP1r_XGPH_DISABLEDf_GET(strap1)) {
        MDIO2ARM_DATA_LOWr_SET(data_lo, 0x001e);
        ioerr += WRITE_MDIO2ARM_DATA_LOWr(pc, data_lo);
    }

    /* Deassert reset */
    TOP_SCRATCH_3r_CLR(top3);
    ioerr += WRITE_TOP_SCRATCH_3r(pc, top3);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

static int
_bcm84888_write_arm_addr(phy_ctrl_t *pc, uint32_t addr)
{
    int ioerr = 0;
    MDIO2ARM_ADDR_LOWr_t arm_addr_lo;
    MDIO2ARM_ADDR_HIGHr_t arm_addr_hi;

    MDIO2ARM_ADDR_HIGHr_SET(arm_addr_hi, addr >> 16);
    ioerr += WRITE_MDIO2ARM_ADDR_HIGHr(pc, arm_addr_hi);

    MDIO2ARM_ADDR_LOWr_SET(arm_addr_lo, addr & 0xffff);
    ioerr += WRITE_MDIO2ARM_ADDR_LOWr(pc, arm_addr_lo);

    return ioerr;
}

static int
_bcm84888_write_arm_data(phy_ctrl_t *pc, uint32_t data)
{
    int ioerr = 0;
    MDIO2ARM_DATA_LOWr_t arm_data_lo;
    MDIO2ARM_DATA_HIGHr_t arm_data_hi;

    MDIO2ARM_DATA_HIGHr_SET(arm_data_hi, data >> 16);
    ioerr += WRITE_MDIO2ARM_DATA_HIGHr(pc, arm_data_hi);

    /* Data gets written and the address gets auto-incremented upon write
     * to PHYC_MDIO2ARM_DATA_LOWr if SELF_INC is set. So do not change
     * the order.
     */
    MDIO2ARM_DATA_LOWr_SET(arm_data_lo, data & 0xffff);
    ioerr += WRITE_MDIO2ARM_DATA_LOWr(pc, arm_data_lo);

    return ioerr;
}

/*
 * Function:
 *      _bcm84888_write_to_arm
 * Purpose:
 *      Download firmware via MDIO.
 * Parameters:
 *      pc - PHY control structure
 *      fw_data - firmware data
 *      fw_size - size of firmware data (in bytes)
 * Returns:
 *      CDK_E_xxx
 */
#define ARM_DATA_WR_MSEC                10
static int
_bcm84888_write_to_arm(phy_ctrl_t *pc, uint8_t *fw_data, uint32_t fw_size)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    MDIO2ARM_CTLr_t mdio_ctl;
    MDIO2ARM_STSr_t arm_sts;
    int idx;
    int usec;
    uint8_t *p8;
    uint32_t data32;
    uint32_t addr = 0;

    /* Check firmware data */
    if (fw_data == NULL || fw_size == 0) {
        PHY_WARN(pc, ("MDIO download: invalid firmware\n"));
        return CDK_E_NONE;
    }
    ioerr += READ_MDIO2ARM_CTLr(pc, &mdio_ctl);
    MDIO2ARM_CTLr_SELF_INC_ADDRf_SET(mdio_ctl, 1);
    MDIO2ARM_CTLr_BURSTf_SET(mdio_ctl, 1);
    MDIO2ARM_CTLr_SIZEf_SET(mdio_ctl, 2);
    MDIO2ARM_CTLr_WRf_SET(mdio_ctl, 1);
    ioerr += WRITE_MDIO2ARM_CTLr(pc, mdio_ctl);

    ioerr += _bcm84888_write_arm_addr(pc, addr);
    for (idx = 0; idx < (int)fw_size; idx += 4) {

        p8 = &fw_data[idx];
        data32 = (LSHIFT32(p8[3], 24) |
                  LSHIFT32(p8[2], 16) |
                  LSHIFT32(p8[1], 8)  |
                  LSHIFT32(p8[0], 0));

        ioerr += _bcm84888_write_arm_data(pc, data32);

        /*
         * Reads from a broadcast address always returns 0xffff and hence
         * the following check will always succeed for a broadcast address.
         */
        for (usec = 0; usec < ARM_DATA_WR_MSEC; usec++) {
            ioerr += READ_MDIO2ARM_STSr(pc, &arm_sts);
            if (ioerr) {
                break;
            }
            if (MDIO2ARM_STSr_DONEf_GET(arm_sts)) {
                break;
            }
            PHY_SYS_USLEEP(1000);
        }

        if (MDIO2ARM_STSr_DONEf_GET(arm_sts) == 0) {
            _PHY_DBG(pc, ("MDIO2ARM write failed: addr=%08x\n", addr + idx));
            rv = CDK_E_TIMEOUT;
            break;
        }
    }

    MDIO2ARM_CTLr_CLR(mdio_ctl);
    ioerr += WRITE_MDIO2ARM_CTLr(pc, mdio_ctl);

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _bcm84888_auto_negotiate_gcd
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
_bcm84888_auto_negotiate_gcd(phy_ctrl_t *pc, int *speed, int *duplex)
{
    int ioerr = 0;
    int t_speed, t_duplex;
    MII_STATr_t mii_stat;
    MII_ESRr_t mii_esr;
    MII_GB_STATr_t mii_gb_stat;
    MII_GB_CTRLr_t mii_gb_ctrl;
    MII_ANAr_t mii_ana;
    MII_ANPr_t mii_anp;

    t_speed = 0;
    t_duplex = 0;

    ioerr += READ_MII_STATr(pc, &mii_stat);
    if (MII_STATr_EXTENDED_STATUSf_GET(mii_stat)) {   /* Supports extended status */
        /*
         * If the PHY supports extended status, check if it is 1000MB
         * capable.  If it is, check the 1000Base status register to see
         * if 1000MB negotiated.
         */
        ioerr += READ_MII_ESRr(pc, &mii_esr);
        if (MII_ESRr_X1000BASE_X_FULL_DUPLEX_CAPABLEf_GET(mii_esr) ||
            MII_ESRr_X1000BASE_X_HALF_DUPLEX_CAPABLEf_GET(mii_esr) ||
            MII_ESRr_X1000BASE_T_FULL_DUPLEX_CAPABLEf_GET(mii_esr) ||
            MII_ESRr_X1000BASE_T_HALF_DUPLEX_CAPABLEf_GET(mii_esr)) {
            ioerr += READ_MII_GB_STATr(pc, &mii_gb_stat);
            ioerr += READ_MII_GB_CTRLr(pc, &mii_gb_ctrl);
        }
    }

    /*
     * At this point, if we did not see Gig status, one of mii_gb_stat or
     * mii_gb_ctrl will be 0. This will cause the first 2 cases below to
     * fail and fall into the default 10/100 cases.
     */
    ioerr += READ_MII_ANAr(pc, &mii_ana);
    ioerr += READ_MII_ANPr(pc, &mii_anp);

    if (MII_GB_CTRLr_ADV_1000BASE_T_FULL_DUPLEXf_GET(mii_gb_ctrl) &&
        MII_GB_STATr_LNK_PART_FULL_DUP_ABLEf_GET(mii_gb_stat)) {
        t_speed  = 1000;
        t_duplex = 1;
    } else if (MII_GB_CTRLr_ADV_1000BASE_T_HALF_DUPLEXf_GET(mii_gb_ctrl) &&
               MII_GB_STATr_LNK_PART_HALF_DUP_ABLEf_GET(mii_gb_stat)) {
        t_speed  = 1000;
        t_duplex = 0;
    } else if (MII_ANAr_X100BASE_TX_FULL_DUP_CAPf_GET(mii_ana) &&
               MII_ANPr_X100BASE_TX_FULL_DUP_CAPf_GET(mii_anp)) {
        t_speed = 100;
        t_duplex = 1;
    } else if (MII_ANAr_X100BASE_T4_CAPABLEf_GET(mii_ana) &&
               MII_ANPr_X100BASE_T4_CAPf_GET(mii_anp)) {
        t_speed = 100;
        t_duplex = 0;
    } else if (MII_ANAr_X100BASE_TX_HALF_DUP_CAPf_GET(mii_ana) &&
               MII_ANPr_X100BASE_TX_HALF_DUP_CAPf_GET(mii_anp)) {
        t_speed = 100;
        t_duplex = 0;
    } else if (MII_ANAr_X10BASE_T_FULL_DUP_CAPf_GET(mii_ana) &&
               MII_ANPr_X10BASE_T_FULL_DUP_CAPf_GET(mii_anp)) {
        t_speed = 10;
        t_duplex = 1 ;
    } else if (MII_ANAr_X10BASE_T_HALF_DUP_CAPf_GET(mii_ana) &&
               MII_ANPr_X10BASE_T_HALF_DUP_CAPf_GET(mii_anp)) {
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
 *      _phy84888_xfi_2p5g_5g_mode_set
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
_phy84888_xfi_2p5g_5g_mode_set(phy_ctrl_t *pc, uint32_t mode)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t args[2];
    uint32_t cmd;

    args[0] = mode;
    args[1] = mode;

    cmd = BCM84834_DIAG_CMD_SET_XFI_2P5G_MODE_V2;
    rv = _bcm84888_top_level_cmd_set_v2(pc, cmd, args, 2);

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _bcm84888_mdi_pair_remap
 * Purpose:
 *      Change MDI pair order
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84888_mdi_pair_remap(phy_ctrl_t *pc, uint32_t mapping)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint16_t value;
    uint32_t args[1];
    uint32_t cmd;

    value = (mapping & (0x3 << 12)) >> 6;
    value |= (mapping & (0x3 << 8)) >> 4;
    value |= (mapping & (0x3 << 4)) >> 2;
    value |= mapping & 0x3;

    args[0] = (uint32_t)value;

    cmd = BCM84834_DIAG_CMD_SET_PAIR_SWAP_GENERIC;
    rv = _bcm84888_top_level_cmd_set_v2(pc, cmd, args, 1);

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _bcm84888_eee_mode_set
 * Purpose:
 *      Set EEE mode
 * Parameters:
 *      pc - PHY control structure
 *      mode - EEE mode
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84888_eee_mode_set(phy_ctrl_t *pc, uint32_t mode)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t args[4];
    uint32_t cmd;

    if (mode == PHY_EEE_802_3) {
        args[0] = 5;
        args[1] = 0x0000;
        args[2] = 0x0000;
        args[3] = 0x0000;

        cmd = BCM84834_DIAG_CMD_SET_EEE_MODE_GENERIC;
        rv = _bcm84888_top_level_cmd_set_v2(pc, cmd, args, 4);
        if (CDK_SUCCESS(rv)) {
            rv = PHY_AUTONEG_SET(pc, 1);
        }

        if (CDK_SUCCESS(rv)) {
            rv = _bcm84888_top_level_cmd_get_v2(pc,
                        BCM8484X_DIAG_CMD_GET_EEE_MODE, args, 4);
        }
    } else if (mode == PHY_EEE_AUTO) {
        args[0] = 0xa;
        args[1] = 0x0000;
        args[2] = 0x3d09;
        args[3] = 0x047e;

        cmd = BCM84834_DIAG_CMD_SET_EEE_MODE_GENERIC;
        rv = _bcm84888_top_level_cmd_set_v2(pc, cmd, args, 4);
        if (CDK_SUCCESS(rv)) {
            rv = PHY_AUTONEG_SET(pc, 1);
        }

        if (CDK_SUCCESS(rv)) {
            rv = _bcm84888_top_level_cmd_get_v2(pc,
                        BCM8484X_DIAG_CMD_GET_EEE_MODE, args, 4);
        }
    } else if (mode == PHY_EEE_NONE) {
        args[0] = 0;
        args[1] = 0x0000;
        args[2] = 0x0000;
        args[3] = 0x0000;

        cmd = BCM84834_DIAG_CMD_SET_EEE_MODE_GENERIC;
        rv = _bcm84888_top_level_cmd_set_v2(pc, cmd, args, 4);
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _bcm84888_eee_mode_get
 * Purpose:
 *      Set EEE mode
 * Parameters:
 *      pc - PHY control structure
 *      mode - EEE mode
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84888_eee_mode_get(phy_ctrl_t *pc, uint32_t *mode)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t args[4];

    rv = _bcm84888_top_level_cmd_get_v2(pc,
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
 *      _bcm84888_master_set
 * Purpose:
 *      Set the master mode for the PHY.
 * Parameters:
 *      pc - PHY control structure
 *      master - PHY_MS_xxx
 * Returns:
 *      CDK_E_xxx
 */

static int
_bcm84888_master_set(phy_ctrl_t *pc, uint32_t master)
{
    int ioerr = 0;
    MII_GB_CTRLr_t gb_ctrl;

    if (!IS_COPPER_MODE(pc)) {
        return CDK_E_NONE;
    }

    ioerr += READ_MII_GB_CTRLr(pc, &gb_ctrl);
    switch (master) {
    case PHY_MS_SLAVE:
        MII_GB_CTRLr_MASTER_SLAVE_CONFIG_ENABLEf_SET(gb_ctrl, 1);
        MII_GB_CTRLr_MASTER_SLAVE_CONFIG_VALUEf_SET(gb_ctrl, 0);
        break;
    case PHY_MS_MASTER:
        MII_GB_CTRLr_MASTER_SLAVE_CONFIG_ENABLEf_SET(gb_ctrl, 1);
        MII_GB_CTRLr_MASTER_SLAVE_CONFIG_VALUEf_SET(gb_ctrl, 1);
        break;
    case PHY_MS_AUTO:
        MII_GB_CTRLr_MASTER_SLAVE_CONFIG_ENABLEf_SET(gb_ctrl, 0);
        break;
    }

    ioerr += WRITE_MII_GB_CTRLr(pc, gb_ctrl);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

/*
 * Function:
 *      _bcm84888_master_get
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
_bcm84888_master_get(phy_ctrl_t *pc, uint32_t *master)
{
    int ioerr = 0;
    MII_GB_CTRLr_t gb_ctrl;

    if (!master) {
        return CDK_E_PARAM;
    }

    ioerr += READ_MII_GB_CTRLr(pc, &gb_ctrl);
    if (!MII_GB_CTRLr_MASTER_SLAVE_CONFIG_ENABLEf_GET(gb_ctrl)) {
        *master = PHY_MS_AUTO;
    } else if (MII_GB_CTRLr_MASTER_SLAVE_CONFIG_VALUEf_GET(gb_ctrl)) {
        *master = PHY_MS_MASTER;
    } else {
        *master = PHY_MS_SLAVE;
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

/*
 * Function:
 *      _bcm84888_ability_local_get
 * Purpose:
 *      Get the local abilities of the PHY.
 * Parameters:
 *      pc - PHY control structure
 *      abil - (OUT) ability mask indicating supported options/speeds.
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84888_ability_local_get(phy_ctrl_t *pc, uint32_t *ability)
{
    if (!ability) {
        return CDK_E_PARAM;
    }

    if (IS_COPPER_MODE(pc)) {
        *ability = (PHY_ABIL_100MB_HD | PHY_ABIL_10MB_HD | PHY_ABIL_2500MB |
                    PHY_ABIL_10GB | PHY_ABIL_1000MB_FD | PHY_ABIL_1000MB_HD |
                    PHY_ABIL_100MB_FD | PHY_ABIL_PAUSE | PHY_ABIL_PAUSE_ASYMM |
                    PHY_ABIL_XGMII | PHY_ABIL_LOOPBACK | PHY_ABIL_AN);


        /* Mako supports 5G/2.5G speed     */
        if (is_phy_repeater) {  /* check MAC interface mode */
            /* 5G/2.5G over XFI mode (XGMII) support both speed */
            *ability |= PHY_ABIL_XGMII | PHY_ABIL_NEXT | PHY_ABIL_2500MB;
        } else {
            /* 2500BASE-X mode (SGMII) supports 2.5G only */
            *ability &= ~PHY_ABIL_XGMII;
            *ability |= PHY_ABIL_2500MB | PHY_ABIL_SGMII;
            *ability &= ~PHY_ABIL_XGMII;
        }
    } else {
        *ability = (PHY_ABIL_1000MB_FD | PHY_ABIL_10GB | PHY_ABIL_PAUSE |
                    PHY_ABIL_PAUSE_ASYMM | PHY_ABIL_XGMII | PHY_ABIL_LOOPBACK |
                    PHY_ABIL_AN);
    }

    return CDK_E_NONE;
}

static int
_bcm84888_ability2_local_get(phy_ctrl_t *pc, uint32_t *ability2)
{
    uint32_t abil1;

    if (!ability2) {
        return CDK_E_PARAM;
    }

    *ability2 = 0;
    /* Mako supports 5G speed */
    PHY_CONFIG_GET(pc, PhyConfig_AdvLocal, &abil1, NULL);
    if (is_phy_repeater) {
        *ability2 |= PHY_ABIL2_5000MB;
    }

    return CDK_E_NONE;
}

/*
 * Function:
 *      _bcm84888_ability_remote_get
 * Purpose:
 *      Get the current remoteisement for auto-negotiation.
 * Parameters:
 *      pc - PHY control structure
 *      abil - (OUT) ability mask indicating supported options/speeds.
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84888_copper_ability_remote_get(phy_ctrl_t *pc, uint32_t *ability)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    MII_STATr_t mii_stat;
    MII_CTRLr_t mii_ctrl;
    MII_ANPr_t mii_anp;
    MII_GB_STATr_t mii_gb_stat;
    TENG_AN_STATr_t teng_stat;
    uint32_t an_done, link;

    if (!ability) {
        return CDK_E_PARAM;
    }
    *ability = 0;

    ioerr += READ_MII_STATr(pc, &mii_stat);
    ioerr += READ_MII_CTRLr(pc, &mii_ctrl);
    an_done = MII_STATr_AUTONEG_COMPLETEf_GET(mii_stat);
    link = MII_STATr_LINK_STATUSf_GET(mii_stat);
    if (an_done && link) {
        /* Decode remote advertisement only when link is up and autoneg is completed. */
        ioerr += READ_MII_ANPr(pc, &mii_anp);
        if (MII_ANPr_X100BASE_TX_HALF_DUP_CAPf_GET(mii_anp)) {
            *ability |= PHY_ABIL_100MB_HD;
        }
        if (MII_ANPr_X100BASE_TX_FULL_DUP_CAPf_GET(mii_anp)) {
            *ability |= PHY_ABIL_100MB_FD;
        }
        if (MII_ANPr_X10BASE_T_HALF_DUP_CAPf_GET(mii_anp) ||
            MII_ANPr_X10BASE_T_FULL_DUP_CAPf_GET(mii_anp)) {
            rv = CDK_E_PARAM;
        }

        if (MII_ANPr_PAUSE_CAPABLEf_GET(mii_anp) == 1 &&
            MII_ANPr_LINK_PRTNR_ASYM_PAUSEf_GET(mii_anp) == 0) {
            *ability |= (PHY_ABIL_PAUSE_TX | PHY_ABIL_PAUSE_RX);
        } else if (MII_ANPr_PAUSE_CAPABLEf_GET(mii_anp) == 0 &&
            MII_ANPr_LINK_PRTNR_ASYM_PAUSEf_GET(mii_anp) == 1) {
            *ability |= PHY_ABIL_PAUSE_TX;
        } else if (MII_ANPr_PAUSE_CAPABLEf_GET(mii_anp) == 1 &&
            MII_ANPr_LINK_PRTNR_ASYM_PAUSEf_GET(mii_anp) == 1) {
            *ability |= PHY_ABIL_PAUSE_RX;
        }

        /* GE Specific values */
        ioerr += READ_MII_GB_STATr(pc, &mii_gb_stat);
        if (MII_GB_STATr_LNK_PART_HALF_DUP_ABLEf_GET(mii_gb_stat)) {
            *ability |= PHY_ABIL_1000MB_HD;
        }
        if (MII_GB_STATr_LNK_PART_FULL_DUP_ABLEf_GET(mii_gb_stat)) {
            *ability |= PHY_ABIL_1000MB_FD;
        }

        /* 10G Specific values */
        ioerr += READ_TENG_AN_STATr(pc, &teng_stat);
        if (TENG_AN_STATr_LP_10GBTf_GET(teng_stat)) {
            *ability |= PHY_ABIL_10GB;
        } else {
            *ability &= ~PHY_ABIL_10GB;
        }
    }

    return ioerr ? CDK_E_IO : rv;
}

static int
_bcm84888_copper_ability2_remote_get(phy_ctrl_t *pc, uint32_t *ability2)
{
    if (!ability2) {
        return CDK_E_PARAM;
    }
    *ability2 = 0;

    return CDK_E_NONE;
}

static int
_bcm84888_xaui_ability_remote_get(phy_ctrl_t *pc, uint32_t *ability)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    COMBO_IEEE0_MIISTATr_t mii_stat;
    GP_STATUS_STS3r_t gp_sts3;
    GP_STATUS_LP_UP1r_t gp_up1;
    COMBO_IEEE0_ABILr_t combo_abil;

    if (!ability) {
        return CDK_E_PARAM;
    }
    *ability = 0;

    ioerr += READ_COMBO_IEEE0_MIISTATr(pc, &mii_stat);
    ioerr += READ_GP_STATUS_STS3r(pc, &gp_sts3);
    if (GP_STATUS_STS3r_LINKf_GET(gp_sts3) &&
        COMBO_IEEE0_MIISTATr_AUTONEG_COMPLETEf_GET(mii_stat)) {
        /* Decode remote advertisement only when link is up and autoneg is
         * completed.
         */
        ioerr += READ_GP_STATUS_LP_UP1r(pc, &gp_up1);
        if (GP_STATUS_LP_UP1r_TENGCX4f_GET(gp_up1)) {
            *ability |= PHY_ABIL_10GB;
        }

        ioerr += READ_COMBO_IEEE0_ABILr(pc, &combo_abil);
        if (COMBO_IEEE0_ABILr_X10BASE_T_HALF_DUP_CAPf_GET(combo_abil)) {
            *ability |= PHY_ABIL_1000MB_FD;
        }

        if (COMBO_IEEE0_ABILr_X100BASE_TX_HALF_DUP_CAPf_GET(combo_abil) == 1 &&
            COMBO_IEEE0_ABILr_X100BASE_TX_FULL_DUP_CAPf_GET(combo_abil) == 0) {
            *ability |= (PHY_ABIL_PAUSE_TX | PHY_ABIL_PAUSE_RX);
        } else if (COMBO_IEEE0_ABILr_X100BASE_TX_HALF_DUP_CAPf_GET(combo_abil) == 0 &&
            COMBO_IEEE0_ABILr_X100BASE_TX_FULL_DUP_CAPf_GET(combo_abil) == 1) {
            *ability |= PHY_ABIL_PAUSE_TX;
        } else if (COMBO_IEEE0_ABILr_X100BASE_TX_HALF_DUP_CAPf_GET(combo_abil) == 1 &&
            COMBO_IEEE0_ABILr_X100BASE_TX_FULL_DUP_CAPf_GET(combo_abil) == 1) {
            *ability |= PHY_ABIL_PAUSE_RX;
        }
    }

    return ioerr ? CDK_E_IO : rv;
}

static int
_bcm84888_xaui_ability2_remote_get(phy_ctrl_t *pc, uint32_t *ability2)
{
    if (!ability2) {
        return CDK_E_PARAM;
    }
    *ability2 = 0;

    return CDK_E_NONE;
}

static int
_bcm84888_ability_remote_get(phy_ctrl_t *pc, uint32_t *ability)
{
    int rv = CDK_E_NONE;

    if (IS_COPPER_MODE(pc)) {
        rv = _bcm84888_copper_ability_remote_get(pc, ability);
    } else {
        rv = _bcm84888_xaui_ability_remote_get(pc, ability);
    }

    return rv;
}

static int
_bcm84888_ability2_remote_get(phy_ctrl_t *pc, uint32_t *ability2)
{
    int rv = CDK_E_NONE;

    if (IS_COPPER_MODE(pc)) {
        rv = _bcm84888_copper_ability2_remote_get(pc, ability2);
    } else {
        rv = _bcm84888_xaui_ability2_remote_get(pc, ability2);
    }

    return rv;
}

/*
 * Function:
 *      _bcm84888_ability_eee_remote_get
 * Purpose:
 *      Get the current remote EEE abilities.
 * Parameters:
 *      pc - PHY control structure
 *      abil - (OUT) ability mask indicating supported options/speeds.
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84888_ability_eee_remote_get(phy_ctrl_t *pc, uint32_t *ability)
{
    int ioerr = 0;
    MII_STATr_t mii_stat;
    MII_CTRLr_t mii_ctrl;
    EEE_LPABILr_t eee_abil;
    EEE_ADVr_t eee_adv;
    uint32_t an, an_done, link;

    if (!ability) {
        return CDK_E_PARAM;
    }
    *ability = 0;

    if (IS_COPPER_MODE(pc)) {
        ioerr += READ_MII_STATr(pc, &mii_stat);
        ioerr += READ_MII_CTRLr(pc, &mii_ctrl);
        an = MII_CTRLr_AUTONEG_ENABLEf_GET(mii_ctrl);
        an_done = MII_STATr_AUTONEG_COMPLETEf_GET(mii_stat);
        link = MII_STATr_LINK_STATUSf_GET(mii_stat);
        if (an && an_done && link) {
            ioerr += READ_EEE_LPABILr(pc, &eee_abil);
        } else {
            /* Simply return local abilities */
            ioerr += READ_EEE_ADVr(pc, &eee_adv);
        }
        if (EEE_LPABILr_X10GB_T_EEEf_GET(eee_abil) ||
            EEE_ADVr_X10GB_T_EEEf_GET(eee_adv)) {
            *ability |= PHY_ABIL_EEE_10GB;
        }
        if (EEE_LPABILr_X1000B_T_EEEf_GET(eee_abil) ||
            EEE_ADVr_X1000B_T_EEEf_GET(eee_adv)) {
            *ability |= PHY_ABIL_EEE_1GB;
        }
        if (EEE_LPABILr_X100B_TX_EEEf_GET(eee_abil) ||
            EEE_ADVr_X100B_TX_EEEf_GET(eee_adv)) {
            *ability |= PHY_ABIL_EEE_100MB;
        }
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

/*
 * Function:
 *      _bcm84888_copper_ability_advert_set
 * Purpose:
 *      Set the current advertisement for auto-negotiation.
 * Parameters:
 *      pc - PHY control structure
 *      ability - ability mask indicating supported options/speeds.
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84888_copper_ability_advert_set(phy_ctrl_t *pc, uint32_t ability)
{
    int ioerr = 0;
    MII_GB_CTRLr_t gb_ctrl;
    MII_ANAr_t mii_ana;
    TENG_AN_CTRLr_t an_ctrl;
    MGBASE_AN_CTRLr_t mg_an_ctrl;

    ioerr += READ_MII_GB_CTRLr(pc, &gb_ctrl);
    MII_GB_CTRLr_ADV_1000BASE_T_HALF_DUPLEXf_SET(gb_ctrl, 0);
    MII_GB_CTRLr_ADV_1000BASE_T_FULL_DUPLEXf_SET(gb_ctrl, 0);
    MII_GB_CTRLr_REPEATER_DTEf_SET(gb_ctrl, 1);

    ioerr += READ_MII_ANAr(pc, &mii_ana);
    if (ability & PHY_ABIL_100MB_HD) {
        MII_ANAr_X100BASE_TX_HALF_DUP_CAPf_SET(mii_ana, 1);
    }

    if (ability & PHY_ABIL_100MB_FD) {
        MII_ANAr_X100BASE_TX_FULL_DUP_CAPf_SET(mii_ana, 1);
    }

    if (ability & PHY_ABIL_1000MB_HD) {
        MII_GB_CTRLr_ADV_1000BASE_T_HALF_DUPLEXf_SET(gb_ctrl, 1);
    }
    if (ability & PHY_ABIL_1000MB_FD) {
        MII_GB_CTRLr_ADV_1000BASE_T_FULL_DUPLEXf_SET(gb_ctrl, 1);
    }

    ioerr += READ_TENG_AN_CTRLr(pc, &an_ctrl);
    if (ability & PHY_ABIL_10GB) {
        TENG_AN_CTRLr_X10GBTf_SET(an_ctrl, 1);
    } else {
        TENG_AN_CTRLr_X10GBTf_SET(an_ctrl, 0);
    }
    if (ability & PHY_ABIL_2500MB) {
        TENG_AN_CTRLr_BRSAf_SET(an_ctrl, 1);
    } else {
        TENG_AN_CTRLr_BRSAf_SET(an_ctrl, 0);
    } 
    ioerr += WRITE_TENG_AN_CTRLr(pc, an_ctrl);

    ioerr += READ_MGBASE_AN_CTRLr(pc, &mg_an_ctrl);
    if (ability & PHY_ABIL_2500MB) {
        MGBASE_AN_CTRLr_ADV_2P5Gf_SET(mg_an_ctrl, 1);
        MGBASE_AN_CTRLr_MG_ENABLEf_SET(mg_an_ctrl, 1);
    } else {
        MGBASE_AN_CTRLr_ADV_2P5Gf_SET(mg_an_ctrl, 0);
        MGBASE_AN_CTRLr_MG_ENABLEf_SET(mg_an_ctrl, 0);
    }
    ioerr += WRITE_MGBASE_AN_CTRLr(pc, mg_an_ctrl);

    switch (ability & (PHY_ABIL_PAUSE_TX | PHY_ABIL_PAUSE_RX)) {
    case PHY_ABIL_PAUSE_TX:
        MII_ANAr_ASYMMETRIC_PAUSEf_SET(mii_ana, 1);
        break;
    case PHY_ABIL_PAUSE_RX:
        MII_ANAr_PAUSE_CAPABLEf_SET(mii_ana, 1);
        MII_ANAr_ASYMMETRIC_PAUSEf_SET(mii_ana, 1);
        break;
    case (PHY_ABIL_PAUSE_TX | PHY_ABIL_PAUSE_RX):
        MII_ANAr_PAUSE_CAPABLEf_SET(mii_ana, 1);
        break;
    }
    ioerr += WRITE_MII_ANAr(pc, mii_ana);
    ioerr += WRITE_MII_GB_CTRLr(pc, gb_ctrl);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

/*
 * Function:
 *      _bcm84868_copper_ability2_advert_set
 * Purpose:
 *      Set the current advertisement for auto-negotiation.
 * Parameters:
 *      pc - PHY control structure
 *      ability - ability mask indicating supported options/speeds.
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84888_copper_ability2_advert_set(phy_ctrl_t *pc, uint32_t ability2)
{
    int ioerr = 0;
    MGBASE_AN_CTRLr_t mg_an_ctrl;
    TENG_AN_CTRLr_t an_ctrl;

    /* 5G Specific values */
    ioerr += READ_MGBASE_AN_CTRLr(pc, &mg_an_ctrl);
    ioerr += READ_TENG_AN_CTRLr(pc, &an_ctrl);
    if (ability2 & PHY_ABIL2_5000MB) {
        MGBASE_AN_CTRLr_ADV_5Gf_SET(mg_an_ctrl, 1);
        TENG_AN_CTRLr_BMLT3f_SET(an_ctrl, 1);
    } else {
        MGBASE_AN_CTRLr_ADV_5Gf_SET(mg_an_ctrl, 0);
        TENG_AN_CTRLr_BMLT3f_SET(an_ctrl, 0);
    }
    ioerr += WRITE_MGBASE_AN_CTRLr(pc, mg_an_ctrl);
    ioerr += WRITE_TENG_AN_CTRLr(pc, an_ctrl);
    
    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

static int
_bcm84888_fiber_ability_advert_set(phy_ctrl_t *pc, uint32_t ability)
{
    int ioerr = 0;
    COMBO_IEEE0_ADVr_t combo_adv;
    OVER1G_UP1r_t ovr1g;

    ioerr += READ_COMBO_IEEE0_ADVr(pc, &combo_adv);
    if (ability & PHY_ABIL_1000MB_FD) {
        COMBO_IEEE0_ADVr_C37_FDf_SET(combo_adv, 1);
    }
    switch (ability & (PHY_ABIL_PAUSE_TX | PHY_ABIL_PAUSE_RX)) {
    case PHY_ABIL_PAUSE_TX:
        COMBO_IEEE0_ADVr_C37_ASYM_PAUSEf_SET(combo_adv, 1);
        break;
    case PHY_ABIL_PAUSE_RX:
        COMBO_IEEE0_ADVr_C37_ASYM_PAUSEf_SET(combo_adv, 1);
        COMBO_IEEE0_ADVr_C37_PAUSEf_SET(combo_adv, 1);
        break;
    case PHY_ABIL_PAUSE_TX | PHY_ABIL_PAUSE_RX:
        COMBO_IEEE0_ADVr_C37_PAUSEf_SET(combo_adv, 1);
        break;
    }
    ioerr += WRITE_COMBO_IEEE0_ADVr(pc, combo_adv);

    ioerr += READ_OVER1G_UP1r(pc, &ovr1g);
    if (ability & PHY_ABIL_10GB) {
        OVER1G_UP1r_TENGCX4f_SET(ovr1g, 1);
    }
    ioerr += WRITE_OVER1G_UP1r(pc, ovr1g);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

static int
_bcm84888_ability_advert_set(phy_ctrl_t *pc, uint32_t ability)
{
    int rv = CDK_E_NONE;

    if (IS_COPPER_MODE(pc)) {
        rv = _bcm84888_copper_ability_advert_set(pc, ability);
    } else {
        rv = _bcm84888_fiber_ability_advert_set(pc, ability);
    }

    return rv;
}


/*
 * Function:
 *      _bcm84888_ability_advert_get
 * Purpose:
 *      Get the current advertisement for auto-negotiation.
 * Parameters:
 *      pc - PHY control structure
 *      ability - ability mask indicating supported options/speeds.
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84888_copper_ability_advert_get(phy_ctrl_t *pc, uint32_t *ability)
{
    int ioerr = 0;
    MII_ANAr_t mii_ana;
    MII_GB_CTRLr_t mii_gb_ctrl;
    TENG_AN_CTRLr_t an_ctrl;
    MGBASE_AN_CTRLr_t mg_an_ctrl;

    if (!ability) {
        return CDK_E_PARAM;
    }
    *ability = 0;

    /* Copper AutoNeg Advertisement Reg.(7.0xFFE4) */
    ioerr += READ_MII_ANAr(pc, &mii_ana);

    if (MII_ANAr_X100BASE_TX_HALF_DUP_CAPf_GET(mii_ana)) {
        *ability |= PHY_ABIL_100MB_HD;
    }
    if (MII_ANAr_X100BASE_TX_FULL_DUP_CAPf_GET(mii_ana)) {
        *ability |= PHY_ABIL_100MB_FD;
    }

    if (MII_ANAr_X10BASE_T_HALF_DUP_CAPf_GET(mii_ana) ||
        MII_ANAr_X10BASE_T_FULL_DUP_CAPf_GET(mii_ana)) {
        return CDK_E_PARAM;
    }

    if (MII_ANAr_PAUSE_CAPABLEf_GET(mii_ana) == 1 &&
        MII_ANAr_ASYMMETRIC_PAUSEf_GET(mii_ana) == 0) {
        *ability |= (PHY_ABIL_PAUSE_TX | PHY_ABIL_PAUSE_RX);
    } else if (MII_ANAr_PAUSE_CAPABLEf_GET(mii_ana) == 0 &&
        MII_ANAr_ASYMMETRIC_PAUSEf_GET(mii_ana) == 1) {
        *ability |= PHY_ABIL_PAUSE_TX;
    } else if (MII_ANAr_PAUSE_CAPABLEf_GET(mii_ana) == 1 &&
        MII_ANAr_ASYMMETRIC_PAUSEf_GET(mii_ana) == 1) {
        *ability |= PHY_ABIL_PAUSE_RX;
    }

    /* GE Specific values */
    /* 1000BASE-T Control Reg.(7.0xFFE9) */
    ioerr += READ_MII_GB_CTRLr(pc, &mii_gb_ctrl);
    if (MII_GB_CTRLr_ADV_1000BASE_T_HALF_DUPLEXf_GET(mii_gb_ctrl)) {
        *ability |= PHY_ABIL_1000MB_HD;
    }
    if (MII_GB_CTRLr_ADV_1000BASE_T_FULL_DUPLEXf_GET(mii_gb_ctrl)) {
        *ability |= PHY_ABIL_1000MB_FD;
    }

    /* 10G Specific values */
    /* 10GBASE-T AN Control Reg.(7.0x0020) */
    ioerr += READ_TENG_AN_CTRLr(pc, &an_ctrl);
    if (TENG_AN_CTRLr_X10GBTf_GET(an_ctrl)) {
        *ability |= PHY_ABIL_10GB;
    }

    /* 5G / 2.5G Specific values */
    /* MGBASE-T AN Control Reg.(30.0x0000), Advert_5G/2.5G_capability[2:1] */
    ioerr += READ_MGBASE_AN_CTRLr(pc, &mg_an_ctrl);
    if (MGBASE_AN_CTRLr_ADV_2P5Gf_GET(mg_an_ctrl)) {
        *ability |= PHY_ABIL_2500MB;
    }
    if (MGBASE_AN_CTRLr_ADV_5Gf_GET(mg_an_ctrl)) {
        *ability |= PHY_ABIL_NEXT;
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

STATIC int
_bcm84888_fiber_ability_advert_get(phy_ctrl_t *pc, uint32_t *ability)
{
    int ioerr = 0;
    COMBO_IEEE0_ADVr_t combo_adv;
    OVER1G_UP1r_t ovr1g;

    if (!ability) {
        return CDK_E_PARAM;
    }
    *ability = 0;

    ioerr += READ_COMBO_IEEE0_ADVr(pc, &combo_adv);
    if (COMBO_IEEE0_ADVr_C37_ASYM_PAUSEf_GET(combo_adv) == 1 &&
        COMBO_IEEE0_ADVr_C37_PAUSEf_GET(combo_adv) == 0) {
        *ability |= PHY_ABIL_PAUSE_TX;
    } else if (COMBO_IEEE0_ADVr_C37_ASYM_PAUSEf_GET(combo_adv) == 0 &&
        COMBO_IEEE0_ADVr_C37_PAUSEf_GET(combo_adv) == 1) {
        *ability |= (PHY_ABIL_PAUSE_TX | PHY_ABIL_PAUSE_RX);
    } else if (COMBO_IEEE0_ADVr_C37_ASYM_PAUSEf_GET(combo_adv) == 1 &&
        COMBO_IEEE0_ADVr_C37_PAUSEf_GET(combo_adv) == 1) {
        *ability |= PHY_ABIL_PAUSE_RX;
    }

    if (COMBO_IEEE0_ADVr_C37_FDf_GET(combo_adv)) {
        *ability |= PHY_ABIL_1000MB_FD;
    }

    ioerr += READ_OVER1G_UP1r(pc, &ovr1g);
    if (OVER1G_UP1r_TENGCX4f_GET(ovr1g)) {
        *ability |= PHY_ABIL_10GB;
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

static int
_bcm84888_ability2_advert_set(phy_ctrl_t *pc, uint32_t ability2)
{
    int rv = CDK_E_NONE;

    if (IS_COPPER_MODE(pc)) {
        rv = _bcm84888_copper_ability2_advert_set(pc, ability2);
    }

    return rv;
}

/*
 * Function:
 *      _bcm84888_medium_change
 * Purpose:
 *      Check the link medium.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84888_medium_change(phy_ctrl_t *pc)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int medium, changed = 0;
    TOP_LINK_STATUSr_t top_link_status;
    MDIO_CTRL_1r_t mdio_ctl_1;
    uint32_t ability;

    /* Check the connected medium */
    ioerr += READ_TOP_LINK_STATUSr(pc, &top_link_status);
    if (TOP_LINK_STATUSr_PRIOf_GET(top_link_status)) {
        medium = PHY_MEDIUM_FIBER;
        if (!TOP_LINK_STATUSr_FIBER_DETECTf_GET(top_link_status) &&
            TOP_LINK_STATUSr_COPPER_DETECTf_GET(top_link_status)) {
            medium = PHY_MEDIUM_COPPER;
        }
    } else {
        medium = PHY_MEDIUM_COPPER;
        if (!TOP_LINK_STATUSr_COPPER_DETECTf_GET(top_link_status) &&
             TOP_LINK_STATUSr_FIBER_DETECTf_GET(top_link_status)) {
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
            MDIO_CTRL_1r_SET(mdio_ctl_1, 0x0003);
        } else {
            MDIO_CTRL_1r_SET(mdio_ctl_1, 0x2004);
        }
        ioerr += WRITE_MDIO_CTRL_1r(pc, mdio_ctl_1);

        /* Update the ability advertisement */
        rv = _bcm84888_ability_local_get(pc, &ability);
        if (CDK_SUCCESS(rv)) {
            rv = _bcm84888_ability_advert_set(pc, ability);
        }
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _bcm84888_init_stage_0
 * Purpose:
 *      PHY init.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84888_init_stage_0(phy_ctrl_t *pc)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    TOP_FW_REVr_t fw_rev;
    TOP_XGPHY_STRAP1r_t strap1;
    PMD_CTRLr_t pmd_ctrl;
    int cnt;

    _PHY_DBG(pc, ("init_stage_0\n"));

    PHY_CTRL_FLAGS(pc) |= PHY_F_CLAUSE45;
    PHY_CTRL_FLAGS(pc) &= ~PHY_F_FIBER_MODE;

    /* disable bit7 for 10G, need to clean it */
    ioerr += READ_TOP_XGPHY_STRAP1r(pc, &strap1);
    TOP_XGPHY_STRAP1r_XGPH_DISABLEDf_SET(strap1, 0);
    ioerr += WRITE_TOP_XGPHY_STRAP1r(pc, strap1);

    ioerr += READ_TOP_FW_REVr(pc, &fw_rev);
    if (!TOP_FW_REVr_GET(fw_rev) && !(PHY_CTRL_FLAGS(pc) & PHY_F_FW_MDIO_LOAD)) {

        /* Reset the device PMA/PMD and PCS */
        ioerr += READ_PMD_CTRLr(pc, &pmd_ctrl);
        PMD_CTRLr_RESETf_SET(pmd_ctrl, 1);
        ioerr += WRITE_PMD_CTRLr(pc, pmd_ctrl);

        /* Wait for reset completion */
        for (cnt = 0; cnt < PHY_RESET_POLL_MAX; cnt++) {
            ioerr += READ_PMD_CTRLr(pc, &pmd_ctrl);
            if (PMD_CTRLr_RESETf_GET(pmd_ctrl) == 0) {
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
 *      _bcm84888_init_stage_1
 * Purpose:
 *      PHY init.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84888_init_stage_1(phy_ctrl_t *pc)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    TOP_SCRATCH_2r_t top2;
    TOP_SCRATCH_1r_t top1;

    _PHY_DBG(pc, ("init_stage_1\n"));

    if (PHY_CTRL_FLAGS(pc) & PHY_F_FW_MDIO_LOAD) {
        /* Turn on broadcast mode */
        TOP_SCRATCH_2r_SET(top2, 0xf003);
        ioerr += WRITE_TOP_SCRATCH_2r(pc, top2);
        TOP_SCRATCH_1r_SET(top1, 0x0401);
        ioerr += WRITE_TOP_SCRATCH_1r(pc, top1);

        rv = _bcm84888_halt(pc);
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _bcm84888_init_stage_2
 * Purpose:
 *      PHY init.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84888_init_stage_2(phy_ctrl_t *pc)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    MDIO2ARM_CTLr_t mdio_ctl;
    MDIO2ARM_ADDR_LOWr_t adr_low;
    MDIO2ARM_ADDR_HIGHr_t adr_hi;
    MDIO2ARM_DATA_LOWr_t data_lo;
    MDIO2ARM_DATA_HIGHr_t data_hi;
    PMAD_IEEE_DEV_ID1r_t dev_id1;
    uint32_t rev;
    uint32_t save_phy_addr;

    _PHY_DBG(pc, ("init_stage_2\n"));

    if (PHY_CTRL_FLAGS(pc) & PHY_F_FW_MDIO_LOAD) {
        if ((PHY_CTRL_FLAGS(pc) & PHY_F_STAGED_INIT) &&
            !(PHY_CTRL_FLAGS(pc) & PHY_F_BCAST_MSTR)) {
            return CDK_E_NONE;
        }

        save_phy_addr = PHY_CTRL_ADDR(pc);
        PHY_CTRL_ADDR(pc) &= ~0x1f;

        ioerr += READ_PMAD_IEEE_DEV_ID1r(pc, &dev_id1);
        rev = PMAD_IEEE_DEV_ID1r_REVf_GET(dev_id1);
        if ((rev == 0) || (rev == 8)) {
            rv = _bcm84888_write_to_arm(pc, bcm_84888A0_firmware, bcm_84888A0_firmware_size);
        } else {
            rv = _bcm84888_write_to_arm(pc, bcm_84888B0_firmware, bcm_84888B0_firmware_size);
        }

        if (CDK_SUCCESS(rv)) {
            MDIO2ARM_CTLr_CLR(mdio_ctl);
            ioerr += WRITE_MDIO2ARM_CTLr(pc, mdio_ctl);
            MDIO2ARM_ADDR_LOWr_CLR(adr_low);
            ioerr += WRITE_MDIO2ARM_ADDR_LOWr(pc, adr_low);
            MDIO2ARM_ADDR_HIGHr_SET(adr_hi, 0xc300);
            ioerr += WRITE_MDIO2ARM_ADDR_HIGHr(pc, adr_hi);

            MDIO2ARM_DATA_LOWr_CLR(data_lo);
            ioerr += WRITE_MDIO2ARM_DATA_LOWr(pc, data_lo);
            MDIO2ARM_DATA_HIGHr_CLR(data_hi);
            ioerr += WRITE_MDIO2ARM_DATA_HIGHr(pc, data_hi);

            MDIO2ARM_CTLr_SIZEf_SET(mdio_ctl, 2);
            MDIO2ARM_CTLr_WRf_SET(mdio_ctl, 1);
            ioerr += WRITE_MDIO2ARM_CTLr(pc, mdio_ctl);
        }

        PHY_CTRL_ADDR(pc) = save_phy_addr;
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _bcm84888_init_stage_3
 * Purpose:
 *      PHY init.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84888_init_stage_3(phy_ctrl_t *pc)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    PMAD_SCRATCH_8r_t pmad8;
    TOP_SCRATCH_7r_t top7;
    TOP_SCRATCH_9r_t top9;
    TOP_SCRATCH_1r_t top1;
    TOP_FW_REVr_t top_ver;
    TOP_FW_DATEr_t top_date;
    TOP_XGPHY_STRAP1r_t strap1;
    TOP_SCRATCH_3r_t top3;
    PMD_CTRLr_t pmd_ctrl;

    _PHY_DBG(pc, ("init_stage_3\n"));

    if (PHY_CTRL_FLAGS(pc) & PHY_F_FW_MDIO_LOAD) {
        /* Turn off broadcast mode */
        /* Before reset the processor, add OSF setting for all ports  */
        PMAD_SCRATCH_8r_CLR(pmad8);
        ioerr += WRITE_PMAD_SCRATCH_8r(pc, pmad8);
        TOP_SCRATCH_7r_SET(top7, 0x5555);
        ioerr += WRITE_TOP_SCRATCH_7r(pc, top7);

        ioerr += READ_TOP_SCRATCH_9r(pc, &top9);
        TOP_SCRATCH_9r_SW_RSTf_SET(top9, 1);
        ioerr += WRITE_TOP_SCRATCH_9r(pc, top9);
        PHY_SYS_USLEEP(200);

        PMD_CTRLr_SET(pmd_ctrl, 0x8000);
        ioerr += WRITE_PMD_CTRLr(pc, pmd_ctrl);

        TOP_SCRATCH_1r_CLR(top1);
        ioerr += WRITE_TOP_SCRATCH_1r(pc, top1);

        /* Clear f/w ver. regs. */
        TOP_FW_REVr_CLR(top_ver);
        ioerr += WRITE_TOP_FW_REVr(pc, top_ver);
        TOP_FW_DATEr_CLR(top_date);
        ioerr += WRITE_TOP_FW_DATEr(pc, top_date);

        ioerr += READ_TOP_XGPHY_STRAP1r(pc, &strap1);
        if (TOP_XGPHY_STRAP1r_CFG_13_STRAPf_GET(strap1)) {
            /* Now reset only the ARM core */
            TOP_SCRATCH_3r_SET(top3, 0x0040);
            ioerr += WRITE_TOP_SCRATCH_3r(pc, top3);
            TOP_SCRATCH_3r_CLR(top3);
            ioerr += WRITE_TOP_SCRATCH_3r(pc, top3);
        } else {
            ioerr += READ_PMD_CTRLr(pc, &pmd_ctrl);
            PMD_CTRLr_RESETf_SET(pmd_ctrl, 1);
            ioerr += WRITE_PMD_CTRLr(pc, pmd_ctrl);
        }
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _bcm84888_init_stage_4
 * Purpose:
 *      PHY init.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84888_init_stage_4(phy_ctrl_t *pc)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    TOP_FW_REVr_t fw_rev;
    TOP_LINK_STATUSr_t top_link_status;
    uint8_t chip_rev, main_ver, branch, crc_status;
    PMD_CTRLr_t pmd_ctrl;
    int cnt;
    uint32_t fw_ver;

    _PHY_DBG(pc, ("init_stage_4\n"));

    /* Wait for reset completion */
    for (cnt = 0; cnt < PHY_CMD_POLL_MAX; cnt++) {
        rv = READ_TOP_FW_REVr(pc, &fw_rev);
        fw_ver = TOP_FW_REVr_GET(fw_rev);
        if (rv != 0) {
            break;
        }

        rv = READ_PMD_CTRLr(pc, &pmd_ctrl);
        if (rv != 0) {
            break;
        }
        
        PHY_SYS_USLEEP(10000);
        if (fw_ver && !PMD_CTRLr_RESETf_GET(pmd_ctrl)) {
            /* version is set and the reset bit has cleared */
            break;
        }
    }

    if (PMD_CTRLr_RESETf_GET(pmd_ctrl) || (!fw_ver)) {
        PHY_WARN(pc, ("Firmware might not be running\n"));
    }

    /* Check the firmware version */
    ioerr += READ_TOP_FW_REVr(pc, &fw_rev);
    chip_rev = TOP_FW_REVr_CHIP_REVf_GET(fw_rev);
    main_ver = TOP_FW_REVr_MAINf_GET(fw_rev);
    branch = TOP_FW_REVr_BRANCHf_GET(fw_rev);
    PHY_VERB(pc, ("BCM84888: port: %d, chip rev:%d, version:%d.%d\n",
                                        pc->port, chip_rev, main_ver, branch));

    /* Check the SPIROM CRC status */
    ioerr += READ_TOP_LINK_STATUSr(pc, &top_link_status);
    crc_status = TOP_LINK_STATUSr_SPIROM_CRC_STATf_GET(top_link_status);
    PHY_VERB(pc, ("BCM84888: port: %d, SPIROM CRC check status:%d\n",
                                        pc->port, crc_status));

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _bcm84888_init_stage_5
 * Purpose:
 *      PHY init.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84888_init_stage_5(phy_ctrl_t *pc)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    TOP_XGPHY_STRAP1r_t strap1;
    AUX_CTRL_SHAD1Cr_t aux_shad1c;
    TENG_AN_CTRLr_t an_ctrl;
    uint32_t ability;
    uint32_t arg[1];
    uint32_t cmd;

    _PHY_DBG(pc, ("init_stage_5\n"));

    if (PHY_CTRL_NEXT(pc)) {
        PHY_NOTIFY(PHY_CTRL_NEXT(pc), PhyEvent_PhyEnable);
    }

    /* MAC interface is 5G/2.5G over XFI mode */
    rv = _phy84888_xfi_2p5g_5g_mode_set(pc, 0);

    /* Update the ability advertisement */
    rv = _bcm84888_ability_local_get(pc, &ability);

    if (CDK_SUCCESS(rv)) {
        rv = _bcm84888_ability_advert_set(pc, ability);
    }

    /* Disable carrier extension */
    ioerr += READ_AUX_CTRL_SHAD1Cr(pc, &aux_shad1c);
    AUX_CTRL_SHAD1Cr_BIT6f_SET(aux_shad1c, 1);
    ioerr += WRITE_AUX_CTRL_SHAD1Cr(pc, aux_shad1c);

    ioerr += _phy84888_shadow_reg_write(pc, 0x1c, 0x1b, 1 << 6, 0, 1 << 6);
    if (!is_phy_repeater) {
        /* for 2500base-X, the adv max speed is 2500, force not to use 10G */
        ioerr += READ_TENG_AN_CTRLr(pc, &an_ctrl);
        TENG_AN_CTRLr_X10GBTf_SET(an_ctrl, 0);
        ioerr += WRITE_TENG_AN_CTRLr(pc, an_ctrl);
    }

    /* Enable Jumbo Packet */
    arg[0] = 1;
    cmd = BCM848x_DIAG_CMD_SET_JUMBO_FRAME_MODE;
    rv = _bcm84888_top_level_cmd_set_v2(pc, cmd, arg, 1);

    /* Remove super isolate */
    ioerr += READ_TOP_XGPHY_STRAP1r(pc, &strap1);
    TOP_XGPHY_STRAP1r_SUPER_ISO_CHANGEf_SET(strap1, 0);
    ioerr += WRITE_TOP_XGPHY_STRAP1r(pc, strap1);

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _bcm84888_init_stage
 * Purpose:
 *      Execute specified init stage.
 * Parameters:
 *      pc - PHY control structure
 *      stage - init stage
 * Returns:
 *      CDK_E_xxx
 */
static int
_bcm84888_init_stage(phy_ctrl_t *pc, int stage)
{
    switch (stage) {
    case 0:
        return _bcm84888_init_stage_0(pc);
    case 1:
        return _bcm84888_init_stage_1(pc);
    case 2:
        return _bcm84888_init_stage_2(pc);
    case 3:
        return _bcm84888_init_stage_3(pc);
    case 4:
        return _bcm84888_init_stage_4(pc);
    case 5:
        return _bcm84888_init_stage_5(pc);
    default:
        break;
    }
    return CDK_E_UNAVAIL;
}

/***********************************************************************
 *
 * PHY DRIVER FUNCTIONS
 */

#if PHY_CONFIG_INCLUDE_CHIP_SYMBOLS == 1
extern cdk_symbols_t bcm84888_symbols;
#define SET_SYMBOL_TABLE(_pc) \
    PHY_CTRL_SYMBOLS(_pc) = &bcm84888_symbols
#else
#define SET_SYMBOL_TABLE(_pc)
#endif

/*
 * Function:
 *      bcm84888_phy_probe
 * Purpose:
 *      Probe for 84888 PHY
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
bcm84888_phy_probe(phy_ctrl_t *pc)
{
    int ioerr = 0;
    PMAD_IEEE_DEV_ID0r_t dev_id0;
    PMAD_IEEE_DEV_ID1r_t dev_id1;
    uint32_t phyid0, phyid1;
    uint32_t rev;

    PHY_CTRL_CHECK(pc);

    ioerr += READ_PMAD_IEEE_DEV_ID0r(pc, &dev_id0);
    ioerr += READ_PMAD_IEEE_DEV_ID1r(pc, &dev_id1);
    if (ioerr) {
        return CDK_E_IO;
    }
    phyid0 = PMAD_IEEE_DEV_ID0r_GET(dev_id0);
    phyid1 = PMAD_IEEE_DEV_ID1r_GET(dev_id1) & ~PHY_ID1_REV_MASK;
    rev = PMAD_IEEE_DEV_ID1r_REVf_GET(dev_id1);
    if (phyid0 == BCM84888_PMA_PMD_ID0 &&
       (phyid1 == (BCM84888_PMA_PMD_ID1 & ~PHY_ID1_REV_MASK)) &&
       ((rev == 0x0) || (rev == 0x1)) ) {
#if PHY_CONFIG_EXTERNAL_BOOT_ROM == 0
        /* Use MDIO download if external ROM is disabled */
        PHY_CTRL_FLAGS(pc) |= PHY_F_FW_MDIO_LOAD;
#endif

        SET_SYMBOL_TABLE(pc);
        return ioerr ? CDK_E_IO : CDK_E_NONE;

    }
    return CDK_E_NOT_FOUND;
}

/*
 * Function:
 *      bcm84888_phy_notify
 * Purpose:
 *      Handle PHY notifications
 * Parameters:
 *      pc - PHY control structure
 *      event - PHY event
 * Returns:
 *      CDK_E_xxx
 */
static int
bcm84888_phy_notify(phy_ctrl_t *pc, phy_event_t event)
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
 *      bcm84888_phy_reset
 * Purpose:
 *      Reset 84888 PHY
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
bcm84888_phy_reset(phy_ctrl_t *pc)
{
    PHY_CTRL_CHECK(pc);

    return CDK_E_NONE;
}

/*
 * Function:
 *      bcm84888_phy_init
 * Purpose:
 *      Initialize 84888 PHY driver
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
bcm84888_phy_init(phy_ctrl_t *pc)
{
    int rv = CDK_E_NONE;
    int stage;

    PHY_CTRL_CHECK(pc);

    if (PHY_CTRL_FLAGS(pc) & PHY_F_STAGED_INIT) {
        PHY_CTRL_FLAGS(pc) &= ~PHY_F_STAGED_INIT;
    }

    for (stage = 0; CDK_SUCCESS(rv); stage++) {
        rv = _bcm84888_init_stage(pc, stage);
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
 *      bcm84888_phy_link_get
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
bcm84888_phy_link_get(phy_ctrl_t *pc, int *link, int *autoneg_done)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    MII_STATr_t mii_stat;
    MII_CTRLr_t mii_ctrl;
    PCS_IEEE_ST1r_t pcs_mii_stat;
    TENG_AN_CTRLr_t teng_an;
    PMD_CTRLr_t pmd_ctl;
    TOP_LINK_STATUSr_t top_lnk;
    uint32_t speed, autoneg;

    PHY_CTRL_CHECK(pc);

    if (!link || !autoneg_done) {
        return CDK_E_PARAM;
    }
    *link = 0;
    *autoneg_done = 0;

    if (IS_COPPER_MODE(pc)) { /* Connected at copper mode */
        ioerr += READ_MII_STATr(pc, &mii_stat);
        *autoneg_done = MII_STATr_AUTONEG_COMPLETEf_GET(mii_stat);

        ioerr += READ_MII_CTRLr(pc, &mii_ctrl);
        autoneg = MII_CTRLr_AUTONEG_ENABLEf_GET(mii_ctrl);
        if (autoneg == TRUE && *autoneg_done == FALSE) {
            *link = FALSE;
            return CDK_E_NONE;
        }

        /* Check is to handle removable PHY daughter cards */
        ioerr += READ_PCS_IEEE_ST1r(pc, &pcs_mii_stat);
        if (PCS_IEEE_ST1r_GET(pcs_mii_stat) == 0xffff) {
            *link = FALSE;
            return CDK_E_NONE;
        }

        /* Check if the current speed is set to 10G */
        speed = 0;
        if (autoneg) {
            ioerr += READ_TENG_AN_CTRLr(pc, &teng_an);
            if (TENG_AN_CTRLr_X10GBTf_GET(teng_an) &&
                MII_STATr_LINK_STATUSf_GET(mii_stat)) {
                speed = 10000;
            }
        } else {
            ioerr += READ_PMD_CTRLr(pc, &pmd_ctl);
            if (PMD_CTRLr_SPEED_SEL_10Gf_GET(pmd_ctl) == 0 &&
                PMD_CTRLr_SPEED_SEL_1f_GET(pmd_ctl) == 1 &&
                PMD_CTRLr_SPEED_SEL_0f_GET(pmd_ctl) == 1) {
                speed = 10000;
            }
        }

        *link = MII_STATr_LINK_STATUSf_GET(mii_stat);

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
        ioerr += READ_TOP_LINK_STATUSr(pc, &top_lnk);
        *link = FALSE;
        if (TOP_LINK_STATUSr_GET(top_lnk) != 0xffff &&
             TOP_LINK_STATUSr_FIBER_LINKf_GET(top_lnk)) {
            *link = TRUE;
        }
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      bcm84888_phy_duplex_set
 * Purpose:
 *      Set the current duplex mode (forced).
 * Parameters:
 *      pc - PHY control structure
 *      duplex - non-zero indicates full duplex, zero indicates half
 * Returns:
 *      CDK_E_xxx
 */
static int
bcm84888_phy_duplex_set(phy_ctrl_t *pc, int duplex)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    MII_CTRLr_t mii_ctrl;
    COMBO_IEEE0_MIICNTLr_t combo_mii_ctrl;

    PHY_CTRL_CHECK(pc);

    if (IS_COPPER_MODE(pc)) { /* Connected at copper mode */
        ioerr += READ_MII_CTRLr(pc, &mii_ctrl);
        MII_CTRLr_DUPLEX_MODEf_SET(mii_ctrl, duplex);
        ioerr += WRITE_MII_CTRLr(pc, mii_ctrl);
    } else { /* Connected at fiber mode */
        ioerr += READ_COMBO_IEEE0_MIICNTLr(pc, &combo_mii_ctrl);
        COMBO_IEEE0_MIICNTLr_DUPLEX_MODEf_SET(combo_mii_ctrl, duplex);
        ioerr += WRITE_COMBO_IEEE0_MIICNTLr(pc, combo_mii_ctrl);
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      bcm84888_phy_duplex_get
 * Purpose:
 *      Get the current operating duplex mode.
 * Parameters:
 *      pc - PHY control structure
 *      duplex - (OUT) non-zero indicates full duplex, zero indicates half
 * Returns:
 *      CDK_E_xxx
 */
static int
bcm84888_phy_duplex_get(phy_ctrl_t *pc, int *duplex)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    MII_CTRLr_t mii_ctrl;
    MII_STATr_t mii_stat;
    GP_STATUS_STATUS1000X1r_t gp_sts;
    COMBO_IEEE0_MIICNTLr_t combo_mii_ctrl;
    COMBO_IEEE0_ABILr_t combo_abil;
    uint32_t speed;
    int lb_mode;

    PHY_CTRL_CHECK(pc);

    if (!duplex) {
        return CDK_E_PARAM;
    }

    if (IS_COPPER_MODE(pc)) { /* Connected at copper mode */
        rv = PHY_LOOPBACK_GET(pc, &lb_mode);
        speed = LB_SPEED_GET(pc);

        if ((!lb_mode) || (!speed)) {
            rv = PHY_SPEED_GET(pc, &speed);
        }

        if (CDK_SUCCESS(rv)) {
            if (speed == 10000 || speed == 0 || speed == 5000 || speed == 2500) {
                *duplex =  TRUE;
                return CDK_E_NONE;
            }
        }

        ioerr += READ_MII_CTRLr(pc, &mii_ctrl);
        ioerr += READ_MII_STATr(pc, &mii_stat);
        if (MII_CTRLr_AUTONEG_ENABLEf_GET(mii_ctrl)) {     /* Auto-negotiation enabled */
            if (!MII_STATr_AUTONEG_COMPLETEf_GET(mii_stat)) { /* Auto-neg NOT complete */
                *duplex = FALSE;
            } else {
                rv = _bcm84888_auto_negotiate_gcd(pc, NULL, duplex);
            }
        } else {                /* Auto-negotiation disabled */
            *duplex = MII_CTRLr_DUPLEX_MODEf_GET(mii_ctrl);
        }
    } else { /* Connected at fiber mode */
        *duplex = TRUE;

        ioerr += READ_GP_STATUS_STATUS1000X1r(pc, &gp_sts);
        if (GP_STATUS_STATUS1000X1r_SGMII_MODEf_GET(gp_sts)) {
            /* Retrieve the duplex setting in SGMII mode */
            ioerr += READ_COMBO_IEEE0_MIICNTLr(pc, &combo_mii_ctrl);
            if (COMBO_IEEE0_MIICNTLr_AUTONEG_ENABLEf_GET(combo_mii_ctrl)) {
                ioerr += READ_COMBO_IEEE0_ABILr(pc, &combo_abil);
                /* Make sure link partner is also in SGMII mode
                 * otherwise fall through to use the FD bit in MII_CTRL reg
                 */
                if (COMBO_IEEE0_ABILr_SGMII_MODEf_GET(combo_abil)) {
                    *duplex = COMBO_IEEE0_ABILr_SGMII_FDf_GET(combo_abil) ? TRUE : FALSE;

                    return CDK_E_NONE;
                }
            }
            *duplex = COMBO_IEEE0_MIICNTLr_DUPLEX_MODEf_GET(combo_mii_ctrl);
        }
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      bcm84888_phy_speed_set
 * Purpose:
 *      Set the current operating speed (forced).
 * Parameters:
 *      pc - PHY control structure
 *      speed - new link speed
 * Returns:
 *      CDK_E_xxx
 */
static int
bcm84888_phy_speed_set(phy_ctrl_t *pc, uint32_t speed)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    MGBASE_AN_CTRLr_t mg_an_ctrl;
    PMD_CTRLr_t pmd_ctrl;
    MII_CTRLr_t mii_ctrl;
    TOP_XGPHY_STRAP1r_t strap1;
    int an;
    uint32_t cur_speed;

    PHY_CTRL_CHECK(pc);

    rv = PHY_AUTONEG_GET(pc, &an);
    if (CDK_FAILURE(rv) || an) {
        return rv;
    }

    /* Leave hardware alone if speed is unchanged */
    rv = PHY_SPEED_GET(pc, &cur_speed);
    if (CDK_SUCCESS(rv) && speed == cur_speed) {
        return CDK_E_NONE;
    }

    if (IS_COPPER_MODE(pc)) { /* Connected at copper mode */
        ioerr += READ_MGBASE_AN_CTRLr(pc, &mg_an_ctrl);
        ioerr += READ_PMD_CTRLr(pc, &pmd_ctrl);
        ioerr += READ_MII_CTRLr(pc, &mii_ctrl);
        switch (speed) {
            case 5000:
                MGBASE_AN_CTRLr_MG_ENABLEf_SET(mg_an_ctrl, 1);
                MGBASE_AN_CTRLr_SPEED_5Gf_SET(mg_an_ctrl, 1);
                MGBASE_AN_CTRLr_SPEED_2P5Gf_SET(mg_an_ctrl, 0);
                break;
            case 2500:
                MGBASE_AN_CTRLr_MG_ENABLEf_SET(mg_an_ctrl, 1);
                MGBASE_AN_CTRLr_SPEED_2P5Gf_SET(mg_an_ctrl, 1);
                MGBASE_AN_CTRLr_SPEED_5Gf_SET(mg_an_ctrl, 0);
                break;
            case 10000:
                PMD_CTRLr_SPEED_SEL_10Gf_SET(pmd_ctrl, 0);
                PMD_CTRLr_SPEED_SEL_1f_SET(pmd_ctrl, 1);
                PMD_CTRLr_SPEED_SEL_0f_SET(pmd_ctrl, 1);
                MII_CTRLr_SPEED_SELECT_MSBf_SET(mii_ctrl, 1);
                MII_CTRLr_SPEED_SELECT_LSBf_SET(mii_ctrl, 1);
                break;
            case 1000:
                PMD_CTRLr_SPEED_SEL_10Gf_SET(pmd_ctrl, 0);
                PMD_CTRLr_SPEED_SEL_1f_SET(pmd_ctrl, 1);
                PMD_CTRLr_SPEED_SEL_0f_SET(pmd_ctrl, 0);
                MII_CTRLr_SPEED_SELECT_MSBf_SET(mii_ctrl, 1);
                MII_CTRLr_SPEED_SELECT_LSBf_SET(mii_ctrl, 0);
                break;
            case 100:
                PMD_CTRLr_SPEED_SEL_10Gf_SET(pmd_ctrl, 0);
                PMD_CTRLr_SPEED_SEL_1f_SET(pmd_ctrl, 0);
                PMD_CTRLr_SPEED_SEL_0f_SET(pmd_ctrl, 1);
                MII_CTRLr_SPEED_SELECT_MSBf_SET(mii_ctrl, 0);
                MII_CTRLr_SPEED_SELECT_LSBf_SET(mii_ctrl, 1);
                break;
            case 10:
                PMD_CTRLr_SPEED_SEL_10Gf_SET(pmd_ctrl, 0);
                PMD_CTRLr_SPEED_SEL_1f_SET(pmd_ctrl, 0);
                PMD_CTRLr_SPEED_SEL_0f_SET(pmd_ctrl, 0);
                MII_CTRLr_SPEED_SELECT_MSBf_SET(mii_ctrl, 0);
                MII_CTRLr_SPEED_SELECT_LSBf_SET(mii_ctrl, 0);
                break;
            default:
                return CDK_E_PARAM;
        }

        ioerr += WRITE_MGBASE_AN_CTRLr(pc, mg_an_ctrl);
        ioerr += WRITE_PMD_CTRLr(pc, pmd_ctrl);
        ioerr += WRITE_MII_CTRLr(pc, mii_ctrl);

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
    } else { /* Connected at fiber mode */
        ioerr += READ_TOP_XGPHY_STRAP1r(pc, &strap1);
        if (speed == 1000) {
            TOP_XGPHY_STRAP1r_FIBER_1Gf_SET(strap1, 1);
        }
        ioerr += WRITE_TOP_XGPHY_STRAP1r(pc, strap1);
    }

    LB_SPEED_SET(pc, speed);

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      bcm84888_phy_speed_get
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
bcm84888_phy_speed_get(phy_ctrl_t *pc, uint32_t *speed)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    TOP_LINK_STATUSr_t top_link_status;
    uint32_t speed_c45;
    PMD_CTRLr_t pmd_ctrl;
    TENG_AN_CTRLr_t teng_ctrl;
    TENG_AN_STATr_t teng_stat;
    MII_STATr_t mii_stat;
    TOP_XGPHY_STRAP1r_t strap1;
    int an;
    uint32_t an_done, data;

    PHY_CTRL_CHECK(pc);

    if (!speed) {
        return CDK_E_PARAM;
    }

    if (IS_COPPER_MODE(pc)) {
        ioerr += READ_TOP_LINK_STATUSr(pc, &top_link_status);
        speed_c45 = (TOP_LINK_STATUSr_COPPER_SPEEDf_GET(top_link_status) << 1) |
                     TOP_LINK_STATUSr_PRIOf_GET(top_link_status);

    	switch (speed_c45) {
    	case 0x2 :
            *speed = 100;
            break;
    	case 0x4 :
            *speed = 1000;
            break;
    	case 0x6 :
            *speed = 10000;
            break;
    	case 0x1 :
            *speed = 2500;
            break;
    	case 0x3 :
            *speed = 5000;
            break;
    	default:
            *speed = 0;
            break;
    	}

        if( *speed != 0) {
            return CDK_E_NONE;
        }

        rv = PHY_AUTONEG_GET(pc, &an);
        ioerr += READ_MII_STATr(pc, &mii_stat);
        an_done = MII_STATr_AUTONEG_COMPLETEf_GET(mii_stat);

        if (an) {
            if (an_done) {
                ioerr += READ_TENG_AN_CTRLr(pc, &teng_ctrl);
                ioerr += READ_TENG_AN_STATr(pc, &teng_stat);
                if (TENG_AN_CTRLr_X10GBTf_GET(teng_ctrl) &&
                    TENG_AN_STATr_LP_10GBTf_GET(teng_stat)) {
                    *speed = 10000;
                } else {
                    /* look at the CL22 regs and determine the gcd */
                    rv = _bcm84888_auto_negotiate_gcd(pc, (int *)speed, NULL);
                }
            } else {
                *speed = 0;
                return CDK_E_NONE;
            }
        } else {
            ioerr += READ_PMD_CTRLr(pc, &pmd_ctrl);
            data = (PMD_CTRLr_SPEED_SEL_1f_GET(pmd_ctrl) << 1) |
                    PMD_CTRLr_SPEED_SEL_0f_GET(pmd_ctrl);
            switch(data) {
            case 0x3:
                *speed = 10000;
                break;
            case 0x2:
                *speed = 1000;
                break;
            case 0x1:
                *speed = 100;
                break;
            default:
                *speed = 0;
                break;
            }
        }
    } else { /* Connected at fiber mode */
        ioerr += READ_TOP_XGPHY_STRAP1r(pc, &strap1);
        if (TOP_XGPHY_STRAP1r_FIBER_1Gf_GET(strap1)) {
            *speed = 1000;
        } else {
            *speed = 10000;
        }
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      bcm84888_phy_autoneg_set
 * Purpose:
 *      Enable or disable auto-negotiation on the specified port.
 * Parameters:
 *      pc - PHY control structure
 *      autoneg - non-zero enables autoneg, zero disables autoneg
 * Returns:
 *      CDK_E_xxx
 */
static int
bcm84888_phy_autoneg_set(phy_ctrl_t *pc, int autoneg)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int an;
    MII_CTRLr_t mii_ctrl;
    AN_CTRLr_t an_ctrl;

    PHY_CTRL_CHECK(pc);

    rv = _bcm84888_medium_change(pc);
    if (CDK_FAILURE(rv)) {
        return rv;
    }
    if (IS_COPPER_MODE(pc)) {
        /* Connected at copper mode */

        rv = PHY_AUTONEG_GET(pc, &an);
        if (an != 0 && autoneg == 0) {
            MII_CTRLr_SET(mii_ctrl, 0x2100);
            ioerr += WRITE_MII_CTRLr(pc, mii_ctrl);
        }

        ioerr += READ_MII_CTRLr(pc, &mii_ctrl);
        if (autoneg) {
            MII_CTRLr_AUTONEG_ENABLEf_SET(mii_ctrl, 1);
            MII_CTRLr_RESTART_AUTONEGf_SET(mii_ctrl, 1);
        } else {
            MII_CTRLr_AUTONEG_ENABLEf_SET(mii_ctrl, 0);
            MII_CTRLr_RESTART_AUTONEGf_SET(mii_ctrl, 0);
        }
        ioerr += WRITE_MII_CTRLr(pc, mii_ctrl);

        ioerr += READ_AN_CTRLr(pc, &an_ctrl);
        if (autoneg) {
            AN_CTRLr_AN_RESTARTf_SET(an_ctrl, 1);
        } else {
            AN_CTRLr_AN_RESTARTf_SET(an_ctrl, 0);
        }
        ioerr += WRITE_AN_CTRLr(pc, an_ctrl);
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      bcm84888_phy_autoneg_get
 * Purpose:
 *      Get the current auto-negotiation setting.
 * Parameters:
 *      pc - PHY control structure
 *      autoneg - (OUT) non-zero indicates autoneg enabled
 * Returns:
 *      CDK_E_xxx
 */
static int
bcm84888_phy_autoneg_get(phy_ctrl_t *pc, int *autoneg)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    MII_CTRLr_t mii_ctrl;

    PHY_CTRL_CHECK(pc);

    if (IS_COPPER_MODE(pc)) {
        /* Connected at copper mode */
        ioerr += READ_MII_CTRLr(pc, &mii_ctrl);
        if (MII_CTRLr_AUTONEG_ENABLEf_GET(mii_ctrl)) {
            *autoneg = TRUE;
        } else {
            *autoneg = FALSE;
        }
    } else {
        /* Connected at fiber mode */
        *autoneg = FALSE;
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      bcm84888_phy_loopback_set
 * Purpose:
 *      Set the internal PHY loopback mode.
 * Parameters:
 *      pc - PHY control structure
 *      enable - non-zero enables PHY loopback
 * Returns:
 *      CDK_E_xxx
 */
static int
bcm84888_phy_loopback_set(phy_ctrl_t *pc, int enable)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int lb_mode;
    uint32_t speed, cnt;
    MII_CTRLr_t mii_ctrl;
    PCS_CTRLr_t pcs_ctrl;
    TOP_LINK_STATUSr_t top_link_status;
    COMBO_IEEE0_MIICNTLr_t combo_mii_ctrl;

    PHY_CTRL_CHECK(pc);

    if (IS_COPPER_MODE(pc)) { /* Connected at copper mode */
        if (enable) {
            rv = PHY_LOOPBACK_GET(pc, &lb_mode);
            if (lb_mode) {             /* already in PHY loopback mode */
                return CDK_E_NONE;      /* do nothing               */
            }

            speed = LB_SPEED_GET(pc);
            if (!speed) {
                rv = PHY_SPEED_GET(pc, &speed);
            }

            if (CDK_SUCCESS(rv)) {
                if (speed >= 2500) {
                    ioerr += READ_MII_CTRLr(pc, &mii_ctrl);
                    MII_CTRLr_AUTONEG_ENABLEf_SET(mii_ctrl, 1);
                    MII_CTRLr_LOOPBACKf_SET(mii_ctrl, 1);
                    ioerr += WRITE_MII_CTRLr(pc, mii_ctrl);

                    ioerr += READ_PCS_CTRLr(pc, &pcs_ctrl);
                    PCS_CTRLr_LPBKf_SET(pcs_ctrl, 0);
                    ioerr += WRITE_PCS_CTRLr(pc, pcs_ctrl);

                    ioerr += READ_PCS_CTRLr(pc, &pcs_ctrl);

                    switch (speed) {
                    case  2500 :
                        PCS_CTRLr_ADV_5Gf_SET(pcs_ctrl, 1);
                        PCS_CTRLr_ADV_2P5G_EEEf_SET(pcs_ctrl, 1);
                        PCS_CTRLr_ADV_5G_EEEf_SET(pcs_ctrl, 1);
                        PCS_CTRLr_SPEED_2P5Gf_SET(pcs_ctrl, 0);
                        ioerr += WRITE_PCS_CTRLr(pc, pcs_ctrl);
                        break;
                    case  5000 :
                        PCS_CTRLr_ADV_5Gf_SET(pcs_ctrl, 0);
                        PCS_CTRLr_ADV_2P5G_EEEf_SET(pcs_ctrl, 0);
                        PCS_CTRLr_ADV_5G_EEEf_SET(pcs_ctrl, 0);
                        PCS_CTRLr_SPEED_2P5Gf_SET(pcs_ctrl, 1);
                        ioerr += WRITE_PCS_CTRLr(pc, pcs_ctrl);
                        break;
                    default:
                        PCS_CTRLr_ADV_5Gf_SET(pcs_ctrl, 0);
                        PCS_CTRLr_ADV_2P5G_EEEf_SET(pcs_ctrl, 0);
                        PCS_CTRLr_ADV_5G_EEEf_SET(pcs_ctrl, 0);
                        PCS_CTRLr_SPEED_2P5Gf_SET(pcs_ctrl, 0);
                        ioerr += WRITE_PCS_CTRLr(pc, pcs_ctrl);
                        break;
                    }

                    ioerr += READ_PCS_CTRLr(pc, &pcs_ctrl);
                    PCS_CTRLr_LPBKf_SET(pcs_ctrl, 1);
                    ioerr += WRITE_PCS_CTRLr(pc, pcs_ctrl);
                } else {
                    ioerr += READ_PCS_CTRLr(pc, &pcs_ctrl);
                    PCS_CTRLr_LPBKf_SET(pcs_ctrl, 0);
                    ioerr += WRITE_PCS_CTRLr(pc, pcs_ctrl);

                    if (speed == 100) {
                        MII_CTRLr_CLR(mii_ctrl);
                        MII_CTRLr_LOOPBACKf_SET(mii_ctrl, 1);
                        MII_CTRLr_SPEED_SELECT_LSBf_SET(mii_ctrl, 1);
                        MII_CTRLr_AUTONEG_ENABLEf_SET(mii_ctrl, 1);
                        MII_CTRLr_DUPLEX_MODEf_SET(mii_ctrl, 1);
                        ioerr += WRITE_MII_CTRLr(pc, mii_ctrl);
                    } else {
                        /* speed = 1000 */
                        MII_CTRLr_CLR(mii_ctrl);
                        MII_CTRLr_LOOPBACKf_SET(mii_ctrl, 1);
                        MII_CTRLr_AUTONEG_ENABLEf_SET(mii_ctrl, 1);
                        MII_CTRLr_DUPLEX_MODEf_SET(mii_ctrl, 1);
                        MII_CTRLr_SPEED_SELECT_MSBf_SET(mii_ctrl, 1);
                        ioerr += WRITE_MII_CTRLr(pc, mii_ctrl);
                    }
                }
            }
        } else {
            /* clear intended loopback speed */
            LB_SPEED_SET(pc, 0);

            /* disable loopback */
            ioerr += READ_PCS_CTRLr(pc, &pcs_ctrl);
            if (PCS_CTRLr_LPBKf_GET(pcs_ctrl)) {
                PCS_CTRLr_LPBKf_SET(pcs_ctrl, 0);
                ioerr += WRITE_PCS_CTRLr(pc, pcs_ctrl);
            }

            ioerr += READ_MII_CTRLr(pc, &mii_ctrl);
            if (MII_CTRLr_LOOPBACKf_GET(mii_ctrl)) {
                MII_CTRLr_LOOPBACKf_SET(mii_ctrl, 0);
                ioerr += WRITE_MII_CTRLr(pc, mii_ctrl);
            }
        }

        PHY_SYS_USLEEP(180);

        /* wait for MAC side Link UP/down state while enabling/disabling loopback */
        for (cnt = 0; cnt < PHY_LINK_POLL_MAX; cnt++) {
            ioerr += READ_TOP_LINK_STATUSr(pc, &top_link_status);
            if ((enable && TOP_LINK_STATUSr_MAC_LINKf_GET(top_link_status)) ||
               (!enable && !TOP_LINK_STATUSr_MAC_LINKf_GET(top_link_status))) {
                break;
            }
            PHY_SYS_USLEEP(3000);
        }
        if (cnt >= PHY_LINK_POLL_MAX) {
            PHY_VERB(pc, ("MAC side link is down\n"));
        } else {
            PHY_VERB(pc, ("MAC side link is up\n"));
        }

        rv = PHY_SPEED_GET(pc, &speed);
        if (PHY_CTRL_NEXT(pc)) {
            rv = PHY_SPEED_SET(PHY_CTRL_NEXT(pc), speed);
            rv = PHY_AUTONEG_SET(PHY_CTRL_NEXT(pc), 0);
        }
    } else {
        ioerr += READ_PCS_CTRLr(pc, &pcs_ctrl);
        PCS_CTRLr_LPBKf_SET(pcs_ctrl, enable);
        ioerr += WRITE_PCS_CTRLr(pc, pcs_ctrl);

        /* Configure Loopback in SerDes */
        ioerr += READ_COMBO_IEEE0_MIICNTLr(pc, &combo_mii_ctrl);
        COMBO_IEEE0_MIICNTLr_LOOPBACKf_SET(combo_mii_ctrl, enable);
        ioerr += WRITE_COMBO_IEEE0_MIICNTLr(pc, combo_mii_ctrl);
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      bcm84888_phy_loopback_get
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
bcm84888_phy_loopback_get(phy_ctrl_t *pc, int *enable)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    PCS_CTRLr_t pcs_ctrl;
    MII_CTRLr_t mii_ctrl;
    COMBO_IEEE0_MIICNTLr_t combo_mii_ctrl;

    PHY_CTRL_CHECK(pc);

    if (!enable) {
        return CDK_E_PARAM;
    }

    *enable = FALSE;
    if (IS_COPPER_MODE(pc)) { /* Connected at copper mode */
        ioerr += READ_PCS_CTRLr(pc, &pcs_ctrl);
        if ((PCS_CTRLr_GET(pcs_ctrl) != 0xffff) && PCS_CTRLr_LPBKf_GET(pcs_ctrl)) {
            *enable = TRUE;
        } else {
            *enable = FALSE;
        }

        if (!(*enable)) {
            ioerr += READ_MII_CTRLr(pc, &mii_ctrl);
            if ((MII_CTRLr_GET(mii_ctrl) != 0xffff) &&
                 MII_CTRLr_LOOPBACKf_GET(mii_ctrl)) {
                *enable = TRUE;
            } else {
                *enable = FALSE;
            }
        }
    } else { /* Connected at fiber mode */
        ioerr += READ_COMBO_IEEE0_MIICNTLr(pc, &combo_mii_ctrl);
        if ((COMBO_IEEE0_MIICNTLr_GET(combo_mii_ctrl) != 0xffff) &&
            COMBO_IEEE0_MIICNTLr_LOOPBACKf_GET(combo_mii_ctrl)) {
            *enable = TRUE;
        }
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      bcm84888_phy_ability_get
 * Purpose:
 *      Get the abilities of the PHY.
 * Parameters:
 *      pc - PHY control structure
 *      ability - (OUT) ability mask indicating supported options/speeds.
 * Returns:
 *      CDK_E_xxx
 */
static int
bcm84888_phy_ability_get(phy_ctrl_t *pc, uint32_t *ability)
{
    int rv = CDK_E_NONE;

    PHY_CTRL_CHECK(pc);

    if (IS_COPPER_MODE(pc)) { /* Connected at copper mode */
        rv = _bcm84888_copper_ability_advert_get(pc, ability);
    } else { /* Connected at fiber mode */
        rv = _bcm84888_fiber_ability_advert_get(pc, ability);
    }

    return rv;
}

/*
 * Function:
 *      bcm84888_phy_config_set
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
bcm84888_phy_config_set(phy_ctrl_t *pc, phy_config_t cfg, uint32_t val, void *cd)
{
    int rv = CDK_E_NONE;

    PHY_CTRL_CHECK(pc);

    switch (cfg) {
    case PhyConfig_MdiPairRemap:
        rv = _bcm84888_mdi_pair_remap(pc, val);
        break;
    case PhyConfig_InitStage:
        if (PHY_CTRL_FLAGS(pc) & PHY_F_STAGED_INIT) {
            rv = _bcm84888_init_stage(pc, val);
        }
        break;
    case PhyConfig_EEE:
        rv = _bcm84888_eee_mode_set(pc, val);
        break;
    case PhyConfig_Master:
        rv = _bcm84888_master_set(pc, val);
        break;
    case PhyConfig_PHY_SERDES:
        rv = _phy84888_xfi_2p5g_5g_mode_set(pc, val);
        return rv;
    case PhyConfig_AdvLocal2:
        rv = _bcm84888_ability2_advert_set(pc, val);
        return rv;
    default:
        rv = CDK_E_UNAVAIL;
        break;
    }

    return rv;
}

/*
 * Function:
 *      bcm84888_phy_config_get
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
bcm84888_phy_config_get(phy_ctrl_t *pc, phy_config_t cfg, uint32_t *val, void *cd)
{
    int rv = CDK_E_NONE;
    uint32_t abil;
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
        rv = _bcm84888_eee_mode_get(pc, val);
        return rv;
    case PhyConfig_Master:
        rv = _bcm84888_master_get(pc, val);
        return rv;
    case PhyConfig_AdvLocal:
        rv = _bcm84888_ability_local_get(pc, val);
        return rv;
    case PhyConfig_AdvRemote:
        rv = _bcm84888_ability_remote_get(pc, val);
        return rv;
    case PhyConfig_AdvEEERemote:
        rv = _bcm84888_ability_eee_remote_get(pc, val);
        return rv;
    case PhyConfig_Ability2:
        rv = PHY_ABILITY_GET(pc, &abil);
        *val = PHY_ABIL2_5000MB;
        return rv;
    case PhyConfig_AdvLocal2:
        rv = _bcm84888_ability2_local_get(pc, val);
        return rv;
    case PhyConfig_AdvRemote2:
        rv = _bcm84888_ability2_remote_get(pc, val);
        return rv;
    default:
        break;
    }

    return CDK_E_UNAVAIL;
}

/*
 * Function:
 *      bcm84888_phy_status_get
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
bcm84888_phy_status_get(phy_ctrl_t *pc, phy_status_t st, uint32_t *val)
{
    int ioerr = 0;
    PHY_ESRr_t esr;

    PHY_CTRL_CHECK(pc);

    switch (st) {
    case PhyStatus_PortMDIX:
        ioerr += READ_PHY_ESRr(pc, &esr);
        if (PHY_ESRr_GET(esr) & 0x2000) {
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
 * Variable:    bcm84888_drv
 * Purpose:     PHY Driver for BCM84888.
 */
phy_driver_t bcm84888_drv = {
    "bcm84888",
    "BCM84888 10GbE PHY Driver",
    0,
    bcm84888_phy_probe,                  /* pd_probe */
    bcm84888_phy_notify,                 /* pd_notify */
    bcm84888_phy_reset,                  /* pd_reset */
    bcm84888_phy_init,                   /* pd_init */
    bcm84888_phy_link_get,               /* pd_link_get */
    bcm84888_phy_duplex_set,             /* pd_duplex_set */
    bcm84888_phy_duplex_get,             /* pd_duplex_get */
    bcm84888_phy_speed_set,              /* pd_speed_set */
    bcm84888_phy_speed_get,              /* pd_speed_get */
    bcm84888_phy_autoneg_set,            /* pd_autoneg_set */
    bcm84888_phy_autoneg_get,            /* pd_autoneg_get */
    bcm84888_phy_loopback_set,           /* pd_loopback_set */
    bcm84888_phy_loopback_get,           /* pd_loopback_get */
    bcm84888_phy_ability_get,            /* pd_ability_get */
    bcm84888_phy_config_set,             /* pd_config_set */
    bcm84888_phy_config_get,             /* pd_config_get */
    bcm84888_phy_status_get,             /* pd_status_get */
    NULL                                 /* pd_cable_diag */
};
