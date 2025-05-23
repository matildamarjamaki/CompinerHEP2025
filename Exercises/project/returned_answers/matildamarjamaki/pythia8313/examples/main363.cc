// main363.cc is a part of the PYTHIA event generator.
// Copyright (C) 2025 Torbjorn Sjostrand.
// PYTHIA is licenced under the GNU GPL v2 or later, see COPYING for details.
// Please respect the MCnet Guidelines, see GUIDELINES for details.

// Authors: Philip Ilten <philten@cern.ch>

// Keywords: onia; helaconia

// An example where the quarkonia hard process (p p -> J/psi g) is
// automatically produced externally with HelacOnia, read in, and the
// remainder of the event is then produced by Pythia (MPI, showers,
// and hadronization). A comparison is made between events produced
// with Pythia at LO and HelacOnia at LO.

// For this example to run, HelacOnia must be installed and the
// command "exe" (set by default as "ho_cluster") must be available
// via the command line. Note that this example has only been tested
// with HelacOnia version 2.2.4; due to rapid HelacOnia development,
// this example may not work with other versions. For more details on
// the LHAHelacOnia class see the comments of
// Pythia8Plugins/LHAHelacOnia.h.

#include "Pythia8/Pythia.h"
#include "Pythia8Plugins/LHAHelaconia.h"

using namespace Pythia8;

//==========================================================================

// A simple method to run Pythia, analyze the events, and fill a histogram.

bool run(Pythia& pythia, Hist& hist, int nEvent) {
  pythia.readString("Random:setSeed = on");
  pythia.readString("Random:seed = 1");
  if (!pythia.init()) return false;
  for (int iEvent = 0; iEvent < nEvent; ++iEvent) {
    if (!pythia.next()) continue;
    for (int i = 0; i < pythia.event.size(); ++i) {
      if (pythia.event[i].id() != 443) continue;
      hist.fill(pythia.event[pythia.event[i].iBotCopyId()].pT());
      break;
    }
  }
  pythia.stat();
  return true;
}

//==========================================================================

int main() {

  // The name of the HelacOnia executable.
  // You must prepend this string with the path to the executable
  // on your local installation, or otherwise make it available.
  string exe("ho_cluster");

  // Create the histograms.
  Hist pyPtPsi("Pythia dN/dpT J/psi", 100, 0., 20.);
  Hist hoPtPsi("HelacOnia dN/dpT J/psi", 100, 0., 20.);

  // Produce leading-order events with Pythia.
  {
    Pythia pythia;
    pythia.readString("Beams:eCM = 13000.");
    pythia.readString("Charmonium:gg2ccbar(3S1)[3S1(1)]g = on,off");
    pythia.readString("PhaseSpace:pTHatMin = 2");
    // If we fail to initialize, abort.
    if (!run(pythia, pyPtPsi, 1000)) return 1;
  }

  // Produce leading-order events with HelacOnia.
  {
    Pythia pythia;
    shared_ptr<LHAupHelaconia> helaconiaPtr;
    helaconiaPtr = make_shared<LHAupHelaconia>(&pythia, "helaconiarun", exe);
    helaconiaPtr->readString("generate g g > cc~(3S11) g");
    helaconiaPtr->readString("set energy_beam1 = 6500");
    helaconiaPtr->readString("set energy_beam2 = 6500");
    helaconiaPtr->readString("set minptconia = 2");
    pythia.setLHAupPtr((LHAupPtr)helaconiaPtr);
    // If we fail to initialize, abort.
    if(!run(pythia, hoPtPsi, 1000)) return 1;
  }

  // Print the histograms.
  cout << pyPtPsi;
  cout << hoPtPsi;
  return 0;
}
