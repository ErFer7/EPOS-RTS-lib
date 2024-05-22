// EPOS Scheduler Component Declarations

#ifndef __scheduler_h
#define __scheduler_h

#include <architecture/cpu.h>
#include <architecture/pmu.h>
#include <architecture/tsc.h>
#include <utility/scheduling.h>
#include <utility/math.h>
#include <utility/convert.h>

__BEGIN_SYS

// All scheduling criteria, or disciplines, must define operator int() with
// the semantics of returning the desired order of a given object within the
// scheduling list
class Scheduling_Criterion_Common
{
    friend class Thread;                // for handle()
    friend class Periodic_Thread;       // for handle()
    friend class RT_Thread;             // for handle()

protected:
    typedef Timer_Common::Tick Tick;

public:
    // Priorities
    enum : int {
        CEILING = -1000,
        MAIN    = -1,
        HIGH    = 0,
        NORMAL  = (unsigned(1) << (sizeof(int) * 8 - 3)) - 1,
        LOW     = (unsigned(1) << (sizeof(int) * 8 - 2)) - 1,
        IDLE    = (unsigned(1) << (sizeof(int) * 8 - 1)) - 1
    };

    // Constructor helpers
    enum : unsigned int {
        SAME        = 0,
        NOW         = 0,
        UNKNOWN     = 0,
        ANY         = -1U
    };

    // Policy types
    enum : int {
        PERIODIC    = HIGH,
        SPORADIC    = NORMAL,
        APERIODIC   = LOW
    };

    // Policy events
    typedef int Event;
    enum {
        CREATE          = 1 << 0,
        FINISH          = 1 << 1,
        ENTER           = 1 << 2,
        LEAVE           = 1 << 3,
        JOB_RELEASE     = 1 << 4,
        JOB_FINISH      = 1 << 5
    };

    // Policy operations
    typedef int Operation;
    enum {
        COLLECT         = 1 << 16,
        CHARGE          = 1 << 17,
        AWARD           = 1 << 18,
        UPDATE          = 1 << 19
    };

    // Policy traits
    static const bool timed = false;
    static const bool dynamic = false;
    static const bool preemptive = true;

    // Runtime Statistics (for policies that don't use any; that's why its a union)
    union Dummy_Statistics {  // for Traits<System>::monitored = false
        // Thread related statistics
        Tick thread_creation;                   // tick in which the thread was created
        Tick thread_destruction;                // tick in which the thread was destroyed
        Tick thread_execution_time;             // accumulated execution time (in ticks)
        Tick thread_last_dispatch;              // tick in which the thread was last dispatched to the CPU
        Tick thread_last_preemption;            // tick in which the thread left the CPU by the last time

        // Job related statistics
        bool job_released;
        Tick job_release;                       // tick in which the last job of a periodic thread was made ready for execution
        Tick job_start;                         // tick in which the last job of a periodic thread started (different from "thread_last_dispatch" since jobs can be preempted)
        Tick job_finish;                        // tick in which the last job of a periodic thread finished (i.e. called _alarm->p() at wait_netxt(); different from "thread_last_preemption" since jobs can be preempted)
        Tick job_utilization;                   // accumulated execution time (in ticks)
        unsigned int jobs_released;             // number of jobs of a thread that were released so far (i.e. the number of times _alarm->v() was called by the Alarm::handler())
        unsigned int jobs_finished;             // number of jobs of a thread that finished execution so far (i.e. the number of times alarm->p() was called at wait_next())
    };

    struct Real_Statistics {  // for Traits<System>::monitored = true
        // Thread related statistics
        Tick thread_creation;                   // tick in which the thread was created
        Tick thread_destruction;                // tick in which the thread was destroyed
        Tick thread_execution_time;             // accumulated execution time (in ticks)
        Tick thread_last_dispatch;              // tick in which the thread was last dispatched to the CPU
        Tick thread_last_preemption;            // tick in which the thread left the CPU by the last time

        // Job related statistics
        bool job_released;
        Tick job_release;                       // tick in which the last job of a periodic thread was made ready for execution
        Tick job_start;                         // tick in which the last job of a periodic thread started (different from "thread_last_dispatch" since jobs can be preempted)
        Tick job_finish;                        // tick in which the last job of a periodic thread finished (i.e. called _alarm->p() at wait_netxt(); different from "thread_last_preemption" since jobs can be preempted)
        Tick job_utilization;                   // accumulated execution time (in ticks)
        unsigned int jobs_released;             // number of jobs of a thread that were released so far (i.e. the number of times _alarm->v() was called by the Alarm::handler())
        unsigned int jobs_finished;             // number of jobs of a thread that finished execution so far (i.e. the number of times alarm->p() was called at wait_next())
    };

    typedef IF<Traits<System>::monitored, Real_Statistics, Dummy_Statistics>::Result Statistics;

protected:
    Scheduling_Criterion_Common() {}

public:
    Microsecond period() { return 0;}
    Microsecond deadline() { return 0; }
    Microsecond capacity() { return 0; }

    bool periodic() { return false; }

    volatile Statistics & statistics() { return _statistics; }

protected:
    void handle(Event event) {}

    static void init() {}

protected:
    Statistics _statistics;
};

// Priority (static and dynamic)
class Priority: public Scheduling_Criterion_Common
{
public:
    template <typename ... Tn>
    Priority(int p = NORMAL, Tn & ... an): _priority(p) {}

