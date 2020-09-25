/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 * All Rights Reserved.$
 *
 * TDM function prototypes for core scheduler
 */
#ifndef __BCM56560_B0_TDM_CORE_METHODS_H__
#define __BCM56560_B0_TDM_CORE_METHODS_H__

#ifndef TDM_PREPROCESSOR_PROTOTYPES_H
#define TDM_PREPROCESSOR_PROTOTYPES_H
/*
 * CORE function prototypes 
 */

/* Class functions managed by TDM core executive */
LINKER_DECL int tdm_b0_core_init( tdm_mod_t *_tdm );
LINKER_DECL int tdm_b0_core_post( tdm_mod_t *_tdm );
LINKER_DECL int tdm_b0_core_vbs_scheduler( tdm_mod_t *_tdm );
LINKER_DECL int tdm_b0_core_vbs_scheduler_lr_b0( tdm_mod_t *_tdm );
LINKER_DECL int tdm_b0_core_vbs_scheduler_alloc_lr( tdm_mod_t *_tdm );
LINKER_DECL int tdm_b0_core_vbs_scheduler_ovs( tdm_mod_t *_tdm );
LINKER_DECL int tdm_b0_ovsb_fill_os_grps(tdm_mod_t *_tdm);
LINKER_DECL int tdm_b0_pick_vec( tdm_mod_t *_tdm );
LINKER_DECL int tdm_b0_find_pm( tdm_mod_t *_tdm );
LINKER_DECL int tdm_b0_core_acc_alloc( tdm_mod_t *_tdm );
LINKER_DECL int tdm_b0_core_null( tdm_mod_t *_tdm );
LINKER_DECL int tdm_b0_core_vmap_prealloc(tdm_mod_t *_tdm);
LINKER_DECL int tdm_b0_core_vmap_alloc(tdm_mod_t *_tdm);
LINKER_DECL int tdm_b0_core_vmap_alloc_mix(tdm_mod_t *_tdm);
LINKER_DECL int tdm_b0_core_vbs_scheduler_wrapper_b0(tdm_mod_t *_tdm);
LINKER_DECL int tdm_b0_core_ovsb_alloc(tdm_mod_t *_tdm);

/* Internal functions that can use class-heritable data */
LINKER_DECL void tdm_b0_mem_transpose( tdm_mod_t *_tdm );
LINKER_DECL void tdm_b0_sticky_transpose( tdm_mod_t *_tdm );
LINKER_DECL int tdm_b0_type_chk( tdm_mod_t *_tdm );
LINKER_DECL int tdm_b0_find_fastest_triport( tdm_mod_t *_tdm );
LINKER_DECL int tdm_b0_find_fastest_port( tdm_mod_t *_tdm );
LINKER_DECL int tdm_b0_find_fastest_spd( tdm_mod_t *_tdm );
LINKER_DECL int tdm_b0_map_find_y_indx( tdm_mod_t *_tdm );
LINKER_DECL void tdm_b0_vector_rotate( tdm_mod_t *_tdm );
LINKER_DECL void tdm_b0_vector_clear( tdm_mod_t *_tdm );
LINKER_DECL int tdm_b0_fit_singular_col( tdm_mod_t *_tdm );
LINKER_DECL int tdm_b0_fit_prox( tdm_mod_t *_tdm );
LINKER_DECL int tdm_b0_count_nonsingular( tdm_mod_t *_tdm );
LINKER_DECL int tdm_b0_fit_row_min( tdm_mod_t *_tdm );
LINKER_DECL int tdm_b0_count_param_spd( tdm_mod_t *_tdm );
LINKER_DECL int tdm_b0_nsin_row( tdm_mod_t *_tdm );
LINKER_DECL int tdm_b0_core_filter_refactor( tdm_mod_t *_tdm );
LINKER_DECL int tdm_b0_fill_ovs( tdm_mod_t *_tdm );
LINKER_DECL void tdm_b0_col_transpose_b0( tdm_mod_t *_tdm );

/* Linked list API partially merged with class method */
LINKER_DECL int tdm_b0_ll_retrace(struct node *llist, tdm_mod_t *_tdm, int cadence_start_idx);
LINKER_DECL int tdm_b0_ll_tsc_dist(struct node *llist, tdm_mod_t *_tdm, int idx);
LINKER_DECL void tdm_b0_ll_strip(struct node *llist, tdm_mod_t *_tdm, int cadence_start_idx, int *pool, int *s_idx, int token);

