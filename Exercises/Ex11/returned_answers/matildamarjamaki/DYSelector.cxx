#define DYSelector_cxx
// The class definition in DYSelector.h has been generated automatically
// by the ROOT utility TTree::MakeSelector(). This class is derived
// from the ROOT class TSelector. For more information on the TSelector
// framework see $ROOTSYS/README/README.SELECTOR or the ROOT User Manual.


// The following methods are defined in this file:
//    Begin():        called every time a loop on the tree starts,
//                    a convenient place to create your histograms.
//    SlaveBegin():   called after Begin(), when on PROOF called only on the
//                    slave servers.
//    Process():      called for each event, in this function you decide what
//                    to read and fill your histograms.
//    SlaveTerminate: called at the end of the loop on the tree, when on PROOF
//                    called only on the slave servers.
//    Terminate():    called at the end of the loop on the tree,
//                    a convenient place to draw/fit your histograms.
//
// To use this file, try the following session on your Tree T:
//
// root> T->Process("DYSelector.C")
// root> T->Process("DYSelector.C","some options")
// root> T->Process("DYSelector.C+")
//


#include "DYSelector.h"
#include <TH2.h>
#include <TStyle.h>
#include <TCanvas.h>
#include <TMath.h>

void DYSelector::Begin(TTree * /*tree*/)
{
   // The Begin() function is called at the start of the query.
   // When running with PROOF Begin() is only called on the client.
   // The tree argument is deprecated (on PROOF 0 is passed).

   TString option = GetOption();
   h_pileup = new TH1F("h_pileup", "Pileup Distribution;Number of Pileup Interactions;Events", 60, 0, 60);
}

void DYSelector::SlaveBegin(TTree * /*tree*/)
{
   // The SlaveBegin() function is called after the Begin() function.
   // When running with PROOF SlaveBegin() is called on each slave server.
   // The tree argument is deprecated (on PROOF 0 is passed).

   TString option = GetOption();
   fOutput->Add(h_pileup);
}

bool DYSelector::Process(Long64_t entry)
{
   // The Process() function is called for each entry in the tree (or possibly
   // keyed object in the case of PROOF) to be processed. The entry argument
   // specifies which entry in the currently loaded tree is to be processed.
   // When processing keyed objects with PROOF, the object is already loaded
   // and is available via the fObject pointer.
   //
   // This function should contain the \"body\" of the analysis. It can contain
   // simple or elaborate selection criteria, run algorithms on the data
   // of the event and typically fill histograms.
   //
   // The processing can be stopped by calling Abort().
   //
   // Use fStatus to set the return value of TTree::Process().
   //
   // The return value is currently not used.

   fReader.SetLocalEntry(entry);

   // Retrieve trigger flag and pileup data for the current entry
   bool HLT_IsoMu24_val = *HLT_IsoMu24;  // Trigger flag
   int nPU_val = *PV_npvs;  // Number of pileup interactions (primary vertices)

   // Select events based on the HLT_IsoMu24 trigger
   if (!HLT_IsoMu24_val) return kFALSE;  // Skip events that don't pass the trigger

   // Fill the pileup histogram with the number of pileup interactions
   h_pileup->Fill(nPU_val);

   return kTRUE;

   //return true;
}

void DYSelector::SlaveTerminate()
{
   // The SlaveTerminate() function is called after all entries or objects
   // have been processed. When running with PROOF SlaveTerminate() is called
   // on each slave server.

}

void DYSelector::Terminate()
{
   // The Terminate() function is the last function to be called during
   // a query. It always runs on the client, it can be used to present
   // the results graphically or save the results to file.

   TCanvas* c1 = new TCanvas("c1", "Pileup Distribution", 800, 600);
   h_pileup->Draw();
   c1->SaveAs("pileup_distribution.png");

}