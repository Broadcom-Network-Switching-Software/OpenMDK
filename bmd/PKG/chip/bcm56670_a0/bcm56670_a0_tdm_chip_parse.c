/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 * $All Rights Reserved.$
 *
 * TDM chip based printing and parsing functions
 */
#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56670_A0 == 1

#include "bcm56670_a0_tdm_core_top.h"

/**
@name: tdm_mn_print_tbl
@param:

Prints PGW TDM main calendar
 */
void
tdm_mn_print_tbl(int *cal, int len, const char* name, int id)
{
	int idx;
	
	for (idx=0; idx<len; idx++) {
		TDM_PRINT4("TDM: Pipe %d, %s, Slot -- #%03d, Port -- #%0d\n", id, name, idx, cal[idx]);
	}
}

void
tdm_mn_tdm_print(int *tdm_tbl, int length)
{
        int i;
        for (i=0; i<length; i++) {
                if (i%10==0) {TDM_PRINT2("\nTDM-TBL %3d : %3d ", i, tdm_tbl[i]);}
                else {TDM_PRINT1(" %3d ", tdm_tbl[i]);}
        }
		    TDM_PRINT0("");
}

#if 0
/**
@name: tdm_mn_print_tbl_ovs
@param:

Prints PGW TDM calendar oversub scheduling groups
 */
void
tdm_mn_print_tbl_ovs(int *cal, int *spc, int len, const char* name, int id)
{
	int idx;
	
	for (idx=0; idx<len; idx++) {
		TDM_PRINT5("TDM: Pipe %d, %s, Slot -- #%03d, Port -- #%0d, Spacing -- #%0d\n", id, name, idx, cal[idx], spc[idx]);
	}
}
#endif
#if 0
/**
@name: tdm_mn_print_quad
@param:
 */
void
tdm_mn_print_quad(enum port_speed_e *speed, enum port_state_e *state, int len, int idx_start, int idx_end)
{
	int iter, iter2;
	
	if (idx_start>0 && idx_end<=len){
		TDM_BIG_BAR
		TDM_PRINT0("TDM: --- Pipe Config ---: ");
		for (iter=idx_start; iter<=idx_end; iter++) {
			if ((((iter-1)%MN_NUM_PHY_PM)==0)) {
				TDM_PRINT0("\nTDM: ");
			}		
			TDM_PRINT1("{%03d}\t",(iter));
			if (iter%MN_NUM_PHY_PM==0) {
				TDM_PRINT0("\nTDM: ");
				for (iter2=(iter-MN_NUM_PHY_PM); iter2<iter; iter2++) {
					TDM_PRINT1(" %3d \t", speed[iter2+1]/1000);
				}
				TDM_PRINT0("\nTDM: ");
				for (iter2=(iter-MN_NUM_PHY_PM); iter2<iter; iter2++) {
					if (iter2%4==0){
						if(speed[iter2+1]/1000>0){
							switch ((PSBMP_LP_MASK & state[iter2])) {
								case 1:
									TDM_PRINT0("LINE \t --- \t --- \t --- \t");
									break;
								case 2:
									TDM_PRINT0("OVSB \t --- \t --- \t --- \t");
									break;
								default:
									TDM_PRINT0("     \t     \t     \t     \t");
									break;
							}
						}
						else{
							TDM_PRINT0("     \t     \t     \t     \t");
						}
					}
				}
				TDM_PRINT0("\nTDM: ");
				for (iter2=(iter-MN_NUM_PHY_PM); iter2<iter; iter2++) {
					if(iter2%4==0){
						if(speed[iter2+1]/1000>0){
							switch(( PSBMP_ENCAP_HIGIG & state[iter2])){
								case 0:
									TDM_PRINT0("  EN \t --- \t --- \t --- \t");
									break;
								case 4:
									TDM_PRINT0("  HG \t --- \t --- \t --- \t");
									break;
								default:
									TDM_PRINT0("     \t     \t     \t     \t");
									break;
							}
						}
						else{
							TDM_PRINT0("     \t     \t     \t     \t");
						}
					}
				}
			}
		}
		TDM_PRINT0("\n");
		TDM_BIG_BAR
	}

}
#endif
/**
@name: tdm_mn_parse_mmu_tdm_tbl
@param:
 */
