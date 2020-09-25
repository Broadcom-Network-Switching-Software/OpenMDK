/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56960_A0 == 1

#include <bmd/bmd.h>
#include <bmd/bmd_device.h>

#include <bmdi/bmd_link.h>
#include <bmdi/arch/xgsd_led.h>

#include <cdk/chip/bcm56960_a0_defs.h>
#include <cdk/cdk_device.h>
#include <cdk/cdk_error.h>

#include "bcm56960_a0_bmd.h"
#include "bcm56960_a0_internal.h"

#define LED_DATA_OFFSET 0xa0

static int
_led_update(int unit, int led_idx, int offset, uint32_t flags)
{
    int ram_offset;
    uint32_t led_data;
    
    if (led_idx == 0) {
        ram_offset = CMIC_LED_DATA_RAM(offset);
    } else if (led_idx == 1) {
        ram_offset = CMIC_LED1_DATA_RAM(offset);
    } else if (led_idx == 2) {
        ram_offset = CMIC_LED2_DATA_RAM(offset);
    } else {
        return CDK_E_INTERNAL;
    }

    CDK_DEV_READ32(unit, ram_offset, &led_data);

    led_data &= ~0x81;
    if (flags & XGSD_LED_LINK) {
        led_data |= 0x01;
    }
    if (flags & XGSD_LED_TURBO) {
        led_data |= 0x80;
    }

    CDK_DEV_WRITE32(unit, ram_offset, led_data);

    return CDK_E_NONE;
}

int
bcm56960_a0_bmd_port_mode_update(int unit, int port)
{
    int rv = CDK_E_NONE, ioerr = 0;
    int led_flags = 0;
    int status_change;
    int autoneg;
    int lport;
    EPC_LINK_BMAPm_t epc_link;
    uint32_t pbm[PBM_LPORT_WORDS];
    bmd_port_mode_t mode;
    uint32_t flags;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);

    lport = P2L(unit, port);

    rv = bmd_link_update(unit, port, &status_change);
    if (CDK_SUCCESS(rv) && status_change) {
        rv = bmd_phy_autoneg_get(unit, port, &autoneg);
        if (CDK_SUCCESS(rv) && autoneg) {
            rv = bcm56960_a0_bmd_port_mode_get(unit, port, &mode, &flags);
            if (CDK_SUCCESS(rv)) {
                flags |= BMD_PORT_MODE_F_INTERNAL;
                rv = bcm56960_a0_bmd_port_mode_set(unit, port, mode, flags);
            }
        }

        /* Update link map */
        ioerr += READ_EPC_LINK_BMAPm(unit, 0, &epc_link);
        EPC_LINK_BMAPm_PORT_BITMAPf_GET(epc_link, pbm);
        if (BMD_PORT_STATUS(unit, port) & BMD_PST_LINK_UP) {
            PBM_PORT_ADD(pbm, lport);
            led_flags |= XGSD_LED_LINK;
        } else {
            PBM_PORT_REMOVE(pbm, lport);
        }
        EPC_LINK_BMAPm_PORT_BITMAPf_SET(epc_link, pbm);
        ioerr += WRITE_EPC_LINK_BMAPm(unit, 0, epc_link);

        /* Update LED controller data */
        if (port == 129){
            rv = _led_update(unit, 2, LED_DATA_OFFSET + 0, led_flags);
        } else if (port == 131) {
            rv = _led_update(unit, 2, LED_DATA_OFFSET + 1, led_flags);
        } else if (port >= 33 && port <= 96) {
            rv = _led_update(unit,1, LED_DATA_OFFSET + (port - 33), led_flags);
        } else if ((port >= 1 && port <= 32) || (port >= 97 && port <= 128)) {
            if (port > 32) {
                rv = xgsd_led_update(unit, LED_DATA_OFFSET + (port - 97), 
                                     led_flags);
            } else {
                rv = xgsd_led_update(unit, LED_DATA_OFFSET + (port - 1), 
                                     led_flags);
            }
        }
    }

    return ioerr ? CDK_E_IO : rv;
}
#endif /* CDK_CONFIG_INCLUDE_BCM56960_A0 */

