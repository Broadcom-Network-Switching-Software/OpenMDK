/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56560_B0 == 1

#include <bmd/bmd.h>

#include <cdk/cdk_device.h>
#include <cdk/cdk_error.h>
#include <cdk/chip/bcm56560_b0_defs.h>

#include "bcm56560_b0_bmd.h"
#include "bcm56560_b0_internal.h"

typedef union {
    RDBGC0r_t rdbgc0;
    RRPKTr_t rrpkt;
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
    CLG2MIB_RRPKTr_t clgrrpkt;
    CLG2MIB_TPKTr_t clgtpkt;
    CLG2MIB_TBYTr_t clgtbyt;
    CLG2MIB_TJBRr_t clgtjbr;
    CLG2MIB_TFCSr_t clgtfcs;
    CLG2MIB_TFRGr_t clgtfrg;
    CLG2MIB_TOVRr_t clgtovr;
    CLG2MIB_TUFLr_t clgtufl;
    CLG2MIB_TERRr_t clgterr;
    CLG2MIB_RPKTr_t clgrpkt;
    CLG2MIB_RBYTr_t clgrbyt;
    CLG2MIB_RFCSr_t clgrfcs;
    CLG2MIB_RFLRr_t clgrflr;
    CLG2MIB_RMTUEr_t clgrmtue;
    CLG2MIB_RUNDr_t clgrund;
    CLG2MIB_RJBRr_t clgrjbr;
    CLG2MIB_ROVRr_t clgrovr;
    CLG2MIB_RFRGr_t clgrfrg;
} bcm56560_b0_counter_t;

int
bcm56560_b0_bmd_stat_clear(int unit, int port, bmd_stat_t stat)
{
    int ioerr = 0;
    int lport;
    bcm56560_b0_counter_t ctr;
    cdk_pbmp_t cxxpbmp, clgpbmp;
    
    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);

    CDK_MEMSET(&ctr, 0, sizeof(ctr));
    
    bcm56560_b0_cxxport_pbmp_get(unit, &cxxpbmp);
    bcm56560_b0_clg2port_pbmp_get(unit, &clgpbmp);
    if (port != CMIC_PORT) {
        if (CDK_PBMP_MEMBER(clgpbmp, port)) {
            switch (stat) {
            case bmdStatTxPackets:
                ioerr += WRITE_CLG2MIB_TPKTr(unit, port, ctr.clgtpkt);
                break;
            case bmdStatTxBytes:
                ioerr += WRITE_CLG2MIB_TBYTr(unit, port, ctr.clgtbyt);
                break;
            case bmdStatTxErrors:
                ioerr += WRITE_CLG2MIB_TJBRr(unit, port, ctr.clgtjbr);
                ioerr += WRITE_CLG2MIB_TFRGr(unit, port, ctr.clgtfrg);
                ioerr += WRITE_CLG2MIB_TFCSr(unit, port, ctr.clgtfcs);
                ioerr += WRITE_CLG2MIB_TOVRr(unit, port, ctr.clgtovr);
                ioerr += WRITE_CLG2MIB_TUFLr(unit, port, ctr.clgtufl);
                ioerr += WRITE_CLG2MIB_TERRr(unit, port, ctr.clgterr);
                break;
            case bmdStatRxPackets:
                ioerr += WRITE_CLG2MIB_RPKTr(unit, port, ctr.clgrpkt);
                break;
            case bmdStatRxBytes:
                ioerr += WRITE_CLG2MIB_RBYTr(unit, port, ctr.clgrbyt);
                break;
            case bmdStatRxErrors:
                ioerr += WRITE_CLG2MIB_RFCSr(unit, port, ctr.clgrfcs);
                ioerr += WRITE_CLG2MIB_RFLRr(unit, port, ctr.clgrflr);
                ioerr += WRITE_CLG2MIB_RMTUEr(unit, port, ctr.clgrmtue);
                ioerr += WRITE_CLG2MIB_RUNDr(unit, port, ctr.clgrund);
                ioerr += WRITE_CLG2MIB_RJBRr(unit, port, ctr.clgrjbr);
                ioerr += WRITE_CLG2MIB_ROVRr(unit, port, ctr.clgrovr);
                ioerr += WRITE_CLG2MIB_RFRGr(unit, port, ctr.clgrfrg);
                ioerr += WRITE_CLG2MIB_RRPKTr(unit, port, ctr.clgrrpkt);
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
#endif /* CDK_CONFIG_INCLUDE_BCM56560_B0 */

