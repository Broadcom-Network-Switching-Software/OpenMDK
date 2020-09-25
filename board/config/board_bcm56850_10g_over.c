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

static cdk_port_map_port_t _skip_ports[] = { -1 };

#ifndef _CHIP_DYN_CONFIG 
#define _CHIP_DYN_CONFIG        0
#endif

#define HG2 CDK_DCFG_PORT_MODE_HIGIG2

static cdk_port_config_t _port_configs[] = {
    /* speed flags mode  sys  app */
    {      0,    0,   0,  -1,  -1  },
    {  10000,    0,   0,   1,   1  },
    {  10000,    0,   0,   2,   2  },
    {  10000,    0,   0,   3,   3  },
    {  10000,    0,   0,   4,   4  },
    {  10000,    0,   0,   5,   5  },
    {  10000,    0,   0,   6,   6  },
    {  10000,    0,   0,   7,   7  },
    {  10000,    0,   0,   8,   8  },
    {  10000,    0,   0,   9,   9  },
    {  10000,    0,   0,  10,  10  },
    {  10000,    0,   0,  11,  11  },
    {  10000,    0,   0,  12,  12  },
    {  40000,    0,   0,  13,  13  },
    {  40000,    0,   0,  14,  17  },
    {  10000,    0,   0,  15,  21  },
    {  10000,    0,   0,  16,  22  },
    {  10000,    0,   0,  17,  23  },
    {  10000,    0,   0,  18,  24  },
    {  10000,    0,   0,  19,  25  },
    {  10000,    0,   0,  20,  26  },
    {  10000,    0,   0,  21,  27  },
    {  10000,    0,   0,  22,  28  },
    {  10000,    0,   0,  23,  29  },
    {  10000,    0,   0,  24,  30  },
    {  10000,    0,   0,  25,  31  },
    {  10000,    0,   0,  26,  32  },
    {  10000,    0,   0,  27,  33  },
    {  10000,    0,   0,  28,  34  },
    {  10000,    0,   0,  29,  35  },
    {  10000,    0,   0,  30,  36  },
    {  10000,    0,   0,  31,  37  },
    {  10000,    0,   0,  32,  38  },
    {  10000,    0,   0,  33,  39  },
    {  10000,    0,   0,  34,  40  },
    {  10000,    0,   0,  35,  41  },
    {  10000,    0,   0,  36,  42  },
    {  10000,    0,   0,  37,  43  },
    {  10000,    0,   0,  38,  44  },
    {  40000,    0,   0,  39,  45  },
    {  40000,    0,   0,  40,  49  },
    {  10000,    0,   0,  41,  53  },
    {  10000,    0,   0,  42,  54  },
    {  10000,    0,   0,  43,  55  },
    {  10000,    0,   0,  44,  56  },
    {  10000,    0,   0,  45,  57  },
    {  10000,    0,   0,  46,  58  },
    {  10000,    0,   0,  47,  59  },
    {  10000,    0,   0,  48,  60  },
    {  10000,    0,   0,  49,  61  },
    {  10000,    0,   0,  50,  62  },
    {  10000,    0,   0,  51,  63  },
    {  10000,    0,   0,  52,  64  },
    {  10000,    0,   0,  53,  65  },
    {  10000,    0,   0,  54,  66  },
    {  10000,    0,   0,  55,  67  },
    {  10000,    0,   0,  56,  68  },
    {  10000,    0,   0,  57,  69  },
    {  10000,    0,   0,  58,  70  },
    {  10000,    0,   0,  59,  71  },
    {  10000,    0,   0,  60,  72  },
    {  10000,    0,   0,  61,  73  },
    {  10000,    0,   0,  62,  74  },
    {  10000,    0,   0,  63,  75  },
    {  10000,    0,   0,  64,  76  },
    {  40000,    0,   0,  65,  77  },
    {  40000,    0,   0,  66,  81  },
    {  10000,    0,   0,  67,  85  },
    {  10000,    0,   0,  68,  86  },
    {  10000,    0,   0,  69,  87  },
    {  10000,    0,   0,  70,  88  },
    {  10000,    0,   0,  71,  89  },
    {  10000,    0,   0,  72,  90  },
    {  10000,    0,   0,  73,  91  },
    {  10000,    0,   0,  74,  92  },
    {  10000,    0,   0,  75,  93  },
    {  10000,    0,   0,  76,  94  },
    {  10000,    0,   0,  77,  95  },
    {  10000,    0,   0,  78,  96  },
    {  10000,    0,   0,  79,  97  },
    {  10000,    0,   0,  80,  98  },
    {  10000,    0,   0,  81,  99  },
    {  10000,    0,   0,  82, 100  },
    {  10000,    0,   0,  83, 101  },
    {  10000,    0,   0,  84, 102  },
    {  10000,    0,   0,  85, 103  },
    {  10000,    0,   0,  86, 104  },
    {  10000,    0,   0,  87, 105  },
    {  10000,    0,   0,  88, 106  },
    {  10000,    0,   0,  89, 107  },
    {  10000,    0,   0,  90, 108  },
    {  40000,    0,   0,  91, 109  },
    {  40000,    0,   0,  92, 113  },
    {  10000,    0,   0,  93, 117  },
    {  10000,    0,   0,  94, 118  },
    {  10000,    0,   0,  95, 119  },
    {  10000,    0,   0,  96, 120  },
    {  10000,    0,   0,  97, 121  },
    {  10000,    0,   0,  98, 122  },
    {  10000,    0,   0,  99, 123  },
    {  10000,    0,   0, 100, 124  },
    {  10000,    0,   0, 101, 125  },
    {  10000,    0,   0, 102, 126  },
    {  10000,    0,   0, 103, 127  },
    {  10000,    0,   0, 104, 128  }
};

