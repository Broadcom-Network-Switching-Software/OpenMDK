/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 * $All Rights Reserved.$
 *
 * TDM chip operations
 */
#include "bcm56860_a0_tdm_chip_top.h"
#include "bcm56860_a0_tdm_chip_vec.h"


int tdm_core_vec_load(unsigned short **map, int bw, int port_speed, int yy, int len, int num_ext_ports);

/**
@name: tdm_td2p_ovs_spacer
@param:

Reconfigure oversub TDM calendar into Flexport compatibility mode
 */
void
tdm_td2p_ovs_spacer(int (*wc)[TD2P_NUM_PHY_PM][TD2P_NUM_PM_LNS], int *ovs_tdm_tbl, int *ovs_spacing)
{
    int i, j, v=0, w=0, k=0, a, b, c=0;
    int pivot[TD2P_OS_LLS_GRP_LEN];

    for (i=0; i<TD2P_OS_LLS_GRP_LEN; i++) {pivot[i] = -1;}
    for (j=0; j<TD2P_OS_LLS_GRP_LEN; j++) {if (ovs_tdm_tbl[j] == TD2P_NUM_EXT_PORTS) {k++;}}

    /* Evenly distribute idle slots in PGW oversub table */
    switch (k) {
        case 0:
            break;
        case 1:
            for (j=31; j>16; j--) ovs_tdm_tbl[j] = ovs_tdm_tbl[j-1];
            ovs_tdm_tbl[16] = TD2P_NUM_EXT_PORTS;
            break;
        case 2:
            for (j=30; j>16; j--) ovs_tdm_tbl[j] = ovs_tdm_tbl[j-1];
            ovs_tdm_tbl[16] = TD2P_NUM_EXT_PORTS;
            ovs_tdm_tbl[31] = TD2P_NUM_EXT_PORTS;
            break;
        case 3:
            for (j=31; j>8; j--) ovs_tdm_tbl[j] = ovs_tdm_tbl[j-1];
            for (j=31; j>16; j--) ovs_tdm_tbl[j] = ovs_tdm_tbl[j-1];
            for (j=31; j>24; j--) ovs_tdm_tbl[j] = ovs_tdm_tbl[j-1];
            ovs_tdm_tbl[8] = TD2P_NUM_EXT_PORTS;
            ovs_tdm_tbl[16] = TD2P_NUM_EXT_PORTS;
            ovs_tdm_tbl[24] = TD2P_NUM_EXT_PORTS;
            break;
        default:
            k/=4;
            for (i=0; i<TD2P_NUM_PHY_PM; i++) for (j=0; j<TD2P_NUM_PM_LNS; j++) if ((*wc)[i][j] == ovs_tdm_tbl[0]) {w = i;break;}
            for (j=1; j<TD2P_OS_LLS_GRP_LEN; j++) if (ovs_tdm_tbl[j]==(*wc)[w][0] || ovs_tdm_tbl[j]==(*wc)[w][1] || ovs_tdm_tbl[j]==(*wc)[w][2] || ovs_tdm_tbl[j]==(*wc)[w][3]) {
                pivot[v] = j;
                v++;
            }
            for (j=0; j<3; j++) {
                if (pivot[j]!=-1) {
                    for (i=31; i>(pivot[j]+k-1); i--) ovs_tdm_tbl[i] = ovs_tdm_tbl[i-k];
                    for (i=pivot[j]; i<(pivot[j]+k); i++) ovs_tdm_tbl[i] = TD2P_NUM_EXT_PORTS;
                    for (i=0; i<8; i++) if (pivot[i] != -1) pivot[i]+=k;
                }
                else break;
            }
            break;
    }
    /* Populate PGW spacing table by calculating min spacing for each non-empty slots */
    for (j=0; j<TD2P_OS_LLS_GRP_LEN; j++) {
        w=33; v=33;

        if(ovs_tdm_tbl[j]!=TD2P_NUM_EXT_PORTS){
            for (a=0; a<TD2P_NUM_PHY_PM; a++) for (b=0; b<TD2P_NUM_PM_LNS; b++) if ((*wc)[a][b] == ovs_tdm_tbl[j]) c = a;

            for (i=j+1; i<TD2P_OS_LLS_GRP_LEN; i++) {
                if (ovs_tdm_tbl[i]==(*wc)[c][0] ||
                    ovs_tdm_tbl[i]==(*wc)[c][1] ||
                    ovs_tdm_tbl[i]==(*wc)[c][2] ||
                    ovs_tdm_tbl[i]==(*wc)[c][3]) {
                    w=i-j;
                    break;
                }
            }
            for (k=j-1; k>=0; k--) {
                if (ovs_tdm_tbl[k]==(*wc)[c][0] ||
                    ovs_tdm_tbl[k]==(*wc)[c][1] ||
                    ovs_tdm_tbl[k]==(*wc)[c][2] ||
                    ovs_tdm_tbl[k]==(*wc)[c][3]) {
                    v=j-k;
                    break;
                }
            }
        }

        ovs_spacing[j] = ((w <= v) ? w : v);
    }
    /* Regulate spacing values with TD2P_OS_LLS_GRP_LEN */
    for (k=0; k<TD2P_OS_LLS_GRP_LEN; k++) {
        if (ovs_spacing[k] == 33                ||
            ovs_tdm_tbl[k] == TD2P_NUM_EXT_PORTS){
            ovs_spacing[k] = TD2P_OS_LLS_GRP_LEN;
        }
    }

}


/**
@name: tdm_td2p_lls_scheduler
@param:

Populate a linked list for one quad by traversing ports lane by lane, and populate the PGW oversub calendar.
 */
