/*******************************************************************************
 *
 * DO NOT EDIT THIS FILE!
 * This file is auto-generated from the registers file.
 * Edits to this file will be lost when it is regenerated.
 *
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <cdk/chip/bcm56640_a0_defs.h>

/* Block types */
extern const char *bcm56640_a0_blktype_names[];

/* Block structures */
extern cdk_xgsm_block_t bcm56640_a0_blocks[];

/* Symbol table */
#if CDK_CONFIG_INCLUDE_CHIP_SYMBOLS == 1
#if CDK_CONFIG_CHIP_SYMBOLS_USE_DSYMS == 1
extern cdk_symbols_t bcm56640_a0_dsymbols;
#else
extern cdk_symbols_t bcm56640_a0_symbols;
#endif
#endif

/* Physical port numbers */
#if CDK_CONFIG_INCLUDE_PORT_MAP == 1
static cdk_port_map_port_t _ports[] = {
    0, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 57, 61, 73, 77
};
#endif

/* Chip-specific memory sizes */
static uint32_t
bcm56640_a0_mem_maxidx(int enum_val, uint32_t maxidx)
{
    switch (enum_val) {
    case VLAN_PROFILE_2m_ENUM:
    case VLAN_PROFILE_TABm_ENUM:
        return 63; /* 0x3f */
    case ING_L3_NEXT_HOP_ATTRIBUTE_1_INDEXm_ENUM:
    case MPLS_ENTRYm_ENUM:
    case MPLS_ENTRY_HIT_ONLYm_ENUM:
    case VLAN_XLATEm_ENUM:
    case VLAN_XLATE_HIT_ONLYm_ENUM:
        return 24575; /* 0x5fff */
    case MPLS_ENTRY_EXTDm_ENUM:
    case MPLS_ENTRY_EXTD_HIT_ONLYm_ENUM:
    case VLAN_XLATE_EXTDm_ENUM:
    case VLAN_XLATE_EXTD_HIT_ONLYm_ENUM:
        return 12287; /* 0x2fff */
    case L2_ENTRY_2m_ENUM:
    case L2_ENTRY_2_HIT_ONLYm_ENUM:
    case L3_ENTRY_2m_ENUM:
    case L3_ENTRY_2_HIT_ONLYm_ENUM:
        return 22527; /* 0x57ff */
    case EGR_IP_TUNNEL_IPV6m_ENUM:
        return 511; /* 0x1ff */
    case L2_ENTRY_1m_ENUM:
    case L2_ENTRY_1_HIT_ONLYm_ENUM:
    case L3_ENTRY_1m_ENUM:
    case L3_ENTRY_1_HIT_ONLYm_ENUM:
        return 45055; /* 0xafff */
    case EGR_FRAGMENT_ID_TABLEm_ENUM:
    case EGR_IP_TUNNELm_ENUM:
    case EGR_IP_TUNNEL_MPLSm_ENUM:
    case LMEPm_ENUM:
    case LMEP_DAm_ENUM:
    case MAID_REDUCTIONm_ENUM:
    case MA_STATEm_ENUM:
        return 1023; /* 0x3ff */
    case L3_DEFIPm_ENUM:
    case L3_DEFIP_ONLYm_ENUM:
        return 6143; /* 0x17ff */
    case EGR_L3_NEXT_HOPm_ENUM:
    case ING_L3_NEXT_HOPm_ENUM:
    case INITIAL_ING_L3_NEXT_HOPm_ENUM:
    case INITIAL_PROT_NHI_TABLEm_ENUM:
        return 32767; /* 0x7fff */
    case ESM_ACL_ACTION_CONTROLm_ENUM:
    case ESM_ACL_PROFILEm_ENUM:
    case EXT_L2_ENTRY_1m_ENUM:
    case EXT_L2_ENTRY_2m_ENUM:
    case EXT_L2_ENTRY_DATA_ONLYm_ENUM:
    case EXT_L2_ENTRY_DATA_ONLY_WIDEm_ENUM:
    case EXT_L2_ENTRY_TCAMm_ENUM:
    case EXT_TCAM_VBITm_ENUM:
    case ING_FLEX_CTR_COUNTER_TABLE_10m_ENUM:
    case ING_FLEX_CTR_COUNTER_TABLE_11m_ENUM:
    case ING_FLEX_CTR_COUNTER_TABLE_12m_ENUM:
    case ING_FLEX_CTR_COUNTER_TABLE_13m_ENUM:
    case ING_FLEX_CTR_COUNTER_TABLE_14m_ENUM:
    case ING_FLEX_CTR_COUNTER_TABLE_15m_ENUM:
    case ING_FLEX_CTR_COUNTER_TABLE_8m_ENUM:
    case ING_FLEX_CTR_COUNTER_TABLE_9m_ENUM:
    case ING_FLEX_CTR_OFFSET_TABLE_10m_ENUM:
    case ING_FLEX_CTR_OFFSET_TABLE_11m_ENUM:
    case ING_FLEX_CTR_OFFSET_TABLE_12m_ENUM:
    case ING_FLEX_CTR_OFFSET_TABLE_13m_ENUM:
    case ING_FLEX_CTR_OFFSET_TABLE_14m_ENUM:
    case ING_FLEX_CTR_OFFSET_TABLE_8m_ENUM:
    case ING_FLEX_CTR_OFFSET_TABLE_9m_ENUM:
        return 0; /* 0x0 */
    case ING_FLEX_CTR_COUNTER_TABLE_0m_ENUM:
    case ING_FLEX_CTR_COUNTER_TABLE_1m_ENUM:
    case ING_FLEX_CTR_COUNTER_TABLE_2m_ENUM:
    case ING_FLEX_CTR_COUNTER_TABLE_3m_ENUM:
    case ING_FLEX_CTR_COUNTER_TABLE_4m_ENUM:
    case ING_FLEX_CTR_COUNTER_TABLE_5m_ENUM:
    case ING_FLEX_CTR_COUNTER_TABLE_6m_ENUM:
    case ING_FLEX_CTR_COUNTER_TABLE_7m_ENUM:
        return 2047; /* 0x7ff */
    case EGR_DVP_ATTRIBUTEm_ENUM:
    case EGR_DVP_ATTRIBUTE_1m_ENUM:
    case ING_DVP_2_TABLEm_ENUM:
    case ING_DVP_TABLEm_ENUM:
    case SOURCE_VPm_ENUM:
    case SOURCE_VP_ATTRIBUTES_2m_ENUM:
    case SVM_MACROFLOW_INDEX_TABLEm_ENUM:
        return 16383; /* 0x3fff */
    case L3_ENTRY_4m_ENUM:
    case L3_ENTRY_4_HIT_ONLYm_ENUM:
        return 11263; /* 0x2bff */
    case ING_UNTAGGED_PHBm_ENUM:
        return 62; /* 0x3e */
    case EGR_VFIm_ENUM:
    case RMEPm_ENUM:
    case VFIm_ENUM:
    case VFI_1m_ENUM:
        return 4095; /* 0xfff */
    case EGR_IPMCm_ENUM:
    case EGR_VLAN_XLATEm_ENUM:
    case EGR_VLAN_XLATE_HIT_ONLYm_ENUM:
    case L3_IIFm_ENUM:
    case L3_IPMCm_ENUM:
    case L3_IPMC_1m_ENUM:
    case L3_IPMC_REMAPm_ENUM:
    case MMU_REPL_GROUPm_ENUM:
    case OAM_LM_COUNTERSm_ENUM:
    case VLAN_OR_VFI_MAC_COUNTm_ENUM:
    case VLAN_OR_VFI_MAC_LIMITm_ENUM:
        return 8191; /* 0x1fff */
    }
    return maxidx;
}

