// EPOS Priority Inversion Test Program

#include <time.h>
#include <process.h>
#include <real-time.h>
#include <synchronizer.h>

using namespace EPOS;

const unsigned int iterations = 2;
const unsigned int period = 500;  // ms
OStream cout;
Chronometer chrono;
Periodic_Thread * threads[3];

// Generic test arguments
struct Test_Args
{
    int test;
    char c;
    Mutex *mutex[3];
    Semaphore *semaphore[3];
    int thread_index; 
};

// Job
void work(char c, Periodic_Thread *thread) {
    cout << "Thread [" << c << "] " << "got the job" << endl;

    Microsecond elapsed = chrono.read() / 1000;
    int time = 0;
    int first_elapsed = elapsed;
    int last_cout = 0;

    while (time < 500) {
        if (last_cout + 50 < time) {
            cout << "Thread [" << c << "] " << "is working. Time elapsed: " << elapsed << ", priority: " << thread->priority() << endl;
            last_cout = time;
        }

        elapsed = chrono.read() / 1000;
        time = elapsed - first_elapsed;
    }
}

void simple_lock(char c, Periodic_Thread * thread, Mutex * mutex) {
    cout << "Thread [" << c << "] will try to the take lock, priority: " << thread->priority() << endl;
    mutex->lock();

    cout << "Thread [" << c << "] got the lock, priority: " << thread->priority() << endl;
    work(c, thread);

    cout << "Thread [" << c << "] will release the lock, priority: " << thread->priority() << endl;
    mutex->unlock();
}

// void nested_heavy_work(char c, Periodic_Thread *t) {

//     cout << "Thread " << c << " entered the heavy work" << endl;

//     Microsecond elapsed = chrono.read() / 1000;
//     int time = 0;
//     int first_elapsed = elapsed;
//     int last_cout = 0;

//     critical_section_second_test_second_mutex.lock();

//     cout << c << " got the lock and entered nested heavy work" << endl;

//     while (time < 500) {
//         if (last_cout + 50 < time) {
//             cout << c << " is working... time elapsed: " << elapsed << " priority: " << t->priority() << endl;;
//             last_cout = time;
//         }

//         elapsed = chrono.read() / 1000;
//         time = elapsed - first_elapsed;

//     }

//     cout << "Thread " << c << " will release the lock and finish the nested heavy work" << endl;

//     critical_section_second_test_second_mutex.unlock();

//     Delay test_preempt(1000000);

// }

// void heavy_work_with_sem(char c, Periodic_Thread *t) {

//     cout << "Thread " << c << " entered the heavy work" << endl;

//     Microsecond elapsed = chrono.read() / 1000;
//     int time = 0;
//     int first_elapsed = elapsed;
//     int last_cout = 0;

//     critical_section_third_test_second_semaphore->p();

//     cout << c << " got the semaphore and entered nested heavy work" << endl;

//     while (time < 500) {
//         if (last_cout + 50 < time) {
//             cout << c << " is working... time elapsed: " << elapsed << " priority: " << t->priority() << endl;;
//             last_cout = time;
//         }

//         elapsed = chrono.read() / 1000;
//         time = elapsed - first_elapsed;

//     }

//     cout << "Thread " << c << " will release the semaphore and finish the nested heavy work" << endl;

//     critical_section_third_test_second_semaphore->v();

//     Delay test_preempt(1000000);

// }

// // ---

// int low_priority_second() {
//     do {
//         second_task('L', thread_l_second_test);
//     } while(Periodic_Thread::wait_next());
//     return 'L';
// }

// int medium_priority_second() {
//     do {
//         second_task('M', thread_m_second_test);
//     } while(Periodic_Thread::wait_next());
//     return 'M';
// }

// int high_priority_second() {
//     do {
//         second_task('H', thread_h_second_test);
//     } while(Periodic_Thread::wait_next());
//     return 'H';
// }

// // ---

