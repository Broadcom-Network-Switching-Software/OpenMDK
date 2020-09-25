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

#include <cdk/arch/xgsd_shell.h>


/*******************************************************************************
 *
 * Get or set a CMIC register. 
 */

static int 
_cmic_symop(int unit, const cdk_symbol_t *symbol, cdk_shell_id_t *sid, 
            uint32_t *and_masks, uint32_t *or_masks)
{
    uint32_t addr, data, step; 
    int i;

    /* No port/block identifiers on cmic registers */
    if (sid->port.valid || sid->block.valid) {
        return cdk_shell_parse_error("cmic address", sid->id); 
    }

    /* Did the user specify indices? */
    if (sid->addr.start == -1) {
        /* No indices specified, insert limits for the memory */
        sid->addr.start = CDK_SYMBOL_INDEX_MIN_GET(symbol->index); 
        sid->addr.end = CDK_SYMBOL_INDEX_MAX_GET(symbol->index); 
    }

    step = CDK_SYMBOL_INDEX_STEP_GET(symbol->index);
    
    for (i = sid->addr.start; i <= sid->addr.end; i++) {

        /* Index 32 bit addresses */
        addr = symbol->addr + (i * 4 * step);

        /* Read the data */
        CDK_DEV_READ32(unit, addr, &data); 

        /* This is a read-modify-write if masks are specified */
        if (and_masks || or_masks ) {
            if(and_masks) data &= and_masks[0]; 
            if(or_masks) data |= or_masks[0]; 
        
            /* Write the data */
            CDK_DEV_WRITE32(unit, addr, data); 

        } else {
            /* If we're here, it was a read operation and we should output the data */
            CDK_PRINTF("%s", symbol->name); 
            if (CDK_SYMBOL_INDEX_MAX_GET(symbol->index) > 0) {
                CDK_PRINTF("[%d]", i); 
            }
            CDK_PRINTF(".cmic [0x%08"PRIx32"] = 0x%"PRIx32"\n", addr, data); 
    
            /* Output field data if it is available */
#if CDK_CONFIG_INCLUDE_FIELD_INFO == 1
            if (sid->flags & CDK_SHELL_IDF_RAW) {
                continue;
            }
            if (symbol->fields) {
                int skip_zeros = (sid->flags & CDK_SHELL_IDF_NONZERO) ? 1 : 0;
                cdk_xgsd_shell_show_fields(unit, symbol, &data, skip_zeros); 
            }
#endif
        }
    }

    return 0; 
}


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
            sid->addr.start = 0; 
            sid->addr.end = maxidx; 
        }
    }
    
    /*
     * If the user specified a port number, but not block, then the ports as 
     * specified are not block-based. Since such top-level ports can span 
     * different blocks, we will iterate over all of the top-level ports 
     * specified, and handle each specific port within the loop below. 
     */
    for (port = sid->port.start; port <= sid->port.end; port++) {

        int b, match_block, wsize;
        cdk_shell_id_t sid2; 
        int blktype; 
        const cdk_xgsd_block_t *blkp; 
        uint32_t pipe_info;

        /*
         * Iterate through all blocks of this symbol
         */
        for (blktype = 0; blktype < CDK_XGSD_INFO(unit)->nblktypes; blktype++) {

            if ((symbol->flags & (1 << blktype)) == 0) {
                continue;
            }

            /* Need a copy of the SID for this block iteration */
            CDK_MEMCPY(&sid2, sid, sizeof(sid2)); 

            /*
             * Set block type filter in case identical block types are
             * not contiguous.
             */
            sid2.block.ext32 = (1 << blktype);

            pipe_info = cdk_xgsd_pipe_info(unit, sid->addr.name32,
                                           CDK_XGSD_BLKACC2ACCTYPE(symbol->flags),
                                           blktype, -1);
            if (pipe_info && sid2.block.valid && sid2.block.start != -1) {
                pipe_info &= ~0xff;
                for (b = 0; b < CDK_XGSD_PHYS_INST_MAX; b++) {
                    if (b >= sid2.block.start && b <= sid2.block.end) {
                        CDK_XGSD_PIPE_PINST_SET(pipe_info, b);
                    }
                }
                sid2.block.start = sid2.block.end = 0;
            }

            match_block = 0;

            /*
             * Iterate through all blocks in the chip
             */
            for (b = 0; b < CDK_XGSD_INFO(unit)->nblocks; b++) {

                /* Get the block pointer for this block */
                blkp = CDK_XGSD_INFO(unit)->blocks + b; 
                CDK_ASSERT(blkp); 

                if (blkp->type != blktype) {
                    continue;
                }

                /* Does the ID contain a block specifier? */
                if (!sid2.block.valid) {
                    /* User didn't specify the block, so we'll insert this one */
                    CDK_STRCPY(sid2.block.name, cdk_xgsd_shell_block_type2name(unit, blktype)); 
                
                    /* 
                     * If the user DID specify a top-level port number (port != -1)
                     * we need to see if that port is actually a part of this block. 
                     * If not, we will just punt. 
                     */
                    if (port != -1) {
                        cdk_xgsd_pblk_t pb; 
                        /* Look for this port within this blocktype */
                        if (cdk_xgsd_port_block(unit, port, &pb, blkp->type) == 0 && 
                            pb.block == blkp->blknum) {
                            /* This top-level port is a member of this block */
                            sid2.block.start = sid2.block.end = pb.block; 
                            sid2.port.start = sid2.port.end = pb.bport; 
                            if (CDK_XGSD_PIPE_LINST(pipe_info)) {
                                sid2.port.start = sid2.port.end = port; 
                            }
                        }
                        else {
                            /* This top-level port is not a member of this block */
                            continue;
                        }
                    }
                    else {
                        /* No block and no ports, insert all ports in this block */
                        /* Add all blocks of this type */
                        if (sid2.block.start == -1 || sid2.block.end < blkp->blknum) {
                            sid2.block.end = blkp->blknum; 
                        }
                        if (sid2.block.start == -1 || sid2.block.start > blkp->blknum) {
                            sid2.block.start = blkp->blknum; 
                        }
                    }
                }
                else {
                    /* User specified a block identifier */
                    /* does the block match this one? */
                    if (CDK_STRCMP(sid2.block.name, cdk_xgsd_shell_block_type2name(unit, blktype)) == 0) {
                        /* Block specifier matches */
                        match_block = 1;
                        /* If start and stop were omitted, then we need to put them in */
                        if (sid->block.start == -1) {
                            /* Add all blocks of this type */
                            if (sid2.block.start == -1 || sid2.block.end < blkp->blknum) {
                                sid2.block.end = blkp->blknum; 
                            }
                            if (sid2.block.start == -1 || sid2.block.start > blkp->blknum) {
                                sid2.block.start = blkp->blknum; 
                            }
                        }   
                        else {
                            /* specific blocks were indicated. */
                            /* Need to convert these to physical blocks */
                            sid2.block.start = cdk_xgsd_block_number(unit, blktype, sid2.block.start); 
                            sid2.block.end = cdk_xgsd_block_number(unit, blktype, sid2.block.end); 
                        }                               
                    }
                    else {                      
                        /* Block specified does not match this one. */
                        /* I guess we're done */
                        continue;
                    }
                }
            
                /* Blocks are all setup. Now we need to check ports */
                if (CDK_XGSD_PIPE_LINST(pipe_info)) {
                    if (port == -1) {
                       sid2.port.start = 0;
                       sid2.port.end = CDK_XGSD_PIPE_LINST(pipe_info) - 1;
                    }
                } else if (symbol->flags & CDK_SYMBOL_FLAG_PORT) {
                    /* This is a port-based register */
                    /* Were specific ports specified? */
                    if (!sid2.port.valid) {
                        int p, port_end = -1;

                        /* Ports were not specified, so we'll put them in */
                        sid2.port.start = 0; 
                        CDK_PBMP_ITER(blkp->pbmps, p) {
                            port_end++; 
                        }
                        if (sid2.port.end < port_end) {
                            sid2.port.end = port_end;
                        }
                    }
                }
                else {
                    /* Ignore port specification if not a port-based register */
                    sid2.port.start = -1; 
                    sid2.port.end = -1;
                }
            }
            
            if (sid2.block.valid && !match_block) {
                continue;
            }

            /* Skip if we don't have a valid block range by now */
            if (sid2.block.start == -1) {
                continue;
            }

            /* Get dual-pipe access type from symbol flags */
            sid2.addr.ext32 = CDK_XGSD_BLKACC2ADEXT(symbol->flags);

            /* Lets get it on */
            wsize = CDK_BYTES2WORDS(CDK_SYMBOL_INDEX_SIZE_GET(symbol->index));
            cdk_xgsd_shell_regops(unit, symbol, &sid2, wsize,
                                  CDK_XGSD_PIPE_PMASK(pipe_info),
                                  and_masks, or_masks); 
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
    int b, match_block, wsize; 
    int maxidx, enum_val, linst;
    cdk_shell_id_t sid2; 
    const cdk_xgsd_block_t *blkp; 
    int blktype; 
    uint32_t pipe_info;


    CDK_ASSERT(symbol); 
    CDK_ASSERT(symbol->flags & CDK_SYMBOL_FLAG_MEMORY); 

    /* Silently ignore if ports were specified */
    if (sid->port.valid) {
        return 0;
    }

    /* Did the user specify indices? */
    if (sid->addr.start == -1) {
        /* No indices specified, insert limits for the memory */
        sid->addr.start = CDK_SYMBOL_INDEX_MIN_GET(symbol->index); 
        maxidx = CDK_SYMBOL_INDEX_MAX_GET(symbol->index); 
        enum_val = cdk_symbols_index(CDK_XGSD_SYMBOLS(unit), symbol);
        sid->addr.end = cdk_xgsd_mem_maxidx(unit, enum_val, maxidx);
    }
    
    /* 
     * Iterate through all blocks of which this memory is a member 
     */
    for (blktype = 0; blktype < CDK_XGSD_INFO(unit)->nblktypes; blktype++) {

        if ((symbol->flags & (1 << blktype)) == 0) {
            continue;
        }

        /* Need a copy of this SID for this block iteration */
        CDK_MEMCPY(&sid2, sid, sizeof(sid2)); 
        
        pipe_info = cdk_xgsd_pipe_info(unit, sid->addr.name32,
                                       CDK_XGSD_BLKACC2ACCTYPE(symbol->flags),
                                       blktype, -1);
        if (pipe_info && sid2.block.valid && sid2.block.start != -1) {
            pipe_info &= ~0xff;
            for (b = 0; b < CDK_XGSD_PHYS_INST_MAX; b++) {
                if (b >= sid2.block.start && b <= sid2.block.end) {
                    CDK_XGSD_PIPE_PINST_SET(pipe_info, b);
                }
            }
            sid2.block.start = sid2.block.end = 0;
        }

        match_block = 0;

        for (b = 0; b < CDK_XGSD_INFO(unit)->nblocks; b++) {

            /* Get the block pointer for this block */
            blkp = CDK_XGSD_INFO(unit)->blocks + b;            
            CDK_ASSERT(blkp); 

            if (blkp->type != blktype){
                continue;
            }
            
            /* Does the SID contain a block specifier? */
            if (!sid2.block.valid) {
                /* If no specific blocks were specified, add all blocks of this type */
                if (sid2.block.start == -1 || sid2.block.end < blkp->blknum) {
                    sid2.block.end = blkp->blknum; 
                }
                if (sid2.block.start == -1 || sid2.block.start > blkp->blknum) {
                    sid2.block.start = blkp->blknum; 
                }
            }
            else {
                /* User specified a block identifier */
                /* does the block match this one? */
                if (CDK_STRCMP(sid2.block.name, cdk_xgsd_shell_block_type2name(unit, blktype)) == 0) {
                    /* Block specifier matches */
                    match_block = 1;
                    /* If start and stop were omitted, then we need to put them in */
                    if (sid->block.start == -1) {
                        /* Add all blocks of this type */
                        if (sid2.block.start == -1 || sid2.block.end < blkp->blknum) {
                            sid2.block.end = blkp->blknum; 
                        }
                        if (sid2.block.start == -1 || sid2.block.start > blkp->blknum) {
                            sid2.block.start = blkp->blknum; 
                        }
                    }   
                    else {
                        /* specific blocks were indicated. */
                        /* Need to convert these to physical blocks */
                        sid2.block.start = cdk_xgsd_block_number(unit, blktype, sid->block.start); 
                        sid2.block.end = cdk_xgsd_block_number(unit, blktype, sid->block.end); 
                    }                               
                }
                else {                      
                    /* Block specified does not match this one. */
                    /* I guess we're done */
                    continue;
                }
            }
        }

        /* Does the specified block match? */
        if (sid2.block.valid && !match_block) {
            continue;
        }
            
        /* We don't handle port numbers on memories */
        CDK_ASSERT((symbol->flags & CDK_SYMBOL_FLAG_PORT) == 0); 

        /* Skip if we don't have a valid block range by now */
        if (sid2.block.start == -1) {
            continue;
        }

        linst = CDK_XGSD_PIPE_LINST(pipe_info);
        if (linst > 0) {
            if (sid2.port.start == -1) {
                sid2.port.start = 0;
            }
            if (sid2.port.end == -1 || sid2.port.end >= linst) {
                sid2.port.end = linst - 1;
            }
            if (sid2.port.start > sid2.port.end) {
                sid2.port.start = sid2.port.end;
            }
        } else {
            sid2.port.start = -1;
            sid2.port.end = -1;
        }

        /* Get dual-pipe access type from symbol flags */
        sid2.addr.ext32 = CDK_XGSD_BLKACC2ADEXT(symbol->flags);

        wsize = CDK_BYTES2WORDS(CDK_SYMBOL_INDEX_SIZE_GET(symbol->index));
        cdk_xgsd_shell_memops(unit, symbol, &sid2, wsize, 
                              CDK_XGSD_PIPE_PMASK(pipe_info), 
                              CDK_XGSD_PIPE_SECT_SIZE(pipe_info),
                              and_masks, or_masks); 
    }

    return 0;     
}


/*******************************************************************************
 *
 * Symbolic register and memory operations.
 */

int 
cdk_xgsd_shell_symop(int unit, const cdk_symbol_t *symbol, cdk_shell_id_t *sid, 
                    uint32_t *and_masks, uint32_t *or_masks)
{
    int cmic_blktype = cdk_xgsd_shell_block_name2type(unit, "cmic");

    /* Dispatch according to symbol type */
    if (symbol->flags & (1 << cmic_blktype)) {
        _cmic_symop(unit, symbol, sid, and_masks, or_masks);
    }
    else if (symbol->flags & CDK_SYMBOL_FLAG_REGISTER) {
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

