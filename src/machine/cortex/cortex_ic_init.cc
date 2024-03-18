// EPOS Cortex-A Interrupt Controller Initialization

#include <architecture/cpu.h>
#include <machine.h>
#include <system/memory_map.h>

__BEGIN_SYS

extern "C" {
    void _int_entry();
#ifdef __cortex_a__
    void _int_bad();
    void _undefined_instruction();
    void _software_interrupt();
    void _prefetch_abort();
    void _data_abort();
    void _fiq();
#endif
}

void IC::init()
{
    db<Init, IC>(TRC) << "IC::init()" << endl;

    CPU::int_disable(); // will be reenabled at Thread::init() by Context::load()
    Engine::init();

    disable(); // will be enabled on demand as handlers are registered

#ifdef __cortex_m__
    // Cortex-M vector table is configured at SETUP
#else
#ifdef __armv7__
    CPU::FSR ** vt = reinterpret_cast<CPU::FSR **>(Memory_Map::VECTOR_TABLE + 32); // add 32 bytes to skip the "ldr" instructions and get to the function pointers
    vt[0] = _int_bad; // _reset is only defined for SETUP
    vt[1] = _undefined_instruction;
    vt[2] = _software_interrupt;
    vt[3] = _prefetch_abort;
    vt[4] = _data_abort;
    vt[5] = _int_bad;
    vt[6] = _int_entry;
    vt[7] = _fiq;
#endif
#ifdef __armv8__
    CPU::FSR ** vt = reinterpret_cast<CPU::FSR **>(Memory_Map::VECTOR_TABLE + 2048); // add 4 x 4 x 128 (2048) bytes to skip the vector table and reach .ic_entry.
    vt[0] = _int_entry;
#endif
#endif

    // Set all interrupt handlers to int_not()
    for(Interrupt_Id i = 0; i < INTS; i++)
        _int_vector[i] = int_not;

    // Set all EOI handlers to 0 (must be done manually, even if the variable is static, because of .hex image format used by Cortex-M)
    for(Interrupt_Id i = 0; i < INTS; i++)
        _eoi_vector[i] = 0;

#ifdef __cortex_m__
    _int_vector[IC::INT_HARD_FAULT] = int_bad;

    // There is no TSC in Cortex-M, so we use a software counter instead. It is initialized before IC, so no interrupt handler was registered at TSC::init().
    if(Traits<TSC>::enabled) {
        int_vector(INT_TSC_TIMER, TSC::int_handler, User_Timer::eoi);
        enable(INT_TSC_TIMER);
    }
#else
#ifdef __armv8__
    _int_vector[INT_PREFETCH_ABORT] = prefetch_abort;
    _int_vector[INT_DATA_ABORT] = data_abort;
#endif
#endif
}

__END_SYS
