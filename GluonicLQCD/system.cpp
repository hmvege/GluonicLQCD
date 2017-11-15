#include <random>   // For Mersenne-Twister19937
#include <chrono>
//#include <ctime>
#include <cmath>    // For exp()
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cstdio>   // For io C-style handling.
#include <cstdlib>
#include <mpi.h>
#include "system.h"

using std::cout;
using std::endl;
using std::chrono::steady_clock;
using std::chrono::duration_cast;
using std::chrono::duration;

System::System(double seed, Correlator *correlator, Action *S, Flow *F, Correlator *flowCorrelator)
{
    /*
     * Class for calculating correlators using the System algorithm.
     * Takes an action object as well as a Gamma functional to be used in the action.
     */
    m_NSpatial = Parameters::getNSpatial();
    m_NTemporal = Parameters::getNTemporal();
    m_latticeSize = Parameters::getLatticeSize();
    m_NCf = Parameters::getNCf(); // Number of configurations to run for
    m_NCor = Parameters::getNCor();
    m_NTherm = Parameters::getNTherm();
    m_NUpdates = Parameters::getNUpdates();
    m_NFlows = Parameters::getNFlows();
    m_beta = Parameters::getBeta();
    m_processRank = Parallel::Communicator::getProcessRank();
    m_numprocs = Parallel::Communicator::getNumProc();
    m_batchName = Parameters::getBatchName();
    m_pwd = Parameters::getFilePath();
    // Sets pointers to use
    setAction(S);
    setCorrelator(correlator);
    setFlowCorrelator(flowCorrelator);
    setFlow(F); // Setting it outside, in case we want to specify the exponentiation function.

    // For only one observable
    m_observablePreThermalization = new double[m_NTherm+1];
    m_observable = new double[m_NCf]; // Correlator values
    m_observableSquared = new double[m_NCf];

    std::mt19937_64 gen(seed); // Starting up the Mersenne-Twister19937 function
    std::uniform_real_distribution<double> uni_dist(0,1);
    m_generator = gen;
    m_uniform_distribution = uni_dist;
    for (int alpha = 0; alpha < m_NCf; alpha++)
    {
        m_observable[alpha] = 0;
        m_observableSquared[alpha] = 0;
    }
}

System::~System()
{
    /*
     * Class destructor
     */
    delete [] m_lattice;
}

void System::subLatticeSetup()
{
    /*
     * Sets up the sub-lattices. Adds +2 in every direction to account for sharing of s.
     */
    // Checks initial processor validity
    Parallel::Communicator::checkProcessorValidity();
    int restProc = m_numprocs;
    // Only finds the sub lattice size iteratively if no preset value has been defined.
    if (!m_subLatticeSizePreset) {
        // Sets up sub lattice dimensionality without any splitting
        for (int i = 0; i < 3; i++) {
            m_N[i] = m_NSpatial;
        }
        m_N[3] = m_NTemporal;
        // Iteratively finds and sets the sub-lattice dimensions
        while (restProc >= 2) {
            for (int i = 0; i < 4; i++) { // Counts from x to t
                m_N[i] /= 2;
                restProc /= 2;
                if (restProc < 2) break;
            }
        }
    }
    // Sets the sub lattice dimensions
    Parallel::Index::setN(m_N);
    Parallel::Communicator::setN(m_N);
    // Gets the total size of the sub-lattice(without faces)
    m_subLatticeSize = 1;
    for (int i = 0; i < 4; i++) {
        m_subLatticeSize *= m_N[i];
    }
    Parameters::setSubLatticeSize(m_subLatticeSize);
    // Ensures correct sub lattice dimensions
    Parallel::Communicator::checkSubLatticeValidity();
    // Creates (sub) lattice
    m_lattice = new Links[m_subLatticeSize];
    // If has a size of 2, we exit as that may produce poor results.
    Parallel::Communicator::checkSubLatticeDimensionsValidity();
    // Sets up number of processors per dimension
    for (int i = 0; i < 3; i++) {
        m_processorsPerDimension[i] = m_NSpatial / m_N[i];
    }
    m_processorsPerDimension[3] = m_NTemporal / m_N[3];
    // Initializes the neighbour lists
    m_neighbourLists = new Neighbours;
    m_neighbourLists->initialize(m_processRank, m_numprocs, m_processorsPerDimension);
    // Passes relevant information to the index handler(for the shifts).
    Parallel::Communicator::setNeighbourList(m_neighbourLists);
    // Passes the index handler and dimensionality to the action and correlator classes.
    m_S->setN(m_N);
    m_correlator->setN(m_N);
    m_correlator->setLatticeSize(m_subLatticeSize);
}

