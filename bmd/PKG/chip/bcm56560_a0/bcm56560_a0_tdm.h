/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef __BCM56560_A0_TDM_H__
#define __BCM56560_A0_TDM_H__

#include <cdk/cdk_types.h>
#include "bcm56560_a0_tdm_core_defines.h"
#include "bcm56560_a0_tdm_core_soc.h"
#include "bcm56560_a0_tdm_core_top.h"

#define bcm56560_a0_sel_tdm             SOC_SEL_TDM
#define bcm56560_a0_set_tdm_tbl         _soc_set_tdm_tbl
#define bcm56560_a0_set_iarb_tdm_table  tdm_ap_set_iarb_tdm

#endif /* __BCM56560_A0_TDM_H__ */
