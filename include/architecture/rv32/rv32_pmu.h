// EPOS RV32 PMU Mediator Declarations

#ifndef __rv32_pmu_h
#define __rv32_pmu_h

#include <architecture/cpu.h>
#define __pmu_common_only__
#include <architecture/pmu.h>
#undef __pmu_common_only__

__BEGIN_SYS

class RV32_PMU: public PMU_Common
{
private:
    typedef CPU::Reg Reg;

protected:
    static const unsigned int COUNTERS = 32;
    static const unsigned int CHANNELS = 32;
    static const unsigned int FIXED    = 3;

public:
    enum {
        // Instruction Commit events (mhpmeventX[7:0] = 0)
        CYCLES                                          = 0,
        TIME                                            = 1,
        INSTRUCTIONS_RETIRED                            = 2,
        EXCEPTIONS_TAKEN                                = 1 << 8,
        INTEGER_LOAD_INSTRUCTIONS_RETIRED               = 1 << 9,
        INTEGER_STORE_INSTRUCTIONS_RETIRED              = 1 << 10,
        ATOMIC_MEMEMORY_INSTRUCTIONS_RETIRED            = 1 << 11,
        SYSTEM_INSTRUCTIONS_RETIRED                     = 1 << 12,
        INTEGER_ARITHMETIC_INSTRUCTIONS_RETIRED         = 1 << 13,
        CONDITIONAL_BRANCHES_RETIRED                    = 1 << 14,
        JAL_INSTRUCTIONS_RETIRED                        = 1 << 15,
        JALR_INSTRUCTIONS_RETIRED                       = 1 << 16,
        INTEGER_MULTIPLICATION_INSTRUCTIONS_RETIRED     = 1 << 17,
        INTEGER_DIVISION_INSTRUCTIONS_RETIRED           = 1 << 18,

        // Microarchitectural events (mhpmeventX[7:0] = 1)
        LOAD_USE_INTERLOCK                              = 1 <<  8 | 1,
        LONG_LATENCY_INTERLOCK                          = 1 <<  9 | 1,
        CSR_READ_INTERLOCK                              = 1 << 10 | 1,
        INSTRUCTION_CACHE_ITIM_BUSY                     = 1 << 11 | 1,
        DATA_CACHE_DTIM_BUSY                            = 1 << 12 | 1,
        BRANCH_DIRECTION_MISPREDICTION                  = 1 << 13 | 1,
        BRANCH_JUMP_TARGET_MISPREDICTION                = 1 << 14 | 1,
        PIPELINE_FLUSH_FROM_CSR_WRITE                   = 1 << 15 | 1,
        PIPELINE_FLUSH_FROM_OTHER_EVENT                 = 1 << 16 | 1,
        INTEGER_MULTIPLICATION_INTERLOCK                = 1 << 17 | 1,

        // Memory System events (mhpmeventX[7:0] = 2)
        INSTRUCTION_CACHE_MISS                          = 1 <<  8 | 2,
        MEMORY_MAPPED_IO_ACCESS                         = 1 <<  9 | 2
    };

public:
    RV32_PMU() {}

    static void config(Channel channel, const Event event, Flags flags = NONE) {
        assert((channel < CHANNELS) && (event < EVENTS));

        db<PMU>(TRC) << "PMU::config(c=" << channel << ",e=" << event << ",f=" << flags << ")" << endl;

        if(((channel == 0) && (_events[event] != 0)) || ((channel == 1) && (_events[event] != 1)) || ((channel == 2) && (_events[event] != 2))) {
            db<PMU>(WRN) << "PMU::config: channel " << channel << " is fixed in this architecture and cannot be reconfigured!" << endl;
            return;
        }

        if((channel >= FIXED) && (_events[event] != UNSUPORTED_EVENT)) {
            mhpmevent(_events[event], channel);
            start(channel);
        }
    }

    static void start(Channel channel) {
        db<PMU>(TRC) << "PMU::start(c=" << channel << ")" << endl;
        mcounteren(mcounteren() | 1 << channel);
    }

