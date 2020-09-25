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
#include <cdk/chip/bcm56560_a0_defs.h>

#include "bcm56560_a0_bmd.h"
#include "bcm56560_a0_internal.h"

typedef union {
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
} bcm56560_a0_counter_t;

int
bcm56560_a0_bmd_stat_clear(int unit, int port, bmd_stat_t stat)
{
    int ioerr = 0;
    int lport;
    bcm56560_a0_counter_t ctr;
    cdk_pbmp_t cxxpbmp;
    uint32_t blk_acc;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);

    CDK_MEMSET(&ctr, 0, sizeof(ctr));
    
    bcm56560_a0_cxxport_pbmp_get(unit, &cxxpbmp);
    if (port != CMIC_PORT) {
        if (CDK_PBMP_MEMBER(cxxpbmp, port) && (SPEED_MAX(unit, port) < 100000)) {
            blk_acc = 1 << BCM56560_A0_BLKTYPE_XLPORT;
            switch (stat) {
            case bmdStatTxPackets:
                ioerr += cdk_xgsd_reg64_port_write(unit, blk_acc, port, 
                    BCM56560_A0_TPKTr, 0, &(ctr.tpkt._tpkt));
                break;
            case bmdStatTxBytes:
                ioerr += cdk_xgsd_reg64_port_write(unit, blk_acc, port, 
                    BCM56560_A0_TBYTr, 0, &(ctr.tbyt._tbyt));
                break;
            case bmdStatTxErrors:
                ioerr += cdk_xgsd_reg64_port_write(unit, blk_acc, port, 
                    BCM56560_A0_TJBRr, 0, &(ctr.tjbr._tjbr));
                ioerr += cdk_xgsd_reg64_port_write(unit, blk_acc, port, 
                    BCM56560_A0_TFRGr, 0, &(ctr.tfrg._tfrg));
                ioerr += cdk_xgsd_reg64_port_write(unit, blk_acc, port, 
                    BCM56560_A0_TFCSr, 0, &(ctr.tfcs._tfcs));
                ioerr += cdk_xgsd_reg64_port_write(unit, blk_acc, port, 
                    BCM56560_A0_TOVRr, 0, &(ctr.tovr._tovr));
                ioerr += cdk_xgsd_reg64_port_write(unit, blk_acc, port, 
                    BCM56560_A0_TUFLr, 0, &(ctr.tufl._tufl));
                ioerr += cdk_xgsd_reg64_port_write(unit, blk_acc, port, 
                    BCM56560_A0_TERRr, 0, &(ctr.terr._terr));
                break;
            case bmdStatRxPackets:
                ioerr += cdk_xgsd_reg64_port_write(unit, blk_acc, port, 
                    BCM56560_A0_RPKTr, 0, &(ctr.rpkt._rpkt));
                break;
            case bmdStatRxBytes:
                ioerr += cdk_xgsd_reg64_port_write(unit, blk_acc, 
                    port, BCM56560_A0_RBYTr, 0, &(ctr.rbyt._rbyt));
                break;
            case bmdStatRxErrors:
                ioerr += cdk_xgsd_reg64_port_write(unit, blk_acc, port, 
                    BCM56560_A0_RFCSr, 0, &(ctr.rfcs._rfcs));
                ioerr += cdk_xgsd_reg64_port_write(unit, blk_acc, port, 
                    BCM56560_A0_RFLRr, 0, &(ctr.rflr._rflr));
                ioerr += cdk_xgsd_reg64_port_write(unit, blk_acc, port, 
                    BCM56560_A0_RMTUEr, 0, &(ctr.rmtue._rmtue));
                ioerr += cdk_xgsd_reg64_port_write(unit, blk_acc, port, 
                    BCM56560_A0_RUNDr, 0, &(ctr.rund._rund));
                ioerr += cdk_xgsd_reg64_port_write(unit, blk_acc, port, 
                    BCM56560_A0_RJBRr, 0, &(ctr.rjbr._rjbr));
                ioerr += cdk_xgsd_reg64_port_write(unit, blk_acc, port, 
                    BCM56560_A0_ROVRr, 0, &(ctr.rovr._rovr));
                ioerr += cdk_xgsd_reg64_port_write(unit, blk_acc, port, 
                    BCM56560_A0_RFRGr, 0, &(ctr.rfrg._rfrg));
                ioerr += cdk_xgsd_reg64_port_write(unit, blk_acc, port, 
                    BCM56560_A0_RRPKTr, 0, &(ctr.rrpkt._rrpkt));
                break;
            default:
                break;
            }
            
        } else {
            switch (stat) {
            case bmdStatTxPackets:
                ioerr += WRITE_TPKTr(unit, port, ctr.tpkt);
                break;
            case bmdStatTxBytes:
                ioerr += WRITE_TBYTr(unit, port, ctr.tbyt);
                break;
            case bmdStatTxErrors:
                ioerr += WRITE_TJBRr(unit, port, ctr.tjbr);
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
                ioerr += WRITE_RFLRr(unit, port, ctr.rflr);
                ioerr += WRITE_RMTUEr(unit, port, ctr.rmtue);
                ioerr += WRITE_RUNDr(unit, port, ctr.rund);
                ioerr += WRITE_RJBRr(unit, port, ctr.rjbr);
                ioerr += WRITE_ROVRr(unit, port, ctr.rovr);
                ioerr += WRITE_RFRGr(unit, port, ctr.rfrg);
                ioerr += WRITE_RRPKTr(unit, port, ctr.rrpkt);
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
#endif /* CDK_CONFIG_INCLUDE_BCM56560_A0 */

