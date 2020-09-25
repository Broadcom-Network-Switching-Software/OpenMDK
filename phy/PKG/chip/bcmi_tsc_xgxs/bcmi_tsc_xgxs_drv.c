/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * PHY driver for internal Tsc 40G XGXS PHY.
 */

#include <phy/phy.h>
#include <phy/phy_drvlist.h>
#include <phy/phy_brcm_serdes_id.h>
#include <phy/chip/bcmi_tsc_xgxs_defs.h>
#include <phy/chip/bcmi_tsc_xgxs_firmware_set.h>

#define BCM_SERDES_PHY_ID0              0x600d
#define BCM_SERDES_PHY_ID1              0x8770

#define PHY_ID1_REV_MASK                0x000f

#define PHY_REVID_A0                    0
#define PHY_REVID_A1                    1
#define PHY_REVID_A2                    2

#define SERDES_ID_XGXS_TSC              0x11

#define FW_CTRL_CL72_SWITCH_OVER        0x1
#define FW_CTRL_CREDIT_PROGRAM          0x2

/* Actual speeds */
#define FV_adr_10M                      0x00
#define FV_adr_100M                     0x01
#define FV_adr_1000M                    0x02
#define FV_adr_2p5G_X1                  0x03
#define FV_adr_5G_X4                    0x04
#define FV_adr_6G_X4                    0x05
#define FV_adr_10G_X4                   0x06
#define FV_adr_10G_CX4                  0x07
#define FV_adr_12G_X4                   0x08
#define FV_adr_12p5G_X4                 0x09
#define FV_adr_13G_X4                   0x0a
#define FV_adr_15G_X4                   0x0b
#define FV_adr_16G_X4                   0x0c
#define FV_adr_1G_KX1                   0x0d
#define FV_adr_10G_KX4                  0x0e
#define FV_adr_10G_KR1                  0x0f
#define FV_adr_5G_X1                    0x10
#define FV_adr_6p36G_X1                 0x11
#define FV_adr_20G_CX4                  0x12
#define FV_adr_21G_X4                   0x13
#define FV_adr_25p45G_X4                0x14
#define FV_adr_10G_X2_NOSCRAMBLE        0x15
#define FV_adr_10G_CX2_NOSCRAMBLE       0x16
#define FV_adr_10p5G_X2                 0x17
#define FV_adr_10p5G_CX2_NOSCRAMBLE     0x18
#define FV_adr_12p7G_X2                 0x19
#define FV_adr_12p7G_CX2                0x1a
#define FV_adr_10G_X1                   0x1b
#define FV_adr_40G_X4                   0x1c
#define FV_adr_20G_X2                   0x1d
#define FV_adr_20G_CX2                  0x1e
#define FV_adr_10G_SFI                  0x1f
#define FV_adr_31p5G_X4                 0x20
#define FV_adr_32p7G_X4                 0x21
#define FV_adr_20G_X4                   0x22
#define FV_adr_10G_X2                   0x23
#define FV_adr_10G_CX2                  0x24
#define FV_adr_12G_SCO_R2               0x25
#define FV_adr_10G_SCO_X2               0x26
#define FV_adr_40G_KR4                  0x27
#define FV_adr_40G_CR4                  0x28
#define FV_adr_100G_CR10                0x29
#define FV_adr_5G_X2                    0x2a
#define FV_adr_15p75G_X2                0x2c
#define FV_adr_2G_FC                    0x2e
#define FV_adr_4G_FC                    0x2f
#define FV_adr_8G_FC                    0x30
#define FV_adr_10G_CX1                  0x33
#define FV_adr_1G_CX1                   0x34
#define FV_adr_20G_KR2                  0x39
#define FV_adr_20G_CR2                  0x3a

/* PLL mode AFE */
#define FV_div32                        0x0
#define FV_div36                        0x1
#define FV_div40                        0x2
#define FV_div42                        0x3
#define FV_div48                        0x4
#define FV_div50                        0x5
#define FV_div52                        0x6
#define FV_div54                        0x7
#define FV_div60                        0x8
#define FV_div64                        0x9
#define FV_div66                        0xa
#define FV_div68                        0xb
#define FV_div70                        0xc
#define FV_div80                        0xd
#define FV_div92                        0xe
#define FV_div100                       0xf

/* Port modes */
#define FV_pm_4port_3_2_1_0             0x0
#define FV_pm_3port_32_1_0              0x1
#define FV_pm_3port_3_2_10              0x2
#define FV_pm_2port_32_10               0x3
#define FV_pm_1port_3210                0x4

/* TxDrv modes */
#define FV_txdrv_6250                   0x0
#define FV_txdrv_103125                 0x1
#define FV_txdrv_109375                 0x2
#define FV_txdrv_12500                  0x3
#define FV_txdrv_11500                  0x4
#define FV_txdrv_XFI                    0x5
#define FV_txdrv_XLAUI                  0x6
#define FV_txdrv_SFI                    0x7
#define FV_txdrv_SFIDAC                 0x8
#define FV_txdrv_SR4                    0x9
#define FV_txdrv_6GOS1                  0xa
#define FV_txdrv_6GOS2                  0xb
#define FV_txdrv_6GOS2_CX4              0xc
#define FV_txdrv_AN                     0xd
#define FV_txdrv_DFT                    0xe
#define FV_txdrv_ILKN                   0xf

/* Auto-Negotiation type */
#define FV_an_NONE                      0x0
#define FV_an_CL73                      0x1
#define FV_an_CL37                      0x2
#define FV_an_CL37_10G                  0x3
#define FV_an_CL73_BAM                  0x4
#define FV_an_CL37_BAM                  0x5
#define FV_an_CL37_SGMII                0x6
#define FV_an_HPAM                      0x7

/* Refclk Sel */
#define FV_refclk_25MHz                 0x0
#define FV_refclk_100MHz                0x1
#define FV_refclk_125MHz                0x2
#define FV_refclk_156p25MHz             0x3
#define FV_refclk_187p5MHz              0x4
#define FV_refclk_161p25Mhz             0x5
#define FV_refclk_50Mhz                 0x6
#define FV_refclk_106p25Mhz             0x7

/* Lane from PHY control instance */
#define LANE_NUM_MASK                   0x3

#define PLL_LOCK_MSEC                   200

#define IS_1LANE_PORT(_pc) \
    ((PHY_CTRL_FLAGS(_pc) & (PHY_F_SERDES_MODE | PHY_F_2LANE_MODE)) == PHY_F_SERDES_MODE)
#define IS_2LANE_PORT(_pc) \
    (PHY_CTRL_FLAGS(_pc) & PHY_F_2LANE_MODE)
#define IS_4LANE_PORT(_pc) \
    ((PHY_CTRL_FLAGS(_pc) & PHY_F_SERDES_MODE) == 0)

/*
 * Private driver data
 *
 * We use a single 32-bit word which is used like this:
 *
 * 31               16 15               8 7             0
 * +------------------+------------------+---------------+
 * |     Reserved     | Active interface | Lane polarity |
 * +------------------+------------------+---------------+
 */
#if PHY_CONFIG_PRIVATE_DATA_WORDS > 0

#define PRIV_DATA(_pc) ((_pc)->priv[0])

#define LANE_POLARITY_GET(_pc) (PRIV_DATA(_pc) & 0xff)
#define LANE_POLARITY_SET(_pc,_val) \
do { \
    PRIV_DATA(_pc) &= ~0xff; \
    PRIV_DATA(_pc) |= (_val) & 0xff; \
} while (0)

#define ACTIVE_INTERFACE_GET(_pc) ((PRIV_DATA(_pc) >> 8) & 0xff)
#define ACTIVE_INTERFACE_SET(_pc,_val) \
do { \
    PRIV_DATA(_pc) &= ~0xff00; \
    PRIV_DATA(_pc) |= LSHIFT32(_val, 8) & 0xff00; \
} while (0)

#else

#define LANE_POLARITY_GET(_pc) (0)
#define LANE_POLARITY_SET(_pc,_val)
#define ACTIVE_INTERFACE_GET(_pc) (0)
#define ACTIVE_INTERFACE_SET(_pc,_val)

#endif /* PHY_CONFIG_PRIVATE_DATA_WORDS */

/* Private PHY flag is used to indicate that firmware is running */
#define PHY_F_FW_RUNNING                PHY_F_PRIVATE

/* Low level debugging (off by default) */
#ifdef PHY_DEBUG_ENABLE
#define _PHY_DBG(_pc, _stuff) \
    PHY_VERB(_pc, _stuff)
#else
#define _PHY_DBG(_pc, _stuff)
#endif

/*
 * Function:
 *      _tsc_revid_get(phy_ctrl_t* pc)
 * Purpose:
 *      Get the serdes reversion ID.
 * Parameters:
 *      pc - PHY control structure
  * Returns:
 *      reversion ID
 */
static int
_tsc_revid_get(phy_ctrl_t* pc)
{
    int ioerr = 0;
    SERDESIDr_t serdesid;
    int version = 0;

    ioerr += READ_SERDESIDr(pc, &serdesid);
    if (ioerr == 0) {
        version = SERDESIDr_REV_NUMBERf_GET(serdesid);
    }

    return version;
}

/*
 * Function:
 *      _tsc_addr_type_partial_proxy
 * Purpose:
 *      Get the address access type in partial proxy mode.
 * Parameters:
 *      pc - PHY control structure
 *      addr - PHY register address
 * Returns:
 *      Zero/non-Zero value
 */
static int 
_tsc_addr_type_partial_proxy(phy_ctrl_t* pc, uint32_t addr)
{
    int viol = 0;

    if ((PHY_CTRL_FLAGS(pc) & PHY_F_FW_RUNNING) == 0) {
        return 0;
    }

    addr &= 0xffff;
    if ((addr >= 0xc200) && (addr < 0xc250)) {
        viol = 1;
    } else if ((addr >= 0xc010) && (addr < 0xc030)) {
        viol = 0;
    } else if ((addr >= 0x9010) && (addr < 0x9020)) {
        viol = 3;
    }
    
    return viol;
}

/*
 * Function:
 *      _tsc_serdes_lane
 * Purpose:
 *      Retrieve XGXS lane number for this PHY instance.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      Lane number or -1 if lane is unknown
 */
static int
_tsc_serdes_lane(phy_ctrl_t *pc)
{
    uint32_t inst = PHY_CTRL_INST(pc);

    if (inst & PHY_INST_VALID) {
        return inst & LANE_NUM_MASK;
    }
    return -1;
}

/*
 * Function:
 *      _tsc_serdes_lane_mask
 * Purpose:
 *      Retrieve XGXS lane mask for this PHY instance.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      TRUE/FALSE
 */
static int
_tsc_serdes_lane_mask(phy_ctrl_t *pc)
{
    int idx;
    int lane_num, lane_end, lane_mask;

    lane_num = _tsc_serdes_lane(pc);
    if (lane_num < 0) {
        return -1;
    }

    lane_end = lane_num;
    if (IS_2LANE_PORT(pc)) {
        lane_end += 1;
    } else if (IS_4LANE_PORT(pc)) {
        lane_end += 3;
    }

    lane_mask = 0;
    for (idx = lane_num; idx <= lane_end; idx++) {
        lane_mask |= (0x1 << idx);
    }

    return lane_mask;
}

/*
 * Function:
 *      _tsc_primary_lane
 * Purpose:
 *      Ensure that each tsc is initialized only once.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      TRUE/FALSE
 */
static int
_tsc_primary_lane(phy_ctrl_t *pc)
{
    return ((PHY_CTRL_INST(pc) & LANE_NUM_MASK) == 0) ? TRUE : FALSE;
}

/*
 * Function:
 *      _tsc_aspeed_get
 * Purpose:
 *      Get the actual speed from register.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int 
_tsc_aspeed_get(phy_ctrl_t* pc, int *aspeed)
{
    int ioerr = 0;
    CL72_MISC2_CONTROLr_t cl72_misc2_ctrl;

    ioerr += READ_CL72_MISC2_CONTROLr(pc, &cl72_misc2_ctrl);
    *aspeed = CL72_MISC2_CONTROLr_SW_ACTUAL_SPEEDf_GET(cl72_misc2_ctrl);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

/*
 * Function:
 *      _tsc_plldiv_get
 * Purpose:
 *      Get the PLL div from register.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int 
_tsc_plldiv_get(phy_ctrl_t *pc, uint32_t *plldiv)
{
    int ioerr = 0;
    SETUPr_t setup;

    /* read back plldiv */
    ioerr += READ_SETUPr(pc, &setup);
    *plldiv = SETUPr_DEFAULT_PLL_MODE_AFEf_GET(setup);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

/*
 * Function:
 *      _tsc_credit_set
 * Purpose:
 *      configure credit.
 * Parameters:
 *      pc - PHY control structure
 *      cntl - configure/clear
 * Returns:
 *      CDK_E_xxx
 */
