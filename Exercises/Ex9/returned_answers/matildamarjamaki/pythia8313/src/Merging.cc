// MergingHooks.cc is a part of the PYTHIA event generator.
// Copyright (C) 2025 Torbjorn Sjostrand.
// PYTHIA is licenced under the GNU GPL v2 or later, see COPYING for details.
// Please respect the MCnet Guidelines, see GUIDELINES for details.

// This file is written by Stefan Prestel.
// Function definitions (not found in the header) for the Merging class.

#include "Pythia8/Merging.h"

namespace Pythia8 {

//==========================================================================

// The Merging class.

//--------------------------------------------------------------------------

// Factor by which the maximal value of the merging scale can deviate before
// a warning is printed.
const double Merging::TMSMISMATCH = 1.5;

// Minimum allowed weight value to prevent division by zero.
const double Merging::MINWGT = 1e-10;

//--------------------------------------------------------------------------

// Initialise Merging class.

void Merging::init() {tmsNowMin = infoPtr->eCM();}

//--------------------------------------------------------------------------

// Function to print information.

void Merging::statistics() {

  // Recall switch to enfore merging scale cut.
  bool enforceCutOnLHE  = flag("Merging:enforceCutOnLHE");
  // Recall merging scale value.
  double tmsval         = mergingHooksPtr ? mergingHooksPtr->tms() : 0;
  bool printBanner      = enforceCutOnLHE && tmsNowMin > TMSMISMATCH*tmsval;
  // Reset minimal tms value.
  tmsNowMin             = infoPtr->eCM();

  if (!printBanner) return;

  // Header.
  cout << "\n *-------  PYTHIA Matrix Element Merging Information  ------"
       << "-------------------------------------------------------*\n"
       << " |                                                            "
       << "                                                     |\n";
  // Print warning if the minimal tms value of any event was significantly
  // above the desired merging scale value.
  cout << " | Warning in Merging::statistics: All Les Houches events"
       << " significantly above Merging:TMS cut. Please check.       |\n";

  // Listing finished.
  cout << " |                                                            "
       << "                                                     |\n"
       << " *-------  End PYTHIA Matrix Element Merging Information -----"
       << "-----------------------------------------------------*" << endl;
}

//--------------------------------------------------------------------------

// Function to steer different merging prescriptions.

int Merging::mergeProcess(Event& process){

  int vetoCode = 1;

  // Reinitialise hard process.
  mergingHooksPtr->hardProcess->clear();
  mergingHooksPtr->processNow = word("Merging:Process");
  mergingHooksPtr->hardProcess->initOnProcess(
    mergingHooksPtr->processNow, particleDataPtr);

  settingsPtr->word("Merging:Process", mergingHooksPtr->processSave);

  mergingHooksPtr->doUserMergingSave = flag("Merging:doUserMerging");
  mergingHooksPtr->doMGMergingSave = flag("Merging:doMGMerging");
  mergingHooksPtr->doKTMergingSave = flag("Merging:doKTMerging");
  mergingHooksPtr->doPTLundMergingSave = flag("Merging:doPTLundMerging");
  mergingHooksPtr->doCutBasedMergingSave = flag("Merging:doCutBasedMerging");
  mergingHooksPtr->doNL3TreeSave = flag("Merging:doNL3Tree");
  mergingHooksPtr->doNL3LoopSave = flag("Merging:doNL3Loop");
  mergingHooksPtr->doNL3SubtSave = flag("Merging:doNL3Subt");
  mergingHooksPtr->doUNLOPSTreeSave = flag("Merging:doUNLOPSTree");
  mergingHooksPtr->doUNLOPSLoopSave = flag("Merging:doUNLOPSLoop");
  mergingHooksPtr->doUNLOPSSubtSave = flag("Merging:doUNLOPSSubt");
  mergingHooksPtr->doUNLOPSSubtNLOSave = flag("Merging:doUNLOPSSubtNLO");
  mergingHooksPtr->doUMEPSTreeSave = flag("Merging:doUMEPSTree");
  mergingHooksPtr->doUMEPSSubtSave = flag("Merging:doUMEPSSubt");
  mergingHooksPtr->nReclusterSave = mode("Merging:nRecluster");

  mergingHooksPtr->hasJetMaxLocal  = false;
  mergingHooksPtr->nJetMaxLocal = mergingHooksPtr->nJetMaxSave;
  mergingHooksPtr->nJetMaxNLOLocal = mergingHooksPtr->nJetMaxNLOSave;
  mergingHooksPtr->nRequestedSave = mode("Merging:nRequested");

  // Ensure that merging weight is not counted twice.
  bool includeWGT = mergingHooksPtr->includeWGTinXSEC();

  // Possibility to apply merging scale to an input event.
  bool applyTMSCut = flag("Merging:doXSectionEstimate");
  if ( applyTMSCut && cutOnProcess(process) ) {
    if (includeWGT) infoPtr->weightContainerPtr->setWeightNominal(0.);
    return -1;
  }
  // Done if only a cut should be applied.
  if ( applyTMSCut ) return 1;

  // For the runtime interface between aMCatNLO and Pythia, simply
  // reconstruct scale and dead zone information and exit.
  if (mergingHooksPtr->doRuntimeAMCATNLOInterface())
    return clusterAndStore(process);

  // Possibility to perform CKKW-L merging on this event.
  if ( mergingHooksPtr->doCKKWLMerging() )
    vetoCode = mergeProcessCKKWL(process);

  // Possibility to perform UMEPS merging on this event.
  if ( mergingHooksPtr->doUMEPSMerging() )
     vetoCode = mergeProcessUMEPS(process);

  // Possibility to perform NL3 NLO merging on this event.
  if ( mergingHooksPtr->doNL3Merging() )
    vetoCode = mergeProcessNL3(process);

  // Possibility to perform UNLOPS merging on this event.
  if ( mergingHooksPtr->doUNLOPSMerging() )
    vetoCode = mergeProcessUNLOPS(process);

  return vetoCode;

}

//--------------------------------------------------------------------------

// Store all information required for the runtime interface to aMCatNLO.
// The method proceeds by generating all histories for the input event, then
// loops through the first layer of the history tree to extract all necessary
// information to be passed on to aMCatNLO. This function is
// not used internally, only when interfacing to aMCatNLO at runtime.

int Merging::clusterAndStore(Event& process){

  // Clear all previous event-by-event information.
  clearInfos();

  // Ensure that merging hooks to not veto events in the trial showers.
  mergingHooksPtr->doIgnoreStep(true);
  // For pp > h, allow cut on state, so that underlying processes
  // can be clustered to gg > h
  if ( mergingHooksPtr->getProcessString().compare("pp>h") == 0 )
    mergingHooksPtr->allowCutOnRecState(true);
  // Avoid ordering constraints.
  mergingHooksPtr->orderHistories(false);

  // Prepare process record for merging. If Pythia has already decayed
  // resonances used to define the hard process, remove resonance decay
  // products.
  Event newProcess( mergingHooksPtr->bareEvent( process, true) );
  // Reset any incoming spins for W+-.
  if (mergingHooksPtr->doWeakClustering())
    for (int i = 0;i < newProcess.size();++i)
      newProcess[i].pol(9);
  // Store candidates for the splitting V -> qqbar'.
  mergingHooksPtr->storeHardProcessCandidates( newProcess);

  // Get merging scale in current event.
  double tmsnow = mergingHooksPtr->tmsNow( newProcess );
  // Calculate number of clustering steps.
  int nSteps = mergingHooksPtr->getNumberOfClusteringSteps( newProcess, true);

  // Store hard event cut information, reset veto information.
  mergingHooksPtr->setHardProcessInfo(nSteps, tmsnow);
  mergingHooksPtr->setEventVetoInfo(-1, -1.);

  // Get random number to choose a path.
  double RN = rndmPtr->flat();

  // Set dummy process scale.
  newProcess.scale(0.0);
  // Generate all histories.
  History FullHistory( nSteps, 0.0, newProcess, Clustering(), mergingHooksPtr,
            (*beamAPtr), (*beamBPtr), particleDataPtr, infoPtr,
            trialPartonLevelPtr, coupSMPtr, true, true, true, true, 1.0, 0);

  // Project histories onto desired branches, e.g. only ordered paths.
  FullHistory.projectOntoDesiredHistories();

  // Setup the selected path. Needed for
  FullHistory.select(RN)->setSelectedChild();

  int posOffset = 2;

  // Store information on every possible last clustering.
  for ( int i = 0 ; i < int(FullHistory.children.size()); ++i) {

    // Get all clustering variables.
    map<string,double> stateVars;
    int rad = FullHistory.children[i]->clusterIn.emittor;
    int emt = FullHistory.children[i]->clusterIn.emitted;

    int iemtReq = atoi(infoPtr->getEventAttribute("ifks").c_str());
    int jradReq = atoi(infoPtr->getEventAttribute("jfks").c_str());

    // Only consider last event entry as allowed emission.
    if (emt != iemtReq+posOffset) continue;
    if (rad != jradReq+posOffset) continue;

    // Save clustered event in external container, if necessary.
    if (lhaPtr) {
      lhaPtr->setEventPtr(&FullHistory.children[i]->state);
      lhaPtr->setEvent();
    }

    vector<pair<int,int> > dipEnds;
    // Loop through final state of system to find possible dipole ends.
    for (int ip = 0; ip < FullHistory.children[i]->state.size(); ++ip) {
      if ( FullHistory.children[i]->state[ip].isFinal()
        || FullHistory.children[i]->state[ip].mother1()== 1
        || FullHistory.children[i]->state[ip].mother1()== 2 ) {
        // Find dipole end formed by colour index.
        int colTag = FullHistory.children[i]->state[ip].col();
        if (colTag > 0)
          getDipoles(ip,  colTag,  1, FullHistory.children[i]->state, dipEnds);
        // Find dipole end formed by anticolour index.
        int acolTag = FullHistory.children[i]->state[ip].acol();
        if (acolTag > 0)
          getDipoles(ip, acolTag, -1, FullHistory.children[i]->state, dipEnds);
      }
    }

    for (size_t id = 0; id < dipEnds.size(); ++id) {

      int iRad(0), iRec(0);
      map<int,int>::iterator it
        = FullHistory.children[i]->clusterIn.iPosInMother.find(
          dipEnds[id].first);
      if ( it == FullHistory.children[i]->clusterIn.iPosInMother.end() )
        continue;
      iRad = it->second;

      map<int,int>::iterator it2
        = FullHistory.children[i]->clusterIn.iPosInMother.find(
          dipEnds[id].second);
      if ( it2 == FullHistory.children[i]->clusterIn.iPosInMother.end() )
        continue;
      iRec = it2->second;

      // Already covered clustering.
      vector<int>::iterator itRad = find(radSave.begin(), radSave.end(), iRad);
      vector<int>::iterator itRec = find(recSave.begin(), recSave.end(), iRec);
      int indexRad = std::distance(radSave.begin(), itRad);
      int indexRec = std::distance(recSave.begin(), itRec);
      if ( (itRad != radSave.end() || itRec != recSave.end())
        && indexRad == indexRec) {
        if (FullHistory.children[i]->state[iRad].id() != 21 ||
            FullHistory.children[i]->state[iRec].id() != 21) continue;
        else {
          // Only continue of gluon was already counted as part of two dipoles.
          int ir(0), is(0);
          ir = count(radSave.begin(), radSave.end(), iRad);
          is = count(recSave.begin(), recSave.end(), iRec);
          if (ir==2 || is==2) continue;
        }
      }

      // Function to compute "pythia pT separation" from Particle input.
      int type = FullHistory.state[iRad].isFinal() ? 1 : -1;
      int iPartnerForPT = iRec;
      if ( !settingsPtr->flag("aMC@NLO:debugScales")
        && !FullHistory.state[iRad].isFinal())
        iPartnerForPT = (iRad == 3) ? 4 : 3;
      double t = FullHistory.pTLund(FullHistory.state,
        iRad, emt, iPartnerForPT, type);
      Vec4 qDip;
      if (FullHistory.state[iRad].isFinal())
           qDip += FullHistory.state[iRad].p();
      else qDip -= FullHistory.state[iRad].p();
      if (FullHistory.state[iPartnerForPT].isFinal())
           qDip += FullHistory.state[iPartnerForPT].p();
      else qDip -= FullHistory.state[iPartnerForPT].p();
      if (FullHistory.state[emt].isFinal())
           qDip += FullHistory.state[emt].p();
      else qDip -= FullHistory.state[emt].p();
      double mass = sqrt(abs(qDip.m2Calc()));
      // Just store pT for now.
      double tStore = (t==1e10 ? 0.0 : t);
      stoppingScalesSave.push_back(tStore);
      radSave.push_back(iRad);
      emtSave.push_back(emt);
      recSave.push_back(iRec);
      mDipSave.push_back(mass);
      // Consider cases in the dead zone that:
      // - completely failed the pT reconstruction (t<=0)
      // - had virtuality larer than the dipole mass
      // - had negative dipole mass (measured after emission)
      bool dead = (t<=0. || t==1e10 || t==1e-5);
      //bool dead = (t<=0.);
      isInDeadzone.push_back(dead);

    }

  }

  // Done.
  return -1;

}

//--------------------------------------------------------------------------

// Setup all QCD dipole end for a QCD colour charge. Helper function to be
// able to extract all shower scales by checking all dipoles.

void Merging::getDipoles( int iRad, int colTag, int colSign,
  const Event& event, vector<pair<int,int> >& dipEnds) {

  vector<int> recPos;

  // Colour: other end by same index in final state or opposite in beam.
  if (colSign > 0 && !event[iRad].isFinal())
  for (int iRecNow = 0; iRecNow < event.size(); ++iRecNow) {
    if (iRecNow == iRad) continue;
    if ( ( event[iRecNow].col()  == colTag &&  event[iRecNow].isFinal() )
      || ( event[iRecNow].acol() == colTag && !event[iRecNow].isFinal() ) ) {
      recPos.push_back(iRecNow);
    }
  }

  // Anticolour: other end by same index in final state or opposite in beam.
  if (colSign < 0 && !event[iRad].isFinal())
  for (int iRecNow = 0; iRecNow < event.size(); ++iRecNow) {
    if (iRecNow == iRad) continue;
    if ( ( event[iRecNow].acol() == colTag &&  event[iRecNow].isFinal() )
      || ( event[iRecNow].col()  == colTag && !event[iRecNow].isFinal() ) ) {
      recPos.push_back(iRecNow);
    }
  }

  // Colour: other end by same index in final state or opposite in beam.
  if (colSign > 0 && event[iRad].isFinal())
  for (int iRecNow = 0; iRecNow < event.size(); ++iRecNow) {
    if (iRecNow == iRad) continue;
    if ( ( event[iRecNow].acol() == colTag &&  event[iRecNow].isFinal() )
      || ( event[iRecNow].col()  == colTag && !event[iRecNow].isFinal() ) ) {
      recPos.push_back(iRecNow);
    }
  }

  // Anticolour: other end by same index in final state or opposite in beam.
  if (colSign < 0 && event[iRad].isFinal())
  for (int iRecNow = 0; iRecNow < event.size(); ++iRecNow) {
    if (iRecNow == iRad) continue;
    if ( ( event[iRecNow].col()   == colTag &&  event[iRecNow].isFinal() )
      || ( event[iRecNow].acol()  == colTag && !event[iRecNow].isFinal() ) ) {
      recPos.push_back(iRecNow);
    }
  }

  // Store dipole colour end(s).
  for (unsigned int i = 0; i < recPos.size(); ++i) {
    int iRecNow = recPos[i];
    dipEnds.push_back(make_pair(iRad,iRecNow));
  }

}

//--------------------------------------------------------------------------

// Function to retrieve shower scale information (to be used to set
// scales in aMCatNLO-LHEF-production. This function is
// not used internally, only when interfacing to aMCatNLO at runtime.

void Merging::getStoppingInfo(double scales[100][100],
  double masses[100][100]) {

  int posOffest = 2;
  for (unsigned int i = 0; i < radSave.size(); ++i){
    // In fortran we want scales(radiator,recoiler), hence, we should
    // take the transpose here, since 2-d arrays are treated
    // differently in c++ and fortran.
    scales[recSave[i]-posOffest][radSave[i]-posOffest] = stoppingScalesSave[i];
    masses[recSave[i]-posOffest][radSave[i]-posOffest] = mDipSave[i];
  }

}

//--------------------------------------------------------------------------

// Function to retrieve if any of the shower scales would not have been
// possible to produce by Pythia. This function is
// not used internally, only when interfacing to aMCatNLO at runtime.

void Merging::getDeadzones(bool dzone[100][100]) {
  int posOffest = 2;
  for (unsigned int i = 0; i < radSave.size(); ++i){
    // In fortran we want dzone(radiator,recoiler), hence, we should
    // take the transpose here, since 2-d arrays are treated
    // differently in c++ and fortran.
    dzone[recSave[i]-posOffest][radSave[i]-posOffest] = isInDeadzone[i];
  }
}

//--------------------------------------------------------------------------

// Function to generate Sudakov factors for MCatNLO-Delta. This function is
// not used internally, only when interfacing to aMCatNLO at runtime.

double Merging::generateSingleSudakov(double pTbegAll,
  double pTendAll, double m2dip, int idA, int type, double s, double x) {

  // II
  double prob = 1.;
  if (type == 1) {
    prob = trialPartonLevelPtr->spacePtr->noEmissionProbability( pTbegAll,
      pTendAll, m2dip, idA, -1, s, x);
  // FF
  } else if (type == 2) {
    prob = trialPartonLevelPtr->timesPtr->noEmissionProbability( pTbegAll,
      pTendAll, m2dip, idA, 1, s, x);
  // IF
  } else if (type == 3) {
    prob = trialPartonLevelPtr->spacePtr->noEmissionProbability( pTbegAll,
      pTendAll, m2dip, idA, 1, s, x);
  // FI
  } else if (type == 4) {
    prob = trialPartonLevelPtr->timesPtr->noEmissionProbability( pTbegAll,
      pTendAll, m2dip, idA, -1, s, x);
  }

  return prob;

}

//--------------------------------------------------------------------------

// Function to perform CKKW-L merging on this event.

int Merging::mergeProcessCKKWL( Event& process) {

  // Ensure that merging hooks to not veto events in the trial showers.
  mergingHooksPtr->doIgnoreStep(true);
  // For pp > h, allow cut on state, so that underlying processes
  // can be clustered to gg > h
  if ( mergingHooksPtr->getProcessString().compare("pp>h") == 0 )
    mergingHooksPtr->allowCutOnRecState(true);
  // For now, prefer construction of ordered histories.
  mergingHooksPtr->orderHistories(true);

  // Ensure that merging weight is not counted twice.
  bool includeWGT = mergingHooksPtr->includeWGTinXSEC();

  // Reset weight of the event.
  int nWgts = mergingHooksPtr->nWgts;
  vector<double> wgt( nWgts, 1.0 );
  mergingHooksPtr->setWeightCKKWL(wgt);
  mergingHooksPtr->muMI(-1.);

  // Prepare process record for merging. If Pythia has already decayed
  // resonances used to define the hard process, remove resonance decay
  // products.
  Event newProcess( mergingHooksPtr->bareEvent( process, true) );
  // Reset any incoming spins for W+-.
  if (mergingHooksPtr->doWeakClustering())
    for (int i = 0;i < newProcess.size();++i)
      newProcess[i].pol(9);
  // Store candidates for the splitting V -> qqbar'.
  mergingHooksPtr->storeHardProcessCandidates( newProcess);

  // Check if event passes the merging scale cut.
  double tmsval = mergingHooksPtr->tms();
  // Get merging scale in current event.
  double tmsnow = mergingHooksPtr->tmsNow( newProcess );
  // Calculate number of clustering steps.
  int nSteps = mergingHooksPtr->getNumberOfClusteringSteps( newProcess, true);

  // Check if hard event cut should be applied later.
  bool applyVeto = flag("Merging:applyVeto");

  // Too few steps can be possible if a chain of resonance decays has been
  // removed. In this case, reject this event, since it will be handled in
  // lower-multiplicity samples.
  int nRequested = mergingHooksPtr->nRequested();

  // Store hard event cut information, reset veto information.
  mergingHooksPtr->setHardProcessInfo(nSteps, tmsnow);
  mergingHooksPtr->setEventVetoInfo(-1, -1.);
  if (nSteps < nRequested ) {
    if (!includeWGT) mergingHooksPtr->
                       setWeightCKKWL(vector<double>(nWgts, 0.));
    if ( includeWGT) infoPtr->weightContainerPtr->setWeightNominal(0.);
    if (applyVeto) return -1;
    else return 1;
  }

  // Reset the minimal tms value, if necessary.
  tmsNowMin = (nSteps > 0 && tmsnow < infoPtr->eCM())
    ? min(tmsNowMin, tmsnow) : 0.;

  // Get random number to choose a path.
  double RN = rndmPtr->flat();

  // Set dummy process scale.
  newProcess.scale(0.0);
  // Generate all histories.
  History FullHistory( nSteps, 0.0, newProcess, Clustering(), mergingHooksPtr,
            (*beamAPtr), (*beamBPtr), particleDataPtr, infoPtr,
            trialPartonLevelPtr, coupSMPtr, true, true, true, true, 1.0, 0);

  // Project histories onto desired branches, e.g. only ordered paths.
  FullHistory.projectOntoDesiredHistories();

  // Setup the selected path. Needed for
  FullHistory.select(RN)->setSelectedChild();

  // Do not apply cut if the configuration could not be projected onto an
  // underlying born configuration.
  bool applyCut = nSteps > 0 && FullHistory.select(RN)->nClusterings() > 0;

  // Enfore merging scale cut if the event did not pass the merging scale
  // criterion.
  bool enforceCutOnLHE  = flag("Merging:enforceCutOnLHE");
  if ( enforceCutOnLHE && applyCut && tmsnow < tmsval && tmsnow >= 0. ) {
    loggerPtr->WARNING_MSG(
      "Les Houches Event fails merging scale cut. Rejecting event");
    if (!includeWGT) mergingHooksPtr->
                       setWeightCKKWL(vector<double>(nWgts, 0.));
    if ( includeWGT) infoPtr->weightContainerPtr->setWeightNominal(0.);
    //return -1;
    if (applyVeto) return -1;
    else return 1;
  }

  // Check if more steps should be taken.
  int nFinalP = 0;
  int nFinalW = 0;
  Event coreProcess = Event();
  coreProcess.clear();
  coreProcess.init( "(hard process-modified)", particleDataPtr );
  coreProcess.clear();
  coreProcess = FullHistory.lowestMultProc(RN);
  for ( int i = 0; i < coreProcess.size(); ++i )
    if ( coreProcess[i].isFinal() ) {
      if ( coreProcess[i].colType() != 0 )
        nFinalP++;
      if ( coreProcess[i].idAbs() == 24 )
        nFinalW++;
    }

  bool complete = (FullHistory.select(RN)->nClusterings() == nSteps) ||
    ( mergingHooksPtr->doWeakClustering() && nFinalP == 2 && nFinalW == 0 );

  if ( !complete ) {
    loggerPtr->WARNING_MSG("no clusterings found. History incomplete");
  }

  // Calculate CKKWL weight:
  // Perform reweighting with Sudakov factors, save alpha_s ratios and
  // PDF ratio weights.
  wgt = FullHistory.weightCKKWL( trialPartonLevelPtr,
    mergingHooksPtr->AlphaS_FSR(), mergingHooksPtr->AlphaS_ISR(),
    mergingHooksPtr->AlphaEM_FSR(), mergingHooksPtr->AlphaEM_ISR(), RN);

  // Event with production scales set for further (trial) showering
  // and starting conditions for the shower.
  FullHistory.getStartingConditions( RN, process );
  // If necessary, reattach resonance decay products.
  mergingHooksPtr->reattachResonanceDecays(process);

  // Allow to dampen histories in which the lowest multiplicity reclustered
  // state does not pass the lowest multiplicity cut of the matrix element.
  double dampWeight = mergingHooksPtr->dampenIfFailCuts(
           FullHistory.lowestMultProc(RN) );
  // Save the weight of the event for histogramming. Only change the
  // event weight after trial shower on the matrix element
  // multiplicity event (= in doVetoStep).
  for (double& wgti: wgt) wgti *= dampWeight;

  // Save the weight of the event for histogramming.
  if (!includeWGT) mergingHooksPtr->setWeightCKKWL(wgt);

  // Update the event weight.
  if ( includeWGT) {
    // In this case, central merging weight goes into nominal weight, all
    // variations are saved relative to central merging weight
    vector<double> relWgt({1.});
    for (int iVar = 1; iVar < nWgts; ++iVar) {
      relWgt.push_back(wgt[0] != 0 ? wgt[iVar]/wgt[0] : 0.);
      if (abs(wgt[iVar]) > MINWGT && wgt[0] < MINWGT) {
        loggerPtr->WARNING_MSG("cannot normalize merging weight to zero.",
          "try Merging:includeWeightInXsection off");
      }
    }
    infoPtr->weightContainerPtr->
                    setWeightNominal(infoPtr->weight()*wgt[0]);
    mergingHooksPtr->setWeightCKKWL(relWgt);

  }

  // Allow merging hooks to veto events from now on.
  mergingHooksPtr->doIgnoreStep(false);

  // If no-emission probability is zero.
  if ( applyVeto && wgt[0] == 0. ) return 0;

  // Done
  return 1;

}

//--------------------------------------------------------------------------

// Function to perform UMEPS merging on this event.

int Merging::mergeProcessUMEPS( Event& process) {

  // Initialise which part of UMEPS merging is applied.
  bool doUMEPSTree                = flag("Merging:doUMEPSTree");
  bool doUMEPSSubt                = flag("Merging:doUMEPSSubt");
  // Save number of looping steps
  mergingHooksPtr->nReclusterSave = mode("Merging:nRecluster");
  int nRecluster                  = mode("Merging:nRecluster");

  // Ensure that merging hooks does not remove emissions.
  mergingHooksPtr->doIgnoreEmissions(true);
  // For pp > h, allow cut on state, so that underlying processes
  // can be clustered to gg > h
  if ( mergingHooksPtr->getProcessString().compare("pp>h") == 0 )
    mergingHooksPtr->allowCutOnRecState(true);
  // For now, prefer construction of ordered histories.
  mergingHooksPtr->orderHistories(true);

  // Ensure that merging weight is not counted twice.
  bool includeWGT = mergingHooksPtr->includeWGTinXSEC();

  // Reset any incoming spins for W+-.
  if (mergingHooksPtr->doWeakClustering())
    for (int i = 0;i < process.size();++i)
      process[i].pol(9);

  // Reset weights of the event.
  int nWgts = mergingHooksPtr->nWgts;
  vector<double> wgt( nWgts, 1.);
  mergingHooksPtr->setWeightCKKWL(wgt);
  mergingHooksPtr->muMI(-1.);

  // Prepare process record for merging. If Pythia has already decayed
  // resonances used to define the hard process, remove resonance decay
  // products.
  Event newProcess( mergingHooksPtr->bareEvent( process, true) );
  // Store candidates for the splitting V -> qqbar'.
  mergingHooksPtr->storeHardProcessCandidates( newProcess );

  // Check if event passes the merging scale cut.
  double tmsval   = mergingHooksPtr->tms();
  // Get merging scale in current event.
  double tmsnow  = mergingHooksPtr->tmsNow( newProcess );
  // Calculate number of clustering steps.
  int nSteps = mergingHooksPtr->getNumberOfClusteringSteps( newProcess, true);
  int nRequested = mergingHooksPtr->nRequested();

  // Check if hard event cut should be applied later.
  bool applyVeto = flag("Merging:applyVeto");

  // Too few steps can be possible if a chain of resonance decays has been
  // removed. In this case, reject this event, since it will be handled in
  // lower-multiplicity samples.
  if (nSteps < nRequested) {
    if (!includeWGT) mergingHooksPtr->
                       setWeightCKKWL(vector<double>(nWgts, 0.));
    if ( includeWGT) infoPtr->weightContainerPtr->setWeightNominal(0.);
    if (applyVeto) return -1;
    else return 1;
  }

  // Reset the minimal tms value, if necessary.
  tmsNowMin      = (nSteps == 0) ? 0. : min(tmsNowMin, tmsnow);

  // Get random number to choose a path.
  double RN = rndmPtr->flat();
  // Set dummy process scale.
  newProcess.scale(0.0);
  // Generate all histories.
  History FullHistory( nSteps, 0.0, newProcess, Clustering(), mergingHooksPtr,
            (*beamAPtr), (*beamBPtr), particleDataPtr, infoPtr,
            trialPartonLevelPtr, coupSMPtr, true, true, true, true, 1.0, 0);
  // Project histories onto desired branches, e.g. only ordered paths.
  FullHistory.projectOntoDesiredHistories();

  // Do not apply cut if the configuration could not be projected onto an
  // underlying born configuration.
  bool applyCut = nSteps > 0 && FullHistory.select(RN)->nClusterings() > 0;

  // Enfore merging scale cut if the event did not pass the merging scale
  // criterion.
  bool enforceCutOnLHE  = flag("Merging:enforceCutOnLHE");
  if ( enforceCutOnLHE && applyCut && tmsnow < tmsval ) {
    loggerPtr->WARNING_MSG(
      "Les Houches Event fails merging scale cut. Rejecting event");
    if (!includeWGT) mergingHooksPtr->setWeightCKKWL(vector<double>(nWgts,0.));
    if ( includeWGT) infoPtr->weightContainerPtr->setWeightNominal(0.);
    if (applyVeto) return -1;
    else return 1;
  }

  // Check reclustering steps to correctly apply MPI.
  int nPerformed = 0;
  if ( nSteps > 0 && doUMEPSSubt
    && !FullHistory.getFirstClusteredEventAboveTMS( RN, nRecluster,
          newProcess, nPerformed, false ) ) {
    // Discard if the state could not be reclustered to a state above TMS.
    if (!includeWGT) mergingHooksPtr->setWeightCKKWL(vector<double>(nWgts,0.));
    if ( includeWGT) infoPtr->weightContainerPtr->setWeightNominal(0.);
    if (applyVeto) return -1;
    else return 1;
  }

  mergingHooksPtr->nMinMPI(nSteps - nPerformed);

  // Calculate CKKWL weight:
  // Perform reweighting with Sudakov factors, save alpha_s ratios and
  // PDF ratio weights.
  if ( doUMEPSTree ) {
    wgt = FullHistory.weightUMEPSTree( trialPartonLevelPtr,
      mergingHooksPtr->AlphaS_FSR(), mergingHooksPtr->AlphaS_ISR(),
      mergingHooksPtr->AlphaEM_FSR(), mergingHooksPtr->AlphaEM_ISR(), RN);
  } else {
    wgt = FullHistory.weightUMEPSSubt( trialPartonLevelPtr,
      mergingHooksPtr->AlphaS_FSR(), mergingHooksPtr->AlphaS_ISR(),
      mergingHooksPtr->AlphaEM_FSR(), mergingHooksPtr->AlphaEM_ISR(), RN);
  }

  // Event with production scales set for further (trial) showering
  // and starting conditions for the shower.
  if ( doUMEPSTree ) FullHistory.getStartingConditions( RN, process );
  // Do reclustering (looping) steps.
  else FullHistory.getFirstClusteredEventAboveTMS( RN, nRecluster, process,
    nPerformed, true );

  // Allow to dampen histories in which the lowest multiplicity reclustered
  // state does not pass the lowest multiplicity cut of the matrix element
  double dampWeight = mergingHooksPtr->dampenIfFailCuts(
           FullHistory.lowestMultProc(RN) );
  // Save the weight of the event for histogramming. Only change the
  // event weight after trial shower on the matrix element
  // multiplicity event (= in doVetoStep)
  for (double& wgti: wgt) wgti *= dampWeight;

  // Save the weight of the event for histogramming.
  if (!includeWGT) mergingHooksPtr->setWeightCKKWL(wgt);

  // Update the event weight.
  if ( includeWGT) {
    // In this case, central merging weight goes into nominal weight, all
    // variations are saved relative to central merging weight
    vector<double> relWgt({1.});
    for (int iVar = 1; iVar < nWgts; ++iVar) {
      relWgt.push_back(wgt[0] != 0 ? wgt[iVar]/wgt[0] : 0.);
      if (abs(wgt[iVar]) > MINWGT && wgt[0] < MINWGT) {
        loggerPtr->WARNING_MSG("cannot normalize merging weight to zero",
          "try Merging:includeWeightInXsection off");
      }
    }
    infoPtr->weightContainerPtr->
      setWeightNominal(infoPtr->weight()*wgt[0]);
    mergingHooksPtr->setWeightCKKWL(relWgt);
  }

  // Set QCD 2->2 starting scale different from arbitrary scale in LHEF!
  // --> Set to minimal mT of partons.
  double muf = process[0].e();
  for ( int i=0; i < process.size(); ++i )
  if ( process[i].isFinal()
    && (process[i].colType() != 0 || process[i].id() == 22 ) ) {
    muf = min( muf, abs(process[i].mT()) );
  }

  // For pure QCD dijet events (only!), set the process scale to the
  // transverse momentum of the outgoing partons.
  // Calculate number of clustering steps.
  int nStepsNew = mergingHooksPtr->getNumberOfClusteringSteps( process );
  if ( nStepsNew == 0
    && ( mergingHooksPtr->getProcessString().compare("pp>jj") == 0
      || mergingHooksPtr->getProcessString().compare("pp>aj") == 0) )
    process.scale(muf);

  // Reset hard process candidates (changed after clustering a parton).
  mergingHooksPtr->storeHardProcessCandidates( process );
  // If necessary, reattach resonance decay products.
  mergingHooksPtr->reattachResonanceDecays(process);

  // Allow merging hooks to remove emissions from now on.
  mergingHooksPtr->doIgnoreEmissions(false);

  // If no-emission probability is zero.
  if ( applyVeto && wgt[0] == 0. ) return 0;

  // Done
  return 1;

}

//--------------------------------------------------------------------------

// Function to perform NL3 NLO merging on this event.

int Merging::mergeProcessNL3( Event& process) {

  // Initialise which part of NL3 merging is applied.
  bool doNL3Tree = flag("Merging:doNL3Tree");
  bool doNL3Loop = flag("Merging:doNL3Loop");
  bool doNL3Subt = flag("Merging:doNL3Subt");

  // Ensure that hooks (NL3 part) to not remove emissions.
  mergingHooksPtr->doIgnoreEmissions(true);
  // Ensure that hooks (CKKWL part) to not veto events in trial showers.
  mergingHooksPtr->doIgnoreStep(true);
  // For pp > h, allow cut on state, so that underlying processes
  // can be clustered to gg > h
  if ( mergingHooksPtr->getProcessString().compare("pp>h") == 0)
    mergingHooksPtr->allowCutOnRecState(true);
  // For now, prefer construction of ordered histories.
  mergingHooksPtr->orderHistories(true);

  // Reset weight of the event
  int nWgts = mergingHooksPtr->nWgts;
  vector<double> wgt( nWgts, 1. );
  mergingHooksPtr->setWeightCKKWL(wgt);
  // Reset the O(alphaS)-term of the CKKW-L weight.
  vector<double> wgtFIRST( nWgts, 0. );
  mergingHooksPtr->setWeightFIRST(wgtFIRST);
  mergingHooksPtr->muMI(-1.);

  // Prepare process record for merging. If Pythia has already decayed
  // resonances used to define the hard process, remove resonance decay
  // products.
  Event newProcess( mergingHooksPtr->bareEvent( process, true) );
  // Store candidates for the splitting V -> qqbar'
  mergingHooksPtr->storeHardProcessCandidates( newProcess);

  // Check if event passes the merging scale cut.
  double tmsval  = mergingHooksPtr->tms();
  // Get merging scale in current event.
  double tmsnow  = mergingHooksPtr->tmsNow( newProcess );
  // Calculate number of clustering steps
  int nSteps = mergingHooksPtr->getNumberOfClusteringSteps( newProcess, true);
  int nRequested = mergingHooksPtr->nRequested();

  // Too few steps can be possible if a chain of resonance decays has been
  // removed. In this case, reject this event, since it will be handled in
  // lower-multiplicity samples.
  if (nSteps < nRequested) {
    mergingHooksPtr->setWeightCKKWL(vector<double>( nWgts, 0.));
    mergingHooksPtr->setWeightFIRST(vector<double>( nWgts, 0.));
    return -1;
  }

  // Reset the minimal tms value, if necessary.
  tmsNowMin = (nSteps == 0) ? 0. : min(tmsNowMin, tmsnow);

  // Enfore merging scale cut if the event did not pass the merging scale
  // criterion.
  bool enforceCutOnLHE  = flag("Merging:enforceCutOnLHE");
  if ( enforceCutOnLHE && nSteps > 0 && nSteps == nRequested
    && tmsnow < tmsval ) {
    loggerPtr->WARNING_MSG(
      "Les Houches Event fails merging scale cut. Rejecting event");
    mergingHooksPtr->setWeightCKKWL(vector<double>(nWgts, 0.));
    mergingHooksPtr->setWeightFIRST(vector<double>(nWgts, 0.));
    return -1;
  }

  // Get random number to choose a path.
  double RN = rndmPtr->flat();
  // Set dummy process scale.
  newProcess.scale(0.0);
  // Generate all histories
  History FullHistory( nSteps, 0.0, newProcess, Clustering(), mergingHooksPtr,
            (*beamAPtr), (*beamBPtr), particleDataPtr, infoPtr,
            trialPartonLevelPtr, coupSMPtr, true, true, true, true, 1.0, 0);
  // Project histories onto desired branches, e.g. only ordered paths.
  FullHistory.projectOntoDesiredHistories();

  // Discard states that cannot be projected unto a state with one less jet.
  if ( nSteps > 0 && doNL3Subt
    && FullHistory.select(RN)->nClusterings() == 0 ){
    mergingHooksPtr->setWeightCKKWL(vector<double>(nWgts, 0.));
    mergingHooksPtr->setWeightFIRST(vector<double>(nWgts, 0.));
    return -1;
  }

  // Potentially recluster real emission jets for powheg input containing
  // "too many" jets, i.e. real-emission kinematics.
  bool containsRealKin = nSteps > nRequested && nSteps > 0;

  // Perform one reclustering for real emission kinematics, then apply merging
  // scale cut on underlying Born kinematics.
  if ( containsRealKin ) {
    Event dummy = Event();
    // Initialise temporary output of reclustering.
    dummy.clear();
    dummy.init( "(hard process-modified)", particleDataPtr );
    dummy.clear();
    // Recluster once.
    if ( !FullHistory.getClusteredEvent( RN, nSteps, dummy )) {
      mergingHooksPtr->setWeightCKKWL( vector<double>( nWgts, 0. ) );
      mergingHooksPtr->setWeightFIRST( vector<double>( nWgts, 0. ) );
      return -1;
    }
    double tnowNew  = mergingHooksPtr->tmsNow( dummy );
    // Veto if underlying Born kinematics do not pass merging scale cut.
    if ( enforceCutOnLHE && nRequested > 0 && tnowNew < tmsval ) {
      mergingHooksPtr->setWeightCKKWL(vector<double>( nWgts, 0. ) );
      mergingHooksPtr->setWeightFIRST( vector<double>( nWgts, 0. ) );
      return -1;
    }
  }

  // Remember number of jets, to include correct MPI no-emission probabilities.
  if ( doNL3Subt || containsRealKin ) mergingHooksPtr->nMinMPI(nSteps - 1);
  else mergingHooksPtr->nMinMPI(nSteps);

  // Calculate weight
  // Do LO or first part of NLO tree-level reweighting
  if( doNL3Tree ) {
    // Perform reweighting with Sudakov factors, save as ratios and
    // PDF ratio weights
    wgt = FullHistory.weightNL3Tree( trialPartonLevelPtr,
      mergingHooksPtr->AlphaS_FSR(), mergingHooksPtr->AlphaS_ISR(),
      mergingHooksPtr->AlphaEM_FSR(), mergingHooksPtr->AlphaEM_ISR(), RN);
  } else if( doNL3Loop || doNL3Subt ) {
    // No reweighting, just set event scales properly and incorporate MPI
    // no-emission probabilities.
    wgt = FullHistory.weightNL3Loop( trialPartonLevelPtr, RN);
  }

  // Event with production scales set for further (trial) showering
  // and starting conditions for the shower
  if ( !doNL3Subt && !containsRealKin )
    FullHistory.getStartingConditions(RN, process);
  // For sutraction of nSteps-additional resolved partons from
  // the nSteps-1 parton phase space, recluster the last parton
  // in nSteps-parton events, and sutract later
  else {
    // Function to return the reclustered event
    if ( !FullHistory.getClusteredEvent( RN, nSteps, process )) {
      mergingHooksPtr->setWeightCKKWL(vector<double> (nWgts, 0.));
      mergingHooksPtr->setWeightFIRST(vector<double> (nWgts, 0.));
      return -1;
    }
  }

  // Allow to dampen histories in which the lowest multiplicity reclustered
  // state does not pass the lowest multiplicity cut of the matrix element
  double dampWeight = mergingHooksPtr->dampenIfFailCuts(
           FullHistory.lowestMultProc(RN) );
  // Save the weight of the event for histogramming. Only change the
  // event weight after trial shower on the matrix element
  // multiplicity event (= in doVetoStep)
  for (double& wgti: wgt) wgti *= dampWeight;

  // For tree level samples in NL3, rescale with k-Factor
  if (doNL3Tree ){
    // Find k-factor
    double kFactor = 1.;
    if( nSteps > mergingHooksPtr->nMaxJetsNLO() )
      kFactor = mergingHooksPtr->kFactor( mergingHooksPtr->nMaxJetsNLO() );
    else kFactor = mergingHooksPtr->kFactor(nSteps);
    // For NLO merging, rescale CKKW-L weight with k-factor
    for (double& wgti: wgt) wgti *= kFactor;
  }

  // Save the weight of the event for histogramming
  mergingHooksPtr->setWeightCKKWL(wgt);

  // Check if we need to subtract the O(\alpha_s)-term. If the number
  // of additional partons is larger than the number of jets for
  // which loop matrix elements are available, do standard CKKW-L
  bool doOASTree = doNL3Tree && nSteps <= mergingHooksPtr->nMaxJetsNLO();

  // Now begin NLO part for tree-level events
  if ( doOASTree ) {
    // Calculate the O(\alpha_s)-term of the CKKWL weight
    wgtFIRST = FullHistory.weightNL3First( trialPartonLevelPtr,
      mergingHooksPtr->AlphaS_FSR(), mergingHooksPtr->AlphaS_ISR(),
      mergingHooksPtr->AlphaEM_FSR(), mergingHooksPtr->AlphaEM_ISR(), RN,
      rndmPtr );
    // If necessary, also dampen the O(\alpha_s)-term
    for (double& wFi: wgtFIRST) wFi *= dampWeight;
    // Set the subtractive weight to the value calculated so far
    mergingHooksPtr->setWeightFIRST(wgtFIRST);
    // Subtract the O(\alpha_s)-term from the CKKW-L weight
    // If PDF contributions have not been included, subtract these later
    for (int iVar = 0; iVar < nWgts; ++iVar)
      wgt[iVar] = wgt[iVar] - wgtFIRST[iVar];
  }

  // Set qcd 2->2 starting scale different from arbirtrary scale in LHEF!
  // --> Set to pT of partons
  double pT = 0.;
  for( int i=0; i < process.size(); ++i)
    if(process[i].isFinal() && process[i].colType() != 0) {
      pT = sqrt(pow(process[i].px(),2) + pow(process[i].py(),2));
      break;
    }
  // For pure QCD dijet events (only!), set the process scale to the
  // transverse momentum of the outgoing partons.
  if ( nSteps == 0
    && mergingHooksPtr->getProcessString().compare("pp>jj") == 0)
    process.scale(pT);

  // Reset hard process candidates (changed after clustering a parton).
  mergingHooksPtr->storeHardProcessCandidates( process );
  // If necessary, reattach resonance decay products.
  mergingHooksPtr->reattachResonanceDecays(process);

  // Allow merging hooks (NL3 part) to remove emissions from now on.
  mergingHooksPtr->doIgnoreEmissions(false);
  // Allow merging hooks (CKKWL part) to veto events from now on.
  mergingHooksPtr->doIgnoreStep(false);

  // Done
  return 1;

}

//--------------------------------------------------------------------------

// Function to perform UNLOPS merging on this event.

int Merging::mergeProcessUNLOPS( Event& process) {

  // Initialise which part of UNLOPS merging is applied.
  bool nloTilde         = flag("Merging:doUNLOPSTilde");
  bool doUNLOPSTree     = flag("Merging:doUNLOPSTree");
  bool doUNLOPSLoop     = flag("Merging:doUNLOPSLoop");
  bool doUNLOPSSubt     = flag("Merging:doUNLOPSSubt");
  bool doUNLOPSSubtNLO  = flag("Merging:doUNLOPSSubtNLO");
  // Save number of looping steps
  mergingHooksPtr->nReclusterSave = mode("Merging:nRecluster");
  int nRecluster        = mode("Merging:nRecluster");

  // Ensure that merging hooks to not remove emissions
  mergingHooksPtr->doIgnoreEmissions(true);
  // For now, prefer construction of ordered histories.
  mergingHooksPtr->orderHistories(true);
  // For pp > h, allow cut on state, so that underlying processes
  // can be clustered to gg > h
  if ( mergingHooksPtr->getProcessString().compare("pp>h") == 0)
    mergingHooksPtr->allowCutOnRecState(true);

  // Reset weight of the event.
  int nWgts = mergingHooksPtr->nWgts;
  vector<double> wgt( nWgts, 1. );
  mergingHooksPtr->setWeightCKKWL(wgt);
  // Reset the O(alphaS)-term of the UMEPS weight.
  vector<double> wgtFIRST( nWgts, 0. );
  mergingHooksPtr->setWeightFIRST(wgtFIRST);
  mergingHooksPtr->muMI(-1.);

  // Vectors for UNLOPS-P and UNLOPS-PC weights
  vector<double> wgtP( nWgts, 1. );
  vector<double> wgtPC( nWgts, 1. );
  vector<double> wgtFIRSTP( nWgts, 0. );
  vector<double> wgtFIRSTPC( nWgts, 0. );

  // Check if scheme variations activated, and if so, reset
  bool doSchemeVariation = settingsPtr->flag("Merging:doSchemeVariation");
  if (doSchemeVariation) {
    infoPtr->weightContainerPtr->weightsMerging.weightValuesP = wgtP;
    infoPtr->weightContainerPtr->weightsMerging.weightValuesPC = wgtPC;
    infoPtr->weightContainerPtr->weightsMerging.weightValuesFirstP
      = wgtFIRSTP;
    infoPtr->weightContainerPtr->weightsMerging.weightValuesFirstPC
      = wgtFIRSTPC;
  }


  // Prepare process record for merging. If Pythia has already decayed
  // resonances used to define the hard process, remove resonance decay
  // products.
  Event newProcess( mergingHooksPtr->bareEvent( process, true) );
  // Store candidates for the splitting V -> qqbar'
  mergingHooksPtr->storeHardProcessCandidates( newProcess );

  // Check if event passes the merging scale cut.
  double tmsval  = mergingHooksPtr->tms();
  // Get merging scale in current event.
  double tmsnow  = mergingHooksPtr->tmsNow( newProcess );
  // Calculate number of clustering steps
  int nSteps = mergingHooksPtr->getNumberOfClusteringSteps( newProcess, true);
  int nRequested = mergingHooksPtr->nRequested();

  // Check if hard event cut should be applied later.
  bool allowReject = flag("Merging:applyVeto");

  // Too few steps can be possible if a chain of resonance decays has been
  // removed. In this case, reject this event, since it will be handled in
  // lower-multiplicity samples.
  if (nSteps < nRequested) {
    loggerPtr->WARNING_MSG("not enough partons in LHE after removing"
    " decay products");
    mergingHooksPtr->setWeightCKKWL(vector<double>(nWgts,0.));
    mergingHooksPtr->setWeightFIRST(vector<double>(nWgts,0.));
    if (doSchemeVariation) {
      infoPtr->weightContainerPtr->weightsMerging.weightValuesP =
      infoPtr->weightContainerPtr->weightsMerging.weightValuesPC =
      infoPtr->weightContainerPtr->weightsMerging.weightValuesFirstP =
      infoPtr->weightContainerPtr->weightsMerging.weightValuesFirstPC =
        vector<double>(nWgts,0.);
    }
    return ((allowReject)? -1 : 1);
  }

  // Reset the minimal tms value, if necessary.
  tmsNowMin = (nSteps == 0) ? 0. : min(tmsNowMin, tmsnow);

  // Get random number to choose a path.
  double RN = rndmPtr->flat();
  // Set dummy process scale.
  newProcess.scale(0.0);
  // Generate all histories
  History FullHistory( nSteps, 0.0, newProcess, Clustering(), mergingHooksPtr,
            (*beamAPtr), (*beamBPtr), particleDataPtr, infoPtr,
            trialPartonLevelPtr, coupSMPtr, true, true, true, true, 1.0, 0);
  // Project histories onto desired branches, e.g. only ordered paths.
  FullHistory.projectOntoDesiredHistories();

  // Do not apply cut if the configuration could not be projected onto an
  // underlying born configuration.
  bool applyCut = nSteps > 0 && FullHistory.select(RN)->nClusterings() > 0;

  // Enfore merging scale cut if the event did not pass the merging scale
  // criterion.
  bool enforceCutOnLHE  = flag("Merging:enforceCutOnLHE");
  if ( enforceCutOnLHE && applyCut && nSteps == nRequested
    && tmsnow < tmsval ) {
    loggerPtr->WARNING_MSG(
      "Les Houches Event fails merging scale cut. Rejecting event");
    mergingHooksPtr->setWeightCKKWL(vector<double>(nWgts,0.));
    mergingHooksPtr->setWeightFIRST(vector<double>(nWgts,0.));
    if (doSchemeVariation) {
      infoPtr->weightContainerPtr->weightsMerging.weightValuesP =
      infoPtr->weightContainerPtr->weightsMerging.weightValuesPC =
      infoPtr->weightContainerPtr->weightsMerging.weightValuesFirstP =
      infoPtr->weightContainerPtr->weightsMerging.weightValuesFirstPC =
        vector<double>(nWgts,0.);
    }
    return ((allowReject)? -1 : 1);
  }

  // Potentially recluster real emission jets for powheg input containing
  // "too many" jets, i.e. real-emission kinematics.
  bool containsRealKin = nSteps > nRequested && nSteps > 0;
  if ( containsRealKin ) nRecluster += nSteps - nRequested;

  // Remove real emission events without underlying Born configuration from
  // the loop sample, since such states will be taken care of by tree-level
  // samples.
  bool allowIncompleteReal = flag("Merging:allowIncompleteHistoriesInReal");
  if ( doUNLOPSLoop && containsRealKin && !allowIncompleteReal
    && FullHistory.select(RN)->nClusterings() == 0 ) {
    mergingHooksPtr->setWeightCKKWL(vector<double>(nWgts,0.));
    mergingHooksPtr->setWeightFIRST(vector<double>(nWgts,0.));
    if (doSchemeVariation) {
      infoPtr->weightContainerPtr->weightsMerging.weightValuesP =
      infoPtr->weightContainerPtr->weightsMerging.weightValuesPC =
      infoPtr->weightContainerPtr->weightsMerging.weightValuesFirstP =
      infoPtr->weightContainerPtr->weightsMerging.weightValuesFirstPC =
        vector<double>(nWgts,0.);
    }
    return ((allowReject)? -1 : 1);
  }

  // Discard if the state could not be reclustered to any state above TMS.
  int nPerformed = 0;
  if ( nSteps > 0 && !allowIncompleteReal
    && ( doUNLOPSSubt || doUNLOPSSubtNLO || containsRealKin )
    && !FullHistory.getFirstClusteredEventAboveTMS( RN, nRecluster,
          newProcess, nPerformed, false ) ) {
    mergingHooksPtr->setWeightCKKWL(vector<double>(nWgts, 0.));
    mergingHooksPtr->setWeightFIRST(vector<double>(nWgts, 0.));
    if (doSchemeVariation) {
      infoPtr->weightContainerPtr->weightsMerging.weightValuesP =
      infoPtr->weightContainerPtr->weightsMerging.weightValuesPC =
      infoPtr->weightContainerPtr->weightsMerging.weightValuesFirstP =
      infoPtr->weightContainerPtr->weightsMerging.weightValuesFirstPC =
        vector<double>(nWgts,0.);
    }
    return ((allowReject)? -1 : 1);
  }

  // Check reclustering steps to correctly apply MPI.
  mergingHooksPtr->nMinMPI(nSteps - nPerformed);

  // Perform one reclustering for real emission kinematics, then apply
  // merging scale cut on underlying Born kinematics.
  if ( containsRealKin ) {
    Event dummy = Event();
    // Initialise temporary output of reclustering.
    dummy.clear();
    dummy.init( "(hard process-modified)", particleDataPtr );
    dummy.clear();
    // Recluster once.
    FullHistory.getClusteredEvent( RN, nSteps, dummy );
    double tnowNew  = mergingHooksPtr->tmsNow( dummy );
    // Veto if underlying Born kinematics do not pass merging scale cut.
    if ( enforceCutOnLHE && nRequested > 0 && tnowNew < tmsval ) {
      loggerPtr->WARNING_MSG(
        "Les Houches Event fails merging scale cut. Rejecting event");
      mergingHooksPtr->setWeightCKKWL(vector<double>(nWgts,0.));
      mergingHooksPtr->setWeightFIRST(vector<double>(nWgts,0.));
      if (doSchemeVariation) {
        infoPtr->weightContainerPtr->weightsMerging.weightValuesP =
        infoPtr->weightContainerPtr->weightsMerging.weightValuesPC =
        infoPtr->weightContainerPtr->weightsMerging.weightValuesFirstP =
        infoPtr->weightContainerPtr->weightsMerging.weightValuesFirstPC =
          vector<double>(nWgts,0.);
      }
      return ((allowReject)? -1 : 1);
    }
  }

  // New UNLOPS strategy based on UN2LOPS.
  int depth = (!doSchemeVariation) ? -1 : ( (containsRealKin) ?
      nSteps-1 : nSteps);

  // Calculate weights.
  // Do LO or first part of NLO tree-level reweighting
  if( doUNLOPSTree ) {
    // Perform reweighting with Sudakov factors, save as ratios and
    // PDF ratio weights
    wgt = FullHistory.weightUNLOPSTree( trialPartonLevelPtr,
            mergingHooksPtr->AlphaS_FSR(), mergingHooksPtr->AlphaS_ISR(),
            mergingHooksPtr->AlphaEM_FSR(), mergingHooksPtr->AlphaEM_ISR(),
            RN, depth);
  } else if( doUNLOPSLoop ) {
    // Set event scales properly, reweight for new UNLOPS
    wgt = FullHistory.weightUNLOPSLoop( trialPartonLevelPtr,
            mergingHooksPtr->AlphaS_FSR(), mergingHooksPtr->AlphaS_ISR(),
            mergingHooksPtr->AlphaEM_FSR(), mergingHooksPtr->AlphaEM_ISR(),
            RN, depth);
  } else if( doUNLOPSSubtNLO ) {
    // Set event scales properly, reweight for new UNLOPS
    wgt = FullHistory.weightUNLOPSSubtNLO( trialPartonLevelPtr,
            mergingHooksPtr->AlphaS_FSR(), mergingHooksPtr->AlphaS_ISR(),
            mergingHooksPtr->AlphaEM_FSR(), mergingHooksPtr->AlphaEM_ISR(),
            RN, depth);
  } else if( doUNLOPSSubt ) {
    // Perform reweighting with Sudakov factors, save as ratios and
    // PDF ratio weights
    wgt = FullHistory.weightUNLOPSSubt( trialPartonLevelPtr,
            mergingHooksPtr->AlphaS_FSR(), mergingHooksPtr->AlphaS_ISR(),
            mergingHooksPtr->AlphaEM_FSR(), mergingHooksPtr->AlphaEM_ISR(),
            RN, depth);
  }

  // Set weights for UNLOPS-P and UNLOPS-PC
  if (doSchemeVariation && (doUNLOPSLoop || doUNLOPSSubtNLO)) {
    wgtPC = wgt;
    wgt = mergingHooksPtr->individualWeights.mpiWeightSave;
    wgtP = mergingHooksPtr->getSudakovWeight();
  }

  // Event with production scales set for further (trial) showering
  // and starting conditions for the shower.
  if (!doUNLOPSSubt && !doUNLOPSSubtNLO && !containsRealKin )
    FullHistory.getStartingConditions(RN, process);
  // Do reclustering (looping) steps.
  else FullHistory.getFirstClusteredEventAboveTMS( RN, nRecluster, process,
    nPerformed, true );

  // Allow to dampen histories in which the lowest multiplicity reclustered
  // state does not pass the lowest multiplicity cut of the matrix element
  double dampWeight = mergingHooksPtr->dampenIfFailCuts(
           FullHistory.lowestMultProc(RN) );
  // Save the weight of the event for histogramming. Only change the
  // event weight after trial shower on the matrix element
  // multiplicity event (= in doVetoStep)
  for (double& wgti: wgt) wgti *= dampWeight;
  if (doSchemeVariation && (doUNLOPSLoop || doUNLOPSSubtNLO)) {
    for (double& wgti: wgtP) wgti *= dampWeight;
    for (double& wgti: wgtPC) wgti *= dampWeight;
  }

  // For tree-level or subtractive samples, rescale with k-Factor
  if ( doUNLOPSTree || doUNLOPSSubt ){
    // Find k-factor
    double kFactor = 1.;
    if ( nSteps > mergingHooksPtr->nMaxJetsNLO() )
      kFactor = mergingHooksPtr->kFactor( mergingHooksPtr->nMaxJetsNLO() );
    else kFactor = mergingHooksPtr->kFactor(nSteps);
    // For NLO merging, rescale CKKW-L weight with k-factor
    for (double& wgti: wgt) wgti *= (nRecluster == 2 && nloTilde) ?
      1. : kFactor;
  }

  // Save the weight of the event for histogramming
  mergingHooksPtr->setWeightCKKWL(wgt);
  if ( doSchemeVariation ) {
    if (doUNLOPSLoop || doUNLOPSSubtNLO) {
      infoPtr->weightContainerPtr->weightsMerging.weightValuesP = wgtP;
      infoPtr->weightContainerPtr->weightsMerging.weightValuesPC = wgtPC;
    } else {
      infoPtr->weightContainerPtr->weightsMerging.weightValuesP = wgtP = wgt;
      infoPtr->weightContainerPtr->weightsMerging.weightValuesPC = wgtPC = wgt;
    }
  }

  // Check if we need to subtract the O(\alpha_s)-term. If the number
  // of additional partons is larger than the number of jets for
  // which loop matrix elements are available, do standard UMEPS.
  int nMaxNLO     = mergingHooksPtr->nMaxJetsNLO();
  bool doOASTree  = doUNLOPSTree && nSteps <= nMaxNLO;
  bool doOASSubt  = doUNLOPSSubt && nSteps <= nMaxNLO+1 && nSteps > 0;

  // Now begin NLO part for tree-level events
  if ( doOASTree || doOASSubt ) {

    // Decide on which order to expand to.
    int order = ( nSteps > 0 && nSteps <= nMaxNLO) ? 1 : -1;

    // Exclusive inputs:
    // Subtract only the O(\alpha_s^{n+0})-term from the tree-level
    // subtraction, if we're at the highest NLO multiplicity (nMaxNLO).
    if ( nloTilde && doUNLOPSSubt && nRecluster == 1
      && nSteps == nMaxNLO+1 ) order = 0;

    // Exclusive inputs:
    // Do not remove the O(as)-term if the number of reclusterings
    // exceeds the number of NLO jets, or if more clusterings have
    // been performed.
    if (nloTilde && doUNLOPSSubt && ( nSteps > nMaxNLO+1
      || (nSteps == nMaxNLO+1 && nPerformed != nRecluster) ))
        order = -1;

    // Calculate terms in expansion of the CKKW-L weight.
    wgtFIRST = FullHistory.weightUNLOPSFirst( order,
      trialPartonLevelPtr, mergingHooksPtr->AlphaS_FSR(),
      mergingHooksPtr->AlphaS_ISR(), mergingHooksPtr->AlphaEM_FSR(),
      mergingHooksPtr->AlphaEM_ISR(), RN, rndmPtr );

    // Exclusive inputs:
    // Subtract the O(\alpha_s^{n+1})-term from the tree-level
    // subtraction, not the O(\alpha_s^{n+0})-terms.
    if ( nloTilde && doUNLOPSSubt && nRecluster == 1
      && nPerformed == nRecluster && nSteps <= nMaxNLO )
      for (double& wFi: wgtFIRST) wFi += 1.;

    // If necessary, also dampen the O(\alpha_s)-term
    for (double& wFi: wgtFIRST) wFi *= dampWeight;

    // Set the subtractive weight to the value calculated so far
    mergingHooksPtr->setWeightFIRST(wgtFIRST);
    if (doSchemeVariation) {
      vector<double> sudFact = mergingHooksPtr->getSudakovWeight();
      vector<double> couplingFact = mergingHooksPtr->getCouplingWeight();
      for (int iWgt = 0; iWgt < nWgts; ++iWgt) {
        double wgtF = wgtFIRST[iWgt]*sudFact[iWgt];
        wgtFIRSTP[iWgt] = wgtF;
        wgtF *= couplingFact[iWgt];
        wgtFIRSTPC[iWgt] = wgtF;
      }
      infoPtr->weightContainerPtr->weightsMerging.weightValuesFirstP
        = wgtFIRSTP;
      infoPtr->weightContainerPtr->weightsMerging.weightValuesFirstPC
        = wgtFIRSTPC;
    }

    // Subtract the O(\alpha_s)-term from the CKKW-L weight
    // If PDF contributions have not been included, subtract these later
    // New UNLOPS based on UN2LOPS.
    //if (doUNLOPS2 && order > -1) wgt = -wgt*(wgtFIRST-1.);
    //else if (order > -1) wgt = wgt - wgtFIRST;
    if (order > -1) {
      for (int i = 0; i < nWgts; ++i) {
        wgt[i] = wgt[i] - wgtFIRST[i];
        if (doSchemeVariation) {
          wgtP[i] = wgtP[i] - wgtFIRSTP[i];
          wgtPC[i] = wgtPC[i] - wgtFIRSTPC[i];
        }
      }
    }
  }
  // If no first order weight needs to be subtracted, set it anyway
  else if (doSchemeVariation) {
    infoPtr->weightContainerPtr->weightsMerging.weightValuesFirstP
      = wgtFIRSTP;
    infoPtr->weightContainerPtr->weightsMerging.weightValuesFirstPC
      = wgtFIRSTPC;
  }

  // Set QCD 2->2 starting scale different from arbitrary scale in LHEF!
  // --> Set to minimal mT of partons.
  int nFinal = 0;
  double muf = process[0].e();
  for ( int i=0; i < process.size(); ++i )
  if ( process[i].isFinal()
    && (process[i].colType() != 0 || process[i].id() == 22 ) ) {
    nFinal++;
    muf = min( muf, abs(process[i].mT()) );
  }
  // For pure QCD dijet events (only!), set the process scale to the
  // transverse momentum of the outgoing partons.
  if ( nSteps == 0 && nFinal == 2
    && ( mergingHooksPtr->getProcessString().compare("pp>jj") == 0
      || mergingHooksPtr->getProcessString().compare("pp>aj") == 0) )
    process.scale(muf);

  // Reset hard process candidates (changed after clustering a parton).
  mergingHooksPtr->storeHardProcessCandidates( process );

  // Check if resonance structure has been changed
  //  (e.g. because of clustering W/Z/gluino)
  vector <int> oldResonance;
  for ( int i=0; i < newProcess.size(); ++i )
    if ( newProcess[i].status() == 22 )
      oldResonance.push_back(newProcess[i].id());
  vector <int> newResonance;
  for ( int i=0; i < process.size(); ++i )
    if ( process[i].status() == 22 )
      newResonance.push_back(process[i].id());
  // Compare old and new resonances
  for ( int i=0; i < int(oldResonance.size()); ++i )
    for ( int j=0; j < int(newResonance.size()); ++j )
      if ( newResonance[j] == oldResonance[i] ) {
        oldResonance[i] = 99;
        break;
      }
  bool hasNewResonances = (newResonance.size() != oldResonance.size());
  for ( int i=0; i < int(oldResonance.size()); ++i )
    hasNewResonances = (oldResonance[i] != 99);

  // If necessary, reattach resonance decay products.
  if (!hasNewResonances) mergingHooksPtr->reattachResonanceDecays(process);

  // Allow merging hooks to remove emissions from now on.
  mergingHooksPtr->doIgnoreEmissions(false);

  // If no-emission probability is zero.
  if (allowReject) {
    if (!doSchemeVariation && wgt[0] == 0.) return 0;
    if (doSchemeVariation && wgt[0] == 0. && wgtP[0] == 0. && wgtPC[0] == 0.)
      return 0;
  }

  // If the resonance structure of the process has changed due to reclustering,
  // redo the resonance decays in Pythia::next()
  if (hasNewResonances) return 2;

  // Done
  return 1;

}

//--------------------------------------------------------------------------

// Function to apply the merging scale cut on an input event.

bool Merging::cutOnProcess( Event& process) {

  // Save number of looping steps
  mergingHooksPtr->nReclusterSave = mode("Merging:nRecluster");

  // For now, prefer construction of ordered histories.
  mergingHooksPtr->orderHistories(true);
  // For pp > h, allow cut on state, so that underlying processes
  // can be clustered to gg > h
  if ( mergingHooksPtr->getProcessString().compare("pp>h") == 0)
    mergingHooksPtr->allowCutOnRecState(true);

  // Reset any incoming spins for W+-.
  if (mergingHooksPtr->doWeakClustering())
    for (int i = 0;i < process.size();++i)
      process[i].pol(9);

  // Prepare process record for merging. If Pythia has already decayed
  // resonances used to define the hard process, remove resonance decay
  // products.
  Event newProcess( mergingHooksPtr->bareEvent( process, true) );
  // Store candidates for the splitting V -> qqbar'
  mergingHooksPtr->storeHardProcessCandidates( newProcess );

  // Check if event passes the merging scale cut.
  double tmsval  = mergingHooksPtr->tms();
  // Get merging scale in current event.
  double tmsnow  = mergingHooksPtr->tmsNow( newProcess );
  // Calculate number of clustering steps
  int nSteps = mergingHooksPtr->getNumberOfClusteringSteps( newProcess, true);

  // Too few steps can be possible if a chain of resonance decays has been
  // removed. In this case, reject this event, since it will be handled in
  // lower-multiplicity samples.
  int nRequested = mergingHooksPtr->nRequested();
  if (nSteps < nRequested) return true;

  // Reset the minimal tms value, if necessary.
  tmsNowMin = (nSteps == 0) ? 0. : min(tmsNowMin, tmsnow);

  // Potentially recluster real emission jets for powheg input containing
  // "too many" jets, i.e. real-emission kinematics.
  bool containsRealKin = nSteps > nRequested && nSteps > 0;

  // Get random number to choose a path.
  double RN = rndmPtr->flat();
  // Set dummy process scale.
  newProcess.scale(0.0);
  // Generate all histories
  History FullHistory( nSteps, 0.0, newProcess, Clustering(), mergingHooksPtr,
            (*beamAPtr), (*beamBPtr), particleDataPtr, infoPtr,
            trialPartonLevelPtr, coupSMPtr, true, true, true, true, 1.0, 0);
  // Project histories onto desired branches, e.g. only ordered paths.
  FullHistory.projectOntoDesiredHistories();

  // Remove real emission events without underlying Born configuration from
  // the loop sample, since such states will be taken care of by tree-level
  // samples.
  bool allowIncompleteReal = flag("Merging:allowIncompleteHistoriesInReal");
  if ( containsRealKin && !allowIncompleteReal
    && FullHistory.select(RN)->nClusterings() == 0 )
    return true;

  // Cut if no history passes the cut on the lowest-multiplicity state.
  double dampWeight = mergingHooksPtr->dampenIfFailCuts(
           FullHistory.lowestMultProc(RN) );
  if ( dampWeight == 0. ) return true;

  // Do not apply cut if the configuration could not be projected onto an
  // underlying born configuration.
  if ( nSteps > 0 && FullHistory.select(RN)->nClusterings() == 0 )
    return false;

  // Now enfore merging scale cut if the event did not pass the merging scale
  // criterion.
  if ( nSteps > 0 && nSteps == nRequested && tmsnow < tmsval ) {
    loggerPtr->WARNING_MSG(
      "Les Houches Event fails merging scale cut. Rejecting event");
    return true;
  }

  // Check if more steps should be taken.
  int nFinalP = 0;
  int nFinalW = 0;
  Event coreProcess = Event();
  coreProcess.clear();
  coreProcess.init( "(hard process-modified)", particleDataPtr );
  coreProcess.clear();
  coreProcess = FullHistory.lowestMultProc(RN);
  for ( int i = 0; i < coreProcess.size(); ++i )
    if ( coreProcess[i].isFinal() ) {
      if ( coreProcess[i].colType() != 0 )
        nFinalP++;
      if ( coreProcess[i].idAbs() == 24 )
        nFinalW++;
    }

  bool complete = (FullHistory.select(RN)->nClusterings() == nSteps) ||
    ( mergingHooksPtr->doWeakClustering() && nFinalP == 2 && nFinalW == 0 );

  if ( !complete ) {
    loggerPtr->WARNING_MSG("No clusterings found. History incomplete");
  }

  // Done if no real-emission jets are present.
  if ( !containsRealKin ) return false;

  // Now cut on events that contain an additional real-emission jet.
  // Perform one reclustering for real emission kinematics, then apply merging
  // scale cut on underlying Born kinematics.
  Event dummy = Event();
  // Initialise temporary output of reclustering.
  dummy.clear();
  dummy.init( "(hard process-modified)", particleDataPtr );
  dummy.clear();
  // Recluster once.
  FullHistory.getClusteredEvent( RN, nSteps, dummy );
  double tnowNew = mergingHooksPtr->tmsNow( dummy );
  // Veto if underlying Born kinematics do not pass merging scale cut.
  if ( nRequested > 0 && tnowNew < tmsval ) {
    loggerPtr->WARNING_MSG(
      "Les Houches Event fails merging scale cut. Rejecting event");
    return true;
  }

  // Done if only interested in cross section estimate after cuts.
  return false;

}

//==========================================================================

} // end namespace Pythia8
