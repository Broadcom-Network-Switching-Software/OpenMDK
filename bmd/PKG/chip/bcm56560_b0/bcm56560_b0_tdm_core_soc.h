/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 * All Rights Reserved.$
 *
 * TDM soc header
 */
#ifndef __BCM56560_B0_TDM_CORE_SOC_H__
#define __BCM56560_B0_TDM_CORE_SOC_H__

#ifndef TDM_PREPROCESSOR_SOC_DEFINES_H
#define TDM_PREPROCESSOR_SOC_DEFINES_H

enum port_speed_e {
    SPEED_0=0,
    SPEED_10M=10,
    SPEED_20M=20,
    SPEED_25M=25,
    SPEED_33M=33,
    SPEED_40M=40,
    SPEED_50M=50,
    SPEED_100M=100,
    SPEED_100M_FX=101,
    SPEED_120M=120,
    SPEED_400M=400,
    SPEED_1G=1000,
    SPEED_1G_FX=1001,
    SPEED_1p2G=1200,
    SPEED_1p25G=1250,
    SPEED_2G=2000,
    SPEED_2p5G=2500,
    SPEED_4G=4000,
    SPEED_5G=5000,
    SPEED_7p5G=7500,
    SPEED_10G=10000,
    SPEED_10G_DUAL=10001,
    SPEED_10G_XAUI=10002,
    SPEED_11G=11000,
    SPEED_12G=12000,
    SPEED_12p5G=12500,
    SPEED_13G=13000,
    SPEED_15G=15000,
    SPEED_16G=16000,
    SPEED_20G=20000,
    SPEED_21G=21000,
    SPEED_21G_DUAL=21010,
    SPEED_24G=24000,
    SPEED_25G=25000,
    SPEED_27G=27000,
    SPEED_30G=30000,
    SPEED_40G=40000,
    SPEED_42G=40005,
    SPEED_42G_HG2=42000,
    SPEED_50G=50000,
    SPEED_53G=53000,
    SPEED_75G=75000,
    SPEED_82G=82000,
    SPEED_100G=100000,
    SPEED_106G=106000,
    SPEED_120G=120000,
    SPEED_127G=127000,
    SPEED_200G=200000,
    SPEED_400G=400000
};

/*  enum port_state_e

    Bitmap:
    xx_y_zz
    xx -> Special property
    y  -> Encap type
    zz -> Lane property
*/
enum port_state_e {
    PORT_STATE__DISABLED=0x0,
    PORT_STATE__LINERATE=0x1,
    PORT_STATE__OVERSUB=0x2,
    PORT_STATE__COMBINE=0x3,
    PORT_STATE__LINERATE_HG=0x5,
    PORT_STATE__OVERSUB_HG=0x6,
    PORT_STATE__COMBINE_HG=0x7,
    PORT_STATE__MANAGEMENT=0x9,
    PORT_STATE__FLEX_OVERSUB_UP=0x10,
    PORT_STATE__FLEX_OVERSUB_DN=0x20,
    PORT_STATE__FLEX_OVERSUB_CH=0x30,
    PORT_STATE__MUTABLE=0x11,
    PORT_STATE__CARDINAL=0x19
};

/* speed class index */
typedef enum tdm_b0_speed_idx_e {
    TDM_SPEED_IDX_0=0,
    TDM_SPEED_IDX_1G,
    TDM_SPEED_IDX_1P25G,
    TDM_SPEED_IDX_2P5G,
    TDM_SPEED_IDX_5G,
    TDM_SPEED_IDX_10G,
    TDM_SPEED_IDX_12P5G,
    TDM_SPEED_IDX_20G,
    TDM_SPEED_IDX_25G,
    TDM_SPEED_IDX_40G,
    TDM_SPEED_IDX_50G,
    TDM_SPEED_IDX_100G,
    TDM_SPEED_IDX_120G,
    TDM_SPEED_IDX_200G,
    TDM_SPEED_IDX_400G,
    TDM_SPEED_IDX_NUM
} tdm_b0_speed_idx_t;

/* slot granularity */
typedef enum tdm_b0_slot_unit_e {
    TDM_SLOT_UNIT_0=0,
    TDM_SLOT_UNIT_1P25G=SPEED_1p25G,
    TDM_SLOT_UNIT_2P5G=SPEED_2p5G,
    TDM_SLOT_UNIT_5G=SPEED_5G,
    TDM_SLOT_UNIT_10G=SPEED_10G,
    TDM_SLOT_UNIT_25G=SPEED_25G,
    TDM_SLOT_UNIT_50G=SPEED_50G
} tdm_b0_slot_unit_t;

