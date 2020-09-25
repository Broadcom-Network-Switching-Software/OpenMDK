/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56150_A0 == 1

#include <bmd/bmd.h>
#include <bmdi/bmd_port_mode.h>
#include <cdk/arch/xgsm_chip.h>
#include <cdk/cdk_debug.h>
#include <cdk/chip/bcm56150_a0_defs.h>
#include "bcm56150_a0_bmd.h"
#include "bcm56150_a0_internal.h"
int
bcm56150_a0_bmd_port_mode_get(int unit, int port, 
                              bmd_port_mode_t *mode, uint32_t* flags)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int get_phy_mode = 0;
    cdk_pbmp_t pbmp;
    int intf;
    COMMAND_CONFIGr_t command_cfg;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);

    *mode = bmdPortModeDisabled;
    *flags = 0;

    if (BMD_PORT_STATUS(unit, port) & BMD_PST_LINK_UP) {
        *flags |= BMD_PORT_MODE_F_LINK_UP;
    }

#if BMD_CONFIG_INCLUDE_HIGIG == 1 || BMD_CONFIG_INCLUDE_XE == 1
    bcm56150_a0_xlport_pbmp_get(unit, &pbmp);
    if (CDK_PBMP_MEMBER(pbmp, port)) {
        XLMAC_CTRLr_t mac_ctrl;
        XLPORT_CONFIGr_t xlport_cfg;
        XLMAC_MODEr_t xlmac_mode;        
        ioerr += READ_XLMAC_CTRLr(unit, port, &mac_ctrl);
        ioerr += READ_XLMAC_MODEr(unit, port, &xlmac_mode);
        *mode = bmdPortMode10000fd;
        if (ioerr == 0 && XLMAC_CTRLr_RX_ENf_GET(mac_ctrl)) {
            switch (XLMAC_MODEr_SPEED_MODEf_GET(xlmac_mode)) {
            case COMMAND_CONFIG_SPEED_10:
                *mode = bmdPortMode10fd;
                break;
            case COMMAND_CONFIG_SPEED_100:
                *mode = bmdPortMode100fd;
                break;
            case COMMAND_CONFIG_SPEED_1000:
                *mode = bmdPortMode1000fd;
                break;
            case COMMAND_CONFIG_SPEED_2500:
                *mode = bmdPortMode2500fd;
                break;
            case COMMAND_CONFIG_SPEED_10000:
                *mode = bmdPortMode10000fd;
                break;
            default:
                *mode = bmdPortMode10000fd;
                break;
            }
        }
        if (XLMAC_CTRLr_LOCAL_LPBKf_GET(mac_ctrl) != 0) {
            *flags |= BMD_PORT_MODE_F_MAC_LOOPBACK;
        } else {
            get_phy_mode = 1;
        }
        
        ioerr += READ_XLPORT_CONFIGr(unit, port, &xlport_cfg);
        if (XLPORT_CONFIGr_HIGIG_MODEf_GET(xlport_cfg)) {
            if (XLPORT_CONFIGr_HIGIG2_MODEf_GET(xlport_cfg)) {
                *flags |= BMD_PORT_MODE_F_HIGIG2;
            } else {
                *flags |= BMD_PORT_MODE_F_HIGIG;
            }
        }
    }
#endif
    
    CDK_XGSM_BLKTYPE_PBMP_GET(unit, BLKTYPE_GPORT, &pbmp);
    if (CDK_PBMP_MEMBER(pbmp, port)) {
        ioerr += READ_COMMAND_CONFIGr(unit, port, &command_cfg);
        if (ioerr == 0 && COMMAND_CONFIGr_RX_ENAf_GET(command_cfg)) {
            switch (COMMAND_CONFIGr_ETH_SPEEDf_GET(command_cfg)) {
            case COMMAND_CONFIG_SPEED_10:
                *mode = bmdPortMode10fd;
                break;
            case COMMAND_CONFIG_SPEED_100:
                *mode = bmdPortMode100fd;
                break;
            case COMMAND_CONFIG_SPEED_2500:
                *mode = bmdPortMode2500fd;
                break;
            default:
                *mode = bmdPortMode1000fd;
                break;
            }
            if (COMMAND_CONFIGr_LOOP_ENAf_GET(command_cfg) == 1) {
                *flags |= BMD_PORT_MODE_F_MAC_LOOPBACK;
            } else {
                get_phy_mode = 1;
            }
        }
    }

    if (get_phy_mode) {
        rv = bmd_port_mode_from_phy(unit, port, mode, flags);
        if (*mode == bmdPortMode10000fd) {
            if (CDK_SUCCESS(bmd_phy_line_interface_get(unit, port, &intf))) {
                if (intf == BMD_PHY_IF_XFI) {
                    *mode = bmdPortMode10000XFI;
                } else if (intf == BMD_PHY_IF_SFI) {
                    *mode = bmdPortMode10000SFI;
                } else if (intf == BMD_PHY_IF_KR) {
                    *mode = bmdPortMode10000KR;
                }
            }
        }
    } 

    return ioerr ? CDK_E_IO : rv; 
}
#endif /* CDK_CONFIG_INCLUDE_BCM56150_A0 */
