/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef __CDK_CTYPE_H__
#define __CDK_CTYPE_H__

#include <cdk_config.h>

#include <cdk/cdk_types.h>

#ifndef CDK_TOLOWER
#define CDK_TOLOWER cdk_tolower
#endif

#ifndef CDK_TOUPPER
#define CDK_TOUPPER cdk_toupper
#endif

extern char cdk_tolower(char c);
extern char cdk_toupper(char c);

#endif /* __CDK_CTYPE_H__ */
