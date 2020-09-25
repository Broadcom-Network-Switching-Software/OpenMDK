/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56560_A0 == 1

#include <bmd/bmd.h>

#include <cdk/cdk_device.h>
#include <cdk/cdk_error.h>
#include <cdk/cdk_debug.h>
#include <cdk/chip/bcm56560_a0_defs.h>

#include "bcm56560_a0_bmd.h"
#include "bcm56560_a0_internal.h"

int
bcm56560_a0_bmd_stat_get(int unit, int port, bmd_stat_t stat, 
                         bmd_counter_t *counter)
{
    int ioerr = 0;
    int lport;
    cdk_pbmp_t cxxpbmp;
    RRPKTr_t rrpkt;
    RDBGC0r_t rdbgc0;
    TPKTr_t tpkt;
    TBYTr_t tbyt;
    TJBRr_t tjbr;
    TFCSr_t tfcs;
    TFRGr_t tfrg;
    TOVRr_t tovr;
    TUFLr_t tufl;
    TERRr_t terr;
    RPKTr_t rpkt;
    RBYTr_t rbyt;
    RFCSr_t rfcs;
    RFLRr_t rflr;
    RMTUEr_t rmtue;
    RUNDr_t rund;
    RJBRr_t rjbr;
    ROVRr_t rovr;
    RFRGr_t rfrg;
    uint32_t blk_acc;
    
    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);

    CDK_MEMSET(counter, 0, sizeof(*counter));
    
    bcm56560_a0_cxxport_pbmp_get(unit, &cxxpbmp);
    /* Only support lower 32-bits of counters */
    if (port != CMIC_PORT) {
        if (CDK_PBMP_MEMBER(cxxpbmp, port) && (SPEED_MAX(unit, port) < 100000)) {
            blk_acc = 1 << BCM56560_A0_BLKTYPE_XLPORT;
            /* cxx port uses XLMAC */
            switch (stat) {
            case bmdStatTxPackets:
                ioerr += cdk_xgsd_reg64_port_read(unit, blk_acc, port, 
                    BCM56560_A0_TPKTr, 0, (&tpkt._tpkt));
                counter->v[0] += TPKTr_GET(tpkt, 0);
                break;
            case bmdStatTxBytes:
                ioerr += cdk_xgsd_reg64_port_read(unit, blk_acc, port, 
                    BCM56560_A0_TBYTr, 0, (&tbyt._tbyt));
                counter->v[0] += TBYTr_GET(tbyt, 0);
                break;
            case bmdStatTxErrors:
                ioerr += cdk_xgsd_reg64_port_read(unit, blk_acc, port, 
                    BCM56560_A0_TJBRr, 0, (&tjbr._tjbr));
                counter->v[0] += TJBRr_GET(tjbr, 0);
                ioerr += cdk_xgsd_reg64_port_read(unit, blk_acc, port, 
                    BCM56560_A0_TFCSr, 0, (&tfcs._tfcs));
                counter->v[0] += TFCSr_GET(tfcs, 0);
                ioerr += cdk_xgsd_reg64_port_read(unit, blk_acc, port, 
                    BCM56560_A0_TFRGr, 0, (&tfrg._tfrg));
                counter->v[0] += TFRGr_GET(tfrg, 0);
                ioerr += cdk_xgsd_reg64_port_read(unit, blk_acc, port, 
                    BCM56560_A0_TOVRr, 0, (&tovr._tovr));
                counter->v[0] += TOVRr_GET(tovr, 0);
                ioerr += cdk_xgsd_reg64_port_read(unit, blk_acc, port, 
                    BCM56560_A0_TUFLr, 0, (&tufl._tufl));
                counter->v[0] += TUFLr_GET(tufl, 0);
                ioerr += cdk_xgsd_reg64_port_read(unit, blk_acc, port, 
                    BCM56560_A0_TERRr, 0, (&terr._terr));
                counter->v[0] += TERRr_GET(terr, 0);
                break;
            case bmdStatRxPackets:
                ioerr += cdk_xgsd_reg64_port_read(unit, blk_acc, port, 
                    BCM56560_A0_RPKTr, 0, (&rpkt._rpkt));
                counter->v[0] += RPKTr_GET(rpkt, 0);
                break;
            case bmdStatRxBytes:
                ioerr += cdk_xgsd_reg64_port_read(unit, blk_acc, port, 
                    BCM56560_A0_RBYTr, 0, (&rbyt._rbyt));
                counter->v[0] += RBYTr_GET(rbyt, 0);
                break;
            case bmdStatRxErrors:
                ioerr += cdk_xgsd_reg64_port_read(unit, blk_acc, port, 
                    BCM56560_A0_RFCSr, 0, (&rfcs._rfcs));
                counter->v[0] += RFCSr_GET(rfcs, 0);
                ioerr += cdk_xgsd_reg64_port_read(unit, blk_acc, port, 
                    BCM56560_A0_RFLRr, 0, (&rflr._rflr));
                counter->v[0] += RFLRr_GET(rflr, 0);
                ioerr += cdk_xgsd_reg64_port_read(unit, blk_acc, port, 
                    BCM56560_A0_RMTUEr, 0, (&rmtue._rmtue));
                counter->v[0] += RMTUEr_GET(rmtue, 0);
                ioerr += cdk_xgsd_reg64_port_read(unit, blk_acc, port, 
                    BCM56560_A0_RUNDr, 0, (&rund._rund));
                counter->v[0] += RUNDr_GET(rund, 0);
                ioerr += cdk_xgsd_reg64_port_read(unit, blk_acc, port, 
                    BCM56560_A0_RJBRr, 0, (&rjbr._rjbr));
                counter->v[0] += RJBRr_GET(rjbr, 0);
                ioerr += cdk_xgsd_reg64_port_read(unit, blk_acc, port, 
                    BCM56560_A0_ROVRr, 0, (&rovr._rovr));
                counter->v[0] += ROVRr_GET(rovr, 0);
                ioerr += cdk_xgsd_reg64_port_read(unit, blk_acc, port, 
                    BCM56560_A0_RFRGr, 0, (&rfrg._rfrg));
                counter->v[0] += RFRGr_GET(rfrg, 0);
                ioerr += cdk_xgsd_reg64_port_read(unit, blk_acc, port, 
                    BCM56560_A0_RRPKTr, 0, (&rrpkt._rrpkt));
                counter->v[0] += RRPKTr_GET(rrpkt, 0);
                break;
            default:
                break;
            }
        
        } else {
    
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
                ioerr += READ_TJBRr(unit, port, &tjbr);
                counter->v[0] += TJBRr_GET(tjbr, 0);
                ioerr += READ_TFCSr(unit, port, &tfcs);
                counter->v[0] += TFCSr_GET(tfcs, 0);
                ioerr += READ_TFRGr(unit, port, &tfrg);
                counter->v[0] += TFRGr_GET(tfrg, 0);
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
                ioerr += READ_RFLRr(unit, port, &rflr);
                counter->v[0] += RFLRr_GET(rflr, 0);
                ioerr += READ_RMTUEr(unit, port, &rmtue);
                counter->v[0] += RMTUEr_GET(rmtue, 0);
                ioerr += READ_RUNDr(unit, port, &rund);
                counter->v[0] += RUNDr_GET(rund, 0);
                ioerr += READ_RJBRr(unit, port, &rjbr);
                counter->v[0] += RJBRr_GET(rjbr, 0);
                ioerr += READ_ROVRr(unit, port, &rovr);
                counter->v[0] += ROVRr_GET(rovr, 0);
                ioerr += READ_RFRGr(unit, port, &rfrg);
                counter->v[0] += RFRGr_GET(rfrg, 0);
                ioerr += READ_RRPKTr(unit, port, &rrpkt);
                counter->v[0] += RRPKTr_GET(rrpkt, 0);
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
        counter->v[0] += RDBGC0r_COUNTf_GET(rdbgc0);
        break;
    default:
        break;
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}
#endif /* CDK_CONFIG_INCLUDE_BCM56560_A0 */

