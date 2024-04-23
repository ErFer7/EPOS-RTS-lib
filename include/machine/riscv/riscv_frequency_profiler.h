#define __frequency_profiler__

#include <architecture.h>
#include <system.h>
#include <machine/machine.h>
#include <machine/timer.h>

__BEGIN_SYS

static OStream kout;

class Frequency_Profiler
{
public:
    Frequency_Profiler() {
        _initial_time = 0;
        _diff_time_sum = 0;
        _measurements = 0;
        _profiler = this;
    }

    ~Frequency_Profiler() {}

    static void measure_initial_time() {
        if (!_profiler)
            return;

        _profiler->_initial_time = Timer::mtime();
    }

    static void measure_final_time() {
        if (!_profiler)
            return;

        _profiler->_diff_time_sum += Timer::mtime() - _profiler->_initial_time;
        _profiler->_measurements++;
    }

    void profile() {
        CPU::int_enable();
        for (int i = 0; i < 10000000; i++);
        CPU::int_disabled();

        float average_diff_time = static_cast<float>(_diff_time_sum) / static_cast<float>(_measurements);
        float interruption_time_portion = (average_diff_time / static_cast<float>((Traits<Timer>::CLOCK / Traits<Timer>::FREQUENCY))) * 100.0f;

        if (interruption_time_portion >= 5.0f) {
            kout << "Error" << endl;
        } else if (interruption_time_portion >= 2.0f) {
            kout << "Warning" << endl;
        } else {
            kout << "Info" << endl;
        }
    }

    static Frequency_Profiler * instance() { return _profiler; }
private:
    unsigned long _initial_time;
    unsigned long _diff_time_sum;
    unsigned long _measurements;
    static Frequency_Profiler * _profiler;
};

__END_SYS
