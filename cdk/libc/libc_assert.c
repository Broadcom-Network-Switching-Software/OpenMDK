/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * CDK assert handler.
 */

#include <cdk/cdk_stdlib.h>
#include <cdk/cdk_printf.h>
#include <cdk/cdk_assert.h>

#ifdef __COVERITY__
extern void __coverity_panic__(void); 
#endif

void cdk_assert(const char *expr, const char *file, int line);

/*
 * Function:
 *	cdk_assert
 * Purpose:
 *	Default assertion routine.
 * Parameters:
 *      expr - stringified expression that was tested false
 *      file - source file where assertion occurred
 *      line - line number where assertion occurred
 */
void
cdk_assert(const char *expr, const char *file, int line)
{
    CDK_PRINTF("ERROR: Assertion failed: (%s) at %s:%d\n", expr, file, line);
    CDK_ABORT();

#ifdef __COVERITY__
    __coverity_panic__(); 
#endif

}
