#include <iostream>
#include <ctime>
#include <mpi.h>
#include "system.h"
#include "actions/action.h"
#include "actions/wilsongaugeaction.h"
#include "correlators/plaquette.h"
#include "matrices/su3matrixgenerator.h"
#include "parallelization/indexorganiser.h"

#include "unittests.h"

using std::cout;
using std::endl;

/*
 * TODO:
 * [x] Add plaquette correlator
 * [x] Make actions more general!! Aka, create a Wilson action
 * [x] Change to updating random matrices by X=RST
 * [x] Change to such that time dimension is 2N
 * [x] Add determinant for SU3 matrices
 * [x] Create method for saving lattice configuration
 * [x] Create method for loading lattice configuration
 * [x] Fix bug in matrices
 * [x] Find bug so that N != N_T works.
 * [x] Add write each Plaquette/observable to file-function.
 * [x] Finish SU3 basic properties unit testing such that I dont have to compare by hand
 * [x] Rename metropolis.cpp --> system.cpp
 * [x] Add shifting parallelization
 * [x] Update functions for reading and writing sublattices
 * [ ] Enforce sub lattice cubes when possible(when allocating dimensions)
 * [ ] Switch to CORRECT method syntax, foo --> m_foo
 * [ ] Check that the lattice is gauge invariant: M^-1 * U * M, see Gattinger intro on how to make gauge fields gauge invariant!
 * [ ] Add better test suites, one that prints FAIL if test fails!!
 * [ ] Redo preGammaThermalization
 */

int main(int numberOfArguments, char* cmdLineArguments[])
{
    // Initializing parallelization
    int numprocs, processRank;
    MPI_Init (&numberOfArguments, &cmdLineArguments);
    MPI_Comm_size (MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank (MPI_COMM_WORLD, &processRank);

    // Constants by default initialization
    int N           = 8;            // Spatial lattice points.
    int N_T         = 8;            // Temporal lattice points.
    int NTherm      = 200;           // Thermalization.
    int NCor        = 1;            // Correlation updates.
    int NCf         = 20;           // Number of configurations to generate.
    int NUpdates    = 10;           // Number of link updates before moving on.
    double beta     = 6.0;          // Beta value(connected to the lattice spacing a).
    double SU3Eps   = 0.24;         // Epsilon spread for generating SU(3) matrices.
    double seed                         = std::time(nullptr) + double(processRank);                     // Random matrix seed. Defualt: current time.
    double metropolisSeed               = std::time(nullptr) + double(numprocs) + double(processRank);  // Metropolis seed. Defualt: current time.
    bool writeConfigsToFile             = true;
    bool storeThermalizationPlaquettes  = true;
    bool hotStart                       = false;

    if (numberOfArguments > 1) { // Points for each lattice dimension.
        N           = atoi(cmdLineArguments[2]);
    }
    if (numberOfArguments > 2) { // Time dimension.
        N_T         = atoi(cmdLineArguments[3]);
    }
    if (numberOfArguments > 3) { // Number of times we will thermalize. In production runs this should be around 2000,
        NTherm      = atoi(cmdLineArguments[4]);
    }
    if (numberOfArguments > 4) { // Only keeping every 20th path. In production runs this will be 200.
        NCor        = atoi(cmdLineArguments[5]);
    }
    if (numberOfArguments > 5) { // Number of field configurations that will be generated. In production runs, this should be around 1000 for the smallest, 500 for the others.
        NCf         = atoi(cmdLineArguments[6]);
    }
    if (numberOfArguments > 6) { // Number of times a link will be updated. Because of the 10% acceptance ratio, 10 times means about 1 update.
        NUpdates    = atoi(cmdLineArguments[7]);
    }
    if (numberOfArguments > 7) { // Beta value, phenomelogically connected to lattice spacing a.
        beta        = atof(cmdLineArguments[8]);
    }
    if (numberOfArguments > 8) { // Epsilon used for generating SU(3) matrices
        SU3Eps      = atof(cmdLineArguments[9]);
    }
    if (numberOfArguments > 9) {
        if (atoi(cmdLineArguments[10]) == 0) {
            writeConfigsToFile = false;
        }
    }
    if (numberOfArguments > 10) {
        if (atoi(cmdLineArguments[11]) == 0) {
            storeThermalizationPlaquettes = false;
        }
    }
    if (numberOfArguments > 11) {
        if (atoi(cmdLineArguments[12]) == 1) {
            hotStart = true;
        }
    }
    if (numberOfArguments > 12) { //
        seed = atof(cmdLineArguments[13]) + double(processRank);
    }
    if (numberOfArguments > 13) { // Metrolis seed.
        metropolisSeed = atof(cmdLineArguments[14]) + double(numprocs) + double(processRank);
    }

//    if (processRank == 0) {
//        testInverseMatrix(SU3Eps, seed, 1e3, false);
//        inversePerformanceTest(SU3Eps,seed,1e5);
//        runTestSuite();
//        MPI_Finalize();
//        exit(0);
//    }

    clock_t programStart, programEnd;
    programStart = clock();
    SU3MatrixGenerator SU3Gen(SU3Eps, seed);
    Plaquette G;
    WilsonGaugeAction S(beta);
    System pureGauge(N, N_T, NCf, NCor, NTherm, NUpdates, beta, metropolisSeed, &G, &S, numprocs, processRank);
    pureGauge.latticeSetup(&SU3Gen, hotStart);
    pureGauge.setConfigBatchName("configs_profiling_run");
    pureGauge.runMetropolis(storeThermalizationPlaquettes, writeConfigsToFile);
    pureGauge.runBasicStatistics();
    pureGauge.printAcceptanceRate();
//    pureGauge.writeDataToFile("../output/correlatorData.dat");

    programEnd = clock();
    if (processRank == 0) cout << "Program complete. Time used: " << ((programEnd - programStart)/((double)CLOCKS_PER_SEC)) << endl;
    MPI_Finalize();
    return 0;
}
