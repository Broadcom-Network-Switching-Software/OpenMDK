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
    {  10000,    0,   0,   1,  13  },
    {  10000,    0,   0,   2,  14  },
    {  10000,    0,   0,   3,  15  },
    {  10000,    0,   0,   4,  16  },
    {  10000,    0,   0,   5,  21  },
    {  10000,    0,   0,   6,  22  },
    {  10000,    0,   0,   7,  23  },
    {  10000,    0,   0,   8,  24  },
    {  10000,    0,   0,   9,  25  },
    {  10000,    0,   0,  10,  26  },
    {  10000,    0,   0,  11,  27  },
    {  10000,    0,   0,  12,  28  },
    {  10000,    0,   0,  13,  29  },
    {  10000,    0,   0,  14,  30  },
    {  10000,    0,   0,  15,  31  },
    {  10000,    0,   0,  16,  32  },
    {  10000,    0,   0,  17,  45  },
    {  10000,    0,   0,  18,  46  },
    {  10000,    0,   0,  19,  47  },
    {  10000,    0,   0,  20,  48  },
    {  10000,    0,   0,  21,  49  },
    {  10000,    0,   0,  22,  50  },
    {  10000,    0,   0,  23,  51  },
    {  10000,    0,   0,  24,  52  },
    {  10000,    0,   0,  25,  53  },
    {  10000,    0,   0,  26,  54  },
    {  10000,    0,   0,  27,  55  },
    {  10000,    0,   0,  28,  56  },
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
    {  10000,    0,   0,  49,  77  },
    {  10000,    0,   0,  50,  78  },
    {  10000,    0,   0,  51,  79  },
    {  10000,    0,   0,  52,  80  },
    {  10000,    0,   0,  53,  81  },
    {  10000,    0,   0,  54,  82  },
    {  10000,    0,   0,  55,  83  },
    {  10000,    0,   0,  56,  84  },
    {  10000,    0,   0,  57,  97  },
    {  10000,    0,   0,  58,  98  },
    {  10000,    0,   0,  59,  99  },
    {  10000,    0,   0,  60, 100  },
    {  10000,    0,   0,  61, 101  },
    {  10000,    0,   0,  62, 102  },
    {  10000,    0,   0,  63, 103  },
    {  10000,    0,   0,  64, 104  },
    {  10000,    0,   0,  65, 105  },
    {  10000,    0,   0,  66, 106  },
    {  10000,    0,   0,  67, 107  },
    {  10000,    0,   0,  68, 108  },
    {  10000,    0,   0,  69, 109  },
    {  10000,    0,   0,  70, 110  },
    {  10000,    0,   0,  71, 111  },
    {  10000,    0,   0,  72, 112  }
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
                /* Remap rx lanes */
                rx_map = 0x2103;
                rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiRxLaneRemap,
                                        rx_map, NULL);
                PHY_VERB(lpc, ("Remap Rx lanes (0x%04"PRIx32")\n", rx_map));
                
                /* Remap tx lanes */
                tx_map = 0x0321;
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

board_config_t board_bcm56854_10g = {
    "bcm56854_10g",
    _chip_configs,
    &_phy_reset_cb,
    &_phy_init_cb,
};
