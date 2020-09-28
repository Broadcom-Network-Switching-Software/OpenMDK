/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 * $All Rights Reserved.$
 *
 * TDM chip data structure scanning functions
 */
#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53570_B0 == 1

#include "bcm53570_b0_tdm_core_top.h"


/**
@name: tdm_gh2_which_tsc
@param:

Returns the TSC to which the input port belongs given pointer to transcribed pmap
 */
int
tdm_gh2_which_tsc( tdm_mod_t *_tdm_s )
{
    GH2_TOKEN_CHECK(_tdm_s->_core_data.vars_pkg.port) {
        return tdm_find_pm( _tdm_s );
    }

    return GH2_NUM_EXT_PORTS;
}


/**
@name: tdm_gh2_check_ethernet
@param:

Returns BOOL_TRUE or BOOL_FALSE depending on if pipe of the given port has traffic entirely Ethernet
 */
int
tdm_gh2_check_ethernet( tdm_mod_t *_tdm )
{
	int i, idx_s=0, idx_e=0, port, port_speed, port_state, type=BOOL_TRUE;
	
    port = _tdm->_core_data.vars_pkg.port;
    
	if (port<=32) {
		idx_s = 1;
		idx_e = 32;
	}
	else if (port<=64){
		idx_s = 33;
		idx_e = 64;
	}
	else if (port<=96){
		idx_s = 65;
		idx_e = 96;
	}
	else if (port<=128){
		idx_s = 97;
		idx_e = 128;
	}

	for (i=idx_s; i<idx_e; i++) {
        port_state = _tdm->_chip_data.soc_pkg.state[i-1];
        port_speed = _tdm->_chip_data.soc_pkg.speed[i];
        if ( (port_speed>SPEED_0) && 
             (port_state==PORT_STATE__LINERATE_HG ||
              port_state==PORT_STATE__OVERSUB_HG   ) ){
			type=BOOL_FALSE;
			break;
		}
	}
	
	return type;
}


/**
@name: tdm_gh2_legacy_which_tsc
@param:

Returns the TSC to which the input port belongs given pointer to transcribed pmap
 */
int
tdm_gh2_legacy_which_tsc(unsigned short port, int **tsc)
{
	int i, j, which=GH2_NUM_EXT_PORTS;
	
	GH2_TOKEN_CHECK(port) {
		for (i=0; i<GH2_NUM_PHY_PM; i++) {
			for (j=0; j<GH2_NUM_PM_LNS; j++) {
				if (tsc[i][j]==port) {
					which=i;
				}
			}
			if (which!=GH2_NUM_EXT_PORTS) {
				break;
			}
		}
	}
	
	return which;
	
}


/**
@name: tdm_gh2_check_same_port_dist_dn
@param:

Returns distance to next index with same port number, in down direction
Wraparound without mirroring
 */
int
tdm_gh2_check_same_port_dist_dn(int idx, int *tdm_tbl, int lim)
{
	int j, dist=1, slot;

	slot=idx;
	for (j=1; j<lim; j++) {
		if (++slot==lim) {slot=0;}
		if (tdm_tbl[slot]==tdm_tbl[idx]) {
			break;
		}
		dist++;
	}

	return dist;

}


/**
@name: tdm_gh2_check_same_port_dist_up
@param:

Returns distance to next index with same port number, in down direction
Wraparound without mirroring
 */
int
tdm_gh2_check_same_port_dist_up(int idx, int *tdm_tbl, int lim)
{
	int j, dist=1, slot;

	slot=idx;
	for (j=1; j<lim; j++) {
		if (--slot<=0) {slot=(lim-1);}
		if (tdm_tbl[slot]==tdm_tbl[idx]) {
			break;
		}
		dist++;
	}

	return dist;

}


/**
@name: tdm_gh2_check_same_port_dist_dn_port
@param:

Returns distance to next index with same port number, in down direction
Wraparound without mirroring
 */
int
tdm_gh2_check_same_port_dist_dn_port(int port, int idx, int *tdm_tbl, int lim)
{
	int j, dist=1, slot;

	slot=idx;
	for (j=1; j<lim; j++) {
		if (++slot==lim) {slot=0;}
		if (tdm_tbl[slot]==port) {
			break;
		}
		dist++;
	}

	return dist;
}


/**
@name: tdm_gh2_check_same_port_dist_up_port
@param:

Returns distance to next index with same port number, in down direction
Wraparound without mirroring
 */
int
tdm_gh2_check_same_port_dist_up_port(int port, int idx, int *tdm_tbl, int lim)
{
	int j, dist=1, slot;

	slot=idx;
	for (j=1; j<lim; j++) {
		if (--slot<=0) {slot=(lim-1);}
		if (tdm_tbl[slot]==port) {
			break;
		}
		dist++;
	}

	return dist;
}


/**
@name: tdm_gh2_slice_size_local
@param:

Given index, returns size of largest contiguous slice
 */
