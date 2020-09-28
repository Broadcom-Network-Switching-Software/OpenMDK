/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 * $All Rights Reserved.$
 *
 * TDM chip main functions
 */
#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56670_A0 == 1

#include "bcm56670_a0_tdm_core_top.h"

extern void tdm_mn_tdm_print(int *tdm_tbl, int length);

int mn_is_clport(int pnum) {
  if(pnum > 64) return 0;
  if( (pnum >=1 && pnum <=20) || (pnum >=45 && pnum <=64)) {
    return 1;
  } else {
    return 0;
  }
}
/**
@name: tdm_ap_corereq
@param:

Allocate memory for core data execute request to core executive
 */
int
tdm_mn_corereq( tdm_mod_t *_tdm )
{
	int idx1;
	
	/* X-Pipe: */
	if (_tdm->_core_data.vars_pkg.pipe==MN_PIPE_X_ID) {
		_tdm->_core_data.vmap=(unsigned short **) TDM_MN_ALLOC((_tdm->_core_data.vmap_max_wid)*sizeof(unsigned short *), "vector_map_l1");
		for (idx1=0; idx1<(_tdm->_core_data.vmap_max_wid); idx1++) {
			_tdm->_core_data.vmap[idx1]=(unsigned short *) TDM_MN_ALLOC((_tdm->_core_data.vmap_max_len)*sizeof(unsigned short), "vector_map_l2");
		}
	}
	/* unrecognised Pipe number */
	else {
		TDM_ERROR1("Unrecgonized PIPE ID %d \n", _tdm->_core_data.vars_pkg.pipe);
		return (TDM_EXEC_CORE_SIZE+1);
	}

	return ( _tdm->_core_exec[TDM_CORE_EXEC__INIT]( _tdm ) );
}


/**
@name: tdm_mn_vbs_wrapper
@param:

Code wrapper for egress TDM scheduling
 */
int
tdm_mn_vbs_wrapper( tdm_mod_t *_tdm )
{
	return ( _tdm->_chip_exec[TDM_CHIP_EXEC__COREREQ]( _tdm ) );
}

/**
@name: tdm_mn_lls_wrapper
@param:

Code wrapper for ingress TDM scheduling
 */
int
tdm_mn_lls_wrapper( tdm_mod_t *_tdm )
{
  int idx=0;
  int *iarb_tdm_wrap_ptr_lr_x, *iarb_tdm_tbl_lr_x;
  int *ovs_tdm_tbl = NULL, *ovs_spacing = NULL, *is_falcon_32_40 = NULL; /* is_falcon_32_40 - 1 means falcon-32 and 0 means falcon-40*/
  tdm_mn_chip_legacy_t *_mn_chip;

  _mn_chip = TDM_MN_ALLOC(sizeof(tdm_mn_chip_legacy_t), "TDM chip legacy");
  is_falcon_32_40 = TDM_MN_ALLOC(sizeof(int), "is_falcon_32_40");
  if (!_mn_chip) {
      TDM_FREE(is_falcon_32_40);     
      return FAIL;
  }
  tdm_chip_mn_shim__ing_wrap(_tdm, _mn_chip);


  iarb_tdm_wrap_ptr_lr_x = TDM_MN_ALLOC(sizeof(int), "IARB TDM PTR");	
  iarb_tdm_tbl_lr_x = TDM_MN_ALLOC((sizeof(int)*512), "IARB TDM TABLE");	
  
  for(idx=0; idx<512; idx++){
    iarb_tdm_tbl_lr_x[idx] = MN_NUM_EXT_PORTS;
  }
 
 switch(_tdm->_chip_data.soc_pkg.clk_freq) {
   case 815:
     tdm_mn_init_iarb_tdm_table (815, iarb_tdm_wrap_ptr_lr_x, iarb_tdm_tbl_lr_x); /*Customer 10G*/
   break;
   case 816:
     tdm_mn_init_iarb_tdm_table (816, iarb_tdm_wrap_ptr_lr_x, iarb_tdm_tbl_lr_x); /*Customer 25G*/
   break;
   case 817:
     tdm_mn_init_iarb_tdm_table (817, iarb_tdm_wrap_ptr_lr_x, iarb_tdm_tbl_lr_x); 
   break;
   case 818:
     tdm_mn_init_iarb_tdm_table (818, iarb_tdm_wrap_ptr_lr_x, iarb_tdm_tbl_lr_x); /*CPRI 25G*/
   break;
   case 819:
     tdm_mn_init_iarb_tdm_table (819, iarb_tdm_wrap_ptr_lr_x, iarb_tdm_tbl_lr_x); /*CPRI10G-A*/
   break;
   case 820:
     tdm_mn_init_iarb_tdm_table (820, iarb_tdm_wrap_ptr_lr_x, iarb_tdm_tbl_lr_x); /*CPRI10G-B*/
   break;
   case 861: case 862: /* Both Falcon32/40 has same IARB TDM */
     tdm_mn_init_iarb_tdm_table (861, iarb_tdm_wrap_ptr_lr_x, iarb_tdm_tbl_lr_x); /*FALCON32/40*/
   break;
   default: break;
 }
  if(_tdm->_chip_data.soc_pkg.clk_freq == 861 || _tdm->_chip_data.soc_pkg.clk_freq == 862){
    if(_tdm->_chip_data.soc_pkg.clk_freq == 861){
      *is_falcon_32_40 = 1;
    } else if(_tdm->_chip_data.soc_pkg.clk_freq == 862) {
      *is_falcon_32_40 = 0;
    }
    for(idx=0; idx<MN_NUM_QUAD; idx++){
      switch(idx){
        case 0:
          ovs_spacing = _mn_chip->tdm_pgw.ovs_spacing_x0;
          ovs_tdm_tbl = _tdm->_chip_data.cal_0.cal_grp[0];
          break;
        case 1:
          ovs_spacing = _mn_chip->tdm_pgw.ovs_spacing_x1;
          ovs_tdm_tbl = _tdm->_chip_data.cal_1.cal_grp[0];
          break;
      }
     tdm_mn_falcon_32_40__ovs_gen(_mn_chip, ovs_tdm_tbl, ovs_spacing, idx, is_falcon_32_40);
    }
  }
/* tdm_mn_zte(_tdm); */
 /* Realign port state array to old specification */	
 for (idx=0; idx<((_tdm->_chip_data.soc_pkg.num_ext_ports) - MN_EXT_PLUS_CPU_PORT); idx++) {
 	_tdm->_chip_data.soc_pkg.state[idx] = _tdm->_chip_data.soc_pkg.state[idx+1];
 }
 _tdm->_core_data.vars_pkg.pipe       = 0;
  TDM_FREE(is_falcon_32_40);     
  TDM_FREE(iarb_tdm_wrap_ptr_lr_x);     
  TDM_FREE(iarb_tdm_tbl_lr_x);     
  TDM_FREE(_mn_chip);     

 return ( _tdm->_chip_exec[TDM_CHIP_EXEC__EGRESS_WRAP]( _tdm ) );
}

