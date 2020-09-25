# $Id: d9beb34e36dc86583011254e866e349e881575a2 $ Broadcom SDK $
# This software is governed by the Broadcom Switch APIs license.
# This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
# 
# Copyright 2007-2020 Broadcom Inc. All rights reserved.
#

LOCALDIR = pkgsrc
LIBNAME = libphypkgsrc

SDIRS = $(patsubst $(PHY)/$(LOCALDIR)/%/.,%,$(wildcard $(PHY)/$(LOCALDIR)/*/.))
S2DIRS = $(foreach DIR,$(SDIRS),$(patsubst $(PHY)/$(LOCALDIR)/%/.,%,$(wildcard $(PHY)/$(LOCALDIR)/$(DIR)/*/.)))
VPATH = $(foreach DIR,$(S2DIRS),$(patsubst $(PHY)/$(LOCALDIR)/%/.,%,$(wildcard $(PHY)/$(LOCALDIR)/$(DIR)/*/.))) $(S2DIRS)
VSRCS = $(wildcard *.c $(addsuffix /*.c,$(VPATH)))
LSRCS = $(notdir $(VSRCS))
LOBJS = $(addsuffix .$(OBJSUFFIX), $(basename $(LSRCS)))
BOBJS = $(addprefix $(BLDDIR)/,$(LOBJS))

include $(CDK)/make/lib.mk

