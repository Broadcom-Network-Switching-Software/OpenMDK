#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56685_B0 == 1

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
#include <cdk/chip/bcm56685_b0_defs.h>

#include "bcm56685_b0_bmd.h"

typedef union {
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
} bcm56685_b0_counter_t;

int
bcm56685_b0_bmd_stat_clear(int unit, int port, bmd_stat_t stat)
{
    int ioerr = 0;
    bcm56685_b0_counter_t ctr;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);

    CDK_MEMSET(&ctr, 0, sizeof(ctr));

    if (BMD_PORT_PROPERTIES(unit, port) & (BMD_PORT_HG | BMD_PORT_XE)) {
#if BMD_CONFIG_INCLUDE_HIGIG == 1 || BMD_CONFIG_INCLUDE_XE == 1
        switch (stat) {
        case bmdStatTxPackets:
            ioerr += WRITE_ITPKTr(unit, port, ctr.itpkt);
            break;
        case bmdStatTxBytes:
            ioerr += WRITE_ITBYTr(unit, port, ctr.itbyt);
            break;
        case bmdStatTxErrors:
            ioerr += WRITE_ITFRGr(unit, port, ctr.itfrg);
            ioerr += WRITE_ITFCSr(unit, port, ctr.itfcs);
            ioerr += WRITE_ITOVRr(unit, port, ctr.itovr);
            ioerr += WRITE_ITUFLr(unit, port, ctr.itufl);
            ioerr += WRITE_ITERRr(unit, port, ctr.iterr);
            break;
        case bmdStatRxPackets:
            ioerr += WRITE_IRPKTr(unit, port, ctr.irpkt);
            break;
        case bmdStatRxBytes:
            ioerr += WRITE_IRBYTr(unit, port, ctr.irbyt);
            break;
        case bmdStatRxErrors:
            ioerr += WRITE_IRFCSr(unit, port, ctr.irfcs);
            ioerr += WRITE_IRJBRr(unit, port, ctr.irjbr);
            ioerr += WRITE_IROVRr(unit, port, ctr.irovr);
            ioerr += WRITE_IRMEGr(unit, port, ctr.irmeg);
            ioerr += WRITE_IRMEBr(unit, port, ctr.irmeb);
            ioerr += WRITE_IRFRGr(unit, port, ctr.irfrg);
            ioerr += WRITE_IRERPKTr(unit, port, ctr.irerpkt);
            break;
        default:
            break;
        }
#endif
    }
    switch (stat) {
    case bmdStatTxPackets:
        ioerr += WRITE_GTPKTr(unit, port, ctr.gtpkt);
        break;
    case bmdStatTxBytes:
        ioerr += WRITE_GTBYTr(unit, port, ctr.gtbyt);
        break;
    case bmdStatTxErrors:
        ioerr += WRITE_GTJBRr(unit, port, ctr.gtjbr);
        ioerr += WRITE_GTFCSr(unit, port, ctr.gtfcs);
        ioerr += WRITE_GTOVRr(unit, port, ctr.gtovr);
        break;
    case bmdStatRxPackets:
        ioerr += WRITE_GRPKTr(unit, port, ctr.grpkt);
        break;
    case bmdStatRxBytes:
        ioerr += WRITE_GRBYTr(unit, port, ctr.grbyt);
        break;
    case bmdStatRxErrors:
        ioerr += WRITE_GRJBRr(unit, port, ctr.grjbr);
        ioerr += WRITE_GRFCSr(unit, port, ctr.grfcs);
        ioerr += WRITE_GROVRr(unit, port, ctr.grovr);
        ioerr += WRITE_GRFLRr(unit, port, ctr.grflr);
        ioerr += WRITE_GRMTUEr(unit, port, ctr.grmtue);
        ioerr += WRITE_GRUNDr(unit, port, ctr.grund);
        ioerr += WRITE_GRFRGr(unit, port, ctr.grfrg);
        ioerr += WRITE_RRPKTr(unit, port, ctr.rrpkt);
        break;
    case bmdStatRxDrops:
        ioerr += WRITE_RDBGC0r(unit, port, ctr.rdbgc0);
        break;
    default:
        break;
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

#endif /* CDK_CONFIG_INCLUDE_BCM56685_B0 */
