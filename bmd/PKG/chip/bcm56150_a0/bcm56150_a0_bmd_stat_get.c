/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56150_A0 == 1

#include <bmd/bmd.h>
#include <cdk/cdk_device.h>
#include <cdk/cdk_debug.h>
#include <cdk/cdk_error.h>
#include <cdk/chip/bcm56150_a0_defs.h>
#include "bcm56150_a0_bmd.h"
#include "bcm56150_a0_internal.h"

int 
bcm56150_a0_bmd_stat_get(int unit, int port, bmd_stat_t stat, bmd_counter_t *counter)
{
    int ioerr = 0;
    int lport;
    cdk_pbmp_t pbmp1,pbmp2;
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

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);

    CDK_XGSM_BLKTYPE_PBMP_GET(unit, BLKTYPE_GPORT, &pbmp1);
    bcm56150_a0_xlport_pbmp_get(unit, &pbmp2);
    CDK_MEMSET(counter, 0, sizeof(*counter));

    if (port != CMIC_PORT) {
        if (CDK_PBMP_MEMBER(pbmp2, port)) {
#if BMD_CONFIG_INCLUDE_HIGIG == 1 || BMD_CONFIG_INCLUDE_XE == 1
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
        

            /* Only support lower 32-bits of counters */
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
                counter->v[0] +=ROVRr_GET(rovr, 0);
                ioerr += READ_RFRGr(unit, port, &rfrg);
                counter->v[0] += RFRGr_GET(rfrg, 0);
                ioerr += READ_RERPKTr(unit, port, &rerpkt);
                counter->v[0] += RERPKTr_GET(rerpkt, 0);
                break;
            default:
                break;
            }
#endif
        } else if (CDK_PBMP_MEMBER(pbmp1, port)) {
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
                ioerr += READ_GRRPKTr(unit, port, &grrpkt);
                counter->v[0] += GRRPKTr_GET(grrpkt);
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
        ioerr += READ_RDBGC0r(unit, lport, &rdbgc0);
        counter->v[0] += RDBGC0r_GET(rdbgc0);
        break;
    default:
        break;
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

#endif /* CDK_CONFIG_INCLUDE_BCM56150_A0 */
