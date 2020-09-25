#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53570_B0 == 1

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
#include <cdk/chip/bcm53570_b0_defs.h>
#include <cdk/cdk_debug.h>

#include "bcm53570_b0_bmd.h"
#include "bcm53570_b0_internal.h"

int
bcm53570_b0_bmd_stat_get(int unit, int port, bmd_stat_t stat, bmd_counter_t *counter)
{
    int ioerr = 0;
    int lport;
    GRRPKTr_t grrpkt;
    RDBGC0r_t rdbgc0;
    GTPKTr_t gtpkt;
    GTBYTr_t gtbyt;
    GTFCSr_t gtfcs;
    GTFRGr_t gtfrg;
    GTOVRr_t gtovr;
    GRPKTr_t grpkt;
    GRBYTr_t grbyt;
    GRFCSr_t grfcs;
    GRJBRr_t grjbr;
    GROVRr_t grovr;
    GRFRGr_t grfrg;
    CLMIB_RRPKTr_t cl_rrpkt;
    CLMIB_TPKTr_t cl_tpkt;
    CLMIB_TBYTr_t cl_tbyt;
    CLMIB_TFCSr_t cl_tfcs;
    CLMIB_TFRGr_t cl_tfrg;
    CLMIB_TOVRr_t cl_tovr;
    CLMIB_TUFLr_t cl_tufl;
    CLMIB_TERRr_t cl_terr;
    CLMIB_RPKTr_t cl_rpkt;
    CLMIB_RBYTr_t cl_rbyt;
    CLMIB_RFCSr_t cl_rfcs;
    CLMIB_RJBRr_t cl_rjbr;
    CLMIB_ROVRr_t cl_rovr;
    CLMIB_RFRGr_t cl_rfrg;
    CLMIB_RERPKTr_t cl_rerpkt;
    XLMIB_RRPKTr_t xl_rrpkt;
    XLMIB_TPKTr_t xl_tpkt;
    XLMIB_TBYTr_t xl_tbyt;
    XLMIB_TFCSr_t xl_tfcs;
    XLMIB_TFRGr_t xl_tfrg;
    XLMIB_TOVRr_t xl_tovr;
    XLMIB_TUFLr_t xl_tufl;
    XLMIB_TERRr_t xl_terr;
    XLMIB_RPKTr_t xl_rpkt;
    XLMIB_RBYTr_t xl_rbyt;
    XLMIB_RFCSr_t xl_rfcs;
    XLMIB_RJBRr_t xl_rjbr;
    XLMIB_ROVRr_t xl_rovr;
    XLMIB_RFRGr_t xl_rfrg;
    XLMIB_RERPKTr_t xl_rerpkt;

    cdk_pbmp_t gport_pbmp, clport_pbmp;
    
    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);

    bcm53570_b0_clport_pbmp_get(unit, &clport_pbmp);
    bcm53570_b0_gport_pbmp_get(unit, &gport_pbmp);
    
    CDK_MEMSET(counter, 0, sizeof(*counter));

    if (CDK_PBMP_MEMBER(gport_pbmp, port)) {
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
            ioerr += READ_GTFRGr(unit, port, &gtfrg);
            counter->v[0] += GTFRGr_GET(gtfrg);
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
            ioerr += READ_GRFCSr(unit, port, &grfcs);
            counter->v[0] += GRFCSr_GET(grfcs);
            ioerr += READ_GRJBRr(unit, port, &grjbr);
            counter->v[0] += GRJBRr_GET(grjbr);
            ioerr += READ_GROVRr(unit, port, &grovr);
            counter->v[0] += GROVRr_GET(grovr);
            ioerr += READ_GRFRGr(unit, port, &grfrg);
            counter->v[0] += GRFRGr_GET(grfrg);
            ioerr += READ_GRRPKTr(unit, port, &grrpkt);
            counter->v[0] += GRRPKTr_GET(grrpkt);
            break;
        default:
            break;
        }
    } else if (CDK_PBMP_MEMBER(clport_pbmp, port)) {
        switch (stat) {
        case bmdStatTxPackets:
            ioerr += READ_CLMIB_TPKTr(unit, port, &cl_tpkt);
            counter->v[0] += CLMIB_TPKTr_GET(cl_tpkt, 0);
            break;
        case bmdStatTxBytes:
            ioerr += READ_CLMIB_TBYTr(unit, port, &cl_tbyt);
            counter->v[0] += CLMIB_TBYTr_GET(cl_tbyt, 0);
            break;
        case bmdStatTxErrors:
            ioerr += READ_CLMIB_TFRGr(unit, port, &cl_tfrg);
            counter->v[0] += CLMIB_TFRGr_GET(cl_tfrg, 0);
            ioerr += READ_CLMIB_TFCSr(unit, port, &cl_tfcs);
            counter->v[0] += CLMIB_TFCSr_GET(cl_tfcs, 0);
            ioerr += READ_CLMIB_TOVRr(unit, port, &cl_tovr);
            counter->v[0] += CLMIB_TOVRr_GET(cl_tovr, 0);
            ioerr += READ_CLMIB_TUFLr(unit, port, &cl_tufl);
            counter->v[0] += CLMIB_TUFLr_GET(cl_tufl, 0);
            ioerr += READ_CLMIB_TERRr(unit, port, &cl_terr);
            counter->v[0] += CLMIB_TERRr_GET(cl_terr, 0);
            break;
        case bmdStatRxPackets:
            ioerr += READ_CLMIB_RPKTr(unit, port, &cl_rpkt);
            counter->v[0] += CLMIB_RPKTr_GET(cl_rpkt, 0);
            break;
        case bmdStatRxBytes:
            ioerr += READ_CLMIB_RBYTr(unit, port, &cl_rbyt);
            counter->v[0] += CLMIB_RBYTr_GET(cl_rbyt, 0);
            break;
        case bmdStatRxErrors:
            ioerr += READ_CLMIB_RFCSr(unit, port, &cl_rfcs);
            counter->v[0] += CLMIB_RFCSr_GET(cl_rfcs, 0);
            ioerr += READ_CLMIB_RJBRr(unit, port, &cl_rjbr);
            counter->v[0] += CLMIB_RJBRr_GET(cl_rjbr, 0);
            ioerr += READ_CLMIB_ROVRr(unit, port, &cl_rovr);
            counter->v[0] += CLMIB_ROVRr_GET(cl_rovr, 0);
            ioerr += READ_CLMIB_RFRGr(unit, port, &cl_rfrg);
            counter->v[0] += CLMIB_RFRGr_GET(cl_rfrg, 0);
            ioerr += READ_CLMIB_RERPKTr(unit, port, &cl_rerpkt);
            counter->v[0] += CLMIB_RERPKTr_GET(cl_rerpkt, 0);
            ioerr += READ_CLMIB_RRPKTr(unit, port, &cl_rrpkt);
            counter->v[0] += CLMIB_RRPKTr_GET(cl_rrpkt, 0);
            break;
        default:
            break;
        }
    } else {
        switch (stat) {
        case bmdStatTxPackets:
            ioerr += READ_XLMIB_TPKTr(unit, port, &xl_tpkt);
            counter->v[0] += XLMIB_TPKTr_GET(xl_tpkt, 0);
            break;
        case bmdStatTxBytes:
            ioerr += READ_XLMIB_TBYTr(unit, port, &xl_tbyt);
            counter->v[0] += XLMIB_TBYTr_GET(xl_tbyt, 0);
            break;
        case bmdStatTxErrors:
            ioerr += READ_XLMIB_TFRGr(unit, port, &xl_tfrg);
            counter->v[0] += XLMIB_TFRGr_GET(xl_tfrg, 0);
            ioerr += READ_XLMIB_TFCSr(unit, port, &xl_tfcs);
            counter->v[0] += XLMIB_TFCSr_GET(xl_tfcs, 0);
            ioerr += READ_XLMIB_TOVRr(unit, port, &xl_tovr);
            counter->v[0] += XLMIB_TOVRr_GET(xl_tovr, 0);
            ioerr += READ_XLMIB_TUFLr(unit, port, &xl_tufl);
            counter->v[0] += XLMIB_TUFLr_GET(xl_tufl, 0);
            ioerr += READ_XLMIB_TERRr(unit, port, &xl_terr);
            counter->v[0] += XLMIB_TERRr_GET(xl_terr, 0);
            break;
        case bmdStatRxPackets:
            ioerr += READ_XLMIB_RPKTr(unit, port, &xl_rpkt);
            counter->v[0] += XLMIB_RPKTr_GET(xl_rpkt, 0);
            break;
        case bmdStatRxBytes:
            ioerr += READ_XLMIB_RBYTr(unit, port, &xl_rbyt);
            counter->v[0] += XLMIB_RBYTr_GET(xl_rbyt, 0);
            break;
        case bmdStatRxErrors:
            ioerr += READ_XLMIB_RFCSr(unit, port, &xl_rfcs);
            counter->v[0] += XLMIB_RFCSr_GET(xl_rfcs, 0);
            ioerr += READ_XLMIB_RJBRr(unit, port, &xl_rjbr);
            counter->v[0] += XLMIB_RJBRr_GET(xl_rjbr, 0);
            ioerr += READ_XLMIB_ROVRr(unit, port, &xl_rovr);
            counter->v[0] += XLMIB_ROVRr_GET(xl_rovr, 0);
            ioerr += READ_XLMIB_RFRGr(unit, port, &xl_rfrg);
            counter->v[0] += XLMIB_RFRGr_GET(xl_rfrg, 0);
            ioerr += READ_XLMIB_RERPKTr(unit, port, &xl_rerpkt);
            counter->v[0] += XLMIB_RERPKTr_GET(xl_rerpkt, 0);
            ioerr += READ_XLMIB_RRPKTr(unit, port, &xl_rrpkt);
            counter->v[0] += XLMIB_RRPKTr_GET(xl_rrpkt, 0);
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
        counter->v[0] += RDBGC0r_GET(rdbgc0);
        break;
    default:
        break;
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

#endif /* CDK_CONFIG_INCLUDE_BCM53570_B0 */
