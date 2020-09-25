# $Id: bc25224e8e13af4cfa8739f086e2548b2963f0cd $ Broadcom SDK $
# This software is governed by the Broadcom Switch APIs license.
# This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
# 
# Copyright 2007-2020 Broadcom Inc. All rights reserved.
#

LOCALDIR = shell
LIBNAME = libcdkshell

VPATH += arch
VSRCS = $(wildcard *.c $(addsuffix /*.c,$(VPATH)))
LSRCS = $(notdir $(VSRCS))
LOBJS = $(addsuffix .$(OBJSUFFIX), $(basename $(LSRCS)))
BOBJS = $(addprefix $(BLDDIR)/,$(LOBJS))

include $(CDK)/make/lib.mk

