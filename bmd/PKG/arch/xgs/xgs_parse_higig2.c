/*
 *
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#ifdef CDK_CONFIG_ARCH_XGS_INSTALLED

#include <bmd/bmd_device.h>

#include <bmdi/arch/xgs_dma.h>

#include <cdk/cdk_debug.h>

#if BMD_CONFIG_INCLUDE_HIGIG

/*
 * Function:
 *	bmd_xgs_parse_higig2
 * Purpose:
 *	Parse HiGig2 header into bmd_pkt_t.
 * Parameters:
 *	unit - BMD device
 *	pkt - BMD packet structure
 *	mh - HiGig2 module header (host format)
 * Returns:
 *      CDK_XXX
 */
int
bmd_xgs_parse_higig2(int unit, bmd_pkt_t *pkt, uint32_t *mh)
{
    int rv = CDK_E_NONE;
    HIGIG2_t *hg2_mh = (HIGIG2_t *)mh;
    int opcode;

    pkt->mh_src_mod = HIGIG2_SRC_MODIDf_GET(*hg2_mh);
    pkt->mh_src_port = HIGIG2_SRC_PIDf_GET(*hg2_mh);

    pkt->mh_dst_mod = HIGIG2_DST_MODID_MGIDHf_GET(*hg2_mh);
    pkt->mh_dst_port = HIGIG2_DST_PORT_MGIDLf_GET(*hg2_mh);

    opcode = HIGIG2_PPD0_OPCODEf_GET(*hg2_mh);
    pkt->mh_pkt_type = BMD_PKT_TYPE_FROM_HIGIG(opcode);

    return rv;
}

#endif
#endif /* CDK_CONFIG_ARCH_XGS_INSTALLED */
