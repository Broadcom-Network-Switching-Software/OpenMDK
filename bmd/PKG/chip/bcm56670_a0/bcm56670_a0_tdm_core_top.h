/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 * All Rights Reserved.$
 *
 * TDM top header for core scheduler
 */
#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56670_A0 == 1

#ifndef TDM_PREPROCESSOR_DIRECTIVES_H
#define TDM_PREPROCESSOR_DIRECTIVES_H

#include <cdk/cdk_debug.h>
#include <cdk/cdk_types.h>
#include <cdk/cdk_string.h>
#include <cdk/cdk_shell.h>
#include "bcm56670_a0_internal.h"

/* Prints */
#define TDM_PRINT0(a) CDK_VERB((a))
#define TDM_VERBOSE(_frmt,a) CDK_VVERB((_frmt, a))
#define TDM_ERROR8(a,b,c,d,e,f,g,h,i) CDK_ERR((a, b, c, d, e, f, g, h, i))
#define TDM_ERROR7(a,b,c,d,e,f,g,h) CDK_ERR((a, b, c, d, e, f, g, h))
#define TDM_ERROR6(a,b,c,d,e,f,g) CDK_ERR((a, b, c, d, e, f, g))
#define TDM_ERROR5(a,b,c,d,e,f) CDK_ERR((a, b, c, d, e, f))
#define TDM_ERROR4(a,b,c,d,e) CDK_ERR((a, b, c, d, e))
#define TDM_ERROR3(a,b,c,d) CDK_ERR((a, b, c, d))
#define TDM_ERROR2(a,b,c) CDK_ERR((a, b, c))
#define TDM_ERROR1(a,b) CDK_ERR((a, b))
#define TDM_ERROR0(a) CDK_ERR((a))
#define TDM_WARN6(a,b,c,d,e,f,g) CDK_WARN((a, b, c, d, e, f, g))
#define TDM_WARN5(a,b,c,d,e,f) CDK_WARN((a, b, c, d, e, f))
#define TDM_WARN4(a,b,c,d,e) CDK_WARN((a, b, c, d, e))
#define TDM_WARN3(a,b,c,d) CDK_WARN((a, b, c, d))
#define TDM_WARN2(a,b,c) CDK_WARN((a, b, c))
#define TDM_WARN1(a,b) CDK_WARN((a, b))
#define TDM_WARN0(a) CDK_WARN((a))
#define TDM_PRINT9(a,b,c,d,e,f,g,h,i,j) CDK_VERB((a, b, c, d, e, f, g, h, i, j))
#define TDM_PRINT8(a,b,c,d,e,f,g,h,i) CDK_VERB((a, b, c, d, e, f, g, h, i))
#define TDM_PRINT7(a,b,c,d,e,f,g,h) CDK_VERB((a, b, c, d, e, f, g, h))
#define TDM_PRINT6(a,b,c,d,e,f,g) CDK_VERB((a, b, c, d, e, f, g))
#define TDM_PRINT5(a,b,c,d,e,f) CDK_VERB((a, b, c, d, e, f))
#define TDM_PRINT4(a,b,c,d,e) CDK_VERB((a, b, c, d, e))
#define TDM_PRINT3(a,b,c,d) CDK_VERB((a, b, c, d))
#define TDM_PRINT2(a,b,c) CDK_VERB((a, b, c))
#define TDM_PRINT1(a,b) CDK_VERB((a, b))
#define TDM_BIG_BAR TDM_PRINT0(("#################################################################################################################################\n"));
#define TDM_SML_BAR TDM_PRINT0(("---------------------------------------------------------------------------------------------------------------------------------\n"));
#define TDM_DEBUG TDM_PRINT0(("--- DEBUG ---\n"));
/* Compiler */
#define LINKER_DECL
#define TDM_MN_ALLOC(_sz,_id) bcm56670_a0_tdm_malloc(cdk_shell_unit_get(), _sz)
#define TDM_MSET(_str,_val,_len)												\
					{																\
						int TDM_MSET_IDX;											\
						for (TDM_MSET_IDX=0; TDM_MSET_IDX<_len; TDM_MSET_IDX++) {	\
							_str[TDM_MSET_IDX]=(unsigned short) _val;				\
						}															\
					}
#define TDM_FREE(_sz) bcm56670_a0_tdm_free(cdk_shell_unit_get(), _sz)
#define TDM_COPY(_dst,_src,_len) CDK_MEMCPY(_dst,_src,_len)