/* Internal functions managed entirely by compiler */
LINKER_DECL void tdm_b0_core_prealloc(unsigned short stack[TDM_AUX_SIZE], int buffer[TDM_AUX_SIZE], short *x, char *boolstr, int j);
LINKER_DECL int tdm_b0_core_postalloc(unsigned short **vector_map, int freq, unsigned short spd, short *yy, short *y, int lr_idx_limit, unsigned short lr_stack[TDM_AUX_SIZE], int token, const char* speed, int num_ext_ports);
LINKER_DECL int tdm_b0_core_postalloc_vmap_b0(unsigned short **vector_map, int freq, unsigned short spd, short *yy, short *y, int lr_idx_limit, unsigned short lr_stack[TDM_AUX_SIZE], int token, const char* speed, int num_ext_ports, int vmap_wid, int vmap_len);
LINKER_DECL int tdm_b0_slots(int port_speed);
LINKER_DECL int tdm_b0_arr_exists(int element, int len, int arr[ARR_FIXED_LEN]);
LINKER_DECL int tdm_b0_arr_append(int element, int len, int arr[ARR_FIXED_LEN], int num_ext_ports);
LINKER_DECL int tdm_b0_empty_row(unsigned short **map, unsigned short y_idx, int num_ext_ports, int vec_map_wid);
LINKER_DECL void tdm_b0_vector_rotate_step(unsigned short vector[], int size, int step);
LINKER_DECL int tdm_b0_slice_size_2d(unsigned short **map, unsigned short y_idx, int lim, int num_ext_ports, int vec_map_wid);
LINKER_DECL int tdm_b0_fit_singular_cnt(unsigned short **map, int node_y, int vec_map_wid, int num_ext_ports);
LINKER_DECL int tdm_b0_map_cadence_count(unsigned short *vector, int idx, int vec_map_len);
LINKER_DECL int tdm_b0_map_retrace_count(unsigned short **map, int x_idx, int y_idx, int vec_map_len, int vec_map_wid, int num_ext_ports);
LINKER_DECL int tdm_b0_fill_ovs_simple(short *z, unsigned short ovs_buf[TDM_AUX_SIZE], int *bucket1, unsigned short *z11, int *bucket2, unsigned short *z22, int *bucket3, unsigned short *z33, int *bucket4, unsigned short *z44, int *bucket5, unsigned short *z55, int *bucket6, unsigned short *z66, int *bucket7, unsigned short *z77, int *bucket8, unsigned short *z88, int grp_len);

/* Linked list API managed entirely by compiler */
LINKER_DECL void tdm_b0_ll_append(struct node *llist, unsigned short port_append);
LINKER_DECL void tdm_b0_ll_insert(struct node *llist, unsigned short port_insert, int idx);
LINKER_DECL void tdm_b0_ll_print(struct node *llist);
LINKER_DECL void tdm_b0_ll_deref(struct node *llist, int *tdm, int lim);
LINKER_DECL int tdm_b0_ll_delete(struct node *llist, int idx);
LINKER_DECL int tdm_b0_ll_get(struct node *llist, int idx, int num_ext_ports);
LINKER_DECL int tdm_b0_ll_dist(struct node *llist, int idx);
LINKER_DECL int tdm_b0_ll_free(struct node *llist);

/* PARSE/PRINT API */
LINKER_DECL void tdm_b0_class_data( tdm_mod_t *_tdm );
LINKER_DECL void tdm_b0_class_output( tdm_mod_t *_tdm );
LINKER_DECL void tdm_b0_print_stat( tdm_mod_t *_tdm );
LINKER_DECL void tdm_b0_print_config_b0( tdm_mod_t *_tdm );
LINKER_DECL void tdm_b0_vector_dump( tdm_mod_t *_tdm );
LINKER_DECL void tdm_b0_print_vmap_vector( tdm_mod_t *_tdm );
LINKER_DECL void tdm_b0_vector_zrow( tdm_mod_t *_tdm );

/* CMATH API */
LINKER_DECL int tdm_b0_PQ(int f);
LINKER_DECL int tdm_b0_sqrt(int input_signed);
LINKER_DECL int tdm_b0_pow(int num, int pow);
LINKER_DECL int tdm_b0_fac(int num_signed);
LINKER_DECL int tdm_b0_abs(int num);
LINKER_DECL int tdm_b0_math_div_ceil(int a, int b);
LINKER_DECL int tdm_b0_math_div_floor(int a, int b);
LINKER_DECL int tdm_b0_math_div_round_b0(int a, int b);

