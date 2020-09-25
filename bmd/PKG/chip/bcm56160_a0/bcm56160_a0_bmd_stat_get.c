/*
 *
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56160_A0 == 1

#include <bmd/bmd.h>
#include <cdk/cdk_device.h>
#include <cdk/cdk_debug.h>
#include <cdk/cdk_error.h>
#include <cdk/chip/bcm56160_a0_defs.h>
#include "bcm56160_a0_bmd.h"
#include "bcm56160_a0_internal.h"

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
    GRRPKTr_t grrpkt;
    TPKTr_t tpkt;
    TBYTr_t tbyt;
    TFCSr_t tfcs;
    TFRGr_t tfrg;
    TOVRr_t tovr;
    TUFLr_t tufl;
    TERRr_t terr;
    RPKTr_t rpkt;
    RBYTr_t rbyt;
    RFCSr_t rfcs;
    RJBRr_t rjbr;
    ROVRr_t rovr;
    RFRGr_t rfrg;
    RERPKTr_t rerpkt;
    RDBGC0r_t rdbgc0;
} _counter_regs_t;

int
bcm56160_a0_bmd_stat_get(int unit, int port, bmd_stat_t stat, bmd_counter_t *counter)
{
    int ioerr = 0;
    int lport;
    _counter_regs_t ctr;
    cdk_pbmp_t gpbmp, xlpbmp;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);

    CDK_MEMSET(counter, 0, sizeof(*counter));

    bcm56160_a0_gport_pbmp_get(unit, &gpbmp);
    bcm56160_a0_xlport_pbmp_get(unit, &xlpbmp);

    if (CDK_PBMP_MEMBER(xlpbmp, port)) {
#if BMD_CONFIG_INCLUDE_HIGIG == 1 || BMD_CONFIG_INCLUDE_XE == 1
        /* Only support lower 32-bits of counters */
        switch (stat) {
        case bmdStatTxPackets:
            ioerr += READ_TPKTr(unit, port, &ctr.tpkt);
            counter->v[0] += TPKTr_GET(ctr.tpkt, 0);
            break;
        case bmdStatTxBytes:
            ioerr += READ_TBYTr(unit, port, &ctr.tbyt);
            counter->v[0] += TBYTr_GET(ctr.tbyt, 0);
            break;
        case bmdStatTxErrors:
            ioerr += READ_TFRGr(unit, port, &ctr.tfrg);
            counter->v[0] += TFRGr_GET(ctr.tfrg, 0);
            ioerr += READ_TFCSr(unit, port, &ctr.tfcs);
            counter->v[0] += TFCSr_GET(ctr.tfcs, 0);
            ioerr += READ_TOVRr(unit, port, &ctr.tovr);
            counter->v[0] += TOVRr_GET(ctr.tovr, 0);
            ioerr += READ_TUFLr(unit, port, &ctr.tufl);
            counter->v[0] += TUFLr_GET(ctr.tufl, 0);
            ioerr += READ_TERRr(unit, port, &ctr.terr);
            counter->v[0] += TERRr_GET(ctr.terr, 0);
            break;
        case bmdStatRxPackets:
            ioerr += READ_RPKTr(unit, port, &ctr.rpkt);
            counter->v[0] += RPKTr_GET(ctr.rpkt, 0);
            break;
        case bmdStatRxBytes:
            ioerr += READ_RBYTr(unit, port, &ctr.rbyt);
            counter->v[0] += RBYTr_GET(ctr.rbyt, 0);
            break;
        case bmdStatRxErrors:
            ioerr += READ_RFCSr(unit, port, &ctr.rfcs);
            counter->v[0] += RFCSr_GET(ctr.rfcs, 0);
            ioerr += READ_RJBRr(unit, port, &ctr.rjbr);
            counter->v[0] += RJBRr_GET(ctr.rjbr, 0);
            ioerr += READ_ROVRr(unit, port, &ctr.rovr);
            counter->v[0] +=ROVRr_GET(ctr.rovr, 0);
            ioerr += READ_RFRGr(unit, port, &ctr.rfrg);
            counter->v[0] += RFRGr_GET(ctr.rfrg, 0);
            ioerr += READ_RERPKTr(unit, port, &ctr.rerpkt);
            counter->v[0] += RERPKTr_GET(ctr.rerpkt, 0);
            break;
        default:
            break;
        }
#endif
    } else if (CDK_PBMP_MEMBER(gpbmp, port)) {
        switch (stat) {
        case bmdStatTxPackets:
            ioerr += READ_GTPKTr(unit, port, &ctr.gtpkt);
            counter->v[0] += GTPKTr_GET(ctr.gtpkt);
            break;
        case bmdStatTxBytes:
            ioerr += READ_GTBYTr(unit, port, &ctr.gtbyt);
            counter->v[0] += GTBYTr_GET(ctr.gtbyt);
            break;
        case bmdStatTxErrors:
            ioerr += READ_GTJBRr(unit, port, &ctr.gtjbr);
            counter->v[0] += GTJBRr_GET(ctr.gtjbr);
            ioerr += READ_GTFCSr(unit, port, &ctr.gtfcs);
            counter->v[0] += GTFCSr_GET(ctr.gtfcs);
            ioerr += READ_GTOVRr(unit, port, &ctr.gtovr);
            counter->v[0] += GTOVRr_GET(ctr.gtovr);
            break;
        case bmdStatRxPackets:
            ioerr += READ_GRPKTr(unit, port, &ctr.grpkt);
            counter->v[0] += GRPKTr_GET(ctr.grpkt);
            break;
        case bmdStatRxBytes:
            ioerr += READ_GRBYTr(unit, port, &ctr.grbyt);
            counter->v[0] += GRBYTr_GET(ctr.grbyt);
            break;
        case bmdStatRxErrors:
            ioerr += READ_GRJBRr(unit, port, &ctr.grjbr);
            counter->v[0] += GRJBRr_GET(ctr.grjbr);
            ioerr += READ_GRFCSr(unit, port, &ctr.grfcs);
            counter->v[0] += GRFCSr_GET(ctr.grfcs);
            ioerr += READ_GROVRr(unit, port, &ctr.grovr);
            counter->v[0] += GROVRr_GET(ctr.grovr);
            ioerr += READ_GRFLRr(unit, port, &ctr.grflr);
            counter->v[0] += GRFLRr_GET(ctr.grflr);
            ioerr += READ_GRMTUEr(unit, port, &ctr.grmtue);
            counter->v[0] += GRMTUEr_GET(ctr.grmtue);
            ioerr += READ_GRUNDr(unit, port, &ctr.grund);
            counter->v[0] += GRUNDr_GET(ctr.grund);
            ioerr += READ_GRFRGr(unit, port, &ctr.grfrg);
            counter->v[0] += GRFRGr_GET(ctr.grfrg);
            ioerr += READ_GRRPKTr(unit, port, &ctr.grrpkt);
            counter->v[0] += GRRPKTr_GET(ctr.grrpkt);
            break;
        default:
            break;
        }
    }

    lport = P2L(unit, port);

    /* Non-MAC counters */
    switch (stat) {
    case bmdStatRxDrops:
        ioerr += READ_RDBGC0r(unit, lport, &ctr.rdbgc0);
        counter->v[0] += RDBGC0r_GET(ctr.rdbgc0);
        break;
    default:
        break;
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

#endif /* CDK_CONFIG_INCLUDE_BCM56160_A0 */
