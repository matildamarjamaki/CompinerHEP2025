
#include <iostream>
#include <sstream>
#include <cmath>

// Luokka rekonstruoidulle radalle, sisältää 4-momentin komponentit
class Track {
private:
    double px, py, pz, E; // 4-momentin komponentit (px, py, pz = impulssi, E = energia)

public:
    // Konstruktori, joka alustaa momenttikomponentit
    Track(double px, double py, double pz, double E) : px(px), py(py), pz(pz), E(E) {}

    // Getterit 4-momentin komponenttien hakemiseen
    double getPx() const { return px; } // Palauttaa x-momenttikomponentin
    double getPy() const { return py; } // Palauttaa y-momenttikomponentin
    double getPz() const { return pz; } // Palauttaa z-momenttikomponentin
    double getEnergy() const { return E; } // Palauttaa energian
    
    // Laskee ja palauttaa poikittaisimpulssin p_T = sqrt(px^2 + py^2)
    double transverseMomentum() const {
        return std::sqrt(px * px + py * py);
    }
    
    // Laskee ja palauttaa pseudorapiditeetin eta = -log(tan(theta / 2))
    double pseudorapidity() const {
        if (pz == 0) return 0; // Varmistetaan, ettei log(0) aiheuta virhettä
        double theta = std::atan2(std::sqrt(px * px + py * py), pz); // Polaarikulma
        return -std::log(std::tan(theta / 2.0)); // Pseudorapiditeetti
    }
};

// Simuloidun radan luokka, käyttää Track-luokkaa ja lisää Monte Carlo -tiedot
class SimulatedTrack : public Track {
private:
    int particleid; // Hiukkastunniste (normihiukkanen)
    int parentid;   // Emo(parent)-hiukkasen tunniste

public:
    // Konstruktori, jossa asetetaan SimulatedTrack-luokan olion jäsenmuuttujille alkuarvot, jotka sisältävät momentin komponentit ja Monte Carlo(MC) -simulaatiotiedot
    SimulatedTrack(double px, double py, double pz, double E, int pid, int parent) 
        : Track(px, py, pz, E), particleid(pid), parentid(parent) {}

    // Getterit MC-tiedolle
    int getParticleid() const { return particleid; } // Palauttaa hiukkasen tunnisteen
    int getParentid() const { return parentid; } // Palauttaa emo-hiukkasen tunnisteen
};

int main() {
    double px, py, pz, E;
    int pid, parent;

    // Pyydetään käyttäjältä syöte 4-momentille ja hiukkas-id:lle
    std::cout << "Enter the track's 4-momentum components (px py pz E), particle id and parent particle id: ";
    
    std::string inputLine;
    std::getline(std::cin, inputLine); // Luetaan käyttäjän syöte kokonaisena rivinä
    std::istringstream iss(inputLine); // Käytetään stringstreamiä syötteen käsittelyyn

    // Luetaan px, py, pz ja E
    if (!(iss >> px >> py >> pz >> E)) {
        std::cerr << "Error: Invalid input. Please enter four numerical values for px, py, pz, and E.\n";
        return 1; // Jos syöte on virheellinen, tulostetaan virheilmoitus ja lopetetaan ohjelma
    }
    
    // Tarkistetaan, syöttikö käyttäjä lisätietoja simuloidulle radalle
    if (iss >> pid >> parent) {  // Yritetään lukea pid ja parent
    // Tulostetaan simuloidun radan laskennallisia arvoja
        SimulatedTrack simTrack(px, py, pz, E, pid, parent);
        std::cout << "Simulated track transverse momentum: " << simTrack.transverseMomentum() << "\n";
        std::cout << "Simulated track pseudorapidity: " << simTrack.pseudorapidity() << "\n";
        std::cout << "Particle id: " << simTrack.getParticleid() << "\n";
        std::cout << "Parent id: " << simTrack.getParentid() << "\n";
    // Jos pid ja parent eivät ole mukana syötteessä, luodaan tavallinen Track-objekti
    } else {
        Track track(px, py, pz, E); // Luodaan Track-objekti
        // Tulostetaan tavallisen radan laskennallisia arvoja
        std::cout << "Track transverse momentum: " << track.transverseMomentum() << "\n";
        std::cout << "Track pseudorapidity: " << track.pseudorapidity() << "\n";
    }

    return 0;
}