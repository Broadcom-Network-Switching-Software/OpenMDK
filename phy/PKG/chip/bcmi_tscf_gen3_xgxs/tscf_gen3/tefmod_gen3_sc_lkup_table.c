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
 *  Description:  speed table  
 *----------------------------------------------------------------------*/
#define _SDK_TEFMOD_GEN3_ 1
 
#ifdef _SDK_TEFMOD_GEN3_
#include "tefmod_gen3_enum_defines.h"
#include "tefmod_gen3.h"
#include <phymod/phymod.h>
#endif

#ifdef _SDK_TEFMOD_GEN3_
#define PHYMOD_ST const phymod_access_t
#else
#define PHYMOD_ST tefmod_st
#endif

#include "tefmod_gen3_sc_lkup_table.h"

const sc_pmd_dpll_entry_st sc_pmd_dpll_entry[] = {
  /*`SPEED_10G_CR1: 0*/             { OS_MODE_2, phymod_TSCF_PLL_DIV132},
  /*`SPEED_10G_KR1: 1*/             { OS_MODE_2, phymod_TSCF_PLL_DIV132},
  /*`SPEED_10G_X1: 2*/              { OS_MODE_2, phymod_TSCF_PLL_DIV132},
  /*`NULL ENTRY: 3*/                {         0, 0},
  /*`SPEED_10G_HG2_CR1: 4*/         { OS_MODE_2, phymod_TSCF_PLL_DIV140},
  /*`SPEED_10G_HG2_KR1: 5*/         { OS_MODE_2, phymod_TSCF_PLL_DIV140},
  /*`SPEED_10G_HG2_X1: 6*/          { OS_MODE_2, phymod_TSCF_PLL_DIV140},
  /*`NULL ENTRY: 7*/                {         0, 0},
  /*`SPEED_20G_CR1: 8*/             { OS_MODE_1, phymod_TSCF_PLL_DIV132},
  /*`SPEED_20G_KR1: 9 */            { OS_MODE_1, phymod_TSCF_PLL_DIV132},
  /*`SPEED_20G_X1: 10*/             { OS_MODE_1, phymod_TSCF_PLL_DIV132},
  /*`NULL ENTRY: 11*/               {         0, 0},
  /*`SPEED_20G_HG2_CR1: 12*/        { OS_MODE_1, phymod_TSCF_PLL_DIV140},
  /*`SPEED_20G_HG2_KR1: 13*/        { OS_MODE_1, phymod_TSCF_PLL_DIV140},
  /*`SPEED_20G_HG2_X1: 14*/         { OS_MODE_1, phymod_TSCF_PLL_DIV140},
  /*`NULL ENTRY: 15*/               {         0, 0},
  /*`SPEED_25G_CR1: 16*/            { OS_MODE_1, phymod_TSCF_PLL_DIV165},
  /*`SPEED_25G_KR1:  17*/           { OS_MODE_1, phymod_TSCF_PLL_DIV165},
  /*`SPEED_25G_X1: 18*/             { OS_MODE_1, phymod_TSCF_PLL_DIV165},
  /*`NULL ENTRY: 19*/               {         0, 0},
  /*`SPEED_25G_HG2_CR1: 20*/        { OS_MODE_1, phymod_TSCF_PLL_DIV175},
  /*`SPEED_25G_HG2_KR1: 21*/        { OS_MODE_1, phymod_TSCF_PLL_DIV175},
  /*`SPEED_25G_HG2_X1: 22*/         { OS_MODE_1, phymod_TSCF_PLL_DIV175},
  /*`NULL ENTRY: 23*/               {         0, 0},
  /*`SPEED_20G_CR2:  24*/           { OS_MODE_2, phymod_TSCF_PLL_DIV132},
  /*`SPEED_20G_KR2: 25*/            { OS_MODE_2, phymod_TSCF_PLL_DIV132},
  /*`SPEED_20G_X2: 26*/             { OS_MODE_2, phymod_TSCF_PLL_DIV132},
  /*`NULL ENTRY: 27*/               {         0, 0},
  /*`SPEED_20G_HG2_CR2: 28*/        { OS_MODE_2, phymod_TSCF_PLL_DIV140},
  /*`SPEED_20G_HG2_KR2: 29*/        { OS_MODE_2, phymod_TSCF_PLL_DIV140},
  /*`SPEED_20G_HG2_X2: 30*/         { OS_MODE_2, phymod_TSCF_PLL_DIV140},
  /*`NULL ENTRY: 31*/               {         0, 0},
  /*`SPEED_40G_CR2: 32*/            { OS_MODE_1, phymod_TSCF_PLL_DIV132},
  /*`SPEED_40G_KR2: 33*/            { OS_MODE_1, phymod_TSCF_PLL_DIV132},
  /*`SPEED_40G_X2: 34*/             { OS_MODE_1, phymod_TSCF_PLL_DIV132},
  /*`NULL ENTRY: 35*/               {         0, 0},
  /*`SPEED_40G_HG2_CR2: 36*/        { OS_MODE_1, phymod_TSCF_PLL_DIV140},
  /*`SPEED_40G_HG2_KR2: 37*/        { OS_MODE_1, phymod_TSCF_PLL_DIV140},
  /*`SPEED_40G_HG2_X2: 38*/         { OS_MODE_1, phymod_TSCF_PLL_DIV140},
  /*`NULL ENTRY: 39*/               {         0, 0},
  /*`SPEED_40G_CR4:  40*/           { OS_MODE_2, phymod_TSCF_PLL_DIV132},
  /*`SPEED_40G_KR4:  41*/           { OS_MODE_2, phymod_TSCF_PLL_DIV132},
  /*`SPEED_40G_X4: 42*/             { OS_MODE_2, phymod_TSCF_PLL_DIV132},
  /*`NULL ENTRY: 43*/               {         0, 0},
  /*`SPEED_40G_HG2_CR4: 44*/        { OS_MODE_2, phymod_TSCF_PLL_DIV140},
  /*`SPEED_40G_HG2_KR4: 45*/        { OS_MODE_2, phymod_TSCF_PLL_DIV140},
  /*`SPEED_40G_HG2_X4: 46*/         { OS_MODE_2, phymod_TSCF_PLL_DIV140},
  /*`NULL ENTRY: 47*/               {         0, 0},
  /*`SPEED_50G_CR2: 48*/            { OS_MODE_1, phymod_TSCF_PLL_DIV165},
  /*`SPEED_50G_KR2: 49*/            { OS_MODE_1, phymod_TSCF_PLL_DIV165},
  /*`SPEED_50G_X2: 50*/             { OS_MODE_1, phymod_TSCF_PLL_DIV165},
  /*`NULL ENTRY: 51*/               {         0, 0},
  /*`SPEED_50G_HG2_CR2: 52*/        { OS_MODE_1, phymod_TSCF_PLL_DIV175},
  /*`SPEED_50G_HG2_KR2: 53*/        { OS_MODE_1, phymod_TSCF_PLL_DIV175},
  /*`SPEED_50G_HG2_X2: 54*/         { OS_MODE_1, phymod_TSCF_PLL_DIV175},
  /*`NULL ENTRY: 55*/               {         0, 0},
  /*`SPEED_50G_CR4: 56*/            { OS_MODE_2, phymod_TSCF_PLL_DIV165},
  /*`SPEED_50G_KR4:  57*/           { OS_MODE_2, phymod_TSCF_PLL_DIV165},
  /*`SPEED_50G_X4: 58*/             { OS_MODE_2, phymod_TSCF_PLL_DIV165},
  /*`NULL ENTRY: 59*/               {         0, 0},
  /*`SPEED_50G_HG2_CR4: 60*/        { OS_MODE_2, phymod_TSCF_PLL_DIV175},
  /*`SPEED_50G_HG2_KR4: 61*/        { OS_MODE_2, phymod_TSCF_PLL_DIV175},
  /*`SPEED_50G_HG2_X4: 62*/         { OS_MODE_2, phymod_TSCF_PLL_DIV175},
  /*`NULL ENTRY: 63*/               {         0, 0},
  /*`SPEED_100G_CR4: 64*/           { OS_MODE_1, phymod_TSCF_PLL_DIV165},
  /*`SPEED_100G_KR4: 65*/           { OS_MODE_1, phymod_TSCF_PLL_DIV165},
  /*`SPEED_100G_X4: 66*/            { OS_MODE_1, phymod_TSCF_PLL_DIV165},
  /*`NULL ENTRY: 67*/               {         0, 0},
  /*`SPEED_100G_HG2_CR4: 68*/       { OS_MODE_1, phymod_TSCF_PLL_DIV175},
  /*`SPEED_100G_HG2_KR4: 69*/       { OS_MODE_1, phymod_TSCF_PLL_DIV175},
  /*`SPEED_100G_HG2_X4: 70*/        { OS_MODE_1, phymod_TSCF_PLL_DIV175},
  /*`NULL ENTRY: 71*/               {         0, 0},
  /*`SPEED_CL73_20GVCO: 72*/        { OS_MODE_16p5, phymod_TSCF_PLL_DIV140},
  /*`NULL ENTRY: 73*/               {         0, 0},
  /*`NULL ENTRY: 74*/               {         0, 0},
  /*`NULL ENTRY: 75*/               {         0, 0},
  /*`NULL ENTRY: 76*/               {         0, 0},
  /*`NULL ENTRY: 77*/               {         0, 0},
  /*`NULL ENTRY: 78*/               {         0, 0},
  /*`NULL ENTRY: 79*/               {         0, 0},
  /*`SPEED_CL73_25GVCO: 80*/        { OS_MODE_20p625, phymod_TSCF_PLL_DIV140},
  /*`NULL ENTRY: 81*/               {         0, 0},
  /*`NULL ENTRY: 82*/               {         0, 0},
  /*`NULL ENTRY: 83*/               {         0, 0},
  /*`NULL ENTRY: 84*/               {         0, 0},
  /*`NULL ENTRY: 84*/               {         0, 0},
  /*`NULL ENTRY: 86*/               {         0, 0},
  /*`NULL ENTRY: 87*/               {         0, 0},
  /*`SPEED_1G_20GVCO: 88*/          { OS_MODE_16p5, phymod_TSCF_PLL_DIV132},
  /*`NULL ENTRY: 89*/               {         0, 0},
  /*`NULL ENTRY: 90*/               {         0, 0},
  /*`NULL ENTRY: 91*/               {         0, 0},
  /*`NULL ENTRY: 92*/               {         0, 0},
  /*`NULL ENTRY: 93*/               {         0, 0},
  /*`NULL ENTRY: 94*/               {         0, 0},
  /*`NULL ENTRY: 95*/               {         0, 0},
  /*`SPEED_1G_25GVCO: 96*/          { OS_MODE_20p625, phymod_TSCF_PLL_DIV165},
  /*`NULL ENTRY: 97*/               {         0, 0},
  /*`SPEED_2P5G_KX1: 98*/           { OS_MODE_8p25, phymod_TSCF_PLL_DIV165},
  /*`SPEED_5G_KR1: 99*/             { OS_MODE_4, phymod_TSCF_PLL_DIV132}
};

