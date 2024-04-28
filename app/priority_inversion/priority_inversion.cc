// EPOS Priority Inversion Test Program

#include <time.h>
#include <process.h>
#include <real-time.h>
#include <synchronizer.h>

using namespace EPOS;

const unsigned int iterations = 2; // for the test to run faster
const unsigned int period = 500; // ms
OStream cout;
Chronometer chrono;

// First test

Mutex critical_section_first_test;
Periodic_Thread * thread_l_first_test;
Periodic_Thread * thread_m_first_test;
Periodic_Thread * thread_h_first_test;

// Second Test

Mutex critical_section_second_test_first_mutex;
Mutex critical_section_second_test_second_mutex;
Periodic_Thread * thread_l_second_test;
Periodic_Thread * thread_m_second_test;
Periodic_Thread * thread_h_second_test;

// Third Test

Semaphore * critical_section_third_test_first_semaphore;
Semaphore * critical_section_third_test_second_semaphore;
Periodic_Thread * thread_l_third_test;
Periodic_Thread * thread_m_third_test;
Periodic_Thread * thread_h_third_test;

// Job

void heavy_work(char c, Periodic_Thread *t) {

    cout << c << " entered the heavy work" << endl;

    Microsecond elapsed = chrono.read() / 1000;
    int time = 0;
    int first_elapsed = elapsed;
    int last_cout = 0;

    while (time < 500) {
        if (last_cout + 50 < time) {
            cout << c << " is working... time elapsed: " << elapsed << " priority: " << t->priority() << endl;;
            last_cout = time;
        }

        elapsed = chrono.read() / 1000;
        time = elapsed - first_elapsed;
    }
}

void nested_heavy_work(char c, Periodic_Thread *t) {

    cout << "Thread " << c << " entered the heavy work" << endl;

    Microsecond elapsed = chrono.read() / 1000;
    int time = 0;
    int first_elapsed = elapsed;
    int last_cout = 0;

    critical_section_second_test_second_mutex.lock();

    cout << c << " got the lock and entered nested heavy work" << endl;

    while (time < 500) {
        if (last_cout + 50 < time) {
            cout << c << " is working... time elapsed: " << elapsed << " priority: " << t->priority() << endl;;
            last_cout = time;
        }

        elapsed = chrono.read() / 1000;
        time = elapsed - first_elapsed;

    }

    cout << "Thread " << c << " will release the lock and finish the nested heavy work" << endl;

    critical_section_second_test_second_mutex.unlock();

    Delay test_preempt(1000000);

}

void heavy_work_with_sem(char c, Periodic_Thread *t) {

    cout << "Thread " << c << " entered the heavy work" << endl;

    Microsecond elapsed = chrono.read() / 1000;
    int time = 0;
    int first_elapsed = elapsed;
    int last_cout = 0;

    critical_section_third_test_second_semaphore->p();

    cout << c << " got the semaphore and entered nested heavy work" << endl;

    while (time < 500) {
        if (last_cout + 50 < time) {
            cout << c << " is working... time elapsed: " << elapsed << " priority: " << t->priority() << endl;;
            last_cout = time;
        }

        elapsed = chrono.read() / 1000;
        time = elapsed - first_elapsed;

    }

    cout << "Thread " << c << " will release the semaphore and finish the nested heavy work" << endl;

    critical_section_third_test_second_semaphore->v();

    Delay test_preempt(1000000);

}

// Tasks

void first_task(char c, Periodic_Thread* this_thread) {
    cout << "Thread " << c << " tries to take lock, [p(" << c << ") = " << this_thread->priority() << "]" << endl;
    critical_section_first_test.lock();

    cout << "Thread " << c << " got the lock" << endl;
    heavy_work(c, this_thread);

    cout << "Thread " << c << " will release the lock and finish the heavy work" << endl;
    critical_section_first_test.unlock();
}

void second_task(char c, Periodic_Thread* this_thread) {
    cout << "Thread " << c << " tries to take lock, [p(" << c << ") = " << this_thread->priority() << "]" << endl;
    critical_section_second_test_first_mutex.lock();

    cout << "Thread " << c << " got the lock" << endl;
    nested_heavy_work(c, this_thread);

    cout << "Thread " << c << " will release the lock and finish the heavy work" << endl;
    critical_section_second_test_first_mutex.unlock();
}

void third_task(char c, Periodic_Thread* this_thread) {
    cout << "Thread " << c << " tries to take semaphore, [p(" << c << ") = " << this_thread->priority() << "]" << endl;
    critical_section_third_test_first_semaphore->p();

    cout << "Thread " << c << " got the semaphore" << endl;
    heavy_work_with_sem(c, this_thread);

    cout << "Thread " << c << " will release the semaphore and finish the heavy work" << endl;
    critical_section_third_test_first_semaphore->v();
}

// Assignments by priority

int low_priority_first() {
    do {
        first_task('L', thread_l_first_test);
    } while(Periodic_Thread::wait_next());
    return 'L';
}

int medium_priority_first() {
    do {
        first_task('M', thread_m_first_test);
    } while(Periodic_Thread::wait_next());
    return 'M';
}

int high_priority_first() {
    do {
        first_task('H', thread_h_first_test);
    } while(Periodic_Thread::wait_next());
    return 'H';
}

// ---

int low_priority_second() {
    do {
        second_task('L', thread_l_second_test);
    } while(Periodic_Thread::wait_next());
    return 'L';
}

int medium_priority_second() {
    do {
        second_task('M', thread_m_second_test);
    } while(Periodic_Thread::wait_next());
    return 'M';
}

int high_priority_second() {
    do {
        second_task('H', thread_h_second_test);
    } while(Periodic_Thread::wait_next());
    return 'H';
}

