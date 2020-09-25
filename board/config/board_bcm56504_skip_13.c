/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.1,
 * WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING
 * ANY FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.$
 */

/*
 * This port configuration is intended for a Firebolt-based regression
 * system on which port 13 is broken.
 */

#include <board/board_config.h>
#include <board/sdk56504.h>

static cdk_port_map_port_t _skip_ports[] = { 13, -1 };

static board_chip_config_t _chip_config = {
    _skip_ports,
    NULL,
    sdk56504_ledprog_info,
};

static board_chip_config_t *_chip_configs[] = {
    &_chip_config,
    NULL
};

board_config_t board_bcm56504_skip_13 = {
    "bcm56504_skip_13",
    _chip_configs
};
