# $Id$
# This software is governed by the Broadcom Switch APIs license.
# This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
# 
# Copyright 2007-2020 Broadcom Inc. All rights reserved.
#
# CDK default targets.
#
# This file may be included from the application makefile as well.
#

CDK_DEFAULT_TARGETS = shell main pkgsrc shared sym libc dsym

ifndef CDK_TARGETS
CDK_TARGETS = $(CDK_DEFAULT_TARGETS)
endif

CDK_LIBNAMES = $(addprefix libcdk,$(CDK_TARGETS))

ifndef CDK_LIBSUFFIX
CDK_LIBSUFFIX = a
endif

CDK_LIBS = $(addsuffix .$(CDK_LIBSUFFIX),$(CDK_LIBNAMES))
