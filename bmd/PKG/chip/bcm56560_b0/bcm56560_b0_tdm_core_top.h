/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 * All Rights Reserved.$
 *
 * TDM top header for core scheduler
 */
#ifndef __BCM56560_B0_TDM_CORE_TOP_H__
#define __BCM56560_B0_TDM_CORE_TOP_H__

#ifndef TDM_PREPROCESSOR_DIRECTIVES_H
#define TDM_PREPROCESSOR_DIRECTIVES_H

#include <cdk/cdk_debug.h>
#include <cdk/cdk_types.h>
#include <cdk/cdk_string.h>
#include <cdk/cdk_shell.h>
#include "bcm56560_b0_internal.h"

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
#define TDM_ALLOC(_sz,_id) bcm56560_b0_tdm_malloc(0, _sz)
#define TDM_FREE(_sz) bcm56560_b0_tdm_free(0, _sz)
#define TDM_COPY(_dst,_src,_len) CDK_MEMCPY(_dst, _src, _len)
#define TDM_MSET(_str, _val, _len) \
    do { \
        int _idx; \
        for (_idx = 0; _idx < _len; _idx++) \
        { \
            *(_str + _idx) = _val; \
        } \
    } while (0)

/* Redefine the function names in bcm56560_b0_tdm_core_methods.h */
#define tdm_b0_core_init bcm56560_b0_tdm_core_init
#define tdm_b0_core_post bcm56560_b0_tdm_core_post
#define tdm_b0_core_vbs_scheduler bcm56560_b0_tdm_core_vbs_scheduler
#define tdm_b0_core_vbs_scheduler_ovs bcm56560_b0_tdm_core_vbs_scheduler_ovs
#define tdm_b0_pick_vec bcm56560_b0_tdm_pick_vec
#define tdm_b0_find_pm bcm56560_b0_tdm_find_pm
#define tdm_b0_core_acc_alloc bcm56560_b0_tdm_core_acc_alloc
#define tdm_b0_core_null bcm56560_b0_tdm_core_null
#define tdm_b0_mem_transpose bcm56560_b0_tdm_mem_transpose
#define tdm_b0_sticky_transpose bcm56560_b0_tdm_sticky_transpose
#define tdm_b0_type_chk bcm56560_b0_tdm_type_chk
#define tdm_b0_class_data bcm56560_b0_tdm_class_data
#define tdm_b0_class_output bcm56560_b0_tdm_class_output
#define tdm_b0_print_stat bcm56560_b0_tdm_print_stat
#define tdm_b0_vector_dump bcm56560_b0_tdm_vector_dump
#define tdm_b0_print_vmap_vector bcm56560_b0_tdm_print_vmap_vector
#define tdm_b0_vector_zrow bcm56560_b0_tdm_vector_zrow
#define tdm_b0_find_fastest_triport bcm56560_b0_tdm_find_fastest_triport
#define tdm_b0_find_fastest_port bcm56560_b0_tdm_find_fastest_port
#define tdm_b0_find_fastest_spd bcm56560_b0_tdm_find_fastest_spd
#define tdm_b0_map_find_y_indx bcm56560_b0_tdm_map_find_y_indx
#define tdm_b0_vector_rotate bcm56560_b0_tdm_vector_rotate
#define tdm_b0_vector_clear bcm56560_b0_tdm_vector_clear
#define tdm_b0_fit_singular_col bcm56560_b0_tdm_fit_singular_col
#define tdm_b0_fit_prox bcm56560_b0_tdm_fit_prox
#define tdm_b0_count_nonsingular bcm56560_b0_tdm_count_nonsingular
#define tdm_b0_fit_row_min bcm56560_b0_tdm_fit_row_min
#define tdm_b0_count_param_spd bcm56560_b0_tdm_count_param_spd
#define tdm_b0_nsin_row bcm56560_b0_tdm_nsin_row
#define tdm_b0_core_filter_refactor bcm56560_b0_tdm_core_filter_refactor
#define tdm_b0_fill_ovs bcm56560_b0_tdm_fill_ovs
#define tdm_b0_ll_retrace bcm56560_b0_tdm_ll_retrace
#define tdm_b0_ll_tsc_dist bcm56560_b0_tdm_ll_tsc_dist
#define tdm_b0_ll_strip bcm56560_b0_tdm_ll_strip
#define tdm_b0_core_prealloc bcm56560_b0_tdm_core_prealloc
#define tdm_b0_core_postalloc bcm56560_b0_tdm_core_postalloc
#define tdm_b0_slots bcm56560_b0_tdm_slots
#define tdm_b0_arr_exists bcm56560_b0_tdm_arr_exists
#define tdm_b0_arr_append bcm56560_b0_tdm_arr_append
#define tdm_b0_empty_row bcm56560_b0_tdm_empty_row
#define tdm_b0_vector_rotate_step bcm56560_b0_tdm_vector_rotate_step
#define tdm_b0_slice_size_2d bcm56560_b0_tdm_slice_size_2d
#define tdm_b0_fit_singular_cnt bcm56560_b0_tdm_fit_singular_cnt
#define tdm_b0_map_cadence_count bcm56560_b0_tdm_map_cadence_count
#define tdm_b0_map_retrace_count bcm56560_b0_tdm_map_retrace_count
#define tdm_b0_fill_ovs_simple bcm56560_b0_tdm_fill_ovs_simple
#define tdm_b0_ll_append bcm56560_b0_tdm_ll_append
#define tdm_b0_ll_insert bcm56560_b0_tdm_ll_insert
#define tdm_b0_ll_print bcm56560_b0_tdm_ll_print
#define tdm_b0_ll_deref bcm56560_b0_tdm_ll_deref
#define tdm_b0_ll_delete bcm56560_b0_tdm_ll_delete
#define tdm_b0_ll_get bcm56560_b0_tdm_ll_get
#define tdm_b0_ll_dist bcm56560_b0_tdm_ll_dist
#define tdm_b0_ll_free bcm56560_b0_tdm_ll_free
#define tdm_b0_PQ bcm56560_b0_tdm_PQ
#define tdm_b0_sqrt bcm56560_b0_tdm_sqrt
#define tdm_b0_pow bcm56560_b0_tdm_pow
#define tdm_b0_fac bcm56560_b0_tdm_fac
#define tdm_b0_abs bcm56560_b0_tdm_abs

