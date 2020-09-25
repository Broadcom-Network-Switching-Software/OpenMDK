/*
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
 * Basic types
 *
 * Use size_t and unsigned types provided by system to avoid
 * potential conflicts in type declarations.
 */

#include <inttypes.h>

#define CDK_CONFIG_DEFINE_SIZE_T                0
#define CDK_CONFIG_DEFINE_UINT8_T               0
#define CDK_CONFIG_DEFINE_UINT16_T              0
#define CDK_CONFIG_DEFINE_UINT32_T              0
#define CDK_CONFIG_DEFINE_PRIu32                0
#define CDK_CONFIG_DEFINE_PRIx32                0


/*******************************************************************************
 *
 * C library
 *
 * The CDK will use its own C library functions by default. If the
 * system provides a C library, the CDK should be configured to use
 * the system library functions in order to reduce the size of the
 * application.
 */

#ifdef USE_SYSTEM_LIBC

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

#endif /* USE_SYSTEM_LIBC */

/* We increase the default size to include all installed devices */
#define CDK_CONFIG_MAX_UNITS 128

/*******************************************************************************
 *
 * Minimal CDK
 *
 * The CDK is by default fairly small in size, however it is possible
 * to reduce the size by excluding various features.
 */

#ifdef MINIMAL_BUILD

/* Exclude all chips by default */
#define CDK_CONFIG_INCLUDE_CHIP_DEFAULT         0

/* Include support for one chip only */
#define CDK_CONFIG_INCLUDE_BCM56504             1

#undef  CDK_CONFIG_MAX_UNITS
#define CDK_CONFIG_MAX_UNITS                    1

/* Exclude all symbols */
#define CDK_CONFIG_INCLUDE_CHIP_SYMBOLS         0
#define CDK_CONFIG_INCLUDE_FIELD_INFO           0

/* Exclude all shell commands by default */
#define CDK_CONFIG_SHELL_INCLUDE_DEFAULT        0

/* Include only GETI and SETI */
#define CDK_CONFIG_SHELL_INCLUDE_GETI           1
#define CDK_CONFIG_SHELL_INCLUDE_SETI           1

/* Exclude help text fo shell commands */
#define CDK_CONFIG_SHELL_INCLUDE_HELP           0

/* Exclude debug messages */
#define CDK_CONFIG_INCLUDE_DEBUG                0

#endif /* CDK_MINIMAL_BUILD */

#endif /* __CDK_CUSTOM_CONFIG_H__ */
