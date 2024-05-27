// EPOS Heap Utility Implementation

#include <utility/heap.h>

__BEGIN_UTIL

void Heap::out_of_memory(unsigned long bytes)
{
    db<Heaps, System>(ERR) << "Heap::alloc(this=" << this << "): out of memory while allocating " << bytes << " bytes!" << endl;
}

__END_UTIL
