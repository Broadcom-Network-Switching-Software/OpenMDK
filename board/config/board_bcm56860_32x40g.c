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
    {  40000,    0,   0,   9,  33  },
    {  40000,    0,   0,  10,  37  },
    {  40000,    0,   0,  11,  41  },
    {  40000,    0,   0,  12,  45  },
    {  40000,    0,   0,  13,  49  },
    {  40000,    0,   0,  14,  53  },
    {  40000,    0,   0,  15,  57  },
    {  40000,    0,   0,  16,  61  },
    {  40000,    0,   0,  17,  65  },
    {  40000,    0,   0,  18,  69  },
    {  40000,    0,   0,  19,  73  },
    {  40000,    0,   0,  20,  77  },
    {  40000,    0,   0,  21,  81  },
    {  40000,    0,   0,  22,  85  },
    {  40000,    0,   0,  23,  89  },
    {  40000,    0,   0,  24,  93  },
    {  40000,    0,   0,  25,  97  },
    {  40000,    0,   0,  26, 101  },
    {  40000,    0,   0,  27, 105  },
    {  40000,    0,   0,  28, 109  },
    {  40000,    0,   0,  29, 113  },
    {  40000,    0,   0,  30, 117  },
    {  40000,    0,   0,  31, 121  },
    {  40000,    0,   0,  32, 125  },
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

board_config_t board_bcm56860_32x40g_K = {
    "bcm56860_32x40g",
    _chip_configs,
    &_phy_reset_cb,
    &_phy_init_cb,
};
