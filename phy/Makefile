# $Id: ebd9ee855eb93c01f25db35430adc334e7f3e590 $ Broadcom SDK $
# This software is governed by the Broadcom Switch APIs license.
# This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
# 
# Copyright 2007-2020 Broadcom Inc. All rights reserved.
#
# PHY top-level makefile
#

# CDK variable is required
ifndef CDK
$(warning setting $$CDK automatically)
export CDK=${MDK}/cdk
endif

# Set default PHY directory
ifndef PHY
PHY = $(CURDIR)
export PHY
endif

# Default build directory
ifndef PHY_BLDDIR
PHY_BLDDIR = $(PHY)/build
endif

# Replace original CDK_BLDDIR with updated version
override MAKEOVERRIDES := $(subst CDK_BLDDIR,_BLDDIR,$(MAKEOVERRIDES))
override CDK_BLDDIR = $(PHY_BLDDIR)
export CDK_BLDDIR

# Default include path
PHY_INCLUDE_PATH = -I$(PHY)/include

# Import preprocessor flags avoiding include duplicates
TMP_PHY_CPPFLAGS := $(filter-out $(PHY_INCLUDE_PATH),$(PHY_CPPFLAGS))
TMP_PHY_CPPFLAGS += $(PHY_INCLUDE_PATH)

# Enable local configuration of PHYMOD library
TMP_PHY_CPPFLAGS += -DPHYMOD_INCLUDE_CUSTOM_CONFIG

# Import compiler flags
TMP_PHY_CFLAGS := $(PHY_CFLAGS)

include $(CDK)/make/config.mk

# Replace original CDK_CPPFLAGS with updated version
override MAKEOVERRIDES := $(subst CDK_CPPFLAGS,_CPPFLAGS,$(MAKEOVERRIDES))
override CDK_CPPFLAGS := $(TMP_PHY_CPPFLAGS)
export CDK_CPPFLAGS

# Replace original CDK_CFLAGS with updated version
override MAKEOVERRIDES := $(subst CDK_CFLAGS,_CFLAGS,$(MAKEOVERRIDES))
override CDK_CFLAGS := $(TMP_PHY_CFLAGS)
export CDK_CFLAGS

include $(PHY)/targets.mk

.PHONY: all instpkgs clean cleanpkgs $(PHY_TARGETS)

ifeq ($(MAKECMDGOALS),clean)
PHY_CLEAN = clean
endif

ifndef PHY_PKG_OPTIONS
PHY_PKG_OPTIONS = $(PKG_OPTIONS)
endif

all:
	$(MAKE) instpkgs
	$(MAKE) phylibs

phylibs: $(PHY_TARGETS)

$(PHY_TARGETS):
	@${ECHO} Processing PHY subdirectory $@
	$(MAKE) -C $@ $(PHY_CLEAN)

relpkgs:
	@perl $(PHY)/tools/instpkgs.pl -d $(PHY) -r

instpkgs:
	perl $(PHY)/tools/instpkgs.pl -d $(PHY) $(PHY_PKG_OPTIONS)

clean:: $(PHY_TARGETS)

cleanpkgs::
	$(Q)$(RM) $(PHY)/pkgsrc/installed-chips
	$(Q)$(RM) $(PHY)/include/phy_config_chips.h
	$(Q)$(RM) $(PHY)/include/phymod/chip
	$(Q)$(RM) $(PHY)/include/phy/chip
	$(Q)$(RM) $(PHY)/include/phy/bus
	$(Q)$(RM) $(PHY)/include/phy/phy_buslist.h
	$(Q)$(RM) $(PHY)/include/phy/phy_devlist.h
	$(Q)$(RM) $(PHY)/pkgsrc/chip
	$(Q)$(RM) $(PHY)/pkgsrc/bus
	$(Q)$(RM) $(PHY)/sym/chip
	$(Q)$(RM) $(PHY)/sym/phy_allsyms.c

