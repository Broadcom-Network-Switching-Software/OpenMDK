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

static cdk_port_config_t _port_configs[] = {
    /* speed flags mode  sys  app */
    {      0,    0,   0,  -1,  -1  },
    {   1000,    0,   0,   1,   1  },
    {   1000,    0,   0,   2,   2  },
    {   1000,    0,   0,   3,   3  },
    {   1000,    0,   0,   4,   4  },
    {   1000,    0,   0,   5,  49  },
    {  10000,    0,   0,   9,  53  },
    {  10000,    0,   0,  10,  54  },
    {  10000,    0,   0,  11,  55  },
    {  10000,    0,   0,  12,  56  },
    {  21000,    0,   0,  13,  57  },
    {  21000,    0,   0,  17,  61  }
};                             
                               
static int                     
_phy_reset_cb(phy_ctrl_t *pc)
{
    return CDK_E_NONE;
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

board_config_t board_bcm56041_10g = {
    "bcm56041_10g",
    _chip_configs,
    &_phy_reset_cb,
    &_phy_init_cb,
};
