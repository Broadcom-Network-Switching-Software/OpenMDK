/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <cdk/cdk_device.h>
#include <cdk/cdk_debug.h>

#include <cdk/arch/xgsd_schan.h>
#include <cdk/arch/xgsd_chip.h>
#include <cdk/arch/xgsd_mem.h>

#include <cdk/arch/xgsd_schan.h>

#if 0
#include <cdk/chip/bcm56624_b0_defs.h>
#endif

int
cdk_xgsd_mem_op(int unit, cdk_xgsd_mem_op_info_t *moi)
{
#if 0
    int rv;
    schan_msg_t schan_msg;
    int dstblk;
    int rsp_type;

    if (CDK_XGSD_INFO(unit)->mem_op) {
        return CDK_XGSD_INFO(unit)->mem_op(unit, moi);
    }

    if (moi == NULL) {
        return CDK_E_PARAM;
    }

    dstblk = (moi->addr >> CDK_XGSD_BLOCK_BP) & 0xf;

    if (moi->mem_op == CDK_XGSD_MEM_OP_INSERT) {
        SCHAN_MSG_CLEAR(&schan_msg);
        SCMH_OPCODE_SET(schan_msg.gencmd.header, TABLE_INSERT_CMD_MSG);
        SCMH_SRCBLK_SET(schan_msg.gencmd.header, CDK_XGSD_CMIC_BLOCK(unit)); 
        SCMH_DSTBLK_SET(schan_msg.gencmd.header, dstblk); 
        SCMH_DATALEN_SET(schan_msg.gencmd.header, moi->size * 4); 
        CDK_MEMCPY(schan_msg.gencmd.data, moi->data, moi->size * 4);
        schan_msg.gencmd.address = moi->addr;
        return cdk_xgsd_schan_op(unit, &schan_msg, moi->size + 2, 1);
    }
    if (moi->mem_op == CDK_XGSD_MEM_OP_DELETE) {
        SCHAN_MSG_CLEAR(&schan_msg);
        SCMH_OPCODE_SET(schan_msg.gencmd.header, TABLE_DELETE_CMD_MSG);
        SCMH_SRCBLK_SET(schan_msg.gencmd.header, CDK_XGSD_CMIC_BLOCK(unit)); 
        SCMH_DSTBLK_SET(schan_msg.gencmd.header, dstblk); 
        SCMH_DATALEN_SET(schan_msg.gencmd.header, moi->size * 4); 
        CDK_MEMCPY(schan_msg.gencmd.data, moi->data, moi->size * 4);
        schan_msg.gencmd.address = moi->addr;
        return cdk_xgsd_schan_op(unit, &schan_msg, moi->size + 2, 1);
    }
    if (moi->mem_op == CDK_XGSD_MEM_OP_LOOKUP) {
        SCHAN_MSG_CLEAR(&schan_msg);
        SCMH_OPCODE_SET(schan_msg.gencmd.header, TABLE_LOOKUP_CMD_MSG);
        SCMH_SRCBLK_SET(schan_msg.gencmd.header, CDK_XGSD_CMIC_BLOCK(unit)); 
        SCMH_DSTBLK_SET(schan_msg.gencmd.header, dstblk); 
        SCMH_DATALEN_SET(schan_msg.gencmd.header, moi->size * 4); 
        CDK_MEMCPY(schan_msg.gencmd.data, moi->key, moi->size * 4);
        schan_msg.gencmd.address = moi->addr;
        rv = cdk_xgsd_schan_op(unit, &schan_msg, moi->size + 2, moi->size + 2);
        if (CDK_SUCCESS(rv)) {
            rsp_type = SCGR_TYPE_GET(schan_msg.genresp.response);
            if (rsp_type == SCGR_TYPE_NOT_FOUND) {
                return CDK_E_NOT_FOUND;
            }
            moi->idx_min = SCGR_INDEX_GET(schan_msg.genresp.response);
            if (moi->data != NULL) {
                CDK_MEMCPY(moi->data, schan_msg.genresp.data, moi->size * 4);
            }
        }
        return rv;
    }
    if (moi->mem_op == CDK_XGSD_MEM_OP_PUSH) {
        SCHAN_MSG_CLEAR(&schan_msg);
        SCMH_OPCODE_SET(schan_msg.pushcmd.header, FIFO_PUSH_CMD_MSG);
        SCMH_SRCBLK_SET(schan_msg.pushcmd.header, CDK_XGSD_CMIC_BLOCK(unit)); 
        SCMH_DSTBLK_SET(schan_msg.pushcmd.header, dstblk); 
        SCMH_DATALEN_SET(schan_msg.pushcmd.header, moi->size * 4); 
        schan_msg.pushcmd.address = moi->addr;
        CDK_MEMCPY(schan_msg.pushcmd.data, moi->data, moi->size * 4);
        rv = cdk_xgsd_schan_op(unit, &schan_msg, moi->size + 2, 1);
        if (CDK_SUCCESS(rv) && SCMH_CPU_GET(schan_msg.pushresp.header)) {
            rv = CDK_E_FULL;
        }
        return rv;
    }
    if (moi->mem_op == CDK_XGSD_MEM_OP_POP) {
        SCHAN_MSG_CLEAR(&schan_msg);
        SCMH_OPCODE_SET(schan_msg.popcmd.header, FIFO_POP_CMD_MSG);
        SCMH_SRCBLK_SET(schan_msg.popcmd.header, CDK_XGSD_CMIC_BLOCK(unit)); 
        SCMH_DSTBLK_SET(schan_msg.popcmd.header, dstblk); 
        SCMH_DATALEN_SET(schan_msg.popcmd.header, 0); 
        schan_msg.popcmd.address = moi->addr;
        rv = cdk_xgsd_schan_op(unit, &schan_msg, 2, moi->size + 1);
        if (CDK_SUCCESS(rv)) {
            if (SCMH_CPU_GET(schan_msg.popresp.header)) {
                rv = CDK_E_NOT_FOUND;
            } else {
                CDK_MEMCPY(moi->data, schan_msg.popresp.data, moi->size * 4);
            }
        }
        return rv;
    }
#endif
    return CDK_E_UNAVAIL;
}
