#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53280_A0 == 1

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
#include <cdk/chip/bcm53280_a0_defs.h>

#include "bcm53280_a0_bmd.h"

typedef union {
    TXUNICASTPKTSr_t txunicastpkts;
    TXMULTICASTPKTSr_t txmulticastpkts;
    TXBROADCASTPKTSr_t txbroadcastpkts;
    TXOCTETSr_t txoctets;
    TXEXCESSIVECOLLISIONr_t txexcessivecollision;
    RXUNICASTPKTSr_t rxunicastpkts;
    RXMULTICASTPKTSr_t rxmulticastpkts;
    RXBROADCASTPKTSr_t rxbroadcastpkt;
    RXOCTETSr_t rxoctets;
    RXJABBERPKTSr_t rxjabbers;
    RXFCSERRORSr_t rxfcserrors;
    RXOVERSIZEPKTSr_t rxoversizepkts;
    RXALIGNMENTERRORSr_t rxalignmenterrors;
    RXSYMBOLERRr_t rxsymblerr;
    RXUNDERSIZEPKTSr_t rxundersizepkts;
    RXFRAGMENTSr_t rxfragments;
    RXFWDDISCPKTSr_t rxdiscard;
} bcm53280_a0_counter_t;

int
bcm53280_a0_bmd_stat_clear(int unit, int port, bmd_stat_t stat)
{
    int ioerr = 0;
    bcm53280_a0_counter_t ctr;
    MIB_PORT_SELr_t mib_port_sel;
    
    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);

    CDK_MEMSET(&ctr, 0, sizeof(ctr));

    MIB_PORT_SELr_MIB_PORTf_SET(mib_port_sel, port);
    ioerr += WRITE_MIB_PORT_SELr(unit, mib_port_sel);

    switch (stat) {
    case bmdStatTxPackets:
        ioerr += WRITE_TXUNICASTPKTSr(unit, ctr.txunicastpkts);
        ioerr += WRITE_TXMULTICASTPKTSr(unit, ctr.txmulticastpkts);
        ioerr += WRITE_TXBROADCASTPKTSr(unit, ctr.txbroadcastpkts);
        break;
    case bmdStatTxBytes:
        ioerr += WRITE_TXOCTETSr(unit, ctr.txoctets);
        break;
    case bmdStatTxErrors:
        ioerr += WRITE_TXEXCESSIVECOLLISIONr(unit, ctr.txexcessivecollision);
        break;
    case bmdStatRxPackets:
        ioerr += WRITE_RXUNICASTPKTSr(unit, ctr.rxunicastpkts);
        ioerr += WRITE_RXMULTICASTPKTSr(unit, ctr.rxmulticastpkts);
        ioerr += WRITE_RXBROADCASTPKTSr(unit, ctr.rxbroadcastpkt);
        break;
    case bmdStatRxBytes:
        ioerr += WRITE_RXOCTETSr(unit, ctr.rxoctets);
        break;
    case bmdStatRxErrors:
        ioerr += WRITE_RXJABBERPKTSr(unit, ctr.rxjabbers);
        ioerr += WRITE_RXFCSERRORSr(unit, ctr.rxfcserrors);
        ioerr += WRITE_RXOVERSIZEPKTSr(unit, ctr.rxoversizepkts);
        ioerr += WRITE_RXALIGNMENTERRORSr(unit, ctr.rxalignmenterrors);
        ioerr += WRITE_RXSYMBOLERRr(unit, ctr.rxsymblerr);
        ioerr += WRITE_RXUNDERSIZEPKTSr(unit, ctr.rxundersizepkts);
        ioerr += WRITE_RXFRAGMENTSr(unit, ctr.rxfragments);
        break;
    case bmdStatRxDrops:
        ioerr += WRITE_RXFWDDISCPKTSr(unit, ctr.rxdiscard);
        break;
    default:
        break;
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

#endif /* CDK_CONFIG_INCLUDE_BCM53280_A0 */
