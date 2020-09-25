/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * System interface definitions for Switch SDK
 */

#ifndef __PHYMOD_CUSTOM_CONFIG_H__
#define __PHYMOD_CUSTOM_CONFIG_H__

#include <cdk/cdk_debug.h>
#include <phy/phy_reg.h>

#undef VENDOR_BROADCOM

/* No need to define */
#define PHYMOD_CONFIG_DEFINE_UINT8_T        0
#define PHYMOD_CONFIG_DEFINE_UINT16_T       0
#define PHYMOD_CONFIG_DEFINE_UINT32_T       0
#define PHYMOD_CONFIG_DEFINE_UINT64_T       0
#define PHYMOD_CONFIG_DEFINE_INT8_T         0
#define PHYMOD_CONFIG_DEFINE_INT16_T        0
#define PHYMOD_CONFIG_DEFINE_INT32_T        0
#define PHYMOD_CONFIG_DEFINE_PRIu32         0
#define PHYMOD_CONFIG_DEFINE_PRIx32         0
#define PHYMOD_CONFIG_DEFINE_PRIu64         0
#define PHYMOD_CONFIG_DEFINE_SIZE_T         0

#define PHYMOD_DEBUG_ERROR(stuff_)          do { CDK_ERR(stuff_); } while(0)
#define PHYMOD_DEBUG_VERBOSE(stuff_)        do { CDK_VERB(stuff_); } while(0)
#define PHYMOD_DIAG_OUT(stuff_)             do { CDK_WARN(stuff_); } while(0)

#define PHYMOD_USLEEP                       phymod_udelay
#define PHYMOD_SLEEP                        my_sleep
#define PHYMOD_MALLOC                       my_malloc
#define PHYMOD_FREE                         my_free

/* These functions map directly to Standard C functions */
#include <cdk/cdk_string.h>
#define PHYMOD_STRCMP                       CDK_STRCMP
#define PHYMOD_MEMSET                       CDK_MEMSET
#define PHYMOD_MEMCPY                       CDK_MEMCPY
#define PHYMOD_STRNCMP                      CDK_STRNCMP
#define PHYMOD_STRCHR                       CDK_STRCHR
#define PHYMOD_STRSTR                       CDK_STRSTR
#define PHYMOD_STRLEN                       CDK_STRLEN
#define PHYMOD_STRCAT                       CDK_STRCAT
#define PHYMOD_STRNCAT(_s1, _s2, _n)        CDK_STRCAT(_s1, _s2)
#define PHYMOD_STRCPY                       CDK_STRCPY
#define PHYMOD_STRNCPY                      CDK_STRNCPY
#include <cdk/cdk_stdlib.h>
#define PHYMOD_STRTOUL                      CDK_STRTOUL
#define PHYMOD_ABS                          CDK_ABS
#include <cdk/cdk_printf.h>
#define PHYMOD_SPRINTF                      CDK_SPRINTF
#define PHYMOD_SNPRINTF                     CDK_SNPRINTF

/* Definiations for PHYMOD usage */
#define int8_t                              int
#define int16_t                             int
#define int32_t                             int
#define int64_t                             long
#define uint16                              unsigned short
#define uint32                              unsigned int
#define uint64_t                            unsigned long

#endif /* __PHYMOD_CUSTOM_CONFIG_H__ */