void System::setSubLatticeDimensions(int *NSub)
{
    /*
     * Function for specifying sub-lattice dimensions.
     * Arguments:
     *  (int*) NSub     : takes 4 integers, one integer for each sub-lattice dimension.
     */
    for (int i = 0; i < 4; i++) {
        m_N[i] = NSub[i];
    }
    m_subLatticeSizePreset = true;
}

void System::latticeSetup(SU3MatrixGenerator *SU3Generator, bool hotStart)
{
    /*
     * Sets up the lattice and its matrices.
     */
    subLatticeSetup();
    m_SU3Generator = SU3Generator;
    if (hotStart) {
        // All starts with a completely random matrix.
        for (int i = 0; i < m_subLatticeSize; i++)
        {
            for (int mu = 0; mu < 4; mu++)
            {
                if (!m_RSTInit)
                {
                    cout << "FULLY RANDOM BY DEFAULT! exits in latticeSetup, system.cpp line 225" << endl;
                    exit(1);
                    m_lattice[i].U[mu] = m_SU3Generator->generateRandom(); // Fully random
                } else {
                    m_lattice[i].U[mu] = m_SU3Generator->generateRST(); // Random close to unity
                }
            }
        }
    } else {
        // Cold start: everything starts out at unity.
        for (int i = 0; i < m_subLatticeSize; i++)
        {
            for (int mu = 0; mu < 4; mu++)
            {
                m_lattice[i].U[mu].identity();
            }
        }
    }
    if (m_processRank == 0) {
        printf("\nLattice setup complete");
    }
}

void System::printRunInfo(bool verbose) {
    /*
     * Function for printing system information in the beginning.
     * Arguments:
     *  verbose     : for printing more detailed information
     */
    if (m_processRank == 0) {
        cout << endl;
        printLine();
        cout << "Batch name:                            " << m_batchName << endl;
        cout << "Threads:                               " << m_numprocs << endl;
        if (verbose) cout << "Lattice size:                          " << m_latticeSize << endl;
        cout << "Lattice dimensions(spatial, temporal): " << m_NSpatial << " " << m_NTemporal << endl;
        cout << "N configurations:                      " << m_NCf << endl;
        cout << "N flow updates per configuration:      " << m_NFlows << endl;
        cout << "N correlation updates:                 " << m_NCor << endl;
        cout << "N thermalization updates:              " << m_NTherm << endl;
        cout << "N link updates:                        " << m_NUpdates << endl;
        cout << "Beta:                                  " << m_beta << endl;
        if (verbose) {
            cout << "SU3Eps:                                " << m_SU3Generator->getEpsilon() << endl;
            cout << "Sub lattice Size:                      " << m_subLatticeSize << endl;
            cout << "Sub latticedimensions:                 ";
            for (int i = 0; i < 4; i++) {
                cout << m_N[i] << " ";
            }
            cout << endl;
            cout << "Processsors per dimension:             ";
            for (int i = 0; i < 4; i++) {
                cout << m_processorsPerDimension[i] << " ";
            }
            cout << endl;
        }
        printLine();
    }
}

