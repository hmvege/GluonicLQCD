#ifndef UNITTESTS_H
#define UNITTESTS_H

#include "matrices/su3.h"

void SU3BaseTests();
bool testOrthogonality(SU3 H, bool verbose);
bool testHermicity(SU3 H, bool verbose);
bool testNorm(int col, SU3 H);
bool SU2UnitTest(complex * r, complex * s, complex * t);
void testDeterminant(SU3 U);
void checkDim(int N, int N_T);
// For testing different basic operations
void testMatrixSU3Properties();
void runMatrixPerformanceTest(double eps, double seed, int NTests, bool testMatrix, bool testComplex);

void testMatrixMultiplication();
void testMatrixAddition();
void testMatrixSubtraction();
void testMatrixConjugation();
void testMatrixTranspose();
void testMatrixDagger();

void runTestSuite();

#endif // UNITTESTS_H
