/*
 *
 *
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <phymod/phymod.h>
#include <phymod/phymod_diagnostics.h>
#include <phymod/phymod_config.h>
#include <phymod/phymod_util.h>
#include <phymod/phymod_config.h>
#include "temod_dpll_enum_defines.h"
#include "temod_dpll.h"
#include "../../bcmi_eagle_dpll_xgxs/eagle_dpll/eagle2_tsc2pll_common.h"
#include "../../bcmi_eagle_dpll_xgxs/eagle_dpll/eagle2_tsc2pll_interface.h"
#include "../../bcmi_eagle_dpll_xgxs/eagle_dpll/eagle2_cfg_seq.h"
#include "../../bcmi_eagle_dpll_xgxs/eagle_dpll/eagle2_tsc2pll_debug_functions.h"

#define PATTERN_MAX_LENGTH 240
#ifdef PHYMOD_TSCE_DPLL_SUPPORT

#define TSCE_DPLL_PHY_ACCESS_PLLIDX_SET(_phy_access) \
    do{\
        (_phy_access)->access.pll_idx = 0;\
        (_phy_access)->access.pll_idx = temod2pll_pll_index_get(&((_phy_access)->access));\
    }while(0)


/*phymod, internal enum mappings*/
STATIC
int _tsce_dpll_prbs_poly_phymod_to_eagle(phymod_prbs_poly_t phymod_poly, enum srds_prbs_polynomial_enum *eagle_poly)
{
    switch(phymod_poly){
    case phymodPrbsPoly7:
        *eagle_poly = PRBS_7;
        break;
    case phymodPrbsPoly9:
        *eagle_poly = PRBS_9;
        break;
    case phymodPrbsPoly11:
        *eagle_poly = PRBS_11;
        break;
    case phymodPrbsPoly15:
        *eagle_poly = PRBS_15;
        break;
    case phymodPrbsPoly23:
        *eagle_poly = PRBS_23;
        break;
    case phymodPrbsPoly31:
        *eagle_poly = PRBS_31;
        break;
    case phymodPrbsPoly58:
        *eagle_poly = PRBS_58;
        break;
    default:
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM, (_PHYMOD_MSG("unsupported poly for tsce %u"), phymod_poly));
    }
    return PHYMOD_E_NONE;
}

STATIC
int _tsce_dpll_prbs_poly_tsce_to_phymod(eagle_prbs_polynomial_type_t tsce_poly, phymod_prbs_poly_t *phymod_poly)
{
    switch(tsce_poly){
    case EAGLE_PRBS_POLYNOMIAL_7:
        *phymod_poly = phymodPrbsPoly7;
        break;
    case EAGLE_PRBS_POLYNOMIAL_9:
        *phymod_poly = phymodPrbsPoly9;
        break;
    case EAGLE_PRBS_POLYNOMIAL_11:
        *phymod_poly = phymodPrbsPoly11;
        break;
    case EAGLE_PRBS_POLYNOMIAL_15:
        *phymod_poly = phymodPrbsPoly15;
        break;
    case EAGLE_PRBS_POLYNOMIAL_23:
        *phymod_poly = phymodPrbsPoly23;
        break;
    case EAGLE_PRBS_POLYNOMIAL_31:
        *phymod_poly = phymodPrbsPoly31;
        break;
    case EAGLE_PRBS_POLYNOMIAL_58:
        *phymod_poly = phymodPrbsPoly58;
        break;
    default:
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_INTERNAL, (_PHYMOD_MSG("uknown poly %u"), tsce_poly));
    }
    return PHYMOD_E_NONE;
}

int tsce_dpll_phy_rx_slicer_position_set(const phymod_phy_access_t* phy, uint32_t flags, const phymod_slicer_position_t* position)
{


    /* Place your code here */


    return PHYMOD_E_NONE;

}

int tsce_dpll_phy_rx_slicer_position_get(const phymod_phy_access_t* phy, uint32_t flags, phymod_slicer_position_t* position)
{


    /* Place your code here */


    return PHYMOD_E_NONE;

}