int
tdm_mn_parse_mmu_tdm_tbl( tdm_mod_t *_tdm )
{
	const char *name;
	int i_tmp=0, j=0, k=512, m=0, cal_len=0, num_lpbk_slots=0, num_cpu_slots=0,
		*mmu_tdm_tbl, *mmu_tdm_ovs_1, *mmu_tdm_ovs_2, *mmu_tdm_ovs_3, *mmu_tdm_ovs_4, *mmu_tdm_ovs_5, *mmu_tdm_ovs_6,
		bw=_tdm->_chip_data.soc_pkg.clk_freq, avg_lpbk_spacing=0, avg_cpu_spacing=0, avg_anc_spacing=0 ,
     avg_sbus_spacing=0, num_anc_slots=0, num_sbus_slots=0, num_macsec_slots=0;
  int lb_slots_required=0, cpu_slots_required=0, anc_slots_required=0, sbus_slots_required=0, macsec_slots_required=0,
      alloted_lb_slots=0, alloted_cpu_slots=0, alloted_anc_slots=0, alloted_sbus_slots=0, alloted_macsec_slots=0;
	  tdm_calendar_t *cal;
	
	switch (_tdm->_core_data.vars_pkg.cal_id) {
		case 0:	cal=(&(_tdm->_chip_data.cal_0)); break;
		case 1:	cal=(&(_tdm->_chip_data.cal_1)); break;
		case 2:	cal=(&(_tdm->_chip_data.cal_2)); break;
		case 3:	cal=(&(_tdm->_chip_data.cal_3)); break;
		case 4:	cal=(&(_tdm->_chip_data.cal_4)); break;
		case 5:	cal=(&(_tdm->_chip_data.cal_5)); break;
		case 6:	cal=(&(_tdm->_chip_data.cal_6)); break;
		case 7:	cal=(&(_tdm->_chip_data.cal_7)); break;
		default:													
			TDM_PRINT1("Invalid calendar ID - %0d\n",_tdm->_core_data.vars_pkg.cal_id);		
			return (TDM_EXEC_CORE_SIZE+1);							
	}
	switch (_tdm->_core_data.vars_pkg.cal_id) {
		case 2: name = "X-PIPE"; break;
		default: name = "UNKNOWN"; break;
	}	
	mmu_tdm_tbl = cal->cal_main;
	mmu_tdm_ovs_1 = cal->cal_grp[0];
	mmu_tdm_ovs_2 = cal->cal_grp[1];
	mmu_tdm_ovs_3 = cal->cal_grp[2];
	mmu_tdm_ovs_4 = cal->cal_grp[3];
	mmu_tdm_ovs_5 = cal->cal_grp[4];
	mmu_tdm_ovs_6 = cal->cal_grp[5];
  cal_len = _tdm->_chip_data.soc_pkg.soc_vars.mn.mmu_len;

/*for (j=0; j<MN_NUM_PHY_PM; j++) {
		if (mmu_tdm_ovs_1[j]!=TD2P_NUM_EXT_PORTS || mmu_tdm_ovs_2[j]!=TD2P_NUM_EXT_PORTS || mmu_tdm_ovs_3[j]!=TD2P_NUM_EXT_PORTS || mmu_tdm_ovs_4[j]!=TD2P_NUM_EXT_PORTS) {
			oversub=BOOL_TRUE;
		}
	}*/
	/* Allocate slots for MGMT, CMIC/LPBK, ANCL and RERESH among available MN_ANCL_TOKENs */
  if(bw != 861 && bw != 862){
    switch(bw){
      case 815: case 816:
         avg_lpbk_spacing = 5;  lb_slots_required=6;
         avg_cpu_spacing = 10;  cpu_slots_required=3;
         avg_anc_spacing = 9;   anc_slots_required=3;
         avg_sbus_spacing = 15; sbus_slots_required=2;
          macsec_slots_required=15;
      break;
      case 817:
         avg_lpbk_spacing = 3;  lb_slots_required=6;
         avg_cpu_spacing = 6;  cpu_slots_required=3;
         avg_anc_spacing = 6;   anc_slots_required=3;
         avg_sbus_spacing = 16; sbus_slots_required=1;
          macsec_slots_required=0;
      break;
      case 818:
         avg_lpbk_spacing = 2;  lb_slots_required=6;
         avg_cpu_spacing = 3;  cpu_slots_required=3;
         avg_anc_spacing = 2;   anc_slots_required=3;
         avg_sbus_spacing = 1; sbus_slots_required=2;
          macsec_slots_required=0;
      break;
      case 819:
         avg_lpbk_spacing = 3;  lb_slots_required=5;
         avg_cpu_spacing = 6;  cpu_slots_required=3;
         avg_anc_spacing = 6;   anc_slots_required=3;
         avg_sbus_spacing = 9; sbus_slots_required=1;
          macsec_slots_required=15;
      break;
      case 820:
      default : 
         avg_lpbk_spacing = 6;  lb_slots_required=6;
         avg_cpu_spacing = 12;  cpu_slots_required=3;
         avg_anc_spacing = 7;   anc_slots_required=4;
         avg_sbus_spacing = 9; sbus_slots_required=1;
          macsec_slots_required=17;
      break;
    }
    for(i_tmp=0; i_tmp<cal_len; i_tmp++){
      if(mmu_tdm_tbl[i_tmp] == MN_ANCL_TOKEN && (num_lpbk_slots % avg_lpbk_spacing == 0) && (alloted_lb_slots<lb_slots_required) ){
        mmu_tdm_tbl[i_tmp] = MN_LOOPBACK_PORT;
			  TDM_PRINT2("MMU TDM TABLE, element #%0d, contains LOOPBACK port #%0d \n", i_tmp, mmu_tdm_tbl[i_tmp]);
        num_lpbk_slots = num_lpbk_slots +1;
        alloted_lb_slots = alloted_lb_slots + 1;
      } else if (mmu_tdm_tbl[i_tmp] == MN_ANCL_TOKEN) { 
        num_lpbk_slots = num_lpbk_slots +1;
      }
    }
    for(i_tmp=0; i_tmp<cal_len; i_tmp++){
      if(mmu_tdm_tbl[i_tmp] == MN_ANCL_TOKEN && (num_cpu_slots % avg_cpu_spacing == 0) && (alloted_cpu_slots<cpu_slots_required) ){
        mmu_tdm_tbl[i_tmp] = MN_CPU_PORT;
			  TDM_PRINT2("MMU TDM TABLE, element #%0d, contains CPU port #%0d \n", i_tmp, mmu_tdm_tbl[i_tmp]);
        num_cpu_slots = num_cpu_slots +1;
        alloted_cpu_slots = alloted_cpu_slots + 1;
      } else if (mmu_tdm_tbl[i_tmp] == MN_ANCL_TOKEN) { 
        num_cpu_slots = num_cpu_slots +1;
      }
    }
    for(i_tmp=0; i_tmp<cal_len; i_tmp++){
      if(mmu_tdm_tbl[i_tmp] == MN_ANCL_TOKEN && (num_anc_slots % avg_anc_spacing == 0) && (alloted_anc_slots<anc_slots_required) ){
        mmu_tdm_tbl[i_tmp] = MN_AUX_TOKEN;
			  TDM_PRINT2("MMU TDM TABLE, element #%0d, contains AUX port #%0d\n", i_tmp, mmu_tdm_tbl[i_tmp]);
        num_anc_slots = num_anc_slots +1;
        alloted_anc_slots = alloted_anc_slots + 1;
      } else if (mmu_tdm_tbl[i_tmp] == MN_ANCL_TOKEN) { 
        num_anc_slots = num_anc_slots +1;
      }
    }
    for(i_tmp=0; i_tmp<cal_len; i_tmp++){
      if(mmu_tdm_tbl[i_tmp] == MN_ANCL_TOKEN && (num_sbus_slots % avg_sbus_spacing == 0) && (alloted_sbus_slots<sbus_slots_required) ){
        mmu_tdm_tbl[i_tmp] = MN_SBUS_TOKEN;
			  TDM_PRINT2("MMU TDM TABLE, element #%0d, contains SBUS port #%0d\n", i_tmp, mmu_tdm_tbl[i_tmp]);
        num_sbus_slots = num_sbus_slots +1;
        alloted_sbus_slots = alloted_sbus_slots + 1;
      } else if (mmu_tdm_tbl[i_tmp] == MN_ANCL_TOKEN) { 
        num_sbus_slots = num_sbus_slots +1;
      }
    }
    for(i_tmp=0; i_tmp<cal_len; i_tmp++){
      if(macsec_slots_required>0){
        if(mmu_tdm_tbl[i_tmp] == MN_ANCL_TOKEN  && (alloted_macsec_slots<macsec_slots_required) ){
          mmu_tdm_tbl[i_tmp] = MN_MACSEC_PORT;
		      TDM_PRINT2("MMU TDM TABLE, element #%0d, contains MACSEC port #%0d\n", i_tmp, mmu_tdm_tbl[i_tmp]);
          num_macsec_slots = num_macsec_slots +1;
          alloted_macsec_slots = alloted_macsec_slots + 1;
        } else if (mmu_tdm_tbl[i_tmp] == MN_ANCL_TOKEN) {
          num_macsec_slots = num_macsec_slots +1;
        }
      }
    }
  }
	for (j=0; j<k; j++) {
		if (mmu_tdm_tbl[j]!=MN_ANCL_TOKEN ) {
			TDM_PRINT3("PIPE: %s, MMU TDM TABLE, element #%0d, contains physical port #%0d\n", name, j, mmu_tdm_tbl[j]);
		}
	 	else {
			TDM_PRINT2("PIPE: %s, MMU TDM TABLE, element #%0d, CONTAINS ACCESSORY TOKEN - assigned as ", name, j);
			m++;
			if (bw==933) {
				if (_tdm->_core_data.vars_pkg.cal_id==MN_XPIPE_CAL_ID) {
					switch (m) {
						case 1: case 3: case 5: case 7:
							MN_ANCL(j)
						case 2: case 6: 
							MN_CMIC(j)
            case 4: case 9:
							MN_LPBK(j)
            case 8:
							MN_SBUS(j)
					}
				}
			}
			else if (bw==840) {
				if (_tdm->_core_data.vars_pkg.cal_id==MN_XPIPE_CAL_ID) {
					switch (m) {
						case 1: case 3: case 5: case 7:
							MN_ANCL(j)
						case 2: case 6: 
							MN_CMIC(j)
            case 4: case 9:
							MN_LPBK(j)
            case 8:
							MN_SBUS(j)
					}
				}
			}
			else if (bw==793 || bw==794) {
				if (_tdm->_core_data.vars_pkg.cal_id==MN_XPIPE_CAL_ID) {
					switch (m) {
						case 3: case 10:
							MN_ANCL(j)
						case 1: case 4: case 8: case 11: 
							MN_LPBK(j)
            case 6: case 13:
							MN_CMIC(j)
            case 7:
							MN_SBUS(j)
            case 2: case 5: case 9: case 12:
							MN_MACSEC(j)
					}
				}
			}
			else if (bw==705) {
				if (_tdm->_core_data.vars_pkg.cal_id==MN_XPIPE_CAL_ID) {
					switch (m) {
            case 3: case 4: case 7: case 8: case 10: case 12: case 13: case 15: case 20: case 23: case 25: case 26: case 28: case 30: case 31: case 33:
							MN_ANCL(j)
            case 2: case 11: case 18: case 27:
							MN_LPBK(j)
            case 6: case 22:
							MN_CMIC(j)
            case 16: case 34:
							MN_SBUS(j)
            case 1: case 5: case 9: case 14: case 17: case 19: case 21: case 24: case 29: case 32:
							MN_MACSEC(j)
					}
				}
			}
			else if (bw==815 || bw==816) {
				/*if (_tdm->_core_data.vars_pkg.cal_id==MN_XPIPE_CAL_ID) {
					switch (m) {
            case 4:case 10: case 19:
							MN_ANCL(j)
            case 2: case 11: case 16: case 7: case 25:case 29:
							MN_LPBK(j)
            case 6: case 14: case 23:
							MN_CMIC(j)
            case 12: case 18:
							MN_SBUS(j)
            case 1: case 3: case 5: case 7: case 9: case 11: case 13: case 15: case 17: case 5: case 20: case 22: case 24: case 18: case 20:
							MN_MACSEC(j)
            default :  MN_IDLE(j) 
					}
				} */
			}
			else if (bw==817) {
				if (_tdm->_core_data.vars_pkg.cal_id==MN_XPIPE_CAL_ID) {
					/*switch (m) {
            case 3: case 11: case 5:
							MN_ANCL(j)
            case 2: case 4: case 6: case 8: case 10: case 12: 
							MN_LPBK(j)
            case 7: case 9: case 13:
							MN_CMIC(j)
            case 1:
							MN_SBUS(j)
					}*/
				}
			}
			else if (bw==818) {
				if (_tdm->_core_data.vars_pkg.cal_id==MN_XPIPE_CAL_ID) {
					/*switch (m) {
            case 3: case 11: case 14:
							MN_ANCL(j)
            case 2: case 4: case 6: case 8: case 10: case 12: 
							MN_LPBK(j) 
            case 7: case 9: case 13:
							MN_CMIC(j)
            case 1: case 5:
							MN_SBUS(j)
            default :
              MN_IDLE(j)
					}*/
				}
			}
			else if (bw==819) {
				if (_tdm->_core_data.vars_pkg.cal_id==MN_XPIPE_CAL_ID) {
					/*switch (m) {
            case 10: case 16: case  22:
							MN_ANCL(j)
            case 2: case 27: case 8: case 12: case 20:
							MN_LPBK(j)
            case 14: case 18: case 24:
							MN_CMIC(j)
            case 4:
							MN_SBUS(j)
            case 1: case 3: case 5: case 7: case 9: case 11: case 13: case 15: case 17: case 19: case 21: case 23: case 25: case 26: case 6:
							MN_MACSEC(j)
            default :
              MN_IDLE(j)
					}*/
				}
			}
			else if (bw==820) {
				if (_tdm->_core_data.vars_pkg.cal_id==MN_XPIPE_CAL_ID) {
					/*switch (m) {
            case 10: case 18: case  22: case 29:
							MN_ANCL(j)
            case 2: case 27: case 8: case 12: case 16: case 31:
							MN_LPBK(j)
            case 14: case 20: case 24: 
							MN_CMIC(j)
            case 4:
							MN_SBUS(j)
            case 1: case 3: case 5: case 7: case 9: case 11: case 13: case 15: case 17: case 19: case 21: case 23: case 25: case 26: case 6: case 28: case 30:
							MN_MACSEC(j)
            default :
              MN_IDLE(j)
					}*/
				}
			}
			else if (bw==861 || bw==862) {
				if (_tdm->_core_data.vars_pkg.cal_id==MN_XPIPE_CAL_ID) {
					switch (m) {
            case 2: case 5:
							MN_ANCL(j)
            case 1:
							MN_LPBK(j)
            case 3:
							MN_CMIC(j)
            case 4:
							MN_SBUS(j)
					}
				}
			}
			else if (bw==525 ) {
				if (_tdm->_core_data.vars_pkg.cal_id==MN_XPIPE_CAL_ID) {
					switch (m) {
            case 2: case 6: case 9:
							MN_ANCL(j)
            case 1: case 4: case 7: case 10:
							MN_LPBK(j)
            case 3: case 8:
							MN_CMIC(j)
            case 5:
							MN_SBUS(j)
					}
				}
			}
			else if (bw==575) {
				if (_tdm->_core_data.vars_pkg.cal_id==MN_XPIPE_CAL_ID) {
					switch (m) {
						case 1: case 5: 
							MN_CMIC(j)
						case 2: case 4: case 6: case 8:
							MN_ANCL(j)
            case 3: case 7:
              MN_LPBK(j)
            case 9:
							MN_SBUS(j)
					}
				}
			}
			else if (bw==510) {
				if (_tdm->_core_data.vars_pkg.cal_id==MN_XPIPE_CAL_ID) {
					switch (m) {
						case 1: case 3: case 5: case 7: case 9:
							MN_CMIC(j)
						case 2: case 4: case 6: case 8:
							MN_ANCL(j)
            case 10:
							MN_SBUS(j)
					}
				}
			}
		  else if (bw==435) { 
			  if (_tdm->_core_data.vars_pkg.cal_id==MN_XPIPE_CAL_ID) {
					switch (m) {
						case 1: case 3: case 6: case 8:
							MN_ANCL(j)
            case 2: case 5: case 7:
              MN_LPBK(j)
						case 4: case 9: 
							MN_CMIC(j)
            case 10:
							MN_SBUS(j)
					}
        }
      }
			else {
				TDM_PRINT0("INVALID PARAMETERS\n");
			}
		}
	}

	for (j=0; j<MN_OS_VBS_GRP_LEN; j++) {
		TDM_PRINT3("PIPE: %s, MMU OVS BUCKET 0, element #%0d, contains physical port #%0d\n", name, j, mmu_tdm_ovs_1[j]);
	}
	for (j=0; j<MN_OS_VBS_GRP_LEN; j++) {
		TDM_PRINT3("PIPE: %s, MMU OVS BUCKET 1, element #%0d, contains physical port #%0d\n", name, j, mmu_tdm_ovs_2[j]);
	}
	for (j=0; j<MN_OS_VBS_GRP_LEN; j++) {
		TDM_PRINT3("PIPE: %s, MMU OVS BUCKET 2, element #%0d, contains physical port #%0d\n", name, j, mmu_tdm_ovs_3[j]);
	}
	for (j=0; j<MN_OS_VBS_GRP_LEN; j++) {
		TDM_PRINT3("PIPE: %s, MMU OVS BUCKET 3, element #%0d, contains physical port #%0d\n", name, j, mmu_tdm_ovs_4[j]);
	}
	for (j=0; j<MN_OS_VBS_GRP_LEN; j++) {
		TDM_PRINT3("PIPE: %s, MMU OVS BUCKET 4, element #%0d, contains physical port #%0d\n", name, j, mmu_tdm_ovs_5[j]);
	}
	for (j=0; j<MN_OS_VBS_GRP_LEN; j++) {
		TDM_PRINT3("PIPE: %s, MMU OVS BUCKET 5, element #%0d, contains physical port #%0d\n", name, j, mmu_tdm_ovs_6[j]);
	}
	
	return ( ( _tdm->_core_exec[TDM_CORE_EXEC__POST]( _tdm ) ) );
}
#endif /* CDK_CONFIG_INCLUDE_BCM56670_A0 */
