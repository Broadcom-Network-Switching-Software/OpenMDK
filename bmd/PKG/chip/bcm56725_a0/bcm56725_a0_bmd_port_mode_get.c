#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56725_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#include <bmdi/bmd_port_mode.h>

#include <cdk/chip/bcm56725_a0_defs.h>
#include <cdk/arch/xgs_chip.h>

#include "bcm56725_a0_bmd.h"
#include "bcm56725_a0_internal.h"

int
bcm56725_a0_bmd_port_mode_get(int unit, int port, 
                              bmd_port_mode_t *mode, uint32_t* flags)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int get_phy_mode = 0;
    MAC_CTRLr_t mac_ctrl;
    XPORT_CONFIGr_t xport_cfg;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);

    *mode = bmdPortModeDisabled;
    *flags = 0;

    if (BMD_PORT_STATUS(unit, port) & BMD_PST_LINK_UP) {
        *flags |= BMD_PORT_MODE_F_LINK_UP;
    }

    ioerr += READ_MAC_CTRLr(unit, port, &mac_ctrl);
    if (MAC_CTRLr_RXENf_GET(mac_ctrl) != 0) {
        *mode = bmdPortMode10000fd;
        if (ioerr == 0 && MAC_CTRLr_LCLLOOPf_GET(mac_ctrl) != 0) {
            *flags |= BMD_PORT_MODE_F_MAC_LOOPBACK;
        } else {
            get_phy_mode = 1;
        }
        ioerr += READ_XPORT_CONFIGr(unit, port, &xport_cfg);
        if (XPORT_CONFIGr_HIGIG2_MODEf_GET(xport_cfg)) {
            *flags |= BMD_PORT_MODE_F_HIGIG2;
        } else {
            *flags |= BMD_PORT_MODE_F_HIGIG;
        }
    }
    if (get_phy_mode) {
        rv = bmd_port_mode_from_phy(unit, port, mode, flags);
    }

    return ioerr ? CDK_E_IO : rv; 
}
#endif /* CDK_CONFIG_INCLUDE_BCM56725_A0 */
