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

#if defined(CDK_CONFIG_INCLUDE_BCM53400_A0) && CDK_CONFIG_INCLUDE_BCM53400_A0 == 1
#include <cdk/chip/bcm53400_a0_defs.h>
#define _CHIP_DYN_CONFIG        0
#endif

static cdk_port_map_port_t _skip_ports[] = { -1 };

#ifndef _CHIP_DYN_CONFIG 
#define _CHIP_DYN_CONFIG        0
#endif

#ifdef CDK_CONFIG_ARCH_XGSD_INSTALLED
#include <cdk/arch/xgsd_miim.h>

static uint32_t
_phy_addr(int port)
{
    if (port >= 2 && port < 18) {
        return (port - 2) + ((port - 2) >> 3) + CDK_XGSD_MIIM_EBUS(0);
    } else if (port >= 18 && port < 38) {
        return (port - 18) + CDK_XGSD_MIIM_EBUS(1);
    } 
    return 0;
}

static int 
_read(int unit, uint32_t addr, uint32_t reg, uint32_t *val)
{
    return cdk_xgsd_miim_read(unit, addr, reg, val);
}

static int 
_write(int unit, uint32_t addr, uint32_t reg, uint32_t val)
{
    return cdk_xgsd_miim_write(unit, addr, reg, val);
}

static int
_phy_inst(int port)
{
    return port - 2;
}

static phy_bus_t _phy_bus_miim_ext = {
    "bcm53411_svk",
    _phy_addr,
    _read,
    _write,
    _phy_inst
};
#endif /* CDK_CONFIG_ARCH_XGSD_INSTALLED */

static phy_bus_t *_phy_bus[] = {
#ifdef CDK_CONFIG_ARCH_XGSD_INSTALLED
#ifdef PHY_BUS_BCM53400_MIIM_INT_INSTALLED
    &phy_bus_bcm53400_miim_int,
#endif
    &_phy_bus_miim_ext,
#endif /* CDK_CONFIG_ARCH_XGSD_INSTALLED */
    NULL
};

static int                     
_phy_reset_cb(phy_ctrl_t *pc)
{
    int rv = CDK_E_NONE;

    return rv;
}

static int 
_phy_init_cb(phy_ctrl_t *pc)
{
    return CDK_E_NONE;
}

static int 
_post_init(int unit)
{
    int ioerr = 0;
#if defined(CDK_CONFIG_INCLUDE_BCM53400_A0) && CDK_CONFIG_INCLUDE_BCM53400_A0 == 1
    CMIC_GP_OUT_ENr_t cmic_gp_en;
    CMIC_GP_DATA_OUTr_t cmic_gp_data;
    TOP_XG_PLL_CTRL_6r_t xg_pll_ctrl_6;
    int enable, data;

    ioerr += READ_CMIC_GP_OUT_ENr(unit, &cmic_gp_en);
    enable = CMIC_GP_OUT_ENr_OUT_ENABLEf_GET(cmic_gp_en);
    enable |= (1 << 3);
    CMIC_GP_OUT_ENr_OUT_ENABLEf_SET(cmic_gp_en, enable);
    ioerr += WRITE_CMIC_GP_OUT_ENr(unit, cmic_gp_en);
    
    ioerr += READ_CMIC_GP_DATA_OUTr(unit, &cmic_gp_data);
    data = CMIC_GP_DATA_OUTr_DATA_OUTf_GET(cmic_gp_data);
    data &= ~(1 << 3);
    CMIC_GP_DATA_OUTr_DATA_OUTf_SET(cmic_gp_data, data);
    ioerr += WRITE_CMIC_GP_DATA_OUTr(unit, cmic_gp_data);

    ioerr += READ_TOP_XG_PLL_CTRL_6r(unit, 0, &xg_pll_ctrl_6);
    TOP_XG_PLL_CTRL_6r_MSC_CTRLf_SET(xg_pll_ctrl_6, 0x1020);
    ioerr += WRITE_TOP_XG_PLL_CTRL_6r(unit, 0, xg_pll_ctrl_6);

    ioerr += READ_CMIC_GP_OUT_ENr(unit, &cmic_gp_en);
    enable = CMIC_GP_OUT_ENr_OUT_ENABLEf_GET(cmic_gp_en);
    enable |= (1 << 3);
    CMIC_GP_OUT_ENr_OUT_ENABLEf_SET(cmic_gp_en, enable);
    ioerr += WRITE_CMIC_GP_OUT_ENr(unit, cmic_gp_en);
    
    ioerr += READ_CMIC_GP_DATA_OUTr(unit, &cmic_gp_data);
    data = CMIC_GP_DATA_OUTr_DATA_OUTf_GET(cmic_gp_data);
    data |= (1 << 3);
    CMIC_GP_DATA_OUTr_DATA_OUTf_SET(cmic_gp_data, data);
    ioerr += WRITE_CMIC_GP_DATA_OUTr(unit, cmic_gp_data);
#endif

    return ioerr ? CDK_E_IO : CDK_E_NONE;
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

board_config_t board_bcm53411_svk = {
    "bcm53411_svk",
    _chip_configs,
    &_phy_reset_cb,
    &_phy_init_cb,
    &_post_init,
};
