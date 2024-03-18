// EPOS Initializer Starting Point

#include <system.h>
#include <machine.h>

__BEGIN_SYS

// This class purpose is simply to define a well-known starting point for the initialization of the system.
// It must be linked last so init_begin becomes the first constructor in the global's constructor list.
class Init_Begin
{
public:
    Init_Begin() { Machine::pre_init(System::info()); }
};

Init_Begin init_begin;

__END_SYS
