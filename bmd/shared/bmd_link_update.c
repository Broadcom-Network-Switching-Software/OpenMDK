/*
 *
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>
#include <bmd/bmd_device.h>

#include <bmdi/bmd_link.h>

/* Track port status changes */
static uint32_t port_status[BMD_CONFIG_MAX_UNITS][BMD_CONFIG_MAX_PORTS];

/*
 * Function:
 *	bmd_link_update
 * Purpose:
 *	Update port status flags.
 * Parameters:
 *	unit - BMD device
 *	port - port number to update
 *	status_change - (OUT) non-zero if port status has changed
 * Returns:
 *      CDK_XXX
 * Notes:
 *      This is a helper function for the bmd_port_mode_update API.
 */
int
bmd_link_update(int unit, int port, int *status_change)
{
    int rv = CDK_E_NONE;
    int an, an_done, link;

    if ((BMD_PORT_STATUS(unit, port) & BMD_PST_FORCE_LINK) == 0) {
        rv = bmd_phy_link_get(unit, port, &link, &an_done);
        if (CDK_SUCCESS(rv)) {
            if (link) {
                BMD_PORT_STATUS_SET(unit, port, BMD_PST_LINK_UP);
                rv = bmd_phy_autoneg_get(unit, port, &an);
                if (CDK_SUCCESS(rv) && an && an_done) {
                    BMD_PORT_STATUS_SET(unit, port, BMD_PST_AN_DONE);
                }
            } else {
                BMD_PORT_STATUS_CLR(unit, port, 
                                    BMD_PST_LINK_UP | BMD_PST_AN_DONE);
            }
        }
    }

    *status_change = 0;
    if (port_status[unit][port] != BMD_PORT_STATUS(unit, port)) {
        BMD_PORT_STATUS_CLR(unit, port, BMD_PST_FORCE_UPDATE);
        port_status[unit][port] = BMD_PORT_STATUS(unit, port);
        *status_change = 1;
    }

    return rv;
}
