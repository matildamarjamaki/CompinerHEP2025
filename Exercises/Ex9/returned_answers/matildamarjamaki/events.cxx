
#include "Pythia8/Pythia.h"
#include <iostream>
#include <fstream>

using namespace Pythia8;

int main() {
    Pythia pythia;
    pythia.readString("Beams:eCM = 13600.");  // Set center-of-mass energy to 13.6 TeV
    pythia.readString("SoftQCD:inelastic = on"); // Minimum bias events
    pythia.init();

    std::ofstream outFile("muon_distributions.txt");
    if (!outFile) {
        std::cerr << "Error: Could not open file for writing." << std::endl;
        return 1;
    }

    int nEvents = 100000; // Number of events to simulate
    for (int iEvent = 0; iEvent < nEvents; ++iEvent) {
        if (!pythia.next()) continue;

        for (int i = 0; i < pythia.event.size(); ++i) {
            if (abs(pythia.event[i].id()) == 13) { // Muons only
                double pT = pythia.event[i].pT();
                double eta = pythia.event[i].eta();
                outFile << pT << " " << eta << "\n";
            }
        }
    }

    outFile.close();
    pythia.stat();
    return 0;
}
