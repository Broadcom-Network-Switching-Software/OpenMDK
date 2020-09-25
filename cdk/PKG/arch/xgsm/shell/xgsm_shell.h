/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef __CDK_XGSM_SHELL_H__
#define __CDK_XGSM_SHELL_H__

/*******************************************************************************
 *
 * Various utility functions needed by different parts of the shell. 
 *
 * These are all declared her in this header, but implemented in separate, 
 * individual source files. 
 *
 * As such, if any utility function is unused in a given configuration, its 
 * code will just get dropped by the linker. 
 */

#include <cdk/cdk_assert.h>
#include <cdk/cdk_stdlib.h>
#include <cdk/cdk_string.h>
#include <cdk/cdk_printf.h>
#include <cdk/cdk_symbols.h>
#include <cdk/cdk_chip.h>
#include <cdk/cdk_shell.h>
#include <cdk/cdk_debug.h>
#include <cdk/cdk_field.h>

#include <cdk/arch/xgsm_chip.h>
#include <cdk/arch/xgsm_reg.h>
#include <cdk/arch/xgsm_mem.h>
#include <cdk/arch/xgsm_miim.h>

/*******************************************************************************
 *
 * Register and Memory Output Functions (cdk_xgsm_shell_memregs.c)
 */

extern int
cdk_xgsm_shell_regops(int unit, const cdk_symbol_t *symbol, cdk_shell_id_t *sid, 
		      uint32_t size, uint32_t *and_masks, uint32_t *or_masks); 

extern int
cdk_xgsm_shell_memops(int unit, const cdk_symbol_t *symbol, cdk_shell_id_t *sid, 
		      uint32_t size, uint32_t *and_masks, uint32_t *or_masks); 

/*******************************************************************************
 *
 * Symbolic Register and Memory operations (cdk_xgsm_shell_symops.c)
 */

extern int 
cdk_xgsm_shell_symop(int unit, const cdk_symbol_t *symbol, cdk_shell_id_t *sid, 
                    uint32_t *and_masks, uint32_t *or_masks);

/*******************************************************************************
 *
 * Block functions (cdk_xgsm_shell_blocks.c)
 */

extern const char *
cdk_xgsm_shell_block_type2name(int unit, int blktype); 

extern int
cdk_xgsm_shell_block_name2type(int unit, const char *name); 

extern char*
cdk_xgsm_shell_block_name(int unit, int block, char *dst); 

/*******************************************************************************
 *
 * Symbol Flags
 */

extern const char *
cdk_xgsm_shell_symflag_type2name(int unit, uint32_t flag); 

extern uint32_t
cdk_xgsm_shell_symflag_name2type(int unit, const char *name); 

extern int
cdk_xgsm_shell_symflag_cst2flags(int unit, const cdk_shell_tokens_t *cst,
                                uint32_t *present, uint32_t *absent); 

/*******************************************************************************
 *
 * Input parsing utilities (cdk_xgsm_shell_parse.c)
 */

extern int 
cdk_xgsm_shell_parse_args(int argc, char *argv[], cdk_shell_tokens_t *cst, int max);

/*******************************************************************************
 *
 * Field dump with XGSM filtering
 */

extern int 
cdk_xgsm_shell_show_fields(int unit, const cdk_symbol_t *symbol,
                          uint32_t *data, int skip_zeros);

/*******************************************************************************
 *
 * Shell macros for XGSM chips
 */

/* Iterate over all blocktypes */
#define CDK_SHELL_BLKTYPE_ITER(flags, blktype) \
        for(blktype = CDK_XGSM_BLOCK_START; blktype <= CDK_XGSM_BLOCK_LAST; blktype <<= 1) \
           if(flags & blktype) 


#endif /* __CDK_XGSM_SHELL_H__ */