/* Redefine the function names in bcm56670_tdm_core_methods.h */
#define tdm_core_init bcm56670_a0_tdm_core_init
#define tdm_core_post bcm56670_a0_tdm_core_post
#define tdm_core_vbs_scheduler bcm56670_a0_tdm_core_vbs_scheduler
#define tdm_core_vbs_scheduler_lr bcm56670_a0_tdm_mn_core_vbs_scheduler_lr
#define tdm_core_vbs_scheduler_alloc_lr bcm56670_a0_tdm_core_vbs_scheduler_alloc_lr
#define tdm_core_vbs_scheduler_ovs bcm56670_a0_tdm_core_vbs_scheduler_ovs
#define tdm_pick_vec bcm56670_a0_tdm_pick_vec
#define tdm_find_pm bcm56670_a0_tdm_find_pm
#define tdm_core_acc_alloc bcm56670_a0_tdm_core_acc_alloc
#define tdm_core_null bcm56670_a0_tdm_core_null
#define tdm_core_vmap_prealloc bcm56670_a0_tdm_core_vmap_prealloc
#define tdm_core_vmap_alloc bcm56670_a0_tdm_core_vmap_alloc
#define tdm_core_vmap_alloc_mix bcm56670_a0_tdm_core_vmap_alloc_mix
#define tdm_core_vbs_scheduler_wrapper bcm56670_a0_tdm_core_vbs_scheduler_wrapper
#define tdm_core_ovsb_alloc bcm56670_a0_tdm_core_ovsb_alloc
#define tdm_mem_transpose bcm56670_a0_tdm_mem_transpose
#define tdm_sticky_transpose bcm56670_a0_tdm_sticky_transpose
#define tdm_type_chk bcm56670_a0_tdm_type_chk
#define tdm_find_fastest_triport bcm56670_a0_tdm_find_fastest_triport
#define tdm_find_fastest_port bcm56670_a0_tdm_find_fastest_port
#define tdm_find_fastest_spd bcm56670_a0_tdm_find_fastest_spd
#define tdm_map_find_y_indx bcm56670_a0_tdm_map_find_y_indx
#define tdm_vector_rotate bcm56670_a0_tdm_vector_rotate
#define tdm_vector_clear bcm56670_a0_tdm_vector_clear
#define tdm_fit_singular_col bcm56670_a0_tdm_fit_singular_col
#define tdm_fit_prox bcm56670_a0_tdm_fit_prox
#define tdm_count_nonsingular bcm56670_a0_tdm_count_nonsingular
#define tdm_fit_row_min bcm56670_a0_tdm_fit_row_min
#define tdm_count_param_spd bcm56670_a0_tdm_count_param_spd
#define tdm_nsin_row bcm56670_a0_tdm_nsin_row
#define tdm_core_filter_refactor bcm56670_a0_tdm_core_filter_refactor
#define tdm_fill_ovs bcm56670_a0_tdm_fill_ovs
#define tdm_col_transpose bcm56670_a0_tdm_col_transpose
#define tdm_ll_retrace bcm56670_a0_tdm_ll_retrace
#define tdm_ll_tsc_dist bcm56670_a0_tdm_ll_tsc_dist
#define tdm_ll_strip bcm56670_a0_tdm_ll_strip
#define tdm_core_prealloc bcm56670_a0_tdm_core_prealloc
#define tdm_core_postalloc bcm56670_a0_tdm_core_postalloc
#define tdm_core_postalloc_vmap bcm56670_a0_tdm_core_postalloc_vmap
#define tdm_slots bcm56670_a0_tdm_slots
#define tdm_arr_exists bcm56670_a0_tdm_arr_exists
#define tdm_arr_append bcm56670_a0_tdm_arr_append
#define tdm_empty_row bcm56670_a0_tdm_empty_row
#define tdm_vector_rotate_step bcm56670_a0_tdm_vector_rotate_step
#define tdm_slice_size_2d bcm56670_a0_tdm_slice_size_2d
#define tdm_fit_singular_cnt bcm56670_a0_tdm_fit_singular_cnt
#define tdm_map_cadence_count bcm56670_a0_tdm_map_cadence_count
#define tdm_map_retrace_count bcm56670_a0_tdm_map_retrace_count
#define tdm_fill_ovs_simple bcm56670_a0_tdm_fill_ovs_simple
#define tdm_ll_append bcm56670_a0_tdm_ll_append
#define tdm_ll_insert bcm56670_a0_tdm_ll_insert
#define tdm_ll_print bcm56670_a0_tdm_ll_print
#define tdm_ll_deref bcm56670_a0_tdm_ll_deref
#define tdm_ll_delete bcm56670_a0_tdm_ll_delete
#define tdm_ll_get bcm56670_a0_tdm_ll_get
#define tdm_ll_dist bcm56670_a0_tdm_ll_dist
#define tdm_ll_free bcm56670_a0_tdm_ll_free
#define tdm_class_data bcm56670_a0_tdm_class_data
#define tdm_class_output bcm56670_a0_tdm_class_output
#define tdm_print_stat bcm56670_a0_tdm_print_stat
#define tdm_print_config bcm56670_a0_tdm_print_config
#define tdm_vector_dump bcm56670_a0_tdm_vector_dump
#define tdm_print_vmap_vector bcm56670_a0_tdm_print_vmap_vector
#define tdm_vector_zrow bcm56670_a0_tdm_vector_zrow
#define tdm_PQ bcm56670_a0_tdm_PQ
#define tdm_sqrt bcm56670_a0_tdm_sqrt
#define tdm_pow bcm56670_a0_tdm_pow
#define tdm_fac bcm56670_a0_tdm_fac
#define tdm_abs bcm56670_a0_tdm_abs
#define tdm_math_div_ceil bcm56670_a0_tdm_math_div_ceil
#define tdm_math_div_floor bcm56670_a0_tdm_math_div_floor
#define tdm_math_div_round bcm56670_a0_tdm_math_div_round
#define tdm_vmap_print bcm56670_a0_tdm_vmap_print
#define tdm_vmap_chk_singularity bcm56670_a0_tdm_vmap_chk_singularity
#define tdm_vmap_chk_sister bcm56670_a0_tdm_vmap_chk_sister
#define tdm_vmap_chk_same bcm56670_a0_tdm_vmap_chk_same
#define tdm_vmap_chk_vmap_sister_xy bcm56670_a0_tdm_vmap_chk_vmap_sister_xy
#define tdm_vmap_chk_vmap_sister_col bcm56670_a0_tdm_vmap_chk_vmap_sister_col
#define tdm_vmap_chk_vmap_sister bcm56670_a0_tdm_vmap_chk_vmap_sister
#define tdm_vmap_get_port_pm bcm56670_a0_tdm_vmap_get_port_pm
#define tdm_vmap_get_port_speed bcm56670_a0_tdm_vmap_get_port_speed
#define tdm_vmap_get_speed_slots bcm56670_a0_tdm_vmap_get_speed_slots
#define tdm_vmap_get_port_slots bcm56670_a0_tdm_vmap_get_port_slots
#define tdm_vmap_get_vmap_wid bcm56670_a0_tdm_mn_vmap_get_vmap_wid
#define tdm_vmap_get_vmap_len bcm56670_a0_tdm_mn_vmap_get_vmap_len
#define tdm_vmap_get_port_space_sister bcm56670_a0_tdm_vmap_get_port_space_sister
#define tdm_vmap_get_port_space_same bcm56670_a0_tdm_vmap_get_port_space_same
#define tdm_vmap_calc_port_dist_sister bcm56670_a0_tdm_vmap_calc_port_dist_sister
#define tdm_vmap_calc_port_dist_same bcm56670_a0_tdm_vmap_calc_port_dist_same
#define tdm_vmap_switch_vmap_col bcm56670_a0_tdm_vmap_switch_vmap_col
#define tdm_vmap_rotate_vmap bcm56670_a0_tdm_vmap_rotate_vmap
#define tdm_vmap_shift_port bcm56670_a0_tdm_vmap_shift_port
#define tdm_vmap_filter_sister bcm56670_a0_tdm_vmap_filter_sister
#define tdm_vmap_filter_same bcm56670_a0_tdm_vmap_filter_same
#define tdm_vmap_calc_slot_pos bcm56670_a0_tdm_vmap_calc_slot_pos
#define tdm_vmap_alloc_one_port bcm56670_a0_tdm_vmap_alloc_one_port
#define tdm_vmap_alloc bcm56670_a0_tdm_vmap_alloc
#define tdm_vmap_alloc_mix bcm56670_a0_tdm_vmap_alloc_mix
#define tdm_cmn_chk_port_is_fp bcm56670_a0_tdm_cmn_chk_port_is_fp
#define tdm_cmn_chk_pipe_os_spd_types bcm56670_a0_tdm_cmn_chk_pipe_os_spd_types
#define tdm_cmn_get_port_pm bcm56670_a0_tdm_cmn_get_port_pm
#define tdm_cmn_get_port_speed bcm56670_a0_tdm_cmn_get_port_speed
#define tdm_cmn_get_port_speed_eth bcm56670_a0_tdm_cmn_get_port_speed_eth
#define tdm_cmn_get_port_speed_idx bcm56670_a0_tdm_cmn_get_port_speed_idx
#define tdm_cmn_get_slot_unit bcm56670_a0_tdm_cmn_get_slot_unit
#define tdm_cmn_get_speed_slots bcm56670_a0_tdm_cmn_get_speed_slots
#define tdm_cmn_get_port_slots bcm56670_a0_tdm_cmn_get_port_slots
#define tdm_cmn_get_pipe_cal bcm56670_a0_tdm_cmn_get_pipe_cal
#define tdm_cmn_get_pipe_cal_prev bcm56670_a0_tdm_cmn_get_pipe_cal_prev

