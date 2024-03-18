// EPOS RISC V Initialization

#include <machine.h>

__BEGIN_SYS

void Machine::pre_init(System_Info *si) {
  CLINT::mtvec(CLINT::DIRECT, &IC::entry);

  Display::init();

  db<Init, Machine>(TRC) << "Machine::pre_init()" << endl;
}

void Machine::init() {
  db<Init, Machine>(TRC) << "Machine::init()" << endl;

  if (Traits<IC>::enabled)
    IC::init();

  if (Traits<Timer>::enabled)
    Timer::init();

#ifdef __NIC_H
#ifdef __ethernet__
  if (Traits<Ethernet>::enabled)
    Initializer<Ethernet>::init();
#endif
#endif
}

__END_SYS
