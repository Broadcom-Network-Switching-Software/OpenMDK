/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

/*******************************************************************************
 *
 * CDK Symbol Field Routines
 */

#include <cdk/cdk_symbols.h>


uint32_t* 
cdk_field_info_decode(uint32_t* fp, cdk_field_info_t* finfo, const char** fnames)
{
    if(!fp) {
        return NULL; 
    }

    if(finfo) {
        /*
         * Single or Double Word Descriptor?
         */
        if(CDK_SYMBOL_FIELD_EXT(*fp)) {
            /* Double Word */
            finfo->fid = CDK_SYMBOL_FIELD_EXT_ID_GET(*fp); 
            finfo->maxbit = CDK_SYMBOL_FIELD_EXT_MAX_GET(*(fp+1)); 
            finfo->minbit = CDK_SYMBOL_FIELD_EXT_MIN_GET(*(fp+1)); 
        }       
        else {
            /* Single Word */
            finfo->fid = CDK_SYMBOL_FIELD_ID_GET(*fp); 
            finfo->maxbit = CDK_SYMBOL_FIELD_MAX_GET(*fp); 
            finfo->minbit = CDK_SYMBOL_FIELD_MIN_GET(*fp); 
        }       

        if(fnames) {
            finfo->name = fnames[finfo->fid]; 
        }
        else {
            finfo->name = NULL; 
        }       
    }

    if(CDK_SYMBOL_FIELD_LAST(*fp)) {
        return NULL; 
    }

    if(CDK_SYMBOL_FIELD_EXT(*fp)) {
        return fp+2; 
    }

    return fp+1; 
}

uint32_t
cdk_field_info_count(uint32_t* fp)
{    
    int count = 0; 
    while(fp) {
        fp = cdk_field_info_decode(fp, NULL, NULL); 
        count++; 
    }   
    return count; 
}
