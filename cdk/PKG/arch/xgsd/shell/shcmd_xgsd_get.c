/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * XGSD shell command GET
 */

#include <cdk/arch/xgsd_shell.h>

#include <cdk/arch/shcmd_xgsd_get.h>

#if CDK_CONFIG_SHELL_INCLUDE_GET == 1

#if CDK_CONFIG_INCLUDE_CHIP_SYMBOLS == 1

/*******************************************************************************
 *
 * Private data for symbol iterator.
 */

typedef struct xgsd_iter_s {
    int unit; 
    cdk_shell_id_t *sid;
} xgsd_iter_t; 

/*******************************************************************************
 *
 * Get or Set the data for a symbol -- symbol iterator function
 */

static int
_iter_op(const cdk_symbol_t *symbol, void *vptr)
{
    xgsd_iter_t *xgsd_iter = (xgsd_iter_t *)vptr;
    cdk_shell_id_t sid;
    int unit = xgsd_iter->unit;

    CDK_MEMCPY(&sid, xgsd_iter->sid, sizeof(sid)); 

    /* Copy the address in for this symbol */
    CDK_STRCPY(sid.addr.name, symbol->name); 
    sid.addr.name32 = symbol->addr;

    cdk_xgsd_shell_symop(unit, symbol, &sid, NULL, NULL);

    return 0; 
}

#endif /* CDK_CONFIG_INCLUDE_CHIP_SYMBOLS */

int
cdk_shcmd_xgsd_get(int argc, char *argv[])
{       
#if CDK_CONFIG_INCLUDE_CHIP_SYMBOLS == 0
    return CDK_SHELL_CMD_NO_SYM; 
#else
    int i; 
    int unit;
    const cdk_symbols_t *symbols;
    cdk_shell_id_t sid; 
    cdk_symbols_iter_t iter; 
    xgsd_iter_t xgsd_iter; 
    cdk_shell_tokens_t csts[4]; 
    uint32_t sid_flags = 0;

    unit = cdk_shell_unit_arg_extract(&argc, argv, 1);
    if(!CDK_DEV_EXISTS(unit)) {
        return CDK_SHELL_CMD_BAD_ARG;
    }
    symbols = CDK_XGSD_SYMBOLS(unit);

    CDK_SHELL_CMD_REQUIRE_SYMBOLS(symbols); 
    CDK_SHELL_CMD_ARGCHECK(1, COUNTOF(csts)); 
    
    /* Parse all of our input arguments for options */
    i = 0; 
    if ((argc == 0) || 
        ((i = cdk_xgsd_shell_parse_args(argc, argv, csts, COUNTOF(csts))) >= 0)) {
        /* Error in argument i */
        return cdk_shell_parse_error("symbol", argv[i]); 
    }

    CDK_MEMSET(&iter, 0, sizeof(iter)); 
    CDK_MEMSET(&xgsd_iter, 0, sizeof(xgsd_iter)); 

    /* Match any symbol by default */
    cdk_shell_parse_id("*", &sid, 0); 

    /* Look through our arguments */
    for (i = 0; i < argc; i++) {
        /* Flags specified? */
        if (CDK_STRCMP("nz", csts[i].argv[0]) == 0) {
            sid_flags |= CDK_SHELL_IDF_NONZERO;
        }
        else if (CDK_STRCMP("raw", csts[i].argv[0]) == 0) {
            sid_flags |= CDK_SHELL_IDF_RAW;
        }
        else if (CDK_STRCMP("flags", csts[i].argv[0]) == 0) {
            cdk_xgsd_shell_symflag_cst2flags(unit, &csts[i],
                                            &iter.pflags, &iter.aflags);
        }
        else {
            /* Crack the identifier */
            if (cdk_shell_parse_id(csts[i].argv[0], &sid, 0) < 0) {
                return cdk_shell_parse_error("identifier", *argv); 
            }
        }
    }   
    sid.flags = sid_flags;

    xgsd_iter.unit = unit; 
    xgsd_iter.sid = &sid; 

    iter.name = sid.addr.name; 
    iter.matching_mode = CDK_SYMBOLS_ITER_MODE_EXACT; 
    iter.symbols = symbols; 
    iter.function = _iter_op; 
    iter.vptr = &xgsd_iter; 

    /* Iterate */
    if (cdk_symbols_iter(&iter) <= 0) {
        CDK_PRINTF("no matching symbols\n"); 
    }

    return 0;
#endif /* CDK_CONFIG_INCLUDE_CHIP_SYMBOLS */
}

#endif /* CDK_CONFIG_SHELL_INCLUDE_GET */
