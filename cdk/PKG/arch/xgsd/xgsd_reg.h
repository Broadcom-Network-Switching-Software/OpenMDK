/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * XGS register access functions.
 */

#ifndef __XGSD_REG_H__
#define __XGSD_REG_H__

#include <cdk/arch/xgsd_chip.h>

/*******************************************************************************
 *
 * Common register routines (internal)
 */

extern int
cdk_xgsd_reg_read(int unit, uint32_t adext, uint32_t addr,
                  uint32_t *data, int size);

extern int
cdk_xgsd_reg_write(int unit, uint32_t adext, uint32_t addr,
                   uint32_t *data, int size);

extern int
cdk_xgsd_reg_port_read(int unit, uint32_t blkacc, int port,
                       uint32_t offset, int idx, void *data, int size);

extern int
cdk_xgsd_reg_port_write(int unit, uint32_t blkacc, int port,
                        uint32_t offset, int idx, void *data, int size);

extern int
cdk_xgsd_reg_block_read(int unit, uint32_t blkacc, int block,
                        uint32_t offset, int idx, void *data, int size);

extern int
cdk_xgsd_reg_block_write(int unit, uint32_t blkacc, int block,
                         uint32_t offset, int idx, void *data, int size);

extern int
cdk_xgsd_reg_blocks_read(int unit, uint32_t blkacc, int port,
                         uint32_t offset, int idx, void *vptr, int size);

extern int
cdk_xgsd_reg_blocks_write(int unit, uint32_t blkacc, int port,
                          uint32_t offset, int idx, void *vptr, int size);

extern int
cdk_xgsd_reg_unique_blockport_read(int unit, uint32_t blkacc, int block, 
                                   int blkidx, int port, uint32_t offset, 
                                   int idx, void *data, int size);

extern int
cdk_xgsd_reg_unique_blockport_write(int unit, uint32_t blkacc, int block, 
                                    int blkidx, int port, uint32_t offset, 
                                    int idx, void *data, int size);

/*******************************************************************************
 *
 * Access 32 bit internal registers
 */

extern int 
cdk_xgsd_reg32_read(int unit, uint32_t adext, uint32_t addr, void *data); 

extern int 
cdk_xgsd_reg32_write(int unit, uint32_t adext, uint32_t addr, void *data); 

extern int
cdk_xgsd_reg32_writei(int unit, uint32_t adext, uint32_t addr, uint32_t data); 


/*******************************************************************************
 *
 * Access 32 bit internal port-based registers
 */

extern int
cdk_xgsd_reg32_port_read(int unit, uint32_t blkacc, int port,
                         uint32_t offset, int idx, void *data);

extern int 
cdk_xgsd_reg32_port_write(int unit, uint32_t blkacc, int port,
                          uint32_t offset, int idx, void *data);

extern int
cdk_xgsd_reg32_port_writei(int unit, uint32_t blkacc, int port,
                           uint32_t offset, int idx, uint32_t data);


/*******************************************************************************
 *
 * Access 32 bit internal block-based registers
 */

extern int 
cdk_xgsd_reg32_block_read(int unit, uint32_t blkacc, int block,
                          uint32_t offset, int idx, void *data);

extern int
cdk_xgsd_reg32_block_write(int unit, uint32_t blkacc, int block,
                           uint32_t offset, int idx, void *data);

extern int
cdk_xgsd_reg32_block_writei(int unit, uint32_t blkacc, int block,
                            uint32_t offset, int idx, uint32_t data);


/*******************************************************************************
 *
 * Access 32 bit internal registers by both block and port. 
 */

extern int 
cdk_xgsd_reg32_blockport_read(int unit, uint32_t blkacc, int block, int port,
                              uint32_t offset, int idx, void *data);

extern int
cdk_xgsd_reg32_blockport_write(int unit, uint32_t blkacc, int block, int port,
                               uint32_t offset, int idx, void *data);

extern int
cdk_xgsd_reg32_blockport_writei(int unit, uint32_t blkacc, int block, int port,
                                uint32_t offset, int idx, uint32_t data); 


/*******************************************************************************
 *
 * Access 32 bit internal block-based registers on all blocks of given types
 */

extern int
cdk_xgsd_reg32_blocks_read(int unit, uint32_t blkacc, int port,
                           uint32_t offset, int idx, void *data);

extern int
cdk_xgsd_reg32_blocks_write(int unit, uint32_t blkacc, int port,
                            uint32_t offset, int idx, void *data);

