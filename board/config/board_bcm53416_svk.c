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

#if defined(CDK_CONFIG_INCLUDE_BCM53400_A0) && CDK_CONFIG_INCLUDE_BCM53400_A0 == 1
#include <cdk/chip/bcm53400_a0_defs.h>
#define _CHIP_DYN_CONFIG        DCFG_FLEX
#endif

static cdk_port_map_port_t _skip_ports[] = { -1 };

#ifndef _CHIP_DYN_CONFIG 
#define _CHIP_DYN_CONFIG        0
#endif

static cdk_port_config_t _port_configs[] = {
    /* speed flags mode  sys  app */
    {      0,    0,   0,  -1,  -1  },
    {   2500,    0,   0,   2,   2  },
    {   2500,    0,   0,   3,   3 },
    {   2500,    0,   0,   4,   4 },
    {   2500,    0,   0,   5,   5 },
    {   2500,    0,   0,   6,  18  },
    {   2500,    0,   0,   7,  19  },
    {   2500,    0,   0,   8,  20  },
    {   2500,    0,   0,   9,  21  },
    {   2500,    0,   0,  10,  22  },
    {   2500,    0,   0,  11,  23  },
    {   2500,    0,   0,  12,  24  },
    {   2500,    0,   0,  13,  25  },
    {  10000,    0,   0,  14,  26  },
    {  10000,    0,   0,  15,  27  },
    {  10000,    0,   0,  16,  28  },
    {  10000,    0,   0,  17,  29  },
    {  10000,    0,   0,  18,  30  },
    {  10000,    0,   0,  19,  31  },
    {  10000,    0,   0,  20,  32  },
    {  10000,    0,   0,  21,  33  },
    {  10000,    0,   0,  22,  34  },
    {  10000,    0,   0,  23,  35  },
    {  10000,    0,   0,  24,  36  },
    {  10000,    0,   0,  25,  37  }
};                             

static int                     
_phy_reset_cb(phy_ctrl_t *pc)
{
    int rv = CDK_E_NONE;

    return rv;
}

static int 
_phy_init_cb(phy_ctrl_t *pc)
{
    return CDK_E_NONE;
}

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

board_config_t board_bcm53416_svk = {
    "bcm53416_svk",
    _chip_configs,
    &_phy_reset_cb,
    &_phy_init_cb,
};