/* Variable register array info */
extern cdk_xgsm_numel_info_t bcm56640_a0_numel_info;

/* Chip information structure */
static cdk_xgsm_chip_info_t bcm56546_a0_chip_info = {

    /* CMIC block */
    BCM56640_A0_CMIC_BLOCK,

    /* CMC instance */
    0,

    /* Other (non-CMIC) block types */
    12,
    bcm56640_a0_blktype_names,

    /* Address calculation */
    NULL,

    /* Other (non-CMIC) blocks */
    16,
    bcm56640_a0_blocks,

    /* Valid ports for this chip */
    CDK_PBMP_3(0xfffffe01, 0x2200003f, 0x003e2200),

    /* Chip flags */
    BCM56640_A0_CHIP_FLAG_BW200G |
    BCM56640_A0_CHIP_FLAG_MMU19 |
    BCM56640_A0_CHIP_FLAG_GE28 |
    0,

#if CDK_CONFIG_INCLUDE_CHIP_SYMBOLS == 1
#if CDK_CONFIG_CHIP_SYMBOLS_USE_DSYMS == 1
    /* Use regenerated symbol tables from the DSYM program */
    &bcm56640_a0_dsymbols,
#else
    /* Use the static per-chip symbol tables */
    &bcm56640_a0_symbols,
#endif
#endif

    /* Port map */
#if CDK_CONFIG_INCLUDE_PORT_MAP == 1
    sizeof(_ports)/sizeof(_ports[0]),
    _ports,
#endif

    /* Variable register array info */
    &bcm56640_a0_numel_info,

    /* Configuration dependent memory max index */
    &bcm56640_a0_mem_maxidx,

};

/* Declare function first to prevent compiler warnings */
extern int
bcm56546_a0_setup(cdk_dev_t *dev);

int
bcm56546_a0_setup(cdk_dev_t *dev)
{
    dev->chip_info = &bcm56546_a0_chip_info;

    return cdk_xgsm_setup(dev);
}