int
tdm_gh2_slice_size_local(unsigned short idx, int *tdm, int lim)
{
	int i, slice_size=(-1);
	
	if (tdm[idx]!=GH2_OVSB_TOKEN && tdm[idx]!=GH2_NUM_EXT_PORTS) {
		for (i=idx; i>=0; i--) {
			if (tdm[i]!=GH2_OVSB_TOKEN && tdm[i]!=GH2_NUM_EXT_PORTS) {
				slice_size++;
			}
			else {
				break;
			}
		}
		for (i=idx; i<lim; i++) {
			if (tdm[i]!=GH2_OVSB_TOKEN && tdm[i]!=GH2_NUM_EXT_PORTS) {
				slice_size++;
			}
			else {
				break;
			}
		}
	}
	else if (tdm[idx]==GH2_OVSB_TOKEN) {
		for (i=idx; i>=0; i--) {
			if (tdm[i]==GH2_OVSB_TOKEN) {
				slice_size++;
			}
			else {
				break;
			}
		}
		for (i=idx; i<lim; i++) {
			if (tdm[i]==GH2_OVSB_TOKEN) {
				slice_size++;
			}
			else {
				break;
			}
		}
	}

	return slice_size;
}


/**
@name: tdm_gh2_slice_size
@param:

Given port number, returns size of largest slice
 */
int
tdm_gh2_slice_size(unsigned short port, int *tdm, int lim)
{
	int i, j, k=0, slice_size=0;
	
	if (port<129 && port>0) {
		for (i=0; i<lim; i++) {
			GH2_TOKEN_CHECK(tdm[i]) {
				k=1;
				for (j=(i+1); j<lim; j++) {
					GH2_TOKEN_CHECK(tdm[j]) {k++;}
					else {break;}
				}
			}
			slice_size = (k>slice_size)?(k):(slice_size);
		}
	}
	else {
		for (i=2; i<lim; i++) {
			if (tdm[i]==port) {
				k=1;
				for (j=(i+1); j<lim; j++) {
					if (tdm[j]==port) {k++;}
					else {break;}
				}
			}
			slice_size = (k>slice_size)?(k):(slice_size);
		}
	}

	return slice_size;
}


/**
@name: tdm_gh2_slice_idx
@param:

Given port number, returns index of largest slice
 */
int
tdm_gh2_slice_idx(unsigned short port, int *tdm, int lim)
{
	int i, j, k=0, slice_size=0, slice_idx=0;
	
	if (port<129 && port>0) {
		for (i=0; i<lim; i++) {
			GH2_TOKEN_CHECK(tdm[i]) {
				k=1;
				for (j=(i+1); j<lim; j++) {
					GH2_TOKEN_CHECK(tdm[j]) {k++;}
					else {break;}
				}
			}
			if (k>slice_size) {
				slice_idx=i;
				slice_size=k;
			}
		}
	}
	else {
		for (i=2; i<lim; i++) {
			if (tdm[i]==port) {
				k=1;
				for (j=(i+1); j<lim; j++) {
					if (tdm[j]==port) {k++;}
					else {break;}
				}
			}
			if (k>slice_size) {
				slice_idx=i;
				slice_size=k;
			}
		}
	}
	
	return slice_idx;
}


/**
@name: tdm_gh2_slice_prox_dn
@param:

Given port number, checks min spacing in a slice in down direction
 */
int
tdm_gh2_slice_prox_dn(int slot, int *tdm, int lim, int **tsc, enum port_speed_e *speed)
{
	int i, cnt=0, wc, idx=(slot+1), slice_prox=PASS;
	
	wc=(tdm[slot]==GH2_ANCL_TOKEN)?(tdm[slot]):(tdm_gh2_legacy_which_tsc(tdm[slot],tsc));
	if (slot<=(lim-5)) {
		if ( wc==tdm_gh2_legacy_which_tsc(tdm[slot+1],tsc) ||
		     wc==tdm_gh2_legacy_which_tsc(tdm[slot+2],tsc) ||
		     wc==tdm_gh2_legacy_which_tsc(tdm[slot+3],tsc) ||
			 wc==tdm_gh2_legacy_which_tsc(tdm[slot+4],tsc) ) {
			slice_prox=FAIL;
		}
	}
	else {
		while (idx<lim) {
			if (wc==tdm_gh2_legacy_which_tsc(tdm[idx],tsc)) {
				slice_prox=FAIL;
				break;
			}
			idx++; cnt++;
		}
		for (i=(lim-slot-cnt-1); i>=0; i--) {
			if (wc==tdm_gh2_legacy_which_tsc(tdm[i],tsc)) {
				slice_prox=FAIL;
				break;
			}
		}
	}
/* #ifdef _LLS_SCHEDULER */
	{
		int tmp_i=slot, j;
		if (speed[tdm[tmp_i]]<=SPEED_42G_HG2) {
			if (tmp_i<(GH2_VMAP_MAX_LEN-1)) {
				for (j=1; j<11; j++) {
					if (tdm[tmp_i+j]==tdm[tmp_i]) {
						slice_prox=FAIL;
						break;
					}
				}
			}
		}
	}
/* #endif */

	return slice_prox;
}


/**
@name: tdm_gh2_slice_prox_up
@param:

Given port number, checks min spacing in a slice in up direction
 */
