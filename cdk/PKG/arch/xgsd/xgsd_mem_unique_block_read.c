/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * XGS memory access functions.
 */

#include <cdk/cdk_device.h>
#include <cdk/cdk_debug.h>
#include <cdk/arch/xgsd_chip.h>
#include <cdk/arch/xgsd_mem.h>

/*******************************************************************************
 *
 * Access block-based memories
 */

int
cdk_xgsd_mem_unique_block_read(int unit, uint32_t blkacc, int block, 
                               int blkidx, int baseidx, uint32_t offset, int idx, 
                               void *entry_data, int size)
{
    uint32_t addr, adext = CDK_XGSD_BLKACC2ADEXT(blkacc);
    int acctype = CDK_XGSD_BLKACC2ACCTYPE(blkacc);
    uint32_t pmask, inst_mask, pipe_info = 0;
    int linst, pdx, blktype, pipe_offset;

    if (baseidx < 0) {
        return -1;
    }

    if (cdk_xgsd_block_type(unit, block, &blktype, NULL) < 0) {
        return -1;
    }

    pipe_info = cdk_xgsd_pipe_info(unit, offset, acctype, blktype, -1);
    pmask = CDK_XGSD_PIPE_PMASK(pipe_info);
    linst = CDK_XGSD_PIPE_LINST(pipe_info);

    /* Check Parameters */
    if (blkidx >= 0 && pmask != 0) {
        if (((1 << blkidx) & pmask) == 0) {
            CDK_ERR(("cdk_xgsd_mem_unique_block_read[%d]:" 
                     "unique blk idx err "
                     "block=%d blkidx=%d pmask=0x%02"PRIx32"\n",
                     unit, block, blkidx, pmask));
            return -1;
        }
    }
    if (baseidx >= linst && linst > 0) {
        CDK_ERR(("cdk_xgsd_mem_unique_block_read[%d]:" 
                 "base idx err "
                 "block=%d basetype=%d baseidx=%d linst=0x%02"PRIx32"\n",
                 unit, block, (int)CDK_XGSD_ADDR2BASETYPE(offset), 
                 baseidx, (uint32_t)linst));
        return -1;
    }

    addr = cdk_xgsd_blockport_addr(unit, block, -1, offset, 0);

    /* Update address extension with specified block */
    CDK_XGSD_ADEXT_BLOCK_SET(adext, block);

    if (pmask != 0) {
        if (blkidx < 0) {
            /* Read from first valid unique block if blkidx < 0 */
            if (linst > 0) { /* unique block with basetype defined */
                inst_mask = cdk_xgsd_pipe_info(unit, offset, acctype, blktype, 
                                               baseidx);
                for (pdx = 0; pdx < CDK_XGSD_PHYS_INST_MAX; pdx++) {
                    if (inst_mask & (1 << pdx)) {
                        blkidx = pdx;
                        break;
                    }
                }
            } else { /* unique block without basetype defined */
                blkidx = 0;
            }
        }
        /* Update access type in address extension */
        CDK_XGSD_ADEXT_ACCTYPE_SET(adext, blkidx);
    }

    pipe_offset = baseidx * CDK_XGSD_PIPE_SECT_SIZE(pipe_info);

    return cdk_xgsd_mem_read(unit, adext, addr, pipe_offset + idx,
                             entry_data, size);
}
