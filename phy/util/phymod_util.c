/* 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include <phymod/phymod_util.h>

int phymod_util_lane_config_get(const phymod_access_t *phys, int *start_lane, int *num_of_lane)
{
    int i ;
    switch (phys->lane_mask) {
        case 0x1:
            *start_lane = 0;
            *num_of_lane = 1;
            break;
        case 0x2:
            *start_lane = 1;
            *num_of_lane = 1;
            break;
        case 0x4:
            *start_lane = 2;
            *num_of_lane = 1;
            break;
        case 0x8:
            *start_lane = 3;
            *num_of_lane = 1;
            break;
        case 0x3:
            *start_lane = 0;
            *num_of_lane = 2;
            break;
        case 0xc:
            *start_lane = 2;
            *num_of_lane = 2;
            break;
        case 0x7:
            *start_lane = 0;
            *num_of_lane = 3;
            break;
        case 0xf:
            *start_lane = 0;
            *num_of_lane = 4;
            break;
        case 0x10:
            *start_lane = 4;
            *num_of_lane = 1;
            break;
        case 0x20:
            *start_lane = 5;
            *num_of_lane = 1;
            break;
        case 0x40:
            *start_lane = 6;
            *num_of_lane = 1;
            break;
        case 0x80:
            *start_lane = 7;
            *num_of_lane = 1;
            break;
        case 0x30:
            *start_lane = 4;
            *num_of_lane = 2;
            break;
        case 0xc0:
            *start_lane = 6;
            *num_of_lane = 2;
            break;
        case 0xf0:
            *start_lane = 4;
            *num_of_lane = 4;
            break;
        case 0xff:
            *start_lane = 0;
            *num_of_lane = 8;
            break;
        case 0x100:
            *start_lane = 8;
            *num_of_lane = 1;
            break;
        case 0x200:
            *start_lane = 9;
            *num_of_lane = 1;
            break;
        case 0x400:
            *start_lane = 10;
            *num_of_lane = 1;
            break;
        case 0x800:
            *start_lane = 11;
            *num_of_lane = 1;
            break;
        case 0x1000:
            *start_lane = 12;
            *num_of_lane = 1;
            break;
        case 0x2000:
            *start_lane = 13;
            *num_of_lane = 1;
            break;
        case 0x4000:
            *start_lane = 14;
            *num_of_lane = 1;
            break;
        case 0x8000:
            *start_lane = 15;
            *num_of_lane = 1;
            break;
        default:
            /*Support non-consecutive lanes*/
            for(i = 0; i < 16; i++)
            {
                if(phys->lane_mask & (1 << i))
                {
                    *start_lane = i;
                    break;
                }
            }
            *num_of_lane = 4;
    }
    return PHYMOD_E_NONE;
}

int phymod_core_name_get(const phymod_core_access_t *core,  uint32_t serdes_id, char *core_name, phymod_core_info_t *info)
{
    char* rev_char[4]={"A", "B", "C", "D"};
    char rev_num[]="7";
    uint8_t rev_letter, rev_number;

    info->serdes_id = serdes_id;
    rev_letter = (serdes_id & 0xc000) >> 14;
    rev_number = (serdes_id & 0x3800) >> 11;
    PHYMOD_STRCAT(core_name, rev_char[rev_letter]);
    PHYMOD_SNPRINTF(rev_num, 2, "%d", rev_number);
    PHYMOD_STRCAT(core_name, rev_num);
    PHYMOD_STRNCPY(info->name, core_name, PHYMOD_STRLEN(core_name)+1);

    return PHYMOD_E_NONE;
}
