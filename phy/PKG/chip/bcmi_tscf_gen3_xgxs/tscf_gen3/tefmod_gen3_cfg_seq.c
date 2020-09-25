/*----------------------------------------------------------------------
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * $Copyright: Copyright 2019 Broadcom Corporation.
 * This program is the proprietary software of Broadcom Corporation
 * and/or its licensors, and may only be used, duplicated, modified
 * or distributed pursuant to the terms and conditions of a separate,
 * written license agreement executed between you and Broadcom
 * (an "Authorized License").  Except as set forth in an Authorized
 * License, Broadcom grants no license (express or implied), right
 * to use, or waiver of any kind with respect to the Software, and
 * Broadcom expressly reserves all rights in and to the Software
 * and all intellectual property rights therein.  IF YOU HAVE
 * NO AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE
 * IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE
 * ALL USE OF THE SOFTWARE.  
 *  
 * Except as expressly set forth in the Authorized License,
 *  
 * 1.     This program, including its structure, sequence and organization,
 * constitutes the valuable trade secrets of Broadcom, and you shall use
 * all reasonable efforts to protect the confidentiality thereof,
 * and to use this information only in connection with your use of
 * Broadcom integrated circuit products.
 *  
 * 2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS
 * PROVIDED "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
 * REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY,
 * OR OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
 * DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
 * NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
 * ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
 * CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING
 * OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
 * 
 * 3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL
 * BROADCOM OR ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL,
 * INCIDENTAL, SPECIAL, INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER
 * ARISING OUT OF OR IN ANY WAY RELATING TO YOUR USE OF OR INABILITY
 * TO USE THE SOFTWARE EVEN IF BROADCOM HAS BEEN ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES; OR (ii) ANY AMOUNT IN EXCESS OF
 * THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF OR USD 1.00,
 * WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING
 * ANY FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.$
 * Broadcom Corporation
 * Proprietary and Confidential information
 * All rights reserved
 * This source file is the property of Broadcom Corporation, and
 * may not be copied or distributed in any isomorphic form without the
 * prior written consent of Broadcom Corporation.
 *---------------------------------------------------------------------
 * File       : tefmod_gen3_cfg_seq.c
 * Description: c functions implementing Tier1s for TEFMod Serdes Driver
 *---------------------------------------------------------------------*/
#define _SDK_TEFMOD_GEN3_ 1 

#include <phymod/phymod.h>
#include <phymod/phymod_types.h> 
#include <phymod/phymod_util.h>
#include <phymod/phymod_debug.h>
#include <phymod/chip/bcmi_tscf_gen3_xgxs_defs.h>
#include "tefmod_gen3_enum_defines.h"
#include "tefmod_gen3.h"
#include "tefmod_gen3_sc_lkup_table.h"

#ifdef _SDK_TEFMOD_GEN3_
#define PHYMOD_ST const phymod_access_t
#else
#define PHYMOD_ST tefmod_st
#endif

#ifdef _SDK_TEFMOD_GEN3_
#include "falcon2_monterey_enum.h"
#include "falcon2_monterey_interface.h"
#endif

#define TEFMOD_CL73_2P5GBASE_KX1_POS 11 
#define TEFMOD_CL73_5GBASE_KR1_POS   12 
#define TEFMOD_CL73_25GBASE_KR1_POS  10
#define TEFMOD_CL73_25GBASE_CR1_POS  10
#define TEFMOD_CL73_25GBASE_KRS1_POS  9
#define TEFMOD_CL73_25GBASE_CRS1_POS  9
#define TEFMOD_CL73_25GBASE_FEC_POS 24
#define TEFMOD_CL73_25G_RS_FEC_POS 23
#define TEFMOD_BAM_CL74_REQ 0x3
#define TEFMOD_BAM_CL91_REQ 0x3


#ifdef _SDK_TEFMOD_GEN3_
    #define TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc) \
        PHYMOD_VDBG(TEFMOD_GEN3_DBG_FUNC,pc,("-22%s: Adr:%08x Ln:%02d\n", __func__, (unsigned int)pc->addr, (int)pc->lane_mask))
    #define TEFMOD_GEN3_DBG_IN_FUNC_VIN_INFO(pc,_print_) \
        PHYMOD_VDBG(TEFMOD_GEN3_DBG_FUNCVALIN,pc,_print_)
    #define TEFMOD_GEN3_DBG_IN_FUNC_VOUT_INFO(pc,_print_) \
        PHYMOD_VDBG(TEFMOD_GEN3_DBG_FUNCVALOUT,pc,_print_)
#endif

/*!
@brief   This function reads TX-PLL PLL_LOCK bit.
@param   pc  handle to current TSCF context (#PHYMOD_ST)
@param   lockStatus reference which is updated with lock status.
@returns PHYMOD_E_NONE if no errors. PHYMOD_E_FAIL else.
@details Read PLL lock status. Returns  1 or 0 (locked/not)
*/

int tefmod_gen3_pmd_lock_get(PHYMOD_ST* pc, uint32_t* lockStatus)
{
    PMD_X4_STSr_t  reg_pmd_x4_sts;
    int i;
    phymod_access_t pa_copy;

    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
    *lockStatus = 1;
    PHYMOD_MEMCPY(&pa_copy, pc, sizeof(pa_copy));
   
    for(i=0; i<4; i++) {
        if(((pc->lane_mask >> i) & 0x1) == 1) {
            pa_copy.lane_mask = 0x1 << i;
            READ_PMD_X4_STSr(&pa_copy, &reg_pmd_x4_sts);
            *lockStatus = *lockStatus & PMD_X4_STSr_RX_LOCK_STSf_GET(reg_pmd_x4_sts);
        }
    }

    /* TEFMOD_GEN3_DBG_IN_FUNC_VOUT_INFO(pc,("PMD lockStatus: %d", *lockStatus)); */

    return PHYMOD_E_NONE;
}


/**
@brief   Init routine sets various operating modes of TSCF.
@param   pc handle to current TSCF context (#PHYMOD_ST)
@param   pmd_touched   Is this the first time we are visiting the PMD.
@param   spd_intf  Input of enum type #tefmod_spd_intfc_type_t
@param   pll_mode to override the pll div 
@returns PHYMOD_E_NONE if successful. PHYMOD_E_FAIL else.
@details

This function is called once per TSCF. It cannot be called multiple times
and should be called immediately after reset. Elements of #PHYMOD_ST should be

\li Set pll divider for VCO setting in PMD. pll divider is calculated from max_speed. 
*/

int tefmod_gen3_set_pll_mode(PHYMOD_ST* pc, int pmd_touched, tefmod_gen3_spd_intfc_type_t spd_intf, int pll_mode)
{
    PLL_CAL_CTL7r_t    reg_ctl7;
    int speed;

    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
    /* TEFMOD_GEN3_DBG_IN_FUNC_VIN_INFO(pc,("pmd_touched: %d, spd_intf: %d, pll_mode: %d", pmd_touched, spd_intf, pll_mode)); */

    PLL_CAL_CTL7r_CLR(reg_ctl7);
    if (pmd_touched == 0) {
        tefmod_gen3_get_mapped_speed(spd_intf, &speed);
        /*Support Override PLL DIV */
        if(pll_mode & 0x80000000) {
          PLL_CAL_CTL7r_PLL_MODEf_SET(reg_ctl7, (pll_mode) & 0x0000ffff);
        } else {
           PLL_CAL_CTL7r_PLL_MODEf_SET(reg_ctl7, (sc_pmd_dpll_entry[speed].pll_mode));
        }
        PHYMOD_IF_ERR_RETURN (MODIFY_PLL_CAL_CTL7r(pc, reg_ctl7));
    }

    return PHYMOD_E_NONE;

}

/*!
@brief   get  port speed id configured
@param   pc handle to current TSCF context (#PHYMOD_ST)
@param   speed_id Receives the resolved speed cfg in the speed_id
@returns The value PHYMOD_E_NONE upon successful completion.
@details get  port speed configured
*/
int tefmod_gen3_speed_id_get(PHYMOD_ST* pc, int *speed_id)
{
    SC_X4_RSLVD_SPDr_t sc_final_resolved_speed;

    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
    SC_X4_RSLVD_SPDr_CLR(sc_final_resolved_speed);
    READ_SC_X4_RSLVD_SPDr(pc,&sc_final_resolved_speed);
    *speed_id = SC_X4_RSLVD_SPDr_SPEEDf_GET(sc_final_resolved_speed);
    /* get_actual_speed(SC_X4_RSLVD_SPDr_SPEEDf_GET(sc_final_resolved_speed), spd_intf); */
    /* TEFMOD_GEN3_DBG_IN_FUNC_VOUT_INFO(pc,("speed_id: %d", *speed_id)); */

    return PHYMOD_E_NONE;
}

int tefmod_gen3_refclk_set(PHYMOD_ST* pc, phymod_ref_clk_t ref_clock)
{
    DIG_TOP_USER_CTL0r_t dig_top_user_reg;

    READ_DIG_TOP_USER_CTL0r(pc, &dig_top_user_reg);
    switch  (ref_clock) {
        case phymodRefClk156Mhz:
            DIG_TOP_USER_CTL0r_HEARTBEAT_COUNT_1USf_SET(dig_top_user_reg, 0x271);
            break;
        case phymodRefClk125Mhz:
            DIG_TOP_USER_CTL0r_HEARTBEAT_COUNT_1USf_SET(dig_top_user_reg, 0x1f4);
            break;
        default:
            DIG_TOP_USER_CTL0r_HEARTBEAT_COUNT_1USf_SET(dig_top_user_reg, 0x271);
            break;
    }
    MODIFY_DIG_TOP_USER_CTL0r(pc, dig_top_user_reg);

    return PHYMOD_E_NONE;
}

/**
@brief   Init the PMD
@param   pc handle to current TSCF context (#PHYMOD_ST)
@param   pmd_touched If the PMD is already initialized
@returns The value PHYMOD_E_NONE upon successful completion
@details Per core PMD resets (both datapath and entire core)
We only intend to use this function if the PMD has never been initialized.
*/
int tefmod_gen3_pmd_reset_seq(PHYMOD_ST* pc, int pmd_touched) /* PMD_RESET_SEQ */
{
    PMD_X1_CTLr_t PMD_X1_CTLr_reg;
    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
    /* TEFMOD_GEN3_DBG_IN_FUNC_VIN_INFO(pc,("pmd_touched: %x", pmd_touched)); */

    PMD_X1_CTLr_CLR(PMD_X1_CTLr_reg);

    if (pmd_touched == 0) {
        PMD_X1_CTLr_POR_H_RSTBf_SET(PMD_X1_CTLr_reg, 0);
        PMD_X1_CTLr_CORE_DP_H_RSTBf_SET(PMD_X1_CTLr_reg, 0);
        PHYMOD_IF_ERR_RETURN(WRITE_PMD_X1_CTLr(pc, PMD_X1_CTLr_reg));

        PMD_X1_CTLr_CORE_DP_H_RSTBf_SET(PMD_X1_CTLr_reg, 1);
        PMD_X1_CTLr_POR_H_RSTBf_SET(PMD_X1_CTLr_reg, 1);
        PHYMOD_IF_ERR_RETURN(WRITE_PMD_X1_CTLr(pc,PMD_X1_CTLr_reg));
    }
    return PHYMOD_E_NONE;
}

int tefmod_gen3_pmd_reset_seq_dp(PHYMOD_ST* pc, int pmd_touched) /* PMD_RESET_SEQ */
{
    CORE_PLL_TOP_USER_CTLr_t  CORE_PLL_TOP_USER_CTLr_reg; 

    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
    CORE_PLL_TOP_USER_CTLr_CLR(CORE_PLL_TOP_USER_CTLr_reg);

    if (pmd_touched == 0) {
        CORE_PLL_TOP_USER_CTLr_CORE_DP_S_RSTBf_SET(CORE_PLL_TOP_USER_CTLr_reg, 1);
        PHYMOD_IF_ERR_RETURN(MODIFY_CORE_PLL_TOP_USER_CTLr(pc,CORE_PLL_TOP_USER_CTLr_reg));
    }
    return PHYMOD_E_NONE;
}

/**
@brief   Checks config valid status for the port 
@param   pc handle to current TSCF context (#PHYMOD_ST)
@param   port_num Port Number
@returns The value PHYMOD_E_NONE upon successful completion
@details This register bit indicates that PCS is now programmed as required by
the HT entry for that speed
*/
int tefmod_gen3_master_port_num_set( PHYMOD_ST *pc,  int port_num)
{

    MAIN0_SETUPr_t main_reg;
    /* TEFMOD_GEN3_DBG_IN_FUNC_VIN_INFO(pc,("port_num: %d", port_num)); */

    MAIN0_SETUPr_CLR(main_reg);
    READ_MAIN0_SETUPr(pc, &main_reg);
    MAIN0_SETUPr_MASTER_PORT_NUMf_SET(main_reg, port_num);
    PHYMOD_IF_ERR_RETURN  
        (MODIFY_MAIN0_SETUPr(pc, main_reg));

    return PHYMOD_E_NONE;
}

/**
@brief   update the port mode 
@param   pc handle to current TSCF context (#PHYMOD_ST)
@param   pll_restart Receives info. on whether to restart pll.
@returns The value PHYMOD_E_NONE upon successful completion
*/
int tefmod_gen3_port_mode_select( PHYMOD_ST *pc)
{
    MAIN0_SETUPr_t mode_reg;
    int port_mode_sel = 0, port_mode_sel_value;
    uint32_t single_port_mode = 0;
    uint32_t first_couple_mode = 0, second_couple_mode = 0;

    READ_MAIN0_SETUPr(pc, &mode_reg);
    port_mode_sel_value = MAIN0_SETUPr_PORT_MODE_SELf_GET(mode_reg);

    if(pc->lane_mask == 0xf){
        port_mode_sel = 4;
    } else {
        first_couple_mode = ((port_mode_sel_value == 2) || (port_mode_sel_value == 3) || (port_mode_sel_value == 4));
        second_couple_mode = ((port_mode_sel_value == 1) || (port_mode_sel_value == 3) || (port_mode_sel_value == 4));
        switch(pc->lane_mask){
            case 1:
            case 2:
                first_couple_mode = 0;
                break;
            case 4:
            case 8:
                second_couple_mode = 0;
                break;
            case 3:
                first_couple_mode = 1;
                break;
            case 0xc:
                second_couple_mode = 1;
                break;
            default:
                /* dprintf("%-22s: ERROR port_mode_sel=%0d undefined\n", 
                                 __func__, port_mode_sel_reg); */
                break;
        }

        if(first_couple_mode ){
            port_mode_sel =(second_couple_mode)? 3: 2;
        } else if(second_couple_mode) {
            port_mode_sel = 1;
        } else{
            port_mode_sel = 0 ;
        }
    }

    /*if(pc->verbosity & TEFMOD_DBG_INIT)
        dprintf("%-22s u=%0d p=%0d port_mode_sel old=%0d new=%0d\n", __func__, 
                 pc->unit, pc->port, port_mode_sel_reg, port_mode_sel) ; */

    MAIN0_SETUPr_SINGLE_PORT_MODEf_SET(mode_reg, single_port_mode);
    MAIN0_SETUPr_PORT_MODE_SELf_SET(mode_reg, port_mode_sel);
    PHYMOD_IF_ERR_RETURN  
        (MODIFY_MAIN0_SETUPr(pc, mode_reg));
    /* TEFMOD_GEN3_DBG_IN_FUNC_VOUT_INFO(pc,("pll_restart: %d", *pll_restart)); */

    return PHYMOD_E_NONE ;
}

/**
@brief   Enable the pll reset bit
@param   pc handle to current TSCF context (#PHYMOD_ST)
@param   enable Controls whether to reset PLL
@returns The value PHYMOD_E_NONE upon successful completion
@details
Resets the PLL
*/
int tefmod_gen3_pll_reset_enable_set (PHYMOD_ST *pc, int enable)
{
    MAIN0_SPD_CTLr_t main_reg;

    MAIN0_SPD_CTLr_CLR(main_reg);
    MAIN0_SPD_CTLr_PLL_RESET_ENf_SET(main_reg, enable);
    PHYMOD_IF_ERR_RETURN(MODIFY_MAIN0_SPD_CTLr(pc, main_reg));

    return PHYMOD_E_NONE ;
}

/**
@brief   Read PCS Link status
@param   pc handle to current TSCF context (#PHYMOD_ST)
@param   *link Reference for Status of PCS Link
@returns The value PHYMOD_E_NONE upon successful completion
@details Return the status of the PCS link. The link up implies the PCS is able
to decode the digital bits coming in on the serdes. It automatically implies
that the PLL is stable and that the PMD sublayer has successfully recovered the
clock on the receive line.
*/
int tefmod_gen3_get_pcs_link_status(PHYMOD_ST* pc, uint32_t *link)
{
    RX_X4_PCS_LIVE_STS1r_t reg_pcs_live_sts;

    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
    RX_X4_PCS_LIVE_STS1r_CLR(reg_pcs_live_sts);
    PHYMOD_IF_ERR_RETURN(READ_RX_X4_PCS_LIVE_STS1r(pc, &reg_pcs_live_sts));
    *link = RX_X4_PCS_LIVE_STS1r_LINK_STATUSf_GET(reg_pcs_live_sts);
    /* TEFMOD_GEN3_DBG_IN_FUNC_VOUT_INFO(pc,("pcs_live_stats_link: %d", *link)); */

    return PHYMOD_E_NONE;
}

int tefmod_gen3_get_pcs_latched_link_status(PHYMOD_ST* pc, uint32_t *link)
{
    RX_X4_PCS_LIVE_STS1r_t reg_pcs_live_sts;
    RX_X4_PCS_LATCH_STS1r_t latched_sts ;
    int  latched_val ;

    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
    RX_X4_PCS_LIVE_STS1r_CLR(reg_pcs_live_sts);
    RX_X4_PCS_LATCH_STS1r_CLR(latched_sts) ;

    PHYMOD_IF_ERR_RETURN(READ_RX_X4_PCS_LIVE_STS1r(pc, &reg_pcs_live_sts));
    PHYMOD_IF_ERR_RETURN(READ_RX_X4_PCS_LATCH_STS1r(pc, &latched_sts)) ;
    latched_val = RX_X4_PCS_LATCH_STS1r_LINK_STATUS_LLf_GET(latched_sts) ;
    if(latched_val) {
        *link = 0 ;
    } else {
        *link = RX_X4_PCS_LIVE_STS1r_LINK_STATUSf_GET(reg_pcs_live_sts);
    }
    /* TEFMOD_GEN3_DBG_IN_FUNC_VOUT_INFO(pc,("pcs_live_stats_link: %d", *link)); */

    return PHYMOD_E_NONE;
}

/**
@brief   Get the Port status
@param   pc handle to current TSCF context (#PHYMOD_ST)
@param   disabled  Receives status on port disabledness
@returns The value PHYMOD_E_NONE upon successful completion
@details Ports can be disabled in several ways. In this function we simply write
0 to the speed change which will bring the PCS down for that lane.

*/
int tefmod_gen3_disable_get(PHYMOD_ST* pc, uint32_t* disabled)
{
    SC_X4_CTLr_t reg_sc_ctrl;

    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
    SC_X4_CTLr_CLR(reg_sc_ctrl);

    PHYMOD_IF_ERR_RETURN(READ_SC_X4_CTLr(pc,&reg_sc_ctrl));
    *disabled = SC_X4_CTLr_SW_SPEED_CHANGEf_GET(reg_sc_ctrl);

    return PHYMOD_E_NONE;
}
/**
@brief   Get info on Disable status of the Port
@param   pc handle to current TSCF context (#PHYMOD_ST)
@returns The value PHYMOD_E_NONE upon successful completion
@details Disables the port by writing 0 to the speed config logic in PCS.
This makes the PCS to bring down the PCS blocks and also apply lane datapath
reset to the PMD. There is no control input to this function since it only does
one thing.
*/
int tefmod_gen3_disable_set(PHYMOD_ST* pc)
{
    SC_X4_CTLr_t reg_sc_ctrl;

    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
    SC_X4_CTLr_CLR(reg_sc_ctrl);
    READ_SC_X4_CTLr(pc, &reg_sc_ctrl);

    /* write 0 to the speed change */
    SC_X4_CTLr_SW_SPEED_CHANGEf_SET(reg_sc_ctrl, 0);
    PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_CTLr(pc,reg_sc_ctrl));

    return PHYMOD_E_NONE;
}

int tefmod_gen3_enable_set(PHYMOD_ST* pc)
{
    SC_X4_CTLr_t reg_sc_ctrl;

    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
    SC_X4_CTLr_CLR(reg_sc_ctrl);
    READ_SC_X4_CTLr(pc, &reg_sc_ctrl);

    /* write 0 to the speed change */
    SC_X4_CTLr_SW_SPEED_CHANGEf_SET(reg_sc_ctrl, 1);
    PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_CTLr(pc,reg_sc_ctrl));

    return PHYMOD_E_NONE;
}


/**
@brief  Get the plldiv from lookup table
@param  pc handle to current TSCF context (#PHYMOD_ST)
@param  spd_intf  Input of enum type #tefmod_spd_intfc_type_t
@param  plldiv  Receives PLL Divider value
@returns The value PHYMOD_E_NONE upon successful completion
@details Get the plldiv from lookup table as a function of the speed.
*/

int tefmod_gen3_pll_div_lkup_get(PHYMOD_ST* pc, tefmod_gen3_spd_intfc_type_t spd_intf, phymod_ref_clk_t refclk, uint32_t* plldiv) 
{
    int speed_id;

    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
    tefmod_gen3_get_mapped_speed(spd_intf, &speed_id);
    if (refclk == phymodRefClk125Mhz) {
        *plldiv = sc_pmd_dpll_entry_125M_ref[speed_id].pll_mode;
    } else {
        *plldiv = sc_pmd_dpll_entry[speed_id].pll_mode;
    }
    /* TEFMOD_GEN3_DBG_IN_FUNC_VOUT_INFO(pc,("plldiv: %d", *plldiv)); */

    return PHYMOD_E_NONE;

}

int tefmod_gen3_pll_div_get(PHYMOD_ST* pc, uint32_t* pll0_div, uint32_t* pll1_div)
{
    phymod_access_t pc_copy;
    PLL_CAL_CTL7r_t PLL_CAL_CTL7r_reg;
    int start_lane, num_lane;

    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);

    /* Need to figure out what's the starting lane */
    PHYMOD_MEMCPY(&pc_copy, pc, sizeof(pc_copy));
    PHYMOD_IF_ERR_RETURN
        (phymod_util_lane_config_get(pc, &start_lane, &num_lane));
    pc_copy.lane_mask = 0x1 << start_lane;

    PLL_CAL_CTL7r_CLR(PLL_CAL_CTL7r_reg); 

    pc_copy.pll_idx = 0; 
    PHYMOD_IF_ERR_RETURN
        (READ_PLL_CAL_CTL7r(&pc_copy, &PLL_CAL_CTL7r_reg));
    *pll0_div = PLL_CAL_CTL7r_PLL_MODEf_GET(PLL_CAL_CTL7r_reg);

    pc_copy.pll_idx = 1; 
    PHYMOD_IF_ERR_RETURN
        (READ_PLL_CAL_CTL7r(&pc_copy, &PLL_CAL_CTL7r_reg));
    *pll1_div = PLL_CAL_CTL7r_PLL_MODEf_GET(PLL_CAL_CTL7r_reg);

    return PHYMOD_E_NONE;

}


/**
@brief   Get the osmode from lookup table
@param   pc handle to current TSCF context (#PHYMOD_ST)
@param   spd_intf  Input of enum type #tefmod_spd_intfc_type_t
@param   osmode Receives the OS mode as assumed in the hard table.
@returns The value PHYMOD_E_NONE upon successful completion
@details Get the osmode from software version of Speed table as a function of
the speed
*/
int tefmod_gen3_osmode_lkup_get(PHYMOD_ST* pc, tefmod_gen3_spd_intfc_type_t spd_intf, phymod_ref_clk_t refclk, uint32_t *osmode)
{
    int speed_id;

    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
    tefmod_gen3_get_mapped_speed(spd_intf, &speed_id);

    *osmode = sc_pmd_dpll_entry[speed_id].pma_os_mode;

    return PHYMOD_E_NONE;
}

/**
@brief   Gets PMD TX lane swap values for all lanes simultaneously.
@param   pc handle to current TSCF context (#PHYMOD_ST)
@param   tx_lane_map returns the pmd tx lane map
@returns The value PHYMOD_E_NONE upon successful completion
@details
This function gets the TX lane swap values for all lanes simultaneously.

*/
int tefmod_gen3_pmd_lane_swap_tx_rx_get ( PHYMOD_ST *pc, uint32_t *tx_lane_map, uint32_t *rx_lane_map)
{
    uint16_t tx_lane_map_0, tx_lane_map_1, tx_lane_map_2, tx_lane_map_3;
    uint16_t rx_lane_map_0, rx_lane_map_1, rx_lane_map_2, rx_lane_map_3;
    LN_ADDR0r_t ln0_reg;
    LN_ADDR1r_t ln1_reg;
    LN_ADDR2r_t ln2_reg;
    LN_ADDR3r_t ln3_reg;

    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
    LN_ADDR0r_CLR(ln0_reg);
    LN_ADDR1r_CLR(ln1_reg);
    LN_ADDR2r_CLR(ln2_reg);
    LN_ADDR3r_CLR(ln3_reg);

    PHYMOD_IF_ERR_RETURN
        (READ_LN_ADDR0r(pc, &ln0_reg));
    PHYMOD_IF_ERR_RETURN
        (READ_LN_ADDR1r(pc, &ln1_reg));
    PHYMOD_IF_ERR_RETURN
        (READ_LN_ADDR2r(pc, &ln2_reg));
    PHYMOD_IF_ERR_RETURN
        (READ_LN_ADDR3r(pc, &ln3_reg));

    tx_lane_map_0 = LN_ADDR0r_TX_LANE_ADDR_0f_GET(ln0_reg);
    rx_lane_map_0 = LN_ADDR0r_RX_LANE_ADDR_0f_GET(ln0_reg);
    tx_lane_map_1 = LN_ADDR1r_TX_LANE_ADDR_1f_GET(ln1_reg);
    rx_lane_map_1 = LN_ADDR1r_RX_LANE_ADDR_1f_GET(ln1_reg);
    tx_lane_map_2 = LN_ADDR2r_TX_LANE_ADDR_2f_GET(ln2_reg);
    rx_lane_map_2 = LN_ADDR2r_RX_LANE_ADDR_2f_GET(ln2_reg);
    tx_lane_map_3 = LN_ADDR3r_TX_LANE_ADDR_3f_GET(ln3_reg);
    rx_lane_map_3 = LN_ADDR3r_RX_LANE_ADDR_3f_GET(ln3_reg);

    *tx_lane_map = ((tx_lane_map_0 & 0xf) << 0)
              | ((tx_lane_map_1 & 0xf) << 4)
              | ((tx_lane_map_2 & 0xf) << 8)
              | ((tx_lane_map_3 & 0xf) << 12);

    *rx_lane_map = ((rx_lane_map_0 & 0xf) << 0)
              | ((rx_lane_map_1 & 0xf) << 4)
              | ((rx_lane_map_2 & 0xf) << 8)
              | ((rx_lane_map_3 & 0xf) << 12);

    return PHYMOD_E_NONE ;
}

/**
@brief   rx lane reset and enable of any particular lane
@param   pc handle to current TSCF context (#PHYMOD_ST)
@param   enable to reset the lane.
@returns The value PHYMOD_E_NONE upon successful completion.
@details
This function enables/disables rx lane (RSTB_LANE) or read back control bit for
that based on per_lane_control being 1 or 0. If per_lane_control is 0xa, only
read back RSTB_LANE.
*/
int tefmod_gen3_rx_lane_control_set(PHYMOD_ST* pc, int enable)         /* REG map not match */
{
    RX_X4_PMA_CTL0r_t    reg_pma_ctrl;

    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
    RX_X4_PMA_CTL0r_CLR(reg_pma_ctrl);
    if (enable) {
        RX_X4_PMA_CTL0r_RSTB_LANEf_SET( reg_pma_ctrl, 0);
        PHYMOD_IF_ERR_RETURN (MODIFY_RX_X4_PMA_CTL0r(pc, reg_pma_ctrl));
        RX_X4_PMA_CTL0r_RSTB_LANEf_SET(reg_pma_ctrl, 1);
        PHYMOD_IF_ERR_RETURN (MODIFY_RX_X4_PMA_CTL0r(pc, reg_pma_ctrl));
    } else {
        /* bit set to 0 for disabling RXP */
        RX_X4_PMA_CTL0r_RSTB_LANEf_SET( reg_pma_ctrl, 0);
        PHYMOD_IF_ERR_RETURN (MODIFY_RX_X4_PMA_CTL0r(pc, reg_pma_ctrl));
    }
    return PHYMOD_E_NONE;
}

/**
@brief   rx lane reset and enable of any particular lane
@param   pc handle to current TSCF context (#PHYMOD_ST)
@param   enable Handle to get the configured reset lane.
@returns The value PHYMOD_E_NONE upon successful completion.
@details
          This function read back control bit
*/
int tefmod_gen3_rx_lane_control_get(PHYMOD_ST* pc, int *enable)
{
    RX_X4_PMA_CTL0r_t    reg_pma_ctrl;

    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
    RX_X4_PMA_CTL0r_CLR(reg_pma_ctrl);
    PHYMOD_IF_ERR_RETURN (READ_RX_X4_PMA_CTL0r(pc, &reg_pma_ctrl));
    *enable =   RX_X4_PMA_CTL0r_RSTB_LANEf_GET( reg_pma_ctrl);

    return PHYMOD_E_NONE;
}



/**
@brief   Gets the TX And RX Polarity 
@param   pc handle to current TSCF context (#PHYMOD_ST)
@param   tx_polarity Receives the TX polarity
@param   rx_polarity Receives the RX polarity
@returns The value PHYMOD_E_NONE upon successful completion.
@details Gets the TX And RX Polarity from hardware.

*/
int tefmod_gen3_tx_rx_polarity_get ( PHYMOD_ST *pc, uint32_t* tx_polarity, uint32_t* rx_polarity)
{
    TLB_TX_TLB_TX_MISC_CFGr_t tx_pol_inv;
    TLB_RX_TLB_RX_MISC_CFGr_t rx_pol_inv;

    PHYMOD_IF_ERR_RETURN(READ_TLB_TX_TLB_TX_MISC_CFGr(pc, &tx_pol_inv));
    *tx_polarity = TLB_TX_TLB_TX_MISC_CFGr_TX_PMD_DP_INVERTf_GET(tx_pol_inv);

    PHYMOD_IF_ERR_RETURN(READ_TLB_RX_TLB_RX_MISC_CFGr(pc, &rx_pol_inv));
    *rx_polarity = TLB_RX_TLB_RX_MISC_CFGr_RX_PMD_DP_INVERTf_GET(rx_pol_inv);

    return PHYMOD_E_NONE;
}

/**
@brief   Sets the TX And RX Polarity 
@param   pc handle to current TSCF context (#PHYMOD_ST)
@param   tx_polarity Controls the TX polarity
@param   rx_polarity Controls the RX polarity
@returns The value PHYMOD_E_NONE upon successful completion.
@details Sets the TX And RX Polarity
*/
int tefmod_gen3_tx_rx_polarity_set ( PHYMOD_ST *pc, uint32_t tx_polarity, uint32_t rx_polarity)
{
    TLB_TX_TLB_TX_MISC_CFGr_t tx_pol_inv;
    TLB_RX_TLB_RX_MISC_CFGr_t rx_pol_inv;

    TLB_TX_TLB_TX_MISC_CFGr_CLR(tx_pol_inv);
    TLB_RX_TLB_RX_MISC_CFGr_CLR(rx_pol_inv);

    TLB_TX_TLB_TX_MISC_CFGr_TX_PMD_DP_INVERTf_SET(tx_pol_inv, tx_polarity);
    PHYMOD_IF_ERR_RETURN(MODIFY_TLB_TX_TLB_TX_MISC_CFGr(pc, tx_pol_inv));

    TLB_RX_TLB_RX_MISC_CFGr_RX_PMD_DP_INVERTf_SET(rx_pol_inv, rx_polarity);
    PHYMOD_IF_ERR_RETURN(MODIFY_TLB_RX_TLB_RX_MISC_CFGr(pc, rx_pol_inv));

    return PHYMOD_E_NONE;
}

/**
@brief   Trigger speed change.
@param   pc handle to current TSCF context (#PHYMOD_ST)
@returns The value PHYMOD_E_NONE upon successful completion
@details Per Port PCS Init
*/

int tefmod_gen3_trigger_speed_change(PHYMOD_ST* pc)
{
    SC_X4_CTLr_t    reg;

    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);

    /* write 0 to the speed change */
    SC_X4_CTLr_CLR(reg);
    PHYMOD_IF_ERR_RETURN(READ_SC_X4_CTLr(pc, &reg));
    SC_X4_CTLr_SW_SPEED_CHANGEf_SET(reg, 0);
    PHYMOD_IF_ERR_RETURN(WRITE_SC_X4_CTLr(pc, reg));

    /* write 1 to the speed change. No need to read again before write*/
    SC_X4_CTLr_SW_SPEED_CHANGEf_SET(reg, 1);
    PHYMOD_IF_ERR_RETURN(WRITE_SC_X4_CTLr(pc, reg));

    return PHYMOD_E_NONE;
}

/*!
@brief Squelch TX lane output.
@param  pc handle to current TSCF context (#PHYMOD_ST)
@param  tx control lanes to disable 
@returns PHYMOD_E_NONE if no errors. PHYMOD_EERROR else.
@details This function disables transmission on a specific lane. No reset is
required to restart the transmission. Lane control is done through 'tx' input.
Set bits 0, 8, 16, or 24 to <B>0 to disable TX on lanes 0/1/2/3</B>
Set the bits to <B>1 to enable TX</B>
*/

int tefmod_gen3_tx_lane_disable (PHYMOD_ST* pc, int tx)
{
    PMD_X4_CTLr_t PMD_X4_PMD_X4_CONTROLr_reg;

    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
    PMD_X4_CTLr_CLR(PMD_X4_PMD_X4_CONTROLr_reg);

    PMD_X4_CTLr_TX_DISABLEf_SET(PMD_X4_PMD_X4_CONTROLr_reg, tx);
    PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X4_CTLr(pc, PMD_X4_PMD_X4_CONTROLr_reg));

    return PHYMOD_E_NONE;
}

