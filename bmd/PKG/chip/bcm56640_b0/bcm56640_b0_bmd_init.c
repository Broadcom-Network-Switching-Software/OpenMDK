#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56640_B0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>
#include "../bcm56640_a0/bcm56640_a0_bmd.h"

#include <bmdi/arch/xgsm_dma.h>

#include <cdk/chip/bcm56640_b0_defs.h>

#include "bcm56640_b0_bmd.h"

#define CMIC_NUM_PKT_DMA_CHAN           4

int 
bcm56640_b0_bmd_init(int unit)
{
    int rv ;

    rv = bcm56640_a0_bmd_init(unit);

#if BMD_CONFIG_INCLUDE_DMA
    /*
     * Enable only 45 CPU COS queues for Rx DMA channel, as the last
     * three queues are reserved for BP (back-pressure) control.
     * Enable BP control queues for CMC0
     */
    if (CDK_SUCCESS(rv)) {
        int ioerr = 0;
        CMIC_CMC_COS_CTRL_RX_0r_t cos_ctrl_0;
        CMIC_CMC_COS_CTRL_RX_1r_t cos_ctrl_1;
        int idx, cmc;
        uint32_t cos_bmp;

        CMIC_CMC_COS_CTRL_RX_0r_CLR(cos_ctrl_0);
        for (idx = 0; idx < CMIC_NUM_PKT_DMA_CHAN; idx++) {
            cos_bmp = (idx == XGSM_DMA_RX_CHAN) ? 0xffffffff : 0;
            CMIC_CMC_COS_CTRL_RX_0r_COS_BMPf_SET(cos_ctrl_0, cos_bmp);
            ioerr += WRITE_CMIC_CMC_COS_CTRL_RX_0r(unit, idx, cos_ctrl_0);
        }

        cmc = CDK_XGSM_CMC_GET(unit);
        CMIC_CMC_COS_CTRL_RX_1r_CLR(cos_ctrl_1);
        for (idx = 0; idx < CMIC_NUM_PKT_DMA_CHAN; idx++) {
            cos_bmp = (idx == XGSM_DMA_RX_CHAN) ? 0x1fff : 0;
            cos_bmp |= (1 << (13 + cmc));
            CMIC_CMC_COS_CTRL_RX_1r_COS_BMPf_SET(cos_ctrl_1, cos_bmp);
            ioerr += WRITE_CMIC_CMC_COS_CTRL_RX_1r(unit, idx, cos_ctrl_1);
        }

        if (ioerr) {
            return CDK_E_IO;
        }
    }
#endif

    return rv;
}
#endif /* CDK_CONFIG_INCLUDE_BCM56640_B0 */

