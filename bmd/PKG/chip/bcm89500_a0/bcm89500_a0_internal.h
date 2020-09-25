/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef __BCM89500_A0_INTERNAL_H__
#define __BCM89500_A0_INTERNAL_H__

#define SPDSTS_SPEED_10         0x0
#define SPDSTS_SPEED_100        0x1
#define SPDSTS_SPEED_1000       0x2

#define ROBO_BRCM_HDR_SIZE      4
#define ROBO_DEFAULT_BRCMID     0x8874

/* Port bits for BRCM_HDR_CTRLr register */
#define BRCM_HDR_IMP0           0x1
#define BRCM_HDR_IMP1           0x2
#define BRCM_HDR_ARM            0x4

extern int
bcm89500_a0_arl_write(int unit, int port, int vlan, const bmd_mac_addr_t *mac_addr);

extern int
bcm89500_a0_max_port(int unit);

#endif /* __BCM89500_A0_INTERNAL_H__ */
