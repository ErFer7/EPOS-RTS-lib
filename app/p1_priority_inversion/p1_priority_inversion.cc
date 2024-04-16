// EPOS Priority Inversion Test Program

#include <time.h>
#include <process.h>
#include <real-time.h>
#include <synchronizer.h>

using namespace EPOS;


// Por enquanto manter todos iguais, menos activation e criterion;
const unsigned int iterations = 50; // Isso importa, do jeito que eu fiz o teste?
const unsigned int period = 50; // ms
const unsigned int wcet = 20; // ms -> Influencia em algo?


OStream cout;
Chronometer chrono;
Mutex criticalSection;

Periodic_Thread * threadL;
Periodic_Thread * threadM;
Periodic_Thread * threadH;

void heavyWork(char c, Periodic_Thread *t) {
    cout << c << " entered the heavy work " << endl;
    Microsecond elapsed = chrono.read() / 1000;
    Microsecond last_elapsed = elapsed;
    while (elapsed < 500) {
        if (last_elapsed + wcet < elapsed) {
            // periodic cout
            cout << c << " is working... time elapsed: " << elapsed << " priority: " << t->priority() << endl;;
            last_elapsed = elapsed;
        }
        elapsed = chrono.read() / 1000;
    }
}

void criticalSectionCode(char c, Periodic_Thread* this_thread) {
    cout << "Thread " << c << " tries to take lock, [p(" << c << ") = " << this_thread->priority() << "]" << endl;
    criticalSection.lock();

    Microsecond elapsed = chrono.read() / 1000;
    cout << "Thread " << c << " got the lock, " << elapsed << " time has passed" << endl;
    cout << " new priority: " << this_thread->priority() << endl;
    heavyWork(c, this_thread);    

    cout << "Thread " << c << " will release the lock and finish" << endl;
    criticalSection.unlock();
    cout << " new priority: " << this_thread->priority() << endl;
}

int lowPriority() { 
    criticalSectionCode('L', threadL);
    return 'L';
}

int mediumPriority() {
    cout << "Thread M started [p(M) = " << threadM->priority() << "]" <<endl;
    heavyWork('M', threadM);

    Microsecond elapsed = chrono.read() / 1000;
    cout << "Thread M finished " << elapsed << " has passed" << endl;
    return 'M';
}

int highPriority() { 
    criticalSectionCode('H', threadH);
    return 'H';
}

int main() {
    cout << "Priority Inversion Test" << endl;

    // INITIAL STATE:
    // Thread L should start, acquire the lock and start heavy work.
    // Thread H can't start work because is waiting for lock
    // Thread M should preempt L and delay the lock release from H
    // So Thread M has "higher priority" than H without being in the critical section, that should not happen


    // Understand the parameters to make priorities L < M < H

    chrono.start();
    threadL = new Periodic_Thread(RTConf(period * 1000, 0, wcet * 1000, 0, iterations, Thread::READY, Thread::LOW), &lowPriority);
    cout << "voltou pra main " << endl;
    Delay pickLockFirst(100000); // to make sure L starts first
    threadH = new Periodic_Thread(RTConf(period * 1000, 0, wcet * 1000, 0, iterations, Thread::READY, Thread::HIGH), &highPriority);
    threadM = new Periodic_Thread(RTConf(period * 1000, 0, wcet * 1000, 0 , iterations, Thread:: READY, Thread::NORMAL - 1), &mediumPriority);
    // Normal - 1 pq acho que se for == NORMAL ele faz o cálculo pelo Criterion -> LLF()
    cout << "Thread L: " << threadL << " Thread M: " << threadM <<  endl;

    threadL->join();
    threadH->join();
    threadM->join();

    chrono.stop();

    return 0;
}