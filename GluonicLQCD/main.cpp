#include <iostream>
#include <ctime>
#include <chrono>
#include <mpi.h>
#include "system.h"
#include "actions/action.h"
#include "actions/wilsongaugeaction.h"
#include "observables/plaquette.h"
#include "math/matrices/su3matrixgenerator.h"
#include "parallelization/index.h"
#include "config/parameters.h"
#include "config/configloader.h"

#include "tests/unittests.h"
#include "tests/testsuite.h"

using std::cout;
using std::endl;
using std::chrono::steady_clock;
using std::chrono::duration_cast;
using std::chrono::duration;

/*
 * SETUP INSTRUCTIONS: (UPDATE THIS!)
 * Set all parameters
 * Set correlators
 * Set and run system
 *
 * TODO:
 * [ ] Implement better map structure system. e.g. latmath.h ect
 * [ ] Enforce sub lattice cubes as standard
 * [x] Add batch name to print-out
 * [x] Add function for loading fields? Or make a seperate program? Should probably be done here.
 * [ ] (optional) Switch to CORRECT method syntax, foo --> m_foo
 * [ ] (optional) Check that the lattice is gauge invariant: M^-1 * U * M, see Gattinger intro on how to make gauge fields gauge invariant!
 */

void runUnitTests();

int main(int numberOfArguments, char* cmdLineArguments[])
{
    // Initializing parallelization, HIDE THIS?
    int numprocs, processRank;
    MPI_Init (&numberOfArguments, &cmdLineArguments);
    MPI_Comm_size (MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank (MPI_COMM_WORLD, &processRank);

    if (numberOfArguments != 2) {
        Parallel::Communicator::MPIExit("Error: please provide a json file to parse.");
    }
    Parallel::Communicator::init(numprocs,processRank);
    ConfigLoader::load(cmdLineArguments[1]);

    // Program timers
    steady_clock::time_point programStart;
    programStart = steady_clock::now();

    // Main program part
    if (Parameters::getUnitTesting() && processRank == 0) runUnitTests();
    System pureGauge;
    pureGauge.latticeSetup();
    SysPrint::printSystemInfo();
    pureGauge.runMetropolis();

    // Finalizing and printing time taken
    duration<double> programTime = duration_cast<duration<double>>(steady_clock::now() - programStart);
    if (processRank == 0) printf("\nProgram complete. Time used: %f hours (%f seconds)", double(programTime.count())/3600.0, programTime.count());

    MPI_Finalize();
    return 0;
}

void runUnitTests()
{
//    runBoolTest(1e9);
    TestSuite unitTester;
    unitTester.runFullTestSuite(Parameters::getUnitTestingVerbose());
//    SU3BaseTests();
//    runMatrixPerformanceTest(std::time(nullptr),1e7,true,false);
    Parallel::Communicator::MPIExit("");
}
