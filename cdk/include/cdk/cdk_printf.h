/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef __CDK_PRINTF_H__
#define __CDK_PRINTF_H__

#include <cdk_config.h>

#include <cdk/cdk_types.h>

/* System MUST supply stdarg.h */
#include <stdarg.h>

#ifdef CDK_CC_TYPE_CHECK
/* Allow compiler to check printf arguments */
#define cdk_vsnprintf vsnprintf
#define cdk_vsprintf vsprintf
#define cdk_vprintf vprintf
#define cdk_snprintf snprintf
#define cdk_sprintf sprintf
#define cdk_printf printf
#endif

#ifndef CDK_VSNPRINTF
#define CDK_VSNPRINTF cdk_vsnprintf
#endif

#ifndef CDK_VSPRINTF
#define CDK_VSPRINTF cdk_vsprintf
#endif

#ifndef CDK_VPRINTF
#define CDK_VPRINTF cdk_vprintf
#endif

#ifndef CDK_SNPRINTF
#define CDK_SNPRINTF cdk_snprintf
#endif

#ifndef CDK_SPRINTF
#define CDK_SPRINTF cdk_sprintf
#endif

#ifndef CDK_PRINTF
#define CDK_PRINTF cdk_printf
#endif

#ifndef CDK_PUTS
#define CDK_PUTS cdk_puts
#endif

#ifndef CDK_PUTCHAR
/* undefined by default */
#endif

/*
 * All printf functions that would normally print to stdout rely
 * on the CDK_PUTS macro which defaults to cdk_puts.
 * The cdk_puts function will attempt to output characters using
 * the CDK_PUTCHAR macro, which is undefined by default.
 */
extern int (*cdk_printhook)(const char *str);
extern int cdk_puts(const char *s);

extern int cdk_vsnprintf(char *buf, size_t bufsize, const char *fmt, va_list ap);
extern int cdk_vsprintf(char *buf, const char *fmt, va_list ap);
extern int cdk_vprintf(const char *fmt, va_list ap);
extern int cdk_snprintf(char *buf, size_t bufsize, const char *fmt, ...);
extern int cdk_sprintf(char *buf, const char *fmt, ...);
extern int cdk_printf(const char *fmt, ...);

/*
 * Internal use only
 */
#define CDK_VSNPRINTF_X_INF     0x7ff0

#endif /* __CDK_PRINTF_H__ */
