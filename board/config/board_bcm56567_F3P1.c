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
    {  10000,    OVP,   0,   1,   9  },
    {  10000,    OVP,   0,   2,   10 },
    {  10000,    OVP,   0,   3,   11 },
    {  10000,    OVP,   0,   4,   12 },
    {  10000,    OVP,   0,   5,   13 },
    {  10000,    OVP,   0,   6,   14 },
    {  10000,    OVP,   0,   7,   15 },
    {  10000,    OVP,   0,   8,   16 },
    {  10000,    OVP,   0,   9,   17 },
    {  10000,    OVP,   0,  10,   18  },
    {  10000,    OVP,   0,  11,   19  },
    {  10000,    OVP,   0,  12,   20  },
    {  10000,    OVP,   0,  13,  37  },
    {  10000,    OVP,   0,  14,  38  },
    {  10000,    OVP,   0,  15,  39  },
    {  10000,    OVP,   0,  16,  40  },
    {  10000,    OVP,   0,  17,  41  },
    {  10000,    OVP,   0,  18,  42  },
    {  10000,    OVP,   0,  19,  43  },
    {  10000,    OVP,   0,  20,  44  },
    {  10000,    OVP,   0,  21,  45  },
    {  10000,    OVP,   0,  22,  46  },
    {  10000,    OVP,   0,  23,  47  },
    {  10000,    OVP,   0,  24,  48  },
    {  10000,    OVP,   0,  25,  49  },
    {  10000,    OVP,   0,  26,  50  },
    {  10000,    OVP,   0,  27,  51  },
    {  10000,    OVP,   0,  28,  52  },
    {  10000,    OVP,   0,  29,  53  },
    {  10000,    OVP,   0,  30,  54  },
    {  10000,    OVP,   0,  31,  55  },
    {  10000,    OVP,   0,  32,  56  },
    {  10000,    OVP,   0,  33,  57  },
    {  10000,    OVP,   0,  34,  58  },
    {  10000,    OVP,   0,  35,  59  },
    {  10000,    OVP,   0,  36,  60  },
    {  10000,    OVP,   0,  37,  61 },
    {  10000,    OVP,   0,  38,  62 },
    {  10000,    OVP,   0,  39,  63 },
    {  10000,    OVP,   0,  40,  64 },
    {  10000,    OVP,   0,  41,   1  },
    {  10000,    OVP,   0,  42,   2  },
    {  10000,    OVP,   0,  43,   3  },
    {  10000,    OVP,   0,  44,   4  },
    {  10000,    OVP,   0,  45,   5  },
    {  10000,    OVP,   0,  46,   6  },
    {  10000,    OVP,   0,  47,   7  },
    {  10000,    OVP,   0,  48,   8  },
    {  42000,    OVP,   0,  49,  21  },
    {  0,        OVP,   0,  50,  22  },
    {  0,        OVP,   0,  51,  23  },
    {  0,        OVP,   0,  52,  24  },
    {  42000,    OVP,   0,  53,  25  },
    {      0,    OVP,   0,  54,  26  },
    {      0,    OVP,   0,  55,  27  },
    {      0,    OVP,   0,  56,  28  },
    {  42000,    OVP,   0,  57,  29  },
    {      0,    OVP,   0,  58,  30  },
    {  42000,    OVP,   0,  61,  33  },
    {      0,    OVP,   0,  62,  34  },
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
    int tx_pol_port[] = {10, 11};
    int rx_pol_port[] = {10, 11};

    while (lpc) {
        if (lpc->drv == NULL || lpc->drv->drv_name == NULL) {
            return CDK_E_INTERNAL;
        }
        {
            /* Remap Tx/Rx lane */
            if (lpc->port == 1 || lpc->port == 5 || lpc->port == 21 || lpc->port == 33
             || lpc->port == 57 || lpc->port == 69) {
                rx_map = 0x3012;
                rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiRxLaneRemap,
                                    rx_map, NULL);
                PHY_VERB(lpc, ("Remap Rx lanes (0x%04"PRIx32")\n", rx_map));                
            } else if (lpc->port == 9 || lpc->port == 13 || lpc->port == 17 || lpc->port == 25
                    || lpc->port == 29 || lpc->port == 37 || lpc->port == 41 || lpc->port == 45
                    || lpc->port == 49 || lpc->port == 53 || lpc->port == 61 || lpc->port == 65) {
                rx_map = 0x0321;
                rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiRxLaneRemap,
                                    rx_map, NULL);
                PHY_VERB(lpc, ("Remap Rx lanes (0x%04"PRIx32")\n", rx_map));                
            }
            
            if (lpc->port == 1 || lpc->port == 5 || lpc->port == 33 || lpc->port == 69) {
                tx_map = 0x0123;
                rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiTxLaneRemap,
                                    tx_map, NULL);
                PHY_VERB(lpc, ("Remap Tx lanes (0x%04"PRIx32")\n", tx_map));      
            } else if (lpc->port == 9 || lpc->port == 29 || lpc->port == 45 || lpc->port == 53
                    || lpc->port == 65) {
                tx_map = 0x1230;
                rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiTxLaneRemap,
                                    tx_map, NULL);
                PHY_VERB(lpc, ("Remap Tx lanes (0x%04"PRIx32")\n", tx_map));      
            } else if (lpc->port == 13 || lpc->port == 17 || lpc->port == 25 || lpc->port == 37
                    || lpc->port == 41 || lpc->port == 49 || lpc->port == 61) {
                tx_map = 0x3210;
                rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiTxLaneRemap,
                                    tx_map, NULL);
                PHY_VERB(lpc, ("Remap Tx lanes (0x%04"PRIx32")\n", tx_map));      
            } else if (lpc->port == 21 || lpc->port == 57) {
                tx_map = 0x2103;
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

board_config_t board_bcm56567_F3P1 = {
    "bcm56567_F3P1",
    _chip_configs,
    &_phy_reset_cb,
    &_phy_init_cb,
};
