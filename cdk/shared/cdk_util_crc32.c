/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * CDK utility for byte swapping
 */

#include <cdk/cdk_types.h>
#include <cdk/cdk_util.h>

/*
 * Ethernet CRC Algorithm
 *
 * To generate CRC, do not include CRC field in data:
 *    uint32_t crc = ~cdk_util_crc32(~0, data, len)
 *
 * To check CRC, include CRC field in data:
 *    uint32_t check = cdk_util_crc32(~0, data, len)
 *    If CRC is correct, result will be _CDK_CRC32_CORRECT.
 *
 * NOTE: This routine generates the same 32-bit value whether the
 * platform is big- or little-endian.  The value must be stored into a
 * network packet in big-endian order, i.e. using htonl() or equivalent.
 * (Polynomial x ^ 32 + x ^ 28 + x ^ 23 + x ^ 22 + x ^ 16 + x ^ 12 + x ^ 11 +
 *             x ^ 10 + x ^ 8 + x ^ 7 + x ^ 5 + x ^ 4 + x ^ 2  + x ^ 1 + 1)
 */

static int _cdk_crc_table_created;
static uint32_t _cdk_crc_table[256];

uint32_t
cdk_util_crc32(uint32_t crc, uint8_t *data, uint32_t len)
{
    uint32_t i, j, accum;

    if (!_cdk_crc_table_created) {
	for (i = 0; i < 256; i++) {
	    accum = i;
	    for (j = 0; j < 8; j++) {
		if (accum & 1) {
		    accum = accum >> 1 ^ 0xedb88320UL;
		} else {
		    accum = accum >> 1;
		}
	    }
	    _cdk_crc_table[i] = cdk_util_swap32(accum);
	}
	_cdk_crc_table_created = 1;
    }

    for (i = 0; i < len; i++) {
	crc = crc << 8 ^ _cdk_crc_table[crc >> 24 ^ data[i]];
    }

    return crc;
}
