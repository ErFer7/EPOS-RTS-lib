// EPOS ARMv8 PMU Mediator Declarations

#ifndef __armv8_pmu_h
#define __armv8_pmu_h

#include <architecture/cpu.h>
#define __pmu_common_only__
#include <architecture/pmu.h>
#undef __pmu_common_only__

__BEGIN_SYS

class ARMv8_A_PMU: public PMU_Common
{
private:
    typedef CPU::Reg64 Reg64;

protected:
    static const unsigned int CHANNELS = 6;
    static const unsigned int FIXED = 0;
    static const unsigned int EVENTS = PMU_Event::LAST_EVENT;


public:
    // Useful bits in the PMUSERENR_EL0 register
    enum {                                // Description                          Type    Value after reset
        PMUSERENR_EN_EL0 = 1 << 0,        // EL0 access enable          r/w
    };

    // Useful bits in the PMCNTENSET register
    enum {                           // Description                          Type    Value after reset
        PMCNTENSET_EN_EL0 = 1 << 31, // Cycle counter enable                 r/w
    };

    // Useful bits in the PMCR register
    enum {                      // Description                          Type    Value after reset
        PMCR_E = 1 << 0,        // Enable all counters                  r/w
        PMCR_P = 1 << 1,        // Reset event counters                 r/w
        PMCR_C = 1 << 2,        // Cycle counter reset                  r/w
    };
    // Performance events
    enum {
        L1I_REFILL                              = 0x01,
        L1I_TLB_REFILL                          = 0x02,
        L1D_REFILL                              = 0x03,
        L1D_ACCESS                              = 0x04,
        L1D_TLB_REFILL                          = 0x05,
        INSTRUCTIONS_ARCHITECTURALLY_EXECUTED_L = 0x06,
        INSTRUCTIONS_ARCHITECTURALLY_EXECUTED_S = 0x07,
        INSTRUCTIONS_ARCHITECTURALLY_EXECUTED   = 0x08,
        EXCEPTION_TAKEN                         = 0x09,
        EXCEPTION_RETURN                        = 0x0a,
        CHANGE_TO_CONTEXT_ID_RETIRED            = 0x0b,
        BRANCHES_ARCHITECTURALLY_EXECUTED       = 0x0c,
        IMMEDIATE_BRANCH                        = 0x0d,
        RETURN_EXECUTED                         = 0x0e,
        UNALIGNED_LOAD_STORE                    = 0x0f,
        MISPREDICTED_BRANCH                     = 0x10,
        CYCLE                                   = 0x11,
        PREDICTABLE_BRANCH_EXECUTED             = 0x12,
        DATA_MEMORY_ACCESS                      = 0x13,
        L1I_ACCESS                              = 0x14,
        L1D_WRITEBACK                           = 0x15,
        L2D_ACCESS                              = 0x16,
        L2D_REFILL                              = 0x17,
        L2D_WRITEBACK                           = 0x18,
        BUS_ACCESS                              = 0x19,
        LOCAL_MEMORY_ERROR                      = 0x1a,
        // INSTRUCTION_SPECULATIVELY_EXECUTED      = 0x1b,
        BUS_CYCLE                               = 0x1d,
        CHAIN                                   = 0x1e,
        // Cortex A-53 specific events
        BUS_ACCESS_LD                           = 0x60,
        BUS_ACCESS_ST                           = 0x61,
        BR_INDIRECT_SPEC                        = 0x7a,
        EXC_IRQ                                 = 0x86,
        EXC_FIQ                                 = 0x87,
        EXTERNAL_MEM_REQUEST                    = 0xc0,
        EXTERNAL_MEM_REQUEST_NON_CACHEABLE      = 0xc1,
        PREFETCH_LINEFILL                       = 0xc2,
        ICACHE_THROTTLE                         = 0xc3,
        ENTER_READ_ALLOC_MODE                   = 0xc4,
        READ_ALLOC_MODE                         = 0xc5,
        PRE_DECODE_ERROR                        = 0xc6,
        DATA_WRITE_STALL_ST_BUFFER_FULL         = 0xc7,
        SCU_SNOOPED_DATA_FROM_OTHER_CPU         = 0xc8,
        CONDITIONAL_BRANCH_EXECUTED             = 0xc9,
        IND_BR_MISP                             = 0xca,
        IND_BR_MISP_ADDRESS_MISCOMPARE          = 0xcb,
        CONDITIONAL_BRANCH_MISP                 = 0xcc,
        L1_ICACHE_MEM_ERROR                     = 0xd0,
        L1_DCACHE_MEM_ERROR                     = 0xd1,
        TLB_MEM_ERROR                           = 0xd2,
        EMPTY_DPU_IQ_NOT_GUILTY                 = 0xe0,
        EMPTY_DPU_IQ_ICACHE_MISS                = 0xe1,
        EMPTY_DPU_IQ_IMICRO_TLB_MISS            = 0xe2,
        EMPTY_DPU_IQ_PRE_DECODE_ERROR           = 0xe3,
        INTERLOCK_CYCLE_NOT_GUILTY              = 0xe4,
        INTERLOCK_CYCLE_LD_ST_WAIT_AGU_ADDRESS  = 0xe5,
        INTERLOCK_CYCLE_ADV_SIMD_FP_INST        = 0xe6,
        INTERLOCK_CYCLE_WR_STAGE_STALL_BC_MISS  = 0xe7,
        INTERLOCK_CYCLE_WR_STAGE_STALL_BC_STR   = 0xe8,
    };

public:
    ARMv8_A_PMU() {}

