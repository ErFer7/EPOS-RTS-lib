// EPOS RISC-V Frequency Profiler Declarations

#ifndef __riscv_frequency_profiler_h
#define __riscv_frequency_profiler_h

#include <architecture/cpu.h>
#include <machine/machine.h>
#include <machine/timer.h>
#include <machine/frequency_profiler.h>

__BEGIN_SYS

static OStream kout;

class Frequency_Profiler : protected Frequency_Profiler_Common
{
    friend class IC;

public:
    Frequency_Profiler();
    ~Frequency_Profiler();

    static void profile();
    static void analyse_profiled_data();

protected:
    inline static void measure_initial_time() {
        db<Frequency_Profiler>(TRC) << "measure_initial_time(profiler=" << _profiler << ")" << endl;
        db<Frequency_Profiler>(INF) << "sp=[" << CPU::sp() << "], epc=[" << CPU::epc() << "]" << endl;

        if (!_profiler)
            return;

        if (_profiler->_timing_error) {
            db<Frequency_Profiler>(ERR) << "Frequency_Profiler:measure_initial_time: Timing error! The Frequency is too high" << endl;
            Machine::reboot();
        }

        if (_profiler->_profiling)
            _profiler->_initial_time = Timer::mtime();

        _profiler->_timing_error = true;
    }

    inline static void measure_final_time() {
        db<Frequency_Profiler>(TRC) << "measure_final_time(profiler=" << _profiler << ")" << endl;
        db<Frequency_Profiler>(INF) << "sp=[" << CPU::sp() << "], epc=[" << CPU::epc() << "]" << endl;

        if (!_profiler)
            return;

        if (_profiler->_profiling)
            _profiler->_diff_time_sum += Timer::mtime() - _profiler->_initial_time;

        _profiler->_timing_error = false;
    }

private:
    CPU::Reg64 _diff_time_sum;
    CPU::Reg64 _initial_time;
    CPU::Reg64 _execution_time;
    bool _timing_error;
    bool _profiling;
    static Frequency_Profiler * _profiler;
};

__END_SYS

#endif
