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
    {  10000,    0,   0,   1,   5  },
    {  10000,    0,   0,   2,   6  },
    {  10000,    0,   0,   3,   7  },
    {  10000,    0,   0,   4,   8  },
    {  10000,    0,   0,   5,   9  },
    {  10000,    0,   0,   6,  10  },
    {  10000,    0,   0,   7,  11  },
    {  10000,    0,   0,   8,  12  },
    {  10000,    0,   0,   9,  13  },
    {  10000,    0,   0,  10,  14  },
    {  10000,    0,   0,  11,  15  },
    {  10000,    0,   0,  12,  16  },
    {  10000,    0,   0,  13,  17  },
    {  10000,    0,   0,  14,  18  },
    {  10000,    0,   0,  15,  19  },
    {  10000,    0,   0,  16,  20  },
    {  10000,    0,   0,  17,  21  },
    {  10000,    0,   0,  18,  22  },
    {  10000,    0,   0,  19,  23  },
    {  10000,    0,   0,  20,  24  },
    {  10000,    0,   0,  21,  25  },
    {  10000,    0,   0,  22,  26  },
    {  10000,    0,   0,  23,  27  },
    {  10000,    0,   0,  24,  28  },
    {  10000,    0,   0,  25,  29  },
    {  10000,    0,   0,  26,  30  },
    {  10000,    0,   0,  27,  31  },
    {  10000,    0,   0,  28,  32  },
    {  10000,    0,   0,  29,  33  },
    {  10000,    0,   0,  30,  34  },
    {  10000,    0,   0,  31,  35  },
    {  10000,    0,   0,  32,  36  },
    {  10000,    0,   0,  33,  37  },
    {  10000,    0,   0,  34,  38  },
    {  10000,    0,   0,  35,  39  },
    {  10000,    0,   0,  36,  40  },
    {  10000,    0,   0,  37,  41  },
    {  10000,    0,   0,  38,  42  },
    {  10000,    0,   0,  39,  43  },
    {  10000,    0,   0,  40,  44  },
    {  10000,    0,   0,  41,  45  },
    {  10000,    0,   0,  42,  46  },
    {  10000,    0,   0,  43,  47  },
    {  10000,    0,   0,  44,  48  },
    {  10000,    0,   0,  45,  49  },
    {  10000,    0,   0,  46,  50  },
    {  10000,    0,   0,  47,  51  },
    {  10000,    0,   0,  48,  52  },
    {  10000,    0,   0,  49,  57  },
    {  10000,    0,   0,  50,  58  },
    {  10000,    0,   0,  51,  59  },
    {  10000,    0,   0,  52,  60  },
    {  10000,    0,   0,  53,  61  },
    {  10000,    0,   0,  54,  62  },
    {  10000,    0,   0,  55,  63  },
    {  10000,    0,   0,  56,  64  },
    {  10000,    0,   0,  57,  65  },
    {  10000,    0,   0,  58,  66  },
    {  10000,    0,   0,  59,  67  },
    {  10000,    0,   0,  60,  68  },
    {  10000,    0,   0,  61,  69  },
    {  10000,    0,   0,  62,  70  },
    {  10000,    0,   0,  63,  71  },
    {  10000,    0,   0,  64,  72  }
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

board_config_t board_bcm56845_ext = {
    "bcm56845_ext",
    _chip_configs,
    &_phy_reset_cb,
    &_phy_init_cb,
};
