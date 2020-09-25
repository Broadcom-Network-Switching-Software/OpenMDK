/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <cdk_config.h>
#include <cdk/cdk_device.h>

/*
 * Function:
 *	cdk_dev_lport_get
 * Purpose:
 *	Transform physical port number to logical port number.
 * Parameters:
 *      unit - unit number
 *      pport - physical port number
 * Returns:
 *      Logical port number.
 */
int
cdk_dev_lport_get(int unit, int pport)
{
#if CDK_CONFIG_INCLUDE_PORT_MAP == 1
    int p;

    if (CDK_DEV(unit)->port_map_size) {
        for (p = 0; p < CDK_DEV(unit)->port_map_size; p++) {
            if (pport == CDK_DEV(unit)->port_map[p]) {
                return p;
            }
        }
        return -1;
    }
#endif
    return pport;
}