/**
@brief   Select the ILKN path and bypass TSCF PCS
@param   pc handle to current TSCF context (#PHYMOD_ST)
@returns The value PHYMOD_E_NONE upon successful completion
@details This will enable ILKN path. PCS is set to bypass to relinquish PMD
control. Expect PMD to be programmed elsewhere.
*/
int tefmod_gen3_pcs_ilkn_mode_set(PHYMOD_ST* pc)              
{
    SC_X4_BYPASSr_t reg_sc_bypass;

    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
    SC_X4_BYPASSr_CLR(reg_sc_bypass);
    SC_X4_BYPASSr_SC_BYPASSf_SET(reg_sc_bypass, 1);
    PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_BYPASSr(pc,reg_sc_bypass));

    return PHYMOD_E_NONE;
}


/**
@brief   Select the ILKN path and bypass TSCF PCS
@param   pc handle to current TSCF context (#PHYMOD_ST)
@returns The value PHYMOD_E_NONE upon successful completion
@details This will enable ILKN path. PCS is set to bypass to relinquish PMD
control. Expect PMD to be programmed elsewhere.
*/
int tefmod_gen3_init_pcs_ilkn(PHYMOD_ST* pc)            
{
    ILKN_CTL0r_t ILKN_CONTROL0r_reg;

    PHYMOD_IF_ERR_RETURN(READ_ILKN_CTL0r(pc, &ILKN_CONTROL0r_reg));
    ILKN_CTL0r_CREDIT_ENf_SET(ILKN_CONTROL0r_reg, 1);
    ILKN_CTL0r_ILKN_SELf_SET(ILKN_CONTROL0r_reg, 1);
    PHYMOD_IF_ERR_RETURN(MODIFY_ILKN_CTL0r(pc, ILKN_CONTROL0r_reg));

    return PHYMOD_E_NONE;
}

/**
@brief   Check if ILKN is set
@param   pc handle to current TSCF context (#PHYMOD_ST), ilkn_set status
@returns The value PHYMOD_E_NONE upon successful completion
@details Check if ILKN is set, ilkn_set = 1 if it is set.
*/
int tefmod_gen3_pcs_ilkn_chk(PHYMOD_ST* pc, int *ilkn_set)
{
    ILKN_CTL0r_t ILKN_CONTROL0r_reg;

    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
    PHYMOD_IF_ERR_RETURN(READ_ILKN_CTL0r(pc, &ILKN_CONTROL0r_reg));
    *ilkn_set = ILKN_CTL0r_ILKN_SELf_GET(ILKN_CONTROL0r_reg);

    return PHYMOD_E_NONE;
}

#ifdef _SDK_TEFMOD_GEN3_

int tefmod_gen3_autoneg_ability_set(PHYMOD_ST* pc, tefmod_gen3_an_adv_ability_t *cl73_adv)
{
    uint32_t override_v;
    AN_X4_LD_BASE_ABIL1r_t AN_X4_LD_BASE_ABIL1r_reg;
    AN_X4_LD_BASE_ABIL0r_t AN_X4_LD_BASE_ABILI0r_reg;
    AN_X4_LD_UP1_ABIL0r_t AN_X4_LD_UP1_ABIL0r_reg;
    AN_X4_LD_UP1_ABIL1r_t AN_X4_LD_UP1_ABIL1r_reg;
    AN_X4_LD_BASE_ABIL3r_t AN_X4_LD_BASE_ABIL3r_reg;
    AN_X4_LD_BASE_ABIL4r_t AN_X4_LD_BASE_ABIL4r_reg;
    AN_X4_LD_BASE_ABIL5r_t AN_X4_LD_BASE_ABIL5r_reg;
    AN_X4_LD_FEC_BASEPAGE_ABILr_t AN_X4_LD_FEC_BASEPAGE_ABILr_reg;
    int         adv100g = 0;

    AN_X4_LD_BASE_ABIL1r_CLR(AN_X4_LD_BASE_ABIL1r_reg);
    /******* Base Abilities 0xC1C4[5:0]****/
    AN_X4_LD_BASE_ABIL1r_SET(AN_X4_LD_BASE_ABIL1r_reg, cl73_adv->an_base_speed & 0x3f);

    /******* Pause Settings 0xC1C4[7:6]********/
    if (cl73_adv->an_pause == TEFMOD_NO_PAUSE){
        AN_X4_LD_BASE_ABIL1r_CL73_PAUSEf_SET(AN_X4_LD_BASE_ABIL1r_reg, 0);
    }
    if (cl73_adv->an_pause == TEFMOD_SYMM_PAUSE){
        AN_X4_LD_BASE_ABIL1r_CL73_PAUSEf_SET(AN_X4_LD_BASE_ABIL1r_reg, 1);
    }
    if (cl73_adv->an_pause == TEFMOD_ASYM_PAUSE){
        AN_X4_LD_BASE_ABIL1r_CL73_PAUSEf_SET(AN_X4_LD_BASE_ABIL1r_reg, 2);
    }
    if (cl73_adv->an_pause == TEFMOD_ASYM_SYMM_PAUSE){
        AN_X4_LD_BASE_ABIL1r_CL73_PAUSEf_SET(AN_X4_LD_BASE_ABIL1r_reg, 3);
    }
    /****** FEC Settings 0xC1C4[9:8]********/
    if (cl73_adv->an_fec == TEFMOD_FEC_NOT_SUPRTD){
        AN_X4_LD_BASE_ABIL1r_FEC_REQf_SET(AN_X4_LD_BASE_ABIL1r_reg, 0);
    }
    if(cl73_adv->an_fec == TEFMOD_FEC_SUPRTD_NOT_REQSTD){
        AN_X4_LD_BASE_ABIL1r_FEC_REQf_SET(AN_X4_LD_BASE_ABIL1r_reg, 1);
    }
    if (AN_X4_LD_BASE_ABIL1r_BASE_100G_CR4f_GET(AN_X4_LD_BASE_ABIL1r_reg) ||
        AN_X4_LD_BASE_ABIL1r_BASE_100G_KR4f_GET(AN_X4_LD_BASE_ABIL1r_reg)) {
        adv100g = 1;
    }
    if ((cl73_adv->an_fec & TEFMOD_FEC_CL74_SUPRTD_REQSTD) ||
       ((cl73_adv->an_fec & TEFMOD_FEC_CL91_SUPRTD_REQSTD) && adv100g)){
       AN_X4_LD_BASE_ABIL1r_FEC_REQf_SET(AN_X4_LD_BASE_ABIL1r_reg, 3);
    }

    /****** Setting AN_X4_ABILITIES_ld_base_abilities_1 0xC1C4 ********/
    PHYMOD_IF_ERR_RETURN(WRITE_AN_X4_LD_BASE_ABIL1r(pc, AN_X4_LD_BASE_ABIL1r_reg));

    AN_X4_LD_BASE_ABIL3r_CLR(AN_X4_LD_BASE_ABIL3r_reg);
    AN_X4_LD_BASE_ABIL4r_CLR(AN_X4_LD_BASE_ABIL4r_reg);
    AN_X4_LD_BASE_ABIL5r_CLR(AN_X4_LD_BASE_ABIL5r_reg);
    AN_X4_LD_FEC_BASEPAGE_ABILr_CLR(AN_X4_LD_FEC_BASEPAGE_ABILr_reg);
    AN_X4_LD_UP1_ABIL1r_CLR(AN_X4_LD_UP1_ABIL1r_reg);
    
    /*****  25G base speed abilities_3 0xC1C8 ******/
    if((cl73_adv->an_base_speed  >> TEFMOD_CL73_25GBASE_KR1 ) & 1) {
        AN_X4_LD_BASE_ABIL3r_BASE_25G_KR1_ENf_SET(AN_X4_LD_BASE_ABIL3r_reg, 1);
        /* IEEE bit location for KR1/Cr1 is 10 */ 
        AN_X4_LD_BASE_ABIL3r_BASE_25G_KR1_SELf_SET(AN_X4_LD_BASE_ABIL3r_reg, TEFMOD_CL73_25GBASE_KR1_POS);
    }
    if((cl73_adv->an_base_speed  >> TEFMOD_CL73_25GBASE_CR1 ) & 1) {
        AN_X4_LD_BASE_ABIL3r_BASE_25G_CR1_ENf_SET(AN_X4_LD_BASE_ABIL3r_reg, 1);
        /* IEEE bit location for KR1/Cr1 is 10 */ 
        AN_X4_LD_BASE_ABIL3r_BASE_25G_CR1_SELf_SET(AN_X4_LD_BASE_ABIL3r_reg, TEFMOD_CL73_25GBASE_CR1_POS);
    }     
    /*****  25G base speed abilities_4 0xC1C9 ******/
    if((cl73_adv->an_base_speed  >> TEFMOD_CL73_25GBASE_KRS1 ) & 1) {
        AN_X4_LD_BASE_ABIL4r_BASE_25G_KRS1_ENf_SET(AN_X4_LD_BASE_ABIL4r_reg, 1);
        /* IEEE bit location for KRS1/CrS1 is 9 */ 
        AN_X4_LD_BASE_ABIL4r_BASE_25G_KRS1_SELf_SET(AN_X4_LD_BASE_ABIL4r_reg, TEFMOD_CL73_25GBASE_KRS1_POS);
    }     
    if((cl73_adv->an_base_speed  >> TEFMOD_CL73_25GBASE_CRS1 ) & 1) {
        AN_X4_LD_BASE_ABIL4r_BASE_25G_CRS1_ENf_SET(AN_X4_LD_BASE_ABIL4r_reg, 1);
        /* IEEE bit location for KRS1/CrS1 is 9 */ 
        AN_X4_LD_BASE_ABIL4r_BASE_25G_CRS1_SELf_SET(AN_X4_LD_BASE_ABIL4r_reg, TEFMOD_CL73_25GBASE_CRS1_POS);
    }     
    /*****  2P5G base speed abilities_5 0xC1CA ******/
    if((cl73_adv->an_base_speed  >> TEFMOD_CL73_2P5GBASE_KX1 ) & 1) {
        AN_X4_LD_BASE_ABIL5r_BASE_2P5G_ENf_SET(AN_X4_LD_BASE_ABIL5r_reg, 1);
        AN_X4_LD_BASE_ABIL5r_BASE_2P5G_SELf_SET(AN_X4_LD_BASE_ABIL5r_reg, TEFMOD_CL73_2P5GBASE_KX1_POS);
    } 
    /*****  5G base speed abilities_5 0xC1CA ******/
    if((cl73_adv->an_base_speed  >> TEFMOD_CL73_5GBASE_KR1 ) & 1) {
        AN_X4_LD_BASE_ABIL5r_BASE_5P0G_ENf_SET(AN_X4_LD_BASE_ABIL5r_reg, 1);
        AN_X4_LD_BASE_ABIL5r_BASE_5P0G_SELf_SET(AN_X4_LD_BASE_ABIL5r_reg, TEFMOD_CL73_5GBASE_KR1_POS);
    } 
    PHYMOD_IF_ERR_RETURN(WRITE_AN_X4_LD_BASE_ABIL3r(pc, AN_X4_LD_BASE_ABIL3r_reg));
    PHYMOD_IF_ERR_RETURN(WRITE_AN_X4_LD_BASE_ABIL4r(pc, AN_X4_LD_BASE_ABIL4r_reg));
    PHYMOD_IF_ERR_RETURN(WRITE_AN_X4_LD_BASE_ABIL5r(pc, AN_X4_LD_BASE_ABIL5r_reg));

    /*
     * We always set the SEL bit for BASE_FEC (CL74 bits 11:07=24) and RS_FEC (CL91 bits 05:01 =23)
     * Irrespective of whether FEC74/FEC91 is requested or not.
     */
    AN_X4_LD_FEC_BASEPAGE_ABILr_BASE_R_FEC_REQ_SELf_SET(AN_X4_LD_FEC_BASEPAGE_ABILr_reg, TEFMOD_CL73_25GBASE_FEC_POS);
    AN_X4_LD_FEC_BASEPAGE_ABILr_RS_FEC_REQ_SELf_SET(AN_X4_LD_FEC_BASEPAGE_ABILr_reg, TEFMOD_CL73_25G_RS_FEC_POS);
    AN_X4_LD_FEC_BASEPAGE_ABILr_BASE_R_FEC_REQ_ENf_SET(AN_X4_LD_FEC_BASEPAGE_ABILr_reg, 0);
    AN_X4_LD_FEC_BASEPAGE_ABILr_RS_FEC_REQ_ENf_SET(AN_X4_LD_FEC_BASEPAGE_ABILr_reg, 0);

    /* Next we need to check which fec to enable */
    if (cl73_adv->an_fec & TEFMOD_FEC_CL74_SUPRTD_REQSTD) {
        AN_X4_LD_FEC_BASEPAGE_ABILr_BASE_R_FEC_REQ_ENf_SET(AN_X4_LD_FEC_BASEPAGE_ABILr_reg, 1);
    }
    if (cl73_adv->an_fec & TEFMOD_FEC_CL91_SUPRTD_REQSTD) {
        AN_X4_LD_FEC_BASEPAGE_ABILr_RS_FEC_REQ_ENf_SET(AN_X4_LD_FEC_BASEPAGE_ABILr_reg, 1);
    }

    PHYMOD_IF_ERR_RETURN(MODIFY_AN_X4_LD_FEC_BASEPAGE_ABILr(pc, AN_X4_LD_FEC_BASEPAGE_ABILr_reg));

    /****** Base selector 0xC1C3[4:0]*****/
    AN_X4_LD_BASE_ABIL0r_CLR(AN_X4_LD_BASE_ABILI0r_reg);
    AN_X4_LD_BASE_ABIL0r_CL73_BASE_SELECTORf_SET(AN_X4_LD_BASE_ABILI0r_reg, 1);
    PHYMOD_IF_ERR_RETURN(MODIFY_AN_X4_LD_BASE_ABIL0r(pc, AN_X4_LD_BASE_ABILI0r_reg));

    /****** User page abilities_0 0xC1C1[3:0] and 0xC1C1[9:6]******/
    AN_X4_LD_UP1_ABIL0r_CLR(AN_X4_LD_UP1_ABIL0r_reg);
    AN_X4_LD_UP1_ABIL0r_SET(AN_X4_LD_UP1_ABIL0r_reg, cl73_adv->an_bam_speed & 0x3cf);
    PHYMOD_IF_ERR_RETURN(WRITE_AN_X4_LD_UP1_ABIL0r(pc, AN_X4_LD_UP1_ABIL0r_reg));

    /****** User page abilities_0 0xC1C1[15] ******/
    AN_X4_LD_UP1_ABIL0r_BAM_HG2f_SET(AN_X4_LD_UP1_ABIL0r_reg, cl73_adv->an_hg2);
    PHYMOD_IF_ERR_RETURN(MODIFY_AN_X4_LD_UP1_ABIL0r(pc, AN_X4_LD_UP1_ABIL0r_reg));

    /****** User page abilities_1 0xC1C2[4:1] ******/
    AN_X4_LD_UP1_ABIL1r_CLR(AN_X4_LD_UP1_ABIL1r_reg);
    AN_X4_LD_UP1_ABIL1r_SET(AN_X4_LD_UP1_ABIL1r_reg, cl73_adv->an_bam_speed1 & 0x1e);
    PHYMOD_IF_ERR_RETURN(MODIFY_AN_X4_LD_UP1_ABIL1r(pc, AN_X4_LD_UP1_ABIL1r_reg));

    /* Next check if BAM speed */
    /****** User page abilities_1 0xC1C2[15:14] and 0xC1C2[13:12] ******/
    if ((cl73_adv->an_bam_speed) || (cl73_adv->an_bam_speed1)) {
        AN_X4_LD_UP1_ABIL1r_CLR(AN_X4_LD_UP1_ABIL1r_reg);
        PHYMOD_IF_ERR_RETURN(READ_AN_X4_LD_UP1_ABIL1r(pc, &AN_X4_LD_UP1_ABIL1r_reg));
        /* Enable LD's 25G 50G CL74*/
        AN_X4_LD_UP1_ABIL1r_CL74_REQf_SET(AN_X4_LD_UP1_ABIL1r_reg, 0x1);
        /* Enable LD's 25G 50G CL91*/
        AN_X4_LD_UP1_ABIL1r_CL91_REQf_SET(AN_X4_LD_UP1_ABIL1r_reg, 0x1);
        if (cl73_adv->an_fec & TEFMOD_FEC_CL74_SUPRTD_REQSTD) {
            AN_X4_LD_UP1_ABIL1r_CL74_REQf_SET(AN_X4_LD_UP1_ABIL1r_reg, TEFMOD_BAM_CL74_REQ);
        }
        if (cl73_adv->an_fec & TEFMOD_FEC_CL91_SUPRTD_REQSTD) {
            AN_X4_LD_UP1_ABIL1r_CL91_REQf_SET(AN_X4_LD_UP1_ABIL1r_reg, TEFMOD_BAM_CL91_REQ);
        }
        PHYMOD_IF_ERR_RETURN(MODIFY_AN_X4_LD_UP1_ABIL1r(pc, AN_X4_LD_UP1_ABIL1r_reg));
    }

    
    if((cl73_adv->an_cl72 & 0x1) == 1) {
         override_v = ((OVERRIDE_CL72_EN_DIS<<16) | 1);
         tefmod_gen3_set_override_1(pc, 0, override_v);
    } else {
         override_v = ((OVERRIDE_CL72_EN<<16) | 0);
         tefmod_gen3_set_override_1(pc, 0, override_v);
    }

    return PHYMOD_E_NONE;
}

#endif /* _SDK_TEFMOD_GEN3_ */


/*!
@brief To get autoneg advertisement registers.
@param  pc handle to current TSCF context (#PHYMOD_ST)
@returns PHYMOD_E_NONE if no errors. PHYMOD_EERROR else.
@details
*/
int tefmod_gen3_autoneg_get(PHYMOD_ST* pc)
{
  TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
  /* TBD */
  return PHYMOD_E_NONE;
}

/*!
@brief   Controls the setting/resetting of autoneg ability and enabling/disabling
@param   pc handle to current TSCF context (#PHYMOD_ST)
@param   an_control handle to TSCF an control struct #tefmod_an_control_t
@returns PHYMOD_E_NONE if no errors. PHYMOD_EERROR else.

@details
This function programs auto-negotiation (AN) modes for the TEF. It can
enable/disable clause37/clause73/BAM autonegotiation capabilities. Call this
function once for combo mode and once per lane in independent lane mode.

The autonegotiation mode is indicated by setting an_control as required.
*/


int tefmod_gen3_autoneg_control(PHYMOD_ST* pc, tefmod_gen3_an_control_t *an_control)
{
    phymod_access_t pa_copy;
    uint16_t num_advertised_lanes, cl73_bam_enable  ;
    int start_lane, num_of_lane, i;
    uint16_t cl73_hpam_enable, cl73_enable;
    uint16_t cl73_next_page;
    uint16_t cl73_restart ;
    uint16_t cl73_bam_code;
    uint16_t msa_overrides;
    uint32_t pll0_div = 0, pll1_div = 0;
    int  pll_select = 0;
    MAIN0_SETUPr_t  reg_setup;
    tefmod_gen3_an_adv_ability_t value;
 
    AN_X4_CL73_CFGr_t      AN_X4_CL73_CFGr_reg;
    AN_X1_CL73_ERRr_t      AN_X1_CL73_ERRr_reg;
    AN_X4_CL73_CTLSr_t     AN_X4_CL73_CTLSr_reg;
    AN_X4_LD_BASE_ABIL1r_t AN_X4_LD_BASE_ABIL1r_reg;
    AN_X4_LD_BAM_ABILr_t   AN_X4_ABILITIES_LD_BAM_ABILITIESr_reg;

    PHYMOD_IF_ERR_RETURN
        (phymod_util_lane_config_get(pc, &start_lane, &num_of_lane));
    PHYMOD_MEMCPY(&pa_copy, pc, sizeof(pa_copy));

    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
    num_advertised_lanes          = an_control->num_lane_adv;
    cl73_bam_code                 = 0x0;
    cl73_bam_enable               = 0x0;
    cl73_hpam_enable              = 0x0;
    cl73_enable                   = 0x0;
    cl73_next_page                = 0x0;
    cl73_restart                  = 0x0;
    msa_overrides                 = 0x0;

    switch (an_control->an_type) {
        case TEFMOD_AN_MODE_CL73:
            cl73_restart                = an_control->enable;
            cl73_enable                 = an_control->enable;
            break;
        case TEFMOD_AN_MODE_CL73BAM:
            cl73_restart                = an_control->enable;
            cl73_enable                 = an_control->enable;
            cl73_bam_enable             = an_control->enable;
            cl73_bam_code               = 0x3;
            cl73_next_page              = 0x1;
            break;
        case TEFMOD_AN_MODE_CL73_MSA:
            cl73_restart                = an_control->enable;
            cl73_enable                 = an_control->enable;
            cl73_bam_enable             = an_control->enable;
            cl73_bam_code               = 0x3;
            cl73_next_page              = 0x1;
            msa_overrides               = 0x1;
            break;
        case TEFMOD_AN_MODE_MSA:
            cl73_restart                = an_control->enable;
            cl73_enable                 = an_control->enable;
            cl73_bam_enable             = an_control->enable;
            cl73_bam_code               = 0x3;
            cl73_next_page              = 0x1;
            msa_overrides               = 0x1;
            break;
        case TEFMOD_AN_MODE_HPAM:
            cl73_restart                = an_control->enable;
            cl73_enable                 = an_control->enable;
            cl73_hpam_enable            = an_control->enable;
            /* cl73_bam_code               = 0x4; */
            cl73_next_page              = 0x1;
            break;
        default:
            return PHYMOD_E_FAIL;
            break;
    }

    if(msa_overrides == 0x1) {
        tefmod_gen3_an_oui_t oui;
        oui.oui = 0x6a737d;
        oui.oui_override_bam73_adv = 0x1;
        oui.oui_override_bam73_det = 0x1;
        oui.oui_override_hpam_adv  = 0x0;
        oui.oui_override_hpam_det  = 0x0;
        PHYMOD_IF_ERR_RETURN(tefmod_gen3_an_oui_set(pc, oui));
        PHYMOD_IF_ERR_RETURN(tefmod_gen3_an_msa_mode_set(pc, msa_overrides));
    }
    /* RESET Speed Change bit */
    if(an_control->enable){
        tefmod_gen3_disable_set(pc);
    }

    MAIN0_SETUPr_CLR(reg_setup);
    PHYMOD_MEMSET(&value, 0x0, sizeof(value));
    PHYMOD_IF_ERR_RETURN
        (tefmod_gen3_autoneg_ability_get(pc, &value));
    /* Get current pll_mode and pll_select */
    PHYMOD_IF_ERR_RETURN(tefmod_gen3_pll_div_get(&pa_copy, &pll0_div, &pll1_div));
    PHYMOD_IF_ERR_RETURN(tefmod_gen3_pll_select_get(pc, &pll_select));        

    if(an_control->enable){
        /* HIGIG mode, PLL0 should be programmed to 20G for page exchange When HG speed is advertised */
        if (value.an_hg2) {
            PHYMOD_IF_ERR_RETURN
                (tefmod_gen3_pll_select_set(pc, 1));
            PHYMOD_IF_ERR_RETURN
                (tefmod_gen3_pll_idx_set(&pa_copy, 0));
            PHYMOD_IF_ERR_RETURN
                (tefmod_gen3_configure_pll(&pa_copy, phymod_TSCF_PLL_DIV132, phymodRefClk156Mhz));
        } else {
            /* 
             * Eth port of mix mode, only PLL1 is used, VCO is either 25G or 20G
             * IEEE mode, PLL0 is 20G and PLL1 is 25G
             */
            if(pll_select == 1){
                if(pll1_div == phymod_TSCF_PLL_DIV132){
                    MAIN0_SETUPr_CL73_VCOf_SET(reg_setup, 0);
                } 
                if(pll1_div == phymod_TSCF_PLL_DIV165){
                    MAIN0_SETUPr_CL73_VCOf_SET(reg_setup, 1);
                }
            } else {
                MAIN0_SETUPr_CL73_VCOf_SET(reg_setup, 0);
            }
            PHYMOD_IF_ERR_RETURN(MODIFY_MAIN0_SETUPr(pc, reg_setup));
        }
    }

    /* Timer for the amount of time tot wait to receive a page from the link partner */
    AN_X1_CL73_ERRr_CLR(AN_X1_CL73_ERRr_reg);
    if(an_control->an_type == (tefmod_gen3_an_mode_type_t)TEFMOD_AN_MODE_CL73) {
        AN_X1_CL73_ERRr_SET(AN_X1_CL73_ERRr_reg, 0);
    } else if(an_control->an_type == (tefmod_gen3_an_mode_type_t)TEFMOD_AN_MODE_HPAM){
        AN_X1_CL73_ERRr_SET(AN_X1_CL73_ERRr_reg, 0xfff0);
    } else if (an_control->an_type == (tefmod_gen3_an_mode_type_t)TEFMOD_AN_MODE_CL73BAM) {
        AN_X1_CL73_ERRr_SET(AN_X1_CL73_ERRr_reg, 0x1a10);
    }
    PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_CL73_ERRr(pc, AN_X1_CL73_ERRr_reg));

    /* Set cl73 next page probably*/
    AN_X4_LD_BASE_ABIL1r_CLR(AN_X4_LD_BASE_ABIL1r_reg);
    AN_X4_LD_BASE_ABIL1r_NEXT_PAGEf_SET(AN_X4_LD_BASE_ABIL1r_reg, cl73_next_page & 1);
    PHYMOD_IF_ERR_RETURN(MODIFY_AN_X4_LD_BASE_ABIL1r(pc, AN_X4_LD_BASE_ABIL1r_reg));

    /* Writing bam_code */
    AN_X4_LD_BAM_ABILr_CLR(AN_X4_ABILITIES_LD_BAM_ABILITIESr_reg);
    AN_X4_LD_BAM_ABILr_CL73_BAM_CODEf_SET(AN_X4_ABILITIES_LD_BAM_ABILITIESr_reg, cl73_bam_code);
    PHYMOD_IF_ERR_RETURN(WRITE_AN_X4_LD_BAM_ABILr(pc, AN_X4_ABILITIES_LD_BAM_ABILITIESr_reg));

    /*  Set 1G/2.5G PD function 0xC1C6 */
    AN_X4_CL73_CTLSr_CLR(AN_X4_CL73_CTLSr_reg);
    AN_X4_CL73_CTLSr_PD_KX_ENf_SET(AN_X4_CL73_CTLSr_reg, an_control->pd_kx_en);
    AN_X4_CL73_CTLSr_PD_2P5G_KX_ENf_SET(AN_X4_CL73_CTLSr_reg, an_control->pd_2P5G_kx_en);
    PHYMOD_IF_ERR_RETURN(MODIFY_AN_X4_CL73_CTLSr(pc, AN_X4_CL73_CTLSr_reg));

    AN_X4_CL73_CFGr_CLR(AN_X4_CL73_CFGr_reg);
    AN_X4_CL73_CFGr_CL73_ENABLEf_SET(AN_X4_CL73_CFGr_reg, 0);
    AN_X4_CL73_CFGr_CL73_AN_RESTARTf_SET(AN_X4_CL73_CFGr_reg, 0);
    PHYMOD_IF_ERR_RETURN (MODIFY_AN_X4_CL73_CFGr(pc, AN_X4_CL73_CFGr_reg));

    /* Set AN X4 abilities 0xC1C0 */
    AN_X4_CL73_CFGr_CLR(AN_X4_CL73_CFGr_reg);
    AN_X4_CL73_CFGr_CL73_BAM_ENABLEf_SET(AN_X4_CL73_CFGr_reg,cl73_bam_enable);
    AN_X4_CL73_CFGr_CL73_HPAM_ENABLEf_SET(AN_X4_CL73_CFGr_reg,cl73_hpam_enable);
    AN_X4_CL73_CFGr_CL73_ENABLEf_SET(AN_X4_CL73_CFGr_reg, cl73_enable);
    AN_X4_CL73_CFGr_CL73_AN_RESTARTf_SET(AN_X4_CL73_CFGr_reg,cl73_restart);
    if (an_control->an_property_type & TEFMOD_AN_PROPERTY_ENABLE_HPAM_TO_CL73_AUTO){
        AN_X4_CL73_CFGr_AD_TO_CL73_ENf_SET(AN_X4_CL73_CFGr_reg, 0x1);
    } else {
        AN_X4_CL73_CFGr_AD_TO_CL73_ENf_SET(AN_X4_CL73_CFGr_reg, 0x0);
        /* AN_X4_CL73_CFGr_HPAM_TO_CL73_AUTO_ENABLEf_SET(AN_X4_CL73_CFGr_reg,0x0); */
    }
    if (an_control->an_property_type & TEFMOD_AN_PROPERTY_ENABLE_CL73_BAM_TO_HPAM_AUTO){
        AN_X4_CL73_CFGr_BAM_TO_HPAM_AD_ENf_SET(AN_X4_CL73_CFGr_reg, 0x1);
    } else {
        AN_X4_CL73_CFGr_BAM_TO_HPAM_AD_ENf_SET(AN_X4_CL73_CFGr_reg, 0x0);
    }
    AN_X4_CL73_CFGr_NUM_ADVERTISED_LANESf_SET(AN_X4_CL73_CFGr_reg,num_advertised_lanes);
    PHYMOD_IF_ERR_RETURN (MODIFY_AN_X4_CL73_CFGr(pc, AN_X4_CL73_CFGr_reg));

    /* if AN is enabled, the restart bit needs to be cleared */
    if (an_control->enable) {
        AN_X4_CL73_CFGr_CLR(AN_X4_CL73_CFGr_reg);
        AN_X4_CL73_CFGr_CL73_AN_RESTARTf_SET(AN_X4_CL73_CFGr_reg, 0);
        PHYMOD_IF_ERR_RETURN (MODIFY_AN_X4_CL73_CFGr(pc, AN_X4_CL73_CFGr_reg));
    }
  
    /* Disable the cl72 , when AN port is disabled. */
    if (an_control->cl72_config_allowed) {
        if(an_control->enable == 0) {
            if(an_control->num_lane_adv == 1) {
                num_of_lane = 2;
            } else if (an_control->num_lane_adv == 2) {
                num_of_lane = 4;
            } else {
                num_of_lane = 1;
            }
            for (i = (num_of_lane-1); i >= 0; i--) {
                pa_copy.lane_mask = 0x1 << (i + start_lane);
                tefmod_gen3_clause72_control(&pa_copy, 0);
            }
        }
    }
    return PHYMOD_E_NONE;
} /* temod_autoneg_control */


/**
@brief   getRevDetails , calls revid_read
@param   pc handle to current TSCF context (#PHYMOD_ST)
@returns The value PHYMOD_E_NONE upon successful completion
@details
*/
/* internal function */
STATIC
int _tefmod_gen3_getRevDetails(PHYMOD_ST* pc)
{
    MAIN0_SERDESIDr_t MAIN0_SERDESIDr_reg;

    MAIN0_SERDESIDr_CLR(MAIN0_SERDESIDr_reg);
    PHYMOD_IF_ERR_RETURN(READ_MAIN0_SERDESIDr(pc, &MAIN0_SERDESIDr_reg));
    return MAIN0_SERDESIDr_GET(MAIN0_SERDESIDr_reg);
}


/* to update port_mode_select value.  If the update warrants a pll reset,
   then return accData=1, otherwise return accData=0.  The update shall 
   support flex port technology. Called forced speed modes */

int tefmod_gen3_update_port_mode_select(PHYMOD_ST* pc, tefmod_gen3_port_type_t port_type, 
					int master_port, int tsc_clk_freq_pll_by_48, int pll_reset_en)
{   
    MAIN0_SETUPr_t MAIN0_SETUPr_reg;
    MAIN0_SPD_CTLr_t MAIN0_SPD_CTLr_reg;

    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);

    PHYMOD_IF_ERR_RETURN (READ_MAIN0_SETUPr(pc, &MAIN0_SETUPr_reg));
  
    if((port_type == TEFMOD_MULTI_PORT) ||(port_type == TEFMOD_DXGXS)||
       (port_type == TEFMOD_SINGLE_PORT)||(port_type == TEFMOD_TRI1_PORT)||
       (port_type == TEFMOD_TRI2_PORT)){
    } else {
        PHYMOD_DEBUG_ERROR(("%-22s: ERROR port_type=%0d undefined\n", __func__, port_type));
        return PHYMOD_E_FAIL;
    }

    MAIN0_SETUPr_CLR(MAIN0_SETUPr_reg);
    MAIN0_SETUPr_PORT_MODE_SELf_SET(MAIN0_SETUPr_reg, port_type);
    /* Leave port mode to quad in an_en */
       
    MAIN0_SETUPr_MASTER_PORT_NUMf_SET(MAIN0_SETUPr_reg, master_port);
    PHYMOD_IF_ERR_RETURN (MODIFY_MAIN0_SETUPr(pc, MAIN0_SETUPr_reg));
    PHYMOD_IF_ERR_RETURN(READ_MAIN0_SETUPr(pc, &MAIN0_SETUPr_reg));
    

    MAIN0_SETUPr_CLR(MAIN0_SETUPr_reg);

    MAIN0_SETUPr_TSC_CLK_CTRLf_SET(MAIN0_SETUPr_reg, tsc_clk_freq_pll_by_48);
    
    PHYMOD_IF_ERR_RETURN(MODIFY_MAIN0_SETUPr(pc, MAIN0_SETUPr_reg));

    PHYMOD_IF_ERR_RETURN(READ_MAIN0_SETUPr(pc, &MAIN0_SETUPr_reg));

    MAIN0_SPD_CTLr_CLR(MAIN0_SPD_CTLr_reg);
    MAIN0_SPD_CTLr_PLL_RESET_ENf_SET(MAIN0_SPD_CTLr_reg, pll_reset_en);
    PHYMOD_IF_ERR_RETURN(MODIFY_MAIN0_SPD_CTLr(pc, MAIN0_SPD_CTLr_reg));

    return PHYMOD_E_NONE; 
}

