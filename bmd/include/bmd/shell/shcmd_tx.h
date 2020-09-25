/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

extern int bmd_shcmd_tx(int argc, char *argv[]);

#define BMD_SHCMD_TX_DESC "Transmit packet"
#define BMD_SHCMD_TX_SYNOP \
"count [port] [size=n] [shdr=xx:xx ... ] [dmac=mac] [smac=mac] [vlan=n] [untag]"
#define BMD_SHCMD_TX_HELP \
"Transmit packet from the CPU. By default a valid Ethernet packet\n" \
"of 68 bytes (incl. CRC and VLAN tag) will be sent. If the port\n"\
"parameter is omitted, the packet will be ingressed on the CPU port\n" \
"if supported by the switch device.\n\n" \
"Use the size parameter to change the packet size and the dmac/smac\n" \
"parameters to change the destination MAC and source MAC addresses.\n" \
"The MAC address must be specified as 6 hex bytes separated by\n" \
"colons, e.g. 00:01:02:03:04:05.\n"
#define BMD_SHCMD_TX_HELP_2 \
"By default the packet will contain a valid VLAN tag. Use the untag\n" \
"parameter to send the packet untagged.\n\n" \
"The shdr parameter is used to prepend the packet with a stacking\n" \
"header when a packet is sent out on e.g. a HiGig port. The header\n" \
"is specified as hex bytes (up to 16) separated by colons. The number\n" \
"of bytes to specify depends on the stacking protocol, but is typically\n" \
"12 bytes for an XGS HiGig packet.\n\n" \
"Note that the size, dmac, smac and vlan parameters are sticky."

