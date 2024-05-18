// EPOS Frequency Profiler Declarations

#ifndef __frequency_profiler_h
#define __frequency_profiler_h

#include <architecture/cpu.h>
#include <system/config.h>

__BEGIN_SYS

class Frequency_Profiler_Common
{
public:
    static void profile();
    static void analyse_profiled_data();

protected:
    Frequency_Profiler_Common() {}
    static void measure_initial_time();
    static void measure_final_time();

private:
    CPU::Reg64 _diff_time_sum;
    CPU::Reg64 _initial_time;
    CPU::Reg64 _execution_time;
    bool _timing_error;
    bool _profiling;
};

__END_SYS

#endif

#if defined(__FREQ_PROF_H) && !defined(__frequency_profiler_common_only__)
#include __FREQ_PROF_H
#endif