int
tdm_gh2_slice_prox_up(int slot, int *tdm, int **tsc, enum port_speed_e *speed)
{
	int wc, slice_prox=PASS;
	
	wc=(tdm[slot]==GH2_ANCL_TOKEN)?(tdm[slot]):(tdm_gh2_legacy_which_tsc(tdm[slot],tsc));
	if (slot>=4) {
		if ( wc==tdm_gh2_legacy_which_tsc(tdm[slot-1],tsc) ||
		     wc==tdm_gh2_legacy_which_tsc(tdm[slot-2],tsc) ||
		     wc==tdm_gh2_legacy_which_tsc(tdm[slot-3],tsc) ||
			 wc==tdm_gh2_legacy_which_tsc(tdm[slot-4],tsc) ) {
			slice_prox=FAIL;
		}
	}
/* #ifdef _LLS_SCHEDULER */
	{
		int i=slot, j;
		if (speed[tdm[i]]<=SPEED_42G_HG2) {
			if (i>=1) {
				for (j=1; j<11; j++) {
					if (tdm[i-j]==tdm[i]) {
						slice_prox=FAIL;
						break;
					}
				}
			}
		}
	}
/* #endif */

	return slice_prox;
}


/**
@name: tdm_gh2_check_fit_smooth
@param:

Inside of table array, returns number of nodes inside a port vector that clump with other nodes of the same type
 */
int
tdm_gh2_check_fit_smooth(int *tdm_tbl, int port, int lr_idx_limit, int clump_thresh)
{
	int i, cnt=0;

	for (i=0; i<lr_idx_limit; i++) {
		if ( (tdm_tbl[i]==port) && (tdm_gh2_slice_size_local(i,tdm_tbl,lr_idx_limit)>=clump_thresh) ) {
			cnt++;
		}
	}

	return cnt;

}


/**
@name: tdm_gh2_check_lls_flat_up
@param:

Checks LLS scheduler min spacing in tdm array, up direction only, returns dist
 */
int
tdm_gh2_check_lls_flat_up(int idx, int *tdm_tbl, enum port_speed_e *speed)
{
	int lls_prox=GH2_VMAP_MAX_LEN;

/* #ifdef _LLS_SCHEDULER */
	{
		int i=idx, j;
		lls_prox=1;
		if (i>=11 && tdm_tbl[idx]<=SPEED_42G_HG2) {
			for (j=1; j<11; j++) {
				if (tdm_tbl[i-j]==tdm_tbl[i]) {
					break;
				}
				lls_prox++;
			}
		}
	}
/* #endif */

	return lls_prox;

}


/**
@name: tdm_gh2_slice_prox_local
@param:

Given index, checks min spacing of two nearest non-token ports
 */
int
tdm_gh2_slice_prox_local(unsigned short idx, int *tdm, int lim, int **tsc)
{
	int i, prox_len=0, wc=GH2_NUM_EXT_PORTS;
	
	/* Nearest non-token port */
	GH2_TOKEN_CHECK(tdm[idx]) {
		wc=tdm_gh2_legacy_which_tsc(tdm[idx],tsc);
	}
	else {
		for (i=1; (idx-i)>=0; i++) {
			GH2_TOKEN_CHECK(tdm[i]) {
				wc=tdm_gh2_legacy_which_tsc(tdm[idx-i],tsc);
				break;
			}
		}
	}
	for (i=1; (idx+i)<lim; i++) {
		if (tdm_gh2_legacy_which_tsc(tdm[idx+i],tsc)!=wc) {
			prox_len++;
		}
		else {
			break;
		}
	}

	return prox_len;
}


/**
@name: tdm_gh2_scan_lr_slot_num
@param:
 */
int
tdm_gh2_scan_lr_slot_num(int *tdm_tbl)
{
	int i, cnt=0;
	
	for (i=0; i<GH2_VMAP_MAX_LEN; i++) {
		GH2_TOKEN_CHECK(tdm_tbl[i]) {
			cnt++;
		}
	}
	
	return cnt;
}


/**
@name: tdm_gh2_pick_vec
@param:

Select vector index on x axis to rotate based on priority of TSC pipeline
 */
int
tdm_gh2_pick_vec( tdm_mod_t *_tdm )
{
	int i, vec_sel=1, port=_tdm->_core_data.vars_pkg.port;
	
	for (i=_tdm->_core_data.vars_pkg.m_tdm_pick_vec.prev_vec; i<_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.lr_vec_cnt; i++) {
		_tdm->_core_data.vars_pkg.port=_tdm->_core_data.vmap[i][0];
		if ( (_tdm->_core_exec[TDM_CORE_EXEC__PM_SCAN](_tdm)==_tdm->_core_data.vars_pkg.m_tdm_pick_vec.tsc_dq) ) {
			if (_tdm->_core_data.vars_pkg.m_tdm_pick_vec.triport_priority) {
				if (tdm_find_fastest_triport(_tdm)) {
					vec_sel=i;
					_tdm->_core_data.vars_pkg.m_tdm_pick_vec.triport_priority=BOOL_FALSE;
					break;
				}
				else {
					continue;
				}
			}
			else {
				vec_sel=i;
				break;
			}
		}
	}
	
	_tdm->_core_data.vars_pkg.port=port;
	return vec_sel;
	
}


/**
@name: tdm_gh2_scan_slice_size_local
@param:

Given index, returns the largest size of local slice
 */
