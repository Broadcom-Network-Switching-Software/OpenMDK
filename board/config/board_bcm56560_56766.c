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

#ifndef _CHIP_DYN_CONFIG 
#define _CHIP_DYN_CONFIG        0
#endif

/* Set the port flag in flags */
#define OVP CDK_DCFG_PORT_F_OVERSUB

static cdk_port_config_t _port_configs[] = {
    /* speed flags mode  sys  app */
    {      0,    0,   0,  -1,  -1  },
    {  10000,    OVP,   0,   1,   1  },
    {  10000,    OVP,   0,   2,   2  },
    {  10000,    OVP,   0,   3,   3  },
    {  10000,    OVP,   0,   4,   4  },
    {  10000,    OVP,   0,   5,   5  },
    {  10000,    OVP,   0,   6,   6  },
    {  10000,    OVP,   0,   7,   7  },
    {  10000,    OVP,   0,   8,   8  },
    {  10000,    OVP,   0,   9,   9  },
    {  10000,    OVP,   0,  10,  10  },
    {  10000,    OVP,   0,  11,  11  },
    {  10000,    OVP,   0,  12,  12  },
    {  10000,    OVP,   0,  13,  13  },
    {  10000,    OVP,   0,  14,  14  },
    {  10000,    OVP,   0,  15,  15  },
    {  10000,    OVP,   0,  16,  16  },
    {  10000,    OVP,   0,  17,  17  },
    {  10000,    OVP,   0,  18,  18  },
    {  10000,    OVP,   0,  19,  19  },
    {  10000,    OVP,   0,  20,  20  },
    {  10000,    OVP,   0,  21,  21  },
    {  10000,    OVP,   0,  22,  22  },
    {  10000,    OVP,   0,  23,  23  },
    {  10000,    OVP,   0,  24,  24  },
    {  10000,    OVP,   0,  25,  25  },
    {  10000,    OVP,   0,  26,  26  },
    {  10000,    OVP,   0,  27,  27  },
    {  10000,    OVP,   0,  28,  28  },
    {  10000,    OVP,   0,  29,  29  },
    {  10000,    OVP,   0,  30,  30  },
    {  10000,    OVP,   0,  31,  31  },
    {  10000,    OVP,   0,  32,  32  },
    {  10000,    OVP,   0,  33,  33  },
    {  10000,    OVP,   0,  34,  34  },
    {  10000,    OVP,   0,  35,  35  },
    {  10000,    OVP,   0,  36,  36  },
    {  10000,    OVP,   0,  37,  37  },
    {  10000,    OVP,   0,  38,  38  },
    {  10000,    OVP,   0,  39,  39  },
    {  10000,    OVP,   0,  40,  40  },
    {  10000,    OVP,   0,  41,  41  },
    {  10000,    OVP,   0,  42,  42  },
    {  10000,    OVP,   0,  43,  43  },
    {  10000,    OVP,   0,  44,  44  },
    {  10000,    OVP,   0,  45,  45  },
    {  10000,    OVP,   0,  46,  46  },
    {  10000,    OVP,   0,  47,  47  },
    {  10000,    OVP,   0,  48,  48  },
    {  10000,    OVP,   0,  49,  49  },
    {  10000,    OVP,   0,  50,  50  },
    {  10000,    OVP,   0,  51,  51  },
    {  10000,    OVP,   0,  52,  52  },
    {  10000,    OVP,   0,  53,  53  },
    {  10000,    OVP,   0,  54,  54  },
    {  10000,    OVP,   0,  55,  55  },
    {  10000,    OVP,   0,  56,  56  },
    {  10000,    OVP,   0,  57,  57  },
    {  10000,    OVP,   0,  58,  58  },
    {  10000,    OVP,   0,  59,  59  },
    {  10000,    OVP,   0,  60,  60  },
    {  10000,    OVP,   0,  61,  61  },
    {  10000,    OVP,   0,  62,  62  },
    {  10000,    OVP,   0,  63,  63  },
    {  10000,    OVP,   0,  64,  64  },
    {  10000,    OVP,   0,  65,  65  },
    {  10000,    OVP,   0,  66,  66  },
    {  10000,    OVP,   0,  67,  67  },
    {  10000,    OVP,   0,  68,  68  },
    {  10000,    OVP,   0,  69,  69  },
    {  10000,    OVP,   0,  70,  70  },
    {  10000,    OVP,   0,  71,  71  },
    {  10000,    OVP,   0,  72,  72  }
};                             


