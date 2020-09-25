# $Id: 79d4d761412070048515d7bc1ed0bb3816c0061f $ Broadcom SDK $
# This software is governed by the Broadcom Switch APIs license.
# This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
# 
# Copyright 2007-2020 Broadcom Inc. All rights reserved.
#
# CDK top-level makefile
#

# Set default CDK directory
ifndef CDK
CDK = $(CURDIR)
export CDK
endif

include $(CDK)/make/config.mk

include $(CDK)/targets.mk

.PHONY: all instpkgs cdklibs clean cleanpkgs $(CDK_TARGETS)

ifeq ($(MAKECMDGOALS),clean)
CDK_CLEAN = clean
endif

ifndef CDK_PKG_OPTIONS
CDK_PKG_OPTIONS = $(PKG_OPTIONS)
endif

all:
	$(MAKE) instpkgs
	$(MAKE) cdklibs

cdklibs: $(CDK_TARGETS)

$(CDK_TARGETS):
	@${ECHO} Processing CDK subdirectory $@
	$(MAKE) -C $@ $(CDK_CLEAN)

relpkgs:
	@perl $(CDK)/tools/instpkgs.pl -d $(CDK) -r

instpkgs:
	perl $(CDK)/tools/instpkgs.pl -d $(CDK) $(CDK_PKG_OPTIONS)

clean:: $(CDK_TARGETS)

cleanpkgs::
	$(Q)$(RM) $(CDK)/pkgsrc/installed-chips
	$(Q)$(RM) $(CDK)/include/cdk_config_chips.h
	$(Q)$(RM) $(CDK)/include/cdk_config_archs.h
	$(Q)$(RM) $(CDK)/include/cdk_config_phys.h
	$(Q)$(RM) $(CDK)/include/cdk/chip
	$(Q)$(RM) $(CDK)/include/cdk/arch
	$(Q)$(RM) $(CDK)/include/cdk/cdk_devlist.h
	$(Q)$(RM) $(CDK)/pkgsrc/chip
	$(Q)$(RM) $(CDK)/pkgsrc/arch
	$(Q)$(RM) $(CDK)/shell/arch
	$(Q)$(RM) $(CDK)/sym/chip
	$(Q)$(RM) $(CDK)/sym/cdk_allsyms.c
ifdef DSYM
	$(MAKE) -C $(CDK)/dsym cleangen
	$(MAKE) -C $(CDK)/dsym cleantool
endif

ccver:
	@echo "CC is '$(CC)'"
	$(CC) $(CC_VERFLAGS)

