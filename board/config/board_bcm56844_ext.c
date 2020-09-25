/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.1,
 * WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING
 * ANY FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.$
 */

#include <board/board_config.h>
#include <board/sdk56840.h>
#include <cdk/cdk_string.h>
#include <phy/phy_buslist.h>

#if defined(CDK_CONFIG_INCLUDE_BCM56840_A0) && CDK_CONFIG_INCLUDE_BCM56840_A0 == 1
#include <cdk/chip/bcm56840_a0_defs.h>
#define _CHIP_DYN_CONFIG        DCFG_LCPLL_156
#endif

static cdk_port_map_port_t _skip_ports[] = { -1 };

#ifndef _CHIP_DYN_CONFIG 
#define _CHIP_DYN_CONFIG        0
#endif

#define HG2 CDK_DCFG_PORT_MODE_HIGIG2

static cdk_port_config_t _port_configs[] = {
    /* speed flags mode  sys  app */
    {      0,    0,   0,  -1,  -1  },
    {  10000,    0,   0,   1,   9  },
    {  10000,    0,   0,   2,  10  },
    {  10000,    0,   0,   3,  11  },
    {  10000,    0,   0,   4,  12  },
    {  10000,    0,   0,   5,  13  },
    {  10000,    0,   0,   6,  14  },
    {  10000,    0,   0,   7,  15  },
    {  10000,    0,   0,   8,  16  },
    {  10000,    0,   0,   9,  17  },
    {  10000,    0,   0,  10,  18  },
    {  10000,    0,   0,  11,  19  },
    {  10000,    0,   0,  12,  20  },
    {  10000,    0,   0,  13,  25  },
    {  10000,    0,   0,  14,  26  },
    {  10000,    0,   0,  15,  27  },
    {  10000,    0,   0,  16,  28  },
    {  10000,    0,   0,  17,  29  },
    {  10000,    0,   0,  18,  30  },
    {  10000,    0,   0,  19,  31  },
    {  10000,    0,   0,  20,  32  },
    {  10000,    0,   0,  21,  33  },
    {  10000,    0,   0,  22,  34  },
    {  10000,    0,   0,  23,  35  },
    {  10000,    0,   0,  24,  36  },
    {  10000,    0,   0,  25,  37  },
    {  10000,    0,   0,  26,  38  },
    {  10000,    0,   0,  27,  39  },
    {  10000,    0,   0,  28,  40  },
    {  10000,    0,   0,  29,  41  },
    {  10000,    0,   0,  30,  42  },
    {  10000,    0,   0,  31,  43  },
    {  10000,    0,   0,  32,  44  },
    {  10000,    0,   0,  33,  45  },
    {  10000,    0,   0,  34,  46  },
    {  10000,    0,   0,  35,  47  },
    {  10000,    0,   0,  36,  48  },
    {  10000,    0,   0,  37,  57  },
    {  10000,    0,   0,  38,  58  },
    {  10000,    0,   0,  39,  59  },
    {  10000,    0,   0,  40,  60  },
    {  10000,    0,   0,  41,  61  },
    {  10000,    0,   0,  42,  62  },
    {  10000,    0,   0,  43,  63  },
    {  10000,    0,   0,  44,  64  },
    {  10000,    0,   0,  45,  65  },
    {  10000,    0,   0,  46,  66  },
    {  10000,    0,   0,  47,  67  },
    {  10000,    0,   0,  48,  68  } 
};                             

#ifdef CDK_CONFIG_ARCH_XGS_INSTALLED
#include <cdk/arch/xgs_miim.h>

static uint32_t
_phy_addr(int port)
{
    if (port > 48) {
        return (port - 49) + CDK_XGS_MIIM_EBUS(2);
    }
    if (port > 44) {
        return (port - 45 + 4) + CDK_XGS_MIIM_EBUS(2);
    }
    if (port > 28) {
        return (port - 29 + 4) + CDK_XGS_MIIM_EBUS(1);
    }
    if (port > 24) {
        return (port - 9) + CDK_XGS_MIIM_EBUS(0);
    }
    return port - 9 + 4 + CDK_XGS_MIIM_EBUS(0);
}

static int 
_read(int unit, uint32_t addr, uint32_t reg, uint32_t *val)
{
    return cdk_xgs_miim_read(unit, addr, reg, val);
}

static int 
_write(int unit, uint32_t addr, uint32_t reg, uint32_t val)
{
    return cdk_xgs_miim_write(unit, addr, reg, val);
}

static int
_phy_inst(int port)
{
    return (port - 1) & 3;
}

static phy_bus_t _phy_bus_miim_ext = {
    "bcm56844",
    _phy_addr,
    _read,
    _write,
    _phy_inst
};
#endif /* CDK_CONFIG_ARCH_XGS_INSTALLED */

static phy_bus_t *_phy_bus[] = {
#ifdef CDK_CONFIG_ARCH_XGS_INSTALLED
#ifdef PHY_BUS_BCM56840_MIIM_INT_INSTALLED
    &phy_bus_bcm56840_miim_int,
#endif
    &_phy_bus_miim_ext,
#endif /* CDK_CONFIG_ARCH_XGS_INSTALLED */
    NULL
};
                               
static int                     
_phy_reset_cb(phy_ctrl_t *pc)
{
    int rv = CDK_E_NONE;
    phy_ctrl_t *lpc = pc;
    uint32_t tx_pol, rx_map;

    while (lpc) {
        if (lpc->drv == NULL || lpc->drv->drv_name == NULL) {
            return CDK_E_INTERNAL;
        }
        if (CDK_STRSTR(lpc->drv->drv_name, "warpcore") != NULL) {
            /* Invert Tx polarity on all lanes */
            tx_pol = 0x1111;
            rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiTxPolInvert,
                                tx_pol, NULL);
            PHY_VERB(lpc, ("Flip Tx pol (0x%04"PRIx32")\n", tx_pol));
            /* Remap Rx lanes */
            if (PHY_CTRL_PHY_INST(pc) == 0) {
                rx_map = 0x1032;
                rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiRxLaneRemap,
                                    rx_map, NULL);
                PHY_VERB(lpc, ("Remap Rx lanes (0x%04"PRIx32")\n", rx_map));
            }
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
    sdk56840_ledprog_info,
    _CHIP_DYN_CONFIG,
    COUNTOF(_port_configs),
    _port_configs
};

static board_chip_config_t *_chip_configs[] = {
    &_chip_config,
    NULL
};

board_config_t board_bcm56844_ext = {
    "bcm56844_ext",
    _chip_configs,
    &_phy_reset_cb,
    &_phy_init_cb,
};
