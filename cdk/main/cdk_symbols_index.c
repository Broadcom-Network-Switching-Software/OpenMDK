/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

/*******************************************************************************
 *
 * CDK Symbol Routines
 */

#include <cdk/cdk_symbols.h>

int
cdk_symbols_index(const cdk_symbols_t *symbols, const cdk_symbol_t *symbol)
{
    int size, i; 

    if (symbols == NULL || symbols->symbols == NULL) {
        return -1;
    }

    size = symbols->size;
    for (i = 0; i < size; i++) {
        /* Sufficient to compare just the pointers */
        if (symbol->name == symbols->symbols[i].name) {
            return i;
        }
    }
    return -1;
}