extern int
cdk_xgsd_reg32_blocks_writei(int unit, uint32_t blkacc, int port,
                             uint32_t offset, int idx, uint32_t data); 


/*******************************************************************************
 *
 * Access 32 bit internal block-based registers with ACCTYPE equivalent to UNIQUE
 */

extern int
cdk_xgsd_reg32_unique_block_read(int unit, uint32_t blkacc, int block, 
                                 int blkidx, uint32_t offset, int idx, 
                                 void *data);

extern int
cdk_xgsd_reg32_unique_block_write(int unit, uint32_t blkacc, int block,
                                  int blkidx, uint32_t offset, int idx, 
                                  void *data);


/*******************************************************************************
 *
 * Access 32 bit internal registers with ACCTYPE equivalent to UNIQUE by both block and port. 
 */

extern int
cdk_xgsd_reg32_unique_blockport_read(int unit, uint32_t blkacc, int block, 
                                     int blkidx, int port, uint32_t offset, 
                                     int idx, void *data);

extern int
cdk_xgsd_reg32_unique_blockport_write(int unit, uint32_t blkacc, int block,
                                      int blkidx, int port, uint32_t offset, 
                                      int idx, void *data);


/*******************************************************************************
 *
 * Access 64 bit internal registers
 */

extern int
cdk_xgsd_reg64_read(int unit, uint32_t adext, uint32_t addr, void *data); 

extern int
cdk_xgsd_reg64_write(int unit, uint32_t adext, uint32_t addr, void *data); 


/*******************************************************************************
 *
 * Access 64 bit internal port-based registers
 */

extern int
cdk_xgsd_reg64_port_read(int unit, uint32_t blkacc, int port,
                         uint32_t offset, int idx, void *data);

extern int
cdk_xgsd_reg64_port_write(int unit, uint32_t blkacc, int port,
                          uint32_t offset, int idx, void *data);



/*******************************************************************************
 *
 * Access 64 bit internal block-based registers
 */

extern int
cdk_xgsd_reg64_block_read(int unit, uint32_t blkacc, int block,
                          uint32_t offset, int idx, void *data);

extern int
cdk_xgsd_reg64_block_write(int unit, uint32_t blkacc, int block,
                           uint32_t offset, int idx, void *data);


/*******************************************************************************
 *
 * Access 64 bit internal registers by both block and port. 
 */

extern int 
cdk_xgsd_reg64_blockport_read(int unit, uint32_t blkacc, int block, int port,
                              uint32_t offset, int idx, void *data);

extern int
cdk_xgsd_reg64_blockport_write(int unit, uint32_t blkacc, int block, int port,
                               uint32_t offset, int idx, void *data);


/*******************************************************************************
 *
 * Access 64 bit internal block-based registers on all blocks of given types
 */

extern int
cdk_xgsd_reg64_blocks_read(int unit, uint32_t blkacc, int port,
                            uint32_t offset, int idx, void *data);

extern int
cdk_xgsd_reg64_blocks_write(int unit, uint32_t blkacc, int port,
                            uint32_t offset, int idx, void *data);


/*******************************************************************************
 *
 * Access 64 bit internal block-based registers with ACCTYPE equivalent to UNIQUE
 */

extern int
cdk_xgsd_reg64_unique_block_read(int unit, uint32_t blkacc, int block, 
                                 int blkidx, uint32_t offset, int idx, 
                                 void *data);

extern int
cdk_xgsd_reg64_unique_block_write(int unit, uint32_t blkacc, int block,
                                  int blkidx, uint32_t offset, int idx, 
                                  void *data);


/*******************************************************************************
 *
 * Access 64 bit internal registers with ACCTYPE equivalent to UNIQUE by both block and port. 
 */

extern int
cdk_xgsd_reg64_unique_blockport_read(int unit, uint32_t blkacc, int block, 
                                     int blkidx, int port, uint32_t offset, 
                                     int idx, void *data);

extern int
cdk_xgsd_reg64_unique_blockport_write(int unit, uint32_t blkacc, int block,
                                      int blkidx, int port, uint32_t offset, 
                                      int idx, void *data);


/*******************************************************************************
 *
 * Validate index for per-port variable register array
 */

extern int
cdk_xgsd_reg_index_valid(int unit, int port, int regidx,
                         int encoding, int regidx_max); 


#endif /* __XGSD_REG_H__ */
