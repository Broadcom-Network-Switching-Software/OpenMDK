# $Id: 222b6d9daf9f76dcc131de19269f7f457b3d531e $ Broadcom SDK $
# This software is governed by the Broadcom Switch APIs license.
# This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
# 
# Copyright 2007-2020 Broadcom Inc. All rights reserved.
#

LOCALDIR = pkgsrc
LIBNAME = libbmdpkgsrc

SDIRS = $(notdir $(wildcard $(BMD)/$(LOCALDIR)/*))
VPATH += $(foreach DIR,$(SDIRS),$(addprefix $(DIR)/,$(notdir $(wildcard $(BMD)/$(LOCALDIR)/$(DIR)/*))))
VSRCS = $(wildcard *.c $(addsuffix /*.c,$(VPATH)))
LSRCS = $(notdir $(VSRCS))
LOBJS = $(addsuffix .$(OBJSUFFIX), $(basename $(LSRCS)))
BOBJS = $(addprefix $(BLDDIR)/,$(LOBJS))

include $(CDK)/make/lib.mk