int
tdm_gh2_scan_slice_size_local(unsigned short idx, int *tdm, int lim, int *slice_start_idx)
{
	int i, slice_size=(-1), idx_start=(-1);
	
	if(idx<lim){
		/* linerate */
		GH2_TOKEN_CHECK(tdm[idx]){
			for (i=idx; i>=0; i--) {
				GH2_TOKEN_CHECK(tdm[i]) {slice_size++; idx_start=i;}
				else {break;}
			}
			for (i=idx; i<lim; i++) {
				GH2_TOKEN_CHECK(tdm[i]) {slice_size++;}
				else {break;}
			}
		}
		/* ovsb */
		if (tdm[idx]==GH2_OVSB_TOKEN) {
			for (i=idx; i>=0; i--) {
				if (tdm[i]==GH2_OVSB_TOKEN) {slice_size++; idx_start=i;}
				else {break;}
			}
			for (i=idx; i<lim; i++) {
				if (tdm[i]==GH2_OVSB_TOKEN) {slice_size++;}
				else {break;}
			}
		}
		/* idle */
		else if (tdm[idx]==GH2_IDL1_TOKEN || tdm[idx]==GH2_IDL2_TOKEN) {
			for (i=idx; i>=0; i--) {
				if (tdm[i]==GH2_IDL1_TOKEN || tdm[i]==GH2_IDL2_TOKEN) {slice_size++; idx_start=i;}
				else {break;}
			}
			for (i=idx; i<lim; i++) {
				if (tdm[i]==GH2_IDL1_TOKEN || tdm[i]==GH2_IDL2_TOKEN) {slice_size++;}
				else {break;}
			}
		}
	}
	
	(*slice_start_idx) = idx_start;
	return slice_size;
}


/**
@name: tdm_gh2_scan_slice_min
@param:

Given port number, returns the MIN size of port slices in an array
 */
int
tdm_gh2_scan_slice_min(unsigned short port, int *tdm, int lim, int *slice_start_idx, int pos)
{
	int i, k=0, idx0, slice_size_min=256, slice_idx=-1 , idx_start;
	
	if(pos>=0 && pos<lim){
		/* linerate */
		GH2_TOKEN_CHECK(port) {
			for (i=0; i<lim; i++) {
				idx0 = ((i+pos)<lim)?(i+pos):(i+pos-lim);
				GH2_TOKEN_CHECK(tdm[idx0]) {
					k = tdm_gh2_scan_slice_size_local(idx0, tdm, lim, &idx_start);
					if(k>0 && k<slice_size_min){
						slice_size_min = k;
						slice_idx = idx_start;
					}
				}
			}
		}
		/* oversub */
		else if (port==GH2_OVSB_TOKEN){
			for (i=0; i<lim; i++) {
				idx0 = ((i+pos)<lim)?(i+pos):(i+pos-lim);
				if (tdm[idx0]==GH2_OVSB_TOKEN) {
					k = tdm_gh2_scan_slice_size_local(idx0, tdm, lim, &idx_start);
					if(k>0 && k<slice_size_min){
						slice_size_min = k;
						slice_idx = idx_start;
					}
				}
			}
		}
		/* idle */
		else if (port==GH2_IDL1_TOKEN || port==GH2_IDL2_TOKEN ){
			for (i=0; i<lim; i++) {
				idx0 = ((i+pos)<lim)?(i+pos):(i+pos-lim);
				if (tdm[idx0]==GH2_IDL1_TOKEN || tdm[idx0]==GH2_IDL2_TOKEN) {
					k = tdm_gh2_scan_slice_size_local(idx0, tdm, lim, &idx_start);
					if(k>0 && k<slice_size_min){
						slice_size_min = k;
						slice_idx = idx_start;
					}
				}
			}
		}
	}
	
	(*slice_start_idx) = slice_idx;
	return slice_size_min;
}


/**
@name: tdm_gh2_scan_slice_max
@param:

Given port number, returns the MAX size of port slices in an array
 */
int
tdm_gh2_scan_slice_max(unsigned short port, int *tdm, int lim, int *slice_start_idx, int pos)
{
	int i, k=0, idx0, slice_size_max=0, slice_idx=-1 , idx_start;
	
	if(pos>=0 && pos<lim){
		/* linerate */
		GH2_TOKEN_CHECK(port) {
			for (i=0; i<lim; i++) {
				idx0 = ((i+pos)<lim)?(i+pos):(i+pos-lim);
				GH2_TOKEN_CHECK(tdm[idx0]) {
					k = tdm_gh2_scan_slice_size_local(idx0, tdm, lim, &idx_start);
					if(k>slice_size_max){
						slice_size_max = k;
						slice_idx = idx_start;
					}
				}
			}
		}
		/* oversub */
		else if (port==GH2_OVSB_TOKEN){
			for (i=0; i<lim; i++) {
				idx0 = ((i+pos)<lim)?(i+pos):(i+pos-lim);
				if (tdm[idx0]==GH2_OVSB_TOKEN) {
					k = tdm_gh2_scan_slice_size_local(idx0, tdm, lim, &idx_start);
					if(k>slice_size_max){
						slice_size_max = k;
						slice_idx = idx_start;
					}
				}
			}
		}
		/* idle */
		else if (port==GH2_IDL1_TOKEN || port==GH2_IDL2_TOKEN ){
			for (i=0; i<lim; i++) {
				idx0 = ((i+pos)<lim)?(i+pos):(i+pos-lim);
				if (tdm[idx0]==GH2_IDL1_TOKEN || tdm[idx0]==GH2_IDL2_TOKEN) {
					k = tdm_gh2_scan_slice_size_local(idx0, tdm, lim, &idx_start);
					if(k>slice_size_max){
						slice_size_max = k;
						slice_idx = idx_start;
					}
				}
			}
		}
	}
	
	(*slice_start_idx) = slice_idx;
	return slice_size_max;
}


