/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef __BMD_PHY_H__
#define __BMD_PHY_H__

#include <bmd_config.h>
#include <cdk/cdk_chip.h>

/*
 * BMD PHY interface.
 *
 * These functions act as glue for the external PHY driver interface
 * in order to reduce impact in case the external PHY interface
 * changes.
 */

#define BMD_PHY_MODE_WAN        1
#define BMD_PHY_MODE_2LANE      2
#define BMD_PHY_MODE_SCRAMBLE   3
#define BMD_PHY_MODE_SERDES     4
#define BMD_PHY_MODE_MULTI_CORE 5
#define BMD_PHY_MODE_PASSTHRU   6
#define BMD_PHY_MODE_DISABLED   7

#define BMD_PHY_M_EEE_OFF       0
#define BMD_PHY_M_EEE_802_3     1
#define BMD_PHY_M_EEE_AUTO      2

#define BMD_PHY_IF_DEFAULT      0
#define BMD_PHY_IF_NULL         1
#define BMD_PHY_IF_MII          2
#define BMD_PHY_IF_GMII         3
#define BMD_PHY_IF_SGMII        4
#define BMD_PHY_IF_TBI          5
#define BMD_PHY_IF_XGMII        6
#define BMD_PHY_IF_RGMII        7
#define BMD_PHY_IF_RVMII        8
#define BMD_PHY_IF_XAUI         9
#define BMD_PHY_IF_XLAUI        10
#define BMD_PHY_IF_XFI          11
#define BMD_PHY_IF_SFI          12
#define BMD_PHY_IF_KR           13
#define BMD_PHY_IF_CR           14
#define BMD_PHY_IF_FIBER        15
#define BMD_PHY_IF_HIGIG        16
#define BMD_PHY_IF_KX           17
#define BMD_PHY_IF_RXAUI        18
#define BMD_PHY_IF_SR           19

#define BMD_PHY_ABIL_LOOPBACK       (1L << 0)
#define BMD_PHY_ABIL_AN             (1L << 1)
#define BMD_PHY_ABIL_TBI            (1L << 2)
#define BMD_PHY_ABIL_MII            (1L << 3)
#define BMD_PHY_ABIL_GMII           (1L << 4)
#define BMD_PHY_ABIL_SGMII          (1L << 5)
#define BMD_PHY_ABIL_XGMII          (1L << 6)
#define BMD_PHY_ABIL_SERDES         (1L << 7)
#define BMD_PHY_ABIL_AN_SGMII       (1L << 8)
#define BMD_PHY_ABIL_RGMII          (1L << 9)
#define BMD_PHY_ABIL_RVMII          (1L << 10)
#define BMD_PHY_ABIL_XAUI           (1L << 11)
#define BMD_PHY_ABIL_20GB           (1L << 12)
#define BMD_PHY_ABIL_PAUSE_TX       (1L << 13)
#define BMD_PHY_ABIL_PAUSE_RX       (1L << 14)
#define BMD_PHY_ABIL_PAUSE_ASYMM    (1L << 15)
#define BMD_PHY_ABIL_10MB_HD        (1L << 16)
#define BMD_PHY_ABIL_10MB_FD        (1L << 17)
#define BMD_PHY_ABIL_100MB_HD       (1L << 18)
#define BMD_PHY_ABIL_100MB_FD       (1L << 19)
#define BMD_PHY_ABIL_1000MB_HD      (1L << 20)
#define BMD_PHY_ABIL_1000MB_FD      (1L << 21)
#define BMD_PHY_ABIL_2500MB         (1L << 22)
#define BMD_PHY_ABIL_3000MB         (1L << 23)
#define BMD_PHY_ABIL_10GB           (1L << 24)
#define BMD_PHY_ABIL_13GB           (1L << 25)
#define BMD_PHY_ABIL_16GB           (1L << 26)
#define BMD_PHY_ABIL_21GB           (1L << 27)
#define BMD_PHY_ABIL_25GB           (1L << 28)
#define BMD_PHY_ABIL_30GB           (1L << 29)
#define BMD_PHY_ABIL_40GB           (1L << 30)
#define BMD_PHY_ABIL_100GB          (1L << 31)
#define BMD_PHY_ABIL_PAUSE          (BMD_PHY_ABIL_PAUSE_TX | BMD_PHY_ABIL_PAUSE_RX)
#define BMD_PHY_ABIL_10MB           (BMD_PHY_ABIL_10MB_HD | BMD_PHY_ABIL_10MB_FD)
#define BMD_PHY_ABIL_100MB          (BMD_PHY_ABIL_100MB_HD | BMD_PHY_ABIL_100MB_FD)
#define BMD_PHY_ABIL_1000MB         (BMD_PHY_ABIL_1000MB_HD | BMD_PHY_ABIL_1000MB_FD)

extern int 
bmd_phy_probe(int unit, int port);

extern int 
bmd_phy_init(int unit, int port);

extern int 
bmd_phy_staged_init(int unit, cdk_pbmp_t *pbmp);

extern int 
bmd_phy_attach(int unit, int port);

extern int 
bmd_phy_detach(int unit, int port);

extern int 
bmd_phy_mode_set(int unit, int port, char *name, int mode, int enable);

extern int 
bmd_phy_notify_mac_enable(int unit, int port, int enable);

extern int 
bmd_phy_notify_mac_loopback(int unit, int port, int enable);

extern int 
bmd_phy_link_get(int unit, int port, int *link, int *an_done);

extern int 
bmd_phy_autoneg_set(int unit, int port, int an);

extern int 
bmd_phy_autoneg_get(int unit, int port, int *an);

extern int 
bmd_phy_speed_set(int unit, int port, uint32_t speed);

extern int 
bmd_phy_speed_get(int unit, int port, uint32_t *speed);

extern int 
bmd_phy_duplex_set(int unit, int port, int duplex);

extern int 
bmd_phy_duplex_get(int unit, int port, int *duplex);

extern int 
bmd_phy_loopback_set(int unit, int port, int enable);

extern int 
bmd_phy_loopback_get(int unit, int port, int *enable);

extern int 
bmd_phy_remote_loopback_set(int unit, int port, int enable);

extern int 
bmd_phy_remote_loopback_get(int unit, int port, int *enable);

extern int 
bmd_phy_line_interface_set(int unit, int port, int intf);

extern int 
bmd_phy_line_interface_get(int unit, int port, int *intf);

extern int 
bmd_phy_eee_set(int unit, int port, int mode);

extern int 
bmd_phy_eee_get(int unit, int port, int *mode);

extern int 
bmd_phy_fw_helper_set(int unit, int port,
                      int (*fw_helper)(void *, uint32_t, uint32_t, void *));

extern int 
bmd_phy_fw_info_get(void *ctx, int *unit, int *port, const char **drv_name);

extern int 
bmd_phy_fw_base_set(int unit, int port, char *name, uint32_t fw_base);

extern int 
bmd_phy_default_ability_set(int unit, int port, uint32_t ability);

extern int
bmd_phy_set_slave(int unit, int port, int slave_port, int slave_idx);

#endif /* __BMD_PHY_H__ */
