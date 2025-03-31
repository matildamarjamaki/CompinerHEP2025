
#include "Pythia8/Pythia.h"
#include "TFile.h"
#include "TTree.h"
#include <iostream>
#include "TH1F.h"
#include "TCanvas.h"
#include "TLegend.h"
#include <random>

using namespace Pythia8;
using namespace std;


// Function to apply Gaussian smearing to the muons' momentum
void applyMuonSmearing(Pythia8::Particle &muon) {
    std::random_device rd;
    std::mt19937 gen(rd());

    // Applying Gaussian smearing to the transverse momentum (1% smearing)
    std::normal_distribution<> d_momentum(0, 0.01);  // 1% Gaussian smearing
    double momentumSmear = d_momentum(gen); // Draw a random number from the distribution
    double smearedMomentum = muon.pT() * (1 + momentumSmear); // Apply smearing to pT

    // Calculate the new components of the 4-momentum after applying smearing
    double eta = muon.eta(); // Pseudorapidity
    double phi = muon.phi(); // Azimuthal angle
    double pT = smearedMomentum;
    double pZ = pT * sinh(eta);  // pZ = pT * sinh(eta)

    // Components of the 3-momentum
    double pX = pT * cos(phi);
    double pY = pT * sin(phi);

    // Update the particle's 4-momentum
    Pythia8::Vec4 newP(pX, pY, pZ, sqrt(pX * pX + pY * pY + pZ * pZ + muon.m() * muon.m()));
    muon.p() = newP;  // Update the particle's 4-momentum

    // Apply Gaussian smearing to the angles (2 mrad)
    std::normal_distribution<> d_angle(0, 0.002);  // 2 mrad Gaussian smearing
    double smearedTheta = muon.theta() + d_angle(gen);
    double smearedPhi = muon.phi() + d_angle(gen);

    // Update the particle’s 4-momentum based on smeared angles
    double smearedPt = muon.pT();
    double smearedEta = muon.eta();
    Pythia8::Vec4 smearedP(smearedPt * cos(smearedPhi), smearedPt * sin(smearedPhi), smearedPt * sinh(smearedEta), sqrt(smearedPt * smearedPt + muon.m() * muon.m()));
    muon.p() = smearedP;  // Apply the smeared momentum
}

// Function to calculate ΔR (the separation between two particles in η-φ space)
double calculateDeltaR(double eta1, double phi1, double eta2, double phi2) {
    double dEta = eta1 - eta2;
    double dPhi = fabs(phi1 - phi2);
    if (dPhi > M_PI) dPhi -= 2 * M_PI; // Adjust for periodicity of φ
    return sqrt(dEta * dEta + dPhi * dPhi);
}

// Function to check if a muon is isolated (no other particles within a cone of ΔR < 0.3)
bool isMuonIsolated(Pythia8::Particle &muon, const vector<Pythia8::Particle> &chargedPions) {
    double isolationSum = 0.0;
    
    // Check isolation within ΔR < 0.3
    for (auto &pion : chargedPions) {
        double deltaR = calculateDeltaR(muon.eta(), muon.phi(), pion.eta(), pion.phi());
        if (deltaR < 0.3) {
            isolationSum += pion.pT(); // Sum pT of particles close to the muon
        }
    }

    return isolationSum < 1.5;  // Isolation criterion: sum of pT of nearby pions should be < 1.5 GeV/c
}

