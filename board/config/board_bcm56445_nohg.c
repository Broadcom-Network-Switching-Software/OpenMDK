/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.1,
 * WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING
 * ANY FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.$
 */

/*
 * This port configuration is intended for a Raptor-based regression
 * system on which port 18 and 21 are broken.
 */

#include <board/board_config.h>

static cdk_port_map_port_t _skip_ports[] = { 25, 26, 27, 28, -1 };

static board_chip_config_t _chip_config = {
    _skip_ports,
};

static board_chip_config_t *_chip_configs[] = {
    &_chip_config,
    NULL
};

board_config_t board_bcm56445_nohg = {
    "bcm56445_nohg",
    _chip_configs
};
