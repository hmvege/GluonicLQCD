#include "su3.h"
#include "complex.h"

#include <iostream>
#include <iomanip>
using std::cout;
using std::endl;

/*
 * For storing the SU3 matrix
 */

SU3::SU3()
{
    for (int i = 0; i < 9; i++)
    {
        mat[i].re = 0;
        mat[i].im = 0;
    }
}

SU3::~SU3()
{

}

void SU3::transpose()
{
    complex *temp = new complex[9];
    for (int i = 0; i < 9; i++) { temp[i] = mat[i]; }
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < i; j++)
        {
            mat[i*3+j] = temp[j*3+i];
            mat[j*3+i] = temp[i*3+j];
        }
    }
    delete []temp;
}

void SU3::conjugate()
{
    for (int i = 0; i < 9; i++)
    {
        mat[i].conjugate();
    }
}

void SU3::copy(SU3 B)
{
    for (int i = 0; i < 9; i++)
    {
        mat[i] = B[i];
    }
}


SU3 &SU3::operator+=(SU3 B)
{
    for (int i = 0; i < 9; i++)
    {
        mat[i] += B.mat[i];
    }
    return *this;
}

SU3 &SU3::operator*=(SU3 B)
{
    /*
     * a*b = (a + bi)(c + id) = a*c + iad + ibc - bd;
     */
    complex *temp = new complex[9];
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            for (int k = 0; k < 3; k++)
            {
                temp[3*i+j] += mat[(3*i+k)]*B[(3*k+j)];
            }
        }
    }
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            mat[(i*3+j)] = temp[(i*3+j)];
        }
    }
    delete [] temp;
    return *this;
}

SU3 &SU3::operator-=(SU3 B)
{
    for (int i = 0; i < 9; i++)
    {
        mat[i] += B.mat[i];
    }
    return *this;
}


void SU3::print()
{
    /*
     * Temporary class for printing matrix. Might remove in the future to get better performance
     */
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            cout << std::setw(15) << mat[3*i+j];
        }
        cout << endl;
    }
}
