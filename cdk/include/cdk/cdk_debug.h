/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * CDK debug message functions.
 */

#ifndef __CDK_DEBUG_H__
#define __CDK_DEBUG_H__

#include <cdk/cdk_types.h>
#include <cdk/cdk_printf.h>

#if CDK_CONFIG_INCLUDE_DEBUG == 1

/*
 * These are the possible debug types/flags for cdk_debug_level (below).
 */
#define CDK_DBG_ERR       (1 << 0)    /* Print errors */
#define CDK_DBG_WARN      (1 << 1)    /* Print warnings */
#define CDK_DBG_VERBOSE   (1 << 2)    /* General verbose output */
#define CDK_DBG_VVERBOSE  (1 << 3)    /* Very verbose output */
#define CDK_DBG_DEV       (1 << 4)    /* Device access */
#define CDK_DBG_REG       (1 << 5)    /* Register access */
#define CDK_DBG_MEM       (1 << 6)    /* Memory access */
#define CDK_DBG_SCHAN     (1 << 7)    /* S-channel operations */
#define CDK_DBG_MIIM      (1 << 8)    /* MII managment access */
#define CDK_DBG_DMA       (1 << 9)    /* DMA operations */
#define CDK_DBG_HIGIG     (1 << 10)   /* HiGig information */
#define CDK_DBG_PACKET    (1 << 11)   /* Packet data */

#define CDK_DBG_NAMES   \
    "error",            \
    "warning",          \
    "verbose",          \
    "vverbose",         \
    "device",           \
    "register",         \
    "memory",           \
    "schannel",         \
    "miim",             \
    "dma",              \
    "higig",            \
    "packet"

extern uint32_t cdk_debug_level;
extern int (*cdk_debug_printf)(const char *format, ...);

#define CDK_DEBUG_CHECK(flags) (((flags) & cdk_debug_level) == (flags))

#ifdef CDK_CC_TYPE_CHECK
/* Allow compiler to check printf arguments */
#define CDK_DEBUG(flags, stuff) \
    if (CDK_DEBUG_CHECK(flags)) \
	CDK_PRINTF stuff
#else
/* Normal definition */
#define CDK_DEBUG(flags, stuff) \
    if (CDK_DEBUG_CHECK(flags) && cdk_debug_printf != 0) \
	(*cdk_debug_printf) stuff
#endif

#define CDK_ERR(stuff) CDK_DEBUG(CDK_DBG_ERR, stuff)
#define CDK_WARN(stuff) CDK_DEBUG(CDK_DBG_WARN, stuff)
#define CDK_VERB(stuff) CDK_DEBUG(CDK_DBG_VERBOSE, stuff)
#define CDK_VVERB(stuff) CDK_DEBUG(CDK_DBG_VVERBOSE, stuff)
#define CDK_DEBUG_DEV(stuff) CDK_DEBUG(CDK_DBG_DEV, stuff)
#define CDK_DEBUG_REG(stuff) CDK_DEBUG(CDK_DBG_REG, stuff)
#define CDK_DEBUG_MEM(stuff) CDK_DEBUG(CDK_DBG_MEM, stuff)
#define CDK_DEBUG_SCHAN(stuff) CDK_DEBUG(CDK_DBG_SCHAN, stuff)
#define CDK_DEBUG_MIIM(stuff) CDK_DEBUG(CDK_DBG_MIIM, stuff)
#define CDK_DEBUG_DMA(stuff) CDK_DEBUG(CDK_DBG_DMA, stuff)
#define CDK_DEBUG_HIGIG(stuff) CDK_DEBUG(CDK_DBG_HIGIG, stuff)
#define CDK_DEBUG_PACKET(stuff) CDK_DEBUG(CDK_DBG_PACKET, stuff)

#else /* CDK_CONFIG_INCLUDE_DEBUG == 0 */

#define CDK_DEBUG_CHECK(flags) 0
#define CDK_DEBUG(flags, stuff)

#define CDK_ERR(stuff)
#define CDK_WARN(stuff)
#define CDK_VERB(stuff)
#define CDK_VVERB(stuff)
#define CDK_DEBUG_DEV(stuff)
#define CDK_DEBUG_REG(stuff)
#define CDK_DEBUG_MEM(stuff)
#define CDK_DEBUG_SCHAN(stuff)
#define CDK_DEBUG_MIIM(stuff)
#define CDK_DEBUG_DMA(stuff)
#define CDK_DEBUG_HIGIG(stuff)
#define CDK_DEBUG_PACKET(stuff)

#endif /* CDK_CONFIG_INCLUDE_DEBUG */

#endif /* __CDK_DEBUG_H__ */
