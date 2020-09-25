/*
 *         
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *         
 * Shell diagnostics of Phymod    
 */

#ifndef _PHYMOD_UTIL_H_
#define _PHYMOD_UTIL_H_

#include <phymod/phymod.h>


/******************************************************************************
Functions
 */


int phymod_util_lane_config_get(const phymod_access_t *phys, int *start_lane, int *num_of_lane);
#define _SHR_LANEBMP_WBIT(_lane)                  (1U << (_lane))
#define PHYMOD_LANEPBMP_MEMBER(_bmp, _lane)       (((_bmp) & (_SHR_LANEBMP_WBIT(_lane))) != 0)

int phymod_core_name_get(const phymod_core_access_t *core, uint32_t serdes_id, char *core_name, phymod_core_info_t *info);
#endif /*_PHYMOD_UTIL_H_*/
