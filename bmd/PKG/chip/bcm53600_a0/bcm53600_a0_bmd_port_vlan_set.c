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
bcm53600_a0_bmd_port_vlan_set(int unit, int port, int vlan)
{
    int ioerr = 0;
    DEF_PORT_QOS_CFGr_t def_port_qos_cfg;
    
    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);

    ioerr += READ_DEF_PORT_QOS_CFGr(unit, port, (uint32_t *)&def_port_qos_cfg);
    DEF_PORT_QOS_CFGr_SVIDf_SET(def_port_qos_cfg, vlan);
    DEF_PORT_QOS_CFGr_CVIDf_SET(def_port_qos_cfg, vlan);
    ioerr += WRITE_DEF_PORT_QOS_CFGr(unit, port, def_port_qos_cfg);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}
#endif /* CDK_CONFIG_INCLUDE_BCM53600_A0 */
