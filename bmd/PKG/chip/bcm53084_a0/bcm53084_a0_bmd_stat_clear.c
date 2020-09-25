#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53084_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#include <cdk/chip/bcm53084_a0_defs.h>

#include "bcm53084_a0_bmd.h"

typedef union {
    TXUNICASTPKTSr_t txunicastpkts;
    TXMULTICASTPKTSr_t txmulticastpkts;
    TXBROADCASTPKTSr_t txbroadcastpkts;
    TXPAUSEPKTSr_t txpausepkts;
    TXOCTETSr_t txoctets;
    TXEXCESSIVECOLLISIONr_t txexcessivecollision;
    TXLATECOLLISIONr_t txlatecollision;
    RXUNICASTPKTSr_t rxunicastpkts;
    RXMULTICASTPKTSr_t rxmulticastpkts;
    RXBROADCASTPKTSr_t rxbroadcastpkts;
    RXPAUSEPKTSr_t rxpausepkts;
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
} bcm53084_a0_counter_t;

int 
bcm53084_a0_bmd_stat_clear(int unit, int port, bmd_stat_t stat)
{
    int ioerr = 0;
    bcm53084_a0_counter_t ctr;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);

    CDK_MEMSET(&ctr, 0, sizeof(ctr));

    switch (stat) {
    case bmdStatTxPackets:
        ioerr += WRITE_TXUNICASTPKTSr(unit, port, ctr.txunicastpkts);
        ioerr += WRITE_TXMULTICASTPKTSr(unit, port, ctr.txmulticastpkts);
        ioerr += WRITE_TXBROADCASTPKTSr(unit, port, ctr.txbroadcastpkts);
        ioerr += WRITE_TXPAUSEPKTSr(unit, port, ctr.txpausepkts);
        break;
    case bmdStatTxBytes:
        ioerr += WRITE_TXOCTETSr(unit, port, ctr.txoctets);
        break;
    case bmdStatTxErrors:
        ioerr += WRITE_TXEXCESSIVECOLLISIONr(unit, 
			port, ctr.txexcessivecollision);
        ioerr += WRITE_TXLATECOLLISIONr(unit, port, ctr.txlatecollision);
        break;
    case bmdStatRxPackets:
        ioerr += WRITE_RXUNICASTPKTSr(unit, port, ctr.rxunicastpkts);
        ioerr += WRITE_RXMULTICASTPKTSr(unit, port, ctr.rxmulticastpkts);
        ioerr += WRITE_RXBROADCASTPKTSr(unit, port, ctr.rxbroadcastpkts);
        ioerr += WRITE_RXPAUSEPKTSr(unit, port, ctr.rxpausepkts);
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
        break;
    case bmdStatRxDrops:
        ioerr += WRITE_RXDROPPKTSr(unit, port, ctr.rxdroppkts);
        ioerr += WRITE_RXDISCARDr(unit, port, ctr.rxdiscard);
        break;
    default:
        break;
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

#endif /* CDK_CONFIG_INCLUDE_BCM53084_A0 */