static int                     
_phy_reset_cb(phy_ctrl_t *pc)
{
    int rv = CDK_E_NONE;
    phy_ctrl_t *lpc = pc;
    int idx;
    uint32_t rx_map, tx_map, tx_pol, rx_pol;
    int tx_pol_port[] = {25, 26, 27, 28, 29, 30, 31, 32, 37, 38, 39, 40, 41, 42,
                         43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56,
                         57, 58, 59, 60, 68, 69, 70, 71, 72};
    int rx_pol_port[] = {25, 26, 27, 28, 29, 30, 31, 32, 37, 38, 39, 40, 41, 42,
                         43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56,
                         57, 58, 59, 60, 65, 66, 69, 70, 71, 72};

    while (lpc) {
        if (lpc->drv == NULL || lpc->drv->drv_name == NULL) {
            return CDK_E_INTERNAL;
        }
        {
            /* Remap Tx/Rx lane */
            if (lpc->port == 21 || lpc->port == 57) {
                rx_map = 0x0123;
                rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiRxLaneRemap,
                                    rx_map, NULL);
                PHY_VERB(lpc, ("Remap Rx lanes (0x%04"PRIx32")\n", rx_map));                
            } else if (lpc->port == 29 || lpc->port == 37) {
                rx_map = 0x2031;
                rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiRxLaneRemap,
                                    rx_map, NULL);
                PHY_VERB(lpc, ("Remap Rx lanes (0x%04"PRIx32")\n", rx_map));                
            } else if (lpc->port == 65 ||lpc->port == 69) {
                rx_map = 0x0213;
                rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiRxLaneRemap,
                                    rx_map, NULL);
                PHY_VERB(lpc, ("Remap Rx lanes (0x%04"PRIx32")\n", rx_map));                
            }
            
            
            if (lpc->port == 1 || lpc->port == 5 || lpc->port == 9 || lpc->port == 13 
             || lpc->port == 17 || lpc->port == 25 || lpc->port == 41 || lpc->port == 45 
             || lpc->port == 49 || lpc->port == 53 || lpc->port == 61) {
                tx_map = 0x0123;
                rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiTxLaneRemap,
                                    tx_map, NULL);
                PHY_VERB(lpc, ("Remap Tx lanes (0x%04"PRIx32")\n", tx_map));      
            } else if (lpc->port == 29 || lpc->port == 37) {
                tx_map = 0x1302;
                rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiTxLaneRemap,
                                    tx_map, NULL);
                PHY_VERB(lpc, ("Remap Tx lanes (0x%04"PRIx32")\n", tx_map));      
            } else if ( lpc->port == 33) {
                tx_map = 0x1032;
                rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiTxLaneRemap,
                                    tx_map, NULL);
                PHY_VERB(lpc, ("Remap Tx lanes (0x%04"PRIx32")\n", tx_map));      
            } else if (lpc->port == 65) {
                tx_map = 0x3102;
                rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiTxLaneRemap,
                                    tx_map, NULL);
                PHY_VERB(lpc, ("Remap Tx lanes (0x%04"PRIx32")\n", tx_map));      
            } else if (lpc->port == 69) {
                tx_map = 0x3120;
                rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiTxLaneRemap,
                                    tx_map, NULL);
                PHY_VERB(lpc, ("Remap Tx lanes (0x%04"PRIx32")\n", tx_map));      
            }        }
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

board_config_t board_bcm56560_56766 = {
    "bcm56560_56766",
    _chip_configs,
    &_phy_reset_cb,
    &_phy_init_cb,
};
