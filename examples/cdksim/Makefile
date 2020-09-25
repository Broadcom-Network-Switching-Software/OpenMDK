# $Id: 80c7fed80416c5665d647223290ead311d9e891a $ Broadcom SDK $
# This software is governed by the Broadcom Switch APIs license.
# This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
# 
# Copyright 2007-2020 Broadcom Inc. All rights reserved.
#
# Simple CDK shell (cdksim)
#
# General notes:
#
# This makefile is intended for POSIX compatible operating systems
# (e.g. Linux) with the GNU make utility (gmake) installed.
#
# The MDK environment variable must be set to point to the root
# of the MDK source files, e.g. (bash):
#
# export MDK=~/projects/broadcom/mdk
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
# Example invokations:
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

################################################################################
#
# Configure build environment
#
################################################################################

# Tools prefix
export CROSS_COMPILE =

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
CPPFLAGS += -I$(CDK)/include -I$(CURDIR)

# Use CDK custom config file and system C library
# (see cdk_custom_config.h for details)
CPPFLAGS += -DCDK_INCLUDE_CUSTOM_CONFIG -DUSE_SYSTEM_LIBC

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

# CDK_LIBS is defined in $(CDK)/targets.mk
include $(CDK)/targets.mk
CDK_BLIBS = $(addprefix $(CDK_BLDDIR)/,$(CDK_LIBS))

################################################################################
#
# Configure main application
#
################################################################################

OBJS = cdksim.o
BOBJS = $(addprefix $(BLDDIR)/,$(OBJS))
APPL = $(DSTDIR)/cdksim

################################################################################
#
# Make rules
#
################################################################################

.PHONY: all cdk clean cleanall

all: cdk $(APPL)

cdk:
	$(MAKE) -C $(CDK)

$(APPL): $(BOBJS) $(CDK_BLIBS)
	$(CC) $(BOBJS) $(CDK_BLIBS) $(LDFLAGS) -o $@

$(BLDDIR)/%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
	$(MAKE) -C $(CDK) clean
	rm -f $(APPL) $(BOBJS)

cleanall: clean
	$(MAKE) -C $(CDK) cleanpkgs

# endif $MDK
endif