const sc_pmd_dpll_entry_st sc_pmd_dpll_entry_125M_ref[] = {
  /*`SPEED_10G_CR1: 0*/             { OS_MODE_2, phymod_TSCF_PLL_DIV165},
  /*`SPEED_10G_KR1: 1*/             { OS_MODE_2, phymod_TSCF_PLL_DIV165},
  /*`SPEED_10G_X1: 2*/              { OS_MODE_2, phymod_TSCF_PLL_DIV165},
  /*`NULL ENTRY: 3*/                {         0, 0},
  /*`SPEED_10G_HG2_CR1: 4*/         { OS_MODE_2, phymod_TSCF_PLL_DIV175},
  /*`SPEED_10G_HG2_KR1: 5*/         { OS_MODE_2, phymod_TSCF_PLL_DIV175},
  /*`SPEED_10G_HG2_X1: 6*/          { OS_MODE_2, phymod_TSCF_PLL_DIV175},
  /*`NULL ENTRY: 7*/                {         0, 0},
  /*`SPEED_20G_CR1: 8*/             { OS_MODE_1, phymod_TSCF_PLL_DIV165},
  /*`SPEED_20G_KR1: 9 */            { OS_MODE_1, phymod_TSCF_PLL_DIV165},
  /*`SPEED_20G_X1: 10*/             { OS_MODE_1, phymod_TSCF_PLL_DIV165},
  /*`NULL ENTRY: 11*/               {         0, 0},
  /*`SPEED_20G_HG2_CR1: 12*/        { OS_MODE_1, phymod_TSCF_PLL_DIV175},
  /*`SPEED_20G_HG2_KR1: 13*/        { OS_MODE_1, phymod_TSCF_PLL_DIV175},
  /*`SPEED_20G_HG2_X1: 14*/         { OS_MODE_1, phymod_TSCF_PLL_DIV175},
  /*`NULL ENTRY: 15*/               {         0, 0},
  /*`SPEED_25G_CR1: 16*/            { OS_MODE_1, phymod_TSCF_PLL_DIV165},
  /*`SPEED_25G_KR1:  17*/           { OS_MODE_1, phymod_TSCF_PLL_DIV165},
  /*`SPEED_25G_X1: 18*/             { OS_MODE_1, phymod_TSCF_PLL_DIV165},
  /*`NULL ENTRY: 19*/               {         0, 0},
  /*`SPEED_25G_HG2_CR1: 20*/        { OS_MODE_1, phymod_TSCF_PLL_DIV175},
  /*`SPEED_25G_HG2_KR1: 21*/        { OS_MODE_1, phymod_TSCF_PLL_DIV175},
  /*`SPEED_25G_HG2_X1: 22*/         { OS_MODE_1, phymod_TSCF_PLL_DIV175},
  /*`NULL ENTRY: 23*/               {         0, 0},
  /*`SPEED_20G_CR2:  24*/           { OS_MODE_2, phymod_TSCF_PLL_DIV165},
  /*`SPEED_20G_KR2: 25*/            { OS_MODE_2, phymod_TSCF_PLL_DIV165},
  /*`SPEED_20G_X2: 26*/             { OS_MODE_2, phymod_TSCF_PLL_DIV165},
  /*`NULL ENTRY: 27*/               {         0, 0},
  /*`SPEED_20G_HG2_CR2: 28*/        { OS_MODE_2, phymod_TSCF_PLL_DIV175},
  /*`SPEED_20G_HG2_KR2: 29*/        { OS_MODE_2, phymod_TSCF_PLL_DIV175},
  /*`SPEED_20G_HG2_X2: 30*/         { OS_MODE_2, phymod_TSCF_PLL_DIV175},
  /*`NULL ENTRY: 31*/               {         0, 0},
  /*`SPEED_40G_CR2: 32*/            { OS_MODE_1, phymod_TSCF_PLL_DIV165},
  /*`SPEED_40G_KR2: 33*/            { OS_MODE_1, phymod_TSCF_PLL_DIV165},
  /*`SPEED_40G_X2: 34*/             { OS_MODE_1, phymod_TSCF_PLL_DIV165},
  /*`NULL ENTRY: 35*/               {         0, 0},
  /*`SPEED_40G_HG2_CR2: 36*/        { OS_MODE_1, phymod_TSCF_PLL_DIV175},
  /*`SPEED_40G_HG2_KR2: 37*/        { OS_MODE_1, phymod_TSCF_PLL_DIV175},
  /*`SPEED_40G_HG2_X2: 38*/         { OS_MODE_1, phymod_TSCF_PLL_DIV175},
  /*`NULL ENTRY: 39*/               {         0, 0},
  /*`SPEED_40G_CR4:  40*/           { OS_MODE_2, phymod_TSCF_PLL_DIV165},
  /*`SPEED_40G_KR4:  41*/           { OS_MODE_2, phymod_TSCF_PLL_DIV165},
  /*`SPEED_40G_X4: 42*/             { OS_MODE_2, phymod_TSCF_PLL_DIV165},
  /*`NULL ENTRY: 43*/               {         0, 0},
  /*`SPEED_40G_HG2_CR4: 44*/        { OS_MODE_2, phymod_TSCF_PLL_DIV175},
  /*`SPEED_40G_HG2_KR4: 45*/        { OS_MODE_2, phymod_TSCF_PLL_DIV175},
  /*`SPEED_40G_HG2_X4: 46*/         { OS_MODE_2, phymod_TSCF_PLL_DIV175},
  /*`NULL ENTRY: 47*/               {         0, 0},
  /*`SPEED_50G_CR2: 48*/            { OS_MODE_1, phymod_TSCF_PLL_DIV165},
  /*`SPEED_50G_KR2: 49*/            { OS_MODE_1, phymod_TSCF_PLL_DIV165},
  /*`SPEED_50G_X2: 50*/             { OS_MODE_1, phymod_TSCF_PLL_DIV165},
  /*`NULL ENTRY: 51*/               {         0, 0},
  /*`SPEED_50G_HG2_CR2: 52*/        { OS_MODE_1, phymod_TSCF_PLL_DIV175},
  /*`SPEED_50G_HG2_KR2: 53*/        { OS_MODE_1, phymod_TSCF_PLL_DIV175},
  /*`SPEED_50G_HG2_X2: 54*/         { OS_MODE_1, phymod_TSCF_PLL_DIV175},
  /*`NULL ENTRY: 55*/               {         0, 0},
  /*`SPEED_50G_CR4: 56*/            { OS_MODE_2, phymod_TSCF_PLL_DIV165},
  /*`SPEED_50G_KR4:  57*/           { OS_MODE_2, phymod_TSCF_PLL_DIV165},
  /*`SPEED_50G_X4: 58*/             { OS_MODE_2, phymod_TSCF_PLL_DIV165},
  /*`NULL ENTRY: 59*/               {         0, 0},
  /*`SPEED_50G_HG2_CR4: 60*/        { OS_MODE_2, phymod_TSCF_PLL_DIV165},
  /*`SPEED_50G_HG2_KR4: 61*/        { OS_MODE_2, phymod_TSCF_PLL_DIV165},
  /*`SPEED_50G_HG2_X4: 62*/         { OS_MODE_2, phymod_TSCF_PLL_DIV165},
  /*`NULL ENTRY: 63*/               {         0, 0},
  /*`SPEED_100G_CR4: 64*/           { OS_MODE_1, phymod_TSCF_PLL_DIV165},
  /*`SPEED_100G_KR4: 65*/           { OS_MODE_1, phymod_TSCF_PLL_DIV165},
  /*`SPEED_100G_X4: 66*/            { OS_MODE_1, phymod_TSCF_PLL_DIV165},
  /*`NULL ENTRY: 67*/               {         0, 0},
  /*`SPEED_100G_HG2_CR4: 68*/       { OS_MODE_1, phymod_TSCF_PLL_DIV175},
  /*`SPEED_100G_HG2_KR4: 69*/       { OS_MODE_1, phymod_TSCF_PLL_DIV175},
  /*`SPEED_100G_HG2_X4: 70*/        { OS_MODE_1, phymod_TSCF_PLL_DIV175},
  /*`NULL ENTRY: 71*/               {         0, 0},
  /*`SPEED_CL73_20GVCO: 72*/        { OS_MODE_16p5, phymod_TSCF_PLL_DIV175},
  /*`NULL ENTRY: 73*/               {         0, 0},
  /*`NULL ENTRY: 74*/               {         0, 0},
  /*`NULL ENTRY: 75*/               {         0, 0},
  /*`NULL ENTRY: 76*/               {         0, 0},
  /*`NULL ENTRY: 77*/               {         0, 0},
  /*`NULL ENTRY: 78*/               {         0, 0},
  /*`NULL ENTRY: 79*/               {         0, 0},
  /*`SPEED_CL73_25GVCO: 80*/        { OS_MODE_20p625, phymod_TSCF_PLL_DIV175},
  /*`NULL ENTRY: 81*/               {         0, 0},
  /*`NULL ENTRY: 82*/               {         0, 0},
  /*`NULL ENTRY: 83*/               {         0, 0},
  /*`NULL ENTRY: 84*/               {         0, 0},
  /*`NULL ENTRY: 84*/               {         0, 0},
  /*`NULL ENTRY: 86*/               {         0, 0},
  /*`NULL ENTRY: 87*/               {         0, 0},
  /*`SPEED_1G_20GVCO: 88*/          { OS_MODE_16p5, phymod_TSCF_PLL_DIV165},
  /*`NULL ENTRY: 89*/               {         0, 0},
  /*`NULL ENTRY: 90*/               {         0, 0},
  /*`NULL ENTRY: 91*/               {         0, 0},
  /*`NULL ENTRY: 92*/               {         0, 0},
  /*`NULL ENTRY: 93*/               {         0, 0},
  /*`NULL ENTRY: 94*/               {         0, 0},
  /*`NULL ENTRY: 95*/               {         0, 0},
  /*`SPEED_1G_25GVCO: 96*/          { OS_MODE_20p625, phymod_TSCF_PLL_DIV165}
};


