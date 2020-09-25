/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Basic type definitions.
 */

#ifndef __CDK_TYPES_H__
#define __CDK_TYPES_H__

#include <cdk_config.h>
#include <cdk/cdk_assert.h>

#if CDK_CONFIG_DEFINE_UINT8_T == 1
typedef CDK_CONFIG_TYPE_UINT8_T uint8_t; 
#endif

#if CDK_CONFIG_DEFINE_UINT16_T == 1
typedef CDK_CONFIG_TYPE_UINT16_T uint16_t; 
#endif

#if CDK_CONFIG_DEFINE_UINT32_T == 1
typedef CDK_CONFIG_TYPE_UINT32_T uint32_t; 
#endif

#if CDK_CONFIG_DEFINE_SIZE_T == 1
typedef CDK_CONFIG_TYPE_SIZE_T size_t; 
#endif

#if CDK_CONFIG_DEFINE_DMA_ADDR_T == 1
typedef CDK_CONFIG_TYPE_DMA_ADDR_T dma_addr_t; 
#endif

#if CDK_CONFIG_DEFINE_PRIx32 == 1
#define PRIx32 CDK_CONFIG_MACRO_PRIx32
#endif

#if CDK_CONFIG_DEFINE_PRIu32 == 1
#define PRIu32 CDK_CONFIG_MACRO_PRIu32
#endif


#ifndef NULL
#define NULL (void*)0
#endif

#ifndef STATIC
#define STATIC static
#endif

#ifndef VOLATILE
#define VOLATILE volatile
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef COUNTOF
#define COUNTOF(ary) ((int) (sizeof(ary) / sizeof((ary)[0])))
#endif

#ifndef PTR2INT
#define PTR2INT(_p) ((int)((long)(_p)))
#endif

#ifndef INT2PTR
#define INT2PTR(_i) ((void *)((long)(_i)))
#endif

#define LSHIFT32(_val, _cnt) ((uint32_t)(_val) << (_cnt))

#ifndef COMPILER_REFERENCE
#define COMPILER_REFERENCE(_a) ((void)(_a))
#endif


/* These must be moved */
#ifndef __F_MASK
#define __F_MASK(w) \
        (((uint32_t)1 << w) - 1)
#endif

#ifndef __F_GET
#define __F_GET(d,o,w) \
        (((d) >> o) & __F_MASK(w))
#endif

#ifndef __F_SET
#define __F_SET(d,o,w,v) \
        (d = ((d & ~(__F_MASK(w) << o)) | (((v) & __F_MASK(w)) << o)))
#endif

#ifndef __F_ENCODE

/* Encode a value of a given width at a given offset. Performs compile-time error checking on the value */
/* To ensure it fits within the given width */
#define __F_ENCODE(v,o,w) \
        ( ((v & __F_MASK(w)) == v) ? /* Value fits in width */ ( (uint32_t)(v) << o ) : /* Value does not fit -- compile time error */ 1 << 99)

#endif



/* 
 * Compile-time datatype checks
 */
CDK_COMPILE_ASSERT(sizeof(uint32_t) == 4, bad_uint32_t_size); 
CDK_COMPILE_ASSERT(sizeof(uint16_t) == 2, bad_uint16_t_size); 
CDK_COMPILE_ASSERT(sizeof(uint8_t) == 1, bad_uint8_t_size); 


#endif /* __CDK_TYPES_H__ */
