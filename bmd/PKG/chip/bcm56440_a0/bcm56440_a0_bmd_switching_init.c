#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56440_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>


#include <cdk/chip/bcm56440_a0_defs.h>
#include <cdk/arch/xgsm_chip.h>
#include <cdk/cdk_debug.h>

#include "bcm56440_a0_bmd.h"
#include "bcm56440_a0_internal.h"

static int
_config_port(int unit, int port, uint32_t vlan_flags, uint32_t port_mode_flags)
{
    int rv;
    rv = bcm56440_a0_bmd_vlan_port_add(unit, BMD_CONFIG_DEFAULT_VLAN,
                                       port, vlan_flags);
    if (CDK_SUCCESS(rv)) {
        rv = bcm56440_a0_bmd_port_stp_set(unit, port, 
                                          bmdSpanningTreeForwarding);
    }
    if (CDK_SUCCESS(rv)) {
        rv = bcm56440_a0_bmd_port_mode_set(unit, port, bmdPortModeAuto,
                                           port_mode_flags);
    }
    return rv;
}

int
bcm56440_a0_bmd_switching_init(int unit)
{
    int ioerr = 0;
    int rv =0;
    int port, lport;
    cdk_pbmp_t pbmp, lpbmp;
    uint32_t vlan_flags;
    uint32_t port_mode_flags;
    uint32_t pbm;
    EPC_LINK_BMAPm_t epc_link;
    ING_COS_MODEr_t cos_mode;
    RQE_PORT_CONFIGr_t rqe_port;
    CTR_DEQ_STATS_CFGr_t ctr_deq;
    rv = bcm56440_a0_bmd_reset(unit);
    if (CDK_SUCCESS(rv)) {
        rv = bcm56440_a0_bmd_init(unit);
    }
    if (CDK_SUCCESS(rv)) {
        rv = bcm56440_a0_bmd_vlan_create(unit, BMD_CONFIG_DEFAULT_VLAN);
    }
    vlan_flags = BMD_VLAN_PORT_F_UNTAGGED;

    CDK_XGSM_BLKTYPE_PBMP_GET(unit, BLKTYPE_GPORT, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        if (CDK_SUCCESS(rv)) {
            rv = _config_port(unit, port, vlan_flags, 0);
        }
    }

    CDK_XGSM_BLKTYPE_PBMP_GET(unit, BLKTYPE_MXQPORT, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        if (port <= 34 ) {
            port_mode_flags = 0;
            if (BMD_PORT_PROPERTIES(unit, port) & BMD_PORT_HG) {
                port_mode_flags |= BMD_PORT_MODE_F_HIGIG;
            }
            if (CDK_SUCCESS(rv)) {
                rv = _config_port(unit, port, vlan_flags, port_mode_flags);
            }
        }
    }

    vlan_flags = 0;

    if (CDK_SUCCESS(rv)) {
        rv = bcm56440_a0_bmd_vlan_port_add(unit, BMD_CONFIG_DEFAULT_VLAN,
                                           CMIC_PORT, vlan_flags);
    }

    /* Enable all ports in MMU */
    CDK_PBMP_CLEAR(lpbmp);
    CDK_PBMP_ITER(pbmp, port) {
        lport = P2L(unit, port);
        CDK_PBMP_PORT_ADD(lpbmp, lport);
    }
    CDK_XGSM_BLKTYPE_PBMP_GET(unit, BLKTYPE_GPORT, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        lport = P2L(unit, port);
        CDK_PBMP_PORT_ADD(lpbmp, lport);
    }
    lport = P2L(unit, CMIC_PORT);
    CDK_PBMP_PORT_ADD(lpbmp, lport);

    EPC_LINK_BMAPm_CLR(epc_link);
    pbm = CDK_PBMP_WORD_GET(lpbmp, 0);
    EPC_LINK_BMAPm_PORT_BITMAP_W0f_SET(epc_link, pbm);
    pbm = CDK_PBMP_WORD_GET(lpbmp, 1);
    EPC_LINK_BMAPm_PORT_BITMAP_W1f_SET(epc_link, pbm);
    ioerr += WRITE_EPC_LINK_BMAPm(unit, 0, epc_link);

    /* cosq */
    CDK_PBMP_ITER(lpbmp, lport) {
        ioerr += READ_ING_COS_MODEr(unit,lport, &cos_mode);
        ioerr += READ_RQE_PORT_CONFIGr(unit, lport, &rqe_port);
        RQE_PORT_CONFIGr_BASE_QUEUEf_SET(rqe_port, ((0 == lport) ? 0 : ((8*lport)+40)));
        ING_COS_MODEr_BASE_QUEUE_NUMf_SET(cos_mode, ((0 == lport) ? 0 : ((8*lport)+40)));
        ioerr += WRITE_ING_COS_MODEr(unit, lport, cos_mode);
        ioerr += WRITE_RQE_PORT_CONFIGr(unit, lport, rqe_port);
    }

    CTR_DEQ_STATS_CFGr_ACTIVE0f_SET(ctr_deq, 1);
    WRITE_CTR_DEQ_STATS_CFGr(unit, 0, ctr_deq); 

    return ioerr ? CDK_E_IO : rv;
}
#endif /* CDK_CONFIG_INCLUDE_BCM56440_A0 */