    static Count read(Channel channel) {
        db<PMU>(TRC) << "PMU::read(c=" << channel << ")" << endl;
        return mhpmcounter(channel);
    }

    static void write(Channel channel, Count count) {
        db<PMU>(TRC) << "PMU::write(ch=" << channel << ",ct=" << count << ")" << endl;
        mhpmcounter(channel, count);
    }

    static void stop(Channel channel) {
        db<PMU>(TRC) << "PMU::stop(c=" << channel << ")" << endl;
        if(channel < FIXED)
            db<PMU>(WRN) << "PMU::stop(c=" << channel << ") : fixed channels cannot be stopped!" << endl;
        mcounteren(mcounteren() & ~(1 << channel));
    }

    static void reset(Channel channel) {
        db<PMU>(TRC) << "PMU::reset(c=" << channel << ")" << endl;
        write(channel, 0);
    }

    static void init() {}

private:
    static Reg mcounteren(){ Reg reg; ASM("csrr %0, mcounteren" : "=r"(reg) :); return reg;}
    static void mcounteren(Reg reg){    ASM("csrw mcounteren, %0" : : "r"(reg));}

    static Reg mhpmevent(Channel channel) {
        Reg reg;
        switch(channel)
        {
        case 3:
            ASM("csrr %0, mhpmevent3" : : "r"(reg));
            break;
        case 4:
            ASM("csrr %0, mhpmevent4" : : "r"(reg));
            break;
        case 5:
            ASM("csrr %0, mhpmevent5" : : "r"(reg));
            break;
        case 6:
            ASM("csrr %0, mhpmevent6" : : "r"(reg));
            break;
        case 7:
            ASM("csrr %0, mhpmevent7" : : "r"(reg));
            break;
        case 8:
            ASM("csrr %0, mhpmevent8" : : "r"(reg));
            break;
        case 9:
            ASM("csrr %0, mhpmevent9" : : "r"(reg));
            break;
        case 10:
            ASM("csrr %0, mhpmevent10" : : "r"(reg));
            break;
        case 11:
            ASM("csrr %0, mhpmevent11" : : "r"(reg));
            break;
        case 12:
            ASM("csrr %0, mhpmevent12" : : "r"(reg));
            break;
        case 13:
            ASM("csrr %0, mhpmevent13" : : "r"(reg));
            break;
        case 14:
            ASM("csrr %0, mhpmevent14" : : "r"(reg));
            break;
        case 15:
            ASM("csrr %0, mhpmevent15" : : "r"(reg));
            break;
        case 16:
            ASM("csrr %0, mhpmevent16" : : "r"(reg));
            break;
        case 17:
            ASM("csrr %0, mhpmevent17" : : "r"(reg));
            break;
        case 18:
            ASM("csrr %0, mhpmevent18" : : "r"(reg));
            break;
        case 19:
            ASM("csrr %0, mhpmevent19" : : "r"(reg));
            break;
        case 20:
            ASM("csrr %0, mhpmevent20" : : "r"(reg));
            break;
        case 21:
            ASM("csrr %0, mhpmevent21" : : "r"(reg));
            break;
        case 22:
            ASM("csrr %0, mhpmevent22" : : "r"(reg));
            break;
        case 23:
            ASM("csrr %0, mhpmevent23" : : "r"(reg));
            break;
        case 24:
            ASM("csrr %0, mhpmevent24" : : "r"(reg));
            break;
        case 25:
            ASM("csrr %0, mhpmevent25" : : "r"(reg));
            break;
        case 26:
            ASM("csrr %0, mhpmevent26" : : "r"(reg));
            break;
        case 27:
            ASM("csrr %0, mhpmevent27" : : "r"(reg));
            break;
        case 28:
            ASM("csrr %0, mhpmevent28" : : "r"(reg));
            break;
        case 29:
            ASM("csrr %0, mhpmevent29" : : "r"(reg));
            break;
        case 30:
            ASM("csrr %0, mhpmevent30" : : "r"(reg));
            break;
        case 31:
            ASM("csrr %0, mhpmevent31" : : "r"(reg));
            break;
        }

        return reg;
    }

