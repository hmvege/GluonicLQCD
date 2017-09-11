#include "IndexOrganiser.h"
#include "links.h"
#include "neighbours.h"
#include "matrices/su3.h"
#include <mpi.h>
#include <vector>

// TEMP
#include <iostream>
using std::cout;
using std::endl;

IndexOrganiser::IndexOrganiser(int processRank) : m_processRank(processRank)
{
//    m_N = new int[4];
//    MPI_Barrier(MPI_COMM_WORLD);
//    for (int i = 0; i < 4; i++) {
//        cout << N[i] << endl;
//        m_N[i] = N[i];
//    }
//    cout << "INITIALIZATION COMPELTE" << endl;
    // Shorthand should be n()
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
//    exchangeU = lattice[getIndex(n[0],n[1],n[2],n[3])].U[SU3Dir];
    MPI_Barrier(MPI_COMM_WORLD);
    cout << "MPI SENDING" << endl;
//    MPI_Sendrecv(&lattice[getIndex(n[0],n[1],n[2],n[3])].U[SU3Dir],18,MPI_DOUBLE,m_neighbourLists->getNeighbours(m_processRank)->list[2*mu+1],0,       // Send
//            &exchangeU,18,MPI_DOUBLE,m_processRank,0,    // Receive
//            MPI_COMM_WORLD,MPI_STATUS_IGNORE);
    DOUBLE CHECK THIS ON PAPER!!
    MPI_Sendrecv(&lattice[getIndex(n[0],n[1],n[2],n[3])].U[SU3Dir],18,MPI_DOUBLE,m_neighbourLists->getNeighbours(m_processRank)->list[2*mu],0,       // Send
            &exchangeU,18,MPI_DOUBLE,m_neighbourLists->getNeighbours(m_processRank)->list[2*mu+1],0,    // Receive
            MPI_COMM_WORLD,MPI_STATUS_IGNORE);
    cout << "MPI COMPLETE" << endl;
    exit(1);
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
//    exchangeU = lattice[getIndex(n[0],n[1],n[2],n[3])].U[SU3Dir];
    MPI_Sendrecv(&lattice[getIndex(n[0],n[1],n[2],n[3])].U[SU3Dir],18,MPI_DOUBLE, m_neighbourLists->getNeighbours(m_processRank)->list[2*mu],0, // Send
            &exchangeU,18,MPI_DOUBLE,m_processRank,0,                 // Receive
            MPI_COMM_WORLD,MPI_STATUS_IGNORE);
}

SU3 IndexOrganiser::getPositiveLink(Links *lattice, std::vector<int> n, int mu, int *muIndex, int SU3Dir)
{
    /*
     * TWO CASES:
     * - Neighbour link
     * - Neighbours neighbor link, wont matter the order of the mu's
     * REQUIREMENTS:
     * - Need to pass the lorentz index, as that determines the directionality.
     * - The positive/negative direction determined by wetter or not the index is 0 (negative) or N (positive) after the modulus has been performed
     * IF i + N % N == 0 positive direction
     * IF i + N & N == N-1
     * IDEA: make (for single shift) one function for positive direction and one for negative direction
     */
    // IDEA: TAKE A VECTOR OF n AND MAKE IT ACCESSIBLE BY MU IN ORDER TO FIGURE OUT IF WE ARE AT AN EDGE OR NOT
//    MPI_Barrier(MPI_COMM_WORLD);
//    cout << "INSIDE INDEXORGANISER " << n[mu] << " " << mu << " " << muIndex[mu] << endl;
//    MPI_Barrier(MPI_COMM_WORLD);
//    for (int i = 0; i < 4; i++) {
//        cout << "i = " << i << " muIndex[" << i << "] = " << muIndex[i] << " n[" << i << "] = " << n[i] << " N[" << i << "] = " << m_N[i] << endl;
//    }
//    exit(1);
//    MPI_Barrier(MPI_COMM_WORLD);
//    lattice[getIndex(n[0]+muIndex[0], n[1]+muIndex[1], n[2]+muIndex[2], n[3]+muIndex[3])].U[SU3Dir].print(); // ERROR IN ACCESSING THIS ELEMENT!
    cout << n[mu]<<endl;
    if ((n[mu]+muIndex[mu]) % m_N[mu] == 0) {
//        exchangeU = lattice[getIndex(n[0],n[1],n[2],n[3])].U[mu];
//        MPI_Sendrecv(&exchangeU,18,MPI_DOUBLE,m_neighbourLists->getNeighbours(m_processRank)->list[2*mu+1],0,   // Send
//                &lattice[getIndex(n[0],n[1],n[2],n[3])].U[mu],18,MPI_DOUBLE,m_processRank,0,                    // Receive
//                MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        n[mu] = 0;
        MPI_Barrier(MPI_COMM_WORLD);
        MPIfetchSU3Positive(lattice,n,mu,SU3Dir);
        cout << "INSIDE INDEXORGANISER: AFTER MPI" << endl;
        exit(1);
        return exchangeU;
    }
    else {
        return lattice[getIndex(n[0]+muIndex[0], n[1]+muIndex[1], n[2]+muIndex[2], n[3]+muIndex[3])].U[SU3Dir];
    }
}

SU3 IndexOrganiser::getNegativeLink(Links *lattice, std::vector<int> n, int mu, int *muIndex, int SU3Dir)
{
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
    if (muDir) {
        // Single mpi call?
        n[mu] = 0;
        MPIfetchSU3Positive(lattice, n, mu, SU3Dir);
        return exchangeU;
    }
    else if (nuDir) {
        // Single mpi call?
        n[mu] = m_N[mu] - 1;
        MPIfetchSU3Negative(lattice, n, nu, SU3Dir);
        return exchangeU;
    }
    else if (muDir && nuDir) {
        // Double mpi call?
        n[mu] = 0;
        n[mu] = m_N[mu] - 1;
//        exchangeU = lattice[getIndex(n[0],n[1],n[2],n[3])].U[SU3Dir];
        MPI_Sendrecv(&lattice[getIndex(n[0],n[1],n[2],n[3])].U[SU3Dir],18,MPI_DOUBLE, m_neighbourLists->getNeighbours((m_neighbourLists->getNeighbours(m_processRank)->list[2*mu+1]))->list[2*nu],0, // Send
                &exchangeU,18,MPI_DOUBLE,m_processRank,0,                 // Receive
                MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        return exchangeU;
    }
    else {
        return lattice[getIndex(n[0]+muIndex[0]-nuIndex[0], n[1]+muIndex[1]-nuIndex[1], n[2]+muIndex[2]-nuIndex[2], n[3]+muIndex[3]-nuIndex[2])].U[SU3Dir];
    }
    // Position of targeted neighbour
//    m_neighbourLists->getNeighbours((m_neighbourLists->getNeighbours(m_processRank)->list[2*mu+1]))->list[2*nu];
}

int IndexOrganiser::getIndex(int i, int j, int k, int l)
{
    return (m_N[3]*(m_N[2]*(m_N[1]*i + j) + k) + l);
}

void IndexOrganiser::setN(int *N)
{
    for (int i = 0; i < 4; i++) {
        m_N[i] = N[i];
    }
}