/**
@name: tdm_gh2_scan_mix_slice_size_local
@param:

Given index, returns the largest size of local slice (mixed with ANCL)
 */
int
tdm_gh2_scan_mix_slice_size_local(unsigned short idx, int *tdm, int lim, int *slice_start_idx)
{
	int i, slice_size=(-1), idx_start=(-1);
	
	if(idx<lim){
		/* linerate mix ancl */
		GH2_TOKEN_CHECK(tdm[idx]){
			for (i=idx; i>=0; i--) {
				GH2_TOKEN_CHECK(tdm[i]) {slice_size++; idx_start=i;}
				else if (tdm[i]==GH2_ANCL_TOKEN) {slice_size++; idx_start=i;}
				else {break;}
			}
			for (i=idx; i<lim; i++) {
				GH2_TOKEN_CHECK(tdm[i]) {slice_size++;}
				else if (tdm[i]==GH2_ANCL_TOKEN) {slice_size++;}
				else {break;}
			}
		}
		/* ancl mix linerate */
		else if (tdm[idx]==GH2_ANCL_TOKEN){
			for (i=idx; i>=0; i--) {
				GH2_TOKEN_CHECK(tdm[i]) {slice_size++; idx_start=i;}
				else if (tdm[i]==GH2_ANCL_TOKEN) {slice_size++; idx_start=i;}
				else {break;}
			}
			for (i=idx; i<lim; i++) {
				GH2_TOKEN_CHECK(tdm[i]) {slice_size++;}
				else if (tdm[i]==GH2_ANCL_TOKEN) {slice_size++;}
				else {break;}
			}
		}
		/* oversub mix ancl */
		else if (tdm[idx]==GH2_OVSB_TOKEN) {
			for (i=idx; i>=0; i--) {
				if      (tdm[i]==GH2_OVSB_TOKEN) {slice_size++; idx_start=i;}
				else if (tdm[i]==GH2_ANCL_TOKEN) {slice_size++; idx_start=i;}
				else {break;}
			}
			for (i=idx; i<lim; i++) {
				if      (tdm[i]==GH2_OVSB_TOKEN) {slice_size++;}
				else if (tdm[i]==GH2_ANCL_TOKEN) {slice_size++;}
				else {break;}
			}
		}
		/* idle mix ancl */
		else if (tdm[idx]==GH2_IDL1_TOKEN || tdm[idx]==GH2_IDL2_TOKEN) {
			for (i=idx; i>=0; i--) {
				if      (tdm[i]==GH2_IDL1_TOKEN || tdm[i]==GH2_IDL2_TOKEN) {slice_size++; idx_start=i;}
				else if (tdm[i]==GH2_ANCL_TOKEN) {slice_size++; idx_start=i;}
				else {break;}
			}
			for (i=idx; i<lim; i++) {
				if      (tdm[i]==GH2_IDL1_TOKEN || tdm[i]==GH2_IDL2_TOKEN) {slice_size++;}
				else if (tdm[i]==GH2_ANCL_TOKEN) {slice_size++;}
				else {break;}
			}
		}
	}
	
	(*slice_start_idx) = idx_start;
	return slice_size;
}


/**
@name: tdm_gh2_slice_size_min
@param:

Given port number, returns the MIN size of port slices (mixed with ANCL) in an array
 */
int
tdm_gh2_scan_mix_slice_min(unsigned short port, int *tdm, int lim, int *slice_start_idx, int pos)
{
	int i, k=0, idx0, slice_size_min=256, slice_idx=-1 , idx_start;
	if (pos>=0 && pos<lim) {
		/* linerate */
		GH2_TOKEN_CHECK(port) {
			for (i=0; i<lim; i++) {
				idx0 = ((i+pos)<lim)?(i+pos):(i+pos-lim);
				GH2_TOKEN_CHECK(tdm[idx0]) {
					k = tdm_gh2_scan_mix_slice_size_local(idx0, tdm, lim, &idx_start);
					if(k>0 && k<slice_size_min){
						slice_size_min = k;
						slice_idx= idx_start;
					}
				}
			}
		}
		/* oversub */
		else if (port==GH2_OVSB_TOKEN){
			for (i=0; i<lim; i++) {
				idx0 = ((i+pos)<lim)?(i+pos):(i+pos-lim);
				if (tdm[idx0]==GH2_OVSB_TOKEN) {
					k = tdm_gh2_scan_mix_slice_size_local(idx0, tdm, lim, &idx_start);
					if(k>0 && k<slice_size_min){
						slice_size_min = k;
						slice_idx= idx_start;
					}
				}
			}
		}
		/* idle */
		else if (port==GH2_IDL1_TOKEN || port==GH2_IDL2_TOKEN ){
			for (i=0; i<lim; i++) {
				idx0 = ((i+pos)<lim)?(i+pos):(i+pos-lim);
				if (tdm[idx0]==GH2_IDL1_TOKEN || tdm[idx0]==GH2_IDL2_TOKEN) {
					k = tdm_gh2_scan_mix_slice_size_local(idx0, tdm, lim, &idx_start);
					if(k>0 && k<slice_size_min){
						slice_size_min = k;
						slice_idx= idx_start;
					}
				}
			}
		}
	}
	
	(*slice_start_idx) = slice_idx;
	return slice_size_min;
}


