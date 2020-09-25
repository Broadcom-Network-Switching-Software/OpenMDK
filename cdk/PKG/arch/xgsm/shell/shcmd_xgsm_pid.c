/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * XGSM shell command PID
 *
 * This command prints out how an input identifier is parsed. 
 * Used mainly for debugging. 
 * 
 * If you aren't getting the right information based on the input identifier, 
 * you can use this command to see if it got parsed incorrectly. 
 */

#include <cdk/arch/xgsm_shell.h>

#include <cdk/arch/shcmd_xgsm_pid.h>

#if CDK_CONFIG_SHELL_INCLUDE_PID == 1

int
cdk_shcmd_xgsm_pid(int argc, char* argv[])
{
    cdk_shell_id_t sid; 
    int unit;

    unit = cdk_shell_unit_arg_extract(&argc, argv, 1);
    if(!CDK_DEV_EXISTS(unit)) {
        return CDK_SHELL_CMD_BAD_ARG;
    }

    if(argc == 0) {
        return cdk_shell_parse_error("identifier", *argv); 
    }
    
    if(cdk_shell_parse_id(argv[0], &sid, 0) < 0) {
        return cdk_shell_parse_error("identifier", *argv); 
    }

    if(sid.addr.valid) {
        CDK_PRINTF("Address: '%s' : 0x%"PRIx32" : %d : %d\n", 
                   sid.addr.name, sid.addr.name32, 
                   sid.addr.start, sid.addr.end); 
    }
    else {
        CDK_PRINTF("Address: (invalid)\n"); 
    }

    if(sid.block.valid) {
        CDK_PRINTF("Block:   '%s' : 0x%"PRIx32" : %d : %d\n", 
                   sid.block.name, sid.block.name32, 
                   sid.block.start, sid.block.end); 
    }
    else {
        CDK_PRINTF("Block:   (invalid)\n"); 
    }

    if(sid.port.valid) {
        CDK_PRINTF("Port:    '%s' : 0x%"PRIx32" : %d : %d\n", 
                   sid.port.name, sid.port.name32, 
                   sid.port.start, sid.port.end); 
    }
    else {
        CDK_PRINTF("Port:    (invalid)\n"); 
    }
    
    return 0; 
}

#endif
