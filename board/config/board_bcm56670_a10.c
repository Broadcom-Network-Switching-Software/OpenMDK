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
#include <cdk/cdk_debug.h>
#include <phy/phy_buslist.h>

static cdk_port_map_port_t _skip_ports[] = { -1 };

#if defined(CDK_CONFIG_INCLUDE_BCM56670_A0) && CDK_CONFIG_INCLUDE_BCM56670_A0 == 1
#include <cdk/chip/bcm56670_a0_defs.h>
#define _CHIP_DYN_CONFIG        DCFG_FREQ815
#endif

#ifndef _CHIP_DYN_CONFIG 
#define _CHIP_DYN_CONFIG        0x20000000
#endif

/* Set the port flag in flags */
#define OVP CDK_DCFG_PORT_F_OVERSUB

static cdk_port_config_t _port_configs[] = {
    /* speed flags mode  sys  app */
    {      0,    0,   0,  -1,  -1  },
    {  40000,    0,   0,   1,   1  },
    {  10000,    0,   0,   2,   5  },
    {  10000,    0,   0,   3,   6  },
    {  10000,    0,   0,   4,   7  },
    {  10000,    0,   0,   5,   8  },
    {  10000,    0,   0,   6,   9  },
    {  10000,    0,   0,   7,   10 },
    {  10000,    0,   0,   8,   11 },
    {  10000,    0,   0,   9,   12 },
    {  10000,    0,   0,  10,   13  },
    {  10000,    0,   0,  11,   14  },
    {  10000,    0,   0,  12,   15  },
    {  10000,    0,   0,  13,   16  },
    { 100000,    0,   0,  14,   17  },
    {  40000,    0,   0,  15,   21  },
    {  40000,    0,   0,  16,   25  },
    {  10000,    0,   0,  17,   29  },
    {  10000,    0,   0,  18,   33  },
    {  10000,    0,   0,  19,   34  },
    {  10000,    0,   0,  20,   35  },
    {  10000,    0,   0,  21,   36  },
    {  40000,    0,   0,  22,   37  },
    {  40000,    0,   0,  23,   41  },
    { 100000,    0,   0,  24,   45  },
    {  40000,    0,   0,  25,   49  },
    {  40000,    0,   0,  26,   53  },
    {  40000,    0,   0,  27,   57  },
    {  40000,    0,   0,  28,   61  },
};                             


