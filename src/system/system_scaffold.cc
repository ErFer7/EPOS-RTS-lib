// EPOS System Scaffold and System Component Implementation

#include <utility/ostream.h>
#include <utility/heap.h>
#include <machine.h>
#include <process.h>
#include <system.h>

extern char __boot_time_system_info[];

__BEGIN_SYS

// Global objects
// These objects might be reconstructed several times in multicore configurations, so their constructors must be stateless!
OStream kout;
OStream kerr;

// System class attributes
System_Info * System::_si = (Memory_Map::SYS_INFO != Memory_Map::NOT_USED) ? reinterpret_cast<System_Info *>(Memory_Map::SYS_INFO) : reinterpret_cast<System_Info *>(&__boot_time_system_info);
char System::_preheap[];
Heap * System::_heap;

__END_SYS

// Bindings
extern "C" {
    __USING_SYS;

    // Libc legacy
    void _panic() { Machine::panic(); }
    void _exit(int s) { Thread::exit(s); for(;;); }
    void __exit() { _exit(CPU::fr()); }  // must be handled by the Page Fault handler for user-level tasks
    void __cxa_pure_virtual() { db<void>(ERR) << "Pure Virtual method called!" << endl; }

    // Utility-related methods that differ from kernel and user space.
    // OStream
    void _print(const char * s) { Display::puts(s); }
    void _print_preamble() {}
    void _print_trailler(bool error) { if(error) _panic(); }
}
