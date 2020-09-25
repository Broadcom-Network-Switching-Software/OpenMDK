#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56224_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#include <cdk/cdk_device.h>
#include <cdk/cdk_error.h>
#include <cdk/chip/bcm56224_a0_defs.h>

#include "bcm56224_a0_bmd.h"

int
bcm56224_a0_bmd_stat_clear(int unit, int port, bmd_stat_t stat)
{
    GTPKTr_t gtpkt;
    GTBYTr_t gtbyt;
    GTJBRr_t gtjbr;
    GTFCSr_t gtfcs;
    GTOVRr_t gtovr;
    GRPKTr_t grpkt;
    GRBYTr_t grbyt;
    GRJBRr_t grjbr;
    GRFCSr_t grfcs;
    GROVRr_t grovr;
    GRFLRr_t grflr;
    GRMTUEr_t grmtue;
    GRUNDr_t grund;
    GRFRGr_t grfrg;
    RRPKTr_t rrpkt;
    RDBGC0r_t rdbgc0;
    int ioerr = 0;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);

    if (BMD_PORT_PROPERTIES(unit, port) & (BMD_PORT_HG | BMD_PORT_ENET)) {
        switch (stat) {
        case bmdStatTxPackets:
            GTPKTr_CLR(gtpkt);
            ioerr += WRITE_GTPKTr(unit, port, gtpkt);
            break;
        case bmdStatTxBytes:
            GTBYTr_CLR(gtbyt);
            ioerr += WRITE_GTBYTr(unit, port, gtbyt);
            break;
        case bmdStatTxErrors:
            GTJBRr_CLR(gtjbr);
            ioerr += WRITE_GTJBRr(unit, port, gtjbr);
            GTFCSr_CLR(gtfcs);
            ioerr += WRITE_GTFCSr(unit, port, gtfcs);
            GTOVRr_CLR(gtovr);
            ioerr += WRITE_GTOVRr(unit, port, gtovr);
            break;
        case bmdStatRxPackets:
            GRPKTr_CLR(grpkt);
            ioerr += WRITE_GRPKTr(unit, port, grpkt);
            break;
        case bmdStatRxBytes:
            GRBYTr_CLR(grbyt);
            ioerr += WRITE_GRBYTr(unit, port, grbyt);
            break;
        case bmdStatRxErrors:
            GRJBRr_CLR(grjbr);
            ioerr += WRITE_GRJBRr(unit, port, grjbr);
            GRFCSr_CLR(grfcs);
            ioerr += WRITE_GRFCSr(unit, port, grfcs);
            GROVRr_CLR(grovr);
            ioerr += WRITE_GROVRr(unit, port, grovr);
            GRFLRr_CLR(grflr);
            ioerr += WRITE_GRFLRr(unit, port, grflr);
            GRMTUEr_CLR(grmtue);
            ioerr += WRITE_GRMTUEr(unit, port, grmtue);
            GRUNDr_CLR(grund);
            ioerr += WRITE_GRUNDr(unit, port, grund);
            GRFRGr_CLR(grfrg);
            ioerr += WRITE_GRFRGr(unit, port, grfrg);
            RRPKTr_CLR(rrpkt);
            ioerr += WRITE_RRPKTr(unit, port, rrpkt);
            break;
        case bmdStatRxDrops:
            RDBGC0r_CLR(rdbgc0);
            ioerr += WRITE_RDBGC0r(unit, port, rdbgc0);
            break;
        default:
            break;
        }
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

#endif /* CDK_CONFIG_INCLUDE_BCM56224_A0 */
