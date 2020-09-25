# $Id: bed2ee920cb69ab62b5d5dababd7d749f8b38ef7 $ Broadcom SDK $
# This software is governed by the Broadcom Switch APIs license.
# This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
# 
# Copyright 2007-2020 Broadcom Inc. All rights reserved.
#

# CDK variable is required
ifndef CDK
nocdk:; @echo 'The $$CDK environment variable is not set'; exit 1
else

LOCALDIR = board
LIBNAME = libboard

ifdef BOARD_BLDDIR
override CDK_BLDDIR = $(BOARD_BLDDIR)
endif

ifdef BOARD_CPPFLAGS
override CDK_CPPFLAGS = $(BOARD_CPPFLAGS)
endif

ifdef BOARD_CFLAGS
override CDK_CFLAGS = $(BOARD_CFLAGS)
endif

VPATH = config xgsled
VSRCS = $(wildcard *.c $(addsuffix /*.c,$(VPATH)))
LSRCS = $(notdir $(VSRCS))
LOBJS = $(addsuffix .$(OBJSUFFIX), $(basename $(LSRCS)))
BOBJS = $(addprefix $(BLDDIR)/,$(LOBJS))

include $(CDK)/make/lib.mk

endif
