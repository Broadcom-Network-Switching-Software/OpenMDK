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
#include <board/sdk56018.h>

static cdk_port_map_port_t _skip_ports[] = { 18, 21, -1 };

static board_chip_config_t _chip_config = {
    _skip_ports,
    NULL,
    sdk56018_ledprog_info,
};

static board_chip_config_t *_chip_configs[] = {
    &_chip_config,
    NULL
};

board_config_t board_bcm56218_skip_18 = {
    "bcm56218_skip_18",
    _chip_configs
};
