/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * XGS register access functions.
 */

#include <cdk/cdk_device.h>
#include <cdk/cdk_debug.h>
#include <cdk/arch/xgsd_chip.h>
#include <cdk/arch/xgsd_reg.h>

/*******************************************************************************
 *
 * Access 32 bit internal registers with ACCTYPE equivalent to UNIQUE by both block and port. 
 */

int
cdk_xgsd_reg_unique_blockport_write(int unit, uint32_t blkacc, int block, 
                                    int blkidx, int port, uint32_t offset, 
                                    int idx, void *data, int size)
{
    uint32_t addr, adext = CDK_XGSD_BLKACC2ADEXT(blkacc);
    int acctype = CDK_XGSD_BLKACC2ACCTYPE(blkacc);
    int rv = 0; 
    uint32_t pmask, inst_mask, pipe_info = 0;
    int linst, pdx, blktype, unique_idx = 0;

    if (port < 0) {
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
            CDK_ERR(("cdk_xgsd_reg_unique_blockport_write[%d]:" 
                     "unique blk idx err "
                     "block=%d blkidx=%d pmask=0x%02"PRIx32"\n",
                     unit, block, blkidx, pmask));
            return -1;
        }
    }
    if (port >= linst && linst > 0) {
        CDK_ERR(("cdk_xgsd_reg_unique_blockport_write[%d]:" 
                 "base idx err "
                 "block=%d basetype=%d baseidx=%d linst=0x%02"PRIx32"\n",
                 unit, block, (int)CDK_XGSD_ADDR2BASETYPE(offset), 
                 port, (uint32_t)linst));
        return -1;
    }

    addr = cdk_xgsd_blockport_addr(unit, block, port, offset, idx);

    /* Update address extension with specified block */
    CDK_XGSD_ADEXT_BLOCK_SET(adext, block);

    if (pmask != 0) {
        if (blkidx < 0) {
            /* Write all unique blocks if blkidx < 0 */
            if (linst > 0) { /* unique block with basetype defined */
                inst_mask = cdk_xgsd_pipe_info(unit, offset, acctype, blktype, 
                                               port);
                for (pdx = 0; pdx < CDK_XGSD_PHYS_INST_MAX; pdx++) {
                    if (inst_mask & (1 << pdx)) {
                        /* Update access type in address extension */
                        CDK_XGSD_ADEXT_ACCTYPE_SET(adext, pdx);                               
                    }
                    rv += cdk_xgsd_reg_write(unit, adext, addr, data, size);
                }
            } else { /* unique block without basetype defined */
                while (((1 << unique_idx) & pmask) != 0) {
                    /* Update access type in address extension */
                    CDK_XGSD_ADEXT_ACCTYPE_SET(adext, unique_idx);
                    rv = cdk_xgsd_reg_write(unit, adext, addr, data, size);                    
                    unique_idx ++;
                }
            }
        } else {
            /* Update access type in address extension */
            CDK_XGSD_ADEXT_ACCTYPE_SET(adext, blkidx);
            rv = cdk_xgsd_reg_write(unit, adext, addr, data, size);
        }
    } else {
        rv = cdk_xgsd_reg_write(unit, adext, addr, data, size); 
    }

    return rv;
}
