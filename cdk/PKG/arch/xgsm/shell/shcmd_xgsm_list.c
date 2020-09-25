/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * XGSM shell command LIST
 */

#include <cdk/arch/xgsm_shell.h>

#include <cdk/arch/shcmd_xgsm_list.h>

#if CDK_CONFIG_SHELL_INCLUDE_LIST == 1

#if CDK_CONFIG_INCLUDE_CHIP_SYMBOLS == 1

typedef struct sym_info_s {    
    int unit; 
    uint32_t flags;
    char *view;
} sym_info_t; 


#if CDK_CONFIG_INCLUDE_FIELD_INFO == 1

static int 
_view_filter(const cdk_symbol_t *symbol, const char **fnames,
             const char *encoding, void *cookie)
{
    char *view = (char *)cookie;
    char *ptr;

    /* Do not filter if no (or unknown) encoding */
    if (encoding == NULL || *encoding != '{' || view == NULL) {
        return 0;
    }

    /* Do not filter if encoding cannot be parsed */
    if ((ptr = CDK_STRCHR(encoding, '}')) == NULL) {
        return 0;
    }

    /* Do not filter if view (partially) matches */
    ptr++;
    if (CDK_STRCMP(view, "*") == 0 ||
        CDK_STRNCASECMP(ptr, view, CDK_STRLEN(view)) == 0) {
        return 0;
    }

    /* No match - filter this field */
    return 1; 
}

#endif

/*******************************************************************************
 *
 * Prints the information for a single symbol
 */

static int
_print_sym(const cdk_symbol_t *s, void *vptr)
{
    sym_info_t *si = (sym_info_t *)vptr;
    uint32_t mask, flags, acc_type; 
    uint32_t size, min, max, step; 
    int enum_val;
    const char *flagstr;
    
    if (si->flags & CDK_SHELL_IDF_RAW) {
        CDK_PRINTF("%s\n", s->name); 
        return 0;
    }

    size = CDK_SYMBOL_INDEX_SIZE_GET(s->index); 
    min = CDK_SYMBOL_INDEX_MIN_GET(s->index); 
    max = CDK_SYMBOL_INDEX_MAX_GET(s->index); 
    step = CDK_SYMBOL_INDEX_STEP_GET(s->index); 

    /* Extract dual-pipe access type from flags */
    flags = s->flags;
    acc_type = (flags >> 20 ) & 0x7;
    flags &= ~(0x7 << 20);

    /* Max index may depend on current chip configuration */
    if (flags & CDK_SYMBOL_FLAG_MEMORY) {
        enum_val = cdk_symbols_index(CDK_XGSM_SYMBOLS(si->unit), s);
        max = cdk_xgsm_mem_maxidx(si->unit, enum_val, max);
    }

    CDK_PRINTF("Name:     %s\n", s->name); 
    CDK_PRINTF("Offset:   0x%08"PRIx32"\n", s->addr); 
    if (acc_type) {
        CDK_PRINTF("Access:   %"PRIu32"\n", acc_type); 
    }
    if (flags & CDK_SYMBOL_FLAG_MEMORY) {
        CDK_PRINTF("Size:     %"PRIu32" bytes (%"PRIu32" words)\n", 
                   size, CDK_BYTES2WORDS(size)); 
    } else {
        CDK_PRINTF("Size:     %s-bit\n", 
                   (flags & CDK_SYMBOL_FLAG_R64) ? "64" : "32"); 
    }
    CDK_PRINTF("Flags:    "); 
    
    for (mask = 1; mask; mask <<= 1) {
        if (flags & mask) {
            flagstr = cdk_xgsm_shell_symflag_type2name(si->unit, mask);
            if (flagstr != NULL) {
                CDK_PRINTF("%s,", flagstr);
            }
        }
    }

    CDK_PRINTF("(0x%"PRIx32")\n", flags); 

    if ((flags & CDK_SYMBOL_FLAG_MEMORY) || (max - min) > 0) {
        CDK_PRINTF("Index:    %"PRIu32":%"PRIu32"", min, max); 
        if (step > 1) {
            CDK_PRINTF(" (step 0x%"PRIx32")", step); 
        }
        CDK_PRINTF("\n"); 
    }

#if CDK_CONFIG_INCLUDE_FIELD_INFO == 1
    if (s->fields) {
        cdk_symbol_filter_cb_t filter_cb;
        filter_cb = (si->view) ? _view_filter : cdk_symbol_field_filter;
        CDK_PRINTF("Fields:   %"PRIu32"\n", cdk_field_info_count(s->fields)); 
        cdk_symbol_show_fields(s, CDK_XGSM_SYMBOLS(si->unit)->field_names,
                               NULL, 0, filter_cb, si->view);
    }
#endif

    CDK_PRINTF("\n"); 
    return 0; 
}
#endif /* CDK_CONFIG_INCLUDE_CHIP_SYMBOLS */

