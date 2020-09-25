/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 * All Rights Reserved.$
 *
 * TDM function prototypes for BCM56860
 */
#ifndef __BCM56860_A0_TDM_CHIP_METHODS_H__
#define __BCM56860_A0_TDM_CHIP_METHODS_H__

#ifndef TDM_TD2P_PREPROCESSOR_PROTOTYPES_H
#define TDM_TD2P_PREPROCESSOR_PROTOTYPES_H

#include "bcm56860_a0_tdm_chip_soc.h"
#include "bcm56860_a0_tdm_core_top.h"

/*
 * CHIP function prototypes
 */

/* Functions managed by TDM chip executive */
LINKER_DECL int tdm_td2p_init( tdm_mod_t *_tdm );
LINKER_DECL int tdm_td2p_pmap_transcription( tdm_mod_t *_tdm );
LINKER_DECL int tdm_td2p_lls_wrapper( tdm_mod_t *_tdm );
LINKER_DECL int tdm_td2p_vbs_wrapper( tdm_mod_t *_tdm );
LINKER_DECL int tdm_td2p_extract_cal( tdm_mod_t *_tdm );
LINKER_DECL int tdm_td2p_corereq( tdm_mod_t *_tdm );
LINKER_DECL int tdm_td2p_filter_chain( tdm_mod_t *_tdm );
LINKER_DECL int tdm_td2p_post( tdm_mod_t *_tdm );
LINKER_DECL int tdm_td2p_free( tdm_mod_t *_tdm );

/* Core executive polymorphic functions */
LINKER_DECL int tdm_td2p_which_tsc( tdm_mod_t *_tdm_s );
LINKER_DECL int tdm_td2p_vmap_alloc( tdm_mod_t *_tdm );
LINKER_DECL int tdm_td2p_parse_mmu_tdm_tbl( tdm_mod_t *_tdm );

/* Internalized functions */
LINKER_DECL int tdm_td2p_check_ethernet( tdm_mod_t *_tdm_s );

/* TDM.4 - Trident2+ linked list API */
LINKER_DECL void tdm_td2p_ll_print(struct ll_node *llist);
LINKER_DECL void tdm_td2p_ll_deref(struct ll_node *llist, int *tdm[TD2P_LR_LLS_LEN], int lim);
LINKER_DECL void tdm_td2p_ll_append(struct ll_node *llist, unsigned short port_append, int *pointer);
LINKER_DECL void tdm_td2p_ll_insert(struct ll_node *llist, unsigned short port_insert, int idx);
LINKER_DECL int tdm_td2p_ll_delete(struct ll_node *llist, int idx);
LINKER_DECL int tdm_td2p_ll_get(struct ll_node *llist, int idx);
LINKER_DECL int tdm_td2p_ll_len(struct ll_node *llist);
LINKER_DECL void tdm_td2p_ll_strip(struct ll_node *llist, int *pool, int token);
LINKER_DECL int tdm_td2p_ll_count(struct ll_node *llist, int token);
LINKER_DECL void tdm_td2p_ll_weave(struct ll_node *llist, int wc_array[TD2P_NUM_PHY_PM][TD2P_NUM_PM_LNS], int token);
LINKER_DECL void tdm_td2p_ll_retrace(struct ll_node *llist, int wc_array[TD2P_NUM_PHY_PM][TD2P_NUM_PM_LNS]);
LINKER_DECL int tdm_td2p_ll_single_100(struct ll_node *llist);
LINKER_DECL int tdm_td2p_ll_free(struct ll_node *llist);

/* TDM.4 - Trident2+ chip API */
LINKER_DECL int tdm_td2p_lls_scheduler(struct ll_node *pgw_tdm, tdm_chip_legacy_t *td2p_chip, td2p_pgw_pntrs_t *pntrs_pkg, td2p_pgw_scheduler_vars_t *vars_pkg, int *pgw_tdm_tbl[TD2P_LR_LLS_LEN], int *ovs_tdm_tbl[TD2P_OS_LLS_GRP_LEN], int op_flags[2]);
LINKER_DECL void tdm_td2p_ovs_spacer(int (*wc)[TD2P_NUM_PHY_PM][TD2P_NUM_PM_LNS], int *ovs_tdm_tbl, int *ovs_spacing);
LINKER_DECL int tdm_td2p_legacy_which_tsc(unsigned short port, int **tsc);