/* TDM headers */
#include "bcm56670_a0_tdm_core_defines.h"
#include "bcm56670_a0_tdm_core_soc.h"
#include "bcm56670_a0_tdm_core_methods.h"

#define TOKEN_CHECK(a)  											\
			if (a>=_tdm->_chip_data.soc_pkg.soc_vars.fp_port_lo &&	\
				a<=_tdm->_chip_data.soc_pkg.soc_vars.fp_port_hi)	\
				
#define TDM_SEL_CAL(_cal_id,_cal_pntr)										\
			switch (_cal_id) {												\
				case 0: _cal_pntr=_tdm->_chip_data.cal_0.cal_main; break;	\
				case 1: _cal_pntr=_tdm->_chip_data.cal_1.cal_main; break;	\
				case 2: _cal_pntr=_tdm->_chip_data.cal_2.cal_main; break;	\
				case 3: _cal_pntr=_tdm->_chip_data.cal_3.cal_main; break;	\
				case 4: _cal_pntr=_tdm->_chip_data.cal_4.cal_main; break;	\
				case 5: _cal_pntr=_tdm->_chip_data.cal_5.cal_main; break;	\
				case 6: _cal_pntr=_tdm->_chip_data.cal_6.cal_main; break;	\
				case 7: _cal_pntr=_tdm->_chip_data.cal_7.cal_main; break;	\
				default:													\
					_cal_pntr=NULL;											\
					TDM_PRINT1("Invalid calendar ID - %0d\n",_cal_id);		\
					return (TDM_EXEC_CORE_SIZE+1);							\
			}
