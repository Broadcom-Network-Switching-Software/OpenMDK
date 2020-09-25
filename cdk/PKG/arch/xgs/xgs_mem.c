/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * XGS memory access functions.
 */

#include <cdk/cdk_device.h>
#include <cdk/cdk_assert.h>
#include <cdk/cdk_debug.h>
#include <cdk/cdk_simhook.h>

#include <cdk/arch/xgs_chip.h>
#include <cdk/arch/xgs_mem.h>
#include <cdk/arch/xgs_schan.h>

/*******************************************************************************
 *
 * Access memories
 */

int
cdk_xgs_mem_read(int unit, uint32_t addr, uint32_t idx, void* vptr, int size)
{
    schan_msg_t schan_msg;
    int i; 
    int rv; 
    int srcblk, dstblk, datalen;
    uint32_t* entry_data = (uint32_t*)vptr; 
    int entry_dw = size; 
    int adx = idx;

    CDK_ASSERT(CDK_DEV_EXISTS(unit)); 

    if (adx & CDK_XGS_MEM_FLAG_STEP2) {
        adx <<= 1;
        idx &= (CDK_XGS_MEM_FLAG_STEP2 - 1);
    }

    /* Simulator hooks */
    if (cdk_simhook_read) {
        return cdk_simhook_read(unit, 2, addr + adx, vptr,
                                CDK_WORDS2BYTES(size)); 
    }

    /* Configure S-Channel parameters */
    srcblk = CDK_XGS_CMIC_BLOCK(unit);
    dstblk = CDK_XGS_ADDR2BLOCK(addr);
    datalen = entry_dw * sizeof(uint32_t);
    if (CDK_XGS_FLAGS(unit) & CDK_XGS_CHIP_FLAG_SCHAN_SB0) {
        srcblk = 0;
    }
    if (CDK_XGS_FLAGS(unit) & CDK_XGS_CHIP_FLAG_SCHAN_MBI) {
        datalen = 0;
	addr &= 0x3f0fffff;
    }

    /*
     * Setup S-Channel command packet
     *
     * NOTE: the datalen field matters only for the Write Memory and
     * Write Register commands, where it is used only by the CMIC to
     * determine how much data to send, and is in units of bytes.
     */

    SCHAN_MSG_CLEAR(&schan_msg);
    SCMH_OPCODE_SET(schan_msg.readcmd.header, READ_MEMORY_CMD_MSG);
    SCMH_SRCBLK_SET(schan_msg.readcmd.header, srcblk);
    SCMH_DSTBLK_SET(schan_msg.readcmd.header, dstblk);
    SCMH_DATALEN_SET(schan_msg.readcmd.header, datalen);
    schan_msg.readcmd.address = addr + adx; 

    /* Issue SCHAN op */
    rv = cdk_xgs_schan_op(unit, &schan_msg, 2, 1 + entry_dw);
    if (CDK_FAILURE(rv)) {
        CDK_ERR(("cdk_xgs_mem_read[%d]: S-channel error addr=0x%08"PRIx32"\n",
                 unit, addr));
	return rv; 
    }
    
    /* Check for errors */
    if (SCMH_OPCODE_GET(schan_msg.readresp.header) != READ_MEMORY_ACK_MSG) {
        CDK_ERR(("cdk_xgs_mem_read[%d]: Invalid S-channel ACK: %"PRIu32""
                 " (expected %d) addr=0x%08"PRIx32"\n", unit,
                 SCMH_OPCODE_GET(schan_msg.readresp.header),
                 READ_MEMORY_ACK_MSG, addr));
	return CDK_E_FAIL; 
    }

    /* Copy the data out */
    CDK_DEBUG_MEM(("cdk_xgs_mem_read[%d]: addr=0x%08"PRIx32" idx=%"PRIx32" data:",
                   unit, addr, idx));
    for (i = 0; i < entry_dw; i++) {
	entry_data[i] = schan_msg.readresp.data[i]; 
        CDK_DEBUG_MEM((" 0x%08"PRIx32"", entry_data[i]));
    }    
    CDK_DEBUG_MEM(("\n"));

    return CDK_E_NONE; 
}   

int
cdk_xgs_mem_write(int unit, uint32_t addr, uint32_t idx, void* vptr, int size)
{
    int rv; 
    int srcblk, dstblk, datalen;
    schan_msg_t schan_msg;
    int i;
    uint32_t* entry_data = (uint32_t*)vptr; 
    int entry_dw = size;
    int adx = idx;

    CDK_ASSERT(CDK_DEV_EXISTS(unit)); 

    if (adx & CDK_XGS_MEM_FLAG_STEP2) {
        adx <<= 1;
        idx &= (CDK_XGS_MEM_FLAG_STEP2 - 1);
    }
    
    /* Simulator hooks */
    if (cdk_simhook_write) {
        return cdk_simhook_write(unit, 2, addr + adx, vptr,
                                 CDK_WORDS2BYTES(size)); 
    }

    /* Configure S-Channel parameters */
    srcblk = CDK_XGS_CMIC_BLOCK(unit);
    dstblk = CDK_XGS_ADDR2BLOCK(addr);
    datalen = entry_dw * sizeof(uint32_t);
    if (CDK_XGS_FLAGS(unit) & CDK_XGS_CHIP_FLAG_SCHAN_SB0) {
        srcblk = 0;
    }
    if (CDK_XGS_FLAGS(unit) & CDK_XGS_CHIP_FLAG_SCHAN_MBI) {
	addr &= 0x3f0fffff;
    }

    /*
     * Setup S-Channel command packet
     *
     * NOTE: the datalen field matters only for the Write Memory and
     * Write Register commands, where it is used only by the CMIC to
     * determine how much data to send, and is in units of bytes.
     */

    SCHAN_MSG_CLEAR(&schan_msg);
    SCMH_OPCODE_SET(schan_msg.writecmd.header, WRITE_MEMORY_CMD_MSG);
    SCMH_SRCBLK_SET(schan_msg.writecmd.header, srcblk);
    SCMH_DSTBLK_SET(schan_msg.writecmd.header, dstblk);
    SCMH_DATALEN_SET(schan_msg.writecmd.header, datalen);
    
    CDK_DEBUG_MEM(("cdk_xgs_mem_write[%d]: addr=0x%08"PRIx32" idx=%"PRIx32" data:",
                   unit, addr, idx));
    for (i = 0; i < entry_dw; i++) {
	schan_msg.writecmd.data[i] = entry_data[i]; 
        CDK_DEBUG_MEM((" 0x%08"PRIx32"", entry_data[i]));
    }
    CDK_DEBUG_MEM(("\n"));

    schan_msg.writecmd.address = addr + adx; 

    /* 
     * Write header + address + entry_dw data DWORDs
     * Note: The hardware does not send WRITE_MEMORY_ACK_MSG. 
     */
    rv = cdk_xgs_schan_op(unit, &schan_msg, 2 + entry_dw, 0); 
    if (CDK_FAILURE(rv)) {
        CDK_ERR(("cdk_xgs_reg_read[%d]: S-channel error addr=0x%08"PRIx32"\n",
                 unit, addr));
    }
    return rv;
}
