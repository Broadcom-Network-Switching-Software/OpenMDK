/*
 *
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53540_A0 == 1

#include <bmd/bmd.h>
#include <cdk/cdk_device.h>
#include <cdk/cdk_debug.h>
#include <cdk/cdk_error.h>
#include <cdk/chip/bcm53540_a0_defs.h>
#include "bcm53540_a0_bmd.h"
#include "bcm53540_a0_internal.h"

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
    RDBGC0r_t rdbgc0;
} _counter_regs_t;

int
bcm53540_a0_bmd_stat_get(int unit, int port, bmd_stat_t stat, bmd_counter_t *counter)
{
    int ioerr = 0;
    int lport;
    _counter_regs_t ctr;
    cdk_pbmp_t gpbmp;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);

    CDK_MEMSET(counter, 0, sizeof(*counter));

    bcm53540_a0_gport_pbmp_get(unit, &gpbmp);

    if (CDK_PBMP_MEMBER(gpbmp, port)) {
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

#endif /* CDK_CONFIG_INCLUDE_BCM53540_A0 */
