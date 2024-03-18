// EPOS ARM Cortex GPIO Mediator Declarations

#ifndef __cortex_gpio_h
#define __cortex_gpio_h

#include <architecture/cpu.h>
#include <machine/gpio.h>
#include <machine/ic.h>
#include __HEADER_MMOD(gpio)
#include <utility/observer.h>

__BEGIN_SYS

class GPIO: public GPIO_Engine, public Observed
{
    friend class Machine; // for init()
    friend class IC; // for eoi()

private:
    typedef GPIO_Engine Engine;

public:
    typedef _UTIL::Observed Observed;
    typedef _UTIL::Observer Observer;

public:
    GPIO(Port port, Pin pin, Direction dir, Pull pull = UP, Edge int_edge = NONE)
    : Engine(port, pin, dir, pull, int_edge), _port(port), _pin(pin) {
        if(int_edge != NONE) {
            _gpios[_port][_pin] = this;
            IC::Interrupt_Id i = IC::INT_GPIOA + _port;
            IC::disable(i);
            Engine::int_disable();
            IC::int_vector(i, int_handler, eoi);
            Engine::int_enable(int_edge);
            Engine::clear_interrupts();
            Engine::int_enable();
            IC::enable(i);

        }
    }

    virtual ~GPIO() {
        int_disable();
        _gpios[_port][_pin] = 0;
    }

    using Engine::get;
    using Engine::set;
    using Engine::clear;
    using Engine::direction;
    using Engine::pull;
    using Engine::int_enable;
    using Engine::int_disable;

private:
    using Engine::init;

    static void int_handler(IC::Interrupt_Id i);
    static void eoi(IC::Interrupt_Id i);

private:
    Port _port;
    Pin _pin;

    static GPIO * _gpios[PORTS][8];
};

__END_SYS

#endif