#define PSBMP_ENCAP_HIGIG 0x4
#define PSBMP_LP_MASK 0x3

typedef struct node {
    unsigned short port;
    struct node *next;
} node;

typedef struct td2p_vars_t td2p_vars_t;
typedef struct td3_vars_t td3_vars_t;
typedef struct th_vars_t th_vars_t;
typedef struct th2_vars_t th2_vars_t;
typedef struct ap_vars_t ap_vars_t;

#include "bcm56560_b0_tdm_chip_soc.h"
struct ap_vars_t {
    int mgmtbw;
    int pgw_num;
    int start_port;
    int stop_port;
    int op_flags_0[NUM_OP_FLAGS];
    int op_flags_1[NUM_OP_FLAGS];
    int pm_encap_type[AP_NUM_PM_MOD];
    unsigned short higig_mgmt;
    int iarb_core_bw;
    int iarb_is_x_ovs;
    int iarb_mgm4x1;
    int iarb_mgm4x2p5;
    int iarb_mgm1x10;
    int *iarb_tdm_b0_wrap_ptr_x;
    int *iarb_tdm_b0_tbl_x;
    int tdm_b0_chk_en; /* 1->enable; otherwise->disable */
};

struct td2p_vars_t {
    int dummy;
};

struct th_vars_t {
    int dummy;
};

struct th2_vars_t {
    int dummy;
};

struct td3_vars_t {
    int dummy;
};

typedef struct _tdm_b0_chip_vars_t {
    int **gmap; /* Group mapping for weighting LLS round robin */
    int gmap_max_len;
    int gmap_max_wid;
    int ovsb_token;
    int idl1_token;
    int idl2_token;
    int ancl_token;
    int fp_port_lo;
    int fp_port_hi;
    td2p_vars_t td2p;
    td3_vars_t td3;
    th_vars_t th;
    th2_vars_t th2;
    ap_vars_t ap;
} tdm_b0_chip_vars_t;

typedef struct _tdm_b0_soc_t {
    int unit;
    int **pmap; /* Transcribed port module mapping */
    int pmap_num_modules;
    int pmap_num_lanes;
    int pm_num_phy_modules;
    enum port_speed_e *speed;
    enum port_state_e *state;
    int clk_freq;
    int lr_idx_limit; /* Length of TDM round standardized to slot quanta */
    int tvec_size; /* Number of guaranteed slots in tokenized vector */
    int num_ext_ports;
    tdm_b0_chip_vars_t soc_vars;
    int flex_port_en; /* 1 when FlexPort, else is 0 by default*/
} tdm_b0_soc_t;

/* Functional group of TDM sequences - one calendar and up to 8 sortable/weightable groups for round robin */
typedef struct _tdm_b0_calendar_t {
    int *cal_main;
    int cal_len;
    int **cal_grp;
    int grp_num;
    int grp_len;
} tdm_b0_calendar_t;

/* Chip output and architecturally specific data */
typedef struct _tdm_b0_chip_t {
    tdm_b0_soc_t soc_pkg;
    tdm_b0_calendar_t cal_0;
    tdm_b0_calendar_t cal_1;
    tdm_b0_calendar_t cal_2;
    tdm_b0_calendar_t cal_3;
    tdm_b0_calendar_t cal_4;
    tdm_b0_calendar_t cal_5;
    tdm_b0_calendar_t cal_6;
    tdm_b0_calendar_t cal_7;
} tdm_b0_chip_t;