int tefmod_gen3_get_mapped_speed(tefmod_gen3_spd_intfc_type_t spd_intf, int *speed) 
{
  /* speeds covered in Forced-speed */
  if(spd_intf == TEFMOD_SPD_10000_XFI)           *speed = sc_x4_control_sc_S_10G_X1;
  if(spd_intf == TEFMOD_SPD_10600_XFI_HG)        *speed = sc_x4_control_sc_S_10G_HG2_X1;
  if(spd_intf == TEFMOD_SPD_20000_XFI)           *speed = sc_x4_control_sc_S_20G_X1;
  if(spd_intf == TEFMOD_SPD_21200_XFI_HG)        *speed = sc_x4_control_sc_S_20G_HG2_X1;
  if(spd_intf == TEFMOD_SPD_25000_XFI)           *speed = sc_x4_control_sc_S_25G_X1;
  if(spd_intf == TEFMOD_SPD_26500_XFI_HG)        *speed = sc_x4_control_sc_S_25G_HG2_X1;
  if(spd_intf == TEFMOD_SPD_20G_MLD_X2)          *speed = sc_x4_control_sc_S_20G_X2;
  if(spd_intf == TEFMOD_SPD_21G_MLD_HG_X2)       *speed = sc_x4_control_sc_S_20G_HG2_X2;
  if(spd_intf == TEFMOD_SPD_40G_MLD_X2)          *speed = sc_x4_control_sc_S_40G_X2;
  if(spd_intf == TEFMOD_SPD_42G_MLD_HG_X2)       *speed = sc_x4_control_sc_S_40G_HG2_X2;
  if(spd_intf == TEFMOD_SPD_40G_MLD_X4)          *speed = sc_x4_control_sc_S_40G_X4;
  if(spd_intf == TEFMOD_SPD_42G_MLD_HG_X4)       *speed = sc_x4_control_sc_S_40G_HG2_X4;
  if(spd_intf == TEFMOD_SPD_50G_MLD_X2)          *speed = sc_x4_control_sc_S_50G_X2;
  if(spd_intf == TEFMOD_SPD_53G_MLD_HG_X2)       *speed = sc_x4_control_sc_S_50G_HG2_X2;
  if(spd_intf == TEFMOD_SPD_50G_MLD_X4)          *speed = sc_x4_control_sc_S_50G_X4;
  if(spd_intf == TEFMOD_SPD_53G_MLD_HG_X4)       *speed = sc_x4_control_sc_S_50G_HG2_X4;
  if(spd_intf == TEFMOD_SPD_100G_MLD_X4)         *speed = sc_x4_control_sc_S_100G_X4;
  if(spd_intf == TEFMOD_SPD_106G_MLD_HG_X4)      *speed = sc_x4_control_sc_S_100G_HG2_X4;
  /*  These are the speeds that are not in forced mode or AN mode */
  if(spd_intf == TEFMOD_SPD_10000_XFI_CR1)       *speed = sc_x4_control_sc_S_10G_CR1;
  if(spd_intf == TEFMOD_SPD_10600_XFI_HG_CR1)    *speed = sc_x4_control_sc_S_10G_HG2_CR1;
  if(spd_intf == TEFMOD_SPD_CL73_20G)            *speed = sc_x4_control_sc_S_CL73_20GVCO;
  if(spd_intf == TEFMOD_SPD_CL73_25G)            *speed = sc_x4_control_sc_S_CL73_25GVCO;
  if(spd_intf == TEFMOD_SPD_1G_20G)              *speed = sc_x4_control_sc_S_CL36_20GVCO;
  if(spd_intf == TEFMOD_SPD_1G_25G)              *speed = sc_x4_control_sc_S_CL36_25GVCO;
  if(spd_intf == TEFMOD_SPD_2P5G_KX1)            *speed = sc_x4_control_sc_S_2P5G_KX1;
  if(spd_intf == TEFMOD_SPD_5G_KR1)              *speed = sc_x4_control_sc_S_5G_KR1;


  return PHYMOD_E_NONE;
}
