/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef __CDK_ROBO_SHELL_H__
#define __CDK_ROBO_SHELL_H__

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

#include <cdk/arch/robo_chip.h>
#include <cdk/arch/robo_reg.h>
#include <cdk/arch/robo_mem.h>
#include <cdk/arch/robo_miim.h>

/*******************************************************************************
 *
 * Register and Memory Output Functions (cdk_robo_shell_memregs.c)
 */

extern int
cdk_robo_shell_regops(int unit, const cdk_symbol_t *symbol, cdk_shell_id_t *sid, 
		      uint32_t size, uint32_t *and_masks, uint32_t *or_masks); 

extern int
cdk_robo_shell_memops(int unit, const cdk_symbol_t *symbol, cdk_shell_id_t *sid, 
		      uint32_t size, uint32_t *and_masks, uint32_t *or_masks); 

/*******************************************************************************
 *
 * Symbolic Register and Memory operations (cdk_robo_shell_symops.c)
 */

extern int 
cdk_robo_shell_symop(int unit, const cdk_symbol_t *symbol, cdk_shell_id_t *sid, 
                    uint32_t *and_masks, uint32_t *or_masks);

/*******************************************************************************
 *
 * Symbol Flags
 */

extern const char *
cdk_shell_symflag_type2name(uint32_t flag); 

extern uint32_t
cdk_shell_symflag_name2type(const char *name); 

extern int
cdk_robo_shell_symflag_cst2flags(const cdk_shell_tokens_t *cst, uint32_t *present, uint32_t *absent); 

/*******************************************************************************
 *
 * Input parsing utilities (cdk_robo_shell_parse.c)
 */

extern int 
cdk_robo_shell_parse_args(int argc, char *argv[], cdk_shell_tokens_t *cst, int max);

/*******************************************************************************
 *
 * Shell macros for ROBO chips
 */

/* Iterate over all blocktypes */
#define CDK_ROBO_SHELL_BLKTYPE_ITER(flags, blktype) \
        for (blktype = CDK_ROBO_BLOCK_START; blktype <= CDK_ROBO_BLOCK_LAST; blktype <<= 1) \
           if (flags & blktype) 


#endif /* __CDK_ROBO_SHELL_H__ */
