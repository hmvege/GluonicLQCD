#ifndef WILSONGAUGEACTION_H
#define WILSONGAUGEACTION_H

#include "action.h"

class WilsonGaugeAction : public Action
{
private:
    double m_beta;
    SU3 m_staple;
public:
    WilsonGaugeAction(double beta);
//    WilsonGaugeAction(int *N, double beta);
    ~WilsonGaugeAction();
    double getDeltaAction(Links *lattice, SU3 UPrime, int i, int j, int k, int l, int mu);
    void computeStaple(Links *lattice, int i, int j, int k, int l, int mu);
};

#endif // WILSONGAUGEACTION_H
