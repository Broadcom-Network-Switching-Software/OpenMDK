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
#if CDK_CONFIG_INCLUDE_BCM53570_B0 == 1

#ifndef __BCM53570_B0_TDM_CORE_TOP_H__
#define __BCM53570_B0_TDM_CORE_TOP_H__

#ifndef TDM_PREPROCESSOR_DIRECTIVES_H
#define TDM_PREPROCESSOR_DIRECTIVES_H

#include <cdk/cdk_debug.h>
#include <cdk/cdk_types.h>
#include <cdk/cdk_string.h>
#include <cdk/cdk_shell.h>
#include "bcm53570_b0_internal.h"

/*
 * These are the possible debug types/flags for cdk_debug_level (below).
 */
#define CDK_DBG_ERR       (1 << 0)    /* Print errors */
#define CDK_DBG_WARN      (1 << 1)    /* Print warnings */
#define CDK_DBG_VERBOSE   (1 << 2)    /* General verbose output */
#define CDK_DBG_VVERBOSE  (1 << 3)    /* Very verbose output */
#define CDK_DBG_DEV       (1 << 4)    /* Device access */
#define CDK_DBG_REG       (1 << 5)    /* Register access */
#define CDK_DBG_MEM       (1 << 6)    /* Memory access */
#define CDK_DBG_SCHAN     (1 << 7)    /* S-channel operations */
#define CDK_DBG_MIIM      (1 << 8)    /* MII managment access */
#define CDK_DBG_DMA       (1 << 9)    /* DMA operations */
#define CDK_DBG_HIGIG     (1 << 10)   /* HiGig information */
#define CDK_DBG_PACKET    (1 << 11)   /* Packet data */

extern unsigned int cdk_debug_level;
extern int (*cdk_debug_printf)(const char *format, ...);
extern int cdk_printf(const char *fmt, ...);

#define CDK_DEBUG_CHECK(flags) (((flags) & cdk_debug_level) == (flags))

#define CDK_DEBUG(flags, stuff) \
    if (CDK_DEBUG_CHECK(flags) && cdk_debug_printf != 0) \
    (*cdk_debug_printf) stuff

#define CDK_ERR(stuff) CDK_DEBUG(CDK_DBG_ERR, stuff)
#define CDK_WARN(stuff) CDK_DEBUG(CDK_DBG_WARN, stuff)
#define CDK_VERB(stuff) CDK_DEBUG(CDK_DBG_VERBOSE, stuff)
#define CDK_VVERB(stuff) CDK_DEBUG(CDK_DBG_VVERBOSE, stuff)
#define CDK_DEBUG_DEV(stuff) CDK_DEBUG(CDK_DBG_DEV, stuff)
#define CDK_DEBUG_REG(stuff) CDK_DEBUG(CDK_DBG_REG, stuff)
#define CDK_DEBUG_MEM(stuff) CDK_DEBUG(CDK_DBG_MEM, stuff)
#define CDK_DEBUG_SCHAN(stuff) CDK_DEBUG(CDK_DBG_SCHAN, stuff)
#define CDK_DEBUG_MIIM(stuff) CDK_DEBUG(CDK_DBG_MIIM, stuff)
#define CDK_DEBUG_DMA(stuff) CDK_DEBUG(CDK_DBG_DMA, stuff)
#define CDK_DEBUG_HIGIG(stuff) CDK_DEBUG(CDK_DBG_HIGIG, stuff)
#define CDK_DEBUG_PACKET(stuff) CDK_DEBUG(CDK_DBG_PACKET, stuff)

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
#define TDM_ALLOC(_sz,_id) bcm53570_b0_tdm_malloc(0, _sz)
#define TDM_FREE(_sz) bcm53570_b0_tdm_free(0, _sz)
#define TDM_COPY(_dst,_src,_len) CDK_MEMCPY(_dst, _src, _len)
#define TDM_MSET(_str, _val, _len) \
    do { \
        int _idx; \
        for (_idx = 0; _idx < _len; _idx++) \
        { \
            *(_str + _idx) = _val; \
        } \
    } while (0)