/**
@name: tdm_gh2_scan_mix_slice_max
@param:

Given port number, returns the MAX size of port slices (mixed with ANCL) in an array
 */
int
tdm_gh2_scan_mix_slice_max(unsigned short port, int *tdm, int lim, int *slice_start_idx, int pos)
{
	int i, k=0, idx0, slice_size_max=0, slice_idx=-1 , idx_start;
	if (pos>=0 && pos<lim) {
		/* linerate */
		GH2_TOKEN_CHECK(port) {
			for (i=0; i<lim; i++) {
				idx0 = ((i+pos)<lim)?(i+pos):(i+pos-lim);
				GH2_TOKEN_CHECK(tdm[idx0]) {
					k = tdm_gh2_scan_mix_slice_size_local(idx0, tdm, lim, &idx_start);
					if(k>slice_size_max){
						slice_size_max = k;
						slice_idx= idx_start;
					}
				}
			}
		}
		/* oversub */
		else if (port==GH2_OVSB_TOKEN){
			for (i=0; i<lim; i++) {
				idx0 = ((i+pos)<lim)?(i+pos):(i+pos-lim);
				if (tdm[idx0]==GH2_OVSB_TOKEN) {
					k = tdm_gh2_scan_mix_slice_size_local(idx0, tdm, lim, &idx_start);
					if(k>slice_size_max){
						slice_size_max = k;
						slice_idx= idx_start;
					}
				}
			}
		}
		/* idle */
		else if (port==GH2_IDL1_TOKEN || port==GH2_IDL2_TOKEN ){
			for (i=0; i<lim; i++) {
				idx0 = ((i+pos)<lim)?(i+pos):(i+pos-lim);
				if (tdm[idx0]==GH2_IDL1_TOKEN || tdm[idx0]==GH2_IDL2_TOKEN) {
					k = tdm_gh2_scan_mix_slice_size_local(idx0, tdm, lim, &idx_start);
					if(k>slice_size_max){
						slice_size_max = k;
						slice_idx= idx_start;
					}
				}
			}
		}
	}
	
	(*slice_start_idx) = slice_idx;
	return slice_size_max;
}


/**
@name: tdm_gh2_check_slot_swap_cond
@param:

Check if two consecutive slots can be swapped in an array
		--- _X_Y_ -> _Y_X_
		--- [idx]=X
 */
int
tdm_gh2_check_slot_swap_cond(int idx, int *tdm_tbl, int tdm_tbl_len, int **tsc, enum port_speed_e *speed)
{
	int idx_x, idx_y, idx0, tsc0, idx1, tsc1, result, check_pass=BOOL_TRUE;
	
	idx_x = idx;
	idx_y = idx+1;
	if( !(idx>=0 && idx<(tdm_tbl_len-1)) ) {check_pass = BOOL_FALSE;}
	
	/* Check sister port spacing: x3_x2_x1_X_Y_y1_y2_y3 */
	if (check_pass==BOOL_TRUE){
		GH2_TOKEN_CHECK(tdm_tbl[idx_x]){
			idx0 = idx_x;
			idx1 = ((idx_x + VBS_MIN_SPACING)<tdm_tbl_len)? (idx_x + VBS_MIN_SPACING): (idx_x + VBS_MIN_SPACING - tdm_tbl_len);
			tsc0 = tdm_gh2_legacy_which_tsc(tdm_tbl[idx0],tsc);
			tsc1 = tdm_gh2_legacy_which_tsc(tdm_tbl[idx1],tsc);
			if (tsc0==tsc1) {check_pass = BOOL_FALSE;}
		}
		GH2_TOKEN_CHECK(tdm_tbl[idx_y]){
			idx0 = idx_y;
			idx1 = ((idx_y - VBS_MIN_SPACING)>=0)? (idx_y - VBS_MIN_SPACING): (idx_y - VBS_MIN_SPACING + tdm_tbl_len);
			tsc0 = tdm_gh2_legacy_which_tsc(tdm_tbl[idx0],tsc);
			tsc1 = tdm_gh2_legacy_which_tsc(tdm_tbl[idx1],tsc);
			if (tsc0==tsc1) {check_pass = BOOL_FALSE;}
		}
	}
	/* Check same port spacing */
	if (check_pass==BOOL_TRUE){
		GH2_TOKEN_CHECK(tdm_tbl[idx_x]){
			if (speed[tdm_tbl[idx_x]]<=SPEED_42G_HG2) {
				idx0 = ((idx_x + LLS_MIN_SPACING)<tdm_tbl_len)? (idx_x + LLS_MIN_SPACING): (idx_x + LLS_MIN_SPACING - tdm_tbl_len);
				if (tdm_tbl[idx0]==tdm_tbl[idx_x]){
					check_pass = BOOL_FALSE;
				}
			}
		}
		GH2_TOKEN_CHECK(tdm_tbl[idx_y]){
			if (speed[tdm_tbl[idx_y]]<=SPEED_42G_HG2) {
				idx0 = ((idx_y - LLS_MIN_SPACING)>=0)? (idx_y - LLS_MIN_SPACING): (idx_y - LLS_MIN_SPACING + tdm_tbl_len);
				if (tdm_tbl[idx0]==tdm_tbl[idx_y]){
					check_pass = BOOL_FALSE;
				}
			}
		}
	}
	
	result = (check_pass==BOOL_TRUE)? (PASS): (FAIL);
	return result;
}


