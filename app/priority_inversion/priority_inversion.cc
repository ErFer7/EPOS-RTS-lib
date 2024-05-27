// EPOS Priority Inversion Test Program

#include <process.h>
#include <time.h>
#include <real-time.h>
#include <synchronizer.h>

using namespace EPOS;

const unsigned int iterations = 2;
const unsigned int period = 500;  // ms
const unsigned int low_capacity = 0;
const unsigned int medium_capacity = 100;
const unsigned int high_capacity = 200;

int test = 0;

OStream cout;
Chronometer chrono;
Periodic_Thread * threads[3];
Mutex print_mutex = Mutex(false);

struct Test_Args
{
    Test_Args(char c, int thread_index): c(c), thread_index(thread_index) {}

    char c;
    int thread_index;
    Mutex * mutex[3];
    Semaphore * semaphore;
};

void print(Periodic_Thread * thread, char c, const char * message);

void work(char c, Periodic_Thread *thread);

void simple_lock(char c, Periodic_Thread *thread, Mutex *mutex);
void nested_lock(char c, Periodic_Thread *thread, Mutex *mutex, Mutex *nested_mutex);
void doubly_nested_lock(char c, Periodic_Thread *thread, Mutex *mutex, Mutex *nested_mutex, Mutex *doubly_nested_mutex);
void simple_lock_semaphore(char c, Periodic_Thread *thread, Semaphore *semaphore);

int test_task(Test_Args *args);

void test_simple_case();
void test_low_and_high_case();
void test_classical_case();
void test_nested_case();
void test_doubly_nested_case();
void test_simple_semaphore_case();

// The static scheduling was used to make it easier to see the priority inversion being handled.
int main() {
    chrono.start();

    test_simple_case();
    test_low_and_high_case();
    test_classical_case();
    test_nested_case();
    test_doubly_nested_case();
    test_simple_semaphore_case();

    chrono.stop();

    return 0;
}

void print(Periodic_Thread * thread, char c, const char * message) {
    print_mutex.lock();
    cout << "CPU [" << CPU::id() << "]" << " Thread [" << c << "]" << " Priority [" << thread->priority() << "]: " << message << endl;
    print_mutex.unlock();
}

void work(char c, Periodic_Thread *thread) {
    print(thread, c, "got the job");

    Microsecond elapsed = chrono.read() / 1000;
    int time = 0;
    int first_elapsed = elapsed;
    int last_cout = 0;

    while (time < 500) {
        if (last_cout + 50 < time) {
            print(thread, c, "is working");
            last_cout = time;
        }

        elapsed = chrono.read() / 1000;
        time = elapsed - first_elapsed;
    }
}

void simple_lock(char c, Periodic_Thread * thread, Mutex * mutex) {
    print(thread, c, "will try to take the lock");
    mutex->lock();
    print(thread, c, "got the lock");

    work(c, thread);

    print(thread, c, "will release the lock");
    mutex->unlock();
    print(thread, c, "released the lock");
}

void nested_lock(char c, Periodic_Thread * thread, Mutex * mutex, Mutex *nested_mutex) {
    print(thread, c, "will try to take the lock");
    mutex->lock();
    print(thread, c, "got the lock");
    print(thread, c, "will try to take the nested lock");
    nested_mutex->lock();
    print(thread, c, "got the nested lock");

    work(c, thread);

    print(thread, c, "will release the nested lock");
    nested_mutex->unlock();
    print(thread, c, "released the nested lock");
    print(thread, c, "will release the lock");
    mutex->unlock();
    print(thread, c, "released the lock");
}

void doubly_nested_lock(char c, Periodic_Thread * thread, Mutex * mutex, Mutex *nested_mutex, Mutex *doubly_nested_mutex) {
    print(thread, c, "will try to take the lock");
    mutex->lock();
    print(thread, c, "got the lock");
    print(thread, c, "will try to take the nested lock");
    nested_mutex->lock();
    print(thread, c, "got the nested lock");
    print(thread, c, "will try to take the doubly nested lock");
    doubly_nested_mutex->lock();
    print(thread, c, "got the doubly nested lock");

    work(c, thread);

    print(thread, c, "will release the doubly nested lock");
    doubly_nested_mutex->unlock();
    print(thread, c, "released the doubly nested lock");
    print(thread, c, "will release the nested lock");
    nested_mutex->unlock();
    print(thread, c, "released the nested lock");
    print(thread, c, "will release the lock");
    mutex->unlock();
    print(thread, c, "released the lock");
}

void simple_lock_semaphore(char c, Periodic_Thread *thread, Semaphore *semaphore) {
    print(thread, c, "will try to take the semaphore");
    semaphore->p();
    print(thread, c, "got the semaphore");

    work(c, thread);

    print(thread, c, "will release the semaphore");
    semaphore->v();
    print(thread, c, "released the semaphore");
}

