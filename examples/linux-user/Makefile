# $Id: 2377789e29c822b6a710dd7a116e131be4b92793 $ Broadcom SDK $
# This software is governed by the Broadcom Switch APIs license.
# This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
# 
# Copyright 2007-2020 Broadcom Inc. All rights reserved.
#
# Simple Linux user mode CDK Shell
#
# General notes:
#
# This makefile is intended for POSIX compatible operating systems
# (e.g. Linux) with the GNU make utility (gmake) installed.
#
# The CDK environment variable mus be set to point to the root
# of the CDK source files, e.g. (bash):
#
# export CDK=~/projects/broadcom/cdk
#
#
# Make options:
#
# NO_PERL
# If your host system (build platform) does not have Perl installed
# you must specify NO_PERL=1 to avoid build errors.
#
# SYS_NO_TTY
# If your target system does not provide a character-based terminal
# interface, you must specify SYS_NO_TTY=1. Note that this will also
# disable advanced command line editing and history.
#
#
# Example invocations:
#
# gmake
# gmake CFLAGS="-g" BLDDIR=`pwd`/linux64 NO_PERL=1
# gmake CFLAGS="-g" BLDDIR=`pwd`/linux64 SYS_NO_TTY=1
#
#

ifndef MDK
nocdk:; @echo 'The $$MDK environment variable is not set'; exit 1
else

ifndef CDK
CDK = $(MDK)/cdk
export CDK
endif

ifndef BMD
BMD = $(MDK)/bmd
export BMD
endif

ifndef PHY
PHY = $(MDK)/phy
export PHY
endif

ifndef LIBBDE
LIBBDE = $(MDK)/libbde
export LIBBDE
endif

################################################################################
#
# Configure build environment
#
################################################################################

# Build tools
COMPILER = gcc
LINKER   = ld
ARCHIVE  = ar

# Export toolchain
export CC = $(CROSS_COMPILE)$(COMPILER)
export LD = $(CROSS_COMPILE)$(LINKER)
export AR = $(CROSS_COMPILE)$(ARCHIVE)

# Toolchain compiler flags
CFLAGS += -fPIC

# Make tools
MAKE = gmake
ECHO = echo

# Build directory
ifndef BLDDIR
BLDDIR = $(CURDIR)
endif

# Destination directory for application
ifndef DSTDIR
DSTDIR = $(BLDDIR)
endif

################################################################################
#
# Configure compiler and linker options
#
################################################################################

ifndef SYS_NO_TTY
CPPFLAGS += -DSYS_HAS_TTY
endif

# Configure include path
CPPFLAGS += -I$(CDK)/include -I$(BMD)/include -I$(PHY)/include -I$(CURDIR) -I$(LIBBDE)/include

# Use CDK custom config file and system C library
# (see cdk_custom_config.h for details)
CPPFLAGS += -DCDK_INCLUDE_CUSTOM_CONFIG -DUSE_SYSTEM_LIBC -DBMD_CONFIG_INCLUDE_DMA=0 -DBMD_SYS_USLEEP=_usleep -DPHY_SYS_USLEEP=_usleep

# Default application flags
CFLAGS += -Wall -Werror

################################################################################
#
# Configure CDK Libraries
#
#  1) Once you import $(CDK)/targets.mk, $(CDK_LIBS) will contain the list
#     of CDK libraries in correct link order.
#
#  2) To build the CDK, the following should be specified:
#
#     CDK_BLDDIR   - the directory where the libraries will be placed
#     CDK_CPPFLAGS - C preprocessor flags
#     CDK_CFLAGS   - C compiler flags
#     CDK_CC       - compiler
#     CDK_LD       - linker (currently not used)
#     CDK_AR       - archiver (for creating libraries)
#
################################################################################

# Check for Perl support
ifdef NO_PERL
export CDK_PERL   = n/a
endif
# Configure CDK libraries
export CDK_BLDDIR = $(BLDDIR)/cdk
export CDK_CFLAGS = $(CFLAGS)
export CDK_CPPFLAGS = $(CPPFLAGS)
# Configure BMD libraries
export BMD_BLDDIR = $(BLDDIR)/bmd
export BMD_CFLAGS = $(CFLAGS)
export BMD_CPPFLAGS = $(CPPFLAGS)
# Configure BMD libraries
export PHY_BLDDIR = $(BLDDIR)/phy
export PHY_CFLAGS = $(CFLAGS)
export PHY_CPPFLAGS = $(CPPFLAGS)
# Configure LIBBDE libraries
export LIBBDE_BLDDIR = $(BLDDIR)/libbde
export LIBBDE_CFLAGS = $(CFLAGS)
export LIBBDE_CPPFLAGS = $(CPPFLAGS)

# CDK_LIBS is defined in $(CDK)/targets.mk
include $(CDK)/targets.mk
CDK_BLIBS = $(addprefix $(CDK_BLDDIR)/,$(CDK_LIBS))

# BMD_LIBS is defined in $(BMD)/targets.mk
include $(BMD)/targets.mk
BMD_BLIBS = $(addprefix $(BMD_BLDDIR)/,$(BMD_LIBS))

# PHY_LIBS is defined in $(PHY)/targets.mk
include $(PHY)/targets.mk
PHY_BLIBS = $(addprefix $(PHY_BLDDIR)/,$(PHY_LIBS))

# LIBBDE_LIBS is defined in $(LIBBDE)/targets.mk
include $(LIBBDE)/targets.mk
LIBBDE_BLIBS = $(addprefix $(LIBBDE_BLDDIR)/,$(LIBBDE_LIBS))

################################################################################
#
# Configure main application
#
################################################################################

OBJS = linux-user.o linux_shbde.o
BOBJS = $(addprefix $(BLDDIR)/,$(OBJS))
APPL = $(DSTDIR)/linux-user-mdk

################################################################################
#
# Make rules
#
################################################################################

.PHONY: all cdk clean cleanall bmd phy libbde

all: cdk phy bmd libbde $(APPL)

cdk:
	$(MAKE) -C $(CDK)

bmd:
	$(MAKE) -C $(BMD)

phy:
	$(MAKE) -C $(PHY)

libbde:
	$(MAKE) -C $(LIBBDE)

$(APPL): cdk bmd phy libbde $(BOBJS) $(LIBBDE_BLIBS) $(PHY_BLIBS) $(BMD_BLIBS) $(CDK_BLIBS)
	$(CC) $(BOBJS) $(BMD_BLIBS) $(LIBBDE_BLIBS) $(PHY_BLIBS) $(CDK_BLIBS) $(OSLIBS) $(LDFLAGS) -o $@

$(BLDDIR)/%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
	$(MAKE) -C $(CDK) clean
	$(MAKE) -C $(BMD) clean
	$(MAKE) -C $(PHY) clean
	$(MAKE) -C $(LIBBDE) clean
	rm -f $(APPL) $(BOBJS)

cleanall: clean
	$(MAKE) -C $(CDK) cleanpkgs
	$(MAKE) -C $(BMD) cleanpkgs
	$(MAKE) -C $(PHY) cleanpkgs
	$(MAKE) -C $(LIBBDE) cleanpkgs

# endif $MDK
endif