int
tdm_td2p_lls_scheduler(struct ll_node *pgw_tdm, tdm_chip_legacy_t *td2p_chip, td2p_pgw_pntrs_t *pntrs_pkg, td2p_pgw_scheduler_vars_t *vars_pkg, int *pgw_tdm_tbl[TD2P_LR_LLS_LEN], int *ovs_tdm_tbl[TD2P_OS_LLS_GRP_LEN], int op_flags[2])
{
    int ll_nodes_appended=0, p, q, r, l1_tdm_len, max_tdm_len, pgw_tdm_idx_sub, xcount, sm_iter=0, wc_array[TD2P_NUM_PHY_PM][TD2P_NUM_PM_LNS],
        num_lr=0, num_lr_1=0, num_lr_10=0, num_lr_12=0, num_lr_20=0, num_lr_40=0, num_lr_100=0, num_lr_120=0, num_ovs=0, num_off=0;

    for (p=0; p<TD2P_NUM_PHY_PM; p++) {
        for (q=0; q<4; q++) {
            wc_array[p][q]=TD2P_NUM_EXT_PORTS;
        }
    }
    for (p=0; p<TD2P_NUM_PHY_PM; p++) {
        TDM_COPY(&(wc_array[p][0]), &((*td2p_chip).pmap[p][0]), (4*sizeof(int)));
    }
    for (p=0; p<TD2P_NUM_PHY_PM; p++) {
        TD2P_TOKEN_CHECK(wc_array[p][0]){
            if ((*td2p_chip).tdm_globals.speed[(wc_array[p][0])]<SPEED_100G && wc_array[p][0]!=TD2P_NUM_EXT_PORTS && wc_array[p][0]==wc_array[p][1] && wc_array[p][1]!=wc_array[p][2]) {
                r=wc_array[p][2];
                wc_array[p][2]=wc_array[p][1];
                wc_array[p][1]=r;
            }
            else if ((*td2p_chip).tdm_globals.speed[(wc_array[p][0])]<SPEED_100G && wc_array[p][2]!=TD2P_NUM_EXT_PORTS && wc_array[p][2]==wc_array[p][3] && wc_array[p][2]!=wc_array[p][1]) {
                r=wc_array[p][1];
                wc_array[p][1]=wc_array[p][2];
                wc_array[p][2]=r;
            }
        }
    }
    switch((*td2p_chip).tdm_globals.clk_freq) {
        case 415:
        case 416:
            pgw_tdm_idx_sub = 3;
            break;
        case 517:
        case 518:
            pgw_tdm_idx_sub = 4;
            break;
        case 608:
        case 609:
            if (((*vars_pkg).first_port == 32 || (*vars_pkg).first_port == 64) && ((*vars_pkg).last_port == 64 || (*vars_pkg).last_port == 96)) {pgw_tdm_idx_sub = 5;}
            else {pgw_tdm_idx_sub = 4;}
            break;
        case 760:
            pgw_tdm_idx_sub = 6;
            break;
        default:
            pgw_tdm_idx_sub = 8;
            break;
    }
    for (p = (*vars_pkg).first_port; p < (*vars_pkg).last_port; p++) {
        /* linerate */
        if ( (*td2p_chip).tdm_globals.port_rates_array[p] == PORT_STATE__LINERATE   ||
             (*td2p_chip).tdm_globals.port_rates_array[p] == PORT_STATE__LINERATE_HG){
            num_lr++;
            switch ((*td2p_chip).tdm_globals.speed[p+1]) {
                case SPEED_10G:
                case SPEED_10G_DUAL:
                case SPEED_10G_XAUI:
                case SPEED_11G:
                    if ( ((p+1)==TD2P_MGMT_PORT_0 || (p+1)==TD2P_MGMT_PORT_1 || (p+1)==TD2P_MGMT_PORT_2 || (p+1)==TD2P_MGMT_PORT_3) &&
                         (wc_array[TD2P_MGMT_TSC_ID][0]==TD2P_MGMT_TOKEN) ){
                        num_lr--;
                    }
                    else {
                        num_lr_10++;
                    }
                    break;
                case SPEED_12G:
                    num_lr_12++;
                    break;
                case SPEED_20G:
                case SPEED_21G:
                case SPEED_21G_DUAL:
                    num_lr_20++;
                    break;
                case SPEED_40G:
                case SPEED_42G:
                case SPEED_42G_HG2:
                    num_lr_40++;
                    break;
                case SPEED_100G:
                case SPEED_106G:
                    num_lr_100++;
                    break;
                case SPEED_120G:
                case SPEED_126G:
                    num_lr_120++;
                    break;
                default:
                    break;
            }
        }
        /* oversub */
        else if ((*td2p_chip).tdm_globals.port_rates_array[p] == PORT_STATE__OVERSUB   ||
                 (*td2p_chip).tdm_globals.port_rates_array[p] == PORT_STATE__OVERSUB_HG){
            num_ovs++;
        }
        /* disabled */
        else if ((*td2p_chip).tdm_globals.port_rates_array[p] == PORT_STATE__DISABLED || ( (p%4==0) && ((*td2p_chip).tdm_globals.port_rates_array[p]==PORT_STATE__COMBINE) ) ) {
            num_off++;
        }
    }
    max_tdm_len=(4*pgw_tdm_idx_sub);
    if (num_lr_12>0) {
        max_tdm_len*=2;
        l1_tdm_len = (num_lr_1+(num_lr_10*2)+(num_lr_12*3)+(num_lr_20*4)+(num_lr_40*8)+(num_lr_100*20)+(num_lr_120*24));
    }
    else {
        l1_tdm_len = (num_lr_1+num_lr_10+(num_lr_20*2)+(num_lr_40*4)+(num_lr_100*10)+(num_lr_120*12));
    }
    TDM_SML_BAR
    if ( (l1_tdm_len>max_tdm_len) && (l1_tdm_len>32) ) {
        TDM_ERROR0("Bandwidth overloaded\n");
        return 0;
    }
    TDM_PRINT1("Base oversubscription token pool is %0d\n", pgw_tdm_idx_sub);
    TDM_SML_BAR
    TDM_PRINT1("1G line rate ports in this quadrant: %0d\n", num_lr_1);
    TDM_PRINT1("10G line rate ports in this quadrant: %0d\n", num_lr_10);
    TDM_PRINT1("12G line rate ports in this quadrant: %0d\n", num_lr_12);
    TDM_PRINT1("20G line rate ports in this quadrant: %0d\n", num_lr_20);
    TDM_PRINT1("40G line rate ports in this quadrant: %0d\n", num_lr_40);
    TDM_PRINT1("100G gestalt line rate ports in this quadrant: %0d\n", num_lr_100);
    TDM_PRINT1("120G gestalt line rate ports in this quadrant: %0d\n", num_lr_120);
    TDM_PRINT1("Oversubscribed ports in this quadrant: %0d\n", num_ovs);
    TDM_PRINT1("Disabled subports in this quadrant: %0d\n", num_off);
    TDM_PRINT2("COVERAGE FROM PHYSICAL PORTS %0d to %0d\n", (*vars_pkg).first_port, (*vars_pkg).last_port);

    if ((l1_tdm_len>max_tdm_len) && (num_ovs == 0)) {
        TDM_PRINT0("Applying obligate oversubscription correction\n");
        for (xcount=0; xcount<max_tdm_len; xcount++) {
            while (((*pntrs_pkg).cur_idx < (*vars_pkg).cur_idx_max) &&
                   ((wc_array[(*pntrs_pkg).cur_idx][(*vars_pkg).subport] == TD2P_NUM_EXT_PORTS) ||
                    (wc_array[(*pntrs_pkg).cur_idx][(*vars_pkg).subport] == TD2P_MGMT_TOKEN)     ) ){
                ((*pntrs_pkg).cur_idx)++;
            }
            if ((*pntrs_pkg).cur_idx >= (*vars_pkg).cur_idx_max) {
                break;
            }
            if ( tdm_td2p_ll_len(pgw_tdm) < max_tdm_len ) {
                tdm_td2p_ll_append(pgw_tdm,TD2P_OVSB_TOKEN,&ll_nodes_appended);
                TDM_PRINT2("Pipe %0d: PGW TDM calendar element #0%0d, content is OVSB token\n",(((*vars_pkg).cur_idx_max/8)-1),(((*vars_pkg).subport*pgw_tdm_idx_sub)+ll_nodes_appended));
            }
            (*ovs_tdm_tbl)[(*pntrs_pkg).ovs_tdm_idx] = wc_array[(*pntrs_pkg).cur_idx][(*vars_pkg).subport];
            TDM_PRINT3("Pipe %0d: Oversub subcalendar index %0d is %0d\n",(((*vars_pkg).cur_idx_max/8)-1),(*pntrs_pkg).ovs_tdm_idx,(*ovs_tdm_tbl)[(*pntrs_pkg).ovs_tdm_idx]);
            ((*pntrs_pkg).ovs_tdm_idx)++;
            ((*pntrs_pkg).cur_idx)++;
        }
        TDM_WARN3("Pipe %0d: PGW TDM calendar, Line-rate bandwidth overflow (%d,%d). \n", (((*vars_pkg).cur_idx_max/8)-1), l1_tdm_len, max_tdm_len);
    }
    else {
        while ( (sm_iter++) < TD2P_NUM_EXT_PORTS) {
            while ( ((*pntrs_pkg).cur_idx < (*vars_pkg).cur_idx_max) &&
                    ((wc_array[(*pntrs_pkg).cur_idx][(*vars_pkg).subport] == TD2P_MGMT_TOKEN)    ||
                     (wc_array[(*pntrs_pkg).cur_idx][(*vars_pkg).subport] == TD2P_NUM_EXT_PORTS) ||
                     ((*td2p_chip).tdm_globals.port_rates_array[(wc_array[(*pntrs_pkg).cur_idx][(*vars_pkg).subport])-1]==PORT_STATE__MANAGEMENT) ) ){
                ((*pntrs_pkg).cur_idx)++;
            }
            if ( ( ((*pntrs_pkg).cur_idx)>=((*vars_pkg).cur_idx_max)) ) {
                break;
            }
            /* Print selected active port info */
            TDM_SML_BAR
            TDM_PRINT2("Polling current index %0d subport %0d: [",(*pntrs_pkg).cur_idx,(*vars_pkg).subport);
            for (p=0; p<4; p++) {
                if (p==(*vars_pkg).subport) {
                    TDM_PRINT1(" >%0d< ",wc_array[(*pntrs_pkg).cur_idx][p]);
                }
                else {
                    TDM_PRINT1(" %0d ",wc_array[(*pntrs_pkg).cur_idx][p]);
                }
            }
            TDM_PRINT0("]\n");
            TDM_PRINT3("[ Port=%0d, Speed=%0dG, State=%0d ]\n", wc_array[(*pntrs_pkg).cur_idx][(*vars_pkg).subport],(*td2p_chip).tdm_globals.speed[wc_array[(*pntrs_pkg).cur_idx][(*vars_pkg).subport]]/1000,(*td2p_chip).tdm_globals.port_rates_array[(wc_array[(*pntrs_pkg).cur_idx][(*vars_pkg).subport])-1]);
            switch((*td2p_chip).tdm_globals.port_rates_array[(wc_array[(*pntrs_pkg).cur_idx][(*vars_pkg).subport])-1]) {
                case PORT_STATE__DISABLED:
                    TDM_PRINT0("The current port is DISABLED\n");
                    if (num_lr > 0 && num_ovs > 0) {
                            tdm_td2p_ll_append(pgw_tdm,TD2P_OVSB_TOKEN,&ll_nodes_appended);
                            TDM_PRINT3("Pipe %0d: PGW TDM calendar element #0%0d, content is %0d\n",(((*vars_pkg).cur_idx_max/8)-1),(((*vars_pkg).subport*pgw_tdm_idx_sub)+ll_nodes_appended),tdm_td2p_ll_get(pgw_tdm,(tdm_td2p_ll_len(pgw_tdm)-1)));
                    }
                    ((*pntrs_pkg).cur_idx)++;
                    break;
                case PORT_STATE__LINERATE:
                case PORT_STATE__LINERATE_HG:
                    TDM_PRINT0("The current port is LINE RATE\n");
                    if (num_ovs==0) {
                        while ( (((*pntrs_pkg).tdm_stk_idx)>0) && (tdm_td2p_ll_get(pgw_tdm,(tdm_td2p_ll_len(pgw_tdm)-1))!=(*vars_pkg).swap_array[(*pntrs_pkg).tdm_stk_idx]) ) {
                            tdm_td2p_ll_append(pgw_tdm,(*vars_pkg).swap_array[(*pntrs_pkg).tdm_stk_idx],&ll_nodes_appended);
                            TDM_PRINT4("Pipe %0d: Dequeue from stack, pointer index %0d, to pgw tdm tbl element #0%0d, content is %0d\n",(((*vars_pkg).cur_idx_max/8)-1),(*pntrs_pkg).tdm_stk_idx,(((*vars_pkg).subport*pgw_tdm_idx_sub)+ll_nodes_appended),(*vars_pkg).swap_array[(*pntrs_pkg).tdm_stk_idx]);
                            ((*pntrs_pkg).tdm_stk_idx)--;
                            TDM_PRINT1("Swap stack pointer at %0d\n",(*pntrs_pkg).tdm_stk_idx);
                        }
                        if ( (wc_array[(*pntrs_pkg).cur_idx][(*vars_pkg).subport]!=tdm_td2p_ll_get(pgw_tdm,(tdm_td2p_ll_len(pgw_tdm)-1)) ) || (ll_nodes_appended==0) ) {
                            tdm_td2p_ll_append(pgw_tdm,wc_array[(*pntrs_pkg).cur_idx][(*vars_pkg).subport],&ll_nodes_appended);
                            TDM_PRINT3("Pipe %0d: PGW TDM calendar element #0%0d, content is %0d\n",(((*vars_pkg).cur_idx_max/8)-1),(((*vars_pkg).subport*pgw_tdm_idx_sub)+ll_nodes_appended),tdm_td2p_ll_get(pgw_tdm,(tdm_td2p_ll_len(pgw_tdm)-1)));
                            ((*pntrs_pkg).cur_idx)++;
                        }
                        else {
                            ((*pntrs_pkg).tdm_stk_idx)++;
                            (*vars_pkg).swap_array[(*pntrs_pkg).tdm_stk_idx] = wc_array[(*pntrs_pkg).cur_idx][(*vars_pkg).subport];
                            TDM_PRINT3("Pipe %0d: pushed port %0d at stack index %0d\n",(((*vars_pkg).cur_idx_max/8)-1),wc_array[(*pntrs_pkg).cur_idx][(*vars_pkg).subport],(*pntrs_pkg).tdm_stk_idx);
                            ((*pntrs_pkg).cur_idx)++;
                        }
                    }
                    else {
                        if ( (((*pntrs_pkg).tdm_stk_idx)>0) &&
                             (tdm_td2p_ll_get(pgw_tdm,(tdm_td2p_ll_len(pgw_tdm)-2))!=(*vars_pkg).swap_array[(*pntrs_pkg).tdm_stk_idx]) &&
                             (tdm_td2p_ll_get(pgw_tdm,(tdm_td2p_ll_len(pgw_tdm)-1))==TD2P_OVSB_TOKEN) ) {
                            tdm_td2p_ll_append(pgw_tdm,(*vars_pkg).swap_array[(*pntrs_pkg).tdm_stk_idx],&ll_nodes_appended);
                            TDM_PRINT4("Pipe %0d: Dequeue from stack, pointer index %0d, to pgw tdm tbl element #0%0d, content is %0d\n",(((*vars_pkg).cur_idx_max/8)-1),(*pntrs_pkg).tdm_stk_idx,(((*vars_pkg).subport*pgw_tdm_idx_sub)+ll_nodes_appended),(*vars_pkg).swap_array[(*pntrs_pkg).tdm_stk_idx]);
                            ((*pntrs_pkg).tdm_stk_idx)--;
                            TDM_PRINT1("Swap stack pointer at %0d\n",(*pntrs_pkg).tdm_stk_idx);
                        }
                        while ( (((*pntrs_pkg).tdm_stk_idx)>0) &&
                                (tdm_td2p_ll_get(pgw_tdm,(tdm_td2p_ll_len(pgw_tdm)-1))!=(*vars_pkg).swap_array[(*pntrs_pkg).tdm_stk_idx]) &&
                                (tdm_td2p_ll_get(pgw_tdm,(tdm_td2p_ll_len(pgw_tdm)-1))!=TD2P_OVSB_TOKEN) ) {
                            tdm_td2p_ll_append(pgw_tdm,(*vars_pkg).swap_array[(*pntrs_pkg).tdm_stk_idx],&ll_nodes_appended);
                            TDM_PRINT4("Pipe %0d: Dequeue from stack, pointer index %0d, to pgw tdm tbl element #0%0d, content is %0d\n",(((*vars_pkg).cur_idx_max/8)-1),(*pntrs_pkg).tdm_stk_idx,(((*vars_pkg).subport*pgw_tdm_idx_sub)+ll_nodes_appended),(*vars_pkg).swap_array[(*pntrs_pkg).tdm_stk_idx]);
                            ((*pntrs_pkg).tdm_stk_idx)--;
                            TDM_PRINT1("Swap stack pointer at %0d\n",(*pntrs_pkg).tdm_stk_idx);
                        }
                        if ( ((wc_array[(*pntrs_pkg).cur_idx][(*vars_pkg).subport]!=tdm_td2p_ll_get(pgw_tdm,(tdm_td2p_ll_len(pgw_tdm)-1))) || (ll_nodes_appended==0)) ) {
                            tdm_td2p_ll_append(pgw_tdm,wc_array[(*pntrs_pkg).cur_idx][(*vars_pkg).subport],&ll_nodes_appended);
                            TDM_PRINT3("Pipe %0d: PGW TDM calendar element #0%0d, content is %0d\n",(((*vars_pkg).cur_idx_max/8)-1),(((*vars_pkg).subport*pgw_tdm_idx_sub)+ll_nodes_appended),tdm_td2p_ll_get(pgw_tdm,(tdm_td2p_ll_len(pgw_tdm)-1)));
                            ((*pntrs_pkg).cur_idx)++;
                        }
                        else {
                            ((*pntrs_pkg).tdm_stk_idx)++;
                            (*vars_pkg).swap_array[(*pntrs_pkg).tdm_stk_idx] = wc_array[(*pntrs_pkg).cur_idx][(*vars_pkg).subport];
                            TDM_PRINT3("Pipe %0d: pushed port %0d at stack index %0d\n",(((*vars_pkg).cur_idx_max/8)-1),wc_array[(*pntrs_pkg).cur_idx][(*vars_pkg).subport],(*pntrs_pkg).tdm_stk_idx);
                            ((*pntrs_pkg).cur_idx)++;
                        }
                    }
                    break;
                case PORT_STATE__OVERSUB:
                case PORT_STATE__OVERSUB_HG:
                    TDM_PRINT0("The current port is OVERSUBSCRIBED\n");
                    TDM_PRINT1("The oversub subcalendar index is %0d\n", (*pntrs_pkg).ovs_tdm_idx);
                    (*ovs_tdm_tbl)[(*pntrs_pkg).ovs_tdm_idx] = wc_array[(*pntrs_pkg).cur_idx][(*vars_pkg).subport];
                    ((*pntrs_pkg).ovs_tdm_idx)++;
                    if (((*vars_pkg).subport!=0) &&
                        ((*td2p_chip).tdm_globals.port_rates_array[(wc_array[(*pntrs_pkg).cur_idx][(*vars_pkg).subport-1])-1]==PORT_STATE__LINERATE ||
                         (*td2p_chip).tdm_globals.port_rates_array[(wc_array[(*pntrs_pkg).cur_idx][(*vars_pkg).subport-1])-1]==PORT_STATE__LINERATE_HG) ){
                            tdm_td2p_ll_append(pgw_tdm,TD2P_OVSB_TOKEN,&ll_nodes_appended);
                            TDM_PRINT3("Pipe %0d: PGW TDM calendar element #0%0d, content is %0d\n",(((*vars_pkg).cur_idx_max/8)-1),(((*vars_pkg).subport*pgw_tdm_idx_sub)+ll_nodes_appended),tdm_td2p_ll_get(pgw_tdm,(tdm_td2p_ll_len(pgw_tdm)-1)));
                    }
                    ((*pntrs_pkg).cur_idx)++;
                    break;
                default:
                    break;
            }
        }
        while ((*pntrs_pkg).tdm_stk_idx>0) {
            tdm_td2p_ll_append(pgw_tdm,(*vars_pkg).swap_array[(*pntrs_pkg).tdm_stk_idx], &ll_nodes_appended);
            TDM_PRINT4("Pipe %0d: Dequeue from stack, pointer index %0d, to pgw tdm tbl element #0%0d, content is %0d\n",(((*vars_pkg).cur_idx_max/8)-1),(*pntrs_pkg).tdm_stk_idx,(((*vars_pkg).subport*pgw_tdm_idx_sub)+ll_nodes_appended),(*vars_pkg).swap_array[(*pntrs_pkg).tdm_stk_idx]);
            ((*pntrs_pkg).tdm_stk_idx)--;
            TDM_PRINT1("Swap stack pointer at %0d\n",(*pntrs_pkg).tdm_stk_idx);
        }
        if ( (ll_nodes_appended>0) ||
             (ll_nodes_appended==0 && ((*pntrs_pkg).ovs_tdm_idx)>0) ){
            while ( ll_nodes_appended<pgw_tdm_idx_sub ) {
                TDM_SML_BAR
                TDM_PRINT1("Extending current cadence at index %0d\n",tdm_td2p_ll_len(pgw_tdm));
                tdm_td2p_ll_append(pgw_tdm,TD2P_OVSB_TOKEN,&ll_nodes_appended);
                TDM_PRINT3("Pipe %0d: PGW TDM calendar element #0%0d, content is %0d\n",(((*vars_pkg).cur_idx_max/8)-1),(((*vars_pkg).subport*pgw_tdm_idx_sub)+ll_nodes_appended),tdm_td2p_ll_get(pgw_tdm,(tdm_td2p_ll_len(pgw_tdm)-1)));
            }
        }
    }

    TDM_PRINT0("\n\tCalendar dump:\n");
    tdm_td2p_ll_print(pgw_tdm);
    TDM_SML_BAR
    TDM_SML_BAR
    TDM_PRINT1("Current cycle: The PGW TDM calendar index is %0d\n",tdm_td2p_ll_len(pgw_tdm));
    TDM_PRINT1("Current cycle: %0d ll_nodes appended to PGW TDM linked list\n",ll_nodes_appended);
    TDM_PRINT2("Current cycle: The current index is %0d at subport %0d\n",(*pntrs_pkg).cur_idx,(*vars_pkg).subport);
    TDM_SML_BAR
    TDM_SML_BAR

    return ll_nodes_appended;
}


