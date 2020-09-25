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
cdk_symbols_get(const cdk_symbols_t* symbols, uint32_t sindex, cdk_symbol_t* rsym)
{
    if (symbols) {
	if (sindex < symbols->size) {
	    /* Index is within the symbol table */
	    *rsym = symbols->symbols[sindex]; 
	    return 0;
	}
    }
    return -1; 
}

