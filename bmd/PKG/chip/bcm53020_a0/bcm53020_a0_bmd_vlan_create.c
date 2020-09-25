#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53020_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#include <cdk/chip/bcm53020_a0_defs.h>

#include "bcm53020_a0_bmd.h"

int
bcm53020_a0_bmd_vlan_create(int unit, int vlan)
{
    int ioerr = 0;
    VLAN_1Qm_t vlan_tab;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_VLAN(unit, vlan);

    ioerr += READ_VLAN_1Qm(unit, vlan, &vlan_tab);
    if (VLAN_1Qm_MSPT_IDf_GET(vlan_tab) != 0) {
#ifdef PRERELEASE_STAGE_FPGA
        /* the reset process will be skipped for FPGA board, the 
         * vlan 1 create in multipole init process (example : to
         * run bmddiag test) will got CDK exists error code in 
         * original process.
         */
        if (vlan != BMD_CONFIG_DEFAULT_VLAN) {
            return CDK_E_EXISTS;
        }
#else   /* PRERELEASE_STAGE_FPGA */
        return CDK_E_EXISTS;
#endif   /* PRERELEASE_STAGE_FPGA */
    }
    VLAN_1Qm_MSPT_IDf_SET(vlan_tab, 1);
    ioerr += WRITE_VLAN_1Qm(unit, vlan, vlan_tab);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}
#endif /* CDK_CONFIG_INCLUDE_BCM53020_A0 */