/**
 * @name: tdm_mn_zte
 */
/*
void tdm_mn_zte(tdm_mod_t *_tdm)
{
  int i=0, j=0;
    tdm_mn_zte_vars_backup(_tdm);
    tdm_mn_zte_change_vars(_tdm);
  }
}
void tdm_mn_zte_change_vars(tdm_mod_t *_tdm)
{
  int i=0, j=0;
  for(i=1; i<(MN_NUM_PHY_PORTS);i=i+MN_NUM_PM_LNS){
    if(_tdm->_chip_data.soc_pkg.speed[i] == SPEED_12p5G){
      for(j=0; j<MN_NUM_PM_LNS; j++){
        switch(j){
          case 0:
            _tdm->_chip_data.soc_pkg.state[i+j] = PORT_STATE__LINERATE;
            break;
          case 1: case 2: case 3:
            _tdm->_chip_data.soc_pkg.state[i+j] = PORT_STATE__DISABLED;
            break;
        }
      }
    }
  }
  for(i=1; i<=(MN_NUM_PHY_PORTS);i=i+MN_NUM_PM_LNS){
    if(_tdm->_chip_data.soc_pkg.speed[i] == SPEED_12p5G){
      for(j=0; j<MN_NUM_PM_LNS; j++){
        switch(j){
          case 0: case 1: 
            _tdm->_chip_data.soc_pkg.pmap[i/MN_NUM_PM_LNS][j] = i;
            break;
          case 2: case 3:
            _tdm->_chip_data.soc_pkg.pmap[i/MN_NUM_PM_LNS][j] = MN_NUM_EXT_PORTS;
            break;
        }
      }
    }
  }
  for(i=1; i<(MN_NUM_PHY_PORTS);i=i+MN_NUM_PM_LNS){
    if(_tdm->_chip_data.soc_pkg.speed[i] == SPEED_12p5G){
      for(j=0; j<MN_NUM_PM_LNS; j++){
        switch(j){
          case 0:
            _tdm->_chip_data.soc_pkg.speed[i+j] = SPEED_50G;
            break;
          case 1: case 2: case 3:
            _tdm->_chip_data.soc_pkg.speed[i+j] = SPEED_0;
          break;
        }
      }
    }
  }
}
void tdm_mn_zte_vars_backup(tdm_mod_t *_tdm)
{
  int i=0, j=0;
	_tdm->_chip_data.soc_pkg.soc_vars.mn.pmap=(int **) TDM_MN_ALLOC((_tdm->_chip_data.soc_pkg.pmap_num_modules)*sizeof(int *), "portmod_map_l1_backup");
	for (i=0; i<(_tdm->_chip_data.soc_pkg.pmap_num_modules); i++) {
		_tdm->_chip_data.soc_pkg.soc_vars.mn.pmap[i]=(int *) TDM_MN_ALLOC((_tdm->_chip_data.soc_pkg.pmap_num_lanes)*sizeof(int), "portmod_map_l2_backup");
		TDM_MSET(_tdm->_chip_data.soc_pkg.soc_vars.mn.pmap[i],(_tdm->_chip_data.soc_pkg.num_ext_ports),_tdm->_chip_data.soc_pkg.pmap_num_lanes);
	}
  for(i=0; i<MN_NUM_PHY_PM;i++){
    for(j=0; j< MN_NUM_PM_LNS; j++){
      _tdm->_chip_data.soc_pkg.soc_vars.mn.pmap[i][j] = _tdm->_chip_data.soc_pkg.pmap[i][j];
    }
  }
	_tdm->_chip_data.soc_pkg.soc_vars.mn.state = TDM_MN_ALLOC((MN_NUM_EXT_PORTS)*sizeof(enum port_speed_e *), "port state list backup");
	_tdm->_chip_data.soc_pkg.soc_vars.mn.speed = TDM_MN_ALLOC((MN_NUM_EXT_PORTS)*sizeof(enum port_state_e *), "port speed list backup");

  _tdm->_chip_data.soc_pkg.soc_vars.mn.state[0] = PORT_STATE__LINERATE; 
	for (i=1; i<(_tdm->_chip_data.soc_pkg.num_ext_ports-MN_EXT_PLUS_CPU_PORT); i++) {
		_tdm->_chip_data.soc_pkg.soc_vars.mn.state[i] = _tdm->_chip_data.soc_pkg.state[i];
  }
  _tdm->_chip_data.soc_pkg.soc_vars.mn.state[_tdm->_chip_data.soc_pkg.num_ext_ports-MN_EXT_PLUS_CPU_PORT] = PORT_STATE__LINERATE; 
 
	for (i=0; i<(_tdm->_chip_data.soc_pkg.num_ext_ports-MN_NUM_PHY_PORTS); i++) {
		_tdm->_chip_data.soc_pkg.soc_vars.mn.speed[i] = _tdm->_chip_data.soc_pkg.speed[i];
	}

   _tdm->_chip_data.soc_pkg.soc_vars.mn.speed[MN_SBUS_TOKEN] = SPEED_5G;
}
*/
/**
@name: tdm_mn_falcon_32_40__ovs_gen
@param:
Api to replace tdm_mn_ovs_spacer.
this api is helpful in generating tdm table based on the template provided by Brendan 
 */
