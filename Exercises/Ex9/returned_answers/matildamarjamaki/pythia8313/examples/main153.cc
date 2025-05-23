// main153.cc is a part of the PYTHIA event generator.
// Copyright (C) 2025 Torbjorn Sjostrand.
// PYTHIA is licenced under the GNU GPL v2 or later, see COPYING for details.
// Please respect the MCnet Guidelines, see GUIDELINES for details.

// Authors: Philip Ilten <philten@cern.ch>

// Keywords: matching; madgraph; aMC@NLO

// An example where the hard process (p p -> mu+ mu-) is automatically
// produced externally with MadGraph 5, read in, and the remainder of
// the event is then produced by Pythia (MPI, showers, hadronization,
// and decays). A comparison is made between events produced with
// Pythia at LO, MadGraph 5 at LO, and aMC@NLO at NLO.

// For this example to run, MadGraph 5 must be installed and the
// command "exe" (set by default as "mg5_aMC") must be available via
// the command line. Additionally, GZIP support must be enabled via
// the "--with-gzip" configuration option(s). Note that this example has
// only been tested with MadGraph 5 version 2.3.3; due to rapid
// MadGraph development, this example may not work with other
// versions. For more details on the LHAMadgraph class see the
// comments of Pythia8Plugins/LHAMadgraph.h.

#include "Pythia8/Pythia.h"
#include "Pythia8Plugins/LHAMadgraph.h"

using namespace Pythia8;

//==========================================================================

// A simple method to run Pythia, analyze the events, and fill a histogram.

bool run(Pythia& pythia, Hist& hist, int nEvent) {
  pythia.readString("Random:setSeed = on");
  pythia.readString("Random:seed = 1");

  // If Pythia fails to initialize, exit with error.
  if (!pythia.init()) return false;

  for (int iEvent = 0; iEvent < nEvent; ++iEvent) {
    if (!pythia.next()) continue;
    int iMu1(0), iMu2(0);
    for (int i = 0; i < pythia.event.size(); ++i) {
      if (!iMu1 && pythia.event[i].id() == 13)  iMu1 = i;
      if (!iMu2 && pythia.event[i].id() == -13) iMu2 = i;
      if (iMu1 && iMu2) {
        iMu1 = pythia.event[iMu1].iBotCopyId();
        iMu2 = pythia.event[iMu2].iBotCopyId();
        hist.fill((pythia.event[iMu1].p() + pythia.event[iMu2].p()).pT());
        break;
      }
    }
  }
  pythia.stat();
  return true;
}

//==========================================================================

int main() {

  // The name of the MadGraph5_aMC@NLO executable.
  // You must prepend this string with the path to the executable
  // on your local installation, or otherwise make it available.
  string exe("mg5_aMC");

  // Create the histograms.
  Hist pyPtZ("Pythia dN/dpTZ", 100, 0., 100.);
  Hist mgPtZ("MadGraph dN/dpTZ", 100, 0., 100.);
  Hist amPtZ("aMC@NLO dN/dpTZ", 100, 0., 100.);

  // Produce leading-order events with Pythia.
  {
    Pythia pythia;
    pythia.readString("Beams:eCM = 13000.");
    pythia.readString("WeakSingleBoson:ffbar2gmZ = on");
    pythia.readString("23:onMode = off");
    pythia.readString("23:onIfMatch = -13 13");
    pythia.readString("PhaseSpace:mHatMin = 80.");
    // If initialization fails, return with error.
    if (!run(pythia, pyPtZ, 1000)) return 1;
  }

  // Produce leading-order events with MadGraph 5.
  {
    Pythia pythia;
    shared_ptr<LHAupMadgraph> madgraph = make_shared<LHAupMadgraph>(&pythia,
      true, "madgraphrun", exe);
    madgraph->readString("generate p p > mu+ mu-");
    // Note the need for a blank character before "set".
    madgraph->readString(" set ebeam1 6500");
    madgraph->readString(" set ebeam2 6500");
    madgraph->readString(" set mmll 80");
    pythia.setLHAupPtr((LHAupPtr)madgraph);
    // If initialization fails, return with error.
    if (!run(pythia, mgPtZ, 1000)) return 1;
  }

  // Produce next-to-leading-order events with aMC@NLO.
  {
    Pythia pythia;
    shared_ptr<LHAupMadgraph> amcatnlo = make_shared<LHAupMadgraph>(&pythia,
      true, "amcatnlorun", exe);
    amcatnlo->readString("generate p p > mu+ mu- [QCD]");
    // Note the need for a blank character before "set".
    amcatnlo->readString(" set ebeam1 6500");
    amcatnlo->readString(" set ebeam2 6500");
    amcatnlo->readString(" set mll 80");
    pythia.setLHAupPtr((LHAupPtr)amcatnlo);
    // If initialization fails, return with error.
    if (!run(pythia, amPtZ, 1000)) return 1;
  }

  // Print the histograms.
  cout << pyPtZ;
  cout << mgPtZ;
  cout << amPtZ;
  return 0;
}
