# EPOS Configuration Makefile

# Functions
GETTK		= $(shell sed -n -e '/^.* $(1)[ ]*=.*;.*$$/s/^.* =[ ]*\(.*\)[ ]*;.*$$/\1/p' $(2) | head -1 2> /dev/null)
DBSEC		= $(1) 0x$(shell objdump -h $(1) | grep $(2) | tr -s ' ' | cut -d ' ' -f 5 2> /dev/null)
TOLOWER		= $(shell echo $(1) | tr A-Z a-z)

# Basic system configuration
PREFIX		= $($(ARCH)_PREFIX)
SMOD		= $(call TOLOWER,$(call GETTK,SMOD,$(TRAITS)))
ARCH		= $(call TOLOWER,$(call GETTK,ARCHITECTURE,$(TRAITS)))
MACH		= $(call TOLOWER,$(call GETTK,MACHINE,$(TRAITS)))
MMOD		= $(call TOLOWER,$(call GETTK,MODEL,$(TRAITS)))
NETW		= $(call TOLOWER,$(call GETTK,NETWORKING,$(TRAITS)))

# Targets
all: $(APP)/$(APPLICATION)/$(APPLICATION)_traits.h config eposcfg epos.cfg

$(APP)/$(APPLICATION)/$(APPLICATION)_traits.h:
		$(error Missing traits file $@ !)

config: FORCE
		sed -e 's/^#define SMOD.*$$/#define SMOD $(SMOD)/' -i $(INC)/system/config.h
		sed -e 's/^#define ARCH.*$$/#define ARCH $(ARCH)/' -i $(INC)/system/config.h
		sed -e 's/^#define MACH.*$$/#define MACH $(MACH)/' -i $(INC)/system/config.h
		sed -e 's/^#define MMOD.*$$/#define MMOD $(MMOD)/' -i $(INC)/system/config.h
		sed -e 's/^#define NETW.*$$/#define NETW $(NETW)/' -i $(INC)/system/config.h
		sed -e 's/^#define APPL.*$$/#define APPL $(APPLICATION)/' -i $(INC)/system/config.h
		sed -e 's/^#define __mode_.*__$$/#define __$(SMOD)__/' -i $(INC)/system/config.h
		sed -e 's/^#define __arch_.*__$$/#define __$(ARCH)__/' -i $(INC)/system/config.h
		sed -e 's/^#define __mach_.*__$$/#define __$(MACH)__/' -i $(INC)/system/config.h
		sed -e 's/^#define __mmod_.*__$$/#define __$(MMOD)__/' -i $(INC)/system/config.h
		sed -e 's/^#define __netw_.*__$$/#define __$(NETW)__/' -i $(INC)/system/config.h

eposcfg: FORCE
		(cd $(TLS)/eposcfg && $(MAKE) clean all)

epos.cfg: FORCE
		$(shell $(BIN)/eposcfg -f $(ETC)/$@)
		@echo "NODE_ID=-1" >> $@
		@echo "UUID=$(shell cat /proc/sys/kernel/random/uuid | sed 's/-//g')" >> $@
		@echo "PREFIX=$(PREFIX)" >> $@

clean: $(APP)/$(APPLICATION)/$(APPLICATION)_traits.h
		sed -e '/^#define SMOD.*$$/{:1;/^#define.*APPL.*$$/!{N;N;N;N;N;N;N;N;N;N;b 1}' \
		        -e 's/.*/#define SMOD xxx\n#define ARCH xxx\n#define MACH xxx\n#define MMOD xxx\n#define NETW xxx\n#define APPL xxx\n#define __mode_xxx__\n#define __arch_xxx__\n#define __mach_xxx__\n#define __mmod_xxx__\n#define __netw_xxx__/g}' -i $(INC)/system/config.h
		$(CLEAN) *.cfg

FORCE:
