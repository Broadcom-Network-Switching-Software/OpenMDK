/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 * All Rights Reserved.$
 *
 * TDM top header for BCM56560
 */
#ifndef __BCM56560_B0_TDM_CHIP_TOP_H__
#define __BCM56560_B0_TDM_CHIP_TOP_H__

#ifndef TDM_AP_PREPROCESSOR_DIRECTIVES_H
#define TDM_AP_PREPROCESSOR_DIRECTIVES_H

#include "bcm56560_b0_tdm_chip_defines.h"
#include "bcm56560_b0_tdm_core_defines.h"
#include "bcm56560_b0_tdm_core_soc.h"

/* Excludes all tokenized port numbers */
#define AP_TOKEN_CHECK(a)  				\
			if (a!=AP_NUM_EXT_PORTS && 	\
			a!=AP_OVSB_TOKEN && 			\
			a!=AP_IDL1_TOKEN && 			\
			a!=AP_AUX_TOKEN && 			\
			a!=AP_IDL2_TOKEN && 			\
			a!=AP_ANCL_TOKEN &&			\
			a!=AP_MGMT_TOKEN && 			\
			a!=AP_US1G_TOKEN && 			\
			a!=AP_LOOPBACK_PORT &&		\
			a!=AP_CPU_PORT) 				\

/* Excludes all tokenized port numbers except OVS */
#define AP_OVS_TOKEN_CHECK(a)  				\
			if (a!=AP_NUM_EXT_PORTS && 	\
			a!=AP_IDL1_TOKEN && 			\
			a!=AP_AUX_TOKEN && 			\
			a!=AP_IDL2_TOKEN && 			\
			a!=AP_ANCL_TOKEN &&			\
			a!=AP_MGMT_TOKEN && 			\
			a!=AP_US1G_TOKEN && 			\
			a!=AP_LOOPBACK_PORT &&		\
			a!=AP_CPU_PORT) 				\

#define AP_CMIC(a) {						\
			mmu_tdm_b0_tbl[a]=AP_CPU_PORT; 	\
			TDM_PRINT0("CMIC\n"); 			\
			break;							\
		}
#define AP_LPBK(a) {							\
			mmu_tdm_b0_tbl[a]=AP_LOOPBACK_PORT; 	\
			TDM_PRINT0("LOOPBACK\n"); 			\
			break;								\
		}
#define AP_ANCL(a) {							\
			mmu_tdm_b0_tbl[a]=AP_AUX_TOKEN; 	\
			TDM_PRINT0("AUXILLARY\n");		\
			break;								\
		}
#define AP_SBUS(a) {							\
			mmu_tdm_b0_tbl[a]=AP_SBUS_TOKEN; 	\
			TDM_PRINT0("SBUS\n");		\
			break;								\
		}
#define AP_REFR(a) {							\
			mmu_tdm_b0_tbl[a]=AP_IDL2_TOKEN; 	\
			TDM_PRINT0("REFRESH\n");			\
			break;								\
		}

#define IARB_MAIN_TDM__TDM_SLOT_PGW_0         0
#define IARB_MAIN_TDM__TDM_SLOT_PGW_1         1
#define IARB_MAIN_TDM__TDM_SLOT_CMIC_PORT     2
#define IARB_MAIN_TDM__TDM_SLOT_EP_LOOPBACK   3
#define IARB_MAIN_TDM__TDM_SLOT_RDB_PORT      4
#define IARB_MAIN_TDM__TDM_SLOT_RESERVED      6
#define IARB_MAIN_TDM__TDM_SLOT_AUX_OPS_SLOT  7

typedef struct ap_ll_node {
	unsigned short port;
	struct ap_ll_node *next;
} ap_ll_node;

typedef struct {
	int pgw_tdm_b0_tbl_x0[AP_LR_LLS_LEN];
	int ovs_tdm_b0_tbl_x0[AP_OS_LLS_GRP_LEN]; 
	int ovs_spacing_x0[AP_OS_LLS_GRP_LEN];
	int pgw_tdm_b0_tbl_x1[AP_LR_LLS_LEN];
	int ovs_tdm_b0_tbl_x1[AP_OS_LLS_GRP_LEN];
	int ovs_spacing_x1[AP_OS_LLS_GRP_LEN];
	int iarb_tdm_b0_tbl_x[AP_LR_IARB_STATIC_LEN];
	int op_flags_x[NUM_OP_FLAGS];
} ap_tdm_b0_pgw_t;

typedef struct {
	int tdm_b0_pipe_main[AP_LR_VBS_LEN];
	int tdm_b0_ovs_grp_0[AP_OS_VBS_GRP_LEN];
	int tdm_b0_ovs_grp_1[AP_OS_VBS_GRP_LEN];
	int tdm_b0_ovs_grp_2[AP_OS_VBS_GRP_LEN];
	int tdm_b0_ovs_grp_3[AP_OS_VBS_GRP_LEN];
	int tdm_b0_ovs_grp_4[AP_OS_VBS_GRP_LEN];
	int tdm_b0_ovs_grp_5[AP_OS_VBS_GRP_LEN];
	int tdm_b0_ovs_grp_6[AP_OS_VBS_GRP_LEN];
	int tdm_b0_ovs_grp_7[AP_OS_VBS_GRP_LEN];
} ap_tdm_b0_pipe_t;

typedef struct {
	enum port_speed_e speed[AP_NUM_EXT_PORTS];
	int clk_freq;
	enum port_state_e port_rates_array[AP_NUM_EXT_PORTS];
	int pm_encap_type[AP_NUM_PM_MOD];
  int cl_flag;
  int num_1g;
} ap_tdm_b0_globals_t;

typedef struct {
	int pmap[AP_NUM_PHY_PM][AP_NUM_PM_LNS];
	ap_tdm_b0_pipe_t tdm_b0_pipe_table_x;
	ap_tdm_b0_globals_t tdm_b0_globals;
	ap_tdm_b0_pgw_t tdm_b0_pgw;
} tdm_b0_ap_b0_chip_legacy_t;
/*
typedef enum {
	AP_PGW_OS_MV_720G =0x00,
	AP_PGW_OS_MV_880G =0x01,
	AP_PGW_OS_FB_565  =0x10,
	AP_PGW_OS_FB_567  =0x20,
	AP_PGW_OS_FB_568_1=0x30,
	AP_PGW_OS_FB_568_2=0x31
} ap_pgw_os_tdm_b0_sel_e; */
typedef enum {
	AP_PGW_OS_MV_720G =0x0,
	AP_PGW_OS_MV_880G =0x1,
	AP_PGW_OS_FB_565  =0x2,
	AP_PGW_OS_FB_567  =0x3,
	AP_PGW_OS_FB_568_1=0x4,
	AP_PGW_OS_FB_568_2=0x5
} ap_pgw_os_tdm_b0_sel_e;

#include "bcm56560_b0_tdm_chip_methods.h"

/* API shim layer */
LINKER_DECL int tdm_b0_chip_ap_shim_core( tdm_mod_t *_tdm, tdm_b0_ap_b0_chip_legacy_t *_tdm_b0_ap );
LINKER_DECL int tdm_b0_chip_ap_shim__ing_wrap( tdm_mod_t *_tdm, tdm_b0_ap_b0_chip_legacy_t *_tdm_b0_ap );
LINKER_DECL int tdm_b0_chip_ap_shim__egr_wrap( tdm_mod_t *_tdm, tdm_b0_ap_b0_chip_legacy_t *_tdm_b0_ap );
LINKER_DECL tdm_mod_t* tdm_b0_chip_ap_shim__which_tsc_alloc( int port, int tsc[AP_NUM_PHY_PM][AP_NUM_PM_LNS] );
LINKER_DECL int tdm_b0_chip_ap_shim__which_tsc_free( tdm_mod_t *_tdm ); 
LINKER_DECL tdm_mod_t* tdm_b0_chip_ap_shim__check_ethernet( int port, enum port_speed_e speed[AP_NUM_EXT_PORTS], int tsc[AP_NUM_PHY_PM][AP_NUM_PM_LNS], int traffic[AP_NUM_PM_MOD] );
LINKER_DECL tdm_mod_t* tdm_b0_chip_ap_shim__check_ethernet_d( int port, enum port_speed_e speed[AP_NUM_EXT_PORTS], enum port_state_e state[AP_NUM_EXT_PORTS], int **tsc, int traffic[AP_NUM_PM_MOD] );
LINKER_DECL int tdm_b0_chip_ap_shim__core_vbs_scheduler_ovs( tdm_mod_t *_tdm );

#endif /* TDM_AP_PREPROCESSOR_DIRECTIVES_H */

#endif /* __BCM56560_B0_TDM_CHIP_TOP_H__ */
