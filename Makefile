###############################################################################
#
# Release Makefile
#
# Must be executed on the exported release. 
#
###############################################################################
#
#

RELEASE = $(shell cat RELEASE)


all:
	@echo ""
	@echo " MDK Release Makefile"
	@echo ""
	@echo "    relpkgs    Release CDK, BMD, and PHY Pkgs"
	@echo ""
	@echo "    RELEASE    Performs all RELEASE tasks and deletes this makefile"
	@echo '               You must specify the $$RELDOCS directory.'
	@echo ""


relpkgs:
	@echo "CDK Release Packages..."
	@$(MAKE) -C cdk relpkgs
	@echo "BMD Release Packages..."
	@$(MAKE) -C bmd relpkgs
	@echo "PHY Release Packages..."
	@$(MAKE) -C phy relpkgs

cleandsyms:
	@echo "Removing DSYM..."
	@rm -rf cdk/tools/dsyms
	@rm -rf cdk/dsym/chips cdk/dsym/dispatch cdk/dsym/fields cdk/dsym/syms
	@mv cdk/dsym/Makefile.rel cdk/dsym/Makefile

reinst:
	@$(MAKE) -C cdk cleanpkgs instpkgs
	@$(MAKE) -C bmd cleanpkgs instpkgs
	@$(MAKE) -C phy cleanpkgs instpkgs

relinst:
	@$(MAKE) -C cdk cleanpkgs instpkgs PKG_OPTIONS="-s -g"
	@$(MAKE) -C bmd cleanpkgs instpkgs PKG_OPTIONS="-s -g"
	@$(MAKE) -C phy cleanpkgs instpkgs PKG_OPTIONS="-s -g"




ifeq ($(MAKECMDGOALS),RELEASE)
# Need $RELDOCS for relnotes
ifndef RELDOCS
$(error The $$RELDOCS location is not defined)
endif
endif

ifeq ($(MAKECMDGOALS),relnotes)
# Need $RELDOCS for relnotes
ifndef RELDOCS
$(error The $$RELDOCS location is not defined)
endif
endif


relnotes:
	@cp $(RELDOCS)/RELNOTES-MDK-$(RELEASE).html doc
	@cp $(RELDOCS)/RELNOTES-MDK-$(RELEASE).pdf doc


RELEASE: relpkgs cleandsyms relnotes
ifndef SAVEMAKE
	@echo "Removing Makefile. Release is ready."
	@rm Makefile
endif

IRELEASE:
	$(MAKE) RELEASE SAVEMAKE=1


.PHONY: RELEASE



