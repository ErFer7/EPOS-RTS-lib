#include <process.h>
#include <time.h>
#include <real-time.h>
#include <synchronizer.h>

using namespace EPOS;

const int work_time = 10000000; // us

Mutex print_mutex;
OStream cout;
Thread * threads[8];
Chronometer chrono;

void print();

int simple_task();

void test_single_thread_creation();
void test_multiple_threads_creation();
void test_threads_over_time();

int main()
{
    cout << "Running multicore tests" << endl;

    chrono.start();

    test_single_thread_creation();
    test_multiple_threads_creation();
    test_threads_over_time();

    chrono.stop();

    return 0;
}

void print() {
    print_mutex.lock();
    cout << "Thread [" << Thread::self() << "] is running on core [" << CPU::id() << "]" << endl;
    print_mutex.unlock();
}

int simple_task() {
    print();
    return 0;
}

int heavy_task() {

    int initial_time = chrono.read();
    int final_time = 0;
    int total_elapsed_time = 0;
    int last_print_time = 0;
    int print_interval = 10000;

    do {
        final_time = chrono.read();

        if (last_print_time > print_interval) {
            last_print_time = 0;
            print();
        }

        int elapsed_time = final_time - initial_time;
        last_print_time += elapsed_time;
        total_elapsed_time += elapsed_time;
    } while (total_elapsed_time < work_time);

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
    for (int i = 0; i < 8; i++) {
        threads[i] = new Thread(&simple_task);
    }

    for (int i = 0; i < 8; i++) {
        int status = threads[i]->join();
        delete threads[i];
        if (status != 0) {
            cout << "Test 2: Failed" << endl;
            return;
        }
    }

    cout << "Test 2: Passed\n" << endl;
}

void test_threads_over_time() {
    cout << "Test 3: Creating multiple threads over time" << endl;
    for (int i = 0; i < 8; i++) {
        threads[i] = new Thread(&heavy_task);
    }

    for (int i = 0; i < 8; i++) {
        int status = threads[i]->join();
        delete threads[i];
        if (status != 0) {
            cout << "Test 3: Failed" << endl;
            return;
        }
    }

    cout << "Test 3: Passed\n" << endl;
}
