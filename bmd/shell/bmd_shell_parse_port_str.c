/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Parse port string into port bitmap.
 */

#include "bmd_shell_util.h"

int bmd_shell_parse_port_str(int unit, const char *str, cdk_pbmp_t *pbmp)
{
    int rv = 0;
    int port = 0;
    int pstart = -1;
    int ptmp;
    char ch;
    const char *cptr = str;;

    if (CDK_STRCMP(str, "all") == 0) {
        bmd_port_type_pbmp(unit, BMD_PORT_ALL, pbmp);
        return rv;
    }
    CDK_PBMP_CLEAR(*pbmp);
    do {
        ch = *cptr++;
        if (ch >= '0' && ch <= '9') {
            port = (port * 10) + (ch - '0');
        } else {
            if (pstart >= 0) {
                while (pstart < port) {
                    ptmp = CDK_PORT_MAP_L2P(unit, pstart++);
                    if (ptmp >= 0) {
                        CDK_PBMP_ADD(*pbmp, ptmp);
                    }
                }
                pstart = -1;
            }
            if (ch == ',' || ch == 0) {
                ptmp = CDK_PORT_MAP_L2P(unit, port);
                if (ptmp >= 0) {
                    CDK_PBMP_ADD(*pbmp, ptmp);
                }
                port = 0;
            } else if (ch == '-') {
                pstart = port;
                port = 0;
            } else {
                rv = -1;
                break;
            }
        }
    } while (ch != 0);

    return rv;
}