void
tdm_mn_falcon_32_40__ovs_gen(tdm_mn_chip_legacy_t *_mn_chip,int *ovs_tdm_tbl, int *ovs_spacing, int pgw_num, int *is_falcon_32_40)
{
  mn_pgw_os_tdm_sel_e mn_pgw_os_tdm_sel;
int falcon_pgw0_ovs_tbl_template[2][48] = {
                                                  {1,5,9,13,1,5,9,13,1,5,9,13,1,5,9,13,1,5,9,13,1,5,9,13,1,5,9,13,1,5,9,13,1,5,9,13,1,5,9,13,1,5,9,13,1,5,9,13},             /* FALCON32 - 800G */
					          {1,5,9,13,17,1,5,9,13,17,1,5,9,13,17,1,5,9,13,17,1,5,9,13,17,1,5,9,13,17,1,5,9,13,17,1,5,9,13,17,MN_OVSB_TOKEN,MN_OVSB_TOKEN,MN_OVSB_TOKEN,MN_OVSB_TOKEN,MN_OVSB_TOKEN,MN_OVSB_TOKEN,MN_OVSB_TOKEN,MN_OVSB_TOKEN}, /* FALCON40 - 1000G*/
                                                  };
  int falcon_pgw1_ovs_tbl_template[2][48] = {
                                                  {49,53,57,61,49,53,57,61,49,53,57,61,49,53,57,61,49,53,57,61,49,53,57,61,49,53,57,61,49,53,57,61,49,53,57,61,49,53,57,61,49,53,57,61,49,53,57,61}, /* FALCON32 - 800G */
                                                  {45,49,53,57,61,45,49,53,57,61,45,49,53,57,61,45,49,53,57,61,45,49,53,57,61,45,49,53,57,61,45,49,53,57,61,45,49,53,57,61,MN_OVSB_TOKEN,MN_OVSB_TOKEN,MN_OVSB_TOKEN,MN_OVSB_TOKEN,MN_OVSB_TOKEN,MN_OVSB_TOKEN,MN_OVSB_TOKEN,MN_OVSB_TOKEN}, /* FALCON40 - 1000G */
                                                 };

  tdm_mn_check_mv_fb_config(&mn_pgw_os_tdm_sel, _mn_chip);
  switch(pgw_num){
    case 0:
      tdm_mn_falcon_32_40_ovs_tbl(_mn_chip, ovs_tdm_tbl, (unsigned int *) falcon_pgw0_ovs_tbl_template[mn_pgw_os_tdm_sel], &mn_pgw_os_tdm_sel, pgw_num, is_falcon_32_40);
      tdm_mn_falcon_32_40_ovs_tbl_spacing(ovs_tdm_tbl, ovs_spacing);
      tdm_mn_tdm_print((int*) falcon_pgw0_ovs_tbl_template[mn_pgw_os_tdm_sel], 48);
      tdm_mn_tdm_print(ovs_tdm_tbl, 48);
      tdm_mn_tdm_print(ovs_spacing, 48);
      break;
    case 1:
      tdm_mn_falcon_32_40_ovs_tbl(_mn_chip, ovs_tdm_tbl, (unsigned int*) falcon_pgw1_ovs_tbl_template[mn_pgw_os_tdm_sel], &mn_pgw_os_tdm_sel, pgw_num, is_falcon_32_40);
      tdm_mn_falcon_32_40_ovs_tbl_spacing(ovs_tdm_tbl, ovs_spacing);
      tdm_mn_tdm_print(falcon_pgw1_ovs_tbl_template[mn_pgw_os_tdm_sel], 48);
      tdm_mn_tdm_print(ovs_tdm_tbl, 48);
      tdm_mn_tdm_print(ovs_spacing, 48);
      break;
  }
}

