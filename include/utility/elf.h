// EPOS ELF Utility Declarations

#ifndef __elf_h
#define	__elf_h

#include <system/config.h>
#include "elf-linux.h"

__BEGIN_UTIL

typedef IF<Traits<CPU>::WORD_SIZE == 32, Elf32_Ehdr, Elf64_Ehdr>::Result Elf_Ehdr;
typedef IF<Traits<CPU>::WORD_SIZE == 32, Elf32_Addr, Elf64_Addr>::Result Elf_Addr;
typedef IF<Traits<CPU>::WORD_SIZE == 32, Elf32_Word, Elf64_Word>::Result Elf_Word;
typedef IF<Traits<CPU>::WORD_SIZE == 32, Elf32_Phdr, Elf64_Phdr>::Result Elf_Phdr;

class ELF: private Elf_Ehdr
{
public:
    struct Loadable {
        Elf_Addr entry;
        unsigned long segments;
        Elf_Addr code;
        unsigned long code_size;
        Elf_Addr data;
        unsigned long data_size;
    };

public:
    ELF() {}

    // check for ELF magic number
    bool valid() { 
        return (e_ident[EI_MAG0] == ELFMAG0) && (e_ident[EI_MAG1] == ELFMAG1)
            && (e_ident[EI_MAG2] == ELFMAG2) && (e_ident[EI_MAG3] == ELFMAG3);
    }

    Elf_Addr entry() { return e_entry; }

    unsigned int segments() { return e_phnum; }

    Elf_Word segment_type(unsigned int i) { return (i > segments()) ? PT_NULL : seg(i)->p_type; }

    Elf_Addr segment_address(unsigned int i) { return (i > segments()) ? 0 : seg(i)->p_align ? seg(i)->p_vaddr: (seg(i)->p_vaddr & ~(seg(i)->p_align - 1)); }

    long segment_size(unsigned int i) { return (i > segments()) ? -1 : (long)(((seg(i)->p_offset % seg(i)->p_align) + seg(i)->p_memsz + seg(i)->p_align - 1) & ~(seg(i)->p_align - 1)); }

    void scan(Loadable * obj, Elf_Addr code_low, Elf_Addr code_high, Elf_Addr data_low, Elf_Addr data_high) {
        obj->entry = entry();
        obj->segments = 0; // 1 (code only) or 2 (code and data)
        obj->code_size = 0;
        obj->data_size = 0;

        for(unsigned int i = 0; i < segments(); i++) {
            if((segment_size(i) == 0) || (segment_type(i) != PT_LOAD))
                continue;

            if((segment_address(i) >= code_low) && (segment_address(i) <= code_high)) { // CODE
                if(obj->code_size == 0) {
                    obj->code_size = segment_size(i);
                    obj->code = segment_address(i);
                } else if(segment_address(i) < obj->code) {
                    obj->code_size = obj->code - segment_address(i) + segment_size(i);
                    obj->code = segment_address(i);
                } else if(segment_address(i) > (obj->code + obj->code_size)) {
                    obj->code_size = segment_address(i) - obj->code;
                } else
                    obj->code_size += segment_size(i);
            } else if((segment_address(i) >= data_low) && (segment_address(i) <= data_high)) { // DATA
                if(obj->data_size == 0) {
                    obj->data_size = segment_size(i);
                    obj->data = segment_address(i);
                } else if(segment_address(i) < obj->data) {
                    obj->data_size = obj->data - segment_address(i) + segment_size(i);
                    obj->data = segment_address(i);
                } else if(segment_address(i) > (obj->data + obj->data_size)) {
                    obj->data_size = segment_address(i) - obj->data;
                } else
                    obj->data_size += segment_size(i);
            } else {
                db<ELF>(WRN) << "Ignoring ELF segment " << i << " at " << hex << segment_address(i) << "!"<< endl;
                continue;
            }

            obj->segments++;
        }
    }

    long load_segment(unsigned int i, Elf_Addr addr = 0);

    void load(Loadable * obj);

private:
    Elf_Phdr * pht() { return (Elf_Phdr *)(((char *) this) + e_phoff); }
    Elf_Phdr * seg(int i) { return &pht()[i];  }
};

__END_UTIL

#endif
