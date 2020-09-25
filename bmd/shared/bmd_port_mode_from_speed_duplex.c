/*
 *
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#include <bmdi/bmd_port_mode.h>

/*
 * Function:
 *	bmd_port_mode_from_speed_duplex
 * Purpose:
 *	Determine BMD port mode based on speed and duplex.
 * Parameters:
 *	speed - port speed setting
 *	duplex - port duplex setting
 *	mode - (OUT) BMD port mode
 * Returns:
 *      CDK_XXX
 * Notes:
 *      This is a helper function for the bmd_port_mode_get API.
 */
int
bmd_port_mode_from_speed_duplex(uint32_t speed, int duplex, bmd_port_mode_t *mode)
{
    switch (speed) {
    case 0:
        *mode = bmdPortModeAuto;
        break;
    case 10:
        *mode = duplex ? bmdPortMode10fd : bmdPortMode10hd;
        break;
    case 100:
        *mode = duplex ? bmdPortMode100fd : bmdPortMode100hd;
        break;
    case 1000:
        *mode = duplex ? bmdPortMode1000fd : bmdPortMode1000hd;
        break;
    case 2500:
        /* Full duplex only */
        *mode = bmdPortMode2500fd;
        break;
    case 5000:
        /* Full duplex only */
        *mode = bmdPortMode5000fd;
        break;
    case 10000:
        /* Full duplex only */
        *mode = bmdPortMode10000fd;
        break;
    case 11000:
        /* Full duplex only */
        *mode = bmdPortMode11000fd;
        break;
    case 12000:
        /* Full duplex only */
        *mode = bmdPortMode12000fd;
        break;
    case 13000:
        /* Full duplex only */
        *mode = bmdPortMode13000fd;
        break;
    case 15000:
        /* Full duplex only */
        *mode = bmdPortMode15000fd;
        break;
    case 16000:
        /* Full duplex only */
        *mode = bmdPortMode16000fd;
        break;
    case 20000:
        /* Full duplex only */
        *mode = bmdPortMode20000fd;
        break;
    case 21000:
        /* Full duplex only */
        *mode = bmdPortMode21000fd;
        break;
    case 25000:
        /* Full duplex only */
        *mode = bmdPortMode25000fd;
        break;
    case 30000:
        /* Full duplex only */
        *mode = bmdPortMode30000fd;
        break;
    case 40000:
        /* Full duplex only */
        *mode = bmdPortMode40000fd;
        break;
    case 42000:
        /* Full duplex only */
        *mode = bmdPortMode42000fd;
        break;
    case 50000:
        /* Full duplex only */
        *mode = bmdPortMode50000fd;
        break;
    case 53000:
        /* Full duplex only */
        *mode = bmdPortMode53000fd;
        break;
    case 100000:
        /* Full duplex only */
        *mode = bmdPortMode100000fd;
        break;
    case 127000:
        /* Full duplex only */
        *mode = bmdPortMode127000fd;
        break;
    default:
        return CDK_E_INTERNAL;
    }
    return CDK_E_NONE;
}
