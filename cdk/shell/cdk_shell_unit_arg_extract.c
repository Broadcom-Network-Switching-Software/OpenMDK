/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <cdk/cdk_shell.h>
#include <cdk/cdk_assert.h>
#include <cdk/cdk_string.h>
#include <cdk/cdk_stdlib.h>

/*
 * Function:
 *	cdk_shell_unit_arg_extract
 * Purpose:
 *	Extract unit number from end of argument list.
 * Parameters:
 *	argc - pointer to current number of arguments
 *      argv - list of arguments
 * Returns:
 *      Unit number or -1 if no unit specified.
 * Notes:
 *      This function will optionally strip the unit argument
 *      from the list and update argc accordingly.
 */
int
cdk_shell_unit_arg_extract(int *argc, char *argv[], int strip)
{
    int idx, udx, unit = cdk_shell_unit_get();
    char *ptr;

    CDK_ASSERT(argc);

    for (idx = 0; idx < *argc; idx++) {
        udx = idx;
        if ((ptr = cdk_shell_opt_val(*argc, argv, "unit", &idx)) != NULL ||
            (ptr = cdk_shell_opt_val(*argc, argv, "u", &idx)) != NULL) {
            unit = CDK_ATOI(ptr);
            if (strip) {
                *argc = udx;
            }
        }
    }
    return unit;
}
