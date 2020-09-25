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

#if defined(CDK_CONFIG_INCLUDE_BCM56450_A0) && CDK_CONFIG_INCLUDE_BCM56450_A0 == 1
#include <cdk/chip/bcm56450_a0_defs.h>
#define _CHIP_DYN_CONFIG        9
#endif

static cdk_port_map_port_t _skip_ports[] = { -1 };

#ifndef _CHIP_DYN_CONFIG 
#define _CHIP_DYN_CONFIG        0
#endif

#define HG2 CDK_DCFG_PORT_MODE_HIGIG2

static cdk_port_config_t _port_configs[] = {
    /* speed flags mode  sys  app */
    {      0,    0,   0,  -1,  -1  },
    {   1000,    0,   0,   1,   1  },
    {   1000,    0,   0,   2,   2  },
    {   1000,    0,   0,   3,   3  },
    {   1000,    0,   0,   4,   4  },
    {   1000,    0,   0,   5,   5  },
    {   1000,    0,   0,   6,   6  },
    {   1000,    0,   0,   7,   7  },
    {   1000,    0,   0,   8,   8  },
    {   1000,    0,   0,   9,   9  },
    {   1000,    0,   0,  10,  10  },
    {   1000,    0,   0,  11,  11  },
    {   1000,    0,   0,  12,  12  },
    {   1000,    0,   0,  13,  13  },
    {   1000,    0,   0,  14,  14  },
    {   1000,    0,   0,  15,  15  },
    {   1000,    0,   0,  16,  16  },
    {   1000,    0,   0,  17,  17  },
    {   1000,    0,   0,  18,  18  },
    {   1000,    0,   0,  19,  19  },
    {   1000,    0,   0,  20,  20  },
    {   1000,    0,   0,  21,  21  },
    {   1000,    0,   0,  22,  22  },
    {   1000,    0,   0,  23,  23  },
    {   1000,    0,   0,  24,  24  },
    {  13000,    0,   0,  25,  25  },
    {  13000,    0,   0,  26,  26  },
    {  21000,    0,   0,  27,  27  },
    {  21000,    0,   0,  28,  28  },
    {   2500,    0,   0,  29,  42  }
};                             

static int                     
_phy_reset_cb(phy_ctrl_t *pc)
{
    int rv = CDK_E_NONE;
    phy_ctrl_t *lpc = pc;
    uint32_t rx_pol, tx_pol;

    while (lpc) {
        if (lpc->drv == NULL || lpc->drv->drv_name == NULL) {
            return CDK_E_INTERNAL;
        }

        if (CDK_STRSTR(lpc->drv->drv_name, "warpcore") != NULL) {
            /* Invert Rx polarity */
            if (lpc->port == 28) {
                rx_pol = 0x0001;
                rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiRxPolInvert,
                                    rx_pol, NULL);
                PHY_VERB(lpc, ("Flip Rx pol (0x%04"PRIx32")\n", rx_pol));
            }
        } else if (CDK_STRSTR(lpc->drv->drv_name, "unicore") != NULL) {
            if (lpc->port != 40) {
                if (lpc->port <= 16) {
                    /* Invert Tx polarity on all lanes */
                    tx_pol = 0x1111;
                    rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiTxPolInvert,
                                        tx_pol, NULL);
                    PHY_VERB(lpc, ("Flip Rx pol (0x%04"PRIx32")\n", tx_pol));
                } else {
                    /* Invert Rx polarity on all lanes */
                    rx_pol = 0x1111;
                    rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiRxPolInvert,
                                        rx_pol, NULL);
                    PHY_VERB(lpc, ("Flip Rx pol (0x%04"PRIx32")\n", rx_pol));
                }
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

board_config_t board_bcm56450_svk2 = {
    "bcm56450_svk2",
    _chip_configs,
    &_phy_reset_cb,
    &_phy_init_cb,
};
