// EPOS Geometry Utility Declarations

#ifndef __geometry_h
#define __geometry_h

#include <utility/math.h>

__BEGIN_UTIL

template<typename T, unsigned int dimensions>
struct Point;

template<typename T>
struct Point<T, 2>
{
private:
    typedef typename IF<EQUAL<char, T>::Result, int, T>::Result Print_Type;
    typedef typename LARGER<T>::Result Larger_T;

public:
    using Number = T;
    using Distance = typename UNSIGNED<Larger_T>::Result;

    Point() {}
    Point(const T & xi, const T & yi): _x(xi), _y(yi) {}

    Number x() const { return _x; }
    Number y() const { return _y; }

    bool operator==(const Point & p) const { return (_x == p._x) && (_y == p._y); }
    bool operator!=(const Point & p) const { return !(*this == p); }

    // Euclidean distance
    template<typename P>
    Distance operator-(const P & p) const {
        // Care for unsigned T
        Larger_T xx = p._x > _x ? p._x - _x : _x - p._x;
        Larger_T yy = p._y > _y ? p._y - _y : _y - p._y;
        return Math::sqrt<Number>(xx * xx + yy * yy);
    }

    // Translation
    template<typename P>
    Point & operator-=(const P & p) {
        _x -= p._x;
        _y -= p._y;
        return *this;
    }
    template<typename P>
    Point & operator+=(const P & p) {
        _x += p._x;
        _y += p._y;
        return *this;
    }
    template<typename P>
    Point operator+(const P & p) const {
        Point ret(_x, _y);
        return ret += p;
    }

    static Point trilaterate(const Point & p1, const Distance & d1,  const Point & p2, const Distance & d2, const Point & p3, const Distance & d3) {
        Larger_T a[2][3];

        a[0][0] = 2 * (-p1._x + p3._x);
        a[0][1] = 2 * (-p1._y + p3._y);
        a[0][2] = 1 * ((d1 * d1 - (p1._x * p1._x + p1._y * p1._y)) - (d3 * d3 - (p3._x * p3._x + p3._y * p3._y)));

        a[1][0] = 2 * (-p2._x + p3._x);
        a[1][1] = 2 * (-p2._y + p3._y);
        a[1][2] = 1 * ((d2 * d2 - (p2._x * p2._x + p2._y * p2._y))-(d3 * d3 - (p3._x * p3._x + p3._y * p3._y)));

        if(a[0][0] != 0) {
            a[1][1] -= a[0][1] * a[1][0] / a[0][0];
            a[1][2] -= a[0][2] * a[1][0] / a[0][0];
            a[1][0] = 0;
        }

        T _x, _y;

        if(a[1][1] == 0)
            _y = 0;
        else
            _y = a[1][2] / a[1][1];

        if(a[0][0] == 0)
            _x = 0;
        else
            _x = (a[0][2] - (_y * a[0][1])) / a[0][0];

        return Point(_x, _y);
    }

    friend OStream & operator<<(OStream & os, const Point<T, 2> & c) {
        os << "{" << static_cast<Print_Type>(c._x) << "," << static_cast<Print_Type>(c._y) << "}";
        return os;
    }

private:
    T _x, _y;
}__attribute__((packed));

template<typename T>
struct Point<T, 3>
{
private:
    typedef typename IF<EQUAL<char, T>::Result, int, T>::Result Print_Type;
    typedef typename LARGER<T>::Result Larger_T;

public:
    using Number = T;
    using Distance = typename UNSIGNED<Larger_T>::Result;

    Point() {}
    Point(const T & xi, const T & yi, const T & zi): _x(xi), _y(yi), _z(zi) {}

    Number x() const { return _x; }
    Number y() const { return _y; }
    Number z() const { return _z; }

    bool operator==(const Point & p) const { return (_x == p._x) && (_y == p._y) && (_z == p._z); }
    bool operator!=(const Point & p) const { return !(*this == p); }

    // Euclidean distance
    template<typename P>
    Distance operator-(const P & p) const {
        // Care for unsigned T
        Distance xx = p._x > _x ? p._x - _x : _x - p._x;
        Distance yy = p._y > _y ? p._y - _y : _y - p._y;
        Distance zz = p._z > _z ? p._z - _z : _z - p._z;
        return Math::sqrt<Number>(xx * xx + yy * yy + zz * zz);
    }

    // Translation
    template<typename P>
    Point & operator-=(const P & p) {
        _x -= p._x;
        _y -= p._y;
        _z -= p._z;
        return *this;
    }
    template<typename P>
    Point & operator+=(const P & p) {
        _x += p._x;
        _y += p._y;
        _z += p._z;
        return *this;
    }
    template<typename P>
    Point operator+(const P & p) const {
        Point ret(_x, _y, _z);
        return ret += p;
    }

    // TODO: 3D trilateration
    static Point trilaterate(const Point & p1, const Distance & d1,  const Point & p2, const Distance & d2, const Point & p3, const Distance & d3) {
        Larger_T a[2][3];

        a[0][0] = 2 * (-p1._x + p3._x);
        a[0][1] = 2 * (-p1._y + p3._y);
        a[0][2] = 1 * ((d1 * d1 - (p1._x * p1._x + p1._y * p1._y)) - (d3 * d3 - (p3._x * p3._x + p3._y * p3._y)));

        a[1][0] = 2 * (-p2._x + p3._x);
        a[1][1] = 2 * (-p2._y + p3._y);
        a[1][2] = 1 * ((d2 * d2 - (p2._x * p2._x + p2._y * p2._y))-(d3 * d3 - (p3._x * p3._x + p3._y * p3._y)));

        if(a[0][0] != 0) {
            a[1][1] -= a[0][1] * a[1][0] / a[0][0];
            a[1][2] -= a[0][2] * a[1][0] / a[0][0];
            a[1][0] = 0;
        }

        T _x, _y;

        if(a[1][1] == 0)
            _y = 0;
        else
            _y = a[1][2] / a[1][1];

        if(a[0][0] == 0)
            _x = 0;
        else
            _x = (a[0][2] - (_y * a[0][1])) / a[0][0];

        return Point(_x, _y, 0);
    }

    friend OStream & operator<<(OStream & os, const Point & c) {
        os << "(" << static_cast<Print_Type>(c._x) << "," << static_cast<Print_Type>(c._y) << "," << static_cast<Print_Type>(c._z) << ")";
        return os;
    }

    T _x, _y, _z;
}__attribute__((packed));

template<typename T1, typename T2 = void>
struct Sphere
{
private:
    typedef typename IF<EQUAL<T1, char>::Result, int, T1>::Result Print_Type;

public:
    using Number = T1;
    using Center = Point<Number, 3>;
    using Radius = typename IF<EQUAL<T2, void>::Result, typename Center::Distance, T2>::Result;

    Sphere() {}
    Sphere(const Center & c, const Radius & r = 0): _c(c), _r(r) {}

    Center center() const { return _c; }
    Radius radius() const { return _r; }

    bool contains(const Center & c) const { return (_c - c) <= _r; }

    friend OStream & operator<<(OStream & os, const Sphere & s) {
        os << "{" << "c=" << s._c << ",r=" << static_cast<Print_Type>(s._r) << "}";
        return os;
    }

private:
    Center _c;
    Radius _r;
}__attribute__((packed));

__END_UTIL

#endif