/* TDM VMAP */
LINKER_DECL void tdm_b0_vmap_print(tdm_mod_t *_tdm, unsigned short **vmap);
LINKER_DECL void tdm_b0_vmap_print_vmap(tdm_mod_t *_tdm, unsigned short **vmap);
LINKER_DECL void tdm_b0_vmap_print_pmlist(tdm_mod_t *_tdm, tdm_b0_vmap_pm_t *pmlist, int pmlist_size);
LINKER_DECL int tdm_b0_vmap_chk_singularity(tdm_mod_t *_tdm, unsigned short **vmap);
LINKER_DECL int tdm_b0_vmap_chk_sister(tdm_mod_t *_tdm);
LINKER_DECL int tdm_b0_vmap_chk_same(tdm_mod_t *_tdm);
LINKER_DECL int tdm_b0_vmap_chk_vmap_sister_xy(tdm_mod_t *_tdm, unsigned short **vmap, int col, int row);
LINKER_DECL int tdm_b0_vmap_chk_vmap_sister_col(tdm_mod_t *_tdm, unsigned short **vmap, int col);
LINKER_DECL int tdm_b0_vmap_chk_vmap_sister(tdm_mod_t *_tdm, unsigned short **vmap);
LINKER_DECL int tdm_b0_vmap_chk_pipe_bandwidth(tdm_mod_t *_tdm, int *lr_buff, int lr_buff_size);
LINKER_DECL int tdm_b0_vmap_chk_port_space(tdm_mod_t *_tdm, int *cal, int cal_len, int espace,
                        int idx, int dir);
LINKER_DECL int tdm_b0_vmap_chk_lr_ports(tdm_mod_t *_tdm, int *lr_buff, int lr_buff_size);
LINKER_DECL void tdm_b0_vmap_op_pm_reset(tdm_mod_t *_tdm, tdm_b0_vmap_pm_t *pm);
LINKER_DECL void tdm_b0_vmap_op_pm_update(tdm_mod_t *_tdm, tdm_b0_vmap_pm_t *pm);
LINKER_DECL void tdm_b0_vmap_op_pm_swap(tdm_b0_vmap_pm_t *pmlist, int pmlist_size,
                    int src_idx, int dst_idx);
LINKER_DECL void tdm_b0_vmap_op_pm_migrate(tdm_b0_vmap_pm_t *pmlist, int pmlist_size,
                       int src_idx, int dst_idx);
LINKER_DECL int tdm_b0_vmap_op_pm_addPort(tdm_mod_t *_tdm, tdm_b0_vmap_pm_t *pm,
                       int port_token, int port_slots);
LINKER_DECL void tdm_b0_vmap_op_pmlist_init(tdm_mod_t *_tdm, tdm_b0_vmap_pm_t *pmlist, int
                        pmlist_size);
LINKER_DECL int tdm_b0_vmap_op_pmlist_gen(tdm_mod_t *_tdm, int *lr_buff, int lr_buff_size,
                       tdm_b0_vmap_pm_t *pmlist, int pmlist_size);
LINKER_DECL void tdm_b0_vmap_op_pmlist_sort(tdm_mod_t *_tdm, tdm_b0_vmap_pm_t *pmlist,
                        int pmlist_size);
LINKER_DECL void tdm_b0_vmap_op_pmlist_sort_2(tdm_mod_t *_tdm, tdm_b0_vmap_pm_t *pmlist,
                        int pmlist_size);
LINKER_DECL void tdm_b0_vmap_op_pmlist_shift(tdm_mod_t *_tdm, tdm_b0_vmap_pm_t *pmlist,
                         int pmlist_size, int pm_idx);
LINKER_DECL int tdm_b0_vmap_op_pmlist_adjust_lr(tdm_mod_t *_tdm, tdm_b0_vmap_pm_t *pmlist,
                             int pmlist_size);
LINKER_DECL int tdm_b0_vmap_op_pmlist_adjust_os_4lanes(tdm_mod_t *_tdm, tdm_b0_vmap_pm_t *pmlist,
                                    int pmlist_size);
LINKER_DECL int tdm_b0_vmap_op_pmlist_adjust_lr_4lanes(tdm_mod_t *_tdm, tdm_b0_vmap_pm_t *pmlist,
                                    int pmlist_size);
LINKER_DECL int tdm_b0_vmap_op_pmlist_adjust_os(tdm_mod_t *_tdm, tdm_b0_vmap_pm_t *pmlist,
                             int pmlist_size);
LINKER_DECL int tdm_b0_vmap_gen_lr_buff(int *src_buff, int src_buff_size,
                     int *dst_buff, int dst_buff_size, int token_empty);
LINKER_DECL int tdm_b0_vmap_gen_vmap(tdm_mod_t *_tdm, tdm_b0_vmap_pm_t *pmlist,
                  int pmlist_size, unsigned short **vmap);
