// main224.cc is a part of the PYTHIA event generator.
// Copyright (C) 2025 Torbjorn Sjostrand.
// PYTHIA is licenced under the GNU GPL v2 or later, see COPYING for details.
// Please respect the MCnet Guidelines, see GUIDELINES for details.

// Authors: Nadine Fischer

// Keywords: Dire; Vincia; hepmc; OpenMP; command file; command line option

// The following functions analyze a scattering event and save the event in
// an output format that can be converted into a postscript figure using the
// "graphviz" program.

// Pythia includes.
#include "Pythia8/Pythia.h"
#include "Pythia8Plugins/InputParser.h"
#ifdef HEPMC3
#include "Pythia8Plugins/HepMC3.h"
#endif

// OpenMP includes.
#ifdef OPENMP
#include <omp.h>
#endif

// Include graphviz visualisation plugin.
#include "Pythia8Plugins/Visualisation.h"

using namespace Pythia8;

//==========================================================================

int main( int argc, char* argv[] ) {

  // Set up command line options.
  InputParser ip("Visualize an event with graphivz.",
    {"./main224 --nevents 50 --setting \"WeakSingleBoson:ffbar2gmZ=on\"",
        "./main224 --input main224.cmnd --hepmc_output myfile.hepmc"});
  ip.add("v", "false", "Saves an event for visialization.",
    {"-visualize_event"});
  ip.add("n", "-1", "Number of events to generate.",
    {"-nevents"});
#ifdef OPENMP
  ip.add("j", "1", "Number of threads to use if OpenMP enabled.",
    {"-nthreads"});
#endif
  ip.add("c", "", "Input files for settings, can use multiple times.",
    {"-input"});
  ip.add("m", "", "Store generated events in HepMC file.",
    {"-hepmc_output"});
  ip.add("l", "", "Store generated events in LHEF file.",
    {"-lhef_output"});
  ip.add("s", "", "Settings to be read by Pythia, can use multiple times.",
    {"-setting"});

  // Initialize the parser and exit if necessary.
  InputParser::Status status = ip.init(argc, argv);
  if (status != InputParser::Valid) return status;

  string input             = ip.get<string>("c");
  string hepmc_output      = ip.get<string>("m");
  string lhef_output       = ip.get<string>("l");
  int nevents              = ip.get<int>("n");
#ifdef OPENMP
  int nThreads             = ip.get<int>("j");;
#endif
  bool visualize_event     = ip.get<bool>("v");
  string visualize_output  = (input == "") ? "event" : "event-" + input;
  replace(visualize_output.begin(), visualize_output.end(), '/', '-');

  vector<Pythia*> pythiaPtr;

  // Read input files.
  vector<string> input_file = ip.getVector<string>("c");
  int countInput(input_file.size());
  if (input_file.size() < 1) input_file.push_back("");

  // For several settings files as input, check that they use
  // a different process.
  if (input_file.size() > 1) {
    bool sameProcess = false;
    for (int i = 1; i < int(input_file.size()); ++i)
      if (input_file[i] == input_file[i-1]) sameProcess = true;
    if (sameProcess)
      cout << "WARNING: several input files with the same name\n"
        << " found; this will lead to a wrong cross section!\n";
  }

  std::streambuf *old = cout.rdbuf();
  stringstream ss;
  for (int i = 0; i < int(input_file.size()); ++i) {
    ss.str("");
    // Redirect output so that Pythia banner will not be printed twice.
    if(i>0) cout.rdbuf (ss.rdbuf());
    pythiaPtr.push_back( new Pythia());
    // Restore print-out.
    cout.rdbuf (old);
  }

#ifdef OPENMP
  old = cout.rdbuf();
  int nThreadsMax = omp_get_max_threads();
  int nThreadsReq = min(nThreads,nThreadsMax);
  // Divide a single Pythia object into several, in case of multiple threads.
  int nPythiaOrg = (int)pythiaPtr.size();

  if (nThreadsReq > 1 && (nPythiaOrg == 1 || nThreadsReq%nPythiaOrg ==0)) {
    int niterations(0);
    for (int i = nPythiaOrg; i < nThreadsReq; ++i) {
      if (i%nPythiaOrg == 0) niterations++;
      int ifile    = i-niterations*nPythiaOrg;
      string sfile = input_file[ifile];
      input_file.push_back(sfile);
      ss.str("");
      // Redirect output so that Pythia banner will not be printed twice.
      cout.rdbuf (ss.rdbuf());
      pythiaPtr.push_back( new Pythia());
    }
  }

  // Restore print-out.
  cout.rdbuf (old);

  bool doParallel = true;
  int  nPythiaAct = (int)pythiaPtr.size();
  // The number of Pythia objects exceeds the number of available threads.
  if (nPythiaAct > nThreadsReq) {
    cout << "WARNING: The number of requested Pythia instances exceeds the\n"
      << " number of available threads! No parallelization will be done!\n";
    nThreadsReq = 1;
    doParallel  = false;
  } else if (nPythiaAct == 1) {
    cout << "WARNING: The number of requested Pythia instances still one!\n"
      << " No parallelization will be done!\n";
    nThreadsReq = 1;
    doParallel  = false;
  } else if (nPythiaAct < nThreadsReq) {
    cout << "WARNING: The number of requested Pythia instances too small!\n"
      << " Reset to minimal parallelization!\n";
    nThreadsReq = nPythiaAct;
  }

  // Only print with first Pythia instance to avoid output mangling.
  if (doParallel) for (int j = 1; j < int(pythiaPtr.size()); ++j)
    pythiaPtr[j]->readString("Print:quiet = on");

#endif

  // Force PhaseSpace:pTHatMinDiverge to something very small to not bias DIS.
  for (int i = 0; i < int(pythiaPtr.size()); ++i)
    pythiaPtr[i]->settings.forceParm("PhaseSpace:pTHatMinDiverge",5e-2);
  // Print warning that the parameter has been reset, some general guidelines,
  // and then wait for a few seconds, so that users can read the comments.
  cout
    << "\nINFORMATION:\n"
    << "   Forcing PhaseSpace:pTHatMinDiverge to very small\n"
    << "   value 0.05 GeV to prevent aggressive phase-space\n"
    << "   restriction of deep-inelastic scattering configurations.\n"
    << "   Default minimum value of 0.5 GeV may be reinstated from command\n"
    << "   line or input file settings.\n"
    << "\nNOTE THAT\n"
    << "   2->2 processes in pp collisions require setting "
    <<     "PhaseSpace:pTHatMin\n"
    << "   2->2 processes in DIS collisions require setting "
    <<     "PhaseSpace:Q2min\n"
    << "   (see details in the ''Phase Space Cuts'' section of the online "
    <<     "manual)" << endl;

  // Read command line settings.
  vector<string> settings = ip.getVector<string>("s");
  int countSettings(0);
  for (int i = 0; i < (int)settings.size(); ++i) {
    string setting = settings[i];
    replace(setting.begin(), setting.end(), '"', ' ');

    // Skip Dire settings at this stage.
    if (setting.find("Dire") != string::npos) continue;
    if (setting.find("Enhance") != string::npos) continue;

    for (int j = 0; j < int(pythiaPtr.size()); ++j) {
      pythiaPtr[j]->readString(setting);
      countSettings++;
    }
  }

  // Read command line settings again and overwrite file settings.
  for (int i = 0; i < (int)settings.size(); ++i) {
    string setting = settings[i];
    replace(setting.begin(), setting.end(), '"', ' ');
    for (int j = 0; j < int(pythiaPtr.size()); ++j)
      pythiaPtr[j]->readString(setting);
  }

  if( countInput == 0 && countSettings == 0 ) {
    cout << "Error:  no runtime parameters have been passed to Pythia" ;
    cout << " using --input or --setting.  Run with --help for options."
         << endl;
    return 1;
  }

  for (int i = 0; i < int(pythiaPtr.size()); ++i)
    if (i < int(input_file.size()) && input_file[i] != "")
      pythiaPtr[i]->readFile(input_file[i].c_str());

  // Two classes to do the two PDFs externally. Hand pointers to Pythia.
  PDFPtr pdfAPtr = nullptr;
  PDFPtr pdfBPtr = nullptr;

  // Allow Pythia to use Dire merging classes.
  for (int i = 0; i < int(pythiaPtr.size()); ++i) {
    if (pdfAPtr != nullptr) pythiaPtr[i]->setPDFAPtr(pdfAPtr);
    if (pdfBPtr != nullptr) pythiaPtr[i]->setPDFBPtr(pdfBPtr);
  }

  for (int i = 0; i < int(pythiaPtr.size()); ++i) {
    if (i < int(input_file.size()) && input_file[i] != "")
      pythiaPtr[i]->readFile(input_file[i].c_str());
    if (!pythiaPtr[i]->init()) return 1;
  }

#ifdef OPENMP
  // A divided single Pythia run does not work with Les Houches events.
  if (nThreadsReq > 1 && nPythiaOrg == 1 &&
    pythiaPtr.front()->mode("Beams:frameType") > 3) {
    cout << "WARNING: can not divide the run into subruns as the\n"
      << " same hard events from the Les Houches file would be\n"
      << " used multiple times!\n";
    // Clean-up.
    nThreadsReq = 1;
    for (int i = 1; i < int(pythiaPtr.size()); ++i) {
      delete pythiaPtr[i]; pythiaPtr[i]=0;
    }
  }
  // Add random seeds for parallelization of a single Pythia run.
  bool splitSingleRun = false;
  if (nThreadsReq > 1 && nPythiaOrg == 1) {
    splitSingleRun = true;
    int randomOffset = 100;
    for (int j = 0; j < int(pythiaPtr.size()); ++j) {
      pythiaPtr[j]->readString("Random:setSeed = on");
      pythiaPtr[j]->readString("Random:seed = " +
                               std::to_string(randomOffset + j));
    }
  }
#endif

  int nEventEst = pythiaPtr.front()->settings.mode("Main:numberOfEvents");
  if (nevents > 0) nEventEst = nevents;

  int nEventEstEach = nEventEst;
#ifdef OPENMP
  // Number of events per thread.
  if (nThreadsReq > 1) {
    while (nEventEst%nThreadsReq != 0) nEventEst++;
    nEventEstEach = nEventEst/nThreadsReq;
  }
#endif

  // Switch off all showering and MPI when estimating the cross section,
  // and re-initialise (unfortunately).
  bool fsr = pythiaPtr.front()->flag("PartonLevel:FSR");
  bool isr = pythiaPtr.front()->flag("PartonLevel:ISR");
  bool mpi = pythiaPtr.front()->flag("PartonLevel:MPI");
  bool had = pythiaPtr.front()->flag("HadronLevel:all");
  bool rem = pythiaPtr.front()->flag("PartonLevel:Remnants");
  bool chk = pythiaPtr.front()->flag("Check:Event");
  vector<bool> merge;
  if (!visualize_event) {
    for (int i = 0; i < int(pythiaPtr.size()); ++i) {
      bool doMerge = pythiaPtr[i]->flag("Merging:doMerging");
      merge.push_back(doMerge);
      pythiaPtr[i]->settings.flag("PartonLevel:FSR",false);
      pythiaPtr[i]->settings.flag("PartonLevel:ISR",false);
      pythiaPtr[i]->settings.flag("PartonLevel:MPI",false);
      pythiaPtr[i]->settings.flag("HadronLevel:all",false);
      pythiaPtr[i]->settings.flag("PartonLevel:Remnants",false);
      pythiaPtr[i]->settings.flag("Check:Event",false);
      if (doMerge) pythiaPtr[i]->settings.flag
                     ("Merging:doXSectionEstimate", true);
    }
  }

  for (int i = 0; i < int(pythiaPtr.size()); ++i)
    if(!pythiaPtr[i]->init()) return 1;

  // Cross section estimate run.
  vector<double> nash, sumsh;
  for (int i = 0; i < int(pythiaPtr.size()); ++i) {
    nash.push_back(0.);
    sumsh.push_back(0.);
  }

  // Save a single event for event generation visualization.
  if (visualize_event) {
    while ( !pythiaPtr.front()->next() )
      if ( pythiaPtr.front()->info.atEndOfFile() ) break;
    printEvent(pythiaPtr.front()->event, visualize_output);
    cout << "\nCreated event visualization. Exiting event generation.\n"<<endl;
    // Clean-up.
    for (int i = 0; i < int(pythiaPtr.size()); ++i) {
      delete pythiaPtr[i]; pythiaPtr[i]=0;
    }
    return 0;
  }

#ifdef OPENMP
#pragma omp parallel num_threads(nThreadsReq)
{
  for (int i = 0; i < nEventEstEach; ++i) {
    vector<int> pythiaIDs;
    // If parallelization can not be done, loop through all
    // Pythia objects.
    if (!doParallel)
      for (int j = 0; j < int(pythiaPtr.size()); ++j)
        pythiaIDs.push_back(j);
    else pythiaIDs.push_back(omp_get_thread_num());
    for (int id = 0; id < int(pythiaIDs.size()); ++id) {
      int j = pythiaIDs[id];
      if ( !pythiaPtr[j]->next() ) {
        if ( pythiaPtr[j]->info.atEndOfFile() ) break;
        else continue;
      }
      sumsh[j] += pythiaPtr[j]->info.weight();
      map <string,string> eventAttributes;
      if (pythiaPtr[j]->info.eventAttributes)
        eventAttributes = *(pythiaPtr[j]->info.eventAttributes);
      string trials = (eventAttributes.find("trials") != eventAttributes.end())
                    ?  eventAttributes["trials"] : "";
      if (trials != "") nash[j] += atof(trials.c_str());
    }
  }
}
#pragma omp barrier
#else

  for (int i = 0; i < nEventEstEach; ++i) {
    for (int j = 0; j < int(pythiaPtr.size()); ++j) {
      if ( !pythiaPtr[j]->next() ) {
        if ( pythiaPtr[j]->info.atEndOfFile() ) break;
        else continue;
      }
      sumsh[j] += pythiaPtr[j]->info.weight();
      map <string,string> eventAttributes;
      if (pythiaPtr[j]->info.eventAttributes)
        eventAttributes = *(pythiaPtr[j]->info.eventAttributes);
      string trials = (eventAttributes.find("trials") != eventAttributes.end())
                    ?  eventAttributes["trials"] : "";
      if (trials != "") nash[j] += atof(trials.c_str());
    }
  }
#endif

  // Store estimated cross sections.
  vector<double> na, xss;
  old = cout.rdbuf();
  for (int i = 0; i < int(pythiaPtr.size()); ++i) {
    // Redirect output so that Pythia banner will not be printed twice.
    ss.str("");
    cout.rdbuf (ss.rdbuf());
    pythiaPtr[i]->stat();
    na.push_back(pythiaPtr[i]->info.nAccepted());
    xss.push_back(pythiaPtr[i]->info.sigmaGen());
  }
  // Restore print-out.
  cout.rdbuf (old);

#ifdef HEPMC3
  bool printHepMC = !(hepmc_output == "");
  // Interface for conversion from Pythia8::Event to HepMC one.
  HepMC3::Pythia8ToHepMC3 toHepMC;
  // Specify file where HepMC events will be stored.
  HepMC3::WriterAscii ascii_io(hepmc_output);
  // Switch off warnings for parton-level events.
  toHepMC.set_print_inconsistency(false);
  toHepMC.set_free_parton_warnings(false);
  // Do not store cross section information, as this will be done manually.
  toHepMC.set_store_pdf(false);
  toHepMC.set_store_proc(false);
  toHepMC.set_store_xsec(false);
  vector< HepMC3::WriterAscii* > ascii_io_var;
#endif

  // Cross section and weights:
  // Total for all runs combined: main.
  double sigmaTotAll(0.);
  // Total for all runs combined: variations.
  vector<double> sigmaTotVarAll;
  // Individual run: main.
  vector<double> sigmaInc, sigmaTot, sumwt, sumwtsq;
  // Individual run: variations.
  vector<vector<double> > sigmaTotVar;
  // Check variations.
  bool doVariationsAll(true);
  for (int i = 0; i < int(pythiaPtr.size()); ++i)
    if ( ! pythiaPtr.front()->settings.flag("Variations:doVariations") )
      doVariationsAll = false;
  if ( doVariationsAll ) {
    for (int iwt=0; iwt < 3; ++iwt) {
#ifdef HEPMC3
      if (printHepMC) {
        string hepmc_var = hepmc_output + "-" + std::to_string(iwt);
        ascii_io_var.push_back(new HepMC3::WriterAscii(hepmc_var));
      }
#endif
      sigmaTotVarAll.push_back(0.);
    }
  }
  for (int i = 0; i < int(pythiaPtr.size()); ++i) {
    sigmaTotVar.push_back(vector<double>());
    if ( doVariationsAll ) {
      for (int iwt=0; iwt < 3; ++iwt)
        sigmaTotVar.back().push_back(0.);
    }
    sigmaInc.push_back(0.);
    sigmaTot.push_back(0.);
    sumwt.push_back(0.);
    sumwtsq.push_back(0.);
  }

  int nEvent = pythiaPtr.front()->settings.mode("Main:numberOfEvents");
  if (nevents > 0) nEvent = nevents;

  int nEventEach = nEvent;
#ifdef OPENMP
  // Number of events per thread.
  if (nThreadsReq > 1) {
    while (nEvent%nThreadsReq != 0) nEvent++;
    nEventEach = nEvent/nThreadsReq;
  }
#endif

  // Histogram of the event weight.
  Hist weight_histogram("weight",10000,-100.,100.);
  Hist pos_weight_histogram("weight",100,0.,100.);
  Hist neg_weight_histogram("weight",100,0.,100.);

  cout << endl << endl << endl;
  cout << "Start generating events" << endl;

  // Switch showering and multiple interaction back on.
  for (int i = 0; i < int(pythiaPtr.size()); ++i) {
    pythiaPtr[i]->settings.flag("PartonLevel:FSR",fsr);
    pythiaPtr[i]->settings.flag("PartonLevel:ISR",isr);
    pythiaPtr[i]->settings.flag("HadronLevel:all",had);
    pythiaPtr[i]->settings.flag("PartonLevel:MPI",mpi);
    pythiaPtr[i]->settings.flag("PartonLevel:Remnants",rem);
    pythiaPtr[i]->settings.flag("Check:Event",chk);
    pythiaPtr[i]->settings.flag("Merging:doMerging",merge[i]);
    pythiaPtr[i]->settings.flag("Merging:doXSectionEstimate", false);
  }

  // Reinitialize Pythia for event generation runs. If we fail to
  // initialize, exit with an error.
  for (int i = 0; i < int(pythiaPtr.size()); ++i)
    if (!pythiaPtr[i]->init()) return 1;

  // Event generation run.

#ifdef OPENMP
#pragma omp parallel num_threads(nThreadsReq)
{
  for (int i = 0; i < nEventEach; ++i) {

    vector<int> pythiaIDs;
    // If parallelization can not be done, loop through all
    // Pythia objects.
    if (!doParallel)
      for (int j = 0; j < int(pythiaPtr.size()); ++j)
        pythiaIDs.push_back(j);
    else pythiaIDs.push_back(omp_get_thread_num());
    for (int id = 0; id < int(pythiaIDs.size()); ++id) {
      int j = pythiaIDs[id];
      if ( !pythiaPtr[j]->next() ) {
        if ( pythiaPtr[j]->info.atEndOfFile() ) break;
        else continue;
      }

      // Do MEM.
      if (pythiaPtr[j]->settings.flag("Dire:doMEM")) { ; }

      // Get event weight(s).
      double evtweight = pythiaPtr[j]->info.weight();

      // Do not print zero-weight events.
      if ( evtweight == 0. ) continue;

#pragma omp critical
{
      weight_histogram.fill( evtweight, 1.0);
      if (evtweight>0.) pos_weight_histogram.fill( evtweight, 1.0);
      if (evtweight<0.) neg_weight_histogram.fill(-evtweight, 1.0);
      if (i>0 && i%1000==0) {
        ofstream wr;
        wr.open("weights.dat");
        weight_histogram /= double(i);
        weight_histogram.table(wr);
        weight_histogram *= double(i);
        wr.close();
        wr.open("pos_weights.dat");
        pos_weight_histogram /= double(i);
        pos_weight_histogram.table(wr);
        pos_weight_histogram *= double(i);
        wr.close();
        wr.open("neg_weights.dat");
        neg_weight_histogram /= double(i);
        neg_weight_histogram.table(wr);
        neg_weight_histogram *= double(i);
        wr.close();
      }
}

      if (abs(evtweight) > 1e3) {
#pragma omp critical
{
        cout << scientific << setprecision(8)
             << "Warning in main program main224.cc: Large shower weight wt="
             << evtweight << endl;
        if (abs(evtweight) > 1e4) {
          cout << "Warning in main program main224.cc: Shower weight larger"
               << " than 10000. Discard event with rare shower weight"
               << " fluctuation." << endl;
          evtweight = 0.;
        }
}
      }

      // Do not print zero-weight events.
      if ( evtweight == 0. ) continue;

      // Now retrieve additional shower weights, and combine these
      // into muR-up and muR-down variations.
      vector<double> evtweights;

#pragma omp critical
{

      sumwt[j]   += evtweight;
      sumwtsq[j] += pow2(evtweight);

      double normhepmc = xss[j]/na[j];

      // Weighted events with additional number of trial events to consider.
      if ( pythiaPtr[j]->info.lhaStrategy() != 0
        && pythiaPtr[j]->info.lhaStrategy() != 3
        && nash[j] > 0)
        normhepmc = 1. / (1e9*nash[j]);
      // Weighted events.
      else if ( pythiaPtr[j]->info.lhaStrategy() != 0
        && pythiaPtr[j]->info.lhaStrategy() != 3
        && nash[j] == 0)
        normhepmc = 1. / (1e9*na[j]);

      if (pythiaPtr[j]->settings.flag("PhaseSpace:bias2Selection"))
        normhepmc = xss[j] / (sumsh[j]);

      if (pythiaPtr[j]->event.size() > 3) {

        double w = evtweight*normhepmc;
        // Add the weight of the current event to the cross section.
        double divide = (splitSingleRun ? double(nThreadsReq) : 1.0);
        sigmaTotAll += w/divide;
        sigmaInc[j] += evtweight*normhepmc;
        sigmaTot[j] += w;
#ifdef HEPMC3
        if (printHepMC) {
          // Construct new empty HepMC event.
          HepMC3::GenEvent hepmcevt;
          // Set event weight
          hepmcevt.weights().push_back(w);
          // Fill HepMC event
          toHepMC.fill_next_event( *pythiaPtr[j], &hepmcevt );
          // Report cross section to hepmc.
          shared_ptr<HepMC3::GenCrossSection> xsec;
          xsec = make_shared<HepMC3::GenCrossSection>();
          // First add object to event, then set cross section. This
          // order ensures that the lengths of the cross section and
          // the weight vector agree.
          hepmcevt.set_cross_section( xsec );
          xsec->set_cross_section( sigmaTotAll*1e9,
            pythiaPtr[j]->info.sigmaErr()*1e9 );
          // Write the HepMC event to file. Done with it.
          ascii_io.write_event(hepmcevt);
        }
#endif

        // Write additional HepMC events.
        for (int iwt=0; iwt < int(evtweights.size()); ++iwt) {
          double wVar = evtweight*evtweights[iwt]*normhepmc;
          // Add the weight of the current event to the cross section.
          double divideVar = (splitSingleRun ? double(nThreadsReq) : 1.0);
          sigmaTotVarAll[iwt] += wVar/divideVar;
          sigmaTotVar[j][iwt] += wVar;
#ifdef HEPMC3
          if (printHepMC) {
            HepMC3::GenEvent hepmcevt;
            // Set event weight
            hepmcevt.weights().push_back(wVar);
            // Fill HepMC event
            toHepMC.fill_next_event( *pythiaPtr[j], &hepmcevt );
            // Report cross section to hepmc.
            shared_ptr<HepMC3::GenCrossSection> xsec;
            xsec = make_shared<HepMC3::GenCrossSection>();
            // First add object to event, then set cross section. This
            // order ensures that the lengths of the cross section and
            // the weight vector agree.
            hepmcevt.set_cross_section( xsec );
            xsec->set_cross_section( sigmaTotVarAll[iwt]*1e9,
              pythiaPtr[j]->info.sigmaErr()*1e9 );
            // Write the HepMC event to file. Done with it.
            ascii_io_var[iwt]->write_event(hepmcevt);
          }
#endif
        }
      }
    }
  }
}
}
#pragma omp barrier
#else
  for (int i = 0; i < nEventEach; ++i) {

    for (int j = 0; j < int(pythiaPtr.size()); ++j) {
      if ( !pythiaPtr[j]->next() ) {
        if ( pythiaPtr[j]->info.atEndOfFile() ) break;
        else continue;
      }

      // Get event weight(s).
      double evtweight = pythiaPtr[j]->info.weight();

      // Do not print zero-weight events.
      if ( evtweight == 0. ) continue;

      weight_histogram.fill( evtweight, 1.0);
      if (evtweight>0.) pos_weight_histogram.fill( evtweight, 1.0);
      if (evtweight<0.) neg_weight_histogram.fill(-evtweight, 1.0);
      if (i>0 && i%1000==0) {
        ofstream wr;
        wr.open("weights.dat");
        weight_histogram /= double(i);
        weight_histogram.table(wr);
        weight_histogram *= double(i);
        wr.close();
        wr.open("pos_weights.dat");
        pos_weight_histogram /= double(i);
        pos_weight_histogram.table(wr);
        pos_weight_histogram *= double(i);
        wr.close();
        wr.open("neg_weights.dat");
        neg_weight_histogram /= double(i);
        neg_weight_histogram.table(wr);
        neg_weight_histogram *= double(i);
        wr.close();
      }

      if (abs(evtweight) > 1e3) {
        cout << scientific << setprecision(8)
          << "Warning in main program main224.cc: Large shower weight wt="
          << evtweight << endl;
        if (abs(evtweight) > 1e4) {
          cout << "Warning in main program main224.cc: Shower weight larger"
               << " than 10000. Discard event with rare shower weight"
               << " fluctuation." << endl;
          evtweight = 0.;
        }
      }

      // Do not print zero-weight events.
      if ( evtweight == 0. ) continue;

      // Now retrieve additional shower weights, and combine these
      // into muR-up and muR-down variations.
      vector<double> evtweights;
      sumwt[j]   += evtweight;
      sumwtsq[j] += pow2(evtweight);
      double normhepmc = xss[j]/na[j];

      // Weighted events with additional number of trial events to consider.
      if ( pythiaPtr[j]->info.lhaStrategy() != 0
        && pythiaPtr[j]->info.lhaStrategy() != 3
        && nash[j] > 0)
        normhepmc = 1. / (1e9*nash[j]);
      // Weighted events.
      else if ( pythiaPtr[j]->info.lhaStrategy() != 0
        && pythiaPtr[j]->info.lhaStrategy() != 3
        && nash[j] == 0)
        normhepmc = 1. / (1e9*na[j]);

      if (pythiaPtr[j]->settings.flag("PhaseSpace:bias2Selection"))
        normhepmc = 1. / (1e9*na[j]);

      if (pythiaPtr[j]->event.size() > 3) {

        double w = evtweight*normhepmc;
        // Add the weight of the current event to the cross section.
        sigmaTotAll += w;
        sigmaInc[j] += evtweight*normhepmc;
        sigmaTot[j] += w;
#ifdef HEPMC3
        if (printHepMC) {
          // Construct new empty HepMC event.
          HepMC3::GenEvent hepmcevt;
          // Set event weight
          hepmcevt.weights().push_back(w);
          // Fill HepMC event
          toHepMC.fill_next_event( *pythiaPtr[j], &hepmcevt );
          // Report cross section to hepmc.
          shared_ptr<HepMC3::GenCrossSection> xsec;
          xsec = make_shared<HepMC3::GenCrossSection>();
          // First add object to event, then set cross section. This
          // order ensures that the lengths of the cross section and
          // the weight vector agree.
          hepmcevt.set_cross_section( xsec );
          xsec->set_cross_section( sigmaTotAll*1e9,
            pythiaPtr[j]->info.sigmaErr()*1e9 );
          // Write the HepMC event to file.
          ascii_io.write_event(hepmcevt);
        }
#endif

        // Write additional HepMC events.
        for (int iwt=0; iwt < int(evtweights.size()); ++iwt) {
          double wVar = evtweight*evtweights[iwt]*normhepmc;
          // Add the weight of the current event to the cross section.
          sigmaTotVarAll[iwt] += wVar;
          sigmaTotVar[j][iwt] += wVar;
#ifdef HEPMC3
          if (printHepMC) {
            HepMC3::GenEvent hepmcevt;
            // Set event weight
            hepmcevt.weights().push_back(wVar);
            // Fill HepMC event
            toHepMC.fill_next_event( *pythiaPtr[j], &hepmcevt );
            // Report cross section to hepmc.
            shared_ptr<HepMC3::GenCrossSection> xsec;
            xsec = make_shared<HepMC3::GenCrossSection>();
            // First add object to event, then set cross section. This
            // order ensures that the lengths of the cross section and
            // the weight vector agree.
            hepmcevt.set_cross_section( xsec );
            xsec->set_cross_section( sigmaTotVarAll[iwt]*1e9,
              pythiaPtr[j]->info.sigmaErr()*1e9 );
            // Write the HepMC event to file.
            ascii_io_var[iwt]->write_event(hepmcevt);
          }
#endif
        }
      }
    }
  }
#endif

  // Print cross section, errors.
  double sumTot(0.), sum2Tot(0.), nacTot(0.);
  cout << "\nSummary of individual runs:\n";
  for (int i = 0; i < int(pythiaPtr.size()); ++i) {
    cout << "\nSummary of runs #" << i << " :\n";
    pythiaPtr[i]->stat();
    int nAc = pythiaPtr[i]->info.nAccepted();
    cout << scientific << setprecision(6)
      << "\n\t Mean shower weight         = " << sumwt[i]/double(nAc) << "\n"
      << "\t Variance of shower weight  = "
      << sqrt(1/double(nAc)*(sumwtsq[i] - pow(sumwt[i],2)/double(nAc)))
      << endl;
    cout << "\t Inclusive cross section    : " << sigmaInc[i] << "\n";
    cout << "\t Cross section after shower : " << sigmaTot[i] << "\n";
    sumTot += sumwt[i];
    sum2Tot += sumwtsq[i];
    nacTot += nAc;
  }
  cout << "\nCombination of runs:\n"
    << scientific << setprecision(6)
    << "\t Mean shower weight         = " << sumTot/nacTot << "\n"
    << "\t Variance of shower weight  = "
    << sqrt(1/nacTot*(sum2Tot - pow(sumTot,2)/nacTot ))
    << " " << 1/nacTot*(sum2Tot - pow(sumTot,2)/nacTot )
    << "\n"
    << "\t Cross section after shower : " << sigmaTotAll << "\n";

#ifdef HEPMC3
  // Clean-up.
  if ( pythiaPtr.front()->settings.flag("Variations:doVariations") ) {
    if (printHepMC) for (int iwt=0; iwt < 3; ++iwt) delete ascii_io_var[iwt];
 }
#endif


  // Write endtag. Overwrite initialization info with new cross sections.
  ofstream write;
  write.open("weights.dat");
  weight_histogram /= double(nacTot);
  weight_histogram.table(write);
  write.close();
  write.open("pos_weights.dat");
  pos_weight_histogram /= double(nacTot);
  pos_weight_histogram.table(write);
  write.close();
  write.open("neg_weights.dat");
  neg_weight_histogram /= double(nacTot);
  neg_weight_histogram.table(write);
  write.close();

  // Clean-up.
  for (int i = 0; i < int(pythiaPtr.size()); ++i) {
    delete pythiaPtr[i]; pythiaPtr[i]=0;
  }

  // Done.
  return 0;
}
