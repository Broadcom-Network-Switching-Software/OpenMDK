/*----------------------------------------------------------------------
 *
 * Broadcom Corporation
 * Proprietary and Confidential information
 * All rights reserved
 * This source file is the property of Broadcom Corporation, and
 * may not be copied or distributed in any isomorphic form without the
 * prior written consent of Broadcom Corporation.
 *---------------------------------------------------------------------
 * File       : falcon_cfg_seq.h
 * Description: c functions implementing Tier1s for TEMod Serdes Driver
 *---------------------------------------------------------------------*/
/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *  $Id: cfa3300e6686e225142e226fa0889f8020b049b5 $
*/


#ifndef FALCON_MONTEREY_CFG_SEQ_H 
#define FALCON_MONTEREY_CFG_SEQ_H

#include "srds_api_err_code.h"
#include "falcon2_monterey_enum.h"

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
} falcon2_monterey_rev_id0_t;

typedef struct {
  int8_t revid_eee;
  int8_t revid_llp; 
  int8_t revid_pir; 
  int8_t revid_cl72; 
  int8_t revid_micro; 
  int8_t revid_mdio; 
  int8_t revid_multiplicity;
} falcon2_monterey_rev_id1_t;

typedef enum {
  TX = 0,
  Rx
} tx_rx_t;

typedef enum {
    FALCON_PRBS_POLYNOMIAL_7 = 0,
    FALCON_PRBS_POLYNOMIAL_9,
    FALCON_PRBS_POLYNOMIAL_11,
    FALCON_PRBS_POLYNOMIAL_15,
    FALCON_PRBS_POLYNOMIAL_23,
    FALCON_PRBS_POLYNOMIAL_31,
    FALCON_PRBS_POLYNOMIAL_58,
    FALCON_PRBS_POLYNOMIAL_TYPE_COUNT 
}falcon2_monterey_prbs_polynomial_type_t;

#define PATTERN_MAX_SIZE 8

