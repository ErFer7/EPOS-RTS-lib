// EPOS System/Application Containers and Dynamic Memory Allocators Declarations

#ifndef __system_h
#define __system_h

#include <utility/string.h>
#include <utility/heap.h>
#include <system/info.h>

__BEGIN_SYS

class Application
{
    friend class Init_Application;
    friend void * ::malloc(size_t);
    friend void ::free(void *);

private:
    static void init();

private:
    static char _preheap[sizeof(Heap)];
    static Heap * _heap;
};

class System
{
    friend class Init_System;                        // for _heap
    friend void CPU::Context::load() const volatile;
    friend void * kmalloc(size_t);                   // for _heap
    friend void kfree(void *);                       // for _heap

public:
    static System_Info * const info() { assert(_si); return _si; }

private:
    static void init();

private:
    static System_Info * _si;
    static char _preheap[sizeof(Heap)];
    static Heap * _heap;
};

inline void * kmalloc(size_t bytes) {
    return System::_heap->alloc(bytes);
}

inline void kfree(void * ptr) {
    System::_heap->free(ptr);
}

__END_SYS

extern "C"
{
    // Standard C Library allocators
    inline void * malloc(size_t bytes) {
        __USING_SYS;
        return Application::_heap->alloc(bytes);
    }

    inline void * calloc(size_t n, unsigned int bytes) {
        void * ptr = malloc(n * bytes);
        memset(ptr, 0, n * bytes);
        return ptr;
    }

    inline void free(void * ptr) {
        __USING_SYS;
        Application::_heap->free(ptr);
    }
}

// C++ dynamic memory allocators and deallocators
inline void * operator new(size_t bytes) {
    return malloc(bytes);
}

inline void * operator new[](size_t bytes) {
    return malloc(bytes);
}

// Delete cannot be declared inline due to virtual destructors
void operator delete(void * ptr);
void operator delete[](void * ptr);
void operator delete(void * ptr, size_t bytes);
void operator delete[](void * ptr, size_t bytes);

#endif
