#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56840_A0 == 1

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

#include <cdk/chip/bcm56840_a0_defs.h>
#include <cdk/cdk_device.h>
#include <cdk/cdk_error.h>

#include "bcm56840_a0_bmd.h"
#include "bcm56840_a0_internal.h"

#define LED_DATA_OFFSET 0x80

int
bcm56840_a0_port_enable_set(int unit, int port, int mac_mode, int enable)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int lport;
    int gmac_en, xmac_en;
    COMMAND_CONFIGr_t command_cfg;
    XMAC_CTRLr_t xmac_ctrl;
    EPC_LINK_BMAPm_t epc_link;
    uint32_t pbm, set_mask, clr_mask;

    lport = P2L(unit, port);

    gmac_en = 0;
    xmac_en = 0;
    if (enable) {
        if (mac_mode == 1) {
            gmac_en = 1;
        } else {
            xmac_en = 1;
        }
    }

    /* Update GMAC */
    ioerr += READ_COMMAND_CONFIGr(unit, port, &command_cfg);
    COMMAND_CONFIGr_RX_ENAf_SET(command_cfg, gmac_en);
    COMMAND_CONFIGr_SW_RESETf_SET(command_cfg, !gmac_en);
    ioerr += WRITE_COMMAND_CONFIGr(unit, port, command_cfg);

    /* Update XMAC */
    ioerr += READ_XMAC_CTRLr(unit, port, &xmac_ctrl);
    XMAC_CTRLr_RX_ENf_SET(xmac_ctrl, xmac_en);
    XMAC_CTRLr_SOFT_RESETf_SET(xmac_ctrl, !xmac_en);
    ioerr += WRITE_XMAC_CTRLr(unit, port, xmac_ctrl);

    /* Update link map */
    ioerr += READ_EPC_LINK_BMAPm(unit, 0, &epc_link);
    clr_mask = LSHIFT32(1, lport & 0x1f);
    set_mask = 0;
    if (enable) {
        set_mask = clr_mask;
    }
    if (lport >= 64) {
        pbm = EPC_LINK_BMAPm_PORT_BITMAP_W2f_GET(epc_link);
        pbm &= ~clr_mask;
        pbm |= set_mask;
        EPC_LINK_BMAPm_PORT_BITMAP_W2f_SET(epc_link, pbm);
    } else if (lport >= 32) {
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

    /* Let PHYs know the new MAC state */
    rv = bmd_phy_notify_mac_enable(unit, port, enable);

    return ioerr ? CDK_E_IO : rv;
}

int
bcm56840_a0_bmd_port_mode_update(int unit, int port)
{
    int rv = CDK_E_NONE;
    int ioerr = 0;
    int led_flags;
    int status_change;
    int autoneg;
    int mac_mode, mac_en;
    bmd_port_mode_t mode;
    uint32_t flags;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);

    rv = bmd_link_update(unit, port, &status_change);
    if (CDK_SUCCESS(rv) && status_change) {
        rv = bmd_phy_autoneg_get(unit, port, &autoneg);
        if (CDK_SUCCESS(rv) && autoneg) {
            rv = bcm56840_a0_bmd_port_mode_get(unit, port, &mode, &flags);
            if (CDK_SUCCESS(rv)) {
                flags |= BMD_PORT_MODE_F_INTERNAL;
                rv = bcm56840_a0_bmd_port_mode_set(unit, port, mode, flags);
            }
        }

        /* Set configuration according to link state */
        mac_en = 0;
        led_flags = 0;
        mac_mode = 0;
        if (BMD_PORT_STATUS(unit, port) & BMD_PST_LINK_UP) {
            mac_en = 1;
            led_flags = XGS_LED_LINK;
            ioerr += bcm56840_a0_mac_mode_get(unit, port, &mac_mode);
        }
        if (CDK_SUCCESS(rv)) {
            rv = bcm56840_a0_port_enable_set(unit, port, mac_mode, mac_en);
        }

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
            if (led_flags & XGS_LED_LINK) {
                led_data |= 0x01;
            }
            CMIC_LEDUP1_DATA_RAMr_SET(led_data1, led_data);
            ioerr += WRITE_CMIC_LEDUP1_DATA_RAMr(unit, offset, led_data1);
        } else {
            xgs_led_update(unit, LED_DATA_OFFSET + port, led_flags);
        }
    }

    return ioerr ? CDK_E_IO : rv;
}
#endif /* CDK_CONFIG_INCLUDE_BCM56840_A0 */
