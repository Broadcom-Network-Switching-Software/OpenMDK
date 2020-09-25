#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56260_A0 == 1

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

#include <cdk/chip/bcm56260_a0_defs.h>
#include <cdk/cdk_device.h>
#include <cdk/cdk_error.h>
#include <cdk/cdk_debug.h>

#include "bcm56260_a0_bmd.h"
#include "bcm56260_a0_internal.h"

#define LED_DATA_OFFSET 0x80

int
bcm56260_a0_bmd_port_mode_update(int unit, int port)
{
    int rv = CDK_E_NONE;
    int ioerr = 0;
    int status_change;
    int ppport;
    EPC_LINK_BMAPm_t epc_link;
    uint32_t pbm, set_mask;
    
    bmd_port_mode_t mode;
    uint32_t flags;
    
    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);
    ppport = P2PP(unit, port);

    rv = bmd_link_update(unit, port, &status_change);
    if (CDK_SUCCESS(rv) && status_change) {
        rv = bcm56260_a0_bmd_port_mode_get(unit, port, &mode, &flags);
        if (CDK_SUCCESS(rv)) {
            flags |= BMD_PORT_MODE_F_INTERNAL;
            rv = bcm56260_a0_bmd_port_mode_set(unit, port, mode, flags);
        }
        
        ioerr += READ_EPC_LINK_BMAPm(unit, 0, &epc_link);
        pbm = EPC_LINK_BMAPm_GET(epc_link, (ppport >> 5));

        set_mask = LSHIFT32(1, ppport & 0x1f);
        if (BMD_PORT_STATUS(unit, port) & BMD_PST_LINK_UP) {
            pbm |= set_mask;
        } else {
            pbm &= ~set_mask;
        }
        EPC_LINK_BMAPm_SET(epc_link, (ppport >> 5), pbm);
        ioerr += WRITE_EPC_LINK_BMAPm(unit, 0, epc_link);
    }
    return ioerr ? CDK_E_IO : rv;
}
#endif /* CDK_CONFIG_INCLUDE_BCM56260_A0 */
