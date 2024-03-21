// EPOS Alarm Implementation

#include <machine/display.h>
#include <synchronizer.h>
#include <time.h>
#include <process.h>

__BEGIN_SYS

Alarm_Timer * Alarm::_timer;
volatile Alarm::Tick Alarm::_elapsed;
Alarm::Queue Alarm::_request;

Alarm::Alarm(const Microsecond & time, Handler * handler, unsigned int times)
: _time(time), _handler(handler), _times(times), _ticks(ticks(time)), _link(this, _ticks)
{
    lock();

    db<Alarm>(TRC) << "Alarm(t=" << time << ",tk=" << _ticks << ",h=" << reinterpret_cast<void *>(handler) << ",x=" << times << ") => " << this << endl;

    if(_ticks) {
        _request.insert(&_link);
        unlock();
    } else {
        assert(times == 1);
        unlock();
        (*handler)();
    }
}

Alarm::~Alarm()
{
    lock();

    db<Alarm>(TRC) << "~Alarm(this=" << this << ")" << endl;

    _request.remove(this);

    unlock();
}

void Alarm::reset()
{
    bool locked = Thread::locked();
    if(!locked)
        lock();

    db<Alarm>(TRC) << "Alarm::reset(this=" << this << ")" << endl;

    _request.remove(this);
    _link.rank(_ticks);
    _request.insert(&_link);

    if(!locked)
        unlock();
}

void Alarm::period(const Microsecond & p)
{
    bool locked = Thread::locked();
    if(!locked)
        lock();

    db<Alarm>(TRC) << "Alarm::period(this=" << this << ",p=" << p << ")" << endl;

    _request.remove(this);
    _time = p;
    _ticks = ticks(p);
    _request.insert(&_link);

    if(!locked)
        unlock();
}


void Alarm::delay(const Microsecond & time)
{
    db<Alarm>(TRC) << "Alarm::delay(time=" << time << ")" << endl;

    Tick t = _elapsed + ticks(time);

    while(_elapsed < t);
}


void Alarm::handler(IC::Interrupt_Id i)
{
    static Tick next_tick;
    static Handler * next_handler;

    lock();

    _elapsed++;

    if(Traits<Alarm>::visible) {
        Display display;
        int lin, col;
        display.position(&lin, &col);
        display.position(0, 79);
        display.putc(_elapsed);
        display.position(lin, col);
    }

    if(next_tick)
        next_tick--;
    if(!next_tick) {
        if(next_handler) {
            db<Alarm>(TRC) << "Alarm::handler(h=" << reinterpret_cast<void *>(next_handler) << ")" << endl;
            (*next_handler)();
        }
        if(_request.empty())
            next_handler = 0;
        else {
            Queue::Element * e = _request.remove();
            Alarm * alarm = e->object();
            next_tick = alarm->_ticks;
            next_handler = alarm->_handler;
            if(alarm->_times != INFINITE)
                alarm->_times--;
            if(alarm->_times) {
                e->rank(alarm->_ticks);
                _request.insert(e);
            }
        }
    }

    unlock();
}

__END_SYS