// ---

int low_priority_third() {
    do {
        third_task('L', thread_l_third_test);
    } while(Periodic_Thread::wait_next());
    return 'L';
}

int medium_priority_third() {
    do {
        third_task('M', thread_m_third_test);
    } while(Periodic_Thread::wait_next());
    return 'M';
}

int high_priority_third() {
    do {
        third_task('H', thread_h_third_test);
    } while(Periodic_Thread::wait_next());
    return 'H';
}

int main() {

    /* 
    First test: Classic  

    How the inversion priority occurs:

    1. Thread L should start, acquire the lock and start the heavy work;
    2. Thread H can't start the work because is waiting for lock;
    3. Thread M should preempt L and delay the lock release from H;
    4. So Thread M has "higher priority" than H without being in the critical section.

    How to pass the test:

    1. Thread L starts and finish the heavy work and it's not preempted;
    2. Thread H waits until L releases the lock, then acquire it;
    3. Thread M waits until H releases the lock.

    */

    cout << "Priority Inversion Test - Classic" << endl;

    chrono.start();

    // Threads

    thread_l_first_test = new Periodic_Thread(RTConf(period * 1000, 0, 0, 0, iterations, Thread::READY, Thread::LOW), &low_priority_first);
    Delay pick_lock_first(100000);

    thread_h_first_test = new Periodic_Thread(RTConf(period * 1000, 0, 0, 0, iterations, Thread::READY, Thread::HIGH), &high_priority_first);
    // Normal - 1 because if it's == NORMAL the calculation is made by Criterion -> LLF()
    thread_m_first_test = new Periodic_Thread(RTConf(period * 1000, 0, 0, 0 , iterations, Thread:: READY, Thread::NORMAL - 1), &medium_priority_first);

    int status_l_first = thread_l_first_test->join();
    int status_h_first = thread_h_first_test->join();
    int status_m_first = thread_m_first_test->join();

    cout << " Threads finished, s[L] = " << char(status_l_first) << " s[H] = " << char(status_h_first)  << " s[M]= "<< char(status_m_first) << endl;
    cout << "\n\n\n";

    /*

    Second test: Nested Mutexes

    How the problem with nested Mutexes occurs:

    1. Thread L take a lock, but inside the critical zone there's another (or several others) lock(s), which L takes as well;
    2. Thread L's priority is raised by one of the algorithms (PCP or Inheritance);
    3. Thread L releases a lock and its priority returns to the original value;
    4. A Thread with higher priority preempts L.

    How to pass the test:

    - If the priority of a Thread raises temporarily inside a critical zone, it returns to the original value only when this Thread exit the zone.

    */

    cout << "Priority Inversion Test - Nested Mutexes" << endl;

    // Threads

    thread_l_second_test = new Periodic_Thread(RTConf(period * 1000, 0, 0, 0, iterations, Thread::READY, Thread::LOW), &low_priority_second);
    Delay pick_lock_second(100000);

    thread_h_second_test = new Periodic_Thread(RTConf(period * 1000, 0, 0, 0, iterations, Thread::READY, Thread::HIGH), &high_priority_second);
    thread_m_second_test = new Periodic_Thread(RTConf(period * 1000, 0, 0, 0 , iterations, Thread:: READY, Thread::NORMAL - 1), &medium_priority_second);

    int status_l_second = thread_l_second_test->join();
    int status_h_second = thread_h_second_test->join();
    int status_m_second = thread_m_second_test->join();

    cout << " Threads finished, s[L] = " << char(status_l_second) << " s[H] = " << char(status_h_second)  << " s[M]= "<< char(status_m_second) << endl;
    cout << "\n\n\n";

    /* 

    Third test: Nested Binary Semaphores

    ** Priority inversion in the case of non-binary semaphores is a really complex issue **

    How the problem with nested Binary Semaphores occurs:

    1. Thread L takes a semaphore, but inside the critical zone there's another (or several others) semaphore(s), which L takes as well;
    2. Thread L's priority is raised by one of the algorithms (PCP or Inheritance);
    3. Thread L releases a semaphore and its priority returns to the original value;
    4. A Thread with higher priority preempts L.

    How to pass the test:

    - If the priority of a Thread raises temporarily inside a critical zone, it returns to the original value only when this Thread exit the zone.

    */

    cout << "Priority Inversion Test - Nested Binary Semaphores" << endl;

    // Semaphores

    critical_section_third_test_first_semaphore = new Semaphore;
    critical_section_third_test_second_semaphore = new Semaphore;

    // Threads

    thread_l_third_test = new Periodic_Thread(RTConf(period * 1000, 0, 0, 0, iterations, Thread::READY, Thread::LOW), &low_priority_third);
    Delay pick_lock_third(100000);

    thread_h_third_test = new Periodic_Thread(RTConf(period * 1000, 0, 0, 0, iterations, Thread::READY, Thread::HIGH), &high_priority_third);
    thread_m_third_test = new Periodic_Thread(RTConf(period * 1000, 0, 0, 0 , iterations, Thread:: READY, Thread::NORMAL - 1), &medium_priority_third);

    int status_l_third = thread_l_third_test->join();
    int status_h_third = thread_h_third_test->join();
    int status_m_third = thread_m_third_test->join();

    cout << " Threads finished, s[L] = " << char(status_l_third) << " s[H] = " << char(status_h_third)  << " s[M]= "<< char(status_m_third) << endl;
    cout << "\n\n\n";

    // Finishing tests...

    delete critical_section_third_test_first_semaphore;
    delete critical_section_third_test_second_semaphore;

    chrono.stop();

    return 0;
}