// Function to calculate the invariant mass of two particles
double invariantMass(double pT1, double eta1, double phi1, double m1, double pT2, double eta2, double phi2, double m2) {
    // Calculate the energy and momentum components for both particles
    double theta1 = 2 * atan(exp(-eta1)); // Convert eta to theta
    double theta2 = 2 * atan(exp(-eta2)); // Convert eta to theta
    
    double pX1 = pT1 * cos(phi1); // x-component of momentum for particle 1
    double pY1 = pT1 * sin(phi1); // y-component of momentum for particle 1
    double pZ1 = pT1 / tan(theta1); // z-component of momentum for particle 1
    double E1 = sqrt(pX1 * pX1 + pY1 * pY1 + pZ1 * pZ1 + m1 * m1); // Energy of particle 1
    
    double pX2 = pT2 * cos(phi2); // x-component of momentum for particle 2
    double pY2 = pT2 * sin(phi2); // y-component of momentum for particle 2
    double pZ2 = pT2 / tan(theta2); // z-component of momentum for particle 2
    double E2 = sqrt(pX2 * pX2 + pY2 * pY2 + pZ2 * pZ2 + m2 * m2); // Energy of particle 2
    
    // Calculate the invariant mass (mZ) of the two particles
    double mZ = sqrt(pow(E1 + E2, 2) - (pow(pX1 + pX2, 2) + pow(pY1 + pY2, 2) + pow(pZ1 + pZ2, 2)));
    return mZ; // Return the invariant mass
}

