// EPOS Alarm Initialization

#include <time.h>
#include <system.h>

__BEGIN_SYS

void Alarm::init()
{
    db<Init, Alarm>(TRC) << "Alarm::init()" << endl;

    _timer = new (kmalloc(sizeof(Alarm_Timer))) Alarm_Timer(handler);
}

__END_SYS
