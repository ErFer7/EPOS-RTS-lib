// EPOS RISC-V IC Mediator Implementation

#include <architecture.h>
#include <machine/machine.h>
#include <machine/ic.h>
#include <machine/timer.h>
#include <process.h>

extern "C" { static void print_context(bool push); }

__BEGIN_SYS

PLIC::Reg32 PLIC::_claimed;
IC::Interrupt_Handler IC::_int_vector[IC::INTS];

void IC::entry()
{
    // Save context into the stack
    CPU::Context::push(true);

    if(Traits<IC>::hysterically_debugged)
        print_context(true);

    dispatch();

    if(Traits<IC>::hysterically_debugged)
        print_context(false);

    // Restore context from the stack
    CPU::Context::pop(true);
    CPU::iret();
}

void IC::dispatch()
{
    Interrupt_Id id = int_id();

    if(((id != INT_SYS_TIMER) && (id != INT_SYSCALL) && ((id == CPU::EXC_IPF) && (CPU::epc() != CPU::Log_Addr(&__exit)))) || Traits<IC>::hysterically_debugged)
        db<IC, System>(TRC) << "IC::dispatch(i=" << id << ") [cpu=" << CPU::id() << ", sp=" << CPU::sp() << "]" << endl;


    if(id == INT_RESCHEDULER)
        IC::ipi_eoi(id & CLINT::INT_MASK);
    else if(id == INT_SYS_TIMER)
        Timer::reset();             // MIP.MTI is a direct logic on (MTIME == MTIMECMP) and reseting the Timer seems to be the only way to clear it

    _int_vector[id](id);

    if(id > HARD_INT) {
        complete(int2irq(id));
        db<IC, System>(TRC) << "IC::dispatch(i=" << id << ") => interrupt handled!" << endl;
    }

    if(id >= EXCS)
        CPU::fr(0); // tell CPU::Context::pop(true) not to increment PC since it is automatically incremented for hardware interrupts
}

void IC::int_not(Interrupt_Id id)
{
    db<IC>(WRN) << "IC::int_not(i=" << id << ")" << endl;
    if(Traits<Build>::hysterically_debugged)
        Machine::panic();
}

void IC::exception(Interrupt_Id id)
{
    CPU::Log_Addr ksp = CPU::sp();
    CPU::Log_Addr usp = multitask ? CPU::sscratch() : 0;
    CPU::Log_Addr atp = 0;
    CPU::Log_Addr epc = CPU::epc();
    CPU::Reg status = CPU::status();
    CPU::Reg cause = CPU::cause();
    CPU::Log_Addr tval = CPU::tval();
    Thread * thread = Thread::self();
    
    db<IC,System>(WRN) << "IC::Exception(" << id << ") => {" << hex << "thread=" << thread << ",atp=" << atp << ",usp=" << usp << ",ksp=" << ksp << ",status=" << status << ",cause=" << cause << ",epc=" << epc << ",tval=" << tval << "}" << dec;

    switch(id) {
    case CPU::EXC_IALIGN: // instruction address misaligned
        db<IC, System>(WRN) << " => unaligned instruction";
        break;
    case CPU::EXC_IFAULT: // instruction access fault
        db<IC, System>(WRN) << " => instruction protection violation";
        break;
    case CPU::EXC_IILLEGAL: // illegal instruction
        db<IC, System>(WRN) << " => illegal instruction";
        break;
    case CPU::EXC_BREAK: // break point
        db<IC, System>(WRN) << " => break point";
        break;
    case CPU::EXC_DRALIGN: // load address misaligned
        db<IC, System>(WRN) << " => unaligned data read";
        break;
    case CPU::EXC_DRFAULT: // load access fault
        db<IC, System>(WRN) << " => data protection violation (read)";
        break;
    case CPU::EXC_DWALIGN: // store/AMO address misaligned
        db<IC, System>(WRN) << " => unaligned data write";
        break;
    case CPU::EXC_DWFAULT: // store/AMO access fault
        db<IC, System>(WRN) << " => data protection violation (write)";
        break;
    case CPU::EXC_ENVU: // ecall from user-mode
    case CPU::EXC_ENVS: // ecall from supervisor-mode
    case CPU::EXC_ENVH: // reserved
    case CPU::EXC_ENVM: // reserved
        db<IC, System>(WRN) << " => bad ecall";
        break;
    case CPU::EXC_IPF: // Instruction page fault
        db<IC, System>(WRN) << " => instruction page fault";
        break;
    case CPU::EXC_DRPF: // load page fault
    case CPU::EXC_RES: // reserved
    case CPU::EXC_DWPF: // store/AMO page fault
        if(ksp < thread->_stack)
            db<IC, Thread>(WRN) << " => kernel stack underflow";
        else if(ksp > thread->_stack + thread->STACK_SIZE)
            db<IC, Thread>(WRN) << " => kernel stack overflow";
        else
            db<IC, System>(WRN) << " => data page fault";
        break;
    default:
        int_not(id);
        break;
    }

    db<IC, System>(WRN) << endl;

    if(Traits<Build>::hysterically_debugged)
        db<IC, System>(ERR) << "Exception stopped execution due to hysterically debugging!" << endl;
    else {
        db<IC, Machine>(WRN) << "The running thread will now be terminated!" << endl;
        Thread::exit(-1);
    }

    CPU::fr(4); // since exceptions do not increment PC, tell CPU::Context::pop(true) to perform PC = PC + 4 on return
}

__END_SYS

static void print_context(bool push) {
    __USING_SYS
    db<IC, System>(TRC) << "IC::entry:" << (push ? "push" : "pop") << ":ctx=" << *static_cast<CPU::Context *>(CPU::sp() + 3 * sizeof(CPU::Reg) + (push ? sizeof(CPU::Context) : 0)) << endl; // 3 words for function's stack frame
    CPU::fr(0);
}

