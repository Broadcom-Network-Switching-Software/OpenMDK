#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53280_B0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#include <cdk/chip/bcm53280_b0_defs.h>

#include "bcm53280_b0_bmd.h"

int
bcm53280_b0_bmd_port_vlan_get(int unit, int port, int *vlan)
{
    int ioerr = 0;
    
    DEF_PORT_QOS_CFGr_t def_port_qos_cfg;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);


    ioerr += READ_DEF_PORT_QOS_CFGr(unit, port, (uint32_t *)&def_port_qos_cfg);
    *vlan = DEF_PORT_QOS_CFGr_SVIDf_GET(def_port_qos_cfg);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}
#endif /* CDK_CONFIG_INCLUDE_BCM53280_B0 */
