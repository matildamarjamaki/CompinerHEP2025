// HadronLevel.cc is a part of the PYTHIA event generator.
// Copyright (C) 2025 Torbjorn Sjostrand.
// PYTHIA is licenced under the GNU GPL v2 or later, see COPYING for details.
// Please respect the MCnet Guidelines, see GUIDELINES for details.

// Function definitions (not found in the header) for the HadronLevel class.

#include "Pythia8/HadronLevel.h"
#include "Pythia8/StringInteractions.h"

namespace Pythia8 {

//==========================================================================

// The PriorityNode class.
// Used to order scatterings and decays during hadronic rescattering.

//--------------------------------------------------------------------------

class HadronLevel::PriorityNode {

public:

  PriorityNode(int iDecayIn, Vec4 originIn)
    : i1(iDecayIn), i2(0), origin(originIn) {}
  PriorityNode(int i1In, int i2In, Vec4 originIn, Vec4 displaced1In,
    Vec4 displaced2In) : i1(i1In), i2(i2In), origin(originIn),
    displaced1(displaced1In), displaced2(displaced2In) {}

  bool isDecay() { return i2 == 0; }

  // Priority comparison to be used by priority_queue.
  // Note that lower t means higher priority!
  // @FUTURE: allow the user to pick the comparer (time-ordering problem)?
  bool operator<(const PriorityNode& r) const {
    return origin.e() > r.origin.e(); }

