#include "su2.h"


#include <iostream>
#include <iomanip>
using std::cout;
using std::endl;


SU2::SU2()
{
//    mat = new complex[4];
    for (int i = 0; i < 4; i++)
    {
        mat[i].re = 0;
        mat[i].im = 0;
    }
}

SU2::~SU2()
{
//    delete [] mat;
}

void SU2::transpose()
{
    complex *temp = new complex[4];
    for (int i = 0; i < 4; i++) { temp[i] = mat[i]; }
    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < i; j++)
        {
            mat[i*2+j] = temp[j*2+i];
            mat[j*2+i] = temp[i*2+j];
        }
    }
    delete [] temp;
}

void SU2::conjugate()
{
    for (int i = 0; i < 4; i++)
    {
        mat[i].conjugate();
    }
}

void SU2::copy(SU2 B)
{
    for (int i = 0; i < 4; i++)
    {
        mat[i] = B.mat[i];
    }
}

SU2 &SU2::operator+=(SU2 B)
{
    for (int i = 0; i < 4; i++)
    {
        mat[i] += B.mat[i];
    }
    return *this;
}

SU2 &SU2::operator*=(SU2 B)
{
    /*
     * a*b = (a + bi)(c + id) = a*c + iad + ibc - bd;
     */
    complex *temp = new complex[4];
    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            for (int k = 0; k < 2; k++)
            {
                temp[2*i+j] += mat[(2*i+k)]*B[(2*k+j)];
            }
        }
    }
    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            mat[(i*2+j)] = temp[(i*2+j)];
        }
    }
    delete [] temp;
    return *this;
}

SU2 &SU2::operator*=(double b)
{
    for (int i = 0; i < 4; i++) {
        mat[i].re *= b;
        mat[i].im *= b;
    }
    return *this;
}

SU2 &SU2::operator-=(SU2 B)
{
    for (int i = 0; i < 4; i++)
    {
        mat[i] += B.mat[i];
    }
    return *this;
}

void SU2::zeros()
{
    for (int i = 0; i < 4; i++)
    {
        mat[i].re = 0;
        mat[i].im = 0;
    }
}

void SU2::print()
{
    /*
     * Temporary class for printing matrix. Might remove in the future to get better performance
     */
    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            cout << std::setw(15) << mat[2*i+j];
        }
        cout << endl;
    }
}