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
#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56670_A0 == 1
 
#include "bcm56670_a0_tdm_core_top.h"
#include "bcm56670_a0_tdm_chip_top.h"
#include "bcm56670_a0_tdm_chip_methods.h"

size_t stack_size = 0;

#ifndef _TDM_STANDALONE
/* Initialize TDM internal driving functions */
tdm_mod_t
*SOC_SEL_TDM(tdm_soc_t *chip, tdm_mod_t *_tdm)
{
    int (*core_exec[TDM_EXEC_CORE_SIZE])( tdm_mod_t* ) = {
        &tdm_core_init,
        &tdm_core_post,
        &tdm_mn_proc_cal,
        &tdm_core_vbs_scheduler_wrapper,
        &tdm_core_vbs_scheduler_ovs,
        &tdm_core_null, /* TDM_CORE_EXEC__EXTRACT */
        &tdm_core_null, /* TDM_CORE_EXEC__FILTER */
        &tdm_core_acc_alloc,
        &tdm_core_vmap_prealloc,
        &tdm_core_vmap_alloc,
        &tdm_core_null,
        &tdm_core_null,
        &tdm_core_null,
        &tdm_mn_check_ethernet,
        &tdm_pick_vec,
        &tdm_mn_which_tsc
    };
    int (*chip_exec[TDM_EXEC_CHIP_SIZE])( tdm_mod_t* ) = {
        &tdm_mn_init,
        &tdm_mn_pmap_transcription,
        &tdm_mn_lls_wrapper,
        &tdm_mn_vbs_wrapper,
        &tdm_mn_filter_chain,
        &tdm_mn_parse_mmu_tdm_tbl,
        &tdm_mn_proc_cal_ancl,
        &tdm_core_null,
        &tdm_core_null,
        &tdm_core_null,
        &tdm_core_null,
        &tdm_core_null,
        &tdm_core_null,
        &tdm_mn_free,
        &tdm_mn_corereq,
        &tdm_mn_post
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
	
    /* stack_size = (size_t)&idx; */
    
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
#endif /* CDK_CONFIG_INCLUDE_BCM56670_A0 */
