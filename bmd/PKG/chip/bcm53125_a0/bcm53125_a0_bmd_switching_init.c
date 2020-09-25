#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53125_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#include <cdk/chip/bcm53125_a0_defs.h>
#include <cdk/arch/robo_chip.h>
#include <cdk/cdk_debug.h>

#include "bcm53125_a0_bmd.h"

static int
_config_e_port(int unit, int port, uint32_t vlan_flags)
{
    int rv;
    rv = bcm53125_a0_bmd_vlan_port_add(unit, BMD_CONFIG_DEFAULT_VLAN,
                                       port, vlan_flags);
    if (CDK_SUCCESS(rv)) {
        rv = bcm53125_a0_bmd_port_stp_set(unit, port, 
                                          bmdSpanningTreeForwarding);
    }
    if (CDK_SUCCESS(rv)) {
        rv = bcm53125_a0_bmd_port_mode_set(unit, port, bmdPortModeAuto, 0);
    }

    return rv;
}

int
bcm53125_a0_bmd_switching_init(int unit)
{
    int rv = 0;
    int port;
    cdk_pbmp_t pbmp;
    uint32_t vlan_flags;

    rv = bcm53125_a0_bmd_reset(unit);

    if (CDK_SUCCESS(rv)) {
        rv = bcm53125_a0_bmd_init(unit);
    }

    if (CDK_SUCCESS(rv)) {
        rv = bcm53125_a0_bmd_vlan_create(unit, BMD_CONFIG_DEFAULT_VLAN);
    }

    vlan_flags = BMD_VLAN_PORT_F_UNTAGGED;

    CDK_ROBO_BLKTYPE_PBMP_GET(unit, BLKTYPE_GPIC, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        if (CDK_SUCCESS(rv)) {
            rv = _config_e_port(unit, port, vlan_flags);
        }
    }

    vlan_flags = 0;

    if (CDK_SUCCESS(rv)) {
        rv = bcm53125_a0_bmd_vlan_port_add(unit, BMD_CONFIG_DEFAULT_VLAN,
                                          CPIC_PORT, vlan_flags);
    }

    return rv;
}

#endif /* CDK_CONFIG_INCLUDE_BCM53125_A0 */
