
#ifndef DYSelector_h
#define DYSelector_h

#include <TSelector.h>
#include <TTree.h>
#include <TH1F.h>

class DYSelector : public TSelector {
public:
   TTree *fChain = nullptr;

   // Histogram
   TH1F *h_pileup;

   // Branches
   Bool_t HLT_IsoMu24;
   Float_t Pileup_nTrueInt;

   TBranch *b_HLT_IsoMu24 = nullptr;
   TBranch *b_Pileup_nTrueInt = nullptr;

   DYSelector() {}
   virtual ~DYSelector() {}

   virtual Int_t Version() const { return 2; }
   virtual void Begin(TTree *tree);
   virtual void SlaveBegin(TTree *tree) {}
   virtual Bool_t Process(Long64_t entry);
   virtual void SlaveTerminate() {}
   virtual void Terminate();

   virtual void Init(TTree *tree) {
      fChain = tree;
      fChain->SetBranchAddress("HLT_IsoMu24", &HLT_IsoMu24, &b_HLT_IsoMu24);
      fChain->SetBranchAddress("Pileup_nTrueInt", &Pileup_nTrueInt, &b_Pileup_nTrueInt);
   }

   ClassDef(DYSelector, 0);
};

#endif