/*!
@brief Init routine sets various operating modes of TEF.
@param  pc handle to current TSCF context (#PHYMOD_ST)
@param  refclk  Reference clock
@param  plldiv  PLL Divider value
@param  port_type Port type as in enum #tefmod_port_type_t
@param  master_port master port (which controls the PLL)
@param  master_port master port (which controls the PLL)
@param  tsc_clk_freq_pll_by_48  TBD
@param  pll_reset_en  TBD
@returns PHYMOD_E_NONE if successful. PHYMOD_E_ERROR else.
@details
This function is called once per TEF. It cannot be called multiple times
and should be called immediately after reset. Elements of #PHYMOD_ST should be
initialized to required values prior to calling this function. The following
sub-configurations are performed here.

\li Read the revision Id.
\li Set reference clock (0x9000[15:13])
\li Set pll divider for VCO setting (0x9000[12, 11:8]). This is a function of max_speed. 
\li Port mode select single/dual/quad combo lane mode
\li PMA/PMD mux/demux (lane swap) (0x9004[15:0])
\li Load Firmware. (In MDK/SDK this is done externally. The platform provides a
method to load firmware. TEFMod cannot load firmware via MDIO.)
*/



int tefmod_gen3_set_port_mode(PHYMOD_ST* pc, int refclk, int plldiv, tefmod_gen3_port_type_t port_type, 
				int master_port, int tsc_clk_freq_pll_by_48, int pll_reset_en)
{
    uint16_t dataref;
    MAIN0_SETUPr_t MAIN0_SETUPr_reg;

    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
    tefmod_gen3_update_port_mode_select(pc, port_type, master_port, tsc_clk_freq_pll_by_48, pll_reset_en);

    switch(refclk) {
        case 25 : dataref=main0_refClkSelect_clk_25MHz;     break;
        case 50 : dataref=main0_refClkSelect_clk_50Mhz;     break;
        case 100: dataref=main0_refClkSelect_clk_100MHz;    break;
        case 106: dataref=main0_refClkSelect_clk_106p25Mhz; break;
        case 125: dataref=main0_refClkSelect_clk_125MHz;    break;
        case 156: dataref=main0_refClkSelect_clk_156p25MHz; break;
        case 161: dataref=main0_refClkSelect_clk_161p25Mhz; break;
        case 187: dataref=main0_refClkSelect_clk_187p5MHz;  break;
        default : dataref=main0_refClkSelect_clk_156p25MHz;
    }

    MAIN0_SETUPr_CLR(MAIN0_SETUPr_reg);

    MAIN0_SETUPr_REFCLK_SELf_SET(MAIN0_SETUPr_reg, dataref);

    switch(plldiv) {
    /*
    case 32 : datapll=MAIN0_SETUP_DEFAULT_PLL_MODE_AFE_div32;  break;
    case 36 : datapll=MAIN0_SETUP_DEFAULT_PLL_MODE_AFE_div36;  break;
    case 40 : datapll=MAIN0_SETUP_DEFAULT_PLL_MODE_AFE_div40;  break;
    case 42 : datapll=MAIN0_SETUP_DEFAULT_PLL_MODE_AFE_div42;  break;
    case 48 : datapll=MAIN0_SETUP_DEFAULT_PLL_MODE_AFE_div48;  break;
    case 50 : datapll=MAIN0_SETUP_DEFAULT_PLL_MODE_AFE_div50;  break;
    case 52 : datapll=MAIN0_SETUP_DEFAULT_PLL_MODE_AFE_div52;  break;
    case 54 : datapll=MAIN0_SETUP_DEFAULT_PLL_MODE_AFE_div54;  break;
    case 60 : datapll=MAIN0_SETUP_DEFAULT_PLL_MODE_AFE_div60;  break;
    case 64 : datapll=MAIN0_SETUP_DEFAULT_PLL_MODE_AFE_div64;  break;
    case 66 : datapll=MAIN0_SETUP_DEFAULT_PLL_MODE_AFE_div66;  break;
    case 68 : datapll=MAIN0_SETUP_DEFAULT_PLL_MODE_AFE_div68;  break;
    case 70 : datapll=MAIN0_SETUP_DEFAULT_PLL_MODE_AFE_div70;  break;
    case 80 : datapll=MAIN0_SETUP_DEFAULT_PLL_MODE_AFE_div80;  break;
    case 92 : datapll=MAIN0_SETUP_DEFAULT_PLL_MODE_AFE_div92;  break;
    case 100: datapll=MAIN0_SETUP_DEFAULT_PLL_MODE_AFE_div100; break;
    default : datapll=MAIN0_SETUP_DEFAULT_PLL_MODE_AFE_div40;
    */
    }
    PHYMOD_IF_ERR_RETURN (MODIFY_MAIN0_SETUPr(pc, MAIN0_SETUPr_reg));
    return PHYMOD_E_NONE;
} /* tefmod_set_port_mode(PHYMOD_ST* pc) */


/*!
@brief Sets loopback mode from Tx to Rx at PCS/PMS parallel interface. (gloop).
@param  pc handle to current TSCF context (#PHYMOD_ST)
@param  pcs_gloop_en  Enable or disable PCS loopback.
@param  starting_lane first of multiple lanes if in  multi lane ports
@param  num_lanes Number of lanes to configure if in multi lane ports
@returns The value PHYMOD_E_NONE upon successful completion
@details
This function sets the TX-to-RX digital loopback mode, which is set
independently for each lane at the PCS/PMS parallel interface. If the port in
question contains multiple lanes we specify the starting and the lane count and
all those lanes are configured to loopback mode

The 1st, 2nd, 3rd and 4th byte of 'lane' input arg. is associated with lanes 0,
1, 2, and 3 respectively. The bits of each byte control their lanes as follows.

\li 0:1 : Enable  TX->RX loopback
\li 0:0 : Disable TX-RX loopback

Note that this function can program <B>multiple lanes simultaneously</B>.
As an example, to enable gloop on all lanes and enable TX on lane 0 only,
set lane to 0x01010103
*/

int tefmod_gen3_tx_loopback_control(PHYMOD_ST* pc, int pcs_gloop_en, int starting_lane, int num_lanes)
{
    MAIN0_LPBK_CTLr_t MAIN0_LOOPBACK_CONTROLr_reg;
    PMD_X4_OVRRr_t PMD_X4_OVERRIDEr_reg;
    PMD_X4_CTLr_t PMD_X4_PMD_X4_CONTROLr_reg;
    uint16_t lane_mask, i, data, tmp_data;
    phymod_access_t pa_copy;

    PHYMOD_MEMCPY(&pa_copy, pc, sizeof(pa_copy));
    pa_copy.lane_mask = 0x1 << starting_lane;

    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
    MAIN0_LPBK_CTLr_CLR(MAIN0_LOOPBACK_CONTROLr_reg);
    READ_MAIN0_LPBK_CTLr(pc, &MAIN0_LOOPBACK_CONTROLr_reg); 
    tmp_data = MAIN0_LPBK_CTLr_LOCAL_PCS_LOOPBACK_ENABLEf_GET(MAIN0_LOOPBACK_CONTROLr_reg);

    lane_mask = 0;
    data = 0;

    for (i = 0; i < num_lanes; i++) {
        if (!PHYMOD_LANEPBMP_MEMBER(pc->lane_mask, starting_lane + i)) {
            continue;
        }
        lane_mask |= 1 << (starting_lane + i);
        data |= pcs_gloop_en << (starting_lane + i);
    }
    tmp_data &= ~lane_mask;
    tmp_data |= data;

    MAIN0_LPBK_CTLr_LOCAL_PCS_LOOPBACK_ENABLEf_SET(MAIN0_LOOPBACK_CONTROLr_reg,  tmp_data);
    PHYMOD_IF_ERR_RETURN(MODIFY_MAIN0_LPBK_CTLr(pc, MAIN0_LOOPBACK_CONTROLr_reg));

    /* signal_detect and rx_lock */
    PMD_X4_OVRRr_CLR(PMD_X4_OVERRIDEr_reg);
    if (pcs_gloop_en) {
        PMD_X4_OVRRr_RX_LOCK_OVRDf_SET(PMD_X4_OVERRIDEr_reg, 1);
        PMD_X4_OVRRr_SIGNAL_DETECT_OVRDf_SET(PMD_X4_OVERRIDEr_reg, 1);
        PMD_X4_OVRRr_TX_DISABLE_OENf_SET(PMD_X4_OVERRIDEr_reg, 1);
    } else {
        PMD_X4_OVRRr_RX_LOCK_OVRDf_SET(PMD_X4_OVERRIDEr_reg, 0);
        PMD_X4_OVRRr_SIGNAL_DETECT_OVRDf_SET(PMD_X4_OVERRIDEr_reg, 0);
        PMD_X4_OVRRr_TX_DISABLE_OENf_SET(PMD_X4_OVERRIDEr_reg, 0);
    }
    PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X4_OVRRr(pc, PMD_X4_OVERRIDEr_reg));

    /* set tx_disable */
    PMD_X4_CTLr_CLR(PMD_X4_PMD_X4_CONTROLr_reg);
    PMD_X4_CTLr_TX_DISABLEf_SET(PMD_X4_PMD_X4_CONTROLr_reg, 1);

    PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X4_CTLr(pc, PMD_X4_PMD_X4_CONTROLr_reg));
    /* PHYMOD_IF_ERR_RETURN(tefmod_gen3_rx_lane_control_set(pc, 1)); */

    /* trigger speed change */
    tefmod_gen3_trigger_speed_change(&pa_copy);

    return PHYMOD_E_NONE;
}  


/*!
@brief Set remote loopback mode for GMII, cl49, aggregate(XAUI) and R2 modes.
@param  pc handle to current TSCF context (#PHYMOD_ST)
@param  pcs_rloop_en  controls rloop  enable or disable
@returns The value PHYMOD_E_NONE upon successful completion
@details
This function sets the remote loopback (RX-to-TX) mode for one lane at a time,
where the lane is indicated by the  field. To enable remote loopback, set
pcs_rloop_en to 1; to disable remote loopback, set the it to 0.
*/

int tefmod_gen3_rx_loopback_control(PHYMOD_ST* pc, int pcs_rloop_en)
{
    MAIN0_LPBK_CTLr_t MAIN0_LOOPBACK_CONTROLr_reg;
    DSC_CDR_CTL2r_t DSC_C_CDR_CONTROL_2r_reg;
    TX_PI_CTL0r_t TX_PI_CTL0r_reg;

    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);

    /* remote device is set in rloop */
    MAIN0_LPBK_CTLr_CLR(MAIN0_LOOPBACK_CONTROLr_reg);
    MAIN0_LPBK_CTLr_REMOTE_PCS_LOOPBACK_ENABLEf_SET(MAIN0_LOOPBACK_CONTROLr_reg, pcs_rloop_en);
    PHYMOD_IF_ERR_RETURN(MODIFY_MAIN0_LPBK_CTLr(pc, MAIN0_LOOPBACK_CONTROLr_reg));

    /* set Tx_PI */
    DSC_CDR_CTL2r_CLR(DSC_C_CDR_CONTROL_2r_reg);
    DSC_CDR_CTL2r_TX_PI_LOOP_TIMING_SRC_SELf_SET(DSC_C_CDR_CONTROL_2r_reg, 1);

    PHYMOD_IF_ERR_RETURN(MODIFY_DSC_CDR_CTL2r(pc, DSC_C_CDR_CONTROL_2r_reg));

    TX_PI_CTL0r_CLR(TX_PI_CTL0r_reg);
    TX_PI_CTL0r_TX_PI_ENf_SET(TX_PI_CTL0r_reg, 1);
    PHYMOD_IF_ERR_RETURN(MODIFY_TX_PI_CTL0r(pc, TX_PI_CTL0r_reg));

    return PHYMOD_E_NONE;
} 

int tefmod_gen3_tx_loopback_get(PHYMOD_ST* pc, uint32_t *enable)
{
    MAIN0_LPBK_CTLr_t loopReg;

    READ_MAIN0_LPBK_CTLr(pc, &loopReg);
    *enable = MAIN0_LPBK_CTLr_LOCAL_PCS_LOOPBACK_ENABLEf_GET(loopReg);

    return PHYMOD_E_NONE;
}


/**
@brief   PMD per lane reset
@param   pc handle to current TSCF context (#PHYMOD_ST)
@returns The value PHYMOD_E_NONE upon successful completion
@details Per lane PMD ln_rst and ln_dp_rst by writing to PMD_X4_CONTROL(0xc010)in pcs space
*/
int tefmod_gen3_pmd_x4_reset(PHYMOD_ST* pc)
{
    PMD_X4_CTLr_t reg_pmd_x4_ctrl;

    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
    PMD_X4_CTLr_CLR(reg_pmd_x4_ctrl);
    PMD_X4_CTLr_LN_H_RSTBf_SET(reg_pmd_x4_ctrl,0);
    PMD_X4_CTLr_LN_DP_H_RSTBf_SET(reg_pmd_x4_ctrl,0);
    PHYMOD_IF_ERR_RETURN (MODIFY_PMD_X4_CTLr(pc, reg_pmd_x4_ctrl));

    PMD_X4_CTLr_CLR(reg_pmd_x4_ctrl);
    PMD_X4_CTLr_LN_H_RSTBf_SET(reg_pmd_x4_ctrl,1);
    PMD_X4_CTLr_LN_DP_H_RSTBf_SET(reg_pmd_x4_ctrl,1);
    PHYMOD_IF_ERR_RETURN (MODIFY_PMD_X4_CTLr(pc, reg_pmd_x4_ctrl));

    return PHYMOD_E_NONE;
}

/**
@brief   PMD per lane reset get
@param   pc handle to current TSCF context (#PHYMOD_ST)
@returns The value PHYMOD_E_NONE upon successful completion
@details Per lane PMD ln_rst and ln_dp_rst by writing to PMD_X4_CONTROL(0xc010)in pcs space
*/
int tefmod_gen3_pmd_x4_reset_get(PHYMOD_ST* pc, int *is_in_reset)
{
    PMD_X4_CTLr_t reg_pmd_x4_ctrl;

    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
    PMD_X4_CTLr_CLR(reg_pmd_x4_ctrl);
    PHYMOD_IF_ERR_RETURN (READ_PMD_X4_CTLr(pc, &reg_pmd_x4_ctrl));
  
    if(PMD_X4_CTLr_LN_H_RSTBf_GET(reg_pmd_x4_ctrl) &&
        PMD_X4_CTLr_LN_DP_H_RSTBf_GET(reg_pmd_x4_ctrl)) {
        *is_in_reset = 0 ;  
    } else {
        *is_in_reset = 1 ;
    }
    return PHYMOD_E_NONE;
}


/**
@brief   Set Per lane OS mode set in PMD
@param   pc handle to current TSCF context (#PHYMOD_ST)
@param   spd_intf speed type #tefmod_spd_intfc_type_t
@param   os_mode over sample rate.
@returns The value PHYMOD_E_NONE upon successful completion
@details Per Port PMD Init

*/
int tefmod_gen3_pmd_os_mode_set(PHYMOD_ST* pc, tefmod_gen3_spd_intfc_type_t spd_intf, phymod_ref_clk_t refclk, int os_mode) 
{
    RXTXCOM_OSR_MODE_CTLr_t  RXTXCOM_OSR_MODE_CTLr_reg;
    int speed;
    phymod_access_t pa_copy;
    int i, start_lane = 0, num_lane = 0;
    uint32_t  lane_mask;

    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
    PHYMOD_MEMCPY(&pa_copy, pc, sizeof(pa_copy));
    PHYMOD_IF_ERR_RETURN
        (phymod_util_lane_config_get(pc, &start_lane, &num_lane)); 

    lane_mask = pa_copy.lane_mask;

    RXTXCOM_OSR_MODE_CTLr_CLR(RXTXCOM_OSR_MODE_CTLr_reg);
    tefmod_gen3_get_mapped_speed(spd_intf, &speed);

    /* osr_mode 
     * 0 = OS MODE 1  
     * 1 = OS MODE 2
     * 2 = OS MODE 4 
     * 4 = OS MODE 8P25 
     * 5 = OS MODE 8 
     * 8 = OS MODE 16P5 
     * 9 = OS MODE 16
     * 12 = OS MODE 20P625; 
     * 13 = OS MODE 32 
     */
    if (os_mode & 0x80000000) {
        os_mode = (os_mode) & 0x0000ffff; 
    } else {
        if (refclk == phymodRefClk125Mhz) {
            os_mode =  sc_pmd_dpll_entry_125M_ref[speed].pma_os_mode;
        } else {
            os_mode =  sc_pmd_dpll_entry[speed].pma_os_mode;
        }
    } 

    RXTXCOM_OSR_MODE_CTLr_OSR_MODE_FRCf_SET(RXTXCOM_OSR_MODE_CTLr_reg, 1);
    RXTXCOM_OSR_MODE_CTLr_OSR_MODE_FRC_VALf_SET(RXTXCOM_OSR_MODE_CTLr_reg, os_mode);

    for (i = 0; i < num_lane; i++) {
        if (!PHYMOD_LANEPBMP_MEMBER(lane_mask, start_lane + i)) {
          continue;
        }
        pa_copy.lane_mask = 0x1 << (i + start_lane);
        PHYMOD_IF_ERR_RETURN
          (MODIFY_RXTXCOM_OSR_MODE_CTLr(&pa_copy, RXTXCOM_OSR_MODE_CTLr_reg));
    }

    return PHYMOD_E_NONE;
}


/**
@brief   Checks config valid status for the port 
@param   pc handle to current TSCF context (#PHYMOD_ST)
@returns The value PHYMOD_E_NONE upon successful completion
@details This register bit indicates that PCS is now programmed as required
by the HT entry for that speed
*/
STATIC
int _tefmod_gen3_wait_sc_stats_set(PHYMOD_ST* pc)
{
    uint16_t data;
    uint16_t i;
    SC_X4_STSr_t reg_sc_ctrl_sts;
    SC_X4_STSr_CLR(reg_sc_ctrl_sts);
    i = 0;
    data = 0;

    for (i= 0; i < 10; i++) {
        PHYMOD_USLEEP(1);
        PHYMOD_IF_ERR_RETURN(READ_SC_X4_STSr(pc, &reg_sc_ctrl_sts));
        data = SC_X4_STSr_SW_SPEED_CONFIG_VLDf_GET(reg_sc_ctrl_sts);
        if(data == 1) {
            return PHYMOD_E_NONE;
        }
    }
    if (data != 1) {
        return PHYMOD_E_TIMEOUT;
    }
    /* Intentional check .if none of the above MACROs are defined,return PHYMOD_E_NONE */
    /* coverity[dead_error_line] */ 
    return PHYMOD_E_NONE;
}


/**
@brief   Set the port speed and enable the port
@param   pc handle to current TSCF context (#PHYMOD_ST)
@param   spd_intf the speed to set the port enum #tefmod_set_spd_intf
@returns The value PHYMOD_E_NONE upon successful completion
@details Sets the port to the specified speed and triggers the port, and waits 
         for the port to be configured
*/
int tefmod_gen3_set_spd_intf(PHYMOD_ST *pc, tefmod_gen3_spd_intfc_type_t spd_intf)
{
    SC_X4_CTLr_t xgxs_x4_ctrl;
    phymod_access_t pa_copy;
    int speed_id = 0, start_lane = 0, num_lane = 0;

    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
    TEFMOD_GEN3_DBG_IN_FUNC_VIN_INFO(pc,("spd_intf: %d", spd_intf));

    /* Need to figure out what's the starting lane */
    PHYMOD_MEMCPY(&pa_copy, pc, sizeof(pa_copy));
    PHYMOD_IF_ERR_RETURN
        (phymod_util_lane_config_get(pc, &start_lane, &num_lane));
    pa_copy.lane_mask = 0x1 << start_lane;

    PHYMOD_IF_ERR_RETURN (tefmod_gen3_get_mapped_speed(spd_intf, &speed_id));
    /* Write the speed_id into the speed_change register */
    SC_X4_CTLr_CLR(xgxs_x4_ctrl);
    SC_X4_CTLr_SPEEDf_SET(xgxs_x4_ctrl, speed_id);
    MODIFY_SC_X4_CTLr(pc, xgxs_x4_ctrl);
    /*next call the speed_change routine */
    PHYMOD_IF_ERR_RETURN (tefmod_gen3_trigger_speed_change(&pa_copy));
    /* Check the speed_change_done nit*/
    PHYMOD_IF_ERR_RETURN (_tefmod_gen3_wait_sc_stats_set(pc));

    return PHYMOD_E_NONE;
}

/**
@brief   Set the AN Portmode and Single Port for AN of TSCF.
@param   pc handle to current TSCF context (#PHYMOD_ST)
@param   num_of_lanes Number of lanes in this port
@param   starting_lane first lane in the port
@param   single_port Single port or not
@returns PHYMOD_E_NONE if successful. PHYMOD_E_FAIL else.
@details

This function can be called multiple times.Elements of #PHYMOD_ST should be
initialized to required values prior to calling this function. The following
sub-configurations are performed here.

\li Set pll divider for VCO setting (0x9000[12, 11:8]). pll divider is calculated from max_speed. 

*/
int tefmod_gen3_set_an_single_port_mode(PHYMOD_ST* pc, int an_enable)
{
    MAIN0_SETUPr_t  reg_setup;
    SC_X4_CTLr_t xgxs_x4_ctrl;
    phymod_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, pc, sizeof(phy_copy));
    SC_X4_CTLr_CLR(xgxs_x4_ctrl);

    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
    TEFMOD_GEN3_DBG_IN_FUNC_VIN_INFO(pc,("an_enable %d \n", an_enable));

    MAIN0_SETUPr_CLR(reg_setup);
    PHYMOD_IF_ERR_RETURN (READ_MAIN0_SETUPr(pc, &reg_setup));

    /* Ref clock selection tied to 156.25MHz */
    MAIN0_SETUPr_REFCLK_SELf_SET(reg_setup, 0x3);

    /* Set single port mode*/
    MAIN0_SETUPr_SINGLE_PORT_MODEf_SET(reg_setup, an_enable); 
    if (an_enable) {
        SC_X4_CTLr_SW_SPEED_CHANGEf_SET(xgxs_x4_ctrl, 0);
        phy_copy.lane_mask= 0xf;
        MODIFY_SC_X4_CTLr(&phy_copy, xgxs_x4_ctrl); 
    } else {
        SC_X4_CTLr_SW_SPEED_CHANGEf_SET(xgxs_x4_ctrl, 1);
        phy_copy.lane_mask= 0xf;
        MODIFY_SC_X4_CTLr(pc, xgxs_x4_ctrl); 
    } 
    PHYMOD_IF_ERR_RETURN(MODIFY_MAIN0_SETUPr(pc,reg_setup));

    return PHYMOD_E_NONE;
}

/**
@brief   Set port_mode_sel for AN of TSCF.
@param   pc handle to current TSCF context (#PHYMOD_ST)
@param   num_of_lanes Number of lanes in this port
@param   starting_lane first lane in the port
@returns PHYMOD_E_NONE if successful. PHYMOD_E_FAIL else.
@details

This function is called to set the proper port_mode_sel when AN mode is enabled.
*/
int tefmod_gen3_set_an_port_mode(PHYMOD_ST* pc, int num_of_lanes, int starting_lane)
{
    MAIN0_SETUPr_t  reg_setup;
    int port_mode_sel=0, new_port_mode_sel=0;
    
    MAIN0_SETUPr_CLR(reg_setup);
    PHYMOD_IF_ERR_RETURN(READ_MAIN0_SETUPr(pc, &reg_setup));
    port_mode_sel = MAIN0_SETUPr_PORT_MODE_SELf_GET(reg_setup);
    
    switch(port_mode_sel) {
    case TEFMOD_MULTI_PORT:  /* 3, 2, 1, 0; QUAD */
        if (num_of_lanes==2) {
            if(starting_lane==0) {
                new_port_mode_sel = TEFMOD_TRI2_PORT; 
            } else {
                new_port_mode_sel = TEFMOD_TRI1_PORT;
            }
        } else {
            new_port_mode_sel = TEFMOD_MULTI_PORT;
        }
        break;
    case TEFMOD_TRI1_PORT:  /* 3-2, 1, 0; TRI_1 */
        if (num_of_lanes==2 && starting_lane==0) {
            new_port_mode_sel = TEFMOD_DXGXS;
        } else if (num_of_lanes==4 || (num_of_lanes==1 && starting_lane >= 2)) {
            new_port_mode_sel = TEFMOD_MULTI_PORT;
        } else {
            new_port_mode_sel = TEFMOD_TRI1_PORT;
        }
        break;
    case TEFMOD_TRI2_PORT: /* 3, 2, 1-0; TRI_2 */
        if (num_of_lanes==2 && starting_lane==2) {
            new_port_mode_sel = TEFMOD_DXGXS;
        } else if (num_of_lanes==4 || (num_of_lanes==1 && starting_lane <= 1)) {
            new_port_mode_sel = TEFMOD_MULTI_PORT;
        } else {
            new_port_mode_sel = TEFMOD_TRI2_PORT;
        }
        break;
    case TEFMOD_DXGXS: /* 3-2, 1-0; DUAL */
        if (num_of_lanes==1 && starting_lane <= 1) {
            new_port_mode_sel = TEFMOD_TRI1_PORT; 
        } else if (num_of_lanes==1 && starting_lane >= 2) {
            new_port_mode_sel = TEFMOD_TRI2_PORT;
        } else if (num_of_lanes==2) {
            new_port_mode_sel = TEFMOD_DXGXS;
        } else {
            new_port_mode_sel = TEFMOD_MULTI_PORT;
        }
        break;
    case TEFMOD_SINGLE_PORT: /* 3-2-1-0; SINGLE PORT */
        if (num_of_lanes==2) {
            new_port_mode_sel = TEFMOD_DXGXS;
        } else if (num_of_lanes==1 && starting_lane <= 1) {
            new_port_mode_sel = TEFMOD_TRI1_PORT;
        } else if (num_of_lanes==1 && starting_lane >= 2) {
            new_port_mode_sel = TEFMOD_TRI2_PORT;
        } else {
            new_port_mode_sel = TEFMOD_MULTI_PORT;
        }
        break;
      default:
          return PHYMOD_E_FAIL;
          break ;
    }
    MAIN0_SETUPr_PORT_MODE_SELf_SET(reg_setup, new_port_mode_sel);
    PHYMOD_IF_ERR_RETURN(MODIFY_MAIN0_SETUPr(pc, reg_setup));

    return PHYMOD_E_NONE;
} 



/*!
@brief supports per field override 
@param  pc handle to current TSCF context (#PHYMOD_ST)
@param  per_field_override_en Controls which field to override
@returns PHYMOD_E_NONE if no errors. PHYMOD_EERROR else.
@details
  TBD
*/
int tefmod_gen3_set_override_0(PHYMOD_ST* pc, int per_field_override_en)
{
    int or_value;
    override_type_t or_en;
    RX_X4_PCS_CTL0r_t    RX_X4_CONTROL0_PCS_CONTROL_0r_reg;
    SC_X4_SC_X4_OVRRr_t  SC_X4_CONTROL_OVERRIDEr_reg;
    TX_X4_ENC0r_t        TX_X4_CONTROL0_ENCODE_0r_reg;
    TX_X4_MISCr_t        TX_X4_CONTROL0_MISCr_reg;
    SC_X4_FLD_OVRR_EN0_TYPEr_t SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg;
    SC_X4_FLD_OVRR_EN1_TYPEr_t SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg;

    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
    or_en = (override_type_t) (per_field_override_en >> 16);

    or_value = per_field_override_en & 0x0000ffff;
  
    switch(or_en) {
        case OVERRIDE_CLEAR:
            SC_X4_FLD_OVRR_EN0_TYPEr_CLR(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg);
            SC_X4_FLD_OVRR_EN0_TYPEr_SET(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg, or_value);
            SC_X4_FLD_OVRR_EN1_TYPEr_CLR(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg);
            SC_X4_FLD_OVRR_EN1_TYPEr_SET(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg, or_value);
            PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN0_TYPEr(pc, SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg));
            PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN1_TYPEr(pc, SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg));
            break;
        case OVERRIDE_NUM_LANES:
            SC_X4_SC_X4_OVRRr_CLR(SC_X4_CONTROL_OVERRIDEr_reg);
            SC_X4_SC_X4_OVRRr_NUM_LANES_OVERRIDE_VALUEf_SET(SC_X4_CONTROL_OVERRIDEr_reg, or_value);
            PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_SC_X4_OVRRr(pc, SC_X4_CONTROL_OVERRIDEr_reg));
            SC_X4_FLD_OVRR_EN0_TYPEr_CLR(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg);
            SC_X4_FLD_OVRR_EN0_TYPEr_NUM_LANES_OENf_SET(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg, 1);
            PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN0_TYPEr(pc, SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg));
            break;
        case OVERRIDE_NUM_LANES_DIS:
            SC_X4_FLD_OVRR_EN0_TYPEr_CLR(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg);
            SC_X4_FLD_OVRR_EN0_TYPEr_NUM_LANES_OENf_SET(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg, 0);
            PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN0_TYPEr(pc, SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg));
            break;
        case OVERRIDE_OS_MODE:
            TX_X4_MISCr_CLR(TX_X4_CONTROL0_MISCr_reg);
            TX_X4_MISCr_OS_MODEf_SET(TX_X4_CONTROL0_MISCr_reg, or_value);
            PHYMOD_IF_ERR_RETURN(MODIFY_TX_X4_MISCr(pc, TX_X4_CONTROL0_MISCr_reg));

            /* Set the override enable*/
            SC_X4_FLD_OVRR_EN0_TYPEr_CLR(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg);
            SC_X4_FLD_OVRR_EN0_TYPEr_OS_MODE_OENf_SET(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg, 1);
            PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN0_TYPEr(pc, SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg));
            break;
        case OVERRIDE_OS_MODE_DIS:
            SC_X4_FLD_OVRR_EN0_TYPEr_CLR(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg);
            SC_X4_FLD_OVRR_EN0_TYPEr_OS_MODE_OENf_SET(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg, 0);
            PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN0_TYPEr(pc, SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg));
            break;
        case OVERRIDE_T_FIFO_MODE:
            TX_X4_ENC0r_CLR(TX_X4_CONTROL0_ENCODE_0r_reg);
            TX_X4_ENC0r_T_FIFO_MODEf_SET(TX_X4_CONTROL0_ENCODE_0r_reg, or_value);
            PHYMOD_IF_ERR_RETURN(MODIFY_TX_X4_ENC0r(pc, TX_X4_CONTROL0_ENCODE_0r_reg));

            /* Set the override enable*/
            SC_X4_FLD_OVRR_EN0_TYPEr_CLR(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg);
            SC_X4_FLD_OVRR_EN0_TYPEr_T_FIFO_MODE_OENf_SET(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg, 1);
            PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN0_TYPEr(pc, SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg));
            break;
        case OVERRIDE_T_FIFO_MODE_DIS:
            SC_X4_FLD_OVRR_EN0_TYPEr_CLR(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg);
            SC_X4_FLD_OVRR_EN0_TYPEr_T_FIFO_MODE_OENf_SET(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg, 0);
            PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN0_TYPEr(pc, SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg));
            break;
        case OVERRIDE_T_ENC_MODE:
            TX_X4_ENC0r_CLR(TX_X4_CONTROL0_ENCODE_0r_reg);
            TX_X4_ENC0r_T_ENC_MODEf_SET(TX_X4_CONTROL0_ENCODE_0r_reg, or_value);
            PHYMOD_IF_ERR_RETURN(MODIFY_TX_X4_ENC0r(pc, TX_X4_CONTROL0_ENCODE_0r_reg));

            /* Set the override enable*/
            SC_X4_FLD_OVRR_EN0_TYPEr_CLR(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg);
            SC_X4_FLD_OVRR_EN0_TYPEr_T_ENC_MODE_OENf_SET(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg, 1);
            PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN0_TYPEr(pc, SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg));
            break;
        case OVERRIDE_T_ENC_MODE_DIS:
            /* Set the override enable*/
            SC_X4_FLD_OVRR_EN0_TYPEr_CLR(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg);
            SC_X4_FLD_OVRR_EN0_TYPEr_T_ENC_MODE_OENf_SET(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg, 0);
            PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN0_TYPEr(pc, SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg));
            break;
        case OVERRIDE_T_HG2_ENABLE:
            TX_X4_ENC0r_CLR(TX_X4_CONTROL0_ENCODE_0r_reg);
            TX_X4_ENC0r_HG2_ENABLEf_SET(TX_X4_CONTROL0_ENCODE_0r_reg, or_value);
            PHYMOD_IF_ERR_RETURN(MODIFY_TX_X4_ENC0r(pc, TX_X4_CONTROL0_ENCODE_0r_reg));
            /* Set the override enable*/
            SC_X4_FLD_OVRR_EN0_TYPEr_CLR(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg);
            SC_X4_FLD_OVRR_EN0_TYPEr_T_HG2_ENABLE_OENf_SET(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg, 1);
            PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN0_TYPEr(pc, SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg));
            break;
        case OVERRIDE_T_HG2_ENABLE_DIS:
            /*Set the override enable*/
            SC_X4_FLD_OVRR_EN0_TYPEr_CLR(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg);
            SC_X4_FLD_OVRR_EN0_TYPEr_T_HG2_ENABLE_OENf_SET(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg, 0);
            PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN0_TYPEr(pc, SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg));
            break;
        case OVERRIDE_T_PMA_BTMX_MODE_OEN:
            TX_X4_MISCr_CLR(TX_X4_CONTROL0_MISCr_reg);
            TX_X4_MISCr_T_PMA_BTMX_MODEf_SET(TX_X4_CONTROL0_MISCr_reg, or_value);
            PHYMOD_IF_ERR_RETURN(MODIFY_TX_X4_MISCr(pc, TX_X4_CONTROL0_MISCr_reg));
            /* Set the override enable*/
            SC_X4_FLD_OVRR_EN0_TYPEr_CLR(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg);
            SC_X4_FLD_OVRR_EN0_TYPEr_T_PMA_BTMX_MODE_OENf_SET(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg, 1);
            PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN0_TYPEr(pc, SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg));
            break;
        case OVERRIDE_T_PMA_BTMX_MODE_OEN_DIS:
            /* Set the override enable*/
            SC_X4_FLD_OVRR_EN0_TYPEr_CLR(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg);
            SC_X4_FLD_OVRR_EN0_TYPEr_T_PMA_BTMX_MODE_OENf_SET(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg, 0);
            PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN0_TYPEr(pc, SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg));
            /* No missing break. Intentional fall through case */
            /* coverity[unterminated_case] */
        case OVERRIDE_SCR_MODE:
            TX_X4_MISCr_CLR(TX_X4_CONTROL0_MISCr_reg);
            TX_X4_MISCr_SCR_MODEf_SET(TX_X4_CONTROL0_MISCr_reg, or_value);
            PHYMOD_IF_ERR_RETURN(MODIFY_TX_X4_MISCr(pc, TX_X4_CONTROL0_MISCr_reg));

            /* Set the override enable*/
            SC_X4_FLD_OVRR_EN0_TYPEr_CLR(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg);
            SC_X4_FLD_OVRR_EN0_TYPEr_SCR_MODE_OENf_SET(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg, 1);
            PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN0_TYPEr(pc, SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg));

        case OVERRIDE_SCR_MODE_DIS:
            /* Set the override enable*/
            SC_X4_FLD_OVRR_EN0_TYPEr_CLR(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg);
            SC_X4_FLD_OVRR_EN0_TYPEr_SCR_MODE_OENf_SET(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg, 0);
            PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN0_TYPEr(pc, SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg));
            break;
        case OVERRIDE_DESCR_MODE_OEN:
            RX_X4_PCS_CTL0r_CLR(RX_X4_CONTROL0_PCS_CONTROL_0r_reg);
            RX_X4_PCS_CTL0r_DESCR_MODEf_SET(RX_X4_CONTROL0_PCS_CONTROL_0r_reg, or_value);
            PHYMOD_IF_ERR_RETURN(MODIFY_RX_X4_PCS_CTL0r(pc, RX_X4_CONTROL0_PCS_CONTROL_0r_reg));

            /* Set the override enable*/
            SC_X4_FLD_OVRR_EN0_TYPEr_CLR(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg);
            SC_X4_FLD_OVRR_EN0_TYPEr_DESCR_MODE_OENf_SET(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg, 1);
            PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN0_TYPEr(pc, SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg));
            break;
        case OVERRIDE_DESCR_MODE_OEN_DIS:
            /* Set the override enable*/
            SC_X4_FLD_OVRR_EN0_TYPEr_CLR(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg);
            SC_X4_FLD_OVRR_EN0_TYPEr_DESCR_MODE_OENf_SET(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg, 0);
            PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN0_TYPEr(pc, SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg));
            break;
        case OVERRIDE_DEC_TL_MODE:
            RX_X4_PCS_CTL0r_CLR(RX_X4_CONTROL0_PCS_CONTROL_0r_reg);
            RX_X4_PCS_CTL0r_DEC_TL_MODEf_SET(RX_X4_CONTROL0_PCS_CONTROL_0r_reg, or_value);
            PHYMOD_IF_ERR_RETURN(MODIFY_RX_X4_PCS_CTL0r(pc, RX_X4_CONTROL0_PCS_CONTROL_0r_reg));

            /* Set the override enable*/
            SC_X4_FLD_OVRR_EN0_TYPEr_CLR(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg);
            SC_X4_FLD_OVRR_EN0_TYPEr_DEC_TL_MODE_OENf_SET(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg, 1);
            PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN0_TYPEr(pc, SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg));
            break;
        case OVERRIDE_DEC_TL_MODE_DIS:
            /* Set the override enable*/
            SC_X4_FLD_OVRR_EN0_TYPEr_CLR(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg);
            SC_X4_FLD_OVRR_EN0_TYPEr_DEC_TL_MODE_OENf_SET(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg, 0);
            PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN0_TYPEr(pc, SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg));
            break;
        case OVERRIDE_DESKEW_MODE:
            RX_X4_PCS_CTL0r_CLR(RX_X4_CONTROL0_PCS_CONTROL_0r_reg);
            RX_X4_PCS_CTL0r_DESKEW_MODEf_SET(RX_X4_CONTROL0_PCS_CONTROL_0r_reg, or_value);
            PHYMOD_IF_ERR_RETURN(MODIFY_RX_X4_PCS_CTL0r(pc, RX_X4_CONTROL0_PCS_CONTROL_0r_reg));

            /* Set the override enable*/
            SC_X4_FLD_OVRR_EN0_TYPEr_CLR(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg);
            SC_X4_FLD_OVRR_EN0_TYPEr_DESKEW_MODE_OENf_SET(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg, 1);
            PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN0_TYPEr(pc, SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg));
            break;
        case OVERRIDE_DESKEW_MODE_DIS:
            /* Set the override enable*/
            SC_X4_FLD_OVRR_EN0_TYPEr_CLR(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg);
            SC_X4_FLD_OVRR_EN0_TYPEr_DESKEW_MODE_OENf_SET(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg, 0);
            PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN0_TYPEr(pc, SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg));
            break;
        case OVERRIDE_DEC_FSM_MODE:
            RX_X4_PCS_CTL0r_CLR(RX_X4_CONTROL0_PCS_CONTROL_0r_reg);
            RX_X4_PCS_CTL0r_DEC_FSM_MODEf_SET(RX_X4_CONTROL0_PCS_CONTROL_0r_reg, or_value);
            PHYMOD_IF_ERR_RETURN(MODIFY_RX_X4_PCS_CTL0r(pc, RX_X4_CONTROL0_PCS_CONTROL_0r_reg));

            /* Set the override enable*/
            SC_X4_FLD_OVRR_EN0_TYPEr_CLR(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg);
            SC_X4_FLD_OVRR_EN0_TYPEr_DEC_FSM_MODE_OENf_SET(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg, 1);
            PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN0_TYPEr(pc, SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg));
            break;
        case OVERRIDE_DEC_FSM_MODE_DIS:
            /* Set the override enable*/
            SC_X4_FLD_OVRR_EN0_TYPEr_CLR(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg);
            SC_X4_FLD_OVRR_EN0_TYPEr_DEC_FSM_MODE_OENf_SET(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg, 0);
            PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN0_TYPEr(pc, SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg));
            break;
        default:
        break;
  }
  return PHYMOD_E_NONE;
}  /* OVERRIDE_SET  */

