/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 * All Rights Reserved.$
 *
 * TDM soc header for BCM56560
 */
#ifndef __BCM56560_B0_TDM_CHIP_SOC_H__
#define __BCM56560_B0_TDM_CHIP_SOC_H__

#ifndef TDM_BCM56560_PREPROCESSOR_SOC_DEFINES_H
#define TDM_BCM56560_PREPROCESSOR_SOC_DEFINES_H

#include "bcm56560_b0_tdm_chip_defines.h"

typedef struct {
	int cur_idx;
	int pgw_tdm_b0_idx;
	int ovs_tdm_b0_idx;
	int tdm_b0_stk_idx;
} ap_pgw_pntrs_t;

typedef struct {
	int subport;
	int cur_idx_max;
	int first_port;
	int last_port;
	int swap_array[AP_OS_LLS_GRP_LEN];
} ap_pgw_scheduler_vars_t;


#endif /* TDM_BCM56560_PREPROCESSOR_SOC_DEFINES_H */

#endif /* __BCM56560_B0_TDM_CHIP_SOC_H__ */
