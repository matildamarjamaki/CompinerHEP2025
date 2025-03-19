
#include "Pythia8/Pythia.h"
#include <iostream>

using namespace Pythia8;

int main() {
    Pythia pythia;
    
    // Set SM Higgs boson mass
    pythia.readString("Higgs:useBSM = off");  // Ensure SM Higgs is used
    pythia.readString("25:m0 = 125.0");       // Higgs mass (GeV)

    // Initialize Pythia
    pythia.init();

    // Retrieve the Higgs boson width
    double higgsWidth = pythia.particleData.mWidth(25);

    // Print result
    std::cout << "SM Higgs boson width for m_H = 125 GeV: " 
              << higgsWidth << " GeV" << std::endl;

    return 0;
}