extern err_code_t _falcon2_monterey_pmd_mwr_reg_byte( const phymod_access_t *pa, uint16_t addr, uint16_t mask, uint8_t lsb, uint8_t val);
extern uint8_t _falcon2_monterey_pmd_rde_field_byte( const phymod_access_t *pa, uint16_t addr, uint8_t shift_left, uint8_t shift_right, err_code_t *err_code_p);
extern uint16_t _falcon2_monterey_pmd_rde_field( const phymod_access_t *pa, uint16_t addr, uint8_t shift_left, uint8_t shift_right, err_code_t *err_code_p);
err_code_t falcon2_monterey_tx_pi_control_get(const phymod_access_t *pa,  int16_t *value);
err_code_t falcon2_monterey_tx_pi_enable_set(const phymod_access_t *pa, uint32_t enable);
err_code_t falcon2_monterey_tx_pi_enable_get(const phymod_access_t *pa, uint32_t *enable);
err_code_t falcon2_monterey_tx_pi_ext_pd_select_set(const phymod_access_t *pa, uint32_t ext_pd);
err_code_t falcon2_monterey_tx_pi_ext_pd_select_get(const phymod_access_t *pa, uint32_t *ext_pd);
err_code_t falcon2_monterey_identify(const phymod_access_t *pa, falcon2_monterey_rev_id0_t *rev_id0, falcon2_monterey_rev_id1_t *rev_id1);
err_code_t falcon2_monterey_tx_rx_polarity_set(const phymod_access_t *pa, uint32_t tx_pol, uint32_t rx_pol);
err_code_t falcon2_monterey_tx_rx_polarity_get(const phymod_access_t *pa, uint32_t *tx_pol, uint32_t *rx_pol);
err_code_t falcon2_monterey_uc_active_set(const phymod_access_t *pa, uint32_t enable);
err_code_t falcon2_monterey_uc_active_get(const phymod_access_t *pa, uint32_t *enable);
err_code_t falcon2_monterey_prbs_tx_inv_data_get(const phymod_access_t *pa, uint32_t *inv_data);
err_code_t falcon2_monterey_prbs_rx_inv_data_get(const phymod_access_t *pa, uint32_t *inv_data);
err_code_t falcon2_monterey_prbs_tx_poly_get(const phymod_access_t *pa, falcon2_monterey_prbs_polynomial_type_t *prbs_poly);
err_code_t falcon2_monterey_prbs_rx_poly_get(const phymod_access_t *pa, falcon2_monterey_prbs_polynomial_type_t *prbs_poly);
err_code_t falcon2_monterey_prbs_tx_enable_get(const phymod_access_t *pa, uint32_t *enable);
err_code_t falcon2_monterey_prbs_rx_enable_get(const phymod_access_t *pa, uint32_t *enable);
err_code_t falcon2_monterey_pmd_force_signal_detect(const phymod_access_t *pa, uint32_t enable);
err_code_t falcon2_monterey_pll_mode_set(const phymod_access_t *pa, int pll_mode);
err_code_t falcon2_monterey_pll_mode_get(const phymod_access_t *pa, uint32_t *pll_mode);
err_code_t falcon2_monterey_afe_pll_reg_set(const phymod_access_t *pa, const phymod_afe_pll_t *afe_pll);
err_code_t falcon2_monterey_afe_pll_reg_get(const phymod_access_t *pa, phymod_afe_pll_t *afe_pll);
err_code_t falcon2_monterey_osr_mode_set(const phymod_access_t *pa, int osr_mode);
err_code_t falcon2_monterey_osr_mode_get(const phymod_access_t *pa, int *osr_mode);
err_code_t falcon2_monterey_tsc_dig_lpbk_get(const phymod_access_t *pa, uint32_t *lpbk);
err_code_t falcon2_monterey_tsc_rmt_lpbk_get(const phymod_access_t *pa, uint32_t *lpbk);
err_code_t falcon2_monterey_core_soft_reset(const phymod_access_t *pa);
err_code_t falcon2_monterey_core_soft_reset_release(const phymod_access_t *pa, uint32_t enable);
err_code_t falcon2_monterey_core_soft_reset_read(const phymod_access_t *pa, uint32_t *enable);
err_code_t falcon2_monterey_lane_soft_reset_read(const phymod_access_t *pa, uint32_t *enable);
err_code_t falcon2_monterey_pmd_tx_disable_pin_dis_set(const phymod_access_t *pa, uint32_t enable);
err_code_t falcon2_monterey_pmd_tx_disable_pin_dis_get(const phymod_access_t *pa, uint32_t *enable);
err_code_t falcon2_monterey_tsc_pwrdn_set(const phymod_access_t *pa, int tx_rx, int pwrdn);
err_code_t falcon2_monterey_tsc_pwrdn_get(const phymod_access_t *pa, power_status_t *pwrdn);
err_code_t falcon2_monterey_pcs_lane_swap_tx(const phymod_access_t *pa, uint32_t tx_lane_map);
err_code_t falcon2_monterey_pmd_lane_swap (const phymod_access_t *pa, uint32_t lane_map);
err_code_t falcon2_monterey_pmd_lane_swap_tx_get(const phymod_access_t *pa, uint32_t *lane_map);
err_code_t falcon2_monterey_pmd_loopback_get(const phymod_access_t *pa, uint32_t *enable);   
err_code_t falcon2_monterey_tsc_identify(const phymod_access_t *pa, falcon2_monterey_rev_id0_t *rev_id0, falcon2_monterey_rev_id1_t *rev_id1);
err_code_t falcon2_monterey_pmd_ln_h_rstb_pkill_override( const phymod_access_t *pa, uint16_t val); 
err_code_t falcon2_monterey_lane_soft_reset(const phymod_access_t *pa, uint32_t enable);
err_code_t falcon2_monterey_lane_soft_reset_release(const phymod_access_t *pa, uint32_t enable);
err_code_t falcon2_monterey_lane_soft_reset_release_get(const phymod_access_t *pa, uint32_t *enable);
err_code_t falcon2_monterey_rx_lane_soft_reset_release(const phymod_access_t *pa, uint32_t enable);
err_code_t falcon2_monterey_rx_lane_soft_reset_release_get(const phymod_access_t *pa, uint32_t *enable);
err_code_t falcon2_monterey_tx_lane_soft_reset_release(const phymod_access_t *pa, uint32_t enable);
err_code_t falcon2_monterey_tx_lane_soft_reset_release_get(const phymod_access_t *pa, uint32_t *enable);
err_code_t falcon2_conterey_lane_hard_soft_reset_release(const phymod_access_t *pa, uint32_t enable);
err_code_t falcon2_monterey_lane_hard_soft_reset_release(const phymod_access_t *pa, uint32_t enable);
err_code_t falcon2_monterey_clause72_control(const phymod_access_t *pc, uint32_t cl_72_en);        /* CLAUSE_72_CONTROL */
err_code_t falcon2_monterey_clause72_control_get(const phymod_access_t *pc, uint32_t *cl_72_en);   /* CLAUSE_72_CONTROL */
err_code_t falcon2_monterey_pmd_cl72_enable_get(const phymod_access_t *pa, uint32_t *enable);
err_code_t falcon2_monterey_pmd_cl72_receiver_status(const phymod_access_t *pa, uint32_t *status); 
err_code_t falcon2_monterey_ucode_init( const phymod_access_t *pa );
err_code_t falcon2_monterey_pram_firmware_enable(const phymod_access_t *pa, int enable, int wait);  
err_code_t falcon2_monterey_pmd_reset_seq(const phymod_access_t *pa, int pmd_touched);
err_code_t falcon2_monterey_reset_enable_set (const phymod_access_t *pa, int enable);
err_code_t falcon2_monterey_pll_reset_enable_set(const phymod_access_t *pa, int enable);
err_code_t falcon2_monterey_tsc_read_pll_range(const phymod_access_t *pa, uint32_t *pll_range);
err_code_t falcon2_monterey_tsc_signal_detect (const phymod_access_t *pa, uint32_t *signal_detect);
err_code_t falcon2_monterey_ladder_setting_to_mV(const phymod_access_t *pa, int8_t y, int16_t* level);
err_code_t falcon2_monterey_force_tx_set_rst (const phymod_access_t *pa, uint32_t rst);
err_code_t falcon2_monterey_force_tx_get_rst (const phymod_access_t *pa, uint32_t *rst);
err_code_t falcon2_monterey_force_rx_set_rst (const phymod_access_t *pa, uint32_t rst);
err_code_t falcon2_monterey_force_rx_get_rst (const phymod_access_t *pa, uint32_t *rst);
err_code_t falcon2_monterey_tsc_ladder_setting_to_mV(const phymod_access_t *pa, int8_t y, int16_t* level);
err_code_t falcon2_monterey_electrical_idle_set(const phymod_access_t *pa, uint32_t en);
err_code_t falcon2_monterey_tsc_get_vco (const phymod_phy_inf_config_t* config, uint32_t *vco_rate, uint32_t *new_pll_div, int16_t *new_os_mode);
err_code_t falcon2_monterey_tsc_tx_shared_patt_gen_en_get( const phymod_access_t *pa, uint8_t *enable);
err_code_t falcon2_monterey_tsc_config_shared_tx_pattern_idx_set( const phymod_access_t *pa, const uint32_t *pattern_len);
err_code_t falcon2_monterey_tsc_config_shared_tx_pattern_idx_get( const phymod_access_t *pa, uint32_t *pattern_len, uint32_t *pattern);
err_code_t falcon2_monterey_tsc_tx_disable_get (const phymod_access_t *pa, uint8_t *enable);
err_code_t falcon2_monterey_refclk_set(const phymod_access_t *pa, phymod_ref_clk_t ref_clock);
err_code_t falcon2_monterey_pll_idx_set(phymod_access_t *pa, uint8_t pll_idx);
err_code_t falcon2_monterey_pll_idx_get(const phymod_access_t *pa, uint8_t* pll_idx);
err_code_t falcon2_monterey_write_cip_clk_div(const phymod_access_t *pa, int cip_clk_div);
err_code_t falcon2_monterey_pll_select_get(const phymod_access_t *pa, uint8_t* pll_select);
err_code_t falcon2_monterey_pll_select_set(const phymod_access_t *pa, uint8_t pll_select);
err_code_t falcon2_monterey_get_vco (const phymod_phy_inf_config_t* config, uint32_t *vco_rate, uint32_t *new_pll_div, int16_t *new_os_mode);
err_code_t falcon2_monterey_rescal_val_set(const phymod_access_t *pa, uint8_t enable, uint32_t rescal_val);
err_code_t falcon2_monterey_rescal_val_get(const phymod_access_t *pa , uint32_t *rescal_val);
err_code_t falcon2_monterey_frc_tx_osr (const phymod_access_t *pa, int osr_mode);
err_code_t falcon2_monterey_frc_rx_osr (const phymod_access_t *pa, int osr_mode);

#endif /* PHY_TSC_IBLK_H */
