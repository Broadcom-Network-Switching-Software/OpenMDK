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
    {  10000,    0,   0,   1,   1  },
    {  10000,    0,   0,   2,   2  },
    {  10000,    0,   0,   3,   3  },
    {  10000,    0,   0,   4,   4  },
    {  10000,    0,   0,   5,   5  },
    {  10000,    0,   0,   6,   6  },
    {  10000,    0,   0,   7,   7  },
    {  10000,    0,   0,   8,   8  },
    {  10000,    0,   0,   9,   9  },
    {  10000,    0,   0,  10,  10  },
    {  10000,    0,   0,  11,  11  },
    {  10000,    0,   0,  12,  12  },
    {  10000,    0,   0,  13,  13  },
    {  10000,    0,   0,  14,  14  },
    {  10000,    0,   0,  15,  15  },
    {  10000,    0,   0,  16,  16  },
    {  10000,    0,   0,  17,  17  },
    {  10000,    0,   0,  18,  18  },
    {  10000,    0,   0,  19,  19  },
    {  10000,    0,   0,  20,  20  },
    {  10000,    0,   0,  21,  21  },
    {  10000,    0,   0,  22,  22  },
    {  10000,    0,   0,  23,  23  },
    {  10000,    0,   0,  24,  24  },
    {  10000,    0,   0,  25,  25  },
    {  10000,    0,   0,  26,  26  },
    {  10000,    0,   0,  27,  27  },
    {  10000,    0,   0,  28,  28  },
    {  10000,    0,   0,  29,  29  },
    {  10000,    0,   0,  30,  30  },
    {  10000,    0,   0,  31,  31  },
    {  10000,    0,   0,  32,  32  },
    {  10000,    0,   0,  34,  33  },
    {  10000,    0,   0,  35,  34  },
    {  10000,    0,   0,  36,  35  },
    {  10000,    0,   0,  37,  36  },
    {  10000,    0,   0,  38,  37  },
    {  10000,    0,   0,  39,  38  },
    {  10000,    0,   0,  40,  39  },
    {  10000,    0,   0,  41,  40  },
    {  10000,    0,   0,  42,  41  },
    {  10000,    0,   0,  43,  42  },
    {  10000,    0,   0,  44,  43  },
    {  10000,    0,   0,  45,  44  },
    {  10000,    0,   0,  46,  45  },
    {  10000,    0,   0,  47,  46  },
    {  10000,    0,   0,  48,  47  },
    {  10000,    0,   0,  49,  48  },
    {  10000,    0,   0,  50,  49  },
    {  10000,    0,   0,  51,  50  },
    {  10000,    0,   0,  52,  51  },
    {  10000,    0,   0,  53,  52  },
    {  10000,    0,   0,  54,  53  },
    {  10000,    0,   0,  55,  54  },
    {  10000,    0,   0,  56,  55  },
    {  10000,    0,   0,  57,  56  },
    {  10000,    0,   0,  58,  57  },
    {  10000,    0,   0,  59,  58  },
    {  10000,    0,   0,  60,  59  },
    {  10000,    0,   0,  61,  60  },
    {  10000,    0,   0,  62,  61  },
    {  10000,    0,   0,  63,  62  },
    {  10000,    0,   0,  64,  63  },
    {  10000,    0,   0,  65,  64  },
    {  10000,    0,   0,  68,  65  },
    {  10000,    0,   0,  69,  66  },
    {  10000,    0,   0,  70,  67  },
    {  10000,    0,   0,  71,  68  },
    {  10000,    0,   0,  72,  69  },
    {  10000,    0,   0,  73,  70  },
    {  10000,    0,   0,  74,  71  },
    {  10000,    0,   0,  75,  72  },
    {  10000,    0,   0,  76,  73  },
    {  10000,    0,   0,  77,  74  },
    {  10000,    0,   0,  78,  75  },
    {  10000,    0,   0,  79,  76  },
    {  10000,    0,   0,  80,  77  },
    {  10000,    0,   0,  81,  78  },
    {  10000,    0,   0,  82,  79  },
    {  10000,    0,   0,  83,  80  },
    {  10000,    0,   0,  84,  81  },
    {  10000,    0,   0,  85,  82  },
    {  10000,    0,   0,  86,  83  },
    {  10000,    0,   0,  87,  84  },
    {  10000,    0,   0,  88,  85  },
    {  10000,    0,   0,  89,  86  },
    {  10000,    0,   0,  90,  87  },
    {  10000,    0,   0,  91,  88  },
    {  10000,    0,   0,  92,  89  },
    {  10000,    0,   0,  93,  90  },
    {  10000,    0,   0,  94,  91  },
    {  10000,    0,   0,  95,  92  },
    {  10000,    0,   0,  96,  93  },
    {  10000,    0,   0,  97,  94  },
    {  10000,    0,   0,  98,  95  },
    {  10000,    0,   0,  99,  96  },    
    {  10000,    0,   0, 102,  97  },
    {  10000,    0,   0, 103,  98  },
    {  10000,    0,   0, 104,  99  },
    {  10000,    0,   0, 105, 100  },
    {  10000,    0,   0, 106, 101  },
    {  10000,    0,   0, 107, 102  },
    {  10000,    0,   0, 108, 103  },
    {  10000,    0,   0, 109, 104  },
    {  10000,    0,   0, 110, 105  },
    {  10000,    0,   0, 111, 106  },
    {  10000,    0,   0, 112, 107  },
    {  10000,    0,   0, 113, 108  },
    {  10000,    0,   0, 114, 109  },
    {  10000,    0,   0, 115, 110  },
    {  10000,    0,   0, 116, 111  },
    {  10000,    0,   0, 117, 112  },
    {  10000,    0,   0, 118, 113  },
    {  10000,    0,   0, 119, 114  },
    {  10000,    0,   0, 120, 115  },
    {  10000,    0,   0, 121, 116  },
    {  10000,    0,   0, 122, 117  },
    {  10000,    0,   0, 123, 118  },
    {  10000,    0,   0, 124, 119  },
    {  10000,    0,   0, 125, 120  },
    {  10000,    0,   0, 126, 121  },
    {  10000,    0,   0, 127, 122  },
    {  10000,    0,   0, 128, 123  },
    {  10000,    0,   0, 129, 124  },
    {  10000,    0,   0, 130, 125  },
    {  10000,    0,   0, 131, 126  },
    {  10000,    0,   0, 132, 127  },
    {  10000,    0,   0, 133, 128  },
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

board_config_t board_bcm56960_128x10g = {
    "bcm56960_128x10g",
    _chip_configs,
    &_phy_reset_cb,
    &_phy_init_cb,
};
