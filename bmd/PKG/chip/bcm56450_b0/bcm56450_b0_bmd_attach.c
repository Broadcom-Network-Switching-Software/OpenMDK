/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56450_B0 == 1

#include <bmd/bmd.h>

#include <cdk/chip/bcm56450_b0_defs.h>

#include "bcm56450_b0_bmd.h"
#include "bcm56450_b0_internal.h"

#include "../bcm56450_a0/bcm56450_a0_bmd.h"

int
bcm56450_b0_mxqport_pbmp_get(int unit, cdk_pbmp_t *pbmp)
{
    int port;
    CDK_XGSM_BLKTYPE_PBMP_GET(unit, BLKTYPE_MXQPORT, pbmp);
    CDK_PBMP_ITER(*pbmp, port) {
        if (BMD_PORT_PROPERTIES(unit, port) == 0) {
            CDK_PBMP_PORT_REMOVE(*pbmp, port);
        }
    }
    return 0;
}

int
bcm56450_b0_p2l(int unit, int port, int inverse)
{
    if (inverse) {
        if (port == GS_LPORT) {
            return GS_PORT;
        }
    } else {
        if (port == GS_PORT) {
            return GS_LPORT;
        }
    }
    return port;
}

int 
bcm56450_b0_bmd_attach(
    int unit)
{
    return bcm56450_a0_bmd_attach(unit);
}
#endif /* CDK_CONFIG_INCLUDE_BCM56450_B0 */
