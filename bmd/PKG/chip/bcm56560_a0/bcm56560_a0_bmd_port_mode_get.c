/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56560_A0 == 1

#include <bmd/bmd.h>

#include <bmdi/bmd_port_mode.h>

#include <cdk/chip/bcm56560_a0_defs.h>
#include <cdk/cdk_debug.h>
#include "bcm56560_a0_bmd.h"
#include "bcm56560_a0_internal.h"

int
bcm56560_a0_bmd_port_mode_get(int unit, int port, 
                              bmd_port_mode_t *mode, uint32_t *flags)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int get_phy_mode = 0;
    int intf;
    XLMAC_CTRLr_t xlmac_ctrl;
    XLPORT_CONFIGr_t xlport_cfg;
    CLMAC_CTRLr_t clmac_ctrl;
    CLPORT_CONFIGr_t cport_cfg;
    XLMAC_B0_CTRLr_t xlbmac_ctrl;
    cdk_pbmp_t clpbmp, xlpbmp, cxxpbmp;
    
    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);
    
    *mode = bmdPortModeDisabled;
    *flags = 0;

    if (BMD_PORT_STATUS(unit, port) & BMD_PST_DISABLED) {
        return CDK_E_NONE;
    }

    if (BMD_PORT_STATUS(unit, port) & BMD_PST_LINK_UP) {
        *flags |= BMD_PORT_MODE_F_LINK_UP;
    }
    bcm56560_a0_clport_pbmp_get(unit, &clpbmp);
    bcm56560_a0_xlport_pbmp_get(unit, &xlpbmp);
    bcm56560_a0_cxxport_pbmp_get(unit, &cxxpbmp);
    CDK_PBMP_OR(xlpbmp, cxxpbmp);
    
    if ((SPEED_MAX(unit, port) >= 100000) ||
         CDK_PBMP_MEMBER(clpbmp, port)) {
         
        ioerr += READ_CLMAC_CTRLr(unit, port, &clmac_ctrl);
        *mode = bmdPortMode10000fd;
        if (CLMAC_CTRLr_LOCAL_LPBKf_GET(clmac_ctrl) != 0) {
            *flags |= BMD_PORT_MODE_F_MAC_LOOPBACK;
        } else {
            get_phy_mode = 1;
        }
        ioerr += READ_CLPORT_CONFIGr(unit, port, &cport_cfg);
        if (CLPORT_CONFIGr_HIGIG_MODEf_GET(cport_cfg)) {
            if (CLPORT_CONFIGr_HIGIG2_MODEf_GET(cport_cfg)) {
                *flags |= BMD_PORT_MODE_F_HIGIG2;
            } else {
                *flags |= BMD_PORT_MODE_F_HIGIG;
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
                    } else if (intf == BMD_PHY_IF_CR) {
                        *mode = bmdPortMode10000CR;
                    }
                }
            } else if (*mode == bmdPortMode20000fd) {                
                if (CDK_SUCCESS(bmd_phy_line_interface_get(unit, port, &intf))) {
                    if (intf == BMD_PHY_IF_KR) {
                        *mode = bmdPortMode20000KR;
                    }
                }
            } else if (*mode == bmdPortMode25000fd) {
                if (CDK_SUCCESS(bmd_phy_line_interface_get(unit, port, &intf))) {
                    if (intf == BMD_PHY_IF_XFI) {
                        *mode = bmdPortMode25000XFI;
                    }
                }
            } else if (*mode == bmdPortMode40000fd) {                
                if (CDK_SUCCESS(bmd_phy_line_interface_get(unit, port, &intf))) {
                    if (intf == BMD_PHY_IF_CR) {
                        *mode = bmdPortMode40000CR;
                    } else if (intf == BMD_PHY_IF_KR) {
                        *mode = bmdPortMode40000KR;
                    } else if (intf == BMD_PHY_IF_SR) {
                        *mode = bmdPortMode40000SR;
                    }
                }
            } else if (*mode == bmdPortMode100000fd) {
                if (CDK_SUCCESS(bmd_phy_line_interface_get(unit, port, &intf))) {
                    if (intf == BMD_PHY_IF_CR) {
                        *mode = bmdPortMode100000CR;
                    } else if (intf == BMD_PHY_IF_KR) {
                        *mode = bmdPortMode100000KR;
                    } else if (intf == BMD_PHY_IF_SR) {
                        *mode = bmdPortMode100000SR;
                    }
                }
            }
        }
    } else {
        if (CDK_PBMP_MEMBER(xlpbmp, port)) {
            ioerr += READ_XLMAC_CTRLr(unit, port, &xlmac_ctrl);
            
            *mode = bmdPortMode10000fd;
            if (XLMAC_CTRLr_LOCAL_LPBKf_GET(xlmac_ctrl) != 0) {
                *flags |= BMD_PORT_MODE_F_MAC_LOOPBACK;
            } else {
                get_phy_mode = 1;
            }
        } else {
            /* XLBLK_B */
            ioerr += READ_XLMAC_B0_CTRLr(unit, port, &xlbmac_ctrl);
            
            *mode = bmdPortMode10000fd;
            if (XLMAC_B0_CTRLr_LOCAL_LPBKf_GET(xlbmac_ctrl) != 0) {
                *flags |= BMD_PORT_MODE_F_MAC_LOOPBACK;
            } else {
                get_phy_mode = 1;
            }
        }

        ioerr += READ_XLPORT_CONFIGr(unit, port, &xlport_cfg);
        if (XLPORT_CONFIGr_HIGIG_MODEf_GET(xlport_cfg)) {
            if (XLPORT_CONFIGr_HIGIG2_MODEf_GET(xlport_cfg)) {
                *flags |= BMD_PORT_MODE_F_HIGIG2;
            } else {
                *flags |= BMD_PORT_MODE_F_HIGIG;
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
            } else if (*mode == bmdPortMode40000fd) {
                if (CDK_SUCCESS(bmd_phy_line_interface_get(unit, port, &intf))) {
                    if (intf == BMD_PHY_IF_KR) {
                        *mode = bmdPortMode40000KR;
                    } else if (intf == BMD_PHY_IF_CR) {
                        *mode = bmdPortMode40000CR;
                    }
                }
            }
        }
    }

    return ioerr ? CDK_E_IO : rv; 
}
#endif /* CDK_CONFIG_INCLUDE_BCM56560_A0 */

