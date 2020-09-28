/**********************************************************************************
**********************************************************************************
*  File Name     :  srds_api_err_code.h                                          *
*  Created On    :  18/09/2013                                                   *
*  Created By    :  Kiran Divakar                                                *
*  Description   :  Header file with Error Code enums                            *
*  Revision      :  $Id: srds_api_err_code.h 1078 Broadcom SDK $ *
*                                                                                *
* This software is governed by the Broadcom Switch APIs license.
* This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
* 
* Copyright 2007-2020 Broadcom Inc. All rights reserved.                                                          *
*  No portions of this material may be reproduced in any form without            *
*  the written permission of:                                                    *
*      Broadcom Corporation                                                      *
*      5300 California Avenue                                                    *
*      Irvine, CA  92617                                                         *
*                                                                                *
*  All information contained in this document is Broadcom Corporation            *
*  company private proprietary, and trade secret.                                *
 */

/** @file srds_api_err_code.h
 * Error Code enumerations
 */

#ifndef SRDS_API_ERR_CODE_H
#define SRDS_API_ERR_CODE_H

#define err_code_t uint16_t

/** ERROR CODE Enum */
enum srds_err_code_enum {
	ERR_CODE_NONE = 0,
	ERR_CODE_INVALID_RAM_ADDR,
	ERR_CODE_SERDES_DELAY,
	ERR_CODE_POLLING_TIMEOUT,
	ERR_CODE_CFG_PATT_INVALID_PATTERN,
	ERR_CODE_CFG_PATT_INVALID_PATT_LENGTH,
	ERR_CODE_CFG_PATT_LEN_MISMATCH,
	ERR_CODE_CFG_PATT_PATTERN_BIGGER_THAN_MAXLEN,
	ERR_CODE_CFG_PATT_INVALID_HEX,
	ERR_CODE_CFG_PATT_INVALID_BIN2HEX,
	ERR_CODE_CFG_PATT_INVALID_SEQ_WRITE,
	ERR_CODE_PATT_GEN_INVALID_MODE_SEL,
	ERR_CODE_INVALID_UCODE_LEN,
	ERR_CODE_MICRO_INIT_NOT_DONE,
	ERR_CODE_UCODE_LOAD_FAIL,
	ERR_CODE_UCODE_VERIFY_FAIL,
	ERR_CODE_INVALID_TEMP_IDX,
	ERR_CODE_INVALID_PLL_CFG,
	ERR_CODE_TX_HPF_INVALID,
	ERR_CODE_VGA_INVALID,
	ERR_CODE_PF_INVALID,
	ERR_CODE_TX_AMP_CTRL_INVALID,
	ERR_CODE_INVALID_EVENT_LOG_WRITE,
	ERR_CODE_INVALID_EVENT_LOG_READ,
	ERR_CODE_UC_CMD_RETURN_ERROR,
	ERR_CODE_DATA_NOTAVAIL,
	ERR_CODE_BAD_PTR_OR_INVALID_INPUT,
	ERR_CODE_UC_NOT_STOPPED,
	ERR_CODE_UC_CRC_NOT_MATCH,
	ERR_CODE_CORE_DP_NOT_RESET,
	ERR_CODE_LANE_DP_NOT_RESET,
	ERR_CODE_INVALID_CLK90_ADJUST,
	ERR_CODE_INVALID_CLK90_OVERRIDE,
	ERR_CODE_INVALID_TTS_PRBS_TRAFFIC_SEL,
	ERR_CODE_TXFIR   = 1 << 8,
	ERR_CODE_DFE_TAP = 2 << 8,
	ERR_CODE_DIAG    = 3 << 8
};

/** TXFIR Error Codes Enum */
enum srds_txfir_failcodes {
	ERR_CODE_TXFIR_PRE_INVALID         = ERR_CODE_TXFIR +   1,
	ERR_CODE_TXFIR_MAIN_INVALID        = ERR_CODE_TXFIR +   2,
	ERR_CODE_TXFIR_POST1_INVALID       = ERR_CODE_TXFIR +   4,
	ERR_CODE_TXFIR_POST2_INVALID       = ERR_CODE_TXFIR +   8,
	ERR_CODE_TXFIR_POST3_INVALID       = ERR_CODE_TXFIR +  16,
	ERR_CODE_TXFIR_V2_LIMIT            = ERR_CODE_TXFIR +  32,
	ERR_CODE_TXFIR_SUM_LIMIT           = ERR_CODE_TXFIR +  64,
	ERR_CODE_TXFIR_PRE_POST1_SUM_LIMIT = ERR_CODE_TXFIR + 128
};

/** DFE Tap Error Codes Enum */
enum srds_dfe_tap_failcodes {
	ERR_CODE_DFE1_INVALID        = ERR_CODE_DFE_TAP +  1,
	ERR_CODE_DFE2_INVALID        = ERR_CODE_DFE_TAP +  2,
	ERR_CODE_DFE3_INVALID        = ERR_CODE_DFE_TAP +  4,
	ERR_CODE_DFE4_INVALID        = ERR_CODE_DFE_TAP +  8,
	ERR_CODE_DFE5_INVALID        = ERR_CODE_DFE_TAP + 16,
	ERR_CODE_DFE_TAP_IDX_INVALID = ERR_CODE_DFE_TAP + 32
};

/** DIAG Error Codes Enum */
enum srds_diag_failcodes {
	ERR_CODE_DIAG_TIMEOUT               = ERR_CODE_DIAG + 1,
	ERR_CODE_DIAG_INVALID_STATUS_RETURN = ERR_CODE_DIAG + 2,
	ERR_CODE_DIAG_SCAN_NOT_COMPLETE     = ERR_CODE_DIAG + 3
};
#endif