void tdm_mn_falcon_32_40_ovs_tbl(tdm_mn_chip_legacy_t *mn_chip,int *ovs_tdm_tbl, unsigned int *pgw0_ovs_tbl_template, mn_pgw_os_tdm_sel_e *mn_pgw_os_tdm_sel, int pgw_num, int * is_falcon_32_40)
{
  int i, j, token;
  for(i=0; i<48; i++) {
    ovs_tdm_tbl[i] = pgw0_ovs_tbl_template[i];
  }
  for(i=1; i<= MN_NUM_PHY_PORTS; i=i+4) {
    switch((*mn_chip).tdm_globals.speed[i]){
      case SPEED_100G: /*As template is having all Single mode port entries and 100/40G ports in single modes so skip processing  */
        for (j=0, token=0; j<48;j++) {
	       if (ovs_tdm_tbl[j] == i) { 
	          token++;
            if(*is_falcon_32_40 == 1){
	            if (token>10) ovs_tdm_tbl[j]= MN_OVSB_TOKEN; /* Inline-edit template has 12 slots allocated for 100G ports in template. This would eatup bandwidth of other low-speed ports. To avoid the BW loss on low speed ports, 100G port should get only 10 slots.  */
            } else if(*is_falcon_32_40 == 0){
	            if (token>8) ovs_tdm_tbl[j]= MN_OVSB_TOKEN; /* Falcon-40 has per slot BW of 12.5G so max slots for 100G or 4x25 port is 8 slots*/
            }
	       }
        }
	break;
      case SPEED_40G: 
	      if (mn_is_clport( i )) { /* Only for Falcon ports enter this processing. Reduce the tokens from 8 to 4 */
          for (j=0, token=0; j<48;j++) {
	          if (ovs_tdm_tbl[j] == i) { 
	            token++;
	            if (token>4) ovs_tdm_tbl[j]= MN_OVSB_TOKEN; /* To handle FB5 config: Falcon ports can be configured in 40G mode*/
	            else if (token%2) ovs_tdm_tbl[j]= ((*mn_chip).tdm_globals.speed[i+2] == SPEED_40G) ? i+2 : i;
	          }
          }
	      }
	break;
      case SPEED_50G:
        for (j=0, token=0; j<48;j++) {
	        if (ovs_tdm_tbl[j] == i) { 
	          token++;
            if(*is_falcon_32_40 == 1) {
              if (token>10) ovs_tdm_tbl[j]= MN_OVSB_TOKEN; 
	            else if (token%2) ovs_tdm_tbl[j]=i;
	            else ovs_tdm_tbl[j]=((*mn_chip).tdm_globals.speed[i+2] == SPEED_50G) ? i+2: 
		          	((*mn_chip).tdm_globals.speed[i+1] == SPEED_25G) ? i+1 :
		          	((*mn_chip).tdm_globals.speed[i+3] == SPEED_25G) ? i+3 : MN_OVSB_TOKEN /*IDLE Port*/; 
            } else if(*is_falcon_32_40 == 0){
              if (token>8) ovs_tdm_tbl[j]= MN_OVSB_TOKEN; 
	            switch (token) {
		            case 1:
		            case 3: 
			            ovs_tdm_tbl[j] = i;
			            break;
		            case 2:
		            case 4: 
			            ovs_tdm_tbl[j] = ((*mn_chip).tdm_globals.speed[i+2] == SPEED_50G) ? i+2 : 
                                   ((*mn_chip).tdm_globals.speed[i+2] == SPEED_25G && token == 2) ? i+2:
                                   ((*mn_chip).tdm_globals.speed[i+3] == SPEED_25G && token == 4) ? i+3:
                                   ((*mn_chip).tdm_globals.speed[i+2] <= SPEED_10G && token == 4) ? i+2: /* 1x50+2x10/5/2.5/1G */
                                    MN_OVSB_TOKEN;
			          break;
		            case 5:
		            case 7: 
			            ovs_tdm_tbl[j] = i;
		              break;
		            case 6: 
		            case 8: 
			            ovs_tdm_tbl[j] = ((*mn_chip).tdm_globals.speed[i+2] == SPEED_50G) ? i+2 : 
                                   ((*mn_chip).tdm_globals.speed[i+2] == SPEED_25G && token == 6) ? i+2:
                                   ((*mn_chip).tdm_globals.speed[i+3] == SPEED_25G && token == 8) ? i+3:
                                   ((*mn_chip).tdm_globals.speed[i+3] <= SPEED_10G && token == 8) ? i+3: /* 1x50+2x10/5/2.5/1G */
                                    MN_OVSB_TOKEN;
			          break;
		          }
            }
	        }
        }
      break;
      case SPEED_25G:
	if (mn_is_clport( i )) { /* Only for Falcon ports enter this processing */
    for (j=0, token=0; j<48;j++) {
	    if (ovs_tdm_tbl[j] == i) {
	      token++;
        if(*is_falcon_32_40 == 1) {
	        switch (token) {
		        case 1:
		        case 5: 
		        case 9: /* 25 G*/
			        ovs_tdm_tbl[j] = i;
			        break;
		        case 2:
		        case 6: 
		        case 10: /* Check for 50G first, then check for 25G */
			        ovs_tdm_tbl[j] =  ((*mn_chip).tdm_globals.speed[i+2] == SPEED_50G) ?  i+2 : 
			    		 ((*mn_chip).tdm_globals.speed[i+2] == SPEED_25G) ? i+2 : 
               ((*mn_chip).tdm_globals.speed[i+2] <= SPEED_10G && token ==2) ? i+2: /* 2x25+2x10 (P0 P2 P1 P3 -- P0,P1 25G and P2 P3 10G */
               ((*mn_chip).tdm_globals.speed[i+3] <= SPEED_10G && token ==6) ? i+3: /* 2x25+2x10 (P0 P2 P1 P3 -- P0,P1 25G and P2 P3 10G */
               MN_OVSB_TOKEN /*IDLE port*/;
			         break;
		        case 3:
		        case 7: 
		        case 11: /* Say PM port 1, 2, 3, 4 are in trimode. then the mapping should be 1,3,2,3 considering port 3 as 50G port */
		          ovs_tdm_tbl[j] =  ((*mn_chip).tdm_globals.speed[i+1] == SPEED_25G) ?  i+1 : MN_OVSB_TOKEN;  
		        break;
		        case 4: 
		        case 8: 
		        case 12: /* Check for 50G first, then check for 25G */
		          ovs_tdm_tbl[j] =  ((*mn_chip).tdm_globals.speed[i+2] == SPEED_50G) ?  i+2 : 
		      		                	 ((*mn_chip).tdm_globals.speed[i+3] == SPEED_25G) ? i+3 : MN_OVSB_TOKEN;
		         	break;
		      }
		    } else if(*is_falcon_32_40 == 0){ /* only 8 token in falcon40  */
	        switch (token) {
		        case 1:
		        case 5: 
			        ovs_tdm_tbl[j] = i;
			        break;
		        case 2:
		        case 6: 
			        ovs_tdm_tbl[j] =  ((*mn_chip).tdm_globals.speed[i+2] == SPEED_50G) ?  i+2 : 
			    		 ((*mn_chip).tdm_globals.speed[i+2] == SPEED_25G) ? i+2 : 
               ((*mn_chip).tdm_globals.speed[i+2] <= SPEED_10G && token ==2) ? i+2: /* 2x25+2x10 (P0 P2 P1 P3 -- P0,P1 25G and P2 P3 10G */
               MN_OVSB_TOKEN /*IDLE port*/;
			         break;
		        case 3:
		        case 7: 
		          ovs_tdm_tbl[j] =  ((*mn_chip).tdm_globals.speed[i+1] == SPEED_25G) ?  i+1 : MN_OVSB_TOKEN;  
		        break;
		        case 4: 
		        case 8: 
		          ovs_tdm_tbl[j] =  ((*mn_chip).tdm_globals.speed[i+2] == SPEED_50G) ?  i+2 : 
		      		                  ((*mn_chip).tdm_globals.speed[i+3] == SPEED_25G) ? i+3 : 
		      		                  ((*mn_chip).tdm_globals.speed[i+3] <= SPEED_10G && token ==4) ? i+3 : 
                                MN_OVSB_TOKEN;
		         	break;
		      }
        }
	    }
    }
        } else {
	/* For Eagle - insert one IDLE slot out of 4 valid slots*/
	/* No spacing rules applicable in OS calendar.*/
          for (j=0, token=0; j<48;j++) {
	     if (ovs_tdm_tbl[j] == i) { 
               ovs_tdm_tbl[j] = MN_OVSB_TOKEN;
               break;
	     }
          }
	}
      break;
      case SPEED_20G:
        for (j=0, token=0; j<48;j++) {
	   if (ovs_tdm_tbl[j] == i) { 
	      token++;
	      if ((mn_is_clport( i )) && token>4) ovs_tdm_tbl[j] = MN_OVSB_TOKEN; /* Falcon ports when configured as 20G dont require more than 4 tokens (2x20G)*/
	      else if (token%2) ovs_tdm_tbl[j]=i;
	      else ovs_tdm_tbl[j]=((*mn_chip).tdm_globals.speed[i+2] == SPEED_20G) ? i+2: 
					((*mn_chip).tdm_globals.speed[i+1] == SPEED_10G) ? i+1 :
					((*mn_chip).tdm_globals.speed[i+3] == SPEED_10G) ? i+3 : MN_OVSB_TOKEN /*IDLE Port*/; 
	   }
        }
      break;
      case SPEED_10G:
      case SPEED_1G:
      case SPEED_2p5G:
      case SPEED_5G:
        for (j=0, token=0; j<48;j++) {
	        if (ovs_tdm_tbl[j] == i) { 
            if((*mn_chip).tdm_globals.port_rates_array[i] == PORT_STATE__LINERATE){
              ovs_tdm_tbl[j] = MN_OVSB_TOKEN;
              continue;
            }
	          token++;
          if(*is_falcon_32_40 == 1) {
	          switch (token) {
		          case 1:
		          case 5: 
		          case 9: /* 10G*/
		            ovs_tdm_tbl[j] = ((*mn_chip).tdm_globals.speed[i] <= SPEED_10G) ? i : MN_OVSB_TOKEN;
		            break;
		          case 2:
		          case 6: 
		          case 10: /* Check for 50G first, then check for 25G */
		            ovs_tdm_tbl[j] =  ((*mn_chip).tdm_globals.speed[i+2] == SPEED_50G) ?  i+2 : 
		        		                  ((*mn_chip).tdm_globals.speed[i+2] == SPEED_25G) ? i+2 : 
		        		                  ((*mn_chip).tdm_globals.speed[i+2] <= SPEED_10G) ? i+2 : 
                                  MN_OVSB_TOKEN;
		          break;
		          case 3:
		          case 7: 
		          case 11:
		            ovs_tdm_tbl[j] = ((*mn_chip).tdm_globals.speed[i+3] <= SPEED_10G) ? i+3 : MN_OVSB_TOKEN;
	              break;
	            case 4: 
	            case 8: 
	            case 12: /* Check for 50G first, then check for 25G */
	              ovs_tdm_tbl[j] =  
                                 ((*mn_chip).tdm_globals.speed[i+3] == SPEED_50G && token != 12) ?  i+3 : 
	              			           ((*mn_chip).tdm_globals.speed[i+3] == SPEED_25G) ? i+3 :
		        		                 ((*mn_chip).tdm_globals.speed[i+3] <= SPEED_10G) ? i+3 : 
                                 MN_OVSB_TOKEN;
	              break;
	          }
	        } else if(*is_falcon_32_40 == 0){
	            switch (token) {
		            case 1:
		            case 5: 
		              ovs_tdm_tbl[j] = ((*mn_chip).tdm_globals.speed[i] <= SPEED_10G && token == 1) ? i : MN_OVSB_TOKEN;
		              break;
		            case 2:
		            case 6: 
		              ovs_tdm_tbl[j] =  ((*mn_chip).tdm_globals.speed[i+2] == SPEED_50G) ?  i+2 : 
		          		                  ((*mn_chip).tdm_globals.speed[i+2] == SPEED_25G) ? i+2 : 
		          		                  ((*mn_chip).tdm_globals.speed[i+2] <= SPEED_10G) ? i+2 : 
                                    MN_OVSB_TOKEN;
		            break;
		            case 3:
		            case 7: 
		              ovs_tdm_tbl[j] = ((*mn_chip).tdm_globals.speed[i+1] <= SPEED_10G && token == 3) ? i+1 : MN_OVSB_TOKEN;
	                break;
	              case 4: 
	              case 8: 
	                ovs_tdm_tbl[j] =  
                                   ((*mn_chip).tdm_globals.speed[i+2] == SPEED_50G) ?  i+2 : 
	                			           ((*mn_chip).tdm_globals.speed[i+3] == SPEED_25G) ? i+3 :
		          		                 ((*mn_chip).tdm_globals.speed[i+3] <= SPEED_10G) ? i+3 : 
                                   MN_OVSB_TOKEN;
	                break;
	            }
          }
	    }
    }
      break;
      case SPEED_0:
        for (j=0; j<48;j++) {
	        if (ovs_tdm_tbl[j] == i) { 
            ovs_tdm_tbl[j] = MN_OVSB_TOKEN;
	        }
        }
      break;
      default:
        break;
    }

  }
}
/* Process below OVS Spacing Table   */

 void tdm_mn_falcon_32_40_ovs_tbl_spacing(int *ovs_tdm_tbl, int *ovs_spacing) {
  int i, j, wrap_arnd; 
  for (i=0; i<48; i++ ) {
    ovs_spacing[i] =0;
    for (j=i+1, wrap_arnd=0; j<=48 && wrap_arnd<2; j++ ) { /* j goes till 48 - to account for wrap-around condition. this is to take care of port slots allocated at the end of calendar*/
      if (j == 48) {
        j = -1;
        wrap_arnd++;
        continue;
      } 
      ovs_spacing[i]++;
      if (ovs_tdm_tbl[i] == ovs_tdm_tbl[j]) break;
     }
   }
   
}
void
tdm_mn_check_mv_fb_config(mn_pgw_os_tdm_sel_e *mn_pgw_os_tdm_sel, tdm_mn_chip_legacy_t *mn_chip)
{
  int port_num;
  unsigned int num_lr_ports = 0, num_os_ports = 0;
  for(port_num=0; port_num< MN_NUM_PHY_PORTS; port_num++) {
    if((*mn_chip).tdm_globals.port_rates_array[port_num] == PORT_STATE__OVERSUB   ||
        (*mn_chip).tdm_globals.port_rates_array[port_num] == PORT_STATE__OVERSUB_HG){
      num_os_ports++;
    }else if((*mn_chip).tdm_globals.port_rates_array[port_num] == PORT_STATE__LINERATE   ||
        (*mn_chip).tdm_globals.port_rates_array[port_num] == PORT_STATE__LINERATE_HG){
      num_lr_ports++;
    }
  }
  switch ((*mn_chip).tdm_globals.clk_freq) {
    case 861:/*  with new Inline edit template from Brendan*/
        *mn_pgw_os_tdm_sel = MN_PGW_OS_FALCN_800G;
        break;
    case 862:
        *mn_pgw_os_tdm_sel = MN_PGW_OS_FALCN_1000G;
        break;
    default: break;
  }

}
/**
@name: tdm_mn_pmap_transcription
@param:

For Trident2+ BCM56850
Transcription algorithm for generating port module mapping

	40G 	- xxxx
      	20G 	- xx
	      	    xx
      	10G	- x
		   x
		    x
		     x
     	1X0G 	- xxxx_xxxx_xxxx
 */
