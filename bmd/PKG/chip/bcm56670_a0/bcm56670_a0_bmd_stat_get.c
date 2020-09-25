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
#include <cdk/cdk_debug.h>
#include <cdk/chip/bcm56670_a0_defs.h>

#include "bcm56670_a0_bmd.h"
#include "bcm56670_a0_internal.h"

int
bcm56670_a0_bmd_stat_get(int unit, int port, bmd_stat_t stat,
                         bmd_counter_t *counter)
{
    int ioerr = 0;
    int lport;
    cdk_pbmp_t clpbmp;
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
    
    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);

    CDK_MEMSET(counter, 0, sizeof(*counter));

    bcm56670_a0_clport_pbmp_get(unit, &clpbmp);
    /* Only support lower 32-bits of counters */
    if (port != CMIC_PORT) {
        if (CDK_PBMP_MEMBER(clpbmp, port)) {
            switch (stat) {
            case bmdStatTxPackets:
                ioerr += READ_CLMIB_TPKTr(unit, port, &cltpkt);
                counter->v[0] += CLMIB_TPKTr_GET(cltpkt, 0);
                break;
            case bmdStatTxBytes:
                ioerr += READ_CLMIB_TBYTr(unit, port, &cltbyt);
                counter->v[0] += CLMIB_TBYTr_GET(cltbyt, 0);
                break;
            case bmdStatTxErrors:
                ioerr += READ_CLMIB_TJBRr(unit, port, &cltjbr);
                counter->v[0] += CLMIB_TJBRr_GET(cltjbr, 0);
                ioerr += READ_CLMIB_TFCSr(unit, port, &cltfcs);
                counter->v[0] += CLMIB_TFCSr_GET(cltfcs, 0);
                ioerr += READ_CLMIB_TFRGr(unit, port, &cltfrg);
                counter->v[0] += CLMIB_TFRGr_GET(cltfrg, 0);
                ioerr += READ_CLMIB_TOVRr(unit, port, &cltovr);
                counter->v[0] += CLMIB_TOVRr_GET(cltovr, 0);
                ioerr += READ_CLMIB_TUFLr(unit, port, &cltufl);
                counter->v[0] += CLMIB_TUFLr_GET(cltufl, 0);
                ioerr += READ_CLMIB_TERRr(unit, port, &clterr);
                counter->v[0] += CLMIB_TERRr_GET(clterr, 0);
                break;
            case bmdStatRxPackets:
                ioerr += READ_CLMIB_RPKTr(unit, port, &clrpkt);
                counter->v[0] += CLMIB_RPKTr_GET(clrpkt, 0);
                break;
            case bmdStatRxBytes:
                ioerr += READ_CLMIB_RBYTr(unit, port, &clrbyt);
                counter->v[0] += CLMIB_RBYTr_GET(clrbyt, 0);
                break;
            case bmdStatRxErrors:
                ioerr += READ_CLMIB_RFCSr(unit, port, &clrfcs);
                counter->v[0] += CLMIB_RFCSr_GET(clrfcs, 0);
                ioerr += READ_CLMIB_RFLRr(unit, port, &clrflr);
                counter->v[0] += CLMIB_RFLRr_GET(clrflr, 0);
                ioerr += READ_CLMIB_RMTUEr(unit, port, &clrmtue);
                counter->v[0] += CLMIB_RMTUEr_GET(clrmtue, 0);
                ioerr += READ_CLMIB_RUNDr(unit, port, &clrund);
                counter->v[0] += CLMIB_RUNDr_GET(clrund, 0);
                ioerr += READ_CLMIB_RJBRr(unit, port, &clrjbr);
                counter->v[0] += CLMIB_RJBRr_GET(clrjbr, 0);
                ioerr += READ_CLMIB_ROVRr(unit, port, &clrovr);
                counter->v[0] += CLMIB_ROVRr_GET(clrovr, 0);
                ioerr += READ_CLMIB_RFRGr(unit, port, &clrfrg);
                counter->v[0] += CLMIB_RFRGr_GET(clrfrg, 0);
                ioerr += READ_CLMIB_RRPKTr(unit, port, &clrrpkt);
                counter->v[0] += CLMIB_RRPKTr_GET(clrrpkt, 0);
                break;
            default:
                break;
            }
        } else {
            switch (stat) {
            case bmdStatTxPackets:
                ioerr += READ_XLMIB_TPKTr(unit, port, &xltpkt);
                counter->v[0] += XLMIB_TPKTr_GET(xltpkt, 0);
                break;
            case bmdStatTxBytes:
                ioerr += READ_XLMIB_TBYTr(unit, port, &xltbyt);
                counter->v[0] += XLMIB_TBYTr_GET(xltbyt, 0);
                break;
            case bmdStatTxErrors:
                ioerr += READ_XLMIB_TJBRr(unit, port, &xltjbr);
                counter->v[0] += XLMIB_TJBRr_GET(xltjbr, 0);
                ioerr += READ_XLMIB_TFCSr(unit, port, &xltfcs);
                counter->v[0] += XLMIB_TFCSr_GET(xltfcs, 0);
                ioerr += READ_XLMIB_TFRGr(unit, port, &xltfrg);
                counter->v[0] += XLMIB_TFRGr_GET(xltfrg, 0);
                ioerr += READ_XLMIB_TOVRr(unit, port, &xltovr);
                counter->v[0] += XLMIB_TOVRr_GET(xltovr, 0);
                ioerr += READ_XLMIB_TUFLr(unit, port, &xltufl);
                counter->v[0] += XLMIB_TUFLr_GET(xltufl, 0);
                ioerr += READ_XLMIB_TERRr(unit, port, &xlterr);
                counter->v[0] += XLMIB_TERRr_GET(xlterr, 0);
                break;
            case bmdStatRxPackets:
                ioerr += READ_XLMIB_RPKTr(unit, port, &xlrpkt);
                counter->v[0] += XLMIB_RPKTr_GET(xlrpkt, 0);
                break;
            case bmdStatRxBytes:
                ioerr += READ_XLMIB_RBYTr(unit, port, &xlrbyt);
                counter->v[0] += XLMIB_RBYTr_GET(xlrbyt, 0);
                break;
            case bmdStatRxErrors:
                ioerr += READ_XLMIB_RFCSr(unit, port, &xlrfcs);
                counter->v[0] += XLMIB_RFCSr_GET(xlrfcs, 0);
                ioerr += READ_XLMIB_RFLRr(unit, port, &xlrflr);
                counter->v[0] += XLMIB_RFLRr_GET(xlrflr, 0);
                ioerr += READ_XLMIB_RMTUEr(unit, port, &xlrmtue);
                counter->v[0] += XLMIB_RMTUEr_GET(xlrmtue, 0);
                ioerr += READ_XLMIB_RUNDr(unit, port, &xlrund);
                counter->v[0] += XLMIB_RUNDr_GET(xlrund, 0);
                ioerr += READ_XLMIB_RJBRr(unit, port, &xlrjbr);
                counter->v[0] += XLMIB_RJBRr_GET(xlrjbr, 0);
                ioerr += READ_XLMIB_ROVRr(unit, port, &xlrovr);
                counter->v[0] += XLMIB_ROVRr_GET(xlrovr, 0);
                ioerr += READ_XLMIB_RFRGr(unit, port, &xlrfrg);
                counter->v[0] += XLMIB_RFRGr_GET(xlrfrg, 0);
                ioerr += READ_XLMIB_RRPKTr(unit, port, &xlrrpkt);
                counter->v[0] += XLMIB_RRPKTr_GET(xlrrpkt, 0);
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
#endif /* CDK_CONFIG_INCLUDE_BCM56670_A0 */

