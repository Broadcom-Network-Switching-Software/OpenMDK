#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56800_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#include <bmdi/arch/xgs_dma.h>

#include <cdk/chip/bcm56800_a0_defs.h>
#include <cdk/arch/xgs_chip.h>
#include <cdk/cdk_debug.h>

#include "bcm56800_a0_bmd.h"

int
bcm56800_a0_bmd_switching_init(int unit)
{
    int ioerr = 0;
    int rv;
    int port;
    cdk_pbmp_t pbmp;
    uint32_t port_mode_flags;
    uint32_t vlan_flags;
    EPC_LINK_BMAPr_t epc_link;

    rv = bcm56800_a0_bmd_reset(unit);

    if (CDK_SUCCESS(rv)) {
        rv = bcm56800_a0_bmd_init(unit);
    }

    if (CDK_SUCCESS(rv)) {
        rv = bcm56800_a0_bmd_vlan_create(unit, BMD_CONFIG_DEFAULT_VLAN);
    }

    vlan_flags = BMD_VLAN_PORT_F_UNTAGGED;

    CDK_XGS_BLKTYPE_PBMP_GET(unit, BLKTYPE_GXPORT, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        if (CDK_SUCCESS(rv)) {
            rv = bcm56800_a0_bmd_vlan_port_add(unit, BMD_CONFIG_DEFAULT_VLAN,
                                               port, vlan_flags);
        }
        if (CDK_SUCCESS(rv)) {
            rv = bcm56800_a0_bmd_port_stp_set(unit, port, 
                                              bmdSpanningTreeForwarding);
        }
        if (CDK_SUCCESS(rv)) {
            port_mode_flags = 0;
            if (BMD_PORT_PROPERTIES(unit, port) & BMD_PORT_HG) {
                port_mode_flags |= BMD_PORT_MODE_F_HIGIG;
            }
            rv = bcm56800_a0_bmd_port_mode_set(unit, port, bmdPortModeAuto, 
                                               port_mode_flags);
        }
    }

    vlan_flags = 0;

    if (CDK_SUCCESS(rv)) {
        rv = bcm56800_a0_bmd_vlan_port_add(unit, BMD_CONFIG_DEFAULT_VLAN,
                                           CMIC_PORT, vlan_flags);
    }

    /* Enable all ports in MMU */
    EPC_LINK_BMAPr_CLR(epc_link);
    EPC_LINK_BMAPr_PORT_BITMAPf_SET(epc_link, CDK_PBMP_WORD_GET(pbmp, 0));
    ioerr += WRITE_EPC_LINK_BMAPr(unit, epc_link);

    return ioerr ? CDK_E_IO : rv;
}
#endif /* CDK_CONFIG_INCLUDE_BCM56800_A0 */
