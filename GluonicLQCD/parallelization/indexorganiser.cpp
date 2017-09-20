#include "indexorganiser.h"
#include "links.h"
#include "neighbours.h"
#include "matrices/su3.h"
#include <mpi.h>
#include <vector>

// TEMP TESTS
#include <iostream>
using std::cout;
using std::endl;

IndexOrganiser::IndexOrganiser(int processRank) : m_processRank(processRank)
{
    /*
     * IndexOrganiser initialiser. After initialisation, must set the int*N and m_neighbourLists manually.
     * Takes:
     *  processRank : process ID as given by MPI initialisation
     */
}

IndexOrganiser::~IndexOrganiser()
{

}

void IndexOrganiser::MPIfetchSU3Positive(Links *lattice, std::vector<int> n, int mu, int SU3Dir)
{
    /*
     * Performs an MPI call to retrieve matrix in the positive direction.
     * Arguments:
     *  lattice     : the entire lattice passed
     *  n           : position vector
     *  mu          : lorentz index for shift direction(always negative in either x,y,z or t direction)
     *  SU3Dir      : SU3 matrix direction at link
     */
    MPI_Sendrecv(&lattice[getIndex(n[0],n[1],n[2],n[3])].U[SU3Dir],18,MPI_DOUBLE,m_neighbourLists->getNeighbours(m_processRank)->list[2*mu+1],0, // Send
            &exchangeU,18,MPI_DOUBLE,m_neighbourLists->getNeighbours(m_processRank)->list[2*mu],0,                                               // Receive
            MPI_COMM_WORLD,MPI_STATUS_IGNORE);
}

void IndexOrganiser::MPIfetchSU3Negative(Links *lattice, std::vector<int> n, int mu, int SU3Dir)
{
    /*
     * Performs an MPI call to retrieve matrix in the negative direction.
     * Arguments:
     *  lattice     : the entire lattice passed
     *  n           : position vector
     *  mu          : lorentz index for shift direction(always negative in either x,y,z or t direction)
     *  SU3Dir      : SU3 matrix direction at link
     */
    MPI_Sendrecv(&lattice[getIndex(n[0],n[1],n[2],n[3])].U[SU3Dir],18,MPI_DOUBLE,m_neighbourLists->getNeighbours(m_processRank)->list[2*mu],0,  // Send
            &exchangeU,18,MPI_DOUBLE,m_neighbourLists->getNeighbours(m_processRank)->list[2*mu+1],0,                                            // Receive
            MPI_COMM_WORLD,MPI_STATUS_IGNORE);
}

SU3 IndexOrganiser::getPositiveLink(Links *lattice, std::vector<int> n, int mu, int *muIndex, int SU3Dir)
{
    /*
     * Function for retrieving link in positive direction.
     * Takes:
     *  lattice     : constisting of links
     *  n           : position vector in lattice
     *  mu          : lorentz index mu
     *  muIndex     : lorentz "vector"
     *  SU3Dir      : which of the four SU3 matrices which we need
     */
    if ((n[mu]+muIndex[mu]) % m_N[mu] == 0) {
        n[mu] = 0;
        MPIfetchSU3Positive(lattice,n,mu,SU3Dir);
        return exchangeU;
    }
    else {
        return lattice[getIndex(n[0]+muIndex[0], n[1]+muIndex[1], n[2]+muIndex[2], n[3]+muIndex[3])].U[SU3Dir];
    }
}

SU3 IndexOrganiser::getNegativeLink(Links *lattice, std::vector<int> n, int mu, int *muIndex, int SU3Dir)
{
    /*
     * Function for retrieving link in negative direction.
     * Takes:
     *  lattice     : constisting of links
     *  n           : position vector in lattice
     *  mu          : lorentz index mu
     *  muIndex     : lorentz "vector"
     *  SU3Dir      : which of the four SU3 matrices which we need
     */
    if ((n[mu] - muIndex[mu] + m_N[mu]) % m_N[mu] == (m_N[mu] - 1)) {
        n[mu] = m_N[mu] - 1;
        MPIfetchSU3Negative(lattice,n,mu,SU3Dir);
        return exchangeU;
    }
    else {
        return lattice[getIndex(n[0]-muIndex[0], n[1]-muIndex[1], n[2]-muIndex[2], n[3]-muIndex[3])].U[SU3Dir];
    }
}