int main() {
    Pythia pythia; // Create a Pythia object to generate events

    // Set up the beams and production of Z-bosons (center-of-mass energy = 13600 GeV)
    pythia.readString("Beams:eCM = 13600.");
    pythia.readString("WeakSingleBoson:ffbar2gmZ = on"); // Enable Z-boson production
    pythia.readString("23:onMode = off"); // Turn off decay channels for Z-boson
    pythia.readString("23:onIfAny = 13 -13"); // Enable Z-boson decay to muons (μμ)

    // Set up background process: t-tbar production
    pythia.readString("Top:gg2ttbar = on"); // Enable t-tbar production from gluon-gluon fusion
    pythia.readString("Top:qqbar2ttbar = on"); // Enable t-tbar production from quark-antiquark annihilation

    // Don't show events on the console
    pythia.readString("Next:numberShowEvent = 0");

    pythia.init();

    // ROOT file and TTree objects for signal and background events
    TTree *treeSignal = new TTree("Zmm", "Z->mumu events");
    TTree *treeBackground = new TTree("ttbar", "ttbar background events");

    // Variables to store event data
    float muon1_pT, muon2_pT, muon1_eta, muon2_eta, muon1_phi, muon2_phi;
    int muon1_charge, muon2_charge, eventType;
    bool passedTrigger;

    // Branches for signal TTree
    treeSignal->Branch("muon1_pT", &muon1_pT);
    treeSignal->Branch("muon2_pT", &muon2_pT);
    treeSignal->Branch("muon1_eta", &muon1_eta);
    treeSignal->Branch("muon2_eta", &muon2_eta);
    treeSignal->Branch("muon1_charge", &muon1_charge);
    treeSignal->Branch("muon2_charge", &muon2_charge);
    treeSignal->Branch("passedTrigger", &passedTrigger);
    treeSignal->Branch("eventType", &eventType);
    
    // Branches for background TTree
    treeBackground->Branch("muon1_pT", &muon1_pT);
    treeBackground->Branch("muon2_pT", &muon2_pT);
    treeBackground->Branch("muon1_eta", &muon1_eta);
    treeBackground->Branch("muon2_eta", &muon2_eta);
    treeBackground->Branch("muon1_charge", &muon1_charge);
    treeBackground->Branch("muon2_charge", &muon2_charge);
    treeBackground->Branch("passedTrigger", &passedTrigger);
    treeBackground->Branch("eventType", &eventType);
        

    // Counters for trigger efficiency calculation
    int totalZEvents = 0, triggeredZEvents = 0;
    int totalTTbarEvents = 0, triggeredTTbarEvents = 0;

    // Event loop
    int nEvents = 5000; // Number of events to simulate
    int printLimit = 10; // Limit the number of events printed to the console
    int printedZ = 0, printedTTbar = 0;

    // Loop over events to generate and process them
    for (int iEvent = 0; iEvent < nEvents; ++iEvent) {
        if (!pythia.next()) continue; // Generate the next event and skip if failed

        vector<int> selectedMuons; // Store the selected muons from the event
        vector<Pythia8::Particle> chargedPions; // Store charged pions for isolation check

        // Loop over the particles in the event and select muons and pions
        for (int i = 0; i < pythia.event.size(); ++i) {
            if (abs(pythia.event[i].id()) == 13) {  // Muons
                applyMuonSmearing(pythia.event[i]);  // Apply smearing

                // Apply transverse momentum cut
                if (pythia.event[i].pT() > 30.0) {
                    selectedMuons.push_back(i); // Add the muon to the selected list if it passes the pT cut
                }
            }
            if (abs(pythia.event[i].id()) == 211 || abs(pythia.event[i].id()) == 321) {  // Charged pions
                chargedPions.push_back(pythia.event[i]);
            }
        }

        // Skip events that don't have at least two selected muons
        if (selectedMuons.size() < 2) continue;

        // Select the two muons
        Pythia8::Particle &muon1 = pythia.event[selectedMuons[0]];
        Pythia8::Particle &muon2 = pythia.event[selectedMuons[1]];

        // Check track isolation for the muons
        if (!isMuonIsolated(muon1, chargedPions) || !isMuonIsolated(muon2, chargedPions)) continue;

        // Store the muon information
        muon1_pT = muon1.pT();
        muon2_pT = muon2.pT();
        muon1_eta = muon1.eta();
        muon2_eta = muon2.eta();
        muon1_charge = muon1.id() > 0 ? -1 : 1;
        muon2_charge = muon2.id() > 0 ? -1 : 1;

        // Determine if this is a Z event
        bool isZEvent = (muon1_charge != muon2_charge); // Z events must have opposite charges
        eventType = isZEvent ? 0 : 1; // Event type 0 for Z, 1 for ttbar

        // Count events for statistics
        if (isZEvent) totalZEvents++;
        else totalTTbarEvents++;

        // Apply trigger condition (both muons must pass the trigger requirements)
        passedTrigger = (muon1.pT() > 20.0 && muon2.pT() > 20.0 &&
                         fabs(muon1.eta()) < 2.1 && fabs(muon2.eta()) < 2.1);

        if (isZEvent) {
            if (passedTrigger) triggeredZEvents++; // Count the number of triggered Z events
            treeSignal->Fill(); // Store signal event in ROOT tree
        } else {
            if (passedTrigger) triggeredTTbarEvents++; // Count the number of triggered ttbar events
            treeBackground->Fill(); // Store background event in ROOT tree
        }
    }

    // Save ROOT files with the events
    TFile *fileSignal = new TFile("events_Zmm.root", "RECREATE");
    treeSignal->Write(); // Write signal tree to file
    fileSignal->Close();

    TFile *fileBackground = new TFile("events_ttbar.root", "RECREATE");
    treeBackground->Write(); // Write background tree to file
    fileBackground->Close();

    // Print statistics on the trigger efficiency to the console
    double triggerEfficiencyZ = totalZEvents > 0 ? (double)triggeredZEvents / totalZEvents : 0;
    double triggerEfficiencyTTbar = totalTTbarEvents > 0 ? (double)triggeredTTbarEvents / totalTTbarEvents : 0;

    cout << "\nSimulated events: " << nEvents << endl;
    cout << "Total Z->mumu events: " << totalZEvents << ", Passed trigger: " << triggeredZEvents << endl;
    cout << "Total ttbar events: " << totalTTbarEvents << ", Passed trigger: " << triggeredTTbarEvents << endl;
    cout << "Trigger efficiency (Z->mumu): " << triggerEfficiencyZ << endl;
    cout << "Trigger efficiency (ttbar background): " << triggerEfficiencyTTbar << endl;

    // Additional analysis with ROOT could be performed here (e.g., plotting invariant mass, pT, etc.)
    
    // Load data from ROOT files for plotting
    TFile *file = new TFile("events_Zmm.root");
    TTree *tree = (TTree*)file->Get("Zmm");
    
    return 0;
}
