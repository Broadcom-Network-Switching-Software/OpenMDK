/*----------------------------------------------------------------------
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Broadcom Corporation
 * Proprietary and Confidential information
 * All rights reserved
 * This source file is the property of Broadcom Corporation, and
 * may not be copied or distributed in any isomorphic form without the
 * prior written consent of Broadcom Corporation.
 *---------------------------------------------------------------------
 * File       : eagle_cfg_seq.h
 * Description: c functions implementing Tier1s for TEMod Serdes Driver
 *---------------------------------------------------------------------*/
/*
 *  $Copyright: Copyright 2019 Broadcom Corporation.
 *  This program is the proprietary software of Broadcom Corporation
 *  and/or its licensors, and may only be used, duplicated, modified
 *  or distributed pursuant to the terms and conditions of a separate,
 *  written license agreement executed between you and Broadcom
 *  (an "Authorized License").  Except as set forth in an Authorized
 *  License, Broadcom grants no license (express or implied), right
 *  to use, or waiver of any kind with respect to the Software, and
 *  Broadcom expressly reserves all rights in and to the Software
 *  and all intellectual property rights therein.  IF YOU HAVE
 *  NO AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE
 *  IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE
 *  ALL USE OF THE SOFTWARE.  
 *   
 *  Except as expressly set forth in the Authorized License,
 *   
 *  1.     This program, including its structure, sequence and organization,
 *  constitutes the valuable trade secrets of Broadcom, and you shall use
 *  all reasonable efforts to protect the confidentiality thereof,
 *  and to use this information only in connection with your use of
 *  Broadcom integrated circuit products.
 *   
 *  2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS
 *  PROVIDED "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
 *  REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY,
 *  OR OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
 *  DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
 *  NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
 *  ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
 *  CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING
 *  OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
 *  
 *  3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL
 *  BROADCOM OR ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL,
 *  INCIDENTAL, SPECIAL, INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER
 *  ARISING OUT OF OR IN ANY WAY RELATING TO YOUR USE OF OR INABILITY
 *  TO USE THE SOFTWARE EVEN IF BROADCOM HAS BEEN ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGES; OR (ii) ANY AMOUNT IN EXCESS OF
 *  THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF OR USD 1.00,
 *  WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING
 *  ANY FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.$
 *  $Id: 99c134ba7738e9cfaa12537d70d8cd9c5c18f750 $
*/


#ifndef EAGLE_CFG_SEQ_H
#define EAGLE_CFG_SEQ_H

#include "srds_api_err_code.h"
#include "eagle2_tsc2pll_enum.h"

typedef struct {
  int8_t pll_pwrdn;
  int8_t tx_s_pwrdn;
  int8_t rx_s_pwrdn;
} power_status_t;

typedef struct {
  int8_t revid_model;
  int8_t revid_process;
  int8_t revid_bonding;
  int8_t revid_rev_number;
  int8_t revid_rev_letter;
} eagle_rev_id0_t;


typedef enum {
  TX = 0,
  Rx
} tx_rx_t;

typedef struct {
  int8_t revid_eee;
  int8_t revid_llp;
  int8_t revid_pir;
  int8_t revid_cl72;
  int8_t revid_micro;
  int8_t revid_mdio;
  int8_t revid_multiplicity;
} eagle_rev_id1_t;


typedef enum {
    EAGLE_PRBS_POLYNOMIAL_7 = 0,
    EAGLE_PRBS_POLYNOMIAL_9,
    EAGLE_PRBS_POLYNOMIAL_11,
    EAGLE_PRBS_POLYNOMIAL_15,
    EAGLE_PRBS_POLYNOMIAL_23,
    EAGLE_PRBS_POLYNOMIAL_31,
    EAGLE_PRBS_POLYNOMIAL_58,
    EAGLE_PRBS_POLYNOMIAL_TYPE_COUNT
}eagle_prbs_polynomial_type_t;

#define PATTERN_MAX_SIZE 8

err_code_t eagle2_tsc2pll_identify(const phymod_access_t *pa, eagle_rev_id0_t *rev_id0, eagle_rev_id1_t *rev_id1);

