
// background.cc
// Include necessary headers from Pythia8 and ROOT
#include "Pythia8/Pythia.h"
#include "TFile.h"
#include "TTree.h"
#include "TH1F.h"
#include "TLorentzVector.h"
#include "TRandom3.h"
#include <vector>
#include <iostream>

using namespace Pythia8;

// Trigger requirement: both muons must have pT > 20 GeV and |eta| < 2.1
bool passesTrigger(const TLorentzVector& mu1, const TLorentzVector& mu2) {
    return (fabs(mu1.Eta()) < 2.1 && mu1.Pt() > 20.0 &&
            fabs(mu2.Eta()) < 2.1 && mu2.Pt() > 20.0);
}

// Apply Gaussian smearing to muon kinematics (to simulate detector resolution)
void smearMuon(TLorentzVector& muon, TRandom3& rng) {
    float smear = rng.Gaus(1.0, 0.01);                  // pT smearing
    float theta = muon.Theta() + rng.Gaus(0.0, 0.002);  // angle smearing
    float phi   = muon.Phi()   + rng.Gaus(0.0, 0.002);
    float pt    = muon.Pt() * smear;
    float mass  = muon.M();
    float px = pt * cos(phi);
    float py = pt * sin(phi);
    float pz = pt / tan(theta);
    float energy = sqrt(px*px + py*py + pz*pz + mass*mass);
    muon.SetPxPyPzE(px, py, pz, energy);    // update muon 4-momentum
}

// Isolation requirement: sum of nearby pion pT within ΔR < 0.3 must be < 1.5 GeV
bool isIsolated(const TLorentzVector& muon, const std::vector<TLorentzVector>& pions) {
    float ptSum = 0.0;
    for (const auto& pion : pions) {
        if (pion.Pt() < 0.1 || fabs(pion.Eta()) > 2.5) continue;
        if (muon.DeltaR(pion) < 0.3) ptSum += pion.Pt();
    }
    return ptSum < 1.5;
}

int main() {
    // Initialize Pythia for top-antitop pair production
    Pythia pythia;
    pythia.readString("Top:gg2ttbar = on"); // Enable gg → tt̅
    pythia.readString("Top:qqbar2ttbar = on");
    pythia.readString("6:m0 = 172.5");      // Set top quark mass
    pythia.readString("Beams:eCM = 13600.");  // LHC Run 3 energy
    pythia.init();

    // Create output ROOT file and objects
    TFile* file = new TFile("background.root", "RECREATE");
    TTree* tree = new TTree("Events", "Background Events");
    TH1F* hist = new TH1F("h_mass", "tt̅→μμ Invariant Mass;M_{#mu#mu} [GeV];Events", 100, 50, 150);
    TRandom3 rng(0);    // Random generator for smearing

    // Define variables for tree branches
    float mu1_pt, mu2_pt, mass;
    int isSignal = 0;
    tree->Branch("mu1_pt", &mu1_pt, "mu1_pt/F");
    tree->Branch("mu2_pt", &mu2_pt, "mu2_pt/F");
    tree->Branch("mass", &mass, "mass/F");
    tree->Branch("isSignal", &isSignal, "isSignal/I");

    // Counters for statistics
    int totalEvents = 0;
    int triggeredEvents = 0;
    int ptPassed = 0, isoPassed = 0, massPassed = 0;

    // Event loop
    for (int i = 0; i < 1000000; ++i) {
        if (!pythia.next()) continue;
        totalEvents++;

        std::vector<TLorentzVector> muons, pions;

        // Loop over all final state particles
        for (int j = 0; j < pythia.event.size(); ++j) {
            const auto& p = pythia.event[j];
            // Select muons
            if (p.id() == 13 || p.id() == -13) {
                TLorentzVector mu; mu.SetPxPyPzE(p.px(), p.py(), p.pz(), p.e());
                muons.push_back(mu);
            }
            // Select final-state charged pions
            if (p.idAbs() == 211 && p.isFinal()) {
                TLorentzVector pi; pi.SetPxPyPzE(p.px(), p.py(), p.pz(), p.e());
                pions.push_back(pi);
            }
        }

        // Require at least two muons
        if (muons.size() < 2) continue;
        TLorentzVector mu1 = muons[0], mu2 = muons[1];

        // Trigger cut
        if (!passesTrigger(mu1, mu2)) continue;
        triggeredEvents++;

        // Smear muon momenta
        smearMuon(mu1, rng);
        smearMuon(mu2, rng);

        // Transverse momentum cut
        if (mu1.Pt() < 30 || mu2.Pt() < 30) continue;
        ptPassed++;

        // Isolation cut
        if (!isIsolated(mu1, pions) || !isIsolated(mu2, pions)) continue;
        isoPassed++;

        // Save muon pT and dimuon invariant mass
        mu1_pt = mu1.Pt(); mu2_pt = mu2.Pt();
        mass = (mu1 + mu2).M();

        // Mass window cut
        if (mass < 40 || mass > 200) continue;
        massPassed++;

        // Fill tree and histogram
        tree->Fill();
        hist->Fill(mass);
    }

    // Write tree, histogram, and metadata to file
    tree->Write();
    hist->Write();
    double sigma_fb = pythia.info.sigmaGen() * 1e9;
    TNamed sigma("cross_section", std::to_string(sigma_fb).c_str());
    TNamed ngen("n_generated", std::to_string(totalEvents).c_str());
    sigma.Write();
    ngen.Write();
    file->Close();

    // Print statistics
    std::cout << "Total events: " << totalEvents << std::endl;
    std::cout << "Triggered events: " << triggeredEvents << std::endl;
    std::cout << "pT > 30 GeV passed: " << ptPassed << std::endl;
    std::cout << "Isolation passed: " << isoPassed << std::endl;
    std::cout << "Mass window passed: " << massPassed << std::endl;
    std::cout << "Trigger efficiency: " << (float)triggeredEvents / totalEvents << std::endl;
    std::cout << "Cross section: " << sigma_fb << " fb" << std::endl;

    return 0;
}