/*!
@brief TBD
@param  pc handle to current TSCF context (#PHYMOD_ST)
@param  per_lane_control
@param  per_field_override_en
@returns PHYMOD_E_NONE if no errors. PHYMOD_EERROR else.
@details
  TBD
*/
int tefmod_gen3_set_override_1(PHYMOD_ST* pc, int per_lane_control, uint32_t per_field_override_en)   /* OVERRIDE_SET */
{
    int or_value;
    override_type_t or_en;
    MAIN0_SETUPr_t                     MAIN0_SETUPr_reg;
    RX_X4_BLKSYNC_CFGr_t   RX_X4_CONTROL0_BLKSYNC_CONFIGr_reg;
    TX_X4_MISCr_t             TX_X4_CONTROL0_MISCr_reg;
    TX_X4_CRED0r_t           TX_X4_CREDIT0_CREDIT0r_reg;
    TX_X4_CRED1r_t           TX_X4_CREDIT0_CREDIT1r_reg;
    TX_X4_LOOPCNTr_t           TX_X4_CREDIT0_LOOPCNTr_reg;
    TX_X4_MAC_CREDGENCNTr_t  TX_X4_CREDIT0_MAC_CREDITGENCNTr_reg;
    SC_X4_FLD_OVRR_EN0_TYPEr_t SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg;
    SC_X4_FLD_OVRR_EN1_TYPEr_t SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg;

    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
    or_en = (override_type_t) (per_field_override_en >> 16);
    or_value = per_field_override_en & 0x0000ffff;
  
    switch(or_en) {
        case OVERRIDE_R_HG2_ENABLE:
            TX_X4_MISCr_CLR(TX_X4_CONTROL0_MISCr_reg);
            TX_X4_MISCr_SET(TX_X4_CONTROL0_MISCr_reg, or_value);
            PHYMOD_IF_ERR_RETURN(MODIFY_TX_X4_MISCr(pc, TX_X4_CONTROL0_MISCr_reg));
            /* Set the override enable*/
            SC_X4_FLD_OVRR_EN1_TYPEr_CLR(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg);
            SC_X4_FLD_OVRR_EN1_TYPEr_R_HG2_ENABLE_OENf_SET(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg, 1);
            PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN1_TYPEr(pc, SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg));
            break;
        case OVERRIDE_R_HG2_ENABLE_DIS:
            /* Set the override enable*/
            SC_X4_FLD_OVRR_EN1_TYPEr_CLR(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg);
            SC_X4_FLD_OVRR_EN1_TYPEr_R_HG2_ENABLE_OENf_SET(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg, 0);
            PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN1_TYPEr(pc, SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg));
            break;
        case OVERRIDE_BS_SM_SYNC_MODE_OEN:
            RX_X4_BLKSYNC_CFGr_CLR(RX_X4_CONTROL0_BLKSYNC_CONFIGr_reg);
            RX_X4_BLKSYNC_CFGr_BS_SM_SYNC_MODEf_SET(RX_X4_CONTROL0_BLKSYNC_CONFIGr_reg, or_value);
            PHYMOD_IF_ERR_RETURN(MODIFY_RX_X4_BLKSYNC_CFGr(pc, RX_X4_CONTROL0_BLKSYNC_CONFIGr_reg));

            /* Set the override enable*/
            SC_X4_FLD_OVRR_EN1_TYPEr_CLR(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg);
            SC_X4_FLD_OVRR_EN1_TYPEr_BS_SM_SYNC_MODE_OENf_SET(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg, 1);
            PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN1_TYPEr(pc, SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg));
            break;
        case OVERRIDE_BS_SM_SYNC_MODE_OEN_DIS:
            /* Set the override enable*/
            SC_X4_FLD_OVRR_EN1_TYPEr_CLR(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg);
            SC_X4_FLD_OVRR_EN1_TYPEr_BS_SM_SYNC_MODE_OENf_SET(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg, 0);
            PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN1_TYPEr(pc, SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg));
            break;
        case OVERRIDE_BS_SYNC_EN_OEN:
            RX_X4_BLKSYNC_CFGr_CLR(RX_X4_CONTROL0_BLKSYNC_CONFIGr_reg);
            RX_X4_BLKSYNC_CFGr_BS_SYNC_ENf_SET(RX_X4_CONTROL0_BLKSYNC_CONFIGr_reg, or_value);
            PHYMOD_IF_ERR_RETURN(MODIFY_RX_X4_BLKSYNC_CFGr(pc, RX_X4_CONTROL0_BLKSYNC_CONFIGr_reg));

            /* Set the override enable*/
            SC_X4_FLD_OVRR_EN1_TYPEr_CLR(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg);
            SC_X4_FLD_OVRR_EN1_TYPEr_BS_SYNC_EN_OENf_SET(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg, 1);
            PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN1_TYPEr(pc, SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg));
            break;
        case OVERRIDE_BS_SYNC_EN_OEN_DIS:
            /* Set the override enable*/
            SC_X4_FLD_OVRR_EN1_TYPEr_CLR(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg);
            SC_X4_FLD_OVRR_EN1_TYPEr_BS_SYNC_EN_OENf_SET(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg, 0);
            PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN1_TYPEr(pc, SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg));
            break;
        case OVERRIDE_BS_DIST_MODE_OEN:
            RX_X4_BLKSYNC_CFGr_CLR(RX_X4_CONTROL0_BLKSYNC_CONFIGr_reg);
            RX_X4_BLKSYNC_CFGr_BS_DIST_MODEf_SET(RX_X4_CONTROL0_BLKSYNC_CONFIGr_reg, or_value);
            PHYMOD_IF_ERR_RETURN(MODIFY_RX_X4_BLKSYNC_CFGr(pc, RX_X4_CONTROL0_BLKSYNC_CONFIGr_reg));

            /* Set the override enable*/
            SC_X4_FLD_OVRR_EN1_TYPEr_CLR(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg);
            SC_X4_FLD_OVRR_EN1_TYPEr_BS_DIST_MODE_OENf_SET(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg, 1);
            PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN1_TYPEr(pc, SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg));
            break;
        case OVERRIDE_BS_DIST_MODE_OEN_DIS:
            /* Set the override enable*/
            SC_X4_FLD_OVRR_EN1_TYPEr_CLR(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg);
            SC_X4_FLD_OVRR_EN1_TYPEr_BS_DIST_MODE_OENf_SET(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg, 0);
            PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN1_TYPEr(pc, SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg));
            break;
        case OVERRIDE_BS_BTMX_MODE_OEN:
            RX_X4_BLKSYNC_CFGr_CLR(RX_X4_CONTROL0_BLKSYNC_CONFIGr_reg);
            RX_X4_BLKSYNC_CFGr_BS_BTMX_MODEf_SET(RX_X4_CONTROL0_BLKSYNC_CONFIGr_reg, or_value);
            PHYMOD_IF_ERR_RETURN(MODIFY_RX_X4_BLKSYNC_CFGr(pc, RX_X4_CONTROL0_BLKSYNC_CONFIGr_reg));

            /* Set the override enable*/
            SC_X4_FLD_OVRR_EN1_TYPEr_CLR(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg);
            SC_X4_FLD_OVRR_EN1_TYPEr_BS_BTMX_MODE_OENf_SET(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg, 1);
            PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN1_TYPEr(pc, SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg));
            break;
        case OVERRIDE_BS_BTMX_MODE_OEN_DIS:
            /*Set the override enable*/
            SC_X4_FLD_OVRR_EN1_TYPEr_CLR(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg);
            SC_X4_FLD_OVRR_EN1_TYPEr_BS_BTMX_MODE_OENf_SET(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg, 0);
            PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN1_TYPEr(pc, SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg));
            break;
        case OVERRIDE_CL72_EN:
            MAIN0_SETUPr_CLR(MAIN0_SETUPr_reg);
            MAIN0_SETUPr_CL72_ENf_SET(MAIN0_SETUPr_reg, or_value);
            PHYMOD_IF_ERR_RETURN(MODIFY_MAIN0_SETUPr(pc, MAIN0_SETUPr_reg));

            /* Set the override enable*/
            SC_X4_FLD_OVRR_EN0_TYPEr_CLR(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg);
            SC_X4_FLD_OVRR_EN0_TYPEr_CL72_EN_OENf_SET(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg, 1);
            PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN0_TYPEr(pc, SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg));
            break;
        case OVERRIDE_CL72_EN_DIS:
            /* Set the override enable*/
            SC_X4_FLD_OVRR_EN0_TYPEr_CLR(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg);
            SC_X4_FLD_OVRR_EN0_TYPEr_CL72_EN_OENf_SET(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg, 0);
            PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN0_TYPEr(pc, SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE0_TYPEr_reg));
            break;
        case OVERRIDE_CLOCKCNT0:
            TX_X4_CRED0r_CLR(TX_X4_CREDIT0_CREDIT0r_reg);
            TX_X4_CRED0r_CLOCKCNT0f_SET(TX_X4_CREDIT0_CREDIT0r_reg, or_value);
            PHYMOD_IF_ERR_RETURN(MODIFY_TX_X4_CRED0r(pc, TX_X4_CREDIT0_CREDIT0r_reg));

            /* Set the override enable*/
            SC_X4_FLD_OVRR_EN1_TYPEr_CLR(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg);
            SC_X4_FLD_OVRR_EN1_TYPEr_CLOCKCNT0_OENf_SET(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg, 1);
            PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN1_TYPEr(pc, SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg));
            break;
        case OVERRIDE_CLOCKCNT0_DIS:
            /* Set the override enable*/
            SC_X4_FLD_OVRR_EN1_TYPEr_CLR(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg);
            SC_X4_FLD_OVRR_EN1_TYPEr_CLOCKCNT0_OENf_SET(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg, 0);
            PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN1_TYPEr(pc, SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg));
            break;
        case OVERRIDE_CLOCKCNT1:
            TX_X4_CRED1r_CLR(TX_X4_CREDIT0_CREDIT1r_reg);
            TX_X4_CRED1r_CLOCKCNT1f_SET(TX_X4_CREDIT0_CREDIT1r_reg, or_value);
            PHYMOD_IF_ERR_RETURN(MODIFY_TX_X4_CRED1r(pc, TX_X4_CREDIT0_CREDIT1r_reg));

            /* Set the override enable*/
            SC_X4_FLD_OVRR_EN1_TYPEr_CLR(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg);
            SC_X4_FLD_OVRR_EN1_TYPEr_CLOCKCNT1_OENf_SET(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg, 1);
            PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN1_TYPEr(pc, SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg));
            break;
        case OVERRIDE_CLOCKCNT1_DIS:
            /* Set the override enable*/
            SC_X4_FLD_OVRR_EN1_TYPEr_CLR(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg);
            SC_X4_FLD_OVRR_EN1_TYPEr_CLOCKCNT1_OENf_SET(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg, 0);
            PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN1_TYPEr(pc, SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg));
            break;
        case OVERRIDE_LOOPCNT0:
            TX_X4_LOOPCNTr_CLR(TX_X4_CREDIT0_LOOPCNTr_reg);
            TX_X4_LOOPCNTr_LOOPCNT0f_SET(TX_X4_CREDIT0_LOOPCNTr_reg, or_value);
            PHYMOD_IF_ERR_RETURN(MODIFY_TX_X4_LOOPCNTr(pc, TX_X4_CREDIT0_LOOPCNTr_reg));

            /* Set the override enable*/
            SC_X4_FLD_OVRR_EN1_TYPEr_CLR(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg);
            SC_X4_FLD_OVRR_EN1_TYPEr_LOOPCNT0_OENf_SET(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg, 1);
            PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN1_TYPEr(pc, SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg));
            break;
        case OVERRIDE_LOOPCNT0_DIS:
            /* Set the override enable*/
            SC_X4_FLD_OVRR_EN1_TYPEr_CLR(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg);
            SC_X4_FLD_OVRR_EN1_TYPEr_LOOPCNT0_OENf_SET(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg, 0);
            PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN1_TYPEr(pc, SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg));
            break;
        case OVERRIDE_LOOPCNT1:
            TX_X4_LOOPCNTr_CLR(TX_X4_CREDIT0_LOOPCNTr_reg);
            TX_X4_LOOPCNTr_LOOPCNT1f_SET(TX_X4_CREDIT0_LOOPCNTr_reg, or_value);
            PHYMOD_IF_ERR_RETURN(MODIFY_TX_X4_LOOPCNTr(pc, TX_X4_CREDIT0_LOOPCNTr_reg));

            /* Set the override enable*/
            SC_X4_FLD_OVRR_EN1_TYPEr_CLR(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg);
            SC_X4_FLD_OVRR_EN1_TYPEr_LOOPCNT1_OENf_SET(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg, 1);
            PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN1_TYPEr(pc, SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg));
            break;
        case OVERRIDE_LOOPCNT1_DIS:
            /* Set the override enable*/
            SC_X4_FLD_OVRR_EN1_TYPEr_CLR(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg);
            SC_X4_FLD_OVRR_EN1_TYPEr_LOOPCNT1_OENf_SET(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg, 0);
            PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN1_TYPEr(pc, SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg));
            break;
        case OVERRIDE_MAC_CREDITGENCNT:
            TX_X4_MAC_CREDGENCNTr_CLR(TX_X4_CREDIT0_MAC_CREDITGENCNTr_reg);
            TX_X4_MAC_CREDGENCNTr_MAC_CREDITGENCNTf_SET(TX_X4_CREDIT0_MAC_CREDITGENCNTr_reg, or_value);
            PHYMOD_IF_ERR_RETURN(MODIFY_TX_X4_MAC_CREDGENCNTr(pc, TX_X4_CREDIT0_MAC_CREDITGENCNTr_reg));

            /* Set the override enable*/
            SC_X4_FLD_OVRR_EN1_TYPEr_CLR(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg);
            SC_X4_FLD_OVRR_EN1_TYPEr_MAC_CREDITGENCNT_OENf_SET(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg, 1);
            PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN1_TYPEr(pc, SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg));
            break;
        case OVERRIDE_MAC_CREDITGENCNT_DIS:
            /* Set the override enable*/
            SC_X4_FLD_OVRR_EN1_TYPEr_CLR(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg);
            SC_X4_FLD_OVRR_EN1_TYPEr_MAC_CREDITGENCNT_OENf_SET(SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg, 0);
            PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN1_TYPEr(pc, SC_X4_FIELD_OVERRIDE_ENABLE_FIELD_OVERRIDE_ENABLE1_TYPEr_reg));
        default:
        break;
    }

    return PHYMOD_E_NONE;
}  /* OVERRIDE_SET  */

/*!
@brief Enable / Disable credits for any particular lane
@param  pc handle to current TSCF context (#PHYMOD_ST)
@param  per_lane_control Control to enable or disable credits
@returns The value PHYMOD_E_NONE upon successful completion.
@details
This function is used to enable or disable the credit generation.

\li set to 0x1 to enable credit
\li Set to 0x0 disable credit for selected lane.

This function reads/modifies the following registers:

\li enable credits: clockcnt0 (0xC100[14])
*/
int tefmod_gen3_credit_control(PHYMOD_ST* pc, int per_lane_control)
{
    int cntl;
    TX_X4_CRED0r_t TX_X4_CREDIT0_CREDIT0r_reg;
    cntl = per_lane_control;

    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
    TX_X4_CRED0r_CLR(TX_X4_CREDIT0_CREDIT0r_reg);
    if (cntl) { /* enable credits */
        TX_X4_CRED0r_CREDITENABLEf_SET(TX_X4_CREDIT0_CREDIT0r_reg, 1);
        PHYMOD_IF_ERR_RETURN(MODIFY_TX_X4_CRED0r(pc, TX_X4_CREDIT0_CREDIT0r_reg));
     } else { /* disable credits */
        TX_X4_CRED0r_CREDITENABLEf_SET(TX_X4_CREDIT0_CREDIT0r_reg, 0);
        PHYMOD_IF_ERR_RETURN(MODIFY_TX_X4_CRED0r(pc, TX_X4_CREDIT0_CREDIT0r_reg));
    }
    return PHYMOD_E_NONE;
}

/*!
@brief tx lane reset and enable of any particular lane
@param  pc handle to current TSCF context (#PHYMOD_ST)
@param  tx_dis_type tx property to control, see enum #tefmod_tx_disable_enum_t
@returns The value PHYMOD_E_NONE upon successful completion.
@details
This function is used to reset tx lane and enable/disable tx lane of any
transmit lane.  Set bit 0 of per_lane_control to enable the TX_LANE
(1) or disable the TX lane (0).
When diable TX lane, two types of operations are inovked independently.  
If per_lane_control bit [7:4] = 1, only traffic is disabled. 
      (TEFMOD_TX_LANE_TRAFFIC =0x10)
If bit [7:4] = 2, only reset function is invoked.
      (TEFMOD_TX_LANE_RESET = 0x20)

This function reads/modifies the following registers:

\li rstb_tx_lane  :   0xc113[1]
\li enable_tx_lane:   0xc113[0]
*/

int tefmod_gen3_tx_lane_control_set(PHYMOD_ST* pc,  tefmod_gen3_tx_disable_enum_t tx_dis_type)
{
    TX_X4_MISCr_t TX_X4_CONTROL0_MISCr_reg;

    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
    TX_X4_MISCr_CLR(TX_X4_CONTROL0_MISCr_reg);

    switch(tx_dis_type) {
        case TEFMOD_TX_LANE_RESET:
            TX_X4_MISCr_RSTB_TX_LANEf_SET(TX_X4_CONTROL0_MISCr_reg, 0);
            PHYMOD_IF_ERR_RETURN (MODIFY_TX_X4_MISCr(pc, TX_X4_CONTROL0_MISCr_reg));
            TX_X4_MISCr_CLR(TX_X4_CONTROL0_MISCr_reg);
            TX_X4_MISCr_RSTB_TX_LANEf_SET(TX_X4_CONTROL0_MISCr_reg, 1);
            PHYMOD_IF_ERR_RETURN (MODIFY_TX_X4_MISCr(pc, TX_X4_CONTROL0_MISCr_reg));
            break;
        case TEFMOD_TX_LANE_TRAFFIC_ENABLE:
            TX_X4_MISCr_ENABLE_TX_LANEf_SET(TX_X4_CONTROL0_MISCr_reg, 1);
            PHYMOD_IF_ERR_RETURN (MODIFY_TX_X4_MISCr(pc, TX_X4_CONTROL0_MISCr_reg));
            break;
        case TEFMOD_TX_LANE_TRAFFIC_DISABLE:
            TX_X4_MISCr_ENABLE_TX_LANEf_SET(TX_X4_CONTROL0_MISCr_reg, 0);
            PHYMOD_IF_ERR_RETURN (MODIFY_TX_X4_MISCr(pc, TX_X4_CONTROL0_MISCr_reg));
            break;
        case TEFMOD_TX_LANE_RESET_TRAFFIC_ENABLE:
            TX_X4_MISCr_RSTB_TX_LANEf_SET(TX_X4_CONTROL0_MISCr_reg, 1);
            PHYMOD_IF_ERR_RETURN (MODIFY_TX_X4_MISCr(pc, TX_X4_CONTROL0_MISCr_reg));
            TX_X4_MISCr_CLR(TX_X4_CONTROL0_MISCr_reg);
            TX_X4_MISCr_ENABLE_TX_LANEf_SET(TX_X4_CONTROL0_MISCr_reg, 1);
            PHYMOD_IF_ERR_RETURN (MODIFY_TX_X4_MISCr(pc, TX_X4_CONTROL0_MISCr_reg));
            break;
        case TEFMOD_TX_LANE_RESET_TRAFFIC_DISABLE:
            TX_X4_MISCr_RSTB_TX_LANEf_SET(TX_X4_CONTROL0_MISCr_reg, 0);
            PHYMOD_IF_ERR_RETURN (MODIFY_TX_X4_MISCr(pc, TX_X4_CONTROL0_MISCr_reg));
            TX_X4_MISCr_CLR(TX_X4_CONTROL0_MISCr_reg);
            TX_X4_MISCr_ENABLE_TX_LANEf_SET(TX_X4_CONTROL0_MISCr_reg, 0);
            PHYMOD_IF_ERR_RETURN (MODIFY_TX_X4_MISCr(pc, TX_X4_CONTROL0_MISCr_reg));
            break;
        default:
            break;
    }

    return PHYMOD_E_NONE;
}

/*!
@brief tx lane reset and enable of any particular lane
@param  pc handle to current TSCF context (#PHYMOD_ST)
@param  reset gets the configured tx rstb
@param  enable gets the configured tx enable
@returns The value PHYMOD_E_NONE upon successful completion.
@details
This function reads the following registers:

\li rstb_tx_lane  :   0xc113[1]
\li enable_tx_lane:   0xc113[0]
*/
int tefmod_gen3_tx_lane_control_get(PHYMOD_ST* pc,  int *reset, int *enable)
{
    TX_X4_MISCr_t TX_X4_CONTROL0_MISCr_reg;

    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
    TX_X4_MISCr_CLR(TX_X4_CONTROL0_MISCr_reg);

    PHYMOD_IF_ERR_RETURN (READ_TX_X4_MISCr(pc, &TX_X4_CONTROL0_MISCr_reg));
    *reset = TX_X4_MISCr_RSTB_TX_LANEf_GET(TX_X4_CONTROL0_MISCr_reg);
    *enable = TX_X4_MISCr_ENABLE_TX_LANEf_GET(TX_X4_CONTROL0_MISCr_reg);

    return PHYMOD_E_NONE;
}


/*!
@brief rx lane reset and enable of any particular lane
@param  pc handle to current TSCF context (#PHYMOD_ST)
@param  accData TBD
@param  per_lane_control TBD
@returns The value PHYMOD_E_NONE upon successful completion.
@details
This function enables/disables rx lane (RSTB_LANE) or read back control bit for
that based on per_lane_control being 1 or 0. If per_lane_control is 0xa, only
read back RSTB_LANE.
\li select encodeMode
This function reads/modifies the following registers:
\li rstb_rx_lane  :   0xc137[0]
*/
/* coverity[param_set_but_not_used] */
int tefmod_gen3_rx_lane_control(PHYMOD_ST* pc, int accData, int per_lane_control)
{
    int cntl;
    RX_X4_PMA_CTL0r_t RX_X4_CONTROL0_PMA_CONTROL_0r_reg;
    /* coverity */
    accData = accData; 

    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
    cntl = per_lane_control;

    RX_X4_PMA_CTL0r_CLR(RX_X4_CONTROL0_PMA_CONTROL_0r_reg);
    if (cntl==0xa) {
        /* set encoder */
        PHYMOD_IF_ERR_RETURN(READ_RX_X4_PMA_CTL0r(pc, &RX_X4_CONTROL0_PMA_CONTROL_0r_reg));
        if (RX_X4_PMA_CTL0r_RSTB_LANEf_GET(RX_X4_CONTROL0_PMA_CONTROL_0r_reg)){
            accData =1;
        } else {
            accData =0;
        }
     } else if (cntl) {
        /* set encoder */
        RX_X4_PMA_CTL0r_RSTB_LANEf_SET(RX_X4_CONTROL0_PMA_CONTROL_0r_reg, 1);
        PHYMOD_IF_ERR_RETURN(MODIFY_RX_X4_PMA_CTL0r(pc, RX_X4_CONTROL0_PMA_CONTROL_0r_reg));
    } else {
         /* bit set to 0 for disabling RXP */
        RX_X4_PMA_CTL0r_RSTB_LANEf_SET(RX_X4_CONTROL0_PMA_CONTROL_0r_reg, 0);
        PHYMOD_IF_ERR_RETURN(MODIFY_RX_X4_PMA_CTL0r(pc, RX_X4_CONTROL0_PMA_CONTROL_0r_reg));
    }
    return PHYMOD_E_NONE;
}


/*!
@brief Read the 16 bit rev. id value etc.
@param  pc handle to current TSCF context (#PHYMOD_ST)
@param  revIdV receives the revid
@returns The value of the revid.
@details
This  fucntion reads the revid register that contains core number, revision
number and returns it.
*/
int tefmod_gen3_revid_read(PHYMOD_ST* pc, uint32_t *revIdV)
{
    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);

    *revIdV=_tefmod_gen3_getRevDetails(pc);
    return PHYMOD_E_NONE;
}

int tefmod_gen3_clause72_control(PHYMOD_ST* pc, int cl72en) 
{
    int cl72_enable;
    SC_X4_CTLr_t                      SC_X4_CONTROL_SC_X4_CONTROL_CONTROLr_reg;
    CL93N72_IT_BASE_R_PMD_CTLr_t      CL93N72_IEEE_TX_CL93N72IT_BASE_R_PMD_CONTROLr_reg;
    int start_lane = 0, num_lane = 0 , i=0;
    uint32_t enable = 0;
    phymod_access_t pa_copy;

    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
    cl72_enable  = cl72en & 0x1;

    /* Need to figure out what's the starting lane */
    PHYMOD_MEMCPY(&pa_copy, pc, sizeof(pa_copy));
    PHYMOD_IF_ERR_RETURN
        (phymod_util_lane_config_get(pc, &start_lane, &num_lane));
    pa_copy.lane_mask = 0x1 << start_lane;

    CL93N72_IT_BASE_R_PMD_CTLr_CLR(CL93N72_IEEE_TX_CL93N72IT_BASE_R_PMD_CONTROLr_reg);
    PHYMOD_IF_ERR_RETURN(READ_CL93N72_IT_BASE_R_PMD_CTLr(&pa_copy, &CL93N72_IEEE_TX_CL93N72IT_BASE_R_PMD_CONTROLr_reg));

    if(CL93N72_IT_BASE_R_PMD_CTLr_CL93N72_IEEE_TRAINING_ENABLEf_GET(CL93N72_IEEE_TX_CL93N72IT_BASE_R_PMD_CONTROLr_reg) 
				!= (uint32_t)cl72_enable) {

        for (i = (num_lane-1); i >= 0; i--) {
            pa_copy.lane_mask = 0x1 << (i + start_lane);
            CL93N72_IT_BASE_R_PMD_CTLr_CLR(CL93N72_IEEE_TX_CL93N72IT_BASE_R_PMD_CONTROLr_reg);
            PHYMOD_IF_ERR_RETURN(READ_CL93N72_IT_BASE_R_PMD_CTLr(&pa_copy, &CL93N72_IEEE_TX_CL93N72IT_BASE_R_PMD_CONTROLr_reg));
            CL93N72_IT_BASE_R_PMD_CTLr_CL93N72_IEEE_TRAINING_ENABLEf_SET(CL93N72_IEEE_TX_CL93N72IT_BASE_R_PMD_CONTROLr_reg, cl72_enable);
            PHYMOD_IF_ERR_RETURN(MODIFY_CL93N72_IT_BASE_R_PMD_CTLr(&pa_copy, CL93N72_IEEE_TX_CL93N72IT_BASE_R_PMD_CONTROLr_reg));
        }

        pa_copy.lane_mask = 0x1 << start_lane;
        tefmod_gen3_disable_get(&pa_copy,&enable);
        if(enable == 0x1) {
            SC_X4_CTLr_CLR(SC_X4_CONTROL_SC_X4_CONTROL_CONTROLr_reg);
            SC_X4_CTLr_SW_SPEED_CHANGEf_SET(SC_X4_CONTROL_SC_X4_CONTROL_CONTROLr_reg, 0);
            PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_CTLr(pc, SC_X4_CONTROL_SC_X4_CONTROL_CONTROLr_reg));

            SC_X4_CTLr_CLR(SC_X4_CONTROL_SC_X4_CONTROL_CONTROLr_reg);
            SC_X4_CTLr_SW_SPEED_CHANGEf_SET(SC_X4_CONTROL_SC_X4_CONTROL_CONTROLr_reg, 1);
            PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_CTLr(&pa_copy, SC_X4_CONTROL_SC_X4_CONTROL_CONTROLr_reg));
        }
    }

    return PHYMOD_E_NONE;
} /* CLAUSE_72_CONTROL */


/*!
@brief Sets CJPAT/CRPAT parameters for a particular port
@param  pc handle to current TSCF context (#PHYMOD_ST)
@returns The value PHYMOD_E_NONE upon successful completion
@details
This function enables either a CJPAT or CRPAT for a particular port. 
*/

int tefmod_gen3_cjpat_crpat_control(PHYMOD_ST* pc)     /* CJPAT_CRPAT_CONTROL  */
{
    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);

    /* needed only for TSCF-B0 */
    return PHYMOD_E_NONE;
}

/*!
@brief Checks CJPAT/CRPAT parameters for a particular port
@param  pc handle to current TSCF context (#PHYMOD_ST)
@returns The value PHYMOD_E_NONE upon successful completion
@details
This function checks for CJPAT or CRPAT for a particular port. 
*/

