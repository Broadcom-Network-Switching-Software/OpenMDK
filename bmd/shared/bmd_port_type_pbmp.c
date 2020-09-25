/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Return bitmap of specified BMD port type.
 */

#include <bmd/bmd_device.h>

int
bmd_port_type_pbmp(int unit, uint32_t port_type, cdk_pbmp_t *pbmp)
{
    int port;

    CDK_PBMP_CLEAR(*pbmp);
    for (port = 0; port < BMD_CONFIG_MAX_PORTS; port++) {
        if (BMD_PORT_PROPERTIES(unit, port) & port_type) {
            CDK_PBMP_ADD(*pbmp, port);
        }
    }
    return CDK_E_NONE;
}
