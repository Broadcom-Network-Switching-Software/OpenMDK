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
#include <cdk/cdk_string.h>
#include <cdk/cdk_assert.h>

#if CDK_CONFIG_INCLUDE_CHIP_SYMBOLS == 1

static const void *
__cdk_symbol_find(const char *name, const void *table, int size, int entry_size)
{
    int i; 
    cdk_symbol_t *sym;
    unsigned char *ptr = (unsigned char*)table; 

    CDK_ASSERT(table); 
    
    for (i = 0; (sym = (cdk_symbol_t*)(ptr)) && (i < size); i++) {
	if (CDK_STRCMP(sym->name, name) == 0) {
	    return (void*) sym; 
	}
#if CDK_CONFIG_INCLUDE_ALIAS_NAMES == 1
	if (sym->ufname && CDK_STRCMP(sym->ufname, name) == 0) {
	    return (void*) sym; 
	}
	if (sym->alias && CDK_STRCMP(sym->alias, name) == 0) {
	    return (void*) sym; 
	}
#endif
	ptr += entry_size; 
    }

    return NULL; 
}
	

const cdk_symbol_t *
cdk_symbol_find(const char *name, const cdk_symbol_t *table, int size)
{
    return (cdk_symbol_t*) __cdk_symbol_find(name, table, size, sizeof(cdk_symbol_t)); 
}


int 
cdk_symbols_find(const char *name, const cdk_symbols_t *symbols, cdk_symbol_t *rsym)
{
    const cdk_symbol_t *s = NULL; 

    if (rsym == NULL) return -1; 

    if ((symbols->symbols) && (s = cdk_symbol_find(name, symbols->symbols, symbols->size))) {
	*rsym = *s;
	return 0; 
    }
    return -1;
}

#endif
