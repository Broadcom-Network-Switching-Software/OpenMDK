/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * CDK utility functions
 */

#ifndef __CDK_UTIL_H__
#define __CDK_UTIL_H__

#include <cdk/cdk_types.h>

extern uint32_t
cdk_util_swap32(uint32_t u32);

extern uint32_t
cdk_util_bit_rev_by_byte_word32(uint32_t n);

extern uint32_t
cdk_util_crc32(uint32_t crc, uint8_t *data, uint32_t len);

extern int
cdk_util_bit_range(char *buf, int size, int minbit, int maxbit);

#endif /* __CDK_UTIL_H__ */
