#include <process.h>
#include <time.h>
#include <real-time.h>
#include <synchronizer.h>

using namespace EPOS;

OStream cout;
Thread * threads[8];

int simple_task();

void test_single_thread_creation();
void test_multiple_threads_creation();

int main()
{
    cout << "Running multicore tests" << endl;

    test_single_thread_creation();
    test_multiple_threads_creation();

    return 0;
}

int simple_task() {
    cout << "Thread [" << Thread::self() << "] is running on core [" << CPU::id() << "]" << endl;
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