int
tdm_mn_pmap_transcription( tdm_mod_t *_tdm )
{
	int i, j, last_port=MN_NUM_EXT_PORTS, tsc_active;
  int phy_lo, phy_hi, pm_lane_num, bcm_config_check=PASS;
	
	/* Regular ports */
	for (i=1; i<=MN_NUM_PHY_PORTS; i+=MN_NUM_PM_LNS) {
		tsc_active=BOOL_FALSE;
		for (j=0; j<MN_NUM_PM_LNS; j++) {
			if ( _tdm->_chip_data.soc_pkg.state[i+j]==PORT_STATE__LINERATE    || _tdm->_chip_data.soc_pkg.state[i+j]==PORT_STATE__OVERSUB    ||
			     _tdm->_chip_data.soc_pkg.state[i+j]==PORT_STATE__LINERATE_HG || _tdm->_chip_data.soc_pkg.state[i+j]==PORT_STATE__OVERSUB_HG ){
				tsc_active=BOOL_TRUE;
				break;
			}
		}
		if(tsc_active==BOOL_TRUE){
			/*else { */
		  if ( _tdm->_chip_data.soc_pkg.speed[i]>SPEED_0 || _tdm->_chip_data.soc_pkg.state[i]==PORT_STATE__DISABLED ) {
		  	for (j=0; j<MN_NUM_PM_LNS; j++) {
		  		switch (_tdm->_chip_data.soc_pkg.state[i+j]) {
		  			case PORT_STATE__LINERATE:
		  			case PORT_STATE__OVERSUB:
		  			case PORT_STATE__LINERATE_HG:
		  			case PORT_STATE__OVERSUB_HG:
		  				_tdm->_chip_data.soc_pkg.pmap[(i-1)/MN_NUM_PM_LNS][j]=(i+j);
		  				last_port=(i+j);
		  				break;
		  			case PORT_STATE__COMBINE:
		  				_tdm->_chip_data.soc_pkg.pmap[(i-1)/MN_NUM_PM_LNS][j]=last_port;
		  				break;
		  			default:
		  				_tdm->_chip_data.soc_pkg.pmap[(i-1)/MN_NUM_PM_LNS][j]=MN_NUM_EXT_PORTS;
		  				break;
		  		}
		  	}
		  	if ( _tdm->_chip_data.soc_pkg.speed[i]>_tdm->_chip_data.soc_pkg.speed[i+2] && _tdm->_chip_data.soc_pkg.speed[i+2]==_tdm->_chip_data.soc_pkg.speed[i+3] && _tdm->_chip_data.soc_pkg.speed[i+2]!=SPEED_0 &&_tdm->_chip_data.soc_pkg.speed[i]>=SPEED_40G ) {
		  	  /*_tdm->_chip_data.soc_pkg.pmap[(i-1)/MN_NUM_PM_LNS][1] = _tdm->_chip_data.soc_pkg.pmap[(i-1)/MN_NUM_PM_LNS][2];
		  	  _tdm->_chip_data.soc_pkg.pmap[(i-1)/MN_NUM_PM_LNS][2] = _tdm->_chip_data.soc_pkg.pmap[(i-1)/MN_NUM_PM_LNS][0];*/
		  	}
		  	else if ( _tdm->_chip_data.soc_pkg.speed[i]==_tdm->_chip_data.soc_pkg.speed[i+1] && _tdm->_chip_data.soc_pkg.speed[i]<_tdm->_chip_data.soc_pkg.speed[i+2] && _tdm->_chip_data.soc_pkg.speed[i]!=SPEED_0 &&_tdm->_chip_data.soc_pkg.speed[i+2]>=SPEED_40G )  {
		  	  /*_tdm->_chip_data.soc_pkg.pmap[(i-1)/MN_NUM_PM_LNS][2] = _tdm->_chip_data.soc_pkg.pmap[(i-1)/MN_NUM_PM_LNS][1];
		  	  _tdm->_chip_data.soc_pkg.pmap[(i-1)/MN_NUM_PM_LNS][1] = _tdm->_chip_data.soc_pkg.pmap[(i-1)/MN_NUM_PM_LNS][3]; */
		  	}
		  /* dual mode x_x_ */
		      else if (_tdm->_chip_data.soc_pkg.speed[i]!=_tdm->_chip_data.soc_pkg.speed[i+1]&&_tdm->_chip_data.soc_pkg.speed[i]==_tdm->_chip_data.soc_pkg.speed[i+2]&&_tdm->_chip_data.soc_pkg.speed[i]>=SPEED_40G) {
		  	_tdm->_chip_data.soc_pkg.pmap[(i-1)/MN_NUM_PM_LNS][1] = _tdm->_chip_data.soc_pkg.pmap[(i-1)/MN_NUM_PM_LNS][3];
		  	_tdm->_chip_data.soc_pkg.pmap[(i-1)/MN_NUM_PM_LNS][2] = _tdm->_chip_data.soc_pkg.pmap[(i-1)/MN_NUM_PM_LNS][0];
		      }
		  }
	  }
  }
	tdm_print_stat( _tdm );

    /* Check if invalid port config exists */
    phy_lo     = _tdm->_chip_data.soc_pkg.soc_vars.fp_port_lo;
    phy_hi     = _tdm->_chip_data.soc_pkg.soc_vars.fp_port_hi;
    pm_lane_num= _tdm->_chip_data.soc_pkg.pmap_num_lanes;
    for (i=phy_lo; i<=(phy_hi-pm_lane_num); i+=pm_lane_num) {
        if (_tdm->_chip_data.soc_pkg.speed[i]==SPEED_0){
            for (j=1; j<pm_lane_num; j++) {
                if (_tdm->_chip_data.soc_pkg.speed[i+j]>SPEED_0){
                    bcm_config_check = FAIL;
                    break;
                }
            }
        }
        if (bcm_config_check==FAIL){
            TDM_ERROR8("Invalid port configuration, port [%3d, %3d, %3d, %3d], speed [%3dG, %3dG, %3dG, %3dG]\n",
                i, i+1, i+2, i+3,
                _tdm->_chip_data.soc_pkg.speed[i]/1000,
                _tdm->_chip_data.soc_pkg.speed[i+1]/1000,
                _tdm->_chip_data.soc_pkg.speed[i+2]/1000,
                _tdm->_chip_data.soc_pkg.speed[i+3]/1000);
            return FAIL;
        }
    }
    for (i=1; i<=MN_NUM_PHY_PORTS; i+=MN_NUM_PM_LNS) {
      if( mn_is_clport( i ) ){
        if ( ( _tdm->_chip_data.soc_pkg.speed[i]>_tdm->_chip_data.soc_pkg.speed[i+2] && _tdm->_chip_data.soc_pkg.speed[i+2]==_tdm->_chip_data.soc_pkg.speed[i+3] && _tdm->_chip_data.soc_pkg.speed[i+2]!=SPEED_0 && _tdm->_chip_data.soc_pkg.speed[i]>=SPEED_40G ) || ( _tdm->_chip_data.soc_pkg.speed[i]==_tdm->_chip_data.soc_pkg.speed[i+1] && _tdm->_chip_data.soc_pkg.speed[i]<_tdm->_chip_data.soc_pkg.speed[i+2] && _tdm->_chip_data.soc_pkg.speed[i]!=SPEED_0 && _tdm->_chip_data.soc_pkg.speed[i+2]>=SPEED_40G )) {
         /*TDM_ERROR8("tri port configuration(25GE+50GE)on Falcon not supported in APACHE, port [%3d, %3d, %3d, %3d], speed [%3dG, %3dG, %3dG, %3dG]\n", i, i+1, i+2, i+3, _tdm->_chip_data.soc_pkg.speed[i]/1000, _tdm->_chip_data.soc_pkg.speed[i+1]/1000, _tdm->_chip_data.soc_pkg.speed[i+2]/1000, _tdm->_chip_data.soc_pkg.speed[i+3]/1000);*/
         /*return FAIL; */
				}
      }
    }

	return ( _tdm->_chip_exec[TDM_CHIP_EXEC__INGRESS_WRAP]( _tdm ) );
}

