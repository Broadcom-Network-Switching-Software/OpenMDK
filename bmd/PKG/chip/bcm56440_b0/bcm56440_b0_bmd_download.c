/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>
#include "../bcm56440_a0/bcm56440_a0_bmd.h"
#include "bcm56440_b0_bmd.h"

int 
bcm56440_b0_bmd_download(
    int unit, 
    bmd_download_t type, 
    uint8_t *data, 
    int size)
{
    return bcm56440_a0_bmd_download(unit, type, data, size);
}

