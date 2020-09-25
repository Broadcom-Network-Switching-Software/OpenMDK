/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <cdk/arch/xgs_shell.h>

#if CDK_CONFIG_INCLUDE_FIELD_INFO == 1

/*
 * Function:
 *	cdk_xgs_shell_show_fields
 * Purpose:
 *	Display fields of a register/memory
 * Parameters:
 *	unit - CDK unit number
 *	symbol - symbol information
 *	data - symbol data (one or more 32-bit words)
 *	skip_zeros - skip fields with value of zero
 * Returns:
 *      Always 0
 */
int 
cdk_xgs_shell_show_fields(int unit, const cdk_symbol_t *symbol,
                          uint32_t *data, int skip_zeros)
{
    return cdk_symbol_show_fields(symbol, CDK_XGS_SYMBOLS(unit)->field_names,
                                  data, skip_zeros,
                                  cdk_symbol_field_filter, data);
}

#endif /* CDK_CONFIG_INCLUDE_FIELD_INFO */
