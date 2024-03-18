// EPOS ARM Cortex Timer Mediator Declarations

#ifndef __cortex_timer_h
#define __cortex_timer_h

#include <architecture/cpu.h>
#include <machine/ic.h>
#include <machine/timer.h>
#include __HEADER_MMOD(timer)
#include <utility/convert.h>

__BEGIN_SYS

// Tick timer used by the system
class Timer: public System_Timer_Engine
{
    friend Machine;             // for init()
    friend class Init_System;   // for init()
    friend IC;                  // for eoi()

protected:
    static const unsigned int CHANNELS = 2;
    static const unsigned int FREQUENCY = Traits<Timer>::FREQUENCY;

    typedef System_Timer_Engine Engine;
    typedef IC_Common::Interrupt_Id Interrupt_Id;

protected:
    Timer(Channel channel, Hertz frequency, const Handler & handler, bool retrigger = true)
    : _channel(channel), _initial(FREQUENCY / frequency), _retrigger(retrigger), _handler(handler) {
        db<Timer>(TRC) << "Timer(f=" << frequency << ",h=" << reinterpret_cast<void*>(handler) << ",ch=" << channel << ") => {count=" << _initial << "}" << endl;

        if(_initial && (channel < CHANNELS) && !_channels[channel])
            _channels[channel] = this;
        else
            db<Timer>(WRN) << "Timer not installed!"<< endl;

        _current = _initial;
    }

public:
    ~Timer() {
        db<Timer>(TRC) << "~Timer(f=" << frequency() << ",h=" << reinterpret_cast<void*>(_handler) << ",ch=" << _channel << ") => {count=" << _initial << "}" << endl;

        _channels[_channel] = 0;
    }

    Tick read() { return _current; }

    int restart() {
        db<Timer>(TRC) << "Timer::restart() => {f=" << frequency() << ",h=" << reinterpret_cast<void *>(_handler) << ",count=" << _current << "}" << endl;

        int percentage = _current * 100 / _initial;
        _current= _initial;

        return percentage;
    }

    static void reset() { db<Timer>(TRC) << "Timer::reset()" << endl; Engine::reset(); }
    static void enable() { db<Timer>(TRC) << "Timer::enable()" << endl; Engine::enable(); }
    static void disable() { db<Timer>(TRC) << "Timer::disable()" << endl; Engine::disable(); }

    PPB accuracy();
    Hertz frequency() const { return (FREQUENCY / _initial); }
    void frequency(Hertz f) { _initial = FREQUENCY / f; reset(); }

    void handler(const Handler & handler) { _handler = handler; }

private:
    static void int_handler(Interrupt_Id i);
    static void eoi(Interrupt_Id i);

    static void init();

protected:
    unsigned int _channel;
    Tick _initial;
    bool _retrigger;
    volatile Tick _current;
    Handler _handler;

    static Timer * _channels[CHANNELS];
};


// Timer used by Thread::Scheduler
class Scheduler_Timer: public Timer
{
public:
    Scheduler_Timer(Microsecond quantum, const Handler & handler): Timer(SCHEDULER, 1000000 / quantum, handler) {}
};

// Timer used by Alarm
class Alarm_Timer: public Timer
{
public:
    Alarm_Timer(const Handler & handler): Timer(ALARM, FREQUENCY, handler) {}
};


// Timer available for users
class User_Timer: private User_Timer_Engine
{
    friend IC;          // for eoi()
    friend class PWM;

private:
    typedef User_Timer_Engine Engine;
    typedef Engine::Count Count;
    typedef IC_Common::Interrupt_Id Interrupt_Id;

    static const unsigned int UNITS = 1;

public:
    using Timer_Common::Handler;

public:
    User_Timer(Channel channel, Microsecond time, const Handler & handler, bool periodic = false)
    : Engine(channel, time, handler ? true : false, periodic), _channel(channel), _handler(handler) {
        assert(channel < UNITS);
        if(_handler) {
            IC::int_vector(IC::INT_USR_TIMER, _handler, eoi);
            IC::enable(IC::INT_USR_TIMER);
        }
    }
    
    ~User_Timer() {
        if(_handler)
            IC::disable(IC::INT_USR_TIMER);
    }

    using Engine::read;

    using Engine::enable;
    using Engine::disable;

    void power(const Power_Mode & mode);

private:
    static void eoi(Interrupt_Id i);

    static void init() {} // user timers are initialized at the constructor

private:
    unsigned int _channel;
    Handler _handler;
};

__END_SYS

#endif
