# EPOS Compiler Wraper Makefile

include	../../makedefs

all:		install

eposcc.cfg:	FORCE
		sed -e "s/=/='/;s/$$/'/" < $(ETC)/epos.cfg > $@
		@echo "MACH_CC_FLAGS='$(MACH_CC_FLAGS)'" >> $@
		@echo "MACH_LD_FLAGS='$(MACH_LD_FLAGS)'" >> $@
		@echo "MACH_CODE_NAME='$(MACH_CODE_NAME)'" >> $@
		@echo "MACH_DATA_NAME='$(MACH_DATA_NAME)'" >> $@

install:	eposcc.cfg eposcc
		$(INSTALL) eposcc.cfg $(ETC)
		$(INSTALL) -m 775 eposcc $(BIN)
		sed -e 's/^EPOS=.*$$/EPOS=$(subst /,\/,$(EPOS))/' -i $(BIN)/eposcc

clean:
		$(CLEAN) eposcc.cfg

FORCE: