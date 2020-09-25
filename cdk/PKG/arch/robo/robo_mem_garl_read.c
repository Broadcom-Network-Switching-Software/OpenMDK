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
cdk_robo_mem_garl_read(int unit, uint32_t addr, uint32_t idx, void *vptr, int size)
{
    int ioerr = 0;
    int wsize = CDK_BYTES2WORDS(size);
    uint32_t *wdata = (uint32_t *)vptr;
    uint32_t data_reg, mem_type;
    ROBO_OTHER_TABLE_INDEXr_t tidx;
    ROBO_ARLA_RWCTLr_t arla_rwctl;
    int cnt;
    uint32_t data[6];

    data_reg = addr & 0xffff;
    mem_type = (addr >> 16) & 0xff;

    /* Clear data registers */
    CDK_MEMSET(data, 0, sizeof(data));
    ioerr += cdk_robo_reg_write(unit, data_reg, data, 8);
    if (size > 8) {
        ioerr += cdk_robo_reg_write(unit, data_reg + 8, data, 8);
        if (size > 16) {
            ioerr += cdk_robo_reg_write(unit, data_reg + 16, data, 8);
        }
    }

    /* Set memory index */
    ROBO_OTHER_TABLE_INDEXr_SET(tidx, idx);
    ioerr += ROBO_WRITE_OTHER_TABLE_INDEXr(unit, tidx);

    /* Initialize read operation */
    ROBO_ARLA_RWCTLr_CLR(arla_rwctl);
    ROBO_ARLA_RWCTLr_TAB_RWf_SET(arla_rwctl, ROBO_MEM_OP_READ);
    ROBO_ARLA_RWCTLr_TAB_INDEXf_SET(arla_rwctl, mem_type);
    ROBO_ARLA_RWCTLr_ARL_STRTDNf_SET(arla_rwctl, 1);
    ioerr += ROBO_WRITE_ARLA_RWCTLr(unit, arla_rwctl);

    cnt = 0;
    while (++cnt < MAX_POLL) {
        ioerr += ROBO_READ_ARLA_RWCTLr(unit, &arla_rwctl);
        if (ioerr == 0 && ROBO_ARLA_RWCTLr_ARL_STRTDNf_GET(arla_rwctl) == 0) {
            /* Read data register(s) */
            ioerr += cdk_robo_reg_read(unit, data_reg, data, 8);
            if (size > 8) {
                ioerr += cdk_robo_reg_read(unit, data_reg + 8, &data[2], 8);
                if (size > 16) {
                    ioerr += cdk_robo_reg_read(unit, data_reg + 16, &data[4], 8);
                }
            }
            break;
        }
    }

    /* Check for errors */
    if (ioerr || cnt >= MAX_POLL) {
        CDK_ERR(("cdk_robo_mem_garl_read[%d]: error reading addr=%08"PRIx32"\n",
                 unit, addr));
        return CDK_E_FAIL;
    }

    /* Debug output */
    CDK_DEBUG_MEM(("cdk_robo_mem_garl_read[%d]: addr=0x%08"PRIx32" idx=%"PRIu32" data:",
                   unit, addr, idx));

    /* Copy data to output buffer */
    for (cnt = 0; cnt < wsize; cnt++) {
        wdata[cnt] = data[cnt];
        CDK_DEBUG_MEM((" 0x%08"PRIx32, wdata[cnt]));
    }
    CDK_DEBUG_MEM(("\n"));

    return CDK_E_NONE;
}