    static void config(Channel channel, const Event event, Flags flags = NONE) {
        assert((channel < CHANNELS) && (event < EVENTS) && (_events[event] != UNSUPORTED_EVENT));
        db<PMU>(TRC) << "PMU::config(c=" << channel << ",e=" << event << ",f=" << flags << ")" << endl;
        pmselr_el0(channel);
        pmxevtyper_el0(_events[event]);
        start(channel);
    }

    static Count read(Channel channel) {
        db<PMU>(TRC) << "PMU::read(c=" << channel << ")" << endl;
        pmselr_el0(channel);
        return pmxevcntr_el0();
    }

    static void write(Channel channel, Count count) {
        db<PMU>(TRC) << "PMU::write(ch=" << channel << ",ct=" << count << ")" << endl;
        pmselr_el0(channel);
        pmxevcntr_el0(count);
    }

    static void start(Channel channel) {
        db<PMU>(TRC) << "PMU::start(c=" << channel << ")" << endl;
        pmcntenset_el0(pmcntenset_el0() | (1 << channel));
    }

    static void stop(Channel channel) {
        db<PMU>(TRC) << "PMU::stop(c=" << channel << ")" << endl;
        pmcntenclr_el0(pmcntenclr_el0() | (1 << channel));
    }

    static void reset(Channel channel) {
        db<PMU>(TRC) << "PMU::reset(c=" << channel << ")" << endl;
        write(channel, 0);
    }

    static void init();

private:
    static void pmuserenr_el0(Reg64 reg) { ASM("msr pmuserenr_el0, %0\n\t" : : "r"(reg)); }
    static Reg64 pmuserenr_el0() { Reg64 reg; ASM("mrs %0, pmuserenr_el0\n\t" : "=r"(reg) : ); return reg; }

    static void pmcntenset_el0(Reg64 reg) { ASM("msr pmcntenset_el0, %0\n\t" : : "r"(reg)); }
    static Reg64 pmcntenset_el0() { Reg64 reg; ASM("mrs %0, pmcntenset_el0\n\t" : "=r"(reg) : ); return reg; }

    static void pmcntenclr_el0(Reg64 reg) { ASM("msr pmcntenclr_el0, %0\n\t" : : "r"(reg)); }
    static Reg64 pmcntenclr_el0() { Reg64 reg; ASM("mrs %0, pmcntenclr_el0\n\t" : "=r"(reg) : ); return reg; }

    static void pmcr_el0(Reg64 reg) { ASM("msr pmcr_el0, %0\n\t" : : "r"(reg)); }
    static Reg64 pmcr_el0() { Reg64 reg; ASM("mrs %0, pmcr_el0\n\t" : "=r"(reg) : ); return reg; }

    static void pmselr_el0(Reg64 reg) { ASM("msr pmselr_el0, %0\n\t" : : "r"(reg)); }
    static Reg64 pmselr_el0() { Reg64 reg; ASM("mrs %0, pmselr_el0\n\t" : "=r"(reg) : ); return reg; }

    static void pmxevtyper_el0(Reg64 reg) { ASM("msr pmxevtyper_el0, %0\n\t" : : "r"(reg)); }
    static Reg64 pmxevtyper_el0() { Reg64 reg; ASM("mrs %0, pmxevtyper_el0\n\t" : "=r"(reg) : ); return reg; }

    static void pmxevcntr_el0(Reg64 reg) { ASM("msr pmxevcntr_el0, %0\n\t" : : "r"(reg)); }
    static Reg64 pmxevcntr_el0() { Reg64 reg; ASM("mrs %0, pmxevcntr_el0\n\t" : "=r"(reg) : ); return reg; }

private:
    static const Event _events[EVENTS];
};

class PMU: public ARMv8_A_PMU
{
    friend class CPU;

private:
    typedef ARMv8_A_PMU Engine;

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

__END_SYS

#endif
