// EPOS RISC-V Frequency Profiler Implementation

#include <system/config.h>
#include <architecture/cpu.h>
#include <machine/machine.h>
#include <machine/timer.h>
#include <machine/frequency_profiler.h>

__BEGIN_SYS

Frequency_Profiler * Frequency_Profiler::_profiler = nullptr;
Frequency_Profiler frequency_Profiler;

Frequency_Profiler::Frequency_Profiler() {
    _initial_time = 0UL;
    _diff_time_sum = 0UL;
    _execution_time = 0UL;
    _timing_error = false;
    _profiling = false;
    _profiler = this;
}

void Frequency_Profiler::profile() {
    if (!_profiler)
        return;

    _profiler->_profiling = true;

    CPU::Reg64 init_time = Timer::mtime();
    CPU::Reg64 final_time = 0;

    do {
        final_time = Timer::mtime();
        for (volatile unsigned long i = 0; i < Traits<Frequency_Profiler>::PROFILING_WAIT_LOAD; i++);
    } while (final_time - init_time < Traits<Frequency_Profiler>::PROFILING_TIME);

    _profiler->_execution_time = final_time - init_time;
    _profiler->_profiling = false;
}

void Frequency_Profiler::analyse_profiled_data() {
    if (!_profiler)
        return;

    _profiler->_profiling = false;

    float diff_time_sum = static_cast<float>(_profiler->_diff_time_sum);
    float execution_time = static_cast<float>(_profiler->_execution_time);
    float interruption_time_ratio = diff_time_sum / execution_time;

    if (interruption_time_ratio >= Traits<Frequency_Profiler>::INTERRUPTION_TIME_RATIO_THRESHOLD) {
        float frequency = static_cast<float>(Traits<Timer>::FREQUENCY);
        unsigned long recommended_frequency = (Traits<Frequency_Profiler>::INTERRUPTION_TIME_RATIO_THRESHOLD * execution_time * frequency) / diff_time_sum;

        db<Frequency_Profiler>(WRN) << "Frequency_Profiler:analyse_profiled_data: The current frequency of "
        << Traits<Timer>::FREQUENCY << " Hz is too high and it can cause errors during execution. "
        << "The recommended frequency for your machine is: " << recommended_frequency << " Hz." << endl;
    } else {
        db<Frequency_Profiler>(INF) << "Frequency_Profiler:analyse_profiled_data: The current frequency results "
        << "in an interruption time ratio of " << interruption_time_ratio * 100.0f << "%." << endl;
    }

    _profiler = nullptr;
}

__END_SYS
