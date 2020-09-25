/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 * All Rights Reserved.$
 *
 * TDM soc header for BCM56860
 */
#ifndef __BCM56860_A0_TDM_CHIP_SOC_H__
#define __BCM56860_A0_TDM_CHIP_SOC_H__

#ifndef TDM_BCM56860_PREPROCESSOR_SOC_DEFINES_H
#define TDM_BCM56860_PREPROCESSOR_SOC_DEFINES_H

#include "bcm56860_a0_tdm_chip_defines.h"

typedef struct {
    int cur_idx;
    int pgw_tdm_idx;
    int ovs_tdm_idx;
    int tdm_stk_idx;
} td2p_pgw_pntrs_t;

typedef struct {
    int subport;
    int cur_idx_max;
    int first_port;
    int last_port;
    int swap_array[TD2P_OS_LLS_GRP_LEN];
} td2p_pgw_scheduler_vars_t;


#endif /* TDM_BCM56860_PREPROCESSOR_SOC_DEFINES_H */

#endif /* __BCM56860_A0_TDM_CHIP_SOC_H__ */
