// main321.cc is a part of the PYTHIA event generator.
// Copyright (C) 2025 Torbjorn Sjostrand.
// PYTHIA is licenced under the GNU GPL v2 or later, see COPYING for details.
// Please respect the MCnet Guidelines, see GUIDELINES for details.

// Keywords: basic usage; command file; cross sections; minimum bias;
//           diffraction

// This is a simple test program.
// It illustrates how to generate and study "total cross section" processes,
// i.e. elastic, single and double diffractive, and the "minimum-bias" rest.
// All input is specified in the default main321.cmnd file.
// Optionally the main321photons.cmnd can be used as input (or your own code).
// Note that the "total" cross section does NOT include
// the Coulomb contribution to elastic scattering, as switched on here.

#include "Pythia8/Pythia.h"
#include "Pythia8Plugins/InputParser.h"

using namespace Pythia8;

//==========================================================================

int main(int argc, char* argv[]) {

  // Set up command line options.
  InputParser ip("Filters a resonance decay.",
    {"./main321", "./main321 -c main321.cmnd"});
  ip.add("c", "main321.cmnd", "Use this user-written command file.",
    {"-cmnd"});

  // Initialize the parser and exit if necessary.
  InputParser::Status status = ip.init(argc, argv);
  if (status != InputParser::Valid) return status;

  // Generator. Shorthand for the event.
  Pythia pythia;
  Event& event = pythia.event;
  pythia.readFile(ip.get<string>("c"));

  // Extract settings to be used in the main program.
  int    nEvent    = pythia.mode("Main:numberOfEvents");
  int    nAbort    = pythia.mode("Main:timesAllowErrors");

  // If Pythia fails to initialize, exit with error.
  if (!pythia.init()) return 1;

  // Book histograms: multiplicities and mean transverse momenta.
  double nMax = 799.5;
  Hist yChg("rapidity of charged particles; all",      100, -10., 10.);
  Hist nChg("number of charged particles; all",        100, -0.5, nMax);
  Hist nChgSD("number of charged particles; single diffraction",
                                                       100, -0.5, nMax);
  Hist nChgDD("number of charged particles, double diffractive",
                                                       100, -0.5, nMax);
  Hist nChgCD("number of charged particles, central diffractive",
                                                       100, -0.5, nMax);
  Hist nChgND("number of charged particles, non-diffractive",
                                                       100, -0.5, nMax);
  Hist pTnChg("<pt>(n_charged) all",                   100, -0.5, nMax);
  Hist pTnChgSD("<pt>(n_charged) single diffraction",  100, -0.5, nMax);
  Hist pTnChgDD("<pt>(n_charged) double diffraction",  100, -0.5, nMax);
  Hist pTnChgCD("<pt>(n_charged) central diffraction", 100, -0.5, nMax);
  Hist pTnChgND("<pt>(n_charged) non-diffractive   ",  100, -0.5, nMax);

  // Book histograms: ditto as function of separate subsystem mass.
  Hist mLogInel("log10(mass), by diffractive system",  100, 0., 5.);
  Hist nChgmLog("<n_charged>(log10(mass))",            100, 0., 5.);
  Hist pTmLog("<pT>_charged>(log10(mass))",            100, 0., 5.);

  // Book histograms: elastic/diffractive.
  Hist tSpecEl("elastic |t| spectrum",              100, 0., 1.);
  Hist tSpecElLog("elastic log10(|t|) spectrum",    100, -5., 0.);
  Hist tSpecSD("single diffractive |t| spectrum",   100, 0., 2.);
  Hist tSpecDD("double diffractive |t| spectrum",   100, 0., 5.);
  Hist tSpecCD("central diffractive |t| spectrum",  100, 0., 5.);
  Hist mSpec("diffractive mass spectrum",           100, 0., 100.);
  Hist mLogSpec("log10(diffractive mass spectrum)", 100, 0., 4.);

  // Book histograms: inelastic nondiffractive.
  double pTmax = 20.;
  double bMax  = 4.;
  Hist pTspec("total pT_hard spectrum",             100, 0., pTmax);
  Hist pTspecND("nondiffractive pT_hard spectrum",  100, 0., pTmax);
  Hist bSpec("b impact parameter spectrum",         100, 0., bMax);
  Hist enhanceSpec("b enhancement spectrum",        100, 0., 10.);
  Hist number("number of interactions",             100, -0.5, 99.5);
  Hist pTb1("pT spectrum for b < 0.5",               50, 0., pTmax);
  Hist pTb2("pT spectrum for 0.5 < b < 1",           50, 0., pTmax);
  Hist pTb3("pT spectrum for 1 < b < 1.5",           50, 0., pTmax);
  Hist pTb4("pT spectrum for 1.5 < b",               50, 0., pTmax);
  Hist bpT1("b spectrum for pT < 2",                 50, 0., bMax);
  Hist bpT2("b spectrum for 2 < pT < 5",             50, 0., bMax);
  Hist bpT3("b spectrum for 5 < pT < 15",            50, 0., bMax);
  Hist bpT4("b spectrum for 15 < pT",                50, 0., bMax);

  // Begin event loop.
  int iAbort = 0;
  for (int iEvent = 0; iEvent < nEvent; ++iEvent) {

    // Generate events. Quit if too many failures.
    if (!pythia.next()) {
      if (++iAbort < nAbort) continue;
      cout << " Event generation aborted prematurely, owing to error!\n";
      break;
    }

    // Extract event classification.
    int code = pythia.info.code();

    // Charged multiplicity and mean pT: all and by event class.
    int nch = 0;
    double pTsum = 0.;
    for (int i = 1; i < event.size(); ++i)
    if (event[i].isFinal() && event[i].isCharged()) {
      yChg.fill( event[i].y() );
      ++nch;
      pTsum += event[i].pT();
    }
    nChg.fill( nch );
    if (nch > 0) pTnChg.fill( nch, pTsum/nch);
    if (code == 103 || code == 104) {
      nChgSD.fill( nch );
      if (nch > 0) pTnChgSD.fill( nch, pTsum/nch);
    } else if (code == 105) {
      nChgDD.fill( nch );
      if (nch > 0) pTnChgDD.fill( nch, pTsum/nch);
    } else if (code == 106) {
      nChgCD.fill( nch );
      if (nch > 0) pTnChgCD.fill( nch, pTsum/nch);
    } else if (code == 101) {
      nChgND.fill( nch );
      if (nch > 0) pTnChgND.fill( nch, pTsum/nch);
      double mLog = log10( event[0].m() );
      mLogInel.fill( mLog );
      nChgmLog.fill( mLog, nch );
      if (nch > 0) pTmLog.fill( mLog, pTsum / nch );
    }

    // Charged multiplicity and mean pT: per diffractive system.
    for (int iDiff = 0; iDiff < 3; ++iDiff)
    if ( (iDiff == 0 && pythia.info.isDiffractiveA())
      || (iDiff == 1 && pythia.info.isDiffractiveB())
      || (iDiff == 2 && pythia.info.isDiffractiveC()) ) {
      int ndiff = 0;
      double pTdiff = 0.;
      int nDoc = (iDiff < 2) ? 4 : 5;
      for (int i = nDoc + 1; i < event.size(); ++i)
      if (event[i].isFinal() && event[i].isCharged()) {
        // Trace back final particle to see which system it comes from.
        int k = i;
        do k = event[k].mother1();
        while (k > nDoc);
        if (k == iDiff + 3) {
          ++ndiff;
          pTdiff += event[i].pT();
        }
      }
      // Study diffractive mass spectrum.
      double mDiff = event[iDiff+3].m();
      double mLog  = log10( mDiff);
      mLogInel.fill( mLog );
      nChgmLog.fill( mLog, ndiff );
      if (ndiff > 0) pTmLog.fill( mLog, pTdiff / ndiff );
      mSpec.fill( mDiff );
      mLogSpec.fill( mLog );
    }

    // Study pT spectrum of all hard collisions, no distinction.
    double pT = pythia.info.pTHat();
    pTspec.fill( pT );

    // Study t distribution of elastic/diffractive events.
    if (code > 101) {
      double tAbs = abs(pythia.info.tHat());
      if (code == 102) {
        tSpecEl.fill(tAbs);
        tSpecElLog.fill(log10(tAbs));
      }
      else if (code == 103 || code == 104) tSpecSD.fill(tAbs);
      else if (code == 105) tSpecDD.fill(tAbs);
      else if (code == 106) {
        double t1Abs = abs( (event[3].p() - event[1].p()).m2Calc() );
        double t2Abs = abs( (event[4].p() - event[2].p()).m2Calc() );
        tSpecCD.fill(t1Abs);
        tSpecCD.fill(t2Abs);
      }

    // Study nondiffractive inelastic events in (pT, b) space.
    } else {
      double b = pythia.info.bMPI();
      double enhance = pythia.info.enhanceMPI();
      int nMPI = pythia.info.nMPI();
      pTspecND.fill( pT );
      bSpec.fill( b );
      enhanceSpec.fill( enhance );
      number.fill( nMPI );
      if (b < 0.5) pTb1.fill( pT );
      else if (b < 1.0) pTb2.fill( pT );
      else if (b < 1.5) pTb3.fill( pT );
      else pTb4.fill( pT );
      if (pT < 2.) bpT1.fill( b );
      else if (pT < 5.) bpT2.fill( b );
      else if (pT < 15.) bpT3.fill( b );
      else bpT4.fill( b );
    }

  // End of event loop.
  }

  // Final statistics and histograms.
  pythia.stat();
  pTnChg   /= nChg;
  pTnChgSD /= nChgSD;
  pTnChgDD /= nChgDD;
  pTnChgCD /= nChgCD;
  pTnChgND /= nChgND;
  nChgmLog /= mLogInel;
  pTmLog   /= mLogInel;
  cout << yChg << nChg << nChgSD << nChgDD << nChgCD << nChgND
       << pTnChg << pTnChgSD << pTnChgDD << pTnChgCD << pTnChgND
       << mLogInel << nChgmLog << pTmLog
       << tSpecEl << tSpecElLog << tSpecSD << tSpecDD << tSpecCD
       << mSpec << mLogSpec
       << pTspec << pTspecND << bSpec << enhanceSpec << number
       << pTb1 << pTb2 << pTb3 << pTb4 << bpT1 << bpT2 << bpT3 << bpT4;

  // Present some of the plots in a PDF file.
  HistPlot plt("plot321");
  plt.frame("fig321",
    "Hardest MPI $p_{\\perp}$ dependence on impact parameter",
    "$p_{\\perp}$", "Rate", 6.4, 4.8);
  plt.add( pTb1, "h,black", "$b < 0.5$");
  plt.add( pTb2, "h,green", "$0.5 < b < 1$");
  plt.add( pTb3, "h,red", "$1 < b < 1.5$");
  plt.add( pTb4, "h,blue", "$b > 1.5$");
  plt.plot();
  plt.frame("fig321",
    "Impact parameter dependence on hardest MPI $p_{\\perp}$",
    "$b$", "Rate", 6.4, 4.8);
  plt.add( bpT1, "h,black", "$p_{\\perp} < 2$");
  plt.add( bpT2, "h,green", "$2 < p_{\\perp} < 5$");
  plt.add( bpT3, "h,red", "$5 < p_{\\perp} < 15$");
  plt.add( bpT4, "h,blue", "$p_{\\perp} > 15$");
  plt.plot();

  // Done.
  return 0;
}
