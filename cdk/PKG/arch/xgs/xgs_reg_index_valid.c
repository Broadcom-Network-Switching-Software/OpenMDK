/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <cdk/arch/xgs_reg.h>

int cdk_xgs_reg_index_valid(int unit, int port, int regidx,
                            int encoding, int regidx_max)
{
    cdk_xgs_numel_info_t *numel_info = CDK_XGS_INFO(unit)->numel_info;
    cdk_xgs_numel_range_t *numel_range;
    int idx;
    int *range_id;

    if (encoding > 0 && numel_info != NULL) {
        /* The first range ID of encoding 0 is the number of valid encodings */
        if (encoding < numel_info->encodings[0].range_id[0]) {
            range_id = numel_info->encodings[encoding].range_id;
            idx = 0;
            while (range_id[idx] >= 0) {
                numel_range = &numel_info->chip_ranges[range_id[idx]];
                if (regidx >= numel_range->min && regidx <= numel_range->max
                    && CDK_PBMP_MEMBER(numel_range->pbmp, port)) {
                    return 1;
                }
                idx++;
            }
        }
    } else if (regidx <= regidx_max || regidx_max < 0) {
        return 1;
    }
    return 0;
}
