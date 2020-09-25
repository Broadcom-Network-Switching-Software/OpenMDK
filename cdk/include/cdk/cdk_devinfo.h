/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef __CDK_DEVINFO_H__
#define __CDK_DEVINFO_H__

#include <cdk/cdk_device.h>
#include <cdk/cdk_symbols.h>

/*******************************************************************************
 *
 * CDK_DEVINFO Header
 *
 * This header can be included in your source files to define a datastructure
 * describing all devices in the current configuration. 
 *
 * This is provided as a simple convenience when you want to work programmatically
 * with the current device configuration. 
 *
 * NOTES:
 *      This header declares a datastructure in the file including it. 
 */

/*
 * This structure describes each device. 
 * It contains the fields available in the CDK_DEVLIST_ENTRY macro. 
 */
typedef struct cdk_devinfo_s {
    
    /* 
     * The following members are populated directly from the 
     * CDK_DEVLIST_ENTRY macro. 
 */
    cdk_dev_type_t dev_type; 
    const char* name; 
    uint32_t vendor_id; 
    uint32_t device_id; 
    uint32_t revision_id; 
    uint32_t model; 
    uint32_t probe_info; 
    const char* base_driver; 
    const char* base_configuration; 
    const char* fullname;     
    uint32_t flags; 
    const char* codename; 
    const char* product_family; 
    const char* description; 
    

#ifdef CDK_DEVINFO_INCLUDE_SYMBOLS
    /*
     * This device's symbol table. You will need to link against
     * the sym library or include the allsyms.c file. 
     */
    cdk_symbols_t* syms; 
#endif


    /*
     * Custom member for your use
     */
#ifdef CDK_DEVINFO_CUSTOM_MEMBERS
    CDK_DEVINFO_CUSTOM_MEMBERS ;
#endif

    void* cookie; 

} cdk_devinfo_t; 


/*
 * This is the default name for the generated table. 
 * Override if necessary. 
 */
#ifndef CDK_DEVINFO_TABLE_NAME
#define CDK_DEVINFO_TABLE_NAME cdk_devinfo_table
#endif

#ifdef CDK_DEVINFO_DEFINE

/* Define the actual table */

#ifdef CDK_DEVINFO_INCLUDE_ALL
#define CDK_DEVLIST_INCLUDE_ALL
#endif

cdk_devinfo_t CDK_DEVINFO_TABLE_NAME [] = 
    {
#ifdef CDK_DEVINFO_INCLUDE_SYMBOLS
#define CDK_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_fl,_cn,_pf,_pd,_r0,_r1) \
{ cdkDevType_##_bd, #_nm, _vn, _dv, _rv, _md, _pi, #_bd, #_bc, #_fn, _fl, _cn, _pf, _pd, &_bd##_symbols},
#else
#define CDK_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_fl,_cn,_pf,_pd,_r0,_r1) \
{ cdkDevType_##_bd, #_nm, _vn, _dv, _rv, _md, _pi, #_bd, #_bc, #_fn, _fl, _cn, _pf, _pd },
#endif

#include <cdk/cdk_devlist.h>

        /* Last Entry */
        { cdkDevTypeCount, NULL }

    };

#else

/* Extern the table. This should be defined elsewhere in your code with CDK_DEVINFO_DEFINE=1 */

extern cdk_devinfo_t CDK_DEVINFO_TABLE_NAME []; 

#endif


#endif /* __CDK_DEVINFO_H__ */
