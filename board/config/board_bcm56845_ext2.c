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
#define _CHIP_DYN_CONFIG        (DCFG_LCPLL_156 | DCFG_PLL_MOD)
#endif

static cdk_port_map_port_t _skip_ports[] = { -1 };

#ifndef _CHIP_DYN_CONFIG 
#define _CHIP_DYN_CONFIG        0
#endif

#define HG2 CDK_DCFG_PORT_MODE_HIGIG2

static cdk_port_config_t _port_configs[] = {
    /* speed flags mode  sys  app */
    {      0,    0,   0,  -1,  -1  },
    {  10000,    0,   0,   1,   5  },
    {  10000,    0,   0,   2,   6  },
    {  10000,    0,   0,   3,   7  },
    {  10000,    0,   0,   4,   8  },
    {  10000,    0,   0,   5,   9  },
    {  10000,    0,   0,   6,  10  },
    {  10000,    0,   0,   7,  11  },
    {  10000,    0,   0,   8,  12  },
    {  40000,    0,   0,   9,  13  },
    {  40000,    0,   0,  10,  17  },
    {  40000,    0,   0,  11,  21  },
    {  40000,    0,   0,  12,  25  },
    {  40000,    0,   0,  13,  29  },
    {  40000,    0,   0,  14,  33  },
    {  40000,    0,   0,  15,  37  },
    {  40000,    0,   0,  16,  41  },
    {  40000,    0,   0,  17,  45  },
    {  40000,    0,   0,  18,  49  },
    {  40000,    0,   0,  19,  57  },
    {  40000,    0,   0,  20,  61  },
    {  40000,    0,   0,  21,  65  },
    {  40000,    0,   0,  22,  69  }
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
    NULL,
    sdk56840_ledprog_info,
    _CHIP_DYN_CONFIG,
    COUNTOF(_port_configs),
    _port_configs
};

static board_chip_config_t *_chip_configs[] = {
    &_chip_config,
    NULL
};

board_config_t board_bcm56845_ext2 = {
    "bcm56845_ext2",
    _chip_configs,
    &_phy_reset_cb,
    &_phy_init_cb,
};
