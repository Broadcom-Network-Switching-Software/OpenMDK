/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53570_A0 == 1

#include <bmd/bmd.h>
#include <cdk/cdk_device.h>
#include <cdk/cdk_debug.h>
#include <cdk/cdk_error.h>
#include <cdk/chip/bcm53570_a0_defs.h>
#include "bcm53570_a0_bmd.h"
#include "bcm53570_a0_internal.h"

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
    CLMIB_RRPKTr_t cl_rrpkt;
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
    XLMIB_RRPKTr_t xl_rrpkt;
} bcm53570_a0_counter_t;

int 
bcm53570_a0_bmd_stat_clear(int unit, int port, bmd_stat_t stat)
{
    int ioerr = 0;
    int lport;
    bcm53570_a0_counter_t ctr;
    cdk_pbmp_t gport_pbmp, clport_pbmp;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);
    
    CDK_MEMSET(&ctr, 0, sizeof(ctr));
    
    bcm53570_a0_clport_pbmp_get(unit, &clport_pbmp);
    bcm53570_a0_gport_pbmp_get(unit, &gport_pbmp);

    if (CDK_PBMP_MEMBER(gport_pbmp, port)) {
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
            ioerr += WRITE_GRRPKTr(unit, port, ctr.grrpkt); 
            break;
        default:
            break;
        }
    } else if (CDK_PBMP_MEMBER(clport_pbmp, port)) {
        switch (stat) {
        case bmdStatTxPackets:
            ioerr += WRITE_CLMIB_TPKTr(unit, port, ctr.cl_tpkt);
            break;                                 
        case bmdStatTxBytes:                       
            ioerr += WRITE_CLMIB_TBYTr(unit, port, ctr.cl_tbyt);
            break;                                 
        case bmdStatTxErrors:                      
            ioerr += WRITE_CLMIB_TFRGr(unit, port, ctr.cl_tfrg);
            ioerr += WRITE_CLMIB_TFCSr(unit, port, ctr.cl_tfcs);
            ioerr += WRITE_CLMIB_TOVRr(unit, port, ctr.cl_tovr);
            ioerr += WRITE_CLMIB_TUFLr(unit, port, ctr.cl_tufl);
            ioerr += WRITE_CLMIB_TERRr(unit, port, ctr.cl_terr);
            break;
        case bmdStatRxPackets:
            ioerr += WRITE_CLMIB_RPKTr(unit, port, ctr.cl_rpkt);
            break;
        case bmdStatRxBytes:
            ioerr += WRITE_CLMIB_RBYTr(unit, port, ctr.cl_rbyt);
            break;
        case bmdStatRxErrors:
            ioerr += WRITE_CLMIB_RFCSr(unit, port, ctr.cl_rfcs);
            ioerr += WRITE_CLMIB_RJBRr(unit, port, ctr.cl_rjbr);
            ioerr += WRITE_CLMIB_ROVRr(unit, port, ctr.cl_rovr);
            ioerr += WRITE_CLMIB_RFRGr(unit, port, ctr.cl_rfrg);
            ioerr += WRITE_CLMIB_RERPKTr(unit, port, ctr.cl_rerpkt);
            ioerr += WRITE_CLMIB_RRPKTr(unit, port, ctr.cl_rrpkt); 
            break;
        default:
            break;
        }
    } else {
        switch (stat) {
        case bmdStatTxPackets:
            ioerr += WRITE_XLMIB_TPKTr(unit, port, ctr.xl_tpkt);
            break;                                 
        case bmdStatTxBytes:                       
            ioerr += WRITE_XLMIB_TBYTr(unit, port, ctr.xl_tbyt);
            break;                                 
        case bmdStatTxErrors:                      
            ioerr += WRITE_XLMIB_TFRGr(unit, port, ctr.xl_tfrg);
            ioerr += WRITE_XLMIB_TFCSr(unit, port, ctr.xl_tfcs);
            ioerr += WRITE_XLMIB_TOVRr(unit, port, ctr.xl_tovr);
            ioerr += WRITE_XLMIB_TUFLr(unit, port, ctr.xl_tufl);
            ioerr += WRITE_XLMIB_TERRr(unit, port, ctr.xl_terr);
            break;
        case bmdStatRxPackets:
            ioerr += WRITE_XLMIB_RPKTr(unit, port, ctr.xl_rpkt);
            break;
        case bmdStatRxBytes:
            ioerr += WRITE_XLMIB_RBYTr(unit, port, ctr.xl_rbyt);
            break;
        case bmdStatRxErrors:
            ioerr += WRITE_XLMIB_RFCSr(unit, port, ctr.xl_rfcs);
            ioerr += WRITE_XLMIB_RJBRr(unit, port, ctr.xl_rjbr);
            ioerr += WRITE_XLMIB_ROVRr(unit, port, ctr.xl_rovr);
            ioerr += WRITE_XLMIB_RFRGr(unit, port, ctr.xl_rfrg);
            ioerr += WRITE_XLMIB_RERPKTr(unit, port, ctr.xl_rerpkt);
            ioerr += WRITE_XLMIB_RRPKTr(unit, port, ctr.xl_rrpkt); 
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

#endif /* CDK_CONFIG_INCLUDE_BCM53570_A0 */
