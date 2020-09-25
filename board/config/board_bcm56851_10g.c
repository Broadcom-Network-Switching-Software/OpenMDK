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
    {  10000,    0,   0,   9,  25  },
    {  10000,    0,   0,  10,  26  },
    {  10000,    0,   0,  11,  27  },
    {  10000,    0,   0,  12,  28  },
    {  10000,    0,   0,  13,  29  },
    {  10000,    0,   0,  14,  30  },
    {  10000,    0,   0,  15,  31  },
    {  10000,    0,   0,  16,  32  },
    {  10000,    0,   0,  17,  33  },
    {  10000,    0,   0,  18,  34  },
    {  10000,    0,   0,  19,  35  },
    {  10000,    0,   0,  20,  36  },
    {  10000,    0,   0,  21,  37  },
    {  10000,    0,   0,  22,  38  },
    {  10000,    0,   0,  23,  39  },
    {  10000,    0,   0,  24,  40  },
    {  10000,    0,   0,  25,  41  },
    {  10000,    0,   0,  26,  42  },
    {  10000,    0,   0,  27,  43  },
    {  10000,    0,   0,  28,  44  },
    {  10000,    0,   0,  29,  57  },
    {  10000,    0,   0,  30,  58  },
    {  10000,    0,   0,  31,  59  },
    {  10000,    0,   0,  32,  60  },
    {  10000,    0,   0,  33,  61  },
    {  10000,    0,   0,  34,  62  },
    {  10000,    0,   0,  35,  63  },
    {  10000,    0,   0,  36,  64  },
    {  10000,    0,   0,  37,  65  },
    {  10000,    0,   0,  38,  66  },
    {  10000,    0,   0,  39,  67  },
    {  10000,    0,   0,  40,  68  },
    {  10000,    0,   0,  41,  69  },
    {  10000,    0,   0,  42,  70  },
    {  10000,    0,   0,  43,  71  },
    {  10000,    0,   0,  44,  72  },
    {  10000,    0,   0,  45,  73  },
    {  10000,    0,   0,  46,  74  },
    {  10000,    0,   0,  47,  75  },
    {  10000,    0,   0,  48,  76  },
    {  10000,    0,   0,  49,  89  },
    {  10000,    0,   0,  50,  90  },
    {  10000,    0,   0,  51,  91  },
    {  10000,    0,   0,  52,  92  },
    {  10000,    0,   0,  53,  93  },
    {  10000,    0,   0,  54,  94  },
    {  10000,    0,   0,  55,  95  },
    {  10000,    0,   0,  56,  96  },
    {  10000,    0,   0,  57,  97  },
    {  10000,    0,   0,  58,  98  },
    {  10000,    0,   0,  59,  99  },
    {  10000,    0,   0,  60, 100  },
    {  10000,    0,   0,  61, 101  },
    {  10000,    0,   0,  62, 102  },
    {  10000,    0,   0,  63, 103  },
    {  10000,    0,   0,  64, 104  },
    {  10000,    0,   0,  65, 121  },
    {  10000,    0,   0,  66, 122  },
    {  10000,    0,   0,  67, 123  },
    {  10000,    0,   0,  68, 124  },
    {  10000,    0,   0,  69, 125  },
    {  10000,    0,   0,  70, 126  },
    {  10000,    0,   0,  71, 127  },
    {  10000,    0,   0,  72, 128  }
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
            if ((PHY_CTRL_PHY_INST(lpc) & 0x3) == 0) {
                /* Remap rx lanes: swap lane 0 and lanes 2 on all TSC */
                rx_map = 0x3012;
                rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiRxLaneRemap,
                                        rx_map, NULL);
                PHY_VERB(lpc, ("Remap Rx lanes (0x%04"PRIx32")\n", rx_map));
                
                /* Remap tx lanes: swap lane 1 and lanes 3 on all even TSC (except TSC16) */
                tx_map = 0x3210;
                if ((((lpc->port >> 2) & 0x1) == 0) && (lpc->port != 65)) {
                    tx_map = 0x1230;
                }
                rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiTxLaneRemap,
                                        tx_map, NULL);
                PHY_VERB(lpc, ("Remap Tx lanes (0x%04"PRIx32")\n", tx_map));
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
    NULL,
    NULL,
    _CHIP_DYN_CONFIG,
    COUNTOF(_port_configs),
    _port_configs
};

static board_chip_config_t *_chip_configs[] = {
    &_chip_config,
    NULL
};

board_config_t board_bcm56851_10g = {
    "bcm56851_10g",
    _chip_configs,
    &_phy_reset_cb,
    &_phy_init_cb,
};
