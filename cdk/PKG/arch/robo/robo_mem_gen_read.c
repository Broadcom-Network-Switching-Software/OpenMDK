/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * ROBO ARL access through debug memory interface.
 */

#include <cdk/cdk_device.h>
#include <cdk/cdk_assert.h>
#include <cdk/cdk_debug.h>
#include <cdk/cdk_field.h>

#include <cdk/arch/robo_mem_regs.h>
#include <cdk/arch/robo_mem.h>

#define MAX_POLL 20

int
cdk_robo_mem_gen_read(int unit, uint32_t addr, uint32_t idx, void *vptr, int size)
{
    int ioerr = 0;
    int wsize = CDK_BYTES2WORDS(size);
    ROBO_MEM_INDEXr_t mem_index;
    ROBO_MEM_ADDR_0r_t mem_addr_0;
    ROBO_MEM_CTRLr_t mem_ctrl;
    uint32_t *wdata = (uint32_t *)vptr;
    uint32_t data_reg;
    uint32_t index_reg, mem_type, addr_reg, ctrl_reg, data_0_reg, data_1_reg;
    int cnt;
    int bin, bin_no;
    uint32_t data[4];
    /* Initialize access register addresses */
    index_reg = addr & 0xffff;
    mem_type = (addr >> 16) & 0xff;
    
    addr_reg = index_reg + 0x10;
    ctrl_reg = index_reg + 0x08;
    data_0_reg = addr_reg + 0x10;
    data_1_reg = data_0_reg + 0x08;
    /* Clear data */
    CDK_MEMSET(data, 0, sizeof(data));
    ioerr += cdk_robo_reg_write(unit, data_0_reg, data, 8);
    if (size > 8) {
        ioerr += cdk_robo_reg_write(unit, data_1_reg, data, 8);
    }
    /* Set entry (idx) */
    bin_no = (addr >> 24) & 0xf;
    if (bin_no == 0) {
        bin = idx;
    } else {
        /* arl index size = 16K */
        bin = idx >> 2;
    }
    /* Set memory index */
    ROBO_MEM_INDEXr_INDEXf_SET(mem_index, mem_type);
    ioerr += cdk_robo_reg_write(unit, index_reg, &mem_index, 1);

    /* Set MEM_ADDR_0 to read, Addr = 0x10 */
    ioerr += cdk_robo_reg_read(unit, addr_reg, &mem_addr_0, 2);
    ROBO_MEM_ADDR_0r_MEM_ADDR_OFFSETf_SET(mem_addr_0, bin);
    ioerr += cdk_robo_reg_write(unit,addr_reg, &mem_addr_0, 2);

    /* Set MEM_CTRL, OP_CMD=0x01 MEM_STDN=1 */
    ioerr += cdk_robo_reg_read(unit, ctrl_reg, &mem_ctrl, 1);
    ROBO_MEM_CTRLr_OP_CMDf_SET(mem_ctrl, 0x01);
    ROBO_MEM_CTRLr_MEM_STDNf_SET(mem_ctrl, 1);
    ioerr += cdk_robo_reg_write(unit, ctrl_reg, &mem_ctrl, 1);
        
    cnt = 0;
    while (cnt < MAX_POLL) {
        ioerr += cdk_robo_reg_read(unit, ctrl_reg, &mem_ctrl, 1);
        if (ioerr == 0 && 
            ROBO_MEM_CTRLr_MEM_STDNf_GET(mem_ctrl) == 0) {
            /* Read data register(s) */
            if (bin_no == 0) {
                data_reg = data_0_reg;
            } else {
                data_reg = data_0_reg + ((idx & 0x11) * 0x10);
            }

            ioerr += cdk_robo_reg_read(unit, data_reg, data, 8);
            if (size > 8) {
                ioerr += cdk_robo_reg_read(unit, data_reg + 0x08, &data[2], 8);
            }
            break;
        }
    }

    /* Check for errors */
    if (ioerr || cnt >= MAX_POLL) {
        CDK_ERR(("cdk_robo_mem_gen_read[%d]: error reading addr=%08"PRIx32"\n",
                 unit, addr));
        return CDK_E_FAIL;
    }

    /* Debug output */
    CDK_DEBUG_MEM(("cdk_robo_mem_gen_read[%d]: addr=0x%08"PRIx32" idx=%"PRIu32" data:",
                   unit, addr, idx));

    for (cnt = 0; cnt < wsize; cnt++) {
        wdata[cnt] = data[cnt];
        CDK_DEBUG_MEM((" 0x%08"PRIx32, wdata[cnt]));
    }

    CDK_DEBUG_MEM(("\n"));

    return CDK_E_NONE;
}