// int low_priority_third() {
//     do {
//         third_task('L', thread_l_third_test);
//     } while(Periodic_Thread::wait_next());
//     return 'L';
// }

// int medium_priority_third() {
//     do {
//         third_task('M', thread_m_third_test);
//     } while(Periodic_Thread::wait_next());
//     return 'M';
// }

// int high_priority_third() {
//     do {
//         third_task('H', thread_h_third_test);
//     } while(Periodic_Thread::wait_next());
//     return 'H';
// }

int test_task(Test_Args *args) {
    do {
        switch (args->test)
        {
        case 0:
        case 1:
            simple_lock(args->c, threads[args->thread_index], args->mutex[0]);
            break;
        case 2:
            if (args->c == 'L' || args->c == 'H')
                simple_lock(args->c, threads[args->thread_index], args->mutex[0]);
            else
                work(args->c, threads[args->thread_index]);
            break;
        default:
            return '?';
        }
    } while(Periodic_Thread::wait_next());

    return args->c;
}

void test_simple_case() {
    cout << "Test case 0: 1 thread [normal] and 1 mutex." << endl;
    cout << "Expected: The thread should finish its job without any issues." << endl;

    Mutex mutex;

    Test_Args args;
    args.test = 0;
    args.c = 'M';
    args.mutex[0] = &mutex;
    args.thread_index = 0;

    threads[0] = new Periodic_Thread(RTConf(period * 1000, 0, 0, 0, iterations, Thread::READY, Thread::NORMAL - 1), &test_task, &args);

    int status = threads[0]->join();

    delete threads[0];

    cout << "Thread [M] finished with status " << char(status) << "." << endl;
    cout << "\n\n\n";
}

void test_low_and_high_case() {
    cout << "Test case 1: 2 threads [low and high] and 1 mutex." << endl;
    cout << "Expected: The low priority thread should inherit the priority of the high priority thread." << endl;

    Mutex mutex;

    Test_Args args_l;
    args_l.test = 1;
    args_l.c = 'L';
    args_l.mutex[0] = &mutex;
    args_l.thread_index = 0;

    Test_Args args_h;
    args_h.test = 1;
    args_h.c = 'H';
    args_h.mutex[0] = &mutex;
    args_h.thread_index = 1;

    threads[0] = new Periodic_Thread(RTConf(period * 1000, 0, 0, 0, iterations, Thread::READY, Thread::LOW), &test_task, &args_l);
    Delay pick_lock_first(100000);
    threads[1] = new Periodic_Thread(RTConf(period * 1000, 0, 0, 0, iterations, Thread::READY, Thread::HIGH), &test_task, &args_h);

    int status_l = threads[0]->join();
    int status_h = threads[1]->join();

    delete threads[0];
    delete threads[1];

    cout << "Threads finished with the status: [L] = " << char(status_l) << ", [H] = " << char(status_h) << endl;
    cout << "\n\n\n";
}

// /* 
// First test: Classic  

// How the inversion priority occurs:

// 1. Thread L should start, acquire the lock and start the heavy work;
// 2. Thread H can't start the work because is waiting for lock;
// 3. Thread M should preempt L and delay the lock release from H;
// 4. So Thread M has "higher priority" than H without being in the critical section.

// How to pass the test:

// 1. Thread L starts and finish the heavy work and it's not preempted;
// 2. Thread H waits until L releases the lock, then acquire it;
// 3. Thread M waits until H releases the lock.

// */
// void test_classical_case() {
//     cout << "Test case 2: 3 threads [low, medium and high] and 1 mutex." << endl;
//     cout << "Expected: The low priority thread should inherit the priority of the high priority thread. The medium"
//          << "thread should not cause priority inversions." << endl;

//     Mutex mutex;

//     Test_Args args_l;
//     args_l.test = 2;
//     args_l.c = 'L';
//     args_l.mutex[0] = &mutex;
//     args_l.thread_index = 0;

