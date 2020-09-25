/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <cdk_config.h>
#include <cdk/cdk_device.h>

#if CDK_CONFIG_INCLUDE_DYN_CONFIG == 1
static cdk_port_config_t *
_port_config_get(int unit, int port)
{
    int pcfg_id;

    pcfg_id = CDK_DEV(unit)->port_config_id[port];
    
    if (pcfg_id < CDK_DEV(unit)->num_port_configs) {
        return &CDK_DEV(unit)->port_configs[pcfg_id];
    }
    return NULL;
}
#endif

/*
 * Function:
 *	cdk_dev_port_speed_max_get
 * Purpose:
 *	Get maximum allowed speed for a given port.
 * Parameters:
 *      unit - unit number
 *      port - port number
 * Returns:
 *      Maximum port speed.
 */
uint32_t
cdk_dev_port_speed_max_get(int unit, int port)
{
#if CDK_CONFIG_INCLUDE_DYN_CONFIG == 1
    cdk_port_config_t *pcfg = _port_config_get(unit, port);

    if (pcfg != NULL) {
        return pcfg->speed_max;
    }
#endif
    return 0;
}

/*
 * Function:
 *	cdk_dev_port_flags_get
 * Purpose:
 *	Get default port flags for a given port.
 * Parameters:
 *      unit - unit number
 *      port - port number
 * Returns:
 *      Default port flags.
 */
uint32_t
cdk_dev_port_flags_get(int unit, int port)
{
#if CDK_CONFIG_INCLUDE_DYN_CONFIG == 1
    cdk_port_config_t *pcfg = _port_config_get(unit, port);

    if (pcfg != NULL) {
        return pcfg->port_flags;
    }
#endif
    return 0;
}

/*
 * Function:
 *	cdk_dev_port_mode_get
 * Purpose:
 *	Get default port mode for a given port.
 * Parameters:
 *      unit - unit number
 *      port - port number
 * Returns:
 *      Default port mode.
 */
int
cdk_dev_port_mode_get(int unit, int port)
{
#if CDK_CONFIG_INCLUDE_DYN_CONFIG == 1
    cdk_port_config_t *pcfg = _port_config_get(unit, port);

    if (pcfg != NULL) {
        return pcfg->port_mode;
    }
#endif
    return 0;
}

/*
 * Function:
 *	cdk_dev_sys_port_get
 * Purpose:
 *	Get system port mapping for a given (physical) port.
 * Parameters:
 *      unit - unit number
 *      port - port number
 * Returns:
 *      System port number.
 */
int
cdk_dev_sys_port_get(int unit, int port)
{
#if CDK_CONFIG_INCLUDE_DYN_CONFIG == 1
    cdk_port_config_t *pcfg = _port_config_get(unit, port);

    if (pcfg != NULL) {
        if (pcfg->sys_port >= 0) {
            return pcfg->sys_port;
        }
        return -1;
    }
#endif
    return port;
}

/*
 * Function:
 *	cdk_dev_num_lanes_get
 * Purpose:
 *	Get override lane number for a given (physical) port.
 * Parameters:
 *      unit - unit number
 *      port - port number
 * Returns:
 *      System port number.
 */
int
cdk_dev_num_lanes_get(int unit, int port)
{
#if CDK_CONFIG_INCLUDE_DYN_CONFIG == 1
    cdk_port_config_t *pcfg = _port_config_get(unit, port);

    if (pcfg != NULL) {
        if (pcfg->num_lanes > 0) {
            return pcfg->num_lanes;
        }
        return -1;
    }
#endif
    return port;
}
