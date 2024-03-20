# EPOS Application Makefile

include ../makedefs

all:		$(APPLICATION)

$(APPLICATION):	FORCE
		(cd $@ && $(MAKE))

clean:
		make MAKE:="$(MAKECLEAN)" $(APPLICATION)

FORCE:
		