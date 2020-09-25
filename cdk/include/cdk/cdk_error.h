/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * CDK error codes
 */

#ifndef __CDK_ERROR_H__
#define __CDK_ERROR_H__

typedef enum {
    CDK_E_NONE          = 0,
    CDK_E_INTERNAL      = -1,
    CDK_E_MEMORY        = -2,
    CDK_E_UNIT          = -3,
    CDK_E_PARAM         = -4,
    CDK_E_EMPTY         = -5,
    CDK_E_FULL          = -6,
    CDK_E_NOT_FOUND     = -7,
    CDK_E_EXISTS        = -8,
    CDK_E_TIMEOUT       = -9,
    CDK_E_BUSY          = -10,
    CDK_E_FAIL          = -11,
    CDK_E_DISABLED      = -12,
    CDK_E_BADID         = -13,
    CDK_E_RESOURCE      = -14,
    CDK_E_CONFIG        = -15,
    CDK_E_UNAVAIL       = -16,
    CDK_E_INIT          = -17,
    CDK_E_PORT          = -18,
    CDK_E_IO            = -19,

    CDK_E_LIMIT         = -20           /* Must come last */
} cdk_error_t;

#define CDK_ERRMSG_INIT { \
    "CDK_E_NONE", \
    "CDK_E_INTERNAL", \
    "CDK_E_MEMORY", \
    "CDK_E_UNIT", \
    "CDK_E_PARAM", \
    "CDK_E_EMPTY", \
    "CDK_E_FULL", \
    "CDK_E_NOT_FOUND", \
    "CDK_E_EXISTS", \
    "CDK_E_TIMEOUT", \
    "CDK_E_BUSY", \
    "CDK_E_FAIL", \
    "CDK_E_DISABLED", \
    "CDK_E_BADID", \
    "CDK_E_RESOURCE", \
    "CDK_E_CONFIG", \
    "CDK_E_UNAVAIL", \
    "CDK_E_INIT", \
    "CDK_E_PORT", \
    "CDK_E_IO", \
    "CDK_E_LIMIT" \
}

extern char *cdk_errmsg[];

#define	CDK_ERRMSG(r)		\
	cdk_errmsg[((r) <= 0 && (r) > CDK_E_LIMIT) ? -(r) : -CDK_E_LIMIT]

#define CDK_SUCCESS(rv)         ((rv) >= 0)
#define CDK_FAILURE(rv)         ((rv) < 0)


/*
 * Convenience macro to return an error if the given unit number is invalid. 
 */     
#define CDK_UNIT_CHECK(_u) do { if(!CDK_DEV_EXISTS(_u)) { return CDK_E_UNIT; } } while(0)

#endif /* __CDK_ERROR_H__ */
