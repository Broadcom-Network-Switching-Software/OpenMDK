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
#include <bmd/bmd_device.h>

#include <bmdi/bmd_link.h>
#include <bmdi/arch/xgs_led.h>

#include <cdk/chip/bcm56800_a0_defs.h>
#include <cdk/cdk_device.h>
#include <cdk/cdk_error.h>

#include "bcm56800_a0_bmd.h"

#define LED_DATA_OFFSET 0x80

int
bcm56800_a0_bmd_port_mode_update(int unit, int port)
{
    int rv = CDK_E_NONE;
    int ioerr = 0;
    int led_flags = 0;
    int status_change;
    EPC_LINK_BMAPr_t epc_link;
    uint32_t pbmp;
    bmd_port_mode_t mode;
    uint32_t flags;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);

    rv = bmd_link_update(unit, port, &status_change);
    if (CDK_SUCCESS(rv) && status_change) {
        rv = bcm56800_a0_bmd_port_mode_get(unit, port, &mode, &flags);
        if (CDK_SUCCESS(rv)) {
            flags |= BMD_PORT_MODE_F_INTERNAL;
            rv = bcm56800_a0_bmd_port_mode_set(unit, port, mode, flags);
        }
        ioerr += READ_EPC_LINK_BMAPr(unit, &epc_link);
        pbmp = EPC_LINK_BMAPr_PORT_BITMAPf_GET(epc_link);
        if (BMD_PORT_STATUS(unit, port) & BMD_PST_LINK_UP) {
            EPC_LINK_BMAPr_PORT_BITMAPf_SET(epc_link, pbmp | (1 << port));
            led_flags = XGS_LED_LINK;
        } else {
            EPC_LINK_BMAPr_PORT_BITMAPf_SET(epc_link, pbmp & ~(1 << port));
        }
        ioerr += WRITE_EPC_LINK_BMAPr(unit, epc_link);
        /* Update LED controller data */
        xgs_led_update(unit, LED_DATA_OFFSET + port, led_flags);
    }

    return ioerr ? CDK_E_IO : rv;
}
#endif /* CDK_CONFIG_INCLUDE_BCM56800_A0 */
