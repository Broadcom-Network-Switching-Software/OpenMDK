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
 *	cdk_dev_pport_get
 * Purpose:
 *	Transform logical port number to physical port number.
 * Parameters:
 *      unit - unit number
 *      lport - logical port number
 * Returns:
 *      Physical port number.
 */
int
cdk_dev_pport_get(int unit, int lport)
{
#if CDK_CONFIG_INCLUDE_PORT_MAP == 1
    if (CDK_DEV(unit)->port_map_size) {
        if (lport >= 0 && lport < CDK_DEV(unit)->port_map_size) {
            return CDK_DEV(unit)->port_map[lport];
        }
        return -1;
    }
#endif
    return (lport < CDK_CONFIG_MAX_PORTS) ? lport : -1;
}