/* TDM.4 - Trident2+ chip based prints and parses */
LINKER_DECL void tdm_td2p_print_tbl(int *cal, int len, const char* name, int id);
LINKER_DECL void tdm_td2p_print_tbl_ovs(int *cal, int *spc, int len, const char* name, int id);
LINKER_DECL void tdm_td2p_print_quad(enum port_speed_e *speed, enum port_state_e *state, int len, int idx_start, int idx_end);

/* TDM.4 - Trident2+ chip based checks and scans */
LINKER_DECL int tdm_td2p_check_same_port_dist_dn(int idx, int *tdm_tbl, int lim);
LINKER_DECL int tdm_td2p_check_same_port_dist_up(int idx, int *tdm_tbl, int lim);
LINKER_DECL int tdm_td2p_check_same_port_dist_dn_port(int port, int idx, int *tdm_tbl, int lim);
LINKER_DECL int tdm_td2p_check_same_port_dist_up_port(int port, int idx, int *tdm_tbl, int lim);
LINKER_DECL int tdm_td2p_check_fit_smooth(int *tdm_tbl, int port, int lr_idx_limit, int clump_thresh);
LINKER_DECL int tdm_td2p_slice_size_local(unsigned short idx, int *tdm, int lim);
LINKER_DECL int tdm_td2p_slice_prox_dn(int slot, int *tdm, int lim, int **tsc, enum port_speed_e *speed);
LINKER_DECL int tdm_td2p_slice_prox_up(int slot, int *tdm, int **tsc, enum port_speed_e *speed);
LINKER_DECL int tdm_td2p_slice_size(unsigned short port, int *tdm, int lim);
LINKER_DECL int tdm_td2p_slice_idx(unsigned short port, int *tdm, int lim);
LINKER_DECL int tdm_td2p_slice_prox_local(unsigned short idx, int *tdm, int lim, int **tsc);
LINKER_DECL int tdm_td2p_check_lls_flat_up(int idx, int *tdm_tbl, enum port_speed_e *speed);
LINKER_DECL int tdm_td2p_num_lr_slots(int *tdm_tbl);
LINKER_DECL int tdm_td2p_scan_slice_min(unsigned short port, int *tdm, int lim, int *slice_start_idx, int pos);
LINKER_DECL int tdm_td2p_scan_slice_max(unsigned short port, int *tdm, int lim, int *slice_start_idx, int pos);
LINKER_DECL int tdm_td2p_scan_slice_size_local(unsigned short idx, int *tdm, int lim, int *slice_start_idx);
LINKER_DECL int tdm_td2p_scan_mix_slice_min(unsigned short port, int *tdm, int lim, int *slice_idx, int pos);
LINKER_DECL int tdm_td2p_scan_mix_slice_max(unsigned short port, int *tdm, int lim, int *slice_idx, int pos);
LINKER_DECL int tdm_td2p_scan_mix_slice_size_local(unsigned short idx, int *tdm, int lim, int *slice_start_idx);
LINKER_DECL int tdm_td2p_check_shift_cond_pattern(unsigned short port, int *tdm_tbl, int tdm_tbl_len, int **tsc, int dir);
LINKER_DECL int tdm_td2p_check_shift_cond_local_slice(unsigned short port, int *tdm_tbl, int tdm_tbl_len, int **tsc, int dir);
LINKER_DECL int tdm_td2p_check_slot_swap_cond(int idx, int *tdm_tbl, int tdm_tbl_len, int **tsc, enum port_speed_e *speed);
LINKER_DECL int tdm_td2p_scan_which_tsc(int port, int tsc[TD2P_NUM_PHY_PM][TD2P_NUM_PM_LNS]);
LINKER_DECL int tdm_td2p_check_pipe_ethernet(int port, tdm_mod_t *_tdm);

