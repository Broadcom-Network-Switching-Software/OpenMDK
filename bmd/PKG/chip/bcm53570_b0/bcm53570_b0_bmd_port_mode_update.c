#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53570_B0 == 1

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

#include <cdk/chip/bcm53570_b0_defs.h>
#include <cdk/cdk_device.h>
#include <cdk/cdk_error.h>
#include <cdk/cdk_debug.h>

#include "bcm53570_b0_bmd.h"
#include "bcm53570_b0_internal.h"

int
bcm53570_b0_bmd_port_mode_update(int unit, int port)
{
    int rv = CDK_E_NONE;
    int ioerr = 0;
    int status_change;
    int autoneg;
    int lport;
    CLMAC_TX_CTRLr_t clmac_tx_ctrl;
    XLMAC_TX_CTRLr_t xlmac_tx_ctrl;
    EPC_LINK_BMAP_LO_64r_t epc_link_lo;
    EPC_LINK_BMAP_HI_64r_t epc_link_hi;
    bmd_port_mode_t mode;
    uint32_t flags;
    uint32_t pbmp[2];
    cdk_pbmp_t clpbmp, xlpbmp, qpbmp;
   
    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);
    
    lport = P2L(unit, port);

    bcm53570_b0_clport_pbmp_get(unit, &clpbmp);
    bcm53570_b0_xlport_pbmp_get(unit, &xlpbmp);
    bcm53570_b0_tscq_pbmp_get(unit, &qpbmp);

    rv = bmd_link_update(unit, port, &status_change);
    if (CDK_SUCCESS(rv) && status_change) {
        rv = bcm53570_b0_bmd_port_mode_get(unit, port, &mode, &flags);
        if (CDK_PBMP_MEMBER(clpbmp, port)) {
            if (CDK_SUCCESS(rv)) {
                rv = bmd_phy_autoneg_get(unit, port, &autoneg);
                if (CDK_SUCCESS(rv) && autoneg) {
                    flags |= BMD_PORT_MODE_F_INTERNAL;
                    rv = bcm53570_b0_bmd_port_mode_set(unit, port, mode, flags);
                }
            }

            /* XLMAC required to drain Tx packets when link down */
            ioerr += READ_CLMAC_TX_CTRLr(unit, port, &clmac_tx_ctrl);
            if (BMD_PORT_STATUS(unit, port) & BMD_PST_LINK_UP) {
                CLMAC_TX_CTRLr_DISCARDf_SET(clmac_tx_ctrl, 0);
                CLMAC_TX_CTRLr_EP_DISCARDf_SET(clmac_tx_ctrl, 0);
            } else {
                CLMAC_TX_CTRLr_DISCARDf_SET(clmac_tx_ctrl, 1);
                CLMAC_TX_CTRLr_EP_DISCARDf_SET(clmac_tx_ctrl, 1);
            }
            ioerr += WRITE_CLMAC_TX_CTRLr(unit, port, clmac_tx_ctrl);
        } else if (CDK_PBMP_MEMBER(xlpbmp, port)) {
            if (CDK_SUCCESS(rv)) {
                rv = bmd_phy_autoneg_get(unit, port, &autoneg);
                if (CDK_SUCCESS(rv) && autoneg) {
                    flags |= BMD_PORT_MODE_F_INTERNAL;
                    rv = bcm53570_b0_bmd_port_mode_set(unit, port, mode, flags);
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
        } else {
            if (CDK_SUCCESS(rv)) {
                if (IS_TSCQ(port)) {
                    rv = bmd_phy_autoneg_get(unit, port, &autoneg);
                    if (CDK_SUCCESS(rv) && autoneg) {
                        flags |= BMD_PORT_MODE_F_INTERNAL;
                        rv = bcm53570_b0_bmd_port_mode_set(unit, port, mode, flags);
                    }
                } else {
                    flags |= BMD_PORT_MODE_F_INTERNAL;
                    rv = bcm53570_b0_bmd_port_mode_set(unit, port, mode, flags);
                }
            }
        }

        
        if (lport < 64) {
            ioerr += READ_EPC_LINK_BMAP_LO_64r(unit, &epc_link_lo);
            EPC_LINK_BMAP_LO_64r_PORT_BITMAPf_GET(epc_link_lo, pbmp);
            if (BMD_PORT_STATUS(unit, port) & BMD_PST_LINK_UP) {
                PBM_PORT_ADD(pbmp, lport);
            } else {
                PBM_PORT_REMOVE(pbmp, lport);
            }
            EPC_LINK_BMAP_LO_64r_PORT_BITMAPf_SET(epc_link_lo, pbmp);
            ioerr += WRITE_EPC_LINK_BMAP_LO_64r(unit, epc_link_lo);
        } else {
            ioerr += READ_EPC_LINK_BMAP_HI_64r(unit, &epc_link_hi);
            pbmp[0] = EPC_LINK_BMAP_HI_64r_PORT_BITMAPf_GET(epc_link_hi);
            if (BMD_PORT_STATUS(unit, port) & BMD_PST_LINK_UP) {
                PBM_PORT_ADD(pbmp, lport - 64);
            } else {
                PBM_PORT_REMOVE(pbmp, lport - 64);
            }
            EPC_LINK_BMAP_HI_64r_PORT_BITMAPf_SET(epc_link_hi, pbmp[0]);
            ioerr += WRITE_EPC_LINK_BMAP_HI_64r(unit, epc_link_hi);
        }
    }
    
    return ioerr ? CDK_E_IO : rv;
}
#endif /* CDK_CONFIG_INCLUDE_BCM53570_B0 */
