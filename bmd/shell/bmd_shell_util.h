/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Private BMD shell utility functions.
 */

#include <bmd/bmd.h>

#include <bmd/shell/util.h>

#include <cdk/cdk_shell.h>
#include <cdk/cdk_device.h>
#include <cdk/cdk_string.h>
#include <cdk/cdk_stdlib.h>
#include <cdk/cdk_chip.h>
#include <cdk/cdk_error.h>
#include <cdk/cdk_debug.h>

#if BMD_CONFIG_INCLUDE_PHY == 1

#include <phy/phy_reg.h>

extern int
bmd_shell_phy_sym(phy_ctrl_t *pc, int argc, char *argv[]);

#endif
