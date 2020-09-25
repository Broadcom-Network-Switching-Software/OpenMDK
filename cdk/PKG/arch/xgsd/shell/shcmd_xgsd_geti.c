/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * XGSD shell command GETI
 */

#include <cdk/arch/xgsd_shell.h>

#include <cdk/arch/shcmd_xgsd_geti.h>

#if CDK_CONFIG_SHELL_INCLUDE_GETI == 1

/*******************************************************************************
 *
 * Subcommand: cmic
 *
 * Get a cmic register
 */

static int
_geti_cmic(int argc, char *argv[], void *context)
{
    int unit = *((int *)context);
    uint32_t addr; 
    uint32_t data; 
    
    if (argc == 0 || cdk_shell_parse_uint32(argv[0], &addr) < 0) {
        return cdk_shell_parse_error("address", argv[0]); 
    }

    addr &= ~3;
    CDK_DEV_READ32(unit, addr, &data); 
    
    CDK_PRINTF("cmic[0x%"PRIx32"] = 0x%"PRIx32"\n", addr, data); 
    return 0;     
}

/*******************************************************************************
 *
 * Subcommand: reg
 *
 * Read and internal
 */

static int
_geti_reg(int argc, char *argv[], void *context)
{
    int unit = *((int *)context);
    int size; 
    cdk_shell_id_t sid; 

    /* Crack the identifier */
    if (cdk_shell_parse_id(*argv, &sid, 1) < 0) {
        return cdk_shell_parse_error("address", *argv); 
    }
    argv++; 
    argc--;

    /* Default size is 1 (32 bits) */
    size = 1; 

    /* Optional second argument is the size of the register */
    if (argc && (cdk_shell_parse_int(*argv, &size) < 0)) {
        return cdk_shell_parse_error("size", *argv); 
    }

    /* Only 32 and 64 bit supported for registers */
    if (size != 1 && size != 2) {
        return cdk_shell_parse_error("size", *argv); 
    }

    CDK_SPRINTF(sid.addr.name, "0x%03"PRIx32"%08"PRIx32"",
                sid.addr.ext32, sid.addr.name32); 

    /* Output all matching registers */
    return cdk_xgsd_shell_regops(unit, NULL, &sid, size, 0, NULL, NULL); 
}

static int
_geti_mem(int argc, char *argv[], void *context)
{
    int unit = *((int *)context);
    int size; 
    cdk_shell_id_t sid; 
    
    /* Crack the identifier */
    if (argc == 0 || cdk_shell_parse_id(*argv, &sid, 1) < 0) {
        return cdk_shell_parse_error("address", *argv); 
    }
    argv++; 
    argc--;
    
    /* Default size is 1 (32 bits) */
    size = 1; 

    /* Optional second argument is the memory size in words */
    if (argc && (cdk_shell_parse_int(*argv, &size) < 0)) {
        return cdk_shell_parse_error("size", *argv); 
    }

    /*
     * Memory specifications can come in a couple of formats:
     *
     * MEM
     * MEM[i0,i1]
     * MEM.blockN[i0, i1]
     * MEM.block[b0, b1].[i0,i1]
     */
    return cdk_xgsd_shell_memops(unit, NULL, &sid, size, 0, 0, NULL, NULL); 
}

static int
_geti_miim(int argc, char *argv[], void *context)
{
    int unit = *((int *)context);
    uint32_t regaddr, data; 
    cdk_shell_id_t sid; 
    int i, devad; 

    /* Crack the phy_id and addresses */
    if (argc == 0 || (cdk_shell_parse_id(*argv, &sid, 1) < 0) ||
       sid.addr.start >= 0) {
        return cdk_shell_parse_error("miim addr", *argv); 
    }
    argv++; 
    
    if (sid.port.start < 0 && sid.port.end < 0) {
        sid.port.start = 0; 
        sid.port.end = 0x1f; 
    } else if (sid.port.end < 0) {
        sid.port.end = sid.port.start; 
    }

    /* If present, treat block number as clause 45 devad */
    devad = 0;
    if (sid.block.start >= 0) {
        devad = sid.block.start;
    }

    for (i = sid.port.start; i <= sid.port.end; i++) {
        regaddr = i + (0x10000 * devad);
        if (cdk_xgsd_miim_read(unit, sid.addr.name32, regaddr, &data) < 0) {
            CDK_PRINTF("%s reading miim(0x%"PRIx32")[0x%x]\n", 
                       CDK_CONFIG_SHELL_ERROR_STR, sid.addr.name32, i); 
        } else {        
            CDK_PRINTF("miim(0x%"PRIx32")[0x%"PRIx32"] = 0x%"PRIx32"\n", 
                       sid.addr.name32, regaddr, data); 
        }
    }

    return 0; 
}
        
static cdk_shell_vect_t _geti_vects[] = 
{
    { "cmic",   _geti_cmic,     }, 
    { "reg",    _geti_reg,      }, 
    { "mem",    _geti_mem,      },     
    { "miim",   _geti_miim,     },
    { 0, 0 }, 
}; 
      

int
cdk_shcmd_xgsd_geti(int argc, char *argv[])
{
    int unit;

    unit = cdk_shell_unit_arg_extract(&argc, argv, 1);
    if(!CDK_DEV_EXISTS(unit)) {
        return CDK_SHELL_CMD_BAD_ARG;
    }
    if (cdk_shell_parse_vect(argc, argv, &unit, _geti_vects, NULL) < 0) {
        cdk_shell_parse_error("type", *argv); 
    }
    return 0; 
}

#endif /*  CDK_CONFIG_SHELL_INCLUDE_GETI */
