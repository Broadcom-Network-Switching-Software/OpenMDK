/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Common XGS chip functions.
 */

#include <cdk/cdk_assert.h>
#include <cdk/cdk_device.h>
#include <cdk/cdk_chip.h>

#include <cdk/arch/xgsd_cmic.h>

/*
 * Global mode flags for XGS architecture
 */
uint32_t cdk_xgsd_chip_flags[CDK_CONFIG_MAX_UNITS];

/*
 * Basic CMIC Initialization
 */
#define CMIC_BIG_ENDIAN_PIO             0x01000001

uint32_t
cmic_xgsd_endian_flags_get(int unit)
{
    uint32_t flags = 0;

    CDK_ASSERT(CDK_DEV_EXISTS(unit)); 

    flags = CDK_DEV_FLAGS(unit);

    if (CDK_XGSD_FLAGS(unit) & CDK_XGSD_CHIP_FLAG_IPROC) {
        if ((flags & CDK_DEV_MBUS_PCI) && (flags & CDK_DEV_BE_PIO)) {
            /*
             * If BE_PIO is set, then we assume that the iProc
             * PCI bridge will byte-swap all access types,
             * i.e. both PIO and DMA access.
             *
             * In order to avoid double-swapping, we then need to
             * invert the DMA endianness setting in the CMIC.
             */
            flags ^= (CDK_DEV_BE_PIO | CDK_DEV_BE_PACKET | CDK_DEV_BE_OTHER);
        }
    }

    return flags;
}

int 
cdk_xgsd_cmic_init(int unit)
{
    uint32_t flags;
    CMIC_CMC_PCIE_IRQ_MASK0r_t irq_mask0;
    CMIC_CMC_PCIE_IRQ_MASK1r_t irq_mask1;
    CMIC_COMMON_UC0_PIO_ENDIANESSr_t uc0_pio_en; 
    CMIC_COMMON_PCIE_PIO_ENDIANESSr_t pcie_pio_en; 
    int ioerr = 0;

    CDK_ASSERT(CDK_DEV_EXISTS(unit)); 

    /*
     * Certain PCIe cores may occasionally return invalid data in the
     * first PCI read following a soft-reset (CPS reset). The following
     * read operation is a dummy read to ensure that any invalid data
     * is flushed from the PCI read pipeline.
     */
    ioerr += READ_CMIC_CMC_PCIE_IRQ_MASK0r(unit, &irq_mask0); 
    ioerr += READ_CMIC_CMC_PCIE_IRQ_MASK1r(unit, &irq_mask1); 

    /* Configure endian */
    flags = cmic_xgsd_endian_flags_get(unit);
    if (CDK_XGSD_FLAGS(unit) & CDK_XGSD_CHIP_FLAG_IPROC) {
        CMIC_COMMON_UC0_PIO_ENDIANESSr_CLR(uc0_pio_en);
        if (flags & CDK_DEV_BE_PIO) {
            CMIC_COMMON_UC0_PIO_ENDIANESSr_SET(uc0_pio_en, CMIC_BIG_ENDIAN_PIO);
        }
        ioerr += WRITE_CMIC_COMMON_UC0_PIO_ENDIANESSr(unit, uc0_pio_en);
    } else {
        CMIC_COMMON_PCIE_PIO_ENDIANESSr_CLR(pcie_pio_en);
        if (flags & CDK_DEV_BE_PIO) {
            CMIC_COMMON_PCIE_PIO_ENDIANESSr_SET(pcie_pio_en, CMIC_BIG_ENDIAN_PIO);
        }
        ioerr += WRITE_CMIC_COMMON_PCIE_PIO_ENDIANESSr(unit, pcie_pio_en);
    }
    
    /* Disable Interrupts */
    CMIC_CMC_PCIE_IRQ_MASK0r_CLR(irq_mask0);
    ioerr += WRITE_CMIC_CMC_PCIE_IRQ_MASK0r(unit, irq_mask0); 
    CMIC_CMC_PCIE_IRQ_MASK1r_CLR(irq_mask1);
    ioerr += WRITE_CMIC_CMC_PCIE_IRQ_MASK1r(unit, irq_mask1); 
    
    return ioerr; 
}