err_code_t eagle2_tsc2pll_uc_active_set(const phymod_access_t *pa, uint32_t enable);           /* set microcontroller active or not  */
err_code_t eagle2_tsc2pll_uc_active_get(const phymod_access_t *pa, uint32_t *enable);          /* get microcontroller active or not  */
err_code_t eagle2_uc_reset(const phymod_access_t *pa, uint32_t enable);           /* set dw8501 reset  */
err_code_t eagle2_tsc2pll_prbs_tx_inv_data_get(const phymod_access_t *pa, uint32_t *inv_data);
err_code_t eagle2_tsc2pll_prbs_rx_inv_data_get(const phymod_access_t *pa, uint32_t *inv_data);
err_code_t eagle2_tsc2pll_prbs_tx_poly_get(const phymod_access_t *pa, eagle_prbs_polynomial_type_t *prbs_poly);
err_code_t eagle2_tsc2pll_prbs_rx_poly_get(const phymod_access_t *pa, eagle_prbs_polynomial_type_t *prbs_poly);
err_code_t eagle2_tsc2pll_prbs_tx_enable_get(const phymod_access_t *pa, uint32_t *enable);
err_code_t eagle2_tsc2pll_prbs_rx_enable_get(const phymod_access_t *pa, uint32_t *enable);
err_code_t eagle2_tsc2pll_pll_mode_set(const phymod_access_t *pa, int pll_mode);   /* PLL divider set */
err_code_t eagle2_tsc2pll_core_soft_reset_release(const phymod_access_t *pa, uint32_t enable);   /* release the pmd core soft reset */
err_code_t eagle2_tsc2pll_lane_soft_reset_release(const phymod_access_t *pa, uint32_t enable);   /* release the pmd core soft reset */
err_code_t eagle2_tsc2pll_pram_firmware_enable(const phymod_access_t *pa, int enable);   /* release the pmd core soft reset */
err_code_t eagle2_tsc2pll_pmd_force_signal_detect(const phymod_access_t *pa, int enable); /*force the signal detect */
err_code_t eagle2_tsc2pll_pmd_loopback_get(const phymod_access_t *pa, uint32_t *enable);
err_code_t eagle2_tsc2pll_uc_init(const phymod_access_t *pa);
err_code_t eagle2_tsc2pll_pram_flop_set(const phymod_access_t *pa, int value);
err_code_t eagle2_tsc2pll_ucode_init( const phymod_access_t *pa );
err_code_t eagle2_tsc2pll_pmd_ln_h_rstb_pkill_override( const phymod_access_t *pa, uint16_t val);
err_code_t eagle2_tsc2pll_pmd_cl72_enable_get(const phymod_access_t *pa, uint32_t *enable);
err_code_t eagle2_tsc2pll_pmd_cl72_receiver_status(const phymod_access_t *pa, uint32_t *status);
err_code_t eagle2_tsc2pll_tx_pi_control_get( const phymod_access_t *pa,  int16_t *value);


