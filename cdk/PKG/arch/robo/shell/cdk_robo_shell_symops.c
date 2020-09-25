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

#include <cdk/arch/robo_shell.h>

/*******************************************************************************
 *
 * Get or set a chip register via S-channel
 */

static int
_reg_symop(int unit, const cdk_symbol_t *symbol, cdk_shell_id_t *sid, 
           uint32_t *and_masks, uint32_t *or_masks)
{
    int port; 
    int maxidx;

    CDK_ASSERT(symbol); 
    CDK_ASSERT(symbol->flags & CDK_SYMBOL_FLAG_REGISTER); 

    /* Did the user specify indices? */
    if (sid->addr.start == -1) {
        /* No indices specified, insert limits if any */
        maxidx = CDK_SYMBOL_INDEX_MAX_GET(symbol->index); 
        if (maxidx) {
            sid->addr.start = CDK_SYMBOL_INDEX_MIN_GET(symbol->index); 
            sid->addr.end = maxidx; 
        }
    }
    
    /* Blocks are all setup. Now we need to check ports */
    if (symbol->flags & CDK_SYMBOL_FLAG_PORT) {
        /* This is a port-based register */
        if (!sid->port.valid) {
            /* Ports were not specified, so we'll put them in */
            sid->port.start = 0; 
            sid->port.end = CDK_CONFIG_MAX_PORTS - 1; 
        }
    }
    else {
        /* Ignore port specification if not a port-based register */
        sid->port.start = -1; 
        sid->port.end = -1; 
    }

    /*
     * For ROBO chips block and blocktype are synonymous, i.e. there is 
     * only one block per blocktype. Since such top-level ports can span 
     * different blocks, we will iterate over all of the top-level ports 
     * specified, and handle each specific port within the loop below. 
     */
    for (port = sid->port.start; port <= sid->port.end; port++) {

        int b, size;
        cdk_shell_id_t sid2; 
        int blktype; 
        const cdk_robo_block_t *blkp; 

        /* Need a copy of the SID for this block iteration */
        CDK_MEMCPY(&sid2, sid, sizeof(sid2)); 

        /*
         * Iterate through all blocks of this symbol
         */
        for (blktype = 0; blktype < CDK_ROBO_INFO(unit)->nblktypes; blktype++) {

            if ((symbol->flags & (1 << blktype)) == 0) {
                continue;
            }

            /*
             * Iterate through all blocks in the chip
             */
            for (b = 0; b < CDK_ROBO_INFO(unit)->nblocks; b++) {

                /* Get the block pointer for this block */
                blkp = CDK_ROBO_INFO(unit)->blocks + b;
                CDK_ASSERT(blkp); 

                if (blkp->type != blktype) {
                    continue;
                }

                /* 
                 * See if the current port is actually a part of the
                 * current block. If not, we will just punt. 
                 */
                if (port >= 0) {
                    if (!CDK_PBMP_MEMBER(blkp->pbmps, port)) {
                        continue;
                    }
                    sid2.port.start = sid2.port.end = port; 
                }

                /* Lets get it on */
                size = CDK_SYMBOL_INDEX_SIZE_GET(symbol->index);
                cdk_robo_shell_regops(unit, symbol, &sid2, size, and_masks, or_masks); 
                break;
            }
        
            /*
             * If a block was specified, the ports we happen to be iterating over 
             * are block-based physical ports, which we already processed. 
             * Lets bail out of the outermost for loop. 
             */
            if (sid->block.valid) {
                break;
            }
        }
    }

    return 0;
}


/*******************************************************************************
 *
 * Get or Set a chip memory via S-channel
 */

static int
_mem_symop(int unit, const cdk_symbol_t *symbol, cdk_shell_id_t *sid, 
           uint32_t *and_masks, uint32_t *or_masks)
{    
    CDK_ASSERT(symbol); 
    CDK_ASSERT(symbol->flags & CDK_SYMBOL_FLAG_MEMORY); 

    /* Did the user specify indices? */
    if (sid->addr.start == -1) {
        /* No indices specified, insert limits for the memory */
        sid->addr.start = CDK_SYMBOL_INDEX_MIN_GET(symbol->index); 
        sid->addr.end = CDK_SYMBOL_INDEX_MAX_GET(symbol->index); 
    }
    
    return cdk_robo_shell_memops(unit, symbol, sid, CDK_SYMBOL_INDEX_SIZE_GET(symbol->index), 
                                 and_masks, or_masks); 
}


/*******************************************************************************
 *
 * Symbolic register and memory operations.
 */

int 
cdk_robo_shell_symop(int unit, const cdk_symbol_t *symbol, cdk_shell_id_t *sid, 
                    uint32_t *and_masks, uint32_t *or_masks)
{
    /* Dispatch according to symbol type */
    if (symbol->flags & CDK_SYMBOL_FLAG_REGISTER) {
        _reg_symop(unit, symbol, sid, and_masks, or_masks);
    }
    else if (symbol->flags & CDK_SYMBOL_FLAG_MEMORY) {
        _mem_symop(unit, symbol, sid, and_masks, or_masks);
    }
    else {
        /* Should never get here */
        CDK_PRINTF("%ssymbol '%s' was not generated correctly\n", 
                   CDK_CONFIG_SHELL_ERROR_STR, symbol->name); 
    }   
    return 0; 
}
