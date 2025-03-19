
#include "Pythia8/Pythia.h"
#include <iostream>

using namespace Pythia8;

int main() {
    Pythia pythia;
    
    pythia.readString("Higgs:useBSM = off");
    pythia.readString("25:m0 = 125.0");

    pythia.init();

    double higgsWidth = pythia.particleData.mWidth(25);

    std::cout << "SM Higgs boson width for m_H = 125 GeV: " 
              << higgsWidth << " GeV" << std::endl;

    return 0;
}