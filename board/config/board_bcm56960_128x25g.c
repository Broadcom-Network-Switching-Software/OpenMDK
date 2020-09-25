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
    {  25000,    0,   0,   1,   1  },
    {  25000,    0,   0,   2,   2  },
    {  25000,    0,   0,   3,   3  },
    {  25000,    0,   0,   4,   4  },
    {  25000,    0,   0,   5,   5  },
    {  25000,    0,   0,   6,   6  },
    {  25000,    0,   0,   7,   7  },
    {  25000,    0,   0,   8,   8  },
    {  25000,    0,   0,   9,   9  },
    {  25000,    0,   0,  10,  10  },
    {  25000,    0,   0,  11,  11  },
    {  25000,    0,   0,  12,  12  },
    {  25000,    0,   0,  13,  13  },
    {  25000,    0,   0,  14,  14  },
    {  25000,    0,   0,  15,  15  },
    {  25000,    0,   0,  16,  16  },
    {  25000,    0,   0,  17,  17  },
    {  25000,    0,   0,  18,  18  },
    {  25000,    0,   0,  19,  19  },
    {  25000,    0,   0,  20,  20  },
    {  25000,    0,   0,  21,  21  },
    {  25000,    0,   0,  22,  22  },
    {  25000,    0,   0,  23,  23  },
    {  25000,    0,   0,  24,  24  },
    {  25000,    0,   0,  25,  25  },
    {  25000,    0,   0,  26,  26  },
    {  25000,    0,   0,  27,  27  },
    {  25000,    0,   0,  28,  28  },
    {  25000,    0,   0,  29,  29  },
    {  25000,    0,   0,  30,  30  },
    {  25000,    0,   0,  31,  31  },
    {  25000,    0,   0,  32,  32  },
    {  25000,    0,   0,  34,  33  },
    {  25000,    0,   0,  35,  34  },
    {  25000,    0,   0,  36,  35  },
    {  25000,    0,   0,  37,  36  },
    {  25000,    0,   0,  38,  37  },
    {  25000,    0,   0,  39,  38  },
    {  25000,    0,   0,  40,  39  },
    {  25000,    0,   0,  41,  40  },
    {  25000,    0,   0,  42,  41  },
    {  25000,    0,   0,  43,  42  },
    {  25000,    0,   0,  44,  43  },
    {  25000,    0,   0,  45,  44  },
    {  25000,    0,   0,  46,  45  },
    {  25000,    0,   0,  47,  46  },
    {  25000,    0,   0,  48,  47  },
    {  25000,    0,   0,  49,  48  },
    {  25000,    0,   0,  50,  49  },
    {  25000,    0,   0,  51,  50  },
    {  25000,    0,   0,  52,  51  },
    {  25000,    0,   0,  53,  52  },
    {  25000,    0,   0,  54,  53  },
    {  25000,    0,   0,  55,  54  },
    {  25000,    0,   0,  56,  55  },
    {  25000,    0,   0,  57,  56  },
    {  25000,    0,   0,  58,  57  },
    {  25000,    0,   0,  59,  58  },
    {  25000,    0,   0,  60,  59  },
    {  25000,    0,   0,  61,  60  },
    {  25000,    0,   0,  62,  61  },
    {  25000,    0,   0,  63,  62  },
    {  25000,    0,   0,  64,  63  },
    {  25000,    0,   0,  65,  64  },
    {  25000,    0,   0,  68,  65  },
    {  25000,    0,   0,  69,  66  },
    {  25000,    0,   0,  70,  67  },
    {  25000,    0,   0,  71,  68  },
    {  25000,    0,   0,  72,  69  },
    {  25000,    0,   0,  73,  70  },
    {  25000,    0,   0,  74,  71  },
    {  25000,    0,   0,  75,  72  },
    {  25000,    0,   0,  76,  73  },
    {  25000,    0,   0,  77,  74  },
    {  25000,    0,   0,  78,  75  },
    {  25000,    0,   0,  79,  76  },
    {  25000,    0,   0,  80,  77  },
    {  25000,    0,   0,  81,  78  },
    {  25000,    0,   0,  82,  79  },
    {  25000,    0,   0,  83,  80  },
    {  25000,    0,   0,  84,  81  },
    {  25000,    0,   0,  85,  82  },
    {  25000,    0,   0,  86,  83  },
    {  25000,    0,   0,  87,  84  },
    {  25000,    0,   0,  88,  85  },
    {  25000,    0,   0,  89,  86  },
    {  25000,    0,   0,  90,  87  },
    {  25000,    0,   0,  91,  88  },
    {  25000,    0,   0,  92,  89  },
    {  25000,    0,   0,  93,  90  },
    {  25000,    0,   0,  94,  91  },
    {  25000,    0,   0,  95,  92  },
    {  25000,    0,   0,  96,  93  },
    {  25000,    0,   0,  97,  94  },
    {  25000,    0,   0,  98,  95  },
    {  25000,    0,   0,  99,  96  },    
    {  25000,    0,   0, 102,  97  },
    {  25000,    0,   0, 103,  98  },
    {  25000,    0,   0, 104,  99  },
    {  25000,    0,   0, 105, 100  },
    {  25000,    0,   0, 106, 101  },
    {  25000,    0,   0, 107, 102  },
    {  25000,    0,   0, 108, 103  },
    {  25000,    0,   0, 109, 104  },
    {  25000,    0,   0, 110, 105  },
    {  25000,    0,   0, 111, 106  },
    {  25000,    0,   0, 112, 107  },
    {  25000,    0,   0, 113, 108  },
    {  25000,    0,   0, 114, 109  },
    {  25000,    0,   0, 115, 110  },
    {  25000,    0,   0, 116, 111  },
    {  25000,    0,   0, 117, 112  },
    {  25000,    0,   0, 118, 113  },
    {  25000,    0,   0, 119, 114  },
    {  25000,    0,   0, 120, 115  },
    {  25000,    0,   0, 121, 116  },
    {  25000,    0,   0, 122, 117  },
    {  25000,    0,   0, 123, 118  },
    {  25000,    0,   0, 124, 119  },
    {  25000,    0,   0, 125, 120  },
    {  25000,    0,   0, 126, 121  },
    {  25000,    0,   0, 127, 122  },
    {  25000,    0,   0, 128, 123  },
    {  25000,    0,   0, 129, 124  },
    {  25000,    0,   0, 130, 125  },
    {  25000,    0,   0, 131, 126  },
    {  25000,    0,   0, 132, 127  },
    {  25000,    0,   0, 133, 128  },
    {  10000,    0,   0,  66, 129  },
    {  10000,    0,   0, 100, 131  }
};                             


