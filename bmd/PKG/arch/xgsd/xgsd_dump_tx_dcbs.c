/*
 *
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#ifdef CDK_CONFIG_ARCH_XGSD_INSTALLED

#include <bmd/bmd_device.h>

#include <bmdi/arch/xgsd_dma.h>

#include <cdk/cdk_higig_defs.h>
#include <cdk/cdk_debug.h>

#include <cdk/arch/xgsd_chip.h>

#if CDK_CONFIG_INCLUDE_DEBUG == 1

static void
_dump_words(char *prefix, uint32_t *wdata, int cnt, int offs, int incr)
{
    int idx;

    if (cnt == 0) {
        return;
    }

    CDK_DEBUG_DMA((prefix));
    for (idx = 0; idx < cnt; idx++, offs += incr) {
        CDK_DEBUG_DMA((" %08"PRIx32, wdata[offs]));
    }
    CDK_DEBUG_DMA(("\n"));
}

/*
 * Function:
 *	bmd_xgsd_dump_tx_dcbs
 * Purpose:
 *	Dump Tx DMA control block information
 * Parameters:
 *	unit - BMD device
 *	dcbs - DCBs as sequential word arrays
 *	dcb_cnt - Number of DCBs
 *	dcb_size - Size of each DCB (in words)
 *	mh_size - Size of module header (in words)
 * Returns:
 *      CDK_XXX
 */
int
bmd_xgsd_dump_tx_dcbs(int unit, uint32_t *dcbs, int dcb_cnt,
                     int dcb_size, int mh_size)
{
    /* Dump DMA descriptor */
    _dump_words("Tx DMA ctrl =", dcbs, dcb_cnt, 1, dcb_size);
    _dump_words("Tx DMA stat =", dcbs, dcb_cnt, dcb_size-1, dcb_size);

#if CDK_CONFIG_INCLUDE_FIELD_INFO == 1
    if (CDK_DEBUG_CHECK(CDK_DBG_DMA | CDK_DBG_VVERBOSE)) {
        int idx;
        /* Decode DMA descriptors */
        for (idx = 0; idx < dcb_cnt; idx++) {
            CDK_DEBUG_DMA(("Tx DCB[%d]:\n", idx));
            cdk_symbol_dump("TX_DCB", CDK_XGSD_SYMBOLS(unit),
                            &dcbs[(idx*dcb_size)]);
        }
    }
#endif

    /* Dump module header if supplied */
    _dump_words("Tx DMA mhdr =", dcbs, mh_size, 2, 1);

#if CDK_CONFIG_INCLUDE_FIELD_INFO == 1
    if (mh_size > 0 &&
        ((dcbs[2] >> 24) == 0xfb || (dcbs[2] >> 24) == 0xfc) &&
        CDK_DEBUG_CHECK(CDK_DBG_DMA | CDK_DBG_HIGIG)) {
        char *sym_name = (mh_size == CDK_HIGIG2_WSIZE) ? "HIGIG2" : "HIGIG";
        /* Decode module header */
        CDK_DEBUG_DMA(("%s module header:\n", sym_name));
        cdk_symbol_dump(sym_name, &higig_symbols, &dcbs[6]); 
    }
#endif

    return 0;
}

#endif
#endif /* CDK_CONFIG_ARCH_XGSD_INSTALLED */
