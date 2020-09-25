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
    {  40000,    0,   0,  13,  13  },
    {  40000,    0,   0,  14,  17  },
    {  10000,    0,   0,  15,  21  },
    {  10000,    0,   0,  16,  22  },
    {  10000,    0,   0,  17,  23  },
    {  10000,    0,   0,  18,  24  },
    {  10000,    0,   0,  19,  25  },
    {  10000,    0,   0,  20,  26  },
    {  10000,    0,   0,  21,  27  },
    {  10000,    0,   0,  22,  28  },
    {  10000,    0,   0,  23,  29  },
    {  10000,    0,   0,  24,  30  },
    {  10000,    0,   0,  25,  31  },
    {  10000,    0,   0,  26,  32  },
    {  10000,    0,   0,  27,  33  },
    {  10000,    0,   0,  28,  34  },
    {  10000,    0,   0,  29,  35  },
    {  10000,    0,   0,  30,  36  },
    {  10000,    0,   0,  31,  37  },
    {  10000,    0,   0,  32,  38  },
    {  10000,    0,   0,  33,  39  },
    {  10000,    0,   0,  34,  40  },
    {  10000,    0,   0,  35,  41  },
    {  10000,    0,   0,  36,  42  },
    {  10000,    0,   0,  37,  43  },
    {  10000,    0,   0,  38,  44  },
    {  40000,    0,   0,  39,  45  },
    {  40000,    0,   0,  40,  49  },
    {  10000,    0,   0,  41,  53  },
    {  10000,    0,   0,  42,  54  },
    {  10000,    0,   0,  43,  55  },
    {  10000,    0,   0,  44,  56  },
    {  10000,    0,   0,  45,  57  },
    {  10000,    0,   0,  46,  58  },
    {  10000,    0,   0,  47,  59  },
    {  10000,    0,   0,  48,  60  },
    {  10000,    0,   0,  49,  61  },
    {  10000,    0,   0,  50,  62  },
    {  10000,    0,   0,  51,  63  },
    {  10000,    0,   0,  52,  64  },
    {  10000,    0, HG2,  53,  65  },
    {  10000,    0, HG2,  54,  66  },
    {  10000,    0, HG2,  55,  67  },
    {  10000,    0, HG2,  56,  68  },
    {  10000,    0, HG2,  57,  69  },
    {  10000,    0, HG2,  58,  70  },
    {  10000,    0, HG2,  59,  71  },
    {  10000,    0, HG2,  60,  72  },
    {  10000,    0, HG2,  61,  73  },
    {  10000,    0, HG2,  62,  74  },
    {  10000,    0, HG2,  63,  75  },
    {  10000,    0, HG2,  64,  76  },
    {  40000,    0, HG2,  65,  77  },
    {  40000,    0, HG2,  66,  81  },
    {  10000,    0, HG2,  67,  85  },
    {  10000,    0, HG2,  68,  86  },
    {  10000,    0, HG2,  69,  87  },
    {  10000,    0, HG2,  70,  88  },
    {  10000,    0, HG2,  71,  89  },
    {  10000,    0, HG2,  72,  90  },
    {  10000,    0, HG2,  73,  91  },
    {  10000,    0, HG2,  74,  92  },
    {  10000,    0, HG2,  75,  93  },
    {  10000,    0, HG2,  76,  94  },
    {  10000,    0, HG2,  77,  95  },
    {  10000,    0, HG2,  78,  96  },
    {  10000,    0, HG2,  79,  97  },
    {  10000,    0, HG2,  80,  98  },
    {  10000,    0, HG2,  81,  99  },
    {  10000,    0, HG2,  82, 100  },
    {  10000,    0, HG2,  83, 101  },
    {  10000,    0, HG2,  84, 102  },
    {  10000,    0, HG2,  85, 103  },
    {  10000,    0, HG2,  86, 104  },
    {  10000,    0, HG2,  87, 105  },
    {  10000,    0, HG2,  88, 106  },
    {  10000,    0, HG2,  89, 107  },
    {  10000,    0, HG2,  90, 108  },
    {  40000,    0, HG2,  91, 109  },
    {  40000,    0, HG2,  92, 113  },
    {  10000,    0, HG2,  93, 117  },
    {  10000,    0, HG2,  94, 118  },
    {  10000,    0, HG2,  95, 119  },
    {  10000,    0, HG2,  96, 120  },
    {  10000,    0, HG2,  97, 121  },
    {  10000,    0, HG2,  98, 122  },
    {  10000,    0, HG2,  99, 123  },
    {  10000,    0, HG2, 100, 124  },
    {  10000,    0, HG2, 101, 125  },
    {  10000,    0, HG2, 102, 126  },
    {  10000,    0, HG2, 103, 127  },
    {  10000,    0, HG2, 104, 128  },
};                             

/*
 * Function:
 *      bcm56860_a0_serdes_lane_swap
 * Purpose:     
 *      Determine PHY lane swap due to bump flipping or package wire bonding swap.
 *
 * Parameters:
 *      port                  - physical port number
 *      tx                    - Tx if non-zero, otherwise Rx
 *      lane_map              - The Tx or Rx lane map
 * Returns:     
 *      The swapped lane map.
 */
static uint32_t
bcm56860_a0_serdes_lane_swap(int port, int tx, uint32_t lane_map)
{
    uint32_t lane_swap = lane_map;
    
    if (((port - 1) >> 2) & 1) {
        /* Odd core */
        if (tx) {
            lane_swap ^= 0x3333;
        }
    } else {
        /* Even core */
        if (!tx) {
            lane_swap ^= 0x3333;
        }
    }

    return lane_swap;
}
                               
static int                     
_phy_reset_cb(phy_ctrl_t *pc)
{
    int rv = CDK_E_NONE;
    phy_ctrl_t *lpc = pc;
    uint32_t txlane_map, rxlane_map;

    while (lpc) {
        if (lpc->drv == NULL || lpc->drv->drv_name == NULL) {
            return CDK_E_INTERNAL;
        }
        if (CDK_STRSTR(lpc->drv->drv_name, "tsce") != NULL) {
            if (((lpc->port - 1) & 3) == 0) {
                rxlane_map = 0x3012;
                rxlane_map = bcm56860_a0_serdes_lane_swap(lpc->port, FALSE,
                                                          rxlane_map);
                rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiRxLaneRemap,
                                    rxlane_map, NULL);
                PHY_VERB(lpc, ("Remap Rx lanes (0x%04"PRIx32")\n", rxlane_map));
    
                txlane_map = 0x3210;
                if (((lpc->port - 1) & 0x7) == 0) {
                    txlane_map = 0x1230;
                }
                txlane_map = bcm56860_a0_serdes_lane_swap(lpc->port, TRUE,
                                                          txlane_map);
                rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiTxLaneRemap,
                                    txlane_map, NULL);            
                PHY_VERB(lpc, ("Remap Tx lanes (0x%04"PRIx32")\n", txlane_map));
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

board_config_t board_bcm56860_96x10g_8x40g_K = {
    "bcm56860_96x10g_8x40g",
    _chip_configs,
    &_phy_reset_cb,
    &_phy_init_cb,
};
