/*
 *
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56560_B0 == 1

#include <bmd/bmd.h>

#include <bmdi/bmd_link.h>
#include <bmdi/arch/xgsd_led.h>
#include <cdk/cdk_debug.h>
#include <cdk/chip/bcm56560_b0_defs.h>

#include "bcm56560_b0_bmd.h"
#include "bcm56560_b0_internal.h"

#define LED_DATA_OFFSET 0x80

int
bcm56560_b0_bmd_port_mode_update(int unit, int port)
{
    int rv = CDK_E_NONE, ioerr = 0;
    int led_flags = 0;
    int status_change;
    int autoneg;
    int lport;
    EPC_LINK_BMAPm_t epc_link;
    ING_DEST_PORT_ENABLEm_t ing_dst_port_en;
    uint32_t pbm[3], ing_pbm[3];

    bmd_port_mode_t mode;
    uint32_t flags;


    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);

    lport = P2L(unit, port);

    rv = bmd_link_update(unit, port, &status_change);

    if (CDK_SUCCESS(rv) && status_change) {
        rv = bcm56560_b0_bmd_port_mode_get(unit, port, &mode, &flags);
        if (CDK_SUCCESS(rv)) {
            rv = bmd_phy_autoneg_get(unit, port, &autoneg);
            if (CDK_SUCCESS(rv) && autoneg) {
                flags |= BMD_PORT_MODE_F_INTERNAL;
                rv = bcm56560_b0_bmd_port_mode_set(unit, port, mode, flags);
            }
        }

        /* Update link map */
        ioerr += READ_EPC_LINK_BMAPm(unit, 0, &epc_link);
        ioerr += READ_ING_DEST_PORT_ENABLEm(unit, 0, &ing_dst_port_en);
        EPC_LINK_BMAPm_PORT_BITMAPf_GET(epc_link, pbm);
        ING_DEST_PORT_ENABLEm_PORT_BITMAPf_GET(ing_dst_port_en, ing_pbm);
        if (BMD_PORT_STATUS(unit, port) & BMD_PST_LINK_UP) {
            PBM_PORT_ADD(pbm, lport);
            PBM_PORT_ADD(ing_pbm, lport);
        } else {
            PBM_PORT_REMOVE(pbm, lport);
            PBM_PORT_REMOVE(ing_pbm, lport);
        }
        EPC_LINK_BMAPm_PORT_BITMAPf_SET(epc_link, pbm);
        ioerr += WRITE_EPC_LINK_BMAPm(unit, 0, epc_link);

        ING_DEST_PORT_ENABLEm_PORT_BITMAPf_SET(ing_dst_port_en, ing_pbm);
        ioerr += WRITE_ING_DEST_PORT_ENABLEm(unit, 0, ing_dst_port_en);

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
            if (led_flags & XGSD_LED_LINK) {
                led_data |= 0x01;
            }
            CMIC_LEDUP1_DATA_RAMr_SET(led_data1, led_data);
            ioerr += WRITE_CMIC_LEDUP1_DATA_RAMr(unit, offset, led_data1);
        } else {
            xgsd_led_update(unit, LED_DATA_OFFSET + port, led_flags);
        }

    }
    return ioerr ? CDK_E_IO : rv;
}
#endif /* CDK_CONFIG_INCLUDE_BCM56560_B0 */