static int                     
_phy_reset_cb(phy_ctrl_t *pc)
{
    int rv = CDK_E_NONE;
    phy_ctrl_t *lpc = pc;
    int idx;
    uint32_t rx_map, tx_map, tx_pol, rx_pol;
    int tx_pol_port[] = { 4, 16, 17, 19, 20, 23, 25, 29, 30, 31, 32, 37, 38, 40,
                         42, 43, 44, 49, 51, 52, 53, 55, 57, 59, 61, 62, 63};
    int rx_pol_port[] = { 2,  4,  6,  8, 10, 12, 13, 17, 18, 19, 21, 23, 24, 26,
                         27, 28, 29, 30, 31, 32, 37, 39, 40, 42, 43, 44, 45, 49,
                         50, 51, 53, 55, 57, 58, 59, 61, 63};

    while (lpc) {
        if (lpc->drv == NULL || lpc->drv->drv_name == NULL) {
            return CDK_E_INTERNAL;
        }
        {
            /* Remap Tx/Rx lane */
            if (lpc->port == 1 || lpc->port == 5 || lpc->port == 9) {
                rx_map = 0x3210;
                rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiRxLaneRemap,
                                    rx_map, NULL);
                PHY_VERB(lpc, ("Remap Rx lanes (0x%04"PRIx32")\n", rx_map));                
            } else if (lpc->port == 13 || lpc->port == 37) {
                rx_map = 0x1302;
                rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiRxLaneRemap,
                                    rx_map, NULL);
                PHY_VERB(lpc, ("Remap Rx lanes (0x%04"PRIx32")\n", rx_map));                
            } else if (lpc->port == 17 ||lpc->port == 41) {
                rx_map = 0x2130;
                rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiRxLaneRemap,
                                    rx_map, NULL);
                PHY_VERB(lpc, ("Remap Rx lanes (0x%04"PRIx32")\n", rx_map));                
            } else if (lpc->port == 21 ||lpc->port == 45) {
                rx_map = 0x2031;
                rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiRxLaneRemap,
                                    rx_map, NULL);
                PHY_VERB(lpc, ("Remap Rx lanes (0x%04"PRIx32")\n", rx_map));                
            } else if (lpc->port == 25 ||lpc->port == 49) {
                rx_map = 0x1203;
                rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiRxLaneRemap,
                                    rx_map, NULL);
                PHY_VERB(lpc, ("Remap Rx lanes (0x%04"PRIx32")\n", rx_map));                
            } else if (lpc->port == 29) {
                rx_map = 0x2310;
                rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiRxLaneRemap,
                                    rx_map, NULL);
                PHY_VERB(lpc, ("Remap Rx lanes (0x%04"PRIx32")\n", rx_map));                
            } else if (lpc->port == 33) {
                rx_map = 0x1023;
                rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiRxLaneRemap,
                                    rx_map, NULL);
                PHY_VERB(lpc, ("Remap Rx lanes (0x%04"PRIx32")\n", rx_map));                
            } else if (lpc->port == 53 ||lpc->port == 57||lpc->port == 61) {
                rx_map = 0x1032;
                rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiRxLaneRemap,
                                    rx_map, NULL);
                PHY_VERB(lpc, ("Remap Rx lanes (0x%04"PRIx32")\n", rx_map));                
            }
            
            
            if (lpc->port == 1 || lpc->port == 5 || lpc->port == 9) {
                tx_map = 0x3210;
                rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiTxLaneRemap,
                                    tx_map, NULL);
                PHY_VERB(lpc, ("Remap Tx lanes (0x%04"PRIx32")\n", tx_map));      
            } else if (lpc->port == 17 || lpc->port == 41) {
                tx_map = 0x0213;
                rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiTxLaneRemap,
                                    tx_map, NULL);
                PHY_VERB(lpc, ("Remap Tx lanes (0x%04"PRIx32")\n", tx_map));      
            } else if ( lpc->port == 21 || lpc->port == 45) {
                tx_map = 0x1302;
                rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiTxLaneRemap,
                                    tx_map, NULL);
                PHY_VERB(lpc, ("Remap Tx lanes (0x%04"PRIx32")\n", tx_map));      
            } else if (lpc->port == 25 || lpc->port == 49) {
                tx_map = 0x3120;
                rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiTxLaneRemap,
                                    tx_map, NULL);
                PHY_VERB(lpc, ("Remap Tx lanes (0x%04"PRIx32")\n", tx_map));      
            } else if (lpc->port == 29) {
                tx_map = 0x3201;
                rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiTxLaneRemap,
                                    tx_map, NULL);
                PHY_VERB(lpc, ("Remap Tx lanes (0x%04"PRIx32")\n", tx_map));      
            } else if (lpc->port == 33) {
                tx_map = 0x0132;
                rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiTxLaneRemap,
                                    tx_map, NULL);
                PHY_VERB(lpc, ("Remap Tx lanes (0x%04"PRIx32")\n", tx_map));      
            } else if (lpc->port == 53 || lpc->port == 57 || lpc->port == 61) {
                tx_map = 0x1032;
                rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiTxLaneRemap,
                                    tx_map, NULL);
                PHY_VERB(lpc, ("Remap Tx lanes (0x%04"PRIx32")\n", tx_map));      
            }
        }
        {
            /* Invert Tx polarity */
            for (idx = 0; idx < COUNTOF(tx_pol_port); idx++) {
                if (lpc->port == tx_pol_port[idx]) {
                    tx_pol = 0x1;
                    rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiTxPolInvert,
                                        tx_pol , NULL);
                    PHY_VERB(lpc, ("Flip Tx pol (0x%04"PRIx32")\n", tx_pol));
                }
            }

            /* Invert Rx polarity */
            for (idx = 0; idx < COUNTOF(rx_pol_port); idx++) {
                if (lpc->port == rx_pol_port[idx]) {
                    rx_pol = 0x1;
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
    NULL, /* _phy_bus, */
    NULL,
    _CHIP_DYN_CONFIG,
    COUNTOF(_port_configs),
    _port_configs
};

static board_chip_config_t *_chip_configs[] = {
    &_chip_config,
    NULL
};

board_config_t board_bcm56670_a10 = {
    "mn_a10",
    _chip_configs,
    &_phy_reset_cb,
    &_phy_init_cb,
};
