/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56150_A0 == 1

#include <bmd/bmd.h>
#include <bmdi/arch/xgsm_dma.h>

#include <cdk/arch/xgsm_chip.h>
#include <cdk/cdk_debug.h>
#include <cdk/chip/bcm56150_a0_defs.h>

#include "bcm56150_a0_bmd.h"
#include "bcm56150_a0_internal.h"

static int
_config_port(int unit, int port, uint32_t vlan_flags)
{
    int rv;

    rv = bcm56150_a0_bmd_vlan_port_add(unit, BMD_CONFIG_DEFAULT_VLAN,
                                       port, vlan_flags);

    if (CDK_SUCCESS(rv)) {
        rv = bcm56150_a0_bmd_port_stp_set(unit, port, 
                                          bmdSpanningTreeForwarding);
    }

    return rv;
}

int 
bcm56150_a0_bmd_switching_init(int unit)
{
    int ioerr = 0;
    int rv;
    int port, speed_max;
    cdk_pbmp_t pbmp, epc_pbmp;
    uint32_t vlan_flags;
    uint32_t port_mode_flags;
    uint32_t pbm;
    EPC_LINK_BMAP_64r_t epc_link;
    bmd_port_mode_t port_mode;

    rv = bcm56150_a0_bmd_reset(unit);
    if (CDK_SUCCESS(rv)) {
        rv = bcm56150_a0_bmd_init(unit);
    }

    if (CDK_SUCCESS(rv)) {
        rv = bcm56150_a0_bmd_vlan_create(unit, BMD_CONFIG_DEFAULT_VLAN);
    }

    vlan_flags = BMD_VLAN_PORT_F_UNTAGGED;
    CDK_XGSM_BLKTYPE_PBMP_GET(unit, BLKTYPE_GPORT, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        if (CDK_SUCCESS(rv)) {
            rv = _config_port(unit, port, vlan_flags);
        }
        if (CDK_SUCCESS(rv)) {
            rv = bcm56150_a0_bmd_port_mode_set(unit, port, bmdPortModeAuto, 0);
        }
    }

    CDK_PBMP_CLEAR(epc_pbmp);
    CDK_PBMP_OR(epc_pbmp, pbmp);
    bcm56150_a0_xlport_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        port_mode_flags = 0;
        speed_max = bcm56150_a0_port_speed_max(unit, port);

        if (BMD_PORT_PROPERTIES(unit, port) & BMD_PORT_HG) {
            port_mode_flags |= BMD_PORT_MODE_F_HIGIG2;
        }

        port_mode = bmdPortModeAuto;
        if (speed_max == 13000) {
            port_mode = bmdPortMode10000fd;
            if (port_mode_flags & BMD_PORT_MODE_F_HIGIG2) {
                port_mode = bmdPortMode13000fd;
            }
        } else if (speed_max == 10000) {
            port_mode = bmdPortMode10000fd;
        } else if (speed_max == 1000) {
            port_mode = bmdPortMode1000fd;
        } 
        
        if (CDK_SUCCESS(rv)) {
            rv = _config_port(unit, port, vlan_flags);
        }

        if (CDK_SUCCESS(rv)) {
            rv = bcm56150_a0_bmd_port_mode_set(unit, port, port_mode, port_mode_flags);
        }
    }
    CDK_PBMP_OR(epc_pbmp, pbmp);

    vlan_flags = 0;

    if (CDK_SUCCESS(rv)) {
        rv = bcm56150_a0_bmd_vlan_port_add(unit, BMD_CONFIG_DEFAULT_VLAN,
                                           CMIC_PORT, vlan_flags);
    }

    /* Enable all ports in MMU */
    EPC_LINK_BMAP_64r_CLR(epc_link);
    pbm = CDK_PBMP_WORD_GET(epc_pbmp, 0);
    EPC_LINK_BMAP_64r_PORT_BITMAP_LOf_SET(epc_link, pbm);
    ioerr += WRITE_EPC_LINK_BMAP_64r(unit, epc_link);

    return ioerr ? CDK_E_IO : rv;
}

#endif /* CDK_CONFIG_INCLUDE_BCM56150_A0 */