void System::thermalize()
{
    /*
     * Function for thermalizing the system.
     */
    if (m_processRank == 0) printf("\nInitiating thermalization.");
    if (m_storeThermalizationObservables) {
        // Storing the number of shifts that are needed in the observable storage container.
        m_NThermSteps = 1 + m_NTherm;
        // Calculating correlator before any updates have began.
        m_correlator->calculate(m_lattice,0);
//        Parallel::Communicator::setBarrier();
//        printf("\nGot it!");
//        Parallel::Communicator::setBarrier();
//        // Summing and sharing correlator to all processors before any updates has begun
//        MPI_Allreduce(&m_observablePreThermalization[0], &m_observablePreThermalization[0], 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

//        // Dividing by the number of processors in order to get the correlator.
//        m_observablePreThermalization[0] /= double(m_numprocs);
        if (m_processRank == 0) {
            printf("\ni    Observable   ");
//            printf("\n%-4d %-12.8f",0,m_observablePreThermalization[0]);
            printf("\n%-4d %-12.8f",0,m_correlator->getObservable(0));
        }
    }

//    Parallel::Communicator::setBarrier();
//    printf("\nOk in thermilization for processor %d",m_processRank);
//    Parallel::Communicator::setBarrier();
//    m_correlator->calculate(m_lattice,0);
//    Parallel::Communicator::setBarrier();
//    exit(1);
    // Running thermalization
    for (int i = 1; i < m_NTherm+1; i++)
    {
        // Pre update time
        m_preUpdate = steady_clock::now();

        // Pre timer
        update();

        // Post timer
        m_postUpdate = steady_clock::now();
        m_updateTime = duration_cast<duration<double>>(m_postUpdate - m_preUpdate);
        m_updateStorerTherm += m_updateTime.count();
        if (i % 20 == 0) { // Avg. time per update every 10th update
            if (m_processRank == 0) {
                printf("\nAvgerage update time(every 10th): %f sec.", m_updateStorerTherm/double(i));
            }
        }
//        if (m_processRank == 0) {
//            printf("\r%6.2f %% done", i/double(m_NTherm));
//        }

        // Print correlator every somehting or store them all(useful when doing the thermalization).
        if (m_storeThermalizationObservables) {
            // Calculating the correlator
//            m_observablePreThermalization[i] = m_correlator->calculate(m_lattice);
            m_correlator->calculate(m_lattice,i);

            // Summing and sharing results across the processors
//            MPI_Allreduce(&m_observablePreThermalization[i], &m_observablePreThermalization[i], 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD); // Turn off!
            // Averaging the results
//            m_observablePreThermalization[i] /= double(m_numprocs);

//            if (m_processRank == 0) {
//                printf("\n%-4d %-12.8f",i,m_observablePreThermalization[i]);
//            }
            if (m_processRank == 0) {
                printf("\n%-4d %-12.8f",i,m_correlator->getObservable(i)); // returns the observable at i(not averaged between by processors?)
            }
        }
    }

    // Taking the average of the acceptance rate across the processors.
    if (m_NTherm != 0) MPI_Allreduce(&m_acceptanceCounter,&m_acceptanceCounter,1,MPI_UNSIGNED_LONG,MPI_SUM,MPI_COMM_WORLD);

    // Printing post-thermalization correlator and acceptance rate
    if (m_processRank == 0 && m_NTherm != 0) printf("\nTermalization complete. Acceptance rate: %f",double(m_acceptanceCounter)/double(4*m_latticeSize*m_NUpdates*m_NTherm));
}

void System::updateLink(int latticeIndex, int mu)
{
    /*
     * Private function used for updating our system. Updates a single gauge link.
     * Arguments:
     *  i   : spacetime getIndex
     *  mu  : Lorentz getIndex
     */
//    m_updatedMatrix = m_SU3Generator->generateRandom()*m_lattice[latticeIndex].U[mu]; // Shorter method of updating matrix
    m_updatedMatrix = m_SU3Generator->generateRST()*m_lattice[latticeIndex].U[mu]; // Shorter method of updating matrix
}

void System::update()
{
    /*
     * Sweeps the entire Lattice, and gives every matrix a chance to update.
     */
    for (unsigned int x = 0; x < m_N[0]; x++) {
        for (unsigned int y = 0; y < m_N[1]; y++) {
            for (unsigned int z = 0; z < m_N[2]; z++) {
                for (unsigned int t = 0; t < m_N[3]; t++) {
                    for (unsigned int mu = 0; mu < 4; mu++) {
                        m_S->computeStaple(m_lattice, x, y, z, t, mu);
                        for (int n = 0; n < m_NUpdates; n++) // Runs avg 10 updates on link, as that is less costly than other parts
                        {
                            updateLink(Parallel::Index::getIndex(x,y,z,t), mu);
//                            m_deltaS = m_S->getDeltaAction(m_lattice, m_updatedMatrix, x, y, z, t, mu);
//                            if (exp(-m_deltaS) > m_uniform_distribution(m_generator))
                            if (exp(-m_S->getDeltaAction(m_lattice, m_updatedMatrix, x, y, z, t, mu)) > m_uniform_distribution(m_generator))
                            {
                                m_lattice[Parallel::Index::getIndex(x,y,z,t)].U[mu] = m_updatedMatrix;
                                m_acceptanceCounter++;
                            }
                        }
                    }
                }
            }
        }
    }
}


