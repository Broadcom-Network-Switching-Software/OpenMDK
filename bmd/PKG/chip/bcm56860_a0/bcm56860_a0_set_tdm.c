/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 * All Rights Reserved.$
 *
 * File:        set_tdm.c
 * Purpose:     TDM algorithm
 */
#include <cdk/cdk_string.h>

#include "bcm56860_a0_tdm_core_top.h"
#include "bcm56860_a0_tdm_chip_top.h"

#ifndef _TDM_STANDALONE
tdm_mod_t
*SOC_SEL_TDM(tdm_soc_t *chip, tdm_mod_t *_tdm)
{
    int (*core_exec[TDM_EXEC_CORE_SIZE])( tdm_mod_t* ) = {
        &tdm_core_init,
        &tdm_core_post,
        &tdm_td2p_vmap_alloc,
        &tdm_core_vbs_scheduler,
        &tdm_chip_td2p_shim__core_vbs_scheduler_ovs,
        &tdm_core_null, /* TDM_CORE_EXEC__EXTRACT */
        &tdm_core_null, /* TDM_CORE_EXEC__FILTER */
        &tdm_core_acc_alloc,
        &tdm_core_null,
        &tdm_core_null,
        &tdm_core_null,
        &tdm_core_null,
        &tdm_core_null,
        &tdm_td2p_check_ethernet,
        &tdm_pick_vec,
        &tdm_td2p_which_tsc
    };
    int (*chip_exec[TDM_EXEC_CHIP_SIZE])( tdm_mod_t* ) = {
        &tdm_td2p_init,
        &tdm_td2p_pmap_transcription,
        &tdm_td2p_lls_wrapper,
        &tdm_td2p_vbs_wrapper,
        &tdm_td2p_filter_chain,
        &tdm_td2p_parse_mmu_tdm_tbl,
        &tdm_core_null,
        &tdm_core_null,
        &tdm_core_null,
        &tdm_core_null,
        &tdm_core_null,
        &tdm_core_null,
        &tdm_core_null,
        &tdm_td2p_free,
        &tdm_td2p_corereq,
        &tdm_td2p_post
    };

    if (!_tdm) {
        return NULL;
    }
    _tdm->_chip_data.soc_pkg = (*chip);

    TDM_COPY(_tdm->_core_exec,core_exec,sizeof(_tdm->_core_exec));
    TDM_COPY(_tdm->_chip_exec,chip_exec,sizeof(_tdm->_chip_exec));

    return _tdm;
}
#endif


tdm_mod_t
*_soc_set_tdm_tbl( tdm_mod_t *_tdm )
{
    int idx, tdm_ver_chk[8];

    if (!_tdm) {
        return NULL;
    }
    TDM_BIG_BAR
    TDM_PRINT0("TDM: Release version: ");
    _soc_tdm_ver(tdm_ver_chk);
    TDM_PRINT2("%d%d",tdm_ver_chk[0],tdm_ver_chk[1]);
    for (idx=2; idx<8; idx+=2) {
        TDM_PRINT2(".%d%d",tdm_ver_chk[idx],tdm_ver_chk[idx+1]);
    }
    TDM_PRINT0("\n"); TDM_SML_BAR

    /* Path virtualized API starting in chip executive */
    return ((_tdm->_chip_exec[TDM_CHIP_EXEC__INIT](_tdm))==PASS)?(_tdm):(NULL);
}
