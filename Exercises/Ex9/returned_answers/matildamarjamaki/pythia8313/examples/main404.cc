// main404.cc is a part of the PYTHIA event generator.
// Copyright (C) 2025 Torbjorn Sjostrand.
// PYTHIA is licenced under the GNU GPL v2 or later, see COPYING for details.
// Please respect the MCnet Guidelines, see GUIDELINES for details.

// Authors: Marius Utheim
//          Peter Skands <peter.skands@monash.edu>

// Keywords: Vincia; Dire; parallelism

// This is a simple test program to compare Pythia and Vincia on
// inclusive jet rates at the LHC, for a sample with pThat > 100 GeV.
// This program is equivalent to main402, but uses the built-in parallelism
// framework to generate events in parallel, instead of running two Pythia
// instances in parallel with OpenMP.

#include "Pythia8/Pythia.h"
#include "Pythia8/PythiaParallel.h"
using namespace Pythia8;

int main() {

  // Common parameters used for both runs
  const int nEvent    = 1000;
  const int nListJets = 5;

  //************************************************************************

  // Histograms.
  Hist nJetsModel1("Model1 number of jets", 20, -0.5, 19.5);
  Hist eTjetsModel1("Model1 pT for jets", 50, 0., 500.);
  Hist yJetsModel1("Model1 y for jets", 20, -4., 4.);
  Hist phiJetsModel1("Model1 phi for jets", 25, -M_PI, M_PI);
  Hist distJetsModel1("Model1 R distance between jets", 50, 0., 10.);
  Hist nJetsModel2("Model2 number of jets", 20, -0.5, 19.5);
  Hist eTjetsModel2("Model2 pT for jets", 50, 0., 500.);
  Hist yJetsModel2("Model2 y for jets", 20, -4., 4.);
  Hist phiJetsModel2("Model2 phi for jets", 25, -M_PI, M_PI);
  Hist distJetsModel2("Model2 R distance between jets", 50, 0., 10.);
  Hist nJetsRatio("Model2/Model1 number of jets", 20, -0.5, 19.5);
  Hist eTjetsRatio("Model2/Model1 pT for jets", 50, 0., 500.);
  Hist yJetsRatio("Model2/Model1 y for jets", 20, -4., 4.);
  Hist phiJetsRatio("Model2/Model1 phi for jets", 25, -M_PI, M_PI);
  Hist distJetsRatio("Model2/Model1 R distance between jets", 50, 0., 10.);

  // Loop over generators.
  for (int iRun = 1; iRun <= 2; ++iRun) {
    PythiaParallel pythia;
    // Settings common to both runs
    pythia.readString("Beams:eCM = 7000.");
    pythia.readString("HardQCD:all = on");
    pythia.readString("PhaseSpace:pTHatMin = 100.");
    pythia.readString("PartonLevel:MPI = on");
    pythia.readString("HadronLevel:all = on");

    // Settings specific to second run
    if (iRun == 2) {
      // Switch to VINCIA shower model
      pythia.readString("PartonShowers:model = 2");
      pythia.readString("Vincia:tune = 0");
      // Output in parallel is not possible.
      pythia.readString("Print:verbosity = 0");
    }
    // Initialise generator for this run
    if(!pythia.init()) {continue;}

    // Set histogram pointers
    Hist* nJetsPtr    = &nJetsModel1;
    Hist* eTjetsPtr   = &eTjetsModel1;
    Hist* yJetsPtr    = &yJetsModel1;
    Hist* phiJetsPtr  = &phiJetsModel1;
    Hist* distJetsPtr = &distJetsModel1;
    // Switch to Model2 histograms for second
    if (iRun == 2) {
      nJetsPtr    = &nJetsModel2;
      eTjetsPtr   = &eTjetsModel2;
      yJetsPtr    = &yJetsModel2;
      phiJetsPtr  = &phiJetsModel2;
      distJetsPtr = &distJetsModel2;
    }

    // Set up SlowJet jet finder, with anti-kT clustering, R = 0.7,
    // pT > 20 GeV, |eta| < 4, and pion mass assumed for non-photons
    double etaMax   = 4.;
    double radius   = 0.7;
    double pTjetMin = 20.;
    // Exclude neutrinos (and other invisible) from study.
    int    nSel     = 2;
    SlowJet slowJet( -1, radius, pTjetMin, etaMax, nSel, 1);

    // Begin event loop.
    double sumWeights = 0.;
    int iEvent = 0;
    pythia.run(nEvent, [&](Pythia* pythiaPtr) {

      // Check for weights
      double weight = pythiaPtr->info.weight();
      sumWeights += weight;

      // Analyze Slowet jet properties. List first few.
      slowJet. analyze( pythiaPtr->event );

      iEvent += 1;
      if (iEvent < nListJets) {
        cout << " iRun = " << iRun << " iEvent = " << iEvent << endl;
        slowJet.list();
      }

      // Fill SlowJet inclusive jet distributions.
      nJetsPtr->fill( slowJet.sizeJet() , weight);
      for (int i = 0; i < slowJet.sizeJet(); ++i) {
        eTjetsPtr->fill( slowJet.pT(i) , weight);
        yJetsPtr->fill( slowJet.y(i) , weight);
        phiJetsPtr->fill( slowJet.phi(i) , weight);
      }

      // Fill SlowJet distance between jets.
      for (int i = 0; i < slowJet.sizeJet() - 1; ++i) {
        for (int j = i +1; j < slowJet.sizeJet(); ++j) {
          double dEta = slowJet.y(i) - slowJet.y(j);
          double dPhi = abs( slowJet.phi(i) - slowJet.phi(j) );
          if (dPhi > M_PI) dPhi = 2. * M_PI - dPhi;
          double dR = sqrt( pow2(dEta) + pow2(dPhi) );
          distJetsPtr->fill( dR , weight);
        }
      }
    });

    // End of event loop. Statistics. Histograms.
    pythia.stat();

  } // End loop over generators.

  // Make ratio histograms
  nJetsRatio    = nJetsModel2/nJetsModel1;
  eTjetsRatio   = eTjetsModel2/eTjetsModel1;
  yJetsRatio    = yJetsModel2/yJetsModel1;
  phiJetsRatio  = phiJetsModel2/phiJetsModel1;
  distJetsRatio = distJetsModel2/distJetsModel1;

  // Output histograms
  cout << nJetsModel1 << nJetsModel2 << nJetsRatio;
  cout << eTjetsModel1 << eTjetsModel2 << eTjetsRatio;
  cout << yJetsModel1 << yJetsModel2 << yJetsRatio;
  cout << phiJetsModel1 << phiJetsModel2 << phiJetsRatio;
  cout << distJetsModel1 << distJetsModel2 << distJetsRatio;

  // Done.
  return 0;
}
