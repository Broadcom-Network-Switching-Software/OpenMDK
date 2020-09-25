#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM89500_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#include "bcm89500_a0_bmd.h"

int
bcm89500_a0_bmd_download(int unit, bmd_download_t type, uint8_t *data, int size)
{
    return CDK_E_UNAVAIL; 
}

#endif /* CDK_CONFIG_INCLUDE_BCM89500_A0 */
