
#include <iostream>
#include <cmath>
#include "MET.h"

int main() {
    double x, y;

    // Pyydetään käyttäjältä syötteet
    std::cout << "Anna MET_x: ";
    std::cin >> x;
    std::cout << "Anna MET_y: ";
    std::cin >> y;

    // Luodaan MET-olio käyttäjän syötteillä
    MET met(x, y);

    // Tulostetaan arvot
    std::cout << "\nTulokset:" << std::endl;
    std::cout << "MET: " << met.getMET() << std::endl;
    std::cout << "MET_x: " << met.getMETx() << std::endl;
    std::cout << "MET_y: " << met.getMETy() << std::endl;
    std::cout << "MET_phi: " << met.getMETphi() << std::endl;

    return 0;
}