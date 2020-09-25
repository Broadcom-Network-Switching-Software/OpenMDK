/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * XGS memory access functions.
 */

#ifndef __CDK_MEM_H__
#define __CDK_MEM_H__

#include <cdk/arch/xgsd_chip.h>

/*******************************************************************************
 *
 * Access memories
 */

extern int
cdk_xgsd_mem_read(int unit, uint32_t adext, uint32_t addr, uint32_t idx,
                  void *entry_data, int size); 

extern int
cdk_xgsd_mem_write(int unit, uint32_t adext, uint32_t addr, uint32_t idx,
                   void *entry_data, int size); 

extern int
cdk_xgsd_mem_clear(int unit, uint32_t adext, uint32_t addr,
                   uint32_t si, uint32_t ei, int size); 

#define CDK_XGSD_MEM_CLEAR(_u, _m) \
    cdk_xgsd_mem_blocks_clear(_u, _m##_BLKACC, _m, _m##_MIN, _m##_MAX, (_m##_SIZE + 3) >> 2)


/*******************************************************************************
 *
 * Access block-based memories
 */

extern int
cdk_xgsd_mem_block_read(int unit, uint32_t blkacc, int block,
                        uint32_t offset, int idx, void *entry_data, int size); 

extern int
cdk_xgsd_mem_block_write(int unit, uint32_t blkacc, int block,
                         uint32_t offset, int idx, void *entry_data, int size); 

extern int
cdk_xgsd_mem_block_clear(int unit, uint32_t blkacc, int block,
                         uint32_t offset, uint32_t si, uint32_t ei, int size); 

extern int
cdk_xgsd_mem_blocks_read(int unit, uint32_t blkacc, int port, uint32_t offset,
                         int idx, void *entry_data, int size);

extern int
cdk_xgsd_mem_blocks_write(int unit, uint32_t blkacc, int port, uint32_t offset,
                          int idx, void *entry_data, int size);

extern int
cdk_xgsd_mem_blocks_clear(int unit, uint32_t blkacc,
                          uint32_t offset, uint32_t si, uint32_t ei, int size);


/*******************************************************************************
 *
 * Access block-based memories
 */

extern int
cdk_xgsd_mem_unique_block_read(int unit, uint32_t blkacc, int block, 
                               int blkidx, int baseidx, uint32_t offset, int idx, 
                               void *entry_data, int size);

extern int
cdk_xgsd_mem_unique_block_write(int unit, uint32_t blkacc, int block, 
                                int blkidx, int baseidx, uint32_t offset, int idx, 
                                void *entry_data, int size);

extern int
cdk_xgsd_mem_unique_block_clear(int unit, uint32_t blkacc, int block,
                                int blkidx, int baseidx, uint32_t offset, 
                                uint32_t si, uint32_t ei, int size);


/*******************************************************************************
 *
 * Active memory sizes may depend on configuration
 */

extern uint32_t
cdk_xgsd_mem_maxidx(int unit, int enum_val, uint32_t maxidx);


/*******************************************************************************
 *
 * Non-indexed access to memories
 */

extern int
cdk_xgsd_mem_op(int unit, cdk_xgsd_mem_op_info_t *mem_op_info);

#endif /* __CDK_MEM_H__ */