/**
@name: tdm_gh2_check_shift_cond_pattern
@param:

Check if all slots of the given port can shift UP/DOWN in an array
		--- shift pattern
		--- sister port spacing
 */
int
tdm_gh2_check_shift_cond_pattern(unsigned short port, int *tdm_tbl, int tdm_tbl_len, int **tsc, int dir)
{
	int i, port_tsc, idx0, tsc0, result, shift_cond_pass=BOOL_FALSE;
	
	/* Check port state */
	GH2_TOKEN_CHECK(port) {
		shift_cond_pass = BOOL_TRUE;
	}
	/* Check shift pattern */
	if (shift_cond_pass==BOOL_TRUE) {
		/* Downward pattern: _x_ovsb_..._x_ovsb_..._x_ovsb_ */
		if (dir==DN) {
			for (i=0; i<(tdm_tbl_len-1); i++) {
				if (tdm_tbl[i]==port && tdm_tbl[i+1]!=GH2_OVSB_TOKEN && tdm_tbl[i+1]!=GH2_ANCL_TOKEN) {
					shift_cond_pass=BOOL_FALSE;
					break;
				}
			}
		}
		/* Upward pattern: _ovsb_x_..._ovsb_x_..._ovsb_x_ */
		else{
			for (i=1; i<tdm_tbl_len; i++) {
				if (tdm_tbl[i]==port && tdm_tbl[i-1]!=GH2_OVSB_TOKEN && tdm_tbl[i-1]!=GH2_ANCL_TOKEN) {
					shift_cond_pass=BOOL_FALSE;
					break;
				}
			}
		}
	}
	/* Check sister port spacing */
	if(shift_cond_pass==BOOL_TRUE){
		port_tsc = tdm_gh2_legacy_which_tsc(port,tsc);
		/* Downward pattern: _x_ovsb_..._x_ovsb_..._x_ovsb_ */
		if (dir==DN){
			for (i=0; i<(tdm_tbl_len-1); i++) {
				if (tdm_tbl[i]==port) {
					idx0 = ((i+VBS_MIN_SPACING)<tdm_tbl_len) ? (i+VBS_MIN_SPACING) : (i+VBS_MIN_SPACING-tdm_tbl_len);
					tsc0 = tdm_gh2_legacy_which_tsc(tdm_tbl[idx0],tsc);
					if ( port_tsc==tsc0 ) {
						shift_cond_pass = BOOL_FALSE;
						break;
					}
				}
			}
		}
		/* Upward pattern: _ovsb_x_..._ovsb_x_..._ovsb_x_ */
		else {
			for (i=1; i<tdm_tbl_len; i++) {
				if (tdm_tbl[i]==port) {
					idx0 = ((i-VBS_MIN_SPACING)>=0) ? (i-VBS_MIN_SPACING) : (i-VBS_MIN_SPACING+tdm_tbl_len);
					tsc0 = tdm_gh2_legacy_which_tsc(tdm_tbl[idx0],tsc);
					if ( port_tsc==tsc0 ) {
						shift_cond_pass = BOOL_FALSE;
						break;
					}
				}
			}
		}
	}
	
	result = (shift_cond_pass==BOOL_TRUE)? (PASS): (FAIL);
	
	return result;
}


/**
@name: tdm_gh2_check_shift_cond_local_slice
@param:

Check if all slots of the given port can shift UP/DOWN in an array
		--- local OVSB slice compared with max OVSB slice 
		--- local LINERATE slice compared with max LINERATE slice
 */
