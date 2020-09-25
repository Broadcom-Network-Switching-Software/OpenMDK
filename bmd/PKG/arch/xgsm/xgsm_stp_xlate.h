/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * XGS DMA definitions.
 */

#ifndef __XGSM_STP_XLATE_H__
#define __XGSM_STP_XLATE_H__

#include <bmd/bmd.h>

extern int 
bmd_xgsm_stp_state_to_hw(bmd_stp_state_t bmd_state, int *hw_state);

extern int 
bmd_xgsm_stp_state_from_hw(int hw_state, bmd_stp_state_t *bmd_state);

#endif /* __XGSM_STP_XLATE_H__ */