#define TDM_SEL_GRP(_grp_id,_grp_pntr)										\
			switch (_grp_id) {												\
				case 0: _grp_pntr=_tdm->_chip_data.cal_0.cal_grp; break;	\
				case 1: _grp_pntr=_tdm->_chip_data.cal_1.cal_grp; break;	\
				case 2: _grp_pntr=_tdm->_chip_data.cal_2.cal_grp; break;	\
				case 3: _grp_pntr=_tdm->_chip_data.cal_3.cal_grp; break;	\
				case 4: _grp_pntr=_tdm->_chip_data.cal_4.cal_grp; break;	\
				case 5: _grp_pntr=_tdm->_chip_data.cal_5.cal_grp; break;	\
				case 6: _grp_pntr=_tdm->_chip_data.cal_6.cal_grp; break;	\
				case 7: _grp_pntr=_tdm->_chip_data.cal_7.cal_grp; break;	\
				default:													\
					_grp_pntr=NULL;											\
					TDM_PRINT1("Invalid group ID - %0d\n",_grp_id);			\
					return (TDM_EXEC_CORE_SIZE+1);							\
			}
#define TDM_PUSH(a,b,c) 												\
				{														\
					int TDM_PORT_POP=_tdm->_core_data.vars_pkg.port;	\
					_tdm->_core_data.vars_pkg.port=a;					\
					c=_tdm->_core_exec[b](_tdm);						\
					_tdm->_core_data.vars_pkg.port=TDM_PORT_POP;		\
				}
#ifdef _TDM_DB_STACK
    #define TDM_PRINT_STACK_SIZE(a)                                        \
                {                                                          \
                    size_t stack_check = 0;                                \
                    stack_check = (stack_size - ((size_t)&(stack_check))); \
                    TDM_PRINT2("TDM: [stack_size %d, %s]\n",               \
                    (int)stack_check, a);                                  \
                }
#else
    #define TDM_PRINT_STACK_SIZE(a)                                        \
                {                                                          \
                    TDM_PRINT2("TDM: [stack_size %d, %s]\n", 0, a);        \
                }
#endif

/* Function names redefinition */
#define SOC_SEL_TDM bcm56670_a0_SOC_SEL_TDM 
#define _soc_set_tdm_tbl bcm56670_a0__soc_set_tdm_tbl
#define _soc_tdm_ver bcm56670_a0__soc_tdm_ver

extern tdm_mod_t
*SOC_SEL_TDM(tdm_soc_t *chip, tdm_mod_t *_tdm);
extern tdm_mod_t
*_soc_set_tdm_tbl(tdm_mod_t *_tdm);
extern void
_soc_tdm_ver(int ver[8]);


#endif /* TDM_PREPROCESSOR_DIRECTIVES_H*/

/* ESW chip support */
#include "bcm56670_a0_tdm_chip_top.h"
#endif /* CDK_CONFIG_INCLUDE_BCM56670_A0 */