    static void mhpmevent(Reg reg, Channel channel) {
        switch (channel)
        {
        case 3:
            ASM("csrw mhpmevent3,  %0" : : "r"(reg));
            break;
        case 4:
            ASM("csrw mhpmevent4,  %0" : : "r"(reg));
            break;
        case 5:
            ASM("csrw mhpmevent5,  %0" : : "r"(reg));
            break;
        case 6:
            ASM("csrw mhpmevent6,  %0" : : "r"(reg));
            break;
        case 7:
            ASM("csrw mhpmevent7,  %0" : : "r"(reg));
            break;
        case 8:
            ASM("csrw mhpmevent8,  %0" : : "r"(reg));
            break;
        case 9:
            ASM("csrw mhpmevent9,  %0" : : "r"(reg));
            break;
        case 10:
            ASM("csrw mhpmevent10, %0" : : "r"(reg));
            break;
        case 11:
            ASM("csrw mhpmevent11, %0" : : "r"(reg));
            break;
        case 12:
            ASM("csrw mhpmevent12, %0" : : "r"(reg));
            break;
        case 13:
            ASM("csrw mhpmevent13, %0" : : "r"(reg));
            break;
        case 14:
            ASM("csrw mhpmevent14, %0" : : "r"(reg));
            break;
        case 15:
            ASM("csrw mhpmevent15, %0" : : "r"(reg));
            break;
        case 16:
            ASM("csrw mhpmevent16, %0" : : "r"(reg));
            break;
        case 17:
            ASM("csrw mhpmevent17, %0" : : "r"(reg));
            break;
        case 18:
            ASM("csrw mhpmevent18, %0" : : "r"(reg));
            break;
        case 19:
            ASM("csrw mhpmevent19, %0" : : "r"(reg));
            break;
        case 20:
            ASM("csrw mhpmevent20, %0" : : "r"(reg));
            break;
        case 21:
            ASM("csrw mhpmevent21, %0" : : "r"(reg));
            break;
        case 22:
            ASM("csrw mhpmevent22, %0" : : "r"(reg));
            break;
        case 23:
            ASM("csrw mhpmevent23, %0" : : "r"(reg));
            break;
        case 24:
            ASM("csrw mhpmevent24, %0" : : "r"(reg));
            break;
        case 25:
            ASM("csrw mhpmevent25, %0" : : "r"(reg));
            break;
        case 26:
            ASM("csrw mhpmevent26, %0" : : "r"(reg));
            break;
        case 27:
            ASM("csrw mhpmevent27, %0" : : "r"(reg));
            break;
        case 28:
            ASM("csrw mhpmevent28, %0" : : "r"(reg));
            break;
        case 29:
            ASM("csrw mhpmevent29, %0" : : "r"(reg));
            break;
        case 30:
            ASM("csrw mhpmevent30, %0" : : "r"(reg));
            break;
        case 31:
            ASM("csrw mhpmevent31, %0" : : "r"(reg));
            break;
        }
    }

    static Count mhpmcounter(Reg counter) {
        assert(counter < COUNTERS);

        Count reg = 0;
        Reg aux = 0;

        switch(counter)
        {
        case 0:
            ASM("rdcycleh %0" : "=r"(reg) : );
            ASM("rdcycle  %0" : "=r"(aux) : );
            break;
#ifndef __sifive_e__
            case 1:
            ASM("rdtimeh %0" : "=r"(reg) : );
            ASM("rdtime  %0" : "=r"(aux) : );
            break;
#endif
        case 2:
            ASM("rdinstreth %0" : "=r"(reg) : );
            ASM("rdinstret  %0" : "=r"(aux) : );
            break;
        case 3:
            ASM("csrr %0, mhpmcounter3h" : "=r"(reg) : );
            ASM("csrr %0, mhpmcounter3"  : "=r"(aux) : );
            break;
        case 4:
            ASM("csrr %0, mhpmcounter4h" : "=r"(reg) : );
            ASM("csrr %0, mhpmcounter4"  : "=r"(aux) : );
            break;
        case 5:
            ASM("csrr %0, mhpmcounter5h" : "=r"(reg) : );
            ASM("csrr %0, mhpmcounter5"  : "=r"(aux) : );
            break;
        case 6:
            ASM("csrr %0, mhpmcounter6h" : "=r"(reg) : );
            ASM("csrr %0, mhpmcounter6"  : "=r"(aux) : );
            break;
        case 7:
            ASM("csrr %0, mhpmcounter7h" : "=r"(reg) : );
            ASM("csrr %0, mhpmcounter7"  : "=r"(aux) : );
            break;
        case 8:
            ASM("csrr %0, mhpmcounter8h" : "=r"(reg) : );
            ASM("csrr %0, mhpmcounter8"  : "=r"(aux) : );
            break;
        case 9:
            ASM("csrr %0, mhpmcounter9h" : "=r"(reg) : );
            ASM("csrr %0, mhpmcounter9"  : "=r"(aux) : );
            break;
        case 10:
            ASM("csrr %0, mhpmcounter10h" : "=r"(reg) : );
            ASM("csrr %0, mhpmcounter10"  : "=r"(aux) : );
            break;
        case 11:
            ASM("csrr %0, mhpmcounter11h" : "=r"(reg) : );
            ASM("csrr %0, mhpmcounter11"  : "=r"(aux) : );
            break;
        case 12:
            ASM("csrr %0, mhpmcounter12h" : "=r"(reg) : );
            ASM("csrr %0, mhpmcounter12"  : "=r"(aux) : );
            break;
        case 13:
            ASM("csrr %0, mhpmcounter13h" : "=r"(reg) : );
            ASM("csrr %0, mhpmcounter13"  : "=r"(aux) : );
            break;
        case 14:
            ASM("csrr %0, mhpmcounter14h" : "=r"(reg) : );
            ASM("csrr %0, mhpmcounter14"  : "=r"(aux) : );
            break;
        case 15:
            ASM("csrr %0, mhpmcounter15h" : "=r"(reg) : );
            ASM("csrr %0, mhpmcounter15"  : "=r"(aux) : );
            break;
        case 16:
            ASM("csrr %0, mhpmcounter16h" : "=r"(reg) : );
            ASM("csrr %0, mhpmcounter16"  : "=r"(aux) : );
            break;
        case 17:
            ASM("csrr %0, mhpmcounter17h" : "=r"(reg) : );
            ASM("csrr %0, mhpmcounter17"  : "=r"(aux) : );
            break;
        case 18:
            ASM("csrr %0, mhpmcounter18h" : "=r"(reg) : );
            ASM("csrr %0, mhpmcounter18"  : "=r"(aux) : );
            break;
        case 19:
            ASM("csrr %0, mhpmcounter19h" : "=r"(reg) : );
            ASM("csrr %0, mhpmcounter19"  : "=r"(aux) : );
            break;
        case 20:
            ASM("csrr %0, mhpmcounter20h" : "=r"(reg) : );
            ASM("csrr %0, mhpmcounter20"  : "=r"(aux) : );
            break;
        case 21:
            ASM("csrr %0, mhpmcounter21h" : "=r"(reg) : );
            ASM("csrr %0, mhpmcounter21"  : "=r"(aux) : );
            break;
        case 22:
            ASM("csrr %0, mhpmcounter22h" : "=r"(reg) : );
            ASM("csrr %0, mhpmcounter22"  : "=r"(aux) : );
            break;
        case 23:
            ASM("csrr %0, mhpmcounter23h" : "=r"(reg) : );
            ASM("csrr %0, mhpmcounter23"  : "=r"(aux) : );
            break;
        case 24:
            ASM("csrr %0, mhpmcounter24h" : "=r"(reg) : );
            ASM("csrr %0, mhpmcounter24"  : "=r"(aux) : );
            break;
        case 25:
            ASM("csrr %0, mhpmcounter25h" : "=r"(reg) : );
            ASM("csrr %0, mhpmcounter25"  : "=r"(aux) : );
            break;
        case 26:
            ASM("csrr %0, mhpmcounter26h" : "=r"(reg) : );
            ASM("csrr %0, mhpmcounter26"  : "=r"(aux) : );
            break;
        case 27:
            ASM("csrr %0, mhpmcounter27h" : "=r"(reg) : );
            ASM("csrr %0, mhpmcounter27"  : "=r"(aux) : );
            break;
        case 28:
            ASM("csrr %0, mhpmcounter28h" : "=r"(reg) : );
            ASM("csrr %0, mhpmcounter28"  : "=r"(aux) : );
            break;
        case 29:
            ASM("csrr %0, mhpmcounter29h" : "=r"(reg) : );
            ASM("csrr %0, mhpmcounter29"  : "=r"(aux) : );
            break;
        case 30:
            ASM("csrr %0, mhpmcounter30h" : "=r"(reg) : );
            ASM("csrr %0, mhpmcounter30"  : "=r"(aux) : );
            break;
        case 31:
            ASM("csrr %0, mhpmcounter31h" : "=r"(reg) : );
            ASM("csrr %0, mhpmcounter31"  : "=r"(aux) : );
            break;
        }
        return (reg << 32) | aux;
    }

