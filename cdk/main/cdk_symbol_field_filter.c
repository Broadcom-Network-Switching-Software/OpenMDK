/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <cdk/cdk_string.h>
#include <cdk/cdk_stdlib.h>
#include <cdk/cdk_field.h>
#include <cdk/cdk_symbols.h>
#include <cdk/cdk_chip.h>

/*
 * Function:
 *	cdk_symbol_field_filter
 * Purpose:
 *	Callback for Filtering fields based on current data view.
 * Parameters:
 *	symbol - symbol information
 *	fnames - list of all field names for this device
 *	encoding - key for decoding overlay
 *	cookie - context data
 * Returns:
 *      Non-zero if field should be filtered out (not displayed)
 * Notes:
 *      The filter key has the following syntax:
 *
 *        {[<keysrc>]:<keyfield>:<keyval>[|<keyval> ... ]}
 *
 *      Ideally the keysrc is the same data entry which is
 *      being decoded, and in this case it can left out, e.g.:
 *
 *        {:KEY_TYPEf:1}
 *
 *      This example encoding means that if KEY_TYPEf=1, then
 *      this field is valid for this view.
 *
 *      Note that a filed can be for multiple views, e.g.:
 *
 *        {:KEY_TYPEf:1|3}
 *
 *      This example encoding means that this field is valid
 *      if KEY_TYPEf=1 or KEY_TYPEf=3.
 *
 *      The special <keyval>=-1 means that this field is valid
 *      even if there is no context (cookie=NULL).
 *
 *      Note that this filter code does NOT support a <keysrc>
 *      which is different from the current data entry.
 */
int 
cdk_symbol_field_filter(const cdk_symbol_t *symbol, const char **fnames,
                              const char *encoding, void *cookie)
{
#if CDK_CONFIG_INCLUDE_FIELD_NAMES == 1
    uint32_t *data = (uint32_t *)cookie;
    uint32_t val[CDK_MAX_REG_WSIZE];
    cdk_field_info_t finfo; 
    char tstr[128];
    char *keyfield, *keyvals;
    char *ptr;
    int wsize;
    int kval = -1;

    /* Do not filter if no (or unknown) encoding */
    if (encoding == NULL || *encoding != '{') {
        return 0;
    }

    /* Do not filter if encoding cannot be parsed */
    CDK_STRLCPY(tstr, encoding, sizeof(tstr));
    ptr = tstr;
    if ((ptr = CDK_STRCHR(ptr, ':')) == NULL) {
        return 0;
    }
    *ptr++ = 0;
    keyfield = ptr;
    if ((ptr = CDK_STRCHR(ptr, ':')) == NULL) {
        return 0;
    }
    *ptr++ = 0;
    keyvals = ptr;

    /* Only show default view if no context */
    if (data == NULL) {
        return (CDK_STRSTR(keyvals, "-1") == NULL) ? 1 : 0;
    }

    /* Look for <keyfield> in data entry */
    CDK_SYMBOL_FIELDS_ITER_BEGIN(symbol->fields, finfo, fnames) {

        if (finfo.name && CDK_STRCMP(finfo.name, keyfield) == 0) {
            /* Get normalized field value */
            CDK_MEMSET(val, 0, sizeof(val));
            if (symbol->flags & CDK_SYMBOL_FLAG_BIG_ENDIAN) {
                wsize = CDK_BYTES2WORDS(CDK_SYMBOL_INDEX_SIZE_GET(symbol->index));
                cdk_field_be_get(data, wsize, finfo.minbit, finfo.maxbit, val);
            } else {
                cdk_field_get(data, finfo.minbit, finfo.maxbit, val);
            }
            kval = val[0];
            break;
        }

    } CDK_SYMBOL_FIELDS_ITER_END(); 

    /* Check if current key matches any <keyval> in encoding */
    ptr = keyvals;
    while (ptr) {
        if (CDK_ATOI(ptr) == kval) {
            return 0;
        }
        if ((ptr = CDK_STRCHR(ptr, '|')) != NULL) {
            ptr++;
        }
    }
#endif

    /* No match - filter this field */
    return 1; 
}
