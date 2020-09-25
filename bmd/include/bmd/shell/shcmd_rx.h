/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

extern int bmd_shcmd_rx(int argc, char *argv[]);

#define BMD_SHCMD_RX_DESC "Receive packets"
#define BMD_SHCMD_RX_SYNOP \
"start\n" \
"poll\n" \
"drain\n" \
"stop"
#define BMD_SHCMD_RX_HELP \
"Use rx start to submit a packet buffer to the DMA engine, and\n" \
"then use rx poll to check for incoming packets. Once a packet\n" \
"has been received, rx start must be called again to re-submit\n" \
"the packet buffer. For convenience, calling rx with no arguments\n" \
"will do a poll, and if a packet was received, the packet buffer\n" \
"will be re-submitted automatically. The rx drain command will\n" \
"repeat this process until all packets have been drained from\n" \
"the CPU receive buffer."
