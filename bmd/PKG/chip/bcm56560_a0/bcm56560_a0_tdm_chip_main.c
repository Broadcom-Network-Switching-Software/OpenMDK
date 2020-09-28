/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 * $All Rights Reserved.$
 *
 * TDM chip main functions
 */
#include "bcm56560_a0_tdm_core_top.h"
#include <cdk/cdk_debug.h>

/**
@name: tdm_ap_corereq
@param:

Allocate memory for core data execute request to core executive
 */
int
tdm_ap_corereq( tdm_mod_t *_tdm )
{
    int idx1;
    
    /* X-Pipe: */
    if (_tdm->_core_data.vars_pkg.pipe==AP_PIPE_X_ID) {
        _tdm->_core_data.vars_pkg.cal_id=2;
        _tdm->_core_data.vmap=(unsigned short **) TDM_ALLOC((_tdm->_core_data.vmap_max_wid)*sizeof(unsigned short *), "vector_map_l1");
        for (idx1=0; idx1<(_tdm->_core_data.vmap_max_wid); idx1++) {
            _tdm->_core_data.vmap[idx1]=(unsigned short *) TDM_ALLOC((_tdm->_core_data.vmap_max_len)*sizeof(unsigned short), "vector_map_l2");
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
@name: tdm_ap_vbs_wrapper
@param:

Code wrapper for egress TDM scheduling
 */
int
tdm_ap_vbs_wrapper( tdm_mod_t *_tdm )
{
    int idx1, acc, clk, lr_buffer_idx=0, os_buffer_idx=0, higig_mgmt=BOOL_FALSE, lr_idx_limit=LEN_760MHZ_EN, port;
    int *pgw_tdm_tbl_0, *pgw_tdm_tbl_1, *ovs_tdm_tbl_0, *ovs_tdm_tbl_1;

  /* to get actual pmap for spacing and other functions */
 /* tdm_ap_actual_pmap(_tdm);*/
    
    /* accessory slots */
    switch (_tdm->_chip_data.soc_pkg.clk_freq) {
        case 933:
            clk=933; acc=AP_ACC_PORT_NUM; break;
        case 840:
            clk=840; acc=AP_ACC_PORT_NUM; break;
        case 793:
        case 794: 
            clk=793; acc=(AP_ACC_PORT_NUM-1); break;
        case 575: 
            clk=575; acc=AP_ACC_PORT_NUM; break;
        case 510:
            clk=510; acc=(AP_ACC_PORT_NUM+4); break;
        case 435:
            clk=435; acc=(AP_ACC_PORT_NUM+1); break;
        default:
            TDM_ERROR1("Invalid frequency %d (MHz)\n", _tdm->_chip_data.soc_pkg.clk_freq);
            return 0;
    }
 
  /*acc=0;*/
    
    switch (_tdm->_core_data.vars_pkg.pipe){
        case AP_PIPE_X_ID:   tdm_ap_print_quad(_tdm->_chip_data.soc_pkg.speed,_tdm->_chip_data.soc_pkg.state, AP_NUM_PHY_PORTS, 1,   72); break;
        default:
            TDM_ERROR1("Invalid pipe ID %d \n", _tdm->_core_data.vars_pkg.pipe);
            return 0;
    }
    
    /* lr_buff and os_buff */
        pgw_tdm_tbl_0 = _tdm->_chip_data.cal_0.cal_main;
        pgw_tdm_tbl_1 = _tdm->_chip_data.cal_1.cal_main;
        ovs_tdm_tbl_0 = _tdm->_chip_data.cal_0.cal_grp[0];
        ovs_tdm_tbl_1 = _tdm->_chip_data.cal_1.cal_grp[0];
    
  for (idx1=0; idx1<TDM_AUX_SIZE; idx1++) {
        _tdm->_core_data.vars_pkg.lr_buffer[idx1]=AP_NUM_EXT_PORTS;
        _tdm->_core_data.vars_pkg.os_buffer[idx1]=AP_NUM_EXT_PORTS;
    }
    for (idx1=0; idx1<AP_LR_LLS_LEN; idx1++) {
        AP_TOKEN_CHECK(pgw_tdm_tbl_0[idx1]) {
            _tdm->_core_data.vars_pkg.lr_buffer[lr_buffer_idx++]=pgw_tdm_tbl_0[idx1];
        }
    }
    for (idx1=0; idx1<AP_LR_LLS_LEN; idx1++) {
        AP_TOKEN_CHECK(pgw_tdm_tbl_1[idx1]) {
            _tdm->_core_data.vars_pkg.lr_buffer[lr_buffer_idx++]=pgw_tdm_tbl_1[idx1];
        }
    }
    for (idx1=0; idx1<AP_OS_LLS_GRP_LEN; idx1++) {
        AP_TOKEN_CHECK(ovs_tdm_tbl_0[idx1]) {
            _tdm->_core_data.vars_pkg.os_buffer[os_buffer_idx++]=ovs_tdm_tbl_0[idx1];
        }
    }
    for (idx1=0; idx1<AP_OS_LLS_GRP_LEN; idx1++) {
        AP_TOKEN_CHECK(ovs_tdm_tbl_1[idx1]) {
            _tdm->_core_data.vars_pkg.os_buffer[os_buffer_idx++]=ovs_tdm_tbl_1[idx1];
        }
    }
    
    /* lr_idx_limit */
    port = _tdm->_core_data.vars_pkg.port;
    _tdm->_core_data.vars_pkg.port = _tdm->_core_data.vars_pkg.lr_buffer[0];
    switch (clk) {
        case 933: lr_idx_limit=(LEN_933MHZ_EN-acc); break;
        case 840: lr_idx_limit=(LEN_840MHZ_EN-acc); break;
        case 793: case 794: lr_idx_limit=(LEN_793MHZ_EN-acc); break;
        case 575: lr_idx_limit=(LEN_575MHZ_EN-acc); break;
        case 510: lr_idx_limit=(LEN_510MHZ_EN-acc); break;
        case 435: lr_idx_limit=(LEN_435MHZ_EN-acc); break;
        default : break;
    }
    _tdm->_core_data.vars_pkg.port = port;
    
    /* core parameters */
    _tdm->_core_data.vmap_max_len        = AP_VMAP_MAX_LEN;
    _tdm->_core_data.vmap_max_wid        = AP_VMAP_MAX_WID;
    _tdm->_core_data.rule__same_port_min = LLS_MIN_SPACING;
    _tdm->_core_data.rule__prox_port_min = VBS_MIN_SPACING;
    
    _tdm->_chip_data.soc_pkg.clk_freq    = clk;
    _tdm->_chip_data.soc_pkg.tvec_size   = acc;
    _tdm->_chip_data.soc_pkg.lr_idx_limit= lr_idx_limit;
    _tdm->_chip_data.soc_pkg.soc_vars.ap.higig_mgmt = higig_mgmt;

    return ( _tdm->_chip_exec[TDM_CHIP_EXEC__COREREQ]( _tdm ) );
}


/**
@name: tdm_ap_lls_wrapper
@param:

Code wrapper for ingress TDM scheduling
 */
int
tdm_ap_lls_wrapper( tdm_mod_t *_tdm )
{
    int idx;
  int n, pgw_ll_len, pgw_ll_len_max; 
    tdm_ap_chip_legacy_t *_ap_chip;

    _ap_chip = TDM_ALLOC(sizeof(tdm_ap_chip_legacy_t), "TDM chip legacy");
    if (!_ap_chip) {return FAIL;}
    tdm_chip_ap_shim__ing_wrap(_tdm, _ap_chip);
    
    TDM_PRINT0("TDM: Linked list round robin\n");
    for (idx=0; idx<AP_NUM_QUAD; idx++) {
        _tdm->_chip_data.soc_pkg.soc_vars.ap.pgw_num = idx;
        _tdm->_chip_data.soc_pkg.soc_vars.ap.start_port = (idx*(AP_NUM_PHY_PORTS/AP_NUM_QUAD));
        _tdm->_chip_data.soc_pkg.soc_vars.ap.stop_port = ((idx+1)*(AP_NUM_PHY_PORTS/AP_NUM_QUAD));
        {
            int i, j, first_wc=0, op_flags_str[3], relink, is_state_changed;
            struct ap_ll_node llist;
            ap_pgw_pntrs_t ap_pntr_pkg;
            ap_pgw_scheduler_vars_t ap_vars_pkg;
            int *pgw_tdm_tbl, *ovs_tdm_tbl, *ovs_spacing = 0;
      int cl_port[2] = {130,130};
      int cxx_port = {130};
      enum port_speed_e cl_speed[2], cxx_speed = 0;
            
            op_flags_str[0]=0; 
            op_flags_str[1]=0;
            op_flags_str[2]=0;
            llist.port=0;
            llist.next=NULL;
            ap_pntr_pkg.pgw_tdm_idx=0;
            ap_pntr_pkg.ovs_tdm_idx=0;
            ap_pntr_pkg.tdm_stk_idx=0;
            for (i=0; i<AP_OS_LLS_GRP_LEN; i++){
                ap_vars_pkg.swap_array[i] = 0;
            }
            
            /* PGW calendars */
            switch(_tdm->_chip_data.soc_pkg.soc_vars.ap.start_port) {
                case 0:
                    pgw_tdm_tbl = _ap_chip->tdm_pgw.pgw_tdm_tbl_x0;
                    ovs_tdm_tbl = _ap_chip->tdm_pgw.ovs_tdm_tbl_x0;
                    ovs_spacing = _ap_chip->tdm_pgw.ovs_spacing_x0;
                    break;
                case 36:
                    pgw_tdm_tbl = _ap_chip->tdm_pgw.pgw_tdm_tbl_x1;
                    ovs_tdm_tbl = _ap_chip->tdm_pgw.ovs_tdm_tbl_x1;
                    ovs_spacing = _ap_chip->tdm_pgw.ovs_spacing_x1;
                    break;      
            }

            /* wrap core index */
            switch(_tdm->_chip_data.soc_pkg.soc_vars.ap.pgw_num) {
                case 0: first_wc = 0; break;
                case 1: first_wc = 9; break;
            }
            /* port number range */
            ap_vars_pkg.first_port=_tdm->_chip_data.soc_pkg.soc_vars.ap.start_port;
            ap_vars_pkg.last_port =_tdm->_chip_data.soc_pkg.soc_vars.ap.stop_port;
            /* linked-list, and PGW oversub calendar */
      /* Round robin scheduler with 5G per slot */
            for (j=0; j<8; j++) { 
        if(j>3)  op_flags_str[1]=1; 
        if(j==7) op_flags_str[2]=1; 
                ap_vars_pkg.subport=j%4;
                ap_pntr_pkg.cur_idx=first_wc;
                ap_vars_pkg.cur_idx_max=(first_wc+9);
                tdm_ap_lls_scheduler(&llist, _ap_chip, &ap_pntr_pkg, &ap_vars_pkg, &pgw_tdm_tbl, &ovs_tdm_tbl, op_flags_str);
            }

            switch(_tdm->_chip_data.soc_pkg.clk_freq){
                case 435:
                    if (ap_vars_pkg.first_port==0 &&
                        ap_vars_pkg.last_port==36){
                        pgw_ll_len_max = 26;
                    }
                    else {
                        pgw_ll_len_max = 24;
                    }
                    break;
                case 510:
                    pgw_ll_len_max = 30;
                    break;
                case 575:
                    pgw_ll_len_max = 34;
                    break;
                case 793:
                case 794:
                case 840:
                    pgw_ll_len_max = 48;
                    break;
                case 933:
                    pgw_ll_len_max = 56;
                    break;
                default:
                    pgw_ll_len_max = 64;
                    TDM_ERROR1("TDM: unrecognized frequency %d in PGW\n", 
                               _tdm->_chip_data.soc_pkg.clk_freq);
                    break;
            }
            pgw_ll_len = tdm_ap_ll_len(&llist);
            if (pgw_ll_len>pgw_ll_len_max){
                TDM_PRINT2("TDM: Adjust PGW linked list, len %d, limit %d\n",
                           pgw_ll_len, pgw_ll_len_max);
                n = pgw_ll_len - pgw_ll_len_max;
                for (i=(pgw_ll_len-1); i>0; i--){
                    if (tdm_ap_ll_get(&llist,i)==AP_OVSB_TOKEN){
                        tdm_ap_ll_delete(&llist, i);
                        n--;
                        TDM_PRINT2("%s, index %d\n",
                                  "remove OVSB slot from PGW linked list",
                                  i);
                    }
                    if (n<=0){
                        break;
                    }
                }
                pgw_ll_len = tdm_ap_ll_len(&llist);
                if (pgw_ll_len>pgw_ll_len_max){
                    TDM_WARN5("TDM: %s, pipe %d, length %d, limit %d, max %d\n",
                               "PGW calendar length may overflow",
                               idx,
                               pgw_ll_len,
                               pgw_ll_len_max,
                               48);
                }
            }
    /* check for 25G ports in the llist */
        /* falcon port  numbers */
            switch(_tdm->_chip_data.soc_pkg.soc_vars.ap.pgw_num) {
                case 0:
          cxx_port      = 17;
          cl_port[0]    = 29;
          cl_port[1]    = 33;
          cxx_speed     =  _tdm->_chip_data.soc_pkg.speed[17];
          cl_speed[0]   =  _tdm->_chip_data.soc_pkg.speed[29];
          cl_speed[1]   =  _tdm->_chip_data.soc_pkg.speed[33];
          break;
                case 1: 
          cxx_port      = 53;
          cl_port[0]    = 65;
          cl_port[1]    = 69;
          cxx_speed     =  _tdm->_chip_data.soc_pkg.speed[53];
          cl_speed[0]   =  _tdm->_chip_data.soc_pkg.speed[65];
          cl_speed[1]   =  _tdm->_chip_data.soc_pkg.speed[69];
          break;          
            }
      /* to be called only for 25G lr ports*/
      tdm_ap_ll_retrace_cl(&llist,_ap_chip,cl_port,cl_speed);
      tdm_ap_ll_retrace_25(&llist,_ap_chip->pmap,cl_port,cl_speed);

            ap_pntr_pkg.pgw_tdm_idx=tdm_ap_ll_len(&llist);
            /* same-port-min-spacing */
            relink=BOOL_FALSE;
            for (i=1; i<tdm_ap_ll_len(&llist); i++) {
                AP_TOKEN_CHECK(tdm_ap_ll_get(&llist,i)) {
                    if ( tdm_ap_scan_which_tsc(tdm_ap_ll_get(&llist,i),_ap_chip->pmap)==tdm_ap_scan_which_tsc(tdm_ap_ll_get(&llist,(i-1)),_ap_chip->pmap) )  {
                        relink=BOOL_TRUE;
                        break;
                    }
                }
            }
            if (relink) {
                TDM_PRINT0("TDM: Retracing calendar\n");
                tdm_ap_ll_retrace(&llist,_ap_chip->pmap);
                tdm_ap_ll_print(&llist);
            }

            /* same-port-min-spacing */
            relink=BOOL_FALSE;
            for (i=1; i<tdm_ap_ll_len(&llist); i++) {
                AP_TOKEN_CHECK(tdm_ap_ll_get(&llist,i)) {
                    if ( tdm_ap_scan_which_tsc(tdm_ap_ll_get(&llist,i),_ap_chip->pmap)==tdm_ap_scan_which_tsc(tdm_ap_ll_get(&llist,(i-1)),_ap_chip->pmap) )  {
                        relink=BOOL_TRUE;
                        break;
                    }
                }
            }
            if ( (tdm_ap_ll_count(&llist,1)>0 && tdm_ap_ll_count(&llist,AP_OVSB_TOKEN)>1) || (tdm_ap_ll_single_100(&llist)) || (relink) ) {
                TDM_PRINT0("TDM: Reweaving calendar\n");
                tdm_ap_ll_weave(&llist,_ap_chip->pmap,AP_OVSB_TOKEN);
                tdm_ap_ll_print(&llist);
            }
          
      /* check for port configs if ports are still back to back*/
            for (i=1; i<tdm_ap_ll_len(&llist); i++) {
                AP_TOKEN_CHECK(tdm_ap_ll_get(&llist,i)) {
                    if ( tdm_ap_scan_which_tsc(tdm_ap_ll_get(&llist,i),_ap_chip->pmap)==tdm_ap_scan_which_tsc(tdm_ap_ll_get(&llist,(i-1)),_ap_chip->pmap) )  {
                        TDM_ERROR1("SPACING VOILATION:Found back to back ports in PGW TDM TBL: %0d\n",tdm_ap_ll_get(&llist,i));
                        TDM_FREE(_ap_chip);
            return (TDM_EXEC_CORE_SIZE+1);  
                    }
                }
            }
            /* PGW main calendar */
            tdm_ap_ll_deref(&llist,&pgw_tdm_tbl,AP_LR_LLS_LEN);
      if((_tdm->_chip_data.soc_pkg.state[cl_port[0]]==PORT_STATE__LINERATE || _tdm->_chip_data.soc_pkg.state[cl_port[1]]==PORT_STATE__LINERATE) && (_tdm->_chip_data.soc_pkg.clk_freq==793 || _tdm->_chip_data.soc_pkg.clk_freq==794) && cxx_speed != SPEED_100G)
       tdm_ap_reconfig_pgw_tbl(pgw_tdm_tbl,cl_port,cl_speed);

            TDM_SML_BAR
            tdm_ap_print_tbl(pgw_tdm_tbl,AP_LR_LLS_LEN,"PGW Main Calendar",idx);

            /* PGW oversub spacer calendar */
      if (ovs_tdm_tbl[0]!=AP_NUM_EXT_PORTS) {
        tdm_ap_ovs_20_40_clport(ovs_tdm_tbl,_ap_chip);
              if ( _ap_chip->tdm_globals.cl_flag || cxx_speed == SPEED_100G) { 
          tdm_ap_clport_ovs_scheduler(ovs_tdm_tbl,cl_port,cl_speed,cxx_port,cxx_speed,_ap_chip->tdm_globals.clk_freq);
        }
                tdm_ap_ovs_spacer(ovs_tdm_tbl,ovs_spacing);
                tdm_ap_print_tbl_ovs(ovs_tdm_tbl,ovs_spacing,AP_OS_LLS_GRP_LEN,"PGW Oversub Calendar",idx);
                
                is_state_changed = BOOL_FALSE;
                for (i=0; i<AP_OS_LLS_GRP_LEN; i++) {
                    if ( (ovs_tdm_tbl[i]!=AP_NUM_EXT_PORTS) && 
                         (_tdm->_chip_data.soc_pkg.state[ovs_tdm_tbl[i]]==PORT_STATE__LINERATE || _tdm->_chip_data.soc_pkg.state[ovs_tdm_tbl[i]]==PORT_STATE__LINERATE_HG) ){
                        switch(_tdm->_chip_data.soc_pkg.state[ovs_tdm_tbl[i]]){
                            case PORT_STATE__LINERATE: 
                                _tdm->_chip_data.soc_pkg.state[ovs_tdm_tbl[i]]=PORT_STATE__OVERSUB;
                                is_state_changed = BOOL_TRUE;
                                break;
                            case PORT_STATE__LINERATE_HG:
                                _tdm->_chip_data.soc_pkg.state[ovs_tdm_tbl[i]]=PORT_STATE__OVERSUB_HG;
                                is_state_changed = BOOL_TRUE;
                                break;
                            default:
                                break;
                        }
                        TDM_PRINT1("TDM: Port [%3d] state changes from LINERATE to OVERSUB\n", ovs_tdm_tbl[i]);
                    }
                }
                if(is_state_changed==BOOL_TRUE) {tdm_print_stat( _tdm );}
            }
        tdm_ap_ll_free(&llist); 
    }
    }
    /* Bind to TDM class object */
    TDM_COPY(_tdm->_chip_data.cal_0.cal_main,_ap_chip->tdm_pgw.pgw_tdm_tbl_x0,sizeof(int)*AP_LR_LLS_LEN);
    TDM_COPY(_tdm->_chip_data.cal_0.cal_grp[0],_ap_chip->tdm_pgw.ovs_tdm_tbl_x0,sizeof(int)*AP_OS_LLS_GRP_LEN);
    TDM_COPY(_tdm->_chip_data.cal_0.cal_grp[1],_ap_chip->tdm_pgw.ovs_spacing_x0,sizeof(int)*AP_OS_LLS_GRP_LEN);
    TDM_COPY(_tdm->_chip_data.cal_1.cal_main,_ap_chip->tdm_pgw.pgw_tdm_tbl_x1,sizeof(int)*AP_LR_LLS_LEN);
    TDM_COPY(_tdm->_chip_data.cal_1.cal_grp[0],_ap_chip->tdm_pgw.ovs_tdm_tbl_x1,sizeof(int)*AP_OS_LLS_GRP_LEN);
    TDM_COPY(_tdm->_chip_data.cal_1.cal_grp[1],_ap_chip->tdm_pgw.ovs_spacing_x1,sizeof(int)*AP_OS_LLS_GRP_LEN);
    
    /* Realign port state array to old specification */ 
    for (idx=0; idx<((_tdm->_chip_data.soc_pkg.num_ext_ports)-57); idx++) {
        _tdm->_chip_data.soc_pkg.state[idx] = _tdm->_chip_data.soc_pkg.state[idx+1];
    }
    
    _tdm->_core_data.vars_pkg.pipe=AP_PIPE_X_ID;
    TDM_FREE(_ap_chip);
    return ( _tdm->_chip_exec[TDM_CHIP_EXEC__EGRESS_WRAP]( _tdm ) );
}


/**
@name: tdm_ap_pmap_transcription
@param:

For Trident2+ BCM56850
Transcription algorithm for generating port module mapping

    40G     - xxxx
        20G     - xx
                xx
        10G - x
           x
            x
             x
        1X0G    - xxxx_xxxx_xxxx
 */
int
tdm_ap_pmap_transcription( tdm_mod_t *_tdm )
{
    int i, j, last_port=AP_NUM_EXT_PORTS, tsc_active;
  int phy_lo, phy_hi, pm_lane_num, bcm_config_check=PASS;
    
    /* Regular ports */
    for (i=1; i<=AP_NUM_PHY_PORTS; i+=AP_NUM_PM_LNS) {
        tsc_active=BOOL_FALSE;
        for (j=0; j<AP_NUM_PM_LNS; j++) {
            if ( _tdm->_chip_data.soc_pkg.state[i+j]==PORT_STATE__LINERATE    || _tdm->_chip_data.soc_pkg.state[i+j]==PORT_STATE__OVERSUB    ||
                 _tdm->_chip_data.soc_pkg.state[i+j]==PORT_STATE__LINERATE_HG || _tdm->_chip_data.soc_pkg.state[i+j]==PORT_STATE__OVERSUB_HG ){
                tsc_active=BOOL_TRUE;
                break;
            }
        }
        if(tsc_active==BOOL_TRUE){
            if ( (i ==17 || i==53)  && _tdm->_chip_data.soc_pkg.speed[i]==SPEED_100G ) {
                    _tdm->_chip_data.soc_pkg.pmap[(i-1)/AP_NUM_PM_LNS][0] = i;
                    _tdm->_chip_data.soc_pkg.pmap[(i-1)/AP_NUM_PM_LNS][1] = i;
                    _tdm->_chip_data.soc_pkg.pmap[(i-1)/AP_NUM_PM_LNS][2] = i;
                    _tdm->_chip_data.soc_pkg.pmap[(i-1)/AP_NUM_PM_LNS][3] = i;
                    _tdm->_chip_data.soc_pkg.pmap[(i+3)/AP_NUM_PM_LNS][0] = i;
                    _tdm->_chip_data.soc_pkg.pmap[(i+3)/AP_NUM_PM_LNS][1] = i;
                    _tdm->_chip_data.soc_pkg.pmap[(i+3)/AP_NUM_PM_LNS][2] = i;
                    _tdm->_chip_data.soc_pkg.pmap[(i+3)/AP_NUM_PM_LNS][3] = i;
                    _tdm->_chip_data.soc_pkg.pmap[(i+7)/AP_NUM_PM_LNS][0] = i;
                    _tdm->_chip_data.soc_pkg.pmap[(i+7)/AP_NUM_PM_LNS][1] = i;
                    _tdm->_chip_data.soc_pkg.pmap[(i+7)/AP_NUM_PM_LNS][2] = AP_NUM_EXT_PORTS;
                    _tdm->_chip_data.soc_pkg.pmap[(i+7)/AP_NUM_PM_LNS][3] = AP_NUM_EXT_PORTS;
                i+=8;
                }
            else {
            if ( _tdm->_chip_data.soc_pkg.speed[i]>SPEED_0 || _tdm->_chip_data.soc_pkg.state[i]==PORT_STATE__DISABLED ) {
                for (j=0; j<AP_NUM_PM_LNS; j++) {
                    switch (_tdm->_chip_data.soc_pkg.state[i+j]) {
                        case PORT_STATE__LINERATE:
                        case PORT_STATE__OVERSUB:
                        case PORT_STATE__LINERATE_HG:
                        case PORT_STATE__OVERSUB_HG:
                            _tdm->_chip_data.soc_pkg.pmap[(i-1)/AP_NUM_PM_LNS][j]=(i+j);
                            last_port=(i+j);
                            break;
                        case PORT_STATE__COMBINE:
                            _tdm->_chip_data.soc_pkg.pmap[(i-1)/AP_NUM_PM_LNS][j]=last_port;
                            break;
                        default:
                            _tdm->_chip_data.soc_pkg.pmap[(i-1)/AP_NUM_PM_LNS][j]=AP_NUM_EXT_PORTS;
                            break;
                    }
                }
                if ( _tdm->_chip_data.soc_pkg.speed[i]>_tdm->_chip_data.soc_pkg.speed[i+2] && _tdm->_chip_data.soc_pkg.speed[i+2]==_tdm->_chip_data.soc_pkg.speed[i+3] && _tdm->_chip_data.soc_pkg.speed[i+2]!=SPEED_0 &&_tdm->_chip_data.soc_pkg.speed[i]>=SPEED_40G ) {
                    _tdm->_chip_data.soc_pkg.pmap[(i-1)/AP_NUM_PM_LNS][1] = _tdm->_chip_data.soc_pkg.pmap[(i-1)/AP_NUM_PM_LNS][2];
                    _tdm->_chip_data.soc_pkg.pmap[(i-1)/AP_NUM_PM_LNS][2] = _tdm->_chip_data.soc_pkg.pmap[(i-1)/AP_NUM_PM_LNS][0];
                }
                else if ( _tdm->_chip_data.soc_pkg.speed[i]==_tdm->_chip_data.soc_pkg.speed[i+1] && _tdm->_chip_data.soc_pkg.speed[i]<_tdm->_chip_data.soc_pkg.speed[i+2] && _tdm->_chip_data.soc_pkg.speed[i]!=SPEED_0 &&_tdm->_chip_data.soc_pkg.speed[i+2]>=SPEED_40G )  {
                    _tdm->_chip_data.soc_pkg.pmap[(i-1)/AP_NUM_PM_LNS][2] = _tdm->_chip_data.soc_pkg.pmap[(i-1)/AP_NUM_PM_LNS][1];
                    _tdm->_chip_data.soc_pkg.pmap[(i-1)/AP_NUM_PM_LNS][1] = _tdm->_chip_data.soc_pkg.pmap[(i-1)/AP_NUM_PM_LNS][3];
                }
            /* dual mode x_x_ */
                else if (_tdm->_chip_data.soc_pkg.speed[i]!=_tdm->_chip_data.soc_pkg.speed[i+1]&&_tdm->_chip_data.soc_pkg.speed[i]==_tdm->_chip_data.soc_pkg.speed[i+2]&&_tdm->_chip_data.soc_pkg.speed[i]>=SPEED_40G) {
                _tdm->_chip_data.soc_pkg.pmap[(i-1)/AP_NUM_PM_LNS][1] = _tdm->_chip_data.soc_pkg.pmap[(i-1)/AP_NUM_PM_LNS][3];
                _tdm->_chip_data.soc_pkg.pmap[(i-1)/AP_NUM_PM_LNS][2] = _tdm->_chip_data.soc_pkg.pmap[(i-1)/AP_NUM_PM_LNS][0];
                }
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
    for (i=1; i<=AP_NUM_PHY_PORTS; i+=AP_NUM_PM_LNS) {
      if(i==AP_CL1_PORT || i==AP_CL2_PORT || i==AP_CL4_PORT || i==AP_CL5_PORT ){
                if ( ( _tdm->_chip_data.soc_pkg.speed[i]>_tdm->_chip_data.soc_pkg.speed[i+2] && _tdm->_chip_data.soc_pkg.speed[i+2]==_tdm->_chip_data.soc_pkg.speed[i+3] && _tdm->_chip_data.soc_pkg.speed[i+2]!=SPEED_0 && _tdm->_chip_data.soc_pkg.speed[i]>=SPEED_40G ) || ( _tdm->_chip_data.soc_pkg.speed[i]==_tdm->_chip_data.soc_pkg.speed[i+1] && _tdm->_chip_data.soc_pkg.speed[i]<_tdm->_chip_data.soc_pkg.speed[i+2] && _tdm->_chip_data.soc_pkg.speed[i]!=SPEED_0 && _tdm->_chip_data.soc_pkg.speed[i+2]>=SPEED_40G )) {
            TDM_ERROR8("tri port configuration(25GE+50GE)on Falcon not supported in APACHE, port [%3d, %3d, %3d, %3d], speed [%3dG, %3dG, %3dG, %3dG]\n", i, i+1, i+2, i+3, _tdm->_chip_data.soc_pkg.speed[i]/1000, _tdm->_chip_data.soc_pkg.speed[i+1]/1000, _tdm->_chip_data.soc_pkg.speed[i+2]/1000, _tdm->_chip_data.soc_pkg.speed[i+3]/1000);
          return FAIL;
                }
      }
    }
    
    return ( _tdm->_chip_exec[TDM_CHIP_EXEC__INGRESS_WRAP]( _tdm ) );
}


/**
@name: tdm_chip_ap_init
@param:
 */
int
tdm_ap_init( tdm_mod_t *_tdm )
{
    int idx;
    
    _tdm->_chip_data.soc_pkg.pmap_num_modules = AP_NUM_PM_MOD;
    _tdm->_chip_data.soc_pkg.pmap_num_lanes = AP_NUM_PM_LNS;
    _tdm->_chip_data.soc_pkg.pm_num_phy_modules = AP_NUM_PHY_PM;
    
    _tdm->_chip_data.soc_pkg.soc_vars.ovsb_token = AP_OVSB_TOKEN;
    _tdm->_chip_data.soc_pkg.soc_vars.idl1_token = AP_IDL1_TOKEN;
    _tdm->_chip_data.soc_pkg.soc_vars.idl2_token = AP_IDL2_TOKEN;
    _tdm->_chip_data.soc_pkg.soc_vars.ancl_token = AP_ANCL_TOKEN;
    _tdm->_chip_data.soc_pkg.soc_vars.fp_port_lo = 1;
    _tdm->_chip_data.soc_pkg.soc_vars.fp_port_hi = AP_NUM_PHY_PORTS;
    
    _tdm->_chip_data.cal_0.cal_len = AP_LR_LLS_LEN;
    _tdm->_chip_data.cal_0.grp_num = AP_OS_LLS_GRP_NUM;
    _tdm->_chip_data.cal_0.grp_len = AP_OS_LLS_GRP_LEN;
    _tdm->_chip_data.cal_1.cal_len = AP_LR_LLS_LEN;
    _tdm->_chip_data.cal_1.grp_num = AP_OS_LLS_GRP_NUM;
    _tdm->_chip_data.cal_1.grp_len = AP_OS_LLS_GRP_LEN;
    _tdm->_chip_data.cal_2.cal_len = AP_LR_VBS_LEN;
    _tdm->_chip_data.cal_2.grp_num = AP_OS_VBS_GRP_NUM;
    _tdm->_chip_data.cal_2.grp_len = AP_OS_VBS_GRP_LEN;
    _tdm->_chip_data.cal_3.cal_len = AP_LR_IARB_STATIC_LEN;
    _tdm->_chip_data.cal_3.grp_num = 0;
    _tdm->_chip_data.cal_3.grp_len = 0;

    
    /* management port state */
    /*if (_tdm->_chip_data.soc_pkg.speed[AP_MGMT_PORT_0] > SPEED_0   && 
        _tdm->_chip_data.soc_pkg.speed[AP_MGMT_PORT_0]<= SPEED_10G &&
        _tdm->_chip_data.soc_pkg.speed[AP_MGMT_PORT_1]==_tdm->_chip_data.soc_pkg.speed[AP_MGMT_PORT_2] && 
        _tdm->_chip_data.soc_pkg.speed[AP_MGMT_PORT_2]==_tdm->_chip_data.soc_pkg.speed[AP_MGMT_PORT_3] ){
        if (_tdm->_chip_data.soc_pkg.speed[AP_MGMT_PORT_0]==SPEED_10G && _tdm->_chip_data.soc_pkg.speed[AP_MGMT_PORT_1]==SPEED_0){
            _tdm->_chip_data.soc_pkg.state[AP_MGMT_PORT_0] = PORT_STATE__MANAGEMENT;
            _tdm->_chip_data.soc_pkg.state[AP_MGMT_PORT_1] = PORT_STATE__DISABLED;
            _tdm->_chip_data.soc_pkg.state[AP_MGMT_PORT_2] = PORT_STATE__DISABLED;
            _tdm->_chip_data.soc_pkg.state[AP_MGMT_PORT_3] = PORT_STATE__DISABLED;
        }
        else if (_tdm->_chip_data.soc_pkg.speed[AP_MGMT_PORT_1]==_tdm->_chip_data.soc_pkg.speed[AP_MGMT_PORT_0] &&
                 _tdm->_chip_data.soc_pkg.speed[AP_MGMT_PORT_1]<SPEED_10G){
            _tdm->_chip_data.soc_pkg.state[AP_MGMT_PORT_0] = PORT_STATE__MANAGEMENT;
            _tdm->_chip_data.soc_pkg.state[AP_MGMT_PORT_1] = PORT_STATE__MANAGEMENT;
            _tdm->_chip_data.soc_pkg.state[AP_MGMT_PORT_2] = PORT_STATE__MANAGEMENT;
            _tdm->_chip_data.soc_pkg.state[AP_MGMT_PORT_3] = PORT_STATE__MANAGEMENT;
        }
    }*/
    /* encap */
    for (idx=0; idx<AP_NUM_PM_MOD; idx++) {
        _tdm->_chip_data.soc_pkg.soc_vars.ap.pm_encap_type[idx] = (_tdm->_chip_data.soc_pkg.state[idx*4]==PORT_STATE__LINERATE_HG||_tdm->_chip_data.soc_pkg.state[idx]==PORT_STATE__OVERSUB_HG)?(PM_ENCAP__HIGIG2):(PM_ENCAP__ETHRNT);
    }
    /* pmap */
    _tdm->_chip_data.soc_pkg.pmap=(int **) TDM_ALLOC((_tdm->_chip_data.soc_pkg.pmap_num_modules)*sizeof(int *), "portmod_map_l1");
    for (idx=0; idx<(_tdm->_chip_data.soc_pkg.pmap_num_modules); idx++) {
        _tdm->_chip_data.soc_pkg.pmap[idx]=(int *) TDM_ALLOC((_tdm->_chip_data.soc_pkg.pmap_num_lanes)*sizeof(int), "portmod_map_l2");
        TDM_MSET(_tdm->_chip_data.soc_pkg.pmap[idx],(_tdm->_chip_data.soc_pkg.num_ext_ports),_tdm->_chip_data.soc_pkg.pmap_num_lanes);
    }
    /* PGW x0 calendar group */
    _tdm->_chip_data.cal_0.cal_main=(int *) TDM_ALLOC((_tdm->_chip_data.cal_0.cal_len)*sizeof(int), "TDM inst 0 main calendar");
    TDM_MSET(_tdm->_chip_data.cal_0.cal_main,(_tdm->_chip_data.soc_pkg.num_ext_ports),_tdm->_chip_data.cal_0.cal_len);
    _tdm->_chip_data.cal_0.cal_grp=(int **) TDM_ALLOC((_tdm->_chip_data.cal_0.grp_num)*sizeof(int *), "TDM inst 0 groups");
    for (idx=0; idx<(_tdm->_chip_data.cal_0.grp_num); idx++) {
        _tdm->_chip_data.cal_0.cal_grp[idx]=(int *) TDM_ALLOC((_tdm->_chip_data.cal_0.grp_len)*sizeof(int), "TDM inst 0 group calendars");
        TDM_MSET(_tdm->_chip_data.cal_0.cal_grp[idx],(_tdm->_chip_data.soc_pkg.num_ext_ports),_tdm->_chip_data.cal_0.grp_len);
    }
    /* PGW x1 calendar group */
    _tdm->_chip_data.cal_1.cal_main=(int *) TDM_ALLOC((_tdm->_chip_data.cal_1.cal_len)*sizeof(int), "TDM inst 1 main calendar");    
    TDM_MSET(_tdm->_chip_data.cal_1.cal_main,(_tdm->_chip_data.soc_pkg.num_ext_ports),_tdm->_chip_data.cal_1.cal_len);
    _tdm->_chip_data.cal_1.cal_grp=(int **) TDM_ALLOC((_tdm->_chip_data.cal_1.grp_num)*sizeof(int *), "TDM inst 1 groups");
    for (idx=0; idx<(_tdm->_chip_data.cal_1.grp_num); idx++) {
        _tdm->_chip_data.cal_1.cal_grp[idx]=(int *) TDM_ALLOC((_tdm->_chip_data.cal_1.grp_len)*sizeof(int), "TDM inst 1 group calendars");
        TDM_MSET(_tdm->_chip_data.cal_1.cal_grp[idx],(_tdm->_chip_data.soc_pkg.num_ext_ports),_tdm->_chip_data.cal_1.grp_len);
    }

    /* MMU x pipe calendar group */
    _tdm->_chip_data.cal_2.cal_main=(int *) TDM_ALLOC((_tdm->_chip_data.cal_2.cal_len)*sizeof(int), "TDM inst 2 main calendar");    
    TDM_MSET(_tdm->_chip_data.cal_2.cal_main,(_tdm->_chip_data.soc_pkg.num_ext_ports),_tdm->_chip_data.cal_2.cal_len);
    _tdm->_chip_data.cal_2.cal_grp=(int **) TDM_ALLOC((_tdm->_chip_data.cal_2.grp_num)*sizeof(int *), "TDM inst 2 groups");
    for (idx=0; idx<(_tdm->_chip_data.cal_2.grp_num); idx++) {
        _tdm->_chip_data.cal_2.cal_grp[idx]=(int *) TDM_ALLOC((_tdm->_chip_data.cal_2.grp_len)*sizeof(int), "TDM inst 2 group calendars");
        TDM_MSET(_tdm->_chip_data.cal_2.cal_grp[idx],(_tdm->_chip_data.soc_pkg.num_ext_ports),_tdm->_chip_data.cal_2.grp_len);
    }

    /* IARB static calendar group */
    _tdm->_chip_data.cal_3.cal_main=(int *) TDM_ALLOC((_tdm->_chip_data.cal_3.cal_len)*sizeof(int), "TDM inst 3 main calendar");    
    TDM_MSET(_tdm->_chip_data.cal_3.cal_main,(_tdm->_chip_data.soc_pkg.num_ext_ports),_tdm->_chip_data.cal_3.cal_len);
    
    return ( _tdm->_chip_exec[TDM_CHIP_EXEC__TRANSCRIPTION]( _tdm ) );
}


/**
@name: tdm_ap_post
@param:
 */
int
tdm_ap_post( tdm_mod_t *_tdm )
{
    /* TDM self-check */
    if (_tdm->_chip_data.soc_pkg.soc_vars.ap.tdm_chk_en==BOOL_TRUE){
        _tdm->_chip_exec[TDM_CHIP_EXEC__CHECK](_tdm);
    }

    return PASS;
}


/**
@name: tdm_ap_free
@param:
 */
int
tdm_ap_free( tdm_mod_t *_tdm )
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

