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

#if defined(CDK_CONFIG_INCLUDE_BCM53570_B0) && CDK_CONFIG_INCLUDE_BCM53570_B0 == 1
#include <cdk/chip/bcm53570_b0_defs.h>
#define _CHIP_DYN_CONFIG        DCFG_OP5
#endif

static cdk_port_map_port_t _skip_ports[] = { -1 };

#ifndef _CHIP_DYN_CONFIG 
#define _CHIP_DYN_CONFIG        0x00400000
#endif


static cdk_port_config_t _port_configs[] = {
    /* speed flags mode  sys  app */
    {      0,    0,   0,  -1,  -1  },
    {      0,    0,   0,  -1,  -1  },
    {   2500,    0,   0,   2,   2  },
    {   2500,    0,   0,   3,   3  },
    {   2500,    0,   0,   4,   4  },
    {   2500,    0,   0,   5,   5  },
    {   2500,    0,   0,   6,   6  },
    {   2500,    0,   0,   7,   7  },
    {   2500,    0,   0,   8,   8  },
    {   2500,    0,   0,   9,   9  },
    {   2500,    0,   0,  10,  10  },
    {   2500,    0,   0,  11,  11  },
    {   2500,    0,   0,  12,  12  },
    {   2500,    0,   0,  13,  13  },
    {   2500,    0,   0,  14,  14  },
    {   2500,    0,   0,  15,  15  },
    {   2500,    0,   0,  16,  16  },
    {   2500,    0,   0,  17,  17  },
    {   2500,    0,   0,  18,  18  },
    {   2500,    0,   0,  19,  19  },
    {   2500,    0,   0,  20,  20  },
    {   2500,    0,   0,  21,  21  },
    {   2500,    0,   0,  22,  22  },
    {   2500,    0,   0,  23,  23  },
    {   2500,    0,   0,  24,  24  },
    {   2500,    0,   0,  25,  25  },
    {   2500,    0,   0,  26,  26  },
    {   2500,    0,   0,  27,  27  },
    {   2500,    0,   0,  28,  28  },
    {   2500,    0,   0,  29,  29  },
    {   2500,    0,   0,  30,  42  },
    {   2500,    0,   0,  31,  43  },
    {   2500,    0,   0,  32,  44  },
    {   2500,    0,   0,  33,  45  },
    {   2500,    0,   0,  34,  58  },
    {   2500,    0,   0,  35,  59  },
    {   2500,    0,   0,  36,  60  },
    {   2500,    0,   0,  37,  61  },
    {   2500,    0,   0,  38,  62  },
    {   2500,    0,   0,  39,  63  },
    {   2500,    0,   0,  40,  64  },
    {   2500,    0,   0,  41,  65  },
    {   2500,    0,   0,  42,  66  },
    {   2500,    0,   0,  43,  67  },
    {   2500,    0,   0,  44,  68  },
    {   2500,    0,   0,  45,  69  },
    {   2500,    0,   0,  46,  70  },
    {   2500,    0,   0,  47,  71  },
    {   2500,    0,   0,  48,  72  },
    {   2500,    0,   0,  49,  73  },
    {  10000,    0,   0,  50,  74  },
    {  10000,    0,   0,  51,  75  },
    {  10000,    0,   0,  52,  76  },
    {  10000,    0,   0,  53,  77  },
    {   2500,    0,   0,  54,  78  },
    {   2500,    0,   0,  55,  79  },
    {   2500,    0,   0,  56,  80  },
    {   2500,    0,   0,  57,  81  },
    {  11000,    0,   0,  58,  82  },
    {  11000,    0,   0,  59,  83  },
    {  11000,    0,   0,  60,  84  },
    {  11000,    0,   0,  61,  85  },
    {  50000,    0,   0,  62,  86  },
    {  50000,    0,   0,  63,  88  },
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

board_config_t board_bcm53570_op5 = {
    "bcm53570_op5",
    _chip_configs,
    &_phy_reset_cb,
    &_phy_init_cb,
};
