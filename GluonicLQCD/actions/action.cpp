#include "action.h"

Action::Action()
{
    m_N = new unsigned int[4];
    for (int i = 0; i < 4; i++) {
        m_N[i] = 0;
    }
    m_position = std::vector<int>(4,0);
}

Action::~Action()
{
    delete [] m_N;
}

double Action::getDeltaAction(Links * lattice, SU3 U, unsigned int i, unsigned int j, unsigned int k, unsigned int l, int mu)
{
    cout << "In Action::getDeltaAction: If you are seeing this, something is wrong!" << endl;
    exit(1);
    return 1.0;
}

void Action::computeStaple(Links *lattice, unsigned int i, unsigned int j, unsigned int k, unsigned int l, int mu)
{
    cout << "In Action::computeStaple: If you are seeing this, something is wrong!" << endl;
    exit(1);
}

void Action::setN(unsigned int *N)
{
    for (int i = 0; i < 4; i++) {
        m_N[i] = N[i];
    }
}

SU3 Action::getActionDerivative(Links * lattice, unsigned int i, unsigned int j, unsigned int k, unsigned int l, int mu)
{
    cout << "In Action::getActionDerivative: If you are seeing this, something is wrong!" << endl;
    exit(1);
}
