/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56670_A0 == 1

#include <bmd/bmd.h>

#include <cdk/cdk_device.h>
#include <cdk/cdk_error.h>
#include <cdk/chip/bcm56670_a0_defs.h>

#include "bcm56670_a0_bmd.h"
#include "bcm56670_a0_internal.h"

typedef union {
    RDBGC0r_t rdbgc0;
    XLMIB_RRPKTr_t xlrrpkt;
    XLMIB_TPKTr_t xltpkt;
    XLMIB_TBYTr_t xltbyt;
    XLMIB_TJBRr_t xltjbr;
    XLMIB_TFCSr_t xltfcs;
    XLMIB_TFRGr_t xltfrg;
    XLMIB_TOVRr_t xltovr;
    XLMIB_TUFLr_t xltufl;
    XLMIB_TERRr_t xlterr;
    XLMIB_RPKTr_t xlrpkt;
    XLMIB_RBYTr_t xlrbyt;
    XLMIB_RFCSr_t xlrfcs;
    XLMIB_RFLRr_t xlrflr;
    XLMIB_RMTUEr_t xlrmtue;
    XLMIB_RUNDr_t xlrund;
    XLMIB_RJBRr_t xlrjbr;
    XLMIB_ROVRr_t xlrovr;
    XLMIB_RFRGr_t xlrfrg;
    CLMIB_RRPKTr_t clrrpkt;
    CLMIB_TPKTr_t cltpkt;
    CLMIB_TBYTr_t cltbyt;
    CLMIB_TJBRr_t cltjbr;
    CLMIB_TFCSr_t cltfcs;
    CLMIB_TFRGr_t cltfrg;
    CLMIB_TOVRr_t cltovr;
    CLMIB_TUFLr_t cltufl;
    CLMIB_TERRr_t clterr;
    CLMIB_RPKTr_t clrpkt;
    CLMIB_RBYTr_t clrbyt;
    CLMIB_RFCSr_t clrfcs;
    CLMIB_RFLRr_t clrflr;
    CLMIB_RMTUEr_t clrmtue;
    CLMIB_RUNDr_t clrund;
    CLMIB_RJBRr_t clrjbr;
    CLMIB_ROVRr_t clrovr;
    CLMIB_RFRGr_t clrfrg;
} bcm56670_a0_counter_t;

int
bcm56670_a0_bmd_stat_clear(int unit, int port, bmd_stat_t stat)
{
    int ioerr = 0;
    int lport;
    bcm56670_a0_counter_t ctr;
    cdk_pbmp_t clpbmp;
    
    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);

    CDK_MEMSET(&ctr, 0, sizeof(ctr));
    
    bcm56670_a0_clport_pbmp_get(unit, &clpbmp);
    if (port != CMIC_PORT) {
        if (CDK_PBMP_MEMBER(clpbmp, port)) {
            switch (stat) {
            case bmdStatTxPackets:
                ioerr += WRITE_CLMIB_TPKTr(unit, port, ctr.cltpkt);
                break;
            case bmdStatTxBytes:
                ioerr += WRITE_CLMIB_TBYTr(unit, port, ctr.cltbyt);
                break;
            case bmdStatTxErrors:
                ioerr += WRITE_CLMIB_TJBRr(unit, port, ctr.cltjbr);
                ioerr += WRITE_CLMIB_TFRGr(unit, port, ctr.cltfrg);
                ioerr += WRITE_CLMIB_TFCSr(unit, port, ctr.cltfcs);
                ioerr += WRITE_CLMIB_TOVRr(unit, port, ctr.cltovr);
                ioerr += WRITE_CLMIB_TUFLr(unit, port, ctr.cltufl);
                ioerr += WRITE_CLMIB_TERRr(unit, port, ctr.clterr);
                break;
            case bmdStatRxPackets:
                ioerr += WRITE_CLMIB_RPKTr(unit, port, ctr.clrpkt);
                break;
            case bmdStatRxBytes:
                ioerr += WRITE_CLMIB_RBYTr(unit, port, ctr.clrbyt);
                break;
            case bmdStatRxErrors:
                ioerr += WRITE_CLMIB_RFCSr(unit, port, ctr.clrfcs);
                ioerr += WRITE_CLMIB_RFLRr(unit, port, ctr.clrflr);
                ioerr += WRITE_CLMIB_RMTUEr(unit, port, ctr.clrmtue);
                ioerr += WRITE_CLMIB_RUNDr(unit, port, ctr.clrund);
                ioerr += WRITE_CLMIB_RJBRr(unit, port, ctr.clrjbr);
                ioerr += WRITE_CLMIB_ROVRr(unit, port, ctr.clrovr);
                ioerr += WRITE_CLMIB_RFRGr(unit, port, ctr.clrfrg);
                ioerr += WRITE_CLMIB_RRPKTr(unit, port, ctr.clrrpkt);
                break;
            default:
                break;
            }
        } else {
            switch (stat) {
            case bmdStatTxPackets:
                ioerr += WRITE_XLMIB_TPKTr(unit, port, ctr.xltpkt);
                break;
            case bmdStatTxBytes:
                ioerr += WRITE_XLMIB_TBYTr(unit, port, ctr.xltbyt);
                break;
            case bmdStatTxErrors:
                ioerr += WRITE_XLMIB_TJBRr(unit, port, ctr.xltjbr);
                ioerr += WRITE_XLMIB_TFRGr(unit, port, ctr.xltfrg);
                ioerr += WRITE_XLMIB_TFCSr(unit, port, ctr.xltfcs);
                ioerr += WRITE_XLMIB_TOVRr(unit, port, ctr.xltovr);
                ioerr += WRITE_XLMIB_TUFLr(unit, port, ctr.xltufl);
                ioerr += WRITE_XLMIB_TERRr(unit, port, ctr.xlterr);
                break;
            case bmdStatRxPackets:
                ioerr += WRITE_XLMIB_RPKTr(unit, port, ctr.xlrpkt);
                break;
            case bmdStatRxBytes:
                ioerr += WRITE_XLMIB_RBYTr(unit, port, ctr.xlrbyt);
                break;
            case bmdStatRxErrors:
                ioerr += WRITE_XLMIB_RFCSr(unit, port, ctr.xlrfcs);
                ioerr += WRITE_XLMIB_RFLRr(unit, port, ctr.xlrflr);
                ioerr += WRITE_XLMIB_RMTUEr(unit, port, ctr.xlrmtue);
                ioerr += WRITE_XLMIB_RUNDr(unit, port, ctr.xlrund);
                ioerr += WRITE_XLMIB_RJBRr(unit, port, ctr.xlrjbr);
                ioerr += WRITE_XLMIB_ROVRr(unit, port, ctr.xlrovr);
                ioerr += WRITE_XLMIB_RFRGr(unit, port, ctr.xlrfrg);
                ioerr += WRITE_XLMIB_RRPKTr(unit, port, ctr.xlrrpkt);
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
#endif /* CDK_CONFIG_INCLUDE_BCM56670_A0 */

