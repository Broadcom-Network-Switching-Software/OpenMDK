/*
 *
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#ifdef CDK_CONFIG_ARCH_XGSM_INSTALLED

#include <bmd/bmd_device.h>

#include <bmdi/arch/xgsm_dma.h>

#include <cdk/cdk_higig_defs.h>
#include <cdk/cdk_debug.h>

#include <cdk/arch/xgsm_chip.h>

#if CDK_CONFIG_INCLUDE_DEBUG == 1

static void
_dump_words(char *prefix, uint32_t *wdata, int wsize)
{
    int idx;

    if (wsize == 0) {
        return;
    }

    CDK_DEBUG_DMA((prefix));
    for (idx = 0; idx < wsize; idx++) {
        CDK_DEBUG_DMA((" %08"PRIx32, wdata[idx]));
    }
    CDK_DEBUG_DMA(("\n"));
}

/*
 * Function:
 *	bmd_xgsm_dump_rx_dcb
 * Purpose:
 *	Dump Rx DMA control block information
 * Parameters:
 *	unit - BMD device
 *	dcb - DCB as word array
 *	dcb_size - Size of DCB (in words)
 *	mh_size - Size of module header (in words)
 * Returns:
 *      CDK_XXX
 */
int
bmd_xgsm_dump_rx_dcb(int unit, uint32_t *dcb,
                    int dcb_size, int mh_size)
{
    /* Dump DMA descriptor */
    _dump_words("Rx DMA ctrl =", &dcb[1], 1);
    _dump_words("Rx DMA stat =", &dcb[dcb_size-1], 1);
    _dump_words("Rx pkt stat =", &dcb[6+mh_size], dcb_size - mh_size - 3);

#if CDK_CONFIG_INCLUDE_FIELD_INFO == 1
    if (CDK_DEBUG_CHECK(CDK_DBG_DMA | CDK_DBG_VVERBOSE)) {
        /* Decode DMA descriptor */
        CDK_DEBUG_DMA(("Rx DCB[0]:\n"));
        cdk_symbol_dump("RX_DCB", CDK_XGSM_SYMBOLS(unit), dcb); 
    }
#endif

    /* Dump module header if supplied */
    _dump_words("Rx DMA mhdr =", &dcb[6], mh_size);

#if CDK_CONFIG_INCLUDE_FIELD_INFO == 1
    if (mh_size > 0 &&
        CDK_DEBUG_CHECK(CDK_DBG_DMA | CDK_DBG_HIGIG)) {
        char *sym_name = (mh_size == CDK_HIGIG2_WSIZE) ? "HIGIG2" : "HIGIG";
        /* Decode module header */
        CDK_DEBUG_DMA(("%s module header:\n", sym_name));
        cdk_symbol_dump(sym_name, &higig_symbols, &dcb[6]); 
    }
#endif

    return 0;
}

#endif
#endif /* CDK_CONFIG_ARCH_XGS_INSTALLED */