void System::runMetropolis(bool storeThermalizationObservables, bool writeConfigsToFile)
{
    /*
     * Runs the generation of gauge field configurations through the Metropolis algorithm.
     */
    m_storeThermalizationObservables = storeThermalizationObservables;
    //// TESTS ==============================================================================
//    // Common files
//    loadFieldConfiguration("FlowTestRun_beta6.000000_spatial16_temporal16_threads8_config0.bin"); // 0.59486412, MAC
////    loadFieldConfiguration("msg01.rec02.ildg-binary-data"); // jack
//    double corr = m_correlator->calculate(m_lattice);
//    MPI_Allreduce(&corr, &corr, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
//    corr /= double(m_numprocs);
//    if (m_processRank == 0) cout << "Plaquette value: " << corr << endl;
//    Flow WFlow(m_N, m_beta, m_numprocs, m_processRank);
//    WFlow.setAction(m_S);
//    /// OLD, run tests and compare times?
////    Clover Clov;
////    Clov.initializeIndexHandler(m_indexHandler);
////    Clov.setN(m_N);
////    Clov.setLatticeSize(m_latticeSize);
////    TopologicalCharge TopCharge;
////    TopCharge.initializeIndexHandler(m_indexHandler);
////    TopCharge.setLatticeSize(m_latticeSize);
////    TopCharge.setN(m_N);
////    EnergyDensity Energy(0.0931, m_latticeSize);
////    Energy.initializeIndexHandler(m_indexHandler);

//    ObservableSampler OSampler(m_N,m_subLatticeSize,0.0931);

//    int NFlows = 20;
//    double * m_observableFlow = new double[NFlows];
//    double * m_topologicalCharge = new double[NFlows];
//    double * m_topologicalSusceptibility = new double[NFlows];
//    double * m_actionDensity = new double[NFlows];
//    for (int tau = 0; tau < NFlows; tau++) {
//        m_topologicalCharge[tau] = 0;
//        m_topologicalSusceptibility[tau] = 0;
//        m_observableFlow[tau] = 0;
//        m_actionDensity[tau] = 0;
//    }
//    double updateTime = 0;
//    for (int tau = 0; tau < NFlows; tau++) {
//        m_preUpdate = steady_clock::now();
//        WFlow.flowField(m_lattice);

//        OSampler.calculate(m_lattice);
//        m_observableFlow[tau] = OSampler.getPlaquette();
//        m_topologicalCharge[tau] = OSampler.getTopologicalCharge();
//        m_actionDensity[tau] = OSampler.getEnergyDensity();

//       /// OLD
////        for (unsigned int x = 0; x < m_N[0]; x++) { // CLEAN UP AND MOVE THIS PART INTO ITS OWN CLASS FOR CALCULATING TOP CHARGE AND ENERGY?!
////            for (unsigned int y = 0; y < m_N[1]; y++) { // HIDE IT, AS IT IS BIG AND UGLY!
////                for (unsigned int z = 0; z < m_N[2]; z++) {
////                    for (unsigned int t = 0; t < m_N[3]; t++) {
////                        Clov.calculateClover(m_lattice,x,y,z,t);
////                        m_topologicalCharge[tau] += TopCharge.calculate(Clov.m_clovers);
////                        m_actionDensity[tau] += Energy.calculate(Clov.m_clovers);
////                        m_observableFlow[tau] += m_correlator->calculate(Clov.m_plaquettes);
////                    }
////                }
////            }
////        }

//        MPI_Allreduce(&m_topologicalCharge[tau], &m_topologicalCharge[tau], 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
//        MPI_Allreduce(&m_actionDensity[tau], &m_actionDensity[tau], 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
//        MPI_Allreduce(&m_observableFlow[tau], &m_observableFlow[tau], 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
//        m_topologicalSusceptibility[tau] = pow(m_topologicalCharge[tau]*m_topologicalCharge[tau],0.25) * 0.1973/(0.0931*16);
//        m_observableFlow[tau] /= double(m_numprocs);
//        if (m_processRank == 0) printf("\n%5d %-5.4f %-18.16f %-18.16f %-18.16f %-18.16f", tau, 0.0931*sqrt(8*double(0.01*tau)), m_observableFlow[tau], m_topologicalCharge[tau], m_topologicalSusceptibility[tau], m_actionDensity[tau]);

//        updateTime += (duration_cast<duration<double>>(steady_clock::now() - m_preUpdate)).count();

//        if (m_processRank == 0) printf("  Update time: : %-.4f",updateTime / (tau+1));
//    }
//    MPI_Barrier(MPI_COMM_WORLD);
//    if (m_processRank == 0) printf("\nTime used to flow: %-.4f",updateTime);

//    delete [] m_observableFlow;
//    delete [] m_topologicalCharge;
//    delete [] m_actionDensity;
//    MPI_Finalize(); exit(1);
    //// ===================================================================================
    if (m_processRank == 0) {
        cout << "Store thermalization observables:      ";
        if (m_storeThermalizationObservables) {
            cout << "TRUE" << endl;
        } else {
            cout << "FALSE" << endl;
        }
        cout << "Store configurations:                  ";
        if (writeConfigsToFile) {
            cout << "TRUE" << endl;
        } else {
            cout << "FALSE" << endl;
        }
        printLine();
    }

    // Variables for checking performance of the thermalization update.
    m_updateStorerTherm = 0;

    // System thermalization
    thermalize();

    // Printing header for main run
    if (m_processRank == 0) {
        printf("\ni     %-20s  Avg.Update-time   Accept/reject", m_correlator->getObservableName().c_str());
    }

    // Setting the System acceptance counter to 0 in order not to count the thermalization
    m_acceptanceCounter = 0;

    // Variables for checking performance of the update.
    m_updateStorer = 0;

    // Main part of algorithm
    for (int iConfig = 0; iConfig < m_NCf; iConfig++)
    {
        for (int i = 0; i < m_NCor; i++) // Updating NCor times before updating the Gamma function
        {
            // Pre timer
            m_preUpdate = steady_clock::now();

            update();

            // Post timer
            m_postUpdate = steady_clock::now();
            m_updateTime = duration_cast<duration<double>>(m_postUpdate - m_preUpdate);
            m_updateStorer += m_updateTime.count();
        }
        // Flow
        for (int iFlow = 0; iFlow < m_NFlows; iFlow++)
        {
            m_Flow->flowField(m_lattice);
            m_flowCorrelator->calculate(m_lattice,iFlow + m_NThermSteps);
//            OSampler.calculate(m_lattice);
//            m_observableFlow[tau] = OSampler.getPlaquette();
//            m_topologicalCharge[tau] = OSampler.getTopologicalCharge();
//            m_actionDensity[tau] = OSampler.getEnergyDensity();
//            MPI_Allreduce(&m_topologicalCharge[tau], &m_topologicalCharge[tau], 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
//            MPI_Allreduce(&m_actionDensity[tau], &m_actionDensity[tau], 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
//            MPI_Allreduce(&m_observableFlow[tau], &m_observableFlow[tau], 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
//            m_topologicalSusceptibility[tau] = pow(m_topologicalCharge[tau]*m_topologicalCharge[tau],0.25) * 0.1973/(0.0931*16);
//            m_observableFlow[tau] /= double(m_numprocs);
        }
        if (m_NFlows != 0) {
            /* Make flow statistics, that is, the only stats that is needed is the sum of all configurations, which cant be reached untill end.
             * So, either print flow during the run, or nothing.
             * Then, write the flow values to file. */
            m_flowCorrelator->runStatistics();
            // Write flow data to file
            m_flowCorrelator->writeStatisticsToFile(iConfig);
        }

        // Averaging the gamma values
        m_correlator->calculate(m_lattice,iConfig);
//        m_observable[iConfig] = m_correlator->calculate(m_lattice);
//        MPI_Allreduce(&m_observable[iConfig], &m_observable[iConfig], 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
//        m_observable[iConfig] /= double(m_numprocs);

        if (m_processRank == 0) {
            // Printing plaquette value
            printf("\n%-4d  %-20.8f  %-12.8f",iConfig,m_correlator->getObservable(iConfig),m_updateStorer/double((iConfig+1)*m_NCor));
            // Adding the acceptance ratio
            if (iConfig % 10 == 0) {
                printf(" %-13.8f", double(m_acceptanceCounter)/double(4*m_subLatticeSize*(iConfig+1)*m_NUpdates*m_NCor));
            }
        }

        // Writing field config to file
        if (writeConfigsToFile) writeConfigurationToFile(iConfig);
    }
    // Taking the average of the acceptance rate across the processors.
    MPI_Allreduce(&m_acceptanceCounter,&m_acceptanceCounter,1,MPI_UNSIGNED_LONG,MPI_SUM,MPI_COMM_WORLD);
    if (m_processRank == 0) {
        printf("\n");
        printLine();
        printf("System completed.");
        printf("\nAverage update time: %.6f sec.", m_updateStorer/double(m_NCf*m_NCor));
        printf("\nTotal update time for %d updates: %.6f sec.\n", m_NCf*m_NCor, m_updateStorer + m_updateStorerTherm);
    }
    m_correlator->runStatistics();
    m_correlator->writeStatisticsToFile(); // Runs statistics, writes to file, and prints results (if verbose is on)

}

