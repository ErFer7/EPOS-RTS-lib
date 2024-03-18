// EPOS OStream Interface

#include <system/config.h>

#ifndef __ostream_h
#define __ostream_h

extern "C" {
    void _print_preamble();
    void _print(const char * s);
    void _print_trailler(bool error);
}

__BEGIN_UTIL

class OStream
{
public:
    struct Begl {};
    struct Endl {};
    struct Hex {};
    struct Dec {};
    struct Oct {};
    struct Bin {};
    struct Err {};

public:
    OStream(): _base(10), _error(false) {}

    OStream & operator<<(const Begl & begl) {
        _error = false;
        _print_preamble();
        return *this;
    }

    OStream & operator<<(const Endl & endl) {
        _print_trailler(_error);
        print("\n");
        _base = 10;
        return *this;
    }

    OStream & operator<<(const Hex & hex) {
        _base = 16;
        return *this;
    }
    OStream & operator<<(const Dec & dec) {
        _base = 10;
        return *this;
    }
    OStream & operator<<(const Oct & oct) {
        _base = 8;
        return *this;
    }
    OStream & operator<<(const Bin & bin) {
        _base = 2;
        return *this;
    }

    OStream & operator<<(const Err & err)
    {
        _error = true;
        return *this;
    }

    OStream & operator<<(char c) {
        char buf[2];
        buf[0] = c;
        buf[1] = '\0';
        print(buf);
        return *this;
    }
    OStream & operator<<(unsigned char c) {
        return operator<<(static_cast<unsigned int>(c));
    }

    OStream & operator<<(int i) {
        char buf[64];
        buf[itoa(i, buf)] = '\0';
        print(buf);
        return *this;
    }
    OStream & operator<<(short s) {
        return operator<<(static_cast<int>(s));
    }
    OStream & operator<<(long l) {
        char buf[64];
        buf[ltoa(l, buf)] = '\0';
        print(buf);
        return *this;
    }

    OStream & operator<<(unsigned int ui) {
        char buf[64];
        buf[utoa(ui, buf)] = '\0';
        print(buf);
        return *this;
    }
    OStream & operator<<(unsigned short us) {
        return operator<<(static_cast<unsigned int>(us));
    }
    OStream & operator<<(unsigned long ul) {
        char buf[64];
        buf[ultoa(ul, buf)] = '\0';
        print(buf);
        return *this;
    }

    OStream & operator<<(long long ll) {
        char buf[64];
        buf[lltoa(ll, buf)] = '\0';
        print(buf);
        return *this;
    }

    OStream & operator<<(unsigned long long ull) {
        char buf[64];
        buf[ulltoa(ull, buf)] = '\0';
        print(buf);
        return *this;
    }

    OStream & operator<<(const void * p) {
        char buf[64];
        buf[ptoa(p, buf)] = '\0';
        print(buf);
        return *this;
    }

    OStream & operator<<(const char * s) {
        print(s);
        return *this;
    }

    OStream & operator<<(float f) {
        if(f < 0.0001f && f > -0.0001f){
            (*this) << "0.0000";
            return *this;
        }
        if (f < 0) {
            (*this) << "-";
            f *= -1;
        }

        long long b = (long long) f;
        (*this) << b << ".";
        long long m = (long long) ((f - b)  * 10000);
        (*this) << m;
        return *this;
    }

    OStream & operator<<(double d) {
        return operator<<(static_cast<float>(d));
    }

private:
    void print(const char * s) { _print(s); }

    int itoa(int v, char * s);
    int utoa(unsigned int v, char * s, unsigned int i = 0);
    int ltoa(long v, char * s);
    int ultoa(unsigned long v, char * s, unsigned int i = 0);
    int lltoa(long long v, char * s);
    int ulltoa(unsigned long long v, char * s, unsigned int i = 0);
    int ptoa(const void * p, char * s);

private:
    unsigned int _base;
    volatile bool _error;

    static const char _digits[];
};

constexpr OStream::Begl begl;
constexpr OStream::Endl endl;
constexpr OStream::Hex hex;
constexpr OStream::Dec dec;
constexpr OStream::Oct oct;
constexpr OStream::Bin bin;

__END_UTIL

#endif
