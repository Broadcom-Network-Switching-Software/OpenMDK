/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Implementation of custom config functions of PHYMOD library.
 */

#include <phymod/phymod_sys.h>

void
phymod_udelay(uint32_t usecs)
{
    PHY_SYS_USLEEP(usecs);
}