#ifdef CDK_CONFIG_ARCH_XGSM_INSTALLED
#include <cdk/arch/xgsm_miim.h>

static uint32_t
_phy_addr(int port)
{
    if (port > 108) {
        return (port - 108) + CDK_XGSM_MIIM_EBUS(5);
    }
    if (port > 84) {
        return (port - 84) + CDK_XGSM_MIIM_EBUS(4);
    }
    if (port > 64) {
        return (port - 64) + CDK_XGSM_MIIM_EBUS(3);
    }
    if (port > 44) {
        return (port - 44) + CDK_XGSM_MIIM_EBUS(2);
    }
    if (port > 20) {
        return (port - 20) + CDK_XGSM_MIIM_EBUS(1);
    }
    return port + CDK_XGSM_MIIM_EBUS(0);
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
#endif /* CDK_CONFIG_ARCH_XGSM_INSTALLED */
    NULL
};

static int                     
_phy_reset_cb(phy_ctrl_t *pc)
{
    int rv = CDK_E_NONE;
    phy_ctrl_t *lpc = pc;
    uint32_t tx_pol, rx_map, tx_map;

    while (lpc) {
        if (lpc->drv == NULL || lpc->drv->drv_name == NULL) {
            return CDK_E_INTERNAL;
        }
        if (CDK_STRSTR(lpc->drv->drv_name, "tsc") != NULL) {
            /* Invert Tx polarity on all lanes */
            tx_pol = 0x1111;
            rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiTxPolInvert,
                                tx_pol, NULL);
            PHY_VERB(lpc, ("Flip Tx pol (0x%04"PRIx32")\n", tx_pol));
            /* Remap Rx lanes */
            if ((PHY_CTRL_PHY_INST(lpc) & 0x3) == 0) {
                tx_map = 0x1230;
                rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiTxLaneRemap,
                                    tx_map, NULL);
                PHY_VERB(lpc, ("Remap Tx lanes (0x%04"PRIx32")\n", tx_map));
 
                rx_map = 0x3012;
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
    NULL,
    _CHIP_DYN_CONFIG,
    COUNTOF(_port_configs),
    _port_configs
};

static board_chip_config_t *_chip_configs[] = {
    &_chip_config,
    NULL
};

board_config_t board_bcm56850_10g_over = {
    "bcm56850_10g_over",
    _chip_configs,
    &_phy_reset_cb,
    &_phy_init_cb,
};
