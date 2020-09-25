/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.1,
 * WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING
 * ANY FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.$
 */

#include <board/board_config.h>
#include <cdk/cdk_string.h>
#include <phy/phy_buslist.h>
#include <cdk/cdk_debug.h>

#if defined(CDK_CONFIG_INCLUDE_BCM56150_A0) && CDK_CONFIG_INCLUDE_BCM56150_A0 == 1
#include <cdk/chip/bcm56150_a0_defs.h>
/* Assign _CHIP_DYN_CONFIG for different port configuration */
#define _CHIP_DYN_CONFIG        0 
#endif

static cdk_port_map_port_t _skip_ports[] = { -1 };

#ifndef _CHIP_DYN_CONFIG 
#define _CHIP_DYN_CONFIG        0
#endif

#define HG2 CDK_DCFG_PORT_MODE_HIGIG2

#ifdef CDK_CONFIG_ARCH_XGSM_INSTALLED

#include <cdk/arch/xgsm_miim.h>

static uint32_t
_phy_addr(int port)
{
    if (port >= 2 && port < 26) {
        return 0x02 + ((port - 0x2) >> 2) + CDK_XGSM_MIIM_EBUS(0);
    } else if (port < 34) {
        return 0x01 + (port - 26) + CDK_XGSM_MIIM_EBUS(1);
    }
    return 0;
}

static int 
_read(int unit, uint32_t addr, uint32_t reg, uint32_t *val)
{
    return cdk_xgsm_miim_read(unit, addr, reg, val);
}

static int 
_write(int unit, uint32_t addr, uint32_t reg, uint32_t val)
{
    return cdk_xgsm_miim_write(unit, addr, reg, val);
}

static int
_phy_inst(int port)
{
    return port - 2;
}

static phy_bus_t _phy_bus_miim_ext = {
    "bcm53393_svk",
    _phy_addr,
    _read,
    _write,
    _phy_inst
};

#endif /* CDK_CONFIG_ARCH_XGSM_INSTALLED */
 
static phy_bus_t *_phy_bus[] = {
#ifdef CDK_CONFIG_ARCH_XGSM_INSTALLED
#ifdef PHY_BUS_BCM56151_MIIM_INT_INSTALLED
    &phy_bus_bcm56151_miim_int,
#endif
    &_phy_bus_miim_ext,
#endif
    NULL
};
                              
static int                     
_phy_reset_cb(phy_ctrl_t *pc)
{
    return CDK_E_NONE;
}

static int 
_phy_init_cb(phy_ctrl_t *pc)
{
    return CDK_E_NONE;
}

static board_chip_config_t _chip_config = {
    _skip_ports,
    _phy_bus,
    NULL,
    _CHIP_DYN_CONFIG,
};

static board_chip_config_t *_chip_configs[] = {
    &_chip_config,
    NULL
};

board_config_t board_bcm53393_svk = {
    "bcm53393_svk",
    _chip_configs,
    &_phy_reset_cb,
    &_phy_init_cb,
};
