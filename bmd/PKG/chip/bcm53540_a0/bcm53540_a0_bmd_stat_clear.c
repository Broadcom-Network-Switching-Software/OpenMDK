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
    RDBGC0r_t rdbgc0;
} _counter_regs_t;

int
bcm53540_a0_bmd_stat_clear(int unit, int port, bmd_stat_t stat)
{
    int ioerr = 0;
    int lport;
    _counter_regs_t ctr;
    cdk_pbmp_t gpbmp;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);

    CDK_MEMSET(&ctr, 0, sizeof(ctr));

    bcm53540_a0_gport_pbmp_get(unit, &gpbmp);

    if (CDK_PBMP_MEMBER(gpbmp, port)) {
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
            break;
        default:
            break;
        }
    }

    lport = P2L(unit, port);

    /* Non-MAC counters */
    switch (stat) {
    case bmdStatRxDrops:
        ioerr += WRITE_RDBGC0r(unit, lport, ctr.rdbgc0);
        break;
    default:
        break;
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

#endif /* CDK_CONFIG_INCLUDE_BCM53540_A0 */