int tefmod_gen3_cjpat_crpat_check(PHYMOD_ST* pc)     /* CJPAT_CRPAT_CHECK */
{
    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
    /* needed only for TSCF-B0 */
    return PHYMOD_E_NONE;  
}
/*!
@brief  Enables or disables the bit error rate testing for a particular lane.
@param  pc handle to current TSCF context (#PHYMOD_ST)
@returns The value PHYMOD_E_NONE upon successful completion.
@details

This function enables or disables bit-error-rate testing (BERT)

TBD
*/
int tefmod_gen3_tx_bert_control(PHYMOD_ST* pc)
{
    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
    /* needed only for TSCF-B0 */
    return PHYMOD_E_NONE;
}

/*!
@brief enables/disables FEC function.
@brief Controls PMD reset pins irrespective of PCS is in Speed Control mode or not
@param  pc        handle to current TSCF context (#PHYMOD_ST)
@param  fec_en    Bit encoded enable for various fec feature
                     TEFMOD_CL91_TX_EN_DIS       Enable CL91 TX
                     TEFMOD_CL91_RX_EN_DIS       Enable CL91 RX
                     TEFMOD_CL91_IND_ONLY_EN_DIS Enable CL91 indication only
                     TEFMOD_CL91_COR_ONLY_EN_DIS Enable CL91 correction only
                     TEFMOD_CL74_TX_EN_DIS       Enable CL74 TX
                     TEFMOD_CL74_RX_EN_DIS       Enable CL74 RX

@param  fec_dis   Bit encoded disable for various fec feature
                     TEFMOD_CL91_TX_EN_DIS       Disable CL91 TX
                     TEFMOD_CL91_RX_EN_DIS       Disable CL91 RX
                     TEFMOD_CL91_IND_ONLY_EN_DIS Disable CL91 indication only
                     TEFMOD_CL91_COR_ONLY_EN_DIS Disable CL91 correction only
                     TEFMOD_CL74_TX_EN_DIS       Disable CL74 TX
                     TEFMOD_CL74_RX_EN_DIS       Disable CL74 RX

@param  cl74or91  make 100G speed use cl74 after autoneg
                              bit 1-0 cl74or91 value

@returns PHYMOD_E_NONE if no errors. PHYMOD_E_ERROR else.

@details
To use cl91 in forced speed mode (in 100G only) Tx only fec_en=TEFMOD_CL91_TX_EN_DIS
To use cl91 in forced speed mode (in 100G only) Rx only fec_en=TEFMOD_CL91_RX_EN_DIS
To use cl91 in forced speed mode (in 100G only) Tx Rx   fec_en=(TEFMOD_CL91_TX_EN_DIS|TEFMOD_CL91_RX_EN_DIS)

To enable cl74 Tx only fec_en = TEFMOD_CL74_TX_EN_DIS
To disable cl74 Tx Rx fec_dis =(TEFMOD_CL74_TX_EN_DIS|TEFMOD_CL74_RX_EN_DIS)

Note: cl74 will be enabled per port.
      cl91 is used only in 100G (so all 4 lanes make a port)

      cl74or91 is only used in autoneg. And other parm used in forced speed.
*/


int tefmod_gen3_FEC_control(PHYMOD_ST* pc, tefmod_gen3_fec_type_t fec_type, int enable, int cl74or91)
{
    phymod_access_t pa_copy;
    int start_lane = 0, num_lane = 0 ;
    uint32_t local_enable = 0;
    uint32_t fec_mode;
    TX_X4_TX_CTL0r_t tx_x4_tx_ctrl_reg;
    RX_X4_PCS_CTL0r_t rx_x4_pcs_crtl_reg;
    TX_X4_MISCr_t TX_X4_CONTROL0_MISCr_reg;
    RX_X4_PCS_CTL0r_t RX_X4_CONTROL0_PCS_CONTROL_0r_reg;
    SC_X4_CTLr_t SC_X4_CONTROL_SC_X4_CONTROL_CONTROLr_reg;

    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
    PHYMOD_MEMCPY(&pa_copy, pc, sizeof(pa_copy));
    PHYMOD_IF_ERR_RETURN
        (phymod_util_lane_config_get(pc, &start_lane, &num_lane));
    pa_copy.lane_mask = 0x1 << start_lane;

    if(enable < TEFMOD_PHY_CONTROL_FEC_AUTO) {
        if (fec_type == TEFMOD_CL91) {
            READ_TX_X4_TX_CTL0r(pc, &tx_x4_tx_ctrl_reg);
            READ_RX_X4_PCS_CTL0r(pc, &rx_x4_pcs_crtl_reg);
            if (enable) { 
                switch (num_lane) {
                    case 1: 
                        fec_mode = 2;
                        break;
                    case 2: 
                        fec_mode = 3;
                        break;
                    case 4: 
                        fec_mode = 4;
                        break;
                    default:
                        fec_mode = 2;
                        break;
                }
            } else {
                fec_mode = 0;
            }
            TX_X4_TX_CTL0r_CL91_FEC_MODEf_SET(tx_x4_tx_ctrl_reg, fec_mode);
            RX_X4_PCS_CTL0r_CL91_FEC_MODEf_SET(rx_x4_pcs_crtl_reg, fec_mode);
            MODIFY_TX_X4_TX_CTL0r(pc, tx_x4_tx_ctrl_reg);  
            MODIFY_RX_X4_PCS_CTL0r(pc, rx_x4_pcs_crtl_reg);

        } else /* cl74 */ {
            READ_TX_X4_MISCr(pc, &TX_X4_CONTROL0_MISCr_reg);
            READ_RX_X4_PCS_CTL0r(pc,  &RX_X4_CONTROL0_PCS_CONTROL_0r_reg);
            if (enable) {
                TX_X4_MISCr_FEC_ENABLEf_SET(TX_X4_CONTROL0_MISCr_reg, 1);
                RX_X4_PCS_CTL0r_FEC_ENABLEf_SET(RX_X4_CONTROL0_PCS_CONTROL_0r_reg, 1);
            } else {
                TX_X4_MISCr_FEC_ENABLEf_SET(TX_X4_CONTROL0_MISCr_reg, 0);
                RX_X4_PCS_CTL0r_FEC_ENABLEf_SET(RX_X4_CONTROL0_PCS_CONTROL_0r_reg, 0);
            }
            PHYMOD_IF_ERR_RETURN(MODIFY_TX_X4_MISCr(pc, TX_X4_CONTROL0_MISCr_reg));
            PHYMOD_IF_ERR_RETURN(MODIFY_RX_X4_PCS_CTL0r(pc, RX_X4_CONTROL0_PCS_CONTROL_0r_reg));
        }
    }      

    /* CL91 AN handling */
    if(fec_type == TEFMOD_CL91) {
        SC_X4_SC_X4_OVRRr_t SC_X4_SC_X4_OVRR_reg;
        SC_X4_SC_X4_OVRRr_CLR(SC_X4_SC_X4_OVRR_reg);
        if(enable == TEFMOD_PHY_CONTROL_FEC_AUTO) {
            SC_X4_SC_X4_OVRRr_AN_FEC_SEL_OVERRIDEf_SET(SC_X4_SC_X4_OVRR_reg, 0x0);
            PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_SC_X4_OVRRr(pc, SC_X4_SC_X4_OVRR_reg));
        } else if(enable == TEFMOD_PHY_CONTROL_FEC_OFF) {
            SC_X4_SC_X4_OVRRr_AN_FEC_SEL_OVERRIDEf_SET(SC_X4_SC_X4_OVRR_reg, 0x1);
            PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_SC_X4_OVRRr(pc, SC_X4_SC_X4_OVRR_reg));
        }
    }
    tefmod_gen3_disable_get(&pa_copy,&local_enable);
    if (local_enable) {
        SC_X4_CTLr_CLR(SC_X4_CONTROL_SC_X4_CONTROL_CONTROLr_reg);
        SC_X4_CTLr_SW_SPEED_CHANGEf_SET(SC_X4_CONTROL_SC_X4_CONTROL_CONTROLr_reg, 0);
        PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_CTLr(pc, SC_X4_CONTROL_SC_X4_CONTROL_CONTROLr_reg));

        SC_X4_CTLr_CLR(SC_X4_CONTROL_SC_X4_CONTROL_CONTROLr_reg);
        SC_X4_CTLr_SW_SPEED_CHANGEf_SET(SC_X4_CONTROL_SC_X4_CONTROL_CONTROLr_reg, 1);
        PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_CTLr(&pa_copy, SC_X4_CONTROL_SC_X4_CONTROL_CONTROLr_reg));
    }
  
    return PHYMOD_E_NONE;
}


int tefmod_gen3_FEC_get(PHYMOD_ST* pc,  tefmod_gen3_fec_type_t fec_type, int *fec_en) 
{
    SC_X4_FEC_STSr_t SC_X4_FEC_STSr_reg;
    uint32_t fec_mode;

    *fec_en = 0;
    SC_X4_FEC_STSr_CLR(SC_X4_FEC_STSr_reg);
    READ_SC_X4_FEC_STSr(pc, &SC_X4_FEC_STSr_reg);
    fec_mode = SC_X4_FEC_STSr_T_CL91_FEC_MODEf_GET(SC_X4_FEC_STSr_reg);

    if (fec_type == TEFMOD_CL91) {
        if (fec_mode == 0) {
            *fec_en = 0;
        } else {
            *fec_en = 1;
        }
    } else {
        *fec_en = SC_X4_FEC_STSr_T_FEC_ENABLEf_GET(SC_X4_FEC_STSr_reg);
    } 
                
    return PHYMOD_E_NONE;
}

/*!
@brief Power down transmitter or receiver per lane basis.
@param  pc handle to current TSCF context (a.k.a #PHYMOD_ST)
@param tx control for power of TX path
@param rx control for power of RX path
@returns PHYMOD_E_NONE if no errors. PHYMOD_EERROR else.
@details
Indi
*/
int tefmod_gen3_power_control(PHYMOD_ST* pc, int tx, int rx)
{
    PMD_X4_CTLr_t PMD_X4_PMD_X4_CONTROLr_reg;

    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
    PMD_X4_CTLr_CLR(PMD_X4_PMD_X4_CONTROLr_reg);

    PMD_X4_CTLr_LN_RX_H_PWRDNf_SET(PMD_X4_PMD_X4_CONTROLr_reg, rx);
    PMD_X4_CTLr_LN_TX_H_PWRDNf_SET(PMD_X4_PMD_X4_CONTROLr_reg, tx);
    PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X4_CTLr(pc, PMD_X4_PMD_X4_CONTROLr_reg));

    return PHYMOD_E_NONE;
}

/*!
@brief sets the lanes in Full Duplex/ half duplex mode.
@param  pc handle to current TSCF context (a.k.a tefmod struct)
@returns PHYMOD_E_NONE if no errors. PHYMOD_EERROR else.
@details
This is bit encoded function. Bit 3 to 0 is for lane 3 to 0. High sets full
duplex. Low is for half duplex.

<B>Currently, this function is not implemented</B>
*/
int tefmod_gen3_duplex_control(PHYMOD_ST* pc)
{
    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
    /* needed for B0 */
    return PHYMOD_E_NONE;
}

/*!
@brief Set reference clock
@param  pc handle to current TSCF context (a.k.a tefmod struct)
@param  ref_clk The reference clock to set the PHY to.
@returns PHYMOD_E_NONE if no errors. PHYMOD_EERROR else.
@details
The reference clock is set to inform the micro. The value written into the
register is interpreted by the micro as thus.

    refclk = regVal * 2 + 100;

Since the refclk field is 7 bits, the range is 100 - 228

*/

int tefmod_gen3_refclk_get(PHYMOD_ST* pc, phymod_ref_clk_t* ref_clk)
{
    MAIN0_SETUPr_t MAIN0_SETUPr_reg;

    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
    MAIN0_SETUPr_CLR(MAIN0_SETUPr_reg);
    READ_MAIN0_SETUPr(pc, &MAIN0_SETUPr_reg);
    *ref_clk = MAIN0_SETUPr_REFCLK_SELf_GET(MAIN0_SETUPr_reg);

    return PHYMOD_E_NONE;
}

int tefmod_gen3_pmd_lane_swap(PHYMOD_ST *pc, int tx_lane_swap, int rx_lane_swap)
{
    unsigned int tx_map, rx_map;
    LN_ADDR0r_t LN_ADDR0r_reg;
    LN_ADDR1r_t LN_ADDR1r_reg;
    LN_ADDR2r_t LN_ADDR2r_reg;
    LN_ADDR3r_t LN_ADDR3r_reg;

    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
    tx_map = (unsigned int)tx_lane_swap;
    rx_map = (unsigned int)rx_lane_swap;

    LN_ADDR0r_CLR(LN_ADDR0r_reg);
    LN_ADDR1r_CLR(LN_ADDR1r_reg);
    LN_ADDR2r_CLR(LN_ADDR2r_reg);
    LN_ADDR3r_CLR(LN_ADDR3r_reg);

    LN_ADDR0r_TX_LANE_ADDR_0f_SET(LN_ADDR0r_reg, ((tx_map >> 0)  & 0xf));
    LN_ADDR1r_TX_LANE_ADDR_1f_SET(LN_ADDR1r_reg, ((tx_map >> 4)  & 0xf));
    LN_ADDR2r_TX_LANE_ADDR_2f_SET(LN_ADDR2r_reg, ((tx_map >> 8)  & 0xf));
    LN_ADDR3r_TX_LANE_ADDR_3f_SET(LN_ADDR3r_reg, ((tx_map >> 12) & 0xf));

    LN_ADDR0r_RX_LANE_ADDR_0f_SET(LN_ADDR0r_reg, ((rx_map >> 0)  & 0xf));
    LN_ADDR1r_RX_LANE_ADDR_1f_SET(LN_ADDR1r_reg, ((rx_map >> 4)  & 0xf));
    LN_ADDR2r_RX_LANE_ADDR_2f_SET(LN_ADDR2r_reg, ((rx_map >> 8)  & 0xf));
    LN_ADDR3r_RX_LANE_ADDR_3f_SET(LN_ADDR3r_reg, ((rx_map >> 12) & 0xf));

    PHYMOD_IF_ERR_RETURN(MODIFY_LN_ADDR0r(pc, LN_ADDR0r_reg));
    PHYMOD_IF_ERR_RETURN(MODIFY_LN_ADDR1r(pc, LN_ADDR1r_reg));
    PHYMOD_IF_ERR_RETURN(MODIFY_LN_ADDR2r(pc, LN_ADDR2r_reg));
    PHYMOD_IF_ERR_RETURN(MODIFY_LN_ADDR3r(pc, LN_ADDR3r_reg));
 
    return PHYMOD_E_NONE;
}

int tefmod_gen3_pcs_tx_lane_swap(PHYMOD_ST *pc, int tx_lane_swap)
{
    unsigned int map;
    TX_X1_TX_LN_SWPr_t TX_X1_TX_LN_SWPr_reg;

    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
    map = (unsigned int)tx_lane_swap;

    TX_X1_TX_LN_SWPr_CLR(TX_X1_TX_LN_SWPr_reg);
    TX_X1_TX_LN_SWPr_LOGICAL2_TO_PHY_SELf_SET(TX_X1_TX_LN_SWPr_reg, ((map >> 8)  & 0x3));
    TX_X1_TX_LN_SWPr_LOGICAL3_TO_PHY_SELf_SET(TX_X1_TX_LN_SWPr_reg, ((map >> 12) & 0x3));
    TX_X1_TX_LN_SWPr_LOGICAL1_TO_PHY_SELf_SET(TX_X1_TX_LN_SWPr_reg, ((map >> 4)  & 0x3));
    TX_X1_TX_LN_SWPr_LOGICAL0_TO_PHY_SELf_SET(TX_X1_TX_LN_SWPr_reg, ((map >> 0)  & 0x3));
    PHYMOD_IF_ERR_RETURN(MODIFY_TX_X1_TX_LN_SWPr(pc, TX_X1_TX_LN_SWPr_reg));
 
    return PHYMOD_E_NONE;
}

int tefmod_gen3_pcs_rx_lane_swap(PHYMOD_ST *pc, int rx_lane_swap)
{
  unsigned int map;
  RX_X1_RX_LN_SWPr_t RX_X1_RX_LN_SWPr_reg;

  TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
  map = (unsigned int)rx_lane_swap;

  RX_X1_RX_LN_SWPr_CLR(RX_X1_RX_LN_SWPr_reg);
  RX_X1_RX_LN_SWPr_LOGICAL3_TO_PHY_SELf_SET(RX_X1_RX_LN_SWPr_reg, ((map >> 12) & 0x3));
  RX_X1_RX_LN_SWPr_LOGICAL2_TO_PHY_SELf_SET(RX_X1_RX_LN_SWPr_reg, ((map >> 8)  & 0x3));
  RX_X1_RX_LN_SWPr_LOGICAL1_TO_PHY_SELf_SET(RX_X1_RX_LN_SWPr_reg, ((map >> 4)  & 0x3));
  RX_X1_RX_LN_SWPr_LOGICAL0_TO_PHY_SELf_SET(RX_X1_RX_LN_SWPr_reg, ((map >> 0)  & 0x3));

  PHYMOD_IF_ERR_RETURN(MODIFY_RX_X1_RX_LN_SWPr(pc, RX_X1_RX_LN_SWPr_reg));

  return PHYMOD_E_NONE;
}

int tefmod_gen3_pcs_tx_lane_swap_get ( PHYMOD_ST *pc,  uint32_t *tx_swap)
{
  unsigned int pcs_map;
  TX_X1_TX_LN_SWPr_t reg;

  TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);

  TX_X1_TX_LN_SWPr_CLR(reg);

  PHYMOD_IF_ERR_RETURN
      (READ_TX_X1_TX_LN_SWPr(pc, &reg));

  pcs_map = TX_X1_TX_LN_SWPr_GET(reg);

  *tx_swap = (((pcs_map >> 0)  & 0x3) << 0) |
                (((pcs_map >> 2)  & 0x3) << 4) |
                (((pcs_map >> 4)  & 0x3) << 8) |
                (((pcs_map >> 6)  & 0x3) << 12) ;

  return PHYMOD_E_NONE ;
}

int tefmod_gen3_pcs_rx_lane_swap_get ( PHYMOD_ST *pc,  uint32_t *rx_swap)
{
  unsigned int pcs_map;
  RX_X1_RX_LN_SWPr_t reg;

  TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);

  RX_X1_RX_LN_SWPr_CLR(reg);

  PHYMOD_IF_ERR_RETURN
      (READ_RX_X1_RX_LN_SWPr(pc, &reg)) ;

  pcs_map = RX_X1_RX_LN_SWPr_GET(reg);

  *rx_swap = (((pcs_map >> 0)  & 0x3) << 0) |
                (((pcs_map >> 2)  & 0x3) << 4) |
                (((pcs_map >> 4)  & 0x3) << 8) |
                (((pcs_map >> 6)  & 0x3) << 12) ;

  return PHYMOD_E_NONE ;
}


/*!
@brief tefmod_poll_for_sc_done , polls for the sc_done bit ,  
@returns The value PHYMOD_E_NONE upon successful completion.
@details
Reads the SC_X4_COntrol_status reg and polls it to see if sc_done bit is set
Sets the sc_done flag=1/0, if speed control is done, after resolving to the correct speed
*/
int tefmod_gen3_poll_for_sc_done(PHYMOD_ST* pc, int mapped_speed)
{
    int done;
    int spd_match, sc_done;
    int cnt;
    uint16_t data;
    SC_X4_STSr_t SC_X4_CONTROL_SC_X4_CONTROL_STATUSr_reg;
    SC_X4_RSLVD_SPDr_t SC_X4_FINAL_CONFIG_STATUS_RESOLVED_SPEEDr_reg;

    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
    cnt=0;
    sc_done = 0;
    spd_match = 0;

    SC_X4_STSr_CLR(SC_X4_CONTROL_SC_X4_CONTROL_STATUSr_reg);
    while (cnt <=2000) {
        PHYMOD_IF_ERR_RETURN(READ_SC_X4_STSr(pc, &SC_X4_CONTROL_SC_X4_CONTROL_STATUSr_reg));
        cnt = cnt + 1;
        if(SC_X4_STSr_SW_SPEED_CHANGE_DONEf_GET(SC_X4_CONTROL_SC_X4_CONTROL_STATUSr_reg)) {
            sc_done = 1;
            break;
        } else {
            if(cnt == 2000) {
                sc_done = 0;
                break;
            }
        }
    }

    cnt=0;
    while (cnt <=400) {
        SC_X4_RSLVD_SPDr_CLR(SC_X4_FINAL_CONFIG_STATUS_RESOLVED_SPEEDr_reg);
        PHYMOD_IF_ERR_RETURN(READ_SC_X4_RSLVD_SPDr(pc, &SC_X4_FINAL_CONFIG_STATUS_RESOLVED_SPEEDr_reg));
        data = SC_X4_RSLVD_SPDr_SPEEDf_GET(SC_X4_FINAL_CONFIG_STATUS_RESOLVED_SPEEDr_reg);
        cnt = cnt + 1;
        if(data == mapped_speed) {
            spd_match = 1;
            break;
        } else {
            if(cnt == 400) {
                spd_match = 0;
                break;
            }
        }
    } 
    if(sc_done && spd_match) {
        done = 1;
    } else {
        done = 0;
    }

   return done;
}

/*!
@brief tefmod_set_port_mode_sel , selects the port_mode,  
@returns The value PHYMOD_E_NONE upon successful completion.
@details
This is used when we are not using any speed control logic, but instead when bypassing this entire SC logic, this has to be programmed
*/
#if 0
int tefmod_gen3_set_port_mode_sel(PHYMOD_ST* pc, int tsc_touched, tefmod_gen3_port_type_t port_type)
{

    int port_mode_sel;
    uint16_t single_port_mode;
    MAIN0_SETUPr_t MAIN0_SETUPr_reg;
 
    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
    port_mode_sel = 0;
    if (tsc_touched == 1){
        single_port_mode = 0;
    } else {
       single_port_mode = 1;
    }

    switch(port_type) {
        case TEFMOD_MULTI_PORT:   port_mode_sel = 0;  break;
        case TEFMOD_TRI1_PORT:    port_mode_sel = 2;  break;
        case TEFMOD_TRI2_PORT:    port_mode_sel = 1;  break;
        case TEFMOD_DXGXS:        port_mode_sel = 3;  break;
        case TEFMOD_SINGLE_PORT:  port_mode_sel = 4;  break;
        default: break;
    }

    MAIN0_SETUPr_CLR(MAIN0_SETUPr_reg);
    MAIN0_SETUPr_PORT_MODE_SELf_SET(MAIN0_SETUPr_reg, port_mode_sel);
    MAIN0_SETUPr_SINGLE_PORT_MODEf_SET(MAIN0_SETUPr_reg, single_port_mode);
    PHYMOD_IF_ERR_RETURN(MODIFY_MAIN0_SETUPr(pc, MAIN0_SETUPr_reg));

    return PHYMOD_E_NONE;
}
#endif

/*!
@brief firmware load request 
@param  pc handle to current TSCF context (#PHYMOD_ST)
@returns The value PHYMOD_E_NONE upon successful completion.
@details
*/
int tefmod_gen3_firmware_set(PHYMOD_ST* pc)
{
  TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
  return PHYMOD_E_NONE;
}

#ifdef _SDK_TEFMOD_GEN3_
/*!
@brief Controls the init setting/resetting of autoneg  registers.
@param  pc handle to current TSCF context (#PHYMOD_ST)
@param  an_init_st structure tefmod_an_init_t with all the one time autoneg init cfg.
@returns PHYMOD_E_NONE if no errors. PHYMOD_E_FAIL else.
@details Get aneg features via #tefmod_an_init_t.
  This does not start the autoneg. That is done in tefmod_autoneg_control
*/
int tefmod_gen3_autoneg_set_init(PHYMOD_ST* pc, tefmod_gen3_an_init_t *an_init_st) /* AUTONEG_SET */
{
    AN_X4_CL73_CTLSr_t AN_X4_CL73_CTLSr_t_reg;
    AN_X4_LD_BASE_ABIL1r_t AN_X4_ABILITIES_LD_BASE_ABILITIES_1r_reg;
    AN_X4_LD_BASE_ABIL0r_t AN_X4_ABILITIES_LD_BASE_ABILITIES_0r_reg;
    AN_X4_CL73_CFGr_t AN_X4_CL73_CFGr_t_reg;

    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
    AN_X4_CL73_CTLSr_CLR(AN_X4_CL73_CTLSr_t_reg);
    AN_X4_CL73_CTLSr_AN_GOOD_TRAPf_SET(AN_X4_CL73_CTLSr_t_reg, an_init_st->an_good_trap);
    AN_X4_CL73_CTLSr_AN_GOOD_CHECK_TRAPf_SET(AN_X4_CL73_CTLSr_t_reg, an_init_st->an_good_check_trap);
    AN_X4_CL73_CTLSr_LINKFAILTIMER_DISf_SET(AN_X4_CL73_CTLSr_t_reg, an_init_st->linkfailtimer_dis);
    AN_X4_CL73_CTLSr_LINKFAILTIMERQUAL_ENf_SET(AN_X4_CL73_CTLSr_t_reg, an_init_st->linkfailtimerqua_en);
    AN_X4_CL73_CTLSr_AN_FAIL_COUNT_LIMITf_SET(AN_X4_CL73_CTLSr_t_reg, an_init_st->an_fail_cnt);
    AN_X4_CL73_CTLSr_AN_OUI_OVERRIDE_BAM73_ADVf_SET(AN_X4_CL73_CTLSr_t_reg, an_init_st->bam73_adv_oui);
    AN_X4_CL73_CTLSr_AN_OUI_OVERRIDE_BAM73_DETf_SET(AN_X4_CL73_CTLSr_t_reg, an_init_st->bam73_det_oui);
    AN_X4_CL73_CTLSr_AN_OUI_OVERRIDE_HPAM_ADVf_SET(AN_X4_CL73_CTLSr_t_reg, an_init_st->hpam_adv_oui);
    AN_X4_CL73_CTLSr_AN_OUI_OVERRIDE_HPAM_DETf_SET(AN_X4_CL73_CTLSr_t_reg, an_init_st->hpam_det_oui);
    PHYMOD_IF_ERR_RETURN(MODIFY_AN_X4_CL73_CTLSr(pc, AN_X4_CL73_CTLSr_t_reg));

    AN_X4_LD_BASE_ABIL1r_CLR(AN_X4_ABILITIES_LD_BASE_ABILITIES_1r_reg);
    if(an_init_st->disable_rf_report == 1) {
        AN_X4_LD_BASE_ABIL1r_CL73_REMOTE_FAULTf_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_1r_reg, 1);
    }
    PHYMOD_IF_ERR_RETURN(MODIFY_AN_X4_LD_BASE_ABIL1r(pc, AN_X4_ABILITIES_LD_BASE_ABILITIES_1r_reg));

    AN_X4_LD_BASE_ABIL0r_CLR(AN_X4_ABILITIES_LD_BASE_ABILITIES_0r_reg);
    AN_X4_LD_BASE_ABIL0r_TX_NONCEf_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_0r_reg, (an_init_st->cl73_transmit_nonce) & 0x1f);
    PHYMOD_IF_ERR_RETURN(MODIFY_AN_X4_LD_BASE_ABIL0r(pc, AN_X4_ABILITIES_LD_BASE_ABILITIES_0r_reg));

    AN_X4_CL73_CFGr_CLR(AN_X4_CL73_CFGr_t_reg);
    AN_X4_CL73_CFGr_CL73_NONCE_MATCH_OVERf_SET(AN_X4_CL73_CFGr_t_reg, an_init_st->cl73_nonce_match_over);
    AN_X4_CL73_CFGr_CL73_NONCE_MATCH_VALf_SET(AN_X4_CL73_CFGr_t_reg,  an_init_st->cl73_nonce_match_val);
    PHYMOD_IF_ERR_RETURN(MODIFY_AN_X4_CL73_CFGr(pc, AN_X4_CL73_CFGr_t_reg));

    return PHYMOD_E_NONE;
}
#endif /* _SDK_TEFMOD_GEN3_ */


/*!
@brief Controls the setting/resetting of autoneg timers.
@param  pc handle to current TSCF context (#PHYMOD_ST)
@returns PHYMOD_E_NONE if no errors. PHYMOD_E_FAIL else.
@details

*/
int tefmod_gen3_autoneg_timer_init(PHYMOD_ST* pc)               /* AUTONEG timer set*/
{
    AN_X1_IGNORE_LNK_TMRr_t AN_X1_IGNORE_LNK_TMRr_reg;
    AN_X1_CL73_DME_LOCKr_t AN_X1_TIMERS_CL73_DME_LOCKr_reg;
    AN_X1_PD_SD_TMRr_t AN_X1_TIMERS_PD_SD_TIMERr_reg;
    AN_X1_CL73_BRK_LNKr_t AN_X1_TIMERS_CL73_BREAK_LINKr_reg;
    AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72r_t AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_NOT_CL72r_reg;
    AN_X1_LNK_FAIL_INHBT_TMR_CL72r_t AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_CL72r_reg;
  
    AN_X1_IGNORE_LNK_TMRr_SET(AN_X1_IGNORE_LNK_TMRr_reg, 0x29a );
    PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_IGNORE_LNK_TMRr(pc,AN_X1_IGNORE_LNK_TMRr_reg));

    AN_X1_CL73_DME_LOCKr_SET(AN_X1_TIMERS_CL73_DME_LOCKr_reg, 0x14d4);
    PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_CL73_DME_LOCKr(pc, AN_X1_TIMERS_CL73_DME_LOCKr_reg));

    AN_X1_PD_SD_TMRr_SET(AN_X1_TIMERS_PD_SD_TIMERr_reg, 0xa6a);
    PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_PD_SD_TMRr(pc, AN_X1_TIMERS_PD_SD_TIMERr_reg));

    AN_X1_CL73_BRK_LNKr_SET(AN_X1_TIMERS_CL73_BREAK_LINKr_reg, 0x10ed);
    PHYMOD_IF_ERR_RETURN (WRITE_AN_X1_CL73_BRK_LNKr(pc, AN_X1_TIMERS_CL73_BREAK_LINKr_reg));

    /* AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72r_SET(AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_NOT_CL72r_reg, 0xbb8); */
    /* Change based on finding in eagle */
    AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72r_SET(AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_NOT_CL72r_reg, 0x3080);
    PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72r(pc, AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_NOT_CL72r_reg));

    AN_X1_LNK_FAIL_INHBT_TMR_CL72r_SET(AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_CL72r_reg, 0x8382);
    PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_LNK_FAIL_INHBT_TMR_CL72r(pc, AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_CL72r_reg));

    return PHYMOD_E_NONE;
}

int tefmod_gen3_default_init(PHYMOD_ST* pc)               /* default set*/
{
    MAIN0_SPD_CTLr_t main_spd_ctlr_reg;
    MAIN0_SPD_CTLr_CLR(main_spd_ctlr_reg);
    /* Enables generation of credit based on tsc_clk */
    MAIN0_SPD_CTLr_TSC_CREDIT_SELf_SET(main_spd_ctlr_reg, 1);
    PHYMOD_IF_ERR_RETURN(MODIFY_MAIN0_SPD_CTLr(pc, main_spd_ctlr_reg));

    return PHYMOD_E_NONE;
}

#if 0
int tefmod_gen3_toggle_sw_speed_change(PHYMOD_ST* pc)
{
    int cnt;
    int sw_sp_cfg_vld;
    SC_X4_CTLr_t SC_X4_CTLr_reg;
    SC_X4_STSr_t SC_X4_STSr_reg;

    cnt=0;
    sw_sp_cfg_vld = 0;

    SC_X4_CTLr_CLR(SC_X4_CTLr_reg);
    SC_X4_CTLr_SW_SPEED_CHANGEf_SET(SC_X4_CTLr_reg, 0);
    PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_CTLr(pc, SC_X4_CTLr_reg));

    SC_X4_CTLr_CLR(SC_X4_CTLr_reg);
    SC_X4_CTLr_SW_SPEED_CHANGEf_SET(SC_X4_CTLr_reg, 1);
    SC_X4_CTLr_SPEEDf_SET(SC_X4_CTLr_reg, 0);
    PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_CTLr(pc, SC_X4_CTLr_reg));

    while (cnt <=2000) {
        SC_X4_STSr_CLR(SC_X4_STSr_reg);
        PHYMOD_IF_ERR_RETURN(READ_SC_X4_STSr(pc, &SC_X4_STSr_reg));
        cnt = cnt + 1;
        if(SC_X4_STSr_SW_SPEED_CONFIG_VLDf_GET(SC_X4_STSr_reg)) {
           sw_sp_cfg_vld = 1;
            break;
        }
    }

    SC_X4_CTLr_CLR(SC_X4_CTLr_reg);
    SC_X4_CTLr_SW_SPEED_CHANGEf_SET(SC_X4_CTLr_reg, 0);
    PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_CTLr(pc, SC_X4_CTLr_reg));
  
    if(!sw_sp_cfg_vld) {
        return PHYMOD_E_FAIL;
    }

    return PHYMOD_E_NONE;
}
#endif
/**
@brief   To get autoneg control registers.
@param   pc handle to current TSCF context (#PHYMOD_ST)
@param   an_control reference to struct with autoneg parms #tefmod_an_control_t
@param   an_complete status of AN completion
@returns PHYMOD_E_NONE if no errors. PHYMOD_E_FAIL else.
@details Upper layer software calls this function to get autoneg
         info.
*/

