#ifndef FLOW_H
#define FLOW_H

#include "math/Links.h"
#include "math/flowexpfunctions.h"
#include "actions/action.h"

class Flow
{
private:
    // Flow update step
    double m_epsilon = 0.02;
    // Lattice constants
    unsigned int m_N[4];
    unsigned int m_subLatticeSize;
    // Temporary lattice to use when flowing
    Links * m_tempLattice;
    // Updates the lattice with the exponantiated lattice values
    inline void updateLattice(Links *lattice);
    // SU3 exponentiation function
    SU3Exp *m_SU3ExpFunc = nullptr;
    void setSU3ExpFunc();
    // Action pointer
    Action *m_S = nullptr;
public:
    Flow(Action *S);
    ~Flow();
    void flowField(Links *lattice);
};

#endif // FLOW_H
