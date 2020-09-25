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
#include <bmd/bmd_device.h>
#include <bmdi/bmd_link.h>
#include <bmdi/arch/xgsd_led.h>

#include <cdk/cdk_device.h>
#include <cdk/cdk_debug.h>
#include <cdk/cdk_error.h>
#include <cdk/chip/bcm53540_a0_defs.h>

#include "bcm53540_a0_bmd.h"
#include "bcm53540_a0_internal.h"

#define LED_DATA_OFFSET 0xa0
int
bcm53540_a0_bmd_port_mode_update(int unit, int port)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int led_flags = 0;
    int lport;
    int status_change;
    EPC_LINK_BMAP_64r_t epc_link;
    uint32_t pbmp;
    bmd_port_mode_t mode;
    uint32_t flags;
    cdk_pbmp_t gpbmp;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);

    rv = bmd_link_update(unit, port, &status_change);

    if (CDK_SUCCESS(rv) && status_change) {
        bcm53540_a0_gport_pbmp_get(unit, &gpbmp);
        if (CDK_PBMP_MEMBER(gpbmp, port)) {
            rv = bcm53540_a0_bmd_port_mode_get(unit, port, &mode, &flags);
            if (CDK_SUCCESS(rv)) {
                flags |= BMD_PORT_MODE_F_INTERNAL;
                rv = bcm53540_a0_bmd_port_mode_set(unit, port, mode, flags);
            }
        }

        /* Update link map */
        ioerr += READ_EPC_LINK_BMAP_64r(unit, &epc_link);
        pbmp = EPC_LINK_BMAP_64r_PORT_BITMAP_LOf_GET(epc_link);
        lport = P2L(unit, port);
        if (BMD_PORT_STATUS(unit, port) & BMD_PST_LINK_UP) {
            pbmp |= (1 << lport);
            led_flags = XGSD_LED_LINK;
        } else {
            pbmp &= ~(1 << lport);
        }
        EPC_LINK_BMAP_64r_PORT_BITMAP_LOf_SET(epc_link, pbmp);
        ioerr += WRITE_EPC_LINK_BMAP_64r(unit, epc_link);
        /* Update LED controller data */
        xgsd_led_update(unit, LED_DATA_OFFSET + port, led_flags);
    }

    return ioerr ? CDK_E_IO : rv;
}
#endif /* CDK_CONFIG_INCLUDE_BCM53540_A0 */
