#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56850_A0 == 1

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

#include <cdk/chip/bcm56850_a0_defs.h>
#include <cdk/cdk_device.h>
#include <cdk/cdk_error.h>

#include "bcm56850_a0_bmd.h"
#include "bcm56850_a0_internal.h"

#define LED_DATA_OFFSET 0x80

int
bcm56850_a0_port_enable_set(int unit, int port, int enable)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int lport;
    XLMAC_CTRLr_t xlmac_ctrl;
    EPC_LINK_BMAPm_t epc_link;
    uint32_t pbm[PBM_LPORT_WORDS];

    lport = P2L(unit, port);

    /* Update XMAC */
    ioerr += READ_XLMAC_CTRLr(unit, port, &xlmac_ctrl);
    XLMAC_CTRLr_RX_ENf_SET(xlmac_ctrl, enable);
    XLMAC_CTRLr_SOFT_RESETf_SET(xlmac_ctrl, !enable);
    ioerr += WRITE_XLMAC_CTRLr(unit, port, xlmac_ctrl);

    /* Update link map */
    ioerr += READ_EPC_LINK_BMAPm(unit, 0, &epc_link);
    EPC_LINK_BMAPm_PORT_BITMAPf_GET(epc_link, pbm);
    if (enable) {
        PBM_PORT_ADD(pbm, lport);
    } else {
        PBM_PORT_REMOVE(pbm, lport);
    }
    EPC_LINK_BMAPm_PORT_BITMAPf_SET(epc_link, pbm);
    ioerr += WRITE_EPC_LINK_BMAPm(unit, 0, epc_link);

    /* Let PHYs know the new MAC state */
    rv = bmd_phy_notify_mac_enable(unit, port, enable);

    return ioerr ? CDK_E_IO : rv;
}

int
bcm56850_a0_bmd_port_mode_update(int unit, int port)
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
            rv = bcm56850_a0_bmd_port_mode_get(unit, port, &mode, &flags);
            if (CDK_SUCCESS(rv)) {
                flags |= BMD_PORT_MODE_F_INTERNAL;
                rv = bcm56850_a0_bmd_port_mode_set(unit, port, mode, flags);
            }
        }

        /* Update link map */
        ioerr += READ_EPC_LINK_BMAPm(unit, 0, &epc_link);
        EPC_LINK_BMAPm_PORT_BITMAPf_GET(epc_link, pbm);
        if (BMD_PORT_STATUS(unit, port) & BMD_PST_LINK_UP) {
            PBM_PORT_ADD(pbm, lport);
        } else {
            PBM_PORT_REMOVE(pbm, lport);
        }
        EPC_LINK_BMAPm_PORT_BITMAPf_SET(epc_link, pbm);
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
#endif /* CDK_CONFIG_INCLUDE_BCM56850_A0 */
