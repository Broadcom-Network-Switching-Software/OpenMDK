/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <cdk/cdk_chip.h>
#include <cdk/cdk_string.h>
#include <cdk/cdk_printf.h>
#include <cdk/cdk_symbols.h>
#include <cdk/cdk_field.h>
#include <cdk/cdk_shell.h>

#if CDK_CONFIG_INCLUDE_FIELD_INFO == 1

/*
 * Function:
 *	cdk_shell_list_fields
 * Purpose:
 *	Display all fields of a register/memory
 * Parameters:
 *	symbol - symbol information
 *	fnames - list of all field names for this device
 * Returns:
 *      Always 0
 */
int 
cdk_shell_list_fields(const cdk_symbol_t *symbol, const char **fnames)
{
    return cdk_symbol_show_fields(symbol, fnames, NULL, 0, NULL, NULL);
}

#endif /* CDK_CONFIG_INCLUDE_FIELD_INFO */