int tefmod_gen3_autoneg_control_get(PHYMOD_ST* pc, tefmod_gen3_an_control_t *an_control, int *an_complete)
{
    AN_X4_CL73_CFGr_t  AN_X4_CL73_CFGr_reg;
    AN_X4_CL73_CTLSr_t AN_X4_CL73_CTLSr_reg;
    AN_X4_AN_MISC_STSr_t  AN_X4_AN_MISC_STSr_reg;
    AN_X4_LD_BASE_ABIL3r_t AN_X4_LD_BASE_ABIL3r_reg;
    AN_X4_LD_BASE_ABIL4r_t AN_X4_LD_BASE_ABIL4r_reg;
    AN_X4_LD_BASE_ABIL5r_t AN_X4_LD_BASE_ABIL5r_reg;
    uint32_t    msa_mode, base_ability,base_ability_1, base_ability_2;
  
    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
    
    PHYMOD_IF_ERR_RETURN(READ_AN_X4_CL73_CTLSr(pc, &AN_X4_CL73_CTLSr_reg));
    an_control->pd_kx_en = AN_X4_CL73_CTLSr_PD_KX_ENf_GET(AN_X4_CL73_CTLSr_reg);
    an_control->pd_2P5G_kx_en = AN_X4_CL73_CTLSr_PD_2P5G_KX_ENf_GET(AN_X4_CL73_CTLSr_reg);

    /*Setting X4 abilities*/
    PHYMOD_IF_ERR_RETURN(READ_AN_X4_CL73_CFGr(pc, &AN_X4_CL73_CFGr_reg));

    if (AN_X4_CL73_CFGr_CL73_BAM_ENABLEf_GET(AN_X4_CL73_CFGr_reg) == 1) {
        PHYMOD_IF_ERR_RETURN(tefmod_gen3_an_msa_mode_get(pc, &msa_mode)); 
        if (msa_mode) {
            /* 
             * Read the msa_mode value.
             * If it is 1, it is a MSA/IEEE_MSA AN mode.
             * Read the base page values (abil_3 & abil_4).
             * If it is set, then set AN mode to IEEE_MSA, else set it to MSA mode. 
 */
               
            PHYMOD_IF_ERR_RETURN(READ_AN_X4_LD_BASE_ABIL3r(pc, &AN_X4_LD_BASE_ABIL3r_reg));
            PHYMOD_IF_ERR_RETURN(READ_AN_X4_LD_BASE_ABIL4r(pc, &AN_X4_LD_BASE_ABIL4r_reg));
            PHYMOD_IF_ERR_RETURN(READ_AN_X4_LD_BASE_ABIL5r(pc, &AN_X4_LD_BASE_ABIL5r_reg));
            base_ability =  AN_X4_LD_BASE_ABIL3r_GET(AN_X4_LD_BASE_ABIL3r_reg) & 0x0fff;
            base_ability_1 = AN_X4_LD_BASE_ABIL4r_GET(AN_X4_LD_BASE_ABIL4r_reg) & 0x0fff;
            base_ability_2 = AN_X4_LD_BASE_ABIL5r_GET(AN_X4_LD_BASE_ABIL5r_reg) & 0x0fff;

            if (!(base_ability) && !(base_ability_1) && !(base_ability_2)) {
                an_control->an_type = TEFMOD_AN_MODE_MSA;
                an_control->enable = 1;
            } else {
                an_control->an_type = TEFMOD_AN_MODE_CL73_MSA;
                an_control->enable = 1;
            }
        } else {
            an_control->an_type = TEFMOD_AN_MODE_CL73BAM;
            an_control->enable = 1;
        }
    } else if (AN_X4_CL73_CFGr_CL73_HPAM_ENABLEf_GET(AN_X4_CL73_CFGr_reg) == 1) {
        an_control->an_type = TEFMOD_AN_MODE_HPAM;
        an_control->enable = 1;
    } else if (AN_X4_CL73_CFGr_CL73_ENABLEf_GET(AN_X4_CL73_CFGr_reg) == 1) {
        an_control->an_type = TEFMOD_AN_MODE_CL73;
        an_control->enable = 1;
    } else {
        an_control->an_type = TEFMOD_AN_MODE_NONE;
        an_control->enable = 0;
    }

    if(AN_X4_CL73_CFGr_AD_TO_CL73_ENf_GET(AN_X4_CL73_CFGr_reg) == 1) {
        an_control->an_property_type = TEFMOD_AN_PROPERTY_ENABLE_HPAM_TO_CL73_AUTO;
    } else if(AN_X4_CL73_CFGr_BAM_TO_HPAM_AD_ENf_GET(AN_X4_CL73_CFGr_reg) == 1) {
        an_control->an_property_type = TEFMOD_AN_PROPERTY_ENABLE_CL73_BAM_TO_HPAM_AUTO;
    }

    an_control->num_lane_adv = AN_X4_CL73_CFGr_NUM_ADVERTISED_LANESf_GET(AN_X4_CL73_CFGr_reg);

    /* an_complete status */
    AN_X4_AN_MISC_STSr_CLR(AN_X4_AN_MISC_STSr_reg);
    PHYMOD_IF_ERR_RETURN (READ_AN_X4_AN_MISC_STSr(pc, &AN_X4_AN_MISC_STSr_reg));
    *an_complete = AN_X4_AN_MISC_STSr_AN_COMPLETEf_GET(AN_X4_AN_MISC_STSr_reg);

    return PHYMOD_E_NONE;
}



/**
@brief   To get local autoneg advertisement registers.
@param   pc handle to current TSCF context (#PHYMOD_ST)
@param   an_ability_st receives autoneg info. #tefmod_an_adv_ability_t)
@returns PHYMOD_E_NONE if no errors. PHYMOD_E_FAIL else.
@details Upper layer software calls this function to get local autoneg
         info. This function is currently not implemented
*/

int tefmod_gen3_autoneg_ability_get(PHYMOD_ST* pc, tefmod_gen3_an_adv_ability_t *an_ability_st)
{ 
    AN_X4_LD_BASE_ABIL1r_t AN_X4_LD_BASE_ABIL1r_reg;
    AN_X4_LD_UP1_ABIL0r_t  AN_X4_LD_UP1_ABIL0r_reg;
    AN_X4_LD_UP1_ABIL1r_t  AN_X4_LD_UP1_ABIL1r_reg;
    AN_X4_LD_BASE_ABIL3r_t AN_X4_LD_BASE_ABIL3r_reg;
    AN_X4_LD_BASE_ABIL4r_t AN_X4_LD_BASE_ABIL4r_reg;
    AN_X4_LD_BASE_ABIL5r_t AN_X4_LD_BASE_ABIL5r_reg;
    AN_X4_LD_FEC_BASEPAGE_ABILr_t AN_X4_LD_FEC_BASEPAGE_ABILr_reg;
    int         an_fec = 0;

    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
  
    /****** Getting local device UP1 abilities 0xC1C1 ******/ 
    READ_AN_X4_LD_UP1_ABIL0r(pc, &AN_X4_LD_UP1_ABIL0r_reg); 
    an_ability_st->an_bam_speed = AN_X4_LD_UP1_ABIL0r_GET(AN_X4_LD_UP1_ABIL0r_reg) & 0x3ff;
    an_ability_st->an_hg2 = AN_X4_LD_UP1_ABIL0r_BAM_HG2f_GET(AN_X4_LD_UP1_ABIL0r_reg);

    /****** Getting local device UP1 abilities_1 0xC1C2 ******/ 
    READ_AN_X4_LD_UP1_ABIL1r(pc, &AN_X4_LD_UP1_ABIL1r_reg); 
    an_ability_st->an_bam_speed1 = AN_X4_LD_UP1_ABIL1r_GET(AN_X4_LD_UP1_ABIL1r_reg) & 0x1f;

    /****** Getting local device base abilities_1 0xC1C4 ******/ 
    READ_AN_X4_LD_BASE_ABIL1r(pc, &AN_X4_LD_BASE_ABIL1r_reg);
    an_ability_st->an_base_speed = AN_X4_LD_BASE_ABIL1r_GET(AN_X4_LD_BASE_ABIL1r_reg) & 0x3f;

    /****** Getting local device base abilities_1 0xC1C4 ******/ 
    an_ability_st->an_pause = AN_X4_LD_BASE_ABIL1r_CL73_PAUSEf_GET(AN_X4_LD_BASE_ABIL1r_reg);
  
    an_fec = AN_X4_LD_BASE_ABIL1r_FEC_REQf_GET(AN_X4_LD_BASE_ABIL1r_reg);
    if (an_fec == 0x3) { /* FEC_SUPPORTED_AND_REQUESTED */
        if (AN_X4_LD_BASE_ABIL1r_BASE_100G_CR4f_GET(AN_X4_LD_BASE_ABIL1r_reg) ||
            AN_X4_LD_BASE_ABIL1r_BASE_100G_KR4f_GET(AN_X4_LD_BASE_ABIL1r_reg) ) {
            an_ability_st->an_fec = TEFMOD_FEC_CL91_SUPRTD_REQSTD;
        } else {
            an_ability_st->an_fec = TEFMOD_FEC_CL74_SUPRTD_REQSTD;
        }
    } else {
        an_ability_st->an_fec = (an_fec==0x1) ? TEFMOD_FEC_SUPRTD_NOT_REQSTD : TEFMOD_FEC_NOT_SUPRTD;
    }
 
    an_fec = 0;
    AN_X4_LD_BASE_ABIL3r_CLR(AN_X4_LD_BASE_ABIL3r_reg);
    AN_X4_LD_BASE_ABIL4r_CLR(AN_X4_LD_BASE_ABIL4r_reg);
    AN_X4_LD_BASE_ABIL5r_CLR(AN_X4_LD_BASE_ABIL5r_reg);
    AN_X4_LD_FEC_BASEPAGE_ABILr_CLR(AN_X4_LD_FEC_BASEPAGE_ABILr_reg);

    PHYMOD_IF_ERR_RETURN(READ_AN_X4_LD_BASE_ABIL3r(pc, &AN_X4_LD_BASE_ABIL3r_reg));
    PHYMOD_IF_ERR_RETURN(READ_AN_X4_LD_BASE_ABIL4r(pc, &AN_X4_LD_BASE_ABIL4r_reg));
    PHYMOD_IF_ERR_RETURN(READ_AN_X4_LD_BASE_ABIL5r(pc, &AN_X4_LD_BASE_ABIL5r_reg));
      
    /****** Getting local device base abilities_3 0xC1C8 ******/ 
    if (AN_X4_LD_BASE_ABIL3r_BASE_25G_KR1_ENf_GET(AN_X4_LD_BASE_ABIL3r_reg)) {
        an_ability_st->an_base_speed |= (1<<TEFMOD_CL73_25GBASE_KR1);
    }
    if (AN_X4_LD_BASE_ABIL3r_BASE_25G_CR1_ENf_GET(AN_X4_LD_BASE_ABIL3r_reg)) {
        an_ability_st->an_base_speed |= (1<<TEFMOD_CL73_25GBASE_CR1);
    }
    /****** Getting local device base abilities_4 0xC1C9 ******/ 
    if (AN_X4_LD_BASE_ABIL4r_BASE_25G_KRS1_ENf_GET(AN_X4_LD_BASE_ABIL4r_reg)) {
        an_ability_st->an_base_speed |= (1<<TEFMOD_CL73_25GBASE_KRS1);
    }
    if (AN_X4_LD_BASE_ABIL4r_BASE_25G_CRS1_ENf_GET(AN_X4_LD_BASE_ABIL4r_reg)) {
        an_ability_st->an_base_speed |= (1<<TEFMOD_CL73_25GBASE_CRS1);
    }
    /****** Getting local device base abilities_5 0xC1CA ******/ 
    if (AN_X4_LD_BASE_ABIL5r_BASE_2P5G_ENf_GET(AN_X4_LD_BASE_ABIL5r_reg)) {
        an_ability_st->an_base_speed |= (1<<TEFMOD_CL73_2P5GBASE_KX1);
    }
    if (AN_X4_LD_BASE_ABIL5r_BASE_5P0G_ENf_GET(AN_X4_LD_BASE_ABIL5r_reg)) {
        an_ability_st->an_base_speed |= (1<<TEFMOD_CL73_5GBASE_KR1);
    }

    /****** Getting local device fec basepage abilities 0xC1Cb ******/ 
    PHYMOD_IF_ERR_RETURN(READ_AN_X4_LD_FEC_BASEPAGE_ABILr(pc, &AN_X4_LD_FEC_BASEPAGE_ABILr_reg));
    if (AN_X4_LD_FEC_BASEPAGE_ABILr_BASE_R_FEC_REQ_ENf_GET(AN_X4_LD_FEC_BASEPAGE_ABILr_reg)) {
        an_fec |= 0x1;
    }
    if (AN_X4_LD_FEC_BASEPAGE_ABILr_RS_FEC_REQ_ENf_GET(AN_X4_LD_FEC_BASEPAGE_ABILr_reg)) {
        an_fec |= 0x2;
    }
    if ((an_ability_st->an_fec == TEFMOD_FEC_SUPRTD_NOT_REQSTD) ||
          (an_ability_st->an_fec == TEFMOD_FEC_NOT_SUPRTD)) {
        if (an_fec) {
            an_ability_st->an_fec = 0;
            if (an_fec & 0x1) {
                an_ability_st->an_fec |= TEFMOD_FEC_CL74_SUPRTD_REQSTD;
            }
            if (an_fec & 0x2) {
                an_ability_st->an_fec |= TEFMOD_FEC_CL91_SUPRTD_REQSTD;
            }
        }
    } else {
        if (an_fec & 0x1) {
            an_ability_st->an_fec |= TEFMOD_FEC_CL74_SUPRTD_REQSTD;
        }
        if (an_fec & 0x2) {
            an_ability_st->an_fec |= TEFMOD_FEC_CL91_SUPRTD_REQSTD;
        }
    }

    return PHYMOD_E_NONE;
} 


/**
@brief   Controls port RX squelch
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   enable is the control to  RX  squelch
@returns The value PHYMOD_E_NONE upon successful completion.
@details Controls port RX squelch
*/
int tefmod_gen3_rx_squelch_set(PHYMOD_ST *pc, int enable)
{
    SIGDET_CTL1r_t sigdet_ctl;

    SIGDET_CTL1r_CLR(sigdet_ctl);
    READ_SIGDET_CTL1r(pc, &sigdet_ctl);

    if(enable){
        SIGDET_CTL1r_SIGNAL_DETECT_FRCf_SET(sigdet_ctl, 1);
        SIGDET_CTL1r_SIGNAL_DETECT_FRC_VALf_SET(sigdet_ctl, 0);
    } else {
        SIGDET_CTL1r_SIGNAL_DETECT_FRCf_SET(sigdet_ctl, 0);
        SIGDET_CTL1r_SIGNAL_DETECT_FRC_VALf_SET(sigdet_ctl, 0);
    }
    PHYMOD_IF_ERR_RETURN(MODIFY_SIGDET_CTL1r(pc, sigdet_ctl));

    return PHYMOD_E_NONE;
}

/**
@brief   Controls port TX squelch
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   enable is the control to  TX  squelch
@returns The value PHYMOD_E_NONE upon successful completion.
@details Controls port TX squelch
*/
int tefmod_gen3_tx_squelch_set(PHYMOD_ST *pc, int enable)
{
    TXFIR_MISC_CTL0r_t  TXFIR_MISC_CTL0r_reg;
    phymod_access_t pa_copy;
    int i, start_lane = 0, num_lane = 0;

    PHYMOD_MEMCPY(&pa_copy, pc, sizeof(pa_copy));
    PHYMOD_IF_ERR_RETURN
        (phymod_util_lane_config_get(pc, &start_lane, &num_lane));

    TXFIR_MISC_CTL0r_CLR(TXFIR_MISC_CTL0r_reg);
    pa_copy.lane_mask = 0x1 << start_lane;
    
    PHYMOD_IF_ERR_RETURN(READ_TXFIR_MISC_CTL0r(&pa_copy, &TXFIR_MISC_CTL0r_reg));

    TXFIR_MISC_CTL0r_SDK_TX_DISABLEf_SET(TXFIR_MISC_CTL0r_reg, enable);

    for (i = 0; i < num_lane; i++) {
        if (!PHYMOD_LANEPBMP_MEMBER(pc->lane_mask, start_lane + i)) {
            continue;
        }
        pa_copy.lane_mask = 0x1 << (i + start_lane);
        PHYMOD_IF_ERR_RETURN(MODIFY_TXFIR_MISC_CTL0r(&pa_copy, TXFIR_MISC_CTL0r_reg));
    }

    return PHYMOD_E_NONE;
}


/**
@brief   Controls port TX/RX squelch
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   enable is the control to  TX/RX  squelch. Enable=1 means enable the port,no squelch
@returns The value PHYMOD_E_NONE upon successful completion.
@details Controls port TX/RX squelch
*/
int tefmod_gen3_port_enable_set(PHYMOD_ST *pc, int enable)
{
    if (enable)  {
        tefmod_gen3_rx_squelch_set(pc, 0);
        tefmod_gen3_tx_squelch_set(pc, 0);
    } else {
        tefmod_gen3_rx_squelch_set(pc, 1);
        tefmod_gen3_tx_squelch_set(pc, 1);
    }

    return PHYMOD_E_NONE;
}

/**
@brief   Get port TX squelch control settings
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   val Receiver for status of TX squelch 
@returns The value PHYMOD_E_NONE upon successful completion.
@details Get port TX squelch control settings
*/
int tefmod_gen3_tx_squelch_get(PHYMOD_ST *pc, int *val)
{
    TXFIR_MISC_CTL0r_t TXFIR_MISC_CTL0r_reg;

    TXFIR_MISC_CTL0r_CLR(TXFIR_MISC_CTL0r_reg);
    PHYMOD_IF_ERR_RETURN(READ_TXFIR_MISC_CTL0r(pc, &TXFIR_MISC_CTL0r_reg));
    *val = TXFIR_MISC_CTL0r_SDK_TX_DISABLEf_GET(TXFIR_MISC_CTL0r_reg);

    return PHYMOD_E_NONE;
}

/**
@brief   Gets port RX squelch settings
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   val Receiver for status of  RX  squelch
@returns The value PHYMOD_E_NONE upon successful completion.
@details Gets port RX squelch settings
*/
int tefmod_gen3_rx_squelch_get(PHYMOD_ST *pc, int *val)
{
    SIGDET_CTL1r_t sigdet_ctl;

    SIGDET_CTL1r_CLR(sigdet_ctl);
    PHYMOD_IF_ERR_RETURN(READ_SIGDET_CTL1r(pc, &sigdet_ctl));
    *val = SIGDET_CTL1r_SIGNAL_DETECT_FRCf_GET(sigdet_ctl);

    return PHYMOD_E_NONE;
}

/**
@brief   Get port TX/RX squelch Settings
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   tx_enable - get the TX  squelch settings
@param   rx_enable - get the RX  squelch settings
@returns The value PHYMOD_E_NONE upon successful completion.
@details Controls port TX/RX squelch
*/
int tefmod_gen3_port_enable_get(PHYMOD_ST *pc, int *tx_enable, int *rx_enable)
{

    tefmod_gen3_rx_squelch_get(pc, rx_enable);
    tefmod_gen3_tx_squelch_get(pc, tx_enable);

    return PHYMOD_E_NONE;
}

/**
@brief   To get remote autoneg advertisement registers.
@param   pc handle to current TSCF context (#PHYMOD_ST)
@param   an_ability_st receives autoneg info. #tefmod_an_adv_ability_t)
@returns PHYMOD_E_NONE if no errors. PHYMOD_E_FAIL else.
@details Upper layer software calls this function to get autoneg
         info. This function is not currently implemented.
*/
int tefmod_gen3_autoneg_remote_ability_get(PHYMOD_ST* pc, tefmod_gen3_an_adv_ability_t *an_ability_st)
{ 
    uint32_t data;
    uint32_t user_code;
    AN_X4_LP_BASE1r_t     AN_X4_LP_BASE1r_reg;
    AN_X4_LP_BASE3r_t     AN_X4_LP_BASE3r_reg;
    AN_X4_LP_OUI_UP3r_t   AN_X4_LP_OUI_UP3r_reg;
    AN_X4_LP_OUI_UP4r_t   AN_X4_LP_OUI_UP4r_reg;
    AN_X4_LD_BAM_ABILr_t  AN_X4_LD_BAM_ABILr_reg; 

    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);

    PHYMOD_IF_ERR_RETURN(READ_AN_X4_LP_BASE1r(pc, &AN_X4_LP_BASE1r_reg));
    PHYMOD_IF_ERR_RETURN(READ_AN_X4_LP_BASE3r(pc, &AN_X4_LP_BASE3r_reg));
    PHYMOD_IF_ERR_RETURN(READ_AN_X4_LP_OUI_UP3r(pc, &AN_X4_LP_OUI_UP3r_reg));
    PHYMOD_IF_ERR_RETURN(READ_AN_X4_LP_OUI_UP4r(pc, &AN_X4_LP_OUI_UP4r_reg));
    PHYMOD_IF_ERR_RETURN(READ_AN_X4_LD_BAM_ABILr(pc, &AN_X4_LD_BAM_ABILr_reg));

    
    /*an_ability_st->cl73_adv.an_bam_speed = 0xdead;*/
    an_ability_st->an_bam_speed = 0x0;

    user_code = AN_X4_LD_BAM_ABILr_CL73_BAM_CODEf_GET(AN_X4_LD_BAM_ABILr_reg) << 11 ;
    data = AN_X4_LP_OUI_UP4r_reg.v[0]; 

    if(data&(1<<1)) {
        an_ability_st->an_bam_speed |=(1<<0) ;
    } else {
        user_code |= data ;
        if(user_code == 0xabe20)
            an_ability_st->an_bam_speed  |=(1<<0) ;
    }
    if(data&(1<<0)) an_ability_st->an_bam_speed |=(1<<1) ;

    an_ability_st->an_pause = (AN_X4_LP_BASE1r_reg.v[0] >> 10 ) & 0x3;
    an_ability_st->an_fec = (AN_X4_LP_BASE3r_reg.v[0] >> 14) & 0x3;

    return PHYMOD_E_NONE;
}

