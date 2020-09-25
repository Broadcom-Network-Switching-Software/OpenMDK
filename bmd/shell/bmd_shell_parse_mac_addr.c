/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Parse zero-terminated ASCII MAC address string into a bmd_mac_addr_t.
 * MAC address must be given as xx:xx:xx:xx:xx:xx.
 */

#include "bmd_shell_util.h"

int 
bmd_shell_parse_mac_addr(char *str, bmd_mac_addr_t *mac_addr)
{
    int rv = CDK_SHELL_CMD_OK;
    int midx;
    char *bstr = str;

    for (midx = 0; midx < 6; midx++) {
        mac_addr->b[midx] = (uint8_t)CDK_STRTOL(bstr, &bstr, 16);
        if (!((midx == 5 && *bstr == 0) || *bstr == ':')) {
            rv = CDK_SHELL_CMD_BAD_ARG;
            break;
        }
        bstr++;
    }
    return rv;
}