//void System::flowConfiguration(std::vector<std::string> configurationName)
//{
//    for (unsigned int i = 0; i < configurationName.size(); i++) {
//        // Flow configurationName[i]
//    }
//}

void System::runBasicStatistics()
{
    /*
     * Class instance for sampling statistics from our system.
     */
    double averagedObservableSquared = 0;
    // Performing an average over the Monte Carlo obtained values
    for (int alpha = 0; alpha < m_NCf; alpha++)
    {
        m_averagedObservable += m_observable[alpha];
        averagedObservableSquared += m_observable[alpha]*m_observable[alpha];
    }
    m_averagedObservable /= double(m_NCf);
    averagedObservableSquared /= double(m_NCf);
    m_varianceObservable = (averagedObservableSquared - m_averagedObservable*m_averagedObservable)/double(m_NCf);
    m_stdObservable = sqrt(m_varianceObservable);
    if (m_processRank == 0) {
        printLine();
        cout << "Average plaqutte:      " << m_averagedObservable << endl;
        cout << "Standard deviation:    " << m_stdObservable << endl;
        cout << "Variance:              " << m_varianceObservable << endl;
        printLine();
    }
}

void System::writeDataToFile()
{
    /*
     * For writing the observables to file.
     */
    if (m_processRank == 0) {
        std::ofstream file;
        std::string fname = m_pwd + m_outputFolder + m_batchName + ".dat";
        file.open(fname);
        file << "beta " << m_beta << endl;
        file << "acceptanceCounter " << getAcceptanceRate() << endl;
        file << "NCor " << m_NCor << endl;
        file << "NCf " << m_NCf << endl;
        file << "NTherm " << m_NTherm << endl;
        file << std::setprecision(15) << "AverageObservable " << m_averagedObservable << endl; // can setprecision be moved outside the write-to-file?
        file << std::setprecision(15) << "VarianceObservable " << m_varianceObservable << endl;
        file << std::setprecision(15) << "stdObservable " << m_stdObservable << endl;
        if (m_storeThermalizationObservables) {
            for (int i = 0; i < m_NTherm+1; i++) {
                file << std::setprecision(15) << m_observablePreThermalization[i] << endl;
            }
            file << endl;
        }
        for (int i = 0; i < m_NCf; i++) {
            file << std::setprecision(15) << m_observable[i] << endl;
        }
        file.close();
        cout << fname << " written." << endl;
    }
}