static int 
_tsc_credit_set(phy_ctrl_t* pc, int cntl)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int actual_speed;
    uint32_t plldiv, replication_cnt, pcs_clkcnt0, pcs_creditgencnt;
    uint32_t clockcnt0, clockcnt1, clockcnt1_rsv;
    uint32_t loopcnt0, loopcnt1, mac_credit;
    uint32_t sgmii_spd_switch;
    TX_X4_CREDIT0r_t tx_x4_creadit0;
    TX_X4_CREDIT1r_t tx_x4_creadit1;
    TX_X4_LOOPCNTr_t tx_x4_loop_cnt;
    TX_X4_MAC_CREDITGENCNTr_t tx_x4_mac_credit_gen_cnt;
    TX_X4_PCS_CLOCKCNT0r_t tx_x4_pcs_clk_cnt0;
    TX_X4_PCS_CREDITGENCNTr_t tx_x4_pcs_credit_gen_cnt;

    sgmii_spd_switch = 0;
    clockcnt0        = 0;
    clockcnt1        = 0;
    clockcnt1_rsv    = 0;
    loopcnt0         = 1;
    loopcnt1         = 0;
    mac_credit       = 0;
    replication_cnt  = 0;
    pcs_clkcnt0      = 0x0;
    pcs_creditgencnt = 0x0;

    rv += _tsc_plldiv_get(pc, &plldiv);
    rv += _tsc_aspeed_get(pc, &actual_speed);
    if (CDK_FAILURE(rv)) {
        return rv;
    }

    if (cntl) {
        switch(actual_speed) {
        case FV_adr_10M:
            if (plldiv == FV_div40) {
            sgmii_spd_switch    = 0x1;
            clockcnt0           = 0x9c4;
            clockcnt1_rsv       = 0x80;
            loopcnt0            = 0x1;
            mac_credit          = 0x9c3;
            replication_cnt     = 0x1;
            pcs_clkcnt0         = 0x19;
            pcs_creditgencnt    = 0x18;
            } else if (plldiv == FV_div66){
            sgmii_spd_switch    = 0x1;
            clockcnt0           = 0x101d;
            clockcnt1_rsv       = 0x80;
            loopcnt0            = 0x1;
            mac_credit          = 0x101c;
            replication_cnt     = 0x1;
            pcs_clkcnt0         = 0xa5;
            pcs_creditgencnt    = 0x29;
            }
            break;
        case FV_adr_100M:
            if (plldiv == FV_div40) {
            sgmii_spd_switch    = 0x1;
            clockcnt0           = 0xfa;
            clockcnt1_rsv       = 0x81;
            loopcnt0            = 0x1;
            mac_credit          = 0xf9;
            pcs_clkcnt0         = 0x19;
            pcs_creditgencnt    = 0x18;
            } else if (plldiv == FV_div66){
            sgmii_spd_switch    = 0x1;
            clockcnt0           = 0x339;
            clockcnt1_rsv       = 0x81;
            loopcnt0            = 0x1;
            mac_credit          = 0x19c;
            pcs_clkcnt0         = 0xa5;
            pcs_creditgencnt    = 0x29;
            }
            break;
        case FV_adr_1000M:
            if (plldiv == FV_div40) {
            clockcnt0           = 0x19;
            clockcnt1_rsv       = 0x82;
            loopcnt0            = 0x1;
            mac_credit          = 0x18;
            } else if (plldiv == FV_div66){
            clockcnt0           = 0xa5;
            clockcnt1_rsv       = 0x82;
            loopcnt0            = 0x1;
            mac_credit          = 0x29;
            }
            break;
        case FV_adr_2p5G_X1:
            if (plldiv == FV_div40) {
            clockcnt0           = 0xa;
            clockcnt1_rsv       = 0x83;
            loopcnt0            = 0x1;
            mac_credit          = 0x6;
            } else if (plldiv == FV_div66) {
            clockcnt0           = 0x21;
            clockcnt1_rsv       = 0x83;
            loopcnt0            = 0x1;
            mac_credit          = 0x10;
            }
            break;
        case FV_adr_10G_KR1:
        case FV_adr_10G_SFI:
        case FV_adr_10G_X1:
        case FV_adr_5G_X1:
            sgmii_spd_switch    = 0x0;
            clockcnt0           = 0x21;
            clockcnt1_rsv       = 0x84;
            loopcnt0            = 0x1;
            mac_credit          = 0x4;
            pcs_clkcnt0         = 0x0;
            pcs_creditgencnt    = 0x0; 
            break;
        case FV_adr_10G_X4:
        case FV_adr_10G_CX4:
            sgmii_spd_switch    = 0x0;
            clockcnt0           = 0x5;
            clockcnt1_rsv       = 0x8d;
            loopcnt1            = 0x0;
            mac_credit          = 0x2;
            break;
        case FV_adr_10G_CX2_NOSCRAMBLE:
        case FV_adr_13G_X4:
        case FV_adr_15G_X4:
        case FV_adr_16G_X4:
            sgmii_spd_switch    = 0x0;
            clockcnt0           = 0x5;
            clockcnt1_rsv       = 0x0;
            loopcnt1            = 0x0;
            mac_credit          = 0x2;
            break;
        case FV_adr_20G_X4:
        case FV_adr_21G_X4:
            sgmii_spd_switch    = 0x0;
            clockcnt0           = 0x5;
            clockcnt1_rsv       = 0x0;
            loopcnt1            = 0x0;
            mac_credit          = 0x1;
            break;
        case FV_adr_20G_CR2:
        case FV_adr_20G_KR2:
            sgmii_spd_switch    = 0x0;
            clockcnt0           = 0x21;
            clockcnt1_rsv       = 0x8b;
            loopcnt1            = 0x0;
            mac_credit          = 0x2;
            pcs_clkcnt0         = 0x0;
            pcs_creditgencnt    = 0x0; 
            break;
        case FV_adr_12p7G_X2:
            sgmii_spd_switch    = 0x0;
            clockcnt0           = 0x21;
            clockcnt1_rsv       = 0x8c;
            loopcnt1            = 0x0;
            mac_credit          = 0x2;
            pcs_clkcnt0         = 0x0;
            pcs_creditgencnt    = 0x0; 
            break;
        case FV_adr_20G_CX2:
            sgmii_spd_switch    = 0x0;
            clockcnt0           = 0x21;
            clockcnt1_rsv       = 0x00;
            loopcnt1            = 0x0;
            mac_credit          = 0x2;
            break;
        case FV_adr_25p45G_X4:
            sgmii_spd_switch    = 0x0;
            clockcnt0           = 0x21;
            clockcnt1_rsv       = 0x00;
            loopcnt1            = 0x0;
            mac_credit          = 0x1;
            break;
        case FV_adr_40G_X4:
            sgmii_spd_switch    = 0x0;
            clockcnt0           = 0x21;
            clockcnt1_rsv       = 0x98;
            loopcnt1            = 0x0;
            mac_credit          = 0x1;
            break;
        case FV_adr_40G_KR4:
        case FV_adr_40G_CR4:
            if (plldiv == FV_div70) {
            sgmii_spd_switch    = 0x0;
            clockcnt0           = 0x21;
            clockcnt1_rsv       = 0x96;
            loopcnt1            = 0x0;
            mac_credit          = 0x1;
            } else if (plldiv == FV_div66){
            sgmii_spd_switch    = 0x0;
            clockcnt0           = 0x21;
            clockcnt1_rsv       = 0x98;
            loopcnt1            = 0x0;
            mac_credit          = 0x1;
            }
            break;
        default:
            return CDK_E_IO;
        }

        ioerr += READ_TX_X4_CREDIT0r(pc, &tx_x4_creadit0);
        TX_X4_CREDIT0r_SGMII_SPD_SWITCHf_SET(tx_x4_creadit0, sgmii_spd_switch);
        TX_X4_CREDIT0r_CLOCKCNT0f_SET(tx_x4_creadit0, clockcnt0);
        ioerr += WRITE_TX_X4_CREDIT0r(pc, tx_x4_creadit0);

        ioerr += READ_TX_X4_CREDIT1r(pc, &tx_x4_creadit1);
        TX_X4_CREDIT1r_CLOCKCNT1f_SET(tx_x4_creadit1, clockcnt1);
        TX_X4_CREDIT1r_RESERVED0f_SET(tx_x4_creadit1, clockcnt1_rsv);
        ioerr += WRITE_TX_X4_CREDIT1r(pc, tx_x4_creadit1);

        ioerr += READ_TX_X4_LOOPCNTr(pc, &tx_x4_loop_cnt);
        TX_X4_LOOPCNTr_LOOPCNT1f_SET(tx_x4_loop_cnt, loopcnt1);
        TX_X4_LOOPCNTr_LOOPCNT0f_SET(tx_x4_loop_cnt, loopcnt0);
        ioerr += WRITE_TX_X4_LOOPCNTr(pc, tx_x4_loop_cnt);

        ioerr += READ_TX_X4_MAC_CREDITGENCNTr(pc, &tx_x4_mac_credit_gen_cnt);
        TX_X4_MAC_CREDITGENCNTr_MAC_CREDITGENCNTf_SET(tx_x4_mac_credit_gen_cnt, mac_credit);
        ioerr += WRITE_TX_X4_MAC_CREDITGENCNTr(pc, tx_x4_mac_credit_gen_cnt);

        ioerr += READ_TX_X4_PCS_CLOCKCNT0r(pc, &tx_x4_pcs_clk_cnt0);
        TX_X4_PCS_CLOCKCNT0r_PCS_CLOCKCNT0f_SET(tx_x4_pcs_clk_cnt0, pcs_clkcnt0);
        TX_X4_PCS_CLOCKCNT0r_REPLICATION_CNTf_SET(tx_x4_pcs_clk_cnt0, replication_cnt);
        ioerr += WRITE_TX_X4_PCS_CLOCKCNT0r(pc, tx_x4_pcs_clk_cnt0);

        ioerr += READ_TX_X4_PCS_CREDITGENCNTr(pc, &tx_x4_pcs_credit_gen_cnt);
        TX_X4_PCS_CREDITGENCNTr_PCS_CREDITGENCNTf_SET(tx_x4_pcs_credit_gen_cnt, pcs_creditgencnt);
        ioerr += WRITE_TX_X4_PCS_CREDITGENCNTr(pc, tx_x4_pcs_credit_gen_cnt);
    } else {
        TX_X4_CREDIT0r_CLR(tx_x4_creadit0);
        ioerr += WRITE_TX_X4_CREDIT0r(pc, tx_x4_creadit0);

        TX_X4_PCS_CLOCKCNT0r_CLR(tx_x4_pcs_clk_cnt0);
        ioerr += WRITE_TX_X4_PCS_CLOCKCNT0r(pc, tx_x4_pcs_clk_cnt0);
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

/*
 * Function:
 *      _tsc_encode_set
 * Purpose:
 *      configure encode.
 * Parameters:
 *      pc - PHY control structure
 *      cntl - configure/clear
 * Returns:
 *      CDK_E_xxx
 */
static int 
_tsc_encode_set(phy_ctrl_t* pc, int cntl)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int actual_speed;
    uint32_t plldiv, encode_mode, cl49_bypass_txsm;
    uint32_t hg2_message_invalid_code_enable;
    uint32_t hg2_codec, hg2_en;
    uint32_t scr_mode, ram_base;
    uint32_t mld_swap_count, mld_swap_en;
    uint32_t disable_packet_misalign;
    uint32_t data;
    TX_X4_ENCODE_0r_t tx_x4_encode_0;
    TX_X4_MISCr_t tx_x4_misc;
    TX_X4_CL36_TX_0r_t tx_x4_cl36_tx_0;
    TX_X2_MLD_SWAP_COUNTr_t tx_x2_mld_swap_count;
    TX_X2_BRCM_MODEr_t tx_x2_brcm_mode;

    data = 0;
    encode_mode       = 0;
    cl49_bypass_txsm  = 0;
    hg2_message_invalid_code_enable = 0;
    hg2_en            = 0;
    hg2_codec         = 0;
    scr_mode          = 0;
    mld_swap_count    = 0;
    mld_swap_en       = 0;
    disable_packet_misalign = 0x0;

    if (!cntl) {
        return CDK_E_NONE;
    }

    rv += _tsc_plldiv_get(pc, &plldiv);
    rv += _tsc_aspeed_get(pc, &actual_speed);
    rv += PHY_CONFIG_GET(pc, PhyConfig_RamBase, &ram_base, NULL);
    if (CDK_FAILURE(rv)) {
        return rv;
    }

    switch(actual_speed) {
    case FV_adr_10M:
    case FV_adr_100M:
    case FV_adr_1000M:
    case FV_adr_2p5G_X1:
        encode_mode       = 0x3;
        hg2_message_invalid_code_enable = 0x0;
        disable_packet_misalign = 0x1;
        break;
    case FV_adr_10G_KR1:
    case FV_adr_10G_SFI:
    case FV_adr_10G_X1:
    case FV_adr_5G_X1:
        encode_mode       = 0x5;
        cl49_bypass_txsm  = 0;
        hg2_message_invalid_code_enable = 0;
        scr_mode          = 0x3;
        mld_swap_en       = 0;
        break;
    case FV_adr_10G_X4:
    case FV_adr_10G_CX4:
    case FV_adr_13G_X4:
    case FV_adr_15G_X4:
    case FV_adr_16G_X4:
    case FV_adr_20G_X4:
        encode_mode       = 0x1;
        cl49_bypass_txsm  = 1;   
        hg2_message_invalid_code_enable = 0;
        scr_mode          = 0x0;
        mld_swap_en       = 0;
        break;
    case FV_adr_10G_CX2_NOSCRAMBLE:
        encode_mode       = 0x2;
        cl49_bypass_txsm  = 1;   
        hg2_message_invalid_code_enable = 0;
        scr_mode          = 0x0;
        mld_swap_en       = 0; 
        break;
    case FV_adr_21G_X4:
        encode_mode       = 0x01;         
        cl49_bypass_txsm  = 1;   
        hg2_message_invalid_code_enable = 0;
        scr_mode          = 0x2;                 
        mld_swap_en       = 0; 
        break;
    case FV_adr_20G_CR2:
    case FV_adr_20G_KR2:
        encode_mode       = 0x4;
        cl49_bypass_txsm  = 0;
        hg2_message_invalid_code_enable = 0;
        scr_mode          = 0x3;
        mld_swap_count    = 0xfffc; 
        mld_swap_en       = 1;
        break;
    case FV_adr_20G_CX2:
        encode_mode       = 0x6;
        cl49_bypass_txsm  = 1;
        hg2_message_invalid_code_enable = 0;
        scr_mode          = 0x1;
        mld_swap_en       = 0;
        break;
    case FV_adr_40G_X4:
    case FV_adr_25p45G_X4:
    case FV_adr_12p7G_X2:
        encode_mode       = 0x6;
        cl49_bypass_txsm  = 1;
        hg2_message_invalid_code_enable = 0;
        scr_mode          = 0x1;
        mld_swap_en       = 0;
        break;
    case FV_adr_40G_KR4:
    case FV_adr_40G_CR4:
        encode_mode       = 0x4;
        cl49_bypass_txsm  = 0;
        hg2_message_invalid_code_enable = 0;
        if (plldiv == FV_div70) {
            hg2_en            = 1;
            hg2_codec         = 1;
        }
        scr_mode          = 0x3;
        mld_swap_count    = 0xfffc; 
        mld_swap_en       = 1;
        break;
    default:
        return CDK_E_IO;
    }

    if((PHY_CTRL_LINE_INTF(pc) & PHY_IF_HIGIG) && (encode_mode >= 3)) {
        /* encode mode is not CL48 */
        hg2_en            = 1;
        hg2_codec         = 1;
        hg2_message_invalid_code_enable = 1; 
    }

    ioerr += READ_TX_X4_ENCODE_0r(pc, &tx_x4_encode_0);
    TX_X4_ENCODE_0r_ENCODEMODEf_SET(tx_x4_encode_0, encode_mode);
    TX_X4_ENCODE_0r_CL49_BYPASS_TXSMf_SET(tx_x4_encode_0, cl49_bypass_txsm);
    TX_X4_ENCODE_0r_HG2_ENABLEf_SET(tx_x4_encode_0, hg2_en);
    TX_X4_ENCODE_0r_HG2_MESSAGE_INVALID_CODE_ENABLEf_SET(tx_x4_encode_0, hg2_message_invalid_code_enable);
    TX_X4_ENCODE_0r_HG2_CODECf_SET(tx_x4_encode_0, hg2_codec);
    ioerr += WRITE_TX_X4_ENCODE_0r(pc, tx_x4_encode_0);

    ioerr += READ_TX_X4_MISCr(pc, &tx_x4_misc);
    TX_X4_MISCr_SCR_MODEf_SET(tx_x4_misc, scr_mode);
    ioerr += WRITE_TX_X4_MISCr(pc, tx_x4_misc);

    ioerr += READ_TX_X4_CL36_TX_0r(pc, &tx_x4_cl36_tx_0);
    TX_X4_CL36_TX_0r_DISABLE_PACKET_MISALIGNf_SET(tx_x4_cl36_tx_0, disable_packet_misalign);
    ioerr += WRITE_TX_X4_CL36_TX_0r(pc, tx_x4_cl36_tx_0);

    if (mld_swap_en) {
        ioerr += READ_TX_X2_MLD_SWAP_COUNTr(pc, &tx_x2_mld_swap_count);
        TX_X2_MLD_SWAP_COUNTr_MLD_SWAP_COUNTf_SET(tx_x2_mld_swap_count, mld_swap_count);
        ioerr += WRITE_TX_X2_MLD_SWAP_COUNTr(pc, tx_x2_mld_swap_count);
    }

    if(encode_mode == 0x6) { /* BRCM mode */
        data = 0x66;
    } else {
        data = 0x2;
    }

    ioerr += READ_TX_X2_BRCM_MODEr(pc, &tx_x2_brcm_mode);
    TX_X2_BRCM_MODEr_ACOL_SWAP_COUNT64B66Bf_SET(tx_x2_brcm_mode, data);
    ioerr += WRITE_TX_X2_BRCM_MODEr(pc, tx_x2_brcm_mode);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

/*
 * Function:
 *      _tsc_decode_set
 * Purpose:
 *      configure decode.
 * Parameters:
 *      pc - PHY control structure
 *      cntl - configure/clear
 * Returns:
 *      CDK_E_xxx
 */
static int
_tsc_decode_set(phy_ctrl_t* pc, int cntl)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int actual_speed;
    uint32_t plldiv, descramblemode, decodemode, deskewmode, desc2_mode;
    uint32_t cl36_en, disable_carrier_extend, reorder_en;  
    uint32_t cl48_syncacq_en, block_sync_mode, cl48_cgbad_en;
    uint32_t bypass_cl49rxsm, hg2_message_ivalid_code_enable;
    uint32_t hg2_en, hg2_codec;
    uint32_t cl36byte_delete_mode, os_mode, rx_gbox_afrst_en;
    uint32_t cl82_dswin, cl48_dswin64b66b;
    uint32_t cl48_dswin8b10b, rx_x1_cntl0_deskw_en;
    uint32_t sync_code_sm_en;
    uint32_t rx_control0_misc0_en ;
    uint32_t data, ram_base;
    RX_X4_PCS_CONTROL_0r_t rx_x4_pcs_control_0;
    RX_X4_CL36_RX_0r_t rx_x4_cl36_rx_0;
    RX_X4_DECODE_CONTROL_0r_t rx_x4_dec_ctrl_0;
    RX_X4_DECODE_CONTROL_1r_t rx_x4_decode_control_1;
    RX_X4_PMA_CONTROL_0r_t rx_x4_pma_ctrl_0;
    RX_X1_DECODE_CONTROL_0r_t rx_x1_decode_control_0;
    RX_X1_DESKEW_WINDOWSr_t rx_x1_deskew_windows;
    RX_X1_SCW0r_t rx_x1_scw0;
    RX_X1_SCW1r_t rx_x1_scw1;
    RX_X1_SCW2r_t rx_x1_scw2;
    RX_X1_SCW3r_t rx_x1_scw3;
    RX_X1_SCW4r_t rx_x1_scw4;
    RX_X1_SCW0_MASKr_t rx_x1_scw0_mask;
    RX_X1_SCW1_MASKr_t rx_x1_scw1_mask;
    RX_X1_SCW2_MASKr_t rx_x1_scw2_mask;
    RX_X1_SCW3_MASKr_t rx_x1_scw3_mask;
    RX_X1_SCW4_MASKr_t rx_x1_scw4_mask;
    RX_X2_MISC_0r_t rx_x2_misc_0;
    CL72_MISC1_CONTROLr_t cl72_misc1_ctrl;
    RX_X4_FEC_0r_t rx_x4_fec_0;
    CL82_LANE_0_AM_BYTE10r_t cl82_lane_0_am_byte10;
    CL82_LANE_1_AM_BYTE10r_t cl82_lane_1_am_byte10;
    CL82_LANES_1_0_AM_BYTE2r_t cl82_lanes_0_1_am_byte2;
    TX_X4_MISCr_t tx_x4_misc;
    TX_X2_CL48_0r_t tx_x2_cl48_0;
    RX_X2_MISC_1r_t rx_x2_misc_1;
    TX_X2_CL82_0r_t tx_x2_cl82_0;

    descramblemode       = 0x0;
    decodemode           = 0x0;
    deskewmode           = 0x0; 
    desc2_mode           = 0x0;
    cl36_en              = 0x0;
    cl36byte_delete_mode = 0x0;
    disable_carrier_extend         = 0x0;
    reorder_en           = 0x0;  
    cl48_syncacq_en      = 0x1;     
    block_sync_mode      = 0x0;
    cl48_cgbad_en        = 0x0;
    bypass_cl49rxsm      = 0x0;
    hg2_message_ivalid_code_enable = 0x1 ;
    hg2_en               = 0;
    hg2_codec            = 0;
    os_mode              = 0x0;
    rx_gbox_afrst_en     = 0x0;
    rx_x1_cntl0_deskw_en = 0;
    cl82_dswin           = 0;
    cl48_dswin64b66b     = 0;
    cl48_dswin8b10b      = 0;
    sync_code_sm_en      = 0; 
    rx_control0_misc0_en = 0;

    if (!cntl) {
        return CDK_E_NONE;
    }

    rv += _tsc_plldiv_get(pc, &plldiv);
    rv += _tsc_aspeed_get(pc, &actual_speed);
    rv += PHY_CONFIG_GET(pc, PhyConfig_RamBase, &ram_base, NULL);
    if (CDK_FAILURE(rv)) {
        return rv;
    }

    switch(actual_speed) {
    case FV_adr_10M:
        decodemode      = 0x5;
        deskewmode      = 0x4;
        desc2_mode      = 0x5;
        cl36byte_delete_mode = 0x1;
        hg2_message_ivalid_code_enable = 0x0;
        cl48_syncacq_en = 0x0;     
        block_sync_mode = 0x3;
        cl36_en         = 0x1;
        rx_gbox_afrst_en = 0x1;
        if (plldiv == FV_div40) {
        os_mode         = 0x5;
        } else if (plldiv == FV_div66){
        os_mode         = 0x7;
        }
        break;
    case FV_adr_100M:
        decodemode      = 0x5;
        deskewmode      = 0x4;
        desc2_mode      = 0x5;
        cl36byte_delete_mode = 0x0;
        hg2_message_ivalid_code_enable = 0x0;
        cl48_syncacq_en = 0x0;     
        block_sync_mode = 0x3;
        cl36_en         = 0x1;
        rx_gbox_afrst_en = 0x1;
        if (plldiv == FV_div40) {
        os_mode         = 0x5;
        } else if (plldiv == FV_div66){
        os_mode         = 0x7;
        }
        break;
    case FV_adr_1000M:
        decodemode      = 0x5;
        deskewmode      = 0x4;
        desc2_mode      = 0x5;
        cl36byte_delete_mode = 0x2;
        hg2_message_ivalid_code_enable = 0x0;
        cl48_syncacq_en = 0x0;     
        block_sync_mode = 0x3;
        cl36_en         = 0x1;
        rx_gbox_afrst_en = 0x1;
        if (plldiv == FV_div40) {
        os_mode         = 0x5;
        } else if (plldiv == FV_div66){
        os_mode         = 0x7;
        }
        break;
    case FV_adr_2p5G_X1:
        decodemode      = 0x5;
        deskewmode      = 0x4;
        desc2_mode      = 0x5;
        cl36byte_delete_mode = 0x2;
        hg2_message_ivalid_code_enable = 0x0;
        cl48_syncacq_en = 0x0;     
        block_sync_mode = 0x3;
        cl36_en         = 0x1;
        rx_gbox_afrst_en = 0x1;
        if (plldiv == FV_div40) {
        os_mode         = 0x1;
        } else if (plldiv == FV_div66){
        os_mode         = 0x3;
        }
        break;
    case FV_adr_10G_KR1:
    case FV_adr_10G_SFI:
    case FV_adr_10G_X1:
        descramblemode  = 0x0;
        decodemode      = 0x1;
        deskewmode      = 0x0;
        desc2_mode      = 0x1;
        cl36_en         = 0x0;
        disable_carrier_extend = 0x0;
        reorder_en      = 0x0;  
        cl48_syncacq_en = 0x0;
        block_sync_mode = 0x1;
        cl48_cgbad_en   = 0x0;
        bypass_cl49rxsm = 0x0;
        hg2_message_ivalid_code_enable = 0x1;
        os_mode = 0;
        break;
    case FV_adr_5G_X1:
        descramblemode  = 0x0;
        decodemode      = 0x1;
        deskewmode      = 0x0;
        desc2_mode      = 0x1;
        cl36_en         = 0x0;
        disable_carrier_extend = 0x0;
        reorder_en      = 0x0;  
        cl48_syncacq_en = 0x0;
        block_sync_mode = 0x1;
        cl48_cgbad_en   = 0x0;
        bypass_cl49rxsm = 0x0;
        hg2_message_ivalid_code_enable = 0x1;
        os_mode = 1;
        break;
    case FV_adr_10G_X4:
    case FV_adr_10G_CX4:
    case FV_adr_13G_X4:
    case FV_adr_15G_X4:
    case FV_adr_16G_X4:
        descramblemode  = 0x0;
        decodemode      = 0x4;
        deskewmode      = 0x1;
        desc2_mode      = 0x4;
        cl36_en         = 0x0;
        disable_carrier_extend = 0x1;
        reorder_en      = 0x1;
        cl48_syncacq_en = 0x1;  
        block_sync_mode = 0x3;
        cl48_cgbad_en   = 0x1;
        bypass_cl49rxsm = 0x1;
        hg2_message_ivalid_code_enable = 0x0;
        os_mode         = 0x1;
        rx_x1_cntl0_deskw_en = 1;
        cl82_dswin           = 0;
        cl48_dswin64b66b     = 1;
        cl48_dswin8b10b      = 7;
        break;
    case FV_adr_10G_CX2_NOSCRAMBLE:
        descramblemode  = 0x0;
        decodemode      = 0x4;
        deskewmode      = 0x1;
        desc2_mode      = 0x4;
        cl36_en         = 0x0;
        disable_carrier_extend = 0x1;
        reorder_en      = 0x1;
        cl48_syncacq_en = 0x1;  
        block_sync_mode = 0x3;
        cl48_cgbad_en   = 0x1;
        bypass_cl49rxsm = 0x1;
        hg2_message_ivalid_code_enable = 0x0;
        os_mode         = 0x0;
        rx_x1_cntl0_deskw_en = 1;
        cl82_dswin           = 0;
        cl48_dswin64b66b     = 1;
        cl48_dswin8b10b      = 7;                                                     
        break;
    case FV_adr_20G_X4:
        descramblemode  = 0x0;
        decodemode      = 0x4;
        deskewmode      = 0x1;
        desc2_mode      = 0x4;
        cl36_en         = 0x0;
        disable_carrier_extend = 0x1;
        reorder_en      = 0x1;
        cl48_syncacq_en = 0x1;  
        block_sync_mode = 0x3;
        cl48_cgbad_en   = 0x1;
        bypass_cl49rxsm = 0x1;
        hg2_message_ivalid_code_enable = 0x0;
        os_mode         = 0x0;
        rx_x1_cntl0_deskw_en = 1;
        cl82_dswin           = 0;
        cl48_dswin64b66b     = 1;
        cl48_dswin8b10b      = 7;
        break;
    case FV_adr_21G_X4:
        descramblemode  = 0x2;
        decodemode      = 0x4;
        deskewmode      = 0x1;
        desc2_mode      = 0x4;
        cl36_en         = 0x0;
        disable_carrier_extend = 0x1;
        reorder_en      = 0x1;
        cl48_syncacq_en = 0x1;  
        block_sync_mode = 0x3;
        cl48_cgbad_en   = 0x1;
        bypass_cl49rxsm = 0x1;
        hg2_message_ivalid_code_enable = 0x0;
        os_mode         = 0x0;
        rx_x1_cntl0_deskw_en = 1;
        cl82_dswin           = 0;
        cl48_dswin64b66b     = 1;
        cl48_dswin8b10b      = 7;
        break;
    case FV_adr_20G_CR2:
    case FV_adr_20G_KR2:
        descramblemode  = 0x0;
        decodemode      = 0x6;
        deskewmode      = 0x3;
        desc2_mode      = 0x6;
        cl36_en         = 0x0;  
        disable_carrier_extend = 0x1;
        reorder_en      = 0x0;
        cl48_syncacq_en = 0x0;  
        block_sync_mode = 0x2;
        cl48_cgbad_en   = 0x0;
        bypass_cl49rxsm = 0x0;
        hg2_message_ivalid_code_enable = 0x1;
        os_mode         = 0x0;
        rx_x1_cntl0_deskw_en = 1;
        cl82_dswin           = 0x1e;
        cl48_dswin64b66b     = 0x1;
        cl48_dswin8b10b      = 0x7;
        break;
    case FV_adr_20G_CX2:
        descramblemode  = 0x1;
        decodemode      = 0x2;
        deskewmode      = 0x2; 
        desc2_mode      = 0x4; 
        cl36_en         = 0x0;
        disable_carrier_extend = 0x1;
        reorder_en      = 0x1;
        cl48_syncacq_en = 0x1;
        block_sync_mode = 0x5;
        cl48_cgbad_en   = 0x1;
        bypass_cl49rxsm = 0x1;
        hg2_message_ivalid_code_enable = 0x0;
        os_mode         = 0x0;
        rx_x1_cntl0_deskw_en = 1; 
        cl82_dswin           = 0x0;
        cl48_dswin64b66b     = 0x1;
        cl48_dswin8b10b      = 0x7;
        break;
    case FV_adr_40G_X4:
    case FV_adr_25p45G_X4:
    case FV_adr_12p7G_X2:
        descramblemode  = 0x1;
        decodemode      = 0x2;
        deskewmode      = 0x2;
        desc2_mode      = 0x4;   
        cl36_en         = 0x0;
        disable_carrier_extend = 0x1;
        reorder_en      = 0x1;
        cl48_syncacq_en = 0x1;
        block_sync_mode = 0x5;
        cl48_cgbad_en   = 0x1;
        bypass_cl49rxsm = 0x1;
        hg2_message_ivalid_code_enable = 0x1;
        os_mode         = 0x0;
        rx_x1_cntl0_deskw_en = 1;
        cl82_dswin           = 0x0;
        cl48_dswin64b66b     = 0x1;
        cl48_dswin8b10b      = 0x7;
        break;
    case FV_adr_40G_KR4:
    case FV_adr_40G_CR4:
        if (plldiv == FV_div70) {
        decodemode      = 0x6;
        desc2_mode      = 0x6;
        deskewmode      = 0x3;
        cl36_en         = 0x0;
        disable_carrier_extend = 0x1;
        reorder_en      = 0x0;
        cl48_syncacq_en = 0x0;
        block_sync_mode = 0x2;
        cl48_cgbad_en   = 0x0;
        bypass_cl49rxsm = 0x0;
        hg2_message_ivalid_code_enable = 0x1;
        hg2_en          = 1;
        hg2_codec       = 1;
        os_mode         = 0x0;
        rx_x1_cntl0_deskw_en = 1;
        cl82_dswin           = 0x1e;
        cl48_dswin64b66b     = 0x1;
        cl48_dswin8b10b      = 0x7;
        } else  if (plldiv == FV_div66){
        decodemode      = 0x6;
        desc2_mode      = 0x6;
        deskewmode      = 0x3;
        cl36_en         = 0x0;
        disable_carrier_extend = 0x1;
        reorder_en      = 0x0;
        cl48_syncacq_en = 0x0;
        block_sync_mode = 0x2;
        cl48_cgbad_en   = 0x0;
        bypass_cl49rxsm = 0x0;
        hg2_message_ivalid_code_enable = 0x1;
        os_mode         = 0x0;
        rx_x1_cntl0_deskw_en = 1;
        cl82_dswin           = 0x1e;
        cl48_dswin64b66b     = 0x1;
        cl48_dswin8b10b      = 0x7;
        }
        break;
    default:
        return CDK_E_IO;
    }

    /* HG setting */
    if(PHY_CTRL_LINE_INTF(pc) & PHY_IF_HIGIG) {
       /* not CL48 */
       hg2_en          = 1;
       hg2_codec       = 1;
    }

    ioerr += READ_RX_X4_PCS_CONTROL_0r(pc, &rx_x4_pcs_control_0);
    RX_X4_PCS_CONTROL_0r_CL36BYTEDELETEMODEf_SET(rx_x4_pcs_control_0, cl36byte_delete_mode);
    RX_X4_PCS_CONTROL_0r_DESC2_MODEf_SET(rx_x4_pcs_control_0, desc2_mode);
    RX_X4_PCS_CONTROL_0r_DESKEWMODEf_SET(rx_x4_pcs_control_0, deskewmode);
    RX_X4_PCS_CONTROL_0r_DECODERMODEf_SET(rx_x4_pcs_control_0, decodemode);
    RX_X4_PCS_CONTROL_0r_DESCRAMBLERMODEf_SET(rx_x4_pcs_control_0, descramblemode);
    ioerr += WRITE_RX_X4_PCS_CONTROL_0r(pc, rx_x4_pcs_control_0);

    ioerr += READ_RX_X4_CL36_RX_0r(pc, &rx_x4_cl36_rx_0);
    RX_X4_CL36_RX_0r_CL36_ENf_SET(rx_x4_cl36_rx_0, cl36_en);
    RX_X4_CL36_RX_0r_REORDER_ENf_SET(rx_x4_cl36_rx_0, reorder_en);
    RX_X4_CL36_RX_0r_DISABLE_CARRIER_EXTENDf_SET(rx_x4_cl36_rx_0, disable_carrier_extend);
    ioerr += WRITE_RX_X4_CL36_RX_0r(pc, rx_x4_cl36_rx_0);

    ioerr += READ_RX_X4_DECODE_CONTROL_0r(pc, &rx_x4_dec_ctrl_0);
    RX_X4_DECODE_CONTROL_0r_BLOCK_SYNC_MODEf_SET(rx_x4_dec_ctrl_0, block_sync_mode);
    RX_X4_DECODE_CONTROL_0r_HG2_CODECf_SET(rx_x4_dec_ctrl_0, hg2_codec);
    RX_X4_DECODE_CONTROL_0r_CL48_CGBAD_ENf_SET(rx_x4_dec_ctrl_0, cl48_cgbad_en);
    RX_X4_DECODE_CONTROL_0r_CL48_SYNCACQ_ENf_SET(rx_x4_dec_ctrl_0, cl48_syncacq_en);
    RX_X4_DECODE_CONTROL_0r_HG2_ENABLEf_SET(rx_x4_dec_ctrl_0, hg2_en);
    RX_X4_DECODE_CONTROL_0r_HG2_MESSAGE_INVALID_CODE_ENABLEf_SET(rx_x4_dec_ctrl_0, hg2_message_ivalid_code_enable);
    RX_X4_DECODE_CONTROL_0r_BYPASS_CL49RXSMf_SET(rx_x4_dec_ctrl_0, bypass_cl49rxsm);
    ioerr += WRITE_RX_X4_DECODE_CONTROL_0r(pc, rx_x4_dec_ctrl_0);

    if ((actual_speed == FV_adr_40G_X4)||
        (actual_speed == FV_adr_25p45G_X4)||
        (actual_speed == FV_adr_12p7G_X2)||
        (actual_speed == FV_adr_20G_CX2)) {
        if (actual_speed == FV_adr_12p7G_X2) {
            data = 0x900f;
        } else {
            data = 0x8001;
        }
        RX_X4_DECODE_CONTROL_1r_SET(rx_x4_decode_control_1, data);
        ioerr += WRITE_RX_X4_DECODE_CONTROL_1r(pc, rx_x4_decode_control_1);
    }

    ioerr += READ_RX_X4_PMA_CONTROL_0r(pc, &rx_x4_pma_ctrl_0);
    RX_X4_PMA_CONTROL_0r_RX_GBOX_AFRST_ENf_SET(rx_x4_pma_ctrl_0, rx_gbox_afrst_en);
    RX_X4_PMA_CONTROL_0r_OS_MODEf_SET(rx_x4_pma_ctrl_0, os_mode);
    ioerr += WRITE_RX_X4_PMA_CONTROL_0r(pc, rx_x4_pma_ctrl_0);

    if ((actual_speed == FV_adr_10G_X4)||
        (actual_speed == FV_adr_10G_CX4)||
        (actual_speed == FV_adr_13G_X4)||
        (actual_speed == FV_adr_15G_X4)||
        (actual_speed == FV_adr_16G_X4)||
        (actual_speed == FV_adr_10G_CX2_NOSCRAMBLE)||
        (actual_speed == FV_adr_20G_X4)){
        data = 0x0444;
        sync_code_sm_en = 1;
    } else if ((actual_speed == FV_adr_40G_X4)||
               (actual_speed == FV_adr_25p45G_X4)||
               (actual_speed == FV_adr_20G_CX2)||
               (actual_speed == FV_adr_12p7G_X2)) {
        data = 0x0323;
        sync_code_sm_en = 1;
    }
    if (sync_code_sm_en) {
        RX_X1_DECODE_CONTROL_0r_SET(rx_x1_decode_control_0, data);
        ioerr += WRITE_RX_X1_DECODE_CONTROL_0r(pc, rx_x1_decode_control_0);
    }

    if (rx_x1_cntl0_deskw_en) {
        ioerr += READ_RX_X1_DESKEW_WINDOWSr(pc, &rx_x1_deskew_windows);
        RX_X1_DESKEW_WINDOWSr_CL48_DSWIN8B10Bf_SET(rx_x1_deskew_windows, cl48_dswin8b10b);
        RX_X1_DESKEW_WINDOWSr_CL48_DSWIN64B66Bf_SET(rx_x1_deskew_windows, cl48_dswin64b66b);
        RX_X1_DESKEW_WINDOWSr_CL82_DSWINf_SET(rx_x1_deskew_windows, cl82_dswin);
        ioerr += WRITE_RX_X1_DESKEW_WINDOWSr(pc, rx_x1_deskew_windows);
    }
    
    if ((actual_speed == FV_adr_40G_X4)||
        (actual_speed == FV_adr_25p45G_X4)||
        (actual_speed == FV_adr_20G_CX2)||
        (actual_speed == FV_adr_12p7G_X2)) {
        RX_X1_SCW0r_SET(rx_x1_scw0, 0x8090);
        ioerr += WRITE_RX_X1_SCW0r(pc, rx_x1_scw0);

        RX_X1_SCW1r_SET(rx_x1_scw1, 0xa0b0);
        ioerr += WRITE_RX_X1_SCW1r(pc, rx_x1_scw1);

        RX_X1_SCW2r_SET(rx_x1_scw2, 0xc0d0);
        ioerr += WRITE_RX_X1_SCW2r(pc, rx_x1_scw2);

        RX_X1_SCW3r_SET(rx_x1_scw3, 0xe070);
        ioerr += WRITE_RX_X1_SCW3r(pc, rx_x1_scw3);

        RX_X1_SCW4r_SET(rx_x1_scw4, 0x0001);
        ioerr += WRITE_RX_X1_SCW4r(pc, rx_x1_scw4);

        RX_X1_SCW0_MASKr_SET(rx_x1_scw0_mask, 0xf0f0);
        ioerr += WRITE_RX_X1_SCW0_MASKr(pc, rx_x1_scw0_mask);

        RX_X1_SCW1_MASKr_SET(rx_x1_scw1_mask, 0xf0f0);
        ioerr += WRITE_RX_X1_SCW1_MASKr(pc, rx_x1_scw1_mask);

        RX_X1_SCW2_MASKr_SET(rx_x1_scw2_mask, 0xf0f0);
        ioerr += WRITE_RX_X1_SCW2_MASKr(pc, rx_x1_scw2_mask);

        RX_X1_SCW3_MASKr_SET(rx_x1_scw3_mask, 0xf0f0);
        ioerr += WRITE_RX_X1_SCW3_MASKr(pc, rx_x1_scw3_mask);

        RX_X1_SCW4_MASKr_SET(rx_x1_scw4_mask, 0x0003);
        ioerr += WRITE_RX_X1_SCW4_MASKr(pc, rx_x1_scw4_mask);
    }

    if ((actual_speed == FV_adr_10G_X4)||
        (actual_speed == FV_adr_10G_CX4)||
        (actual_speed == FV_adr_13G_X4)||
        (actual_speed == FV_adr_15G_X4)||
        (actual_speed == FV_adr_16G_X4)||
        (actual_speed == FV_adr_20G_X4)){
        rx_control0_misc0_en = 1;
    } else if ((actual_speed == FV_adr_40G_X4)||
               (actual_speed == FV_adr_25p45G_X4)||
               (actual_speed == FV_adr_20G_CX2)||
               (actual_speed == FV_adr_12p7G_X2)) {
        rx_control0_misc0_en = 1;
    }
    if (rx_control0_misc0_en) {
        ioerr += READ_RX_X2_MISC_0r(pc, &rx_x2_misc_0);
        RX_X2_MISC_0r_CHK_END_ENf_SET(rx_x2_misc_0, 1);
        RX_X2_MISC_0r_LINK_ENf_SET(rx_x2_misc_0, 1);
        ioerr += WRITE_RX_X2_MISC_0r(pc, rx_x2_misc_0);
    }

    ioerr += READ_RX_X2_MISC_0r(pc, &rx_x2_misc_0);
    if (PHY_CTRL_LINE_INTF(pc) & PHY_IF_HIGIG) {
        RX_X2_MISC_0r_CHK_END_ENf_SET(rx_x2_misc_0, 0);
    } else {
        RX_X2_MISC_0r_CHK_END_ENf_SET(rx_x2_misc_0, 1);
    }
    ioerr += WRITE_RX_X2_MISC_0r(pc, rx_x2_misc_0);

    if ((actual_speed == FV_adr_20G_CR2)||
        (actual_speed == FV_adr_20G_KR2)||
        (actual_speed == FV_adr_40G_KR4)||
        (actual_speed == FV_adr_40G_CR4)) {
        RX_X4_FEC_0r_SET(rx_x4_fec_0, 0);
        ioerr += WRITE_RX_X4_FEC_0r(pc, rx_x4_fec_0);
    }

    if ((actual_speed == FV_adr_20G_CR2)||
        (actual_speed == FV_adr_20G_KR2)) {
        CL82_LANE_0_AM_BYTE10r_SET(cl82_lane_0_am_byte10, 0x7690);
        ioerr += WRITE_CL82_LANE_0_AM_BYTE10r(pc, cl82_lane_0_am_byte10);

        CL82_LANE_1_AM_BYTE10r_SET(cl82_lane_1_am_byte10, 0xc4f0);
        ioerr += WRITE_CL82_LANE_1_AM_BYTE10r(pc, cl82_lane_1_am_byte10);

        CL82_LANES_1_0_AM_BYTE2r_SET(cl82_lanes_0_1_am_byte2, 0xe647);
        ioerr += WRITE_CL82_LANES_1_0_AM_BYTE2r(pc, cl82_lanes_0_1_am_byte2);
    }

    /* CL72 tap selection. add HW tuning */
    ioerr += READ_CL72_MISC1_CONTROLr(pc, &cl72_misc1_ctrl);
    if (os_mode == 0) {  /* os_mode 1 */
       if (plldiv >= FV_div66) { /* >= 10.3125G, select KR mode */
            CL72_MISC1_CONTROLr_TAP_DEFAULT_MUXSEL_FORCEVALf_SET(cl72_misc1_ctrl, 3);
       } else if (plldiv >= FV_div40 ) {  /* >= 6.25, select BR mode */
            CL72_MISC1_CONTROLr_TAP_DEFAULT_MUXSEL_FORCEVALf_SET(cl72_misc1_ctrl, 2);
       } else if (plldiv >= FV_div32) { /* >= 3.125, select OS mode */
            CL72_MISC1_CONTROLr_TAP_DEFAULT_MUXSEL_FORCEVALf_SET(cl72_misc1_ctrl, 1);
       } else {  /* < 3.125, select KX mode */
            CL72_MISC1_CONTROLr_TAP_DEFAULT_MUXSEL_FORCEVALf_SET(cl72_misc1_ctrl, 0);
       }
    } else {
       if ((plldiv == FV_div66) && (os_mode == 3)) {  /* special case os_mode 3.3, select 2p5 mode */
            CL72_MISC1_CONTROLr_TAP_DEFAULT_MUXSEL_FORCEVALf_SET(cl72_misc1_ctrl, 4);
       } else if (((actual_speed == FV_adr_10G_X4) || (actual_speed == FV_adr_10G_CX4)) &&
                 (plldiv == FV_div42)) {
            /* special os_mode 2 case that pll_rate=date rate. The rest of os_mode 2 has pll_rate=2*data_rate */
            CL72_MISC1_CONTROLr_TAP_DEFAULT_MUXSEL_FORCEVALf_SET(cl72_misc1_ctrl, 2);
       } else if (os_mode == 1) {  /* os_mode 2 */
            CL72_MISC1_CONTROLr_TAP_DEFAULT_MUXSEL_FORCEVALf_SET(cl72_misc1_ctrl, 1);
       } else {
            /* so far the reset is below 2.5G, so KX mode */
            CL72_MISC1_CONTROLr_TAP_DEFAULT_MUXSEL_FORCEVALf_SET(cl72_misc1_ctrl, 0);
       }
    }
    CL72_MISC1_CONTROLr_TAP_DEFAULT_MUXSEL_FORCEf_SET(cl72_misc1_ctrl, 1);
    ioerr += WRITE_CL72_MISC1_CONTROLr(pc, cl72_misc1_ctrl);

    data = 1;
    /* Fault CL49 */
    ioerr += READ_TX_X4_MISCr(pc, &tx_x4_misc);
    TX_X4_MISCr_CL49_TX_RF_ENABLEf_SET(tx_x4_misc, data);
    TX_X4_MISCr_CL49_TX_LF_ENABLEf_SET(tx_x4_misc, data);
    TX_X4_MISCr_CL49_TX_LI_ENABLEf_SET(tx_x4_misc, data);
    ioerr += WRITE_TX_X4_MISCr(pc, tx_x4_misc);

    ioerr += READ_RX_X4_DECODE_CONTROL_0r(pc, &rx_x4_dec_ctrl_0);
    RX_X4_DECODE_CONTROL_0r_CL49_RX_RF_ENABLEf_SET(rx_x4_dec_ctrl_0, data);
    RX_X4_DECODE_CONTROL_0r_CL49_RX_LF_ENABLEf_SET(rx_x4_dec_ctrl_0, data);
    RX_X4_DECODE_CONTROL_0r_CL49_RX_LI_ENABLEf_SET(rx_x4_dec_ctrl_0, data);
    ioerr += WRITE_RX_X4_DECODE_CONTROL_0r(pc, rx_x4_dec_ctrl_0);

    /* Fault CL48 */
    ioerr += READ_TX_X2_CL48_0r(pc, &tx_x2_cl48_0);
    TX_X2_CL48_0r_CL48_TX_RF_ENABLEf_SET(tx_x2_cl48_0, data);
    TX_X2_CL48_0r_CL48_TX_LF_ENABLEf_SET(tx_x2_cl48_0, data);
    TX_X2_CL48_0r_CL48_TX_LI_ENABLEf_SET(tx_x2_cl48_0, data);
    ioerr += WRITE_TX_X2_CL48_0r(pc, tx_x2_cl48_0);

    ioerr += READ_RX_X2_MISC_1r(pc, &rx_x2_misc_1);
    RX_X2_MISC_1r_CL48_RX_RF_ENABLEf_SET(rx_x2_misc_1, data);
    RX_X2_MISC_1r_CL48_RX_LF_ENABLEf_SET(rx_x2_misc_1, data);
    RX_X2_MISC_1r_CL48_RX_LI_ENABLEf_SET(rx_x2_misc_1, data);
    ioerr += WRITE_RX_X2_MISC_1r(pc, rx_x2_misc_1);

    /* Fault CL82 */
    ioerr += READ_TX_X2_CL82_0r(pc, &tx_x2_cl82_0);
    TX_X2_CL82_0r_CL82_TX_RF_ENABLEf_SET(tx_x2_cl82_0, data);
    TX_X2_CL82_0r_CL82_TX_LF_ENABLEf_SET(tx_x2_cl82_0, data);
    TX_X2_CL82_0r_CL82_TX_LI_ENABLEf_SET(tx_x2_cl82_0, data);
    ioerr += WRITE_TX_X2_CL82_0r(pc, tx_x2_cl82_0);

    ioerr += READ_RX_X2_MISC_1r(pc, &rx_x2_misc_1);
    RX_X2_MISC_1r_CL82_RX_RF_ENABLEf_SET(rx_x2_misc_1, data);
    RX_X2_MISC_1r_CL82_RX_LF_ENABLEf_SET(rx_x2_misc_1, data);
    RX_X2_MISC_1r_CL82_RX_LI_ENABLEf_SET(rx_x2_misc_1, data);
    ioerr += WRITE_RX_X2_MISC_1r(pc, rx_x2_misc_1);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

/*
 * Function:
 *      _tsc_autoneg_control_set
 * Purpose:
 *      configure auto-negotiation.
 * Parameters:
 *      pc - PHY control structure
 *      an_type - auto-negotiation type
 *      autoneg - current auto-negotiation status
 * Returns:
 *      CDK_E_xxx
 */
static int
_tsc_autoneg_control_set(phy_ctrl_t* pc, int an_type, int autoneg)
{
    int ioerr = 0;
    int lane_num;
    uint32_t data;
    uint32_t an_setup_enable, cl37_next_page;
    uint32_t cl37_full_duplex, sgmii_full_duplex;
    uint32_t cl37_bam_code, over1g_ability;
    uint32_t over1g_page_count, next_page;
    uint32_t cl73_bam_code, hppam_20gkr2;
    uint32_t cl73_nonce_match_over, cl73_nonce_match_val;
    uint32_t base_selector, cl37_bam_enable;
    uint32_t cl73_bam_enable, cl73_hpam_enable;
    uint32_t cl73_enable, cl37_sgmii_enable;
    uint32_t cl37_enable, cl37_restart, cl73_restart;
    RX_X4_PMA_CONTROL_0r_t rx_x4_pma_ctrl_0;
    ENABLESr_t enables;
    CL37_BASE_ABILr_t cl37_base_abil;
    CL37_BAM_ABILr_t cl37_bam_abil;
    CL73_BASE_ABIL_0r_t cl73_base_abil;
    CL73_BAM_ABILr_t cl73_bam_abil;
    CL73_UCTRL1r_t cl73_uctrl1;
    CL73_BASE_ABIL_1r_t cl73_base_abil_1;

    /* AUTONEG_CONTROL */
    an_setup_enable                 = 0x1;
    cl37_bam_enable                 = 0x0;
    cl73_bam_enable                 = 0x0;
    cl73_hpam_enable                = 0x0;
    cl73_enable                     = 0x0;
    cl37_sgmii_enable               = 0x0;
    cl37_enable                     = 0x0;
    cl73_nonce_match_over           = 0x1;
    cl73_nonce_match_val            = 0x0;
    base_selector                   = 0x1;
    cl73_bam_code                   = 0x0;
    hppam_20gkr2                    = 0x0;
    next_page                       = 0x0;
    cl37_next_page                  = 0x0;
    cl37_full_duplex                = 0x1; 
    sgmii_full_duplex               = 0x1;
    cl37_bam_code                   = 0x0;
    over1g_ability                  = 0x0;
    over1g_page_count               = 0x0;
    cl37_restart                    = 0x0;
    cl73_restart                    = 0x0;

    lane_num = _tsc_serdes_lane(pc);
    if (lane_num < 0) {
        return CDK_E_IO;
    }

    switch(an_type) {
    case FV_an_CL73:
        cl73_restart                = 0x1;
        cl73_enable                 = 0x1;
        cl73_nonce_match_over       = 0x1;
        cl73_nonce_match_val        = 0x0;
        base_selector               = 0x1;
        break;
    case FV_an_CL37:
    case FV_an_CL37_10G:
        cl37_restart                = 0x1;
        cl37_enable                 = 0x1;
        cl37_bam_code               = 0x0;
        over1g_ability              = 0x0;
        over1g_page_count           = 0x0;
        sgmii_full_duplex           = 0x0;
        break;
    case FV_an_CL37_BAM:
        cl37_restart                = 0x1;
        cl37_enable                 = 0x1;
        cl37_bam_enable             = 0x1;
        cl37_bam_code               = 0x1;
        over1g_ability              = 0x1;
        over1g_page_count           = 0x1;
        sgmii_full_duplex           = 0x0;
        cl37_next_page              = 0x1;
        break;
    case FV_an_CL37_SGMII:
        cl37_restart                = 0x1;
        cl37_sgmii_enable           = 0x1;
        cl37_enable                 = 0x1;
        cl37_bam_code               = 0x0;
        over1g_ability              = 0x0;
        over1g_page_count           = 0x0;
        break;
    case FV_an_CL73_BAM:
        cl73_restart                = 0x1;
        cl73_enable                 = 0x1; 
        cl73_bam_enable             = 0x1;
        cl73_bam_code               = 0x3;
        next_page                   = 0x1;
        cl73_nonce_match_over       = 0x1;
        cl73_nonce_match_val        = 0x0;
        base_selector               = 0x1;
        break;
    case FV_an_HPAM:
        cl73_restart                = 0x1;
        cl73_enable                 = 0x1; 
        cl73_hpam_enable            = 0x1;
        cl73_bam_code               = 0x3;
        hppam_20gkr2                = 0x1;
        next_page                   = 0x1;
        cl73_nonce_match_over       = 0x1;
        cl73_nonce_match_val        = 0x0;
        base_selector               = 0x1;
        break;
    default:
        break;
    }

    /* OS mode overwrite */
    ioerr += READLN_RX_X4_PMA_CONTROL_0r(pc, lane_num, &rx_x4_pma_ctrl_0);
    if (an_type == FV_an_CL37_10G) {
        RX_X4_PMA_CONTROL_0r_OVERRIDE_OS_MODEf_SET(rx_x4_pma_ctrl_0, 1);
        RX_X4_PMA_CONTROL_0r_OS_MODEf_SET(rx_x4_pma_ctrl_0, 7);
    } else {
        RX_X4_PMA_CONTROL_0r_OVERRIDE_OS_MODEf_SET(rx_x4_pma_ctrl_0, 0);
        RX_X4_PMA_CONTROL_0r_OS_MODEf_SET(rx_x4_pma_ctrl_0, 0);
    }
    ioerr += WRITELN_RX_X4_PMA_CONTROL_0r(pc, lane_num, rx_x4_pma_ctrl_0);

    /* Set an advertised ports */
    ioerr += READLN_ENABLESr(pc, lane_num, &enables);
    ENABLESr_AN_SETUP_ENABLEf_SET(enables, an_setup_enable);
    if (IS_4LANE_PORT(pc)) {
        ENABLESr_NUM_ADVERTISED_LANESf_SET(enables, 2);
    } else if (IS_2LANE_PORT(pc)) {
        ENABLESr_NUM_ADVERTISED_LANESf_SET(enables, 1);
    } else {
        ENABLESr_NUM_ADVERTISED_LANESf_SET(enables, 0);
    }
    ENABLESr_CL73_AN_RESTARTf_SET(enables, 0);
    ENABLESr_CL37_AN_RESTARTf_SET(enables, 0);
    ioerr += WRITELN_ENABLESr(pc, lane_num, enables);

    /* Set an abilities */
    ioerr += READLN_CL37_BASE_ABILr(pc, lane_num, &cl37_base_abil);
    CL37_BASE_ABILr_CL37_NEXT_PAGEf_SET(cl37_base_abil, cl37_next_page);
    CL37_BASE_ABILr_CL37_FULL_DUPLEXf_SET(cl37_base_abil, cl37_full_duplex);
    CL37_BASE_ABILr_SGMII_FULL_DUPLEXf_SET(cl37_base_abil, sgmii_full_duplex);
    ioerr += WRITELN_CL37_BASE_ABILr(pc, lane_num, cl37_base_abil);

    /* CL37 bam abilities */
    ioerr += READLN_CL37_BAM_ABILr(pc, lane_num, &cl37_bam_abil);
    CL37_BAM_ABILr_CL37_BAM_CODEf_SET(cl37_bam_abil, cl37_bam_code);
    CL37_BAM_ABILr_OVER1G_ABILITYf_SET(cl37_bam_abil, over1g_ability);
    CL37_BAM_ABILr_OVER1G_PAGE_COUNTf_SET(cl37_bam_abil, over1g_page_count);
    ioerr += WRITELN_CL37_BAM_ABILr(pc, lane_num, cl37_bam_abil);

    /* Set next page bit */
    ioerr += READLN_CL73_BASE_ABIL_0r(pc, lane_num, &cl73_base_abil);
    CL73_BASE_ABIL_0r_NEXT_PAGEf_SET(cl73_base_abil, next_page);
    ioerr += WRITELN_CL73_BASE_ABIL_0r(pc, lane_num, cl73_base_abil);

    /* Set cl73_bam_code */
    ioerr += READLN_CL73_BAM_ABILr(pc, lane_num, &cl73_bam_abil);
    CL73_BAM_ABILr_CL73_BAM_CODEf_SET(cl73_bam_abil, cl73_bam_code);
    CL73_BAM_ABILr_HPAM_20GKR2f_SET(cl73_bam_abil, hppam_20gkr2);
    ioerr += WRITELN_CL73_BAM_ABILr(pc, lane_num, cl73_bam_abil);

    /* Set cl73 nonce */
    ioerr += READLN_CL73_UCTRL1r(pc, lane_num, &cl73_uctrl1);
    CL73_UCTRL1r_CL73_NONCE_MATCH_OVERf_SET(cl73_uctrl1, cl73_nonce_match_over);
    CL73_UCTRL1r_CL73_NONCE_MATCH_VALf_SET(cl73_uctrl1, cl73_nonce_match_val);
    ioerr += WRITELN_CL73_UCTRL1r(pc, lane_num, cl73_uctrl1);

    /* Set cl73 base_selector to 802.3 */
    ioerr += READLN_CL73_BASE_ABIL_1r(pc, lane_num, &cl73_base_abil_1);
    CL73_BASE_ABIL_1r_BASE_SELECTORf_SET(cl73_base_abil_1, base_selector);
    ioerr += WRITELN_CL73_BASE_ABIL_1r(pc, lane_num, cl73_base_abil_1);

    if ((autoneg == 0) && IS_4LANE_PORT(pc)) {
        data = 0x0d05;  /* 50ms */
    } else {
        data = 0x0005;
    }
    ioerr += PHY_BUS_WRITE(pc, 0x0000924a, data);

    /* Set an enable */
    ioerr += READLN_ENABLESr(pc, lane_num, &enables);
    ENABLESr_CL37_BAM_ENABLEf_SET(enables, cl37_bam_enable);
    ENABLESr_CL73_BAM_ENABLEf_SET(enables, cl73_bam_enable);
    ENABLESr_CL73_HPAM_ENABLEf_SET(enables, cl73_hpam_enable);
    ENABLESr_CL73_ENABLEf_SET(enables, cl73_enable);
    ENABLESr_CL37_SGMII_ENABLEf_SET(enables, cl37_sgmii_enable);
    ENABLESr_CL37_ENABLEf_SET(enables, cl37_enable);
    ENABLESr_CL37_AN_RESTARTf_SET(enables, cl37_restart);
    ENABLESr_CL73_AN_RESTARTf_SET(enables, cl73_restart);
    ioerr += WRITELN_ENABLESr(pc, lane_num, enables);

    /* Set an enable */
    ioerr += READLN_ENABLESr(pc, lane_num, &enables);
    ENABLESr_CL37_AN_RESTARTf_SET(enables, 0);
    ENABLESr_CL73_AN_RESTARTf_SET(enables, 0);
    ioerr += WRITELN_ENABLESr(pc, lane_num, enables);

    if (autoneg == 0) {
        ioerr += READLN_ENABLESr(pc, lane_num, &enables);
        ENABLESr_CL73_AN_RESTARTf_SET(enables, 1);
        ioerr += WRITE_ENABLESr(pc, enables);
        ENABLESr_CL73_AN_RESTARTf_SET(enables, 0);
        ioerr += WRITELN_ENABLESr(pc, lane_num, enables);
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

/*
 * Function:
 *      _tsc_txdrv_set
 * Purpose:
 *      configure tx tap driver.
 * Parameters:
 *      pc - PHY control structure
 *      txdrv - tx tap driver type
 * Returns:
 *      CDK_E_xxx
 */
static int
_tsc_txdrv_set(phy_ctrl_t* pc, int txdrv)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t ipredrive, idrive, post2, preemph;

    switch(txdrv) {
    case FV_txdrv_XFI:
        post2     = 0x00;
        idrive    = 0x02;
        ipredrive = 0x03;
        preemph   = (1 << 15) | (0x08 << 10) | (0x37 << 4) | (0x00 << 0);
        break;
    case FV_txdrv_XLAUI:
        post2     = 0x02;
        idrive    = 0x04;
        ipredrive = 0x04;
        preemph   = (1 << 15) | (0x18 << 10) | (0x27 << 4) | (0x00 << 0);
        break;
    case FV_txdrv_SFI:
        post2     = 0x02;
        idrive    = 0x02;
        ipredrive = 0x02;
        preemph   = (1 << 15) | (0x12 << 10) | (0x2d << 4) | (0x00 << 0);
        break;
    case FV_txdrv_SFIDAC:
        post2     = 0x02;
        idrive    = 0x02;
        ipredrive = 0x02;
        preemph   = (1 << 15) | (0x12 << 10) | (0x2d << 4) | (0x00 << 0);
        break;
    case FV_txdrv_6GOS2:
        post2     = 0x00;
        idrive    = 0x09;
        ipredrive = 0x09;
        preemph   = (1 << 15) | (0x00 << 10) | (0x3f << 4) | (0x00 << 0);
        break;
    case FV_txdrv_AN:
        post2     = 0x02;
        idrive    = 0x06;
        ipredrive = 0x09;
        preemph   = (0 << 15) | (0x00 << 10) | (0x00 << 4) | (0x00 << 0);
        break;
    case FV_txdrv_SR4:
        post2     = 0x02;
        idrive    = 0x05;
        ipredrive = 0x05;
        preemph   = (1 << 15) | (0x13 << 10) | (0x2c << 4) | (0x00 << 0);
        break;
    default:
        return CDK_E_CONFIG;
    }

    rv += PHY_CONFIG_SET(pc, PhyConfig_TxPost2, post2, NULL);
    rv += PHY_CONFIG_SET(pc, PhyConfig_TxIDrv, idrive, NULL);
    rv += PHY_CONFIG_SET(pc, PhyConfig_TxPreIDrv, ipredrive, NULL);
    rv += PHY_CONFIG_SET(pc, PhyConfig_TxPreemp, preemph, NULL);

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _tsc_pll_lock_wait
 * Purpose:
 *      Wait for PLL lock after sequencer (re)start.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_tsc_pll_lock_wait(phy_ctrl_t *pc)
{
    int ioerr = 0;
    ANAPLL_STATUSr_t ana_pll_status;
    int cnt;
    int lock = 0;
    
    for (cnt = 0; ioerr == 0 && cnt < PLL_LOCK_MSEC; cnt++) {
        ioerr += READ_ANAPLL_STATUSr(pc, &ana_pll_status); 
        if (ioerr) {
            return CDK_E_IO;
        }
        lock = ANAPLL_STATUSr_PLLSEQPASSf_GET(ana_pll_status);
        if (lock) {
            _PHY_DBG(pc, ("_tsc_pll_lock_wait: %d times\n", cnt));
            break;
        }
        PHY_SYS_USLEEP(1000);
    }
    if (lock == 0) {
        PHY_WARN(pc, ("ANAPLL did not lock\n"));
        return CDK_E_TIMEOUT;
    }
    return CDK_E_NONE;
}

/*
 * Function:
 *      _tsc_uc_rx_lane_control
 * Purpose:
 *      Put PHY in or out of reset depending on conditions.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_tsc_uc_cmd_seq(phy_ctrl_t *pc, int cmd)
{
    int ioerr = 0;
    int rv = 0;
    int idx, lane_mask, timeout;
    DSC_DIAG_CTRL0r_t dsc_diag_ctrl0;
    UC_CTRLr_t uc_ctrl;

    if ((PHY_CTRL_FLAGS(pc) & PHY_F_FW_RUNNING) == 0) {
        return CDK_E_NONE;
    }

    lane_mask = _tsc_serdes_lane_mask(pc);
    if (lane_mask < 0) {
        return CDK_E_IO;
    }

    for (idx = 0; idx < LANE_NUM_MASK + 1; idx++) {
        if ((lane_mask & (0x1 << idx)) == 0) {
            continue;
        }

        ioerr += READLN_DSC_DIAG_CTRL0r(pc, idx, &dsc_diag_ctrl0);
        if (DSC_DIAG_CTRL0r_DIAGNOSTICS_ENf_GET(dsc_diag_ctrl0)) {
            continue;
        }

        timeout = 0;
        do {
            rv += READLN_UC_CTRLr(pc, idx, &uc_ctrl);
        } while ((UC_CTRLr_READY_FOR_CMDf_GET(uc_ctrl) == 0)
                 && (++timeout < 2000));
        if(timeout >= 20000) {
           _PHY_DBG(pc, 
                ("uController l=%0d cmd=%0d timeOut frozen 1.0: rv=%0d data=%x\n",
                idx, cmd, rv, UC_CTRLr_GET(uc_ctrl)));
        }

        /* check for error */
        rv += READLN_UC_CTRLr(pc, idx, &uc_ctrl);
        if (UC_CTRLr_ERROR_FOUNDf_GET(uc_ctrl)) {
            _PHY_DBG(pc, ("uCode reported error!\n"));
        }

        /* next issue command to stop/resume Uc gracefully */
        ioerr += READLN_UC_CTRLr(pc, idx, &uc_ctrl);
        UC_CTRLr_READY_FOR_CMDf_SET(uc_ctrl, 0);
        UC_CTRLr_ERROR_FOUNDf_SET(uc_ctrl, 0);
        UC_CTRLr_CMD_INFOf_SET(uc_ctrl, 0);
        ioerr += WRITELN_UC_CTRLr(pc, idx, uc_ctrl);

        /* cmd= 0 ->stop gracefully, cmd=1 -> stop immdeidatly, 2 = resume */
        ioerr += READLN_UC_CTRLr(pc, idx, &uc_ctrl);
        UC_CTRLr_SUPPLEMENT_INFOf_SET(uc_ctrl, cmd);
        UC_CTRLr_GP_UC_REQf_SET(uc_ctrl, 1);
        ioerr += WRITELN_UC_CTRLr(pc, idx, uc_ctrl);
        PHY_SYS_USLEEP(1000);
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _tsc_uc_fw_control
 * Purpose:
 *      Put PHY in or out of reset depending on conditions.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_tsc_uc_fw_control(phy_ctrl_t *pc, int code_word, int enable)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int rerun = 0;
    uint32_t ena  = 0x0e00;
    uint32_t disa = 0xdd00;
    uint32_t offset, data;
    int lane_num;

    if ((PHY_CTRL_FLAGS(pc) & PHY_F_FW_RUNNING) == 0) {
        return CDK_E_NONE;
    }
    
    switch(code_word) {
    case FW_CTRL_CL72_SWITCH_OVER:
        if (IS_4LANE_PORT(pc)) {
            offset = 0x0114;
        } else if (IS_2LANE_PORT(pc)) {
            lane_num = _tsc_serdes_lane(pc);
            if (lane_num == 0) {
                offset = 0x12a;
                ena = 0xe300;
            } else {
                offset = 0x12c;
                ena = 0xec00;
            }
        } else {
            lane_num = _tsc_serdes_lane(pc);
            switch(lane_num) {
                case 0: offset = 0x4e4; break;
                case 1: offset = 0x5e4; break;
                case 2: offset = 0x6e4; break;
                case 3: offset = 0x7e4; break;
                default:
                    offset = 0x4e4; break;
            }
        }
        break;
    case FW_CTRL_CREDIT_PROGRAM: offset = 0x12e; break;
    default:
        offset = 0x12e; break;
    }

    /* Add RAM Command */
    offset |= 3 << 20;
    if (enable) {
        rv += phy_tsc_iblk_proxy_read(pc, offset, &data);
        if (data == ena) {
            rerun = 1;
        }
        rv += phy_tsc_iblk_proxy_write(pc, offset, ena);
        if (rerun) {
            rv += phy_tsc_iblk_proxy_write(pc, offset, ena);
        }
    } else {
        rv += phy_tsc_iblk_proxy_write(pc, offset, disa);
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _tsc_uc_rx_lane_control
 * Purpose:
 *      Put PHY in or out of reset depending on conditions.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_tsc_uc_rx_lane_control(phy_ctrl_t *pc, int enable)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int idx, lane_mask, offset;
    uint32_t data;

    if ((PHY_CTRL_FLAGS(pc) & PHY_F_FW_RUNNING) == 0) {
        return CDK_E_NONE;
    }
    
    lane_mask = _tsc_serdes_lane_mask(pc);
    if (lane_mask < 0) {
        return CDK_E_IO;
    }

    if (enable) {
        data = 0x0e00;
    } else {
        data = 0x0d00;
    }
    for (idx = 0; idx < LANE_NUM_MASK + 1; idx++) {
        if ((lane_mask & (0x1 << idx)) == 0) {
            continue;
        }

        switch(idx) {
        case 1:  offset = 0x5d0; break;
        case 2:  offset = 0x6d0; break;
        case 3:  offset = 0x7d0; break;
        default: offset = 0x4d0; break;
        }
        /* Add RAM Command */
        offset |= 3 << 20;

        rv += phy_tsc_iblk_proxy_write(pc, offset, data);
    }

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _tsc_serdes_stop
 * Purpose:
 *      Put PHY in or out of reset depending on conditions.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_tsc_serdes_stop(phy_ctrl_t *pc)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int stop, lane;
    int autoneg, loopback;
    DSC_MISC_CTRL0r_t dsc_misc_ctrl0;
    ANATX_CONTROL6r_t ana_tx_ctrl6;
    ANATX_DRIVERr_t tx_drv;
    RX_X4_PMA_CONTROL_0r_t rx_x4_pma_ctrl_0;
    TX_X4_MISCr_t tx_x4_misc;
    TX_X4_CREDIT0r_t tx_x4_credit0;
    TX_X4_PCS_CLOCKCNT0r_t tx_x4_pcs_clockcnt0;
    uint32_t speed, rx_seq_start;
    uint32_t f_any = PHY_F_PHY_DISABLE | PHY_F_PORT_DRAIN;
    uint32_t f_copper = PHY_F_MAC_DISABLE | PHY_F_SPEED_CHG | PHY_F_DUPLEX_CHG;

    stop = 0;
    if ((PHY_CTRL_FLAGS(pc) & f_any) ||
        ((PHY_CTRL_FLAGS(pc) & PHY_F_FIBER_MODE) == 0 &&
         (PHY_CTRL_FLAGS(pc) & f_copper))) {
        lane = _tsc_serdes_lane(pc);
        /* No power-down if lane is unknown */
        if (lane >= 0) {
            stop = 1;
        }
    }

    _PHY_DBG(pc, ("_tsc_serdes_stop: stop = %d\n", stop));

    if (stop) {
        ioerr += READ_TX_X4_CREDIT0r(pc, &tx_x4_credit0);
        TX_X4_CREDIT0r_CREDITENABLEf_SET(tx_x4_credit0, 0);
        ioerr += WRITE_TX_X4_CREDIT0r(pc, tx_x4_credit0);
        ioerr += READ_TX_X4_PCS_CLOCKCNT0r(pc, &tx_x4_pcs_clockcnt0);
        TX_X4_PCS_CLOCKCNT0r_PCS_CREDITENABLEf_SET(tx_x4_pcs_clockcnt0, 0);
        ioerr += WRITE_TX_X4_PCS_CLOCKCNT0r(pc, tx_x4_pcs_clockcnt0);

        ioerr += READ_TX_X4_MISCr(pc, &tx_x4_misc);
        TX_X4_MISCr_ENABLE_TX_LANEf_SET(tx_x4_misc, 0);
        ioerr += WRITE_TX_X4_MISCr(pc, tx_x4_misc);

        ioerr += READ_TX_X4_MISCr(pc, &tx_x4_misc);
        TX_X4_MISCr_RSTB_TX_LANEf_SET(tx_x4_misc, 0);
        ioerr += WRITE_TX_X4_MISCr(pc, tx_x4_misc);
        PHY_SYS_USLEEP(100);

        rv += _tsc_uc_rx_lane_control(pc, 0);

        ioerr += READ_RX_X4_PMA_CONTROL_0r(pc, &rx_x4_pma_ctrl_0);
        RX_X4_PMA_CONTROL_0r_RSTB_LANEf_SET(rx_x4_pma_ctrl_0, 0);
        ioerr += WRITE_RX_X4_PMA_CONTROL_0r(pc, rx_x4_pma_ctrl_0);

        ioerr += READ_ANATX_DRIVERr(pc, &tx_drv);
        ANATX_DRIVERr_ELEC_IDLEf_SET(tx_drv, stop);
        ioerr += WRITE_ANATX_DRIVERr(pc, tx_drv);

        /* Disable TX lane output */
        ioerr += READ_ANATX_CONTROL6r(pc, &ana_tx_ctrl6);
        ANATX_CONTROL6r_DISABLE_TX_Rf_SET(ana_tx_ctrl6, stop);
        ioerr += WRITE_ANATX_CONTROL6r(pc, ana_tx_ctrl6);

        /* DSC SM reset */
        ioerr += READ_DSC_MISC_CTRL0r(pc, &dsc_misc_ctrl0);
        DSC_MISC_CTRL0r_RXSEQSTARTf_SET(dsc_misc_ctrl0, 1);
        ioerr += WRITE_DSC_MISC_CTRL0r(pc, dsc_misc_ctrl0);

    } else {
        /* Start RX sequence */
        if (PHY_CTRL_FLAGS(pc) & PHY_F_FW_RUNNING) {
            rx_seq_start = 1;
        } else {
            rx_seq_start = 0;
        }
        ioerr += READ_DSC_MISC_CTRL0r(pc, &dsc_misc_ctrl0);
        DSC_MISC_CTRL0r_RXSEQSTARTf_SET(dsc_misc_ctrl0, rx_seq_start);
        ioerr += WRITE_DSC_MISC_CTRL0r(pc, dsc_misc_ctrl0);
        PHY_SYS_USLEEP(1000);

        /* Enable TX lane output */
        ioerr += READ_ANATX_CONTROL6r(pc, &ana_tx_ctrl6);
        ANATX_CONTROL6r_DISABLE_TX_Rf_SET(ana_tx_ctrl6, stop);
        ioerr += WRITE_ANATX_CONTROL6r(pc, ana_tx_ctrl6);

        /* Disable TX Electrical idle */
        ioerr += READ_ANATX_DRIVERr(pc, &tx_drv);
        ANATX_DRIVERr_ELEC_IDLEf_SET(tx_drv, stop);
        ioerr += WRITE_ANATX_DRIVERr(pc, tx_drv);
        PHY_SYS_USLEEP(1000);

        rv += PHY_AUTONEG_GET(pc, &autoneg);
        rv += PHY_SPEED_GET(pc, &speed);
        rv += PHY_LOOPBACK_GET(pc, &loopback);
        if (CDK_FAILURE(rv)) {
            return rv;
        }
        _PHY_DBG(pc, 
            ("_tsc_serdes_stop: an = %d, lb = %d, sp = %d\n", 
            autoneg, loopback, speed));

        if (autoneg || loopback) {
            rv += _tsc_uc_rx_lane_control(pc, 0);

            /* Enable RXP */
            ioerr += READ_RX_X4_PMA_CONTROL_0r(pc, &rx_x4_pma_ctrl_0);
            RX_X4_PMA_CONTROL_0r_RSTB_LANEf_SET(rx_x4_pma_ctrl_0, 1);
            ioerr += WRITE_RX_X4_PMA_CONTROL_0r(pc, rx_x4_pma_ctrl_0);

        } else {
            rv += _tsc_uc_rx_lane_control(pc, 1);
        }

        ioerr += READ_TX_X4_MISCr(pc, &tx_x4_misc);
        TX_X4_MISCr_ENABLE_TX_LANEf_SET(tx_x4_misc, 1);
        ioerr += WRITE_TX_X4_MISCr(pc, tx_x4_misc);

        ioerr += READ_TX_X4_MISCr(pc, &tx_x4_misc);
        TX_X4_MISCr_RSTB_TX_LANEf_SET(tx_x4_misc, 1);
        ioerr += WRITE_TX_X4_MISCr(pc, tx_x4_misc);

        if (autoneg) {
            ioerr += READ_TX_X4_CREDIT0r(pc, &tx_x4_credit0);
            TX_X4_CREDIT0r_CLOCKCNT0f_SET(tx_x4_credit0, 0x21);
            TX_X4_CREDIT0r_CREDITENABLEf_SET(tx_x4_credit0, 1);
            ioerr += WRITE_TX_X4_CREDIT0r(pc, tx_x4_credit0);
            ioerr += READ_TX_X4_PCS_CLOCKCNT0r(pc, &tx_x4_pcs_clockcnt0);
            TX_X4_PCS_CLOCKCNT0r_PCS_CREDITENABLEf_SET(tx_x4_pcs_clockcnt0, 0);
            ioerr += WRITE_TX_X4_PCS_CLOCKCNT0r(pc, tx_x4_pcs_clockcnt0);
        } else {
            ioerr += READ_TX_X4_CREDIT0r(pc, &tx_x4_credit0);
            TX_X4_CREDIT0r_CREDITENABLEf_SET(tx_x4_credit0, 1);
            ioerr += WRITE_TX_X4_CREDIT0r(pc, tx_x4_credit0);
            ioerr += READ_TX_X4_PCS_CLOCKCNT0r(pc, &tx_x4_pcs_clockcnt0);
            if ((speed == 100) || (speed == 10)) {
                TX_X4_PCS_CLOCKCNT0r_PCS_CREDITENABLEf_SET(tx_x4_pcs_clockcnt0, 1);
            } else {
                TX_X4_PCS_CLOCKCNT0r_PCS_CREDITENABLEf_SET(tx_x4_pcs_clockcnt0, 0);
            }
            ioerr += WRITE_TX_X4_PCS_CLOCKCNT0r(pc, tx_x4_pcs_clockcnt0);
        }

    }

    PHY_SYS_USLEEP(1000);

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _tsc_linkup_event
 * Purpose:
 *      PHY link-up event handler
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_tsc_linkup_event(phy_ctrl_t *pc)
{
    int ioerr = 0;

    _PHY_DBG(pc, ("link up event\n"));
    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

/***********************************************************************
 *
 * HELPER FUNCTIONS
 */
/*
 * Function:
 *      _tsc_init_stage_0
 * Purpose:
 *      Initialization required before firmware download.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_tsc_init_stage_0(phy_ctrl_t *pc)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    void *fw_data;
    int data, lane_mask;
    uint32_t tsc_mode;
    uint32_t fw_size;
    ANATX_DRIVERr_t tx_drv;
    RESETr_t reset;
    POWERr_t power;
    DSC_MISC_CTRL0r_t dsc_misc_ctrl0;
    TEMPERATUREr_t temperature;
    SETUPr_t setup;
    LANETESTr_t lane_test;
    ANAPLL_TIMER1r_t ana_pll_timer1;
    CL73_BREAK_LINKr_t cl73_break_link;
    CL37_RESTARTr_t cl37_restart;
    CL37_ACKr_t cl37_ack;
    CL37_ERRORr_t cl37_error;
    CL73_ERRORr_t cl73_error;
    CL73_DME_LOCKr_t cl73_dme_lock;
    IGNORE_LINK_TIMERr_t ignore_link_timer;
    LINK_FAIL_INHIBIT_TIMER_CL72r_t link_fail_inhibit_timer_cl72;
    LINK_FAIL_INHIBIT_TIMER_NOT_CL72r_t link_fail_inhibit_timer_not_cl72;
    PD_SD_TIMERr_t pd_sd_timer;
    LINK_UPr_t link_up;
    CL72_MAX_WAIT_TIMERr_t cl72_max_wait_timer;
    CL72_WAIT_TIMERr_t cl72_wait_timer;
    PRBSCTRLr_t prbs_ctrl;
    CL72_MISC1_CONTROLr_t cl72_misc1_ctrl;
    CL72_AN_OS_DEFAULT_CONTROLr_t cl72_an_os_dft_ctrl;
    CL72_AN_BR_DEFAULT_CONTROLr_t cl72_an_br_dft_ctrl;
    TX_X4_KR_DEFAULT_CONTROLr_t tx_x4_kr_dft_ctrl;
    CL72_AN_TWOP5_DEFAULT_CONTROLr_t cl72_an_2p5_dft_ctrl;
    TX_X4_CREDIT0r_t tx_x4_credit0;
    TX_X4_PCS_CLOCKCNT0r_t tx_x4_pcs_clockcnt0;

    _PHY_DBG(pc, ("init_stage_0\n"));

    /* Initialize resources shared by all 4 lanes */
    if (!_tsc_primary_lane(pc)) {
        return CDK_E_NONE;
    }

    if (IS_4LANE_PORT(pc)) {
        tsc_mode = FV_pm_1port_3210;
    } else if (IS_2LANE_PORT(pc)) {
        tsc_mode = FV_pm_2port_32_10;
    } else {
        tsc_mode = FV_pm_4port_3_2_1_0;
    }
    rv = PHY_CONFIG_SET(pc, PhyConfig_Mode,
                        tsc_mode, NULL);
    _PHY_DBG(pc, ("tsc mode = %d\n", tsc_mode));

    /* Force tx electrical status into idle, Electrical idle control is better to be per port base */
    ioerr += READ_ANATX_DRIVERr(pc, &tx_drv);
    ANATX_DRIVERr_ELEC_IDLEf_SET(tx_drv, 1);
    ioerr += WRITEALL_ANATX_DRIVERr(pc, tx_drv);

    /* Reset PLL */
    ioerr += READ_RESETr(pc, &reset);
    RESETr_RESET_PLLf_SET(reset, 0);
    ioerr += WRITE_RESETr(pc, reset);

    /* Power on the rx analog timer */
    ioerr += READ_POWERr(pc, &power);
    POWERr_POWER_ON_TIMER_ENABLEf_SET(power, 1);
    ioerr += WRITE_POWERr(pc, power);

    /* Enabling all 4 lanes analog (not necessary due to the HW deafult setting) */
    ioerr += READ_POWERr(pc, &power);
    POWERr_TX_PWRDWN_ANALOGf_SET(power, 0);
    POWERr_RX_PWRDWN_ANALOGf_SET(power, 0);
    ioerr += WRITE_POWERr(pc, power);

    PHY_SYS_USLEEP(1000);

    /* Disable the effect resetting sequencer state from autoneg */
    ioerr += READ_DSC_MISC_CTRL0r(pc, &dsc_misc_ctrl0);
    DSC_MISC_CTRL0r_RXSEQSTART_AN_DISABLEf_SET(dsc_misc_ctrl0, 1);
    ioerr += WRITEALL_DSC_MISC_CTRL0r(pc, dsc_misc_ctrl0);

    /* Configure die temperature */
    TEMPERATUREr_CLR(temperature);
    /* Force die temperature, Reset value is 0. */
    TEMPERATUREr_FORCE_TEMPERATUREf_SET(temperature, 1);
    /* Micro-controller reported temperature index,  0x8 = LTE_46p0C, Reset value is 0x0 = LTE_125p1C . */
    TEMPERATUREr_TEMP_IDXf_SET(temperature, 8);
    /* Current die temperature, Reset value is 0x0. */
    TEMPERATUREr_TEMPERATUREf_SET(temperature, 678);
    ioerr += WRITE_TEMPERATUREr(pc, temperature);

    /* Reset rx/tx lanes analog */
    lane_mask = _tsc_serdes_lane_mask(pc);

    ioerr += READ_RESETr(pc, &reset);
    data = RESETr_RX_RESET_ANALOGf_GET(reset);
    data |= lane_mask;
    RESETr_RX_RESET_ANALOGf_SET(reset, data);
    data = RESETr_TX_RESET_ANALOGf_GET(reset);
    data |= lane_mask;
    RESETr_TX_RESET_ANALOGf_SET(reset, data);
    ioerr += WRITE_RESETr(pc, reset);

    /* DSC SM reset: rx sequencer start : force the rx sequencer to its initial state */
    ioerr += READ_DSC_MISC_CTRL0r(pc, &dsc_misc_ctrl0);
    DSC_MISC_CTRL0r_RXSEQSTARTf_SET(dsc_misc_ctrl0, 1);
    ioerr += WRITEALL_DSC_MISC_CTRL0r(pc, dsc_misc_ctrl0);

    PHY_SYS_USLEEP(1000);

    /* Reset PLL sequencer */
    ioerr += READ_SETUPr(pc, &setup);
    SETUPr_START_SEQUENCERf_SET(setup, 0);
    ioerr += WRITE_SETUPr(pc, setup);

    /* Determines default pll clock multiplier factor: 156.25MHz */
    ioerr += READ_SETUPr(pc, &setup);
    SETUPr_REFCLK_SELf_SET(setup, FV_refclk_156p25MHz);
    ioerr += WRITE_SETUPr(pc, setup);

    PHY_SYS_USLEEP(2000);

    /* lfck_bypass, When set causes the LFCK (41us) clock to be driven by CLK25.  CLK25 averages refclk/5, Reset value is 1.*/
    ioerr += READ_LANETESTr(pc, &lane_test);
    LANETESTr_LFCK_BYPASSf_SET(lane_test, 0);
    ioerr += WRITE_LANETESTr(pc, lane_test);

    /* Set the TSC to SW default values for some registers */
    /* VcoStepTime : The number of LFCk clocks to wait after the vco range has been changed, The LFCK is 41us, Reset value is 2 */
    ioerr += READ_ANAPLL_TIMER1r(pc, &ana_pll_timer1);
    ANAPLL_TIMER1r_VCOSTEPTIMEf_SET(ana_pll_timer1, 1);
    ioerr += WRITE_ANAPLL_TIMER1r(pc, ana_pll_timer1);
    /* Period/range is 60 */
    CL73_BREAK_LINKr_CLR(cl73_break_link);
    CL73_BREAK_LINKr_TX_DISABLE_TIMER_PERIODf_SET(cl73_break_link, 0x10ed);
    ioerr += WRITE_CL73_BREAK_LINKr(pc, cl73_break_link);
    /* Period/range is 10 ms */
    CL37_RESTARTr_CLR(cl37_restart);
    CL37_RESTARTr_CL37_RESTART_TIMER_PERIODf_SET(cl37_restart, 0x029a);
    ioerr += WRITE_CL37_RESTARTr(pc, cl37_restart);
    /* Period/range is 10 ms */
    CL37_ACKr_CLR(cl37_ack);
    CL37_ACKr_CL37_ACK_TIMER_PERIODf_SET(cl37_ack, 0x029a);
    ioerr += WRITE_CL37_ACKr(pc, cl37_ack);
    /* Period/range is 20.6 ms */
    CL37_ERRORr_CLR(cl37_error);
    CL37_ERRORr_CL37_ERROR_TIMER_PERIODf_SET(cl37_error, 0x055d);
    ioerr += WRITE_CL37_ERRORr(pc, cl37_error);
    /* Period/range is 20.6 ms */
    CL73_ERRORr_CLR(cl73_error);
    CL73_ERRORr_CL73_ERROR_TIMER_PERIODf_SET(cl73_error, 0x1a10);
    ioerr += WRITE_CL73_ERRORr(pc, cl73_error);
    /* Period/range is 25 */
    CL73_DME_LOCKr_CLR(cl73_dme_lock);
    CL73_DME_LOCKr_PD_DME_LOCK_TIMER_PERIODf_SET(cl73_dme_lock, 0x14d4);
    ioerr += WRITE_CL73_DME_LOCKr(pc, cl73_dme_lock);
    /* Period is in ticks */
    IGNORE_LINK_TIMERr_CLR(ignore_link_timer);
    IGNORE_LINK_TIMERr_IGNORE_LINK_TIMER_PERIODf_SET(ignore_link_timer, 0x07d0);
    ioerr += WRITE_IGNORE_LINK_TIMERr(pc, ignore_link_timer);
    /* Period/range is typically 500ms */
    LINK_FAIL_INHIBIT_TIMER_CL72r_CLR(link_fail_inhibit_timer_cl72);
    LINK_FAIL_INHIBIT_TIMER_CL72r_LINK_FAIL_INHIBIT_TIMER_CL72_PERIODf_SET(link_fail_inhibit_timer_cl72, 0x8236);
    ioerr += WRITE_LINK_FAIL_INHIBIT_TIMER_CL72r(pc, link_fail_inhibit_timer_cl72);
    /* Period/range is typically 40ms */
    LINK_FAIL_INHIBIT_TIMER_NOT_CL72r_CLR(link_fail_inhibit_timer_not_cl72);
    LINK_FAIL_INHIBIT_TIMER_NOT_CL72r_LINK_FAIL_INHIBIT_TIMER_NCL72_PERIODf_SET(link_fail_inhibit_timer_not_cl72, 0x1a10);
    ioerr += WRITE_LINK_FAIL_INHIBIT_TIMER_NOT_CL72r(pc, link_fail_inhibit_timer_not_cl72);
    /* Parallel-Detect Signal Detect timer */
    PD_SD_TIMERr_CLR(pd_sd_timer);
    PD_SD_TIMERr_PD_SD_TIMER_PERIODf_SET(pd_sd_timer, 0x029a);
    ioerr += WRITE_PD_SD_TIMERr(pc, pd_sd_timer);
    /* Timer for the amount of time for the link to come up. Period/range is 100 to single copy*/
    LINK_UPr_CLR(link_up);
    LINK_UPr_CL73_LINK_UP_TIMER_PERIODf_SET(link_up, 0x029a);
    ioerr += WRITE_LINK_UPr(pc, link_up);
    /* Maximum training time, Period/range is 500 ms */
    CL72_MAX_WAIT_TIMERr_CLR(cl72_max_wait_timer);
    CL72_MAX_WAIT_TIMERr_CL72_MAX_WAIT_TIMER_PERIODf_SET(cl72_max_wait_timer, 0x8236);
    ioerr += WRITE_CL72_MAX_WAIT_TIMERr(pc, cl72_max_wait_timer);
    /* Period to keep transiming frames after the local device has completed training, Period/range is 100-300 frames */
    CL72_WAIT_TIMERr_CLR(cl72_wait_timer);
    CL72_WAIT_TIMERr_CL72_WAIT_TIMER_PERIODf_SET(cl72_wait_timer, 0x01fe);
    ioerr += WRITE_CL72_WAIT_TIMERr(pc, cl72_wait_timer);
    /* Reset tx/rx inverted data */
    ioerr += READ_PRBSCTRLr(pc, &prbs_ctrl);
    PRBSCTRLr_PRBS_INV_TXf_SET(prbs_ctrl, 1);
    PRBSCTRLr_PRBS_INV_RXf_SET(prbs_ctrl, 1);
    ioerr += WRITEALL_PRBSCTRLr(pc, prbs_ctrl);

    /* Init tx tap driver registers */
    /* KX */
    ioerr += READ_CL72_MISC1_CONTROLr(pc, &cl72_misc1_ctrl);
    CL72_MISC1_CONTROLr_TX_FIR_TAP_MAIN_KX_INIT_VALf_SET(cl72_misc1_ctrl, 0x3f);
    ioerr += WRITEALL_CL72_MISC1_CONTROLr(pc, cl72_misc1_ctrl);
    /* OS */
    ioerr += READ_CL72_AN_OS_DEFAULT_CONTROLr(pc, &cl72_an_os_dft_ctrl);
    CL72_AN_OS_DEFAULT_CONTROLr_TX_FIR_TAP_POST_OS_INIT_VALf_SET(cl72_an_os_dft_ctrl, 0);
    CL72_AN_OS_DEFAULT_CONTROLr_TX_FIR_TAP_MAIN_OS_INIT_VALf_SET(cl72_an_os_dft_ctrl, 0x3f);
    CL72_AN_OS_DEFAULT_CONTROLr_TX_FIR_TAP_PRE_OS_INIT_VALf_SET(cl72_an_os_dft_ctrl, 0);
    ioerr += WRITE_CL72_AN_OS_DEFAULT_CONTROLr(pc, cl72_an_os_dft_ctrl);
    /* BR */
    ioerr += READ_CL72_AN_BR_DEFAULT_CONTROLr(pc, &cl72_an_br_dft_ctrl);
    CL72_AN_BR_DEFAULT_CONTROLr_TX_FIR_TAP_POST_BR_INIT_VALf_SET(cl72_an_br_dft_ctrl, 0x16);
    CL72_AN_BR_DEFAULT_CONTROLr_TX_FIR_TAP_MAIN_BR_INIT_VALf_SET(cl72_an_br_dft_ctrl, 0x25);
    CL72_AN_BR_DEFAULT_CONTROLr_TX_FIR_TAP_PRE_BR_INIT_VALf_SET(cl72_an_br_dft_ctrl, 0x4);
    ioerr += WRITE_CL72_AN_BR_DEFAULT_CONTROLr(pc, cl72_an_br_dft_ctrl);
    /* KR */
    ioerr += READ_TX_X4_KR_DEFAULT_CONTROLr(pc, &tx_x4_kr_dft_ctrl);
    TX_X4_KR_DEFAULT_CONTROLr_TX_FIR_TAP_POST_KR_INIT_VALf_SET(tx_x4_kr_dft_ctrl, 0x16);
    TX_X4_KR_DEFAULT_CONTROLr_TX_FIR_TAP_MAIN_KR_INIT_VALf_SET(tx_x4_kr_dft_ctrl, 0x25);
    TX_X4_KR_DEFAULT_CONTROLr_TX_FIR_TAP_PRE_KR_INIT_VALf_SET(tx_x4_kr_dft_ctrl, 0x4);
    ioerr += WRITEALL_TX_X4_KR_DEFAULT_CONTROLr(pc, tx_x4_kr_dft_ctrl);
    /* 2p5 */
    ioerr += READ_CL72_AN_TWOP5_DEFAULT_CONTROLr(pc, &cl72_an_2p5_dft_ctrl);
    CL72_AN_TWOP5_DEFAULT_CONTROLr_TX_FIR_TAP_POST_2P5_INIT_VALf_SET(cl72_an_2p5_dft_ctrl, 0);
    CL72_AN_TWOP5_DEFAULT_CONTROLr_TX_FIR_TAP_MAIN_2P5_INIT_VALf_SET(cl72_an_2p5_dft_ctrl, 0x3f);
    CL72_AN_TWOP5_DEFAULT_CONTROLr_TX_FIR_TAP_PRE_2P5_INIT_VALf_SET(cl72_an_2p5_dft_ctrl, 0);
    ioerr += WRITE_CL72_AN_TWOP5_DEFAULT_CONTROLr(pc, cl72_an_2p5_dft_ctrl);

    /* Set default plldiv (66/70) in current TSC port mode */
    ioerr += READ_SETUPr(pc, &setup);
    SETUPr_FORCE_PLL_MODE_AFE_SELf_SET(setup, 1);
    SETUPr_DEFAULT_PLL_MODE_AFEf_SET(setup, FV_div66);
    ioerr += WRITE_SETUPr(pc, setup);
    PHY_SYS_USLEEP(1000);

    /* Clear credtis */
    TX_X4_CREDIT0r_CLR(tx_x4_credit0);
    ioerr += WRITEALL_TX_X4_CREDIT0r(pc, tx_x4_credit0);
    /* Clear pcs clock counter 0 */
    TX_X4_PCS_CLOCKCNT0r_CLR(tx_x4_pcs_clockcnt0);
    ioerr += WRITEALL_TX_X4_PCS_CLOCKCNT0r(pc, tx_x4_pcs_clockcnt0);

    /* Download FirmWare and start uController */
    fw_data = tsc_ucode_bin;
    fw_size = tsc_ucode_bin_len;
    rv = bcmi_tsc_xgxs_firmware_set(pc, 0, fw_size, fw_data);
    if (CDK_FAILURE(rv)) {
        PHY_WARN(pc, ("firmware download error\n"));
    } else {
        PHY_CTRL_FLAGS(pc) |= PHY_F_FW_RUNNING;
    }

    /* Enable pll sequencer with Default PLLdiv FV_div66 (0xa) and Default refclk 156.25 (0x3) */
    ioerr += READ_SETUPr(pc, &setup);
    SETUPr_START_SEQUENCERf_SET(setup, 1);
    ioerr += WRITE_SETUPr(pc, setup);

    /* Wait for pll lock */
    (void)_tsc_pll_lock_wait(pc);

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _tsc_init_stage_1
 * Purpose:
 *      Check firmware CRC (if enabled).
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_tsc_init_stage_1(phy_ctrl_t *pc)
{
    int rv = CDK_E_NONE;

    _PHY_DBG(pc, ("init_stage_1\n"));

    /* Check firmware CRC */
    if (_tsc_primary_lane(pc)) {
        rv = bcmi_tsc_xgxs_firmware_check(pc);
    } else {
        PHY_CTRL_FLAGS(pc) |= PHY_F_FW_RUNNING;
    }

    return rv;
}

/*
 * Function:
 *      _tsc_init_stage_2
 * Purpose:
 *      Initialization required after firmware download.
 * Parameters:
 *      pc - PHY control structure
 * Returns:
 *      CDK_E_xxx
 */
static int
_tsc_init_stage_2(phy_ctrl_t *pc)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int data, lane_mask;
    uint32_t ram_base;
    ANATX_DRIVERr_t anatx_driver;
    RESETr_t reset;
    FIRMWARE_MODEr_t fw_mod;
    CL73_BASE_ABIL_0r_t cl73_base_abil_0;
    OVER1G_ABIL_0r_t over1g_abil_0;
    CL72_MISC1_CONTROLr_t cl72_misc1_ctrl;
    OVER1G_ABIL_1r_t over1g_abil_1;
    TX_X4_PCS_CLOCKCNT0r_t tx_x4_pcs_clockcnt0;
    TX_X4_CREDIT0r_t tx_x4_credit0;
    RX_X2_MISC_1r_t rx_x2_misc_1;
    TX_X2_CL82_0r_t tx_x2_cl82_0;
    CL72_TX_FIR_TAPr_t cl72_tx_fir_tap;
    TX_X4_CL36_TX_0r_t tx_x4_cl36_tx_0;
    TX_X4_CREDIT1r_t tx_x4_credit1;
    CL72_MISC2_CONTROLr_t cl72_misc2_ctrl;
    TX_X4_LOOPCNTr_t tx_x4_loopcnt;
    TX_X4_MAC_CREDITGENCNTr_t tx_x4_mac_creditgencnt;
    TX_X4_PCS_CREDITGENCNTr_t tx_x4_pcs_creditgencnt;
    TX_X4_ENCODE_0r_t tx_x4_encode_0;
    TX_X2_BRCM_MODEr_t tx_x2_brcm_mode;
    RX_X4_PCS_CONTROL_0r_t rx_x4_pcs_ctrl_0;
    RX_X4_CL36_RX_0r_t rx_x4_cl36_rx_0;
    RX_X4_DECODE_CONTROL_0r_t rx_x4_decode_ctrl_0;
    RX_X4_PMA_CONTROL_0r_t rx_x4_pma_ctrl_0;
    RX_X2_MISC_0r_t rx_x2_misc_0;
    TX_X2_CL48_0r_t  tx_x2_cl48_0;
    TX_X4_MISCr_t tx_x4_misc;
    CL73_BAM_ABILr_t cl73_bam_abil;
    CL37_BASE_ABILr_t cl37_base_abil;
    CONTROLSr_t ctrls;
    ANAPLL_STATUSr_t anapll_status;

    /* PLL_LOCK_WAIT */
    PHY_SYS_USLEEP(1000);  
    ioerr += READ_ANAPLL_STATUSr(pc, &anapll_status);

    /* PARALLEL_DETECT_CONTROL */
    ioerr += READ_CONTROLSr(pc, &ctrls);
    CONTROLSr_PD_KX4_ENf_SET(ctrls, 0);
    CONTROLSr_PD_KX_ENf_SET(ctrls, 0);
    ioerr += WRITE_CONTROLSr(pc, ctrls);

    /* CREDIT_CONTROL */
    ioerr += READ_TX_X4_CREDIT0r(pc, &tx_x4_credit0);
    TX_X4_CREDIT0r_CREDITENABLEf_SET(tx_x4_credit0, 0);
    ioerr += WRITE_TX_X4_CREDIT0r(pc, tx_x4_credit0);
    
    ioerr += READ_TX_X4_PCS_CLOCKCNT0r(pc, &tx_x4_pcs_clockcnt0);
    TX_X4_PCS_CLOCKCNT0r_PCS_CREDITENABLEf_SET(tx_x4_pcs_clockcnt0, 0);
    ioerr += WRITE_TX_X4_PCS_CLOCKCNT0r(pc, tx_x4_pcs_clockcnt0);

    /* TX_LANE_CONTROL */
    ioerr += READ_TX_X4_MISCr(pc, &tx_x4_misc);
    TX_X4_MISCr_ENABLE_TX_LANEf_SET(tx_x4_misc, 0);
    ioerr += WRITE_TX_X4_MISCr(pc, tx_x4_misc);

    /* TX_LANE_CONTROL */
    ioerr += READ_TX_X4_MISCr(pc, &tx_x4_misc);
    TX_X4_MISCr_RSTB_TX_LANEf_SET(tx_x4_misc, 0);
    ioerr += WRITE_TX_X4_MISCr(pc, tx_x4_misc);

    /* RX_LANE_CONTROL */
    _tsc_uc_rx_lane_control(pc, 0);
 
    ioerr += READ_RX_X4_PMA_CONTROL_0r(pc, &rx_x4_pma_ctrl_0);
    RX_X4_PMA_CONTROL_0r_RSTB_LANEf_SET(rx_x4_pma_ctrl_0, 0);
    ioerr += WRITE_RX_X4_PMA_CONTROL_0r(pc, rx_x4_pma_ctrl_0);

    /* AUTONEG_SET */
    CL73_BASE_ABIL_0r_CLR(cl73_base_abil_0);
    CL73_BASE_ABIL_0r_CL73_PAUSEf_SET(cl73_base_abil_0, 1);
    if (IS_4LANE_PORT(pc)) {
        CL73_BASE_ABIL_0r_BASE_40GBASE_CR4f_SET(cl73_base_abil_0, 1);
        CL73_BASE_ABIL_0r_BASE_40GBASE_KR4f_SET(cl73_base_abil_0, 1);
        CL73_BASE_ABIL_0r_BASE_10GBASE_KRf_SET(cl73_base_abil_0, 0);
        CL73_BASE_ABIL_0r_BASE_10GBASE_KX4f_SET(cl73_base_abil_0, 1);
        CL73_BASE_ABIL_0r_BASE_1000BASE_KXf_SET(cl73_base_abil_0, 1);
    } else if (IS_2LANE_PORT(pc)) {
        CL73_BASE_ABIL_0r_BASE_10GBASE_KRf_SET(cl73_base_abil_0, 1);
        CL73_BASE_ABIL_0r_BASE_1000BASE_KXf_SET(cl73_base_abil_0, 1);
    } else {
        CL73_BASE_ABIL_0r_BASE_1000BASE_KXf_SET(cl73_base_abil_0, 1);
        CL73_BASE_ABIL_0r_BASE_10GBASE_KRf_SET(cl73_base_abil_0, 1);
    }
    ioerr += WRITE_CL73_BASE_ABIL_0r(pc, cl73_base_abil_0);
    
    OVER1G_ABIL_0r_CLR(over1g_abil_0);
    if (IS_4LANE_PORT(pc)) {
        OVER1G_ABIL_0r_BAM_12P5GBASE_X4f_SET(over1g_abil_0, 1);
        OVER1G_ABIL_0r_BAM_12GBASE_X4f_SET(over1g_abil_0, 1) ;
        OVER1G_ABIL_0r_BAM_10GBASE_X4_CX4f_SET(over1g_abil_0, 1);
        OVER1G_ABIL_0r_BAM_6GBASE_X4f_SET(over1g_abil_0, 1);
        OVER1G_ABIL_0r_BAM_5GBASE_X4f_SET(over1g_abil_0, 1);
        OVER1G_ABIL_0r_BAM_2P5GBASE_Xf_SET(over1g_abil_0, 1);
    } else if (IS_2LANE_PORT(pc)) {
        OVER1G_ABIL_0r_BAM_12P7GBASE_X2f_SET(over1g_abil_0, 1);
        OVER1G_ABIL_0r_BAM_10P5GBASE_X2f_SET(over1g_abil_0, 1);
        OVER1G_ABIL_0r_BAM_10GBASE_X2_CX4f_SET(over1g_abil_0, 1);
        OVER1G_ABIL_0r_BAM_10GBASE_X2f_SET(over1g_abil_0, 1);
        OVER1G_ABIL_0r_BAM_2P5GBASE_Xf_SET(over1g_abil_0, 1);
    } else {
        OVER1G_ABIL_0r_BAM_2P5GBASE_Xf_SET(over1g_abil_0, 1);
    }
    ioerr += WRITE_OVER1G_ABIL_0r(pc, over1g_abil_0);
    
    OVER1G_ABIL_1r_CLR(over1g_abil_1);
    if (IS_4LANE_PORT(pc)) {
        OVER1G_ABIL_1r_BAM_40GBASE_X4f_SET(over1g_abil_1, 1);
        OVER1G_ABIL_1r_BAM_25P455GBASE_X4f_SET(over1g_abil_1, 1);
        OVER1G_ABIL_1r_BAM_25P455GBASE_X4f_SET(over1g_abil_1, 1);
        OVER1G_ABIL_1r_BAM_21GBASE_X4f_SET(over1g_abil_1, 1);
        OVER1G_ABIL_1r_BAM_20GBASE_X4f_SET(over1g_abil_1, 1);
        OVER1G_ABIL_1r_BAM_20GBASE_X4_CX4f_SET(over1g_abil_1, 1);
        OVER1G_ABIL_1r_BAM_16GBASE_X4f_SET(over1g_abil_1, 1);
        OVER1G_ABIL_1r_BAM_15GBASE_X4f_SET(over1g_abil_1, 1);
        OVER1G_ABIL_1r_BAM_13GBASE_X4f_SET(over1g_abil_1, 1);
    } else if (IS_2LANE_PORT(pc)) {
        OVER1G_ABIL_1r_BAM_20GBASE_X2_CX4f_SET(over1g_abil_1, 1);
        OVER1G_ABIL_1r_BAM_20GBASE_X2f_SET(over1g_abil_1, 1);
        OVER1G_ABIL_1r_BAM_15P75GBASE_X2f_SET(over1g_abil_1, 1);
    }
    ioerr += WRITE_OVER1G_ABIL_1r(pc, over1g_abil_1);

    if (IS_2LANE_PORT(pc)) {
        ioerr += READ_CL73_BAM_ABILr(pc, &cl73_bam_abil);
        CL73_BAM_ABILr_BAM_20GBASE_CR2f_SET(cl73_bam_abil, 0);
        CL73_BAM_ABILr_BAM_20GBASE_KR2f_SET(cl73_bam_abil, 1);
        ioerr += WRITE_CL73_BAM_ABILr(pc, cl73_bam_abil);
    }
    
    ioerr += READ_CL37_BASE_ABILr(pc, &cl37_base_abil);
    CL37_BASE_ABILr_CL37_PAUSEf_SET(cl37_base_abil, 1);
    CL37_BASE_ABILr_SGMII_MASTER_MODEf_SET(cl37_base_abil, 0);
    CL37_BASE_ABILr_SGMII_SPEEDf_SET(cl37_base_abil, 2);
    ioerr += WRITE_CL37_BASE_ABILr(pc, cl37_base_abil);

    ioerr += READ_OVER1G_ABIL_1r(pc, &over1g_abil_1);
    OVER1G_ABIL_1r_HG2f_SET(over1g_abil_1, 0);
    OVER1G_ABIL_1r_FECf_SET(over1g_abil_1, 0);
    OVER1G_ABIL_1r_CL72f_SET(over1g_abil_1, 1);
    ioerr += WRITE_OVER1G_ABIL_1r(pc, over1g_abil_1);

    ioerr += READ_CL72_MISC2_CONTROLr(pc, &cl72_misc2_ctrl);
    CL72_MISC2_CONTROLr_SW_ACTUAL_SPEEDf_SET(cl72_misc2_ctrl, 0x1b);
    ioerr += WRITE_CL72_MISC2_CONTROLr(pc, cl72_misc2_ctrl);
    
    FIRMWARE_MODEr_CLR(fw_mod);
    FIRMWARE_MODEr_RESERVED0f_SET(fw_mod, 0);
    FIRMWARE_MODEr_FIRMWARE_MODEf_SET(fw_mod, 0);
    ioerr += WRITE_FIRMWARE_MODEr(pc, fw_mod);

    ioerr += READ_TX_X4_CREDIT0r(pc, &tx_x4_credit0);
    TX_X4_CREDIT0r_CLOCKCNT0f_SET(tx_x4_credit0, 0x21);
    TX_X4_CREDIT0r_CREDITENABLEf_SET(tx_x4_credit0, 1);
    TX_X4_CREDIT0r_SGMII_SPD_SWITCHf_SET(tx_x4_credit0, 0);
    ioerr += WRITE_TX_X4_CREDIT0r(pc, tx_x4_credit0);
    
    ioerr += READ_TX_X4_CREDIT1r(pc, &tx_x4_credit1);
    TX_X4_CREDIT1r_RESERVED0f_SET(tx_x4_credit1, 0x84);
    TX_X4_CREDIT1r_CLOCKCNT1f_SET(tx_x4_credit1, 0);
    ioerr += WRITE_TX_X4_CREDIT1r(pc, tx_x4_credit1);
    
    ioerr += READ_TX_X4_LOOPCNTr(pc, &tx_x4_loopcnt);
    TX_X4_LOOPCNTr_LOOPCNT0f_SET(tx_x4_loopcnt, 1);
    TX_X4_LOOPCNTr_LOOPCNT1f_SET(tx_x4_loopcnt, 0);
    ioerr += WRITE_TX_X4_LOOPCNTr(pc, tx_x4_loopcnt);
    
    ioerr += READ_TX_X4_MAC_CREDITGENCNTr(pc, &tx_x4_mac_creditgencnt);
    TX_X4_MAC_CREDITGENCNTr_MAC_CREDITGENCNTf_SET(tx_x4_mac_creditgencnt, 4);
    ioerr += WRITE_TX_X4_MAC_CREDITGENCNTr(pc, tx_x4_mac_creditgencnt);
    
    ioerr += READ_TX_X4_PCS_CLOCKCNT0r(pc, &tx_x4_pcs_clockcnt0);
    TX_X4_PCS_CLOCKCNT0r_PCS_CLOCKCNT0f_SET(tx_x4_pcs_clockcnt0, 0);
    TX_X4_PCS_CLOCKCNT0r_REPLICATION_CNTf_SET(tx_x4_pcs_clockcnt0, 0);
    ioerr += WRITE_TX_X4_PCS_CLOCKCNT0r(pc, tx_x4_pcs_clockcnt0);
    
    ioerr += READ_TX_X4_PCS_CREDITGENCNTr(pc, &tx_x4_pcs_creditgencnt);
    TX_X4_PCS_CREDITGENCNTr_PCS_CREDITGENCNTf_SET(tx_x4_pcs_creditgencnt, 0);
    ioerr += WRITE_TX_X4_PCS_CREDITGENCNTr(pc, tx_x4_pcs_creditgencnt);

    ioerr += READ_TX_X4_ENCODE_0r(pc, &tx_x4_encode_0);
    TX_X4_ENCODE_0r_HG2_CODECf_SET(tx_x4_encode_0, 0);
    TX_X4_ENCODE_0r_HG2_MESSAGE_INVALID_CODE_ENABLEf_SET(tx_x4_encode_0, 0);
    TX_X4_ENCODE_0r_HG2_ENABLEf_SET(tx_x4_encode_0, 0);
    TX_X4_ENCODE_0r_CL49_BYPASS_TXSMf_SET(tx_x4_encode_0, 0);
    TX_X4_ENCODE_0r_ENCODEMODEf_SET(tx_x4_encode_0, 5);
    ioerr += WRITE_TX_X4_ENCODE_0r(pc, tx_x4_encode_0);
    
    ioerr += READ_TX_X4_MISCr(pc, &tx_x4_misc);
    TX_X4_MISCr_SCR_MODEf_SET(tx_x4_misc, 0x3);
    ioerr += WRITE_TX_X4_MISCr(pc, tx_x4_misc);
    
    ioerr += READ_TX_X4_CL36_TX_0r(pc, &tx_x4_cl36_tx_0);
    TX_X4_CL36_TX_0r_DISABLE_PACKET_MISALIGNf_SET(tx_x4_cl36_tx_0, 0);
    ioerr += WRITE_TX_X4_CL36_TX_0r(pc, tx_x4_cl36_tx_0);
    
    ioerr += READ_TX_X2_BRCM_MODEr(pc, &tx_x2_brcm_mode);
    TX_X2_BRCM_MODEr_ACOL_SWAP_COUNT64B66Bf_SET(tx_x2_brcm_mode, 2);
    ioerr += WRITE_TX_X2_BRCM_MODEr(pc, tx_x2_brcm_mode);
    
    ioerr += READ_RX_X4_PCS_CONTROL_0r(pc, &rx_x4_pcs_ctrl_0);
    RX_X4_PCS_CONTROL_0r_DESCRAMBLERMODEf_SET(rx_x4_pcs_ctrl_0, 0);
    RX_X4_PCS_CONTROL_0r_DECODERMODEf_SET(rx_x4_pcs_ctrl_0, 1);
    RX_X4_PCS_CONTROL_0r_DESKEWMODEf_SET(rx_x4_pcs_ctrl_0, 0);
    RX_X4_PCS_CONTROL_0r_DESC2_MODEf_SET(rx_x4_pcs_ctrl_0, 1);
    RX_X4_PCS_CONTROL_0r_CL36BYTEDELETEMODEf_SET(rx_x4_pcs_ctrl_0, 0);
    ioerr += WRITE_RX_X4_PCS_CONTROL_0r(pc, rx_x4_pcs_ctrl_0);
    
    ioerr += READ_RX_X4_CL36_RX_0r(pc, &rx_x4_cl36_rx_0);
    RX_X4_CL36_RX_0r_CL36_ENf_SET(rx_x4_cl36_rx_0, 0);
    RX_X4_CL36_RX_0r_REORDER_ENf_SET(rx_x4_cl36_rx_0, 0);
    RX_X4_CL36_RX_0r_DISABLE_CARRIER_EXTENDf_SET(rx_x4_cl36_rx_0, 0);
    ioerr += WRITE_RX_X4_CL36_RX_0r(pc, rx_x4_cl36_rx_0);
    
    ioerr += READ_RX_X4_DECODE_CONTROL_0r(pc, &rx_x4_decode_ctrl_0);
    RX_X4_DECODE_CONTROL_0r_BYPASS_CL49RXSMf_SET(rx_x4_decode_ctrl_0, 0);
    RX_X4_DECODE_CONTROL_0r_HG2_MESSAGE_INVALID_CODE_ENABLEf_SET(
        rx_x4_decode_ctrl_0, 1);
    RX_X4_DECODE_CONTROL_0r_HG2_ENABLEf_SET(rx_x4_decode_ctrl_0, 0);
    RX_X4_DECODE_CONTROL_0r_CL48_SYNCACQ_ENf_SET(rx_x4_decode_ctrl_0, 0);
    RX_X4_DECODE_CONTROL_0r_CL48_CGBAD_ENf_SET(rx_x4_decode_ctrl_0, 0);
    RX_X4_DECODE_CONTROL_0r_HG2_CODECf_SET(rx_x4_decode_ctrl_0, 0);
    RX_X4_DECODE_CONTROL_0r_BLOCK_SYNC_MODEf_SET(rx_x4_decode_ctrl_0, 1);
    ioerr += WRITE_RX_X4_DECODE_CONTROL_0r(pc, rx_x4_decode_ctrl_0);
    
    ioerr += READ_RX_X4_PMA_CONTROL_0r(pc, &rx_x4_pma_ctrl_0);
    RX_X4_PMA_CONTROL_0r_OS_MODEf_SET(rx_x4_pma_ctrl_0, 0);
    RX_X4_PMA_CONTROL_0r_RX_GBOX_AFRST_ENf_SET(rx_x4_pma_ctrl_0, 0);
    ioerr += WRITE_RX_X4_PMA_CONTROL_0r(pc, rx_x4_pma_ctrl_0);
    
    ioerr += READ_RX_X2_MISC_0r(pc, &rx_x2_misc_0);
    RX_X2_MISC_0r_CHK_END_ENf_SET(rx_x2_misc_0, 1);
    ioerr += WRITE_RX_X2_MISC_0r(pc, rx_x2_misc_0);
    
    ioerr += READ_CL72_MISC1_CONTROLr(pc, &cl72_misc1_ctrl);
    CL72_MISC1_CONTROLr_TAP_DEFAULT_MUXSEL_FORCEVALf_SET(cl72_misc1_ctrl, 3);
    CL72_MISC1_CONTROLr_TAP_DEFAULT_MUXSEL_FORCEf_SET(cl72_misc1_ctrl, 1);
    ioerr += WRITE_CL72_MISC1_CONTROLr(pc, cl72_misc1_ctrl);
    
    ioerr += READ_TX_X4_MISCr(pc, &tx_x4_misc);
    TX_X4_MISCr_CL49_TX_LI_ENABLEf_SET(tx_x4_misc, 1);
    TX_X4_MISCr_CL49_TX_LF_ENABLEf_SET(tx_x4_misc, 1);
    TX_X4_MISCr_CL49_TX_RF_ENABLEf_SET(tx_x4_misc, 1);
    ioerr += WRITE_TX_X4_MISCr(pc, tx_x4_misc);
    
    ioerr += READ_RX_X4_DECODE_CONTROL_0r(pc, &rx_x4_decode_ctrl_0);
    RX_X4_DECODE_CONTROL_0r_CL49_RX_RF_ENABLEf_SET(rx_x4_decode_ctrl_0, 1);
    RX_X4_DECODE_CONTROL_0r_CL49_RX_LF_ENABLEf_SET(rx_x4_decode_ctrl_0, 1);
    RX_X4_DECODE_CONTROL_0r_CL49_RX_LI_ENABLEf_SET(rx_x4_decode_ctrl_0, 1);
    ioerr += WRITE_RX_X4_DECODE_CONTROL_0r(pc, rx_x4_decode_ctrl_0);
    
    ioerr += READ_TX_X2_CL48_0r(pc, &tx_x2_cl48_0);
    TX_X2_CL48_0r_CL48_TX_RF_ENABLEf_SET(tx_x2_cl48_0, 1);
    TX_X2_CL48_0r_CL48_TX_LF_ENABLEf_SET(tx_x2_cl48_0, 1);
    TX_X2_CL48_0r_CL48_TX_LI_ENABLEf_SET(tx_x2_cl48_0, 1);
    ioerr += WRITE_TX_X2_CL48_0r(pc, tx_x2_cl48_0);
    
    ioerr += READ_RX_X2_MISC_1r(pc, &rx_x2_misc_1);
    RX_X2_MISC_1r_CL48_RX_RF_ENABLEf_SET(rx_x2_misc_1, 1);
    RX_X2_MISC_1r_CL48_RX_LF_ENABLEf_SET(rx_x2_misc_1, 1);
    RX_X2_MISC_1r_CL48_RX_LI_ENABLEf_SET(rx_x2_misc_1, 1);
    ioerr += WRITE_RX_X2_MISC_1r(pc, rx_x2_misc_1);
    
    ioerr += READ_TX_X2_CL82_0r(pc, &tx_x2_cl82_0);
    TX_X2_CL82_0r_CL82_TX_RF_ENABLEf_SET(tx_x2_cl82_0, 1);
    TX_X2_CL82_0r_CL82_TX_LF_ENABLEf_SET(tx_x2_cl82_0, 1);
    TX_X2_CL82_0r_CL82_TX_LI_ENABLEf_SET(tx_x2_cl82_0, 1);
    ioerr += WRITE_TX_X2_CL82_0r(pc, tx_x2_cl82_0);
    
    ioerr += READ_RX_X2_MISC_1r(pc, &rx_x2_misc_1);
    RX_X2_MISC_1r_CL82_RX_RF_ENABLEf_SET(rx_x2_misc_1, 1);
    RX_X2_MISC_1r_CL82_RX_LF_ENABLEf_SET(rx_x2_misc_1, 1);
    RX_X2_MISC_1r_CL82_RX_LI_ENABLEf_SET(rx_x2_misc_1, 1);
    ioerr += WRITE_RX_X2_MISC_1r(pc, rx_x2_misc_1);

    /* TX_AMP_CONTROL */
    ioerr += READ_ANATX_DRIVERr(pc, &anatx_driver);
    ANATX_DRIVERr_IDRIVERf_SET(anatx_driver, 9);
    ioerr += WRITE_ANATX_DRIVERr(pc, anatx_driver);
    
    ioerr += READ_ANATX_DRIVERr(pc, &anatx_driver);
    ANATX_DRIVERr_IPREDRIVERf_SET(anatx_driver, 9);
    ioerr += WRITE_ANATX_DRIVERr(pc, anatx_driver);

    ioerr += READ_ANATX_DRIVERr(pc, &anatx_driver);
    ANATX_DRIVERr_POST2_COEFFf_SET(anatx_driver, 0);
    ioerr += WRITE_ANATX_DRIVERr(pc, anatx_driver);

    ioerr += READ_CL72_TX_FIR_TAPr(pc, &cl72_tx_fir_tap);
    CL72_TX_FIR_TAPr_TX_FIR_TAP_FORCEf_SET(cl72_tx_fir_tap, 1);
    CL72_TX_FIR_TAPr_TX_FIR_TAP_POSTf_SET(cl72_tx_fir_tap, 0);
    CL72_TX_FIR_TAPr_TX_FIR_TAP_MAINf_SET(cl72_tx_fir_tap, 0x3f);
    CL72_TX_FIR_TAPr_TX_FIR_TAP_PREf_SET(cl72_tx_fir_tap, 0);
    ioerr += WRITE_CL72_TX_FIR_TAPr(pc, cl72_tx_fir_tap);

    /* Add address type call back function */
    rv = PHY_CONFIG_GET(pc, PhyConfig_RamBase, &ram_base, NULL);
    if (ram_base == 0x0fb8) {
        PHY_CTRL_ADDR_TYPE(pc) = _tsc_addr_type_partial_proxy;
    } else {
        PHY_CTRL_ADDR_TYPE(pc) = NULL;
    }
    _PHY_DBG(pc, ("ram_base = 0x%04"PRIx32"\n", ram_base));


    /* enable FW credit_programming for A0, A1 TD2 only */
    if (ram_base == 0x0fb8) {
        _tsc_uc_fw_control(pc, FW_CTRL_CREDIT_PROGRAM, 1);
    }

    /* SOFT_RESET */
    lane_mask = _tsc_serdes_lane_mask(pc);
    ioerr += READ_RESETr(pc, &reset);
    data = RESETr_RX_RESET_ANALOGf_GET(reset);
    data &= ~lane_mask;
    RESETr_RX_RESET_ANALOGf_SET(reset, data);
    data = RESETr_TX_RESET_ANALOGf_GET(reset);
    data &= ~lane_mask;
    RESETr_TX_RESET_ANALOGf_SET(reset, data);
    ioerr += WRITE_RESETr(pc, reset);
    
    /* Wait PMD locked */
    PHY_SYS_USLEEP(1000); 
    
    /* RX_LANE_CONTROL */
    _tsc_uc_rx_lane_control(pc, 1);

    /* Default mode is fiber */
    PHY_NOTIFY(pc, PhyEvent_ChangeToFiber);

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      _tsc_init_stage
 * Purpose:
 *      Execute specified init stage.
 * Parameters:
 *      pc - PHY control structure
 *      stage - init stage
 * Returns:
 *      CDK_E_xxx
 */
static int
_tsc_init_stage(phy_ctrl_t *pc, int stage)
{
    switch (stage) {
    case 0:
        return _tsc_init_stage_0(pc);
    case 1:
        return _tsc_init_stage_1(pc);
    case 2:
        return _tsc_init_stage_2(pc);
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
extern cdk_symbols_t bcmi_tsc_xgxs_symbols;
#define SET_SYMBOL_TABLE(_pc) \
    PHY_CTRL_SYMBOLS(_pc) = &bcmi_tsc_xgxs_symbols
#else
#define SET_SYMBOL_TABLE(_pc)
#endif

/*
 * Function:
 *      bcmi_tsc_xgxs_probe
 * Purpose:     
 *      Probe for PHY.
 * Parameters:
 *      pc - PHY control structure
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcmi_tsc_xgxs_probe(phy_ctrl_t *pc)
{
    uint32_t phyid0, phyid1;
    SERDESIDr_t serdesid;
    uint32_t model;
    int ioerr = 0;

    ioerr += phy_brcm_serdes_id(pc, &phyid0, &phyid1);

    phyid1 &= ~PHY_ID1_REV_MASK;

    if (phyid0 == BCM_SERDES_PHY_ID0 && phyid1 == BCM_SERDES_PHY_ID1) {
        /* Common PHY ID found - read specific SerDes ID */
        ioerr += READ_SERDESIDr(pc, &serdesid);
        model = SERDESIDr_MODEL_NUMBERf_GET(serdesid);
        if (model == SERDES_ID_XGXS_TSC) {
            /* Always use clause 45 access */
            PHY_CTRL_FLAGS(pc) |= PHY_F_CLAUSE45;

            /* All lanes are accessed from the same PHY address */
            PHY_CTRL_FLAGS(pc) |= PHY_F_ADDR_SHARE;

            /* Set CL73 as default */
            PHY_CTRL_FLAGS(pc) &= ~PHY_F_CLAUSE37;
            PHY_CTRL_FLAGS(pc) |= PHY_F_CLAUSE73;

            SET_SYMBOL_TABLE(pc);
            return ioerr ? CDK_E_IO : CDK_E_NONE;
        }
    }
    return CDK_E_NOT_FOUND;
}


/*
 * Function:
 *      bcmi_tsc_xgxs_notify
 * Purpose:     
 *      Handle PHY notifications.
 * Parameters:
 *      pc - PHY control structure
 *      event - PHY event
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcmi_tsc_xgxs_notify(phy_ctrl_t *pc, phy_event_t event)
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
    rv = _tsc_serdes_stop(pc);

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:
 *      bcmi_tsc_xgxs_reset
 * Purpose:     
 *      Reset PHY.
 * Parameters:
 *      pc - PHY control structure
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcmi_tsc_xgxs_reset(phy_ctrl_t *pc)
{
    return CDK_E_NONE;
}

/*
 * Function:
 *      bcmi_tsc_xgxs_init
 * Purpose:     
 *      Initialize PHY driver.
 * Parameters:
 *      pc - PHY control structure
 * Returns:     
 *      CDK_E_NONE
 */
static int
bcmi_tsc_xgxs_init(phy_ctrl_t *pc)
{
    int rv = CDK_E_NONE;
    int stage;

    PHY_CTRL_CHECK(pc);

    if (PHY_CTRL_FLAGS(pc) & PHY_F_STAGED_INIT) {
        return CDK_E_NONE;
    }

    for (stage = 0; CDK_SUCCESS(rv); stage++) {
        rv = _tsc_init_stage(pc, stage);
    }

    if (rv == CDK_E_UNAVAIL) {
        /* Successfully completed all stages */
        rv = CDK_E_NONE;
    }

    return rv;
}

/*
 * Function:    
 *      bcmi_tsc_xgxs_link_get
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
bcmi_tsc_xgxs_link_get(phy_ctrl_t *pc, int *link, int *autoneg_done)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t linkup, latched;
    RX_X4_PCS_LIVE_STATUSr_t rx_x4_pcs_live_status;
    RX_X4_PCS_LATCHED_STATUS_0r_t rx_x4_latched_status;
    AN_ABIL_RESOLUTION_STATUSr_t an_abil_resolution_status;

    PHY_CTRL_CHECK(pc);

    *link = FALSE;

    /* Check PCS link status */
    ioerr  += READ_RX_X4_PCS_LIVE_STATUSr(pc, &rx_x4_pcs_live_status);
    linkup  = RX_X4_PCS_LIVE_STATUSr_LINK_STATUSf_GET(rx_x4_pcs_live_status);
    ioerr  += READ_RX_X4_PCS_LATCHED_STATUS_0r(pc, &rx_x4_latched_status);
    latched = RX_X4_PCS_LATCHED_STATUS_0r_LINK_STATUS_LLf_GET(rx_x4_latched_status);
    if (linkup && !latched) {
        *link = TRUE;
    }

    ioerr  += READ_AN_ABIL_RESOLUTION_STATUSr(pc, &an_abil_resolution_status);
    if (autoneg_done) {
        *autoneg_done = (AN_ABIL_RESOLUTION_STATUSr_GET(an_abil_resolution_status) & 0x8000) ? 1 : 0;
    }

    if (*link == TRUE) {
        if ((PHY_CTRL_FLAGS(pc) & PHY_F_LINK_UP) == 0) {
            ioerr += _tsc_linkup_event(pc);
        }
        PHY_CTRL_FLAGS(pc) |=  PHY_F_LINK_UP;
    } else {
        PHY_CTRL_FLAGS(pc) &= ~PHY_F_LINK_UP;
    }

    if (*link == FALSE) {
        ACTIVE_INTERFACE_SET(pc, 0);
    }
    
    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:    
 *      bcmi_tsc_xgxs_duplex_set
 * Purpose:     
 *      Set the current duplex mode (forced).
 * Parameters:
 *      pc - PHY control structure
 *      duplex - non-zero indicates full duplex, zero indicates half
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcmi_tsc_xgxs_duplex_set(phy_ctrl_t *pc, int duplex)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;

    PHY_CTRL_CHECK(pc);

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:    
 *      bcmi_tsc_xgxs_duplex_get
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
bcmi_tsc_xgxs_duplex_get(phy_ctrl_t *pc, int *duplex)
{
    *duplex = 1;

    return CDK_E_NONE;
}

/*
 * Function:    
 *      bcmi_tsc_xgxs_speed_set
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
bcmi_tsc_xgxs_speed_set(phy_ctrl_t *pc, uint32_t speed)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int autoneg, aspeed, txdrv;
    int lane_mask, fw_mode;
    uint32_t plldiv, cur_plldiv, cur_speed, data;
    TX_X4_CREDIT0r_t tx_x4_credit0;
    TX_X4_PCS_CLOCKCNT0r_t tx_x4_pcs_clockcnt0;
    RESETr_t reset;
    DSC_MISC_CTRL0r_t dsc_misc_ctrl0;
    SETUPr_t setup;
    RX_X4_RX_SIGDETr_t rx_x4_rx_sigdet;
    CL72_MISC2_CONTROLr_t misc2_ctrl;
    FIRMWARE_MODEr_t firmware_mode;
    CONTROLSr_t controls;
    LINK_FAIL_INHIBIT_TIMER_CL72r_t link_fail_inhibit_timer_cl72;
    CL72_MAX_WAIT_TIMERr_t cl72_max_wait_timer;
    PMD_10GBASE_KR_PMD_CONTROL_150r_t pmd_10gbase_kr_ctrl_150;

    PHY_CTRL_CHECK(pc);

    _PHY_DBG(pc, ("Speed Set: speed = %d\n", speed));

    /* Do not set speed if auto-negotiation is enabled */
    rv = PHY_AUTONEG_GET(pc, &autoneg);
    if (CDK_FAILURE(rv)) {
        return rv;
    }
    if (autoneg) {
        return CDK_E_NONE;
    }

    /* In custom mode speed is fixed and should be set in init */
    if (IS_2LANE_PORT(pc) && (PHY_CTRL_FLAGS(pc) & PHY_F_CUSTOM_MODE)) {
        return CDK_E_NONE;
    }

    /* Do not set speed if unchanged */
    rv = PHY_SPEED_GET(pc, &cur_speed);
    if (CDK_FAILURE(rv)) {
        return rv;
    }
    /*
    if (speed == cur_speed &&
        PHY_CTRL_LINE_INTF(pc) == ACTIVE_INTERFACE_GET(pc)) {
        return CDK_E_NONE;
    }
    */
    lane_mask = _tsc_serdes_lane_mask(pc);
    if (lane_mask < 0) {
        return CDK_E_IO;
    }

    rv = _tsc_plldiv_get(pc, &cur_plldiv);
    if (CDK_FAILURE(rv)) {
        return rv;
    }

    plldiv  = FV_div66;
    txdrv   = FV_txdrv_6GOS2;
    fw_mode = 0;
    if (IS_4LANE_PORT(pc)) {
        switch (speed) {
        case 10000:
            if (PHY_CTRL_LINE_INTF(pc) == PHY_IF_HIGIG) {
                aspeed = FV_adr_10G_X4;
            } else {
                aspeed = FV_adr_10G_CX4;
            }
            plldiv = FV_div40;
            break;
        case 10500:
            aspeed = FV_adr_10G_X4;
            plldiv = FV_div42;
            break;
        case 11000:
            if (PHY_CTRL_LINE_INTF(pc) == PHY_IF_HIGIG) {
                aspeed = FV_adr_10G_X4;
            } else {
                aspeed = FV_adr_10G_CX4;
            }
            plldiv = FV_div42;
            break;
        case 12700:
        case 13000:
            aspeed = FV_adr_13G_X4;
            plldiv = FV_div52;
            break;
        case 15000:
            aspeed = FV_adr_15G_X4;
            plldiv = FV_div60;
            break;
        case 16000:
            aspeed = FV_adr_16G_X4;
            plldiv = FV_div64;
            break;
        case 20000:
            aspeed = FV_adr_20G_X4;
            plldiv = FV_div40;
            break;
        case 21000:
            aspeed = FV_adr_21G_X4;
            plldiv = FV_div42;
            break;
        case 25000:
            aspeed = FV_adr_25p45G_X4;
            plldiv = FV_div42;
            break;
        case 30000:
            aspeed = FV_adr_31p5G_X4;
            plldiv = FV_div52;
            break;
        case 40000:
            txdrv = FV_txdrv_XLAUI;
            if (PHY_CTRL_LINE_INTF(pc) == PHY_IF_HIGIG) {
                if (PHY_CTRL_LINE_INTF(pc) == PHY_IF_KR) {
                    aspeed = FV_adr_40G_KR4;
                } else {
                    aspeed = FV_adr_40G_X4;
                    fw_mode = 3;
                }
            } else {
                if (PHY_CTRL_LINE_INTF(pc) == PHY_IF_CR) {
                    aspeed = FV_adr_40G_CR4;
                } else {
                    if (PHY_CTRL_LINE_INTF(pc) == PHY_IF_XLAUI) {
                        fw_mode = 3;
                    } else if (PHY_CTRL_LINE_INTF(pc) == PHY_IF_SR) {
                        fw_mode = 1;
                        txdrv = FV_txdrv_SR4;
                    }
                    aspeed = FV_adr_40G_KR4;
                }
            }
            break;
        case 42000:
            txdrv = FV_txdrv_XLAUI;
            if (PHY_CTRL_LINE_INTF(pc) == PHY_IF_HIGIG) {
                if (PHY_CTRL_LINE_INTF(pc) == PHY_IF_KR) {
                    aspeed = FV_adr_40G_KR4;
                } else {
                    aspeed = FV_adr_40G_X4;
                }
            } else {
                if (PHY_CTRL_LINE_INTF(pc) == PHY_IF_CR) {
                    aspeed = FV_adr_40G_CR4;
                } else {
                    aspeed = FV_adr_40G_KR4;
                }
            }
            plldiv = FV_div70;
            fw_mode = 3;
            break;
        default:
            return CDK_E_PARAM;
        }
    } else if (IS_2LANE_PORT(pc)){
        switch (speed) {
        case 10000:
            aspeed = FV_adr_10G_CX2_NOSCRAMBLE;
            plldiv = FV_div40;
            break;
        case 11000:
            plldiv = FV_div70;
            if (PHY_CTRL_LINE_INTF(pc) == PHY_IF_KR) {
                aspeed = FV_adr_10G_KR1;
                txdrv = FV_txdrv_XLAUI;
            } else if (PHY_CTRL_LINE_INTF(pc) == PHY_IF_SFI) {
                aspeed = FV_adr_10G_SFI;
                txdrv = FV_txdrv_SFI;
            } else if (PHY_CTRL_LINE_INTF(pc) == PHY_IF_CR) {
                aspeed = FV_adr_10G_SFI;
                txdrv = FV_txdrv_SFIDAC;
                fw_mode = 2;
            } else {
                aspeed = FV_adr_10G_X1;
            }
            break;
        case 12700:
        case 13000:
            aspeed = FV_adr_12p7G_X2;
            plldiv = FV_div42;
            break;
        case 20000:
            if (ACTIVE_INTERFACE_GET(pc) == PHY_IF_CR) {
                aspeed = FV_adr_20G_CR2;
            } else if (ACTIVE_INTERFACE_GET(pc) == PHY_IF_KR) {
                aspeed = FV_adr_20G_KR2;
            } else {
                aspeed = FV_adr_20G_CX2;
            }
            break;
        case 21000:
            aspeed = FV_adr_20G_CX2;
            plldiv = FV_div70;
            break;
        default:
            return CDK_E_PARAM;
        }
    } else {
        switch (speed) {
        case 10:
            aspeed = FV_adr_10M;
            if ((cur_plldiv == FV_div40) || (cur_plldiv == FV_div66)) {
                plldiv = cur_plldiv;
            } else {
                plldiv = FV_div40;
            }
            break;
        case 100:
            aspeed = FV_adr_100M;
            if ((cur_plldiv == FV_div40) || (cur_plldiv == FV_div66)) {
                plldiv = cur_plldiv;
            } else {
                plldiv = FV_div40;
            }
            break;
        case 1000:
            aspeed = FV_adr_1000M;
            if ((cur_plldiv == FV_div40) || (cur_plldiv == FV_div66)) {
                plldiv = cur_plldiv;
            } else {
                plldiv = FV_div40;
            }
            break;
        case 2500:
            aspeed = FV_adr_2p5G_X1;
            plldiv = FV_div40;
            break;
        case 5000:
            aspeed = FV_adr_5G_X1;
            break;
        case 10000:
            if (PHY_CTRL_LINE_INTF(pc) == PHY_IF_KR) {
                aspeed = FV_adr_10G_KR1;
                txdrv = FV_txdrv_XLAUI;
            } else if (PHY_CTRL_LINE_INTF(pc) == PHY_IF_SFI) {
                aspeed = FV_adr_10G_SFI;
                txdrv = FV_txdrv_SFI;
            } else if (PHY_CTRL_LINE_INTF(pc) == PHY_IF_CR) {
                aspeed = FV_adr_10G_SFI;
                txdrv = FV_txdrv_SFIDAC;
                fw_mode = 2;
            } else {
                aspeed = FV_adr_10G_X1;
            }
            break;
        case 11000:
            plldiv = FV_div70;
            if (PHY_CTRL_LINE_INTF(pc) == PHY_IF_KR) {
                aspeed = FV_adr_10G_KR1;
                txdrv = FV_txdrv_XLAUI;
            } else if (PHY_CTRL_LINE_INTF(pc) == PHY_IF_SFI) {
                aspeed = FV_adr_10G_SFI;
                txdrv = FV_txdrv_SFI;
            } else if (PHY_CTRL_LINE_INTF(pc) == PHY_IF_CR) {
                aspeed = FV_adr_10G_SFI;
                txdrv = FV_txdrv_SFIDAC;
                fw_mode = 2;
            } else {
                aspeed = FV_adr_10G_X1;
            }
            break;
        default:
            return CDK_E_PARAM;
        }
    }

    rv += PHY_NOTIFY(pc, PhyEvent_PhyDisable);

    ioerr += READ_RX_X4_RX_SIGDETr(pc, &rx_x4_rx_sigdet);
    RX_X4_RX_SIGDETr_OVERRIDE_SIGNAL_OKf_SET(rx_x4_rx_sigdet, 1);
    RX_X4_RX_SIGDETr_OVERRIDE_VALUE_SIGNAL_OKf_SET(rx_x4_rx_sigdet, 0);
    ioerr += WRITE_RX_X4_RX_SIGDETr(pc, rx_x4_rx_sigdet);

    /* Gracefully stop uController */
    if (_tsc_revid_get(pc) < PHY_REVID_A2) {
        rv += _tsc_uc_cmd_seq(pc, 0);
    }

    ioerr += READ_DSC_MISC_CTRL0r(pc, &dsc_misc_ctrl0);
    DSC_MISC_CTRL0r_RXSEQSTART_AN_DISABLEf_SET(dsc_misc_ctrl0, 1);
    ioerr += WRITE_DSC_MISC_CTRL0r(pc, dsc_misc_ctrl0);

    if (plldiv != cur_plldiv) {
        ioerr += READ_RESETr(pc, &reset);
        data = RESETr_RX_RESET_ANALOGf_GET(reset);
        data |= lane_mask;
        RESETr_RX_RESET_ANALOGf_SET(reset, data);
        data = RESETr_TX_RESET_ANALOGf_GET(reset);
        data |= lane_mask;
        RESETr_TX_RESET_ANALOGf_SET(reset, data);
        ioerr += WRITE_RESETr(pc, reset);

        ioerr += READ_SETUPr(pc, &setup);
        SETUPr_START_SEQUENCERf_SET(setup, 0);
        ioerr += WRITE_SETUPr(pc, setup);

        ioerr += READ_SETUPr(pc, &setup);
        SETUPr_FORCE_PLL_MODE_AFE_SELf_SET(setup, 1);
        SETUPr_DEFAULT_PLL_MODE_AFEf_SET(setup, plldiv);
        ioerr += WRITE_SETUPr(pc, setup);

        PHY_SYS_USLEEP(1000);

        TX_X4_CREDIT0r_SET(tx_x4_credit0, 0);
        ioerr += WRITE_TX_X4_CREDIT0r(pc, tx_x4_credit0);
        TX_X4_PCS_CLOCKCNT0r_SET(tx_x4_pcs_clockcnt0, 0);
        ioerr += WRITE_TX_X4_PCS_CLOCKCNT0r(pc, tx_x4_pcs_clockcnt0);

        ioerr += READ_SETUPr(pc, &setup);
        SETUPr_START_SEQUENCERf_SET(setup, 1);
        ioerr += WRITE_SETUPr(pc, setup);

        (void)_tsc_pll_lock_wait(pc);

        ioerr += READ_RESETr(pc, &reset);
        data = RESETr_RX_RESET_ANALOGf_GET(reset);
        data &= ~lane_mask;
        RESETr_RX_RESET_ANALOGf_SET(reset, data);
        data = RESETr_TX_RESET_ANALOGf_GET(reset);
        data &= ~lane_mask;
        RESETr_TX_RESET_ANALOGf_SET(reset, data);
        ioerr += WRITE_RESETr(pc, reset);

        /* 1 ms delay to wait for Analog to settle */
        PHY_SYS_USLEEP(1000);
    }

    /* Setting actual speed */
    ioerr += READ_CL72_MISC2_CONTROLr(pc, &misc2_ctrl);
    CL72_MISC2_CONTROLr_SW_ACTUAL_SPEEDf_SET(misc2_ctrl, aspeed);
    ioerr += WRITE_CL72_MISC2_CONTROLr(pc, misc2_ctrl);

    /* Setting firmware mode */
    ioerr += READ_FIRMWARE_MODEr(pc, &firmware_mode);
    FIRMWARE_MODEr_FIRMWARE_MODEf_SET(firmware_mode, fw_mode);
    ioerr += WRITE_FIRMWARE_MODEr(pc, firmware_mode);

    rv += _tsc_credit_set(pc, 1);
    rv += _tsc_encode_set(pc, 1);
    rv += _tsc_decode_set(pc, 1);
    rv += _tsc_txdrv_set(pc, txdrv);

    ioerr += READ_CONTROLSr(pc, &controls);
    CONTROLSr_PD_KX4_ENf_SET(controls, 0);
    CONTROLSr_PD_KX_ENf_SET(controls, 0);
    ioerr += WRITE_CONTROLSr(pc, controls);

    ioerr += READ_LINK_FAIL_INHIBIT_TIMER_CL72r(pc, &link_fail_inhibit_timer_cl72);
    LINK_FAIL_INHIBIT_TIMER_CL72r_LINK_FAIL_INHIBIT_TIMER_CL72_PERIODf_SET(link_fail_inhibit_timer_cl72, 0x8236);
    ioerr += WRITE_LINK_FAIL_INHIBIT_TIMER_CL72r(pc, link_fail_inhibit_timer_cl72);
    
    CL72_MAX_WAIT_TIMERr_SET(cl72_max_wait_timer, 0x8236);
    ioerr += WRITE_CL72_MAX_WAIT_TIMERr(pc, cl72_max_wait_timer);

    /* Gracefully resume uController */
    if (_tsc_revid_get(pc) < PHY_REVID_A2) {
        rv += _tsc_uc_cmd_seq(pc, 2);
    }

    ioerr += READ_RX_X4_RX_SIGDETr(pc, &rx_x4_rx_sigdet);
    RX_X4_RX_SIGDETr_OVERRIDE_SIGNAL_OKf_SET(rx_x4_rx_sigdet, 0);
    RX_X4_RX_SIGDETr_OVERRIDE_VALUE_SIGNAL_OKf_SET(rx_x4_rx_sigdet, 0);
    ioerr += WRITE_RX_X4_RX_SIGDETr(pc, rx_x4_rx_sigdet);
    PHY_SYS_USLEEP(1000);

    ioerr += READ_PMD_10GBASE_KR_PMD_CONTROL_150r(pc, &pmd_10gbase_kr_ctrl_150);
    PMD_10GBASE_KR_PMD_CONTROL_150r_RESTART_TRAININGf_SET(pmd_10gbase_kr_ctrl_150, 0);
    PMD_10GBASE_KR_PMD_CONTROL_150r_TRAINING_ENABLEf_SET(pmd_10gbase_kr_ctrl_150, 0);
    ioerr += WRITE_PMD_10GBASE_KR_PMD_CONTROL_150r(pc, pmd_10gbase_kr_ctrl_150);

    rv += PHY_NOTIFY(pc, PhyEvent_PhyEnable);

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:    
 *      bcmi_tsc_xgxs_speed_get
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
bcmi_tsc_xgxs_speed_get(phy_ctrl_t *pc, uint32_t *speed)
{
    int ioerr = 0;
    int rv;
    AN_ABIL_RESOLUTION_STATUSr_t an_res;
    CL72_MISC2_CONTROLr_t misc2_ctrl;
    int autoneg;
    uint32_t plldiv, sp_val;

    PHY_CTRL_CHECK(pc);

    rv = PHY_AUTONEG_GET(pc, &autoneg);
    if (CDK_FAILURE(rv)) {
        return rv;
    }

    *speed = 0;

    if (autoneg) {
        ioerr += READ_AN_ABIL_RESOLUTION_STATUSr(pc, &an_res);
        sp_val = AN_ABIL_RESOLUTION_STATUSr_AN_HCD_SPEEDf_GET(an_res);
    } else {
        ioerr += READ_CL72_MISC2_CONTROLr(pc, &misc2_ctrl);
        sp_val = CL72_MISC2_CONTROLr_SW_ACTUAL_SPEEDf_GET(misc2_ctrl);
    }

    rv = _tsc_plldiv_get(pc, &plldiv);
    if (CDK_FAILURE(rv)) {
        return rv;
    }

    switch (sp_val) {
    case FV_adr_10M:
        *speed = 10;
        break;
    case FV_adr_100M:
        *speed = 100;
        break;
    case FV_adr_1000M:
        *speed = 1000;
        break;
    case FV_adr_2p5G_X1:
        *speed = 2500;
        break;
    case FV_adr_5G_X4:
        *speed = 5000;
        break;
    case FV_adr_6G_X4:
        *speed = 6000;
        break;
    case FV_adr_10G_X4:
        if (plldiv == FV_div42) {
            *speed = 11000;
        } else {
            *speed = 10000;
        }
        ACTIVE_INTERFACE_SET(pc, PHY_IF_HIGIG);
        break;
    case FV_adr_10G_CX4:
        if (plldiv == FV_div42) {
            *speed = 11000;
        } else {
            *speed = 10000;
        }
        break;
    case FV_adr_12G_X4:
        *speed = 12000;
        break;
    case FV_adr_12p5G_X4:
        *speed = 12500;
        break;
    case FV_adr_13G_X4:
        *speed = 13000;
        break;
    case FV_adr_15G_X4:
        *speed = 15000;
        break;
    case FV_adr_16G_X4:
        *speed = 16000;
        break;
    case FV_adr_1G_KX1:
        *speed = 1000;
        break;
    case FV_adr_10G_KX4:
        *speed = 10000;
        break;
    case FV_adr_10G_KR1:
        *speed = 10000;
        ACTIVE_INTERFACE_SET(pc, PHY_IF_KR);
        break;
    case FV_adr_5G_X1:
        *speed = 5000;
        break;
    case FV_adr_6p36G_X1:
        *speed = 6360;
        break;
    case FV_adr_20G_CX4:
        *speed = 20000;
        break;
    case FV_adr_21G_X4:
        *speed = 21000;
        break;
    case FV_adr_25p45G_X4:
        *speed = 25450;
        break;
    case FV_adr_10G_X2_NOSCRAMBLE:
        *speed = 10000;
        break;
    case FV_adr_10G_CX2_NOSCRAMBLE:
        *speed = 10000;
        break;
    case FV_adr_10p5G_X2:
        *speed = 10500;
        break;
    case FV_adr_10p5G_CX2_NOSCRAMBLE:
        *speed = 10500;
        break;
    case FV_adr_12p7G_X2:
        *speed = 12700;
        *speed = 13000;
        break;
    case FV_adr_12p7G_CX2:
        *speed = 12700;
        break;
    case FV_adr_10G_X1:
        if (plldiv == FV_div70) {
            *speed = 11000;
        } else {
            *speed = 10000;
        }
        break;
    case FV_adr_40G_X4:
        *speed = 40000;
        break;
    case FV_adr_20G_X2:
        *speed = 20000;
        break;
    case FV_adr_20G_CX2:
        *speed = 20000;
        break;
    case FV_adr_10G_SFI:
        *speed = 10000;
        ACTIVE_INTERFACE_SET(pc, PHY_IF_XFI);
        break;
    case FV_adr_31p5G_X4:
        *speed = 31500;
        break;
    case FV_adr_32p7G_X4:
        *speed = 32700;
        break;
    case FV_adr_20G_X4:
        *speed = 20000;
        break;
    case FV_adr_10G_X2:
        *speed = 10000;
        break;
    case FV_adr_10G_CX2:
        *speed = 10000;
        break;
    case FV_adr_12G_SCO_R2:
        *speed = 12000;
        break;
    case FV_adr_10G_SCO_X2:
        *speed = 10000;
        break;
    case FV_adr_40G_KR4:
        *speed = 40000;
        ACTIVE_INTERFACE_SET(pc, PHY_IF_KR);
        break;
    case FV_adr_40G_CR4:
        *speed = 40000;
        ACTIVE_INTERFACE_SET(pc, PHY_IF_CR);
        break;
    case FV_adr_100G_CR10:
        *speed = 100000;
        ACTIVE_INTERFACE_SET(pc, PHY_IF_CR);
        break;
    case FV_adr_5G_X2:
        *speed = 5000;
        break;
    case FV_adr_15p75G_X2:
        *speed = 15750;
        break;
    case FV_adr_2G_FC:
        *speed = 2000;
        break;
    case FV_adr_4G_FC: 
        *speed = 4000;
        break;
    case FV_adr_8G_FC:
        *speed = 8000;
        break;
    case FV_adr_10G_CX1:
        *speed = 10000;
        break;
    case FV_adr_1G_CX1: 
        *speed = 1000;
        break;
    case FV_adr_20G_KR2:
        *speed = 20000;
        ACTIVE_INTERFACE_SET(pc, PHY_IF_KR);
        break;
    case FV_adr_20G_CR2:  
        *speed = 20000;
        ACTIVE_INTERFACE_SET(pc, PHY_IF_CR);
        break;
    default:
        break;
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

/*
 * Function:    
 *      bcmi_tsc_xgxs_autoneg_set
 * Purpose:     
 *      Enable or disabled auto-negotiation on the specified port.
 * Parameters:
 *      pc - PHY control structure
 *      autoneg - non-zero enables autoneg, zero disables autoneg
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcmi_tsc_xgxs_autoneg_set(phy_ctrl_t *pc, int autoneg)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int cl73_an, cl37_an, an_cl72, an_type;
    uint32_t plldiv, pll_target = FV_div66;
    int lane_num, lane_mask, cur_an, txdrv;
    uint32_t data;
    RESETr_t reset;
    SETUPr_t setup;
    TX_X4_CREDIT0r_t tx_x4_credit0;
    RX_X4_RX_SIGDETr_t rx_x4_rx_sigdet;
    DSC_MISC_CTRL0r_t dsc_misc_ctrl0;
    RX_X4_DECODE_CONTROL_0r_t rx_x4_dec_ctrl_0;
    TX_X4_PCS_CLOCKCNT0r_t tx_x4_pcs_clk_cnt0;
    FIRMWARE_MODEr_t firmware_mode;
    CL72_MISC1_CONTROLr_t cl72_misc1_ctrl;
    PMD_10GBASE_KR_PMD_CONTROL_150r_t pmd_10gbase_kr_ctrl_150;
    CL72_TX_FIR_TAPr_t cl72_tx_fir_tap;
    TX_X2_BRCM_MODEr_t tx_x2_brcm_mode;
    CONTROLSr_t controls;
    ENABLESr_t enables;
    LINK_FAIL_INHIBIT_TIMER_NOT_CL72r_t link_fail_inhibit_timer_not_cl72;
    RX_X1_DESKEW_WINDOWSr_t rx_x1_deskew_windows;
    TX_X2_MLD_SWAP_COUNTr_t tx_x2_mld_swap_count;
    CL82_RX_AM_TIMERr_t cl82_rx_am_timer;
    CL72_XMT_CONTROLr_t cl72_xmt_ctrl;
    COMMAND3r_t command3;
    COMMANDr_t command;
    CL73_ERRORr_t cl73_error;

    PHY_CTRL_CHECK(pc);

    _PHY_DBG(pc, ("Auto-negotiation Set: autoneg = %d\n", autoneg));

    if (autoneg) {
        autoneg = 1;
    }

    /* In passthru mode we always disable autoneg */
    if ((PHY_CTRL_FLAGS(pc) & PHY_F_PASSTHRU)) {
        autoneg = 0;
    }

    rv = PHY_AUTONEG_GET(pc, &cur_an);
    if (CDK_FAILURE(rv)) {
        return rv;
    }
    /* Do not set same auto-negotiation status */
    if (autoneg == cur_an) {
        return CDK_E_NONE;
    }

    lane_num = _tsc_serdes_lane(pc);
    lane_mask = _tsc_serdes_lane_mask(pc);
    if ((lane_num < 0) || (lane_mask < 0)) {
        return CDK_E_IO;
    }

    if (PHY_CTRL_LINE_INTF(pc) == PHY_IF_HIGIG) {
        cl73_an = 0;
    } else {
        cl73_an = 2;
    }

    if (PHY_CTRL_FLAGS(pc) & PHY_F_CLAUSE37) {
        cl37_an = FV_an_CL37;
    } else {
        cl37_an = 0;
    }
    if (PHY_CTRL_FLAGS(pc) & PHY_F_CLAUSE73) {
        cl73_an = FV_an_CL73;
    } else {
        cl73_an = 0;
    }

    an_cl72 = 1;     /* Always 1 */
    an_type = FV_an_NONE;

    rv += PHY_NOTIFY(pc, PhyEvent_PhyDisable);

    if (autoneg) {
        rv += _tsc_plldiv_get(pc, &plldiv);

        if (cl73_an) {
            an_type = FV_an_CL73;
        } else if (cl37_an) {
            pll_target = FV_div40;
            an_type = FV_an_CL37;
        } else {
            pll_target = FV_div40;
            if (PHY_CTRL_LINE_INTF(pc) == PHY_IF_HIGIG) {
                an_type = FV_an_CL73_BAM;
            } else {
                if (PHY_CTRL_FLAGS(pc) & PHY_F_FIBER_MODE) {
                    if (plldiv == FV_div66) {
                        an_type = FV_an_CL37_10G;
                        pll_target = FV_div66;
                    } else {
                        an_type = FV_an_CL37;
                    }
                } else {
                    an_type = FV_an_CL37_SGMII;
                }
            }
        }

        if (!IS_4LANE_PORT(pc)) {
            ioerr += READ_SETUPr(pc, &setup);
            SETUPr_FORCE_PLL_MODE_AFE_SELf_SET(setup, 0);
            SETUPr_SINGLE_PORT_MODEf_SET(setup, 0);
            ioerr += WRITE_SETUPr(pc, setup);
        }

        rv += _tsc_uc_fw_control(pc, FW_CTRL_CL72_SWITCH_OVER, 0);

        ioerr += READ_RESETr(pc, &reset);
        data = RESETr_RX_RESET_ANALOGf_GET(reset);
        data |= lane_mask;
        RESETr_RX_RESET_ANALOGf_SET(reset, data);
        data = RESETr_TX_RESET_ANALOGf_GET(reset);
        data |= lane_mask;
        RESETr_TX_RESET_ANALOGf_SET(reset, data);
        ioerr += WRITE_RESETr(pc, reset);

        PHY_SYS_USLEEP(1000);

        if (plldiv != pll_target)
        {
            ioerr += READ_SETUPr(pc, &setup);
            SETUPr_START_SEQUENCERf_SET(setup, 0);
            ioerr += WRITE_SETUPr(pc, setup);

            PHY_SYS_USLEEP(2000);

            ioerr += READ_SETUPr(pc, &setup);
            SETUPr_REFCLK_SELf_SET(setup, FV_refclk_156p25MHz);
            SETUPr_FORCE_PLL_MODE_AFE_SELf_SET(setup, 0);
            if ((an_type == FV_an_CL73) || (an_type == FV_an_HPAM)||
                (an_type == FV_an_CL73_BAM) || (an_type == FV_an_CL37_10G)) {
                SETUPr_DEFAULT_PLL_MODE_AFEf_SET(setup, FV_div66);
            } else {
                SETUPr_DEFAULT_PLL_MODE_AFEf_SET(setup, FV_div40);
            }

            if (IS_4LANE_PORT(pc)) {
                SETUPr_PORT_MODE_SELf_SET(setup, FV_pm_1port_3210);
                SETUPr_SINGLE_PORT_MODEf_SET(setup, 1);
            } else if(IS_2LANE_PORT(pc)) {
                SETUPr_PORT_MODE_SELf_SET(setup, FV_pm_2port_32_10);
                SETUPr_SINGLE_PORT_MODEf_SET(setup, 0);
            } else {
                SETUPr_PORT_MODE_SELf_SET(setup, FV_pm_4port_3_2_1_0);
                SETUPr_SINGLE_PORT_MODEf_SET(setup, 0);
            }
            ioerr += WRITE_SETUPr(pc, setup);

            ioerr += READ_RX_X1_DESKEW_WINDOWSr(pc, &rx_x1_deskew_windows);
            RX_X1_DESKEW_WINDOWSr_CL82_DSWINf_SET(rx_x1_deskew_windows, 0x1e);
            RX_X1_DESKEW_WINDOWSr_CL48_DSWIN64B66Bf_SET(rx_x1_deskew_windows, 1);
            RX_X1_DESKEW_WINDOWSr_CL48_DSWIN8B10Bf_SET(rx_x1_deskew_windows, 7);
            ioerr += WRITE_RX_X1_DESKEW_WINDOWSr(pc, rx_x1_deskew_windows);

            /* MLD swap_count */
            ioerr += READ_TX_X2_MLD_SWAP_COUNTr(pc, &tx_x2_mld_swap_count);
            TX_X2_MLD_SWAP_COUNTr_MLD_SWAP_COUNTf_SET(tx_x2_mld_swap_count, 0xfffc);
            ioerr += WRITE_TX_X2_MLD_SWAP_COUNTr(pc, tx_x2_mld_swap_count);

            /* CL82 shared rc_am_timer */
            ioerr += READ_CL82_RX_AM_TIMERr(pc, &cl82_rx_am_timer);
            CL82_RX_AM_TIMERr_AM_TIMER_INIT_RX_VALf_SET(cl82_rx_am_timer, 0x3fff);
            ioerr += WRITE_CL82_RX_AM_TIMERr(pc, cl82_rx_am_timer);

            /* Disable ECC of uController */
            COMMAND3r_CLR(command3);
            COMMAND3r_DISABLE_ECCf_SET(command3, 1);
            COMMAND3r_OUT_STAGING_FLOP_BYPASSf_SET(command3, 1);
            COMMAND3r_IN_STAGING_FLOP_BYPASSf_SET(command3, 1);
            ioerr += WRITE_COMMAND3r(pc, command3);

            /* Enable uController */
            ioerr += READ_COMMANDr(pc, &command);
            COMMANDr_MDIO_UC_RESET_Nf_SET(command, 1);
            ioerr += WRITE_COMMANDr(pc, command);

            /* Clear credits */
            TX_X4_CREDIT0r_CLR(tx_x4_credit0);
            ioerr += WRITE_TX_X4_CREDIT0r(pc, tx_x4_credit0);
            TX_X4_PCS_CLOCKCNT0r_CLR(tx_x4_pcs_clk_cnt0);
            ioerr += WRITE_TX_X4_PCS_CLOCKCNT0r(pc, tx_x4_pcs_clk_cnt0);
        }
        PHY_SYS_USLEEP(71000);

        ioerr += READ_SETUPr(pc, &setup);
        SETUPr_START_SEQUENCERf_SET(setup, 1);
        ioerr += WRITE_SETUPr(pc, setup);

        (void)_tsc_pll_lock_wait(pc);
        
        txdrv = FV_txdrv_AN;
        rv += _tsc_txdrv_set(pc, txdrv);

        ioerr += READ_RESETr(pc, &reset);
        data = RESETr_RX_RESET_ANALOGf_GET(reset);
        data &= ~lane_mask;
        RESETr_RX_RESET_ANALOGf_SET(reset, data);
        data = RESETr_TX_RESET_ANALOGf_GET(reset);
        data &= ~lane_mask;
        RESETr_TX_RESET_ANALOGf_SET(reset, data);
        ioerr += WRITE_RESETr(pc, reset);

        PHY_SYS_USLEEP(1000);

        ioerr += READ_RX_X4_RX_SIGDETr(pc, &rx_x4_rx_sigdet);
        RX_X4_RX_SIGDETr_OVERRIDE_SIGNAL_OKf_SET(rx_x4_rx_sigdet, 1);
        RX_X4_RX_SIGDETr_OVERRIDE_VALUE_SIGNAL_OKf_SET(rx_x4_rx_sigdet, 0);
        ioerr += WRITE_RX_X4_RX_SIGDETr(pc, rx_x4_rx_sigdet);

        /* Gracefully stop uController */
        if (_tsc_revid_get(pc) < PHY_REVID_A2) {
            rv += _tsc_uc_cmd_seq(pc, 0);
        } else {
            /* Disable AN */
            ioerr += READLN_ENABLESr(pc, lane_num, &enables);
            ENABLESr_CL37_BAM_ENABLEf_SET(enables, 0);
            ENABLESr_CL73_BAM_ENABLEf_SET(enables, 0);
            ENABLESr_CL73_HPAM_ENABLEf_SET(enables, 0);
            ENABLESr_CL73_ENABLEf_SET(enables, 0);
            ENABLESr_CL37_SGMII_ENABLEf_SET(enables, 0);
            ENABLESr_CL37_ENABLEf_SET(enables, 0);
            ioerr += WRITELN_ENABLESr(pc, lane_num, enables);
        }

        ioerr += READ_DSC_MISC_CTRL0r(pc, &dsc_misc_ctrl0);
        DSC_MISC_CTRL0r_RXSEQSTART_AN_DISABLEf_SET(dsc_misc_ctrl0, 0);
        ioerr += WRITE_DSC_MISC_CTRL0r(pc, dsc_misc_ctrl0);

        /* For 10G pdetect */
        ioerr += READ_RX_X4_DECODE_CONTROL_0r(pc, &rx_x4_dec_ctrl_0);
        RX_X4_DECODE_CONTROL_0r_CL48_SYNCACQ_ENf_SET(rx_x4_dec_ctrl_0, 1);
        ioerr += WRITE_RX_X4_DECODE_CONTROL_0r(pc, rx_x4_dec_ctrl_0);

        rv += _tsc_uc_fw_control(pc, FW_CTRL_CL72_SWITCH_OVER, 0);

        /* CLAUSE_72_CONTROL */
        ioerr += READ_PMD_10GBASE_KR_PMD_CONTROL_150r(pc, &pmd_10gbase_kr_ctrl_150);
        PMD_10GBASE_KR_PMD_CONTROL_150r_RESTART_TRAININGf_SET(pmd_10gbase_kr_ctrl_150, 0);
        PMD_10GBASE_KR_PMD_CONTROL_150r_TRAINING_ENABLEf_SET(pmd_10gbase_kr_ctrl_150, 0);
        ioerr += WRITE_PMD_10GBASE_KR_PMD_CONTROL_150r(pc, pmd_10gbase_kr_ctrl_150);

        ioerr += READ_CL72_MISC1_CONTROLr(pc, &cl72_misc1_ctrl);
        CL72_MISC1_CONTROLr_TR_COARSE_LOCKf_SET(cl72_misc1_ctrl, 0);
        CL72_MISC1_CONTROLr_RX_TRAINEDf_SET(cl72_misc1_ctrl, 0);
        ioerr += WRITE_CL72_MISC1_CONTROLr(pc, cl72_misc1_ctrl);

        CL72_XMT_CONTROLr_CLR(cl72_xmt_ctrl);
        ioerr += WRITE_CL72_XMT_CONTROLr(pc, cl72_xmt_ctrl);

        ioerr += READ_PMD_10GBASE_KR_PMD_CONTROL_150r(pc, &pmd_10gbase_kr_ctrl_150);
        PMD_10GBASE_KR_PMD_CONTROL_150r_RESTART_TRAININGf_SET(pmd_10gbase_kr_ctrl_150, 1);
        ioerr += WRITE_PMD_10GBASE_KR_PMD_CONTROL_150r(pc, pmd_10gbase_kr_ctrl_150);

        /* SOFT_RESET */
        LINK_FAIL_INHIBIT_TIMER_NOT_CL72r_CLR(link_fail_inhibit_timer_not_cl72);
        ioerr += WRITE_LINK_FAIL_INHIBIT_TIMER_NOT_CL72r(pc, link_fail_inhibit_timer_not_cl72);

        if (autoneg && an_cl72) {
            LINK_FAIL_INHIBIT_TIMER_NOT_CL72r_SET(link_fail_inhibit_timer_not_cl72, 0x8236);
            ioerr += WRITE_LINK_FAIL_INHIBIT_TIMER_NOT_CL72r(pc, link_fail_inhibit_timer_not_cl72);
        } else {
            LINK_FAIL_INHIBIT_TIMER_NOT_CL72r_SET(link_fail_inhibit_timer_not_cl72, 0x1a10);
            ioerr += WRITE_LINK_FAIL_INHIBIT_TIMER_NOT_CL72r(pc, link_fail_inhibit_timer_not_cl72);
        }

        FIRMWARE_MODEr_SET(firmware_mode, 0);
        ioerr += WRITE_FIRMWARE_MODEr(pc, firmware_mode);

        ioerr += READ_DSC_MISC_CTRL0r(pc, &dsc_misc_ctrl0);
        DSC_MISC_CTRL0r_RXSEQSTARTf_SET(dsc_misc_ctrl0, 1);
        ioerr += WRITEALL_DSC_MISC_CTRL0r(pc, dsc_misc_ctrl0);

        ioerr += READ_CL72_MISC1_CONTROLr(pc, &cl72_misc1_ctrl);
        CL72_MISC1_CONTROLr_TAP_DEFAULT_MUXSEL_FORCEVALf_SET(cl72_misc1_ctrl, 0);
        CL72_MISC1_CONTROLr_TAP_DEFAULT_MUXSEL_FORCEf_SET(cl72_misc1_ctrl, 0);
        ioerr += WRITE_CL72_MISC1_CONTROLr(pc, cl72_misc1_ctrl);

        ioerr += READ_PMD_10GBASE_KR_PMD_CONTROL_150r(pc, &pmd_10gbase_kr_ctrl_150);
        PMD_10GBASE_KR_PMD_CONTROL_150r_RESTART_TRAININGf_SET(pmd_10gbase_kr_ctrl_150, 0);
        PMD_10GBASE_KR_PMD_CONTROL_150r_TRAINING_ENABLEf_SET(pmd_10gbase_kr_ctrl_150, 0);
        ioerr += WRITE_PMD_10GBASE_KR_PMD_CONTROL_150r(pc, pmd_10gbase_kr_ctrl_150);

        if ((an_type == FV_an_CL73) || 
            (an_type == FV_an_CL73_BAM) || 
            (an_type == FV_an_HPAM)) {
            if (an_cl72) {
                ioerr += READ_CL72_TX_FIR_TAPr(pc, &cl72_tx_fir_tap);
                CL72_TX_FIR_TAPr_TX_FIR_TAP_FORCEf_SET(cl72_tx_fir_tap, 0);
                ioerr += WRITE_CL72_TX_FIR_TAPr(pc, cl72_tx_fir_tap);

                ioerr += READ_CL72_MISC1_CONTROLr(pc, &cl72_misc1_ctrl);
                CL72_MISC1_CONTROLr_LINK_CONTROL_FORCEf_SET(cl72_misc1_ctrl, 0);
                CL72_MISC1_CONTROLr_LINK_CONTROL_FORCEVALf_SET(cl72_misc1_ctrl, 0);
                ioerr += WRITE_CL72_MISC1_CONTROLr(pc, cl72_misc1_ctrl);
            } else {
                ioerr += READ_CL72_MISC1_CONTROLr(pc, &cl72_misc1_ctrl);
                CL72_MISC1_CONTROLr_LINK_CONTROL_FORCEf_SET(cl72_misc1_ctrl, 1);
                CL72_MISC1_CONTROLr_LINK_CONTROL_FORCEVALf_SET(cl72_misc1_ctrl, 0);
                ioerr += WRITE_CL72_MISC1_CONTROLr(pc, cl72_misc1_ctrl);
            }

            ioerr += READ_TX_X2_BRCM_MODEr(pc, &tx_x2_brcm_mode);
            TX_X2_BRCM_MODEr_ACOL_SWAP_COUNT64B66Bf_SET(tx_x2_brcm_mode, 0x2);
            ioerr += WRITE_TX_X2_BRCM_MODEr(pc, tx_x2_brcm_mode);
        } else if (an_type == FV_an_CL37_BAM) {
            if (an_cl72) {
                ioerr += READ_CL72_TX_FIR_TAPr(pc, &cl72_tx_fir_tap);
                CL72_TX_FIR_TAPr_TX_FIR_TAP_FORCEf_SET(cl72_tx_fir_tap, 0);
                ioerr += WRITE_CL72_TX_FIR_TAPr(pc, cl72_tx_fir_tap);

                ioerr += READ_CL72_MISC1_CONTROLr(pc, &cl72_misc1_ctrl);
                CL72_MISC1_CONTROLr_LINK_CONTROL_FORCEf_SET(cl72_misc1_ctrl, 0);
                CL72_MISC1_CONTROLr_LINK_CONTROL_FORCEVALf_SET(cl72_misc1_ctrl, 0);
                ioerr += WRITE_CL72_MISC1_CONTROLr(pc, cl72_misc1_ctrl);
            } else {
                ioerr += READ_CL72_MISC1_CONTROLr(pc, &cl72_misc1_ctrl);
                CL72_MISC1_CONTROLr_LINK_CONTROL_FORCEf_SET(cl72_misc1_ctrl, 0);
                CL72_MISC1_CONTROLr_LINK_CONTROL_FORCEVALf_SET(cl72_misc1_ctrl, 0);
                ioerr += WRITE_CL72_MISC1_CONTROLr(pc, cl72_misc1_ctrl);
            }

            ioerr += READ_TX_X2_BRCM_MODEr(pc, &tx_x2_brcm_mode);
            TX_X2_BRCM_MODEr_ACOL_SWAP_COUNT64B66Bf_SET(tx_x2_brcm_mode, 0x66);
            ioerr += WRITE_TX_X2_BRCM_MODEr(pc, tx_x2_brcm_mode);
        }

        /* Gracefully resume uController */
        if (_tsc_revid_get(pc) < PHY_REVID_A2) {
            rv += _tsc_uc_cmd_seq(pc, 2);
        }

        ioerr += READ_RX_X4_RX_SIGDETr(pc, &rx_x4_rx_sigdet);
        RX_X4_RX_SIGDETr_OVERRIDE_VALUE_SIGNAL_OKf_SET(rx_x4_rx_sigdet, 0);
        RX_X4_RX_SIGDETr_OVERRIDE_SIGNAL_OKf_SET(rx_x4_rx_sigdet, 0);
        ioerr += WRITE_RX_X4_RX_SIGDETr(pc, rx_x4_rx_sigdet);

        PHY_SYS_USLEEP(1000);

        if (an_type == FV_an_HPAM) {
            if (cl73_an == 3) {
                data = 0xfff0;
            } else {
                data = 0x1a10;
            }
            ioerr += READ_CL73_ERRORr(pc, &cl73_error);
            CL73_ERRORr_CL73_ERROR_TIMER_PERIODf_SET(cl73_error, data);
            ioerr += WRITE_CL73_ERRORr(pc, cl73_error);
        } else {
            data = 0x1a10;
            ioerr += READ_CL73_ERRORr(pc, &cl73_error);
            CL73_ERRORr_CL73_ERROR_TIMER_PERIODf_SET(cl73_error, data);
            ioerr += WRITE_CL73_ERRORr(pc, cl73_error);
        }

        ioerr += READLN_CONTROLSr(pc, lane_num, &controls);
        if (IS_4LANE_PORT(pc)) {
            CONTROLSr_PD_KX4_ENf_SET(controls, 1);
        }
        CONTROLSr_PD_KX_ENf_SET(controls, 1);
        ioerr += WRITELN_CONTROLSr(pc, lane_num, controls);
    } else {
        ioerr += READ_LINK_FAIL_INHIBIT_TIMER_NOT_CL72r(pc, &link_fail_inhibit_timer_not_cl72);
        LINK_FAIL_INHIBIT_TIMER_NOT_CL72r_LINK_FAIL_INHIBIT_TIMER_NCL72_PERIODf_SET(link_fail_inhibit_timer_not_cl72, 0x1a10);
        ioerr += WRITE_LINK_FAIL_INHIBIT_TIMER_NOT_CL72r(pc, link_fail_inhibit_timer_not_cl72);
    }

    rv += _tsc_autoneg_control_set(pc, an_type, autoneg);
    
    rv += PHY_NOTIFY(pc, PhyEvent_PhyEnable);

    return ioerr ? CDK_E_IO : rv;
}

/*
 * Function:    
 *      bcmi_tsc_xgxs_autoneg_get
 * Purpose:     
 *      Get the current auto-negotiation status (enabled/busy)
 * Parameters:
 *      pc - PHY control structure
 *      autoneg - (OUT) non-zero indicates autoneg enabled
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcmi_tsc_xgxs_autoneg_get(phy_ctrl_t *pc, int *autoneg)
{
    int ioerr = 0;
    int an_cl73, an_cl37;
    ENABLESr_t enable;

    PHY_CTRL_CHECK(pc);

    *autoneg = 0;

    an_cl73 = 0; an_cl37 = 0;
    ioerr += READ_ENABLESr(pc, &enable);
    an_cl37 |= ENABLESr_CL37_BAM_ENABLEf_GET(enable);
    an_cl73 |= ENABLESr_CL73_BAM_ENABLEf_GET(enable);
    an_cl73 |= ENABLESr_CL73_HPAM_ENABLEf_GET(enable);
    an_cl73 |= ENABLESr_CL73_ENABLEf_GET(enable);
    an_cl37 |= ENABLESr_CL37_SGMII_ENABLEf_GET(enable);
    an_cl37 |= ENABLESr_CL37_ENABLEf_GET(enable);

    if (an_cl73 || an_cl37) {
        *autoneg = 1;
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE; 
}

/*
 * Function:    
 *      bcmi_tsc_xgxs_loopback_set
 * Purpose:     
 *      Set PHY loopback mode.
 * Parameters:
 *      pc - PHY control structure
 *      enable - non-zero enables PHY loopback
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcmi_tsc_xgxs_loopback_set(phy_ctrl_t *pc, int enable)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int cur_an, cur_lb, lane_mask;
    LOOPBACK_CONTROLr_t lb_ctrl;
    CL73_UCTRL1r_t cl73_uctrl1;

    PHY_CTRL_CHECK(pc);

    if (enable) {
        /* Used as field value, so cannot be any non-zero value */
        enable = 1;
    }

    rv = PHY_LOOPBACK_GET(pc, &cur_lb);
    if (CDK_FAILURE(rv)) {
        return rv;
    }

    if (cur_lb == enable) {
        return CDK_E_NONE;
    }

    lane_mask = _tsc_serdes_lane_mask(pc);
    if (lane_mask < 0) {
        return CDK_E_IO;
    }

    rv = PHY_NOTIFY(pc, PhyEvent_PhyDisable);
    if (CDK_FAILURE(rv)) {
        return rv;
    }

    ioerr += READ_LOOPBACK_CONTROLr(pc, &lb_ctrl);
    cur_lb = LOOPBACK_CONTROLr_LOCAL_PCS_LOOPBACK_ENABLEf_GET(lb_ctrl);
    if (enable) {
        cur_lb |=  lane_mask;
    } else {
        cur_lb &= ~lane_mask;
    }
    LOOPBACK_CONTROLr_LOCAL_PCS_LOOPBACK_ENABLEf_SET(lb_ctrl, cur_lb);
    ioerr += WRITE_LOOPBACK_CONTROLr(pc, lb_ctrl);

    rv = PHY_AUTONEG_GET(pc, &cur_an);
    if (CDK_SUCCESS(rv) && cur_an) {
        ioerr += READ_CL73_UCTRL1r(pc, &cl73_uctrl1);
        CL73_UCTRL1r_CL73_NONCE_MATCH_VALf_SET(cl73_uctrl1, 0);
        CL73_UCTRL1r_CL73_NONCE_MATCH_OVERf_SET(cl73_uctrl1, enable);
        ioerr += WRITE_CL73_UCTRL1r(pc, cl73_uctrl1);
    }

    rv = PHY_NOTIFY(pc, PhyEvent_PhyEnable);
    if (CDK_FAILURE(rv)) {
        return rv;
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

/*
 * Function:    
 *      bcmi_tsc_xgxs_loopback_get
 * Purpose:     
 *      Get the current PHY loopback mode.
 * Parameters:
 *      pc - PHY control structure
 *      enable - (OUT) non-zero indicates PHY loopback enabled
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcmi_tsc_xgxs_loopback_get(phy_ctrl_t *pc, int *enable)
{
    int ioerr = 0;
    LOOPBACK_CONTROLr_t lb_ctrl;
    int lb_enable, lane_mask;

    PHY_CTRL_CHECK(pc);

    lane_mask = _tsc_serdes_lane_mask(pc);
    if (lane_mask < 0) {
        return CDK_E_IO;
    }

    /* Get loopback mode */
    ioerr += READ_LOOPBACK_CONTROLr(pc, &lb_ctrl);
    lb_enable = LOOPBACK_CONTROLr_LOCAL_PCS_LOOPBACK_ENABLEf_GET(lb_ctrl);
    *enable = ((lb_enable & lane_mask) == lane_mask) ? 1 : 0;

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

/*
 * Function:    
 *      bcmi_tsc_xgxs_ability_get
 * Purpose:     
 *      Get the abilities of the PHY.
 * Parameters:
 *      pc - PHY control structure
 *      abil - (OUT) ability mask indicating supported options/speeds.
 * Returns:     
 *      CDK_E_xxx
 */
static int
bcmi_tsc_xgxs_ability_get(phy_ctrl_t *pc, uint32_t *abil)
{
    PHY_CTRL_CHECK(pc);

    if (IS_4LANE_PORT(pc)) {
        *abil = (PHY_ABIL_40GB | PHY_ABIL_25GB | PHY_ABIL_21GB |
                 PHY_ABIL_16GB | PHY_ABIL_13GB | PHY_ABIL_10GB | 
                 PHY_ABIL_1000MB_FD | PHY_ABIL_PAUSE | PHY_ABIL_LOOPBACK | 
                 PHY_ABIL_XAUI | PHY_ABIL_XGMII);
    } else if (PHY_CTRL_FLAGS(pc) & PHY_F_CUSTOM_MODE) {
        *abil = (PHY_ABIL_13GB | PHY_ABIL_10GB | PHY_ABIL_LOOPBACK |
                 PHY_ABIL_XAUI | PHY_ABIL_XGMII);
    } else if (IS_2LANE_PORT(pc)) {
        *abil = (PHY_ABIL_21GB | PHY_ABIL_13GB | PHY_ABIL_10GB | 
                 PHY_ABIL_LOOPBACK | PHY_ABIL_XAUI | PHY_ABIL_XGMII);
    } else {
        *abil = (PHY_ABIL_10GB | PHY_ABIL_2500MB | 
                 PHY_ABIL_1000MB | PHY_ABIL_100MB | PHY_ABIL_10MB | 
                 PHY_ABIL_SERDES | PHY_ABIL_PAUSE | PHY_ABIL_LOOPBACK |
                 PHY_ABIL_GMII);
    }
    return CDK_E_NONE;
}

/*
 * Function:
 *      bcmi_tsc_xgxs_config_set
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
bcmi_tsc_xgxs_config_set(phy_ctrl_t *pc, phy_config_t cfg, uint32_t val, void *cd)
{
    PHY_CTRL_CHECK(pc);

    switch (cfg) {
    case PhyConfig_Enable:
    case PhyConfig_PortInterface:
        return CDK_E_NONE;
    case PhyConfig_RamBase: {
        int ioerr = 0;
        CONFIG_CONTROLr_t cfg_ctrl;

        CONFIG_CONTROLr_CLR(cfg_ctrl);
        if ((val == 0x0fb8) && (_tsc_revid_get(pc) < PHY_REVID_A2)) {
            CONFIG_CONTROLr_RAM_BASEf_SET(cfg_ctrl, 0x0fb8);
        } else {
            CONFIG_CONTROLr_RAM_BASEf_SET(cfg_ctrl, 0x0ee0);
        }
        ioerr += WRITE_CONFIG_CONTROLr(pc, cfg_ctrl);
        return ioerr ? CDK_E_IO : CDK_E_NONE;
    }
    case PhyConfig_Mode: {
        int ioerr = 0;
        uint32_t spm; /* Single Port Mode */
        SETUPr_t setup;

        switch(val) {
        case FV_pm_4port_3_2_1_0: break;
        case FV_pm_3port_32_1_0: break;
        case FV_pm_3port_3_2_10: break;
        case FV_pm_2port_32_10: break;
        case FV_pm_1port_3210: break;
        default: return CDK_E_CONFIG;
        }

        spm = (val == FV_pm_1port_3210) ? 1 : 0;
        ioerr += READ_SETUPr(pc, &setup);
        SETUPr_PORT_MODE_SELf_SET(setup, val);
        SETUPr_SINGLE_PORT_MODEf_SET(setup, spm);
        ioerr += WRITE_SETUPr(pc, setup);

        return ioerr ? CDK_E_IO : CDK_E_NONE;
    }
#if PHY_CONFIG_INCLUDE_XAUI_TX_LANE_MAP_SET
    case PhyConfig_XauiTxLaneRemap: {
        int ioerr = 0;
        int i, odd_chip;
        uint32_t lswap_tx = 0, data;
        uint32_t bwpl = 4; /* Bit Width Per Lane */
        LANE_SWAPr_t txlnswap1;

        odd_chip = ((PHY_CTRL_PHY_INST(pc) / 4) & 0x1) ? 1 : 0;
        if (odd_chip) {
            for(i = 0; i < 4; i++) {
                lswap_tx |= ((3 - ((val >> (bwpl * (i))) & LANE_NUM_MASK)) << (bwpl * i));
            }
        } else {
            lswap_tx = val;
        }

        ioerr += READ_LANE_SWAPr(pc, &txlnswap1);
        data   = (lswap_tx >> (bwpl*0)) & LANE_NUM_MASK;
        LANE_SWAPr_TX0_LNSWAP_SELf_SET(txlnswap1, data);
        data   = (lswap_tx >> (bwpl*1)) & LANE_NUM_MASK;
        LANE_SWAPr_TX1_LNSWAP_SELf_SET(txlnswap1, data);
        data   = (lswap_tx >> (bwpl*2)) & LANE_NUM_MASK;
        LANE_SWAPr_TX2_LNSWAP_SELf_SET(txlnswap1, data);
        data   = (lswap_tx >> (bwpl*3)) & LANE_NUM_MASK;
        LANE_SWAPr_TX3_LNSWAP_SELf_SET(txlnswap1, data);
        ioerr += WRITE_LANE_SWAPr(pc, txlnswap1);

        return ioerr ? CDK_E_IO : CDK_E_NONE;
    }
#endif
#if PHY_CONFIG_INCLUDE_XAUI_RX_LANE_MAP_SET
    case PhyConfig_XauiRxLaneRemap: {
        int ioerr = 0;
        int i, odd_chip;
        uint32_t lswap_rx = 0, data;
        uint32_t bwpl = 4; /* Bit Width Per Lane */
        LANE_SWAPr_t rxlnswap1;

        odd_chip = ((PHY_CTRL_PHY_INST(pc) / 4) & 0x1) ? 1 : 0;
        if (odd_chip) {
            for(i = 0; i < 4; i++) {
                lswap_rx |= ((val >> (bwpl * (3 - i))) & LANE_NUM_MASK) << (bwpl * i);
            }
        } else {
            lswap_rx = val;
        }

        ioerr += READ_LANE_SWAPr(pc, &rxlnswap1);
        data   = (lswap_rx >> (bwpl*0)) & LANE_NUM_MASK;
        LANE_SWAPr_RX0_LNSWAP_SELf_SET(rxlnswap1, data);
        data   = (lswap_rx >> (bwpl*1)) & LANE_NUM_MASK;
        LANE_SWAPr_RX1_LNSWAP_SELf_SET(rxlnswap1, data);
        data   = (lswap_rx >> (bwpl*2)) & LANE_NUM_MASK;
        LANE_SWAPr_RX2_LNSWAP_SELf_SET(rxlnswap1, data);
        data   = (lswap_rx >> (bwpl*3)) & LANE_NUM_MASK;
        LANE_SWAPr_RX3_LNSWAP_SELf_SET(rxlnswap1, data);
        ioerr += WRITE_LANE_SWAPr(pc, rxlnswap1);

        return ioerr ? CDK_E_IO : CDK_E_NONE;
    }
#endif
#if PHY_CONFIG_INCLUDE_XAUI_TX_POLARITY_SET
    case PhyConfig_XauiTxPolInvert: {
        int ioerr = 0;
        int idx, lane_mask, fval;
        ANATX_CONTROL0r_t tx_ctrl;

        lane_mask = _tsc_serdes_lane_mask(pc);
        if (lane_mask < 0) {
            return CDK_E_IO;
        }

        for (idx = 0; idx < LANE_NUM_MASK + 1; idx++) {
            if ((lane_mask & (0x1 << idx)) == 0) {
                continue;
            }

            fval = (val >> (idx * 4)) & 0x1;
            ioerr += READLN_ANATX_CONTROL0r(pc, idx, &tx_ctrl);
            ANATX_CONTROL0r_TXPOL_FLIPf_SET(tx_ctrl, fval);
            ioerr += WRITELN_ANATX_CONTROL0r(pc, idx, tx_ctrl);
        }
        
        return ioerr ? CDK_E_IO : CDK_E_NONE;
    }
#endif
#if PHY_CONFIG_INCLUDE_XAUI_RX_POLARITY_SET
    case PhyConfig_XauiRxPolInvert: {
        int ioerr = 0;
        int idx, lane_mask, fval;
        ANARX_CONTROL0r_t rx_ctrl;

        lane_mask = _tsc_serdes_lane_mask(pc);
        if (lane_mask < 0) {
            return CDK_E_IO;
        }

        for (idx = 0; idx < LANE_NUM_MASK + 1; idx++) {
            if ((lane_mask & (0x1 << idx)) == 0) {
                continue;
            }

            fval = (val >> (idx * 4)) & 0x1;
            ioerr += READLN_ANARX_CONTROL0r(pc, idx, &rx_ctrl);
            ANARX_CONTROL0r_RXPOL_FLIPf_SET(rx_ctrl, fval);
            ioerr += WRITELN_ANARX_CONTROL0r(pc, idx, rx_ctrl);
        }

        return ioerr ? CDK_E_IO : CDK_E_NONE;
    }
#endif
    case PhyConfig_TxPreemp: {
        int ioerr = 0;
        CL72_TX_FIR_TAPr_t tx_fir_tap;

        CL72_TX_FIR_TAPr_SET(tx_fir_tap, val);
        ioerr += WRITE_CL72_TX_FIR_TAPr(pc, tx_fir_tap);

        return ioerr ? CDK_E_IO : CDK_E_NONE;
    }
    case PhyConfig_TxPost2: {
        int ioerr = 0;
        ANATX_DRIVERr_t tx_drv;

        ioerr += READ_ANATX_DRIVERr(pc, &tx_drv);
        ANATX_DRIVERr_POST2_COEFFf_SET(tx_drv, val);
        ioerr += WRITE_ANATX_DRIVERr(pc, tx_drv);

        return ioerr ? CDK_E_IO : CDK_E_NONE;
    }
    case PhyConfig_TxIDrv: {
        int ioerr = 0;
        ANATX_DRIVERr_t tx_drv;

        ioerr += READ_ANATX_DRIVERr(pc, &tx_drv);
        ANATX_DRIVERr_IDRIVERf_SET(tx_drv, val);
        ioerr += WRITE_ANATX_DRIVERr(pc, tx_drv);

        return ioerr ? CDK_E_IO : CDK_E_NONE;
    }
    case PhyConfig_TxPreIDrv: {
        int ioerr = 0;
        ANATX_DRIVERr_t tx_drv;

        ioerr += READ_ANATX_DRIVERr(pc, &tx_drv);
        ANATX_DRIVERr_IPREDRIVERf_SET(tx_drv, val);
        ioerr += WRITE_ANATX_DRIVERr(pc, tx_drv);

        return ioerr ? CDK_E_IO : CDK_E_NONE;
    }
    case PhyConfig_InitStage: {
        return _tsc_init_stage(pc, val);
    }
    case PhyConfig_AdvLocal: {
        int ioerr = 0;

        return ioerr ? CDK_E_IO : CDK_E_NONE;
    }
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
 *      bcmi_tsc_xgxs_config_get
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
bcmi_tsc_xgxs_config_get(phy_ctrl_t *pc, phy_config_t cfg, uint32_t *val, void *cd)
{
    PHY_CTRL_CHECK(pc);

    switch (cfg) {
    case PhyConfig_Enable:
        *val = 1;
        return CDK_E_NONE;
    case PhyConfig_RamBase: {
        int ioerr = 0;
        CONFIG_CONTROLr_t cfg_ctrl;
    
        ioerr += READ_CONFIG_CONTROLr(pc, &cfg_ctrl);
        *val = CONFIG_CONTROLr_RAM_BASEf_GET(cfg_ctrl);
        return ioerr ? CDK_E_IO : CDK_E_NONE;
    }
    case PhyConfig_Mode: {
        int ioerr = 0;
        SETUPr_t setup;
        
        ioerr += READ_SETUPr(pc, &setup);
        *val = SETUPr_PORT_MODE_SELf_GET(setup);

        return ioerr ? CDK_E_IO : CDK_E_NONE;
    }
    case PhyConfig_Clause45Devs:
        *val = 0;
        if (PHY_CTRL_FLAGS(pc) & PHY_F_CLAUSE45) {
            *val = 0x8b;
        }
        return CDK_E_NONE;
    case PhyConfig_TxPreemp: {
        int ioerr = 0;
        CL72_TX_FIR_TAPr_t tx_fir_tap;

        ioerr += READ_CL72_TX_FIR_TAPr(pc, &tx_fir_tap);
        *val = CL72_TX_FIR_TAPr_GET(tx_fir_tap);

        return ioerr ? CDK_E_IO : CDK_E_NONE;
    }
    case PhyConfig_TxPost2: {
        int ioerr = 0;
        ANATX_DRIVERr_t tx_drv;

        ioerr += READ_ANATX_DRIVERr(pc, &tx_drv);
        *val = ANATX_DRIVERr_POST2_COEFFf_GET(tx_drv);

        return ioerr ? CDK_E_IO : CDK_E_NONE;
    }
    case PhyConfig_TxIDrv: {
        int ioerr = 0;
        ANATX_DRIVERr_t tx_drv;

        ioerr += READ_ANATX_DRIVERr(pc, &tx_drv);
        *val = ANATX_DRIVERr_IDRIVERf_GET(tx_drv);

        return ioerr ? CDK_E_IO : CDK_E_NONE;
    }
    case PhyConfig_TxPreIDrv: {
        int ioerr = 0;
        ANATX_DRIVERr_t tx_drv;

        ioerr += READ_ANATX_DRIVERr(pc, &tx_drv);
        *val = ANATX_DRIVERr_IPREDRIVERf_GET(tx_drv);

        return ioerr ? CDK_E_IO : CDK_E_NONE;
    }
    case PhyConfig_AdvRemote: {
        int ioerr = 0;

        return ioerr ? CDK_E_IO : CDK_E_NONE;
    }
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
 *      bcmi_tsc_xgxs_status_get
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
bcmi_tsc_xgxs_status_get(phy_ctrl_t *pc, phy_status_t stat, uint32_t *val)
{
    PHY_CTRL_CHECK(pc);

    switch (stat) {
    case PhyStatus_LineInterface:
        *val = ACTIVE_INTERFACE_GET(pc);
        if (*val == 0) {
            *val = PHY_CTRL_LINE_INTF(pc);
        }
        if (*val == 0) {
            *val = PHY_IF_XGMII;
        }
        return CDK_E_NONE;
    default:
        break;
    }

    return CDK_E_UNAVAIL;
}

/* Public PHY Driver Structure */
phy_driver_t bcmi_tsc_xgxs_drv = {
    "bcmi_tsc_xgxs", 
    "Internal TSC 40G XGXS PHY Driver",  
    PHY_DRIVER_F_INTERNAL,
    bcmi_tsc_xgxs_probe,                /* pd_probe */
    bcmi_tsc_xgxs_notify,               /* pd_notify */
    bcmi_tsc_xgxs_reset,                /* pd_reset */
    bcmi_tsc_xgxs_init,                 /* pd_init */
    bcmi_tsc_xgxs_link_get,             /* pd_link_get */
    bcmi_tsc_xgxs_duplex_set,           /* pd_duplex_set */
    bcmi_tsc_xgxs_duplex_get,           /* pd_duplex_get */
    bcmi_tsc_xgxs_speed_set,            /* pd_speed_set */
    bcmi_tsc_xgxs_speed_get,            /* pd_speed_get */
    bcmi_tsc_xgxs_autoneg_set,          /* pd_autoneg_set */
    bcmi_tsc_xgxs_autoneg_get,          /* pd_autoneg_get */
    bcmi_tsc_xgxs_loopback_set,         /* pd_loopback_set */
    bcmi_tsc_xgxs_loopback_get,         /* pd_loopback_get */
    bcmi_tsc_xgxs_ability_get,          /* pd_ability_get */
    bcmi_tsc_xgxs_config_set,           /* pd_config_set */
    bcmi_tsc_xgxs_config_get,           /* pd_config_get */
    bcmi_tsc_xgxs_status_get,           /* pd_status_get */
    NULL                                /* pd_cable_diag */
};