LINKER_DECL int tdm_b0_vmap_part_pm_subports(tdm_mod_t *_tdm, int port_token,
                    tdm_b0_vmap_pm_t *pm, int vmap_idx, unsigned short **vmap);
LINKER_DECL void tdm_b0_vmap_patch_special_case(tdm_mod_t *_tdm, tdm_b0_vmap_pm_t *pmlist,
                            int pmlist_size);
LINKER_DECL int tdm_b0_vmap_get_port_pm(tdm_mod_t *_tdm, int port_token);
LINKER_DECL int tdm_b0_vmap_get_port_speed(tdm_mod_t *_tdm, int port_token);
LINKER_DECL int tdm_b0_vmap_get_speed_slots(tdm_mod_t *_tdm, int port_speed);
LINKER_DECL int tdm_b0_vmap_get_port_slots(tdm_mod_t *_tdm, int port_token);
LINKER_DECL int tdm_b0_vmap_get_vmap_wid(tdm_mod_t *_tdm, unsigned short **vmap);
LINKER_DECL int tdm_b0_vmap_get_vmap_len(tdm_mod_t *_tdm, unsigned short **vmap);
LINKER_DECL int tdm_b0_vmap_get_port_space_sister(tdm_mod_t *_tdm, int port_token);
LINKER_DECL int tdm_b0_vmap_get_port_space_same(tdm_mod_t *_tdm, int port_token);
LINKER_DECL int tdm_b0_vmap_calc_port_dist_sister(tdm_mod_t *_tdm, int *cal, int cal_len, int espace, int idx, int dir);
LINKER_DECL int tdm_b0_vmap_calc_port_dist_same(tdm_mod_t *_tdm, int *cal, int cal_len, int espace, int idx, int dir);
LINKER_DECL int tdm_b0_vmap_switch_vmap_col(tdm_mod_t *_tdm, unsigned short **vmap, int col_x, int col_y);
LINKER_DECL int tdm_b0_vmap_rotate_vmap(tdm_mod_t *_tdm, unsigned short **vmap);
LINKER_DECL int tdm_b0_vmap_shift_port(tdm_mod_t *_tdm, int port_token);
LINKER_DECL int tdm_b0_vmap_filter_sister(tdm_mod_t *_tdm);
LINKER_DECL int tdm_b0_vmap_filter_same(tdm_mod_t *_tdm);
LINKER_DECL int tdm_b0_vmap_calc_slot_pos(int vmap_idx, int slot_idx, int vmap_idx_max, int slot_idx_max, int **r_a_arr);
LINKER_DECL int tdm_b0_vmap_alloc_one_port(int port_token, int slot_req, int slot_left, int cal_len, int vmap_idx, int vmap_wid_max, int vmap_len_max, unsigned short **vmap, int **r_a_arr);
LINKER_DECL int tdm_b0_vmap_alloc(tdm_mod_t *_tdm);
LINKER_DECL int tdm_b0_vmap_alloc_mix(tdm_mod_t *_tdm);

/* TDM common API */
LINKER_DECL int tdm_b0_cmn_chk_port_is_fp(tdm_mod_t *_tdm, int port_token);
LINKER_DECL int tdm_b0_cmn_chk_pipe_os_spd_types(tdm_mod_t *_tdm);
LINKER_DECL int tdm_b0_cmn_get_port_pm(tdm_mod_t *_tdm, int port_token);
LINKER_DECL int tdm_b0_cmn_get_port_speed(tdm_mod_t *_tdm, int port_token);
LINKER_DECL int tdm_b0_cmn_get_port_speed_eth(tdm_mod_t *_tdm, int port_token);
LINKER_DECL int tdm_b0_cmn_get_port_speed_idx(tdm_mod_t *_tdm, int port_token);
LINKER_DECL int tdm_b0_cmn_get_slot_unit(tdm_mod_t *_tdm);
LINKER_DECL int tdm_b0_cmn_get_speed_slots(tdm_mod_t *_tdm, int port_speed);
LINKER_DECL int tdm_b0_cmn_get_port_slots(tdm_mod_t *_tdm, int port_token);
LINKER_DECL tdm_b0_calendar_t *tdm_b0_cmn_get_pipe_cal(tdm_mod_t *_tdm);
LINKER_DECL tdm_b0_calendar_t *tdm_b0_cmn_get_pipe_cal_prev(tdm_mod_t *_tdm);

#endif /* TDM_PREPROCESSOR_PROTOTYPES_H */

#endif /* __BCM56560_B0_TDM_CORE_METHODS_H__ */
