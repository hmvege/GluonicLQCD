#ifndef COMPLEX_H
#define COMPLEX_H

// TEMP FOR PRINTING; MUST REMOVE TO STRIP DOWN LATER
#include <string>
#include <iostream>

class complex
{
public:
    complex();
    ~complex();
    complex(double real, double imag);

//    double *reim;
    double im;
    double re;
//    double re() { return reim[0]; }
//    double im() { return reim[1]; }

    complex &operator+=(complex b);
    complex &operator-=(complex b);
    complex &operator*=(complex b);
    complex &operator/=(complex b);
    complex &operator/=(double b);

    double norm();
    complex conjugate();
    complex c();

    // TEMP FOR PRINTING; MUST REMOVE TO STRIP DOWN LATER
    friend std::ostream& operator<<(std::ostream& os, const complex& a); // Allows cout << myVector << endl;
};

inline complex operator+(complex a, complex b)
{
    a += b;
    return a;
}

inline complex operator-(complex a, complex b)
{
    a -= b;
    return a;
}

inline complex operator*(complex a, complex b)
{
    a *= b;
    return a;
}

inline complex operator/(complex a, complex b)
{
    a /= b;
    return a;
}

inline complex operator/(complex a, double b)
{
    a /= b;
    return a;
}


#endif // COMPLEX_H