/* TDM.4 - Trident2+ chip based filter methods */
LINKER_DECL int tdm_td2p_filter_dither(int *tdm_tbl, int lr_idx_limit, int accessories, int **tsc, int threshold, enum port_speed_e *speed);
LINKER_DECL int tdm_td2p_filter_fine_dither(int port, int *tdm_tbl, int lr_idx_limit, int accessories, int **tsc);
LINKER_DECL int tdm_td2p_filter_shift_lr_port(unsigned short port, int *tdm_tbl, int tdm_tbl_len, int dir);
LINKER_DECL int tdm_td2p_filter_migrate_os_slot(int idx_src, int idx_dst, int *tdm_tbl, int tdm_tbl_len, int **tsc, enum port_speed_e *speed);
LINKER_DECL int tdm_td2p_filter_smooth_idle_slice(int *tdm_tbl, int tdm_tbl_len, int **tsc, enum port_speed_e *speed);
LINKER_DECL int tdm_td2p_filter_smooth_os_slice(int *tdm_tbl, int tdm_tbl_len, int **tsc, enum port_speed_e *speed, int dir);
LINKER_DECL int tdm_td2p_filter_smooth_os_slice_fine(int *tdm_tbl, int tdm_tbl_len, int **tsc, enum port_speed_e *speed);
LINKER_DECL int tdm_td2p_filter_smooth_os_os_up(int *tdm_tbl, int tdm_tbl_len, int **tsc, enum port_speed_e *speed);
LINKER_DECL int tdm_td2p_filter_smooth_os_os_dn(int *tdm_tbl, int tdm_tbl_len, int **tsc, enum port_speed_e *speed);
LINKER_DECL int tdm_td2p_filter_smooth_ancl(int ancl_token, int *tdm_tbl, int tdm_tbl_len, int ancl_space_min);

/* TDM.4 - Trident2+ chip specific IARB calendars */
LINKER_DECL void tdm_td2p_init_iarb_tdm_ovs_table(int core_bw, int mgm4x1, int mgm4x2p5, int mgm1x10, int *iarb_tdm_wrap_ptr_ovs_x, int *iarb_tdm_wrap_ptr_ovs_y, int *iarb_tdm_tbl_ovs_x, int *iarb_tdm_tbl_ovs_y);
LINKER_DECL void tdm_td2p_init_iarb_tdm_lr_table(int core_bw, int mgm4x1, int mgm4x2p5, int mgm1x10, int *iarb_tdm_wrap_ptr_lr_x, int *iarb_tdm_wrap_ptr_lr_y, int *iarb_tdm_tbl_lr_x, int *iarb_tdm_tbl_lr_y);
LINKER_DECL int tdm_td2p_set_iarb_tdm_table(int core_bw, int is_x_ovs, int is_y_ovs, int mgm4x1, int mgm4x2p5, int mgm1x10, int *iarb_tdm_wrap_ptr_x, int *iarb_tdm_wrap_ptr_y, int *iarb_tdm_tbl_x, int *iarb_tdm_tbl_y);

/* Trident2+ TDM self-check operations */
LINKER_DECL int tdm_td2p_chk_ethernet(tdm_mod_t *_tdm);
LINKER_DECL int tdm_td2p_chk_struc(tdm_mod_t *_tdm);
LINKER_DECL int tdm_td2p_chk_tsc(tdm_mod_t *_tdm);
LINKER_DECL int tdm_td2p_chk_min(tdm_mod_t *_tdm);
LINKER_DECL int tdm_td2p_chk_sub_lr( tdm_mod_t *_tdm);
LINKER_DECL int tdm_td2p_chk_sub_os(tdm_mod_t *_tdm);
LINKER_DECL int tdm_td2p_chk_sub_cpu( tdm_mod_t *_tdm);
LINKER_DECL int tdm_td2p_chk_sub_lpbk( tdm_mod_t *_tdm);
LINKER_DECL int tdm_td2p_chk_sub_acc( tdm_mod_t *_tdm);
LINKER_DECL int tdm_td2p_chk_subscription(tdm_mod_t *_tdm);
LINKER_DECL int tdm_td2p_chk_os_jitter(tdm_mod_t *_tdm);
LINKER_DECL int tdm_td2p_chk_lr_jitter(tdm_mod_t *_tdm, int fail[15]);
LINKER_DECL int tdm_td2p_chk_lls(tdm_mod_t *_tdm);
LINKER_DECL void tdm_td2p_chk_chip_specific(tdm_mod_t *_tdm, int fail[15]);
LINKER_DECL void tdm_td2p_chk( tdm_mod_t *_tdm, int fail[15]);
LINKER_DECL int tdm_td2p_chk_wrap( tdm_mod_t *_tdm);

#endif /* TDM_TD2P_PREPROCESSOR_PROTOTYPES_H */

#endif /* __BCM56860_A0_TDM_CHIP_METHODS_H__ */
