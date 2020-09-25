/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * XGS shell symbol parsing functions.
 */

#include <cdk/cdk_string.h>
#include <cdk/cdk_stdlib.h>
#include <cdk/cdk_chip.h>
#include <cdk/cdk_symbols.h>
#include <cdk/cdk_field.h>
#include <cdk/cdk_shell.h>

int
cdk_shell_encode_field(const cdk_symbol_t *symbol, 
                       const char** fnames, 
                       const char *field, const char *value, 
                       uint32_t *and_masks, uint32_t *or_masks)
{
#if CDK_CONFIG_INCLUDE_FIELD_NAMES == 1
    int v, len, wsize; 
    cdk_field_info_t finfo; 
    char vstr[8 * CDK_MAX_REG_WSIZE + 32];
    uint32_t val[CDK_MAX_REG_WSIZE]; 


    CDK_SYMBOL_FIELDS_ITER_BEGIN(symbol->fields, finfo, fnames) {

        if (CDK_STRCASECMP(finfo.name, field)) {
            continue; 
        }

        if (!cdk_shell_parse_is_int(value)) {
            return -1;
        }

        CDK_STRLCPY(vstr, value, sizeof(vstr));

        CDK_MEMSET(val, 0, sizeof(val));
        if (symbol->flags & CDK_SYMBOL_FLAG_BIG_ENDIAN) {
            wsize = CDK_BYTES2WORDS(CDK_SYMBOL_INDEX_SIZE_GET(symbol->index));
            cdk_field_be_set(and_masks, wsize, finfo.minbit, finfo.maxbit, val);
        } else {
            cdk_field_set(and_masks, finfo.minbit, finfo.maxbit, val);
        }

        /*
         * If the field value starts with 0x the accept values
         * spanning multiple words, e.g. 0x112233445566.
         */
        v = 0;
        if (vstr[0] == '0' && (vstr[1] == 'x' || vstr[1] == 'X')) {
            while ((len = CDK_STRLEN(vstr)) > 10) {
                len -= 8;
                val[v++] = CDK_STRTOUL(&vstr[len], NULL, 16);
                vstr[len] = 0;
            }
        }
        if (cdk_shell_parse_uint32(vstr, &val[v]) < 0) {
            return -1;
        }
        if (symbol->flags & CDK_SYMBOL_FLAG_BIG_ENDIAN) {
            wsize = CDK_BYTES2WORDS(CDK_SYMBOL_INDEX_SIZE_GET(symbol->index));
            cdk_field_be_set(or_masks, wsize, finfo.minbit, finfo.maxbit, val);
        } else {
            cdk_field_set(or_masks, finfo.minbit, finfo.maxbit, val);
        }

        return 0;
    } CDK_SYMBOL_FIELDS_ITER_END(); 

#endif /* CDK_CONFIG_INCLUDE_FIELD_NAMES */

    return -1; 
}
