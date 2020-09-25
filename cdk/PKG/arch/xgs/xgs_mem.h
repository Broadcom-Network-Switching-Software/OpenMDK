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

#include <cdk/arch/xgs_chip.h>

/*******************************************************************************
 *
 * Access memories
 */

extern int
cdk_xgs_mem_read(int unit, uint32_t addr, uint32_t idx, void *entry_data, int size); 

extern int
cdk_xgs_mem_write(int unit, uint32_t addr, uint32_t idx, void *entry_data, int size); 

extern int
cdk_xgs_mem_clear(int unit, uint32_t addr, uint32_t si, uint32_t ei, int size); 

#define CDK_XGS_MEM_CLEAR(_u, _m) \
    cdk_xgs_mem_clear(_u, _m, _m##_MIN, _m##_MAX, (_m##_SIZE + 3) >> 2)


/*******************************************************************************
 *
 * Access block-based memories
 */

extern int
cdk_xgs_mem_block_read(int unit, int block, uint32_t addr, 
                       int idx, void *entry_data, int size); 

extern int
cdk_xgs_mem_block_write(int unit, int block, uint32_t addr, 
                        int idx, void *entry_data, int size); 

extern int
cdk_xgs_mem_block_clear(int unit, int block, uint32_t addr, 
                        uint32_t si, uint32_t ei, int size); 

extern int
cdk_xgs_mem_blocks_read(int unit, uint32_t blktypes, int port,
                        uint32_t addr, int idx, void *entry_data, int size);

extern int
cdk_xgs_mem_blocks_write(int unit, uint32_t blktypes, int port,
                         uint32_t addr, int idx, void *entry_data, int size);

/*******************************************************************************
 *
 * Active memory sizes may depend on configuration
 */

extern uint32_t
cdk_xgs_mem_maxidx(int unit, uint32_t addr, uint32_t maxidx);


/*******************************************************************************
 *
 * Non-indexed access to memories
 */

extern int
cdk_xgs_mem_op(int unit, cdk_xgs_mem_op_info_t *mem_op_info);

#endif /* __CDK_MEM_H__ */
