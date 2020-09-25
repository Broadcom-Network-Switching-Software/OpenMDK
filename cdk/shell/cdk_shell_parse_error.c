/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <cdk/cdk_printf.h>
#include <cdk/cdk_shell.h>

/*
 * Function:
 *	cdk_shell_parse_error
 * Purpose:
 *	Display error message for command line argument
 * Parameters:
 *	desc - name of command line argument
 *	arg - specified value of argument (or NULL if none)
 * Returns:
 *      Always -1
 */
int 
cdk_shell_parse_error(const char *desc, const char *arg)
{
    if (arg) {
	CDK_PRINTF("%smalformed %s argument '%s'\n", 
                   CDK_CONFIG_SHELL_ERROR_STR, desc, arg); 
    }
    else {
	CDK_PRINTF("%smissing %s argument\n", 
                   CDK_CONFIG_SHELL_ERROR_STR, desc); 
    }
    return -1; 
}
