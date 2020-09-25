/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Common chip definitions.
 */

#ifndef __CDK_CHIP_H__
#define __CDK_CHIP_H__

#include <cdk/cdk_types.h>
#include <cdk/cdk_string.h>
#include <cdk/cdk_error.h>

/* Max size of register/memory in words */
#define CDK_MAX_REG_WSIZE       32

/* Words in port bit maps */
#define CDK_PBMP_WORD_MAX       (((CDK_CONFIG_MAX_PORTS - 1) >> 5) + 1)

typedef struct cdk_pbmp_s {
    uint32_t w[CDK_PBMP_WORD_MAX];
} cdk_pbmp_t;

/* Port bitmap helper functions */
extern int
cdk_pbmp_is_null(const cdk_pbmp_t *pbmp);

#define CDK_BMP_ITER(_bmp, _base, _iter) \
    for ((_iter) = (_base); (_iter) < (_base) + 32; (_iter++)) \
        if ((_bmp) & LSHIFT32(1, ((_iter) - (_base)))) 

#define CDK_PBMP0_ITER(_bmp, _p) CDK_BMP_ITER(_bmp, 0, _p)
#define CDK_PBMP1_ITER(_bmp, _p) CDK_BMP_ITER(_bmp, 32, _p)
#define CDK_PBMP2_ITER(_bmp, _p) CDK_BMP_ITER(_bmp, 64, _p)

#define CDK_PBMP_MEMBER(_pbmp, _port) \
     ((&(_pbmp))->w[(_port) >> 5] & LSHIFT32(1, (_port) & 0x1f))

#define CDK_PBMP_ITER(_pbmp, _port) \
    for (_port = 0; _port < CDK_CONFIG_MAX_PORTS; _port++) \
        if (CDK_PBMP_MEMBER(_pbmp, _port))

#define CDK_PBMP_PORT_ADD(_pbmp, _port) \
     ((&(_pbmp))->w[(_port) >> 5] |= LSHIFT32(1, (_port) & 0x1f))
#define CDK_PBMP_PORT_REMOVE(_pbmp, _port) \
     ((&(_pbmp))->w[(_port) >> 5] &= ~(LSHIFT32(1, (_port) & 0x1f)))

#define CDK_PBMP_CLEAR(_pbmp) CDK_MEMSET(&_pbmp, 0, sizeof(cdk_pbmp_t))

#define CDK_PBMP_WORD_GET(_pbmp, _w)            ((&(_pbmp))->w[_w])
#define CDK_PBMP_WORD_SET(_pbmp, _w, _val)      ((&(_pbmp))->w[_w]) = (_val)

#define CDK_PBMP_BMOP(_pbmp0, _pbmp1, _op) \
    do { \
        int _w; \
        for (_w = 0; _w < CDK_PBMP_WORD_MAX; _w++) { \
            CDK_PBMP_WORD_GET(_pbmp0, _w) _op CDK_PBMP_WORD_GET(_pbmp1, _w); \
        } \
    } while (0)

#define CDK_PBMP_IS_NULL(_pbmp)         (cdk_pbmp_is_null(&(_pbmp)))
#define CDK_PBMP_NOT_NULL(_pbmp)        (!(cdk_pbmp_is_null(&(_pbmp))))

#define CDK_PBMP_ASSIGN(dst, src)       CDK_MEMCPY(&(dst), &(src), sizeof(cdk_pbmp_t))
#define CDK_PBMP_AND(_pbmp0, _pbmp1)    CDK_PBMP_BMOP(_pbmp0, _pbmp1, &=)
#define CDK_PBMP_OR(_pbmp0, _pbmp1)     CDK_PBMP_BMOP(_pbmp0, _pbmp1, |=)
#define CDK_PBMP_XOR(_pbmp0, _pbmp1)    CDK_PBMP_BMOP(_pbmp0, _pbmp1, ^=)
#define CDK_PBMP_REMOVE(_pbmp0, _pbmp1) CDK_PBMP_BMOP(_pbmp0, _pbmp1, &= ~)
#define CDK_PBMP_NEGATE(_pbmp0, _pbmp1) CDK_PBMP_BMOP(_pbmp0, _pbmp1, = ~)

/* Backward compatibility */
#define CDK_PBMP_ADD(_pbmp, _port)      CDK_PBMP_PORT_ADD(_pbmp, _port)

/* Initializer macros */
#define CDK_PBMP_1(_w0)                 { { _w0 } }
#define CDK_PBMP_2(_w0, _w1)            { { _w0, _w1 } }
#define CDK_PBMP_3(_w0, _w1, _w2)       { { _w0, _w1, _w2 } }
#define CDK_PBMP_4(_w0, _w1, _w2, _w3)  { { _w0, _w1, _w2, _w3 } }
#define CDK_PBMP_5(_w0, _w1, _w2, _w3, _w4) \
                                        { { _w0, _w1, _w2, _w3, _w4 } }

#define CDK_BYTES2BITS(x)       ((x) * 8)
#define CDK_BYTES2WORDS(x)      (((x) + 3) / 4)

#define CDK_WORDS2BITS(x)       ((x) * 32)
#define CDK_WORDS2BYTES(x)      ((x) * 4)

#endif /* __CDK_CHIP_H__ */
