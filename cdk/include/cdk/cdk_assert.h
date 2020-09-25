/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * CDK assert definitions.
 */

#ifndef __CDK_ASSERT_H__
#define __CDK_ASSERT_H__

#include <cdk_config.h>

/*
 * Compile-time assertions
 */
#define CDK_COMPILE_ASSERT(expr, str) extern int str [ (expr) ? 1 : -1 ]


#if !CDK_CONFIG_INCLUDE_ASSERT

/* Global disable of CDK asserts */
#define CDK_ASSERT(expr) do { } while (0)

#else

#ifndef CDK_SYS_HAS_ASSERT
/* Provide built-in version if possible */
#if defined(__STDC__)
extern void cdk_assert(const char *expr, const char *file, int line);
#define	CDK_ASSERT(EX) (void)((EX) || (cdk_assert(#EX, __FILE__, __LINE__), 0))
#else
#error Please supply a custom version of assert() that works with your compiler.
#endif	/* __STDC__ */
#else
/* Default to system provided assert */
#define CDK_ASSERT(expr) assert(expr)
#endif /* CDK_SYS_HAS_ASSERT */

#endif /* CDK_CONFIG_INCLUDE_ASSERT */

#endif /* __CDK_ASSERT_H__ */
