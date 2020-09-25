/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <cdk/cdk_chip.h>
#include <cdk/cdk_string.h>
#include <cdk/cdk_symbols.h>
#include <cdk/cdk_field.h>
#include <cdk/cdk_shell.h>

/*
 * Function:
 *	cdk_shell_encode_fields_from_tokens
 * Purpose:
 *	Create masks for modifying register/memory
 * Parameters:
 *	symbol - symbol information
 *      csts - array of tokens with field assignments
 *      and_masks - (OUT) data mask to be AND'ed
 *      or_masks - (OUT) data mask to be OR'ed
 *      max - size of masks arrays
 * Returns:
 *      0 if tokens are parsed and encoded successfully
 * Notes:
 *      The output and_masks and or masks should be applied to the
 *      current contents of a register/memory in order to modify the
 *      contents according to the specified field assignments. 
 */
int
cdk_shell_encode_fields_from_tokens(const cdk_symbol_t *symbol, 
                                    const char** fnames, 
                                    const cdk_shell_tokens_t *csts, 
                                    uint32_t *and_masks,
                                    uint32_t *or_masks,
                                    int max)
{
    const cdk_shell_tokens_t *cst = csts;
    uint32_t data; 
    int idx; 

    /* Initialize masks */
    CDK_MEMSET(and_masks, ~0, max * sizeof(*and_masks));
    CDK_MEMSET(or_masks, 0, max * sizeof(*or_masks));

    if (cst->argc == 1 && CDK_STRCMP(cst->argv[0], "all") == 0) {
        /* All 32-bit data words are assigned the same value */
        if (cdk_shell_parse_uint32(cst->argv[1], &data) < 0) {
            return cdk_shell_parse_error("field", cst->str); 
        }
        CDK_MEMSET(and_masks, 0, max * sizeof(*and_masks));
        CDK_MEMSET(or_masks, data, max * sizeof(*or_masks));
    }
    else if (cst->argc > 0 && cdk_shell_parse_is_int(cst->argv[0])) {
        /* All tokens are treated as 32-bit data words */
        for (idx = 0; cst->argc; idx++, cst++) {
            if (cdk_shell_parse_uint32(cst->argv[0], &or_masks[idx]) < 0) {
                return cdk_shell_parse_error("field", cst->str); 
            }
            and_masks[idx] = 0; 
        }
    }
    else {
        /* All tokens are treated as field=value */
        for (idx = 0; cst->argc; idx++, cst++) {
            if (cdk_shell_encode_field(symbol, fnames, cst->argv[0], cst->argv[1],
                                       and_masks, or_masks)) {
                return cdk_shell_parse_error("field", cst->str); 
            }
        }
    }

    return 0; 
}       
