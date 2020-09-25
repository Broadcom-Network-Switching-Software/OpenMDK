#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56850_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#include <cdk/chip/bcm56850_a0_defs.h>

#include <cdk/arch/xgsm_chip.h>
#include <cdk/cdk_debug.h>

#include "bcm56850_a0_bmd.h"
#include "bcm56850_a0_internal.h"

static int
_config_port(int unit, int port, uint32_t vlan_flags, uint32_t port_mode_flags)
{
    int rv;
    int speed_max;

    rv = bcm56850_a0_bmd_vlan_port_add(unit, BMD_CONFIG_DEFAULT_VLAN,
                                       port, vlan_flags);

    if (CDK_SUCCESS(rv)) {
        rv = bcm56850_a0_bmd_port_stp_set(unit, port, 
                                          bmdSpanningTreeForwarding);
    }
    if (CDK_SUCCESS(rv)) {
        speed_max = bcm56850_a0_port_speed_max(unit, port);
        if (speed_max == 40000) {
            rv = bcm56850_a0_bmd_port_mode_set(unit, port, bmdPortMode40000fd,
                                            port_mode_flags);
        } else if (speed_max == 10000) {
            rv = bcm56850_a0_bmd_port_mode_set(unit, port, bmdPortMode10000KR,
                                            port_mode_flags);
        } else if (speed_max == 1000) {
            rv = bcm56850_a0_bmd_port_mode_set(unit, port, bmdPortModeAuto,
                                            port_mode_flags);
        }
    }
    return rv;
}

int
bcm56850_a0_bmd_switching_init(int unit)
{
    int ioerr = 0;
    int rv;
    int port, lport;
    int port_mode;
    cdk_pbmp_t pbmp, lpbmp;
    uint32_t vlan_flags;
    uint32_t pbm;
    uint32_t port_mode_flags;
    EPC_LINK_BMAPm_t epc_link;

    rv = bcm56850_a0_bmd_reset(unit);

    if (CDK_SUCCESS(rv)) {
        rv = bcm56850_a0_bmd_init(unit);
    }

    if (CDK_SUCCESS(rv)) {
        rv = bcm56850_a0_bmd_vlan_create(unit, BMD_CONFIG_DEFAULT_VLAN);
    }
    vlan_flags = BMD_VLAN_PORT_F_UNTAGGED;

    bcm56850_a0_xlport_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        if (bcm56850_a0_port_speed_max(unit, port) <= 0) {
            /* Disabled flexport*/
            continue;
        }
        port_mode_flags = 0;
        if (BMD_PORT_PROPERTIES(unit, port) & BMD_PORT_HG) {
            /* Select HiGig mode based on dynamic configuration */
            port_mode = CDK_PORT_CONFIG_PORT_MODE(unit, port);
            if (port_mode == CDK_DCFG_PORT_MODE_HIGIG2) {
                port_mode_flags |= BMD_PORT_MODE_F_HIGIG2;
            } else {
                /* Default HiGig mode */
                port_mode_flags |= BMD_PORT_MODE_F_HIGIG;
            }
        }
        if (CDK_SUCCESS(rv)) {
            rv = _config_port(unit, port, vlan_flags, port_mode_flags);
        }
    }

    vlan_flags = 0;

    if (CDK_SUCCESS(rv)) {
        rv = bcm56850_a0_bmd_vlan_port_add(unit, BMD_CONFIG_DEFAULT_VLAN,
                                           CMIC_PORT, vlan_flags);
    }

    /* Enable all ports in MMU */
    CDK_PBMP_CLEAR(lpbmp);
    CDK_PBMP_ITER(pbmp, port) {
        lport = P2L(unit, port);
        CDK_PBMP_PORT_ADD(lpbmp, lport);
    }
    EPC_LINK_BMAPm_CLR(epc_link);
    pbm = CDK_PBMP_WORD_GET(lpbmp, 0);
    EPC_LINK_BMAPm_SET(epc_link, 0, pbm);
    pbm = CDK_PBMP_WORD_GET(lpbmp, 1);
    EPC_LINK_BMAPm_SET(epc_link, 1, pbm);
    pbm = CDK_PBMP_WORD_GET(lpbmp, 2);
    EPC_LINK_BMAPm_SET(epc_link, 2, pbm);
    pbm = CDK_PBMP_WORD_GET(lpbmp, 3);
    EPC_LINK_BMAPm_SET(epc_link, 3, pbm);
    ioerr += WRITE_EPC_LINK_BMAPm(unit, 0, epc_link);

    return ioerr ? CDK_E_IO : rv;
}
#endif /* CDK_CONFIG_INCLUDE_BCM56850_A0 */

