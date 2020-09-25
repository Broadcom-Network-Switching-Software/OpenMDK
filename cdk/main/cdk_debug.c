/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * CDK debug message functions.
 */

#include <cdk/cdk_printf.h>
#include <cdk/cdk_debug.h>

#if CDK_CONFIG_INCLUDE_DEBUG == 1

#ifndef CDK_DEBUG_PRINTF
#define CDK_DEBUG_PRINTF CDK_PRINTF
#endif

uint32_t cdk_debug_level = CDK_DBG_ERR | CDK_DBG_WARN;
int (*cdk_debug_printf)(const char *format, ...) = CDK_DEBUG_PRINTF;

#endif /* CDK_CONFIG_INCLUDE_DEBUG */
