/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53570_A0 == 1

#include <bmd/bmd.h>

#include <cdk/cdk_device.h>
#include <cdk/cdk_debug.h>
#include <cdk/cdk_error.h>
#include <cdk/chip/bcm53570_a0_defs.h>

#include "bcm53570_a0_bmd.h"
#include "bcm53570_a0_internal.h"

int 
bcm53570_a0_bmd_vlan_port_add(int unit, int vlan, int port, uint32_t flags)
{
    int ioerr = 0;
    VLAN_TABm_t vlan_tab;
    EGR_VLANm_t egr_vlan;
    uint32_t pbmp[PBM_LPORT_WORDS];
    int lport;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_VLAN(unit, vlan);
    BMD_CHECK_PORT(unit, port);

    lport = P2L(unit, port);

    ioerr += READ_VLAN_TABm(unit, vlan, &vlan_tab);
    if (VLAN_TABm_VALIDf_GET(vlan_tab) == 0) {
        return ioerr ? CDK_E_IO : CDK_E_NOT_FOUND;
    }
    VLAN_TABm_PORT_BITMAPf_GET(vlan_tab, pbmp);
    PBM_PORT_ADD(pbmp, lport);
    VLAN_TABm_PORT_BITMAPf_SET(vlan_tab, pbmp);
    ioerr += WRITE_VLAN_TABm(unit, vlan, vlan_tab);
    
    ioerr += READ_EGR_VLANm(unit, vlan, &egr_vlan);
    EGR_VLANm_PORT_BITMAPf_GET(egr_vlan, pbmp);
    PBM_PORT_ADD(pbmp, lport);
    EGR_VLANm_PORT_BITMAPf_SET(egr_vlan, pbmp);
    if (flags & BMD_VLAN_PORT_F_UNTAGGED) {
        EGR_VLANm_UT_BITMAPf_GET(egr_vlan, pbmp);
        PBM_PORT_ADD(pbmp, lport);
        EGR_VLANm_UT_BITMAPf_SET(egr_vlan, pbmp);
    }
    ioerr += WRITE_EGR_VLANm(unit, vlan, egr_vlan);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

#endif /* CDK_CONFIG_INCLUDE_BCM53570_A0 */
