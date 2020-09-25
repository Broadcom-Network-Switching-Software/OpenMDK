#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56334_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#include <bmdi/arch/xgs_dma.h>

#include <cdk/chip/bcm56334_a0_defs.h>
#include <cdk/arch/xgs_chip.h>
#include <cdk/cdk_debug.h>

#include "bcm56334_a0_bmd.h"

static int
_config_port(int unit, int port, uint32_t vlan_flags, uint32_t port_mode_flags)
{
    int rv;

    rv = bcm56334_a0_bmd_vlan_port_add(unit, BMD_CONFIG_DEFAULT_VLAN,
                                       port, vlan_flags);

    if (CDK_SUCCESS(rv)) {
        rv = bcm56334_a0_bmd_port_stp_set(unit, port, 
                                          bmdSpanningTreeForwarding);
    }
    if (CDK_SUCCESS(rv)) {
        rv = bcm56334_a0_bmd_port_mode_set(unit, port, bmdPortModeAuto,
                                           port_mode_flags);
    }

    return rv;
}

int
bcm56334_a0_bmd_switching_init(int unit)
{
    int ioerr = 0;
    int rv;
    int port;
    cdk_pbmp_t pbmp, epc_pbmp;
    uint32_t vlan_flags;
    uint32_t port_mode_flags;
    uint32_t pbm;
    EPC_LINK_BMAP_64r_t epc_link;
    rv = bcm56334_a0_bmd_reset(unit);

    if (CDK_SUCCESS(rv)) {
        rv = bcm56334_a0_bmd_init(unit);
    }

    if (CDK_SUCCESS(rv)) {
        rv = bcm56334_a0_bmd_vlan_create(unit, BMD_CONFIG_DEFAULT_VLAN);
    }
    vlan_flags = BMD_VLAN_PORT_F_UNTAGGED;

    CDK_XGS_BLKTYPE_PBMP_GET(unit, BLKTYPE_GPORT, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        if (CDK_SUCCESS(rv)) {
            rv = _config_port(unit, port, vlan_flags, 0);
        }
    }
    CDK_PBMP_CLEAR(epc_pbmp);
    CDK_PBMP_OR(epc_pbmp, pbmp);

    CDK_XGS_BLKTYPE_PBMP_GET(unit, BLKTYPE_XQPORT, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        port_mode_flags = 0;
        if (BMD_PORT_PROPERTIES(unit, port) & BMD_PORT_HG) {
            port_mode_flags |= BMD_PORT_MODE_F_HIGIG;
        }
        if (CDK_SUCCESS(rv)) {
            rv = _config_port(unit, port, vlan_flags, port_mode_flags);
        }
    }
    CDK_PBMP_OR(epc_pbmp, pbmp);

    vlan_flags = 0;

    if (CDK_SUCCESS(rv)) {
        rv = bcm56334_a0_bmd_vlan_port_add(unit, BMD_CONFIG_DEFAULT_VLAN,
                                           CMIC_PORT, vlan_flags);
    }

    /* Enable all ports in MMU */
    EPC_LINK_BMAP_64r_CLR(epc_link);
    pbm = CDK_PBMP_WORD_GET(epc_pbmp, 0);
    EPC_LINK_BMAP_64r_PORT_BITMAP_LOf_SET(epc_link, pbm);
    ioerr += WRITE_EPC_LINK_BMAP_64r(unit, epc_link);

    return ioerr ? CDK_E_IO : rv;
}

#endif /* CDK_CONFIG_INCLUDE_BCM56334_A0 */
