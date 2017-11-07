#ifndef ENERGYDENSITY_H
#define ENERGYDENSITY_H

#include "correlator.h"

class EnergyDensity : public Correlator
{
private:
    double m_multiplicationFactor;
    double m_actionDensity;
public:
    SU3 m_temp;
    EnergyDensity(double a, int latticeSize);
    EnergyDensity();
    double calculate(SU3 *clovers);
    double calculate(Links *lattice);
    void setLatticeSpacing(double a);
};

#endif // ENERGYDENSITY_H