int
tdm_gh2_check_shift_cond_local_slice(unsigned short port, int *tdm_tbl, int tdm_tbl_len, int **tsc, int dir)
{
	int i, j, slice_idx, ovsb_token, idx0, idx1, shift_cond_pass, result, shift_dir,
		os_clump_max_last, lr_clump_max_last, lr_clump_min_last, os_clump_local_above, os_clump_local_below,
		filter_port=0, lr_clump_local_last, lr_clump_local_curr;
	
	ovsb_token = GH2_OVSB_TOKEN;
	shift_dir  = (dir==UP) ? (UP) : (DN);
	
	os_clump_max_last = tdm_gh2_scan_slice_max(ovsb_token,tdm_tbl,tdm_tbl_len, &slice_idx, 0);
	lr_clump_max_last = tdm_gh2_scan_mix_slice_max(1,tdm_tbl,tdm_tbl_len, &slice_idx, 0);
	lr_clump_min_last = tdm_gh2_scan_mix_slice_min(1,tdm_tbl,tdm_tbl_len, &slice_idx, 0);
	
	if ( (lr_clump_max_last<=1) || (lr_clump_max_last==2 && lr_clump_min_last==1) ) {
		shift_cond_pass = BOOL_FALSE;
	}
	else {
		shift_cond_pass = BOOL_TRUE;
		for (i=0; i<tdm_tbl_len; i++) {
			filter_port = tdm_tbl[i];
			if (filter_port!=port){continue;}
			
			/* Check the above/below ovsb slices */		
			idx0 = i-1;
			idx1 = ((i+1)<tdm_tbl_len)? (i+1): (i+1-tdm_tbl_len);
			os_clump_local_above = 0;
			os_clump_local_below = 0;
			if (tdm_tbl[idx0]==ovsb_token){
				os_clump_local_above = tdm_gh2_scan_slice_size_local(idx0,tdm_tbl,tdm_tbl_len, &slice_idx);
			}
			if (tdm_tbl[idx1]==ovsb_token){
				os_clump_local_below = tdm_gh2_scan_slice_size_local(idx1,tdm_tbl,tdm_tbl_len, &slice_idx);
			}
			if ( (shift_dir==DN && (os_clump_local_above>os_clump_local_below || os_clump_local_above==os_clump_max_last)) || 
			     (shift_dir==UP && (os_clump_local_above<os_clump_local_below || os_clump_local_below==os_clump_max_last)) ){
				shift_cond_pass = BOOL_FALSE;
				break;
			}
			
			/* Check with max linerate size */
			lr_clump_local_last = tdm_gh2_scan_mix_slice_size_local(i,tdm_tbl,tdm_tbl_len, &slice_idx);
			lr_clump_local_curr = 1;
			if (dir==DN){
				idx0 = ((i+2)<tdm_tbl_len)? (i+2): (i+2-tdm_tbl_len);
				if (tdm_tbl[idx0]!=ovsb_token){
					for (j=0; j<(tdm_tbl_len-2); j++){
						idx1 = ((idx0+j)<tdm_tbl_len)? (idx0+j): (idx0+j-tdm_tbl_len);
						if (tdm_tbl[idx1]==ovsb_token) {
							lr_clump_local_curr = 1 + tdm_gh2_scan_mix_slice_size_local(idx0,tdm_tbl,tdm_tbl_len, &slice_idx);
							break;
						}
						else if (tdm_tbl[idx1]==filter_port) {
							lr_clump_local_curr = tdm_gh2_scan_mix_slice_size_local(idx0,tdm_tbl,tdm_tbl_len, &slice_idx);
							break;
						}
					}
				}
			}
			else {
				idx0 = ((i-2)>=0)? (i-2): (i-2+tdm_tbl_len);
				if (tdm_tbl[idx0]!=ovsb_token){
					for (j=0; j<(tdm_tbl_len-2); j++){
						idx1 = ((idx0-j)>=0)? (idx0-j): (idx0-j+tdm_tbl_len);
						if (tdm_tbl[idx1]==ovsb_token) {
							lr_clump_local_curr = 1 + tdm_gh2_scan_mix_slice_size_local(idx0,tdm_tbl,tdm_tbl_len, &slice_idx);
							break;
						}
						else if (tdm_tbl[idx1]==filter_port) {
							lr_clump_local_curr = tdm_gh2_scan_mix_slice_size_local(idx0,tdm_tbl,tdm_tbl_len, &slice_idx);
							break;
						}
					}
				}
			}
			if (lr_clump_local_curr>=lr_clump_max_last){
				shift_cond_pass = BOOL_FALSE;
				break;
			}
			else if (lr_clump_local_curr>lr_clump_local_last ){
				shift_cond_pass = BOOL_FALSE;
				break;
			}
		}
	}
	
	result = (shift_cond_pass==BOOL_TRUE)? (PASS): (FAIL);
	return result;
}


/**
@name: tdm_gh2_scan_which_tsc
@param:

Upward abstraction layer between TDM.4 and TDM.5 API
Only returns enough of TDM.5 style struct to drive scan functions, do not use as class
 */
int
tdm_gh2_scan_which_tsc( int port, tdm_mod_t *_tdm )
{
	int port_tmp, result;
	
	port_tmp = _tdm->_core_data.vars_pkg.port;
	_tdm->_core_data.vars_pkg.port = port;
	result = tdm_gh2_which_tsc(_tdm);
	_tdm->_core_data.vars_pkg.port = port_tmp;
	
	return result;
	
}


/**
@name: tdm_gh2_check_pipe_ethernet
@param:

Returns BOOL_TRUE or BOOL_FALSE depending on if pipe of the given port has traffic entirely Ethernet
 */
int
tdm_gh2_check_pipe_ethernet( int port, tdm_mod_t *_tdm )
{
	int port_tmp, result=BOOL_TRUE;
	
	port_tmp = _tdm->_core_data.vars_pkg.port;
	_tdm->_core_data.vars_pkg.port = port;
	result = tdm_gh2_check_ethernet(_tdm);
	_tdm->_core_data.vars_pkg.port = port_tmp;
	
	return result;
}
#endif /* CDK_CONFIG_INCLUDE_BCM53570_B0 */