/**
@name: tdm_chip_mn_init
@param:
 */
int tdm_mn_init( tdm_mod_t *_tdm )
{
	int idx;

	_tdm->_chip_data.soc_pkg.pmap_num_modules = MN_NUM_PM_MOD;
	_tdm->_chip_data.soc_pkg.pmap_num_lanes = MN_NUM_PM_LNS;
	_tdm->_chip_data.soc_pkg.pm_num_phy_modules = MN_NUM_PHY_PM;
	
	_tdm->_chip_data.soc_pkg.soc_vars.ovsb_token = MN_OVSB_TOKEN;
	_tdm->_chip_data.soc_pkg.soc_vars.idl1_token = MN_IDL1_TOKEN;
	_tdm->_chip_data.soc_pkg.soc_vars.idl2_token = MN_IDL2_TOKEN;
	_tdm->_chip_data.soc_pkg.soc_vars.ancl_token = MN_ANCL_TOKEN;
	_tdm->_chip_data.soc_pkg.soc_vars.fp_port_lo = 1;
	_tdm->_chip_data.soc_pkg.soc_vars.fp_port_hi = MN_NUM_PHY_PORTS;
	
	_tdm->_chip_data.cal_0.cal_len = MN_LR_LLS_LEN;
	_tdm->_chip_data.cal_0.grp_num = MN_OS_LLS_GRP_NUM;
	_tdm->_chip_data.cal_0.grp_len = MN_OS_LLS_GRP_LEN;
	_tdm->_chip_data.cal_1.cal_len = MN_LR_LLS_LEN;
	_tdm->_chip_data.cal_1.grp_num = MN_OS_LLS_GRP_NUM;
	_tdm->_chip_data.cal_1.grp_len = MN_OS_LLS_GRP_LEN;
	_tdm->_chip_data.cal_2.cal_len = MN_LR_VBS_LEN;
	_tdm->_chip_data.cal_2.grp_num = MN_OS_VBS_GRP_NUM;
	_tdm->_chip_data.cal_2.grp_len = MN_OS_VBS_GRP_LEN;
	_tdm->_chip_data.cal_3.cal_len = MN_LR_IARB_STATIC_LEN;
	_tdm->_chip_data.cal_3.grp_num = 0;
	_tdm->_chip_data.cal_3.grp_len = 0;

	
    _tdm->_core_data.vars_pkg.pipe       = 0;
    _tdm->_core_data.rule__same_port_min = MN_MIN_SPACING_SAME_PORT_HSP;
    _tdm->_core_data.rule__prox_port_min = MN_MIN_SPACING_SISTER_PORT;
    _tdm->_core_data.vmap_max_wid        = MN_VMAP_MAX_WID;
    _tdm->_core_data.vmap_max_len        = MN_VMAP_MAX_LEN;

	/* management port state */
	/*if (_tdm->_chip_data.soc_pkg.speed[MN_MGMT_PORT_0] > SPEED_0   && 
		_tdm->_chip_data.soc_pkg.speed[MN_MGMT_PORT_0]<= SPEED_10G &&
		_tdm->_chip_data.soc_pkg.speed[MN_MGMT_PORT_1]==_tdm->_chip_data.soc_pkg.speed[MN_MGMT_PORT_2] && 
		_tdm->_chip_data.soc_pkg.speed[MN_MGMT_PORT_2]==_tdm->_chip_data.soc_pkg.speed[MN_MGMT_PORT_3] ){
		if (_tdm->_chip_data.soc_pkg.speed[MN_MGMT_PORT_0]==SPEED_10G && _tdm->_chip_data.soc_pkg.speed[MN_MGMT_PORT_1]==SPEED_0){
			_tdm->_chip_data.soc_pkg.state[MN_MGMT_PORT_0] = PORT_STATE__MANAGEMENT;
			_tdm->_chip_data.soc_pkg.state[MN_MGMT_PORT_1] = PORT_STATE__DISABLED;
			_tdm->_chip_data.soc_pkg.state[MN_MGMT_PORT_2] = PORT_STATE__DISABLED;
			_tdm->_chip_data.soc_pkg.state[MN_MGMT_PORT_3] = PORT_STATE__DISABLED;
		}
		else if (_tdm->_chip_data.soc_pkg.speed[MN_MGMT_PORT_1]==_tdm->_chip_data.soc_pkg.speed[MN_MGMT_PORT_0] &&
		         _tdm->_chip_data.soc_pkg.speed[MN_MGMT_PORT_1]<SPEED_10G){
			_tdm->_chip_data.soc_pkg.state[MN_MGMT_PORT_0] = PORT_STATE__MANAGEMENT;
			_tdm->_chip_data.soc_pkg.state[MN_MGMT_PORT_1] = PORT_STATE__MANAGEMENT;
			_tdm->_chip_data.soc_pkg.state[MN_MGMT_PORT_2] = PORT_STATE__MANAGEMENT;
			_tdm->_chip_data.soc_pkg.state[MN_MGMT_PORT_3] = PORT_STATE__MANAGEMENT;
		}
	}*/
	/* encap */
	for (idx=0; idx<MN_NUM_PM_MOD; idx++) {
		_tdm->_chip_data.soc_pkg.soc_vars.mn.pm_encap_type[idx] = (_tdm->_chip_data.soc_pkg.state[idx*4]==PORT_STATE__LINERATE_HG||_tdm->_chip_data.soc_pkg.state[idx]==PORT_STATE__OVERSUB_HG)?(PM_ENCAP__HIGIG2):(PM_ENCAP__ETHRNT);
	}
	/* pmap */
	_tdm->_chip_data.soc_pkg.pmap=(int **) TDM_MN_ALLOC((_tdm->_chip_data.soc_pkg.pmap_num_modules)*sizeof(int *), "portmod_map_l1");
	for (idx=0; idx<(_tdm->_chip_data.soc_pkg.pmap_num_modules); idx++) {
		_tdm->_chip_data.soc_pkg.pmap[idx]=(int *) TDM_MN_ALLOC((_tdm->_chip_data.soc_pkg.pmap_num_lanes)*sizeof(int), "portmod_map_l2");
		TDM_MSET(_tdm->_chip_data.soc_pkg.pmap[idx],(_tdm->_chip_data.soc_pkg.num_ext_ports),_tdm->_chip_data.soc_pkg.pmap_num_lanes);
	}
	/* PGW x0 calendar group */
	_tdm->_chip_data.cal_0.cal_main=(int *) TDM_MN_ALLOC((_tdm->_chip_data.cal_0.cal_len)*sizeof(int), "TDM inst 0 main calendar");
	TDM_MSET(_tdm->_chip_data.cal_0.cal_main,(_tdm->_chip_data.soc_pkg.num_ext_ports),_tdm->_chip_data.cal_0.cal_len);
	_tdm->_chip_data.cal_0.cal_grp=(int **) TDM_MN_ALLOC((_tdm->_chip_data.cal_0.grp_num)*sizeof(int *), "TDM inst 0 groups");
	for (idx=0; idx<(_tdm->_chip_data.cal_0.grp_num); idx++) {
		_tdm->_chip_data.cal_0.cal_grp[idx]=(int *) TDM_MN_ALLOC((_tdm->_chip_data.cal_0.grp_len)*sizeof(int), "TDM inst 0 group calendars");
		TDM_MSET(_tdm->_chip_data.cal_0.cal_grp[idx],(_tdm->_chip_data.soc_pkg.num_ext_ports),_tdm->_chip_data.cal_0.grp_len);
	}
	/* PGW x1 calendar group */
	_tdm->_chip_data.cal_1.cal_main=(int *) TDM_MN_ALLOC((_tdm->_chip_data.cal_1.cal_len)*sizeof(int), "TDM inst 1 main calendar");	
	TDM_MSET(_tdm->_chip_data.cal_1.cal_main,(_tdm->_chip_data.soc_pkg.num_ext_ports),_tdm->_chip_data.cal_1.cal_len);
	_tdm->_chip_data.cal_1.cal_grp=(int **) TDM_MN_ALLOC((_tdm->_chip_data.cal_1.grp_num)*sizeof(int *), "TDM inst 1 groups");
	for (idx=0; idx<(_tdm->_chip_data.cal_1.grp_num); idx++) {
		_tdm->_chip_data.cal_1.cal_grp[idx]=(int *) TDM_MN_ALLOC((_tdm->_chip_data.cal_1.grp_len)*sizeof(int), "TDM inst 1 group calendars");
		TDM_MSET(_tdm->_chip_data.cal_1.cal_grp[idx],(_tdm->_chip_data.soc_pkg.num_ext_ports),_tdm->_chip_data.cal_1.grp_len);
	}

	/* MMU x pipe calendar group */
	_tdm->_chip_data.cal_2.cal_main=(int *) TDM_MN_ALLOC((_tdm->_chip_data.cal_2.cal_len)*sizeof(int), "TDM inst 2 main calendar");	
	TDM_MSET(_tdm->_chip_data.cal_2.cal_main,(_tdm->_chip_data.soc_pkg.num_ext_ports),_tdm->_chip_data.cal_2.cal_len);
	_tdm->_chip_data.cal_2.cal_grp=(int **) TDM_MN_ALLOC((_tdm->_chip_data.cal_2.grp_num)*sizeof(int *), "TDM inst 2 groups");
	for (idx=0; idx<(_tdm->_chip_data.cal_2.grp_num); idx++) {
		_tdm->_chip_data.cal_2.cal_grp[idx]=(int *) TDM_MN_ALLOC((_tdm->_chip_data.cal_2.grp_len)*sizeof(int), "TDM inst 2 group calendars");
		TDM_MSET(_tdm->_chip_data.cal_2.cal_grp[idx],(_tdm->_chip_data.soc_pkg.num_ext_ports),_tdm->_chip_data.cal_2.grp_len);
	}
    /* Core: VMAP */
    _tdm->_core_data.vmap=(unsigned short **) TDM_MN_ALLOC((_tdm->_core_data.vmap_max_wid)*sizeof(unsigned short *), "vector_map_l1");
    for (idx=0; idx<(_tdm->_core_data.vmap_max_wid); idx++) {
        _tdm->_core_data.vmap[idx]=(unsigned short *) TDM_MN_ALLOC((_tdm->_core_data.vmap_max_len)*sizeof(unsigned short), "vector_map_l2");
    }

    _tdm->_chip_data.cal_4.cal_main= NULL;
    _tdm->_chip_data.cal_4.cal_grp = NULL;
    _tdm->_chip_data.cal_5.cal_main= NULL;
    _tdm->_chip_data.cal_5.cal_grp = NULL;
    _tdm->_chip_data.cal_6.cal_main= NULL;
    _tdm->_chip_data.cal_6.cal_grp = NULL;
    _tdm->_chip_data.cal_7.cal_main= NULL;
    _tdm->_chip_data.cal_7.cal_grp = NULL;

	/* IARB static calendar group */
	_tdm->_chip_data.cal_3.cal_main=(int *) TDM_MN_ALLOC((_tdm->_chip_data.cal_3.cal_len)*sizeof(int), "TDM inst 3 main calendar");	
	TDM_MSET(_tdm->_chip_data.cal_3.cal_main,(_tdm->_chip_data.soc_pkg.num_ext_ports),_tdm->_chip_data.cal_3.cal_len);

	return ( _tdm->_chip_exec[TDM_CHIP_EXEC__TRANSCRIPTION]( _tdm ) );
}


