# $Id: ceef4e6f654771650b061a15feb0d3e4ddc99bbf $ Broadcom SDK $
# This software is governed by the Broadcom Switch APIs license.
# This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
# 
# Copyright 2007-2020 Broadcom Inc. All rights reserved.
#

ifndef CDK_MAKE_DSYMS
CDK_MAKE_DSYMS = 0
endif

ifeq ($(CDK_MAKE_DSYMS),0)

# Build and empty library

LOCALDIR = dsym
LIBNAME = libcdkdsym
include $(CDK)/make/lib.mk

cleangen: 

cleantool:

else

DSYMTOOLDIR = $(CDK)/tools/dsyms
DSYMTOOL = $(DSYMTOOLDIR)/dsyms

# Normal cleaning at stage 2
ifneq (,$(findstring clean,$(MAKECMDGOALS)))
STAGE = 2
endif

ifndef STAGE
start:
	@echo Building DSYM Tool...
	$(MAKE) -C $(DSYMTOOLDIR)
	$(MAKE) STAGE=1
endif

ifeq ($(STAGE),1)

start: gentag
	$(MAKE) STAGE=2

endif

ifeq ($(STAGE),2)

LOCALDIR = dsym
LIBNAME = libcdkdsym

VPATH += $(notdir $(wildcard $(CDK)/$(LOCALDIR)/*))
VSRCS = $(wildcard *.c $(addsuffix /*.c,$(VPATH)))
LSRCS = $(notdir $(VSRCS))
LOBJS = $(addsuffix .$(OBJSUFFIX), $(basename $(LSRCS)))
BOBJS = $(addprefix $(BLDDIR)/,$(LOBJS))

include $(CDK)/make/lib.mk

endif

cleangen:
	@find . -name "*.[c,h]" -exec rm {} \; 
	@rm -f gentag

cleantool:
	$(MAKE) -C $(CDK)/tools/dsyms clean

gentag: $(DSYMTOOL)
	@echo Generating Files...
	$(DSYMTOOL) ot=single od=$(CDK) $(DSYM_GENERATION_OPTIONS)
	@echo Done.
	@touch gentag
	$(MAKE) STAGE=2

$(DSYMTOOL):
	$(MAKE) -C $(DSYMTOOLDIR)

.PRECIOUS: gentag

endif

.PHONY: cleangen cleantool
