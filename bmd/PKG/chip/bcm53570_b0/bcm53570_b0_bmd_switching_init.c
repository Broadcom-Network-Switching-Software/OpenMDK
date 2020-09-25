/*
 *
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53570_B0 == 1

#include <bmd/bmd.h>
#include <bmdi/arch/xgsd_dma.h>

#include <cdk/arch/xgsd_chip.h>
#include <cdk/cdk_debug.h>
#include <cdk/chip/bcm53570_b0_defs.h>

#include "bcm53570_b0_bmd.h"
#include "bcm53570_b0_internal.h"

static int
_config_port(int unit, int port, uint32_t vlan_flags, uint32_t port_mode_flags)
{
    int rv;
    int speed_max;
    bmd_port_mode_t port_mode;

    speed_max = bcm53570_b0_port_speed_max(unit, port);

    port_mode = bmdPortModeAuto;
    if (speed_max == 50000) {
        port_mode = bmdPortMode50000fd;
    } else if (speed_max == 42000) {
        port_mode = bmdPortMode42000fd;
    } else if (speed_max == 25000) {
        port_mode = bmdPortMode25000fd;
    } else if (speed_max == 21000) {
        port_mode = bmdPortMode20000fd;
    } else if ((speed_max == 11000) || (speed_max == 10000)){
        port_mode = bmdPortMode10000fd;
    } else if (speed_max == 2500) {
        port_mode = bmdPortMode2500fd;
    }

    rv = bcm53570_b0_bmd_vlan_port_add(unit, BMD_CONFIG_DEFAULT_VLAN,
                                                        port, vlan_flags);

    if (CDK_SUCCESS(rv)) {
        rv = bcm53570_b0_bmd_port_stp_set(unit, port,
                                            bmdSpanningTreeForwarding);
    }

    if (CDK_SUCCESS(rv)) {
        rv = bcm53570_b0_bmd_port_mode_set(unit, port, port_mode,
                                                    port_mode_flags);
    }

    return rv;
}

int
bcm53570_b0_bmd_switching_init(int unit)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int port, lport, port_mode;
    cdk_pbmp_t pbmp_all;
    uint32_t vlan_flags;
    uint32_t port_mode_flags;
    EPC_LINK_BMAP_LO_64r_t epc_link_lo;
    EPC_LINK_BMAP_HI_64r_t epc_link_hi;
    uint32_t pbmp[2];

    rv = bcm53570_b0_bmd_reset(unit);
    if (CDK_SUCCESS(rv)) {
        rv = bcm53570_b0_bmd_init(unit);
    }

    if (CDK_SUCCESS(rv)) {
        rv = bcm53570_b0_bmd_vlan_create(unit, BMD_CONFIG_DEFAULT_VLAN);
    }

    vlan_flags = BMD_VLAN_PORT_F_UNTAGGED;
    bcm53570_b0_all_pbmp_get(unit, &pbmp_all);
    CDK_PBMP_ITER(pbmp_all, port) {

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
        rv = bcm53570_b0_bmd_vlan_port_add(unit, BMD_CONFIG_DEFAULT_VLAN,
                                           CMIC_PORT, vlan_flags);
    }

    /* Enable all ports */
    CDK_PBMP_ITER(pbmp_all, port) {
        lport = P2L(unit, port);
        if (lport < 64) {
            ioerr += READ_EPC_LINK_BMAP_LO_64r(unit, &epc_link_lo);
            EPC_LINK_BMAP_LO_64r_PORT_BITMAPf_GET(epc_link_lo, pbmp);
            PBM_PORT_ADD(pbmp, lport);
            EPC_LINK_BMAP_LO_64r_PORT_BITMAPf_SET(epc_link_lo, pbmp);
            ioerr += WRITE_EPC_LINK_BMAP_LO_64r(unit, epc_link_lo);
        } else {
            ioerr += READ_EPC_LINK_BMAP_HI_64r(unit, &epc_link_hi);
            pbmp[0] = EPC_LINK_BMAP_HI_64r_PORT_BITMAPf_GET(epc_link_hi);
            PBM_PORT_ADD(pbmp, lport - 64);
            EPC_LINK_BMAP_HI_64r_PORT_BITMAPf_SET(epc_link_hi, pbmp[0]);
            ioerr += WRITE_EPC_LINK_BMAP_HI_64r(unit, epc_link_hi);
        }
    }

    return ioerr ? CDK_E_IO : rv;
}

#endif /* CDK_CONFIG_INCLUDE_BCM53570_B0 */
