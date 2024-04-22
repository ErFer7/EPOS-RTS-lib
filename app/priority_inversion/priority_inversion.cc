// EPOS Priority Inversion Test Program

#include <time.h>
#include <process.h>
#include <real-time.h>
#include <synchronizer.h>

using namespace EPOS;

const unsigned int iterations = 2;
const unsigned int period = 500; // ms

OStream cout;
Chronometer chrono;
Mutex critical_section;

Periodic_Thread * thread_l;
Periodic_Thread * thread_m;
Periodic_Thread * thread_h;

void heavy_work(char c, Periodic_Thread *t) {
    cout << c << " entered the heavy work" << endl;

    Microsecond elapsed = chrono.read() / 1000;
    int time = 0;
    int first_elapsed = elapsed;
    int last_cout = 0;
    while (time < 500) {
        if (last_cout + 50 < time) {
            // periodic cout
            cout << c << " is working... time elapsed: " << elapsed << " priority: " << t->priority() << endl;;
            last_cout = time;
        }

        elapsed = chrono.read() / 1000;
        time = elapsed - first_elapsed;
    }
}

void critical_section_code(char c, Periodic_Thread* this_thread) {
    cout << "Thread " << c << " tries to take lock, [p(" << c << ") = " << this_thread->priority() << "]" << endl;
    critical_section.lock();

    cout << "Thread " << c << " got the lock" << endl;
    heavy_work(c, this_thread);

    cout << "Thread " << c << " will release the lock and finish the job" << endl;
    critical_section.unlock();
}

void medium_thread_code() {
    cout << "Thread M started [p(M) = " << thread_m->priority() << "]" <<endl;
    heavy_work('M', thread_m);

    Microsecond elapsed = chrono.read() / 1000;
    cout << "Thread M finished the job " << elapsed << " has passed" << endl;
}

int low_priority() {
    do {
        critical_section_code('L', thread_l);
    } while(Periodic_Thread::wait_next());
    return 'L';
}

int medium_priority() {
    do {
        medium_thread_code();
    } while(Periodic_Thread::wait_next());
    return 'M';
}

int high_priority() {
    do {
        critical_section_code('H', thread_h);
    } while(Periodic_Thread::wait_next());
    return 'H';
}

int main() {
    cout << "Priority Inversion Test" << endl;

    // INITIAL STATE:
    // Thread L should start, acquire the lock and start the heavy work.
    // Thread H can't start the work because is waiting for lock
    // Thread M should preempt L and delay the lock release from H
    // So Thread M has "higher priority" than H without being in the critical section, that should not happen

    // passing priorities, so i can control it for this problem (not doing LLF calculus)
    chrono.start();
    thread_l = new Periodic_Thread(RTConf(period * 1000, 0, 0, 0, 4, Thread::READY, Thread::LOW), &low_priority);
    Delay pick_lock_first(100000); // to make sure L starts first
    thread_h = new Periodic_Thread(RTConf(period * 1000, 0, 0, 0, iterations, Thread::READY, Thread::HIGH), &high_priority);
    // Normal - 1 because if it's == NORMAL the calculus is made by Criterion -> LLF()
    thread_m = new Periodic_Thread(RTConf(period * 1000, 0, 0, 0 , iterations, Thread:: READY, Thread::NORMAL - 1), &medium_priority);

    int status_l = thread_l->join();
    int status_h = thread_h->join();
    int status_m = thread_m->join();

    cout << " Threads finished, s[L] = " << char(status_l) << " s[H] = " << char(status_h)  << " s[M]= "<< char(status_m) << endl;

    chrono.stop();

    return 0;
}