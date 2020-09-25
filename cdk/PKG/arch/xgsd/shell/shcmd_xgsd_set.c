/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * XGSD shell command SET
 */

#include <cdk/arch/xgsd_shell.h>

#include <cdk/arch/shcmd_xgsd_set.h>

#if CDK_CONFIG_SHELL_INCLUDE_SET == 1

#if CDK_CONFIG_INCLUDE_CHIP_SYMBOLS == 1

/*******************************************************************************
 *
 * Private data for symbol iterator.
 */

typedef struct xgsd_iter_s {
    int unit; 
    cdk_shell_id_t *sid;
    cdk_shell_tokens_t *csts;
} xgsd_iter_t; 

/*******************************************************************************
 *
 * Get or Set the data for a symbol -- symbol iterator function
 */

static int
_iter_op(const cdk_symbol_t *symbol, void *vptr)
{
    uint32_t and_masks[CDK_MAX_REG_WSIZE];
    uint32_t or_masks[CDK_MAX_REG_WSIZE];
    xgsd_iter_t *xgsd_iter = (xgsd_iter_t *)vptr;
    cdk_shell_id_t sid;
    cdk_shell_tokens_t *csts = xgsd_iter->csts; 
    const char **fnames = CDK_XGSD_SYMBOLS(xgsd_iter->unit)->field_names;
    int unit = xgsd_iter->unit;

    CDK_MEMCPY(&sid, xgsd_iter->sid, sizeof(sid)); 

    /* Copy the address in for this symbol */
    CDK_STRCPY(sid.addr.name, symbol->name); 
    sid.addr.name32 = symbol->addr;

    /* These CSTs contain the data and/or field assignments */
    cdk_shell_encode_fields_from_tokens(symbol, fnames, csts,
                                        and_masks, or_masks, CDK_MAX_REG_WSIZE);

    cdk_xgsd_shell_symop(unit, symbol, &sid, and_masks, or_masks);

    return 0; 
}

#endif /* CDK_CONFIG_INCLUDE_CHIP_SYMBOLS */

int
cdk_shcmd_xgsd_set(int argc, char *argv[])
{
#if CDK_CONFIG_INCLUDE_CHIP_SYMBOLS == 0
    return CDK_SHELL_CMD_NO_SYM; 
#else
    int i; 
    int data_start = 0; 
    int trex = 0;
    int unit;
    const cdk_symbols_t *symbols;
    cdk_shell_id_t sid;
    cdk_symbols_iter_t iter; 
    xgsd_iter_t xgsd_iter; 
    cdk_shell_tokens_t csts[CDK_CONFIG_SHELL_MAX_ARGS]; 
    uint32_t sid_flags = 0;

    unit = cdk_shell_unit_arg_extract(&argc, argv, 1);
    if(!CDK_DEV_EXISTS(unit)) {
        return CDK_SHELL_CMD_BAD_ARG;
    }
    symbols = CDK_XGSD_SYMBOLS(unit);

    CDK_SHELL_CMD_REQUIRE_SYMBOLS(symbols); 
    CDK_SHELL_CMD_ARGCHECK(2, CDK_CONFIG_SHELL_MAX_ARGS); 

    CDK_MEMSET(&iter, 0, sizeof(iter)); 
    CDK_MEMSET(&xgsd_iter, 0, sizeof(xgsd_iter)); 

    /*
     * The format of this command must be one of the following:
     *
     * set <symbol> [flags=] <[data words] or [field assignments]>
     * set <flags=> <[data words] or [field assignments]>
     */
    
    /* Parse input arguments */
    if ((i = cdk_xgsd_shell_parse_args(argc, argv, csts, COUNTOF(csts))) >= 0) {
        /* Error in argument i */
        return cdk_shell_parse_error("", argv[i]); 
    }
           
    /* Match any symbol by default */
    cdk_shell_parse_id("*", &sid, 0); 

    /* Look through our arguments */
    for (i = 0; i < argc; i++) {
        /* Flags specified? */
        if (CDK_STRCMP("flags", csts[i].argv[0]) == 0) {
            cdk_xgsd_shell_symflag_cst2flags(unit, &csts[i],
                                            &iter.pflags, &iter.aflags); 
        }
        else {
            /* Field data value specified */
            if (csts[i].argc > 1) {
                data_start = i;
                break;
            }
            /* Raw data value specified */
            if (cdk_shell_parse_is_int(csts[i].argv[0])) {
                data_start = i;
                break;
            }
            /* Crack the identifier */
            if (cdk_shell_parse_id(csts[i].argv[0], &sid, 0) < 0) {
                return cdk_shell_parse_error("symbol", csts[i].argv[0]); 
            }
            data_start = i + 1;
        }
    }

    if (data_start == 0) {
        return cdk_shell_parse_error("data", NULL); 
    }

    /* Optionally enable TREX_DEBUG in S-channel register write */
    CDK_XGSD_CHIP_TREX_SET(unit, trex);

    sid.flags = sid_flags;

    xgsd_iter.unit = unit; 
    xgsd_iter.csts = &csts[data_start]; 
    xgsd_iter.sid = &sid; 

    iter.name = sid.addr.name; 
    iter.symbols = symbols; 
    iter.function = _iter_op; 
    iter.vptr = &xgsd_iter; 

    /* Iterate */
    if (cdk_symbols_iter(&iter) <= 0) {
        CDK_PRINTF("no matching symbols\n"); 
    }

    /* Clear TREX_DEBUG mode */
    CDK_XGSD_CHIP_TREX_SET(unit, 0);

    return 0;     
#endif /* CDK_CONFIG_INCLUDE_CHIP_SYMBOLS */
}

#endif /* CDK_CONFIG_SHELL_INCLUDE_SET */
