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
#define _CHIP_DYN_CONFIG        4
#endif

static cdk_port_map_port_t _skip_ports[] = { -1 };

#ifndef _CHIP_DYN_CONFIG 
#define _CHIP_DYN_CONFIG        0
#endif

#define HG2 CDK_DCFG_PORT_MODE_HIGIG2

static cdk_port_config_t _port_configs[] = {
    /* speed flags mode  sys  app */
    {      0,    0,   0,  -1,  -1  },
    {  10000,    0,   0,   1,   1  },
    {  10000,    0,   0,   2,   5  },
    {  10000,    0,   0,   3,   9  },
    {  10000,    0,   0,   4,  13  },
    {  10000,    0,   0,   5,  17  },
    {  10000,    0,   0,   6,  21  },
    {  10000,    0,   0,   7,  27  },
    {  10000,    0,   0,   8,  28  },
    {  10000,    0,   0,   9,  30  },
    {  10000,    0,   0,  10,  33  },
    {   2500,    0,   0,  11,  42  }
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
            if (lpc->port == 28 || lpc->port == 30) {
                rx_pol = (lpc->port == 28) ? 0x0001 : 0x0100;
                rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiRxPolInvert,
                                    rx_pol, NULL);
                PHY_VERB(lpc, ("Flip Rx pol (0x%04"PRIx32")\n", rx_pol));
            }
        } else if (CDK_STRSTR(lpc->drv->drv_name, "unicore") != NULL) {
            if (lpc->port != 40) {
                if (lpc->port == 17 || lpc->port == 21) {
                    /* Invert Rx polarity on all lanes */
                    rx_pol = 0x1111;
                    rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiRxPolInvert,
                                        rx_pol, NULL);
                    PHY_VERB(lpc, ("Flip Rx pol (0x%04"PRIx32")\n", rx_pol));
                } else {
                    /* Invert Tx polarity on all lanes */
                    tx_pol = 0x1111;
                    rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiTxPolInvert,
                                        tx_pol, NULL);
                    PHY_VERB(lpc, ("Flip Rx pol (0x%04"PRIx32")\n", tx_pol));
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

board_config_t board_bcm56450_svk = {
    "bcm56450_svk",
    _chip_configs,
    &_phy_reset_cb,
    &_phy_init_cb,
};
