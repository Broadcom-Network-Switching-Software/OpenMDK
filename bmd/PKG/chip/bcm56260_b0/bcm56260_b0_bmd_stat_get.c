/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56260_B0 == 1

#include <bmd/bmd.h>

#include <cdk/cdk_device.h>
#include <cdk/cdk_error.h>
#include <cdk/chip/bcm56260_b0_defs.h>
#include <cdk/cdk_debug.h>

#include "bcm56260_b0_bmd.h"
#include "bcm56260_b0_internal.h"

int
bcm56260_b0_bmd_stat_get(int unit, int port, bmd_stat_t stat, bmd_counter_t *counter)
{
    int ioerr = 0;
    int lport;
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

    cdk_pbmp_t mxqport_pbmp, xlport_pbmp;
    
    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);

    bcm56260_a0_mxqport_pbmp_get(unit, &mxqport_pbmp);
    bcm56260_a0_xlport_pbmp_get(unit, &xlport_pbmp);
    
    CDK_MEMSET(counter, 0, sizeof(*counter));

    if (CDK_PBMP_MEMBER(xlport_pbmp, port)) {
        switch (stat) {
        case bmdStatTxPackets:
            ioerr += READ_TPKTr(unit, port, &tpkt);
            counter->v[0] += TPKTr_GET(tpkt, 0);
            break;
        case bmdStatTxBytes:
            ioerr += READ_TBYTr(unit, port, &tbyt);
            counter->v[0] += TBYTr_GET(tbyt, 0);
            break;
        case bmdStatTxErrors:
            ioerr += READ_TFRGr(unit, port, &tfrg);
            counter->v[0] += TFRGr_GET(tfrg, 0);
            ioerr += READ_TFCSr(unit, port, &tfcs);
            counter->v[0] += TFCSr_GET(tfcs, 0);
            ioerr += READ_TOVRr(unit, port, &tovr);
            counter->v[0] += TOVRr_GET(tovr, 0);
            ioerr += READ_TUFLr(unit, port, &tufl);
            counter->v[0] += TUFLr_GET(tufl, 0);
            ioerr += READ_TERRr(unit, port, &terr);
            counter->v[0] += TERRr_GET(terr, 0);
            break;
        case bmdStatRxPackets:
            ioerr += READ_RPKTr(unit, port, &rpkt);
            counter->v[0] += RPKTr_GET(rpkt, 0);
            break;
        case bmdStatRxBytes:
            ioerr += READ_RBYTr(unit, port, &rbyt);
            counter->v[0] += RBYTr_GET(rbyt, 0);
            break;
        case bmdStatRxErrors:
            ioerr += READ_RFCSr(unit, port, &rfcs);
            counter->v[0] += RFCSr_GET(rfcs, 0);
            ioerr += READ_RJBRr(unit, port, &rjbr);
            counter->v[0] += RJBRr_GET(rjbr, 0);
            ioerr += READ_ROVRr(unit, port, &rovr);
            counter->v[0] += ROVRr_GET(rovr, 0);
            ioerr += READ_RFRGr(unit, port, &rfrg);
            counter->v[0] += RFRGr_GET(rfrg, 0);
            ioerr += READ_RERPKTr(unit, port, &rerpkt);
            counter->v[0] += RERPKTr_GET(rerpkt, 0);
            ioerr += READ_RRPKTr(unit, port, &rrpkt);
            counter->v[0] += RRPKTr_GET(rrpkt, 0);
            break;
        default:
            break;
        }
    } else if (CDK_PBMP_MEMBER(mxqport_pbmp, port)) {
        switch (stat) {
        case bmdStatTxPackets:
            ioerr += READ_MXQ_TPKTr(unit, port, &mxq_tpkt);
            counter->v[0] += MXQ_TPKTr_GET(mxq_tpkt, 0);
            break;
        case bmdStatTxBytes:
            ioerr += READ_MXQ_TBYTr(unit, port, &mxq_tbyt);
            counter->v[0] += MXQ_TBYTr_GET(mxq_tbyt, 0);
            break;
        case bmdStatTxErrors:
            ioerr += READ_MXQ_TFRGr(unit, port, &mxq_tfrg);
            counter->v[0] += MXQ_TFRGr_GET(mxq_tfrg, 0);
            ioerr += READ_MXQ_TFCSr(unit, port, &mxq_tfcs);
            counter->v[0] += MXQ_TFCSr_GET(mxq_tfcs, 0);
            ioerr += READ_MXQ_TOVRr(unit, port, &mxq_tovr);
            counter->v[0] += MXQ_TOVRr_GET(mxq_tovr, 0);
            ioerr += READ_MXQ_TUFLr(unit, port, &mxq_tufl);
            counter->v[0] += MXQ_TUFLr_GET(mxq_tufl, 0);
            ioerr += READ_MXQ_TERRr(unit, port, &mxq_terr);
            counter->v[0] += MXQ_TERRr_GET(mxq_terr, 0);
            break;
        case bmdStatRxPackets:
            ioerr += READ_MXQ_RPKTr(unit, port, &mxq_rpkt);
            counter->v[0] += MXQ_RPKTr_GET(mxq_rpkt, 0);
            break;
        case bmdStatRxBytes:
            ioerr += READ_MXQ_RBYTr(unit, port, &mxq_rbyt);
            counter->v[0] += MXQ_RBYTr_GET(mxq_rbyt, 0);
            break;
        case bmdStatRxErrors:
            ioerr += READ_MXQ_RFCSr(unit, port, &mxq_rfcs);
            counter->v[0] += MXQ_RFCSr_GET(mxq_rfcs, 0);
            ioerr += READ_MXQ_RJBRr(unit, port, &mxq_rjbr);
            counter->v[0] += MXQ_RJBRr_GET(mxq_rjbr, 0);
            ioerr += READ_MXQ_ROVRr(unit, port, &mxq_rovr);
            counter->v[0] += MXQ_ROVRr_GET(mxq_rovr, 0);
            ioerr += READ_MXQ_RFRGr(unit, port, &mxq_rfrg);
            counter->v[0] += MXQ_RFRGr_GET(mxq_rfrg, 0);
            ioerr += READ_MXQ_RERPKTr(unit, port, &mxq_rerpkt);
            counter->v[0] += MXQ_RERPKTr_GET(mxq_rerpkt, 0);
            ioerr += READ_MXQ_RRPKTr(unit, port, &mxq_rrpkt);
            counter->v[0] += MXQ_RRPKTr_GET(mxq_rrpkt, 0);
            break;
        default:
            break;
        }
    
    }
    
    lport = P2L(unit, port);

    /* Non-MAC counters */
    switch (stat) {
    case bmdStatRxDrops:
        ioerr += READ_RDBGC0r(unit, lport, &rdbgc0);
        counter->v[0] += RDBGC0r_COUNTf_GET(rdbgc0);
        break;
    default:
        break;
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

#endif /* CDK_CONFIG_INCLUDE_BCM56260_B0 */