int test_task(Test_Args *args) {
    bool locking_threads = args->c == 'L' || args->c == 'H';

    do {
        if (locking_threads) {
            switch (test)
            {
            case 1:
                simple_lock(args->c, threads[args->thread_index], args->mutex[0]);
                break;
            case 2:
                simple_lock(args->c, threads[args->thread_index], args->mutex[0]);
                break;
            case 3:
                nested_lock(args->c, threads[args->thread_index], args->mutex[0], args->mutex[1]);
                break;
            case 4:
                doubly_nested_lock(args->c, threads[args->thread_index], args->mutex[0], args->mutex[1], args->mutex[2]);
                break;
            case 5:
                simple_lock_semaphore(args->c, threads[args->thread_index], args->semaphore);
                break;
            default:
                return '?';
            }
        } else {
            if (test >= 2)
                work(args->c, threads[args->thread_index]);
            else
                simple_lock(args->c, threads[args->thread_index], args->mutex[0]);  // Test 0
        }
    } while(Periodic_Thread::wait_next());

    return args->c;
}

int hi() {
    cout << "Hi" << endl;

    return 0;
}

void test_simple_case() {
    cout << "Test case 0: 1 thread [normal] and 1 mutex." << endl;
    cout << "Expected: The thread should finish its job without any issues." << endl;

    Mutex * mutex = new Mutex();

    Test_Args * args = new Test_Args('M', 0);
    args->mutex[0] = mutex;

    threads[0] = new Periodic_Thread(RTConf(period * 1000, 0, medium_capacity * 1000, 0, iterations), &test_task, args);

    int status = threads[0]->join();

    delete threads[0];
    delete mutex;
    delete args;

    cout << "Thread [M] finished with status " << char(status) << "." << endl;
    cout << "\n\n\n";

    test++;
}

void test_low_and_high_case() {
    cout << "Test case 1: 2 threads [low and high] and 1 mutex." << endl;
    cout << "Expected: The low priority thread should inherit the priority of the high priority thread." << endl;

    Mutex * mutex = new Mutex();

    Test_Args * args_l = new Test_Args('L', 0);
    args_l->mutex[0] = mutex;

    Test_Args * args_h = new Test_Args('H', 1);
    args_h->mutex[0] = mutex;

    threads[0] = new Periodic_Thread(RTConf(period * 1000, 0, low_capacity * 1000, 0, iterations), &test_task, args_l);
    Delay pick_lock_first(100000);
    threads[1] = new Periodic_Thread(RTConf(period * 1000, 0, high_capacity * 1000, 0, iterations), &test_task, args_h);

    int status_l = threads[0]->join();
    int status_h = threads[1]->join();

    delete threads[0];
    delete threads[1];
    delete mutex;
    delete args_h;
    delete args_l;

    cout << "Threads finished with the status: [L] = " << char(status_l) << ", [H] = " << char(status_h) << endl;
    cout << "\n\n\n";

    test++;
}

void test_classical_case() {
    cout << "Test case 2: 3 threads [low, medium and high] and 1 mutex." << endl;
    cout << "Expected: The low priority thread should inherit the priority of the high priority thread. The medium "
         << "thread should not cause priority inversions." << endl;

    Mutex * mutex = new Mutex();

    Test_Args * args_l = new Test_Args('L', 0);
    args_l->mutex[0] = mutex;

    Test_Args * args_m = new Test_Args('M', 2);

    Test_Args * args_h = new Test_Args('H', 1);
    args_h->mutex[0] = mutex;

    threads[0] = new Periodic_Thread(RTConf(period * 1000, 0, low_capacity * 1000, 0, iterations), &test_task, args_l);
    Delay pick_lock_first(100000);
    threads[1] = new Periodic_Thread(RTConf(period * 1000, 0, high_capacity * 1000, 0, iterations), &test_task, args_h);
    threads[2] = new Periodic_Thread(RTConf(period * 1000, 0, medium_capacity * 1000, 0, iterations), &test_task, args_m);

    int status_l = threads[0]->join();
    int status_m = threads[2]->join();
    int status_h = threads[1]->join();

    delete threads[0];
    delete threads[1];
    delete threads[2];
    delete mutex;
    delete args_l;
    delete args_m;
    delete args_h;

    cout << "Threads finished with the status: [L] = " << char(status_l) << ", [M] = " << char(status_m) << ", [H] = " << char(status_h) << endl;
    cout << "\n\n\n";

    test++;
}

