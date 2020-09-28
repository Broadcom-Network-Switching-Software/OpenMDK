/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 * $All Rights Reserved.$
 *
 * TDM chip specific IARB arbitration schedules
 */
#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56670_A0 == 1

#include "bcm56670_a0_tdm_core_top.h"

/**NAK, AP has only 1 Iarb TDM tbl. TD2+ had lr_tbl and ovs_tbl**/ 

/**
@name: init_iarb_tdm_lr_table
@param: int, int, int, int, int*, int*, int[512], int[512]

IARB TDM Linerate Schedule:
  Input  1  => Core bandwidth - 567/480/340/300/260.
  Input  2  => 1 if there are 4x1G management ports, 0 otherwise.
  Input  3  => 1 if there are 4x2.5G management ports, 0 otherwise.
  Input  4  => 1 if there are 1x10G management port, 0 otherwise.
  Output 5  => The X-pipe TDM linerate table wrap pointer value.
  Output 6  => The Y-pipe TDM linerate table wrap pointer value.
  Output 7  => The X-pipe TDM linerate schedule.
  Output 8  => The Y-pipe TDM linerate schedule.
 */
#define IARB_MAIN_TDM__TDM_SLOT_MACSEC_PORT 5
void
tdm_mn_init_iarb_tdm_table (int core_bw, int *iarb_tdm_wrap_ptr_lr_x, int *iarb_tdm_tbl_lr_x)
{
  int i;

  switch (core_bw) {
    case 815:
    for(i=0; i<321; i++){
      if(i%2 ==0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
      } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
      }
    }
    for(i=0; i<321; i++){
      switch(i){
        case 20: case 42: case 62: case 84: case 105: case 126: case 146: case 168: 
        case 189: case 210: case 230: case 252: case 273: case 294: case 315:
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_MACSEC_PORT;
        break;
      }
    }
    for(i=0; i<321; i++){
      if(i%53 ==0 && i !=0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_EP_LOOPBACK;
      }
    }
    for(i=0; i<321; i++){
      switch(i){
        case 0: case 108: case 214 :
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_CMIC_PORT;
        break;
      }
    }
    for(i=0; i<321; i++){
      switch(i){
        case 160: case 320 :
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_AUX_OPS_SLOT;
        break;
      }
    }
    for(i=0; i<321; i++){
      switch(i){
        case 40: case 148: case 255 :
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_AUX_OPS_SLOT;
        break;
      }
    }
      *iarb_tdm_wrap_ptr_lr_x = 321;
/*printf("---------- IARB CALENDAR --------------------\n");
for(i=0 ; i<321; i++){
 switch(iarb_tdm_tbl_lr_x[i]) {
   case IARB_MAIN_TDM__TDM_SLOT_PGW_0:
     printf("IARB Calendar Slot %d allocated to PGW_0\n",i);
   break;
   case IARB_MAIN_TDM__TDM_SLOT_PGW_1:
     printf("IARB Calendar Slot %d allocated to PGW_1\n",i);
   break;
   case IARB_MAIN_TDM__TDM_SLOT_MACSEC_PORT:
     printf("IARB Calendar Slot %d allocated to MACSEC\n",i);
   break;
   case IARB_MAIN_TDM__TDM_SLOT_EP_LOOPBACK:
     printf("IARB Calendar Slot %d allocated to LOOPBACK\n",i);
   break;
   case IARB_MAIN_TDM__TDM_SLOT_CMIC_PORT:
     printf("IARB Calendar Slot %d allocated to CMIC\n",i);
   break;
   case IARB_MAIN_TDM__TDM_SLOT_AUX_OPS_SLOT:
     printf("IARB Calendar Slot %d allocated to AUX/SBUS\n",i);
   break;
 }
}
 printf("--------------------------------------------\n");*/
    break;
    case 818: /* CPRI_25G */
    for(i=0; i<334; i++){
      if(i%2 ==0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
      } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
      }
    }
    for(i=0; i<334; i++){
      if(i%55 ==0 && i !=0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_EP_LOOPBACK;
      }
    }
    for(i=0; i<334; i++){
      switch(i){
        case 0: case 107: case 214 :
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_CMIC_PORT;
        break;
      }
    }
    for(i=0; i<334; i++){
      switch(i){
        case 166: case 333 :
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_AUX_OPS_SLOT;
        break;
      }
    }
    for(i=0; i<334; i++){
      switch(i){
        case 41: case 148: case 255 :
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_AUX_OPS_SLOT;
        break;
      }
    }
      *iarb_tdm_wrap_ptr_lr_x = 334;
