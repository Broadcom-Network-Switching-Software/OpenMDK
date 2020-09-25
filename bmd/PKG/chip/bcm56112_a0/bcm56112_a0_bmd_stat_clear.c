#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56112_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#include <cdk/chip/bcm56112_a0_defs.h>
#include <cdk/cdk_device.h>
#include <cdk/cdk_error.h>
#include <cdk/cdk_string.h>

#include "bcm56112_a0_bmd.h"

int
bcm56112_a0_bmd_stat_clear(int unit, int port, bmd_stat_t stat)
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

    if (BMD_PORT_PROPERTIES(unit, port) & (BMD_PORT_HG | BMD_PORT_XE)) {
#if BMD_CONFIG_INCLUDE_HIGIG == 1 || BMD_CONFIG_INCLUDE_XE == 1
        ITPKTr_t itpkt;
        ITBYTr_t itbyt;
        ITFCSr_t itfcs;
        ITFRGr_t itfrg;
        ITOVRr_t itovr;
        ITUFLr_t itufl;
        ITERRr_t iterr;
        IRPKTr_t irpkt;
        IRBYTr_t irbyt;
        IRFCSr_t irfcs;
        IRJBRr_t irjbr;
        IROVRr_t irovr;
        IRMEGr_t irmeg;
        IRMEBr_t irmeb;
        IRFRGr_t irfrg;
        IRERPKTr_t irerpkt;

        switch (stat) {
        case bmdStatTxPackets:
            ITPKTr_CLR(itpkt);
            ioerr += WRITE_ITPKTr(unit, port, itpkt);
            break;
        case bmdStatTxBytes:
            ITBYTr_CLR(itbyt);
            ioerr += WRITE_ITBYTr(unit, port, itbyt);
            break;
        case bmdStatTxErrors:
            ITFRGr_CLR(itfrg);
            ioerr += WRITE_ITFRGr(unit, port, itfrg);
            ITFCSr_CLR(itfcs);
            ioerr += WRITE_ITFCSr(unit, port, itfcs);
            ITOVRr_CLR(itovr);
            ioerr += WRITE_ITOVRr(unit, port, itovr);
            ITUFLr_CLR(itufl);
            ioerr += WRITE_ITUFLr(unit, port, itufl);
            ITERRr_CLR(iterr);
            ioerr += WRITE_ITERRr(unit, port, iterr);
            break;
        case bmdStatRxPackets:
            IRPKTr_CLR(irpkt);
            ioerr += WRITE_IRPKTr(unit, port, irpkt);
            break;
        case bmdStatRxBytes:
            IRBYTr_CLR(irbyt);
            ioerr += WRITE_IRBYTr(unit, port, irbyt);
            break;
        case bmdStatRxErrors:
            IRFCSr_CLR(irfcs);
            ioerr += WRITE_IRFCSr(unit, port, irfcs);
            IRJBRr_CLR(irjbr);
            ioerr += WRITE_IRJBRr(unit, port, irjbr);
            IROVRr_CLR(irovr);
            ioerr += WRITE_IROVRr(unit, port, irovr);
            IRMEGr_CLR(irmeg);
            ioerr += WRITE_IRMEGr(unit, port, irmeg);
            IRMEBr_CLR(irmeb);
            ioerr += WRITE_IRMEBr(unit, port, irmeb);
            IRFRGr_CLR(irfrg);
            ioerr += WRITE_IRFRGr(unit, port, irfrg);
            IRERPKTr_CLR(irerpkt);
            ioerr += WRITE_IRERPKTr(unit, port, irerpkt);
            break;
        case bmdStatRxDrops:
            RDBGC0r_CLR(rdbgc0);
            ioerr += WRITE_RDBGC0r(unit, port, rdbgc0);
            break;
        default:
            break;
        }
#endif
    } else {
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

#endif /* CDK_CONFIG_INCLUDE_BCM56112_A0 */