/* TDM headers */
#include "bcm56560_b0_tdm_core_defines.h"
#include "bcm56560_b0_tdm_core_soc.h"
#include "bcm56560_b0_tdm_core_methods.h"

#define TOKEN_CHECK(a)                                              \
            if (a>=_tdm->_chip_data.soc_pkg.soc_vars.fp_port_lo &&    \
                a<=_tdm->_chip_data.soc_pkg.soc_vars.fp_port_hi)    \

#define TDM_SEL_CAL(_cal_id,_cal_pntr)                                        \
            switch (_cal_id) {                                                \
                case 0: _cal_pntr=_tdm->_chip_data.cal_0.cal_main; break;    \
                case 1: _cal_pntr=_tdm->_chip_data.cal_1.cal_main; break;    \
                case 2: _cal_pntr=_tdm->_chip_data.cal_2.cal_main; break;    \
                case 3: _cal_pntr=_tdm->_chip_data.cal_3.cal_main; break;    \
                case 4: _cal_pntr=_tdm->_chip_data.cal_4.cal_main; break;    \
                case 5: _cal_pntr=_tdm->_chip_data.cal_5.cal_main; break;    \
                case 6: _cal_pntr=_tdm->_chip_data.cal_6.cal_main; break;    \
                case 7: _cal_pntr=_tdm->_chip_data.cal_7.cal_main; break;    \
                default:                                                    \
                    _cal_pntr=NULL;                                            \
                    TDM_PRINT1("Invalid calendar ID - %0d\n",_cal_id);        \
                    return (TDM_EXEC_CORE_SIZE+1);                            \
            }
#define TDM_SEL_GRP(_grp_id,_grp_pntr)                                        \
            switch (_grp_id) {                                                \
                case 0: _grp_pntr=_tdm->_chip_data.cal_0.cal_grp; break;    \
                case 1: _grp_pntr=_tdm->_chip_data.cal_1.cal_grp; break;    \
                case 2: _grp_pntr=_tdm->_chip_data.cal_2.cal_grp; break;    \
                case 3: _grp_pntr=_tdm->_chip_data.cal_3.cal_grp; break;    \
                case 4: _grp_pntr=_tdm->_chip_data.cal_4.cal_grp; break;    \
                case 5: _grp_pntr=_tdm->_chip_data.cal_5.cal_grp; break;    \
                case 6: _grp_pntr=_tdm->_chip_data.cal_6.cal_grp; break;    \
                case 7: _grp_pntr=_tdm->_chip_data.cal_7.cal_grp; break;    \
                default:                                                    \
                    _grp_pntr=NULL;                                            \
                    TDM_PRINT1("Invalid group ID - %0d\n",_grp_id);            \
                    return (TDM_EXEC_CORE_SIZE+1);                            \
            }
#define TDM_PUSH(a,b,c)                                                 \
                {                                                        \
                    int TDM_PORT_POP=_tdm->_core_data.vars_pkg.port;    \
                    _tdm->_core_data.vars_pkg.port=a;                    \
                    c=_tdm->_core_exec[b](_tdm);                        \
                    _tdm->_core_data.vars_pkg.port=TDM_PORT_POP;        \
                }

/* Function names redefinition */
#define SOC_SEL_TDM bcm56560_b0_SOC_SEL_TDM 
#define _soc_set_tdm_b0_tbl bcm56560_b0__soc_set_tdm_b0_tbl
#define _soc_tdm_b0_ver bcm56560_b0__soc_tdm_b0_ver

extern tdm_mod_t
*SOC_SEL_TDM(tdm_b0_soc_t *chip, tdm_mod_t *_tdm);
extern tdm_mod_t
*_soc_set_tdm_b0_tbl(tdm_mod_t *_tdm);
extern void
_soc_tdm_b0_ver(int ver[8]);


#endif /* TDM_PREPROCESSOR_DIRECTIVES_H*/

/* ESW chip support */
#include "bcm56560_b0_tdm_chip_top.h"

#endif /* __BCM56560_B0_TDM_CORE_TOP_H__ */
