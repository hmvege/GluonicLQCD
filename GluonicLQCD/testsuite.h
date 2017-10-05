#ifndef TESTSUITE_H
#define TESTSUITE_H

#include <iostream>
#include <iomanip>
#include <cmath>
#include <ctime>
#include "matrices/su2.h"
#include "matrices/su3.h"
#include "matrices/su3matrixgenerator.h"
#include "correlators/plaquette.h"
#include "links.h"
#include "complex.h"
#include "functions.h"

class TestSuite
{
private:
    SU3 U1, U2, U3, UAdd, USub, UMul, UC, UT, UCT;
    SU2 s1, s2, s3, sAdd, sSub, sMul, sC, sT, sCT;

    // SU3 generator
    SU3MatrixGenerator *m_SU3Generator = nullptr;

    // RNGs
    std::mt19937_64 m_generator;
    std::uniform_real_distribution<double> m_uniform_distribution;

    // Basic matrix property testers
    bool operationSU2Test(bool verbose, SU2 results, SU2 solution, std::string operation);
    bool operationSU3Test(bool verbose, SU3 results, SU3 solution, std::string operation);

    // Inline matrix comparing functions
    inline bool compareSU3(SU3 A, SU3 B);
    inline bool compareSU2(SU2 A, SU2 B);

    // Basic SU3 matrix operation tests
    bool testSU3Addition(bool verbose);
    bool testSU3Subtraction(bool verbose);
    bool testSU3Multiplication(bool verbose);
    bool testSU3Conjugation(bool verbose);
    bool testSU3Transpose(bool verbose);
    bool testSU3ComplexConjugation(bool verbose);

    // Basic SU2 matrix operation tests
    bool testSU2Addition(bool verbose);
    bool testSU2Subtraction(bool verbose);
    bool testSU2Multiplication(bool verbose);
    // Following not really needed.
//    bool testSU2Conjugation(bool verbose);
//    bool testSU2Transpose(bool verbose);
//    bool testSU2ComplexConjugation(bool verbose);

    // SU2 matrix tests
    bool testSU2Hermicity(bool verbose);
    bool testSU2Orthogonality(bool verbose);
    bool testSU2Norm(bool verbose);
    bool testSU2Inverse(bool verbose);
    bool testSU2Determinant(bool verbose);
    bool testSU2MatrixInverse(bool verbose);

    // SU3 matrix tests
    bool testSU3Hermicity(bool verbose);
    bool testSU3Orthogonality(bool verbose);
    bool testSU3Norm(bool verbose);
    bool testSU3Inverse(bool verbose);
    bool testSU3Determinant(bool verbose);
    bool testSU3MatrixInverse(bool verbose);

    bool testGaugeInvariance(bool verbose);
public:
    TestSuite();

    void runFullTestSuite(bool verbose);
    bool runSU2Tests(bool verbose);
    bool runSU3Tests(bool verbose);
    bool run2x2MatrixTests(bool verbose);
    bool run3x3MatrixTests(bool verbose);
};

#endif // TESTSUITE_H
