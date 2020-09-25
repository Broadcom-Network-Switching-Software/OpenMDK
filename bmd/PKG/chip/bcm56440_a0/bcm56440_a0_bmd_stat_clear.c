#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56440_A0 == 1

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
#include <cdk/chip/bcm56440_a0_defs.h>

#include "bcm56440_a0_bmd.h"
#include "bcm56440_a0_internal.h"

typedef union {
    RRPKTr_t rrpkt;
    RDBGC0r_t rdbgc0;
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
} bcm56440_a0_counter_t;

int
bcm56440_a0_bmd_stat_clear(int unit, int port, bmd_stat_t stat)
{
    int ioerr = 0;
    int lport;
    bcm56440_a0_counter_t ctr;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);

    CDK_MEMSET(&ctr, 0, sizeof(ctr));

    if (port != CMIC_PORT) {
        if (BMD_PORT_PROPERTIES(unit, port) & (BMD_PORT_HG | BMD_PORT_XE)) {
            switch (stat) {
            case bmdStatTxPackets:
                ioerr += WRITE_TPKTr(unit, port, ctr.tpkt);
                break;
            case bmdStatTxBytes:
                ioerr += WRITE_TBYTr(unit, port, ctr.tbyt);
                break;
            case bmdStatTxErrors:
                ioerr += WRITE_TFRGr(unit, port, ctr.tfrg);
                ioerr += WRITE_TFCSr(unit, port, ctr.tfcs);
                ioerr += WRITE_TOVRr(unit, port, ctr.tovr);
                ioerr += WRITE_TUFLr(unit, port, ctr.tufl);
                ioerr += WRITE_TERRr(unit, port, ctr.terr);
                break;
            case bmdStatRxPackets:
                ioerr += WRITE_RPKTr(unit, port, ctr.rpkt);
                break;
            case bmdStatRxBytes:
                ioerr += WRITE_RBYTr(unit, port, ctr.rbyt);
                break;
            case bmdStatRxErrors:
                ioerr += WRITE_RFCSr(unit, port, ctr.rfcs);
                ioerr += WRITE_RJBRr(unit, port, ctr.rjbr);
                ioerr += WRITE_ROVRr(unit, port, ctr.rovr);
                ioerr += WRITE_RFRGr(unit, port, ctr.rfrg);
                ioerr += WRITE_RERPKTr(unit, port, ctr.rerpkt);
                ioerr += WRITE_RRPKTr(unit, port, ctr.rrpkt);
                break;
            default:
                break;
            } 
       } else if (BMD_PORT_PROPERTIES(unit, port) & BMD_PORT_GE) {
            switch (stat) {
            case bmdStatTxPackets:
                ioerr += WRITE_GTPKTr(unit, port, ctr.gtpkt);
                break;
            case bmdStatTxBytes:
                ioerr += WRITE_GTBYTr(unit, port, ctr.gtbyt);
                break;
            case bmdStatTxErrors:
                ioerr += WRITE_GTFCSr(unit, port, ctr.gtfcs);
                ioerr += WRITE_GTOVRr(unit, port, ctr.gtovr);
                ioerr += WRITE_GTJBRr(unit, port, ctr.gtjbr);
                break;
            case bmdStatRxPackets:
                ioerr += WRITE_GRPKTr(unit, port, ctr.grpkt);
                break;
            case bmdStatRxBytes:
                ioerr += WRITE_GRBYTr(unit, port, ctr.grbyt);
                break;
            case bmdStatRxErrors:
                ioerr += WRITE_GRFCSr(unit, port, ctr.grfcs);
                ioerr += WRITE_GRJBRr(unit, port, ctr.grjbr);
                ioerr += WRITE_GROVRr(unit, port, ctr.grovr);
                ioerr += WRITE_GRFRGr(unit, port, ctr.grfrg);
                ioerr += WRITE_GRRPKTr(unit, port, ctr.grrpkt);
                ioerr += WRITE_GRFLRr(unit, port, ctr.grflr);
                ioerr += WRITE_GRMTUEr(unit, port, ctr.grmtue);
                ioerr += WRITE_GRUNDr(unit, port, ctr.grund);

                break;
            default:
                break;
            }
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

#endif /* CDK_CONFIG_INCLUDE_BCM56440_A0 */
