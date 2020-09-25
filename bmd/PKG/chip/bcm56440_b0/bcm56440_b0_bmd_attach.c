#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56440_B0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#include <cdk/chip/bcm56440_b0_defs.h>

#include "bcm56440_b0_bmd.h"
#include "bcm56440_b0_internal.h"

#include "../bcm56440_a0/bcm56440_a0_bmd.h"

static int
_gport_pbmp_get(int unit, cdk_pbmp_t *pbmp)
{
    int port;

    CDK_XGSM_BLKTYPE_PBMP_GET(unit, BLKTYPE_GPORT, pbmp);
    CDK_PBMP_ITER(*pbmp, port) {
        if (BMD_PORT_PROPERTIES(unit, port) == 0) {
            CDK_PBMP_PORT_REMOVE(*pbmp, port);
        }
    }
    return 0;
}

static int
_mxqport_pbmp_get(int unit, cdk_pbmp_t *pbmp)
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
bcm56440_b0_p2l(int unit, int port, int inverse)
{
    cdk_pbmp_t pbmp;
    int pp, lp = 1;

    /* Fixed mappings */
    if (port == CMIC_PORT) {
        return CMIC_LPORT;
    }
    /* Use per-port config if available */
    if (CDK_NUM_PORT_CONFIGS(unit) != 0) {
        if (inverse) {
            for (pp = 0; pp < NUM_PHYS_PORTS; pp++) {
                if (port == CDK_PORT_CONFIG_SYS_PORT(unit, pp)) {
                    return pp;
                }
            }
            return -1;
        } else {
            return CDK_PORT_CONFIG_SYS_PORT(unit, port);
        }
    }

    /* By default logical ports are contiguous starting from 1 */
    _gport_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, pp) {
        if (inverse) {
            if (port == lp) {
                return pp;
            }
        } else {
            if (port == pp) {
                return lp;
            }
        }
        lp++;
    }
    _mxqport_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, pp) {
        if (inverse) {
            if (port == lp) {
                return pp;
            }
        } else {
            if (port == pp) {
                return lp;
            }
        }
        lp++;
    }
    return -1;
}

int 
bcm56440_b0_bmd_attach(
    int unit)
{
    return bcm56440_a0_bmd_attach(unit);
}
#endif /* CDK_CONFIG_INCLUDE_BCM56440_B0 */
