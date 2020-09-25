/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.1,
 * WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING
 * ANY FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.$
 */

/*
 * This port configuration is intended for BCM56644 rev A0 systems on
 * which the uplink ports are not functioning properly.
 */

#include <board/board_config.h>
#include <board/sdk56018.h>

static cdk_port_map_port_t _skip_ports[] = { 57, 61, 73, 77, -1 };

static board_chip_config_t _chip_config = {
    _skip_ports,
};

static board_chip_config_t *_chip_configs[] = {
    &_chip_config,
    NULL
};

board_config_t board_bcm56644_a0 = {
    "bcm56644_a0",
    _chip_configs
};