void System::printAcceptanceRate()
{
    /*
     * Returns the acceptance ratio of the main run of the System algorithm.
     */
    if (m_processRank == 0) printf("Acceptancerate: %.16f \n", getAcceptanceRate());
}

double System::getAcceptanceRate()
{
    /*
     * Returns the acceptance ratio of the main run of the System algorithm.
     */
    return double(m_acceptanceCounter)/double(m_NCf*m_NCor*m_NUpdates*m_latticeSize*4); // Times 4 from the Lorentz indices
}

void System::writeConfigurationToFile(int configNumber)
{
    /*
     * C-method for writing out configuration to file.
     * Arguments:
     *  configNumber   : (int) configuration number
     */

    MPI_File file;
    std::string filename = m_pwd + m_outputFolder + m_batchName
                                            + "_beta" + std::to_string(m_beta)
                                            + "_spatial" + std::to_string(m_NSpatial)
                                            + "_temporal" + std::to_string(m_NTemporal)
                                            + "_threads" + std::to_string(m_numprocs)
                                            + "_config" + std::to_string(configNumber) + ".bin";

    MPI_File_open(MPI_COMM_SELF, filename.c_str(), MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &file);
    MPI_Offset nt = 0, nz = 0, ny = 0, nx = 0;

    for (unsigned int t = 0; t < m_N[3]; t++) {
        nt = (m_neighbourLists->getProcessorDimensionPosition(3) * m_N[3] + t);
        for (unsigned int z = 0; z < m_N[2]; z++) {
            nz = (m_neighbourLists->getProcessorDimensionPosition(2) * m_N[2] + z);
            for (unsigned int y = 0; y < m_N[1]; y++) {
                ny = (m_neighbourLists->getProcessorDimensionPosition(1) * m_N[1] + y);
                for (unsigned int x = 0; x < m_N[0]; x++) {
                    nx = (m_neighbourLists->getProcessorDimensionPosition(0) * m_N[0] + x);
                    MPI_File_write_at(file, Parallel::Index::getGlobalIndex(nx,ny,nz,nt)*linkSize, &m_lattice[Parallel::Index::getIndex(x,y,z,t)], linkDoubles, MPI_DOUBLE, MPI_STATUS_IGNORE);
                }
            }
        }
    }
    MPI_File_close(&file);
}

