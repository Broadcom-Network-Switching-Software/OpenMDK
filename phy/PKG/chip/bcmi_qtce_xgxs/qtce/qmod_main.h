/*----------------------------------------------------------------------
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 *  Broadcom Corporation
 *  Proprietary and Confidential information
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
 *  This source file is the property of Broadcom Corporation, and
 *  may not be copied or distributed in any isomorphic form without the
 *  prior written consent of Broadcom Corporation.
 *----------------------------------------------------------------------
 *  Description: Main header file for TEMod.
 *---------------------------------------------------------------------*/

#ifndef _QMOD_MAIN_H_
#define _QMOD_MAIN_H_

#ifndef STATIC
#define STATIC static
#endif  /* STATIC */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "errno.h"

#include "qmod_defines.h"

#ifndef STANDALONE
  #define printf io_printf
#endif

#ifdef CADENCE
#include <veriuser.h>
#endif /* CADENCE */

#ifdef SYNOPSYS
#include <vcsuser.h>
#endif /* SYNOPSYS */

typedef unsigned char           uint8_t;          /* 8-bit quantity  */
typedef unsigned short          uint16_t;         /* 16-bit quantity */
typedef unsigned int            uint32_t;         /* 32-bit quantity */

typedef uint32_t sal_usecs_t;     /* from "sal/core/time.h" */
typedef int     soc_port_t;     /* from "soc/types.h" */

#define SOC_MAX_NUM_PORTS       72 
#define NUM_PORTS               SOC_MAX_NUM_PORTS

#define SOC_E_MULT_REG        1    /*! not error, but multiple regs. */
#define SOC_E_FUNC_NOT_FOUND  3    /*! tier1 not found. Continue search */
#define PHYMOD_E_NONE        0        /*! No errors */
#define SOC_E_UNAVAIL    -1        /*! register or resource not there */
#define SOC_E_FAIL       -2        /*! catchall completion failure */
#define SOC_E_TIMEOUT    -3        /*! register access timeout */
#define PHYMOD_E_FAIL      -4        /*! catchall failure */
#define SOC_E_FUNC_ERROR -5        /*! function found. But did not execute well. */
#define SOC_E_PARAM      -6        

#define INT_PHY_SW_STATE(unit, port)  (int_phy_ctrl[unit][port])

#define TE_REG_READ(_unit, _pc, _flags, _reg_addr, _val) \
        qmod_reg_aer_read((_unit), (_pc),  (_reg_addr), (_val))

#define TE_REG_WRITE(_unit, _pc, _flags, _reg_addr, _val) \
        qmod_reg_aer_write((_unit), (_pc),  (_reg_addr), (_val))

#define TE_REG_MODIFY(_unit, _pc, _flags, _reg_addr, _val, _mask) \
        qmod_reg_aer_modify((_unit), (_pc),  (_reg_addr), (_val),(_mask))

#define TE_UDELAY(_time) sal_udelay(_time)

#define WC40_REG_READ(_unit, _pc, _flags, _reg_addr, _val) \
        wcmod_reg_aer_read((_unit), (_pc),  (_reg_addr), (_val))

#define WC40_REG_WRITE(_unit, _pc, _flags, _reg_addr, _val) \
        wcmod_reg_aer_write((_unit), (_pc),  (_reg_addr), (_val))

#define WC40_REG_MODIFY(_unit, _pc, _flags, _reg_addr, _val, _mask) \
        wcmod_reg_aer_modify((_unit), (_pc),  (_reg_addr), (_val),(_mask))

#define WC40_UDELAY(_time) sal_udelay(_time)

#define PHYMOD_DEBUG_ERROR(stuff_) printf stuff_