//     Test_Args args_m;
//     args_m.test = 2;
//     args_m.c = 'M';
//     args_m.mutex[0] = &mutex;
//     args_m.thread_index = 1;

//     Test_Args args_h;
//     args_h.test = 2;
//     args_h.c = 'H';
//     args_h.mutex[0] = &mutex;
//     args_h.thread_index = 2;

//     threads[0] = new Periodic_Thread(RTConf(period * 1000, 0, 0, 0, iterations, Thread::READY, Thread::LOW), &test_task, args_l);
//     Delay pick_lock_first(100000);
//     threads[1] = new Periodic_Thread(RTConf(period * 1000, 0, 0, 0, iterations, Thread::READY, Thread::HIGH), &test_task, args_h);
//     threads[2] = new Periodic_Thread(RTConf(period * 1000, 0, 0, 0, iterations, Thread::READY, Thread::NORMAL - 1), &test_task, args_m);

//     int status_l = threads[0]->join();
//     int status_m = threads[1]->join();
//     int status_h = threads[2]->join();

//     delete threads[0];
//     delete threads[1];
//     delete threads[2];

//     cout << "Threads finished with the status: [L] = [" << char(status_l) << "], [M] = " << char(status_m) << ", [H] = " << char(status_h) << endl;
//     cout << "\n\n\n";
// }

int main() {
    chrono.start();

    test_simple_case();
    test_low_and_high_case();
    // test_classical_case();

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

    // cout << "Priority Inversion Test - Nested Mutexes" << endl;

    // // Threads

    // thread_l_second_test = new Periodic_Thread(RTConf(period * 1000, 0, 0, 0, iterations, Thread::READY, Thread::LOW), &low_priority_second);
    // Delay pick_lock_second(100000);

    // thread_h_second_test = new Periodic_Thread(RTConf(period * 1000, 0, 0, 0, iterations, Thread::READY, Thread::HIGH), &high_priority_second);
    // thread_m_second_test = new Periodic_Thread(RTConf(period * 1000, 0, 0, 0 , iterations, Thread:: READY, Thread::NORMAL - 1), &medium_priority_second);

    // int status_l_second = thread_l_second_test->join();
    // int status_h_second = thread_h_second_test->join();
    // int status_m_second = thread_m_second_test->join();

    // cout << " Threads finished, s[L] = " << char(status_l_second) << " s[H] = " << char(status_h_second)  << " s[M]= "<< char(status_m_second) << endl;
    // cout << "\n\n\n";

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

    // cout << "Priority Inversion Test - Nested Binary Semaphores" << endl;

    // // Semaphores

    // critical_section_third_test_first_semaphore = new Semaphore;
    // critical_section_third_test_second_semaphore = new Semaphore;

    // // Threads

    // thread_l_third_test = new Periodic_Thread(RTConf(period * 1000, 0, 0, 0, iterations, Thread::READY, Thread::LOW), &low_priority_third);
    // Delay pick_lock_third(100000);

    // thread_h_third_test = new Periodic_Thread(RTConf(period * 1000, 0, 0, 0, iterations, Thread::READY, Thread::HIGH), &high_priority_third);
    // thread_m_third_test = new Periodic_Thread(RTConf(period * 1000, 0, 0, 0 , iterations, Thread:: READY, Thread::NORMAL - 1), &medium_priority_third);

    // int status_l_third = thread_l_third_test->join();
    // int status_h_third = thread_h_third_test->join();
    // int status_m_third = thread_m_third_test->join();

    // cout << " Threads finished, s[L] = " << char(status_l_third) << " s[H] = " << char(status_h_third)  << " s[M]= "<< char(status_m_third) << endl;
    // cout << "\n\n\n";

    // // Finishing tests...

    // delete critical_section_third_test_first_semaphore;
    // delete critical_section_third_test_second_semaphore;

    chrono.stop();

    return 0;
}
