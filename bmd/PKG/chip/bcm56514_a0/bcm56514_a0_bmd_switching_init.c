#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56514_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#include <bmdi/arch/xgs_dma.h>

#include <cdk/chip/bcm56514_a0_defs.h>
#include <cdk/arch/xgs_chip.h>
#include <cdk/cdk_debug.h>

#include "bcm56514_a0_bmd.h"

static int
_config_e_port(int unit, int port, uint32_t vlan_flags)
{
    int rv;

    rv = bcm56514_a0_bmd_vlan_port_add(unit, BMD_CONFIG_DEFAULT_VLAN,
                                       port, vlan_flags);

    if (CDK_SUCCESS(rv)) {
        rv = bcm56514_a0_bmd_port_stp_set(unit, port, 
                                          bmdSpanningTreeForwarding);
    }

    return rv;
}

int
bcm56514_a0_bmd_switching_init(int unit)
{
    int ioerr = 0;
    int rv;
    int port;
    cdk_pbmp_t pbmp;
    uint32_t vlan_flags;
    uint32_t epc_pbm;
    EPC_LINK_BMAPr_t epc_link;

    rv = bcm56514_a0_bmd_reset(unit);

    if (CDK_SUCCESS(rv)) {
        rv = bcm56514_a0_bmd_init(unit);
    }

    if (CDK_SUCCESS(rv)) {
        rv = bcm56514_a0_bmd_vlan_create(unit, BMD_CONFIG_DEFAULT_VLAN);
    }

    vlan_flags = BMD_VLAN_PORT_F_UNTAGGED;

    CDK_XGS_BLKTYPE_PBMP_GET(unit, BLKTYPE_GPORT, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        if (CDK_SUCCESS(rv)) {
            rv = _config_e_port(unit, port, vlan_flags);
        }
        if (CDK_SUCCESS(rv)) {
            rv = bcm56514_a0_bmd_port_mode_set(unit, port, bmdPortModeAuto, 0);
        }
    }
    epc_pbm = CDK_PBMP_WORD_GET(pbmp, 0);

#if BMD_CONFIG_INCLUDE_HIGIG == 1 || BMD_CONFIG_INCLUDE_XE == 1
    CDK_XGS_BLKTYPE_PBMP_GET(unit, BLKTYPE_XPORT, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        if (CDK_SUCCESS(rv)) {
            rv = _config_e_port(unit, port, vlan_flags);
        }
        if (CDK_SUCCESS(rv)) {
            rv = bcm56514_a0_bmd_port_mode_set(unit, port, bmdPortMode10000fd, 0);
        }
    }
    epc_pbm |= CDK_PBMP_WORD_GET(pbmp, 0);
#endif

    vlan_flags = 0;

    if (CDK_SUCCESS(rv)) {
        rv = bcm56514_a0_bmd_vlan_port_add(unit, BMD_CONFIG_DEFAULT_VLAN,
                                           CMIC_PORT, vlan_flags);
    }

    /* Enable all ports in MMU */
    EPC_LINK_BMAPr_CLR(epc_link);
    EPC_LINK_BMAPr_PORT_BITMAPf_SET(epc_link, epc_pbm);
    ioerr += WRITE_EPC_LINK_BMAPr(unit, epc_link);

    return ioerr ? CDK_E_IO : rv;
}

#endif /* CDK_CONFIG_INCLUDE_BCM56514_A0 */
