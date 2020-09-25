# $Id: 8043d6e44d18e2512fcb01a9a2c2f55b0b77d6be $ Broadcom SDK $
# This software is governed by the Broadcom Switch APIs license.
# This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
# 
# Copyright 2007-2020 Broadcom Inc. All rights reserved.
#

LOCALDIR = pkgsrc
LIBNAME = libcdkpkgsrc

SDIRS = $(notdir $(wildcard *))
VPATH += $(foreach DIR,$(SDIRS),$(addprefix $(DIR)/,$(notdir $(wildcard $(DIR)/*))))
VSRCS = $(wildcard *.c $(addsuffix /*.c,$(VPATH)))
LSRCS = $(notdir $(VSRCS))
LOBJS = $(addsuffix .$(OBJSUFFIX), $(basename $(LSRCS)))
BOBJS = $(addprefix $(BLDDIR)/,$(LOBJS))

include $(CDK)/make/lib.mk