void test_nested_case() {
    cout << "Test case 3: 3 threads [low, medium and high] and 2 mutexes." << endl;
    cout << "Expected: The low priority thread should inherit the priority of the high priority thread. The medium "
         << "thread should not cause priority inversions. The priority must be handled correctly across nested mutexes." << endl;

    Mutex * mutex = new Mutex();
    Mutex * nested_mutex = new Mutex();

    Test_Args * args_l = new Test_Args('L', 0);
    args_l->mutex[0] = mutex;
    args_l->mutex[1] = nested_mutex;

    Test_Args * args_m = new Test_Args('M', 2);

    Test_Args * args_h = new Test_Args('H', 1);
    args_h->mutex[0] = mutex;
    args_h->mutex[1] = nested_mutex;

    threads[0] = new Periodic_Thread(RTConf(period * 1000, 0, low_capacity * 1000, 0, iterations), &test_task, args_l);
    Delay pick_lock_first(100000);
    threads[1] = new Periodic_Thread(RTConf(period * 1000, 0, high_capacity * 1000, 0, iterations), &test_task, args_h);
    threads[2] = new Periodic_Thread(RTConf(period * 1000, 0, medium_capacity * 1000, 0, iterations), &test_task, args_m);

    int status_l = threads[0]->join();
    int status_m = threads[2]->join();
    int status_h = threads[1]->join();

    delete threads[0];
    delete threads[1];
    delete threads[2];
    delete mutex;
    delete nested_mutex;
    delete args_l;
    delete args_m;
    delete args_h;

    cout << "Threads finished with the status: [L] = " << char(status_l) << ", [M] = " << char(status_m) << ", [H] = " << char(status_h) << endl;
    cout << "\n\n\n";

    test++;
}

void test_doubly_nested_case() {
    cout << "Test case 4: 3 threads [low, medium and high] and 3 mutexes." << endl;
    cout << "Expected: The low priority thread should inherit the priority of the high priority thread. The medium "
         << "thread should not cause priority inversions. The priority must be handled correctly across nested mutexes." << endl;

    Mutex * mutex = new Mutex();
    Mutex * nested_mutex = new Mutex();
    Mutex * doubly_nested_mutex = new Mutex();

    Test_Args * args_l = new Test_Args('L', 0);
    args_l->mutex[0] = mutex;
    args_l->mutex[1] = nested_mutex;
    args_l->mutex[2] = doubly_nested_mutex;

    Test_Args * args_m = new Test_Args('M', 2);

    Test_Args * args_h = new Test_Args('H', 1);
    args_h->mutex[0] = mutex;
    args_h->mutex[1] = nested_mutex;
    args_h->mutex[2] = doubly_nested_mutex;

    threads[0] = new Periodic_Thread(RTConf(period * 1000, 0, low_capacity * 1000, 0, iterations), &test_task, args_l);
    Delay pick_lock_first(100000);
    threads[1] = new Periodic_Thread(RTConf(period * 1000, 0, high_capacity * 1000, 0, iterations), &test_task, args_h);
    threads[2] = new Periodic_Thread(RTConf(period * 1000, 0, medium_capacity * 1000, 0, iterations), &test_task, args_m);

    int status_l = threads[0]->join();
    int status_m = threads[2]->join();
    int status_h = threads[1]->join();

    delete threads[0];
    delete threads[1];
    delete threads[2];
    delete mutex;
    delete nested_mutex;
    delete doubly_nested_mutex;
    delete args_l;
    delete args_m;
    delete args_h;

    cout << "Threads finished with the status: [L] = " << char(status_l) << ", [M] = " << char(status_m) << ", [H] = " << char(status_h) << endl;
    cout << "\n\n\n";

    test++;
}

void test_simple_semaphore_case() {
    cout << "Test case 6: 3 threads [low, medium and high] and 1 semaphore." << endl;
    cout << "Expected: The low priority thread should inherit the priority of the high priority thread. The medium "
         << "thread should not cause priority inversions." << endl;

    Semaphore * semaphore = new Semaphore(1);

    Test_Args * args_l = new Test_Args('L', 0);
    args_l->semaphore = semaphore;

    Test_Args * args_m = new Test_Args('M', 2);

    Test_Args * args_h = new Test_Args('H', 1);
    args_h->semaphore = semaphore;

    threads[0] = new Periodic_Thread(RTConf(period * 1000, 0, low_capacity * 1000, 0, iterations), &test_task, args_l);
    Delay pick_lock_first(100000);
    threads[1] = new Periodic_Thread(RTConf(period * 1000, 0, high_capacity * 1000, 0, iterations), &test_task, args_h);
    threads[2] = new Periodic_Thread(RTConf(period * 1000, 0, medium_capacity * 1000, 0, iterations), &test_task, args_m);

    int status_l = threads[0]->join();
    int status_m = threads[2]->join();
    int status_h = threads[1]->join();


    delete threads[0];
    delete threads[1];
    delete threads[2];
    delete semaphore;
    delete args_l;
    delete args_m;
    delete args_h;

    cout << "Threads finished with the status: [L] = " << char(status_l) << ", [M] = " << char(status_m) << ", [H] = " << char(status_h) << endl;
    cout << "\n\n\n";
}
