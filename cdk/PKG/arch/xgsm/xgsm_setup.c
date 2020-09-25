/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <cdk/cdk_device.h>
#include <cdk/cdk_error.h>
#include <cdk/cdk_assert.h>

#include <cdk/arch/xgsm_chip.h>

int
cdk_xgsm_setup(cdk_dev_t *dev)
{
    if (dev->dv.base_addr != NULL || 
        (dev->dv.read32 != NULL && dev->dv.write32 != NULL)) {
#if CDK_CONFIG_MEMMAP_DIRECT == 1
        CDK_ASSERT(dev->dv.base_addr);
#endif
#if CDK_CONFIG_INCLUDE_PORT_MAP == 1
        dev->port_map_size = CDK_XGSM_INFO(dev->unit)->nports;;
        dev->port_map = CDK_XGSM_INFO(dev->unit)->ports;;
#endif
        CDK_PBMP_ASSIGN(dev->valid_pbmps,
                        CDK_XGSM_INFO(dev->unit)->valid_pbmps);
        return CDK_E_NONE;
    }
    return CDK_E_NOT_FOUND;
}