    static void mhpmcounter(Reg counter, Count reg) {
        assert(counter < COUNTERS);

        switch(counter)
        {
        case 3:
            ASM("csrw mhpmcounter3h, %0" : : "r"(reg >> 32));
            ASM("csrw mhpmcounter3,  %0" : : "r"(reg));
            break;
        case 4:
            ASM("csrw mhpmcounter4h, %0" : : "r"(reg >> 32));
            ASM("csrw mhpmcounter4,  %0" : : "r"(reg));
            break;
        case 5:
            ASM("csrw mhpmcounter5h, %0" : : "r"(reg >> 32));
            ASM("csrw mhpmcounter5,  %0" : : "r"(reg));
            break;
        case 6:
            ASM("csrw mhpmcounter6h, %0" : : "r"(reg >> 32));
            ASM("csrw mhpmcounter6,  %0" : : "r"(reg));
            break;
        case 7:
            ASM("csrw mhpmcounter7h, %0" : : "r"(reg >> 32));
            ASM("csrw mhpmcounter7,  %0" : : "r"(reg));
            break;
        case 8:
            ASM("csrw mhpmcounter8h, %0" : : "r"(reg >> 32));
            ASM("csrw mhpmcounter8,  %0" : : "r"(reg));
            break;
        case 9:
            ASM("csrw mhpmcounter9h, %0" : : "r"(reg >> 32));
            ASM("csrw mhpmcounter9,  %0" : : "r"(reg));
            break;
        case 10:
            ASM("csrw mhpmcounter10h, %0" : : "r"(reg >> 32));
            ASM("csrw mhpmcounter10,  %0" : : "r"(reg));
            break;
        case 11:
            ASM("csrw mhpmcounter11h, %0" : : "r"(reg >> 32));
            ASM("csrw mhpmcounter11,  %0" : : "r"(reg));
            break;
        case 12:
            ASM("csrw mhpmcounter12h, %0" : : "r"(reg >> 32));
            ASM("csrw mhpmcounter12,  %0" : : "r"(reg));
            break;
        case 13:
            ASM("csrw mhpmcounter13h, %0" : : "r"(reg >> 32));
            ASM("csrw mhpmcounter13,  %0" : : "r"(reg));
            break;
        case 14:
            ASM("csrw mhpmcounter14h, %0" : : "r"(reg >> 32));
            ASM("csrw mhpmcounter14,  %0" : : "r"(reg));
            break;
        case 15:
            ASM("csrw mhpmcounter15h, %0" : : "r"(reg >> 32));
            ASM("csrw mhpmcounter15,  %0" : : "r"(reg));
            break;
        case 16:
            ASM("csrw mhpmcounter16h, %0" : : "r"(reg >> 32));
            ASM("csrw mhpmcounter16,  %0" : : "r"(reg));
            break;
        case 17:
            ASM("csrw mhpmcounter17h, %0" : : "r"(reg >> 32));
            ASM("csrw mhpmcounter17,  %0" : : "r"(reg));
            break;
        case 18:
            ASM("csrw mhpmcounter18h, %0" : : "r"(reg >> 32));
            ASM("csrw mhpmcounter18,  %0" : : "r"(reg));
            break;
        case 19:
            ASM("csrw mhpmcounter19h, %0" : : "r"(reg >> 32));
            ASM("csrw mhpmcounter19,  %0" : : "r"(reg));
            break;
        case 20:
            ASM("csrw mhpmcounter20h, %0" : : "r"(reg >> 32));
            ASM("csrw mhpmcounter20,  %0" : : "r"(reg));
            break;
        case 21:
            ASM("csrw mhpmcounter21h, %0" : : "r"(reg >> 32));
            ASM("csrw mhpmcounter21,  %0" : : "r"(reg));
            break;
        case 22:
            ASM("csrw mhpmcounter22h, %0" : : "r"(reg >> 32));
            ASM("csrw mhpmcounter22,  %0" : : "r"(reg));
            break;
        case 23:
            ASM("csrw mhpmcounter23h, %0" : : "r"(reg >> 32));
            ASM("csrw mhpmcounter23,  %0" : : "r"(reg));
            break;
        case 24:
            ASM("csrw mhpmcounter24h, %0" : : "r"(reg >> 32));
            ASM("csrw mhpmcounter24,  %0" : : "r"(reg));
            break;
        case 25:
            ASM("csrw mhpmcounter25h, %0" : : "r"(reg >> 32));
            ASM("csrw mhpmcounter25,  %0" : : "r"(reg));
            break;
        case 26:
            ASM("csrw mhpmcounter26h, %0" : : "r"(reg >> 32));
            ASM("csrw mhpmcounter26,  %0" : : "r"(reg));
            break;
        case 27:
            ASM("csrw mhpmcounter27h, %0" : : "r"(reg >> 32));
            ASM("csrw mhpmcounter27,  %0" : : "r"(reg));
            break;
        case 28:
            ASM("csrw mhpmcounter28h, %0" : : "r"(reg >> 32));
            ASM("csrw mhpmcounter28,  %0" : : "r"(reg));
            break;
        case 29:
            ASM("csrw mhpmcounter29h, %0" : : "r"(reg >> 32));
            ASM("csrw mhpmcounter29,  %0" : : "r"(reg));
            break;
        case 30:
            ASM("csrw mhpmcounter30h, %0" : : "r"(reg >> 32));
            ASM("csrw mhpmcounter30,  %0" : : "r"(reg));
            break;
        case 31:
            ASM("csrw mhpmcounter31h, %0" : : "r"(reg >> 32));
            ASM("csrw mhpmcounter31,  %0" : : "r"(reg));
            break;
        default:
            db<PMU>(WRN) << "PMU::mhpmcounter(c=" << counter << "): counter is read-only!" << endl;
        }
    }

protected:
    static const Event _events[EVENTS];
};

#ifndef __rv32_pmu_common_only__

class PMU: public RV32_PMU
{
    friend class CPU;

private:
    typedef RV32_PMU Engine;

public:
    using Engine::CHANNELS;
    using Engine::FIXED;
    using Engine::EVENTS;

    using Engine::Event;
    using Engine::Count;
    using Engine::Channel;

public:
    PMU() {}

    using Engine::config;
    using Engine::read;
    using Engine::write;
    using Engine::start;
    using Engine::stop;
    using Engine::reset;

private:
    static void init() { Engine::init(); }
};

#endif

__END_SYS

#endif
