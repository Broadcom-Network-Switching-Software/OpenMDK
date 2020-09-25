/*
 *
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53540_A0 == 1

#include <bmd/bmd.h>
#include <bmdi/arch/xgsd_dma.h>

#include <cdk/arch/xgsd_chip.h>
#include <cdk/cdk_debug.h>
#include <cdk/chip/bcm53540_a0_defs.h>

#include <bmd/bmd_phy_ctrl.h>

#include "bcm53540_a0_bmd.h"
#include "bcm53540_a0_internal.h"

static int
_config_port(int unit, int port, uint32_t vlan_flags, uint32_t port_mode,
                                                    uint32_t port_mode_flags)
{
    int rv;

    rv = bcm53540_a0_bmd_vlan_port_add(unit, BMD_CONFIG_DEFAULT_VLAN, 
                                       port, vlan_flags);

    if (CDK_SUCCESS(rv)) {
        rv = bcm53540_a0_bmd_port_stp_set(unit, port, 
                                          bmdSpanningTreeForwarding);
    }

    if (CDK_SUCCESS(rv)) {
        rv = bcm53540_a0_bmd_port_mode_set(unit, port, port_mode, 
                                           port_mode_flags);
    }

    return rv;
}

int
bcm53540_a0_bmd_switching_init(int unit)
{
    int ioerr = 0;
    int rv;
    int port, lport;
    cdk_pbmp_t gpbmp, lpbmp;
    uint32_t pbm;
    EPC_LINK_BMAP_64r_t epc_link;
    uint32_t vlan_flags;

    rv = bcm53540_a0_bmd_reset(unit);
    if (CDK_SUCCESS(rv)) {
        rv = bcm53540_a0_bmd_init(unit);
    }

    if (CDK_SUCCESS(rv)) {
        rv = bcm53540_a0_bmd_vlan_create(unit, BMD_CONFIG_DEFAULT_VLAN);
    }

    vlan_flags = BMD_VLAN_PORT_F_UNTAGGED;

    bcm53540_a0_gport_pbmp_get(unit, &gpbmp);
    CDK_PBMP_ITER(gpbmp, port) {
        if (CDK_SUCCESS(rv)) {
            rv = _config_port(unit, port, vlan_flags, bmdPortModeAuto, 0);
        }
    }

    vlan_flags = 0;
    if (CDK_SUCCESS(rv)) {
        rv = bcm53540_a0_bmd_vlan_port_add(unit, BMD_CONFIG_DEFAULT_VLAN,
                                           CMIC_PORT, vlan_flags);
    }

    /* Enable all ports in MMU */
    CDK_PBMP_CLEAR(lpbmp);
    CDK_PBMP_ITER(gpbmp, port) {
        lport = P2L(unit, port);
        CDK_PBMP_PORT_ADD(lpbmp, lport);
    }

    EPC_LINK_BMAP_64r_CLR(epc_link);
    pbm = CDK_PBMP_WORD_GET(lpbmp, 0);
    EPC_LINK_BMAP_64r_PORT_BITMAPf_SET(epc_link, pbm);
    ioerr += WRITE_EPC_LINK_BMAP_64r(unit, epc_link);

    return ioerr ? CDK_E_IO : rv;
}

#endif /* CDK_CONFIG_INCLUDE_BCM53540_A0 */
