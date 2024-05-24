// EPOS Real-time Declarations

#ifndef __real_time_h
#define __real_time_h

#include <utility/handler.h>
#include <utility/math.h>
#include <utility/convert.h>
#include <time.h>
#include <process.h>
#include <synchronizer.h>

__BEGIN_SYS

// Aperiodic Thread
typedef Thread Aperiodic_Thread;

// Periodic threads are achieved by programming an alarm handler to invoke
// p() on a control semaphore after each job (i.e. task activation). Base
// threads are created in BEGINNING state, so the scheduler won't dispatch
// them before the associate alarm and semaphore are created. The first job
// is dispatched by resume() (thus the _state = SUSPENDED statement)

// Periodic Thread
class Periodic_Thread: public Thread
{
public:
    enum {
        SAME    = RT_Common::SAME,
        NOW     = RT_Common::NOW,
        UNKNOWN = RT_Common::UNKNOWN,
        ANY     = RT_Common::ANY
    };

protected:
    // Alarm Handler for periodic threads under static scheduling policies
    class Static_Handler: public Semaphore_Handler
    {
    public:
        Static_Handler(Semaphore * s, Periodic_Thread * t): Semaphore_Handler(s) {}
        ~Static_Handler() {}
    };

    // Alarm Handler for periodic threads under dynamic scheduling policies
    class Dynamic_Handler: public Semaphore_Handler
    {
    public:
        Dynamic_Handler(Semaphore * s, Periodic_Thread * t): Semaphore_Handler(s), _thread(t) {}
        ~Dynamic_Handler() {}

        void operator()() {
            _thread->criterion().handle(Criterion::JOB_RELEASE);
            Semaphore_Handler::operator()();
        }

    private:
        Periodic_Thread * _thread;
    };

    typedef IF<Criterion::dynamic | Traits<System>::monitored, Dynamic_Handler, Static_Handler>::Result Handler;

public:
    struct Configuration: public Thread::Configuration {
        Configuration(Microsecond p, Microsecond d = SAME, Microsecond c = UNKNOWN, Microsecond a = NOW, const unsigned int n = INFINITE, State s = READY, unsigned int ss = STACK_SIZE)
        : Thread::Configuration(s, Criterion(p, d, c), ss), activation(a), times(n) {}

        Microsecond activation;
        unsigned int times;
    };

public:
    template<typename ... Tn>
    Periodic_Thread(Microsecond p, int (* entry)(Tn ...), Tn ... an)
    : Thread(Thread::Configuration(SUSPENDED, Criterion(p)), entry, an ...),
      _semaphore(0), _handler(&_semaphore, this), _alarm(p, &_handler, INFINITE) {
        resume();
        criterion().handle(Criterion::JOB_RELEASE);
    }

    template<typename ... Tn>
    Periodic_Thread(Configuration conf, int (* entry)(Tn ...), Tn ... an)
    : Thread(Thread::Configuration(SUSPENDED, conf.criterion, conf.stack_size), entry, an ...),
      _semaphore(0), _handler(&_semaphore, this), _alarm(conf.criterion.period(), &_handler, conf.times) {
        if((conf.state == READY) || (conf.state == RUNNING)) {
            _state = SUSPENDED;
            resume();
            criterion().handle(Criterion::JOB_RELEASE);
        } else
            _state = conf.state;
    }

    Microsecond period() const { return _alarm.period(); }
    void period(Microsecond p) { _alarm.period(p); }

    static volatile bool wait_next() {
        Periodic_Thread * t = reinterpret_cast<Periodic_Thread *>(running());

        db<Thread>(TRC) << "Thread::wait_next(this=" << t << ",times=" << t->_alarm.times() << ")" << endl;

        t->criterion().handle(Criterion::JOB_FINISH);

        if(t->_alarm.times())
            t->_semaphore.p();

        return t->_alarm.times();
    }

protected:
    Semaphore _semaphore;
    Handler _handler;
    Alarm _alarm;
};

class RT_Thread: public Periodic_Thread
{
public:
    RT_Thread(void (* function)(), Microsecond p, Microsecond d = SAME, Microsecond c = UNKNOWN, Microsecond a = NOW, int n = INFINITE, unsigned int ss = STACK_SIZE)
    : Periodic_Thread(Configuration(p, d, c, a, n, SUSPENDED, ss), &entry, this, function, a, n) {
        resume();
    }

private:
    static int entry(RT_Thread * t, void (* function)(), Microsecond a, int n) {
        if(a) {
            // Wait for activation time
            t->_semaphore.p();

            t->criterion().handle(Criterion::JOB_RELEASE);

            // Adjust alarm's period
            t->_alarm.~Alarm();
            new (&t->_alarm) Alarm(t->criterion().period(), &t->_handler, n);
        }

        // Periodic execution loop
        do {
//            Alarm::Tick tick;
//            if(Traits<Periodic_Thread>::simulate_capacity && t->criterion()._capacity)
//                tick = Alarm::elapsed() + Alarm::ticks(t->criterion()._capacity);

            // Release job
            function();

//            if(Traits<Periodic_Thread>::simulate_capacity && t->criterion()._capacity)
//                while(Alarm::elapsed() < tick);
        } while(wait_next());

        return 0;
    }
};

typedef Periodic_Thread::Configuration RTConf;

__END_SYS

#endif
