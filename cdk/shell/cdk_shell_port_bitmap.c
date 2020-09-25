/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <cdk/cdk_chip.h>
#include <cdk/cdk_printf.h>
#include <cdk/cdk_shell.h>

/*
 * Function:
 *	cdk_shell_bit_range
 * Purpose:
 *	Create bit range string
 * Parameters:
 *	buf - (OUT) output buffer for bit range string
 *	size - size of output buffer
 *	pbmp - port bitmap to print
 *	mask - port bitmap that determines how many words to print
 * Returns:
 *      Always 0
 */
int
cdk_shell_port_bitmap(char *buf, int size,
                      const cdk_pbmp_t *pbmp, const cdk_pbmp_t *mask)
{
    int idx, pbm_no, len;
    const char *cp;
    uint32_t pbm;

    /* Start with an empty string */
    buf[0] = 0;
    len = 0;

    if (pbmp  == NULL || mask == NULL) {
        return -1;
    }

    pbm_no = 0;
    for (idx = (CDK_PBMP_WORD_MAX - 1); idx >= 0; idx--) {
        pbm = CDK_PBMP_WORD_GET(*mask, idx);
        if (pbm || pbm_no) {
            pbm_no++;
        }
        if (pbm_no == 0) {
            continue;
        }
        if (pbm_no == 1) {
            cp = "0x";
        } else {
            cp = "_";
        }
        if ((size - len) > 0) {
            len += CDK_SNPRINTF(&buf[len], size - len, cp); 
        }
        pbm = CDK_PBMP_WORD_GET(*pbmp, idx);
        if ((size - len) > 0) {
            len += CDK_SNPRINTF(&buf[len], size - len, "%08"PRIx32"", pbm);
        }
    }

    /* Ensure string is properly terminated */
    buf[size - 1] = 0;

    return 0;
}