/* Redefine the function names in bcm535700_b0_tdm_core_methods.h */
#define tdm_core_init bcm53570_b0_tdm_core_init
#define tdm_core_post bcm53570_b0_tdm_core_post
#define tdm_core_vbs_scheduler bcm53570_b0_tdm_core_vbs_scheduler
#define tdm_core_vbs_scheduler_ovs bcm53570_b0_tdm_core_vbs_scheduler_ovs
#define tdm_pick_vec bcm53570_b0_tdm_pick_vec
#define tdm_find_pm bcm53570_b0_tdm_find_pm
#define tdm_core_acc_alloc bcm53570_b0_tdm_core_acc_alloc
#define tdm_core_null bcm53570_b0_tdm_core_null
#define tdm_mem_transpose bcm53570_b0_tdm_mem_transpose
#define tdm_sticky_transpose bcm53570_b0_tdm_sticky_transpose
#define tdm_type_chk bcm53570_b0_tdm_type_chk
#define tdm_class_data bcm53570_b0_tdm_class_data
#define tdm_class_output bcm53570_b0_tdm_class_output
#define tdm_print_stat bcm53570_b0_tdm_print_stat
#define tdm_vector_dump bcm53570_b0_tdm_vector_dump
#define tdm_print_vmap_vector bcm53570_b0_tdm_print_vmap_vector
#define tdm_vector_zrow bcm53570_b0_tdm_vector_zrow
#define tdm_find_fastest_triport bcm53570_b0_tdm_find_fastest_triport
#define tdm_find_fastest_port bcm53570_b0_tdm_find_fastest_port
#define tdm_find_fastest_spd bcm53570_b0_tdm_find_fastest_spd
#define tdm_map_find_y_indx bcm53570_b0_tdm_map_find_y_indx
#define tdm_vector_rotate bcm53570_b0_tdm_vector_rotate
#define tdm_vector_clear bcm53570_b0_tdm_vector_clear
#define tdm_fit_singular_col bcm53570_b0_tdm_fit_singular_col
#define tdm_fit_prox bcm53570_b0_tdm_fit_prox
#define tdm_count_nonsingular bcm53570_b0_tdm_count_nonsingular
#define tdm_fit_row_min bcm53570_b0_tdm_fit_row_min
#define tdm_count_param_spd bcm53570_b0_tdm_count_param_spd
#define tdm_nsin_row bcm53570_b0_tdm_nsin_row
#define tdm_core_filter_refactor bcm53570_b0_tdm_core_filter_refactor
#define tdm_fill_ovs bcm53570_b0_tdm_fill_ovs
#define tdm_ll_retrace bcm53570_b0_tdm_ll_retrace
#define tdm_ll_tsc_dist bcm53570_b0_tdm_ll_tsc_dist
#define tdm_ll_strip bcm53570_b0_tdm_ll_strip
#define tdm_core_prealloc bcm53570_b0_tdm_core_prealloc
#define tdm_core_postalloc bcm53570_b0_tdm_core_postalloc
#define tdm_slots bcm53570_b0_tdm_slots
#define tdm_arr_exists bcm53570_b0_tdm_arr_exists
#define tdm_arr_append bcm53570_b0_tdm_arr_append
#define tdm_empty_row bcm53570_b0_tdm_empty_row
#define tdm_vector_rotate_step bcm53570_b0_tdm_vector_rotate_step
#define tdm_slice_size_2d bcm53570_b0_tdm_slice_size_2d
#define tdm_fit_singular_cnt bcm53570_b0_tdm_fit_singular_cnt
#define tdm_map_cadence_count bcm53570_b0_tdm_map_cadence_count
#define tdm_map_retrace_count bcm53570_b0_tdm_map_retrace_count
#define tdm_fill_ovs_simple bcm53570_b0_tdm_fill_ovs_simple
#define tdm_ll_append bcm53570_b0_tdm_ll_append
#define tdm_ll_insert bcm53570_b0_tdm_ll_insert
#define tdm_ll_print bcm53570_b0_tdm_ll_print
#define tdm_ll_deref bcm53570_b0_tdm_ll_deref
#define tdm_ll_delete bcm53570_b0_tdm_ll_delete
#define tdm_ll_get bcm53570_b0_tdm_ll_get
#define tdm_ll_dist bcm53570_b0_tdm_ll_dist
#define tdm_ll_free bcm53570_b0_tdm_ll_free
#define tdm_PQ bcm53570_b0_tdm_PQ
#define tdm_sqrt bcm53570_b0_tdm_sqrt
#define tdm_pow bcm53570_b0_tdm_pow
#define tdm_fac bcm53570_b0_tdm_fac
#define tdm_abs bcm53570_b0_tdm_abs

/* TDM headers */
#include "bcm53570_b0_tdm_core_defines.h"
#include "bcm53570_b0_tdm_core_soc.h"
#include "bcm53570_b0_tdm_core_methods.h"

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
#define SOC_SEL_TDM bcm53570_b0__SOC_SEL_TDM 
#define _soc_set_tdm_gh2_tbl bcm53570_b0__soc_set_tdm_gh2_tbl
#define _soc_tdm_ver bcm53570_b0__soc_tdm_ver

extern tdm_mod_t
*SOC_SEL_TDM(tdm_soc_t *chip, tdm_mod_t *_tdm);
extern tdm_mod_t
*_soc_set_tdm_gh2_tbl(tdm_mod_t *_tdm);
extern void
_soc_tdm_ver(int ver[8]);


#endif /* TDM_PREPROCESSOR_DIRECTIVES_H*/

/* ESW chip support */
#include "bcm53570_b0_tdm_chip_top.h"

#endif /* __BCM53570_B0_TDM_CORE_TOP_H__ */

#endif /* CDK_CONFIG_INCLUDE_BCM53570_B0 */
