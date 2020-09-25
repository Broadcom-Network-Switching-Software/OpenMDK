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
    {  40000,    0,   0,   2,   3  },
    {  40000,    0,   0,   3,   5  },
    {  40000,    0,   0,   4,   7  },
    {  40000,    0,   0,   5,   9  },
    {  40000,    0,   0,   6,  11  },
    {  40000,    0,   0,   7,  13  },
    {  40000,    0,   0,   8,  15  },
    {  40000,    0,   0,   9,  17  },
    {  40000,    0,   0,  10,  19  },
    {  40000,    0,   0,  11,  21  },
    {  40000,    0,   0,  12,  23  },
    {  40000,    0,   0,  13,  25  },
    {  40000,    0,   0,  14,  27  },
    {  40000,    0,   0,  15,  29  },
    {  40000,    0,   0,  16,  31  },
    {  40000,    0,   0,  34,  33  },
    {  40000,    0,   0,  35,  35  },
    {  40000,    0,   0,  36,  37  },
    {  40000,    0,   0,  37,  39  },
    {  40000,    0,   0,  38,  41  },
    {  40000,    0,   0,  39,  43  },
    {  40000,    0,   0,  40,  45  },
    {  40000,    0,   0,  41,  47  },
    {  40000,    0,   0,  42,  49  },
    {  40000,    0,   0,  43,  51  },
    {  40000,    0,   0,  44,  53  },
    {  40000,    0,   0,  45,  55  },
    {  40000,    0,   0,  46,  57  },
    {  40000,    0,   0,  47,  59  },
    {  40000,    0,   0,  48,  61  },
    {  40000,    0,   0,  49,  63  },
    {  40000,    0,   0,  68,  65  },
    {  40000,    0,   0,  69,  67  },
    {  40000,    0,   0,  70,  69  },
    {  40000,    0,   0,  71,  71  },
    {  40000,    0,   0,  72,  73  },
    {  40000,    0,   0,  73,  75  },
    {  40000,    0,   0,  74,  77  },
    {  40000,    0,   0,  75,  79  },
    {  40000,    0,   0,  76,  81  },
    {  40000,    0,   0,  77,  83  },
    {  40000,    0,   0,  78,  85  },
    {  40000,    0,   0,  79,  87  },
    {  40000,    0,   0,  80,  89  },
    {  40000,    0,   0,  81,  91  },
    {  40000,    0,   0,  82,  93  },
    {  40000,    0,   0,  83,  95  },
    {  40000,    0,   0, 102,  97  },
    {  40000,    0,   0, 103,  99  },
    {  40000,    0,   0, 104, 101  },
    {  40000,    0,   0, 105, 103  },
    {  40000,    0,   0, 106, 105  },
    {  40000,    0,   0, 107, 107  },
    {  40000,    0,   0, 108, 109  },
    {  40000,    0,   0, 109, 111  },
    {  40000,    0,   0, 110, 113  },
    {  40000,    0,   0, 111, 115  },
    {  40000,    0,   0, 112, 117  },
    {  40000,    0,   0, 113, 119  },
    {  40000,    0,   0, 114, 121  },
    {  40000,    0,   0, 115, 123  },
    {  40000,    0,   0, 116, 125  },
    {  40000,    0,   0, 117, 127  },
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
        { 23, 0x2}, { 31, 0x2}, { 35, 0x2}, { 43, 0x2}, 
        { 77, 0x2}, { 83, 0x2}, { 91, 0x2}, {103, 0x2},
        {111, 0x2}
    };
    struct _port_polarity rx_pol[] = {
        { 17, 0x2}, { 21, 0x1}, { 23, 0x1}, { 25, 0x2}, 
        { 35, 0x2}, { 41, 0x1}, { 45, 0x2}, { 77, 0x3}, 
        { 87, 0x1}, { 91, 0x2}, { 95, 0x1}, { 99, 0x1}, 
        {103, 0x2}, {107, 0x1}, {111, 0x2}
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
            if (lpc->port >=77 && lpc->port <= 113 && 
                (PHY_CTRL_PHY_INST(lpc) & 0x3) == 0) {
                rx_map = 0x2310;
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

board_config_t board_bcm56960_64x40g = {
    "bcm56960_64x40g",
    _chip_configs,
    &_phy_reset_cb,
    &_phy_init_cb,
};
