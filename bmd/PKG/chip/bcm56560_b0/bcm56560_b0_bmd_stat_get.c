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
#include <cdk/cdk_debug.h>
#include <cdk/chip/bcm56560_b0_defs.h>

#include "bcm56560_b0_bmd.h"
#include "bcm56560_b0_internal.h"

int
bcm56560_b0_bmd_stat_get(int unit, int port, bmd_stat_t stat,
                         bmd_counter_t *counter)
{
    int ioerr = 0;
    int lport;
    cdk_pbmp_t cxxpbmp, clgpbmp;
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
    
    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);

    CDK_MEMSET(counter, 0, sizeof(*counter));

    bcm56560_b0_cxxport_pbmp_get(unit, &cxxpbmp);
    bcm56560_b0_clg2port_pbmp_get(unit, &clgpbmp);
    /* Only support lower 32-bits of counters */
    if (port != CMIC_PORT) {
        if (CDK_PBMP_MEMBER(clgpbmp, port)) {
            switch (stat) {
            case bmdStatTxPackets:
                ioerr += READ_CLG2MIB_TPKTr(unit, port, &clgtpkt);
                counter->v[0] += CLG2MIB_TPKTr_GET(clgtpkt, 0);
                break;
            case bmdStatTxBytes:
                ioerr += READ_CLG2MIB_TBYTr(unit, port, &clgtbyt);
                counter->v[0] += CLG2MIB_TBYTr_GET(clgtbyt, 0);
                break;
            case bmdStatTxErrors:
                ioerr += READ_CLG2MIB_TJBRr(unit, port, &clgtjbr);
                counter->v[0] += CLG2MIB_TJBRr_GET(clgtjbr, 0);
                ioerr += READ_CLG2MIB_TFCSr(unit, port, &clgtfcs);
                counter->v[0] += CLG2MIB_TFCSr_GET(clgtfcs, 0);
                ioerr += READ_CLG2MIB_TFRGr(unit, port, &clgtfrg);
                counter->v[0] += CLG2MIB_TFRGr_GET(clgtfrg, 0);
                ioerr += READ_CLG2MIB_TOVRr(unit, port, &clgtovr);
                counter->v[0] += CLG2MIB_TOVRr_GET(clgtovr, 0);
                ioerr += READ_CLG2MIB_TUFLr(unit, port, &clgtufl);
                counter->v[0] += CLG2MIB_TUFLr_GET(clgtufl, 0);
                ioerr += READ_CLG2MIB_TERRr(unit, port, &clgterr);
                counter->v[0] += CLG2MIB_TERRr_GET(clgterr, 0);
                break;
            case bmdStatRxPackets:
                ioerr += READ_CLG2MIB_RPKTr(unit, port, &clgrpkt);
                counter->v[0] += CLG2MIB_RPKTr_GET(clgrpkt, 0);
                break;
            case bmdStatRxBytes:
                ioerr += READ_CLG2MIB_RBYTr(unit, port, &clgrbyt);
                counter->v[0] += CLG2MIB_RBYTr_GET(clgrbyt, 0);
                break;
            case bmdStatRxErrors:
                ioerr += READ_CLG2MIB_RFCSr(unit, port, &clgrfcs);
                counter->v[0] += CLG2MIB_RFCSr_GET(clgrfcs, 0);
                ioerr += READ_CLG2MIB_RFLRr(unit, port, &clgrflr);
                counter->v[0] += CLG2MIB_RFLRr_GET(clgrflr, 0);
                ioerr += READ_CLG2MIB_RMTUEr(unit, port, &clgrmtue);
                counter->v[0] += CLG2MIB_RMTUEr_GET(clgrmtue, 0);
                ioerr += READ_CLG2MIB_RUNDr(unit, port, &clgrund);
                counter->v[0] += CLG2MIB_RUNDr_GET(clgrund, 0);
                ioerr += READ_CLG2MIB_RJBRr(unit, port, &clgrjbr);
                counter->v[0] += CLG2MIB_RJBRr_GET(clgrjbr, 0);
                ioerr += READ_CLG2MIB_ROVRr(unit, port, &clgrovr);
                counter->v[0] += CLG2MIB_ROVRr_GET(clgrovr, 0);
                ioerr += READ_CLG2MIB_RFRGr(unit, port, &clgrfrg);
                counter->v[0] += CLG2MIB_RFRGr_GET(clgrfrg, 0);
                ioerr += READ_CLG2MIB_RRPKTr(unit, port, &clgrrpkt);
                counter->v[0] += CLG2MIB_RRPKTr_GET(clgrrpkt, 0);
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
#endif /* CDK_CONFIG_INCLUDE_BCM56560_B0 */

