/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Public BMD shell utility functions.
 */

#include <bmd/bmd.h>

#include <cdk/cdk_chip.h>

extern int
bmd_shell_parse_mac_addr(char *str, bmd_mac_addr_t *mac_addr);

extern int
bmd_shell_parse_port_str(int unit, const char *str, cdk_pbmp_t *pbmp);
