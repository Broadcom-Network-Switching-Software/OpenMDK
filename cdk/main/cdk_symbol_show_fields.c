/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <cdk/cdk_chip.h>
#include <cdk/cdk_stdlib.h>
#include <cdk/cdk_string.h>
#include <cdk/cdk_printf.h>
#include <cdk/cdk_symbols.h>
#include <cdk/cdk_field.h>
#include <cdk/cdk_util.h>

#if CDK_CONFIG_INCLUDE_FIELD_INFO == 1

/*
 * Function:
 *	cdk_symbol_show_fields
 * Purpose:
 *	Display all fields of a register/memory
 * Parameters:
 *	symbol - symbol information
 *	fnames - list of all field names
 *	data - symbol data (one or more 32-bit words)
 *	nz - skip fields with value of zero
 *	fcb - filter call-back function
 *	cookie - filter call-back context
 * Returns:
 *      Always 0
 */
int 
cdk_symbol_show_fields(const cdk_symbol_t *symbol,
                       const char **fnames, uint32_t *data, int nz,
                       cdk_symbol_filter_cb_t fcb, void *cookie)
{
    int mval; 
    cdk_field_info_t finfo; 
    char *ptr;
    char vstr[8 * CDK_MAX_REG_WSIZE + 32];
    char fname_str[16];
    const char *fname;
    uint32_t val[CDK_MAX_REG_WSIZE];
    int idx = 0;
    int size;

    CDK_SYMBOL_FIELDS_ITER_BEGIN(symbol->fields, finfo, fnames) {

        /* Create default field name */
        CDK_SPRINTF(fname_str, "field%d", idx++);
        fname = fname_str;

#if CDK_CONFIG_INCLUDE_FIELD_NAMES == 1
        /* Use real field name when available */
        if (finfo.name) {
            if (fcb && fcb(symbol, fnames, finfo.name, cookie) != 0) {
                continue;
            }
            /* Skip encoding part of field name if present */
            if ((fname = CDK_STRCHR(finfo.name, '}')) != NULL) {
                fname++;
            } else {
                fname = finfo.name;
            }
        }
#endif

        /* Create bit range string */
        cdk_util_bit_range(vstr, sizeof(vstr),
                           finfo.minbit, 
                           finfo.maxbit);

        if (data) {
            /* Get normalized field value */
            CDK_MEMSET(val, 0, sizeof(val));
            if (symbol->flags & CDK_SYMBOL_FLAG_BIG_ENDIAN) {
                size = CDK_SYMBOL_INDEX_SIZE_GET(symbol->index);
                cdk_field_be_get(data, CDK_BYTES2WORDS(size),
                                 finfo.minbit, finfo.maxbit, val);
            } else {
                cdk_field_get(data, finfo.minbit, finfo.maxbit, val);
            }

            /* Create field value string */
            mval = COUNTOF(val) - 1;
            while (mval && val[mval] == 0) {
                mval--;
            }

            /* Optionally skip fields with all zeros */
            if (nz && mval == 0 && val[mval] == 0) {
                continue;
            }

            ptr = vstr + CDK_STRLEN(vstr);
            CDK_SPRINTF(ptr, "=0x%"PRIx32"", val[mval]);
            while (mval--) {
                ptr += CDK_STRLEN(ptr);
                CDK_SPRINTF(ptr, "%08"PRIx32"", val[mval]);
            }
        }

        CDK_PRINTF("\t%s%s\n", fname, vstr);

    } CDK_SYMBOL_FIELDS_ITER_END(); 

    return 0; 
}

#endif /* CDK_CONFIG_INCLUDE_FIELD_INFO */
