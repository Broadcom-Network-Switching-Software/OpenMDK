# $Id: 68985ea44e6c077d79fe16585f75c1dbe04421a5 $ Broadcom SDK $
# This software is governed by the Broadcom Switch APIs license.
# This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
# 
# Copyright 2007-2020 Broadcom Inc. All rights reserved.
#
# BMD top-level makefile
#

# CDK variable is required
ifndef CDK
$(warning setting $$CDK automatically)
export CDK=${MDK}/cdk
endif

# Set default BMD directory
ifndef BMD
BMD = $(CURDIR)
export BMD
endif

# Default build directory
ifndef BMD_BLDDIR
BMD_BLDDIR = $(BMD)/build
endif

# Replace original CDK_BLDDIR with updated version
override MAKEOVERRIDES := $(subst CDK_BLDDIR,_BLDDIR,$(MAKEOVERRIDES))
override CDK_BLDDIR = $(BMD_BLDDIR)
export CDK_BLDDIR

# Default include path
BMD_INCLUDE_PATH = -I$(BMD)/include -I$(CDK)/include

# Import preprocessor flags avoiding include duplicates
TMP_BMD_CPPFLAGS := $(filter-out $(BMD_INCLUDE_PATH),$(BMD_CPPFLAGS))
TMP_BMD_CPPFLAGS += $(BMD_INCLUDE_PATH)

# Import compiler flags
TMP_BMD_CFLAGS := $(BMD_CFLAGS)

include $(CDK)/make/config.mk

# Replace original CDK_CPPFLAGS with updated version
override MAKEOVERRIDES := $(subst CDK_CPPFLAGS,_CPPFLAGS,$(MAKEOVERRIDES))
override CDK_CPPFLAGS := $(TMP_BMD_CPPFLAGS)
export CDK_CPPFLAGS

# Replace original CDK_CFLAGS with updated version
override MAKEOVERRIDES := $(subst CDK_CFLAGS,_CFLAGS,$(MAKEOVERRIDES))
override CDK_CFLAGS := $(TMP_BMD_CFLAGS)
export CDK_CFLAGS

include $(BMD)/targets.mk

.PHONY: all instpkgs clean cleanpkgs $(BMD_TARGETS)

ifeq ($(MAKECMDGOALS),clean)
BMD_CLEAN = clean
endif

ifndef BMD_PKG_OPTIONS
BMD_PKG_OPTIONS = $(PKG_OPTIONS)
endif

all:
	$(MAKE) instpkgs
	$(MAKE) bmdlibs

bmdlibs: $(BMD_TARGETS)

$(BMD_TARGETS):
	@${ECHO} Processing BMD subdirectory $@
	$(MAKE) -C $@ $(BMD_CLEAN)

relpkgs:
	@perl $(BMD)/tools/instpkgs.pl -d $(BMD) -r

instpkgs:
	perl $(BMD)/tools/instpkgs.pl -d $(BMD) $(BMD_PKG_OPTIONS)

clean:: $(BMD_TARGETS)

cleanpkgs::
	$(Q)$(RM) $(BMD)/include/bmdi/arch
	$(Q)$(RM) $(BMD)/include/bmdi/bmd_devlist.h
	$(Q)$(RM) $(BMD)/pkgsrc/chip
	$(Q)$(RM) $(BMD)/pkgsrc/arch
	$(Q)$(RM) $(BMD)/pkgsrc/installed-chips