/*printf("---------- IARB CALENDAR --------------------\n");
for(i=0 ; i<334; i++){
 switch(iarb_tdm_tbl_lr_x[i]) {
   case IARB_MAIN_TDM__TDM_SLOT_PGW_0:
     printf("IARB Calendar Slot %d allocated to PGW_0\n",i);
   break;
   case IARB_MAIN_TDM__TDM_SLOT_PGW_1:
     printf("IARB Calendar Slot %d allocated to PGW_1\n",i);
   break;
   case IARB_MAIN_TDM__TDM_SLOT_MACSEC_PORT:
     printf("IARB Calendar Slot %d allocated to MACSEC\n",i);
   break;
   case IARB_MAIN_TDM__TDM_SLOT_EP_LOOPBACK:
     printf("IARB Calendar Slot %d allocated to LOOPBACK\n",i);
   break;
   case IARB_MAIN_TDM__TDM_SLOT_CMIC_PORT:
     printf("IARB Calendar Slot %d allocated to CMIC\n",i);
   break;
   case IARB_MAIN_TDM__TDM_SLOT_AUX_OPS_SLOT:
     printf("IARB Calendar Slot %d allocated to AUX/SBUS\n",i);
   break;
 }
}
 printf("--------------------------------------------\n");*/
    break;
    case 819:
    for(i=0; i<283; i++){
      if(i%2 ==0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
      } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
      }
    }
    for(i=0; i<283; i++){
      switch(i){
        case 17: case 35: case 53: case 69: case 87: case 104: case 122: case 140: case 158: case 192: case 210: case 228: case 246: case 264: case 279:
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_MACSEC_PORT;
      }
    }
    for(i=0; i<283; i++){
      switch(i){
        case 56: case 112 : case 168: case 224: case 282:
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_EP_LOOPBACK;
        break;
      }
    }
    for(i=0; i<283; i++){
      switch(i){
        case 1: case 107: case 215 :
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_CMIC_PORT;
        break;
      }
    }
    for(i=0; i<283; i++){
      switch(i){
        case 45: case 91 : case 137: case 281:
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_AUX_OPS_SLOT;
        break;
      }
    }
      *iarb_tdm_wrap_ptr_lr_x = 283;
/*printf("---------- IARB CALENDAR --------------------\n");
for(i=0 ; i<283; i++){
 switch(iarb_tdm_tbl_lr_x[i]) {
   case IARB_MAIN_TDM__TDM_SLOT_PGW_0:
     printf("IARB Calendar Slot %d allocated to PGW_0\n",i);
   break;
   case IARB_MAIN_TDM__TDM_SLOT_PGW_1:
     printf("IARB Calendar Slot %d allocated to PGW_1\n",i);
   break;
   case IARB_MAIN_TDM__TDM_SLOT_MACSEC_PORT:
     printf("IARB Calendar Slot %d allocated to MACSEC\n",i);
   break;
   case IARB_MAIN_TDM__TDM_SLOT_EP_LOOPBACK:
     printf("IARB Calendar Slot %d allocated to LOOPBACK\n",i);
   break;
   case IARB_MAIN_TDM__TDM_SLOT_CMIC_PORT:
     printf("IARB Calendar Slot %d allocated to CMIC\n",i);
   break;
   case IARB_MAIN_TDM__TDM_SLOT_AUX_OPS_SLOT:
     printf("IARB Calendar Slot %d allocated to AUX/SBUS\n",i);
   break;
 }
}
 printf("--------------------------------------------\n");*/
    break;
    case 820:
    for(i=0; i<335; i++){
      if(i%2 ==0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
      } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
      }
    }
    for(i=0; i<335; i++){
      switch(i){
        case 17: case 35: case 53: case 69: case 87: case 104: case 122: case 140: case 158: case 192: case 210: case 228: case 246: case 264: case 279: case 294: case 310:
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_MACSEC_PORT;
      }
    }
    for(i=0; i<335; i++){
      if(i%55 ==0 && i !=0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_EP_LOOPBACK;
      }
    }
    for(i=0; i<335; i++){
      switch(i){
        case 1: case 107: case 215 :
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_CMIC_PORT;
        break;
      }
    }
    for(i=0; i<335; i++){
      switch(i){
        case 45: case 91 : case 136: case 281: case 334:
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_AUX_OPS_SLOT;
        break;
      }
    }
      *iarb_tdm_wrap_ptr_lr_x = 335;

