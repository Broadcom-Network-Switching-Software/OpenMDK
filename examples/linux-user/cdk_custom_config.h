/******************************************************************************
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.1,
 * WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING
 * ANY FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.$
 */

#ifndef __CDK_CUSTOM_CONFIG_H__
#define __CDK_CUSTOM_CONFIG_H__

/*******************************************************************************
 *
 * CDK_CUSTOM_CONFIG File for the Linux User mode Example
 */

/*
 * Our configuration of the software will use the system built-in data types
 */
#include <inttypes.h>
#include <stdlib.h>

/*
 * Instruct the CDK to inherit these types directly, rather than defining them. 
 */
#define CDK_CONFIG_DEFINE_SIZE_T                0
#define CDK_CONFIG_DEFINE_UINT8_T               0
#define CDK_CONFIG_DEFINE_UINT16_T              0
#define CDK_CONFIG_DEFINE_UINT32_T              0
#define CDK_CONFIG_DEFINE_PRIu32                0
#define CDK_CONFIG_DEFINE_PRIx32                0


/*
 * Instruct the CDK to use the system-provided LIBC
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#define CDK_ABORT                               abort
#define CDK_PRINTF                              printf
#define CDK_VPRINTF                             vprintf
#define CDK_SPRINTF                             sprintf
#define CDK_VSPRINTF                            vsprintf
#define CDK_ATOI                                atoi
#define CDK_STRNCHR                             strnchr
#define CDK_STRCPY                              strcpy
#define CDK_STRNCPY                             strncpy
#define CDK_STRLEN                              strlen
#define CDK_STRCHR                              strchr
#define CDK_STRRCHR                             strrchr
#define CDK_STRCMP                              strcmp
#define CDK_MEMCMP                              memcmp
#define CDK_MEMSET                              memset
#define CDK_MEMCPY                              memcpy
#define CDK_STRUPR                              strupr
#define CDK_TOUPPER                             toupper
#define CDK_STRCAT                              strcat


#endif /* __CDK_CUSTOM_CONFIG_H__ */
