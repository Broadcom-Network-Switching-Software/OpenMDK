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

#include <cdk/cdk_debug.h>

#if BMD_CONFIG_INCLUDE_HIGIG == 1

/*
 * Function:
 *	bmd_xgsm_parse_higig
 * Purpose:
 *	Parse HiGig header into bmd_pkt_t.
 * Parameters:
 *	unit - BMD device
 *	pkt - BMD packet structure
 *	mh - HiGig/HiGig+ module header (host format)
 * Returns:
 *      CDK_XXX
 */
int
bmd_xgsm_parse_higig(int unit, bmd_pkt_t *pkt, uint32_t *mh)
{
    int rv = CDK_E_NONE;
    HIGIG_t *hg_mh = (HIGIG_t *)mh;

    pkt->mh_src_mod = HIGIG_SRC_MODID_LSf_GET(*hg_mh);
    pkt->mh_src_mod |= (HIGIG_SRC_MODID_5f_GET(*hg_mh) << 5);
    pkt->mh_src_mod |= (HIGIG_SRC_MODID_6f_GET(*hg_mh) << 6);
    pkt->mh_src_port = HIGIG_SRC_PORT_TGIDf_GET(*hg_mh);

    pkt->mh_dst_mod = HIGIG_DST_MODID_LSf_GET(*hg_mh);
    pkt->mh_dst_mod |= (HIGIG_DST_MODID_5f_GET(*hg_mh) << 5);
    pkt->mh_dst_mod |= (HIGIG_DST_MODID_6f_GET(*hg_mh) << 6);
    pkt->mh_dst_port = HIGIG_DST_PORTf_GET(*hg_mh);

    pkt->mh_pkt_type = BMD_PKT_TYPE_FROM_HIGIG(HIGIG_OPCODEf_GET(*hg_mh));

    return rv;
}

#endif
#endif /* CDK_CONFIG_ARCH_XGSM_INSTALLED */