/**
@name: tdm_td2p_vmap_alloc
@param:

Populate original vmap containing all linerate ports with matched speed rates accordingly
 */
int
tdm_td2p_vmap_alloc( tdm_mod_t *_tdm )
{
    int j, v, w;
    unsigned short lr1[TDM_AUX_SIZE], lr10[TDM_AUX_SIZE], lr20[TDM_AUX_SIZE], lr40[TDM_AUX_SIZE], lr100[TDM_AUX_SIZE], lr120[TDM_AUX_SIZE];

    for (j=0; j<(TDM_AUX_SIZE); j++) {
        lr1[j]  = TD2P_NUM_EXT_PORTS;
        lr10[j] = TD2P_NUM_EXT_PORTS;
        lr20[j] = TD2P_NUM_EXT_PORTS;
        lr40[j] = TD2P_NUM_EXT_PORTS;
        lr120[j]= TD2P_NUM_EXT_PORTS;
        lr100[j]= TD2P_NUM_EXT_PORTS;

        _tdm->_core_data.vars_pkg.m_tdm_vmap_alloc.os1[j]  = TD2P_NUM_EXT_PORTS;
        _tdm->_core_data.vars_pkg.m_tdm_vmap_alloc.os10[j] = TD2P_NUM_EXT_PORTS;
        _tdm->_core_data.vars_pkg.m_tdm_vmap_alloc.os20[j] = TD2P_NUM_EXT_PORTS;
        _tdm->_core_data.vars_pkg.m_tdm_vmap_alloc.os25[j] = TD2P_NUM_EXT_PORTS;
        _tdm->_core_data.vars_pkg.m_tdm_vmap_alloc.os40[j] = TD2P_NUM_EXT_PORTS;
        _tdm->_core_data.vars_pkg.m_tdm_vmap_alloc.os50[j] = TD2P_NUM_EXT_PORTS;
        _tdm->_core_data.vars_pkg.m_tdm_vmap_alloc.os100[j]= TD2P_NUM_EXT_PORTS;
        _tdm->_core_data.vars_pkg.m_tdm_vmap_alloc.os120[j]= TD2P_NUM_EXT_PORTS;
    }

    for (j=0; j<TDM_AUX_SIZE; j++) {
        if (_tdm->_core_data.vars_pkg.lr_buffer[j] != TD2P_NUM_EXT_PORTS){
            if (_tdm->_core_data.vars_pkg.lr_buffer[j] <= TD2P_NUM_PHY_PORTS){
                if (_tdm->_chip_data.soc_pkg.state[_tdm->_core_data.vars_pkg.lr_buffer[j]-1] == PORT_STATE__LINERATE   ||
                    _tdm->_chip_data.soc_pkg.state[_tdm->_core_data.vars_pkg.lr_buffer[j]-1] == PORT_STATE__LINERATE_HG){
                    switch (_tdm->_chip_data.soc_pkg.speed[_tdm->_core_data.vars_pkg.lr_buffer[j]]) {
                        case SPEED_1G:
                        case SPEED_2p5G:
                            tdm_core_prealloc(lr1, _tdm->_core_data.vars_pkg.lr_buffer, &(_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y8), &(_tdm->_core_data.vars_pkg.lr_1), j);
                            break;
                        case SPEED_10G:
                        case SPEED_10G_DUAL:
                        case SPEED_10G_XAUI:
                        case SPEED_11G:
                            tdm_core_prealloc(lr10, _tdm->_core_data.vars_pkg.lr_buffer, &(_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y1), &(_tdm->_core_data.vars_pkg.lr_10), j);
                            break;
                        case SPEED_20G:
                        case SPEED_21G:
                        case SPEED_21G_DUAL:
                            tdm_core_prealloc(lr20, _tdm->_core_data.vars_pkg.lr_buffer, &(_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y2), &(_tdm->_core_data.vars_pkg.lr_20), j);
                            break;
                        case SPEED_40G:
                        case SPEED_42G:
                        case SPEED_42G_HG2:
                            tdm_core_prealloc(lr40, _tdm->_core_data.vars_pkg.lr_buffer, &(_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y3), &(_tdm->_core_data.vars_pkg.lr_40), j);
                            break;
                        case SPEED_100G:
                        case SPEED_106G:
                            tdm_core_prealloc(lr100, _tdm->_core_data.vars_pkg.lr_buffer, &(_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y4), &(_tdm->_core_data.vars_pkg.lr_100), j);
                            break;
                        case SPEED_120G:
                        case SPEED_126G:
                            tdm_core_prealloc(lr120, _tdm->_core_data.vars_pkg.lr_buffer, &(_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y7), &(_tdm->_core_data.vars_pkg.lr_120), j);
                            break;
                        default:
                            TDM_ERROR2("Speed %0d port %0d was skipped in linerate preallocation\n",_tdm->_chip_data.soc_pkg.speed[_tdm->_core_data.vars_pkg.lr_buffer[j]],_tdm->_core_data.vars_pkg.lr_buffer[j]);
                    }
                }
                else{
                    TDM_ERROR2("Failed to presort linerate port %0d with state mapping %0d\n", _tdm->_core_data.vars_pkg.lr_buffer[j], _tdm->_chip_data.soc_pkg.state[_tdm->_core_data.vars_pkg.lr_buffer[j]-1]);
                }
            }
            else{
                TDM_ERROR1("Unrecognized linerate port %0d in linerate preallocation\n", _tdm->_core_data.vars_pkg.lr_buffer[j]);
            }
        }
        if (_tdm->_core_data.vars_pkg.os_buffer[j] != TD2P_NUM_EXT_PORTS){
            if(_tdm->_core_data.vars_pkg.os_buffer[j] <= TD2P_NUM_PHY_PORTS){
                if (_tdm->_chip_data.soc_pkg.state[_tdm->_core_data.vars_pkg.os_buffer[j]-1] == PORT_STATE__OVERSUB   ||
                    _tdm->_chip_data.soc_pkg.state[_tdm->_core_data.vars_pkg.os_buffer[j]-1] == PORT_STATE__OVERSUB_HG){
                    switch (_tdm->_chip_data.soc_pkg.speed[_tdm->_core_data.vars_pkg.os_buffer[j]]) {
                        case SPEED_1G:
                        case SPEED_2p5G:
                            tdm_core_prealloc(_tdm->_core_data.vars_pkg.m_tdm_vmap_alloc.os1, _tdm->_core_data.vars_pkg.os_buffer, &(_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.z8), &(_tdm->_core_data.vars_pkg.os_1), j);
                            break;
                        case SPEED_10G:
                        case SPEED_10G_DUAL:
                        case SPEED_10G_XAUI:
                        case SPEED_11G:
                            tdm_core_prealloc(_tdm->_core_data.vars_pkg.m_tdm_vmap_alloc.os10, _tdm->_core_data.vars_pkg.os_buffer, &(_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.z1), &(_tdm->_core_data.vars_pkg.os_10), j);
                            break;
                        case SPEED_20G:
                        case SPEED_21G:
                        case SPEED_21G_DUAL:
                            tdm_core_prealloc(_tdm->_core_data.vars_pkg.m_tdm_vmap_alloc.os20, _tdm->_core_data.vars_pkg.os_buffer, &(_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.z2), &(_tdm->_core_data.vars_pkg.os_20), j);
                            break;
                        case SPEED_40G:
                        case SPEED_42G:
                        case SPEED_42G_HG2:
                            tdm_core_prealloc(_tdm->_core_data.vars_pkg.m_tdm_vmap_alloc.os40, _tdm->_core_data.vars_pkg.os_buffer, &(_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.z3), &(_tdm->_core_data.vars_pkg.os_40), j);
                            break;
                        case SPEED_100G:
                        case SPEED_106G:
                            tdm_core_prealloc(_tdm->_core_data.vars_pkg.m_tdm_vmap_alloc.os100, _tdm->_core_data.vars_pkg.os_buffer, &(_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.z4), &(_tdm->_core_data.vars_pkg.os_100), j);
                            break;
                        case SPEED_120G:
                        case SPEED_126G:
                            tdm_core_prealloc(_tdm->_core_data.vars_pkg.m_tdm_vmap_alloc.os120, _tdm->_core_data.vars_pkg.os_buffer, &(_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.z7), &(_tdm->_core_data.vars_pkg.os_120), j);
                            break;
                        default:
                            TDM_ERROR2("Speed %0d port %0d was skipped in oversub preallocation\n",_tdm->_chip_data.soc_pkg.speed[_tdm->_core_data.vars_pkg.os_buffer[j]],_tdm->_core_data.vars_pkg.os_buffer[j]);
                    }
                }
                else{
                    TDM_ERROR2("Failed to presort oversub port %0d with state mapping %0d\n", _tdm->_core_data.vars_pkg.os_buffer[j], _tdm->_chip_data.soc_pkg.state[_tdm->_core_data.vars_pkg.os_buffer[j]-1]);
                }
            }
            else{
                TDM_ERROR1("Unrecognized oversub port %0d in oversub preallocation\n", _tdm->_core_data.vars_pkg.lr_buffer[j]);
            }
        }
    }

    _tdm->_core_data.vars_pkg.os_enable = (_tdm->_core_data.vars_pkg.os_1||_tdm->_core_data.vars_pkg.os_10||_tdm->_core_data.vars_pkg.os_20||_tdm->_core_data.vars_pkg.os_25||_tdm->_core_data.vars_pkg.os_40||_tdm->_core_data.vars_pkg.os_50||_tdm->_core_data.vars_pkg.os_100||_tdm->_core_data.vars_pkg.os_120);
    _tdm->_core_data.vars_pkg.lr_enable = (_tdm->_core_data.vars_pkg.lr_1||_tdm->_core_data.vars_pkg.lr_10||_tdm->_core_data.vars_pkg.lr_20||_tdm->_core_data.vars_pkg.lr_25||_tdm->_core_data.vars_pkg.lr_40||_tdm->_core_data.vars_pkg.lr_50||_tdm->_core_data.vars_pkg.lr_100||_tdm->_core_data.vars_pkg.lr_120);
    _tdm->_core_data.vars_pkg.m_tdm_vmap_alloc.num_lr = (_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y8+_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y1+_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y2+_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y3+_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y4+_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y5+_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y6+_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y7);
    _tdm->_core_data.vars_pkg.m_tdm_vmap_alloc.num_os = (_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.z8+_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.z1+_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.z2+_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.z3+_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.z4+_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.z5+_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.z6+_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.z7);
    _tdm->_core_data.vars_pkg.m_tdm_vmap_alloc.num_100g = _tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y4;
    _tdm->_core_data.vars_pkg.m_tdm_vmap_alloc.num_40g = _tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y3;
    /* 4x106HG+3x40G uses a unique TDM in Tomahawk */
    if (_tdm->_core_data.vars_pkg.m_tdm_vmap_alloc.num_40g==3 && _tdm->_core_data.vars_pkg.m_tdm_vmap_alloc.num_100g==4 && _tdm->_chip_data.soc_pkg.clk_freq==850) {
        _tdm->_chip_data.soc_pkg.tvec_size=9;
        _tdm->_chip_data.soc_pkg.lr_idx_limit=201;
        _tdm->_core_data.vars_pkg.HG4X106G_3X40G=BOOL_TRUE;
    }
    /* To guarantee oversub smoothness in nonscaling 100G+40G case, algorithm must sort with a 120G port vector (bw will be reclaimed) */
    if ( ( (_tdm->_core_data.vars_pkg.m_tdm_vmap_alloc.num_100g==1) && (_tdm->_core_data.vars_pkg.lr_40) && (!_tdm->_core_data.vars_pkg.lr_120) && (_tdm->_core_data.vars_pkg.os_enable) ) ) {
        _tdm->_core_data.vars_pkg.HG1X106G_xX40G_OVERSUB=BOOL_TRUE;
    }
    /* To sort 120G and 100G vectors together, load a special 100G vector */
    if ( (_tdm->_core_data.vars_pkg.lr_100 && _tdm->_core_data.vars_pkg.lr_120) ) {
        _tdm->_core_data.vars_pkg.HGXx120G_Xx100G=BOOL_TRUE;
    }
    TDM_PRINT9("(1G - %0d) (10G - %0d) (20G - %0d) (25G - %0d) (40G - %0d) (50G - %0d) (100G - %0d) (120G - %0d) (Number of Line Rate Types - %0d)\n", _tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y8, _tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y1, _tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y2, _tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y6, _tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y3, _tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y5, _tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y4, _tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y7, (_tdm->_core_data.vars_pkg.lr_1+_tdm->_core_data.vars_pkg.lr_10+_tdm->_core_data.vars_pkg.lr_20+_tdm->_core_data.vars_pkg.lr_25+_tdm->_core_data.vars_pkg.lr_40+_tdm->_core_data.vars_pkg.lr_50+_tdm->_core_data.vars_pkg.lr_100+_tdm->_core_data.vars_pkg.lr_120));
    TDM_BIG_BAR
    /* 120G */
    if (_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y7 > 0) {
        if (tdm_core_postalloc(_tdm->_core_data.vmap, _tdm->_chip_data.soc_pkg.clk_freq, 120, &_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.yy, &(_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y7), _tdm->_chip_data.soc_pkg.lr_idx_limit, lr120, TOKEN_120G, "120", TD2P_NUM_EXT_PORTS)!=PASS) {
            TDM_ERROR0("Critical error in 120G time vector preallocation\n");
            return 0;
        }
    }
    /* 100G */
    if(_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y4 > 0){
        if (_tdm->_core_data.vars_pkg.HG4X106G_3X40G) {
            if (tdm_core_postalloc(_tdm->_core_data.vmap, _tdm->_chip_data.soc_pkg.clk_freq, 107, &_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.yy, &(_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y4), _tdm->_chip_data.soc_pkg.lr_idx_limit, lr100, TOKEN_100G, "100", TD2P_NUM_EXT_PORTS)!=PASS) {
                TDM_ERROR0("Critical error in 100G time vector preallocation\n");
                return 0;
            }
        }
        else if (_tdm->_core_data.vars_pkg.HG1X106G_xX40G_OVERSUB) {
            if (tdm_core_postalloc(_tdm->_core_data.vmap, _tdm->_chip_data.soc_pkg.clk_freq, 108, &_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.yy, &(_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y4), _tdm->_chip_data.soc_pkg.lr_idx_limit, lr100, TOKEN_100G, "100", TD2P_NUM_EXT_PORTS)!=PASS) {
                TDM_ERROR0("Critical error in 100G time vector preallocation\n");
                return 0;
            }
        }
        else if (_tdm->_core_data.vars_pkg.HGXx120G_Xx100G) {
            if (tdm_core_postalloc(_tdm->_core_data.vmap, _tdm->_chip_data.soc_pkg.clk_freq, 109, &_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.yy, &(_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y4), _tdm->_chip_data.soc_pkg.lr_idx_limit, lr100, TOKEN_100G, "100", TD2P_NUM_EXT_PORTS)!=PASS) {
                TDM_ERROR0("Critical error in 100G time vector preallocation\n");
                return 0;
            }
        }
        else{
            if (tdm_td2p_check_pipe_ethernet(lr100[_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y4],_tdm) && _tdm->_chip_data.soc_pkg.soc_vars.td2p.higig_mgmt==BOOL_FALSE) {
                if (tdm_core_postalloc(_tdm->_core_data.vmap, _tdm->_chip_data.soc_pkg.clk_freq, 100, &_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.yy, &(_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y4), _tdm->_chip_data.soc_pkg.lr_idx_limit, lr100, TOKEN_100G, "100", TD2P_NUM_EXT_PORTS)!=PASS) {
                    TDM_ERROR0("Critical error in 100G time vector preallocation\n");
                    return 0;
                }
            }
            else {
                if (tdm_core_postalloc(_tdm->_core_data.vmap, _tdm->_chip_data.soc_pkg.clk_freq, 106, &_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.yy, &(_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y4), _tdm->_chip_data.soc_pkg.lr_idx_limit, lr100, TOKEN_100G, "100", TD2P_NUM_EXT_PORTS)!=PASS) {
                    TDM_ERROR0("Critical error in 100G time vector preallocation\n");
                    return 0;
                }
            }
        }
    }
    /* 40G */
    if (_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y3 > 0) {
        for (v=_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y3; v>0; v--) {
            if (tdm_td2p_legacy_which_tsc(lr40[v+1],_tdm->_chip_data.soc_pkg.pmap)==tdm_td2p_legacy_which_tsc(lr40[v],_tdm->_chip_data.soc_pkg.pmap) ||
                tdm_td2p_legacy_which_tsc(lr40[v-1],_tdm->_chip_data.soc_pkg.pmap)==tdm_td2p_legacy_which_tsc(lr40[v],_tdm->_chip_data.soc_pkg.pmap)) {
                _tdm->_core_data.vars_pkg.m_tdm_vmap_alloc.prox_swap=lr40[v];
                for (w=v; w>0; w--) {
                    lr40[w]=lr40[w-1];
                }
                lr40[1]=_tdm->_core_data.vars_pkg.m_tdm_vmap_alloc.prox_swap;
            }
        }
        for (v=_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y3; v>0; v--) {
            _tdm->_core_data.vars_pkg.m_tdm_vmap_alloc.tri_chk = tdm_td2p_legacy_which_tsc(lr40[v],_tdm->_chip_data.soc_pkg.pmap);
            for (w=0; w<4; w++) {
                if (_tdm->_chip_data.soc_pkg.speed[_tdm->_chip_data.soc_pkg.pmap[_tdm->_core_data.vars_pkg.m_tdm_vmap_alloc.tri_chk][w]]!=SPEED_40G && _tdm->_chip_data.soc_pkg.state[_tdm->_chip_data.soc_pkg.pmap[_tdm->_core_data.vars_pkg.m_tdm_vmap_alloc.tri_chk][w]-1]!=PORT_STATE__DISABLED) {
                    _tdm->_core_data.vars_pkg.m_tdm_vmap_alloc.tri_en_40=BOOL_TRUE;
                    TDM_PRINT0("40G triport detected\n");
                    break;
                }
            }
        }
        if (_tdm->_core_data.vars_pkg.HG4X106G_3X40G==BOOL_TRUE) {
            if (_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y3==3) {
                if (tdm_core_postalloc(_tdm->_core_data.vmap, _tdm->_chip_data.soc_pkg.clk_freq, 43, &_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.yy, &(_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y3), _tdm->_chip_data.soc_pkg.lr_idx_limit, lr40, TOKEN_40G, "40", TD2P_NUM_EXT_PORTS)!=PASS) {
                    TDM_ERROR0("Critical error in 40G time vector preallocation\n");
                    return 0;
                }
            }
            else if (_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y3==2) {
                if (tdm_core_postalloc(_tdm->_core_data.vmap, _tdm->_chip_data.soc_pkg.clk_freq, 44, &_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.yy, &(_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y3), _tdm->_chip_data.soc_pkg.lr_idx_limit, lr40, TOKEN_40G, "40", TD2P_NUM_EXT_PORTS)!=PASS) {
                    TDM_ERROR0("Critical error in 40G time vector preallocation\n");
                    return 0;
                }
            }
            else if (_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y3==1) {
                if (tdm_core_postalloc(_tdm->_core_data.vmap, _tdm->_chip_data.soc_pkg.clk_freq, 45, &_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.yy, &(_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y3), _tdm->_chip_data.soc_pkg.lr_idx_limit, lr40, TOKEN_40G, "40", TD2P_NUM_EXT_PORTS)!=PASS) {
                    TDM_ERROR0("Critical error in 40G time vector preallocation\n");
                    return 0;
                }
            }
        }
        else {
            if (tdm_td2p_check_pipe_ethernet(lr40[_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y3],_tdm) && _tdm->_chip_data.soc_pkg.soc_vars.td2p.higig_mgmt==BOOL_FALSE) {
                if (tdm_core_postalloc(_tdm->_core_data.vmap, _tdm->_chip_data.soc_pkg.clk_freq, 40, &_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.yy, &(_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y3), _tdm->_chip_data.soc_pkg.lr_idx_limit, lr40, TOKEN_40G, "40", TD2P_NUM_EXT_PORTS)!=PASS) {
                    TDM_ERROR0("Critical error in 40G time vector preallocation\n");
                    return 0;
                }
            }
            else {
                if (tdm_core_postalloc(_tdm->_core_data.vmap, _tdm->_chip_data.soc_pkg.clk_freq, 42, &_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.yy, &(_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y3), _tdm->_chip_data.soc_pkg.lr_idx_limit, lr40, TOKEN_40G, "40", TD2P_NUM_EXT_PORTS)!=PASS) {
                    TDM_ERROR0("Critical error in 40G time vector preallocation\n");
                    return 0;
                }
            }
        }
    }
    /* 20G */
    if (_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y2 > 0) {
        /* triport detection */
        for (v=_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y2; v>0; v--) {
            _tdm->_core_data.vars_pkg.m_tdm_vmap_alloc.tri_chk = tdm_td2p_legacy_which_tsc(lr20[v],_tdm->_chip_data.soc_pkg.pmap);
            for (w=0; w<4; w++) {
                if (_tdm->_chip_data.soc_pkg.speed[_tdm->_chip_data.soc_pkg.pmap[_tdm->_core_data.vars_pkg.m_tdm_vmap_alloc.tri_chk][w]]!=SPEED_20G) {
                    _tdm->_core_data.vars_pkg.m_tdm_vmap_alloc.tri_en_20=BOOL_TRUE;
                    TDM_PRINT0("20G triport detected\n");
                    break;
                }
            }
        }
        /* slot allocation */
        if (tdm_td2p_check_pipe_ethernet(lr20[_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y2],_tdm) && _tdm->_chip_data.soc_pkg.soc_vars.td2p.higig_mgmt==BOOL_FALSE) {
            if (tdm_core_postalloc(_tdm->_core_data.vmap, _tdm->_chip_data.soc_pkg.clk_freq, 20, &_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.yy, &(_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y2), _tdm->_chip_data.soc_pkg.lr_idx_limit, lr20, TOKEN_20G, "20", TD2P_NUM_EXT_PORTS)!=PASS) {
                TDM_ERROR0("Critical error in 20G time vector preallocation\n");
                return 0;
            }
        }
        else {
            if (tdm_core_postalloc(_tdm->_core_data.vmap, _tdm->_chip_data.soc_pkg.clk_freq, 21, &_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.yy, &(_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y2), _tdm->_chip_data.soc_pkg.lr_idx_limit, lr20, TOKEN_20G, "20", TD2P_NUM_EXT_PORTS)!=PASS) {
                TDM_ERROR0("Critical error in 20G time vector preallocation\n");
                return 0;
            }
        }
    }
    /* 10G */
    if (_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y1 > 0) {
        if (tdm_td2p_check_pipe_ethernet(lr10[_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y1],_tdm) && _tdm->_chip_data.soc_pkg.soc_vars.td2p.higig_mgmt==BOOL_FALSE) {
            if (tdm_core_postalloc(_tdm->_core_data.vmap, _tdm->_chip_data.soc_pkg.clk_freq, 10, &_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.yy, &(_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y1), _tdm->_chip_data.soc_pkg.lr_idx_limit, lr10, TOKEN_10G, "10", TD2P_NUM_EXT_PORTS)!=PASS) {
                TDM_ERROR0("Critical error in 10G time vector preallocation\n");
                return 0;
            }
        }
        else {
            if (tdm_core_postalloc(_tdm->_core_data.vmap, _tdm->_chip_data.soc_pkg.clk_freq, 11, &_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.yy, &(_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y1), _tdm->_chip_data.soc_pkg.lr_idx_limit, lr10, TOKEN_10G, "10", TD2P_NUM_EXT_PORTS)!=PASS) {
                TDM_ERROR0("Critical error in 10G time vector preallocation\n");
                return 0;
            }
        }
    }
    /* 1G */
    if (_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y8 > 0) {
        if (tdm_core_postalloc(_tdm->_core_data.vmap, _tdm->_chip_data.soc_pkg.clk_freq, 1, &_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.yy, &_tdm->_core_data.vars_pkg.m_tdm_core_vbs_scheduler.y8, _tdm->_chip_data.soc_pkg.lr_idx_limit, lr1, TOKEN_1G, "1", TD2P_NUM_EXT_PORTS)!=PASS) {
            TDM_ERROR0("Critical error in 1G time vector preallocation\n");
            return 0;
        }
    }

    return ( _tdm->_core_exec[TDM_CORE_EXEC__SCHEDULER]( _tdm ) );
}
