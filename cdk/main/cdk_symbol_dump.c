/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <cdk/cdk_chip.h>
#include <cdk/cdk_symbols.h>

#if CDK_CONFIG_INCLUDE_CHIP_SYMBOLS == 1

/*
 * Function:
 *	cdk_symbol_dump
 * Purpose:
 *	Lookup symbol name and display all fields
 * Parameters:
 *	name - symbol name
 *	symbols - symbol table to search
 *	data - symbol data (one or more 32-bit words)
 * Returns:
 *      -1 if symbol not found
 */
int 
cdk_symbol_dump(const char *name, const cdk_symbols_t *symbols, uint32_t *data)
{
    int rv;
    cdk_symbol_t symbol;

    if ((rv = cdk_symbols_find(name, symbols, &symbol)) == 0) {
        cdk_symbol_show_fields(&symbol, symbols->field_names, data,
                               0, cdk_symbol_field_filter, data);
    }

    return rv;
}

#endif /* CDK_CONFIG_INCLUDE_FIELD_INFO */