int tsce_dpll_phy_rx_slicer_position_max_get(const phymod_phy_access_t* phy, uint32_t flags, const phymod_slicer_position_t* position_min, const phymod_slicer_position_t* position_max)
{


    /* Place your code here */


    return PHYMOD_E_NONE;

}


int tsce_dpll_phy_prbs_config_set(const phymod_phy_access_t* phy, uint32_t flags , const phymod_prbs_t* prbs)
{
    phymod_phy_access_t phy_copy;
    int start_lane, num_lane;
    int i = 0;

    enum srds_prbs_polynomial_enum eagle_poly;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    TSCE_DPLL_PHY_ACCESS_PLLIDX_SET(&phy_copy);

    /*next figure out the lane num and start_lane based on the input*/
    PHYMOD_IF_ERR_RETURN
        (phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    PHYMOD_IF_ERR_RETURN(_tsce_dpll_prbs_poly_phymod_to_eagle(prbs->poly, &eagle_poly));
    /*first check which direction */
    if (PHYMOD_PRBS_DIRECTION_RX_GET(flags)) {
        for (i = 0; i < num_lane; i++) {
            phy_copy.access.lane_mask = 0x1 << (i + start_lane);
            PHYMOD_IF_ERR_RETURN
                (eagle2_tsc2pll_config_rx_prbs(&phy_copy.access, eagle_poly, PRBS_INITIAL_SEED_HYSTERESIS,  prbs->invert));
        }
    } else if (PHYMOD_PRBS_DIRECTION_TX_GET(flags)) {
        for (i = 0; i < num_lane; i++) {
            phy_copy.access.lane_mask = 0x1 << (i + start_lane);
            PHYMOD_IF_ERR_RETURN
                (eagle2_tsc2pll_config_tx_prbs(&phy_copy.access, eagle_poly, prbs->invert));
        }
    } else {
        for (i = 0; i < num_lane; i++) {
            phy_copy.access.lane_mask = 0x1 << (i + start_lane);
            PHYMOD_IF_ERR_RETURN
                (eagle2_tsc2pll_config_rx_prbs(&phy_copy.access, eagle_poly, PRBS_INITIAL_SEED_HYSTERESIS,  prbs->invert));
            PHYMOD_IF_ERR_RETURN
                (eagle2_tsc2pll_config_tx_prbs(&phy_copy.access, eagle_poly, prbs->invert));
        }
    }
    return PHYMOD_E_NONE;

}

int tsce_dpll_phy_prbs_config_get(const phymod_phy_access_t* phy, uint32_t flags , phymod_prbs_t* prbs)
{
    phymod_phy_access_t phy_copy;
    phymod_prbs_t config_tmp;
    eagle_prbs_polynomial_type_t tsce_poly;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    TSCE_DPLL_PHY_ACCESS_PLLIDX_SET(&phy_copy);

    if (PHYMOD_PRBS_DIRECTION_TX_GET(flags)) {
        PHYMOD_IF_ERR_RETURN(eagle2_tsc2pll_prbs_tx_inv_data_get(&phy_copy.access, &config_tmp.invert));
        PHYMOD_IF_ERR_RETURN(eagle2_tsc2pll_prbs_tx_poly_get(&phy_copy.access, &tsce_poly));
        PHYMOD_IF_ERR_RETURN(_tsce_dpll_prbs_poly_tsce_to_phymod(tsce_poly, &config_tmp.poly));
        prbs->invert = config_tmp.invert;
        prbs->poly = config_tmp.poly;
    } else if (PHYMOD_PRBS_DIRECTION_RX_GET(flags)) {
        PHYMOD_IF_ERR_RETURN(eagle2_tsc2pll_prbs_rx_inv_data_get(&phy_copy.access, &config_tmp.invert));
        PHYMOD_IF_ERR_RETURN(eagle2_tsc2pll_prbs_rx_poly_get(&phy_copy.access, &tsce_poly));
        PHYMOD_IF_ERR_RETURN(_tsce_dpll_prbs_poly_tsce_to_phymod(tsce_poly, &config_tmp.poly));
        prbs->invert = config_tmp.invert;
        prbs->poly = config_tmp.poly;
    } else {
        PHYMOD_IF_ERR_RETURN(eagle2_tsc2pll_prbs_tx_inv_data_get(&phy_copy.access, &config_tmp.invert));
        PHYMOD_IF_ERR_RETURN(eagle2_tsc2pll_prbs_tx_poly_get(&phy_copy.access, &tsce_poly));
        PHYMOD_IF_ERR_RETURN(_tsce_dpll_prbs_poly_tsce_to_phymod(tsce_poly, &config_tmp.poly));
        prbs->invert = config_tmp.invert;
        prbs->poly = config_tmp.poly;
    }
    return PHYMOD_E_NONE;

}


int tsce_dpll_phy_prbs_enable_set(const phymod_phy_access_t* phy, uint32_t flags , uint32_t enable)
{
    phymod_phy_access_t phy_copy;
    int start_lane, num_lane;
    int i = 0;


    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    TSCE_DPLL_PHY_ACCESS_PLLIDX_SET(&phy_copy);

    /*next figure out the lane num and start_lane based on the input*/
    PHYMOD_IF_ERR_RETURN
        (phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    if (PHYMOD_PRBS_DIRECTION_TX_GET(flags)) {
        for (i = 0; i < num_lane; i++) {
            phy_copy.access.lane_mask = 0x1 << (i + start_lane);
            PHYMOD_IF_ERR_RETURN(eagle2_tsc2pll_tx_prbs_en(&phy_copy.access, enable));
        }
    } else if (PHYMOD_PRBS_DIRECTION_RX_GET(flags)) {
        for (i = 0; i < num_lane; i++) {
            phy_copy.access.lane_mask = 0x1 << (i + start_lane);
            PHYMOD_IF_ERR_RETURN(eagle2_tsc2pll_rx_prbs_en(&phy_copy.access, enable));
        }
    } else {
        for (i = 0; i < num_lane; i++) {
            phy_copy.access.lane_mask = 0x1 << (i + start_lane);
            PHYMOD_IF_ERR_RETURN(eagle2_tsc2pll_tx_prbs_en(&phy_copy.access, enable));
            PHYMOD_IF_ERR_RETURN(eagle2_tsc2pll_rx_prbs_en(&phy_copy.access, enable));
        }
    }
    return PHYMOD_E_NONE;

}

int tsce_dpll_phy_prbs_enable_get(const phymod_phy_access_t* phy, uint32_t flags , uint32_t* enable)
{
    uint32_t enable_tmp;
    phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    TSCE_DPLL_PHY_ACCESS_PLLIDX_SET(&phy_copy);
    if (PHYMOD_PRBS_DIRECTION_TX_GET(flags)) {
        PHYMOD_IF_ERR_RETURN(eagle2_tsc2pll_prbs_tx_enable_get(&phy_copy.access, &enable_tmp));
        *enable = enable_tmp;
    } else if (PHYMOD_PRBS_DIRECTION_RX_GET(flags)) {
        PHYMOD_IF_ERR_RETURN(eagle2_tsc2pll_prbs_rx_enable_get(&phy_copy.access, &enable_tmp));
        *enable = enable_tmp;
    } else {
        PHYMOD_IF_ERR_RETURN(eagle2_tsc2pll_prbs_tx_enable_get(&phy_copy.access, &enable_tmp));
        *enable = enable_tmp;
        PHYMOD_IF_ERR_RETURN(eagle2_tsc2pll_prbs_rx_enable_get(&phy_copy.access, &enable_tmp));
        *enable &= enable_tmp;
    }

    return PHYMOD_E_NONE;

}


int tsce_dpll_phy_prbs_status_get(const phymod_phy_access_t* phy, uint32_t flags, phymod_prbs_status_t* prbs_status)
{
    uint8_t status = 0;
    uint32_t prbs_err_count = 0;
    int i, start_lane, num_lane;
    phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    TSCE_DPLL_PHY_ACCESS_PLLIDX_SET(&phy_copy);
    /* next figure out the lane num and start_lane based on the input */
    PHYMOD_IF_ERR_RETURN
        (phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    prbs_status->prbs_lock = 1;
    prbs_status->error_count = 0;
    prbs_status->prbs_lock_loss = 0;

    for (i = 0; i < num_lane; i++) {
        phy_copy.access.lane_mask = 0x1 << (i + start_lane);
        PHYMOD_IF_ERR_RETURN(eagle2_tsc2pll_prbs_chk_lock_state(&phy_copy.access, &status));
        if (status) {
            /*next check the lost of lock and error count */
            status = 0;
            PHYMOD_IF_ERR_RETURN
                (eagle2_tsc2pll_prbs_err_count_state(&phy_copy.access, &prbs_err_count, &status));
            PHYMOD_DEBUG_VERBOSE((" Lane :: %d PRBS Error count :: %d lock_loss=%0d\n", i, prbs_err_count, status));
            if (status) {
                /*temp lost of lock */
                prbs_status->prbs_lock_loss = 1;
            } else {
                prbs_status->error_count += prbs_err_count;
            }
        } else {
            PHYMOD_DEBUG_VERBOSE((" Lane :: %d PRBS not locked\n", i ));
            prbs_status->prbs_lock = 0;
            return PHYMOD_E_NONE;
        }
    }
    return PHYMOD_E_NONE;
}


int tsce_dpll_phy_pattern_config_set(const phymod_phy_access_t* phy, const phymod_pattern_t* pattern)
{
    int i,j, bit;
    phymod_phy_access_t phy_copy;
    char patt[PATTERN_MAX_LENGTH+1];

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    TSCE_DPLL_PHY_ACCESS_PLLIDX_SET(&phy_copy);
    for (i=0; i< PATTERN_MAX_SIZE; i++)
    {
      for (j=0;j<32; j++)
      {
        if (i*32+j == pattern->pattern_len)
           break;
        bit = pattern->pattern[i] >> j & 00000001;
        switch (bit) {
        case (1):
            patt[i*32+j]='1';
            break;
        case (0):
            patt[i*32+j] = '0';
            break;
       }
      }
       if (i*32+j == pattern->pattern_len)
       {
           if (i*32+j > PATTERN_MAX_LENGTH) {
               return PHYMOD_E_PARAM;
           }
           patt[i*32+j] = '\0';
           break;
       }

     }
    PHYMOD_IF_ERR_RETURN
            (eagle2_tsc2pll_config_shared_tx_pattern_idx_set(&phy_copy.access,
                                  &pattern->pattern_len ));
    PHYMOD_IF_ERR_RETURN
            (eagle2_tsc2pll_config_shared_tx_pattern (&phy_copy.access,
                                (uint8_t) pattern->pattern_len, (const char *) patt));

    return PHYMOD_E_NONE;

}

int tsce_dpll_phy_pattern_config_get(const phymod_phy_access_t* phy, phymod_pattern_t* pattern)
{
    phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    TSCE_DPLL_PHY_ACCESS_PLLIDX_SET(&phy_copy);
    PHYMOD_IF_ERR_RETURN
        (eagle2_tsc2pll_config_shared_tx_pattern_idx_get(&phy_copy.access,
                                  &pattern->pattern_len,
                                  pattern->pattern));

    return PHYMOD_E_NONE;

}


int tsce_dpll_phy_pattern_enable_set(const phymod_phy_access_t* phy, uint32_t enable, phymod_pattern_t* pattern)
{
    phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    TSCE_DPLL_PHY_ACCESS_PLLIDX_SET(&phy_copy);
    PHYMOD_IF_ERR_RETURN
        (eagle2_tsc2pll_tx_shared_patt_gen_en(&phy_copy.access, (uint8_t) enable, (uint8_t)pattern->pattern_len));

    return PHYMOD_E_NONE;

}

int tsce_dpll_phy_pattern_enable_get(const phymod_phy_access_t* phy, uint32_t* enable)
{
    phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    TSCE_DPLL_PHY_ACCESS_PLLIDX_SET(&phy_copy);
    PHYMOD_IF_ERR_RETURN
    (eagle2_tsc2pll_tx_shared_patt_gen_en_get(&phy_copy.access, (uint8_t *) enable));

    return PHYMOD_E_NONE;

}


int tsce_dpll_core_diagnostics_get(const phymod_core_access_t* core, phymod_core_diagnostics_t* diag)
{


    /* Place your code here */


    return PHYMOD_E_NONE;

}


int tsce_dpll_phy_pmd_info_dump(const phymod_phy_access_t* phy, void *arg)
{
    int start_lane, num_lane;
    int tmp_lane_mask;
    phymod_phy_access_t phy_copy;

    /* struct eagle_tsc_detailed_lane_status_st lane_st[4]; */
    int i = 0, special_mode_dsc = 0;
    char *cmd_str = (char*)arg;
    uint32_t type;
    uint8_t trace_mem[768];


    if (!cmd_str) {
        type = (uint32)TEMOD2PLL_DIAG_DSC;
    } else if((!PHYMOD_STRCMP(cmd_str,"TH"))) {
        type = (uint32)TEMOD2PLL_DIAG_DSC;
        special_mode_dsc = 1;
    } else if((!PHYMOD_STRCMP(cmd_str,"ber"))||(!PHYMOD_STRCMP(cmd_str,"Ber"))||(!PHYMOD_STRCMP(cmd_str,"BER"))) {
        type = (uint32)TEMOD2PLL_DIAG_BER;
    } else if((!PHYMOD_STRCMP(cmd_str,"config"))||(!PHYMOD_STRCMP(cmd_str,"Config"))||(!PHYMOD_STRCMP(cmd_str,"CONFIG"))) {
        type = (uint32)TEMOD2PLL_DIAG_CFG;
    } else if((!PHYMOD_STRCMP(cmd_str,"cl72"))||(!PHYMOD_STRCMP(cmd_str,"Cl72"))||(!PHYMOD_STRCMP(cmd_str,"CL72"))) {
        type = (uint32)TEMOD2PLL_DIAG_CL72;
    } else if((!PHYMOD_STRCMP(cmd_str,"debug"))||(!PHYMOD_STRCMP(cmd_str,"Debug"))||(!PHYMOD_STRCMP(cmd_str,"DEBUG"))) {
        type = (uint32)TEMOD2PLL_DIAG_DEBUG;
    } else if((!PHYMOD_STRCMP(cmd_str,"state"))||(!PHYMOD_STRCMP(cmd_str,"State"))||(!PHYMOD_STRCMP(cmd_str,"STATE"))) {
        type = (uint32)TEMOD2PLL_DIAG_STATE;
    } else if((!PHYMOD_STRCMP(cmd_str,"verbose"))||(!PHYMOD_STRCMP(cmd_str,"Verbose"))||(!PHYMOD_STRCMP(cmd_str,"VERBOSE"))) {
        type = (uint32)TEMOD2PLL_DIAG_ALL;
    } else if (!PHYMOD_STRCMP(cmd_str,"STD")){
        type = (uint32)TEMOD2PLL_DIAG_DSC_STD;
    } else {
        type = (uint32)TEMOD2PLL_DIAG_STATE;
    }

    PHYMOD_DEBUG_ERROR((" %s:%d type = %d laneMask  = 0x%X\n", __func__, __LINE__, type, phy->access.lane_mask));

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    TSCE_DPLL_PHY_ACCESS_PLLIDX_SET(&phy_copy);

    /*next figure out the lane num and start_lane based on the input*/
    PHYMOD_IF_ERR_RETURN
        (phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    if(type==TEMOD2PLL_DIAG_DSC) {
       PHYMOD_IF_ERR_RETURN
           (eagle2_tsc2pll_display_core_state(&phy_copy.access));
       PHYMOD_IF_ERR_RETURN
           (eagle2_tsc2pll_display_lane_state_hdr(&phy_copy.access));
       if (special_mode_dsc) {
            for (i = 0; i < num_lane; i++) {
                phy_copy.access.lane_mask = 0x1 << (i + start_lane);
                PHYMOD_IF_ERR_RETURN
                    (eagle2_tsc2pll_display_lane_state(&phy_copy.access));
            }
       } else {
           for (i = 0; i < 4; i++) {
                phy_copy.access.lane_mask = 0x1 << i ;
                PHYMOD_IF_ERR_RETURN
                    (eagle2_tsc2pll_display_lane_state(&phy_copy.access));
           }
       }
    } else if (type==TEMOD2PLL_DIAG_DSC_STD ) {
        PHYMOD_DIAG_OUT(("    +--------------------------------------------------------------------+\n"));
        PHYMOD_DIAG_OUT(("    | DSC Phy: 0x%03x lane_mask: 0x%02x                                 |\n", phy->access.addr, phy->access.lane_mask));
        PHYMOD_DIAG_OUT(("    +--------------------------------------------------------------------+\n"));
        PHYMOD_IF_ERR_RETURN
            (eagle2_tsc2pll_display_core_state(&phy_copy.access));
        PHYMOD_IF_ERR_RETURN
            (eagle2_tsc2pll_display_lane_state_hdr(&phy_copy.access));
        for (i = 0; i < num_lane; i++) {
                phy_copy.access.lane_mask = 0x1 << (i + start_lane);
                PHYMOD_IF_ERR_RETURN
                    (eagle2_tsc2pll_display_lane_state(&phy_copy.access));
            }
        /*in tscf there is falcon_tsc_log_full_pmd_state here,
          does not exist for eagle/tsce
          (Support should be added)*/
        PHYMOD_IF_ERR_RETURN
            (eagle2_tsc2pll_read_event_log((&phy_copy.access), trace_mem, 2));
    } else {
        for (i = 0; i < num_lane; i++) {
            phy_copy.access.lane_mask = 0x1 << (i + start_lane);

            switch(type) {
            case TEMOD2PLL_DIAG_CFG:
                PHYMOD_DEBUG_ERROR((" %s:%d type = CFG\n", __func__, __LINE__));
                if(i==0) {
                    tmp_lane_mask = phy_copy.access.lane_mask;
                    phy_copy.access.lane_mask = 1;
                    PHYMOD_IF_ERR_RETURN
                        (eagle2_tsc2pll_display_core_config(&phy_copy.access));
                    phy_copy.access.lane_mask = tmp_lane_mask;
                }
                PHYMOD_IF_ERR_RETURN
                    (eagle2_tsc2pll_display_lane_config(&phy_copy.access));
                break;

            case TEMOD2PLL_DIAG_CL72:
                PHYMOD_DEBUG_ERROR((" %s:%d type = CL72\n", __func__, __LINE__));
                PHYMOD_IF_ERR_RETURN
                    (eagle2_tsc2pll_display_cl72_status(&phy_copy.access));
                break;

            case TEMOD2PLL_DIAG_DEBUG:
                PHYMOD_DEBUG_ERROR((" %s:%d type = DBG\n", __func__, __LINE__));
                PHYMOD_IF_ERR_RETURN
                    (eagle2_tsc2pll_display_lane_debug_status(&phy_copy.access));
                break;

            case TEMOD2PLL_DIAG_BER:
                PHYMOD_DEBUG_ERROR((" %s:%d type = BER\n", __func__, __LINE__));
                break;

            /*
             * COVERITY
             *
             * TEFMOD_DIAG_ALL branch involve information in TEFMOD_DIAG_STATE branch
             */
            /* coverity[unterminated_case] */
            case TEMOD2PLL_DIAG_ALL:
                PHYMOD_DEBUG_ERROR((" %s:%d type = CFG\n", __func__, __LINE__));
                if(i==0) {
                    tmp_lane_mask = phy_copy.access.lane_mask;
                    phy_copy.access.lane_mask = 1;
                    PHYMOD_IF_ERR_RETURN
                        (eagle2_tsc2pll_display_core_config(&phy_copy.access));
                    phy_copy.access.lane_mask = tmp_lane_mask;
                }
                PHYMOD_IF_ERR_RETURN
                    (eagle2_tsc2pll_display_lane_config(&phy_copy.access));

                PHYMOD_DEBUG_ERROR((" %s:%d type = CL72\n", __func__, __LINE__));
                PHYMOD_IF_ERR_RETURN
                    (eagle2_tsc2pll_display_cl72_status(&phy_copy.access));

                PHYMOD_DEBUG_ERROR((" %s:%d type = DBG\n", __func__, __LINE__));
                PHYMOD_IF_ERR_RETURN
                    (eagle2_tsc2pll_display_lane_debug_status(&phy_copy.access));

            case TEMOD2PLL_DIAG_STATE:
            default:
                PHYMOD_DEBUG_ERROR((" %s:%d type = DEF\n", __func__, __LINE__));
                 PHYMOD_IF_ERR_RETURN
                    (eagle2_tsc2pll_display_core_state_hdr(&phy_copy.access));
                 PHYMOD_IF_ERR_RETURN
                    (eagle2_tsc2pll_display_core_state_line(&phy_copy.access));
                PHYMOD_IF_ERR_RETURN
                    (eagle2_tsc2pll_display_core_state(&phy_copy.access));
                PHYMOD_IF_ERR_RETURN
                    (eagle2_tsc2pll_display_lane_state_hdr(&phy_copy.access));
                PHYMOD_IF_ERR_RETURN
                    (eagle2_tsc2pll_display_lane_state(&phy_copy.access));
/*
                tmp_ln_msk = phy_copy.access.lane_mask;
                for (j = 0; j < 4; j++) {
                     phy_copy.access.lane_mask = 0x1 << j;
                     PHYMOD_IF_ERR_RETURN
                         (eagle_tsc_log_full_pmd_state(&phy_copy.access, &lane_st[j]));
                 }
                phy_copy.access.lane_mask = tmp_ln_msk;
                PHYMOD_IF_ERR_RETURN
                    (eagle_tsc_disp_full_pmd_state(&phy_copy.access, lane_st, 4));
*/
                PHYMOD_IF_ERR_RETURN
                    (eagle2_tsc2pll_read_event_log((&phy_copy.access), trace_mem, 2));
                break;
            }
        }
    }
    return PHYMOD_E_NONE;

}


int tsce_dpll_phy_pcs_info_dump(const phymod_phy_access_t* phy, void *arg)
{
    phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    TSCE_DPLL_PHY_ACCESS_PLLIDX_SET(&phy_copy);
    PHYMOD_IF_ERR_RETURN (temod2pll_diag_disp(&phy_copy.access, (char*)arg));
    return PHYMOD_E_NONE;

}


int tsce_dpll_phy_fec_correctable_counter_get(const phymod_phy_access_t* phy, uint32_t* count)
{


    /* Place your code here */
    PHYMOD_IF_ERR_RETURN(temod2pll_fec_correctable_counter_get(&phy->access, count));

    return PHYMOD_E_NONE;

}


int tsce_dpll_phy_fec_uncorrectable_counter_get(const phymod_phy_access_t* phy, uint32_t* count)
{


    /* Place your code here */
    PHYMOD_IF_ERR_RETURN(temod2pll_fec_uncorrectable_counter_get(&phy->access, count));

    return PHYMOD_E_NONE;

}


#endif /* PHYMOD_TSCE_DPLL_SUPPORT */
