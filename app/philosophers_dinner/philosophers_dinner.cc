// EPOS Scheduler Test Program

#include <machine/display.h>
#include <time.h>
#include <synchronizer.h>
#include <process.h>

using namespace EPOS;

const int iterations = 10;

Mutex table;

Thread * phil[5];
Semaphore * chopstick[5];

OStream cout;

int philosopher(int n, int l, int c);

int main()
{
    //
    Display::clear();
    Display::position(0, 0);
    cout << "The Philosopher's Dinner:" << endl;

    for(int i = 0; i < 5; i++)
        chopstick[i] = new Semaphore;

    phil[0] = new Thread(&philosopher, 0,  5, 32);
    phil[1] = new Thread(&philosopher, 1, 10, 44);
    phil[2] = new Thread(&philosopher, 2, 16, 39);
    phil[3] = new Thread(&philosopher, 3, 16, 24);
    phil[4] = new Thread(&philosopher, 4, 10, 20);

    cout << "Philosophers are alive and hungry!" << endl;

    
    Display::position(7, 44);
    cout << '/';
    Display::position(13, 44);
    cout << '\\';
    Display::position(16, 35);
    cout << '|';
    Display::position(13, 27);
    cout << '/';
    Display::position(7, 27);
    cout << '\\';
    Display::position(19, 0);
    

    cout << "The dinner is served ..." << endl;
    

    for(int i = 0; i < 5; i++) {
        int ret = phil[i]->join();
        
        Display::position(20 + i, 0);
        cout << "Philosopher " << i << " ate " << ret << " times " << endl;
        
    }

    for(int i = 0; i < 5; i++)
        delete chopstick[i];
    for(int i = 0; i < 5; i++)
        delete phil[i];

    cout << "The end!" << endl;

    return 0;
}

int philosopher(int n, int l, int c)
{
    unsigned int first = (n < 4)? n : 0;
    int second = (n < 4)? n + 1 : 4;

    for(int i = iterations; i > 0; i--) {

        
        Display::position(l, c);
        cout << "thinking\n";
        

        Delay thinking(10000000);

        Display::position(l, c);
        table.lock();
        cout << "\nFirst: " << first << ".\n" << endl;
        table.unlock();
        table.lock();
        cout << "\nSecond: " << second << ".\n" << endl;
        table.unlock();

        chopstick[first]->p();   // get first chopstick
        chopstick[second]->p();  // get second chopstick

        
        Display::position(l, c);
        cout << "eating\n";
        

        Delay eating(500000);

        
        Display::position(l, c);
        cout << "\n  sate  \n";
        

        chopstick[first]->v();   // release first chopstick
        chopstick[second]->v();  // release second chopstick
    }

    
    Display::position(l, c);
    cout << "  done  ";
    

    return iterations;
}
