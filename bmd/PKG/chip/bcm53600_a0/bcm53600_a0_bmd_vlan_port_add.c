#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53600_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#include <cdk/chip/bcm53600_a0_defs.h>

#include "bcm53600_a0_bmd.h"

int
bcm53600_a0_bmd_vlan_port_add(int unit, int vlan, int port, uint32_t flags)
{
    int ioerr = 0;
    uint32_t pbmp;
    VLAN_1Qm_t vlan_tab;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_VLAN(unit, vlan);
    BMD_CHECK_PORT(unit, port);

    ioerr += READ_VLAN_1Qm(unit, vlan, &vlan_tab);
    if (VLAN_1Qm_MSPT_IDf_GET(vlan_tab) == 0) {
        return ioerr ? CDK_E_IO : CDK_E_NOT_FOUND;
    }
    pbmp = VLAN_1Qm_FORWARD_MAPf_GET(vlan_tab);
    if (pbmp & (1 << port)) {
        return ioerr ? CDK_E_IO : CDK_E_EXISTS;
    }
    VLAN_1Qm_FORWARD_MAPf_SET(vlan_tab, pbmp | (1 << port));
    if (flags & BMD_VLAN_PORT_F_UNTAGGED) {
        pbmp = VLAN_1Qm_V_UNTAG_MAPf_GET(vlan_tab);
        VLAN_1Qm_V_UNTAG_MAPf_SET(vlan_tab, pbmp | (1 << port));
    }
    ioerr += WRITE_VLAN_1Qm(unit, vlan, vlan_tab);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

#endif /* CDK_CONFIG_INCLUDE_BCM53600_A0 */