/**
@brief   Sets the field of the Soft Table
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   st_entry_no: The ST to write to(0..3)
@param   st_control_field,: The ST field to write to
@param   st_field_value: The ST value to write to the field
returns  The value PHYMOD_E_NONE upon successful completion.
@details Sets the field of the Soft Table
*/
#if 0
int tefmod_gen3_st_control_field_set (PHYMOD_ST* pc,uint16_t st_entry_no, override_type_t  st_control_field, uint16_t st_field_value)
{
    SC_X1_SPD_OVRR0_SPDr_t SC_X1_SPD_OVRR0_SPDr_reg;
    SC_X1_SPD_OVRR1_SPDr_t SC_X1_SPD_OVRR1_SPDr_reg;
    SC_X1_SPD_OVRR2_SPDr_t SC_X1_SPD_OVRR2_SPDr_reg;
    SC_X1_SPD_OVRR3_SPDr_t SC_X1_SPD_OVRR3_SPDr_reg;

    SC_X1_SPD_OVRR0_0r_t   SC_X1_SPD_OVRR0_0r_reg;
    SC_X1_SPD_OVRR1_0r_t   SC_X1_SPD_OVRR1_0r_reg;
    SC_X1_SPD_OVRR2_0r_t   SC_X1_SPD_OVRR2_0r_reg;
    SC_X1_SPD_OVRR3_0r_t   SC_X1_SPD_OVRR3_0r_reg;

    SC_X1_SPD_OVRR0_1r_t   SC_X1_SPD_OVRR0_1r_reg;
    SC_X1_SPD_OVRR1_1r_t   SC_X1_SPD_OVRR1_1r_reg;
    SC_X1_SPD_OVRR2_1r_t   SC_X1_SPD_OVRR2_1r_reg;
    SC_X1_SPD_OVRR3_1r_t   SC_X1_SPD_OVRR3_1r_reg;

    SC_X1_SPD_OVRR0_2r_t   SC_X1_SPD_OVRR0_2r_reg;
    SC_X1_SPD_OVRR1_2r_t   SC_X1_SPD_OVRR1_2r_reg;
    SC_X1_SPD_OVRR2_2r_t   SC_X1_SPD_OVRR2_2r_reg;
    SC_X1_SPD_OVRR3_2r_t   SC_X1_SPD_OVRR3_2r_reg;

    SC_X1_SPD_OVRR0_3r_t   SC_X1_SPD_OVRR0_3r_reg;
    SC_X1_SPD_OVRR1_3r_t   SC_X1_SPD_OVRR1_3r_reg;
    SC_X1_SPD_OVRR2_3r_t   SC_X1_SPD_OVRR2_3r_reg;
    SC_X1_SPD_OVRR3_3r_t   SC_X1_SPD_OVRR3_3r_reg;

    SC_X1_SPD_OVRR0_4r_t   SC_X1_SPD_OVRR0_4r_reg;
    SC_X1_SPD_OVRR1_4r_t   SC_X1_SPD_OVRR1_4r_reg;
    SC_X1_SPD_OVRR2_4r_t   SC_X1_SPD_OVRR2_4r_reg;
    SC_X1_SPD_OVRR3_4r_t   SC_X1_SPD_OVRR3_4r_reg;

    SC_X1_SPD_OVRR0_5r_t   SC_X1_SPD_OVRR0_5r_reg;
    SC_X1_SPD_OVRR1_5r_t   SC_X1_SPD_OVRR1_5r_reg;
    SC_X1_SPD_OVRR2_5r_t   SC_X1_SPD_OVRR2_5r_reg;
    SC_X1_SPD_OVRR3_5r_t   SC_X1_SPD_OVRR3_5r_reg;

    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);

    switch (st_entry_no){
    case 0:
        SC_X1_SPD_OVRR0_SPDr_CLR(SC_X1_SPD_OVRR0_SPDr_reg);
        SC_X1_SPD_OVRR0_0r_CLR(SC_X1_SPD_OVRR0_0r_reg);
        SC_X1_SPD_OVRR0_1r_CLR(SC_X1_SPD_OVRR0_1r_reg);
        SC_X1_SPD_OVRR0_2r_CLR(SC_X1_SPD_OVRR0_2r_reg);
        SC_X1_SPD_OVRR0_3r_CLR(SC_X1_SPD_OVRR0_3r_reg);
        SC_X1_SPD_OVRR0_4r_CLR(SC_X1_SPD_OVRR0_4r_reg);
        SC_X1_SPD_OVRR0_5r_CLR(SC_X1_SPD_OVRR0_5r_reg);
        break;
    case 1:
        SC_X1_SPD_OVRR1_SPDr_CLR(SC_X1_SPD_OVRR1_SPDr_reg);
        SC_X1_SPD_OVRR1_0r_CLR(SC_X1_SPD_OVRR1_0r_reg);
        SC_X1_SPD_OVRR1_1r_CLR(SC_X1_SPD_OVRR1_1r_reg);
        SC_X1_SPD_OVRR1_2r_CLR(SC_X1_SPD_OVRR1_2r_reg);
        SC_X1_SPD_OVRR1_3r_CLR(SC_X1_SPD_OVRR1_3r_reg);
        SC_X1_SPD_OVRR1_4r_CLR(SC_X1_SPD_OVRR1_4r_reg);
        SC_X1_SPD_OVRR1_5r_CLR(SC_X1_SPD_OVRR1_5r_reg);
        break;
    case 2:
        SC_X1_SPD_OVRR2_SPDr_CLR(SC_X1_SPD_OVRR2_SPDr_reg);
        SC_X1_SPD_OVRR2_0r_CLR(SC_X1_SPD_OVRR2_0r_reg);
        SC_X1_SPD_OVRR2_1r_CLR(SC_X1_SPD_OVRR2_1r_reg);
        SC_X1_SPD_OVRR2_2r_CLR(SC_X1_SPD_OVRR2_2r_reg);
        SC_X1_SPD_OVRR2_3r_CLR(SC_X1_SPD_OVRR2_3r_reg);
        SC_X1_SPD_OVRR2_4r_CLR(SC_X1_SPD_OVRR2_4r_reg);
        SC_X1_SPD_OVRR2_5r_CLR(SC_X1_SPD_OVRR2_5r_reg);
        break;
    case 3:
        SC_X1_SPD_OVRR3_SPDr_CLR(SC_X1_SPD_OVRR3_SPDr_reg);
        SC_X1_SPD_OVRR3_0r_CLR(SC_X1_SPD_OVRR3_0r_reg);
        SC_X1_SPD_OVRR3_1r_CLR(SC_X1_SPD_OVRR3_1r_reg);
        SC_X1_SPD_OVRR3_2r_CLR(SC_X1_SPD_OVRR3_2r_reg);
        SC_X1_SPD_OVRR3_3r_CLR(SC_X1_SPD_OVRR3_3r_reg);
        SC_X1_SPD_OVRR3_4r_CLR(SC_X1_SPD_OVRR3_4r_reg);
        SC_X1_SPD_OVRR3_5r_CLR(SC_X1_SPD_OVRR3_5r_reg);
        break;
    }
    switch (st_entry_no){
        case 0:
            switch (st_control_field){
                case OVERRIDE_NUM_LANES:
                    SC_X1_SPD_OVRR0_SPDr_NUM_LANESf_SET(SC_X1_SPD_OVRR0_SPDr_reg, st_field_value);
                    PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR0_SPDr(pc, SC_X1_SPD_OVRR0_SPDr_reg));
                    break;
                case OVERRIDE_OS_MODE:
                    SC_X1_SPD_OVRR0_0r_OS_MODEf_SET(SC_X1_SPD_OVRR0_0r_reg, st_field_value);
                    PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR0_0r(pc, SC_X1_SPD_OVRR0_0r_reg));
                    break;
    		case OVERRIDE_T_FIFO_MODE:
      		    SC_X1_SPD_OVRR0_0r_T_FIFO_MODEf_SET(SC_X1_SPD_OVRR0_0r_reg, st_field_value);
      		    PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR0_0r(pc, SC_X1_SPD_OVRR0_0r_reg));
      		    break;
                case OVERRIDE_T_ENC_MODE:
                    SC_X1_SPD_OVRR0_0r_T_ENC_MODEf_SET(SC_X1_SPD_OVRR0_0r_reg, st_field_value);
      		    PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR0_0r(pc, SC_X1_SPD_OVRR0_0r_reg));
      		    break;
                case OVERRIDE_T_HG2_ENABLE:
      		    SC_X1_SPD_OVRR0_0r_T_HG2_ENABLEf_SET(SC_X1_SPD_OVRR0_0r_reg, st_field_value);
                    PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR0_0r(pc, SC_X1_SPD_OVRR0_0r_reg));
      		    break;
    	        case OVERRIDE_SCR_MODE:
      	            SC_X1_SPD_OVRR0_0r_SCR_MODEf_SET(SC_X1_SPD_OVRR0_0r_reg, st_field_value);
      	            PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR0_0r(pc, SC_X1_SPD_OVRR0_0r_reg));
                    break;
                case OVERRIDE_DESCR_MODE_OEN:
                    SC_X1_SPD_OVRR0_1r_DESCR_MODEf_SET(SC_X1_SPD_OVRR0_1r_reg, st_field_value);
                    PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR0_1r(pc, SC_X1_SPD_OVRR0_1r_reg));
                    break;
                case OVERRIDE_DEC_TL_MODE:
                    SC_X1_SPD_OVRR0_1r_DEC_TL_MODEf_SET(SC_X1_SPD_OVRR0_1r_reg, st_field_value);
                    PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR0_1r(pc, SC_X1_SPD_OVRR0_1r_reg));
                    break;
                case OVERRIDE_DESKEW_MODE:
                    SC_X1_SPD_OVRR0_1r_DESKEW_MODEf_SET(SC_X1_SPD_OVRR0_1r_reg, st_field_value);
                    PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR0_1r(pc, SC_X1_SPD_OVRR0_1r_reg));
                    break;
                case OVERRIDE_DEC_FSM_MODE:
                    SC_X1_SPD_OVRR0_1r_DEC_FSM_MODEf_SET(SC_X1_SPD_OVRR0_1r_reg, st_field_value);
                    PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR0_1r(pc, SC_X1_SPD_OVRR0_1r_reg));
                    break;
                case OVERRIDE_R_HG2_ENABLE:
                    SC_X1_SPD_OVRR0_1r_R_HG2_ENABLEf_SET(SC_X1_SPD_OVRR0_1r_reg, st_field_value);
                    PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR0_1r(pc, SC_X1_SPD_OVRR0_1r_reg));
                    break;
                case OVERRIDE_CL72_EN:
                    SC_X1_SPD_OVRR0_0r_CL72_ENf_SET(SC_X1_SPD_OVRR0_0r_reg, st_field_value);
                    PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR0_0r(pc, SC_X1_SPD_OVRR0_0r_reg));
                    break;
                case OVERRIDE_CLOCKCNT0:
                    SC_X1_SPD_OVRR0_2r_CLOCKCNT0f_SET(SC_X1_SPD_OVRR0_2r_reg, st_field_value);
                    PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR0_2r(pc, SC_X1_SPD_OVRR0_2r_reg));
                    break;
                case OVERRIDE_CLOCKCNT1:
                    SC_X1_SPD_OVRR0_3r_CLOCKCNT1f_SET(SC_X1_SPD_OVRR0_3r_reg, st_field_value);
                    PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR0_3r(pc, SC_X1_SPD_OVRR0_3r_reg));
                    break;
                case OVERRIDE_LOOPCNT0:
                    SC_X1_SPD_OVRR0_4r_LOOPCNT0f_SET(SC_X1_SPD_OVRR0_4r_reg, st_field_value);
                    PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR0_4r(pc, SC_X1_SPD_OVRR0_4r_reg));
                    break;
                case OVERRIDE_LOOPCNT1:
                    SC_X1_SPD_OVRR0_4r_LOOPCNT1f_SET(SC_X1_SPD_OVRR0_4r_reg, st_field_value);
                    PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR0_4r(pc, SC_X1_SPD_OVRR0_4r_reg));
                    break;
                case OVERRIDE_MAC_CREDITGENCNT:
                    SC_X1_SPD_OVRR0_5r_MAC_CREDITGENCNTf_SET(SC_X1_SPD_OVRR0_5r_reg, st_field_value);
                    PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR0_5r(pc, SC_X1_SPD_OVRR0_5r_reg));
                    break;
                default:
                    return PHYMOD_E_FAIL;
                    break;
            }
            break;
        case 1:
            switch (st_control_field){
                case OVERRIDE_NUM_LANES:
                    SC_X1_SPD_OVRR1_SPDr_NUM_LANESf_SET(SC_X1_SPD_OVRR1_SPDr_reg, st_field_value);
      PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR1_SPDr(pc, SC_X1_SPD_OVRR1_SPDr_reg));
      break;
    case OVERRIDE_OS_MODE:
      SC_X1_SPD_OVRR1_0r_OS_MODEf_SET(SC_X1_SPD_OVRR1_0r_reg, st_field_value);
      PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR1_0r(pc, SC_X1_SPD_OVRR1_0r_reg));
      break;
    case OVERRIDE_T_FIFO_MODE:
      SC_X1_SPD_OVRR1_0r_T_FIFO_MODEf_SET(SC_X1_SPD_OVRR1_0r_reg, st_field_value);
      PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR1_0r(pc, SC_X1_SPD_OVRR1_0r_reg));
      break;
    case OVERRIDE_T_ENC_MODE:
      SC_X1_SPD_OVRR1_0r_T_ENC_MODEf_SET(SC_X1_SPD_OVRR1_0r_reg, st_field_value);
      PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR1_0r(pc, SC_X1_SPD_OVRR1_0r_reg));
      break;
    case OVERRIDE_T_HG2_ENABLE:
      SC_X1_SPD_OVRR1_0r_T_HG2_ENABLEf_SET(SC_X1_SPD_OVRR1_0r_reg, st_field_value);
      PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR1_0r(pc, SC_X1_SPD_OVRR1_0r_reg));
      break;
    case OVERRIDE_SCR_MODE:
      SC_X1_SPD_OVRR1_0r_SCR_MODEf_SET(SC_X1_SPD_OVRR1_0r_reg, st_field_value);
      PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR1_0r(pc, SC_X1_SPD_OVRR1_0r_reg));
      break;
    case OVERRIDE_DESCR_MODE_OEN:
      SC_X1_SPD_OVRR1_1r_DESCR_MODEf_SET(SC_X1_SPD_OVRR1_1r_reg, st_field_value);
      PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR1_1r(pc, SC_X1_SPD_OVRR1_1r_reg));
      break;
    case OVERRIDE_DEC_TL_MODE:
      SC_X1_SPD_OVRR1_1r_DEC_TL_MODEf_SET(SC_X1_SPD_OVRR1_1r_reg, st_field_value);
      PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR1_1r(pc, SC_X1_SPD_OVRR1_1r_reg));
      break;
    case OVERRIDE_DESKEW_MODE:
      SC_X1_SPD_OVRR1_1r_DESKEW_MODEf_SET(SC_X1_SPD_OVRR1_1r_reg, st_field_value);
      PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR1_1r(pc, SC_X1_SPD_OVRR1_1r_reg));
      break;
    case OVERRIDE_DEC_FSM_MODE:
      SC_X1_SPD_OVRR1_1r_DEC_FSM_MODEf_SET(SC_X1_SPD_OVRR1_1r_reg, st_field_value);
      PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR1_1r(pc, SC_X1_SPD_OVRR1_1r_reg));
      break;
    case OVERRIDE_R_HG2_ENABLE:
      SC_X1_SPD_OVRR1_1r_R_HG2_ENABLEf_SET(SC_X1_SPD_OVRR1_1r_reg, st_field_value);
      PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR1_1r(pc, SC_X1_SPD_OVRR1_1r_reg));
      break;
    case OVERRIDE_CL72_EN:
      SC_X1_SPD_OVRR1_0r_CL72_ENf_SET(SC_X1_SPD_OVRR1_0r_reg, st_field_value);
      PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR1_0r(pc, SC_X1_SPD_OVRR1_0r_reg));
      break;
    case OVERRIDE_CLOCKCNT0:
      SC_X1_SPD_OVRR1_2r_CLOCKCNT0f_SET(SC_X1_SPD_OVRR1_2r_reg, st_field_value);
      PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR1_2r(pc, SC_X1_SPD_OVRR1_2r_reg));
      break;
    case OVERRIDE_CLOCKCNT1:
      SC_X1_SPD_OVRR1_3r_CLOCKCNT1f_SET(SC_X1_SPD_OVRR1_3r_reg, st_field_value);
      PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR1_3r(pc, SC_X1_SPD_OVRR1_3r_reg));
      break;
    case OVERRIDE_LOOPCNT0:
      SC_X1_SPD_OVRR1_4r_LOOPCNT0f_SET(SC_X1_SPD_OVRR1_4r_reg, st_field_value);
      PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR1_4r(pc, SC_X1_SPD_OVRR1_4r_reg));
      break;
    case OVERRIDE_LOOPCNT1:
      SC_X1_SPD_OVRR1_4r_LOOPCNT1f_SET(SC_X1_SPD_OVRR1_4r_reg, st_field_value);
      PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR1_4r(pc, SC_X1_SPD_OVRR1_4r_reg));
      break;
    case OVERRIDE_MAC_CREDITGENCNT:
      SC_X1_SPD_OVRR1_5r_MAC_CREDITGENCNTf_SET(SC_X1_SPD_OVRR1_5r_reg, st_field_value);
      PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR1_5r(pc, SC_X1_SPD_OVRR1_5r_reg));
      break;
    default:
      return PHYMOD_E_FAIL;
      break;
    }
  break;
  case 2:
    switch (st_control_field){
    case OVERRIDE_NUM_LANES:
      SC_X1_SPD_OVRR2_SPDr_NUM_LANESf_SET(SC_X1_SPD_OVRR2_SPDr_reg, st_field_value);
      PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR2_SPDr(pc, SC_X1_SPD_OVRR2_SPDr_reg));
      break;
    case OVERRIDE_OS_MODE:
      SC_X1_SPD_OVRR2_0r_OS_MODEf_SET(SC_X1_SPD_OVRR2_0r_reg, st_field_value);
      PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR2_0r(pc, SC_X1_SPD_OVRR2_0r_reg));
      break;
    case OVERRIDE_T_FIFO_MODE:
      SC_X1_SPD_OVRR2_0r_T_FIFO_MODEf_SET(SC_X1_SPD_OVRR2_0r_reg, st_field_value);
      PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR2_0r(pc, SC_X1_SPD_OVRR2_0r_reg));
      break;
    case OVERRIDE_T_ENC_MODE:
      SC_X1_SPD_OVRR2_0r_T_ENC_MODEf_SET(SC_X1_SPD_OVRR2_0r_reg, st_field_value);
      PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR2_0r(pc, SC_X1_SPD_OVRR2_0r_reg));
      break;
    case OVERRIDE_T_HG2_ENABLE:
      SC_X1_SPD_OVRR2_0r_T_HG2_ENABLEf_SET(SC_X1_SPD_OVRR2_0r_reg, st_field_value);
      PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR2_0r(pc, SC_X1_SPD_OVRR2_0r_reg));
      break;
    case OVERRIDE_SCR_MODE:
      SC_X1_SPD_OVRR2_0r_SCR_MODEf_SET(SC_X1_SPD_OVRR2_0r_reg, st_field_value);
      PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR2_0r(pc, SC_X1_SPD_OVRR2_0r_reg));
      break;
    case OVERRIDE_DESCR_MODE_OEN:
      SC_X1_SPD_OVRR2_1r_DESCR_MODEf_SET(SC_X1_SPD_OVRR2_1r_reg, st_field_value);
      PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR2_1r(pc, SC_X1_SPD_OVRR2_1r_reg));
      break;
    case OVERRIDE_DEC_TL_MODE:
      SC_X1_SPD_OVRR2_1r_DEC_TL_MODEf_SET(SC_X1_SPD_OVRR2_1r_reg, st_field_value);
      PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR2_1r(pc, SC_X1_SPD_OVRR2_1r_reg));
      break;
    case OVERRIDE_DESKEW_MODE:
      SC_X1_SPD_OVRR2_1r_DESKEW_MODEf_SET(SC_X1_SPD_OVRR2_1r_reg, st_field_value);
      PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR2_1r(pc, SC_X1_SPD_OVRR2_1r_reg));
      break;
    case OVERRIDE_DEC_FSM_MODE:
      SC_X1_SPD_OVRR2_1r_DEC_FSM_MODEf_SET(SC_X1_SPD_OVRR2_1r_reg, st_field_value);
      PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR2_1r(pc, SC_X1_SPD_OVRR2_1r_reg));
      break;
    case OVERRIDE_R_HG2_ENABLE:
      SC_X1_SPD_OVRR2_1r_R_HG2_ENABLEf_SET(SC_X1_SPD_OVRR2_1r_reg, st_field_value);
      PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR2_1r(pc, SC_X1_SPD_OVRR2_1r_reg));
      break;
    case OVERRIDE_CL72_EN:
      SC_X1_SPD_OVRR2_0r_CL72_ENf_SET(SC_X1_SPD_OVRR2_0r_reg, st_field_value);
      PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR2_0r(pc, SC_X1_SPD_OVRR2_0r_reg));
      break;
    case OVERRIDE_CLOCKCNT0:
      SC_X1_SPD_OVRR2_2r_CLOCKCNT0f_SET(SC_X1_SPD_OVRR2_2r_reg, st_field_value);
      PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR2_2r(pc, SC_X1_SPD_OVRR2_2r_reg));
      break;
    case OVERRIDE_CLOCKCNT1:
      SC_X1_SPD_OVRR2_3r_CLOCKCNT1f_SET(SC_X1_SPD_OVRR2_3r_reg, st_field_value);
      PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR2_3r(pc, SC_X1_SPD_OVRR2_3r_reg));
      break;
    case OVERRIDE_LOOPCNT0:
      SC_X1_SPD_OVRR2_4r_LOOPCNT0f_SET(SC_X1_SPD_OVRR2_4r_reg, st_field_value);
      PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR2_4r(pc, SC_X1_SPD_OVRR2_4r_reg));
      break;
    case OVERRIDE_LOOPCNT1:
      SC_X1_SPD_OVRR2_4r_LOOPCNT1f_SET(SC_X1_SPD_OVRR2_4r_reg, st_field_value);
      PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR2_4r(pc, SC_X1_SPD_OVRR2_4r_reg));
      break;
    case OVERRIDE_MAC_CREDITGENCNT:
      SC_X1_SPD_OVRR2_5r_MAC_CREDITGENCNTf_SET(SC_X1_SPD_OVRR2_5r_reg, st_field_value);
      PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR2_5r(pc, SC_X1_SPD_OVRR2_5r_reg));
      break;
    default:
      return PHYMOD_E_FAIL;
      break;
    }
  break;
  case 3:
    switch (st_control_field){
    case OVERRIDE_NUM_LANES:
      SC_X1_SPD_OVRR3_SPDr_NUM_LANESf_SET(SC_X1_SPD_OVRR3_SPDr_reg, st_field_value);
      PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR3_SPDr(pc, SC_X1_SPD_OVRR3_SPDr_reg));
      break;
    case OVERRIDE_OS_MODE:
      SC_X1_SPD_OVRR3_0r_OS_MODEf_SET(SC_X1_SPD_OVRR3_0r_reg, st_field_value);
      PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR3_0r(pc, SC_X1_SPD_OVRR3_0r_reg));
      break;
    case OVERRIDE_T_FIFO_MODE:
      SC_X1_SPD_OVRR3_0r_T_FIFO_MODEf_SET(SC_X1_SPD_OVRR3_0r_reg, st_field_value);
      PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR3_0r(pc, SC_X1_SPD_OVRR3_0r_reg));
      break;
    case OVERRIDE_T_ENC_MODE:
      SC_X1_SPD_OVRR3_0r_T_ENC_MODEf_SET(SC_X1_SPD_OVRR3_0r_reg, st_field_value);
      PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR3_0r(pc, SC_X1_SPD_OVRR3_0r_reg));
      break;
    case OVERRIDE_T_HG2_ENABLE:
      SC_X1_SPD_OVRR3_0r_T_HG2_ENABLEf_SET(SC_X1_SPD_OVRR3_0r_reg, st_field_value);
      PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR3_0r(pc, SC_X1_SPD_OVRR3_0r_reg));
      break;
    case OVERRIDE_SCR_MODE:
      SC_X1_SPD_OVRR3_0r_SCR_MODEf_SET(SC_X1_SPD_OVRR3_0r_reg, st_field_value);
      PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR3_0r(pc, SC_X1_SPD_OVRR3_0r_reg));
      break;
    case OVERRIDE_DESCR_MODE_OEN:
      SC_X1_SPD_OVRR3_1r_DESCR_MODEf_SET(SC_X1_SPD_OVRR3_1r_reg, st_field_value);
      PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR3_1r(pc, SC_X1_SPD_OVRR3_1r_reg));
      break;
    case OVERRIDE_DEC_TL_MODE:
      SC_X1_SPD_OVRR3_1r_DEC_TL_MODEf_SET(SC_X1_SPD_OVRR3_1r_reg, st_field_value);
      PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR3_1r(pc, SC_X1_SPD_OVRR3_1r_reg));
      break;
    case OVERRIDE_DESKEW_MODE:
      SC_X1_SPD_OVRR3_1r_DESKEW_MODEf_SET(SC_X1_SPD_OVRR3_1r_reg, st_field_value);
      PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR3_1r(pc, SC_X1_SPD_OVRR3_1r_reg));
      break;
    case OVERRIDE_DEC_FSM_MODE:
      SC_X1_SPD_OVRR3_1r_DEC_FSM_MODEf_SET(SC_X1_SPD_OVRR3_1r_reg, st_field_value);
      PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR3_1r(pc, SC_X1_SPD_OVRR3_1r_reg));
      break;
    case OVERRIDE_R_HG2_ENABLE:
      SC_X1_SPD_OVRR3_1r_R_HG2_ENABLEf_SET(SC_X1_SPD_OVRR3_1r_reg, st_field_value);
      PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR3_1r(pc, SC_X1_SPD_OVRR3_1r_reg));
      break;
    case OVERRIDE_CL72_EN:
      SC_X1_SPD_OVRR3_0r_CL72_ENf_SET(SC_X1_SPD_OVRR3_0r_reg, st_field_value);
      PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR3_0r(pc, SC_X1_SPD_OVRR3_0r_reg));
      break;
    case OVERRIDE_CLOCKCNT0:
      SC_X1_SPD_OVRR3_2r_CLOCKCNT0f_SET(SC_X1_SPD_OVRR3_2r_reg, st_field_value);
      PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR3_2r(pc, SC_X1_SPD_OVRR3_2r_reg));
      break;
    case OVERRIDE_CLOCKCNT1:
      SC_X1_SPD_OVRR3_3r_CLOCKCNT1f_SET(SC_X1_SPD_OVRR3_3r_reg, st_field_value);
      PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR3_3r(pc, SC_X1_SPD_OVRR3_3r_reg));
      break;
    case OVERRIDE_LOOPCNT0:
      SC_X1_SPD_OVRR3_4r_LOOPCNT0f_SET(SC_X1_SPD_OVRR3_4r_reg, st_field_value);
      PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR3_4r(pc, SC_X1_SPD_OVRR3_4r_reg));
      break;
    case OVERRIDE_LOOPCNT1:
      SC_X1_SPD_OVRR3_4r_LOOPCNT1f_SET(SC_X1_SPD_OVRR3_4r_reg, st_field_value);
      PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR3_4r(pc, SC_X1_SPD_OVRR3_4r_reg));
      break;
    case OVERRIDE_MAC_CREDITGENCNT:
      SC_X1_SPD_OVRR3_5r_MAC_CREDITGENCNTf_SET(SC_X1_SPD_OVRR3_5r_reg, st_field_value);
      PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR3_5r(pc, SC_X1_SPD_OVRR3_5r_reg));
      break;
    default:
      return PHYMOD_E_FAIL;
      break;
    }
  break;
  }
  return PHYMOD_E_NONE;
}
#endif
/**
@brief   EEE Control Set
@param   pc handle to current TSCF context (#PHYMOD_ST)
@param   enable EEE control(enable/disable)
@returns The value PHYMOD_E_NONE upon successful completion
@details EEE Control
         enable =1 : Allow LPI pass through. i.e. dont convert LPI.
         enable =0 : Convert LPI to idle. So  MAC will not see it.

*/
int tefmod_gen3_eee_control_set(PHYMOD_ST* pc, uint32_t enable)
{
    RX_X4_PCS_CTL0r_t reg_pcs_ctrl0;

    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
    RX_X4_PCS_CTL0r_CLR(reg_pcs_ctrl0);
    READ_RX_X4_PCS_CTL0r(pc, &reg_pcs_ctrl0);

    RX_X4_PCS_CTL0r_LPI_ENABLEf_SET(reg_pcs_ctrl0, enable & 0x1);
    PHYMOD_IF_ERR_RETURN (MODIFY_RX_X4_PCS_CTL0r(pc, reg_pcs_ctrl0));

    return PHYMOD_E_NONE;
}

/**
@brief   EEE Control Get
@param   pc handle to current TSCF context (#PHYMOD_ST)
@param   enable handle to receive EEE control
@returns The value PHYMOD_E_NONE upon successful completion
@details EEE Control
         enable =1 : Allow LPI pass through. i.e. dont convert LPI.
         enable =0 : Convert LPI to idle. So  MAC will not see it.

*/
int tefmod_gen3_eee_control_get(PHYMOD_ST* pc, uint32_t *enable)
{
    RX_X4_PCS_CTL0r_t reg_pcs_ctrl0;

    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
    RX_X4_PCS_CTL0r_CLR(reg_pcs_ctrl0);

    PHYMOD_IF_ERR_RETURN (READ_RX_X4_PCS_CTL0r(pc, &reg_pcs_ctrl0));
    *enable = RX_X4_PCS_CTL0r_LPI_ENABLEf_GET(reg_pcs_ctrl0);

    return PHYMOD_E_NONE;
}


/**
@brief   Setup the default for cl74 to ieee compliance
@param   pc handle to current TSCE context (#PHYMOD_ST)
@returns The value PHYMOD_E_NONE upon successful completion
@details cl74 control set to ieee compliance
*/
int tefmod_gen3_cl74_chng_default (PHYMOD_ST* pc)
{
    RX_X4_FEC1r_t RX_X4_FEC1reg;
    phymod_access_t pa_copy;
    int i;

    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);

    PHYMOD_MEMCPY(&pa_copy, pc, sizeof(pa_copy));
    RX_X4_FEC1r_CLR(RX_X4_FEC1reg);
    for (i = 0; i <4; i++) {
         pa_copy.lane_mask = 0x1 << i;
        PHYMOD_IF_ERR_RETURN(WRITE_RX_X4_FEC1r(&pa_copy, RX_X4_FEC1reg));
    }
    return PHYMOD_E_NONE;
}

int tefmod_gen3_cl91_ecc_clear(PHYMOD_ST* pc)
{
    RX_X1_ECC_STS_FEC_MEM0r_t RX_X1_ECC_STS_FEC_MEM0r_reg;
    RX_X1_ECC_STS_FEC_MEM1r_t RX_X1_ECC_STS_FEC_MEM1r_reg;
    RX_X1_ECC_STS_FEC_MEM2r_t RX_X1_ECC_STS_FEC_MEM2r_reg;
    RX_X1_ECC_STS_FEC_MEM3r_t RX_X1_ECC_STS_FEC_MEM3r_reg;
    RX_X1_ECC_INTR_STS_1BITr_t RX_X1_ECC_INTR_STS_1BITr_reg;
    RX_X1_ECC_INTR_STS_2BITr_t RX_X1_ECC_INTR_STS_2BITr_reg;

    RX_X4_ECC_STS_CL91_BUFFER_BLK0r_t RX_X4_ECC_STS_CL91_BUFFER_BLK0r_reg;
    RX_X4_ECC_STS_CL91_BUFFER_BLK1r_t RX_X4_ECC_STS_CL91_BUFFER_BLK1r_reg;
    RX_X4_ECC_STS_CL91_BUFFER_BLK2r_t RX_X4_ECC_STS_CL91_BUFFER_BLK2r_reg;
    RX_X4_ECC_STS_DESKEW_MEM0r_t RX_X4_ECC_STS_DESKEW_MEM0r_reg;
    RX_X4_ECC_STS_DESKEW_MEM1r_t RX_X4_ECC_STS_DESKEW_MEM1r_reg;
    RX_X4_ECC_STS_DESKEW_MEM2r_t RX_X4_ECC_STS_DESKEW_MEM2r_reg;
    RX_X4_ECC_INTR_STS_1BITr_t RX_X4_ECC_INTR_STS_1BITr_reg;
    RX_X4_ECC_INTR_STS_2BITr_t RX_X4_ECC_INTR_STS_2BITr_reg;
    phymod_access_t pa_copy;
    int i;

    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);

    PHYMOD_MEMCPY(&pa_copy, pc, sizeof(pa_copy));
    RX_X1_ECC_STS_FEC_MEM0r_CLR(RX_X1_ECC_STS_FEC_MEM0r_reg);
    RX_X1_ECC_STS_FEC_MEM1r_CLR(RX_X1_ECC_STS_FEC_MEM1r_reg);
    RX_X1_ECC_STS_FEC_MEM2r_CLR(RX_X1_ECC_STS_FEC_MEM2r_reg);
    RX_X1_ECC_STS_FEC_MEM3r_CLR(RX_X1_ECC_STS_FEC_MEM3r_reg);
    RX_X1_ECC_INTR_STS_1BITr_CLR(RX_X1_ECC_INTR_STS_1BITr_reg);
    RX_X1_ECC_INTR_STS_2BITr_CLR(RX_X1_ECC_INTR_STS_2BITr_reg);

    RX_X4_ECC_STS_CL91_BUFFER_BLK0r_CLR(RX_X4_ECC_STS_CL91_BUFFER_BLK0r_reg);
    RX_X4_ECC_STS_CL91_BUFFER_BLK1r_CLR(RX_X4_ECC_STS_CL91_BUFFER_BLK1r_reg);
    RX_X4_ECC_STS_CL91_BUFFER_BLK2r_CLR(RX_X4_ECC_STS_CL91_BUFFER_BLK2r_reg);
    RX_X4_ECC_STS_DESKEW_MEM0r_CLR(RX_X4_ECC_STS_DESKEW_MEM0r_reg);
    RX_X4_ECC_STS_DESKEW_MEM1r_CLR(RX_X4_ECC_STS_DESKEW_MEM1r_reg);
    RX_X4_ECC_STS_DESKEW_MEM2r_CLR(RX_X4_ECC_STS_DESKEW_MEM2r_reg);
    RX_X4_ECC_INTR_STS_1BITr_CLR(RX_X4_ECC_INTR_STS_1BITr_reg);
    RX_X4_ECC_INTR_STS_2BITr_CLR(RX_X4_ECC_INTR_STS_2BITr_reg);

    /* Read Clear  */
    PHYMOD_IF_ERR_RETURN(READ_RX_X1_ECC_STS_FEC_MEM0r(&pa_copy, &RX_X1_ECC_STS_FEC_MEM0r_reg));
    PHYMOD_IF_ERR_RETURN(READ_RX_X1_ECC_STS_FEC_MEM1r(&pa_copy, &RX_X1_ECC_STS_FEC_MEM1r_reg));
    PHYMOD_IF_ERR_RETURN(READ_RX_X1_ECC_STS_FEC_MEM2r(&pa_copy, &RX_X1_ECC_STS_FEC_MEM2r_reg));
    PHYMOD_IF_ERR_RETURN(READ_RX_X1_ECC_STS_FEC_MEM3r(&pa_copy, &RX_X1_ECC_STS_FEC_MEM3r_reg));
    PHYMOD_IF_ERR_RETURN(READ_RX_X1_ECC_INTR_STS_1BITr(&pa_copy, &RX_X1_ECC_INTR_STS_1BITr_reg));
    PHYMOD_IF_ERR_RETURN(READ_RX_X1_ECC_INTR_STS_2BITr(&pa_copy, &RX_X1_ECC_INTR_STS_2BITr_reg));

    for (i = 0; i < 4; i++) {
        pa_copy.lane_mask = 0x1 << i;
        PHYMOD_IF_ERR_RETURN(READ_RX_X4_ECC_STS_CL91_BUFFER_BLK0r(&pa_copy, &RX_X4_ECC_STS_CL91_BUFFER_BLK0r_reg));
        PHYMOD_IF_ERR_RETURN(READ_RX_X4_ECC_STS_CL91_BUFFER_BLK1r(&pa_copy, &RX_X4_ECC_STS_CL91_BUFFER_BLK1r_reg));
        PHYMOD_IF_ERR_RETURN(READ_RX_X4_ECC_STS_CL91_BUFFER_BLK2r(&pa_copy, &RX_X4_ECC_STS_CL91_BUFFER_BLK2r_reg));
        PHYMOD_IF_ERR_RETURN(READ_RX_X4_ECC_STS_DESKEW_MEM0r(&pa_copy, &RX_X4_ECC_STS_DESKEW_MEM0r_reg));
        PHYMOD_IF_ERR_RETURN(READ_RX_X4_ECC_STS_DESKEW_MEM1r(&pa_copy, &RX_X4_ECC_STS_DESKEW_MEM1r_reg));
        PHYMOD_IF_ERR_RETURN(READ_RX_X4_ECC_STS_DESKEW_MEM2r(&pa_copy, &RX_X4_ECC_STS_DESKEW_MEM2r_reg));
        PHYMOD_IF_ERR_RETURN(READ_RX_X4_ECC_INTR_STS_1BITr(&pa_copy, &RX_X4_ECC_INTR_STS_1BITr_reg));
        PHYMOD_IF_ERR_RETURN(READ_RX_X4_ECC_INTR_STS_2BITr(&pa_copy, &RX_X4_ECC_INTR_STS_2BITr_reg));
   }

    return PHYMOD_E_NONE;
}

int tefmod_gen3_25g_rsfec_am_init(PHYMOD_ST *pc)
{
    SC_X4_IEEE_25G_CTLr_t               ieee_25G_am_reg;
    RX_X4_IEEE_25G_PARLLEL_DET_CTRr_t   ieee_25g_parallel_det_reg;
    SC_X4_MSA_25G_50G_CTLr_t       msa_25g_50g_am_reg;

    SC_X4_IEEE_25G_CTLr_CLR(ieee_25G_am_reg);
    RX_X4_IEEE_25G_PARLLEL_DET_CTRr_CLR(ieee_25g_parallel_det_reg);
    SC_X4_MSA_25G_50G_CTLr_CLR(msa_25g_50g_am_reg);

    /* Program AM0 value to use 100G AM  AM123 should use 40G AM
     * which is a default
     */
    PHYMOD_IF_ERR_RETURN(
        READ_SC_X4_IEEE_25G_CTLr(pc, &ieee_25G_am_reg));
    SC_X4_IEEE_25G_CTLr_IEEE_25G_AM0_FORMATf_SET(ieee_25G_am_reg, 1);
    PHYMOD_IF_ERR_RETURN(
        WRITE_SC_X4_IEEE_25G_CTLr(pc, ieee_25G_am_reg));

    /* MSA Mode Program 25G AM0 value to use 100G AM
     * The rest AM should use 40G AM, which is a default
     */
    PHYMOD_IF_ERR_RETURN(
        READ_SC_X4_MSA_25G_50G_CTLr(pc, &msa_25g_50g_am_reg));
    SC_X4_MSA_25G_50G_CTLr_MSA_25G_AM0_FORMATf_SET(msa_25g_50g_am_reg, 1);
    PHYMOD_IF_ERR_RETURN(
        WRITE_SC_X4_MSA_25G_50G_CTLr(pc, msa_25g_50g_am_reg));

    /* Enable AM parallel detect for in case if the legacy devices use 40G AM */
    PHYMOD_IF_ERR_RETURN(
        READ_RX_X4_IEEE_25G_PARLLEL_DET_CTRr(pc, &ieee_25g_parallel_det_reg));
    RX_X4_IEEE_25G_PARLLEL_DET_CTRr_SET(ieee_25g_parallel_det_reg, 1);
    PHYMOD_IF_ERR_RETURN(
        WRITE_RX_X4_IEEE_25G_PARLLEL_DET_CTRr(pc, ieee_25g_parallel_det_reg));

    return PHYMOD_E_NONE;

}

int tefmod_gen3_an_msa_mode_set(PHYMOD_ST* pc, uint32_t val) 
{
    AN_X4_LD_PAGE0r_t AN_X4_LD_PAGE0r_reg;
  
    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
    AN_X4_LD_PAGE0r_CLR(AN_X4_LD_PAGE0r_reg);
    AN_X4_LD_PAGE0r_SET(AN_X4_LD_PAGE0r_reg, val & 0x1);

    /********Setting AN_X4_LD_PAGE0r_t 0xC1E2*****/
    PHYMOD_IF_ERR_RETURN(WRITE_AN_X4_LD_PAGE0r(pc, AN_X4_LD_PAGE0r_reg));

    return PHYMOD_E_NONE;
}

int tefmod_gen3_an_msa_mode_get(PHYMOD_ST* pc, uint32_t* val) 
{
    AN_X4_LD_PAGE0r_t AN_X4_LD_PAGE0r_reg;

    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);

    /******Getting AN_X4_LD_PAGE0r_t 0xC1E2******/
    AN_X4_LD_PAGE0r_CLR(AN_X4_LD_PAGE0r_reg);
    PHYMOD_IF_ERR_RETURN(READ_AN_X4_LD_PAGE0r(pc, &AN_X4_LD_PAGE0r_reg));
    *val = AN_X4_LD_PAGE0r_LD_PAGE_0f_GET(AN_X4_LD_PAGE0r_reg);

    return PHYMOD_E_NONE;
}


int tefmod_gen3_an_oui_set(PHYMOD_ST* pc, tefmod_gen3_an_oui_t oui) 
{
    AN_X1_OUI_LWRr_t   AN_X1_CONTROL_OUI_LOWERr_reg;
    AN_X1_OUI_UPRr_t   AN_X1_CONTROL_OUI_UPPERr_reg;
    AN_X4_CL73_CTLSr_t AN_X4_CL73_CTLSr_t_reg;

    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);

    /********Setting AN_X1_OUI_LWR 0x9241*****/
    AN_X1_OUI_LWRr_CLR(AN_X1_CONTROL_OUI_LOWERr_reg);
    AN_X1_OUI_LWRr_SET(AN_X1_CONTROL_OUI_LOWERr_reg, oui.oui & 0xffff);
    PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_OUI_LWRr(pc, AN_X1_CONTROL_OUI_LOWERr_reg));

    /********Setting AN_X1_OUI_UPR 0x9240*****/
    AN_X1_OUI_UPRr_CLR(AN_X1_CONTROL_OUI_UPPERr_reg);
    AN_X1_OUI_UPRr_SET(AN_X1_CONTROL_OUI_UPPERr_reg, (oui.oui >> 16) & 0xff);
    PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_OUI_UPRr(pc, AN_X1_CONTROL_OUI_UPPERr_reg));

    /********CL73 AN MISC CONTROL 0xC1C6*****/
    AN_X4_CL73_CTLSr_CLR(AN_X4_CL73_CTLSr_t_reg);
    AN_X4_CL73_CTLSr_AN_OUI_OVERRIDE_BAM73_ADVf_SET(AN_X4_CL73_CTLSr_t_reg, oui.oui_override_bam73_adv);
    AN_X4_CL73_CTLSr_AN_OUI_OVERRIDE_BAM73_DETf_SET(AN_X4_CL73_CTLSr_t_reg, oui.oui_override_bam73_det);
    AN_X4_CL73_CTLSr_AN_OUI_OVERRIDE_HPAM_ADVf_SET(AN_X4_CL73_CTLSr_t_reg,  oui.oui_override_hpam_adv);
    AN_X4_CL73_CTLSr_AN_OUI_OVERRIDE_HPAM_DETf_SET(AN_X4_CL73_CTLSr_t_reg,  oui.oui_override_hpam_det);
    PHYMOD_IF_ERR_RETURN(MODIFY_AN_X4_CL73_CTLSr(pc, AN_X4_CL73_CTLSr_t_reg));

    return PHYMOD_E_NONE;
}

int tefmod_gen3_an_oui_get(PHYMOD_ST* pc, tefmod_gen3_an_oui_t* oui) {

    AN_X1_OUI_LWRr_t   AN_X1_CONTROL_OUI_LOWERr_reg;
    AN_X1_OUI_UPRr_t   AN_X1_CONTROL_OUI_UPPERr_reg;
    AN_X4_CL73_CTLSr_t AN_X4_CL73_CTLSr_t_reg;
  
    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);

    /********Setting AN X1 OUI LOWER 16 BITS 0x9241*****/
    PHYMOD_IF_ERR_RETURN(READ_AN_X1_OUI_LWRr(pc, &AN_X1_CONTROL_OUI_LOWERr_reg));
    /********Setting AN X1 OUI UPPER 8 BITS 0x9240*****/
    PHYMOD_IF_ERR_RETURN(READ_AN_X1_OUI_UPRr(pc, &AN_X1_CONTROL_OUI_UPPERr_reg));
    /********CL73 AN MISC CONTROL 0xC1C6*****/
    PHYMOD_IF_ERR_RETURN(READ_AN_X4_CL73_CTLSr(pc, &AN_X4_CL73_CTLSr_t_reg));

    oui->oui  = AN_X1_OUI_LWRr_GET(AN_X1_CONTROL_OUI_LOWERr_reg);
    oui->oui |= ((AN_X1_OUI_UPRr_GET(AN_X1_CONTROL_OUI_UPPERr_reg)) << 16);

    oui->oui_override_bam73_adv = AN_X4_CL73_CTLSr_AN_OUI_OVERRIDE_BAM73_ADVf_GET(AN_X4_CL73_CTLSr_t_reg);
    oui->oui_override_bam73_det = AN_X4_CL73_CTLSr_AN_OUI_OVERRIDE_BAM73_DETf_GET(AN_X4_CL73_CTLSr_t_reg);
    oui->oui_override_hpam_adv  = AN_X4_CL73_CTLSr_AN_OUI_OVERRIDE_HPAM_ADVf_GET(AN_X4_CL73_CTLSr_t_reg);
    oui->oui_override_hpam_det  = AN_X4_CL73_CTLSr_AN_OUI_OVERRIDE_HPAM_DETf_GET(AN_X4_CL73_CTLSr_t_reg);

    return PHYMOD_E_NONE;
}

int tefmod_gen3_autoneg_status_get(PHYMOD_ST* pc, int *an_en, int *an_done) 
{
    AN_X4_CL73_CFGr_t  AN_X4_CL73_CFGr_reg;
    AN_X4_AN_MISC_STSr_t  AN_X4_AN_MISC_STSr_reg;

    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
    AN_X4_CL73_CFGr_CLR(AN_X4_CL73_CFGr_reg);
    AN_X4_AN_MISC_STSr_CLR(AN_X4_AN_MISC_STSr_reg);

    PHYMOD_IF_ERR_RETURN (READ_AN_X4_CL73_CFGr(pc, &AN_X4_CL73_CFGr_reg));
    PHYMOD_IF_ERR_RETURN (READ_AN_X4_AN_MISC_STSr(pc, &AN_X4_AN_MISC_STSr_reg));

    *an_en = AN_X4_CL73_CFGr_CL73_AN_RESTARTf_GET(AN_X4_CL73_CFGr_reg);
    *an_done = AN_X4_AN_MISC_STSr_AN_COMPLETEf_GET(AN_X4_AN_MISC_STSr_reg);

    return PHYMOD_E_NONE;
}

