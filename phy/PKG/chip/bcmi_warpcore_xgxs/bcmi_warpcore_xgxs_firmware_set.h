/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Warpcore firmware download support.
 */

#include <phy/phy.h>

#ifndef __BCMI_WARPCORE_XGXS_FIRMWARE_H__
#define __BCMI_WARPCORE_XGXS_FIRMWARE_H__

/* Download function */
extern int
bcmi_warpcore_xgxs_firmware_set(phy_ctrl_t *pc, uint32_t offset,
                                uint32_t fw_size, uint8_t *fw_data);

/* Check downloaded firmware */
int
bcmi_warpcore_xgxs_firmware_check(phy_ctrl_t *pc);

/* uController's firmware */
extern uint8_t wc40_ucode_bin[];
extern unsigned int wc40_ucode_bin_len;
extern uint8_t wc40_ucode_b0_bin[];
extern unsigned int wc40_ucode_b0_bin_len;

#endif /* __BCMI_WARPCORE_XGXS_FIRMWARE_H__ */