void System::loadFieldConfiguration(std::string filename)
{
    /*
     * Method for loading a field configuration and running the plaquettes on them.
     * Arguments:
     * - filename
     */
    MPI_File file;
    MPI_File_open(MPI_COMM_SELF, (m_pwd + m_outputFolder + filename).c_str(), MPI_MODE_RDONLY, MPI_INFO_NULL, &file);
    MPI_Offset nt = 0, nz = 0, ny = 0, nx = 0;

//    double val = 0;
    for (unsigned int t = 0; t < m_N[3]; t++) {
        nt = (m_neighbourLists->getProcessorDimensionPosition(3) * m_N[3] + t);
        for (unsigned int z = 0; z < m_N[2]; z++) {
            nz = (m_neighbourLists->getProcessorDimensionPosition(2) * m_N[2] + z);
            for (unsigned int y = 0; y < m_N[1]; y++) {
                ny = (m_neighbourLists->getProcessorDimensionPosition(1) * m_N[1] + y);
                for (unsigned int x = 0; x < m_N[0]; x++) {
                    nx = (m_neighbourLists->getProcessorDimensionPosition(0) * m_N[0] + x);
                    MPI_File_read_at(file, Parallel::Index::getGlobalIndex(nx,ny,nz,nt)*linkSize, &m_lattice[Parallel::Index::getIndex(x,y,z,t)], linkDoubles, MPI_DOUBLE, MPI_STATUS_IGNORE);

//                    for (int link = 0; link < 4; link++) {
//                        for (int i = 0; i < 18; i++) {
//                            MPI_File_read_at(file, Parallel::Index::getGlobalIndex(nx,ny,nz,nt)*linkSize + link*18*sizeof(double) + i*sizeof(double), &val, 1, MPI_DOUBLE, MPI_STATUS_IGNORE);
//                            m_lattice[Parallel::Index::getIndex(x,y,z,t)].U[link].mat[i] = Reversedouble(val);
//                            // Checking for corruption
//                            if (isnan(m_lattice[Parallel::Index::getIndex(x,y,z,t)].U[link].mat[i]))
//                            {
//                                m_lattice[Parallel::Index::getIndex(x,y,z,t)].U[link].printMachine();
//                                printf("\nConfiguration is corrupt.\n");
//                                exit(1);
//                            }
//                        }
//                    }
                }
            }
        }
    }
    MPI_File_close(&file);
    if (m_processRank == 0) cout << "Configuration " << m_outputFolder + filename << " loaded." << endl;
}