static int                     
_phy_reset_cb(phy_ctrl_t *pc)
{
    int rv = CDK_E_NONE;
    phy_ctrl_t *lpc = pc;
    int idx;
    uint32_t rx_map, tx_map, tx_pol, rx_pol;
    int tx_pol_port[] = {24, 32, 36, 44, 78, 84, 92, 104, 112};
    int rx_pol_port[] = {18, 21, 23, 26, 36, 41, 46, 77, 78, 87, 92, 95, 
                         99, 104, 107, 112};

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
            if (lpc->port >= 77 && lpc->port <= 113 && 
                (PHY_CTRL_PHY_INST(lpc) & 0x3) == 0) {
                rx_map = 0x2301;
                rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiRxLaneRemap,
                                    rx_map, NULL);
                PHY_VERB(lpc, ("Remap Rx lanes (0x%04"PRIx32")\n", rx_map));                
            }

            /* Invert Tx polarity */
            for (idx = 0; idx < COUNTOF(tx_pol_port); idx++) {
                if (lpc->port == tx_pol_port[idx]) {
                    tx_pol = 0x1;
                    rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiTxPolInvert,
                                        tx_pol, NULL);
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

board_config_t board_bcm56960_128x25g = {
    "bcm56960_128x25g",
    _chip_configs,
    &_phy_reset_cb,
    &_phy_init_cb,
};
