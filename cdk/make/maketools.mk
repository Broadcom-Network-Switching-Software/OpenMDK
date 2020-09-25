# $Id$
# This software is governed by the Broadcom Switch APIs license.
# This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
# 
# Copyright 2007-2020 Broadcom Inc. All rights reserved.
#
# CDK make tools
#

# Compiler command for generating dependencies
CDK_DEPEND ?= $(CC) -M -MP -P $(CPPFLAGS) $< 

# Perl is required for portable build tools
ifndef CDK_PERL
CDK_PERL = perl
endif

ifeq (n/a,$(CDK_PERL))
#
# If perl is not available, try building using standard UNIX utilities
#
RM      = rm -f
MKDIR   = mkdir -p
CP      = cp -d
ECHO    = echo
PRINTF  = printf
else
#
# If perl is available, use portable build tools
#
MKTOOL  = $(CDK_PERL) $(CDK)/tools/mktool.pl
RM      = $(MKTOOL) -rm
MKDIR   = $(MKTOOL) -md
FOREACH = $(MKTOOL) -foreach
CP      = $(MKTOOL) -cp
MAKEDEP = $(MKTOOL) -dep 
ECHO    = $(MKTOOL) -echo
PRINTF  = $(MKTOOL) -echo -n
MKBEEP  = $(MKTOOL) -beep
endif
