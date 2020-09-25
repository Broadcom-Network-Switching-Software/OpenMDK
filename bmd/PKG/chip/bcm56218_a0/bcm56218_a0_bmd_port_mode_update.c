#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56218_A0 == 1

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

#include <cdk/chip/bcm56218_a0_defs.h>
#include <cdk/cdk_device.h>
#include <cdk/cdk_error.h>

#include "bcm56218_a0_bmd.h"

#define LED_DATA_OFFSET 0xa0
static uint32_t linkdown_times[BMD_CONFIG_MAX_PORTS]={0};

int
bcm56218_a0_bmd_port_mode_update(int unit, int port)
{
    int rv = CDK_E_NONE;
    int ioerr = 0;
    int led_flags = 0;
    int status_change;
    EPC_LINK_BMAPr_t epc_link;
    EPC_LINK_BMAP_HIr_t epc_link_hi;
    uint32_t pbmp;
    bmd_port_mode_t mode;
    uint32_t flags;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);

    rv = bmd_link_update(unit, port, &status_change);
    if (CDK_SUCCESS(rv) && status_change) {
        if (!(BMD_PORT_STATUS(unit, port) & BMD_PST_LINK_UP)) {
            linkdown_times[port] += 1;
        }

        rv = bcm56218_a0_bmd_port_mode_get(unit, port, &mode, &flags);
        if (CDK_SUCCESS(rv)) {
            if(linkdown_times[port] < 2) {
                flags |= BMD_PORT_MODE_F_INTERNAL;
            } else {
                /* The link-down ports needs to drain packets, 
                   not to set BMD_PORT_MODE_F_INTERNAL*/
                linkdown_times[port]=0;
            }
            rv = bcm56218_a0_bmd_port_mode_set(unit, port, mode, flags);
        }
        if (port >= 32) {
            ioerr += READ_EPC_LINK_BMAP_HIr(unit, &epc_link_hi);
            pbmp = EPC_LINK_BMAP_HIr_PORT_BITMAPf_GET(epc_link_hi);
            if (BMD_PORT_STATUS(unit, port) & BMD_PST_LINK_UP) {
                pbmp |= (1 << (port - 32));
                led_flags = XGS_LED_LINK;
            } else {
                pbmp &= ~(1 << (port - 32));
            }
            EPC_LINK_BMAP_HIr_PORT_BITMAPf_SET(epc_link_hi, pbmp);
            ioerr += WRITE_EPC_LINK_BMAP_HIr(unit, epc_link_hi);
        } else {
            ioerr += READ_EPC_LINK_BMAPr(unit, &epc_link);
            pbmp = EPC_LINK_BMAPr_PORT_BITMAPf_GET(epc_link);
            if (BMD_PORT_STATUS(unit, port) & BMD_PST_LINK_UP) {
                pbmp |= (1 << port);
                led_flags = XGS_LED_LINK;
            } else {
                pbmp &= ~(1 << port);
            }
            EPC_LINK_BMAPr_PORT_BITMAPf_SET(epc_link, pbmp);
            ioerr += WRITE_EPC_LINK_BMAPr(unit, epc_link);
        }
        /* Update LED controller data */
        xgs_led_update(unit, LED_DATA_OFFSET + port, led_flags);
    }

    return ioerr ? CDK_E_IO : rv;
}
#endif /* CDK_CONFIG_INCLUDE_BCM56218_A0 */