/*printf("---------- IARB CALENDAR --------------------\n");
for(i=0 ; i<335; i++){
 switch(iarb_tdm_tbl_lr_x[i]) {
   case IARB_MAIN_TDM__TDM_SLOT_PGW_0:
     printf("IARB Calendar Slot %d allocated to PGW_0\n",i);
   break;
   case IARB_MAIN_TDM__TDM_SLOT_PGW_1:
     printf("IARB Calendar Slot %d allocated to PGW_1\n",i);
   break;
   case IARB_MAIN_TDM__TDM_SLOT_MACSEC_PORT:
     printf("IARB Calendar Slot %d allocated to MACSEC\n",i);
   break;
   case IARB_MAIN_TDM__TDM_SLOT_EP_LOOPBACK:
     printf("IARB Calendar Slot %d allocated to LOOPBACK\n",i);
   break;
   case IARB_MAIN_TDM__TDM_SLOT_CMIC_PORT:
     printf("IARB Calendar Slot %d allocated to CMIC\n",i);
   break;
   case IARB_MAIN_TDM__TDM_SLOT_AUX_OPS_SLOT:
     printf("IARB Calendar Slot %d allocated to AUX/SBUS\n",i);
   break;
 }
}
 printf("--------------------------------------------\n");*/
    break;
    case 861: case 862:
    for(i=0; i<227; i++){
      if(i%2 ==0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
      } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
      }
    }
    for(i=0; i<227; i++){
      switch(i){
        case 106:
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_EP_LOOPBACK;
        break;
      }
    }
    for(i=0; i<227; i++){
      switch(i){
        case 0:
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_CMIC_PORT;
        break;
      }
    }
    for(i=0; i<227; i++){
      switch(i){
        case 10: case 77 : case 91:
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_AUX_OPS_SLOT;
        break;
      }
    }
      *iarb_tdm_wrap_ptr_lr_x = 227;
/*printf("---------- IARB CALENDAR --------------------\n");
for(i=0 ; i<227; i++){
 switch(iarb_tdm_tbl_lr_x[i]) {
   case IARB_MAIN_TDM__TDM_SLOT_PGW_0:
     printf("IARB Calendar Slot %d allocated to PGW_0\n",i);
   break;
   case IARB_MAIN_TDM__TDM_SLOT_PGW_1:
     printf("IARB Calendar Slot %d allocated to PGW_1\n",i);
   break;
   case IARB_MAIN_TDM__TDM_SLOT_MACSEC_PORT:
     printf("IARB Calendar Slot %d allocated to MACSEC\n",i);
   break;
   case IARB_MAIN_TDM__TDM_SLOT_EP_LOOPBACK:
     printf("IARB Calendar Slot %d allocated to LOOPBACK\n",i);
   break;
   case IARB_MAIN_TDM__TDM_SLOT_CMIC_PORT:
     printf("IARB Calendar Slot %d allocated to CMIC\n",i);
   break;
   case IARB_MAIN_TDM__TDM_SLOT_AUX_OPS_SLOT:
     printf("IARB Calendar Slot %d allocated to AUX/SBUS\n",i);
   break;
 }
}
 printf("--------------------------------------------\n");*/
    break;
    case 816:
    for(i=0; i<297; i++){
      if(i%2 ==0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
      } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
      }
    }
    for(i=0; i<297; i++){
      switch(i) {
        case 18: case 38: case 56: case 76: case 95: case 114: case 133: case 152:
        case 171: case 190: case 209: case 228: case 247: case 266: case 285:
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_MACSEC_PORT;
        break;
      }
    }
    for(i=0; i<297; i++){
      if(i%49 ==0 && i !=0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_EP_LOOPBACK;
      }
    }
    for(i=0; i<297; i++){
      switch(i){
        case 0: case 120: case 214 :
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_CMIC_PORT;
        break;
      }
    }
    for(i=0; i<297; i++){
      switch(i){
        case 160: case 185 :
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_AUX_OPS_SLOT;
        break;
      }
    }
    for(i=0; i<297; i++){
      switch(i){
        case 41: case 148: case 255 :
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_AUX_OPS_SLOT;
        break;
      }
    }
      *iarb_tdm_wrap_ptr_lr_x = 297;

