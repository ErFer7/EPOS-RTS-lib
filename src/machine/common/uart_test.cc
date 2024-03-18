// EPOS UART Test Program

#include <machine.h>

using namespace EPOS;

UART uart;

void print(const char * s) {
    for(unsigned int i = 0; s[i] != '\0'; ++i)
        uart.put(s[i]);
    uart.flush();
}

int main()
{
    print("UART test\n\n");
    print("This test requires human intervention!\n");
    print("Type 'Z' to finish.\n");
    print("You should be able to see what you're typing:\n");

    char c = 'a';

    while(c != 'Z') {
        c = uart.get();

        if(c == '\r')
            c = '\n';

        uart.put(c);
    }

    uart.flush();

    return 0;
}

