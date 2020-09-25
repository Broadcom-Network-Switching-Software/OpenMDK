/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * CDK Shell Utility: SYMOPS
 *
 * These utility functions provide all of the symbolic
 * register/memory encoding and decoding. 
 */

#include <cdk/cdk_string.h>
#include <cdk/cdk_symbols.h>

static char *
_strtolower(char *dst, const char *src, int dmax)
{
    const char *s = src; 
    char *d = dst; 
    
    while (*s && --dmax) {
        *d = *s;
        if (*s >= 'A' && *s <= 'Z') {
            *d += ('a' - 'A'); 
        }
        s++;
        d++;
    }
    *d = 0;

    return dst; 
}

static int
_sym_match(cdk_symbols_iter_t *iter, const char *sym_name)
{
    char search_name[64]; 
    char symbol_name[64]; 

    _strtolower(search_name, iter->name, sizeof(search_name)); 
    _strtolower(symbol_name, sym_name, sizeof(symbol_name));

    switch(iter->matching_mode) {
    case CDK_SYMBOLS_ITER_MODE_EXACT:
        if (CDK_STRCMP(search_name, symbol_name) == 0) {
            /* Name matches */
            return 1;
        }
        break; 
    case CDK_SYMBOLS_ITER_MODE_START:
        if (CDK_STRNCMP(symbol_name, search_name,
                        CDK_STRLEN(search_name)) == 0) {
            /* Name matches */
            return 1;
        }
        break;
    case CDK_SYMBOLS_ITER_MODE_STRSTR:
        if (CDK_STRSTR(symbol_name, search_name) != NULL) {
            /* Name matches */
            return 1;
        }
        break; 
    default:
        break;
    }
    return 0;
}

int
cdk_symbols_iter(cdk_symbols_iter_t *iter)
{       
    int count = 0; 
    int rc; 
    int match;
    uint32_t idx; 
    cdk_symbol_t s; 

    for (idx = 0; cdk_symbols_get(iter->symbols, idx, &s) >= 0; idx++) {

        if (s.name == 0) {
            /* Last */
            continue;
        }

        /* Check flags which must be present */
        if (iter->pflags && ((s.flags & iter->pflags) != iter->pflags)) {
            /* Symbol does not match */
            continue; 
        }

        /* Check flags which must be absent */
        if (iter->aflags && ((s.flags & iter->aflags) != 0)) {
            /* Symbol does not match */
            continue;
        }

        /* Check the name */
        if (CDK_STRCMP("*", iter->name) != 0) {
            /* Not wildcarded */
            match = 0;
            if (_sym_match(iter, s.name)) {
                match = 1;
            }
#if CDK_CONFIG_INCLUDE_ALIAS_NAMES == 1
            else if (s.ufname && _sym_match(iter, s.ufname)) {
                match = 1;
            }
            else if (s.alias && _sym_match(iter, s.alias)) {
                match = 1;
            }
#endif
            if (!match) {
                continue;
            }
        }

        /* Whew, name is okay */
        count++; 

        if ((rc = iter->function(&s, iter->vptr)) < 0) {
            return rc;
        }
    }
    
    return count; 
}