SU3 IndexOrganiser::getNeighboursNeighbourLink(Links * lattice, std::vector<int> n, int mu, int *muIndex, int nu, int *nuIndex, int SU3Dir)
{
    /*
     * For our program, mu is always in the positive direction and nu is always in the negative direction.
     */
    bool muDir = (n[mu] + muIndex[mu]) % m_N[mu] == 0;
    bool nuDir = (n[nu] - nuIndex[nu] + m_N[nu]) % m_N[nu] == (m_N[nu] - 1);
    if (muDir && (!nuDir)) {
        // Positive mu direction
        n[mu] = 0;
        MPI_Sendrecv(&lattice[getIndex(n[0]-nuIndex[0],n[1]-nuIndex[1],n[2]-nuIndex[2],n[3]-nuIndex[3])].U[SU3Dir],18,MPI_DOUBLE, m_neighbourLists->getNeighbours(m_processRank)->list[2*mu+1],0,   // Send
                &exchangeU,18,MPI_DOUBLE,m_neighbourLists->getNeighbours(m_processRank)->list[2*mu],0,                                                                                              // Receive
                MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        return exchangeU;
    }
    else if (nuDir && (!muDir)) {
        // Negative nu direction
        n[nu] = m_N[nu] - 1;
        MPI_Sendrecv(&lattice[getIndex(n[0]+muIndex[0],n[1]+muIndex[1],n[2]+muIndex[2],n[3]+muIndex[3])].U[SU3Dir],18,MPI_DOUBLE, m_neighbourLists->getNeighbours(m_processRank)->list[2*nu],0, // Send
                &exchangeU,18,MPI_DOUBLE,m_neighbourLists->getNeighbours(m_processRank)->list[2*nu+1],0,                                                                                        // Receive
                MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        return exchangeU;
    }
    else if (muDir && nuDir) {
        // True edge case
        n[mu] = 0;
        n[nu] = m_N[nu] - 1;
        MPI_Sendrecv(&lattice[getIndex(n[0],n[1],n[2],n[3])].U[SU3Dir],18,MPI_DOUBLE, m_neighbourLists->getNeighbours((m_neighbourLists->getNeighbours(m_processRank)->list[2*mu+1]))->list[2*nu],0,// Send
                &exchangeU,18,MPI_DOUBLE,m_neighbourLists->getNeighbours((m_neighbourLists->getNeighbours(m_processRank)->list[2*mu]))->list[2*nu+1],0,                                             // Receive
                MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        return exchangeU;
    }
    else {
        return lattice[getIndex(n[0]+muIndex[0]-nuIndex[0], n[1]+muIndex[1]-nuIndex[1], n[2]+muIndex[2]-nuIndex[2], n[3]+muIndex[3]-nuIndex[3])].U[SU3Dir];
    }
}

int IndexOrganiser::getIndex(int i, int j, int k, int l)
{
    /*
     * Function for retrieving lattice position in contigious memory allocation.
     *  i   : x position
     *  j   : y position
     *  k   : z position
     *  l   : t position
     */
    return (m_N[3]*(m_N[2]*(m_N[1]*i + j) + k) + l); // OLD
//    return (m_N[2]*(m_N[1]*(m_N[0]*j + i) + k) + l); // NEW
}

void IndexOrganiser::setN(int *N)
{
    /*
     * Function for setting the dimensionality of the sublattice.
     * Takes:
     *  N       : an array of ints of length 4, where each element is the dimension size of either x,y,z or t
     */
    for (int i = 0; i < 4; i++) {
        m_N[i] = N[i];
    }
    if (m_processRank == 0) {
        cout << "Setting the sub lattice size in indexOrganizer: ";
        for (int i = 0; i < 4; i++) {
            cout << m_N[i];
        }
        cout << endl;
    }
}
