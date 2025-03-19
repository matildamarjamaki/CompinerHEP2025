
#include "Pythia8/Pythia.h"
#include "TH1F.h"
#include "TCanvas.h"
#include "TFile.h"
#include "TROOT.h"
#include "TF1.h"

using namespace Pythia8;

int main() {
    Pythia pythia;
    
    pythia.readString("PDF:pSet = 8");

    pythia.readString("HiggsSM:gg2H = on");
    
    pythia.readString("25:m0 = 125.0");

    pythia.init();

    TH1F *hHiggsMass = new TH1F("hHiggsMass", "Higgs Mass Distribution; Mass (GeV); Events", 50, 110, 140);
    
    int nEvents = 1000;
    for (int i = 0; i < nEvents; i++) {
        if (!pythia.next()) continue;
        
        for (int j = 0; j < pythia.event.size(); j++) {
            if (pythia.event[j].id() == 25) { 
                hHiggsMass->Fill(pythia.event[j].m());
            }
        }
    }

    TFile outFile("HiggsMass.root", "RECREATE");
    hHiggsMass->Write();
    outFile.Close();

    TCanvas *c1 = new TCanvas("c1", "Higgs Mass", 800, 600);
    hHiggsMass->Draw();

    TF1 *bwFit = new TF1("bwFit", "[0]*TMath::BreitWigner(x, [1], [2])", 110, 140);
    bwFit->SetParameters(100, 125, 4.0);
    hHiggsMass->Fit("bwFit");

    double fittedWidth = bwFit->GetParameter(2);
    printf("Fitted Higgs Width: %.3f GeV\n", fittedWidth);

    c1->SaveAs("HiggsMass.png");

    return 0;
}