// main463.cc is a part of the PYTHIA event generator.
// Copyright (C) 2025 Torbjorn Sjostrand.
// PYTHIA is licenced under the GNU GPL v2 or later, see COPYING for details.
// Please respect the MCnet Guidelines, see GUIDELINES for details.

// Authors: Marius Utheim

// Keywords: low energy

// Simple illustration how to generate events at low energies.
// Outputs number of primary hadrons produced in the low energy interaction,
// as well as cos(theta) distribution of 2 -> 2 processes (if any).

#include "Pythia8/Pythia.h"
using namespace Pythia8;

//==========================================================================

int main() {

  // Initialize Pythia.
  Pythia pythia;
  pythia.readFile("main463.cmnd");

  // If Pythia fails to initialize, exit with error.
  if (!pythia.init()) return 1;

  // Histograms.
  Hist multPrim("n_primary",   9,  1., 10.);
  Hist thetaEl("cos(theta) for 2 -> 2 processes", 100, -1.,  1.);

  // Generate events and print statistics.
  int nEvent = pythia.mode("Main:numberOfEvents");
  int nSuccess = 0;
  for (int iEvent = 0; iEvent < nEvent; ++iEvent) {

    if (!pythia.next())
      continue;

    Event& ev = pythia.event;
    nSuccess += 1;

    // Fill number of primary hadrons.
    int nPrimary = 0;
    for (auto& particle : ev)
      if (particle.isHadron() && particle.statusAbs() / 10 == 15)
        nPrimary += 1;
    multPrim.fill(nPrimary);

    // Fill theta if scattering is elastic.
    if (ev.nFinal() == 2) {
      int in1 = ev[1].id(), in2 = ev[2].id();
      int out1 = ev[ev.size() - 2].id(), out2 = ev[ev.size() - 1].id();

      if ( (in1 == out1 && in2 == out2) || (in1 == out2 && in2 == out1) )
        thetaEl.fill(cos(ev[ev.size() - 1].theta()));
    }
  }

  // Plot histograms and statistics.
  multPrim /= nSuccess;
  cout << multPrim;

  if (thetaEl.getEntries() > 0) {
    thetaEl *= 50. / thetaEl.getEntries();
    cout << thetaEl;
  }

  pythia.stat();

  // Done.
  return 0;
}