/*printf("---------- IARB CALENDAR --------------------\n");
for(i=0 ; i<297; i++){
 switch(iarb_tdm_tbl_lr_x[i]) {
   case IARB_MAIN_TDM__TDM_SLOT_PGW_0:
     printf("IARB Calendar Slot %d allocated to PGW_0\n",i);
   break;
   case IARB_MAIN_TDM__TDM_SLOT_PGW_1:
     printf("IARB Calendar Slot %d allocated to PGW_1\n",i);
   break;
   case IARB_MAIN_TDM__TDM_SLOT_MACSEC_PORT:
     printf("IARB Calendar Slot %d allocated to MACSEC\n",i);
   break;
   case IARB_MAIN_TDM__TDM_SLOT_EP_LOOPBACK:
     printf("IARB Calendar Slot %d allocated to LOOPBACK\n",i);
   break;
   case IARB_MAIN_TDM__TDM_SLOT_CMIC_PORT:
     printf("IARB Calendar Slot %d allocated to CMIC\n",i);
   break;
   case IARB_MAIN_TDM__TDM_SLOT_AUX_OPS_SLOT:
     printf("IARB Calendar Slot %d allocated to AUX/SBUS\n",i);
   break;
 }
}
 printf("--------------------------------------------\n");*/ 
    break; /*break*/
    case 817:
    for(i=0; i<302; i++){
      if(i%2 ==0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
      } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
      }
    }
   for(i=0; i<302; i++){
      if(i%50 ==0 && i !=0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_EP_LOOPBACK;
      }
   }
   for(i=0; i<302; i++){
      if(i%99 ==0 && i !=0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_CMIC_PORT;
      }
   }
   for(i=0; i<302; i++){
      if(i%74 ==0 && i !=0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_AUX_OPS_SLOT;
      }
   }
      *iarb_tdm_wrap_ptr_lr_x = 302;
