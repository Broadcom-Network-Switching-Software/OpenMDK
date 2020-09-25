/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef __CDK_STRING_H__
#define __CDK_STRING_H__

#include <cdk_config.h>

#include <cdk/cdk_types.h>

#ifndef CDK_MEMCMP
#define CDK_MEMCMP cdk_memcmp
#endif

#ifndef CDK_MEMSET
#define CDK_MEMSET cdk_memset
#endif

#ifndef CDK_MEMCPY
#define CDK_MEMCPY cdk_memcpy
#endif

#ifndef CDK_STRCPY
#define CDK_STRCPY cdk_strcpy
#endif

#ifndef CDK_STRNCPY
#define CDK_STRNCPY cdk_strncpy
#endif

#ifndef CDK_STRLEN
#define CDK_STRLEN cdk_strlen
#endif

#ifndef CDK_STRCMP
#define CDK_STRCMP cdk_strcmp
#endif

#ifndef CDK_STRNCMP
#define CDK_STRNCMP cdk_strncmp
#endif

#ifndef CDK_STRCHR
#define CDK_STRCHR cdk_strchr
#endif

#ifndef CDK_STRRCHR
#define CDK_STRRCHR cdk_strrchr
#endif

#ifndef CDK_STRSTR
#define CDK_STRSTR cdk_strstr
#endif

#ifndef CDK_STRCAT
#define CDK_STRCAT cdk_strcat
#endif

extern int cdk_memcmp(const void *dest,const void *src,size_t cnt);
extern void *cdk_memcpy(void *dest,const void *src,size_t cnt);
extern void *cdk_memset(void *dest,int c,size_t cnt);
extern char *cdk_strcpy(char *dest,const char *src);
extern char *cdk_strncpy(char *dest,const char *src,size_t cnt);
extern size_t cdk_strlen(const char *str);
extern int cdk_strcmp(const char *dest,const char *src);
extern int cdk_strncmp(const char *dest,const char *src,size_t cnt);
extern char *cdk_strchr(const char *dest,int c);
extern char *cdk_strrchr(const char *dest,int c);
extern char *cdk_strstr(const char *dest,const char *src);
extern char *cdk_strcat(char *dest,const char *src);

/* Non-standard ANSI/ISO functions */

#ifndef CDK_STRCASECMP
#define CDK_STRCASECMP cdk_strcasecmp
#endif

#ifndef CDK_STRNCASECMP
#define CDK_STRNCASECMP cdk_strncasecmp
#endif

#ifndef CDK_STRLCPY
#define CDK_STRLCPY cdk_strlcpy
#endif

#ifndef CDK_STRUPR
#define CDK_STRUPR cdk_strupr
#endif

#ifndef CDK_STRNCHR
#define CDK_STRNCHR cdk_strnchr
#endif

extern int cdk_strcasecmp(const char *dest,const char *src);
extern int cdk_strncasecmp(const char *dest,const char *src,size_t cnt);
extern size_t cdk_strlcpy(char *dest,const char *src,size_t cnt);
extern void cdk_strupr(char *s);
extern char *cdk_strnchr(const char *dest,int c,size_t cnt);

#endif /* __CDK_STRING_H__ */
