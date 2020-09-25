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
#define _CHIP_DYN_CONFIG        DCFG_FLEX
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
    if (port >= 22 && port < 38) {
        return (port - 22) + 16 + CDK_XGSD_MIIM_EBUS(0);
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
    "bcm53415r",
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

static cdk_port_config_t _port_configs[] = {
    /* speed flags mode  sys  app */
    {      0,    0,   0,  -1,  -1  },
    {  10000,    0,   0,   2,  22  },
    {  10000,    0,   0,   3,  23 },
    {  10000,    0,   0,   4,  24 },
    {  10000,    0,   0,   5,  25 },
    {  10000,    0,   0,   6,  26  },
    {  10000,    0,   0,   7,  27  },
    {  10000,    0,   0,   8,  28  },
    {  10000,    0,   0,   9,  29  },
    {  10000,    0,   0,  10,  30  },
    {  10000,    0,   0,  11,  31  },
    {  10000,    0,   0,  12,  32  },
    {  10000,    0,   0,  13,  33  },
    {  10000,    0,   0,  14,  34  },
    {  10000,    0,   0,  15,  35  },
    {  10000,    0,   0,  16,  36  },
    {  10000,    0,   0,  17,  37  }
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
    int rv = CDK_E_NONE;
    phy_ctrl_t *lpc = pc;
    uint32_t mdi_map;

    while (lpc) {
        if (lpc->drv == NULL || lpc->drv->drv_name == NULL) {
            return CDK_E_INTERNAL;
        }
        if ((CDK_STRSTR(lpc->drv->drv_name, "bcm84848") != NULL) && 
            (lpc->port % 2) == 0) {
            mdi_map = 0x0123;
            rv = PHY_CONFIG_SET(lpc, PhyConfig_MdiPairRemap,
                                mdi_map, NULL);
            PHY_VERB(lpc, ("Remap MDI Pair (0x%04"PRIx32")\n", mdi_map));
        }
        lpc = lpc->next;
    }

    return rv;
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

board_config_t board_bcm53415_10g = {
    "bcm53415_10g",
    _chip_configs,
    &_phy_reset_cb,
    &_phy_init_cb,
};
