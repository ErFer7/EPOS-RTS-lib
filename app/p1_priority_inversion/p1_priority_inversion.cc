// EPOS Priority Inversion Test Program

#include <time.h>
#include <process.h>
#include <real-time.h>
#include <synchronizer.h>

using namespace EPOS;


const unsigned int iterations = 2; // for the test to run fast
const unsigned int period = 550; // ms


OStream cout;
Chronometer chrono;
Mutex criticalSection;

Periodic_Thread * threadL;
Periodic_Thread * threadM;
Periodic_Thread * threadH;

void heavyWork(char c, Periodic_Thread *t) {
    cout << c << " entered the heavy work " << endl;

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

void criticalSectionCode(char c, Periodic_Thread* this_thread) {
    cout << "Thread " << c << " tries to take lock, [p(" << c << ") = " << this_thread->priority() << "]" << endl;
    criticalSection.lock();

    cout << "Thread " << c << " got the lock" << endl;
    heavyWork(c, this_thread);    

    cout << "Thread " << c << " will release the lock and finish the job" << endl;
    criticalSection.unlock();
}

void mediumThreadCode() {
    cout << "Thread M started [p(M) = " << threadM->priority() << "]" <<endl;
    heavyWork('M', threadM);

    Microsecond elapsed = chrono.read() / 1000;
    cout << "Thread M finished the job " << elapsed << " has passed" << endl;
}

int lowPriority() { 
    do {
        criticalSectionCode('L', threadL);
    } while(Periodic_Thread::wait_next());
    return 'L';
}

int mediumPriority() {
    do {
        mediumThreadCode();
    } while(Periodic_Thread::wait_next());
    return 'M';
}

int highPriority() { 
    do {
        criticalSectionCode('H', threadH);
    } while(Periodic_Thread::wait_next());
    return 'H';
}

int main() {
    cout << "Priority Inversion Test" << endl;

    // INITIAL STATE:
    // Thread L should start, acquire the lock and start heavy work.
    // Thread H can't start work because is waiting for lock
    // Thread M should preempt L and delay the lock release from H
    // So Thread M has "higher priority" than H without being in the critical section, that should not happen

    // passing priorities, so i can control it for this problem (not doing LLF calculus anymore)
    chrono.start();
    threadL = new Periodic_Thread(RTConf(period * 1000, 0, Periodic_Thread::UNKNOWN, 0, iterations, Thread::READY, Thread::LOW), &lowPriority);
    cout << "voltou pra main " << endl;
    Delay pickLockFirst(100000); // to make sure L starts first
    threadH = new Periodic_Thread(RTConf(period * 1000, 0, Periodic_Thread::UNKNOWN, 0, iterations, Thread::READY, Thread::HIGH), &highPriority);
    // Normal - 1 because if it's == NORMAL the calculus is made by Criterion -> LLF()
    threadM = new Periodic_Thread(RTConf(period * 1000, 0, Periodic_Thread::UNKNOWN, 0 , iterations, Thread:: READY, Thread::NORMAL - 1), &mediumPriority);
    cout << "Thread L: " << threadL << " Thread M: " << threadM <<  endl;

    int status_l = threadL->join();
    int status_h = threadH->join();
    int status_m = threadM->join();

    cout << " Threads finalizaram, s[L] = " << status_l << " s[H] = " << status_h  << " s[M]= "<< status_m << endl;

    chrono.stop();

    return 0;
}