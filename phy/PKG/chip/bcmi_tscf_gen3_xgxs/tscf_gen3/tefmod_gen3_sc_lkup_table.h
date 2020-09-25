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
 *  Broadcom Corporation
 *  Proprietary and Confidential information
 *  All rights reserved
 *  This source file is the property of Broadcom Corporation, and
 *  may not be copied or distributed in any isomorphic form without the
 *  prior written consent of Broadcom Corporation.
 *----------------------------------------------------------------------
 *  Description: define enumerators  
 *----------------------------------------------------------------------*/
#ifndef tefmod_gen3_sc_lkup_table_H_
#define tefmod_gen3_sc_lkup_table_H_ 

#define _SDK_TEFMOD_GEN3_ 1

#include "tefmod_gen3_enum_defines.h"
#include "tefmod_gen3.h"
#include <phymod/phymod.h>

/****************************************************************************
 * Enums: sc_x4_control_sc
 */
#define sc_x4_control_sc_S_10G_CR1                         0
#define sc_x4_control_sc_S_10G_KR1                         1
#define sc_x4_control_sc_S_10G_X1                          2
#define sc_x4_control_sc_S_10G_HG2_CR1                     4
#define sc_x4_control_sc_S_10G_HG2_KR1                     5
#define sc_x4_control_sc_S_10G_HG2_X1                      6
#define sc_x4_control_sc_S_20G_CR1                         8
#define sc_x4_control_sc_S_20G_KR1                         9
#define sc_x4_control_sc_S_20G_X1                          10
#define sc_x4_control_sc_S_20G_HG2_CR1                     12
#define sc_x4_control_sc_S_20G_HG2_KR1                     13
#define sc_x4_control_sc_S_20G_HG2_X1                      14
#define sc_x4_control_sc_S_25G_CR1                         16
#define sc_x4_control_sc_S_25G_KR1                         17
#define sc_x4_control_sc_S_25G_X1                          18
#define sc_x4_control_sc_S_25G_HG2_CR1                     20
#define sc_x4_control_sc_S_25G_HG2_KR1                     21
#define sc_x4_control_sc_S_25G_HG2_X1                      22
#define sc_x4_control_sc_S_20G_CR2                         24
#define sc_x4_control_sc_S_20G_KR2                         25
#define sc_x4_control_sc_S_20G_X2                          26
#define sc_x4_control_sc_S_20G_HG2_CR2                     28
#define sc_x4_control_sc_S_20G_HG2_KR2                     29
#define sc_x4_control_sc_S_20G_HG2_X2                      30
#define sc_x4_control_sc_S_40G_CR2                         32
#define sc_x4_control_sc_S_40G_KR2                         33
#define sc_x4_control_sc_S_40G_X2                          34
#define sc_x4_control_sc_S_40G_HG2_CR2                     36
#define sc_x4_control_sc_S_40G_HG2_KR2                     37
#define sc_x4_control_sc_S_40G_HG2_X2                      38
#define sc_x4_control_sc_S_40G_CR4                         40
#define sc_x4_control_sc_S_40G_KR4                         41
#define sc_x4_control_sc_S_40G_X4                          42
#define sc_x4_control_sc_S_40G_HG2_CR4                     44
#define sc_x4_control_sc_S_40G_HG2_KR4                     45
#define sc_x4_control_sc_S_40G_HG2_X4                      46
#define sc_x4_control_sc_S_50G_CR2                         48
#define sc_x4_control_sc_S_50G_KR2                         49
#define sc_x4_control_sc_S_50G_X2                          50
#define sc_x4_control_sc_S_50G_HG2_CR2                     52
#define sc_x4_control_sc_S_50G_HG2_KR2                     53
#define sc_x4_control_sc_S_50G_HG2_X2                      54
#define sc_x4_control_sc_S_50G_CR4                         56
#define sc_x4_control_sc_S_50G_KR4                         57
#define sc_x4_control_sc_S_50G_X4                          58
#define sc_x4_control_sc_S_50G_HG2_CR4                     60
#define sc_x4_control_sc_S_50G_HG2_KR4                     61
#define sc_x4_control_sc_S_50G_HG2_X4                      62
#define sc_x4_control_sc_S_100G_CR4                        64
#define sc_x4_control_sc_S_100G_KR4                        65
#define sc_x4_control_sc_S_100G_X4                         66
#define sc_x4_control_sc_S_100G_HG2_CR4                    68
#define sc_x4_control_sc_S_100G_HG2_KR4                    69
#define sc_x4_control_sc_S_100G_HG2_X4                     70
#define sc_x4_control_sc_S_CL73_20GVCO                     72
#define sc_x4_control_sc_S_CL73_25GVCO                     80
#define sc_x4_control_sc_S_CL36_20GVCO                     88
#define sc_x4_control_sc_S_CL36_25GVCO                     96
#define sc_x4_control_sc_S_2P5G_KX1                        98
#define sc_x4_control_sc_S_5G_KR1                          99

/****************************************************************************
 * Enums: main0_refClkSelect
 */
#define main0_refClkSelect_clk_25MHz                       0
#define main0_refClkSelect_clk_100MHz                      1
#define main0_refClkSelect_clk_125MHz                      2
#define main0_refClkSelect_clk_156p25MHz                   3
#define main0_refClkSelect_clk_187p5MHz                    4
#define main0_refClkSelect_clk_161p25Mhz                   5
#define main0_refClkSelect_clk_50Mhz                       6
#define main0_refClkSelect_clk_106p25Mhz                   7



/**
\struct sc_pmd_dpll_entry_t 

This embodies all parameters passed from PCS to PMD. For more details
please refer to the micro-arch document.
*/
typedef struct sc_pmd_dpll_entry_t
{
  int pma_os_mode;
  uint16_t pll_mode;
} sc_pmd_dpll_entry_st;

extern const sc_pmd_dpll_entry_st sc_pmd_dpll_entry[], sc_pmd_dpll_entry_125M_ref[];

extern int print_tefmod_gen3_sc_lkup_table(PHYMOD_ST* pc);




#endif