/**
@name: tdm_ap_post
@param:
 */
int
tdm_mn_post( tdm_mod_t *_tdm )
{
	return PASS;
}

/**
@name: tdm_ap_free
@param:
 */
int
tdm_mn_free( tdm_mod_t *_tdm )
{
    int idx;
    /* Chip: pmap */
    for (idx=0; idx<(_tdm->_chip_data.soc_pkg.pmap_num_modules); idx++) {
        TDM_FREE(_tdm->_chip_data.soc_pkg.pmap[idx]);
    }
    TDM_FREE(_tdm->_chip_data.soc_pkg.pmap);
    /* Chip: PGW x0 calendar group */
    TDM_FREE(_tdm->_chip_data.cal_0.cal_main);
    for (idx=0; idx<(_tdm->_chip_data.cal_0.grp_num); idx++) {
        TDM_FREE(_tdm->_chip_data.cal_0.cal_grp[idx]);
    }
    TDM_FREE(_tdm->_chip_data.cal_0.cal_grp);
    /* Chip: PGW x1 calendar group */
    TDM_FREE(_tdm->_chip_data.cal_1.cal_main);
    for (idx=0; idx<(_tdm->_chip_data.cal_1.grp_num); idx++) {
        TDM_FREE(_tdm->_chip_data.cal_1.cal_grp[idx]);
    }
    TDM_FREE(_tdm->_chip_data.cal_1.cal_grp);
    /* Chip: MMU x pipe calendar group */
    TDM_FREE(_tdm->_chip_data.cal_2.cal_main);
    for (idx=0; idx<(_tdm->_chip_data.cal_2.grp_num); idx++) {
        TDM_FREE(_tdm->_chip_data.cal_2.cal_grp[idx]);
    }
    TDM_FREE(_tdm->_chip_data.cal_2.cal_grp);
    /* Chip: IARB static calendar group */
    TDM_FREE(_tdm->_chip_data.cal_3.cal_main);
    /* Core: vmap */
    for (idx=0; idx<(_tdm->_core_data.vmap_max_wid); idx++) {
        TDM_FREE(_tdm->_core_data.vmap[idx]);
    }
    TDM_FREE(_tdm->_core_data.vmap);

    return PASS;
}
/*int mn_is_clport(int pnum) {
  if(pnum > 64) return false;
  if( (pnum >=1 && pnum <=20) || (pnum >=45 && pnum <=64)
    return 1;
  else 
    return 0;
}
bool mn_chk_clport_lrspeed(enum port_state_e *state, enum port_speed_e *speed ) {

int loop;
bool flag;

  for(loop = 0; loop < MN_NUM_PHY_PORTS; loop++) {
    if(state) {
      if(state[loop]==PORT_STATE__LINERATE && mn_is_clport((unsigned int) loop)) {
        flag = TRUE;
      }
    }
  }
  return flag;
} */
#endif /* CDK_CONFIG_INCLUDE_BCM56670_A0 */
