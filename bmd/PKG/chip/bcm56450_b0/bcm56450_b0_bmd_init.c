#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56450_B0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>
#include <bmd/bmd_device.h>
#include <cdk/arch/xgsm_chip.h>
#include <cdk/cdk_debug.h>
#include <cdk/chip/bcm56450_a0_defs.h>
#include <bmd/bmd_phy_ctrl.h>
#include <bmdi/arch/xgsm_dma.h>
#include "../bcm56450_a0/bcm56450_a0_bmd.h"
#include "bcm56450_b0_bmd.h"

int
bcm56450_b0_bmd_init(int unit)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    
    rv = bcm56450_a0_bmd_init(unit);    

#if BMD_CONFIG_INCLUDE_DMA
    if (CDK_SUCCESS(rv)) {
        if (CDK_DEV_FLAGS(unit) & CDK_DEV_MBUS_PCI) {
            CMIC_CMC_HOSTMEM_ADDR_REMAPr_t hostmem_remap;
            CMIC_CMC0_PCIE_MISCELr_t cmic_pcie_miscel;
            uint32_t remap_val[] = { 0x248e2860, 0x29a279a5, 0x2eb6caea, 0x2f };
            int idx;

            /* Send DMA data to external host memory when on PCI bus */
            for (idx = 0; idx < COUNTOF(remap_val); idx++) {
                CMIC_CMC_HOSTMEM_ADDR_REMAPr_SET(hostmem_remap, remap_val[idx]);
                ioerr += WRITE_CMIC_CMC_HOSTMEM_ADDR_REMAPr(unit, idx, hostmem_remap);
            }

            ioerr += READ_CMIC_CMC0_PCIE_MISCELr(unit, &cmic_pcie_miscel);
            CMIC_CMC0_PCIE_MISCELr_MSI_ADDR_SELf_SET(cmic_pcie_miscel, 1);
            ioerr += WRITE_CMIC_CMC0_PCIE_MISCELr(unit, cmic_pcie_miscel);
        }
    }
#endif /* BMD_CONFIG_INCLUDE_DMA */

    return ioerr ? CDK_E_IO : rv;
}
#endif /* CDK_CONFIG_INCLUDE_BCM56450_B0 */
