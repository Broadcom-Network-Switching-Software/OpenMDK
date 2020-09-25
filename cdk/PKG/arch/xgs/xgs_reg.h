/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * XGS register access functions.
 */

#ifndef __XGS_REG_H__
#define __XGS_REG_H__

#include <cdk/arch/xgs_chip.h>

/*******************************************************************************
 *
 * Common register routines (internal)
 */

extern int
cdk_xgs_reg_read(int unit, uint32_t addr, uint32_t *data, int size);

extern int
cdk_xgs_reg_write(int unit, uint32_t addr, uint32_t *data, int size);

extern int
cdk_xgs_reg_port_read(int unit, int port, uint32_t addr, void *data, int size); 

extern int
cdk_xgs_reg_port_write(int unit, int port, uint32_t addr, void *data, int size); 

extern int
cdk_xgs_reg_block_read(int unit, int block, uint32_t addr,
                       void *vptr, int size);

extern int
cdk_xgs_reg_block_write(int unit, int block, uint32_t addr,
                        void *vptr, int size);

extern int
cdk_xgs_reg_blocks_read(int unit, uint32_t blktypes, int port,
                        uint32_t addr, void *vptr, int size);

extern int
cdk_xgs_reg_blocks_write(int unit, uint32_t blktypes, int port,
                         uint32_t addr, void *vptr, int size);

/*******************************************************************************
 *
 * Access 32 bit internal registers
 */

extern int 
cdk_xgs_reg32_read(int unit, uint32_t addr, void *data); 

extern int 
cdk_xgs_reg32_write(int unit, uint32_t addr, void *data); 

extern int
cdk_xgs_reg32_writei(int unit, uint32_t addr, uint32_t data); 


/*******************************************************************************
 *
 * Access 32 bit internal port-based registers
 */

extern int
cdk_xgs_reg32_port_read(int unit, int port, uint32_t addr, void *data); 

extern int 
cdk_xgs_reg32_port_write(int unit, int port, uint32_t addr, void *data); 

extern int
cdk_xgs_reg32_port_writei(int unit, int port, uint32_t addr, uint32_t data); 


/*******************************************************************************
 *
 * Access 32 bit internal block-based registers
 */

extern int 
cdk_xgs_reg32_block_read(int unit, int block, uint32_t addr, void *data); 

extern int
cdk_xgs_reg32_block_write(int unit, int block, uint32_t addr, void *data); 

extern int
cdk_xgs_reg32_block_writei(int unit, int block, uint32_t addr, uint32_t data); 



/*******************************************************************************
 *
 * Access 32 bit internal registers by both block and port. 
 */

extern int 
cdk_xgs_reg32_blockport_read(int unit, int block, int port, uint32_t addr, 
                         void *data); 

extern int
cdk_xgs_reg32_blockport_write(int unit, int block, int port, 
                              uint32_t addr, void *data); 

extern int
cdk_xgs_reg32_blockport_writei(int unit, int block, int port, 
                               uint32_t addr, uint32_t data); 


/*******************************************************************************
 *
 * Access 32 bit internal block-based registers on all blocks of given types
 */

extern int
cdk_xgs_reg32_blocks_read(int unit, uint32_t blktypes, int port,
                          uint32_t addr, void *vptr);

extern int
cdk_xgs_reg32_blocks_write(int unit, uint32_t blktypes, int port,
                           uint32_t addr, void *data); 

extern int
cdk_xgs_reg32_blocks_writei(int unit, uint32_t blktypes, int port,
                            uint32_t addr, uint32_t data); 


/*******************************************************************************
 *
 * Access 64 bit internal registers
 */

extern int
cdk_xgs_reg64_read(int unit, uint32_t addr, void *data); 

extern int
cdk_xgs_reg64_write(int unit, uint32_t addr, void *data); 


/*******************************************************************************
 *
 * Access 64 bit internal port-based registers
 */

extern int
cdk_xgs_reg64_port_read(int unit, int port, uint32_t addr, void *data); 

extern int
cdk_xgs_reg64_port_write(int unit, int port, uint32_t addr, void *data); 



/*******************************************************************************
 *
 * Access 64 bit internal block-based registers
 */

extern int
cdk_xgs_reg64_block_read(int unit, int block, uint32_t addr, void *data); 

extern int
cdk_xgs_reg64_block_write(int unit, int block, uint32_t addr, void *data); 


/*******************************************************************************
 *
 * Access 64 bit internal registers by both block and port. 
 */

extern int 
cdk_xgs_reg64_blockport_read(int unit, int block, int port, 
                             uint32_t addr, void *data); 

extern int
cdk_xgs_reg64_blockport_write(int unit, int block, int port, 
                              uint32_t addr, void *data); 


/*******************************************************************************
 *
 * Access 64 bit internal block-based registers on all blocks of given types
 */

extern int
cdk_xgs_reg64_blocks_read(int unit, uint32_t blktypes, int port,
                          uint32_t addr, void *vptr);
extern int
cdk_xgs_reg64_blocks_write(int unit, uint32_t blktypes, int port,
                           uint32_t addr, void *data); 


/*******************************************************************************
 *
 * Validate index for per-port variable register array
 */

extern int
cdk_xgs_reg_index_valid(int unit, int port, int regidx,
                        int encoding, int regidx_max); 


#endif /* __XGS_REG_H__ */
