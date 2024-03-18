// EPOS ELF Utility Implementation

#include <architecture/cpu.h>
#include <utility/elf.h>
#include <utility/string.h>

__BEGIN_UTIL

long ELF::load_segment(unsigned int i, Elf_Addr addr)
{
    if((i > segments()) || (segment_type(i) != PT_LOAD))
        return 0;

    char * src = reinterpret_cast<char *>(CPU::Reg(this) + seg(i)->p_offset);
    char * dst = reinterpret_cast<char *>((addr) ? addr : segment_address(i));

    memcpy(dst, src, seg(i)->p_filesz);
    memset(dst + seg(i)->p_filesz, 0, seg(i)->p_memsz - seg(i)->p_filesz);

    return seg(i)->p_memsz;
}


void ELF::load(Loadable * obj)
{
    for(unsigned int i = 0; i < segments(); i++) {
        if((segment_size(i) == 0) || (segment_type(i) != PT_LOAD))
            continue;

        Elf_Addr addr = segment_address(i);
        if(((addr >= obj->code) && (addr <= (obj->code + obj->code_size))) || // CODE
           ((addr >= obj->data) && (addr <= (obj->data + obj->data_size))))   // DATA
            load_segment(i);
        else
            db<ELF>(WRN) << "Skipping unknown ELF segment " << i << " at " << hex << addr << "!"<< endl;
    }
}

__END_UTIL
