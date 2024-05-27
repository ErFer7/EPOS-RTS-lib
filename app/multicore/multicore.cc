#include <process.h>
#include <time.h>
#include <real-time.h>
#include <synchronizer.h>
#include <utility/spin.h>

using namespace EPOS;

const int work_time = 1000000; // us
const int thread_count = 16;

int print_count = 0;
Mutex print_lock;
OStream cout;
Thread * threads[thread_count];
Chronometer chrono;

void print();

int simple_task();

void test_single_thread_creation();
void test_multiple_threads_creation();
void test_threads_with_heavy_work();

int main()
{
    cout << "Running multicore tests with " << CPU::cores() << " cores:" << endl;

    chrono.start();

    test_single_thread_creation();
    test_multiple_threads_creation();
    test_threads_with_heavy_work();

    chrono.stop();

    return 0;
}

void print() {
    print_lock.lock();
    cout << "CPU [" << CPU::id() << "]" << " Thread [" << Thread::self() << "]" << " Priority [" << Thread::self()->priority() << "]: " << "is running (" << print_count++ << ")" << endl;
    print_lock.unlock();
}

int simple_task() {
    print();
    return 0;
}

int heavy_task() {

    int initial_time = chrono.read();
    int last_print_time = chrono.read();
    int print_time = last_print_time;
    const int print_interval = 250000;

    do {
        if (print_time > print_interval) {
            last_print_time = chrono.read();
            print();
        }

        print_time = chrono.read() - last_print_time;
    } while (chrono.read() - initial_time < work_time);

    return 0;
}

void test_single_thread_creation() {
    cout << "Test 1: Creating a single thread" << endl;
    threads[0] = new Thread(&simple_task);

    int status = threads[0]->join();

    delete threads[0];

    if (status == 0)
        cout << "Test 1: Passed\n" << endl;
    else
        cout << "Test 1: Failed\n" << endl;
}

void test_multiple_threads_creation() {
    cout << "Test 2: Creating multiple threads" << endl;
    for (int i = 0; i < thread_count; i++) {
        threads[i] = new Thread(&simple_task);
    }

    for (int i = 0; i < thread_count; i++) {
        int status = threads[i]->join();
        delete threads[i];
        if (status != 0) {
            cout << "Test 2: Failed" << endl;
            return;
        }
    }

    cout << "Test 2: Passed\n" << endl;
}

void test_threads_with_heavy_work() {
    cout << "Test 3: Creating multiple threads over time" << endl;
    for (int i = 0; i < thread_count; i++) {
        threads[i] = new Thread(&heavy_task);
    }

    for (int i = 0; i < thread_count; i++) {
        int status = threads[i]->join();
        delete threads[i];
        if (status != 0) {
            cout << "Test 3: Failed" << endl;
            return;
        }
    }

    cout << "Test 3: Passed\n" << endl;
}