/*******************************************************************************
 *
 * Prints out all symbols containing the input string
 */

int
cdk_shcmd_xgsm_list(int argc, char *argv[])
{
#if CDK_CONFIG_INCLUDE_CHIP_SYMBOLS == 0
    return CDK_SHELL_CMD_NO_SYM; 
#else
    int i; 
    int unit;
    const char *name = NULL; 
    const cdk_symbols_t *symbols;
    cdk_shell_tokens_t csts[3]; 
    cdk_symbols_iter_t iter; 
    sym_info_t sym_info;
    uint32_t flags = 0;

    unit = cdk_shell_unit_arg_extract(&argc, argv, 1);
    if(!CDK_DEV_EXISTS(unit)) {
        return CDK_SHELL_CMD_BAD_ARG;
    }
    symbols = CDK_XGSM_SYMBOLS(unit);

    CDK_SHELL_CMD_REQUIRE_SYMBOLS(symbols); 
    CDK_SHELL_CMD_ARGCHECK(0, COUNTOF(csts)); 

    /* Parse all of our input arguments for options */
    i = 0; 
    if ((argc == 0) || 
        ((i = cdk_xgsm_shell_parse_args(argc, argv,
                                       csts, COUNTOF(csts))) >= 0)) {
        /* Error in argument i */
        return cdk_shell_parse_error("symbol", argv[i]); 
    }

    CDK_MEMSET(&iter, 0, sizeof(iter)); 
    CDK_MEMSET(&sym_info, 0, sizeof(sym_info));

    /* Look through our arguments */
    for (i = 0; i < argc; i++) {
        /* Flags Specified? */
        if (CDK_STRCMP("flags", csts[i].argv[0]) == 0) {
            cdk_xgsm_shell_symflag_cst2flags(unit, &csts[i],
                                            &iter.pflags, &iter.aflags); 
        }
        else if (CDK_STRCMP("raw", csts[i].argv[0]) == 0) {
            flags |= CDK_SHELL_IDF_RAW;
        }
        else if (name == NULL) {
            /* Symbol expression */
            name = csts[i].argv[0]; 
        }
        else {
            /* If multi-view memory */
            sym_info.view = csts[i].argv[0]; 
        }
    }

    if (name == NULL) {
        return cdk_shell_parse_error("symbol", NULL); 
    }
        
    /* 
     * By default we list all symbols with the input name as a substring
     */
    iter.matching_mode = CDK_SYMBOLS_ITER_MODE_STRSTR; 

    /*
     * The user can specify explicitly the type of matching with the 
     * first character.
     */
    if (CDK_STRCMP(name, "*") != 0) {
        switch (name[0]) {
        case '^': 
            iter.matching_mode = CDK_SYMBOLS_ITER_MODE_START; 
            name++; 
            break;
        case '*':
            iter.matching_mode = CDK_SYMBOLS_ITER_MODE_STRSTR;
            name++;
            break; 
        case '@':
            iter.matching_mode = CDK_SYMBOLS_ITER_MODE_EXACT;
            name++;
            break;
        default: 
            break;
        }
    }

    sym_info.unit = unit; 
    sym_info.flags = flags; 

    /* Interate over all matching symbols */
    iter.name = name; 
    iter.symbols = symbols; 
    iter.function = _print_sym; 
    iter.vptr = &sym_info; 

    if (cdk_symbols_iter(&iter) <= 0) {
        CDK_PRINTF("no matching symbols\n"); 
    }
    return 0; 
#endif /* CDK_CONFIG_INCLUDE_CHIP_SYMBOLS */
}

#endif