    operator const volatile int() const volatile { return _priority; }

protected:
    volatile int _priority;
};

class Variable_Queue_Scheduler
{
protected:
    Variable_Queue_Scheduler(unsigned int queue): _queue(queue) {};

    const volatile unsigned int & queue() const volatile { return _queue; }
    void queue(unsigned int q) { _queue = q; }

protected:
    volatile unsigned int _queue;
    static volatile unsigned int _next_queue;
};

// Round-Robin
class RR: public Priority
{
public:
    static const bool timed = true;
    static const bool dynamic = false;
    static const bool preemptive = true;

public:
    template <typename ... Tn>
    RR(int p = NORMAL, Tn & ... an): Priority(p) {}
};

// First-Come, First-Served (FIFO)
class FCFS: public Priority
{
public:
    static const bool timed = false;
    static const bool dynamic = false;
    static const bool preemptive = false;

public:
    template <typename ... Tn>
    FCFS(int p = NORMAL, Tn & ... an);
};


// Real-time Algorithms
class RT_Common: public Priority
{
    friend class FCFS;
    friend class Thread;                // for handle() and queue()
    friend class Periodic_Thread;       // for handle() and queue()
    friend class RT_Thread;             // for handle() and queue()

public:
    static const bool timed = true;
    static const bool preemptive = true;

protected:
    RT_Common(int i): Priority(i), _period(0), _deadline(0), _capacity(0) {} // aperiodic
    RT_Common(int i, Microsecond p, Microsecond d, Microsecond c): Priority(i), _period(ticks(p)), _deadline(ticks(d ? d : p)), _capacity(ticks(c)) {}

public:
    Microsecond period() { return time(_period); }
    Microsecond deadline() { return time(_deadline); }
    Microsecond capacity() { return time(_capacity); }

    bool periodic() { return (_priority >= PERIODIC) && (_priority <= SPORADIC); }

    volatile Statistics & statistics() { return _statistics; }

protected:
    Tick ticks(Microsecond time);
    Microsecond time(Tick ticks);

    void handle(Event event);

    static Tick elapsed();

protected:
    Tick _period;
    Tick _deadline;
    Tick _capacity;
    Statistics _statistics;
};

// Rate Monotonic
class RM:public RT_Common
{
public:
    static const bool dynamic = false;

public:
    RM(int p = APERIODIC): RT_Common(p) {}
    RM(Microsecond p, Microsecond d = SAME, Microsecond c = UNKNOWN): RT_Common(int(ticks(p)), p, d, c) {}
};

// Deadline Monotonic
class DM: public RT_Common
{
public:
    static const bool dynamic = false;

public:
    DM(int p = APERIODIC): RT_Common(p) {}
    DM(Microsecond p, Microsecond d = SAME, Microsecond c = UNKNOWN): RT_Common(int(ticks(d ? d : p)), p, d, c) {}
};

// Laxity Monotonic
class LM: public RT_Common
{
public:
    static const bool dynamic = false;

public:
    LM(int p = APERIODIC): RT_Common(p) {}
    LM(Microsecond p, Microsecond d, Microsecond c): RT_Common(int(ticks((d ? d : p) - c)), p, d, c) {}
};

// Earliest Deadline First
class EDF: public RT_Common
{
public:
    static const bool dynamic = true;

public:
    EDF(int p = APERIODIC): RT_Common(p) {}
    EDF(Microsecond p, Microsecond d = SAME, Microsecond c = UNKNOWN);

    void handle(Event event);
};

// Least Laxity First
class LLF: public RT_Common
{
public:
    static const bool dynamic = true;

public:
    LLF(int p = APERIODIC): RT_Common(p) {}
    LLF(Microsecond p, Microsecond d = SAME, Microsecond c = UNKNOWN);

    void handle(Event event);
};

class GLLF: public LLF
{
public:
    static const unsigned int HEADS = Traits<Machine>::CPUS;

public:
    GLLF(int p = APERIODIC): LLF(p) {}
    GLLF(const Microsecond & d, const Microsecond & p = SAME, const Microsecond & c = UNKNOWN, unsigned int cpu = ANY):
        LLF(d, p, c) {}

    unsigned int queue() const { return current_head(); }
    void queue(unsigned int q) {}
    static unsigned int current_head() { return CPU::id(); }
};

class PLLF: public LLF, public Variable_Queue_Scheduler
{
public:
    static const unsigned int QUEUES = Traits<Machine>::CPUS;

public:
    PLLF(int p = APERIODIC)
    : LLF(p), Variable_Queue_Scheduler(((_priority == IDLE) || (_priority == MAIN)) ? CPU::id() : 0) {}

    PLLF(const Microsecond & d, const Microsecond & p = SAME, const Microsecond & c = UNKNOWN, unsigned int cpu = ANY)
    : LLF(d, p, c), Variable_Queue_Scheduler((cpu != ANY) ? cpu : ++_next_queue %= CPU::cores()) {}

    using Variable_Queue_Scheduler::queue;
    static unsigned int current_queue() { return CPU::id(); }
};

__END_SYS

__BEGIN_UTIL

template<typename T>
class Scheduling_Queue<T, GLLF>:
public Multihead_Scheduling_List<T> {};

template<typename T>
class Scheduling_Queue<T, PLLF>:
public Scheduling_Multilist<T> {};

__END_UTIL

#endif
