/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * TSC firmware download support.
 */

#include <phy/phy.h>

#ifndef __BCMI_TSC_XGXS_FIRMWARE_H__
#define __BCMI_TSC_XGXS_FIRMWARE_H__

/* Download function */
extern int
bcmi_tsc_xgxs_firmware_set(phy_ctrl_t *pc, uint32_t offset,
                                 uint32_t fw_size, uint8_t *fw_data);

/* Check downloaded firmware */
int
bcmi_tsc_xgxs_firmware_check(phy_ctrl_t *pc);

/* uController's firmware */
extern uint8_t tsc_ucode_bin[];
extern unsigned int tsc_ucode_bin_len;

#endif /* __BCMI_TSC_XGXS_FIRMWARE_H__ */
