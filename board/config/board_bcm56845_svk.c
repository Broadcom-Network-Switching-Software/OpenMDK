/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.1,
 * WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING
 * ANY FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.$
 */

/*
 * This port configuration is intended for testing the Trident
 * Warpcore SerDes without any external PHYs connected.  The setup
 * matches a lab system with cables hooked up for testing 40G, 20G
 * (DXGXS/RXAUI) and 10G-KR.  Created for a BCM56845-based system, but
 * also works with the latest BCM56846-based system.
 */

#include <board/board_config.h>
#include <board/sdk56840.h>
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
    {  40000,    0,   0,   1,   5  },
    {  40000,    0,   0,   2,   9  },
    {  40000,    0,   0,   3,  13  },
    {  40000,    0,   0,   4,  17  },
    {  40000,    0, HG2,   5,  21  },
    {  40000,    0, HG2,   6,  25  },
    {  40000,    0, HG2,   7,  29  },
    {  40000,    0, HG2,   8,  33  },
    {  20000,    0, HG2,   9,  37  },
    {  20000,    0, HG2,  10,  39  },
    {  20000,    0, HG2,  11,  41  },
    {  20000,    0, HG2,  12,  43  },
    {  20000,    0, HG2,  13,  45  },
    {  20000,    0, HG2,  14,  47  },
    {  20000,    0, HG2,  15,  49  },
    {  20000,    0, HG2,  16,  51  },
    {  10000,    0,   0,  17,  57  },
    {  10000,    0,   0,  18,  58  },
    {  10000,    0,   0,  19,  59  },
    {  10000,    0,   0,  20,  60  },
    {  10000,    0,   0,  21,  61  },
    {  10000,    0,   0,  22,  62  },
    {  10000,    0,   0,  23,  63  },
    {  10000,    0,   0,  24,  64  },
    {  10000,    0,   0,  25,  65  },
    {  10000,    0,   0,  26,  66  },
    {  10000,    0,   0,  27,  67  },
    {  10000,    0,   0,  28,  68  },
    {  10000,    0,   0,  29,  69  },
    {  10000,    0,   0,  30,  70  },
    {  10000,    0,   0,  31,  71  },
    {  10000,    0,   0,  32,  72  }
};

static int 
_phy_reset_cb(phy_ctrl_t *pc)
{
    int rv = CDK_E_NONE;
    phy_ctrl_t *lpc = pc;
    uint32_t tx_pol, rx_pol;

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
            /* Invert Rx polarity on all lanes */
            rx_pol = 0x1111;
            rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiRxPolInvert,
                                rx_pol, NULL);
            PHY_VERB(lpc, ("Flip Rx pol (0x%04"PRIx32")\n", rx_pol));
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

board_config_t board_bcm56845_svk = {
    "bcm56845_svk",
    _chip_configs,
    &_phy_reset_cb,
    &_phy_init_cb,
};
