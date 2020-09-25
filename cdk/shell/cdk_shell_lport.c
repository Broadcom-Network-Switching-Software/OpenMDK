/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <cdk/cdk_device.h>
#include <cdk/cdk_printf.h>
#include <cdk/cdk_shell.h>

/*
 * Function:
 *	cdk_shell_lport
 * Purpose:
 *	Create string with logical and physical port
 * Parameters:
 *	buf - (OUT) output buffer for bit range string
 *	size - size of output buffer
 *	unit - unit number
 *	port - physical port number
 * Returns:
 *      Always 0
 */
int
cdk_shell_lport(char *buf, int size, int unit, int port)
{
#if CDK_CONFIG_INCLUDE_PORT_MAP == 1
    /* Show logical port as well if port is mapped */
    if (CDK_DEV(unit)->port_map_size > 0) {
        CDK_SNPRINTF(buf, size, "%2d -> %2d", CDK_PORT_MAP_P2L(unit, port), port);
    } else 
#endif
    CDK_SNPRINTF(buf, size, "%2d", port);
    
    return 0;
}
