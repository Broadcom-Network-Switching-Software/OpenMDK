#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53010_A0 == 1

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
#include <cdk/chip/bcm53010_a0_defs.h>

#include "bcm53010_a0_bmd.h"

typedef union {
    TXUNICASTPKTSr_t txunicastpkts;
    TXMULTICASTPKTSr_t txmulticastpkts;
    TXBROADCASTPKTSr_t txbroadcastpkts;
    TXOCTETSr_t txoctets;
    TXEXCESSIVECOLLISIONr_t txexcessivecollision;
    TXFRAMEINDISCr_t txframeindisc;
    TXDROPPKTSr_t txdroppkts;
    RXUNICASTPKTSr_t rxunicastpkts;
    RXMULTICASTPKTSr_t rxmulticastpkts;
    RXBROADCASTPKTSr_t rxbroadcastpkt;
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
} bcm53010_a0_counter_t;

int
bcm53010_a0_bmd_stat_clear(int unit, int port, bmd_stat_t stat)
{
    int ioerr = 0;
    bcm53010_a0_counter_t ctr;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);

    CDK_MEMSET(&ctr, 0, sizeof(ctr));

    switch (stat) {
    case bmdStatTxPackets:
        ioerr += WRITE_TXUNICASTPKTSr(unit, port, ctr.txunicastpkts);
        ioerr += WRITE_TXMULTICASTPKTSr(unit, port, ctr.txmulticastpkts);
        ioerr += WRITE_TXBROADCASTPKTSr(unit, port, ctr.txbroadcastpkts);
        break;
    case bmdStatTxBytes:
        ioerr += WRITE_TXOCTETSr(unit, port, ctr.txoctets);
        break;
    case bmdStatTxErrors:
        ioerr += WRITE_TXEXCESSIVECOLLISIONr(unit, port, ctr.txexcessivecollision);
        ioerr += WRITE_TXFRAMEINDISCr(unit, port, ctr.txframeindisc);
        ioerr += WRITE_TXDROPPKTSr(unit, port, ctr.txdroppkts);
        break;
    case bmdStatRxPackets:
        ioerr += WRITE_RXUNICASTPKTSr(unit, port, ctr.rxunicastpkts);
        ioerr += WRITE_RXMULTICASTPKTSr(unit, port, ctr.rxmulticastpkts);
        ioerr += WRITE_RXBROADCASTPKTSr(unit, port, ctr.rxbroadcastpkt);
        break;
    case bmdStatRxBytes:
        ioerr += WRITE_RXOCTETSr(unit, port, ctr.rxoctets);
        break;
    case bmdStatRxErrors:
        ioerr += WRITE_RXJABBERSr(unit, port, ctr.rxjabbers);
        ioerr += WRITE_RXFCSERRORSr(unit, port, ctr.rxfcserrors);
        ioerr += WRITE_RXOVERSIZEPKTSr(unit, port, ctr.rxoversizepkts);
        ioerr += WRITE_RXALIGNMENTERRORSr(unit, port, ctr.rxalignmenterrors);
        ioerr += WRITE_RXSYMBLERRr(unit, port, ctr.rxsymblerr);
        ioerr += WRITE_RXUNDERSIZEPKTSr(unit, port, ctr.rxundersizepkts);
        ioerr += WRITE_RXFRAGMENTSr(unit, port, ctr.rxfragments);
        ioerr += WRITE_RXDROPPKTSr(unit, port, ctr.rxdroppkts);
        break;
    case bmdStatRxDrops:
        ioerr += WRITE_RXDISCARDr(unit, port, ctr.rxdiscard);
        break;
    default:
        break;
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

#endif /* CDK_CONFIG_INCLUDE_BCM53010_A0 */
