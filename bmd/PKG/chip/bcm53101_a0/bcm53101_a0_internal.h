/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef __BCM53101_A0_INTERNAL_H__
#define __BCM53101_A0_INTERNAL_H__

#define SPDSTS_SPEED_10         0x0
#define SPDSTS_SPEED_100        0x1
#define SPDSTS_SPEED_1000       0x2

#define ROBO_BRCM_HDR_SIZE      4
#define ROBO_DEFAULT_BRCMID     0x8874

#define GMII_MODE_RGMII         0x5

extern int
bcm53101_a0_arl_write(int unit, int port, int vlan, const bmd_mac_addr_t *mac_addr);

#endif /* __BCM53101_A0_INTERNAL_H__ */