int tefmod_gen3_autoneg_lp_status_get(PHYMOD_ST* pc, uint16_t *baseP0, uint16_t *baseP1, uint16_t *baseP2, uint16_t *nextP3, uint16_t *nextP4) 
{
    AN_X4_LP_BASE1r_t   AN_X4_LP_BASE1r_reg;
    AN_X4_LP_BASE2r_t   AN_X4_LP_BASE2r_reg;
    AN_X4_LP_BASE3r_t   AN_X4_LP_BASE3r_reg;
    AN_X4_LP_OUI_UP4r_t AN_X4_LP_OUI_UP4r_reg;
    AN_X4_LP_OUI_UP5r_t AN_X4_LP_OUI_UP5r_reg;

    AN_X4_LP_BASE1r_CLR(  AN_X4_LP_BASE1r_reg);
    AN_X4_LP_BASE2r_CLR(  AN_X4_LP_BASE2r_reg);
    AN_X4_LP_BASE3r_CLR(  AN_X4_LP_BASE3r_reg);
    AN_X4_LP_OUI_UP4r_CLR(AN_X4_LP_OUI_UP4r_reg);
    AN_X4_LP_OUI_UP5r_CLR(AN_X4_LP_OUI_UP5r_reg);

    PHYMOD_IF_ERR_RETURN(READ_AN_X4_LP_BASE1r(pc,  &AN_X4_LP_BASE1r_reg));
    PHYMOD_IF_ERR_RETURN(READ_AN_X4_LP_BASE2r(pc,  &AN_X4_LP_BASE2r_reg));
    PHYMOD_IF_ERR_RETURN(READ_AN_X4_LP_BASE3r(pc,  &AN_X4_LP_BASE3r_reg));
    PHYMOD_IF_ERR_RETURN(READ_AN_X4_LP_OUI_UP4r(pc,&AN_X4_LP_OUI_UP4r_reg));
    PHYMOD_IF_ERR_RETURN(READ_AN_X4_LP_OUI_UP5r(pc,&AN_X4_LP_OUI_UP5r_reg));

    *baseP0 = AN_X4_LP_BASE1r_GET(AN_X4_LP_BASE1r_reg);
    *baseP1 = AN_X4_LP_BASE2r_GET(AN_X4_LP_BASE2r_reg);
    *baseP2 = AN_X4_LP_BASE3r_GET(AN_X4_LP_BASE3r_reg);
    *nextP3 = AN_X4_LP_OUI_UP4r_GET(AN_X4_LP_OUI_UP4r_reg);
    *nextP4 = AN_X4_LP_OUI_UP5r_GET(AN_X4_LP_OUI_UP5r_reg);

    return PHYMOD_E_NONE;
}


int tefmod_gen3_hg2_codec_set(PHYMOD_ST* pc, tefmod_gen3_hg2_codec_t hg2_codec)
{
    TX_X4_ENC0r_t  TX_X4_ENC0_reg;
    RX_X4_DEC_CTL0r_t  RX_X4_DEC_CTL0_reg;

    READ_TX_X4_ENC0r(pc, &TX_X4_ENC0_reg);
    READ_RX_X4_DEC_CTL0r(pc, &RX_X4_DEC_CTL0_reg);
    switch (hg2_codec) {
    case TEFMOD_HG2_CODEC_OFF:
        TX_X4_ENC0r_HG2_ENABLEf_SET(TX_X4_ENC0_reg, 0);
        TX_X4_ENC0r_HG2_CODECf_SET(TX_X4_ENC0_reg, 1);
        TX_X4_ENC0r_HG2_MESSAGE_INVALID_CODE_ENABLEf_SET(TX_X4_ENC0_reg, 1);
        RX_X4_DEC_CTL0r_HG2_ENABLEf_SET(RX_X4_DEC_CTL0_reg, 0);
        RX_X4_DEC_CTL0r_HG2_CODECf_SET(RX_X4_DEC_CTL0_reg, 1);
        RX_X4_DEC_CTL0r_HG2_MESSAGE_INVALID_CODE_ENABLEf_SET(RX_X4_DEC_CTL0_reg, 1);
        break;
    case TEFMOD_HG2_CODEC_ON_8BYTE_IPG:
        TX_X4_ENC0r_HG2_ENABLEf_SET(TX_X4_ENC0_reg, 1);
        TX_X4_ENC0r_HG2_CODECf_SET(TX_X4_ENC0_reg, 1);
        TX_X4_ENC0r_HG2_MESSAGE_INVALID_CODE_ENABLEf_SET(TX_X4_ENC0_reg, 1);
        RX_X4_DEC_CTL0r_HG2_ENABLEf_SET(RX_X4_DEC_CTL0_reg, 1);
        RX_X4_DEC_CTL0r_HG2_CODECf_SET(RX_X4_DEC_CTL0_reg, 1);
        RX_X4_DEC_CTL0r_HG2_MESSAGE_INVALID_CODE_ENABLEf_SET(RX_X4_DEC_CTL0_reg, 1);
        break;
    case TEFMOD_HG2_CODEC_ON_9BYTE_IPG:
        TX_X4_ENC0r_HG2_ENABLEf_SET(TX_X4_ENC0_reg, 1);
        TX_X4_ENC0r_HG2_CODECf_SET(TX_X4_ENC0_reg, 0);
        TX_X4_ENC0r_HG2_MESSAGE_INVALID_CODE_ENABLEf_SET(TX_X4_ENC0_reg, 1);
        RX_X4_DEC_CTL0r_HG2_ENABLEf_SET(RX_X4_DEC_CTL0_reg, 1);
        RX_X4_DEC_CTL0r_HG2_CODECf_SET(RX_X4_DEC_CTL0_reg, 0);
        RX_X4_DEC_CTL0r_HG2_MESSAGE_INVALID_CODE_ENABLEf_SET(RX_X4_DEC_CTL0_reg, 1);
        break;
    default:
        TX_X4_ENC0r_HG2_ENABLEf_SET(TX_X4_ENC0_reg, 0);
        TX_X4_ENC0r_HG2_CODECf_SET(TX_X4_ENC0_reg, 1);
        TX_X4_ENC0r_HG2_MESSAGE_INVALID_CODE_ENABLEf_SET(TX_X4_ENC0_reg, 1);
        RX_X4_DEC_CTL0r_HG2_ENABLEf_SET(RX_X4_DEC_CTL0_reg, 0);
        RX_X4_DEC_CTL0r_HG2_CODECf_SET(RX_X4_DEC_CTL0_reg, 1);
        RX_X4_DEC_CTL0r_HG2_MESSAGE_INVALID_CODE_ENABLEf_SET(RX_X4_DEC_CTL0_reg, 1);
        break;
    }
    MODIFY_RX_X4_DEC_CTL0r(pc, RX_X4_DEC_CTL0_reg);
    MODIFY_TX_X4_ENC0r(pc,TX_X4_ENC0_reg); 

    return PHYMOD_E_NONE;
}

int tefmod_gen3_hg2_codec_get(PHYMOD_ST* pc, tefmod_gen3_hg2_codec_t* hg2_codec)
{
    TX_X4_ENC0r_t  TX_X4_ENC0_reg;

    READ_TX_X4_ENC0r(pc, &TX_X4_ENC0_reg);
    if (TX_X4_ENC0r_HG2_ENABLEf_GET(TX_X4_ENC0_reg)) {
        if (TX_X4_ENC0r_HG2_CODECf_GET(TX_X4_ENC0_reg)) {
            *hg2_codec = TEFMOD_HG2_CODEC_ON_8BYTE_IPG;
        } else {
            *hg2_codec = TEFMOD_HG2_CODEC_ON_9BYTE_IPG;
        }
    } else {
        *hg2_codec = TEFMOD_HG2_CODEC_OFF;
    }
    return PHYMOD_E_NONE;
}

int tefmod_gen3_encode_mode_get(PHYMOD_ST* pc, tefmod_gen3_encode_mode *mode)
{
    SC_X4_RSLVD0r_t    reg_resolved;

    SC_X4_RSLVD0r_CLR(reg_resolved);

    PHYMOD_IF_ERR_RETURN
        (READ_SC_X4_RSLVD0r(pc, &reg_resolved));
    *mode = SC_X4_RSLVD0r_T_ENC_MODEf_GET(reg_resolved);

  return PHYMOD_E_NONE;
}

int tefmod_gen3_fec_correctable_counter_get(PHYMOD_ST* pc, uint32_t* count)
{
    RX_X4_FEC_CORRBLKSL0r_t countl_0;
    RX_X4_FEC_CORRBLKSL1r_t countl_1;
    RX_X4_FEC_CORRBLKSL2r_t countl_2;
    RX_X4_FEC_CORRBLKSL3r_t countl_3;
    RX_X4_FEC_CORRBLKSL4r_t countl_4;
    RX_X4_FEC_CORRBLKSH0r_t counth_0;
    RX_X4_FEC_CORRBLKSH1r_t counth_1;
    RX_X4_FEC_CORRBLKSH2r_t counth_2;
    RX_X4_FEC_CORRBLKSH3r_t counth_3;
    RX_X4_FEC_CORRBLKSH4r_t counth_4;
    uint32_t sum = 0, count_vl = 0;

    /* Per lane based counter */
    /* 1 physical lane can map to 5 virtual lane */
    PHYMOD_IF_ERR_RETURN (READ_RX_X4_FEC_CORRBLKSL0r(pc, &countl_0));
    PHYMOD_IF_ERR_RETURN (READ_RX_X4_FEC_CORRBLKSH0r(pc, &counth_0));
    sum = (RX_X4_FEC_CORRBLKSH0r_GET(counth_0) << 16) | (RX_X4_FEC_CORRBLKSL0r_GET(countl_0) & 0xffff);
    PHYMOD_IF_ERR_RETURN (READ_RX_X4_FEC_CORRBLKSL1r(pc, &countl_1));
    PHYMOD_IF_ERR_RETURN (READ_RX_X4_FEC_CORRBLKSH1r(pc, &counth_1));
    count_vl = (RX_X4_FEC_CORRBLKSH1r_GET(counth_1) << 16) | (RX_X4_FEC_CORRBLKSL1r_GET(countl_1) & 0xffff);
    /* Check overflow */
    /* If sum+count_vl > 0xffffffff, will cause overflow, set sum=0xffffffff */
    if (count_vl > 0xffffffff - sum) {
        sum = 0xffffffff;
    } else {
        sum += count_vl;
    }
    PHYMOD_IF_ERR_RETURN (READ_RX_X4_FEC_CORRBLKSL2r(pc, &countl_2));
    PHYMOD_IF_ERR_RETURN (READ_RX_X4_FEC_CORRBLKSH2r(pc, &counth_2));
    count_vl = (RX_X4_FEC_CORRBLKSH2r_GET(counth_2) << 16) | (RX_X4_FEC_CORRBLKSL2r_GET(countl_2) & 0xffff);
    /* Check overflow */
    if (count_vl > 0xffffffff - sum) {
        sum = 0xffffffff;
    } else {
        sum += count_vl;
    }
    PHYMOD_IF_ERR_RETURN (READ_RX_X4_FEC_CORRBLKSL3r(pc, &countl_3));
    PHYMOD_IF_ERR_RETURN (READ_RX_X4_FEC_CORRBLKSH3r(pc, &counth_3));
    count_vl = (RX_X4_FEC_CORRBLKSH3r_GET(counth_3) << 16) | (RX_X4_FEC_CORRBLKSL3r_GET(countl_3) & 0xffff);
    /* Check overflow */
    if (count_vl > 0xffffffff - sum) {
        sum = 0xffffffff;
    } else {
        sum += count_vl;
    }
    PHYMOD_IF_ERR_RETURN (READ_RX_X4_FEC_CORRBLKSL4r(pc, &countl_4));
    PHYMOD_IF_ERR_RETURN (READ_RX_X4_FEC_CORRBLKSH4r(pc, &counth_4));
    count_vl = (RX_X4_FEC_CORRBLKSH4r_GET(counth_4) << 16) | (RX_X4_FEC_CORRBLKSL4r_GET(countl_4) & 0xffff);
    /* Check overflow */
    if (count_vl > 0xffffffff - sum) {
        *count = 0xffffffff;
    } else {
        *count = sum + count_vl;
    }

    return PHYMOD_E_NONE;
}

int tefmod_gen3_fec_uncorrectable_counter_get(PHYMOD_ST* pc, uint32_t* count)
{
    RX_X4_FEC_UNCORRBLKSL0r_t countl_0;
    RX_X4_FEC_UNCORRBLKSL1r_t countl_1;
    RX_X4_FEC_UNCORRBLKSL2r_t countl_2;
    RX_X4_FEC_UNCORRBLKSL3r_t countl_3;
    RX_X4_FEC_UNCORRBLKSL4r_t countl_4;
    RX_X4_FEC_UNCORRBLKSH0r_t counth_0;
    RX_X4_FEC_UNCORRBLKSH1r_t counth_1;
    RX_X4_FEC_UNCORRBLKSH2r_t counth_2;
    RX_X4_FEC_UNCORRBLKSH3r_t counth_3;
    RX_X4_FEC_UNCORRBLKSH4r_t counth_4;
    uint32_t sum = 0, count_vl = 0;

    /* Per lane based counter */
    /* 1 physical lane can map to 5 virtual lane */
    PHYMOD_IF_ERR_RETURN (READ_RX_X4_FEC_UNCORRBLKSL0r(pc, &countl_0));
    PHYMOD_IF_ERR_RETURN (READ_RX_X4_FEC_UNCORRBLKSH0r(pc, &counth_0));
    sum = (RX_X4_FEC_UNCORRBLKSH0r_GET(counth_0) << 16) | (RX_X4_FEC_UNCORRBLKSL0r_GET(countl_0) & 0xffff);
    PHYMOD_IF_ERR_RETURN (READ_RX_X4_FEC_UNCORRBLKSL1r(pc, &countl_1));
    PHYMOD_IF_ERR_RETURN (READ_RX_X4_FEC_UNCORRBLKSH1r(pc, &counth_1));
    count_vl = (RX_X4_FEC_UNCORRBLKSH1r_GET(counth_1) << 16) | (RX_X4_FEC_UNCORRBLKSL1r_GET(countl_1) & 0xffff);
    /* Check overflow */
    /* If sum+count_vl > 0xffffffff, will cause overflow, set sum=0xffffffff */
    if (count_vl > 0xffffffff - sum) {
        sum = 0xffffffff;
    } else {
        sum += count_vl;
    }
    PHYMOD_IF_ERR_RETURN (READ_RX_X4_FEC_UNCORRBLKSL2r(pc, &countl_2));
    PHYMOD_IF_ERR_RETURN (READ_RX_X4_FEC_UNCORRBLKSH2r(pc, &counth_2));
    count_vl = (RX_X4_FEC_UNCORRBLKSH2r_GET(counth_2) << 16) | (RX_X4_FEC_UNCORRBLKSL2r_GET(countl_2) & 0xffff);
    /* Check overflow */
    if (count_vl > 0xffffffff - sum) {
        sum = 0xffffffff;
    } else {
        sum += count_vl;
    }
    PHYMOD_IF_ERR_RETURN (READ_RX_X4_FEC_UNCORRBLKSL3r(pc, &countl_3));
    PHYMOD_IF_ERR_RETURN (READ_RX_X4_FEC_UNCORRBLKSH3r(pc, &counth_3));
    count_vl = (RX_X4_FEC_UNCORRBLKSH3r_GET(counth_3) << 16) | (RX_X4_FEC_UNCORRBLKSL3r_GET(countl_3) & 0xffff);
    /* Check overflow */
    if (count_vl > 0xffffffff - sum) {
        sum = 0xffffffff;
    } else {
        sum += count_vl;
    }
    PHYMOD_IF_ERR_RETURN (READ_RX_X4_FEC_UNCORRBLKSL4r(pc, &countl_4));
    PHYMOD_IF_ERR_RETURN (READ_RX_X4_FEC_UNCORRBLKSH4r(pc, &counth_4));
    count_vl = (RX_X4_FEC_UNCORRBLKSH4r_GET(counth_4) << 16) | (RX_X4_FEC_UNCORRBLKSL4r_GET(countl_4) & 0xffff);
    /* Check overflow */
    if (count_vl > 0xffffffff - sum) {
        *count = 0xffffffff;
    } else {
        *count = sum + count_vl;
    }

    return PHYMOD_E_NONE;
}

int tefmod_gen3_fec_cl91_correctable_counter_get(PHYMOD_ST* pc, uint32_t* count)
{
    RX_X4_FEC_CORR_CTR0r_t count0;
    RX_X4_FEC_CORR_CTR1r_t count1;

    /* Per port based counter */
    PHYMOD_IF_ERR_RETURN (READ_RX_X4_FEC_CORR_CTR0r(pc, &count0));
    PHYMOD_IF_ERR_RETURN (READ_RX_X4_FEC_CORR_CTR1r(pc, &count1));
    *count = (RX_X4_FEC_CORR_CTR1r_GET(count1) << 16)
                 | (RX_X4_FEC_CORR_CTR0r_GET(count0) & 0xffff);

    return PHYMOD_E_NONE;
}

int tefmod_gen3_fec_cl91_uncorrectable_counter_get(PHYMOD_ST* pc, uint32_t* count)
{
    RX_X4_FEC_UNCORR_CTR0r_t count0;
    RX_X4_FEC_UNCORR_CTR1r_t count1;

    /* Per port based counter */
    PHYMOD_IF_ERR_RETURN (READ_RX_X4_FEC_UNCORR_CTR0r(pc, &count0));
    PHYMOD_IF_ERR_RETURN (READ_RX_X4_FEC_UNCORR_CTR1r(pc, &count1));
    *count = (RX_X4_FEC_UNCORR_CTR1r_GET(count1) << 16)
                 | (RX_X4_FEC_UNCORR_CTR0r_GET(count0) & 0xffff);

    return PHYMOD_E_NONE;
}

/* The number of cl82 BIP error */
int tefmod_gen3_bip_error_counter_get(PHYMOD_ST* pc, uint32_t* count)
{
    RX_X4_BIPCNT0r_t count0;
    RX_X4_BIPCNT1r_t count1;
    RX_X4_BIPCNT2r_t count2;

    PHYMOD_IF_ERR_RETURN (READ_RX_X4_BIPCNT0r(pc, &count0));
    PHYMOD_IF_ERR_RETURN (READ_RX_X4_BIPCNT1r(pc, &count1));
    PHYMOD_IF_ERR_RETURN (READ_RX_X4_BIPCNT2r(pc, &count2));
    *count =  RX_X4_BIPCNT0r_BIP_ERROR_COUNT_0f_GET(count0) + RX_X4_BIPCNT0r_BIP_ERROR_COUNT_1f_GET(count0);
    *count += RX_X4_BIPCNT1r_BIP_ERROR_COUNT_2f_GET(count1) + RX_X4_BIPCNT1r_BIP_ERROR_COUNT_3f_GET(count1);
    *count += RX_X4_BIPCNT2r_BIP_ERROR_COUNT_4f_GET(count2);

    return PHYMOD_E_NONE;
}

/* The number of times BER_BAD_SH state is entered for cl49. */
int tefmod_gen3_cl49_ber_counter_get(PHYMOD_ST* pc, uint32_t* count)
{
    RX_X4_DEC_STS3r_t dec_sts;

    PHYMOD_IF_ERR_RETURN (READ_RX_X4_DEC_STS3r(pc, &dec_sts));
    *count = RX_X4_DEC_STS3r_CL49_BER_COUNTf_GET(dec_sts);

    return PHYMOD_E_NONE;
}

/* The number of times BER_BAD_SH state is entered for cl82. */
int tefmod_gen3_cl82_ber_counter_get(PHYMOD_ST* pc, uint32_t* count)
{
    CL82_BER_HOr_t count_ho;
    CL82_BER_LOr_t count_lo;

    PHYMOD_IF_ERR_RETURN (READ_CL82_BER_HOr(pc, &count_ho));
    PHYMOD_IF_ERR_RETURN (READ_CL82_BER_LOr(pc, &count_lo));
    *count = (CL82_BER_HOr_BER_HOf_GET(count_ho) << 8) | (CL82_BER_LOr_BER_LOf_GET(count_lo));

    return PHYMOD_E_NONE;
}

int tefmod_gen3_set_speed_id(PHYMOD_ST *pc, int speed_id)
{
    SC_X4_CTLr_t xgxs_x4_ctrl;
    phymod_access_t pa_copy;
    int start_lane = 0, num_lane = 0;

    /* need to figure out what's the starting lane */
    PHYMOD_MEMCPY(&pa_copy, pc, sizeof(pa_copy));
    PHYMOD_IF_ERR_RETURN
        (phymod_util_lane_config_get(pc, &start_lane, &num_lane));
    pa_copy.lane_mask = 0x1 << start_lane;

    /* write the speed_id into the speed_change register */
    SC_X4_CTLr_CLR(xgxs_x4_ctrl);
    SC_X4_CTLr_SPEEDf_SET(xgxs_x4_ctrl, speed_id);
    MODIFY_SC_X4_CTLr(pc, xgxs_x4_ctrl);

    return PHYMOD_E_NONE;
}

int tefmod_gen3_pxgng_status_an_ack_complete_get(PHYMOD_ST *pc, int *ack_complte) 
{
    AN_X4_PXNG_STSr_t an_pxng_status_reg;

    AN_X4_PXNG_STSr_CLR(an_pxng_status_reg);
    PHYMOD_IF_ERR_RETURN
        (READ_AN_X4_PXNG_STSr(pc, &an_pxng_status_reg));
    PHYMOD_IF_ERR_RETURN
        (READ_AN_X4_PXNG_STSr(pc, &an_pxng_status_reg));    

     *ack_complte = AN_X4_PXNG_STSr_COMPLETE_ACKf_GET(an_pxng_status_reg);
     
     return PHYMOD_E_NONE;
}

int tefmod_gen3_pxgng_status_an_good_chk_get(PHYMOD_ST *pc, uint8_t *an_good_chk) 
{
    AN_X4_PXNG_STSr_t an_pxng_status_reg;

    AN_X4_PXNG_STSr_CLR(an_pxng_status_reg);
    PHYMOD_IF_ERR_RETURN
      (READ_AN_X4_PXNG_STSr(pc, &an_pxng_status_reg));

    *an_good_chk = AN_X4_PXNG_STSr_AN_GOOD_CHECKf_GET(an_pxng_status_reg);

    return PHYMOD_E_NONE;

}

int tefmod_gen3_pll_idx_set(phymod_access_t *pa, int pll_index)
{

    if (pll_index > 2) {
        return PHYMOD_E_CONFIG;
    }
    pa->pll_idx = pll_index;

    return PHYMOD_E_NONE;
}

uint8_t tefmod_gen3_pll_idx_get(PHYMOD_ST *pc)
{
    uint8_t pll_index = 0;
    pll_index = pc->pll_idx;

    return pll_index; 
}


/* 
 * Set PLL mode[4:0] in PLL configure reg to program PLL multiplier
 */
int tefmod_gen3_pll_mode_set(PHYMOD_ST* pc, int pll_mode)
{
    uint8_t pll_index = 0;
    phymod_access_t pc_copy;
    PLL_CAL_CTL7r_t    PLL_CAL_CTL7r_reg;

    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
    PHYMOD_MEMCPY(&pc_copy, pc, sizeof(phymod_access_t));

    pll_index = tefmod_gen3_pll_idx_get(pc);
    pc_copy.pll_idx = pll_index;
    PLL_CAL_CTL7r_CLR(PLL_CAL_CTL7r_reg);
    PLL_CAL_CTL7r_PLL_MODEf_SET(PLL_CAL_CTL7r_reg, pll_mode);
    PHYMOD_IF_ERR_RETURN(MODIFY_PLL_CAL_CTL7r(&pc_copy, PLL_CAL_CTL7r_reg));

    return PHYMOD_E_NONE;
}

/* 
 * Get PLL multiplier 
 */
int tefmod_gen3_pll_mode_get(PHYMOD_ST *pc, int *pll_mode)
{
    uint8_t pll_index = 0;
    phymod_access_t pc_copy;
    PLL_CAL_CTL7r_t PLL_CAL_CTL7r_reg;
  
    TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
    PHYMOD_MEMCPY(&pc_copy, pc, sizeof(phymod_access_t));
    PLL_CAL_CTL7r_CLR(PLL_CAL_CTL7r_reg);
    pll_index = tefmod_gen3_pll_idx_get(pc);
    pc_copy.pll_idx = pll_index;
   
    PHYMOD_IF_ERR_RETURN
        (READ_PLL_CAL_CTL7r(&pc_copy, &PLL_CAL_CTL7r_reg));
    *pll_mode = PLL_CAL_CTL7r_PLL_MODEf_GET(PLL_CAL_CTL7r_reg);  
     
    return PHYMOD_E_NONE;
}

/* 
 * Set PLL selection 
 */
int tefmod_gen3_pll_select_set(PHYMOD_ST* pc, int pll_select)
{
   RXTXCOM_PLL_SEL_CTLr_t   RXTXCOM_PLL_SEL_CTLr_reg;

   TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
   RXTXCOM_PLL_SEL_CTLr_CLR(RXTXCOM_PLL_SEL_CTLr_reg);
   READ_RXTXCOM_PLL_SEL_CTLr(pc, &RXTXCOM_PLL_SEL_CTLr_reg);

   RXTXCOM_PLL_SEL_CTLr_PLL_SELECTf_SET(RXTXCOM_PLL_SEL_CTLr_reg, pll_select);
   PHYMOD_IF_ERR_RETURN(MODIFY_RXTXCOM_PLL_SEL_CTLr(pc, RXTXCOM_PLL_SEL_CTLr_reg));

   return PHYMOD_E_NONE;
}

/* 
 * Get PLL selection 
 */
int tefmod_gen3_pll_select_get(PHYMOD_ST* pc, int *pll_select)
{
   RXTXCOM_PLL_SEL_CTLr_t   RXTXCOM_PLL_SEL_CTLr_reg;

   TEFMOD_GEN3_DBG_IN_FUNC_INFO(pc);
   RXTXCOM_PLL_SEL_CTLr_CLR(RXTXCOM_PLL_SEL_CTLr_reg);
   READ_RXTXCOM_PLL_SEL_CTLr(pc, &RXTXCOM_PLL_SEL_CTLr_reg);

   *pll_select = RXTXCOM_PLL_SEL_CTLr_PLL_SELECTf_GET(RXTXCOM_PLL_SEL_CTLr_reg);

   return PHYMOD_E_NONE;
}


int tefmod_gen3_lane_soft_reset(PHYMOD_ST* pc, int enable)
{
    RXTXCOM_LN_CLK_RST_N_PWRDWN_CTLr_t RXTXCOM_LN_CLK_RST_N_PWRDWN_CTLr_reg;
 
    RXTXCOM_LN_CLK_RST_N_PWRDWN_CTLr_CLR(RXTXCOM_LN_CLK_RST_N_PWRDWN_CTLr_reg);
    RXTXCOM_LN_CLK_RST_N_PWRDWN_CTLr_LN_DP_S_RSTBf_SET(RXTXCOM_LN_CLK_RST_N_PWRDWN_CTLr_reg, enable);
    PHYMOD_IF_ERR_RETURN
        (MODIFY_RXTXCOM_LN_CLK_RST_N_PWRDWN_CTLr(pc, RXTXCOM_LN_CLK_RST_N_PWRDWN_CTLr_reg));

    return PHYMOD_E_NONE;

}

int tefmod_gen3_pmd_ln_h_rstb_pkill_override(PHYMOD_ST* pc, int value)
{
    RXTXCOM_LN_RST_N_PWRDN_PIN_KILL_CTLr_t RXTXCOM_LN_RST_N_PWRDN_PIN_KILL_CTLr_reg;	    

    RXTXCOM_LN_RST_N_PWRDN_PIN_KILL_CTLr_CLR(RXTXCOM_LN_RST_N_PWRDN_PIN_KILL_CTLr_reg);
    RXTXCOM_LN_RST_N_PWRDN_PIN_KILL_CTLr_PMD_LN_H_RSTB_PKILLf_SET(RXTXCOM_LN_RST_N_PWRDN_PIN_KILL_CTLr_reg, value);
    PHYMOD_IF_ERR_RETURN
        (MODIFY_RXTXCOM_LN_RST_N_PWRDN_PIN_KILL_CTLr(pc, RXTXCOM_LN_RST_N_PWRDN_PIN_KILL_CTLr_reg));

    return PHYMOD_E_NONE;

}

int tefmod_gen3_pmd_ams_pll_idq_set(PHYMOD_ST* pc, const phymod_afe_pll_t *afe_pll)
{
    AMS_PLL_PLL_CTL2r_t AMS_PLL_PLL_CTL2r_reg;

    AMS_PLL_PLL_CTL2r_CLR(AMS_PLL_PLL_CTL2r_reg);   
    
    if(afe_pll->afe_pll_change_default) {
        AMS_PLL_PLL_CTL2r_AMS_PLL_IQPf_SET(AMS_PLL_PLL_CTL2r_reg, afe_pll->ams_pll_iqp);
    } else {
        AMS_PLL_PLL_CTL2r_AMS_PLL_IQPf_SET(AMS_PLL_PLL_CTL2r_reg, 0x5);
    }
    PHYMOD_IF_ERR_RETURN
        (MODIFY_AMS_PLL_PLL_CTL2r(pc, AMS_PLL_PLL_CTL2r_reg));

    return PHYMOD_E_NONE;

}

int tefmod_gen3_pmd_core_dp_reset(PHYMOD_ST* pc, int enable)
{
    CORE_PLL_TOP_USER_CTLr_t CORE_PLL_TOP_USER_CTLr_reg;

    CORE_PLL_TOP_USER_CTLr_CLR(CORE_PLL_TOP_USER_CTLr_reg);
    if (enable){
        CORE_PLL_TOP_USER_CTLr_CORE_DP_S_RSTBf_SET(CORE_PLL_TOP_USER_CTLr_reg, 0);
    } else {
        CORE_PLL_TOP_USER_CTLr_CORE_DP_S_RSTBf_SET(CORE_PLL_TOP_USER_CTLr_reg, 1);
    }
    PHYMOD_IF_ERR_RETURN
       (MODIFY_CORE_PLL_TOP_USER_CTLr(pc, CORE_PLL_TOP_USER_CTLr_reg));

    return PHYMOD_E_NONE;
}

int tefmod_gen3_uc_active_get(PHYMOD_ST* pc, uint32_t *enable)
{
    DIG_TOP_USER_CTL0r_t DIG_TOP_USER_CTL0r_reg;

    DIG_TOP_USER_CTL0r_CLR(DIG_TOP_USER_CTL0r_reg);
    PHYMOD_IF_ERR_RETURN(READ_DIG_TOP_USER_CTL0r(pc, &DIG_TOP_USER_CTL0r_reg));   
    *enable = DIG_TOP_USER_CTL0r_UC_ACTIVEf_GET(DIG_TOP_USER_CTL0r_reg);

    return PHYMOD_E_NONE;

}

int tefmod_gen3_configure_pll(phymod_access_t *pc, phymod_tscf_pll_multiplier_t pll_mode, phymod_ref_clk_t ref_clk)
{
    enum falcon2_monterey_pll_enum pll_cfg;

    switch (pll_mode){
        case phymod_TSCF_PLL_DIV128:
            pll_cfg = FALCON2_MONTEREY_pll_div_128x;
            break; 
        case phymod_TSCF_PLL_DIV132:
            pll_cfg = FALCON2_MONTEREY_pll_div_132x;
            break; 
        case phymod_TSCF_PLL_DIV140:
            pll_cfg = FALCON2_MONTEREY_pll_div_140x;
            break; 
        case phymod_TSCF_PLL_DIV160:
            pll_cfg = FALCON2_MONTEREY_pll_div_160x;
            break; 
        case phymod_TSCF_PLL_DIV165:
            pll_cfg = FALCON2_MONTEREY_pll_div_165x;
            break; 
        case phymod_TSCF_PLL_DIV175:
            pll_cfg = FALCON2_MONTEREY_pll_div_175x;
            break; 
        default: 
            pll_cfg = FALCON2_MONTEREY_pll_div_132x;
            break; 
    }

    PHYMOD_IF_ERR_RETURN
        (falcon2_monterey_configure_pll(pc, pll_cfg));

    return PHYMOD_E_NONE;
}

/* 
 * Translate pll_mode to pll multiplier
 */
int tefmod_gen3_pll_multiplier_get(uint32_t pll_div, uint32_t *pll_multiplier)
{
    switch (pll_div) {
        case 0x0:
            *pll_multiplier = 64;
             break;
        case 0x1:
            *pll_multiplier = 66;
            break;
        case 0x2:
            *pll_multiplier = 80;
            break;
        case 0x3:
            *pll_multiplier = 128;
            break;
        case 0x4:
            *pll_multiplier = 132;
            break;
        case 0x5:
            *pll_multiplier = 140;
            break;
        case 0x6:
            *pll_multiplier = 160;
            break;
        case 0x7:
            *pll_multiplier = 165;
            break;
        case 0x8:
            *pll_multiplier = 168;
            break;
        case 0x9:
            *pll_multiplier = 170;
            break;
        case 0xa:
            *pll_multiplier = 175;
            break;
        case 0xb:
            *pll_multiplier = 180;
            break;
        case 0xc:
            *pll_multiplier = 184;
            break;
        case 0xd:
            *pll_multiplier = 200;
            break;
        case 0xe:
            *pll_multiplier = 224;
            break;
        case 0xf:
            *pll_multiplier = 264;
            break;
        default:
            *pll_multiplier = 165;
            break;
    }

    return PHYMOD_E_NONE;
}

/* 
 * Caculate vco rate in MHz 
 */
int tefmod_gen3_pll_to_vco_rate(PHYMOD_ST *pc, phymod_tscf_pll_multiplier_t pll_mode, phymod_ref_clk_t ref_clk, uint32_t* vco_rate)
{
    uint32_t pll_multiplier = 0;

    tefmod_gen3_pll_multiplier_get(pll_mode, &pll_multiplier);
    if (ref_clk == phymodRefClk156Mhz){
        *vco_rate = pll_multiplier * 156 + pll_multiplier *25/100;
    } 
    if (ref_clk == phymodRefClk125Mhz){
       *vco_rate =  pll_multiplier * 125;
    }

    return PHYMOD_E_NONE;
}


