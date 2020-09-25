#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56680_A0 == 1

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
#include <cdk/chip/bcm56680_a0_defs.h>

#include "bcm56680_a0_bmd.h"

int
bcm56680_a0_bmd_stat_get(int unit, int port, bmd_stat_t stat, bmd_counter_t *counter)
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

    CDK_MEMSET(counter, 0, sizeof(*counter));

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

        /* Only support lower 32-bits of counters */
        switch (stat) {
        case bmdStatTxPackets:
            ioerr += READ_ITPKTr(unit, port, &itpkt);
            counter->v[0] += ITPKTr_GET(itpkt, 0);
            break;
        case bmdStatTxBytes:
            ioerr += READ_ITBYTr(unit, port, &itbyt);
            counter->v[0] += ITBYTr_GET(itbyt, 0);
            break;
        case bmdStatTxErrors:
            ioerr += READ_ITFRGr(unit, port, &itfrg);
            counter->v[0] += ITFRGr_GET(itfrg, 0);
            ioerr += READ_ITFCSr(unit, port, &itfcs);
            counter->v[0] += ITFCSr_GET(itfcs, 0);
            ioerr += READ_ITOVRr(unit, port, &itovr);
            counter->v[0] += ITOVRr_GET(itovr, 0);
            ioerr += READ_ITUFLr(unit, port, &itufl);
            counter->v[0] += ITUFLr_GET(itufl, 0);
            ioerr += READ_ITERRr(unit, port, &iterr);
            counter->v[0] += ITERRr_GET(iterr, 0);
            break;
        case bmdStatRxPackets:
            ioerr += READ_IRPKTr(unit, port, &irpkt);
            counter->v[0] += IRPKTr_GET(irpkt, 0);
            break;
        case bmdStatRxBytes:
            ioerr += READ_IRBYTr(unit, port, &irbyt);
            counter->v[0] += IRBYTr_GET(irbyt, 0);
            break;
        case bmdStatRxErrors:
            ioerr += READ_IRFCSr(unit, port, &irfcs);
            counter->v[0] += IRFCSr_GET(irfcs, 0);
            ioerr += READ_IRJBRr(unit, port, &irjbr);
            counter->v[0] += IRJBRr_GET(irjbr, 0);
            ioerr += READ_IROVRr(unit, port, &irovr);
            counter->v[0] += IROVRr_GET(irovr, 0);
            ioerr += READ_IRMEGr(unit, port, &irmeg);
            counter->v[0] += IRMEGr_GET(irmeg, 0);
            ioerr += READ_IRMEBr(unit, port, &irmeb);
            counter->v[0] += IRMEBr_GET(irmeb, 0);
            ioerr += READ_IRFRGr(unit, port, &irfrg);
            counter->v[0] += IRFRGr_GET(irfrg, 0);
            ioerr += READ_IRERPKTr(unit, port, &irerpkt);
            counter->v[0] += IRERPKTr_GET(irerpkt, 0);
            break;
        default:
            break;
        }
#endif
    }
    switch (stat) {
    case bmdStatTxPackets:
        ioerr += READ_GTPKTr(unit, port, &gtpkt);
        counter->v[0] += GTPKTr_GET(gtpkt);
        break;
    case bmdStatTxBytes:
        ioerr += READ_GTBYTr(unit, port, &gtbyt);
        counter->v[0] += GTBYTr_GET(gtbyt);
        break;
    case bmdStatTxErrors:
        ioerr += READ_GTJBRr(unit, port, &gtjbr);
        counter->v[0] += GTJBRr_GET(gtjbr);
        ioerr += READ_GTFCSr(unit, port, &gtfcs);
        counter->v[0] += GTFCSr_GET(gtfcs);
        ioerr += READ_GTOVRr(unit, port, &gtovr);
        counter->v[0] += GTOVRr_GET(gtovr);
        break;
    case bmdStatRxPackets:
        ioerr += READ_GRPKTr(unit, port, &grpkt);
        counter->v[0] += GRPKTr_GET(grpkt);
        break;
    case bmdStatRxBytes:
        ioerr += READ_GRBYTr(unit, port, &grbyt);
        counter->v[0] += GRBYTr_GET(grbyt);
        break;
    case bmdStatRxErrors:
        ioerr += READ_GRJBRr(unit, port, &grjbr);
        counter->v[0] += GRJBRr_GET(grjbr);
        ioerr += READ_GRFCSr(unit, port, &grfcs);
        counter->v[0] += GRFCSr_GET(grfcs);
        ioerr += READ_GROVRr(unit, port, &grovr);
        counter->v[0] += GROVRr_GET(grovr);
        ioerr += READ_GRFLRr(unit, port, &grflr);
        counter->v[0] += GRFLRr_GET(grflr);
        ioerr += READ_GRMTUEr(unit, port, &grmtue);
        counter->v[0] += GRMTUEr_GET(grmtue);
        ioerr += READ_GRUNDr(unit, port, &grund);
        counter->v[0] += GRUNDr_GET(grund);
        ioerr += READ_GRFRGr(unit, port, &grfrg);
        counter->v[0] += GRFRGr_GET(grfrg);
        ioerr += READ_RRPKTr(unit, port, &rrpkt);
        counter->v[0] += RRPKTr_GET(rrpkt);
        break;
    case bmdStatRxDrops:
        ioerr += READ_RDBGC0r(unit, port, &rdbgc0);
        counter->v[0] += RDBGC0r_GET(rdbgc0);
        break;
    default:
        break;
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

#endif /* CDK_CONFIG_INCLUDE_BCM56680_A0 */
