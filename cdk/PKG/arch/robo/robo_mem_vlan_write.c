/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * ROBO VLAN access through 32-bit interface.
 */

#include <cdk/cdk_device.h>
#include <cdk/cdk_assert.h>
#include <cdk/cdk_debug.h>
#include <cdk/cdk_field.h>

#include <cdk/arch/robo_mem_regs.h>
#include <cdk/arch/robo_mem.h>

#define MAX_POLL 20

int
cdk_robo_mem_vlan_write(int unit, uint32_t addr, uint32_t idx, void *vptr, int size)
{
    int ioerr = 0;
    int wsize = CDK_BYTES2WORDS(size);
    int bsize = CDK_WORDS2BYTES(wsize);
    uint32_t *wdata = (uint32_t *)vptr;
    uint32_t rwctrl_reg, addr_reg, entry_reg;
    ROBO_ARLA_VTBL_ADDRr_t vtbl_addr;
    ROBO_ARLA_VTBL_RWCTRLr_t vtbl_rwctrl;
    ROBO_ARLA_VTBL_ENTRYr_t vtbl_entry;
    int cnt;

    /* Initialize access register addresses */
    rwctrl_reg = addr & 0xffff;
    addr_reg = rwctrl_reg + 1;
    entry_reg = rwctrl_reg + 3;

    /* Set VLAN memory index */
    ROBO_ARLA_VTBL_ADDRr_CLR(vtbl_addr);
    ROBO_ARLA_VTBL_ADDRr_VTBL_ADDR_INDEXf_SET(vtbl_addr, idx);
    ioerr += cdk_robo_reg_write(unit, addr_reg, &vtbl_addr, 2);

    /* Write 32-bit VLAN entry */
    for (cnt = 0; cnt < wsize; cnt++) {
        ROBO_ARLA_VTBL_ENTRYr_SET(vtbl_entry, cnt, wdata[cnt]);
    }
    ioerr += cdk_robo_reg_write(unit, entry_reg, &vtbl_entry, bsize);

    /* Initialize write operation */
    ROBO_ARLA_VTBL_RWCTRLr_CLR(vtbl_rwctrl);
    ROBO_ARLA_VTBL_RWCTRLr_ARLA_VTBL_RW_CLRf_SET(vtbl_rwctrl, ROBO_MEM_OP_WRITE);
    ROBO_ARLA_VTBL_RWCTRLr_ARLA_VTBL_STDNf_SET(vtbl_rwctrl, 1);
    ioerr += cdk_robo_reg_write(unit, rwctrl_reg, &vtbl_rwctrl, 1);

    cnt = 0;
    while (cnt < MAX_POLL) {
        ioerr += cdk_robo_reg_read(unit, rwctrl_reg, &vtbl_rwctrl, 1);
        if (ioerr == 0 && 
            ROBO_ARLA_VTBL_RWCTRLr_ARLA_VTBL_STDNf_GET(vtbl_rwctrl) == 0) {
            break;
        }
    }

    /* Check for errors */
    if (ioerr || cnt >= MAX_POLL) {
        CDK_ERR(("cdk_robo_mem_vlan_read[%d]: error reading addr=%08"PRIx32"\n",
                 unit, addr));
        return CDK_E_FAIL;
    }

    /* Debug output */
    CDK_DEBUG_MEM(("cdk_robo_mem_vlan_read[%d]: addr=0x%08"PRIx32" idx=%"PRIu32" data:",
                   unit, addr, idx));

    for (cnt = 0; cnt < wsize; cnt++) {
        CDK_DEBUG_MEM((" 0x%08"PRIx32, wdata[cnt]));
    }
    CDK_DEBUG_MEM(("\n"));

    return CDK_E_NONE;
}
