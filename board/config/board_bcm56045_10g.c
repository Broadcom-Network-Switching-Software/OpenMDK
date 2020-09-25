/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.1,
 * WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING
 * ANY FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.$
 */

#include <board/board_config.h>
#include <board/sdk56840.h>
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
    {  10000,    0,   0,   1,  53  },
    {  10000,    0,   0,   2,  54  },
    {  10000,    0,   0,   3,  55  },
    {  10000,    0,   0,   4,  56  },
    {  10000,    0,   0,   5,  57  },
    {  10000,    0,   0,   6,  58  },
    {  10000,    0,   0,   7,  59  },
    {  10000,    0,   0,   8,  60  },
    {  10000,    0,   0,   9,  61  },
    {  10000,    0,   0,  10,  62  },
    {  10000,    0,   0,  11,  63  },
    {  10000,    0,   0,  12,  64  },
    {  10000,    0,   0,  13,  69  },
    {  10000,    0,   0,  14,  70  },
    {  10000,    0,   0,  15,  71  },
    {  10000,    0,   0,  16,  72  },
    {  10000,    0,   0,  17,  73  },
    {  10000,    0,   0,  18,  74  },
    {  10000,    0,   0,  19,  75  },
    {  10000,    0,   0,  20,  76  },
    {  10000,    0,   0,  21,  77  },
    {  10000,    0,   0,  22,  78  },
    {  10000,    0,   0,  23,  79  },
    {  10000,    0,   0,  24,  80  },
    {   1000,    0,   0,  25,  37  }
};                             
                               
static board_chip_config_t _chip_config = {
    _skip_ports,
    NULL,
    NULL,
    _CHIP_DYN_CONFIG,
    COUNTOF(_port_configs),
    _port_configs
};

static board_chip_config_t *_chip_configs[] = {
    &_chip_config,
    NULL
};

board_config_t board_bcm56045_10g = {
    "bcm56045_10g",
    _chip_configs,
};
