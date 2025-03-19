
#include "Pythia8/Pythia.h"
#include "TH1F.h"
#include "TCanvas.h"
#include "TFile.h"
#include "TROOT.h"
#include "TF1.h"

using namespace Pythia8;

int main() {
    // Initialize PYTHIA
    Pythia pythia;
    
    // Use CTEQ6L1 PDF set
    pythia.readString("PDF:pSet = 8");

    // Enable Higgs production (gg → H)
    pythia.readString("HiggsSM:gg2H = on");
    
    // Set Higgs mass to 125 GeV
    pythia.readString("25:m0 = 125.0");

    // Initialize generator
    pythia.init();

    // Create ROOT histogram
    TH1F *hHiggsMass = new TH1F("hHiggsMass", "Higgs Mass Distribution; Mass (GeV); Events", 50, 110, 140);
    
    // Event loop
    int nEvents = 1000;
    for (int i = 0; i < nEvents; i++) {
        if (!pythia.next()) continue;
        
        // Loop over final state particles
        for (int j = 0; j < pythia.event.size(); j++) {
            if (pythia.event[j].id() == 25) { // PDG ID for Higgs
                hHiggsMass->Fill(pythia.event[j].m());
            }
        }
    }

    // Open ROOT file to save histogram
    TFile outFile("HiggsMass.root", "RECREATE");
    hHiggsMass->Write();
    outFile.Close();

    // Draw histogram
    TCanvas *c1 = new TCanvas("c1", "Higgs Mass", 800, 600);
    hHiggsMass->Draw();

    // Fit histogram with Breit-Wigner function
    TF1 *bwFit = new TF1("bwFit", "[0]*TMath::BreitWigner(x, [1], [2])", 110, 140);
    bwFit->SetParameters(100, 125, 4.0);  // Initial guesses: normalization, mean, width
    hHiggsMass->Fit("bwFit");

    // Show fit parameters
    double fittedWidth = bwFit->GetParameter(2);
    printf("Fitted Higgs Width: %.3f GeV\n", fittedWidth);

    // Save plot
    c1->SaveAs("HiggsMass.png");

    return 0;
}