/* Core variables and algorithm agnostic to soc architecture */
typedef struct _m_tdm_b0_vmap_alloc_t {
    int num_lr;
    int num_os;
    int num_40g;
    int num_100g;
    unsigned short tri_chk;
    unsigned short tri_en_50;
    unsigned short tri_en_40;
    unsigned short tri_en_20;
    unsigned short prox_swap;
    unsigned short os1[TDM_AUX_SIZE];
    unsigned short os10[TDM_AUX_SIZE];
    unsigned short os20[TDM_AUX_SIZE];
    unsigned short os25[TDM_AUX_SIZE];
    unsigned short os40[TDM_AUX_SIZE];
    unsigned short os50[TDM_AUX_SIZE];
    unsigned short os100[TDM_AUX_SIZE];
    unsigned short os120[TDM_AUX_SIZE];
} m_tdm_b0_vmap_alloc_t;
typedef struct _m_tdm_b0_pick_vec_t {
    int tsc_dq;
    int *triport_priority;
    int prev_vec;
} m_tdm_b0_pick_vec_t;
typedef struct _m_tdm_b0_core_vbs_scheduler_t {
    int lr_vec_cnt;
    short y8;
    short y7;
    short y1;
    short y2;
    short y3;
    short y4;
    short y5;
    short y6;
    short yy;
    short z8;
    short z7;
    short z6;
    short z1;
    short z2;
    short z3;
    short z4;
    short z5;
    unsigned short z11;
    unsigned short z22;
    unsigned short z33;
    unsigned short z44;
    unsigned short z55;
    unsigned short z66;
    unsigned short z77;
    unsigned short z88;
    unsigned short z99;
    unsigned short zaa;
    unsigned short zbb;
    unsigned short zcc;
} m_tdm_b0_core_vbs_scheduler_t;
typedef struct _m_tdm_b0_mem_transpose_t {
    int src;
    int dst;
} m_tdm_b0_mem_transpose_t;
typedef struct _m_tdm_b0_map_find_y_indx_t {
    int principle;
    int idx;
} m_tdm_b0_map_find_y_indx_t;
typedef struct _m_tdm_b0_vector_rotate_t {
    int vector;
    int size;
    int step;
} m_tdm_b0_vector_rotate_t;
typedef struct _m_tdm_b0_vector_clear_t {
    int yy;
} m_tdm_b0_vector_clear_t;
typedef struct _m_tdm_b0_fit_singular_col_t {
    int node_x;
} m_tdm_b0_fit_singular_col_t;
typedef struct _m_tdm_b0_fit_prox_t {
    int wid;
    int node_x;
} m_tdm_b0_fit_prox_t;
typedef struct _m_tdm_b0_count_nonsingular_t {
    int x_idx;
} m_tdm_b0_count_nonsingular_t;
typedef struct _m_tdm_b0_fit_row_min_t {
    int y_idx;
} m_tdm_b0_fit_row_min_t;
typedef struct _m_tdm_b0_count_param_spd_t {
    int x_pos;
    int round;
} m_tdm_b0_count_param_spd_t;
typedef struct _m_tdm_b0_nsin_row_t {
    int y_idx;
} m_tdm_b0_nsin_row_t;
typedef struct _m_tdm_b0_sticky_transpose_t {
    int src;
    int dst;
} m_tdm_b0_sticky_transpose_t;

/* vmap: tdm_b0_vmap_pm_t */
typedef struct tdm_b0_vmap_pm_s {
    int pm_en;                               /* pm enable/active        */
    int pm_num;                              /* pm number               */
    int pm_slots;                            /* reserved pm slots       */
    int pm_mode;                             /* pm mode                 */
    int subport_cnt;                         /* active subport count    */
    int subport_phy[TDM_VMAP_PM_LANES];      /* subport buffer          */
    int subport_slot_req[TDM_VMAP_PM_LANES]; /* requested subport slots */
    int subport_slot_rsv[TDM_VMAP_PM_LANES]; /* reserved subport slots  */
} tdm_b0_vmap_pm_t;

/* NG TDM */
typedef struct _m_tdm_b0_pipe_info_t {
    int slot_unit; /* slot granularity in Mbps */
    int lr_en;
    int os_en;
    int num_lr_prt;
    int num_os_prt;
    char lr_spd_en[MAX_SPEED_TYPES];
    char os_spd_en[MAX_SPEED_TYPES];
    short lr_prt_cnt[MAX_SPEED_TYPES];
    short os_prt_cnt[MAX_SPEED_TYPES];
    unsigned short lr_prt[MAX_SPEED_TYPES][TDM_AUX_SIZE];
    unsigned short os_prt[MAX_SPEED_TYPES][TDM_AUX_SIZE];
} m_tdm_b0_pipe_info_t;

