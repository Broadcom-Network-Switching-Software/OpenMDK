/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <cdk/cdk_string.h>
#include <cdk/cdk_shell.h>

/*
 * Function:
 *	cdk_shell_option_filter
 * Purpose:
 *	Filter options package option by name and/or value.
 * Parameters:
 *	argc - argument count
 *	argv - argument vector
 *	fstr - filter string
 *	fval - filter value
 * Returns:
 *      Always 0
 */
int
cdk_shell_option_filter(int argc, char *argv[], const char *fstr, uint32_t fval)
{
    int ax;
    char *arg;

    for (ax = 0; ax < argc; ax++) {
        arg = argv[ax];
        if (CDK_STRCMP(arg, "1") == 0) {
            if (fval == 0) {
                return -1; 
            }
            continue;
        }
        if (CDK_STRCMP(arg, "0") == 0) {
            if (fval != 0) {
                return -1; 
            }
            continue;
        }
        if (arg[0] == '!') {
            if (CDK_STRSTR(fstr, &arg[1]) != NULL) {
                return -1; 
            }
        } else {
            if (CDK_STRSTR(fstr, arg) == NULL) {
                return -1; 
            }
        }
    }

    /* Value passes all filters */
    return 0;
}
