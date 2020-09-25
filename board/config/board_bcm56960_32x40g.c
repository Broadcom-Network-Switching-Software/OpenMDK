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

static cdk_port_map_port_t _skip_ports[] = { -1 };

#ifndef _CHIP_DYN_CONFIG 
#define _CHIP_DYN_CONFIG        0
#endif

#define HG2 CDK_DCFG_PORT_MODE_HIGIG2

static cdk_port_config_t _port_configs[] = {
    /* speed flags mode  sys  app */
    {      0,    0,   0,  -1,  -1  },
    {  40000,    0,   0,   1,   1  },
    {  40000,    0,   0,   2,   5  },
    {  40000,    0,   0,   3,   9  },
    {  40000,    0,   0,   4,  13  },
    {  40000,    0,   0,   5,  17  },
    {  40000,    0,   0,   6,  21  },
    {  40000,    0,   0,   7,  25  },
    {  40000,    0,   0,   8,  29  },
    {  40000,    0,   0,  34,  33  },
    {  40000,    0,   0,  35,  37  },
    {  40000,    0,   0,  36,  41  },
    {  40000,    0,   0,  37,  45  },
    {  40000,    0,   0,  38,  49  },
    {  40000,    0,   0,  39,  53  },
    {  40000,    0,   0,  40,  57  },
    {  40000,    0,   0,  41,  61  },
    {  40000,    0,   0,  68,  65  },
    {  40000,    0,   0,  69,  69  },
    {  40000,    0,   0,  70,  73  },
    {  40000,    0,   0,  71,  77  },
    {  40000,    0,   0,  72,  81  },
    {  40000,    0,   0,  73,  85  },
    {  40000,    0,   0,  74,  89  },
    {  40000,    0,   0,  75,  93  },
    {  40000,    0,   0, 102,  97  },
    {  40000,    0,   0, 103, 101  },
    {  40000,    0,   0, 104, 105  },
    {  40000,    0,   0, 105, 109  },
    {  40000,    0,   0, 106, 113  },
    {  40000,    0,   0, 107, 117  },
    {  40000,    0,   0, 108, 121  },
    {  40000,    0,   0, 109, 125  },
    {  10000,    0,   0,  66, 129  },
    {  10000,    0,   0, 100, 131  }
};                             


static int                     
_phy_reset_cb(phy_ctrl_t *pc)
{
    int rv = CDK_E_NONE;
    phy_ctrl_t *lpc = pc;
    int idx;
    uint32_t rx_map, tx_map;
    struct _port_polarity {
        int port;
        uint32_t pol;
    };   
    struct _port_polarity tx_pol[] = {
        { 21, 0x8}, { 29, 0x8}, { 33, 0x8}, { 41, 0x8}, 
        { 77, 0x2}, { 81, 0x8}, { 89, 0x8}, {101, 0x8},
        {109, 0x8}
    };
    struct _port_polarity rx_pol[] = {
        { 17, 0x2}, { 21, 0x5}, { 25, 0x2}, { 33, 0x8},
        { 41, 0x1}, { 45, 0x2}, { 77, 0x3}, { 85, 0x4},
        { 89, 0x8}, { 93, 0x4}, { 97, 0x8}, {101, 0x8},
        {105, 0x4}, {109, 0x8}
    };

    while (lpc) {
        if (lpc->drv == NULL || lpc->drv->drv_name == NULL) {
            return CDK_E_INTERNAL;
        }
        if (CDK_STRSTR(lpc->drv->drv_name, "tscf") != NULL) {
            /* Remap Tx/Rx lane */
            if (lpc->port == 73) {
                tx_map = 0x1032;
                rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiTxLaneRemap,
                                    tx_map, NULL);
                PHY_VERB(lpc, ("Remap Tx lanes (0x%04"PRIx32")\n", tx_map));
                
                rx_map = 0x1032;
                rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiRxLaneRemap,
                                    rx_map, NULL);
                PHY_VERB(lpc, ("Remap Rx lanes (0x%04"PRIx32")\n", rx_map));                
            }
            if (((PHY_CTRL_PHY_INST(lpc) & 0x3) == 0) && 
                ((lpc->port >=77 && lpc->port <= 93) || 
                (lpc->port >= 101 && lpc->port <= 113))) {
                rx_map = 0x2301;
                rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiRxLaneRemap,
                                    rx_map, NULL);
                PHY_VERB(lpc, ("Remap Rx lanes (0x%04"PRIx32")\n", rx_map));                
            }

            /* Invert Tx polarity */
            for (idx = 0; idx < COUNTOF(tx_pol); idx++) {
                if (lpc->port == tx_pol[idx].port) {
                    rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiTxPolInvert,
                                        tx_pol[idx].pol, NULL);
                    PHY_VERB(lpc, ("Flip Tx pol (0x%04"PRIx32")\n", 
                                   tx_pol[idx].pol));                    
                }
            }

            /* Invert Rx polarity */
            for (idx = 0; idx < COUNTOF(rx_pol); idx++) {
                if (lpc->port == rx_pol[idx].port) {
                    rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiRxPolInvert,
                                        rx_pol[idx].pol, NULL);
                    PHY_VERB(lpc, ("Flip Rx pol (0x%04"PRIx32")\n", 
                             rx_pol[idx].pol));                    
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

board_config_t board_bcm56960_32x40g = {
    "bcm56960_32x40g",
    _chip_configs,
    &_phy_reset_cb,
    &_phy_init_cb,
};