/*printf("---------- IARB CALENDAR --------------------\n");
for(i=0 ; i<302; i++){
 switch(iarb_tdm_tbl_lr_x[i]) {
   case IARB_MAIN_TDM__TDM_SLOT_PGW_0:
     printf("IARB Calendar Slot %d allocated to PGW_0\n",i);
   break;
   case IARB_MAIN_TDM__TDM_SLOT_PGW_1:
     printf("IARB Calendar Slot %d allocated to PGW_1\n",i);
   break;
   case IARB_MAIN_TDM__TDM_SLOT_MACSEC_PORT:
     printf("IARB Calendar Slot %d allocated to MACSEC\n",i);
   break;
   case IARB_MAIN_TDM__TDM_SLOT_EP_LOOPBACK:
     printf("IARB Calendar Slot %d allocated to LOOPBACK\n",i);
   break;
   case IARB_MAIN_TDM__TDM_SLOT_CMIC_PORT:
     printf("IARB Calendar Slot %d allocated to CMIC\n",i);
   break;
   case IARB_MAIN_TDM__TDM_SLOT_AUX_OPS_SLOT:
     printf("IARB Calendar Slot %d allocated to AUX/SBUS\n",i);
   break;
 }
}
 printf("--------------------------------------------\n");*/

    break;
    case 503 :
      for (i = 0; i < 4; i++) {
        if (i%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[4] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
      iarb_tdm_tbl_lr_x[5] = IARB_MAIN_TDM__TDM_SLOT_AUX_OPS_SLOT;
      for (i = 6; i < 34; i++) {
        if (i%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        }
      }
      iarb_tdm_tbl_lr_x[34] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
      iarb_tdm_tbl_lr_x[35] = IARB_MAIN_TDM__TDM_SLOT_EP_LOOPBACK;
      for (i = 36; i < 46; i++) {
        if (i%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[46] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
      iarb_tdm_tbl_lr_x[47] = IARB_MAIN_TDM__TDM_SLOT_MACSEC_PORT;
      for (i = 48; i < 82; i++) {
        if (i%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        }
      }
      iarb_tdm_tbl_lr_x[82] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
      iarb_tdm_tbl_lr_x[83] = IARB_MAIN_TDM__TDM_SLOT_AUX_OPS_SLOT;
      for (i = 84; i < 88; i++) {
        if (i%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[88] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
      iarb_tdm_tbl_lr_x[89] = IARB_MAIN_TDM__TDM_SLOT_EP_LOOPBACK;
      for (i = 90; i < 94; i++) {
        if (i%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        }
      }
      iarb_tdm_tbl_lr_x[94] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
      iarb_tdm_tbl_lr_x[95] = IARB_MAIN_TDM__TDM_SLOT_MACSEC_PORT;
      for (i = 96; i < 106; i++) {
        if (i%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[106] = IARB_MAIN_TDM__TDM_SLOT_CMIC_PORT;
      for (i = 107; i < 143; i++) {
        if (i%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[143] = IARB_MAIN_TDM__TDM_SLOT_CMIC_PORT;
      for (i = 144; i < 148; i++) {
        if (i%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[148] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
      iarb_tdm_tbl_lr_x[149] = IARB_MAIN_TDM__TDM_SLOT_MACSEC_PORT;
      for (i = 150; i < 154; i++) {
        if (i%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[154] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
      iarb_tdm_tbl_lr_x[155] = IARB_MAIN_TDM__TDM_SLOT_AUX_OPS_SLOT;
      for (i = 156; i < 196; i++) {
        if (i%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[196] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
      iarb_tdm_tbl_lr_x[197] = IARB_MAIN_TDM__TDM_SLOT_EP_LOOPBACK;
      for (i = 198; i < 208; i++) {
        if (i%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[208] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
      iarb_tdm_tbl_lr_x[209] = IARB_MAIN_TDM__TDM_SLOT_MACSEC_PORT;
      iarb_tdm_tbl_lr_x[210] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
      iarb_tdm_tbl_lr_x[211] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
      iarb_tdm_tbl_lr_x[212] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
      iarb_tdm_tbl_lr_x[213] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
      iarb_tdm_tbl_lr_x[214] = IARB_MAIN_TDM__TDM_SLOT_CMIC_PORT;
    break;
    case 252 :
      iarb_tdm_tbl_lr_x[0] = IARB_MAIN_TDM__TDM_SLOT_AUX_OPS_SLOT;
      for (i = 1; i < 16; i++) {
        if (i%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[16] = IARB_MAIN_TDM__TDM_SLOT_MACSEC_PORT;
      for (i = 17; i < 26; i++) {
        if (i%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[26] = IARB_MAIN_TDM__TDM_SLOT_CMIC_PORT;
      for (i = 27; i < 35; i++) {
        if (i%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[35] = IARB_MAIN_TDM__TDM_SLOT_CMIC_PORT;
      for (i = 36; i < 45; i++) {
        if (i%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[45] = IARB_MAIN_TDM__TDM_SLOT_EP_LOOPBACK;
      for (i = 46; i < 53; i++) {
        if (i%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[53] = IARB_MAIN_TDM__TDM_SLOT_MACSEC_PORT;
      for (i = 54; i < 65; i++) {
        if (i%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[65] = IARB_MAIN_TDM__TDM_SLOT_AUX_OPS_SLOT;
      for (i = 66; i < 71; i++) {
        if (i%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[71] = IARB_MAIN_TDM__TDM_SLOT_MACSEC_PORT;
      for (i = 72; i < 91; i++) {
        if (i%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[91] = IARB_MAIN_TDM__TDM_SLOT_MACSEC_PORT;
      iarb_tdm_tbl_lr_x[92] = IARB_MAIN_TDM__TDM_SLOT_EP_LOOPBACK;
      for (i = 93; i < 110; i++) {
        if (i%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[110] = IARB_MAIN_TDM__TDM_SLOT_MACSEC_PORT;
      for (i = 111; i < 119; i++) {
        if (i%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[119] = IARB_MAIN_TDM__TDM_SLOT_CMIC_PORT;
      for (i = 120; i < 129; i++) {
        if (i%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[129] = IARB_MAIN_TDM__TDM_SLOT_MACSEC_PORT;
      iarb_tdm_tbl_lr_x[130] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
      iarb_tdm_tbl_lr_x[131] = IARB_MAIN_TDM__TDM_SLOT_AUX_OPS_SLOT;
      for (i = 132; i < 139; i++) {
        if (i%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[139] = IARB_MAIN_TDM__TDM_SLOT_EP_LOOPBACK;
      for (i = 140; i < 147; i++) {
        if (i%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        }
      }
      iarb_tdm_tbl_lr_x[147] = IARB_MAIN_TDM__TDM_SLOT_MACSEC_PORT;
      for (i = 148; i < 165; i++) {
        if (i%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[165] = IARB_MAIN_TDM__TDM_SLOT_MACSEC_PORT;
      for (i = 166; i < 183; i++) {
        if (i%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[183] = IARB_MAIN_TDM__TDM_SLOT_MACSEC_PORT;
      iarb_tdm_tbl_lr_x[184] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
      iarb_tdm_tbl_lr_x[185] = IARB_MAIN_TDM__TDM_SLOT_EP_LOOPBACK;
    break;
    case 567 :
      *iarb_tdm_wrap_ptr_lr_x = 232;
      for (i = 0; i < 28; i++) {
        if (i%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[28] = IARB_MAIN_TDM__TDM_SLOT_AUX_OPS_SLOT;
      for (i = 29; i < 57; i++) {
        if ((i-29)%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[57] = IARB_MAIN_TDM__TDM_SLOT_RDB_PORT;
      for (i = 58; i < 86; i++) {
        if ((i-58)%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[86] = IARB_MAIN_TDM__TDM_SLOT_AUX_OPS_SLOT;
      for (i = 87; i < 115; i++) {
        if ((i-87)%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[115] = IARB_MAIN_TDM__TDM_SLOT_CMIC_PORT;
      for (i = 116; i < 144; i++) {
        if ((i-116)%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[144] = IARB_MAIN_TDM__TDM_SLOT_AUX_OPS_SLOT;
      for (i = 145; i < 173; i++) {
        if ((i-145)%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[173] = IARB_MAIN_TDM__TDM_SLOT_RDB_PORT;
      for (i = 174; i < 202; i++) {
        if ((i-174)%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[202] = IARB_MAIN_TDM__TDM_SLOT_AUX_OPS_SLOT;
      for (i = 203; i < 217; i++) {
        if ((i-203)%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[217] = IARB_MAIN_TDM__TDM_SLOT_EP_LOOPBACK;
      for (i = 218; i < 232; i++) {
        if ((i-218)%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[232] = IARB_MAIN_TDM__TDM_SLOT_CMIC_PORT;

      /* Then check for management ports. */
      /*  NAK, Not suppoted in  AP Iarb. management ports are scheduled with regular ports in PGW".*/
      break;
    case 511 :
      *iarb_tdm_wrap_ptr_lr_x = 210;
      for (i = 0; i < 26; i++) {
        if (i%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[26] = IARB_MAIN_TDM__TDM_SLOT_AUX_OPS_SLOT;
      for (i = 27; i < 51; i++) {
        if ((i-27)%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[51] = IARB_MAIN_TDM__TDM_SLOT_RDB_PORT;
      for (i = 52; i < 78; i++) {
        if ((i-52)%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[78] = IARB_MAIN_TDM__TDM_SLOT_AUX_OPS_SLOT;
      for (i = 79; i < 103; i++) {
        if ((i-79)%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[103] = IARB_MAIN_TDM__TDM_SLOT_CMIC_PORT;
      for (i = 104; i < 130; i++) {
        if ((i-104)%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[130] = IARB_MAIN_TDM__TDM_SLOT_AUX_OPS_SLOT;
      for (i = 131; i < 155; i++) {
        if ((i-131)%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[155] = IARB_MAIN_TDM__TDM_SLOT_RDB_PORT;
      for (i = 156; i < 182; i++) {
        if ((i-156)%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[182] = IARB_MAIN_TDM__TDM_SLOT_EP_LOOPBACK;
      for (i = 183; i < 209; i++) {
        if ((i-183)%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[209] = IARB_MAIN_TDM__TDM_SLOT_AUX_OPS_SLOT;
      iarb_tdm_tbl_lr_x[210] = IARB_MAIN_TDM__TDM_SLOT_CMIC_PORT;
      break;
    case 480 : 
      *iarb_tdm_wrap_ptr_lr_x = 200;
      for (i = 0; i < 24; i++) {
        if (i%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[24] = IARB_MAIN_TDM__TDM_SLOT_AUX_OPS_SLOT;
      for (i = 25; i < 49; i++) {
        if ((i-25)%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        }
      }
      iarb_tdm_tbl_lr_x[49] = IARB_MAIN_TDM__TDM_SLOT_RDB_PORT; 
      for (i = 50; i < 74; i++) {
        if ((i-50)%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[74] = IARB_MAIN_TDM__TDM_SLOT_AUX_OPS_SLOT;
      for (i = 75; i < 99; i++) {
        if ((i-75)%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        }
      }
      iarb_tdm_tbl_lr_x[99] = IARB_MAIN_TDM__TDM_SLOT_CMIC_PORT;
      for (i = 100; i < 124; i++) {
        if ((i-100)%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[124] = IARB_MAIN_TDM__TDM_SLOT_AUX_OPS_SLOT;
      for (i = 125; i < 149; i++) {
        if ((i-125)%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        }
      }
      iarb_tdm_tbl_lr_x[149] = IARB_MAIN_TDM__TDM_SLOT_RDB_PORT;
      for (i = 150; i < 174; i++) {
        if ((i-150)%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[174] = IARB_MAIN_TDM__TDM_SLOT_EP_LOOPBACK;
      for (i = 175; i < 199; i++) {
        if ((i-175)%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        }
      }
      iarb_tdm_tbl_lr_x[199] = IARB_MAIN_TDM__TDM_SLOT_AUX_OPS_SLOT;
      iarb_tdm_tbl_lr_x[200] = IARB_MAIN_TDM__TDM_SLOT_CMIC_PORT;

      /* Then check for management ports. */
      /*  NAK, Not suppoted in  AP Iarb. management ports are scheduled with regular ports in PGW".*/
      break;
    case 340 : 
      *iarb_tdm_wrap_ptr_lr_x = 144;
      for (i = 0; i < 18; i++) {
        if (i%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[18] = IARB_MAIN_TDM__TDM_SLOT_CMIC_PORT;
      for (i = 19; i < 35; i++) {
        if ((i-19)%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        }
      }
      iarb_tdm_tbl_lr_x[35] = IARB_MAIN_TDM__TDM_SLOT_AUX_OPS_SLOT;
      for (i = 36; i < 54; i++) {
        if ((i-36)%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[54] = IARB_MAIN_TDM__TDM_SLOT_RDB_PORT;
      for (i = 55; i < 71; i++) {
        if ((i-55)%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        }
      }
      iarb_tdm_tbl_lr_x[71] = IARB_MAIN_TDM__TDM_SLOT_AUX_OPS_SLOT;
      for (i = 72; i < 90; i++) {
        if ((i-72)%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[90] = IARB_MAIN_TDM__TDM_SLOT_CMIC_PORT;
      for (i = 91; i < 107; i++) {
        if ((i-91)%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        }
      }
      iarb_tdm_tbl_lr_x[107] = IARB_MAIN_TDM__TDM_SLOT_AUX_OPS_SLOT;
      for (i = 108; i < 126; i++) {
        if ((i-108)%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[126] = IARB_MAIN_TDM__TDM_SLOT_RDB_PORT;
      for (i = 127; i < 143; i++) {
        if ((i-127)%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        }
      }
      iarb_tdm_tbl_lr_x[143] = IARB_MAIN_TDM__TDM_SLOT_AUX_OPS_SLOT;
      iarb_tdm_tbl_lr_x[144] = IARB_MAIN_TDM__TDM_SLOT_EP_LOOPBACK;

      /* Then check for management ports. */
      /*  NAK, Not suppoted in  AP Iarb. management ports are scheduled with regular ports in PGW".*/
      break;
    case 300 :
      *iarb_tdm_wrap_ptr_lr_x = 128;
      for (i = 0; i < 16; i++) {
        if (i%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[16] = IARB_MAIN_TDM__TDM_SLOT_RDB_PORT;
      for (i = 17; i < 31; i++) {
        if ((i-17)%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        }
      }
      iarb_tdm_tbl_lr_x[31] = IARB_MAIN_TDM__TDM_SLOT_AUX_OPS_SLOT;
      for (i = 32; i < 48; i++) {
        if ((i-32)%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[48] = IARB_MAIN_TDM__TDM_SLOT_CMIC_PORT;
      for (i = 49; i < 63; i++) {
        if ((i-49)%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        }
      }
      iarb_tdm_tbl_lr_x[63] = IARB_MAIN_TDM__TDM_SLOT_AUX_OPS_SLOT;
      for (i = 64; i < 80; i++) {
        if ((i-64)%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[80] = IARB_MAIN_TDM__TDM_SLOT_RDB_PORT;
      for (i = 81; i < 95; i++) {
        if ((i-81)%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        }
      }
      iarb_tdm_tbl_lr_x[95] = IARB_MAIN_TDM__TDM_SLOT_AUX_OPS_SLOT;
      for (i = 96; i < 112; i++) {
        if ((i-96)%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[112] = IARB_MAIN_TDM__TDM_SLOT_CMIC_PORT;
      for (i = 113; i < 127; i++) {
        if ((i-113)%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        }
      }
      iarb_tdm_tbl_lr_x[127] = IARB_MAIN_TDM__TDM_SLOT_AUX_OPS_SLOT;
      iarb_tdm_tbl_lr_x[128] = IARB_MAIN_TDM__TDM_SLOT_EP_LOOPBACK;

      /* Then check for management ports. */
      /*  NAK, Not suppoted in  AP Iarb. management ports are scheduled with regular ports in PGW".*/
      break;
    case 260 :
      *iarb_tdm_wrap_ptr_lr_x = 109;

      iarb_tdm_tbl_lr_x[0] = IARB_MAIN_TDM__TDM_SLOT_RDB_PORT;
      for (i = 1; i < 10; i++) {
        if (i%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[10] = IARB_MAIN_TDM__TDM_SLOT_EP_LOOPBACK;
      for (i = 11; i < 20; i++) {
        if (i%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[20] = IARB_MAIN_TDM__TDM_SLOT_AUX_OPS_SLOT;
      for (i = 21; i < 31; i++) {
        if (i%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[31] = IARB_MAIN_TDM__TDM_SLOT_CMIC_PORT;
      for (i = 32; i < 41; i++) {
        if (i%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[41] = IARB_MAIN_TDM__TDM_SLOT_AUX_OPS_SLOT;
      for (i = 42; i < 53; i++) {
        if (i%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[53] = IARB_MAIN_TDM__TDM_SLOT_AUX_OPS_SLOT;
      for (i = 54; i < 65; i++) {
        if (i%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[65] = IARB_MAIN_TDM__TDM_SLOT_RDB_PORT;
      for (i = 66; i < 75; i++) {
        if (i%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[75] = IARB_MAIN_TDM__TDM_SLOT_EP_LOOPBACK;
      for (i = 76; i < 85; i++) {
        if (i%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[85] = IARB_MAIN_TDM__TDM_SLOT_AUX_OPS_SLOT;
      for (i = 86; i < 95; i++) {
        if (i%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }
      iarb_tdm_tbl_lr_x[95] = IARB_MAIN_TDM__TDM_SLOT_CMIC_PORT;
      for (i = 96; i < 110; i++) {
        if (i%2 == 0) {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_0;
        } else {
          iarb_tdm_tbl_lr_x[i] = IARB_MAIN_TDM__TDM_SLOT_PGW_1;
        }
      }

      /* Then check for management ports. */
      /*  NAK, Not suppoted in  AP Iarb. management ports are scheduled with regular ports in PGW".*/
      break;
  }
}


/**
@name: set_iarb_tdm_table
@param: int, int, int, int, int, int, int*, int*, int[512], int[512]

IARB TDM Schedule Generator:
  Return    => 1 if error, 0 otherwise.
  Input  1  => Core bandwidth - 960/720/640/480.
  Input  2  => 1 if X-pipe is oversub, 0 otherwise.
  Input  3  => 1 if Y-pipe is oversub, 0 otherwise.
  Input  4  => 1 if there are 4x1G management ports, 0 otherwise.
  Input  5  => 1 if there are 4x2.5G management ports, 0 otherwise.
  Input  6  => 1 if there are 1x10G management port, 0 otherwise.
  Output 7  => The X-pipe TDM table wrap pointer value.
  Output 8  => The Y-pipe TDM table wrap pointer value.
  Output 9  => The X-pipe TDM schedule.
  Output 10 => The Y-pipe TDM schedule.
 */
int tdm_mn_set_iarb_tdm (int core_bw, int is_x_ovs, int *iarb_tdm_wrap_ptr_x, int *iarb_tdm_tbl_x)
{		  
	/* #if ( defined(_SET_TDM_DV) || defined(_SET_TDM_DEV) ) */
		int i;
		int is_succ;
		int iarb_tdm_wrap_ptr_lr_x; 
		int *iarb_tdm_tbl_lr_x;
		iarb_tdm_tbl_lr_x = (int *) TDM_MN_ALLOC(sizeof(int) * 512, "iarb_tdm_tbl_lr_x");
	/* #else
		int i;
		int is_succ;
		int iarb_tdm_wrap_ptr_ovs_x, iarb_tdm_wrap_ptr_ovs_y;
		int iarb_tdm_wrap_ptr_lr_x, iarb_tdm_wrap_ptr_lr_y;
		TDM_MN_ALLOC(iarb_tdm_tbl_ovs_x, int, 512, "iarb_tdm_tbl_ovs_x");
		TDM_MN_ALLOC(iarb_tdm_tbl_ovs_y, int, 512, "iarb_tdm_tbl_ovs_y");
		TDM_MN_ALLOC(iarb_tdm_tbl_lr_x, int, 512, "iarb_tdm_tbl_lr_x");
		TDM_MN_ALLOC(iarb_tdm_tbl_lr_y, int, 512, "iarb_tdm_tbl_lr_y");
	#endif */

  /*
    Initial IARB TDM table containers - to be copied into final container based
    on the TDM selected.
  */
  
/*if (!(!mgm4x1 && !mgm4x2p5 && !mgm1x10) && !(mgm4x1 ^ mgm4x2p5 ^ mgm1x10)) {
      TDM_ERROR0("Multiple management port settings specified!\n");
  } */

	tdm_mn_init_iarb_tdm_table(core_bw, &iarb_tdm_wrap_ptr_lr_x, iarb_tdm_tbl_lr_x);

    /* The following TDMs have linerate X-pipe and linerate Y-pipe. */
    *iarb_tdm_wrap_ptr_x = iarb_tdm_wrap_ptr_lr_x;
    TDM_COPY(iarb_tdm_tbl_x, iarb_tdm_tbl_lr_x, sizeof(int) * 512);
  


  TDM_PRINT1("iarb_tdm_wrap_ptr_x = %d\n",*iarb_tdm_wrap_ptr_x);
  for (i = 0; i <= *iarb_tdm_wrap_ptr_x; i++) {
    TDM_PRINT2("iarb_tdm_tbl_x[%d] = %d\n",i,iarb_tdm_tbl_x[i]);
  }


  /* Always succeeds by definition. */
  is_succ = 1;
  TDM_FREE(iarb_tdm_tbl_lr_x);
  return is_succ;
}
#endif /* CDK_CONFIG_INCLUDE_BCM56670_A0 */

