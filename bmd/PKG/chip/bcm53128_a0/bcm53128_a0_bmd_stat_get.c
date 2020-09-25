#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53128_A0 == 1

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
#include <cdk/chip/bcm53128_a0_defs.h>

#include "bcm53128_a0_bmd.h"

int
bcm53128_a0_bmd_stat_get(int unit, int port, bmd_stat_t stat, bmd_counter_t *counter)
{
    TXUNICASTPKTSr_t txunicastpkts;
    TXMULTICASTPKTSr_t txmulticastpkts;
    TXBROADCASTPKTSr_t txbroadcastpkts;
    TXOCTETSr_t txoctets;
    TXEXCESSIVECOLLISIONr_t txexcessivecollision;
    TXFRAMEINDISCr_t txframeindisc;
    TXDROPPKTSr_t txdroppkts;
    RXUNICASTPKTSr_t rxunicastpkts;
    RXMULTICASTPKTSr_t rxmulticastpkts;
    RXBROADCASTPKTr_t rxbroadcastpkt;
    RXOCTETSr_t rxoctets;
    RXJABBERSr_t rxjabbers;
    RXFCSERRORSr_t rxfcserrors;
    RXOVERSIZEPKTSr_t rxoversizepkts;
    RXALIGNMENTERRORSr_t rxalignmenterrors;
    RXSYMBLERRr_t rxsymblerr;
    RXUNDERSIZEPKTSr_t rxundersizepkts;
    RXFRAGMENTSr_t rxfragments;
    RXDROPPKTSr_t rxdroppkts;
    RXDISCARDr_t rxdiscard;
    int ioerr = 0;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);

    CDK_MEMSET(counter, 0, sizeof(*counter));

    switch (stat) {
    case bmdStatTxPackets:
        ioerr += READ_TXUNICASTPKTSr(unit, port, &txunicastpkts);
        counter->v[0] += TXUNICASTPKTSr_GET(txunicastpkts);
        ioerr += READ_TXMULTICASTPKTSr(unit, port, &txmulticastpkts);
        counter->v[0] += TXMULTICASTPKTSr_GET(txmulticastpkts);
        ioerr += READ_TXBROADCASTPKTSr(unit, port, &txbroadcastpkts);
        counter->v[0] += TXBROADCASTPKTSr_GET(txbroadcastpkts);
        break;
    case bmdStatTxBytes:
        ioerr += READ_TXOCTETSr(unit, port, &txoctets);
        counter->v[0] += TXOCTETSr_GET(txoctets, 0);
        break;
    case bmdStatTxErrors:
        ioerr += READ_TXEXCESSIVECOLLISIONr(unit, port, &txexcessivecollision);
        counter->v[0] += TXEXCESSIVECOLLISIONr_GET(txexcessivecollision);
        ioerr += READ_TXFRAMEINDISCr(unit, port, &txframeindisc);
        counter->v[0] += TXFRAMEINDISCr_GET(txframeindisc);
        ioerr += READ_TXDROPPKTSr(unit, port, &txdroppkts);
        counter->v[0] += TXDROPPKTSr_GET(txdroppkts);
        break;
    case bmdStatRxPackets:
        ioerr += READ_RXUNICASTPKTSr(unit, port, &rxunicastpkts);
        counter->v[0] += RXUNICASTPKTSr_GET(rxunicastpkts);
        ioerr += READ_RXMULTICASTPKTSr(unit, port, &rxmulticastpkts);
        counter->v[0] += RXMULTICASTPKTSr_GET(rxmulticastpkts);
        ioerr += READ_RXBROADCASTPKTr(unit, port, &rxbroadcastpkt);
        counter->v[0] += RXBROADCASTPKTr_GET(rxbroadcastpkt);
        break;
    case bmdStatRxBytes:
        ioerr += READ_RXOCTETSr(unit, port, &rxoctets);
        counter->v[0] += RXOCTETSr_GET(rxoctets, 0);
        break;
    case bmdStatRxErrors:
        ioerr += READ_RXJABBERSr(unit, port, &rxjabbers);
        counter->v[0] += RXJABBERSr_GET(rxjabbers);
        ioerr += READ_RXFCSERRORSr(unit, port, &rxfcserrors);
        counter->v[0] += RXFCSERRORSr_GET(rxfcserrors);
        ioerr += READ_RXOVERSIZEPKTSr(unit, port, &rxoversizepkts);
        counter->v[0] += RXOVERSIZEPKTSr_GET(rxoversizepkts);
        ioerr += READ_RXALIGNMENTERRORSr(unit, port, &rxalignmenterrors);
        counter->v[0] += RXALIGNMENTERRORSr_GET(rxalignmenterrors);
        ioerr += READ_RXSYMBLERRr(unit, port, &rxsymblerr);
        counter->v[0] += RXSYMBLERRr_GET(rxsymblerr);
        ioerr += READ_RXUNDERSIZEPKTSr(unit, port, &rxundersizepkts);
        counter->v[0] += RXUNDERSIZEPKTSr_GET(rxundersizepkts);
        ioerr += READ_RXFRAGMENTSr(unit, port, &rxfragments);
        counter->v[0] += RXFRAGMENTSr_GET(rxfragments);
        ioerr += READ_RXDROPPKTSr(unit, port, &rxdroppkts);
        counter->v[0] += RXDROPPKTSr_GET(rxdroppkts);
        break;
    case bmdStatRxDrops:
        ioerr += READ_RXDISCARDr(unit, port, &rxdiscard);
        counter->v[0] += RXDISCARDr_GET(rxdiscard);
        break;
    default:
        break;
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

#endif /* CDK_CONFIG_INCLUDE_BCM53128_A0 */
