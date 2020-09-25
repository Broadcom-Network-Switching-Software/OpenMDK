/*----------------------------------------------------------------------
SDK This software is governed by the Broadcom Switch APIs license.
SDK This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
SDK 
SDK Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 *  Broadcom Corporation
 *  Proprietary and Confidential information
 *  All rights reserved
 *  This source file is the property of Broadcom Corporation, and
 *  may not be copied or distributed in any isomorphic form without the
 *  prior written consent of Broadcom Corporation.
 *----------------------------------------------------------------------
 *  Description: define enumerators  
 *----------------------------------------------------------------------*/
/*
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
 */
#ifndef _QMOD_DEVICE_H_
#define _QMOD_DEVICE_H_

/* debug mask are used by TEMOD */
#define QMOD_DBG_CL72       (1L << 16) /* CL72 */
#define QMOD_DBG_FWL        (1L << 15) /* FW loading debug */
#define QMOD_DBG_REGACC     (1L << 14) /* Print all register accesses */
#define QMOD_DBG_CFG        (1L << 13) /* CFG */
#define QMOD_DBG_LNK        (1L << 12) /* Link */
#define QMOD_DBG_SPD        (1L << 11) /* Speed */
#define QMOD_DBG_AN         (1L << 10) /* AN */
#define QMOD_DBG_LPK        (1L << 9) /* Local or remote loopback */
#define QMOD_DBG_PMD        (1L << 8) /* PMD */
#define QMOD_DBG_SCN        (1L << 7) /* Link scan*/
#define QMOD_DBG_TST        (1L << 6) /* Testing and PRBS */
#define QMOD_DBG_TOP        (1L << 5) /* Lane swap and polarity */
#define QMOD_DBG_MEM        (1L << 4) /* allocation/object */

#define QMOD_DBG_FUNCVALOUT (1L << 2) /* All values returned by Tier1*/
#define QMOD_DBG_FUNCVALIN  (1L << 1) /* All values pumped into Tier1*/
#define QMOD_DBG_FUNC       (1L << 0) /* Every time we enter a  Tier1*/

typedef struct qmod_device_aux_modes_s {
    uint32_t core_id ; 
    uint16_t st_current_entry ;
    uint16_t st_hcd[4] ;
    uint16_t st_pll_div[4] ;
    uint16_t st_os[4] ;

    uint16_t hto_enable[4]  ;
    uint16_t hto_pll_div[4] ;
    uint16_t hto_os[4] ;
} qmod_device_aux_modes_t ;

#endif
