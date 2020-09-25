#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56270_A0 == 1

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
#include <cdk/chip/bcm56270_a0_defs.h>

#include "bcm56270_a0_bmd.h"
#include "bcm56270_a0_internal.h"

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
    MXQ_RRPKTr_t mxq_rrpkt;
    MXQ_TPKTr_t mxq_tpkt;
    MXQ_TBYTr_t mxq_tbyt;
    MXQ_TFCSr_t mxq_tfcs;
    MXQ_TFRGr_t mxq_tfrg;
    MXQ_TOVRr_t mxq_tovr;
    MXQ_TUFLr_t mxq_tufl;
    MXQ_TERRr_t mxq_terr;
    MXQ_RPKTr_t mxq_rpkt;
    MXQ_RBYTr_t mxq_rbyt;
    MXQ_RFCSr_t mxq_rfcs;
    MXQ_RJBRr_t mxq_rjbr;
    MXQ_ROVRr_t mxq_rovr;
    MXQ_RFRGr_t mxq_rfrg;
    MXQ_RERPKTr_t mxq_rerpkt;
} bcm56270_a0_counter_t;

int
bcm56270_a0_bmd_stat_clear(int unit, int port, bmd_stat_t stat)
{
    int ioerr = 0;
    int lport;
    cdk_pbmp_t mxqport_pbmp, xlport_pbmp;
    bcm56270_a0_counter_t ctr;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);

    bcm56270_a0_mxqport_pbmp_get(unit, &mxqport_pbmp);
    bcm56270_a0_xlport_pbmp_get(unit, &xlport_pbmp);
    
    CDK_MEMSET(&ctr, 0, sizeof(ctr));

    if (CDK_PBMP_MEMBER(xlport_pbmp, port)) {
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
    } else if (CDK_PBMP_MEMBER(mxqport_pbmp, port)) {
        switch (stat) {
        case bmdStatTxPackets:
            ioerr += WRITE_MXQ_TPKTr(unit, port, ctr.mxq_tpkt);
            break;
        case bmdStatTxBytes:
            ioerr += WRITE_MXQ_TBYTr(unit, port, ctr.mxq_tbyt);
            break;
        case bmdStatTxErrors:
            ioerr += WRITE_MXQ_TFRGr(unit, port, ctr.mxq_tfrg);
            ioerr += WRITE_MXQ_TFCSr(unit, port, ctr.mxq_tfcs);
            ioerr += WRITE_MXQ_TOVRr(unit, port, ctr.mxq_tovr);
            ioerr += WRITE_MXQ_TUFLr(unit, port, ctr.mxq_tufl);
            ioerr += WRITE_MXQ_TERRr(unit, port, ctr.mxq_terr);
            break;
        case bmdStatRxPackets:
            ioerr += WRITE_MXQ_RPKTr(unit, port, ctr.mxq_rpkt);
            break;
        case bmdStatRxBytes:
            ioerr += WRITE_MXQ_RBYTr(unit, port, ctr.mxq_rbyt);
            break;
        case bmdStatRxErrors:
            ioerr += WRITE_MXQ_RFCSr(unit, port, ctr.mxq_rfcs);
            ioerr += WRITE_MXQ_RJBRr(unit, port, ctr.mxq_rjbr);
            ioerr += WRITE_MXQ_ROVRr(unit, port, ctr.mxq_rovr);
            ioerr += WRITE_MXQ_RFRGr(unit, port, ctr.mxq_rfrg);
            ioerr += WRITE_MXQ_RERPKTr(unit, port, ctr.mxq_rerpkt);
            ioerr += WRITE_MXQ_RRPKTr(unit, port, ctr.mxq_rrpkt);
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

#endif /* CDK_CONFIG_INCLUDE_BCM56270_A0 */