#define PHYMOD_IF_ERR_RETURN(op) \
    do { int __rv__; if ((__rv__ = (op)) < 0) \
            { PHYMOD_DEBUG_ERROR ((__FILE__ " : " #op " %d\n", __LINE__)); \
                    return(__rv__);} } \
                        while(0)

#define PHYMOD_VDBG(flags_, pa_, stuff_) \
    if (phymod_debug_check(flags_, pa_)) \
            PHYMOD_DEBUG_ERROR (stuff_)

#define PHY_AER_REG_ADDR_AER(_addr)    (((_addr) >> 16) & 0x0000FFFF)
#define PHY_AER_REG_ADDR_BLK(_addr)    (((_addr) & 0x000FFF0))
#define PHY_AER_REG_ADDR_REGAD(_addr)  ((((_addr) & 0x00008000) >> 11) | \
                                        ((_addr) & 0x0000000F))

#define FALSE 0
#define TRUE  1

/* Borrowed from SDK.  This struct controls iterations and time durations
 * while executing a 'wait' or a 'poll'. No doxygen documentation for this.
*/
typedef struct soc_timeout_s
{
    sal_usecs_t		expire;
    sal_usecs_t		usec;
    int			min_polls;
    int			polls;
    int			exp_delay;
} soc_timeout_t;

typedef enum {
    TEMOD_PRBS_POLYNOMIAL_7 = 0,
    TEMOD_PRBS_POLYNOMIAL_9,
    TEMOD_PRBS_POLYNOMIAL_11,
    TEMOD_PRBS_POLYNOMIAL_15,
    TEMOD_PRBS_POLYNOMIAL_23,
    TEMOD_PRBS_POLYNOMIAL_31,
    TEMOD_PRBS_POLYNOMIAL_58,
    TEMOD_PRBS_POLYNOMIAL_TYPE_COUNT 
}prbs_polynomial_type_t;

typedef enum {
    TSCMOD_CL72_HW_ENABLE = 0,
    TSCMOD_CL72_HW_DISABLE,
    TSCMOD_CL72_HW_RESTART,
    TSCMOD_CL72_HW_RESTART_STOP,
    TSCMOD_CL72_AN_FORCED_ENABLE,
    TSCMOD_CL72_AN_FORCED_DISABLE,
    TSCMOD_CL72_AN_NO_FORCED,
    TEMOD_CL72_TYPE_COUNT 
}cl72_type_t;

typedef enum {
    TEMOD_AN_SET_RF_DISABLE = 0,
    TEMOD_AN_SET_SGMII_SPEED,
    TEMOD_AN_SET_SGMII_MASTER,
    TEMOD_AN_SET_HG_MODE,
    TEMOD_AN_SET_FEC_MODE,
    TEMOD_AN_SET_CL72_MODE,
    TEMOD_AN_SET_CL37_ATTR,
    TEMOD_AN_SET_CL48_SYNC,
    TEMOD_AN_SET_LK_FAIL_INHIBIT_TIMER_NO_CL72,
    TEMOD_AN_SET_CL73_FEC_OFF,
    TEMOD_AN_SET_CL73_FEC_ON,
    TEMOD_AN_SET_SPEED_PAUSE,
    TEMOD_AN_SET_TYPE_COUNT
}qmod_an_set_type_t;

#define TE40_XGXSBLK0_XGXSSTATUSr  0x8001
#define TE40_PLL_WAIT              2500
#define TE40_RX_X4_STATUS1_PMA_PMD_LIVE_STATUSr 0xc150
#define DSC1B0_UC_CTRLr 0xc20e

#define NUMBER_PCS_FUNCTION_TABLE_ENTRIES 100

#define TEMOD_ENABLE                 0x1
#define TEMOD_DISABLE                0x0

#define TEMOD_MODEL_TE              0x11 

#define TEMOD_DBG_FLOW               0x1
#define TEMOD_DBG_REG                0x2
#define TEMOD_DBG_MDIO               0x4  
#define TEMOD_DBG_WRCHK              0x8
#define TEMOD_DBG_FUNC              0x10
#define TEMOD_DBG_SUB               0x40
#define TEMOD_DBG_SPD              0x100  
#define TEMOD_DBG_AN               0x400
#define TEMOD_DBG_DSC             0x1000
#define TEMOD_DBG_TXDRV           0x2000
#define TEMOD_DBG_LINK            0x4000
#define TEMOD_DBG_PATH            0x8000
#define TEMOD_DBG_TO             0x10000
#define TEMOD_DBG_INIT           0x40000
#define TEMOD_DBG_CFG           0x100000
#define TEMOD_DBG_PAT           0x400000
#define TEMOD_DBG_UC           0x1000000
#define TEMOD_DBG_SCAN         0x4000000
#define TEMOD_DBG_ERROR        0x8000000

#define USE_CJPAT  1
#define USE_CRPAT  2 

#define TEMOD_DIAG_PRBS_LANE_EN_GET       1
#define TEMOD_DIAG_PRBS_POLYNOMIAL_GET    2
#define TEMOD_DIAG_PRBS_INVERT_DATA_GET   3
#define TEMOD_DIAG_PRBS_MODE_GET          4

#define TEMOD_DIAG_P_HELP                 0
#define TEMOD_DIAG_P_VERB                 1
#define TEMOD_DIAG_P_LINK_UP_BERT         2
#define TEMOD_DIAG_P_RAM_WRITE            3
#define TEMOD_DIAG_P_CTRL_TYPE            4
#define TEMOD_DIAG_P_AN_CTL               5 

#define TEMOD_CTRL_TYPE_DEFAULT   0x378a7000
#define TEMOD_CTRL_TYPE_MASK           0xfff
#define TEMOD_CTRL_TYPE_LB               0x1
#define TEMOD_CTRL_TYPE_HG               0x4 
#define TEMOD_CTRL_TYPE_LINK_BER         0x8 
#define TEMOD_CTRL_TYPE_FW_LOADED       0x10
#define TEMOD_CTRL_TYPE_FAULT_DIS       0x20
#define TEMOD_CTRL_TYPE_FW_AVAIL        0x40
#define TEMOD_CTRL_TYPE_PRBS_M2         0x80
#define TEMOD_CTRL_TYPE_CL72_ON        0x100
#define TEMOD_CTRL_TYPE_UC_STALL       0x200

/*
#define TEMOD_TX_LANE_TRAFFIC            0x10 
#define TEMOD_TX_LANE_RESET              0x20
*/

#define TEMOD_REG_RD    0
#define TEMOD_REG_WR    1
#define TEMOD_REG_MO    2

#define TEMOD_AN_CTL_LINK_FAIL_ZERO_TIMER     0x1 
#define TEMOD_AN_CTL_CL73_ERROR_TIMER         0x2

typedef struct strToPCSFunction {
  char* p;
  int (*fp)(qmod_st*);
} str2PCSFn;


extern str2PCSFn str2PCSFunc;


int qmod_diag(qmod_st *ws, qmod_diag_type diag_type);
extern void init_qmod_st(/* INOUT */qmod_st *ws);
extern void              qmod_init_called(int x);
extern void              qmod_init(void);
extern void              qmod_tier1_selector(qmod_tier1_function_type  tier1_type,qmod_st* c, int *retVal);
extern void              qmod_read_cfg(const char* fileName);
extern void              print_config(int unit, int port);
extern int               qmod_searchPCSFuncTable(const char* selStr, qmod_st* ws); 
extern void              copy_qmod_st(qmod_st *from, qmod_st *to);
extern const char*       getLaneStr(qmod_lane_select ls);
extern qmod_spd_intfc_type qmod_spd_intf_s2e(char* s);
extern qmod_lane_select getLaneSelect(int lane) ;

int qmod_diag(qmod_st *ws, qmod_diag_type diag_type);

/*#include  "qmod_doc.h"*/ 
#endif   /*  _QMOD_MAIN_H_ */
