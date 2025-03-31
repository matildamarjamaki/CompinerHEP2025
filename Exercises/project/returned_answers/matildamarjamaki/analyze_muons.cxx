
#include "TFile.h"
#include "TTree.h"
#include "TRandom3.h"
#include "TLorentzVector.h"
#include <vector>
#include <iostream>

void analyze_muons() {
    TFile *file = new TFile("events_Zmm.root", "READ");
    TTree *tree = (TTree*)file->Get("Events");

    float muon1_pT, muon2_pT, muon1_eta, muon2_eta;
    tree->SetBranchAddress("muon1_pT", &muon1_pT);
    tree->SetBranchAddress("muon2_pT", &muon2_pT);
    tree->SetBranchAddress("muon1_eta", &muon1_eta);
    tree->SetBranchAddress("muon2_eta", &muon2_eta);
    
    TRandom3 randGen;
    int passedEvents = 0;
    int totalEvents = tree->GetEntries();
    
    for (int i = 0; i < totalEvents; i++) {
        tree->GetEntry(i);
        
        // Apply Gaussian smearing
        float muon1_pT_smeared = muon1_pT * (1.0 + randGen.Gaus(0, 0.01));
        float muon2_pT_smeared = muon2_pT * (1.0 + randGen.Gaus(0, 0.01));
        
        // Selection of signal muons
        if (muon1_pT_smeared < 30 || muon2_pT_smeared < 30) continue;
        
        // Track isolation (dummy example, replace with real charged pion sum)
        float pionSum_pT = 0.0; // Compute sum of pion pT within DeltaR < 0.3
        if (pionSum_pT > 1.5) continue;
        
        passedEvents++;
    }
    
    std::cout << "Total events: " << totalEvents << "\n";
    std::cout << "Passed events: " << passedEvents << "\n";
}