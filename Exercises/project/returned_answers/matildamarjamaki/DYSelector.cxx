
#define DYSelector_cxx
#include "DYSelector.h"
#include <TFile.h>
#include <TCanvas.h>

void DYSelector::Begin(TTree * /*tree*/) {
   TString option = GetOption();
   h_pileup = new TH1F("h_pileup", "Pileup distribution;Pileup;Events", 60, 0, 60);
}

Bool_t DYSelector::Process(Long64_t entry) {
   fChain->GetEntry(entry);

   if (!HLT_IsoMu24) return kTRUE; // Trigger condition
   h_pileup->Fill(Pileup_nTrueInt); // Fill histogram

   return kTRUE;
}

void DYSelector::Terminate() {
   TFile *f = new TFile("dy_pileup.root", "RECREATE");
   h_pileup->Write();
   f->Close();

   TCanvas *c = new TCanvas("c", "Pileup", 800, 600);
   h_pileup->Draw("HIST");
   c->SaveAs("dy_pileup_distribution.pdf");
}