typedef struct _vars_pkg {
    int cal_id;
    int grp_id;
    int port;
    int pipe;
    char lr_enable;
    char os_enable;
    int lr_buffer[TDM_AUX_SIZE];
    int os_buffer[TDM_AUX_SIZE];
    char HG4X106G_3X40G;
    char HG1X106G_xX40G_OVERSUB;
    char HGXx120G_Xx100G;
    char os_1;
    char os_10;
    char os_20;
    char os_25;
    char os_40;
    char os_50;
    char os_100;
    char os_120;
    char lr_1;
    char lr_100;
    char lr_10;
    char lr_20;
    char lr_25;
    char lr_40;
    char lr_50;
    char lr_120;
    int cap;
    unsigned short refactor_done;
    m_tdm_b0_pick_vec_t m_tdm_b0_pick_vec;
    m_tdm_b0_vmap_alloc_t m_tdm_b0_vmap_alloc;
    m_tdm_b0_core_vbs_scheduler_t m_tdm_b0_core_vbs_scheduler;
    m_tdm_b0_mem_transpose_t m_tdm_b0_mem_transpose;
    m_tdm_b0_map_find_y_indx_t m_tdm_b0_map_find_y_indx;
    m_tdm_b0_vector_rotate_t m_tdm_b0_vector_rotate;
    m_tdm_b0_vector_clear_t m_tdm_b0_vector_clear;
    m_tdm_b0_fit_singular_col_t m_tdm_b0_fit_singular_col;
    m_tdm_b0_count_nonsingular_t m_tdm_b0_count_nonsingular;
    m_tdm_b0_fit_row_min_t m_tdm_b0_fit_row_min;
    m_tdm_b0_count_param_spd_t m_tdm_b0_count_param_spd;
    m_tdm_b0_nsin_row_t m_tdm_b0_nsin_row;
    m_tdm_b0_sticky_transpose_t m_tdm_b0_sticky_transpose;
    m_tdm_b0_fit_prox_t m_tdm_b0_fit_prox;
    m_tdm_b0_pipe_info_t pipe_info;
} tdm_b0_core_vars_t;
typedef struct {
    unsigned short **vmap; /* Vector mapping for VBS traversal algorithm */
    int vmap_max_len;
    int vmap_max_wid;
    int rule__same_port_min;
    int rule__prox_port_min;
    tdm_b0_core_vars_t vars_pkg;
} tdm_b0_core_t;

/* Define number of method pointers in TDM chip executive */
#define TDM_EXEC_CHIP_SIZE 16

/* Define number of method pointers in TDM core executive */
#define TDM_EXEC_CORE_SIZE 16

/* Virtualizable method signatures - TDM chip executive */
#define TDM_CHIP_EXEC__INIT 0
#define TDM_CHIP_EXEC__TRANSCRIPTION 1
#define TDM_CHIP_EXEC__INGRESS_WRAP 2
#define TDM_CHIP_EXEC__EGRESS_WRAP 3
#define TDM_CHIP_EXEC__FILTER 4
#define TDM_CHIP_EXEC__PARSE 5
#define TDM_CHIP_EXEC__PROC_ANCL 6
#define TDM_CHIP_EXEC__CHECK 12
#define TDM_CHIP_EXEC__FREE 13
#define TDM_CHIP_EXEC__COREREQ 14
#define TDM_CHIP_EXEC__POST (TDM_EXEC_CHIP_SIZE-1)

/* Virtualizable method signatures - TDM core executive */
#define TDM_CORE_EXEC__INIT 0
#define TDM_CORE_EXEC__POST 1
#define TDM_CORE_EXEC__ALLOC 2
#define TDM_CORE_EXEC__SCHEDULER 3
#define TDM_CORE_EXEC__SCHEDULER_OVS 4
#define TDM_CORE_EXEC__EXTRACT 5
#define TDM_CORE_EXEC__FILTER 6
#define TDM_CORE_EXEC__ACCESSORIZE 7
#define TDM_CORE_EXEC__VMAP_PREALLOC 8
#define TDM_CORE_EXEC__VMAP_ALLOC 9
#define TDM_CORE_EXEC__ENCAP_SCAN 13
#define TDM_CORE_EXEC__PICK_VEC 14
#define TDM_CORE_EXEC__PM_SCAN 15

/* TDM executive binds inheritable-polymorphic-encapsulated package of methods */
typedef int (*TDM_CORE_EXEC_t)( tdm_b0_core_t,tdm_b0_chip_t );
typedef int (*TDM_CHIP_EXEC_t)( tdm_b0_core_t,tdm_b0_chip_t );
/* static TDM_CORE_EXEC_t core_exec[TDM_EXEC_CORE_SIZE]; */
/* static TDM_CHIP_EXEC_t chip_exec[TDM_EXEC_CHIP_SIZE]; */

/* Forward subtype the TDM class */
typedef struct _tdm_b0_mod_t {
    tdm_b0_core_t _core_data;
    tdm_b0_chip_t _chip_data;
    int (*_core_exec[TDM_EXEC_CORE_SIZE])( struct _tdm_b0_mod_t * );
    int (*_chip_exec[TDM_EXEC_CHIP_SIZE])( struct _tdm_b0_mod_t * );
    tdm_b0_chip_t _prev_chip_data; /* used only in FlexPort context */
} tdm_mod_t;

#endif /* TDM_PREPROCESSOR_SOC_DEFINES_H */

#endif /* __BCM56560_B0_TDM_CORE_SOC_H__ */
