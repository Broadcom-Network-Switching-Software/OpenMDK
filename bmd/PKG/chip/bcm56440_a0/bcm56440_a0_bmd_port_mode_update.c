#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56440_A0 == 1

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

#include <cdk/chip/bcm56440_a0_defs.h>
#include <cdk/cdk_device.h>
#include <cdk/cdk_error.h>

#include "bcm56440_a0_bmd.h"
#include "bcm56440_a0_internal.h"

#define LED_DATA_OFFSET 0x80

int
bcm56440_a0_bmd_port_mode_update(int unit, int port)
{
    int rv = CDK_E_NONE;
    int ioerr = 0;
    int lport;
    int status_change;
    int autoneg;
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
            rv = bcm56440_a0_bmd_port_mode_get(unit, port, &mode, &flags);
            if (CDK_SUCCESS(rv)) {
                flags |= BMD_PORT_MODE_F_INTERNAL;
                rv = bcm56440_a0_bmd_port_mode_set(unit, port, mode, flags);
            }
        }
        ioerr += READ_EPC_LINK_BMAPm(unit, 0, &epc_link);
        clr_mask = LSHIFT32(1, lport & 0x1f);
        set_mask = 0;
        if (BMD_PORT_STATUS(unit, port) & BMD_PST_LINK_UP) {
            set_mask = clr_mask;
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
    }

    return ioerr ? CDK_E_IO : rv;
}
#endif /* CDK_CONFIG_INCLUDE_BCM56440_A0 */
