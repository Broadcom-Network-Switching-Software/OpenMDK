/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.1,
 * WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING
 * ANY FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.$
 */

#include <board/board_config.h>

board_config_t *
board_config_get(const char *name, board_config_map_t *brd_cfg_map)
{
    board_config_map_t *cfg_map;
    int idx;

    if (name) {
        idx = 0;
        do {
            cfg_map = &brd_cfg_map[idx++];
            if (cfg_map->name == NULL) {
                break;
            }
            if (CDK_STRCMP(name, cfg_map->name) == 0) {
                return cfg_map->bcfg;
            }
        } while (1);
    }

    return NULL;
}

