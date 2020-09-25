#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56640_A0 == 1

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
#include <bmdi/arch/xgsm_led.h>

#include <cdk/chip/bcm56640_a0_defs.h>

#include "bcm56640_a0_bmd.h"
#include "bcm56640_a0_internal.h"

#define LED_DATA_OFFSET 0x80

int 
bcm56640_a0_bmd_port_mode_update(int unit, int port)
{
    int rv = CDK_E_NONE;
    int ioerr = 0;
    int led_flags = 0;
    int lport;
    int status_change;
    int autoneg;
    XMAC_CTRLr_t xmac_ctrl;
    EPC_LINK_BMAPm_t epc_link;
    uint32_t pbm, set_mask, clr_mask;
    bmd_port_mode_t mode;
    uint32_t flags;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);

    lport = P2L(unit, port);

    rv = bmd_link_update(unit, port, &status_change);
    if (CDK_SUCCESS(rv) && status_change) {
        rv = bmd_phy_autoneg_get(unit, port, &autoneg);
        if (CDK_SUCCESS(rv) && autoneg) {
            rv = bcm56640_a0_bmd_port_mode_get(unit, port, &mode, &flags);
            if (CDK_SUCCESS(rv)) {
                flags |= BMD_PORT_MODE_F_INTERNAL;
                rv = bcm56640_a0_bmd_port_mode_set(unit, port, mode, flags);
            }
        }
        /* XMAC soft reset required to drain Tx packets when link down */
        ioerr += READ_XMAC_CTRLr(unit, port, &xmac_ctrl);
        XMAC_CTRLr_SOFT_RESETf_SET(xmac_ctrl, 1);
        if (BMD_PORT_STATUS(unit, port) & BMD_PST_LINK_UP) {
            XMAC_CTRLr_SOFT_RESETf_SET(xmac_ctrl, 0);
        }
        ioerr += WRITE_XMAC_CTRLr(unit, port, xmac_ctrl);
        /* Update link map */
        ioerr += READ_EPC_LINK_BMAPm(unit, 0, &epc_link);
        clr_mask = LSHIFT32(1, lport & 0x1f);
        set_mask = 0;
        if (BMD_PORT_STATUS(unit, port) & BMD_PST_LINK_UP) {
            set_mask = clr_mask;
            led_flags = XGSM_LED_LINK;
        }
        if (lport >= 32) {
            pbm = EPC_LINK_BMAPm_PORT_BITMAP_W1f_GET(epc_link);
            pbm &= ~clr_mask;
            pbm |= set_mask;
            EPC_LINK_BMAPm_PORT_BITMAP_W1f_SET(epc_link, pbm);
        } else {
            pbm = EPC_LINK_BMAPm_PORT_BITMAP_W0f_GET(epc_link);
            pbm &= ~clr_mask;
            pbm |= set_mask;
            EPC_LINK_BMAPm_PORT_BITMAP_W0f_SET(epc_link, pbm);
        }
        ioerr += WRITE_EPC_LINK_BMAPm(unit, 0, epc_link);
        /* Update LED controller data */
        if (port > 36) {
            CMIC_LEDUP1_DATA_RAMr_t led_data1;
            uint32_t led_data;
            int offset;

            /* Update link status port LED processor #1 */
            offset = 0x80 + (port - 36);
            ioerr += READ_CMIC_LEDUP1_DATA_RAMr(unit, offset, &led_data1);
            led_data = CMIC_LEDUP1_DATA_RAMr_GET(led_data1);
            led_data &= ~0x81;
            if (led_flags & XGSM_LED_LINK) {
                led_data |= 0x01;
            }
            CMIC_LEDUP1_DATA_RAMr_SET(led_data1, led_data);
            ioerr += WRITE_CMIC_LEDUP1_DATA_RAMr(unit, offset, led_data1);
        } else {
            xgsm_led_update(unit, LED_DATA_OFFSET + port, led_flags);
        }
    }

    return ioerr ? CDK_E_IO : rv;
}
#endif /* CDK_CONFIG_INCLUDE_BCM56640_A0 */

