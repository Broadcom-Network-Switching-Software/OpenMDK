/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef __CDK_STDLIB_H__
#define __CDK_STDLIB_H__

#include <cdk_config.h>

#include <cdk/cdk_types.h>

#ifndef CDK_ABORT
#define CDK_ABORT cdk_abort
#endif

#ifndef CDK_STRTOL
#define CDK_STRTOL cdk_strtol
#endif

#ifndef CDK_STRTOUL
#define CDK_STRTOUL cdk_strtoul
#endif

#ifndef CDK_ATOI
#define CDK_ATOI cdk_atoi
#endif

#ifndef CDK_CTOI
#define CDK_CTOI cdk_ctoi
#endif

#ifndef CDK_ABS
#define CDK_ABS cdk_abs
#endif

extern void cdk_abort(void);
extern long cdk_strtol(const char *s, char **end, int base);
extern unsigned long cdk_strtoul(const char *s, char **end, int base);
extern int cdk_atoi(const char *s);
extern int cdk_abs(int j);

/* Special CDK library functions */
int cdk_ctoi(const char *s, char **end);

#endif /* __CDK_STDLIB_H__ */
