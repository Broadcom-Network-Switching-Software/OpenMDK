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

#include <bmdi/bmd_port_mode.h>

#include <cdk/chip/bcm56218_a0_defs.h>
#include <cdk/arch/xgs_chip.h>

#include "bcm56218_a0_bmd.h"

int
bcm56218_a0_bmd_port_mode_get(int unit, int port, 
                              bmd_port_mode_t *mode, uint32_t* flags)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    GMACC0r_t gmacc0;
    GMACC1r_t gmacc1;
    FE_MAC1r_t fe_mac1;
    GPORT_CONFIGr_t gport_cfg;
    GE_PORT_CONFIGr_t ge_port_config;
    PORT_TABm_t port_tab;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);

    *mode = bmdPortModeDisabled;
    *flags = 0;

    if (BMD_PORT_STATUS(unit, port) & BMD_PST_LINK_UP) {
        *flags |= BMD_PORT_MODE_F_LINK_UP;
    }

    ioerr += READ_PORT_TABm(unit, port, &port_tab);
    if (PORT_TABm_HIGIG_PACKETf_GET(port_tab)) {
        *flags |= BMD_PORT_MODE_F_HGLITE;
    }

    ioerr += READ_GMACC1r(unit, port, &gmacc1);
    ioerr += READ_FE_MAC1r(unit, port, &fe_mac1);
    if (GMACC1r_RXEN0f_GET(gmacc1) || FE_MAC1r_RX_ENf_GET(fe_mac1)) {
        ioerr += READ_GE_PORT_CONFIGr(unit, port, &ge_port_config);
        switch (GE_PORT_CONFIGr_SPEED_SELECTf_GET(ge_port_config)) {
        case 1:
            *mode = bmdPortMode100fd;
            break;
        case 2:
            *mode = bmdPortMode10fd;
            break;
        default:
            *mode = bmdPortMode1000fd;
            if (port == 1 || port == 2) {
                ioerr += READ_GPORT_CONFIGr(unit, &gport_cfg, port);
                if (port == 1) {
                    if (GPORT_CONFIGr_PLL_MODE_DEF_S0f_GET(gport_cfg)) {
                        *mode = bmdPortMode2500fd;
                    }
                } else { /* port = 2 */
                    if (GPORT_CONFIGr_PLL_MODE_DEF_S1f_GET(gport_cfg)) {
                        *mode = bmdPortMode2500fd;
                    }
                }
            }
            break;
        }
        ioerr += READ_GMACC0r(unit, port, &gmacc0);
        if (ioerr == 0 && GMACC0r_L32Bf_GET(gmacc0) == 1) {
            *flags |= BMD_PORT_MODE_F_MAC_LOOPBACK;
        } else {
            rv = bmd_port_mode_from_phy(unit, port, mode, flags);
        }
    }

    return ioerr ? CDK_E_IO : rv; 
}
#endif /* CDK_CONFIG_INCLUDE_BCM56218_A0 */