err_code_t eagle2_tsc2pll_tx_rx_polarity_set(const phymod_access_t *pa, uint32_t tx_pol, uint32_t rx_pol);
err_code_t eagle2_tsc2pll_tx_rx_polarity_get(const phymod_access_t *pa, uint32_t *tx_pol, uint32_t *rx_pol);
err_code_t eagle2_tsc2pll_force_tx_set_rst (const phymod_access_t *pa, uint32_t rst);
err_code_t eagle2_tsc2pll_force_tx_get_rst (const phymod_access_t *pa, uint32_t *rst);
err_code_t eagle2_tsc2pll_force_rx_set_rst (const phymod_access_t *pa, uint32_t rst);
err_code_t eagle2_tsc2pll_force_rx_get_rst (const phymod_access_t *pa, uint32_t *rst);
err_code_t eagle2_tsc2pll_pll_mode_get(const phymod_access_t *pa, uint32_t *pll_mode);
err_code_t eagle2_tsc2pll_osr_mode_set(const phymod_access_t *pa, int osr_mode);
err_code_t eagle2_tsc2pll_osr_mode_get(const phymod_access_t *pa, int *osr_mode);
err_code_t eagle2_tsc2pll_dig_lpbk_get(const phymod_access_t *pa, uint32_t *lpbk);
err_code_t eagle2_tsc2pll_rmt_lpbk_get(const phymod_access_t *pa, uint32_t *lpbk);
err_code_t eagle2_tsc2pll_core_soft_reset(const phymod_access_t *pa);
err_code_t eagle2_tsc2pll_pwrdn_set(const phymod_access_t *pa, int tx_rx, int pwrdn);
err_code_t eagle2_tsc2pll_pwrdn_get(const phymod_access_t *pa, power_status_t *pwrdn);
err_code_t eagle2_tsc2pll_pcs_lane_swap_tx(const phymod_access_t *pa, uint32_t tx_lane_map);
err_code_t eagle2_tsc2pll_pmd_lane_swap (const phymod_access_t *pa, uint32_t lane_map);
err_code_t eagle2_tsc2pll_pmd_lane_swap_tx_get (const phymod_access_t *pa, uint32_t *lane_map);
err_code_t eagle2_tsc2pll_lane_hard_soft_reset_release(const phymod_access_t *pa, uint32_t enable);
err_code_t eagle2_tsc2pll_clause72_control(const phymod_access_t *pa, uint32_t cl_72_en);
err_code_t eagle2_tsc2pll_clause72_control_get(const phymod_access_t *pa, uint32_t *cl_72_en);
err_code_t eagle2_tsc2pll_pmd_reset_seq(const phymod_access_t *pa, int pmd_touched);
err_code_t eagle2_tsc2pll_pll_reset_enable_set (const phymod_access_t *pa, int enable);
err_code_t eagle2_tsc2pll_read_pll_range (const phymod_access_t *pa, uint32_t *pll_range);
err_code_t eagle2_tsc2pll_signal_detect (const phymod_access_t *pa, uint32_t *signal_detect);
err_code_t eagle2_tsc2pll_ladder_setting_to_mV(const phymod_access_t *pa, int8_t y, int16_t* level);
err_code_t eagle2_tsc2pll_pll_config_get(const phymod_access_t *pa, enum eagle2_tsc2pll_pll_enum *pll_mode);
err_code_t eagle2_tsc2pll_tx_lane_control_set(const phymod_access_t *pa, uint32_t en);
err_code_t eagle2_tsc2pll_tx_lane_control_get(const phymod_access_t *pa, uint32_t* en);
err_code_t eagle2_tsc2pll_rx_lane_control_set(const phymod_access_t *pa, uint32_t en);
err_code_t eagle2_tsc2pll_rx_lane_control_get(const phymod_access_t *pa, uint32_t* en);
err_code_t eagle2_tsc2pll_get_vco (const phymod_phy_inf_config_t* config, uint32_t *vco_rate, uint32_t *new_pll_div, int16_t *new_os_mode);
err_code_t eagle2_tsc2pll_pmd_tx_disable_pin_dis_set(const phymod_access_t *pa, uint32_t enable);
err_code_t eagle2_tsc2pll_pmd_tx_disable_pin_dis_get(const phymod_access_t *pa, uint32_t *enable);
err_code_t eagle2_tsc2pll_tx_shared_patt_gen_en_get( const phymod_access_t *pa, uint8_t *enable);
err_code_t eagle2_tsc2pll_config_shared_tx_pattern_idx_set( const phymod_access_t *pa, const uint32_t *pattern_len);
err_code_t eagle2_tsc2pll_config_shared_tx_pattern_idx_get( const phymod_access_t *pa, uint32_t *pattern_len, uint32_t *pattern);
err_code_t eagle2_tsc2pll_osr_mode_to_enum(int osr_mode, phymod_osr_mode_t* osr_mode_en);
err_code_t eagle2_tsc2pll_select_pll(const phymod_access_t *pa, uint8_t pll_index);
uint8_t eagle2_tsc2pll_get_current_pll(const phymod_access_t *pa);
#endif /* PHY_TSC_IBLK_H */

