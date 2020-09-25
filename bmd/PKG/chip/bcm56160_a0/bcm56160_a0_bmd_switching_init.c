/*
 *
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56160_A0 == 1

#include <bmd/bmd.h>
#include <bmdi/arch/xgsd_dma.h>

#include <cdk/arch/xgsd_chip.h>
#include <cdk/cdk_debug.h>
#include <cdk/chip/bcm56160_a0_defs.h>

#include <bmd/bmd_phy_ctrl.h>

#include "bcm56160_a0_bmd.h"
#include "bcm56160_a0_internal.h"

static int
_config_port(int unit, int port, uint32_t vlan_flags, uint32_t port_mode,
                                                    uint32_t port_mode_flags)
{
    int rv;

    rv = bcm56160_a0_bmd_vlan_port_add(unit, BMD_CONFIG_DEFAULT_VLAN,
                                        port, vlan_flags);

    if (CDK_SUCCESS(rv)) {
        rv = bcm56160_a0_bmd_port_stp_set(unit, port,
                                        bmdSpanningTreeForwarding);
    }

    if (CDK_SUCCESS(rv)) {
        rv = bcm56160_a0_bmd_port_mode_set(unit, port,
                                        port_mode, port_mode_flags);
    }

    return rv;
}

int
bcm56160_a0_bmd_switching_init(int unit)
{
    int ioerr = 0;
    int rv;
    int port, speed_max;
    cdk_pbmp_t pbmp, gpbmp, xlpbmp;
    uint32_t port_mode_flags;
    uint32_t pbm;
    EPC_LINK_BMAP_64r_t epc_link;
    bmd_port_mode_t port_mode;
    uint32_t vlan_flags;
    uint32_t sgmii_flags;

    rv = bcm56160_a0_bmd_reset(unit);
    if (CDK_SUCCESS(rv)) {
        rv = bcm56160_a0_bmd_init(unit);
    }

    if (CDK_SUCCESS(rv)) {
        rv = bcm56160_a0_bmd_vlan_create(unit, BMD_CONFIG_DEFAULT_VLAN);
    }

    vlan_flags = BMD_VLAN_PORT_F_UNTAGGED;

    sgmii_flags = (CDK_CHIP_CONFIG(unit) & DCFG_SGMII_MODE);
    bcm56160_a0_gport_pbmp_get(unit, &gpbmp);
    CDK_PBMP_ITER(gpbmp, port) {
        if (CDK_SUCCESS(rv)) {
            if (sgmii_flags) {
                if ((port >= 2 && port < 10) || (port >= 18 && port < 26)) {
                    speed_max = bcm56160_a0_port_speed_max(unit, port);
                    if (speed_max == 2500) {
                        rv = _config_port(unit, port, vlan_flags, bmdPortMode2500fd, 0);
                    } else {
                        rv = _config_port(unit, port, vlan_flags, bmdPortMode1000fd, 0);
                    }
                    continue;
                }
            }

            rv = _config_port(unit, port, vlan_flags, bmdPortModeAuto, 0);
        }
    }

    bcm56160_a0_xlport_pbmp_get(unit, &xlpbmp);
    CDK_PBMP_ITER(xlpbmp, port) {
        port_mode_flags = 0;
        if (BMD_PORT_PROPERTIES(unit, port) & BMD_PORT_HG) {
            port_mode = CDK_PORT_CONFIG_PORT_MODE(unit, port);
            if (port_mode == CDK_DCFG_PORT_MODE_HIGIG2) {
                port_mode_flags |= BMD_PORT_MODE_F_HIGIG2;
            } else {
                port_mode_flags |= BMD_PORT_MODE_F_HIGIG;
            }
        }

        speed_max = bcm56160_a0_port_speed_max(unit, port);
        switch (speed_max) {
        case 42000:
            port_mode = bmdPortMode40000fd;
            if (BMD_PORT_PROPERTIES(unit, port) & BMD_PORT_HG) {
                port_mode = bmdPortMode42000fd;
            }
            break;

        case 25000:
            port_mode = bmdPortMode25000fd;
            break;

        case 21000:
            port_mode = bmdPortMode20000fd;
            if (BMD_PORT_PROPERTIES(unit, port) & BMD_PORT_HG) {
                port_mode = bmdPortMode21000fd;
            }
            break;

        case 13000:
            port_mode = bmdPortMode10000fd;
            if (BMD_PORT_PROPERTIES(unit, port) & BMD_PORT_HG) {
                port_mode = bmdPortMode13000fd;
            }
            break;

        case 11000:
            port_mode = bmdPortMode10000fd;
            if (BMD_PORT_PROPERTIES(unit, port) & BMD_PORT_HG) {
                port_mode = bmdPortMode11000fd;
            }
            break;

        case 10000:
            port_mode = bmdPortMode10000fd;
            break;

        case 2500:
            port_mode = bmdPortMode2500fd;
            break;

        default:
            port_mode = bmdPortMode1000fd;
            break;
        }

        if (CDK_SUCCESS(rv)) {
            rv = _config_port(unit, port, vlan_flags, port_mode, port_mode_flags);
        }
    }

    vlan_flags = 0;
    if (CDK_SUCCESS(rv)) {
        rv = bcm56160_a0_bmd_vlan_port_add(unit, BMD_CONFIG_DEFAULT_VLAN,
                                           CMIC_PORT, vlan_flags);
    }

    /* Enable all ports in MMU */
    CDK_PBMP_CLEAR(pbmp);
    CDK_PBMP_OR(pbmp, gpbmp);
    CDK_PBMP_OR(pbmp, xlpbmp);
    EPC_LINK_BMAP_64r_CLR(epc_link);
    pbm = CDK_PBMP_WORD_GET(pbmp, 0);
    EPC_LINK_BMAP_64r_PORT_BITMAP_LOf_SET(epc_link, pbm);
    ioerr += WRITE_EPC_LINK_BMAP_64r(unit, epc_link);

    return ioerr ? CDK_E_IO : rv;
}

#endif /* CDK_CONFIG_INCLUDE_BCM56160_A0 */
