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

#define TRIPLE_CORE_444 0 /* 120G */
#define TRIPLE_CORE_343 1 /* 100G 3-4-3 */
#define TRIPLE_CORE_442 2 /* 100G 4-4-2 */
#define TRIPLE_CORE_244 3 /* 100G 2-4-4 */

#define HG2 CDK_DCFG_PORT_MODE_HIGIG2

static cdk_port_config_t _port_configs[] = {
    /* speed flags mode  sys  app */
    {      0,    0,   0,  -1,  -1  },
    { 100000,    0,   0,   1,   1  },
    { 100000,    0,   0,  -1,   5  },
    { 100000,    0,   0,  -1,   9  },
    {  40000,    0,   0,   2,  13  },
    {  40000,    0,   0,   3,  17  },
    {  40000,    0,   0,   4,  21  },
    {  40000,    0,   0,   5,  25  },
    {  40000,    0,   0,   6,  29  },
    { 100000,    0,   0,   7,  33  },
    { 100000,    0,   0,  -1,  37  },
    { 100000,    0,   0,  -1,  41  },
    {  40000,    0,   0,   8,  45  },
    {  40000,    0,   0,   9,  49  },
    {  40000,    0,   0,  10,  53  },
    {  40000,    0,   0,  11,  57  },
    {  40000,    0,   0,  12,  61  },
    { 100000,    0,   0,  13,  65  },
    { 100000,    0,   0,  -1,  69  },
    { 100000,    0,   0,  -1,  73  },
    {  40000,    0,   0,  14,  77  },
    {  40000,    0,   0,  15,  81  },
    {  40000,    0,   0,  16,  85  },
    {  40000,    0,   0,  17,  89  },
    {  40000,    0,   0,  18,  93  },
    { 100000,    0,   0,  19,  97  },
    { 100000,    0,   0,  -1, 101  },
    { 100000,    0,   0,  -1, 105  },
    {  40000,    0,   0,  20, 109  },
    {  40000,    0,   0,  21, 113  },
    {  40000,    0,   0,  22, 117  },
    {  40000,    0,   0,  23, 121  },
    {  40000,    0,   0,  24, 125  },
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
    phy_ctrl_t *pc_inst;
    uint32_t txlane_map, rxlane_map;
    int idx, core_num, core_idx;
    uint32_t rx_lane_maps[] = {
        0x3210, 0x1032, 0x1230, 0x3012, 0x3012, 0x3210, 0x1032, 0x3210,
        0x3210, 0x1032, 0x3012, 0x3012, 0x3012, 0x3210, 0x1032, 0x3210,
        0x3210, 0x1032, 0x3012, 0x3012, 0x3012, 0x3210, 0x1032, 0x3012,
        0x3210, 0x1032, 0x3012, 0x3012, 0x3012, 0x3210, 0x1032, 0x3012
    };
    uint32_t tx_lane_maps[] = {
        0x1032, 0x3210, 0x2301, 0x3210, 0x1230, 0x1032, 0x3210, 0x1032,
        0x3012, 0x3210, 0x3012, 0x3210, 0x1230, 0x1032, 0x3012, 0x1230,
        0x1032, 0x3210, 0x1230, 0x3210, 0x1230, 0x1032, 0x3210, 0x1230,
        0x1032, 0x3210, 0x1230, 0x3210, 0x1230, 0x1032, 0x3210, 0x1230
    };

    while (lpc) {
        if (lpc->drv == NULL || lpc->drv->drv_name == NULL) {
            return CDK_E_INTERNAL;
        }
        if (CDK_STRSTR(lpc->drv->drv_name, "tsce") != NULL) {
            core_num = 1;
            if (lpc->port == 1 || lpc->port == 33 ||
                lpc->port == 65 || lpc->port == 97 ) {
                /* Lane map setting for multi-core 100G PHY */
                if (PHY_CTRL_PORT(lpc) == 1 || PHY_CTRL_PORT(lpc) == 33 ||
                    PHY_CTRL_PORT(lpc) == 65 || PHY_CTRL_PORT(lpc) == 97) {
                    rv = PHY_CONFIG_SET(lpc, PhyConfig_MultiCoreLaneMap,
                                        TRIPLE_CORE_343, NULL);
                }
                
                /* Need to set triple-core for 100G port */
                core_num = 3;
            }

            pc_inst = lpc;
            for (idx = 0; idx < core_num; idx++) {
                if (core_num == 3 && idx > 0) {
                    pc_inst = lpc->slave[idx - 1];
                    if (pc_inst == NULL) {
                        continue;
                    }
                }

                core_idx = ((pc_inst->port - 1) >> 2);

                rxlane_map = rx_lane_maps[core_idx];
                rxlane_map = bcm56860_a0_serdes_lane_swap(pc_inst->port,
                                                          FALSE, rxlane_map);
                rv = PHY_CONFIG_SET(pc_inst, PhyConfig_XauiRxLaneRemap,
                                    rxlane_map, NULL);
                PHY_VERB(pc_inst, ("Remap Rx lanes (0x%04"PRIx32")\n", rxlane_map));

                txlane_map = tx_lane_maps[core_idx];
                txlane_map = bcm56860_a0_serdes_lane_swap(pc_inst->port,
                                                          TRUE, txlane_map);
                rv = PHY_CONFIG_SET(pc_inst, PhyConfig_XauiTxLaneRemap,
                                    txlane_map, NULL);            
                PHY_VERB(pc_inst, ("Remap Tx lanes (0x%04"PRIx32")\n", txlane_map));
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

board_config_t board_bcm56860_4x100g_20x40g_KC = {
    "bcm56860_4x100g_20x40g",
    _chip_configs,
    &_phy_reset_cb,
    &_phy_init_cb,
};
