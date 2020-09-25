/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * CDK shell error handler
 */

#include <cdk/cdk_shell.h>
#include <cdk/cdk_error.h>
#include <cdk/cdk_debug.h>

int 
cdk_shell_error(int cdk_rv)
{
    int rv = CDK_SHELL_CMD_ERROR;

    if (CDK_SUCCESS(cdk_rv)) {
        rv = CDK_SHELL_CMD_OK;
    } else {
        CDK_ERR((CDK_ERRMSG(cdk_rv)));
        CDK_ERR(("\n"));
    }
    return rv;
}
