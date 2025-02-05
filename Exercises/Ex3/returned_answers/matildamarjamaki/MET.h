
#ifndef MISSINGET_H
#define MISSINGET_H

#include <cmath>

class MET {
private:
    double met_x;
    double met_y;

public:
    // Konstruktorin määrittely
    MET(double x, double y) : met_x(x), met_y(y) {}

    // Funktio, joka laskee MET-arvon
    double getMET() const {
        return std::sqrt(met_x * met_x + met_y * met_y);
    }

    // x-komponentin palauttava funktio
    double getMETx() const { return met_x; }

    // y-komponentin palauttava funktio
    double getMETy() const { return met_y; }

    // MET-kulman (phi) laskeminen
    double getMETphi() const {
        return std::atan2(met_y, met_x);
    }
};

#endif