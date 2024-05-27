// EPOS PC Mediator Implementation

#include <machine/machine.h>
#include <machine/timer.h>
#include <machine/keyboard.h>

__BEGIN_SYS

void Machine::reboot()
{
    for(int i = 0; (i < 300) && (i8042::status() & i8042::IN_BUF_FULL); i++)
        i8255::ms_delay(1);

    // Sending 0xfe to the keyboard controller port causes it to pulse
    // the reset line
    i8042::command(i8042::REBOOT);

    for(int i = 0; (i < 300) && (i8042::status() & i8042::IN_BUF_FULL); i++)
        i8255::ms_delay(1);
}

__END_SYS
