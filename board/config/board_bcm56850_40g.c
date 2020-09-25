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

static cdk_port_map_port_t _skip_ports[] = { -1 };

#ifndef _CHIP_DYN_CONFIG 
#define _CHIP_DYN_CONFIG        0
#endif

#define HG2 CDK_DCFG_PORT_MODE_HIGIG2

static cdk_port_config_t _port_configs[] = {
    /* speed flags mode  sys  app */
    {      0,    0,   0,  -1,  -1  },
    {  40000,    0,   0,   1,   1  },
    {  40000,    0,   0,   2,   5  },
    {  40000,    0,   0,   3,   9  },
    {  40000,    0,   0,   4,  21  },
    {  40000,    0,   0,   5,  25  },
    {  40000,    0,   0,   6,  29  },
    {  40000,    0,   0,   7,  33  },
    {  40000,    0,   0,   8,  37  },
    {  40000,    0,   0,   9,  41  },
    {  40000,    0,   0,  10,  53  },
    {  40000,    0,   0,  11,  57  },
    {  40000,    0,   0,  12,  61  },
    {  40000,    0,   0,  13,  65  },
    {  40000,    0,   0,  14,  69  },
    {  40000,    0,   0,  15,  73  },
    {  40000,    0,   0,  16,  85  },
    {  40000,    0,   0,  17,  89  },
    {  40000,    0,   0,  18,  93  },
    {  40000,    0,   0,  19,  97  },
    {  40000,    0,   0,  20, 101  },
    {  40000,    0,   0,  21, 105  },
    {  40000,    0,   0,  22, 117  },
    {  40000,    0,   0,  23, 121  },
    {  40000,    0,   0,  24, 125  }
};                             
                               
#ifdef CDK_CONFIG_ARCH_XGSM_INSTALLED

#include <cdk/arch/xgsm_miim.h>

static uint32_t
_phy_addr(int port)
{
    if (port > 48) {
        return (port - 45) + CDK_XGSM_MIIM_EBUS(2);
    }
    if (port > 24) {
        return (port - 21) + CDK_XGSM_MIIM_EBUS(1);
    }
    return port + 3 + CDK_XGSM_MIIM_EBUS(0);
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
    return (port - 1) & 3;
}

static phy_bus_t _phy_bus_miim_ext = {
    "bcm56850",
    _phy_addr,
    _read,
    _write,
    _phy_inst
};

#endif /* CDK_CONFIG_ARCH_XGSM_INSTALLED */

static phy_bus_t *_phy_bus[] = {
#ifdef CDK_CONFIG_ARCH_XGSM_INSTALLED
#ifdef PHY_BUS_BCM56850_MIIM_INT_INSTALLED
    &phy_bus_bcm56850_miim_int,
#endif
    &_phy_bus_miim_ext,
#endif
    NULL
};

static int                     
_phy_reset_cb(phy_ctrl_t *pc)
{
    int rv = CDK_E_NONE;
    phy_ctrl_t *lpc = pc;
    uint32_t rx_map, tx_map;

    while (lpc) {
        if (lpc->drv == NULL || lpc->drv->drv_name == NULL) {
            return CDK_E_INTERNAL;
        }
        if (CDK_STRSTR(lpc->drv->drv_name, "tsc") != NULL) {
            rx_map = 0x3012;
            rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiRxLaneRemap,
                                rx_map, NULL);
            PHY_VERB(lpc, ("Remap Rx lanes (0x%04"PRIx32")\n", rx_map));

            tx_map = 0x1230;
            rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiTxLaneRemap,
                                tx_map, NULL);
            PHY_VERB(lpc, ("Remap Tx lanes (0x%04"PRIx32")\n", tx_map));
        }
        lpc = lpc->next;
    }

    return rv;
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
    COUNTOF(_port_configs),
    _port_configs
};

static board_chip_config_t *_chip_configs[] = {
    &_chip_config,
    NULL
};

board_config_t board_bcm56850_40g = {
    "bcm56850_40g",
    _chip_configs,
    &_phy_reset_cb,
    &_phy_init_cb,
};
