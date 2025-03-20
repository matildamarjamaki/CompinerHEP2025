
#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>

int main() {
    std::ifstream inFile("muon_distributions.txt");
    if (!inFile) {
        std::cerr << "Error: Could not open file." << std::endl;
        return 1;
    }

    double pT, eta;
    int totalMuons = 0;
    int detectedMuons = 0;

    while (inFile >> pT >> eta) {
        totalMuons++;
        if (pT > 5.0 && std::abs(eta) < 2.5) {
            detectedMuons++;
        }
    }

    inFile.close();

    double probability = (totalMuons > 0) ? static_cast<double>(detectedMuons) / totalMuons : 0.0;

    std::cout << "Total muons: " << totalMuons << std::endl;
    std::cout << "Detected muons: " << detectedMuons << std::endl;
    std::cout << "Detection probability: " << probability << std::endl;

    return 0;
}