/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

extern int bmd_shcmd_vlan(int argc, char *argv[]);

#define BMD_SHCMD_VLAN_DESC "Manage VLANs"
#define BMD_SHCMD_VLAN_SYNOP \
"action vlan [ports [untag]]"
#define BMD_SHCMD_VLAN_HELP \
"Valid actions are create, destroy, add, remove and show. When adding\n" \
"ports to a VLAN, the untag option can be used to add the ports\n" \
"as untagged, which means that the VLAN tag will be stripped from\n" \
"egressing packets.\n\n" \
"Examples:\n\n" \
"vlan create 2\n" \
"vlan add 2 1-24 untag\n" \
"vlan remove 2 1-24\n" \
"vlan destroy 2\n" \
"vlan show"
