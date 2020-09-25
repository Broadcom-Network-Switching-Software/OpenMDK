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
#include <bmd/bmd_device.h>
#include <bmdi/bmd_link.h>
#include <bmdi/arch/xgsd_led.h>

#include <cdk/cdk_device.h>
#include <cdk/cdk_debug.h>
#include <cdk/cdk_error.h>
#include <cdk/chip/bcm56160_a0_defs.h>

#include "bcm56160_a0_bmd.h"
#include "bcm56160_a0_internal.h"

#define LED_DATA_OFFSET 0xa0
int
bcm56160_a0_bmd_port_mode_update(int unit, int port)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int led_flags = 0;
    int lport;
    int status_change;
    int autoneg;
    XLMAC_TX_CTRLr_t xlmac_tx_ctrl;
    EPC_LINK_BMAP_64r_t epc_link;
    uint32_t pbmp;
    bmd_port_mode_t mode;
    uint32_t flags;
    cdk_pbmp_t gpbmp, xlpbmp;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);

    lport = P2L(unit, port);

    bcm56160_a0_gport_pbmp_get(unit, &gpbmp);
    bcm56160_a0_xlport_pbmp_get(unit, &xlpbmp);

    rv = bmd_link_update(unit, port, &status_change);

    if (CDK_SUCCESS(rv) && status_change) {
        /* Update xlport */
        if (CDK_PBMP_MEMBER(xlpbmp, port)) {
            rv = bcm56160_a0_bmd_port_mode_get(unit, port, &mode, &flags);
            if (CDK_SUCCESS(rv)) {
                rv = bmd_phy_autoneg_get(unit, port, &autoneg);
                if (CDK_SUCCESS(rv) && autoneg) {
                    flags |= BMD_PORT_MODE_F_INTERNAL;
                    rv = bcm56160_a0_bmd_port_mode_set(unit, port, mode, flags);
                }
            }

            /* XLMAC required to drain Tx packets when link down */
            ioerr += READ_XLMAC_TX_CTRLr(unit, port, &xlmac_tx_ctrl);
            if (BMD_PORT_STATUS(unit, port) & BMD_PST_LINK_UP) {
                XLMAC_TX_CTRLr_DISCARDf_SET(xlmac_tx_ctrl, 0);
                XLMAC_TX_CTRLr_EP_DISCARDf_SET(xlmac_tx_ctrl, 0);
            } else {
                XLMAC_TX_CTRLr_DISCARDf_SET(xlmac_tx_ctrl, 1);
                XLMAC_TX_CTRLr_EP_DISCARDf_SET(xlmac_tx_ctrl, 1);
            }
            ioerr += WRITE_XLMAC_TX_CTRLr(unit, port, xlmac_tx_ctrl);
        }

        /* Update gport */
        if (CDK_PBMP_MEMBER(gpbmp, port)) {
            rv = bcm56160_a0_bmd_port_mode_get(unit, port, &mode, &flags);
            if (CDK_SUCCESS(rv)) {
                flags |= BMD_PORT_MODE_F_INTERNAL;
                rv = bcm56160_a0_bmd_port_mode_set(unit, port, mode, flags);
            }
        }

        /* Update link map */
        ioerr += READ_EPC_LINK_BMAP_64r(unit, &epc_link);
        pbmp = EPC_LINK_BMAP_64r_PORT_BITMAP_LOf_GET(epc_link);
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
#endif /* CDK_CONFIG_INCLUDE_BCM56160_A0 */
