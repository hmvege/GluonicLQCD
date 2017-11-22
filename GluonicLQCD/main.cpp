#include <chrono>
#include <mpi.h>
#include "system.h"
#include "config/parameters.h"
#include "config/configloader.h"

#include "tests/unittests.h"
#include "tests/testsuite.h"

using std::chrono::steady_clock;
using std::chrono::duration_cast;
using std::chrono::duration;

/*
 * SETUP INSTRUCTIONS: (UPDATE THIS!)
 * Set all parameters
 * Set correlators
 * Set and run system
 */

void runUnitTests();

int main(int numberOfArguments, char* cmdLineArguments[])
{
    Parallel::Communicator::init(&numberOfArguments, &cmdLineArguments);
    ConfigLoader::load(cmdLineArguments[1]);

    // Unit tester
    if (Parameters::getUnitTesting() && Parallel::Communicator::getProcessRank() == 0) runUnitTests();

    // Program timers
    steady_clock::time_point programStart;
    programStart = steady_clock::now();

    // Main program part
    System pureGauge;
    pureGauge.latticeSetup();
    pureGauge.run();

    // Finalizing and printing time taken
    duration<double> programTime = duration_cast<duration<double>>(steady_clock::now() - programStart);
    if (Parallel::Communicator::getProcessRank() == 0) {
        printf("\nProgram complete. Time used: %f hours (%f seconds)", double(programTime.count())/3600.0, programTime.count());
    }

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
    Parallel::Communicator::MPIExit("Unit tests complete.");
}