  int i1, i2;
  Vec4 origin, displaced1, displaced2;
};

//==========================================================================

// The HadronLevel class.

//--------------------------------------------------------------------------

// Constants: could be changed here if desired, but normally should not.

// Small safety mass used in string-end rapidity calculations.
const double HadronLevel::MTINY = 0.1;

//--------------------------------------------------------------------------

// Find settings. Initialize HadronLevel classes as required.


bool HadronLevel::init( TimeShowerPtr timesDecPtrIn, RHadronsPtr rHadronsPtrIn,
  LundFragmentationPtr fragPtrIn, vector<FragmentationModelPtr>* fragPtrsIn,
  DecayHandlerPtr decayHandlePtr, vector<int> handledParticles,
  StringIntPtr stringInteractionsPtrIn, PartonVertexPtr partonVertexPtrIn,
  SigmaLowEnergy& sigmaLowEnergyIn, NucleonExcitations& nucleonExcitationsIn) {

  // Store other input pointers.
  rHadronsPtr     = rHadronsPtrIn;
  timesDecPtr     = timesDecPtrIn;
  fragPtr         = fragPtrIn;
  fragPtrs        = fragPtrsIn;

  // Main flags.
  doHadronize     = flag("HadronLevel:Hadronize");
  doDecay         = flag("HadronLevel:Decay");
  doRescatter     = flag("HadronLevel:Rescatter");
  doBoseEinstein  = flag("HadronLevel:BoseEinstein");
  doDeuteronProd  = flag("HadronLevel:DeuteronProduction");
  doQED           = flag("HadronLevel:QED");

  // For junction processing.
  pNormJunction   = parm("StringFragmentation:pNormJunction");

  // Allow R-hadron formation.
  allowRH         = flag("RHadrons:allow");

  // Particles that should decay or not before Bose-Einstein stage.
  widthSepBE      = parm("BoseEinstein:widthSep");

  // Displace hadron vertices from parton impact-parameter picture.
  partonVertexPtr = partonVertexPtrIn;
  doPartonVertex  = flag("PartonVertex:setVertex");

  // Need string density information be collected?
  closePacking    = flag("ClosePacking:doClosePacking");

  // Initialize string interactions (Ropewalk and Flavour Ropes) if present.
  fragmentationModifierPtr =
    stringInteractionsPtrIn->getFragmentationModifier();
  stringRepulsionPtr = stringInteractionsPtrIn->getStringRepulsion();

  // Initialize auxiliary fragmentation classes.
  flavSel.init();
  pTSel.init();
  zSel.init();

  // Set the fragmentation weights container.
  if (wvec("VariationFrag:list").size() != 0)
    wgtsPtr = &infoPtr->weightContainerPtr->weightsFragmentation;

  // Initialize auxiliary administrative class.
  colConfig.init(infoPtr, &flavSel);

  // Initialize the fragmentation pointers.
  fragPtr->init(&flavSel, &pTSel, &zSel, fragmentationModifierPtr);
  for (auto &ptr: *fragPtrs)
    ptr->init(&flavSel, &pTSel, &zSel, fragmentationModifierPtr);

  // Initialize particle decays.
  decays.init(timesDecPtr, &flavSel, decayHandlePtr, handledParticles);

  // Initialize low-energy framework.
  sigmaLowEnergyPtr = &sigmaLowEnergyIn;
  nucleonExcitationsPtr = &nucleonExcitationsIn;
  lowEnergyProcess.init( &flavSel, fragPtr->stringFragPtr,
    fragPtr->ministringFragPtr, &sigmaLowEnergyIn, &nucleonExcitationsIn);

  // Initialize rescattering settings if applicable.
  if (doRescatter) {

    // Break if conflicting settings.
    if (doBoseEinstein) {
      loggerPtr->ERROR_MSG(
        "rescattering and Bose-Einstein cannot be on at the same time");
      return false;
    }

    // Read settings for rescattering.
    scatterManyTimes  = flag("Rescattering:scatterManyTimes");
    scatterQuickCheck = flag("Rescattering:quickCheck");
    scatterNeighbours = flag("Rescattering:nearestNeighbours");
    impactModel       = mode("Rescattering:impactModel");
    b2Max             = pow2(FM2MM * parm("Rescattering:bMax"));
    impactOpacity     = parm("Rescattering:opacity");
    widthSepRescatter = FMINV2GEV / parm("Rescattering:tau0RapidDecay");
    delayRegeneration = flag("Rescattering:delayRegeneration");
    tauRegeneration   = parm("Rescattering:tauRegeneration");
    boostDir          = mode("Rescattering:boostDir");
    boost             = parm("Rescattering:boost");
    doBoost           = boostDir > 0 && boost > 0.;
    useVelocityFrame  = flag("Rescattering:useVelocityFrame");
  }

  // Initialize BoseEinstein.
  boseEinstein.init();

  // Initialize DeuteronProduction.
  if (doDeuteronProd) deuteronProd.init();

  // Send flavour and z selection pointers to R-hadron machinery.
  rHadronsPtr->init(&flavSel, &pTSel, &zSel);

  // Initialize the colour tracing class.
  colTrace.init(loggerPtr);

  // Initialize the junction splitting class.
  junctionSplitting.init();

  // Done.
  return true;

}

//--------------------------------------------------------------------------

// Hadronize and decay the next parton-level.

bool HadronLevel::next( Event& event) {

  // Clear the fragmentation weights and flavor counts.
  if (wgtsPtr != nullptr) wgtsPtr->clear();

  // Store current event size to mark Parton Level content.
  event.savePartonLevelSize();
  int sizePartonLevel = event.size();

  // Colour-octet onia states must be decayed to singlet + gluon.
  if (!decayOctetOnia(event)) return false;

  // Set lifetimes for already existing hadrons, like onia.
  for (int i = 0; i < event.size(); ++i) if (event[i].isHadron())
    event[i].tau( event[i].tau0() * rndmPtr->exp() );

  // Remove junction structures.
  if (!junctionSplitting.checkColours(event)) {
    loggerPtr->ERROR_MSG("failed colour/junction check");
    return false;
  }

  // Possibility of hadronization inside decay, but then no BE second time.
  bool doBoseEinsteinNow = doBoseEinstein;
  bool doDeuteronProdNow = doDeuteronProd;
  bool decaysCausedHadronization;
  do {
    decaysCausedHadronization = false;
    doHadronizeVeto = false;

    // First part: string fragmentation.
    if (doHadronize) {

      // Find the complete colour singlet configuration of the event.
      // Keep junctions if we do shoving.
      if (!findSinglets( event, (stringRepulsionPtr != nullptr) ))
        return false;

      // Save list with rapidity pairs of the different string pieces.
      if (closePacking) {
        vector< vector< pair<double,double> > > rapPairs =
          rapidityPairs(event);
        colConfig.rapPairs = rapPairs;
      }

      // Let strings interact in rope hadronization treatment.
      // Do the shoving treatment.
      if ( stringRepulsionPtr ) {

        // Extract all string segments from the event and do the
        // string reulsion.
        stringRepulsionPtr->stringRepulsion(event, colConfig);

        // Find singlets again.
        iParton.resize(0);
        colConfig.clear();
        if (!findSinglets( event)) {
          loggerPtr->ERROR_MSG("ropes: failed 2nd singlet tracing.");
          return false;
        }
      }

      // Prepare for flavour ropes.
      if (fragmentationModifierPtr)
        fragmentationModifierPtr->initEvent(event, colConfig);

      // MiniStringFragmentation needs to know if the event is diffractive.
      bool isDiff = infoPtr->isDiffractiveA() || infoPtr->isDiffractiveB();

      // Fragment models that do not use the color systems,
      // e.g. HiddenValleyFragmentation and RHadrons.
      for (auto &ptr: *fragPtrs)
        if (!ptr->fragment(-1, colConfig, event, isDiff)) return false;

      // Process all colour singlet (sub)systems.
      for (int iSub = 0; iSub < colConfig.size(); ++iSub) {

        // Collect sequentially all partons in a colour singlet subsystem.
        colConfig.collect(iSub, event);
        int nBefFrag = event.size();

        // String fragmentation of each colour singlet (sub)system.
        for (auto &ptr: *fragPtrs)
          if (!ptr->fragment(iSub, colConfig, event, isDiff)) return false;

        // Displace hadron vertices transversely from parton MPI + shower.
        if (doPartonVertex) partonVertexPtr->vertexHadrons( nBefFrag, event);
      }
    }

    // Calculate the in-situ flavor weights.
    if (wgtsPtr != nullptr)
      for (auto &parms : wgtsPtr->weightParms[WeightsFragmentation::Flav])
        wgtsPtr->reweightValueByIndex(
          parms.second, wgtsPtr->flavWeight(parms.first));

    // The event can be vetoed here by the user.
    if (userHooksPtr && userHooksPtr->canVetoAfterHadronization() &&
      userHooksPtr->doVetoAfterHadronization(event) ) {
      doHadronizeVeto = true;
      return false;
    }

    // Second part: decays and hadronic rescattering.

    // If rescattering is on, perform rescattering.
    if (doRescatter) {
      decaysCausedHadronization = rescatter(event);
    // If rescattering is off, perform only decays.
    } else if (doDecay) {
      decaysCausedHadronization = decays.decayAll(event, widthSepBE);
    }

    // Third part: include Bose-Einstein effects among current particles.
    if (doBoseEinsteinNow) {
      if (!boseEinstein.shiftEvent(event)) return false;
      doBoseEinsteinNow = false;
    }

    // Fourth part: sequential decays also of long-lived particles.
    if (doDecay) {
      if (decays.decayAll(event))
        decaysCausedHadronization = true;
    }

    // Fifth part: deuteron production.
    if (doDeuteronProdNow) {
      if (!deuteronProd.combine(event)) return false;
      doDeuteronProdNow = false;
      decaysCausedHadronization = doDecay;
    }

  // Normally done first time around, but sometimes not.
  // (e.g. Upsilon decay can cause create unstable hadrons).
  } while (decaysCausedHadronization);

  // Allow for QED radiation to be added to the full post-hadronization system,
  // after particle decays.
  // Up to the shower to decide if everything was already handled during each
  // particle decay, and/or if there is more to do now (e.g., interleaved QED
  // radiation may be added only after all decay chains have been determined).
  // Note: leptons from the perturbative stage were already showered during
  // the showerQEDafterRemnants stage in PartonLevel and not included here.
  if (doQED) {
    // No 4th argument means shower must determine the starting scale itself.
    timesDecPtr->showerQEDafterDecays( sizePartonLevel + 1, event.size(),
      event );
  }

  if (userHooksPtr && !userHooksPtr->onEndHadronLevel(*this, event)) {
    loggerPtr->ERROR_MSG("user event onEndHadronLevel failed");
    return false;
  }

  // Done.
  return true;

}

//--------------------------------------------------------------------------

// Allow more decays if on/off switches changed.
// Note: does not do sequential hadronization, e.g. for Upsilon.

bool HadronLevel::moreDecays( Event& event) {

  // Colour-octet onia states must be decayed to singlet + gluon.
  if (!decayOctetOnia(event)) return false;

  // Loop through all entries to find those that should decay.
  int iDec = 0;
  do {
    decay(iDec, event);
  } while (++iDec < event.size());

  // Done.
  return true;

}

//--------------------------------------------------------------------------

// Prepare to be able to pick a low-energy hadron-hadron scattering.

bool HadronLevel::initLowEnergyProcesses() {

  // Prepare for low-energy QCD processes.
  doNonPertAll     = flag("LowEnergyQCD:all");
  if (!doNonPertAll) {
    if (flag("LowEnergyQCD:nonDiffractive"))      nonPertProc.push_back(1);
    if (flag("LowEnergyQCD:elastic"))             nonPertProc.push_back(2);
    if (flag("LowEnergyQCD:singleDiffractiveXB")) nonPertProc.push_back(3);
    if (flag("LowEnergyQCD:singleDiffractiveAX")) nonPertProc.push_back(4);
    if (flag("LowEnergyQCD:doubleDiffractive"))   nonPertProc.push_back(5);
    if (flag("LowEnergyQCD:excitation"))          nonPertProc.push_back(7);
    if (flag("LowEnergyQCD:annihilation"))        nonPertProc.push_back(8);
    if (flag("LowEnergyQCD:resonant"))            nonPertProc.push_back(9);
  }

  // Return true if any process is switched on.
  return doNonPertAll || (nonPertProc.size() > 0);

}

//--------------------------------------------------------------------------

// Pick process for a low-energy hadron-hadron scattering.

int HadronLevel::pickLowEnergyProcess(int idA, int idB, double eCM,
  double mA, double mB) {

  // Chosen process.
  int procType;

    // If all processes are on, just call SigmaLowEnergy directly.
  if (doNonPertAll) {
    procType = sigmaLowEnergyPtr->pickProcess(idA, idB, eCM, mA, mB);
    if (procType == 0) {
      loggerPtr->ERROR_MSG(
        "no available processes for specified particles and energy");
      return 0;
    }

  // Trivial choice if only one process.
  } else if (nonPertProc.size() ==1) {
    procType = nonPertProc[0];

  // If only certain processes are on, calculate those cross sections.
  } else {
    vector<int> procs;
    vector<double> sigmas;
    for (int proc : nonPertProc) {
      double sigma = sigmaLowEnergyPtr->sigmaPartial(
        idA, idB, eCM, mA, mB, proc);
      if (sigma > 0.) {
        procs.push_back(proc);
        sigmas.push_back(sigma);
      } else {
        loggerPtr->WARNING_MSG(
          "a process with zero cross section was explicitly turned on",
          to_string(proc));
      }
    }

    // Error if no processes has a positive cross section. Else pick process.
    if (procs.size() == 0) {
      loggerPtr->ERROR_MSG(
        "no processes with positive cross sections have been turned on");
      return 0;
    }
    procType = procs[rndmPtr->pick(sigmas)];
  }

  // Pick specific resonance for proc == 9.
  if (procType == 9) {
    procType = sigmaLowEnergyPtr->pickResonance(idA, idB, eCM);
    if (procType == 0) {
      loggerPtr->ERROR_MSG(
        "no available resonances for the given particles and energy");
      return 0;
    }
  }

  // Done.
  return procType;

}

//--------------------------------------------------------------------------

// Decay colour-octet onium states.

bool HadronLevel::decayOctetOnia(Event& event) {

  // Loop over particles and decay any onia encountered.
  for (int iDec = 0; iDec < event.size(); ++iDec)
  if (event[iDec].isFinal()
    && particleDataPtr->isOctetHadron(event[iDec].id())) {
    if (!decays.decay( iDec, event)) return false;

    // Set colour flow by hand: gluon inherits octet-onium state.
    int iGlu = event.size() - 1;
    event[iGlu].cols( event[iDec].col(), event[iDec].acol() );
  }

  // Done.
  return true;

}

//--------------------------------------------------------------------------

// Trace colour flow in the event to form colour singlet subsystems.
// Option will keep junctions in the remainsJunction list,
// and not eliminate any junctions by insertion.

bool HadronLevel::findSinglets(Event& event, bool keepJunctions) {

  // Clear up storage.
  colConfig.clear();

  // Find a list of final partons and of all colour ends and gluons.
  if (colTrace.setupColList(event)) return true;

  // Begin arrange the partons into separate colour singlets.

  // Junctions: loop over them, and identify kind.
  for (int iJun = 0; iJun < event.sizeJunction(); ++iJun)
  if (event.remainsJunction(iJun)) {
    if (!keepJunctions) event.remainsJunction(iJun, false);
    int kindJun = event.kindJunction(iJun);
    iParton.resize(0);

    // Loop over junction legs.
    for (int iCol = 0; iCol < 3; ++iCol) {
      int indxCol = event.colJunction(iJun, iCol);
      iParton.push_back( -(10 + 10 * iJun + iCol) );
      // Junctions: find color ends.
      if (kindJun % 2 == 1 && !colTrace.traceFromAcol(indxCol, event, iJun,
        iCol, iParton)) return false;
      // Antijunctions: find anticolor ends.
      if (kindJun % 2 == 0 && !colTrace.traceFromCol(indxCol, event, iJun,
        iCol, iParton)) return false;
    }

    // A junction may be eliminated by insert if two quarks are nearby.
    if (!keepJunctions) {
      int nJunOld = event.sizeJunction();
      if (!colConfig.insert(iParton, event)) return false;
      if (event.sizeJunction() < nJunOld) --iJun;
    }
  }

  // Open strings: pick up each colour end and trace to its anticolor end.
  while (!colTrace.colFinished()) {
    iParton.resize(0);
    if (!colTrace.traceFromCol( -1, event, -1, -1, iParton)) return false;

    // Store found open string system. Analyze its properties.
    if (!colConfig.insert(iParton, event)) return false;
  }

  // Closed strings : begin at any gluon and trace until back at it.
  while (!colTrace.finished()) {
    iParton.resize(0);
    if (!colTrace.traceInLoop(event, iParton)) return false;

    // Store found closed string system. Analyze its properties.
    if (!colConfig.insert(iParton, event)) return false;
  }

  // Done.
  return true;

}

//--------------------------------------------------------------------------

// Extract rapidity pairs of string pieces. Store in form [yCol, yAcol].

vector< vector< pair<double,double> > > HadronLevel::rapidityPairs(
  Event& event) {

  // Loop over all string systems in the event.
  vector< vector< pair<double,double> > > rapPairs;
  for (int iSub = 0; iSub < int(colConfig.size()); iSub++) {
    vector< pair<double,double> > rapsNow;
    vector<int> iPartons = colConfig[iSub].iParton;

    // Special treatment of junctions.
    // Should not have unprocessed multi-junction systems at this point.
    if (colConfig[iSub].hasJunction) {

      // Loop through iPartons and define junction legs.
      int legBeg[3] = { 0, 0, 0};
      int legEnd[3] = { 0, 0, 0};
      int leg = -1;
      for (int i = 0; i < int(iPartons.size()); ++i) {
        if (iPartons[i] < 0) {
          if (leg == 2) break;
          legBeg[++leg] = i + 1;
        }
        else legEnd[leg] = i;
      }

      // Check if a junction or antijunction to determine flux direction.
      bool antiJunc = ( event[iPartons[legEnd[0]]].colType() < 0 ) ? 1 : 0;

      // Special treatment for the junction. Look at first parton on
      // each leg and construct colourflow of type q --> q <-- q.
      double yJun[3];
      for (int i = 0; i < 3; ++i)
        yJun[i] = yMax(event[iPartons[legBeg[i]]], MTINY);
      int iMin = (yJun[0] < yJun[1]) ? 0 : 1;
      int iMax = 1 - iMin;
      if (yJun[2] < min(yJun[0], yJun[1])) iMin = 2;
      else if (yJun[2] > max(yJun[0], yJun[1])) iMax = 2;
      int iMid = 3 - iMin - iMax;
      if (antiJunc) {
        rapsNow.push_back( make_pair(yJun[iMid], yJun[iMin]) );
        rapsNow.push_back( make_pair(yJun[iMid], yJun[iMax]) );
      } else {
        rapsNow.push_back( make_pair(yJun[iMin], yJun[iMid]) );
        rapsNow.push_back( make_pair(yJun[iMax], yJun[iMid]) );
      }

      // Do standard treatment on other partons on junction legs.
      for (int i = 0; i < 3; ++i) {
        for (int j = legBeg[i]; j < legEnd[i] - 1; ++j) {
          int i1 = iPartons[j];
          int i2 = iPartons[j + 1];
          double y1  = yMax(event[i1], MTINY);
          double y2  = yMax(event[i2], MTINY);
          if (!antiJunc) swap(y1, y2);
          rapsNow.push_back( make_pair(y1, y2) );
        }
      }
    }

    // Normal string treatment.
    else {
      int size = int(iPartons.size());
      int end  = size - (colConfig[iSub].isClosed ? 0 : 1);
      for (int iP = 0; iP < end; iP++) {
        int    i1  = iPartons[iP];
        int    i2  = iPartons[(iP+1)%size];
        double y1  = yMax(event[i1], MTINY);
        double y2  = yMax(event[i2], MTINY);

        // Check flux direction of current string piece and store accordingly.
        if (event[i1].col() == event[i2].acol() && event[i1].col() != 0)
          rapsNow.push_back( make_pair(y1, y2) );
        else rapsNow.push_back( make_pair(y2, y1) );
      }
    }
    rapPairs.push_back(rapsNow);
  }
  // Done.
  return rapPairs;
}

//--------------------------------------------------------------------------

// Perform rescattering. Return true if new strings must be hadronized.

bool HadronLevel::rescatter(Event& event) {

  if (doBoost) {
    if      (boostDir == 1) event.bst(tanh(boost), 0., 0.);
    else if (boostDir == 2) event.bst(0., tanh(boost), 0.);
    else                    event.bst(0., 0., tanh(boost));
  }

  bool decaysCausedHadronization = false;

  // Queue potential decays and rescatterings.
  priority_queue<PriorityNode> candidates;
  queueDecResc(event, 0, candidates);
  while (!candidates.empty()) {

    // Get earliest decay/rescattering.
    PriorityNode node = candidates.top();
    candidates.pop();

    // Skip if either particle has already interacted elsewhere.
    if (!event[node.i1].isFinal()
    || (!node.isDecay() && !event[node.i2].isFinal())) continue;

    // Perform the queued action: decay.
    int oldSize = event.size();
    if (node.isDecay()) {
      decays.decay(node.i1, event);
      // @TODO If there is moreToDo, those things should also
      // be handled in order?
      if (decays.moreToDo()) decaysCausedHadronization = true;

    // Perform the queued action: two-body collision.
    } else {
      Particle& hadA = event[node.i1];
      Particle& hadB = event[node.i2];
      double eCM = (hadA.p() + hadB.p()).mCalc();
      int procType = sigmaLowEnergyPtr->pickProcess(hadA.id(), hadB.id(), eCM,
        hadA.m(),  hadB.m());
      if (procType == 0) {
        loggerPtr->ERROR_MSG("no available rescattering processes",
          to_string(hadA.id()) + " + " + to_string(hadB.id())
          + " @ " + to_string(eCM));
        continue;
      }
      if (!lowEnergyProcess.collide(node.i1, node.i2, procType, event,
        node.origin, node.displaced1, node.displaced2)) continue;
    }

    // If multiple rescattering is on, check for new interactions.
    if (scatterManyTimes) queueDecResc(event, oldSize, candidates);

    // If multiple rescattering is off, decay new hadrons.
    else if (doDecay) {
      for (int i = oldSize; i < event.size(); ++i) {
        if (event[i].isFinal() && event[i].isHadron()
        && event[i].canDecay() && event[i].mayDecay()
        && event[i].mWidth() > widthSepBE) {
          decays.decay(i, event);
          if (decays.moreToDo())
            decaysCausedHadronization = true;
        }
      }
    }
  }

  if (doBoost) {
    if      (boostDir == 1) event.bst(-tanh(boost), 0., 0.);
    else if (boostDir == 2) event.bst(0., -tanh(boost), 0.);
    else                    event.bst(0., 0., -tanh(boost));
  }

  return decaysCausedHadronization;
}

//--------------------------------------------------------------------------

// Calculate possible decays and rescatterings and add all to the queue.

void HadronLevel::queueDecResc(Event& event, int iStart,
  priority_queue<HadronLevel::PriorityNode>& queue) {

  // Loop over all existing or newly added hadrons.
  for (int iFirst = iStart; iFirst < event.size(); ++iFirst) {
    Particle& hadA = event[iFirst];
    if (!hadA.isFinal() || !hadA.isHadron() || hadA.isExotic()) continue;

    // Queue hadrons that should decay.
    if (doDecay && hadA.canDecay() && hadA.mayDecay()
      && hadA.mWidth() > widthSepRescatter)
      queue.push(PriorityNode(iFirst, hadA.vDec()));

    // Find primary hadron mother, if any.
    int iNowA = iFirst;
    int iMotA = event[iNowA].mother1();
    if (!scatterNeighbours)
    while (event[iMotA].isHadron() && event[iNowA].mother2() == 0) {
      iNowA = iMotA;
      iMotA = event[iNowA].mother1();
    }

    // Loop over a second existing hadron to study all pairs.
    for (int iSecond = 0; iSecond < iFirst; ++iSecond) {
      Particle& hadB = event[iSecond];
      if (!hadB.isFinal() || !hadB.isHadron() || hadB.isExotic()) continue;

      // Early skip if particles are moving away from each other.
      if (scatterQuickCheck && dot3( hadB.p() / hadB.e() - hadA.p() / hadA.e(),
        hadB.vProd() - hadA.vProd() ) > 0. ) continue;

      // Skip rescattering among decay products or already scattered.
      if ( event[hadA.mother1()].isHadron() && hadB.mother1() == hadA.mother1()
        && hadB.mother2() == hadA.mother2()) continue;

      // Find primary hadron mother, if any.
      if (!scatterNeighbours) {
        int iNowB = iSecond;
        int iMotB = event[iNowB].mother1();
        while (event[iMotB].isHadron() && event[iNowB].mother2() == 0) {
          iNowB = iMotB;
          iMotB = event[iNowB].mother1();
        }

        // Optionally skip if hadrons are nearest neighbours.
        if (abs(iNowB - iNowA) <= 1) continue;
      }

      // Set up positions for each particle in the pair CM frame.
      RotBstMatrix frame;

      if (useVelocityFrame)
        frame.toSameVframe(hadA.p(), hadB.p());
      else
        frame.toCMframe(hadA.p(), hadB.p());
      Vec4 vA = hadA.vProd(); vA.rotbst(frame);
      Vec4 vB = hadB.vProd(); vB.rotbst(frame);
      Vec4 pA = hadA.p();     pA.rotbst(frame);
      Vec4 pB = hadB.p();     pB.rotbst(frame);

      // Rejection of pairs with large impact parameter.
      double b2 = (vA - vB).pT2();
      if (b2 > b2Max) continue;

      // Offset particles to position when the last particle is created.
      double t0 = max(vA.e(), vB.e());
      double zA = vA.pz() + (t0 - vA.e()) * pA.pz() / pA.e();
      double zB = vB.pz() + (t0 - vB.e()) * pB.pz() / pB.e();

      // Abort if particles have already passed each other.
      if (zA >= zB) continue;

      // Optionally allow regeneration time delay.
      if (delayRegeneration && (hadA.status()/10 == 15
        || hadB.status()/10 == 15)) {
        bool regA = (hadA.status() == 152 || hadA.status() == 157);
        if ( (hadA.status() == 153 || hadA.status() == 154)
          && event[hadA.mother1()].isHadron()) regA = true;
        bool regB = (hadB.status() == 152 || hadB.status() == 157);
        if ( (hadB.status() == 153 || hadB.status() == 154)
          && event[hadB.mother1()].isHadron()) regB = true;
        if (regA || regB) {
          if (regA) vA += tauRegeneration * FM2MM * rndmPtr->exp()
            * pA / hadA.m();
          if (regB) vB += tauRegeneration * FM2MM * rndmPtr->exp()
            * pB / hadB.m();
          t0 = max(vA.e(), vB.e());
          zA = vA.pz() + (t0 - vA.e()) * pA.pz() / pA.e();
          zB = vB.pz() + (t0 - vB.e()) * pB.pz() / pB.e();
          if (zA >= zB) continue;
        }
      }

      // Calculate sigma and abort if impact parameter is too large.
      double eCM = (pA + pB).mCalc();
      double sigma = sigmaLowEnergyPtr->sigmaTotal(hadA.id(), hadB.id(), eCM,
        hadA.m(),  hadB.m());
      if (sigma < SigmaLowEnergy::TINYSIGMA)
        continue;

      double b2Crit = MB2FMSQ * pow2(FM2MM) * sigma / (impactOpacity * M_PI);
      double pAccept;
      // Sharp edge or Gaussian profile.
      if (impactModel == 0)  pAccept = (b2 < b2Crit) ? impactOpacity : 0.;
      else                   pAccept = impactOpacity * exp(-b2 / b2Crit);
      if (rndmPtr->flat() > pAccept) continue;

      // Calculate origin of collision and transform to lab frame.
      // Also particle scattering vertices for elastic and excitation.
      double tColl = t0 - (zB - zA) / (pB.pz() / pB.e() - pA.pz() / pA.e());
      Vec4 origin(0.5 * (vA.px() + vB.px()), 0.5 * (vA.py() + vB.py()),
                  zA + pA.pz() / pA.e() * (tColl - t0), tColl);
      Vec4 displacedA = Vec4( vA.px(), vA.py(), origin.pz(), origin.e() );
      Vec4 displacedB = Vec4( vB.px(), vB.py(), origin.pz(), origin.e() );
      frame.invert();
      origin.rotbst(frame);
      displacedA.rotbst(frame);
      displacedB.rotbst(frame);

      // Queue hadron pair that should rescatter.
      if (isfinite(origin))
        queue.push( PriorityNode(iFirst, iSecond, origin, displacedA,
          displacedB) );
      else
        loggerPtr->ERROR_MSG("got non-finite rescattering vertex");
    }
  }
}

//==========================================================================

} // end namespace Pythia8
