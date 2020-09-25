#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56260_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>


#include <cdk/chip/bcm56260_a0_defs.h>
#include <cdk/arch/xgsd_chip.h>
#include <cdk/cdk_debug.h>

#include "bcm56260_a0_bmd.h"
#include "bcm56260_a0_internal.h"

static int
_config_port(int unit, int port, uint32_t vlan_flags, uint32_t port_mode_flags)
{
    int rv = 0;

    rv = bcm56260_a0_bmd_vlan_port_add(unit, BMD_CONFIG_DEFAULT_VLAN,
                                       port, vlan_flags);

    if (CDK_SUCCESS(rv)) {
        rv = bcm56260_a0_bmd_port_stp_set(unit, port, 
                                          bmdSpanningTreeForwarding);
    }
    
    if (CDK_SUCCESS(rv)) {
        rv = bcm56260_a0_bmd_port_mode_set(unit, port, bmdPortModeAuto,
                                           port_mode_flags);
    }
    return rv;
}

int
bcm56260_a0_bmd_switching_init(int unit)
{
    int ioerr = 0;
    int rv =0;
    uint32_t vlan_flags;
    cdk_pbmp_t pbmp, pbmp_all, mxqpbmp;
    int port;
    uint32_t port_mode_flags;
    int ppport;
    uint32_t pbm;
    EPC_LINK_BMAPm_t epc_link;

    rv = bcm56260_a0_bmd_reset(unit);

    if (CDK_SUCCESS(rv)) {
        rv = bcm56260_a0_bmd_init(unit);
    }
    if (CDK_SUCCESS(rv)) {
        rv = bcm56260_a0_bmd_vlan_create(unit, BMD_CONFIG_DEFAULT_VLAN);
    }

    vlan_flags = BMD_VLAN_PORT_F_UNTAGGED;

    bcm56260_a0_mxqport_pbmp_get(unit, &mxqpbmp);
    CDK_PBMP_ITER(mxqpbmp, port) {
        port_mode_flags = 0;
        if (BMD_PORT_PROPERTIES(unit, port) & BMD_PORT_HG) {
            port_mode_flags |= BMD_PORT_MODE_F_HIGIG;
        }
        
        if (CDK_SUCCESS(rv)) {
            rv = _config_port(unit, port, vlan_flags, port_mode_flags);
        }
    }

    bcm56260_a0_xlport_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        port_mode_flags = 0;
        if (BMD_PORT_PROPERTIES(unit, port) & BMD_PORT_HG) {
            port_mode_flags |= BMD_PORT_MODE_F_HIGIG;
        }
        
        if (CDK_SUCCESS(rv)) {
            rv = _config_port(unit, port, vlan_flags, port_mode_flags);
        }
    }
    vlan_flags = 0;
    if (CDK_SUCCESS(rv)) {
        rv = bcm56260_a0_bmd_vlan_port_add(unit, BMD_CONFIG_DEFAULT_VLAN,
                                           CMIC_PORT, vlan_flags);
    }

    /* Enable all ports in MMU */
    CDK_PBMP_CLEAR(pbmp_all);
    CDK_PBMP_OR(pbmp_all, mxqpbmp);
    CDK_PBMP_OR(pbmp_all, pbmp);
    
    EPC_LINK_BMAPm_CLR(epc_link);
    CDK_PBMP_ITER(pbmp_all, ppport) {
        pbm = EPC_LINK_BMAPm_GET(epc_link, (ppport >> 5));
        pbm |= LSHIFT32(1, ppport & 0x1f);
        EPC_LINK_BMAPm_SET(epc_link, (ppport >> 5), pbm);
    }
    ioerr += WRITE_EPC_LINK_BMAPm(unit, 0, epc_link);

    return ioerr ? CDK_E_IO : rv;
}
#endif /* CDK_CONFIG_INCLUDE_BCM56260_A0 */
