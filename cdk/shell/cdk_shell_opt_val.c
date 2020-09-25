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
 *	cdk_shell_opt_val
 * Purpose:
 *	Extract value from commandline option.
 * Parameters:
 *	argc - pointer to current number of arguments
 *      argv - list of arguments
 *      name - option name
 *      idx - current index in argv list
 * Returns:
 *      Pointer to value string or NULL if name is not matched.
 * Notes:
 *      In order to accomodate different commandline parsers,
 *      this function will accept values in two formats:
 *
 *      argv[*idx]   :  "option=value"
 *
 *         - or -
 *
 *      argv[*idx]   :  "option"
 *      argv[*idx+1] :  "="
 *      argv[*idx+2] :  "value"
 */
char *
cdk_shell_opt_val(int argc, char *argv[], const char *name, int *idx)
{
    int i = *idx;
    int len = CDK_STRLEN(name);

    if (CDK_MEMCMP(argv[i], name, len) == 0) {
        if ((i+2) < argc && CDK_STRCMP(argv[i+1], "=") == 0) {
            *idx += 2;
            return argv[*idx];
        }
        if (argv[i][len] == '=') {
            return &argv[i][len+1];
        }
    }
    return 0;
}