//void System::setFlowSampling(std::vector<std::string> flowObs)
//{
//    m_flowObservableStorage = new ObservableStorer*[flowObs.size()];
//    m_NFlowObs = flowObs.size();
//    for (unsigned int i = 0; i < m_NFlowObs; i++)
//    {
//        if (flowObs[i] == "all") // Default
//        {
//            if (m_NFlowObs != 1 && Parallel::Communicator::getProcessRank() == 0) {
//                printf("\nError: inccorrect number of configs given: %lu", m_NFlowObs);
//                exit(0);
//            }
//            m_sampleFlowTopCharge = true;
//            m_sampleFlowEnergyDensity = true;
//            m_sampleFlowPlaquette = true;
//        }
//        else if (flowObs[i] == "topcharge")
//        {
//            m_sampleFlowTopCharge = true;

//        }
//        else if (flowObs[i] == "energydensity")
//        {
//            m_sampleFlowEnergyDensity = true;
//        }
//        else if (flowObs[i] == "plaquette")
//        {
//            m_sampleFlowPlaquette = true;
//        }
//        else {
//            if (Parallel::Communicator::getProcessRank() == 0) {
//                printf("ERROR: Observable %s not found in library: \n  plaquette\n  topcharge  \nenergydensity",flowObs[i].c_str());
//                exit(1);
//            }
//        }
//    }
//    // Allocate arrays and stuff for observables and their statistics
//    if (m_sampleFlowPlaquette) m_flowObservableStorage[0] = new ObservableStorer(m_NFlows + 1, "plaquette", true);
//    if (m_sampleFlowTopCharge) m_flowObservableStorage[1] = new ObservableStorer(m_NFlows + 1, "topcharge", false);
//    if (m_sampleFlowEnergyDensity) m_flowObservableStorage[2] = new ObservableStorer(m_NFlows + 1, "energydensity", false);
//    // Also add check for user defined observable(or other observables)?

//}

//void System::setConfigurationSampling(std::vector<std::string> configObs)
//{
//    m_NConfigObs = configObs.size();
//    m_configObservableStorage = new ObservableStorer*[m_NConfigObs];
//    for (unsigned int i = 0; i < configObs.size(); i++)
//    {
//        if (configObs[i] == "all") // Default
//        {
//            if (m_NConfigObs > 1 && Parallel::Communicator::getProcessRank() == 0) {
//                printf("\nError: inccorrect number of configs given: %lu", m_NConfigObs);
//                exit(0);
//            }
//            m_sampleTopCharge = true;
//            m_sampleEnergyDensity = true;
//            m_samplePlaquette = true;
//        }
//        else if (configObs[i] == "topcharge")
//        {
//            m_sampleTopCharge = true;

//        }
//        else if (configObs[i] == "energydensity")
//        {
//            m_sampleEnergyDensity = true;
//        }
//        else if (configObs[i] == "plaquette")
//        {
//            m_samplePlaquette = true;
//            if (m_NConfigObs == 1) m_sampleOnlyPlaquette = true;
//        }
//        else {
//            if (Parallel::Communicator::getProcessRank() == 0) {
//                printf("ERROR: Observable %s not found in library: \n  plaquette\n  topcharge  \nenergydensity",configObs[i].c_str());
//                exit(1);
//            }
//        }
//    }
//    // Allocate arrays and stuff for observables and their statistics
//    if (m_samplePlaquette) m_configObservableStorage[0] = new ObservableStorer(m_NCf + 1, "plaquette", true);
//    if (m_sampleTopCharge) m_configObservableStorage[1] = new ObservableStorer(m_NCf + 1, "topcharge", false);
//    if (m_sampleEnergyDensity) m_configObservableStorage[2] = new ObservableStorer(m_NCf + 1, "energydensity", false);
//    // Also add check for user defined observable(or other observables)?

//}

inline void System::printLine()
{
    for (int i = 0; i < 60; i++)
    {
        cout << "=";
    }
    cout << endl;
}
