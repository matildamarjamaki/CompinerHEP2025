// ProcessContainer.cc is a part of the PYTHIA event generator.
// Copyright (C) 2025 Torbjorn Sjostrand.
// PYTHIA is licenced under the GNU GPL v2 or later, see COPYING for details.
// Please respect the MCnet Guidelines, see GUIDELINES for details.

// Function definitions (not found in the header) for the
// ProcessContainer and SetupContainers classes.

#include "Pythia8/ProcessContainer.h"

// Internal headers for special processes.
#include "Pythia8/SigmaCompositeness.h"
#include "Pythia8/SigmaEW.h"
#include "Pythia8/SigmaExtraDim.h"
#include "Pythia8/SigmaGeneric.h"
#include "Pythia8/SigmaHiggs.h"
#include "Pythia8/SigmaLeftRightSym.h"
#include "Pythia8/SigmaLeptoquark.h"
#include "Pythia8/SigmaNewGaugeBosons.h"
#include "Pythia8/SigmaQCD.h"
#include "Pythia8/SigmaSUSY.h"
#include "Pythia8/SigmaDM.h"
#include "Pythia8/SusyCouplings.h"

namespace Pythia8 {

//==========================================================================

// ProcessContainer class.
// Information allowing the generation of a specific process.

//--------------------------------------------------------------------------

// Constants: could be changed here if desired, but normally should not.
// These are of technical nature, as described for each.

// Number of event tries to check maximization finding reliability.
const int ProcessContainer::N12SAMPLE = 100;

// Ditto, but increased for 2 -> 3 processes.
const int ProcessContainer::N3SAMPLE  = 1000;

//--------------------------------------------------------------------------

// Initialize phase space and counters.
// Argument isFirst distinguishes two hard processes in same event.

bool ProcessContainer::init(bool isFirst, ResonanceDecays* resDecaysPtrIn,
  SLHAinterface* slhaInterfacePtr, GammaKinematics* gammaKinPtrIn) {

  registerSubObject(*sigmaProcessPtr);

  // Extract info about current process from SigmaProcess object.
  isLHA       = sigmaProcessPtr->isLHA();
  isNonDiff   = sigmaProcessPtr->isNonDiff();
  isResolved  = sigmaProcessPtr->isResolved();
  isDiffA     = sigmaProcessPtr->isDiffA();
  isDiffB     = sigmaProcessPtr->isDiffB();
  isDiffC     = sigmaProcessPtr->isDiffC();
  isQCD3body  = sigmaProcessPtr->isQCD3body();
  int nFin    = sigmaProcessPtr->nFinal();
  lhaStrat    = (isLHA) ? lhaUpPtr->strategy() : 0;
  lhaStratAbs = abs(lhaStrat);
  allowNegSig = sigmaProcessPtr->allowNegativeSigma();
  processCode = sigmaProcessPtr->code();

  // Switch to ensure that the scales set in a LH input event are
  // respected, even in resonance showers.
  useStrictLHEFscales = flag("Beams:strictLHEFscale");

  // Maximal number of events for more in-depth control of
  // how many events are selected/tried/accepted.
  nTryRequested = mode("Main:numberOfTriedEvents");
  nSelRequested = mode("Main:numberOfSelectedEvents");
  nAccRequested = mode("Main:numberOfAcceptedEvents");

  // Flag for maximum violation handling.
  increaseMaximum = flag("PhaseSpace:increaseMaximum");

  // Store whether beam particle has a photon and save the mode.
  gammaKinPtr = gammaKinPtrIn;

  // Use external photon flux.
  approximatedGammaFlux = beamAPtr->hasApproxGammaFlux() ||
    beamBPtr->hasApproxGammaFlux();

  // Check whether merging is enabled.
  doMerging = flag("Merging:doMerging") || word("Merging:process") != "";

  // Check whether photon sub-beams present.
  bool beamAhasGamma = flag("PDF:beamA2gamma");
  bool beamBhasGamma = flag("PDF:beamB2gamma");
  beamHasGamma = beamAhasGamma || beamBhasGamma;

  // Pick and create phase space generator. Send pointers where required.
  if (phaseSpacePtr != 0) ;
  else if (isLHA)
    phaseSpacePtr = make_shared<PhaseSpaceLHA>();
  else if (isNonDiff)
    phaseSpacePtr = make_shared<PhaseSpace2to2nondiffractive>();
  else if (!isResolved && !isDiffA  && !isDiffB  && !isDiffC )
    phaseSpacePtr = make_shared<PhaseSpace2to2elastic>();
  else if (!isResolved && !isDiffA  && !isDiffB && isDiffC)
    phaseSpacePtr = make_shared<PhaseSpace2to3diffractive>();
  else if (!isResolved)
    phaseSpacePtr = make_shared<PhaseSpace2to2diffractive>(isDiffA, isDiffB);
  else if (nFin == 1)   phaseSpacePtr = make_shared<PhaseSpace2to1tauy>();
  else if (nFin == 2)   phaseSpacePtr = make_shared<PhaseSpace2to2tauyz>();
  else if (isQCD3body)  phaseSpacePtr = make_shared<PhaseSpace2to3yyycyl>();
  else                  phaseSpacePtr = make_shared<PhaseSpace2to3tauycyl>();

  // Store pointers and perform simple initialization.
  resDecaysPtr    = resDecaysPtrIn;
  canVetoResDecay = (userHooksPtr != 0)
                  ? userHooksPtr->canVetoResonanceDecays() : false;
  if (isLHA) {
    sigmaProcessPtr->setLHAPtr(lhaUpPtr);
    phaseSpacePtr->setLHAPtr(lhaUpPtr);

    // Check if beams asymmetric and define a boost to CM frame if yes.
    double relEnergyDiff = ( lhaUpPtr->eBeamA() - lhaUpPtr->eBeamB() )
                         / ( lhaUpPtr->eBeamA() + lhaUpPtr->eBeamB() );
    isAsymLHA = ( abs(relEnergyDiff) > 1.e-10 )
             || ( lhaUpPtr->idBeamA() != lhaUpPtr->idBeamB() );
    if (isAsymLHA) {
      betazLHA = (sqrtpos(pow2(lhaUpPtr->eBeamA()) - pow2(infoPtr->mA()))
               -  sqrtpos(pow2(lhaUpPtr->eBeamB()) - pow2(infoPtr->mB())))
               / (lhaUpPtr->eBeamA() + lhaUpPtr->eBeamB());
    }
  }
  sigmaProcessPtr->init(beamAPtr, beamBPtr, slhaInterfacePtr);

  // Store the state of photon beams using inFlux: 0 = not a photon beam;
  // 1 = resolved photon; 2 = unresolved photon. Resolved photon unless
  // expcilitly an initiator or if beam unresolved and in-state not defined.
  // In case of externally provided Les Houches events rely currently on
  // flag for the process type as not expected to be mixed between different
  // event classes.
  string inState = sigmaProcessPtr->inFlux();
  beamAgammaMode = 0;
  beamBgammaMode = 0;
  gammaModeEvent = 0;
  if ( beamAPtr->isGamma() || beamAhasGamma ) {
    if (isLHA) {
      if (beamAPtr->isGamma()) {
        beamAgammaMode = beamAPtr->isUnresolved() ? 2 : 1;
      } else if (beamAhasGamma) {
        if (mode("Photon:ProcessType") == 3 || mode("Photon:ProcessType") == 4)
          beamAgammaMode = 2;
        else beamAgammaMode = 1;
      }
    } else if ( inState == "gmg" || inState == "gmq" || inState == "gmf"
                || inState == "gmgm" ) {
      beamAgammaMode = 2;
    } else {
      beamAgammaMode = 1;
    }
  }
  if ( beamBPtr->isGamma() || beamBhasGamma ) {
    if (isLHA) {
      if (beamBPtr->isGamma()) {
        beamBgammaMode = beamBPtr->isUnresolved() ? 2 : 1;
      } else if (beamBhasGamma) {
        if (mode("Photon:ProcessType") == 2 || mode("Photon:ProcessType") == 4)
          beamBgammaMode = 2;
        else beamBgammaMode = 1;
      }

    } else if ( inState == "ggm" || inState == "qgm" || inState == "fgm"
                || inState == "gmgm" ) {
      beamBgammaMode = 2;
    } else {
      beamBgammaMode = 1;
    }
  }

  // Save the photon modes and propagate further.
  // For soft processes set possible VM states.
  if ( ( beamAPtr->isGamma() || beamBPtr->isGamma() ) || beamHasGamma )
    setBeamModes(isSoftQCD(), false);

  // Save the state of photon beams.
  beamAhasResGamma = beamAPtr->hasResGamma();
  beamBhasResGamma = beamBPtr->hasResGamma();
  beamHasResGamma  = beamAhasResGamma || beamBhasResGamma;

  // Initialize also phaseSpace pointer.
  if (phaseSpacePtr) registerSubObject(*phaseSpacePtr);
  phaseSpacePtr->init( isFirst, sigmaProcessPtr);

  // Send the pointer to gammaKinematics for sampling of soft QCD processes.
  if ( beamAhasResGamma || beamBhasResGamma )
    phaseSpacePtr->setGammaKinPtr( gammaKinPtr);

  // Reset cross section statistics.
  nTry      = 0;
  nSel      = 0;
  nAcc      = 0;
  nTryStat  = 0;
  sigmaMx   = 0.;
  sigmaSum  = 0.;
  sigma2Sum = 0.;
  sigmaNeg  = 0.;
  sigmaAvg  = 0.;
  sigmaFin  = 0.;
  deltaFin  = 0.;
  wtAccSum  = 0.;

  // Reset temporary cross section summand.
  sigmaTemp   = 0.;
  sigma2Temp  = 0.;

  // Set up normalized variance for lhaStratAbs = 3.
  normVar3 = (lhaStratAbs == 3 && lhaUpPtr->xSecSum() != 0)
           ? pow2( lhaUpPtr->xErrSum() / lhaUpPtr->xSecSum()) : 0.;

  // Initialize process and allowed incoming partons.
  sigmaProcessPtr->initProc();
  if (!sigmaProcessPtr->initFlux()) return false;

  // Find maximum of differential cross section * phasespace.
  bool physical       = phaseSpacePtr->setupSampling();
  sigmaMx             = phaseSpacePtr->sigmaMax();
  double sigmaHalfWay = sigmaMx;

  // Separate signed maximum needed for LHA with negative weight.
  sigmaSgn            = phaseSpacePtr->sigmaSumSigned();

  // Check maximum by a few events, and extrapolate a further increase.
  if (physical & !isLHA && !isSoftQCD()) {
    int nSample = (nFin < 3) ? N12SAMPLE : N3SAMPLE;
    for (int iSample = 0; iSample < nSample; ++iSample) {
      bool test = false;
      while (!test) test = phaseSpacePtr->trialKin(false);
      if (iSample == nSample/2) sigmaHalfWay = phaseSpacePtr->sigmaMax();
    }
    double sigmaFullWay = phaseSpacePtr->sigmaMax();
    sigmaMx = (sigmaHalfWay > 0.) ? pow2(sigmaFullWay) / sigmaHalfWay
                                  : sigmaFullWay;
    phaseSpacePtr->setSigmaMax(sigmaMx);
  }

  // Allow Pythia to overwrite incoming beams or parts of Les Houches input.
  idRenameBeams   = mode("LesHouches:idRenameBeams");
  setLifetime     = mode("LesHouches:setLifetime");
  setQuarkMass    = mode("LesHouches:setQuarkMass");
  setLeptonMass   = mode("LesHouches:setLeptonMass");
  smearHadronMass = mode("LesHouches:smearHadronMass");
  idSmearHadIn    = mvec("LesHouches:idSmearHadrons");
  mRecalculate    = parm("LesHouches:mRecalculate");
  matchInOut      = flag("LesHouches:matchInOut");
  for (int i = 0; i < 6; ++i) idNewM[i] = i;
  for (int i = 6; i < 9; ++i) idNewM[i] = 2 * i - 1;
  for (int i = 1; i < 9; ++i) mNewM[i]  = particleDataPtr->m0(idNewM[i]);
  idSmearHadrons.clear();
  idSmearHadrons.push_back(0);
  for (int i = 0; i < int(idSmearHadIn.size()); ++i)
    idSmearHadrons.push_back(idSmearHadIn[i]);

  // Done.
  return physical;
}

//--------------------------------------------------------------------------

// Generate a trial event; selected or not.

bool ProcessContainer::trialProcess() {

  // Weights for photon flux oversampling with external flux.
  double wtPDF  = 1.;
  double wtFlux = 1.;

  // For photon beams set PDF pointer to resolved or unresolved.
  // Do not reset VM states since they will be sampled in phaseSpace->trialKin.
  if ( beamAPtr->isGamma() || beamBPtr->isGamma() || beamHasGamma )
    setBeamModes(false);

  // Loop over tries only occurs for Les Houches strategy = +-2.
  for (int iTry = 0;  ; ++iTry) {

    // Generate a trial phase space point, if meaningful.
    if (sigmaMx == 0.) return false;
    infoPtr->setEndOfFile(false);
    bool repeatSame = (iTry > 0);
    bool physical = phaseSpacePtr->trialKin(true, repeatSame);

    // For acceptable kinematics sample also kT for photons from leptons.
    if ( physical && !isSoftQCD() && beamHasGamma ) {

      // Save the x_gamma values for unresolved photons.
      if ( !beamAhasResGamma ) beamAPtr->xGamma( x1());
      if ( !beamBhasResGamma ) beamBPtr->xGamma( x2());

      // Sample the kinematics of virtual photons.
      if ( !gammaKinPtr->sampleKTgamma() ) physical = false;

      // Processes with direct photons rescale momenta and cross section.
      if ( physical && !(beamAhasResGamma && beamBhasResGamma) ) {
        double sHatNew = gammaKinPtr->calcNewSHat( phaseSpacePtr->sHat() );
        phaseSpacePtr->rescaleSigma( sHatNew);
      }

      // With external photon flux calculate weight wrt. approximated fluxes.
      // No reweighting with externally generated events.
      if ( physical && beamHasGamma && approximatedGammaFlux && !isLHA) {
        wtPDF  = phaseSpacePtr->weightGammaPDFApprox();
        wtFlux = gammaKinPtr->fluxWeight();
      } else {
        wtPDF  = 1.;
        wtFlux = 1.;
      }
    }

    // Flag to check if more events should be generated.
    bool doTryNext = true;

    // Note if at end of Les Houches file, else do statistics.
    if (isLHA && !physical) infoPtr->setEndOfFile(true);
    else {

      // Check if the number of selected events should be limited.
      if (nSelRequested > 0) doTryNext = (nTry < nSelRequested);

      // Only add to counter if event should count towards cross section.
      if (doTryNext) ++nTry;

      // Statistics for separate Les Houches process codes. Insert new codes.
      if (isLHA) {
        int codeLHANow = lhaUpPtr->idProcess();
        int iFill = -1;
        for (int i = 0; i < int(codeLHA.size()); ++i)
          if (codeLHANow == codeLHA[i]) iFill = i;
        if (iFill >= 0) {
          // Only add to counter if event should count towards cross section.
          if (doTryNext) ++nTryLHA[iFill];
        } else {
          codeLHA.push_back(codeLHANow);
          nTryLHA.push_back(1);
          nSelLHA.push_back(0);
          nAccLHA.push_back(0);
          for (int i = int(codeLHA.size()) - 1; i > 0; --i) {
            if (codeLHA[i] < codeLHA[i - 1]) {
              swap(codeLHA[i], codeLHA[i - 1]);
              swap(nTryLHA[i], nTryLHA[i - 1]);
              swap(nSelLHA[i], nSelLHA[i - 1]);
              swap(nAccLHA[i], nAccLHA[i - 1]);
            }
            else break;
          }
        }
      }
    }

    // Incoming top beams will not be handled, although allowed by LHAPDF.
    if (isLHA && (abs(lhaUpPtr->id(1)) == 6 || abs(lhaUpPtr->id(2)) == 6)) {
      loggerPtr->ERROR_MSG(
        "top not allowed incoming beam parton; event skipped");
      return false;
    }

    // Possibly fail, else cross section.
    if (!physical) return false;
    double sigmaNow = phaseSpacePtr->sigmaNow();

    // For photons with external flux correct the cross section but not
    // if event externally generated as then cross section fixed.
    if (beamHasGamma && approximatedGammaFlux && !isLHA)
      sigmaNow *= wtFlux * wtPDF;

    // Tell if this event comes with weight from cross section.
    double sigmaWeight = 1.;
    if (!isLHA && !increaseMaximum && sigmaNow > sigmaMx)
      sigmaWeight = sigmaNow / sigmaMx;
    if ( lhaStrat < 0 && sigmaNow < 0.) sigmaWeight = -1.;
    if ( lhaStratAbs == 4) sigmaWeight = sigmaNow;

    // Also compensating weight from biased phase-space selection.
    double biasWeight = phaseSpacePtr->biasSelectionWeight();
    weightNow = sigmaWeight * biasWeight;

    // Set event weight to zero if this event should not count.
    if (!doTryNext) weightNow = 0.0;

    infoPtr->setWeight( weightNow, lhaStrat);

    // Check that not negative cross section when not allowed.
    if (!allowNegSig) {
      if (sigmaNow < sigmaNeg) {
        loggerPtr->WARNING_MSG("negative cross section set 0",
          "for " + sigmaProcessPtr->name() );
        sigmaNeg = sigmaNow;
      }
      if (sigmaNow < 0.) sigmaNow = 0.;
    }

    // Cross section of process may come with a weight. Update sum.
    double sigmaAdd = sigmaNow * biasWeight;
    if (lhaStratAbs == 2 || lhaStratAbs == 3) sigmaAdd = sigmaSgn;

    // Do not add event weight to the cross section if it should not count.
    if (!doTryNext) {
      sigmaAdd=0.;
      // Reset stored weight.
      sigmaTemp  = 0.;
      sigma2Temp = 0.;
    }

    // Only update once an event is accepted, as is needed if the event weight
    // can still change by a finite amount due to reweighting.
    if (lhaStratAbs >= 3 ) {
      sigmaTemp  = sigmaAdd;
      sigma2Temp = pow2(sigmaAdd);
    } else {
      sigmaTemp  += sigmaAdd;
      sigma2Temp += pow2(sigmaAdd);
    }

    // Check if maximum violated.
    newSigmaMx = phaseSpacePtr->newSigmaMax();
    if (newSigmaMx) sigmaMx = phaseSpacePtr->sigmaMax();

    // Select or reject trial point. Statistics.
    bool select = true;
    if (lhaStratAbs < 3) select
      = (newSigmaMx || rndmPtr->flat() * abs(sigmaMx) < abs(sigmaNow));
    if (select) {
      // Only add to counter if event should count towards cross section.
      if (doTryNext) ++nSel;

      if (isLHA) {
        int codeLHANow = lhaUpPtr->idProcess();
        int iFill = -1;
        for (int i = 0; i < int(codeLHA.size()); ++i)
          if (codeLHANow == codeLHA[i]) iFill = i;
        // Only add to counter if event should count towards cross section.
        if (iFill >= 0 && doTryNext) ++nSelLHA[iFill];
      }
    }
    if (select || lhaStratAbs != 2) return select;

  }

}

//--------------------------------------------------------------------------

// Accumulate statistics after user veto, including LHA code.

void ProcessContainer::accumulate() {

  // Only update for non-vanishing weight - vanishing weight means
  // event should not count as accepted.
  double wgtNow = infoPtr->weight();
  if (wgtNow != 0.0) {
    ++nAcc;

    if (isLHA) {
      int codeLHANow = lhaUpPtr->idProcess();
      int iFill = -1;
      for (int i = 0; i < int(codeLHA.size()); ++i)
        if (codeLHANow == codeLHA[i]) iFill = i;
      if (iFill >= 0) ++nAccLHA[iFill];
      wgtNow = lhaUpPtr->weight();
      if (lhaStratAbs == 4) wgtNow *= PB2MB;
    }
    wtAccSum += wgtNow;
  }

}

//--------------------------------------------------------------------------

// Pick flavours and colour flow of process.

bool ProcessContainer::constructState() {

  // Construct flavour and colours for selected event.
  if (isResolved && !isNonDiff) sigmaProcessPtr->pickInState();
  sigmaProcessPtr->setIdColAcol();

  // Set beam modes correctly for given process.
  // Propagate the sampled VM states to beams as they are now sampled.
  if ( beamAPtr->isResolvedUnresolved() || beamBPtr->isResolvedUnresolved() )
    setBeamModes();

  // Done.
  return true;

}

//--------------------------------------------------------------------------

// Set the photon modes according to the process.

void ProcessContainer::setBeamModes(bool setVMD, bool isSampled) {

  // Set the modes for the current beams.
  beamAPtr->setGammaMode(beamAgammaMode);
  beamBPtr->setGammaMode(beamBgammaMode);

  // Set the gammaMode for given process.
  if      (beamAgammaMode <= 1 && beamBgammaMode <= 1) gammaModeEvent = 1;
  else if (beamAgammaMode <= 1 && beamBgammaMode == 2) gammaModeEvent = 2;
  else if (beamAgammaMode == 2 && beamBgammaMode <= 1) gammaModeEvent = 3;
  else if (beamAgammaMode == 2 && beamBgammaMode == 2) gammaModeEvent = 4;
  else                                                 gammaModeEvent = 0;

  // Propagate gammaMode and VMD state to info pointer.
  infoPtr->setGammaMode(gammaModeEvent);

  // Check whether explicit VMD sampling is required later on.
  if (setVMD && !isSampled) {
    if (beamAgammaMode > 0) infoPtr->setVMDstateA(true, 22, 0.,0.);
    if (beamBgammaMode > 0) infoPtr->setVMDstateB(true, 22, 0.,0.);
  }

  // Propagate the sampled VMD states to beams.
  if (isSampled) {
    if (infoPtr->isVMDstateA()) beamAPtr->setVMDstate(true,
      infoPtr->idVMDA(), infoPtr->mVMDA(), infoPtr->scaleVMDA(),
      false);
    if (infoPtr->isVMDstateB()) beamBPtr->setVMDstate(true,
      infoPtr->idVMDB(), infoPtr->mVMDB(), infoPtr->scaleVMDB(),
      false);
  }
}

//--------------------------------------------------------------------------

// Give the hard subprocess with kinematics.

bool ProcessContainer::constructProcess( Event& process, bool isHardest) {

  // Construct kinematics from selected phase space point.
  if (!phaseSpacePtr->finalKin()) return false;
  int nFin = sigmaProcessPtr->nFinal();

  // Basic info on process.
  if (isHardest) infoPtr->setType( name(), code(), nFin, isNonDiff,
    isResolved, isDiffA, isDiffB, isDiffC, isLHA);

  // Save sampled values for further use, requires info stored by setType.
  if ( beamHasGamma && !isSoftQCD() ) gammaKinPtr->finalize();

  // Rescale the momenta again when unresolved photons after finalKin.
  if ( beamHasGamma && !(beamAhasResGamma && beamBhasResGamma) ) {
    double sHatNew = infoPtr->sHatNew();
    phaseSpacePtr->rescaleMomenta( sHatNew);
  }

  // Let hard process record begin with the event as a whole and
  // the two incoming beam particles.
  int idA = infoPtr->idA();
  if (abs(idA) == idRenameBeams) idA = 16;
  int idB = infoPtr->idB();
  if (abs(idB) == idRenameBeams) idB = -16;
  process.append( 90, -11, 0, 0, 0, 0, 0, 0,
    Vec4(0., 0., 0., infoPtr->eCM()), infoPtr->eCM(), 0. );
  process.append( idA, -12, 0, 0, 0, 0, 0, 0,
    Vec4(0., 0., infoPtr->pzA(), infoPtr->eA()), infoPtr->mA(), 0. );
  process.append( idB, -12, 0, 0, 0, 0, 0, 0,
    Vec4(0., 0., infoPtr->pzB(), infoPtr->eB()), infoPtr->mB(), 0. );

  // Add intermediate gammas for lepton -> gamma -> parton processes
  // for both non-diffractive and hard processes, including direct-resolved.
  // Add a copy of hadron beam when e->gamma + p.
  int nOffsetGamma = 0;
  bool isGammaHadronDir = (beamAgammaMode == 2 && beamBgammaMode == 0)
                       || (beamAgammaMode == 0 && beamBgammaMode == 2);
  if ( beamHasResGamma || (isGammaHadronDir && beamHasGamma) ) {
    double xGm1 = beamAPtr->xGamma();
    if ( !(beamAPtr->gammaInBeam()) ) {
      process.append( beamAPtr->id(), -13, 1, 0, 0, 0, 0, 0,
        Vec4(0., 0., infoPtr->pzA(), infoPtr->eA()), beamAPtr->m(), 0. );
    } else {
      process.append( 22, -13, 1, 0, 0, 0, 0, 0,
        Vec4(0., 0., xGm1*infoPtr->pzA(), xGm1*infoPtr->eA()), 0, 0. );
    }
    process[1].daughter1(3);
    ++nOffsetGamma;
    double xGm2 = beamBPtr->xGamma();
    if ( !(beamBPtr->gammaInBeam()) ) {
      process.append( beamBPtr->id(), -13, 2, 0, 0, 0, 0, 0,
        Vec4(0., 0., infoPtr->pzB(), infoPtr->eB()), beamBPtr->m(), 0. );
    } else {
      process.append( 22, -13, 2, 0, 0, 0, 0, 0,
        Vec4(0., 0., xGm2*infoPtr->pzB(), xGm2*infoPtr->eB()), 0, 0. );
    }
    process[1 + nOffsetGamma].daughter1(3 + nOffsetGamma);
    ++nOffsetGamma;
  }

  // For nondiffractive process no interaction selected so far, so done.
  if (isNonDiff) return true;

  // Entries 3 and 4, now to be added, come from 1 and 2.
  // Offset from normal locations possible due to intermediate photons.
  process[1 + nOffsetGamma].daughter1(3 + nOffsetGamma);
  process[2 + nOffsetGamma].daughter1(4 + nOffsetGamma);
  double scale  = 0.;
  double scalup = 0.;

  // For DiffC entries 3 - 5 come jointly from 1 and 2 (to keep HepMC happy).
  if (isDiffC) {
    process[1].daughters(3, 5);
    process[2].daughters(3, 5);
  }

  // Insert the subprocess partons - resolved processes.
  int idRes = sigmaProcessPtr->idSChannel();
  if (isResolved && !isLHA) {

    // NOAM: Mothers and daughters without/with intermediate state.
    int m_M1 = 3;
    int m_M2 = 4;
    int m_D1 = 5;
    int m_D2 = (nFin == 1) ? 0 : 4 + nFin;
    if (idRes != 0) {
      m_M1   = 5;
      m_M2   = 0;
      m_D1   = 5;
      m_D2   = 0;
    }

    // Find scale from which to begin MPI/ISR/FSR evolution.
    scale = sqrt(Q2Fac());
    process.scale( scale );

    // Loop over incoming and outgoing partons.
    int colOffset = process.lastColTag();
    for (int i = 1; i <= 2 + nFin; ++i) {

      // Read out particle info from SigmaProcess object.
      int id        = sigmaProcessPtr->id(i);
      int status    = (i <= 2) ? -21 : 23;
      int mother1   = (i <= 2) ? i : m_M1 ;
      int mother2   = (i <= 2) ? 0 : m_M2 ;
      int daughter1 = (i <= 2) ? m_D1 : 0;
      int daughter2 = (i <= 2) ? m_D2 : 0;
      int col       = sigmaProcessPtr->col(i);
      if      (col > 0) col += colOffset;
      else if (col < 0) col -= colOffset;
      int acol      = sigmaProcessPtr->acol(i);
      if      (acol > 0) acol += colOffset;
      else if (acol < 0) acol -= colOffset;

      // If extra photons in event record, offset the mother/daughter list.
      if ( beamAhasResGamma || beamBhasResGamma
         || (beamAgammaMode == 2 && beamBgammaMode == 0)
         || (beamAgammaMode == 0 && beamBgammaMode == 2) ) {
        mother1 += nOffsetGamma;
        if (mother2 > 0)   mother2   += nOffsetGamma;
        if (daughter1 > 0) daughter1 += nOffsetGamma;
        if (daughter2 > 0) daughter2 += nOffsetGamma;
      }

      // Append to process record.
      int iNow = process.append( id, status, mother1, mother2,
        daughter1, daughter2, col, acol, phaseSpacePtr->p(i),
        phaseSpacePtr->m(i), scale);

      // NOAM: If there is an intermediate state, insert the it in
      // the process record after the two incoming particles.
      if (i == 2 && idRes != 0) {

        // Sign of intermediate state: go by charge.
        if (particleDataPtr->hasAnti(idRes)
          && process[3].chargeType() + process[4].chargeType() < 0)
          idRes *= -1;

        // The colour configuration of the intermediate state has to be
        // resolved separately.
        col         = 0;
        acol        = 0;
        int m_col1  = sigmaProcessPtr->col(1);
        int m_acol1 = sigmaProcessPtr->acol(1);
        int m_col2  = sigmaProcessPtr->col(2);
        int m_acol2 = sigmaProcessPtr->acol(2);
        if (m_col1 == m_acol2 && m_col2 != m_acol1) {
          col       = m_col2;
          acol       = m_acol1;
        } else if (m_col2 == m_acol1 && m_col1 != m_acol2) {
          col        = m_col1;
          acol       = m_acol2;
        }
        if      ( col > 0)  col += colOffset;
        else if ( col < 0)  col -= colOffset;
        if      (acol > 0) acol += colOffset;
        else if (acol < 0) acol -= colOffset;

        // Insert the intermediate state into the event record.
        Vec4 pIntMed = phaseSpacePtr->p(1) + phaseSpacePtr->p(2);
        process.append( idRes, -22, 3, 4,  6, 5 + nFin, col, acol,
          pIntMed, pIntMed.mCalc(), scale);
      }

      // Pick lifetime where relevant, else not.
      if (process[iNow].tau0() > 0.) process[iNow].tau(
        process[iNow].tau0() * rndmPtr->exp() );
    }
  }

  // Insert the outgoing particles - unresolved processes.
  else if (!isLHA) {

    // COR: Special handling of soft QCD with photons. If VMD states,
    // then outgoing photon has to be changed to VMD
    if (isSoftQCD() && (infoPtr->isVMDstateA()
      || infoPtr->isVMDstateB())) {
      int id3orig = sigmaProcessPtr->id(3);
      int status3 = (id3orig == process[1+nOffsetGamma].id()) ? 14 : 15;
      int id3     = (status3 == 14 && infoPtr->isVMDstateA())
                  ? infoPtr->idVMDA() : id3orig;
      process.append( id3, status3, 1 + nOffsetGamma, 0, 0, 0, 0, 0,
        phaseSpacePtr->p(3), phaseSpacePtr->m(3));
      int id4orig = sigmaProcessPtr->id(4);
      int status4 = (id4orig == process[2+nOffsetGamma].id()) ? 14 : 15;
      int id4     = (status4 == 14 && infoPtr->isVMDstateB())
                  ? infoPtr->idVMDB() : id4orig;
      process.append( id4, status4, 2 + nOffsetGamma, 0, 0, 0, 0, 0,
        phaseSpacePtr->p(4), phaseSpacePtr->m(4));

    } else {
      int id3     = sigmaProcessPtr->id(3);
      int status3 = (id3 == process[1].id()) ? 14 : 15;
      process.append( id3, status3, 1 + nOffsetGamma, 0, 0, 0, 0, 0,
        phaseSpacePtr->p(3), phaseSpacePtr->m(3));
      int id4     = sigmaProcessPtr->id(4);
      int status4 = (id4 == process[2].id()) ? 14 : 15;
      process.append( id4, status4, 2 + nOffsetGamma, 0, 0, 0, 0, 0,
        phaseSpacePtr->p(4), phaseSpacePtr->m(4));
    }

    // For central diffraction: two scattered protons inserted so far,
    // but modify mothers, add also centrally-produced hadronic system.
    if (isDiffC) {
      process[3].mothers( 1, 2);
      process[4].mothers( 1, 2);
      int id5     = sigmaProcessPtr->id(5);
      int status5 = 15;
      process.append( id5, status5, 1, 2, 0, 0, 0, 0,
        phaseSpacePtr->p(5), phaseSpacePtr->m(5));
    }

  }

  // Insert the outgoing particles - Les Houches Accord processes.
  else {

    // Check if second process; if so partons must be in order.
    int n21 = 0;
    int nOffsetSecond = 0;
    for (int i = 1; i < lhaUpPtr->sizePart(); ++i)
    if (lhaUpPtr->status(i) == -1) {
      ++n21;
      if (n21 == 3) nOffsetSecond = i - 1;
    }
    bool twoHard = (n21 == 4);

    // Since LHA partons may be out of order, determine correct one.
    // (Recall that zeroth particle is empty.)
    vector<int> newPos;
    newPos.reserve(lhaUpPtr->sizePart());
    newPos.push_back(0);
    for (int iNew = 0; iNew < lhaUpPtr->sizePart(); ++iNew) {
      // Simple copy in unchanged order for two hard interactions.
      if (twoHard) newPos.push_back(iNew + 1);
      // For iNew == 0 look for the two incoming partons, then for
      // partons having them as mothers, and so on layer by layer.
      else {
        for (int i = 1; i < lhaUpPtr->sizePart(); ++i)
          if (lhaUpPtr->mother1(i) == newPos[iNew]) newPos.push_back(i);
        if (int(newPos.size()) <= iNew) break;
      }
    }

    // Find scale from which to begin MPI/ISR/FSR evolution.
    scalup = lhaUpPtr->scale();
    scale  = scalup;
    double scalePr = (scale < 0.) ? sqrt(Q2Fac()) : scale;
    process.scale( scalePr);
    if (twoHard) process.scaleSecond( scalePr);
    if ( lhaUpPtr->scaleShowersIsSet() ) {
      process.scale( lhaUpPtr->scaleShowers(0) );
      process.scaleSecond( lhaUpPtr->scaleShowers(1) );
    }
    double scalePr2 = process.scaleSecond();

    // Copy over info from LHA event to process, in proper order.
    vector<int> iFinal;
    int iIn = 0;
    for (int i = 1; i < lhaUpPtr->sizePart(); ++i) {
      int iOld = newPos[i];
      int id = lhaUpPtr->id(iOld);
      if (i == 1 && abs(id) == idRenameBeams) id = 16;
      if (i == 2 && abs(id) == idRenameBeams) id = -16;
      int idAbs = abs(id);

      // Translate from LHA status codes.
      int lhaStatus =  lhaUpPtr->status(iOld);
      int status = -21;
      if (lhaStatus == 2 || lhaStatus == 3) status = -22;
      if (lhaStatus == 1) status = 23;

      // Find where mothers have been moved by reordering.
      int mother1Old = lhaUpPtr->mother1(iOld);
      int mother2Old = lhaUpPtr->mother2(iOld);
      int mother1 = 0;
      int mother2 = 0;
      for (int im = 1; im < i; ++im) {
        if (mother1Old == newPos[im]) mother1 = im + 2;
        if (mother2Old == newPos[im]) mother2 = im + 2;
      }
      if (status == -21) {
        ++iIn;
        mother1 = 1 + (iIn - 1)%2;
      }

      // Ensure that second mother = 0 except for bona fide carbon copies.
      if (mother1 > 0 && mother2 == mother1) {
        int sister1 = process[mother1].daughter1();
        int sister2 = process[mother1].daughter2();
        if (sister2 != sister1 && sister2 != 0) mother2 = 0;
      }

      // Find daughters and where they have been moved by reordering.
      // (Values shifted two steps to account for inserted beams.)
      int daughter1 = 0;
      int daughter2 = 0;
      for (int im = i + 1; im < lhaUpPtr->sizePart(); ++im) {
        if (lhaUpPtr->mother1(newPos[im]) == iOld
          || lhaUpPtr->mother2(newPos[im]) == iOld) {
          if (daughter1 == 0 || im + 2 < daughter1) daughter1 = im + 2;
          if (daughter2 == 0 || im + 2 > daughter2) daughter2 = im + 2;
        }
      }
      // For 2 -> 1 hard scatterings reset second daughter to 0.
      if (daughter2 == daughter1) daughter2 = 0;

      // Colour trivial, except reset irrelevant colour indices.
      int colType = particleDataPtr->colType(id);
      int col1   = (colType == 1 || colType == 2 || abs(colType) == 3)
                 ? lhaUpPtr->col1(iOld) : 0;
      int col2   = (colType == -1 || colType == 2 || abs(colType) == 3)
                 ?  lhaUpPtr->col2(iOld) : 0;

      // Copy momentum and ensure consistent (E, p, m) set.
      double px  = lhaUpPtr->px(iOld);
      double py  = lhaUpPtr->py(iOld);
      double pz  = lhaUpPtr->pz(iOld);
      double e   = lhaUpPtr->e(iOld);
      double m   = lhaUpPtr->m(iOld);
      if (mRecalculate > 0. && m > mRecalculate)
        m = sqrtpos( e*e - px*px - py*py - pz*pz);
      else e = sqrt( m*m + px*px + py*py + pz*pz);

      // Polarization.
      double pol = lhaUpPtr->spin(iOld);

      // Allow scale setting for generic partons.
      double scaleShow = lhaUpPtr->scale(iOld);

      // For resonance decay products use resonance mass as scale.
      double scaleNow = (iIn < 3) ? scalePr : scalePr2;
      int motherBeg = (iIn < 3) ? 4 : 4 + nOffsetSecond;
      if (mother1 > motherBeg && !useStrictLHEFscales)
        scaleNow = process[mother1].m();
      if (scaleShow >= 0.0) scaleNow = scaleShow;

      // Store Les Houches Accord partons. Boost to CM frame if not already.
      // Possibly an offset for mother/daugther list in photoproduction.
      if (nOffsetGamma > 0) {
        mother1 += nOffsetGamma;
        if(mother2 > 0)   mother2 += nOffsetGamma;
        if(daughter1 > 0) daughter1 += nOffsetGamma;
        if(daughter2 > 0) daughter2 += nOffsetGamma;
      }
      int iNow = process.append( id, status, mother1, mother2, daughter1,
        daughter2, col1, col2, Vec4(px, py, pz, e), m, scaleNow, pol);
      if (isAsymLHA) process[iNow].bst( 0., 0., -betazLHA);

      // Check if need to store lifetime.
      double tau = lhaUpPtr->tau(iOld);
      if ( (setLifetime == 1 && idAbs == 15) || setLifetime == 2)
         tau = process[iNow].tau0() * rndmPtr->exp();
      if (tau > 0.) process[iNow].tau(tau);

      // Track outgoing final particles.
      if (status == 23) iFinal.push_back( iNow);
    }

    // Second pass to catch vanishing final lepton and quark masses.
    int iFinalSz = iFinal.size();
    for (int iF = 0; iF < iFinalSz; ++iF) {
      int iMod    = iFinal[iF];
      int idAbs   = process[iMod].idAbs();
      double mOld = process[iMod].m();
      int iQLmod  = 0;
      for (int iQL = 1; iQL < 9; ++iQL)
      if (idAbs == idNewM[iQL]) {
        if ( iQL < 6 && setQuarkMass > 0 && (iQL == 4 || iQL == 5
          || setQuarkMass == 2) && (mOld < 0.5 * mNewM[iQL]
          || mOld > 1.5 * mNewM[iQL]) ) iQLmod = iQL;
        if ( iQL > 5 && setLeptonMass > 0 && (setLeptonMass == 2
          || mOld < 0.9 * mNewM[iQL] || mOld > 1.1 * mNewM[iQL]) )
          iQLmod = iQL;
      }

      // Also catch hadrons that should have smeared masses.
      int iHmod = 0;
      if (iQLmod == 0 && smearHadronMass > 0)
      for (int iH = 1; iH < int(idSmearHadrons.size()); ++iH)
      if (idAbs == idSmearHadrons[iH]) iHmod = iH;

      // Assign new masses where relevant.
      if (iQLmod == 0 && iHmod == 0) continue;
      double mNew = 0;
      if (iQLmod > 0) mNew = mNewM[iQLmod];
      else if (smearHadronMass == 1 && (mOld < particleDataPtr->mMin(idAbs)
        || mOld > particleDataPtr->mMax(idAbs)))
        loggerPtr->WARNING_MSG( "unexpected mass " + to_string(mOld)
        + " for id " + to_string(idAbs) + " so no smearing ");
      else mNew = particleDataPtr->mSel(idAbs);

      // Find partner to exchange energy and momentum with: general.
      int iRec = 0;
      if (iFinalSz == 2) iRec = iFinal[1 - iF];
      else if (process[iMod].mother1() > 2) {
        int iMother = process[iMod].mother1();
        int iMDau1  = process[iMother].daughter1();
        int iMDau2  = process[iMother].daughter2();
        if (iMDau2 == iMDau1 + 1 && iMod == iMDau1) iRec = iMDau2;
        if (iMDau2 == iMDau1 + 1 && iMod == iMDau2) iRec = iMDau1;
      }

      // Find partner to exchange energy and momentum with: lepton.
      if (iRec == 0 && iQLmod > 5) {
        for (int iR = 0; iR < iFinalSz; ++iR) if (iR != iF) {
          int iRtmp = iFinal[iR];
          if (process[iRtmp].idAbs() == idNewM[iQLmod] + 1
            && process[iRtmp].id() * process[iMod].id() < 0) iRec = iRtmp;
        }
      }
      if (iRec == 0 && iQLmod > 5) {
        for (int iR = 0; iR < iFinalSz; ++iR) if (iR != iF) {
          int iRtmp = iFinal[iR];
          if (process[iRtmp].id() == -process[iMod].id()) iRec = iRtmp;
        }
      }

      // Find partner to exchange energy and momentum with: quark.
      if (iRec == 0 && iQLmod > 0 && iQLmod < 6 && process[iMod].col() != 0) {
        for (int iR = 0; iR < iFinalSz; ++iR) if (iR != iF) {
          int iRtmp = iFinal[iR];
          if (process[iRtmp].acol() == process[iMod].col()) iRec = iRtmp;
        }
      }
      if (iRec == 0 && iQLmod > 0 && iQLmod < 6 && process[iMod].acol() != 0) {
        for (int iR = 0; iR < iFinalSz; ++iR) if (iR != iF) {
          int iRtmp = iFinal[iR];
          if (process[iRtmp].col() == process[iMod].acol()) iRec = iRtmp;
        }
      }

      // Find partner to exchange energy and momentum with: largest mass.
      if (iRec == 0) {
        double mMax = 0.;
        for (int iR = 0; iR < iFinalSz; ++iR) if (iR != iF) {
          int iRtmp   = iFinal[iR];
          double mTmp = m( process[iMod], process[iRtmp]) - process[iRtmp].m();
          if (mTmp > mMax) { iRec = iRtmp; mMax = mTmp;}
        }
      }

      // Carry out exchange of energy and momentum, if possible.
      bool doneShift = false;
      Vec4 pMod   = process[iMod].p();
      if (iRec != 0) {
        Vec4 pRecOld = process[iRec].p();
        Vec4 pRecNew = pRecOld;
        double mRec  = process[iRec].m();
        if ( pShift( pMod, pRecNew, mNew, mRec) ) {
          process[iMod].p( pMod);
          process[iMod].m( mNew);
          process[iRec].p( pRecNew);
          // Recursive boost of recoiler decay products, if necessary.
          if (!process[iRec].isFinal()) {
            vector<int> iDauRec = process[iRec].daughterListRecursive();
            RotBstMatrix Mdau;
            Mdau.bst( pRecOld, pRecNew);
            for (int iD = 0; iD < int(iDauRec.size()); ++iD)
              process[iDauRec[iD]].rotbst( Mdau);
          }
          doneShift = true;
        }
      }

      // Emergency solution: just add energy as needed, => new incoming.
      if (!doneShift) {
        pMod.e( sqrtpos( pMod.pAbs2() + mNew * mNew) );
        process[iMod].p( pMod);
        process[iMod].m( mNew);
        loggerPtr->WARNING_MSG("no suitable recoiler found,"
          " so energy violated");
      }
    }

    // Reassign momenta and masses for incoming partons.
    // Possible offset in photoproduction.
    if (matchInOut && !twoHard) {
      Vec4 pSumOut;
      for (int iF = 0; iF < iFinalSz; ++iF)
        pSumOut += process[iFinal[iF]].p();
      double e1 = 0.5 * (pSumOut.e() + pSumOut.pz());
      double e2 = 0.5 * (pSumOut.e() - pSumOut.pz());
      if (max(e1, e2) > 0.500001 * process[0].e()) {
        loggerPtr->ERROR_MSG("setting mass failed");
        return false;
      }
      e1 = min( e1, 0.5 * process[0].e());
      e2 = min( e2, 0.5 * process[0].e());
      process[3 + nOffsetGamma].pz( e1);
      process[3 + nOffsetGamma].e(  e1);
      process[3 + nOffsetGamma].m(  0.);
      process[4 + nOffsetGamma].pz(-e2);
      process[4 + nOffsetGamma].e(  e2);
      process[4 + nOffsetGamma].m(  0.);
    }
  }

  // Loop through decay chains and set secondary vertices when needed.
  for (int i = 3; i < process.size(); ++i) {
    int iMother  = process[i].mother1();

    // If sister to already assigned vertex then assign same.
    if ( process[i - 1].mother1() == iMother && process[i - 1].hasVertex() )
      process[i].vProd( process[i - 1].vProd() );

    // Else if mother already has vertex and/or lifetime then assign.
    else if ( process[iMother].hasVertex() || process[iMother].tau() > 0.)
      process[i].vProd( process[iMother].vDec() );
  }

  // Further info on process. Reset quantities that may or may not be known.
  int    id1Now  = process[3 + nOffsetGamma].id();
  int    id2Now  = process[4 + nOffsetGamma].id();
  int    id1pdf  = 0;
  int    id2pdf  = 0;
  double x1pdf   = 0.;
  double x2pdf   = 0.;
  double pdf1    = 0.;
  double pdf2    = 0.;
  double tHat    = 0.;
  double uHat    = 0.;
  double pTHatL  = 0.;
  double m3      = 0.;
  double m4      = 0.;
  double theta   = 0.;
  double phi     = 0.;
  double x1Now, x2Now, Q2FacNow, alphaEM, alphaS, Q2Ren, sHat;

  // Internally generated and stored information.
  if (!isLHA) {
    id1pdf       = id1Now;
    id2pdf       = id2Now;
    x1Now        = phaseSpacePtr->x1();
    x2Now        = phaseSpacePtr->x2();
    x1pdf        = x1Now;
    x2pdf        = x2Now;
    pdf1         = sigmaProcessPtr->pdf1();
    pdf2         = sigmaProcessPtr->pdf2();
    Q2FacNow     = sigmaProcessPtr->Q2Fac();
    alphaEM      = sigmaProcessPtr->alphaEMRen();
    alphaS       = sigmaProcessPtr->alphaSRen();
    Q2Ren        = sigmaProcessPtr->Q2Ren();
    sHat         = phaseSpacePtr->sHat();
    tHat         = phaseSpacePtr->tHat();
    uHat         = phaseSpacePtr->uHat();
    pTHatL       = phaseSpacePtr->pTHat();
    m3           = phaseSpacePtr->m(3);
    m4           = phaseSpacePtr->m(4);
    theta        = phaseSpacePtr->thetaHat();
    phi          = phaseSpacePtr->phiHat();
  }

  // Les Houches Accord process partly available, partly to be constructed.
  else {
    x1Now        = 2. * process[3].e() / infoPtr->eCM();
    x2Now        = 2. * process[4].e() / infoPtr->eCM();
    Q2FacNow     = (scale < 0.) ? sigmaProcessPtr->Q2Fac() : pow2(scale);
    alphaEM      = lhaUpPtr->alphaQED();
    if (alphaEM < 0.001) alphaEM = sigmaProcessPtr->alphaEMRen();
    alphaS       = lhaUpPtr->alphaQCD();
    if (alphaS  < 0.001) alphaS  = sigmaProcessPtr->alphaSRen();
    Q2Ren        = (scale < 0.) ? sigmaProcessPtr->Q2Ren() : pow2(scale);
    Vec4 pSum    = process[3].p() + process[4].p();
    sHat         = pSum * pSum;
    int nFinLH   = 0;
    double pTLH  = 0.;
    for (int i = 5; i < process.size(); ++i)
    if (process[i].mother1() == 3 && process[i].mother2() == 4) {
      ++nFinLH;
      pTLH      += process[i].pT();
    }
    if (nFinLH > 0) pTHatL = pTLH / nFinLH;

    // Read info on parton densities if provided.
    id1pdf       = lhaUpPtr->id1pdf();
    id2pdf       = lhaUpPtr->id2pdf();
    x1pdf        = lhaUpPtr->x1pdf();
    x2pdf        = lhaUpPtr->x2pdf();
    if (lhaUpPtr->pdfIsSet()) {
      pdf1       = lhaUpPtr->pdf1();
      pdf2       = lhaUpPtr->pdf2();
      Q2FacNow   = pow2(lhaUpPtr->scalePDF());
    }

    // Reconstruct kinematics of 2 -> 2 processes from momenta.
    if (nFin == 2) {
      Vec4 pDifT = process[3].p() - process[5].p();
      tHat       = pDifT * pDifT;
      Vec4 pDifU = process[3].p() - process[6].p();
      uHat       = pDifU * pDifU;
      pTHatL     = process[5].pT();
      m3         = process[5].m();
      m4         = process[6].m();
      Vec4 p5    = process[5].p();
      p5.bstback(pSum);
      theta      = p5.theta();
      phi        = process[5].phi();
    }
  }

  // Store information.
  if (isHardest) {
    infoPtr->setPDFalpha( 0, id1pdf, id2pdf, x1pdf, x2pdf, pdf1, pdf2,
      Q2FacNow, alphaEM, alphaS, Q2Ren, scalup);
    infoPtr->setKin( 0, id1Now, id2Now, x1Now, x2Now, sHat, tHat, uHat,
      pTHatL, m3, m4, theta, phi);

    // For DIS processes, save some kinematic variables.
    bool isDIS = (beamAPtr->isLepton() && beamBPtr->isHadron())
      || (beamAPtr->isHadron() && beamBPtr->isLepton());
    isDIS = isDIS && !beamHasGamma;
    if (isDIS) {
      // Find hardest lepton in event record.
      double eMax = -1.0;
      int iLepScat(0);
      for (int i = process.size()-1; i > 0 ; --i) {
        if (process[i].isFinal() && particleDataPtr->isLepton(process[i].id())
          && process[i].e() > eMax) {
          iLepScat = i;
          eMax = process[i].e();
          break;
        }
      }

      // Return error if no final-state leptons (neutrinos) found.
      if (eMax < 0.) {
        loggerPtr->ERROR_MSG("scattered lepton (neutrino) not found");
        return false;
      }

      // Calculate kinematic variables.
      int iLepIn   = beamAPtr->isLepton() ? 1 : 2;
      int iHadIn   = beamAPtr->isHadron() ? 1 : 2;
      Vec4 pProton = process[iHadIn].p();
      Vec4 peIn    = process[iLepIn].p();
      Vec4 peOut   = process[iLepScat].p();
      Vec4 pPhoton = peIn - peOut;
      // Q2, W2, Bjorken x, y.
      double Q2DIS = -pPhoton.m2Calc();
      double WDIS  = (pProton + pPhoton).mCalc();
      double xDIS  = Q2DIS / (2. * pProton * pPhoton);
      double yDIS  = (pProton * pPhoton) / (pProton * peIn);
      // Save variables in info.
      infoPtr->setDISKinematics(Q2DIS, WDIS, xDIS, yDIS);
    }
  }
  infoPtr->setTypeMPI( code(), pTHatL);

  // For Les Houches event store subprocess classification.
  if (isLHA) {
    int codeSub  = lhaUpPtr->idProcess();
    ostringstream nameSub;
    nameSub << "user process " << codeSub;
    infoPtr->setSubType( 0, nameSub.str(), codeSub, nFin);
  }

  // Done.
  return true;

}

//--------------------------------------------------------------------------

// Give the hard resonance decay chain from Les Houches input.
// Note: mildly modified copy of some constructProcess code; to streamline.

bool ProcessContainer::constructDecays( Event& process) {

  // Let hard process record begin with the event as a whole.
  process.clear();
  process.append( 90, -11, 0, 0, 0, 0, 0, 0, Vec4(0., 0., 0., 0.), 0., 0. );

  // Since LHA partons may be out of order, determine correct one.
  // (Recall that zeroth particle is empty.)
  vector<int> newPos;
  newPos.reserve(lhaUpPtr->sizePart());
  newPos.push_back(0);
  for (int iNew = 0; iNew < lhaUpPtr->sizePart(); ++iNew) {
    // For iNew == 0 look for the two incoming partons, then for
    // partons having them as mothers, and so on layer by layer.
    for (int i = 1; i < lhaUpPtr->sizePart(); ++i)
      if (lhaUpPtr->mother1(i) == newPos[iNew]) newPos.push_back(i);
    if (int(newPos.size()) <= iNew) break;
  }

  // Find scale from which to begin MPI/ISR/FSR evolution.
  double scale = lhaUpPtr->scale();
  process.scale( scale);
  Vec4 pSum;

  // Copy over info from LHA event to process, in proper order.
  for (int i = 1; i < lhaUpPtr->sizePart(); ++i) {
    int iOld = newPos[i];
    int id = lhaUpPtr->id(iOld);

    // Translate from LHA status codes.
    int lhaStatus =  lhaUpPtr->status(iOld);
    int status = -21;
    if (lhaStatus == 2 || lhaStatus == 3) status = -22;
    if (lhaStatus == 1) status = 23;

    // Find where mothers have been moved by reordering.
    int mother1Old = lhaUpPtr->mother1(iOld);
    int mother2Old = lhaUpPtr->mother2(iOld);
    int mother1 = 0;
    int mother2 = 0;
    for (int im = 1; im < i; ++im) {
      if (mother1Old == newPos[im]) mother1 = im;
      if (mother2Old == newPos[im]) mother2 = im;
    }

    // Ensure that second mother = 0 except for bona fide carbon copies.
    if (mother1 > 0 && mother2 == mother1) {
      int sister1 = process[mother1].daughter1();
      int sister2 = process[mother1].daughter2();
      if (sister2 != sister1 && sister2 != 0) mother2 = 0;
    }

    // Find daughters and where they have been moved by reordering.
    int daughter1 = 0;
    int daughter2 = 0;
    for (int im = i + 1; im < lhaUpPtr->sizePart(); ++im) {
      if (lhaUpPtr->mother1(newPos[im]) == iOld
        || lhaUpPtr->mother2(newPos[im]) == iOld) {
        if (daughter1 == 0 || im < daughter1) daughter1 = im;
        if (daughter2 == 0 || im > daughter2) daughter2 = im;
      }
    }
    // For 2 -> 1 hard scatterings reset second daughter to 0.
    if (daughter2 == daughter1) daughter2 = 0;

    // Colour trivial, except reset irrelevant colour indices.
    int colType = particleDataPtr->colType(id);
    int col1   = (colType == 1 || colType == 2 || abs(colType) == 3)
               ? lhaUpPtr->col1(iOld) : 0;
    int col2   = (colType == -1 || colType == 2 || abs(colType) == 3)
               ?  lhaUpPtr->col2(iOld) : 0;

    // Momentum trivial.
    double px  = lhaUpPtr->px(iOld);
    double py  = lhaUpPtr->py(iOld);
    double pz  = lhaUpPtr->pz(iOld);
    double e   = lhaUpPtr->e(iOld);
    double m   = lhaUpPtr->m(iOld);
    if (status > 0) pSum += Vec4( px, py, pz, e);

    // Polarization.
    double pol = lhaUpPtr->spin(iOld);

    // For resonance decay products use resonance mass as scale.
    double scaleNow = scale;
    if (mother1 > 0) scaleNow = process[mother1].m();

    // Store Les Houches Accord partons.
    int iNow = process.append( id, status, mother1, mother2, daughter1,
      daughter2, col1, col2, Vec4(px, py, pz, e), m, scaleNow, pol);

    // Check if need to store lifetime.
    double tau = lhaUpPtr->tau(iOld);
    if ( (setLifetime == 1 && abs(id) == 15) || setLifetime == 2)
       tau = process[iNow].tau0() * rndmPtr->exp();
    if (tau > 0.) process[iNow].tau(tau);
  }

  // Update four-momentum of system as a whole.
  process[0].p( pSum);
  process[0].m( pSum.mCalc());

  // Loop through decay chains and set secondary vertices when needed.
  for (int i = 1; i < process.size(); ++i) {
    int iMother  = process[i].mother1();

    // If sister to already assigned vertex then assign same.
    if ( process[i - 1].mother1() == iMother && process[i - 1].hasVertex()
      && i > 1) process[i].vProd( process[i - 1].vProd() );

    // Else if mother already has vertex and/or lifetime then assign.
    else if ( process[iMother].hasVertex() || process[iMother].tau() > 0.)
      process[i].vProd( process[iMother].vDec() );
  }

  // Done.
  return true;

}

//--------------------------------------------------------------------------

// Handle resonance decays.

bool ProcessContainer::decayResonances( Event& process) {

  // Save current event-record size and status codes.
  process.saveSize();
  vector<int> statusSave( process.size());
  for (int i = 0; i < process.size(); ++i)
    statusSave[i] = process[i].status();
  bool physical    = true;
  bool newChain    = false;
  bool newFlavours = false;

  // Do loop over user veto.
  do {

    // Do sequential chain of uncorrelated isotropic decays.
    do {
      physical = resDecaysPtr->next( process);
      if (!physical) return false;

      // Check whether flavours should be correlated.
      // (Currently only relevant for f fbar -> gamma*/Z0 gamma*/Z0.)
      newFlavours = ( sigmaProcessPtr->weightDecayFlav( process)
                    < rndmPtr->flat() );

      // Reset the decay chains if have to redo.
      if (newFlavours) {
        process.restoreSize();
        for (int i = 0; i < process.size(); ++i)
          process[i].status( statusSave[i]);
      }

    // Loop back where required to generate new decays with new flavours.
    } while (newFlavours);

    // Correct to nonisotropic decays.
    phaseSpacePtr->decayKinematics( process);

    // Optionally user hooks check/veto on decay chain.
    if (canVetoResDecay)
      newChain = userHooksPtr->doVetoResonanceDecays( process);

    // Reset the decay chains if have to redo.
    if (newChain) {
      process.restoreSize();
      for (int i = 0; i < process.size(); ++i)
        process[i].status( statusSave[i]);
    }

  // Loop back where required to generate new decay chain.
  } while(newChain);

  // Done.
  return true;

}

//--------------------------------------------------------------------------

// Reset event generation statistics; but NOT maximum of cross section.

void ProcessContainer::reset() {

  nTry      = 0;
  nSel      = 0;
  nAcc      = 0;
  nTryStat  = 0;
  sigmaSum  = 0.;
  sigma2Sum = 0.;
  sigmaNeg  = 0.;
  sigmaAvg  = 0.;
  sigmaFin  = 0.;
  deltaFin  = 0.;
  wtAccSum  = 0.;
  sigmaTemp = 0.;
  sigma2Temp= 0.;

}

//--------------------------------------------------------------------------

// Estimate integrated cross section and its uncertainty.

void ProcessContainer::sigmaDelta() {

  // Initial values. No analysis meaningful unless accepted events.
  nTryStat = nTry;
  sigmaAvg = 0.;
  sigmaFin = 0.;
  deltaFin = 0.;
  if (nAcc == 0) return;

  // Only update once an event is accepted, as is needed if the event weight
  // can still change by a finite amount due to reweighting.
  double wgtNow = infoPtr->weight();
  if (lhaStratAbs <= 2) wgtNow  = sigmaTemp;
  // If wgtNow includes the sign, do not include the sign again
  if (lhaStratAbs == 3) wgtNow *= abs(sigmaTemp);
  // Refetch the LHA weight in case of second hard
  if (lhaStratAbs == 4) {
    if (doMerging) wgtNow *= PB2MB;
    else wgtNow = lhaUpPtr->weight() * PB2MB;
  }
  if (lhaStratAbs > 0 && infoPtr->atEndOfFile()) wgtNow = 0;

  sigmaSum += wgtNow;

  double wgtNow2 = 1.0;
  if (lhaStratAbs <= 2) wgtNow2 = sigma2Temp;
  if (lhaStratAbs == 3) wgtNow2 = pow2(wgtNow)*sigma2Temp;
  if (lhaStratAbs == 4) wgtNow2 = pow2(wgtNow);
  sigma2Sum += wgtNow2;

  sigmaTemp  = 0.0;
  sigma2Temp = 0.0;

  // Average value. No error analysis unless at least two events.
  double nTryInv  = 1. / nTry;
  double nSelInv  = 1. / nSel;
  double nAccInv  = 1. / nAcc;
  // If Pythia should not perform the unweighting, always simply add accepted
  // event weights.
  sigmaAvg        = sigmaSum * ( (lhaStratAbs >= 3) ? nAccInv : nTryInv );
  double fracAcc  = nAcc * nSelInv;

  sigmaFin        = sigmaAvg * fracAcc;
  deltaFin        = sigmaFin;
  if (nAcc == 1) return;

  // Estimated error. Quadratic sum of cross section term and
  // binomial from accept/reject step.
  double delta2Sig   = 0;
  if (lhaStratAbs == 3)   delta2Sig = normVar3;
  else if (sigmaAvg != 0) delta2Sig = (
    sigma2Sum * nTryInv - pow2(sigmaAvg)) * nTryInv / pow2(sigmaAvg);
  double delta2Veto  = (nSel - nAcc) * nAccInv * nSelInv;
  double delta2Sum   = delta2Sig + delta2Veto;
  deltaFin           = sqrtpos(delta2Sum) * sigmaFin;

}

//==========================================================================

// SetupContainer class.
// Turns list of user-desired processes into a vector of containers.

//--------------------------------------------------------------------------

// Main routine to initialize list of processes.

bool SetupContainers::init(vector<ProcessContainer*>& containerPtrs,
  Info* infoPtr) {

  // Reset process list, if filled in previous subrun.
  if (containerPtrs.size() > 0) {
    for (int i = 0; i < int(containerPtrs.size()); ++i)
      delete containerPtrs[i];
    containerPtrs.clear();
  }
  SigmaProcessPtr sigmaPtr;

  // Reference to settings and pointer to particle data.
  Settings& settings = *infoPtr->settingsPtr;
  ParticleData* particleDataPtr = infoPtr->particleDataPtr;

  // Set up requested objects for soft QCD processes.
  bool softQCD = settings.flag("SoftQCD:all");
  bool inelastic = settings.flag("SoftQCD:inelastic");
  bool singleDiff = settings.flag("SoftQCD:singleDiffractive");
  if (softQCD || inelastic || settings.flag("SoftQCD:nonDiffractive")) {
    sigmaPtr = make_shared<Sigma0nonDiffractive>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (softQCD || settings.flag("SoftQCD:elastic")) {
    sigmaPtr = make_shared<Sigma0AB2AB>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if ( softQCD || inelastic || singleDiff
    || settings.flag("SoftQCD:singleDiffractiveXB")) {
    sigmaPtr = make_shared<Sigma0AB2XB>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if ( softQCD || inelastic || singleDiff
    || settings.flag("SoftQCD:singleDiffractiveAX") ) {
    sigmaPtr = make_shared<Sigma0AB2AX>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (softQCD || inelastic || settings.flag("SoftQCD:doubleDiffractive")) {
    sigmaPtr = make_shared<Sigma0AB2XX>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (softQCD || inelastic || settings.flag("SoftQCD:centralDiffractive")) {
    sigmaPtr = make_shared<Sigma0AB2AXB>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }

  // Set up requested objects for hard QCD processes.
  bool hardQCD = settings.flag("HardQCD:all");
  if (hardQCD || settings.flag("HardQCD:gg2gg")) {
    sigmaPtr = make_shared<Sigma2gg2gg>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (hardQCD || settings.flag("HardQCD:gg2qqbar")) {
    sigmaPtr = make_shared<Sigma2gg2qqbar>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (hardQCD || settings.flag("HardQCD:qg2qg")) {
    sigmaPtr = make_shared<Sigma2qg2qg>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (hardQCD || settings.flag("HardQCD:qq2qq")) {
    sigmaPtr = make_shared<Sigma2qq2qq>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (hardQCD || settings.flag("HardQCD:qqbar2gg")) {
    sigmaPtr = make_shared<Sigma2qqbar2gg>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (hardQCD || settings.flag("HardQCD:qqbar2qqbarNew")) {
    sigmaPtr = make_shared<Sigma2qqbar2qqbarNew>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }

  // Set up requested objects for c cbar and b bbar, also hard QCD.
  bool hardccbar = settings.flag("HardQCD:hardccbar");
  if (hardQCD || hardccbar || settings.flag("HardQCD:gg2ccbar")) {
    sigmaPtr = make_shared<Sigma2gg2QQbar>(4, 121);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (hardQCD || hardccbar || settings.flag("HardQCD:qqbar2ccbar")) {
    sigmaPtr = make_shared<Sigma2qqbar2QQbar>(4, 122);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  bool hardbbbar = settings.flag("HardQCD:hardbbbar");
  if (hardQCD || hardbbbar || settings.flag("HardQCD:gg2bbbar")) {
    sigmaPtr = make_shared<Sigma2gg2QQbar>(5, 123);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (hardQCD || hardbbbar || settings.flag("HardQCD:qqbar2bbbar")) {
    sigmaPtr = make_shared<Sigma2qqbar2QQbar>(5, 124);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }

  // Set up requested objects for hard QCD 2 -> 3 processes.
  bool hardQCD3parton = settings.flag("HardQCD:3parton");
  if (hardQCD3parton || settings.flag("HardQCD:gg2ggg")) {
    sigmaPtr = make_shared<Sigma3gg2ggg>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (hardQCD3parton || settings.flag("HardQCD:qqbar2ggg")) {
    sigmaPtr = make_shared<Sigma3qqbar2ggg>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (hardQCD3parton || settings.flag("HardQCD:qg2qgg")) {
    sigmaPtr = make_shared<Sigma3qg2qgg>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (hardQCD3parton || settings.flag("HardQCD:qq2qqgDiff")) {
    sigmaPtr = make_shared<Sigma3qq2qqgDiff>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (hardQCD3parton || settings.flag("HardQCD:qq2qqgSame")) {
    sigmaPtr = make_shared<Sigma3qq2qqgSame>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (hardQCD3parton || settings.flag("HardQCD:qqbar2qqbargDiff")) {
    sigmaPtr = make_shared<Sigma3qqbar2qqbargDiff>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (hardQCD3parton || settings.flag("HardQCD:qqbar2qqbargSame")) {
    sigmaPtr = make_shared<Sigma3qqbar2qqbargSame>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (hardQCD3parton || settings.flag("HardQCD:gg2qqbarg")) {
    sigmaPtr = make_shared<Sigma3gg2qqbarg>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (hardQCD3parton || settings.flag("HardQCD:qg2qqqbarDiff")) {
    sigmaPtr = make_shared<Sigma3qg2qqqbarDiff>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (hardQCD3parton || settings.flag("HardQCD:qg2qqqbarSame")) {
    sigmaPtr = make_shared<Sigma3qg2qqqbarSame>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }

  // Set up requested objects for prompt photon processes.
  bool promptPhotons = settings.flag("PromptPhoton:all");
  if (promptPhotons
    || settings.flag("PromptPhoton:qg2qgamma")) {
    sigmaPtr = make_shared<Sigma2qg2qgamma>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (promptPhotons
    || settings.flag("PromptPhoton:qqbar2ggamma")) {
    sigmaPtr = make_shared<Sigma2qqbar2ggamma>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (promptPhotons
    || settings.flag("PromptPhoton:gg2ggamma")) {
    sigmaPtr = make_shared<Sigma2gg2ggamma>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (promptPhotons
    || settings.flag("PromptPhoton:ffbar2gammagamma")) {
    sigmaPtr = make_shared<Sigma2ffbar2gammagamma>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (promptPhotons
    || settings.flag("PromptPhoton:gg2gammagamma")) {
    sigmaPtr = make_shared<Sigma2gg2gammagamma>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }

  // Set up requested objects for weak gauge boson t-channel exchange.
  bool weakBosonExchanges = settings.flag("WeakBosonExchange:all");
  if (weakBosonExchanges
    || settings.flag("WeakBosonExchange:ff2ff(t:gmZ)")) {
    sigmaPtr = make_shared<Sigma2ff2fftgmZ>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (weakBosonExchanges
    || settings.flag("WeakBosonExchange:ff2ff(t:W)")) {
    sigmaPtr = make_shared<Sigma2ff2fftW>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }

  // Set up requested objects for weak gauge boson processes.
  bool weakSingleBosons = settings.flag("WeakSingleBoson:all");
  if (weakSingleBosons
    || settings.flag("WeakSingleBoson:ffbar2gmZ")) {
    sigmaPtr = make_shared<Sigma1ffbar2gmZ>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (weakSingleBosons
    || settings.flag("WeakSingleBoson:ffbar2W")) {
    sigmaPtr = make_shared<Sigma1ffbar2W>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }

  // Set up requested object for s-channel gamma exchange.
  // Subset of gamma*/Z0 above, intended for multiparton interactions.
  if (settings.flag("WeakSingleBoson:ffbar2ffbar(s:gm)")) {
    sigmaPtr = make_shared<Sigma2ffbar2ffbarsgm>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }

  // Set up requested object for s-channel gamma*/Z0 or W+- exchange.
  if (settings.flag("WeakSingleBoson:ffbar2ffbar(s:gmZ)")) {
    sigmaPtr = make_shared<Sigma2ffbar2ffbarsgmZ>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (settings.flag("WeakSingleBoson:ffbar2ffbar(s:W)")) {
    sigmaPtr = make_shared<Sigma2ffbar2ffbarsW>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }

  // Set up requested objects for weak gauge boson pair processes.
  bool weakDoubleBosons = settings.flag("WeakDoubleBoson:all");
  if (weakDoubleBosons
    || settings.flag("WeakDoubleBoson:ffbar2gmZgmZ")) {
    sigmaPtr = make_shared<Sigma2ffbar2gmZgmZ>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (weakDoubleBosons
    || settings.flag("WeakDoubleBoson:ffbar2ZW")) {
    sigmaPtr = make_shared<Sigma2ffbar2ZW>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (weakDoubleBosons
    || settings.flag("WeakDoubleBoson:ffbar2WW")) {
    sigmaPtr = make_shared<Sigma2ffbar2WW>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }

  // Set up requested objects for weak gauge boson + parton processes.
  bool weakBosonAndPartons = settings.flag("WeakBosonAndParton:all");
  if (weakBosonAndPartons
    || settings.flag("WeakBosonAndParton:qqbar2gmZg")) {
    sigmaPtr = make_shared<Sigma2qqbar2gmZg>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (weakBosonAndPartons
    || settings.flag("WeakBosonAndParton:qg2gmZq")) {
    sigmaPtr = make_shared<Sigma2qg2gmZq>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (weakBosonAndPartons
    || settings.flag("WeakBosonAndParton:ffbar2gmZgm")) {
    sigmaPtr = make_shared<Sigma2ffbar2gmZgm>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (weakBosonAndPartons
    || settings.flag("WeakBosonAndParton:fgm2gmZf")) {
    sigmaPtr = make_shared<Sigma2fgm2gmZf>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (weakBosonAndPartons
    || settings.flag("WeakBosonAndParton:qqbar2Wg")) {
    sigmaPtr = make_shared<Sigma2qqbar2Wg>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (weakBosonAndPartons
    || settings.flag("WeakBosonAndParton:qg2Wq")) {
    sigmaPtr = make_shared<Sigma2qg2Wq>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (weakBosonAndPartons
    || settings.flag("WeakBosonAndParton:ffbar2Wgm")) {
    sigmaPtr = make_shared<Sigma2ffbar2Wgm>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (weakBosonAndPartons
    || settings.flag("WeakBosonAndParton:fgm2Wf")) {
    sigmaPtr = make_shared<Sigma2fgm2Wf>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }

  // Read in settings related to photon beams.
  bool photonParton = settings.flag("PhotonParton:all");
  int  photonMode   = settings.mode("Photon:ProcessType");
  bool gammaA       = settings.flag("PDF:beamA2gamma")
    || (abs(infoPtr->idA()) == 22);
  bool gammaB       = settings.flag("PDF:beamB2gamma")
    || (abs(infoPtr->idB()) == 22);

  // Initialize process for photon on side A only when explicitly direct
  // photons requested for side A or when a ''normal'' hadron on side A and
  // a resolved photon on side B as important to use the proper flux/PDF.
  // Otherwise initialize only one process (with the photon on side B).
  bool initGammaA = ( (photonMode == 0) && (gammaA || gammaB) )
    || ( (photonMode == 3) && gammaA )
    || ( (photonMode == 1) && (gammaB && !gammaA) );
  bool initGammaB = (photonMode == 0)
    || ( (photonMode == 2) && gammaB )
    || ( (photonMode == 1) && (gammaA && !gammaB) )
    || (!gammaA && !gammaB);

  // Initialize gm+gm processes only when explicitly needed with photon beams
  // or when normal hadronic collisions.
  bool initGammaGamma = ( (initGammaA || !gammaA) && (initGammaB || !gammaB) )
    || (photonMode == 4 && gammaA && gammaB );

  // Set up requested objects for photon collision processes.
  bool photonCollisions = settings.flag("PhotonCollision:all");
  if ( ( photonCollisions || settings.flag("PhotonCollision:gmgm2qqbar") )
       && initGammaGamma ) {
    sigmaPtr = make_shared<Sigma2gmgm2ffbar>(1, 261);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if ( ( photonCollisions || settings.flag("PhotonCollision:gmgm2ccbar") )
       && initGammaGamma ) {
    sigmaPtr = make_shared<Sigma2gmgm2ffbar>(4, 262);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if ( ( photonCollisions || settings.flag("PhotonCollision:gmgm2bbbar") )
       && initGammaGamma ) {
    sigmaPtr = make_shared<Sigma2gmgm2ffbar>(5, 263);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if ( ( photonCollisions || settings.flag("PhotonCollision:gmgm2ee") )
       && initGammaGamma ) {
    sigmaPtr = make_shared<Sigma2gmgm2ffbar>(11, 264);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if ( ( photonCollisions || settings.flag("PhotonCollision:gmgm2mumu") )
       && initGammaGamma ) {
    sigmaPtr = make_shared<Sigma2gmgm2ffbar>(13, 265);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if ( ( photonCollisions || settings.flag("PhotonCollision:gmgm2tautau") )
       && initGammaGamma ) {
    sigmaPtr = make_shared<Sigma2gmgm2ffbar>(15, 266);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }

  // Set up requested objects for photon-parton processes.
  // In case of unresolved photons insert process with correct initiator.
  if (photonParton || settings.flag("PhotonParton:ggm2qqbar")) {
    if ( initGammaB ) {
      sigmaPtr = make_shared<Sigma2ggm2qqbar>(1, 271, "ggm");
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if ( initGammaA ) {
      sigmaPtr = make_shared<Sigma2ggm2qqbar>(1, 281, "gmg");
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
  }
  if (photonParton || settings.flag("PhotonParton:ggm2ccbar")) {
    if ( initGammaB ) {
      sigmaPtr = make_shared<Sigma2ggm2qqbar>(4, 272, "ggm");
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if ( initGammaA) {
      sigmaPtr = make_shared<Sigma2ggm2qqbar>(4, 282, "gmg");
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
  }
  if (photonParton || settings.flag("PhotonParton:ggm2bbbar")) {
    if ( initGammaB ) {
      sigmaPtr = make_shared<Sigma2ggm2qqbar>(5, 273, "ggm");
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if ( initGammaA ) {
      sigmaPtr = make_shared<Sigma2ggm2qqbar>(5, 283, "gmg");
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
  }
  if (photonParton || settings.flag("PhotonParton:qgm2qg")) {
    if ( initGammaB ) {
      sigmaPtr = make_shared<Sigma2qgm2qg>(274, "qgm");
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if ( initGammaA ) {
      sigmaPtr = make_shared<Sigma2qgm2qg>(284, "gmq");
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
  }
  if (settings.flag("PhotonParton:qgm2qgm")) {
    if ( initGammaB ) {
      sigmaPtr = make_shared<Sigma2qgm2qgm>(275, "qgm");
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if ( initGammaA ) {
      sigmaPtr = make_shared<Sigma2qgm2qgm>(285, "gmq");
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
  }

  // Set up requested objects for onia production.
  charmonium = SigmaOniaSetup(infoPtr, 4);
  bottomonium = SigmaOniaSetup(infoPtr, 5);
  vector<SigmaProcessPtr> charmoniumSigmaPtrs, bottomoniumSigmaPtrs;
  charmonium.setupSigma2gg(charmoniumSigmaPtrs);
  charmonium.setupSigma2qg(charmoniumSigmaPtrs);
  charmonium.setupSigma2qq(charmoniumSigmaPtrs);
  bottomonium.setupSigma2gg(bottomoniumSigmaPtrs);
  bottomonium.setupSigma2qg(bottomoniumSigmaPtrs);
  bottomonium.setupSigma2qq(bottomoniumSigmaPtrs);
  for (unsigned int i = 0; i < charmoniumSigmaPtrs.size(); ++i)
    containerPtrs.push_back( new ProcessContainer(charmoniumSigmaPtrs[i]) );
  for (unsigned int i = 0; i < bottomoniumSigmaPtrs.size(); ++i)
    containerPtrs.push_back( new ProcessContainer(bottomoniumSigmaPtrs[i]) );

  // Set up requested objects for top production.
  bool tops = settings.flag("Top:all");
  if (tops || settings.flag("Top:gg2ttbar")) {
    sigmaPtr = make_shared<Sigma2gg2QQbar>(6, 601);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (tops || settings.flag("Top:qqbar2ttbar")) {
    sigmaPtr = make_shared<Sigma2qqbar2QQbar>(6, 602);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (tops || settings.flag("Top:qq2tq(t:W)")) {
    sigmaPtr = make_shared<Sigma2qq2QqtW>(6, 603);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (tops || settings.flag("Top:ffbar2ttbar(s:gmZ)")) {
    sigmaPtr = make_shared<Sigma2ffbar2FFbarsgmZ>(6, 604);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (tops || settings.flag("Top:ffbar2tqbar(s:W)")) {
    sigmaPtr = make_shared<Sigma2ffbar2FfbarsW>(6, 0, 605);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (tops || settings.flag("Top:gmgm2ttbar")) {
    sigmaPtr = make_shared<Sigma2gmgm2ffbar>(6, 606);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (tops || settings.flag("Top:ggm2ttbar")) {
    if ( initGammaB ) {
      sigmaPtr = make_shared<Sigma2ggm2qqbar>(6, 607, "ggm");
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if ( initGammaA ) {
      sigmaPtr = make_shared<Sigma2ggm2qqbar>(6, 617, "gmg");
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
  }

  // Set up requested objects for fourth-generation b' production
  bool bPrimes = settings.flag("FourthBottom:all");
  if (bPrimes || settings.flag("FourthBottom:gg2bPrimebPrimebar")) {
    sigmaPtr = make_shared<Sigma2gg2QQbar>(7, 801);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (bPrimes || settings.flag("FourthBottom:qqbar2bPrimebPrimebar")) {
    sigmaPtr = make_shared<Sigma2qqbar2QQbar>(7, 802);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (bPrimes || settings.flag("FourthBottom:qq2bPrimeq(t:W)")) {
    sigmaPtr = make_shared<Sigma2qq2QqtW>(7, 803);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (bPrimes || settings.flag("FourthBottom:ffbar2bPrimebPrimebar(s:gmZ)")) {
    sigmaPtr = make_shared<Sigma2ffbar2FFbarsgmZ>(7, 804);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (bPrimes || settings.flag("FourthBottom:ffbar2bPrimeqbar(s:W)")) {
    sigmaPtr = make_shared<Sigma2ffbar2FfbarsW>(7, 0, 805);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (bPrimes || settings.flag("FourthBottom:ffbar2bPrimetbar(s:W)")) {
    sigmaPtr = make_shared<Sigma2ffbar2FfbarsW>(7, 6, 806);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }

  // Set up requested objects for fourth-generation t' production
  bool tPrimes = settings.flag("FourthTop:all");
  if (tPrimes || settings.flag("FourthTop:gg2tPrimetPrimebar")) {
    sigmaPtr = make_shared<Sigma2gg2QQbar>(8, 821);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (tPrimes || settings.flag("FourthTop:qqbar2tPrimetPrimebar")) {
    sigmaPtr = make_shared<Sigma2qqbar2QQbar>(8, 822);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (tPrimes || settings.flag("FourthTop:qq2tPrimeq(t:W)")) {
    sigmaPtr = make_shared<Sigma2qq2QqtW>(8, 823);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (tPrimes || settings.flag("FourthTop:ffbar2tPrimetPrimebar(s:gmZ)")) {
    sigmaPtr = make_shared<Sigma2ffbar2FFbarsgmZ>(8, 824);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (tPrimes || settings.flag("FourthTop:ffbar2tPrimeqbar(s:W)")) {
    sigmaPtr = make_shared<Sigma2ffbar2FfbarsW>(8, 0, 825);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }

  // Set up requested objects for two different fourth-generation fermions.
  if (bPrimes || tPrimes
    || settings.flag("FourthPair:ffbar2tPrimebPrimebar(s:W)")) {
    sigmaPtr = make_shared<Sigma2ffbar2FfbarsW>(8, 7, 841);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (settings.flag("FourthPair:ffbar2tauPrimenuPrimebar(s:W)")) {
    sigmaPtr = make_shared<Sigma2ffbar2FfbarsW>(17, 18, 842);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }

  // Flag for global choice between SM and BSM Higgses.
  bool useBSMHiggses = settings.flag("Higgs:useBSM");

  // Set up requested objects for Standard-Model Higgs production.
  if (!useBSMHiggses) {
    bool HiggsesSM = settings.flag("HiggsSM:all");
    if (HiggsesSM || settings.flag("HiggsSM:ffbar2H")) {
      sigmaPtr = make_shared<Sigma1ffbar2H>(0);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (HiggsesSM || settings.flag("HiggsSM:gg2H")) {
     sigmaPtr = make_shared<Sigma1gg2H>(0);
     containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (HiggsesSM || settings.flag("HiggsSM:gmgm2H")) {
      sigmaPtr = make_shared<Sigma1gmgm2H>(0);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (HiggsesSM || settings.flag("HiggsSM:ffbar2HZ")) {
      sigmaPtr = make_shared<Sigma2ffbar2HZ>(0);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (HiggsesSM || settings.flag("HiggsSM:ffbar2HW")) {
      sigmaPtr = make_shared<Sigma2ffbar2HW>(0);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (HiggsesSM || settings.flag("HiggsSM:ff2Hff(t:ZZ)")) {
      sigmaPtr = make_shared<Sigma3ff2HfftZZ>(0);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (HiggsesSM || settings.flag("HiggsSM:ff2Hff(t:WW)")) {
      sigmaPtr = make_shared<Sigma3ff2HfftWW>(0);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (HiggsesSM || settings.flag("HiggsSM:gg2Httbar")) {
      sigmaPtr = make_shared<Sigma3gg2HQQbar>(6,0);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (HiggsesSM || settings.flag("HiggsSM:qqbar2Httbar")) {
      sigmaPtr = make_shared<Sigma3qqbar2HQQbar>(6,0);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }

    // Further Standard-Model Higgs processes, not included in "all".
    if (settings.flag("HiggsSM:qg2Hq")) {
      sigmaPtr = make_shared<Sigma2qg2Hq>(4,0);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
      sigmaPtr = make_shared<Sigma2qg2Hq>(5,0);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (settings.flag("HiggsSM:gg2Hbbbar")) {
      sigmaPtr = make_shared<Sigma3gg2HQQbar>(5,0);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (settings.flag("HiggsSM:qqbar2Hbbbar")) {
      sigmaPtr = make_shared<Sigma3qqbar2HQQbar>(5,0);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (settings.flag("HiggsSM:gg2Hg(l:t)")) {
      sigmaPtr = make_shared<Sigma2gg2Hglt>(0);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (settings.flag("HiggsSM:qg2Hq(l:t)")) {
      sigmaPtr = make_shared<Sigma2qg2Hqlt>(0);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (settings.flag("HiggsSM:qqbar2Hg(l:t)")) {
      sigmaPtr = make_shared<Sigma2qqbar2Hglt>(0);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
  }

  // Common switch for the group of Higgs production BSM.
  if (useBSMHiggses) {
    bool HiggsesBSM = settings.flag("HiggsBSM:all");

    // Set up requested objects for BSM H1 production.
    bool HiggsesH1 = settings.flag("HiggsBSM:allH1");
    if (HiggsesBSM || HiggsesH1 || settings.flag("HiggsBSM:ffbar2H1")) {
      sigmaPtr = make_shared<Sigma1ffbar2H>(1);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (HiggsesBSM || HiggsesH1 || settings.flag("HiggsBSM:gg2H1")) {
      sigmaPtr = make_shared<Sigma1gg2H>(1);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (HiggsesBSM || HiggsesH1 || settings.flag("HiggsBSM:gmgm2H1")) {
      sigmaPtr = make_shared<Sigma1gmgm2H>(1);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (HiggsesBSM || HiggsesH1 || settings.flag("HiggsBSM:ffbar2H1Z")) {
      sigmaPtr = make_shared<Sigma2ffbar2HZ>(1);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (HiggsesBSM || HiggsesH1 || settings.flag("HiggsBSM:ffbar2H1W")) {
      sigmaPtr = make_shared<Sigma2ffbar2HW>(1);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (HiggsesBSM || HiggsesH1 || settings.flag("HiggsBSM:ff2H1ff(t:ZZ)")) {
      sigmaPtr = make_shared<Sigma3ff2HfftZZ>(1);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (HiggsesBSM || HiggsesH1 || settings.flag("HiggsBSM:ff2H1ff(t:WW)")) {
      sigmaPtr = make_shared<Sigma3ff2HfftWW>(1);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (HiggsesBSM || HiggsesH1 || settings.flag("HiggsBSM:gg2H1ttbar")) {
      sigmaPtr = make_shared<Sigma3gg2HQQbar>(6,1);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (HiggsesBSM || HiggsesH1 || settings.flag("HiggsBSM:qqbar2H1ttbar")) {
      sigmaPtr = make_shared<Sigma3qqbar2HQQbar>(6,1);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }

    // Further BSM H1 processes, not included in "all".
    if (settings.flag("HiggsBSM:qg2H1q")) {
      sigmaPtr = make_shared<Sigma2qg2Hq>(4,1);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
      sigmaPtr = make_shared<Sigma2qg2Hq>(5,1);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (settings.flag("HiggsBSM:gg2H1bbbar")) {
      sigmaPtr = make_shared<Sigma3gg2HQQbar>(5,1);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (settings.flag("HiggsBSM:qqbar2H1bbbar")) {
      sigmaPtr = make_shared<Sigma3qqbar2HQQbar>(5,1);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (settings.flag("HiggsBSM:gg2H1g(l:t)")) {
      sigmaPtr = make_shared<Sigma2gg2Hglt>(1);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (settings.flag("HiggsBSM:qg2H1q(l:t)")) {
      sigmaPtr = make_shared<Sigma2qg2Hqlt>(1);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (settings.flag("HiggsBSM:qqbar2H1g(l:t)")) {
      sigmaPtr = make_shared<Sigma2qqbar2Hglt>(1);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }

    // Set up requested objects for BSM H2 production.
    bool HiggsesH2 = settings.flag("HiggsBSM:allH2");
    if (HiggsesBSM || HiggsesH2 || settings.flag("HiggsBSM:ffbar2H2")) {
      sigmaPtr = make_shared<Sigma1ffbar2H>(2);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (HiggsesBSM || HiggsesH2 || settings.flag("HiggsBSM:gg2H2")) {
      sigmaPtr = make_shared<Sigma1gg2H>(2);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (HiggsesBSM || HiggsesH2 || settings.flag("HiggsBSM:gmgm2H2")) {
      sigmaPtr = make_shared<Sigma1gmgm2H>(2);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (HiggsesBSM || HiggsesH2 || settings.flag("HiggsBSM:ffbar2H2Z")) {
      sigmaPtr = make_shared<Sigma2ffbar2HZ>(2);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (HiggsesBSM || HiggsesH2 || settings.flag("HiggsBSM:ffbar2H2W")) {
      sigmaPtr = make_shared<Sigma2ffbar2HW>(2);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (HiggsesBSM || HiggsesH2 || settings.flag("HiggsBSM:ff2H2ff(t:ZZ)")) {
      sigmaPtr = make_shared<Sigma3ff2HfftZZ>(2);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (HiggsesBSM || HiggsesH2 || settings.flag("HiggsBSM:ff2H2ff(t:WW)")) {
      sigmaPtr = make_shared<Sigma3ff2HfftWW>(2);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (HiggsesBSM || HiggsesH2 || settings.flag("HiggsBSM:gg2H2ttbar")) {
      sigmaPtr = make_shared<Sigma3gg2HQQbar>(6,2);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (HiggsesBSM || HiggsesH2 || settings.flag("HiggsBSM:qqbar2H2ttbar")) {
      sigmaPtr = make_shared<Sigma3qqbar2HQQbar>(6,2);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }

    // Further BSM H2 processes, not included in "all".
   if (settings.flag("HiggsBSM:qg2H2q")) {
      sigmaPtr = make_shared<Sigma2qg2Hq>(4,2);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
      sigmaPtr = make_shared<Sigma2qg2Hq>(5,2);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (settings.flag("HiggsBSM:gg2H2bbbar")) {
      sigmaPtr = make_shared<Sigma3gg2HQQbar>(5,2);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (settings.flag("HiggsBSM:qqbar2H2bbbar")) {
      sigmaPtr = make_shared<Sigma3qqbar2HQQbar>(5,2);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (settings.flag("HiggsBSM:gg2H2g(l:t)")) {
      sigmaPtr = make_shared<Sigma2gg2Hglt>(2);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (settings.flag("HiggsBSM:qg2H2q(l:t)")) {
      sigmaPtr = make_shared<Sigma2qg2Hqlt>(2);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (settings.flag("HiggsBSM:qqbar2H2g(l:t)")) {
      sigmaPtr = make_shared<Sigma2qqbar2Hglt>(2);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }

    // Set up requested objects for BSM A3 production.
    bool HiggsesA3 = settings.flag("HiggsBSM:allA3");
    if (HiggsesBSM || HiggsesA3 || settings.flag("HiggsBSM:ffbar2A3")) {
      sigmaPtr = make_shared<Sigma1ffbar2H>(3);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (HiggsesBSM || HiggsesA3 || settings.flag("HiggsBSM:gg2A3")) {
      sigmaPtr = make_shared<Sigma1gg2H>(3);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (HiggsesBSM || HiggsesA3 || settings.flag("HiggsBSM:gmgm2A3")) {
      sigmaPtr = make_shared<Sigma1gmgm2H>(3);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (HiggsesBSM || HiggsesA3 || settings.flag("HiggsBSM:ffbar2A3Z")) {
      sigmaPtr = make_shared<Sigma2ffbar2HZ>(3);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (HiggsesBSM || HiggsesA3 || settings.flag("HiggsBSM:ffbar2A3W")) {
      sigmaPtr = make_shared<Sigma2ffbar2HW>(3);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (HiggsesBSM || HiggsesA3 || settings.flag("HiggsBSM:ff2A3ff(t:ZZ)")) {
      sigmaPtr = make_shared<Sigma3ff2HfftZZ>(3);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (HiggsesBSM || HiggsesA3 || settings.flag("HiggsBSM:ff2A3ff(t:WW)")) {
      sigmaPtr = make_shared<Sigma3ff2HfftWW>(3);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (HiggsesBSM || HiggsesA3 || settings.flag("HiggsBSM:gg2A3ttbar")) {
      sigmaPtr = make_shared<Sigma3gg2HQQbar>(6,3);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (HiggsesBSM || HiggsesA3 || settings.flag("HiggsBSM:qqbar2A3ttbar")) {
      sigmaPtr = make_shared<Sigma3qqbar2HQQbar>(6,3);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }

    // Further BSM A3 processes, not included in "all".
    if (settings.flag("HiggsBSM:qg2A3q")) {
      sigmaPtr = make_shared<Sigma2qg2Hq>(4,3);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
      sigmaPtr = make_shared<Sigma2qg2Hq>(5,3);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (settings.flag("HiggsBSM:gg2A3bbbar")) {
      sigmaPtr = make_shared<Sigma3gg2HQQbar>(5,3);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (settings.flag("HiggsBSM:qqbar2A3bbbar")) {
      sigmaPtr = make_shared<Sigma3qqbar2HQQbar>(5,3);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (settings.flag("HiggsBSM:gg2A3g(l:t)")) {
      sigmaPtr = make_shared<Sigma2gg2Hglt>(3);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (settings.flag("HiggsBSM:qg2A3q(l:t)")) {
      sigmaPtr = make_shared<Sigma2qg2Hqlt>(3);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (settings.flag("HiggsBSM:qqbar2A3g(l:t)")) {
      sigmaPtr = make_shared<Sigma2qqbar2Hglt>(3);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }

    // Set up requested objects for Charged Higgs production
    bool HiggsesChg = settings.flag("HiggsBSM:allH+-");
    if (HiggsesBSM || HiggsesChg || settings.flag("HiggsBSM:ffbar2H+-")) {
      sigmaPtr = make_shared<Sigma1ffbar2Hchg>();
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (HiggsesBSM || HiggsesChg || settings.flag("HiggsBSM:bg2H+-t")) {
      sigmaPtr = make_shared<Sigma2qg2Hchgq>(6, 1062, "b g -> H+- t");
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }

    // Set up requested objects for Higgs pair-production
    bool HiggsesPairs = settings.flag("HiggsBSM:allHpair");
    if (HiggsesBSM || HiggsesPairs || settings.flag("HiggsBSM:ffbar2A3H1")) {
      sigmaPtr = make_shared<Sigma2ffbar2A3H12>(1);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (HiggsesBSM || HiggsesPairs || settings.flag("HiggsBSM:ffbar2A3H2")) {
      sigmaPtr = make_shared<Sigma2ffbar2A3H12>(2);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (HiggsesBSM || HiggsesPairs || settings.flag("HiggsBSM:ffbar2H+-H1")) {
      sigmaPtr = make_shared<Sigma2ffbar2HchgH12>(1);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (HiggsesBSM || HiggsesPairs || settings.flag("HiggsBSM:ffbar2H+-H2")) {
      sigmaPtr = make_shared<Sigma2ffbar2HchgH12>(2);
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
    if (HiggsesBSM || HiggsesPairs || settings.flag("HiggsBSM:ffbar2H+H-")) {
      sigmaPtr = make_shared<Sigma2ffbar2HposHneg>();
      containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
    }
  }

  // Set up requested objects for SUSY pair processes.
  CoupSUSY* coupSUSYPtr = infoPtr->coupSUSYPtr;
  if (coupSUSYPtr->isSUSY) {

    bool SUSYs = settings.flag("SUSY:all");
    bool nmssm = settings.flag("SLHA:NMSSM");

    // Preselected SUSY codes.
    setupIdVecs( settings);

    // MSSM: 4 neutralinos
    int nNeut = 4;
    if (nmssm) nNeut = 5;

    // Gluino-gluino
    if (SUSYs || settings.flag("SUSY:gg2gluinogluino")) {
      // Skip if outgoing codes not asked for
      if (allowIdVals( 1000021, 1000021)) {
        sigmaPtr = make_shared<Sigma2gg2gluinogluino>();
        containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
      }
    }
    if (SUSYs || settings.flag("SUSY:qqbar2gluinogluino")) {
      // Skip if outgoing codes not asked for
      if (allowIdVals( 1000021, 1000021)) {
        sigmaPtr = make_shared<Sigma2qqbar2gluinogluino>();
        containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
      }
    }

    // Gluino-squark
    if (SUSYs || settings.flag("SUSY:qg2squarkgluino")) {
      int iproc = 1202;
      for (int idx = 1; idx <= 6; ++idx) {
        for (int iso = 1; iso <= 2; ++iso) {
          iproc += 2;
          int id3 = iso + ((idx <= 3)
                  ? 1000000+2*(idx-1) : 2000000+2*(idx-4));
          int id4 = 1000021;
          // Skip if outgoing codes not asked for
          if (!allowIdVals( id3, id4)) continue;
          sigmaPtr = make_shared<Sigma2qg2squarkgluino>(id3,iproc-1);
          containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
          sigmaPtr = make_shared<Sigma2qg2squarkgluino>(-id3,iproc);
          containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
        }
      }
    }

    // Squark-antisquark (gg initiated)
    if (SUSYs || settings.flag("SUSY:gg2squarkantisquark")) {
      int iproc = 1214;
      for (int idx = 1; idx <= 6; ++idx) {
        for (int iso = 1; iso <= 2; ++iso) {
          iproc++;
          int id = iso + ((idx <= 3)
                 ? 1000000+2*(idx-1) : 2000000+2*(idx-4));
          // Skip if outgoing codes not asked for
          if (!allowIdVals( id, id)) continue;
          sigmaPtr = make_shared<Sigma2gg2squarkantisquark>(id,iproc);
          containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
        }
      }
    }

    // Squark-antisquark (qqbar initiated)
    if (SUSYs || settings.flag("SUSY:qqbar2squarkantisquark")) {
      int iproc = 1230;
      for (int idx = 1; idx <= 6; ++idx) {
        for (int iso = 1; iso <= 2; ++iso) {
          for (int jso = iso; jso >= 1; --jso) {
            for (int jdx = 1; jdx <= 6; ++jdx) {
              if (iso == jso && jdx < idx) continue;
              int id1 = iso + ((idx <= 3) ? 1000000+2*(idx-1)
                               : 2000000+2*(idx-4));
              int id2 = jso + ((jdx <= 3) ? 1000000+2*(jdx-1)
                               : 2000000+2*(jdx-4));
              // Update process number counter (for ~q~q, +2 if not self-conj)
              //if (iproc == 1302) iproc=1310;
              iproc++;
              if (iso == jso && id1 != id2) iproc++;
              // Skip if outgoing codes not asked for
              if (!allowIdVals( id1, id2)) continue;
              if (iso == jso && id1 != id2) {
                sigmaPtr = make_shared<Sigma2qqbar2squarkantisquark>
                  (id1,-id2,iproc-1);
                containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
                sigmaPtr = make_shared<Sigma2qqbar2squarkantisquark>
                  (id2,-id1,iproc);
                containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
              } else {
                sigmaPtr = make_shared<Sigma2qqbar2squarkantisquark>
                  (id1,-id2,iproc);
                containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
              }
            }
          }
        }
      }
    }

    // Squark-squark
    if (SUSYs || settings.flag("SUSY:qq2squarksquark")) {
      int iproc = 1350;
      for (int idx = 1; idx <= 6; ++idx) {
        for (int iso = 1; iso <= 2; ++iso) {
          for (int jso = iso; jso >= 1; jso--) {
            for (int jdx = 1; jdx <= 6; ++jdx) {
              if (iso == jso && jdx < idx) continue;
              iproc++;
              int id1 = iso + ((idx <= 3)
                      ? 1000000+2*(idx-1) : 2000000+2*(idx-4));
              int id2 = jso + ((jdx <= 3)
                      ? 1000000+2*(jdx-1) : 2000000+2*(jdx-4));
              // Skip if outgoing codes not asked for.
              if (!allowIdVals( id1, id2)) continue;
              sigmaPtr = make_shared<Sigma2qq2squarksquark>(id1,id2,iproc);
              containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
            }
          }
        }
      }
    }

    // Neutralino + squark
    if (SUSYs || settings.flag("SUSY:qg2chi0squark")) {
      int iproc = 1430;
      for (int iNeut= 1; iNeut <= nNeut; iNeut++) {
        for (int idx = 1; idx <= 6; idx++) {
          bool isUp = false;
          for (int iso = 1; iso <= 2; iso++) {
            if (iso == 2) isUp = true;
            iproc++;
            int id3 = coupSUSYPtr->idNeut(iNeut);
            int id4 = iso + ((idx <= 3)
                    ? 1000000+2*(idx-1) : 2000000+2*(idx-4));
            // Skip if outgoing codes not asked for
            if (!allowIdVals( id3, id4)) continue;
            sigmaPtr = make_shared<Sigma2qg2chi0squark>(iNeut,idx,isUp,iproc);
            containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
          }
        }
      }
    }

    // Chargino + squark
    if (SUSYs || settings.flag("SUSY:qg2chi+-squark")) {
      int iproc = 1490;
      for (int iChar = 1; iChar <= 2; iChar++) {
        for (int idx = 1; idx <= 6; idx++) {
          bool isUp = false;
          for (int iso = 1; iso <= 2; iso++) {
            if (iso == 2) isUp = true;
            iproc++;
            int id3 = coupSUSYPtr->idChar(iChar);
            int id4 = iso + ((idx <= 3)
                    ? 1000000+2*(idx-1) : 2000000+2*(idx-4));
            // Skip if outgoing codes not asked for
            if (!allowIdVals( id3, id4)) continue;
            sigmaPtr = make_shared<Sigma2qg2charsquark>(iChar,idx,isUp,iproc);
            containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
          }
        }
      }
    }

    // Neutralino pairs
    if (SUSYs || settings.flag("SUSY:qqbar2chi0chi0")) {
      int iproc = 1550;
      for (int iNeut2 = 1; iNeut2 <= nNeut; iNeut2++) {
        for (int iNeut1 = 1; iNeut1 <= iNeut2; iNeut1++) {
          iproc++;
          // Skip if outgoing codes not asked for
          if (!allowIdVals( coupSUSYPtr->idNeut(iNeut1),
            coupSUSYPtr->idNeut(iNeut2) ) ) continue;
          sigmaPtr = make_shared<Sigma2qqbar2chi0chi0>(iNeut1, iNeut2,iproc);
          containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
        }
      }
    }

    // Neutralino-Chargino
    if (SUSYs || settings.flag("SUSY:qqbar2chi+-chi0")) {
      int iproc = 1570;
      for (int iNeut = 1; iNeut <= nNeut; iNeut++) {
        for (int iChar = 1; iChar <= 2; ++iChar) {
          iproc += 2;
          // Skip if outgoing codes not asked for
          if (!allowIdVals( coupSUSYPtr->idNeut(iNeut),
            coupSUSYPtr->idChar(iChar) ) ) continue;
          sigmaPtr = make_shared<Sigma2qqbar2charchi0>( iChar, iNeut, iproc-1);
          containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
          sigmaPtr = make_shared<Sigma2qqbar2charchi0>(-iChar, iNeut, iproc);
          containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
        }
      }
    }

    // Chargino-Chargino
    if (SUSYs || settings.flag("SUSY:qqbar2chi+chi-")) {
      int iproc = 1590;
      for (int i = 1; i <= 2; ++i) {
        for (int j = 1; j <= 2; ++j) {
          iproc++;
          // Skip if outgoing codes not asked for
          if (!allowIdVals( coupSUSYPtr->idChar(i),
            coupSUSYPtr->idChar(j) ) ) continue;
          sigmaPtr = make_shared<Sigma2qqbar2charchar>( i,-j, iproc);
          containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
        }
      }
    }

    // RPV squark production
    if(SUSYs || settings.flag("SUSY:qq2antisquark")) {
      for (int idx = 1; idx <= 6; ++idx) {
        for (int iso = 1; iso <= 2; ++iso) {
          int id1 = iso + ((idx <= 3) ? 1000000+2*(idx-1) : 2000000+2*(idx-4));
          // Skip if outgoing code not asked for
          if (!allowIdVals( id1, 0)) continue;
          sigmaPtr = make_shared<Sigma1qq2antisquark>(id1);
          containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
        }
      }
    }

    // Neutralino-gluino
    if (SUSYs || settings.flag("SUSY:qqbar2chi0gluino")) {
      int iproc = 1600;
      for (int iNeut = 1; iNeut <= nNeut; iNeut++) {
        iproc++;
        // Skip if outgoing codes not asked for
        if (!allowIdVals( coupSUSYPtr->idNeut(iNeut), 1000021)) continue;
        sigmaPtr = make_shared<Sigma2qqbar2chi0gluino>(iNeut, iproc);
        containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
      }
    }

    // Chargino-Gluino
    if (SUSYs || settings.flag("SUSY:qqbar2chi+-gluino")) {
      int iproc = 1620;
      for (int iChar = 1; iChar <= 2; ++iChar) {
        iproc += 2;
        // Skip if outgoing codes not asked for
        if (!allowIdVals( coupSUSYPtr->idChar(iChar), 1000021)) continue;
        sigmaPtr = make_shared<Sigma2qqbar2chargluino>( iChar, iproc-1);
        containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
        sigmaPtr = make_shared<Sigma2qqbar2chargluino>( -iChar, iproc);
        containerPtrs.push_back( new ProcessContainer(sigmaPtr) );

      }
    }

    // Slepton-antislepton (qqbar initiated); Currently no RH sneutrinos
    if (SUSYs || settings.flag("SUSY:qqbar2sleptonantislepton")) {
      int iproc = 1650;
      for (int idx = 1; idx <= 6; ++idx) {
        for (int iso = 1; iso <= 2; ++iso) {
          for (int jso = iso; jso >= 1; --jso) {
            for (int jdx = 1; jdx <= 6; ++jdx) {
              if (iso == jso && jdx < idx) continue;
              int id1 = iso + ((idx <= 3) ? 1000010+2*(idx-1)
                               : 2000010+2*(idx-4));
              int id2 = jso + ((jdx <= 3) ? 1000010+2*(jdx-1)
                               : 2000010+2*(jdx-4));
              // Update process number counter
              iproc++;
              if (iso == jso && id1 != id2) iproc++;
              // Exclude RH neutrinos from allowed final states
              if (abs(id1) >= 2000012 && id1 % 2 == 0) continue;
              if (abs(id2) >= 2000012 && id2 % 2 == 0) continue;
              // Skip if outgoing codes not asked for
              if (!allowIdVals( id1, id2)) continue;
              if (iso == jso && id1 != id2) {
                sigmaPtr
                  = make_shared<Sigma2qqbar2sleptonantislepton>
                  (id1,-id2,iproc-1);
                containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
                sigmaPtr = make_shared<Sigma2qqbar2sleptonantislepton>
                  (id2,-id1,iproc);
                containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
              } else {
                sigmaPtr = make_shared<Sigma2qqbar2sleptonantislepton>
                  (id1,-id2,iproc);
                containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
              }
            }
          }
        }
      }
    }

  } // End of SUSY processes.

  // Set up requested objects for New-Gauge-Boson processes.
  if (settings.flag("NewGaugeBoson:ffbar2gmZZprime")) {
    sigmaPtr = make_shared<Sigma1ffbar2gmZZprime>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (settings.flag("NewGaugeBoson:ffbar2Wprime")) {
    sigmaPtr = make_shared<Sigma1ffbar2Wprime>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (settings.flag("NewGaugeBoson:ffbar2R0")) {
    sigmaPtr = make_shared<Sigma1ffbar2Rhorizontal>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }

  // Set up requested objects for Left-Right-Symmetry processes.
  bool leftrights = settings.flag("LeftRightSymmmetry:all");
  if (leftrights || settings.flag("LeftRightSymmmetry:ffbar2ZR")) {
    sigmaPtr = make_shared<Sigma1ffbar2ZRight>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (leftrights || settings.flag("LeftRightSymmmetry:ffbar2WR")) {
    sigmaPtr = make_shared<Sigma1ffbar2WRight>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (leftrights || settings.flag("LeftRightSymmmetry:ll2HL")) {
    sigmaPtr = make_shared<Sigma1ll2Hchgchg>(1);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (leftrights || settings.flag("LeftRightSymmmetry:lgm2HLe")) {
    sigmaPtr = make_shared<Sigma2lgm2Hchgchgl>(1, 11);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (leftrights || settings.flag("LeftRightSymmmetry:lgm2HLmu")) {
    sigmaPtr = make_shared<Sigma2lgm2Hchgchgl>(1, 13);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (leftrights || settings.flag("LeftRightSymmmetry:lgm2HLtau")) {
    sigmaPtr = make_shared<Sigma2lgm2Hchgchgl>(1, 15);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (leftrights || settings.flag("LeftRightSymmmetry:ff2HLff")) {
    sigmaPtr = make_shared<Sigma3ff2HchgchgfftWW>(1);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (leftrights || settings.flag("LeftRightSymmmetry:ffbar2HLHL")) {
    sigmaPtr = make_shared<Sigma2ffbar2HchgchgHchgchg>(1);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (leftrights || settings.flag("LeftRightSymmmetry:ll2HR")) {
    sigmaPtr = make_shared<Sigma1ll2Hchgchg>(2);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (leftrights || settings.flag("LeftRightSymmmetry:lgm2HRe")) {
    sigmaPtr = make_shared<Sigma2lgm2Hchgchgl>(2, 11);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (leftrights || settings.flag("LeftRightSymmmetry:lgm2HRmu")) {
    sigmaPtr = make_shared<Sigma2lgm2Hchgchgl>(2, 13);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (leftrights || settings.flag("LeftRightSymmmetry:lgm2HRtau")) {
    sigmaPtr = make_shared<Sigma2lgm2Hchgchgl>(2, 15);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (leftrights || settings.flag("LeftRightSymmmetry:ff2HRff")) {
    sigmaPtr = make_shared<Sigma3ff2HchgchgfftWW>(2);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (leftrights || settings.flag("LeftRightSymmmetry:ffbar2HRHR")) {
    sigmaPtr = make_shared<Sigma2ffbar2HchgchgHchgchg>(2);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }

  // Set up requested objects for leptoquark LQ processes.
  bool leptoquarks = settings.flag("LeptoQuark:all");
  if (leptoquarks || settings.flag("LeptoQuark:ql2LQ")) {
    sigmaPtr = make_shared<Sigma1ql2LeptoQuark>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (leptoquarks || settings.flag("LeptoQuark:qg2LQl")) {
    sigmaPtr = make_shared<Sigma2qg2LeptoQuarkl>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (leptoquarks || settings.flag("LeptoQuark:gg2LQLQbar")) {
    sigmaPtr = make_shared<Sigma2gg2LQLQbar>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (leptoquarks || settings.flag("LeptoQuark:qqbar2LQLQbar")) {
    sigmaPtr = make_shared<Sigma2qqbar2LQLQbar>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }

  // Set up requested objects for excited-fermion processes.
  bool excitedfermions = settings.flag("ExcitedFermion:all");
  if (excitedfermions || settings.flag("ExcitedFermion:dg2dStar")) {
    sigmaPtr = make_shared<Sigma1qg2qStar>(1);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (excitedfermions || settings.flag("ExcitedFermion:ug2uStar")) {
    sigmaPtr = make_shared<Sigma1qg2qStar>(2);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (excitedfermions || settings.flag("ExcitedFermion:sg2sStar")) {
    sigmaPtr = make_shared<Sigma1qg2qStar>(3);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (excitedfermions || settings.flag("ExcitedFermion:cg2cStar")) {
    sigmaPtr = make_shared<Sigma1qg2qStar>(4);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (excitedfermions || settings.flag("ExcitedFermion:bg2bStar")) {
    sigmaPtr = make_shared<Sigma1qg2qStar>(5);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (excitedfermions || settings.flag("ExcitedFermion:egm2eStar")) {
    sigmaPtr = make_shared<Sigma1lgm2lStar>(11);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (excitedfermions || settings.flag("ExcitedFermion:mugm2muStar")) {
    sigmaPtr = make_shared<Sigma1lgm2lStar>(13);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (excitedfermions || settings.flag("ExcitedFermion:taugm2tauStar")) {
    sigmaPtr = make_shared<Sigma1lgm2lStar>(15);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (excitedfermions || settings.flag("ExcitedFermion:qq2dStarq")) {
    sigmaPtr = make_shared<Sigma2qq2qStarq>(1);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (excitedfermions || settings.flag("ExcitedFermion:qq2uStarq")) {
    sigmaPtr = make_shared<Sigma2qq2qStarq>(2);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (excitedfermions || settings.flag("ExcitedFermion:qq2sStarq")) {
    sigmaPtr = make_shared<Sigma2qq2qStarq>(3);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (excitedfermions || settings.flag("ExcitedFermion:qq2cStarq")) {
    sigmaPtr = make_shared<Sigma2qq2qStarq>(4);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (excitedfermions || settings.flag("ExcitedFermion:qq2bStarq")) {
    sigmaPtr = make_shared<Sigma2qq2qStarq>(5);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (excitedfermions || settings.flag("ExcitedFermion:qqbar2eStare")) {
    sigmaPtr = make_shared<Sigma2qqbar2lStarlbar>(11);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (excitedfermions || settings.flag("ExcitedFermion:qqbar2nueStarnue")) {
    sigmaPtr = make_shared<Sigma2qqbar2lStarlbar>(12);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (excitedfermions || settings.flag("ExcitedFermion:qqbar2muStarmu")) {
    sigmaPtr = make_shared<Sigma2qqbar2lStarlbar>(13);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (excitedfermions || settings.flag("ExcitedFermion:qqbar2numuStarnumu")) {
    sigmaPtr = make_shared<Sigma2qqbar2lStarlbar>(14);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (excitedfermions || settings.flag("ExcitedFermion:qqbar2tauStartau")) {
    sigmaPtr = make_shared<Sigma2qqbar2lStarlbar>(15);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (excitedfermions
    || settings.flag("ExcitedFermion:qqbar2nutauStarnutau")) {
    sigmaPtr = make_shared<Sigma2qqbar2lStarlbar>(16);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (excitedfermions || settings.flag("ExcitedFermion:qqbar2eStareStar")) {
    sigmaPtr = make_shared<Sigma2qqbar2lStarlStarBar>(11);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (excitedfermions
    || settings.flag("ExcitedFermion:qqbar2nueStarnueStar")) {
    sigmaPtr = make_shared<Sigma2qqbar2lStarlStarBar>(12);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (excitedfermions || settings.flag("ExcitedFermion:qqbar2muStarmuStar")) {
    sigmaPtr = make_shared<Sigma2qqbar2lStarlStarBar>(13);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (excitedfermions
    || settings.flag("ExcitedFermion:qqbar2numuStarnumuStar")) {
    sigmaPtr = make_shared<Sigma2qqbar2lStarlStarBar>(14);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (excitedfermions
    || settings.flag("ExcitedFermion:qqbar2tauStartauStar")) {
    sigmaPtr = make_shared<Sigma2qqbar2lStarlStarBar>(15);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (excitedfermions
    || settings.flag("ExcitedFermion:qqbar2nutauStarnutauStar")) {
    sigmaPtr = make_shared<Sigma2qqbar2lStarlStarBar>(16);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }

  // Set up requested objects for contact interaction processes.
  if (settings.flag("ContactInteractions:QCqq2qq")) {
    sigmaPtr = make_shared<Sigma2QCqq2qq>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (settings.flag("ContactInteractions:QCqqbar2qqbar")) {
    sigmaPtr = make_shared<Sigma2QCqqbar2qqbar>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (settings.flag("ContactInteractions:QCffbar2eebar")) {
    sigmaPtr = make_shared<Sigma2QCffbar2llbar>(11, 4203);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (settings.flag("ContactInteractions:QCffbar2mumubar")) {
    sigmaPtr = make_shared<Sigma2QCffbar2llbar>(13, 4204);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (settings.flag("ContactInteractions:QCffbar2tautaubar")) {
    sigmaPtr = make_shared<Sigma2QCffbar2llbar>(15, 4205);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }

  // Set spin of particles in the Hidden Valley scenario.
  int spinFv = settings.mode("HiddenValley:spinFv");
  int nFlavV = settings.mode("HiddenValley:nFlav");
  for (int i = 1; i < 7; ++i) {
    if (particleDataPtr->spinType( 4900000 + i) != spinFv + 1)
        particleDataPtr->spinType( 4900000 + i,    spinFv + 1);
    if (particleDataPtr->spinType( 4900010 + i) != spinFv + 1)
        particleDataPtr->spinType( 4900010 + i,    spinFv + 1);
  }
  if (spinFv != 1) {
    for (int i = 1; i <= nFlavV; ++i)
      if (particleDataPtr->spinType( 4900100 + i) != 2)
        particleDataPtr->spinType( 4900100 + i, 2);
  } else {
    int spinqv = settings.mode("HiddenValley:spinqv");
    for (int i = 1; i <= nFlavV; ++i)
      if (particleDataPtr->spinType( 4900100 + i) != 2 * spinqv + 1)
        particleDataPtr->spinType( 4900100 + i,    2 * spinqv + 1);
  }

  // Set up requested objects for HiddenValley processes.
  bool hiddenvalleys = settings.flag("HiddenValley:all");
  if (hiddenvalleys || settings.flag("HiddenValley:gg2DvDvbar")) {
    sigmaPtr = make_shared<Sigma2gg2qGqGbar>( 4900001, 4901, spinFv,
      "g g -> Dv Dvbar");
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (hiddenvalleys || settings.flag("HiddenValley:gg2UvUvbar")) {
    sigmaPtr = make_shared<Sigma2gg2qGqGbar>( 4900002, 4902, spinFv,
      "g g -> Uv Uvbar");
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (hiddenvalleys || settings.flag("HiddenValley:gg2SvSvbar")) {
    sigmaPtr = make_shared<Sigma2gg2qGqGbar>( 4900003, 4903, spinFv,
      "g g -> Sv Svbar");
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (hiddenvalleys || settings.flag("HiddenValley:gg2CvCvbar")) {
    sigmaPtr = make_shared<Sigma2gg2qGqGbar>( 4900004, 4904, spinFv,
      "g g -> Cv Cvbar");
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (hiddenvalleys || settings.flag("HiddenValley:gg2BvBvbar")) {
    sigmaPtr = make_shared<Sigma2gg2qGqGbar>( 4900005, 4905, spinFv,
      "g g -> Bv Bvbar");
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (hiddenvalleys || settings.flag("HiddenValley:gg2TvTvbar")) {
    sigmaPtr = make_shared<Sigma2gg2qGqGbar>( 4900006, 4906, spinFv,
      "g g -> Tv Tvbar");
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (hiddenvalleys || settings.flag("HiddenValley:qqbar2DvDvbar")) {
    sigmaPtr = make_shared<Sigma2qqbar2qGqGbar>( 4900001, 4911, spinFv,
      "q qbar -> Dv Dvbar");
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (hiddenvalleys || settings.flag("HiddenValley:qqbar2UvUvbar")) {
    sigmaPtr = make_shared<Sigma2qqbar2qGqGbar>( 4900002, 4912, spinFv,
      "q qbar -> Uv Uvbar");
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (hiddenvalleys || settings.flag("HiddenValley:qqbar2SvSvbar")) {
    sigmaPtr = make_shared<Sigma2qqbar2qGqGbar>( 4900003, 4913, spinFv,
      "q qbar -> Sv Svbar");
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (hiddenvalleys || settings.flag("HiddenValley:qqbar2CvCvbar")) {
    sigmaPtr = make_shared<Sigma2qqbar2qGqGbar>( 4900004, 4914, spinFv,
      "q qbar -> Cv Cvbar");
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (hiddenvalleys || settings.flag("HiddenValley:qqbar2BvBvbar")) {
    sigmaPtr = make_shared<Sigma2qqbar2qGqGbar>( 4900005, 4915, spinFv,
      "q qbar -> Bv Bvbar");
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (hiddenvalleys || settings.flag("HiddenValley:qqbar2TvTvbar")) {
    sigmaPtr = make_shared<Sigma2qqbar2qGqGbar>( 4900006, 4916, spinFv,
      "q qbar -> Tv Tvbar");
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (hiddenvalleys || settings.flag("HiddenValley:ffbar2DvDvbar")) {
    sigmaPtr = make_shared<Sigma2ffbar2fGfGbar>( 4900001, 4921, spinFv,
      "f fbar -> Dv Dvbar");
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (hiddenvalleys || settings.flag("HiddenValley:ffbar2UvUvbar")) {
    sigmaPtr = make_shared<Sigma2ffbar2fGfGbar>( 4900002, 4922, spinFv,
      "f fbar -> Uv Uvbar");
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (hiddenvalleys || settings.flag("HiddenValley:ffbar2SvSvbar")) {
    sigmaPtr = make_shared<Sigma2ffbar2fGfGbar>( 4900003, 4923, spinFv,
      "f fbar -> Sv Svbar");
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (hiddenvalleys || settings.flag("HiddenValley:ffbar2CvCvbar")) {
    sigmaPtr = make_shared<Sigma2ffbar2fGfGbar>( 4900004, 4924, spinFv,
      "f fbar -> Cv Cvbar");
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (hiddenvalleys || settings.flag("HiddenValley:ffbar2BvBvbar")) {
    sigmaPtr = make_shared<Sigma2ffbar2fGfGbar>( 4900005, 4925, spinFv,
      "f fbar -> Bv Bvbar");
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (hiddenvalleys || settings.flag("HiddenValley:ffbar2TvTvbar")) {
    sigmaPtr = make_shared<Sigma2ffbar2fGfGbar>( 4900006, 4926, spinFv,
      "f fbar -> Tv Tvbar");
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (hiddenvalleys || settings.flag("HiddenValley:ffbar2EvEvbar")) {
    sigmaPtr = make_shared<Sigma2ffbar2fGfGbar>( 4900011, 4931, spinFv,
      "f fbar -> Ev Evbar");
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (hiddenvalleys || settings.flag("HiddenValley:ffbar2nuEvnuEvbar")) {
    sigmaPtr = make_shared<Sigma2ffbar2fGfGbar>( 4900012, 4932, spinFv,
      "f fbar -> nuEv nuEvbar");
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (hiddenvalleys || settings.flag("HiddenValley:ffbar2MUvMUvbar")) {
    sigmaPtr = make_shared<Sigma2ffbar2fGfGbar>( 4900013, 4933, spinFv,
      "f fbar -> MUv MUvbar");
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (hiddenvalleys || settings.flag("HiddenValley:ffbar2nuMUvnuMUvbar")) {
    sigmaPtr = make_shared<Sigma2ffbar2fGfGbar>( 4900014, 4934, spinFv,
      "f fbar -> nuMUv nuMUvbar");
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (hiddenvalleys || settings.flag("HiddenValley:ffbar2TAUvTAUvbar")) {
    sigmaPtr = make_shared<Sigma2ffbar2fGfGbar>( 4900015, 4935, spinFv,
      "f fbar -> TAUv TAUvbar");
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (hiddenvalleys || settings.flag("HiddenValley:ffbar2nuTAUvnuTAUvbar")) {
    sigmaPtr = make_shared<Sigma2ffbar2fGfGbar>( 4900016, 4936, spinFv,
      "f fbar -> nuTAUv nuTAUvbar");
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (hiddenvalleys || settings.flag("HiddenValley:ffbar2Zv")) {
    sigmaPtr = make_shared<Sigma1ffbar2Zv>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }

  // Set up requested objects for RS extra-dimensional G* processes.
  bool extraDimGstars = settings.flag("ExtraDimensionsG*:all");
  if (extraDimGstars || settings.flag("ExtraDimensionsG*:gg2G*")) {
    sigmaPtr = make_shared<Sigma1gg2GravitonStar>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (extraDimGstars || settings.flag("ExtraDimensionsG*:ffbar2G*")) {
    sigmaPtr = make_shared<Sigma1ffbar2GravitonStar>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (settings.flag("ExtraDimensionsG*:gg2G*g")) {
    sigmaPtr = make_shared<Sigma2gg2GravitonStarg>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (settings.flag("ExtraDimensionsG*:qg2G*q")) {
    sigmaPtr = make_shared<Sigma2qg2GravitonStarq>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (settings.flag("ExtraDimensionsG*:qqbar2G*g")) {
    sigmaPtr = make_shared<Sigma2qqbar2GravitonStarg>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }

  //  Set up requested objects for RS extra-dimensional KKgluon processes.
  if (settings.flag("ExtraDimensionsG*:qqbar2KKgluon*")) {
    sigmaPtr = make_shared<Sigma1qqbar2KKgluonStar>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }

  // NOAM: Set up requested objects for TEV extra-dimensional processes.
  if (settings.flag("ExtraDimensionsTEV:ffbar2ddbar")) {
    sigmaPtr = make_shared<Sigma2ffbar2TEVffbar>(1, 5061);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (settings.flag("ExtraDimensionsTEV:ffbar2uubar")) {
    sigmaPtr = make_shared<Sigma2ffbar2TEVffbar>(2, 5062);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (settings.flag("ExtraDimensionsTEV:ffbar2ssbar")) {
    sigmaPtr = make_shared<Sigma2ffbar2TEVffbar>(3, 5063);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (settings.flag("ExtraDimensionsTEV:ffbar2ccbar")) {
    sigmaPtr = make_shared<Sigma2ffbar2TEVffbar>(4, 5064);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (settings.flag("ExtraDimensionsTEV:ffbar2bbbar")) {
    sigmaPtr = make_shared<Sigma2ffbar2TEVffbar>(5, 5065);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (settings.flag("ExtraDimensionsTEV:ffbar2ttbar")) {
    sigmaPtr = make_shared<Sigma2ffbar2TEVffbar>(6, 5066);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (settings.flag("ExtraDimensionsTEV:ffbar2e+e-")) {
    sigmaPtr = make_shared<Sigma2ffbar2TEVffbar>(11, 5071);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (settings.flag("ExtraDimensionsTEV:ffbar2nuenuebar")) {
    sigmaPtr = make_shared<Sigma2ffbar2TEVffbar>(12, 5072);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (settings.flag("ExtraDimensionsTEV:ffbar2mu+mu-")) {
    sigmaPtr = make_shared<Sigma2ffbar2TEVffbar>(13, 5073);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (settings.flag("ExtraDimensionsTEV:ffbar2numunumubar")) {
    sigmaPtr = make_shared<Sigma2ffbar2TEVffbar>(14, 5074);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (settings.flag("ExtraDimensionsTEV:ffbar2tau+tau-")) {
    sigmaPtr = make_shared<Sigma2ffbar2TEVffbar>(15, 5075);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (settings.flag("ExtraDimensionsTEV:ffbar2nutaunutaubar")) {
    sigmaPtr = make_shared<Sigma2ffbar2TEVffbar>(16, 5076);
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }

  // Set up requested objects for large extra-dimensional G processes.
  bool extraDimLEDmono = settings.flag("ExtraDimensionsLED:monojet");
  if (extraDimLEDmono || settings.flag("ExtraDimensionsLED:gg2Gg")) {
    sigmaPtr = make_shared<Sigma2gg2LEDUnparticleg>( true );
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (extraDimLEDmono || settings.flag("ExtraDimensionsLED:qg2Gq")) {
    sigmaPtr = make_shared<Sigma2qg2LEDUnparticleq>( true );
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (extraDimLEDmono || settings.flag("ExtraDimensionsLED:qqbar2Gg")) {
    sigmaPtr = make_shared<Sigma2qqbar2LEDUnparticleg>( true );
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (settings.flag("ExtraDimensionsLED:ffbar2GZ")) {
    sigmaPtr = make_shared<Sigma2ffbar2LEDUnparticleZ>( true );
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (settings.flag("ExtraDimensionsLED:ffbar2Ggamma")) {
    sigmaPtr = make_shared<Sigma2ffbar2LEDUnparticlegamma>( true );
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (settings.flag("ExtraDimensionsLED:ffbar2gammagamma")) {
    sigmaPtr = make_shared<Sigma2ffbar2LEDgammagamma>( true );
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (settings.flag("ExtraDimensionsLED:gg2gammagamma")) {
    sigmaPtr = make_shared<Sigma2gg2LEDgammagamma>( true );
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (settings.flag("ExtraDimensionsLED:ffbar2llbar")) {
    sigmaPtr = make_shared<Sigma2ffbar2LEDllbar>( true );
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (settings.flag("ExtraDimensionsLED:gg2llbar")) {
    sigmaPtr = make_shared<Sigma2gg2LEDllbar>( true );
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  bool extraDimLEDdij = settings.flag("ExtraDimensionsLED:dijets");
  if (extraDimLEDdij || settings.flag("ExtraDimensionsLED:gg2DJgg")) {
    sigmaPtr = make_shared<Sigma2gg2LEDgg>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (extraDimLEDdij || settings.flag("ExtraDimensionsLED:gg2DJqqbar")) {
    sigmaPtr = make_shared<Sigma2gg2LEDqqbar>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (extraDimLEDdij || settings.flag("ExtraDimensionsLED:qg2DJqg")) {
    sigmaPtr = make_shared<Sigma2qg2LEDqg>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (extraDimLEDdij || settings.flag("ExtraDimensionsLED:qq2DJqq")) {
    sigmaPtr = make_shared<Sigma2qq2LEDqq>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (extraDimLEDdij || settings.flag("ExtraDimensionsLED:qqbar2DJgg")) {
    sigmaPtr = make_shared<Sigma2qqbar2LEDgg>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (extraDimLEDdij || settings.flag("ExtraDimensionsLED:qqbar2DJqqbarNew")) {
    sigmaPtr = make_shared<Sigma2qqbar2LEDqqbarNew>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }

  // Set up requested objects for unparticle processes.
  bool extraDimUnpartmono = settings.flag("ExtraDimensionsUnpart:monojet");
  if (extraDimUnpartmono || settings.flag("ExtraDimensionsUnpart:gg2Ug")) {
    sigmaPtr = make_shared<Sigma2gg2LEDUnparticleg>( false );
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (extraDimUnpartmono || settings.flag("ExtraDimensionsUnpart:qg2Uq")) {
    sigmaPtr = make_shared<Sigma2qg2LEDUnparticleq>( false );
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (extraDimUnpartmono || settings.flag("ExtraDimensionsUnpart:qqbar2Ug")) {
    sigmaPtr = make_shared<Sigma2qqbar2LEDUnparticleg>( false );
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (settings.flag("ExtraDimensionsUnpart:ffbar2UZ")) {
    sigmaPtr = make_shared<Sigma2ffbar2LEDUnparticleZ>( false );
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (settings.flag("ExtraDimensionsUnpart:ffbar2Ugamma")) {
    sigmaPtr = make_shared<Sigma2ffbar2LEDUnparticlegamma>( false );
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (settings.flag("ExtraDimensionsUnpart:ffbar2gammagamma")) {
    sigmaPtr = make_shared<Sigma2ffbar2LEDgammagamma>( false );
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (settings.flag("ExtraDimensionsUnpart:gg2gammagamma")) {
    sigmaPtr = make_shared<Sigma2gg2LEDgammagamma>( false );
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (settings.flag("ExtraDimensionsUnpart:ffbar2llbar")) {
    sigmaPtr = make_shared<Sigma2ffbar2LEDllbar>( false );
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (settings.flag("ExtraDimensionsUnpart:gg2llbar")) {
    sigmaPtr = make_shared<Sigma2gg2LEDllbar>( false );
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }

  // Set up requested objects for Dark Matter processes.
  if (settings.flag("DM:ffbar2Zp2XX")) {
    sigmaPtr = make_shared<Sigma1ffbar2Zp2XX>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (settings.flag("DM:ffbar2Zp2XXj")) {
    sigmaPtr = make_shared<Sigma2qqbar2Zpg2XXj>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (settings.flag("DM:ffbar2ZpH")) {
    sigmaPtr = make_shared<Sigma2ffbar2ZpH>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (settings.flag("DM:gg2S2XX")) {
    sigmaPtr = make_shared<Sigma1gg2S2XX>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (settings.flag("DM:gg2S2XXj")) {
    sigmaPtr = make_shared<Sigma2gg2Sg2XXj>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }
  if (settings.flag("DM:qqbar2DY")) {
    sigmaPtr = make_shared<Sigma2qqbar2DY>();
    containerPtrs.push_back( new ProcessContainer(sigmaPtr) );
  }

  for ( ProcessContainer * cont : containerPtrs )
    cont->initInfoPtr(*infoPtr);

  // Done.
  return true;

}

//--------------------------------------------------------------------------

// Routine to initialize list of second hard processes.

bool SetupContainers::init2(vector<ProcessContainer*>& container2Ptrs,
  Info* infoPtr) {

  Settings& settings = *infoPtr->settingsPtr;

  // Reset process list, if filled in previous subrun.
  if (container2Ptrs.size() > 0) {
    for (int i = 0; i < int(container2Ptrs.size()); ++i)
      delete container2Ptrs[i];
    container2Ptrs.clear();
  }
  SigmaProcessPtr sigmaPtr;

  // Two hard QCD jets.
  if (settings.flag("SecondHard:TwoJets")) {
    sigmaPtr = make_shared<Sigma2gg2gg>();
    container2Ptrs.push_back( new ProcessContainer(sigmaPtr) );
    sigmaPtr = make_shared<Sigma2gg2qqbar>();
    container2Ptrs.push_back( new ProcessContainer(sigmaPtr) );
    sigmaPtr = make_shared<Sigma2qg2qg>();
    container2Ptrs.push_back( new ProcessContainer(sigmaPtr) );
    sigmaPtr = make_shared<Sigma2qq2qq>();
    container2Ptrs.push_back( new ProcessContainer(sigmaPtr) );
    sigmaPtr = make_shared<Sigma2qqbar2gg>();
    container2Ptrs.push_back( new ProcessContainer(sigmaPtr) );
    sigmaPtr = make_shared<Sigma2qqbar2qqbarNew>();
    container2Ptrs.push_back( new ProcessContainer(sigmaPtr) );
    sigmaPtr = make_shared<Sigma2gg2QQbar>(4, 121);
    container2Ptrs.push_back( new ProcessContainer(sigmaPtr) );
    sigmaPtr = make_shared<Sigma2qqbar2QQbar>(4, 122);
    container2Ptrs.push_back( new ProcessContainer(sigmaPtr) );
    sigmaPtr = make_shared<Sigma2gg2QQbar>(5, 123);
    container2Ptrs.push_back( new ProcessContainer(sigmaPtr) );
    sigmaPtr = make_shared<Sigma2qqbar2QQbar>(5, 124);
    container2Ptrs.push_back( new ProcessContainer(sigmaPtr) );
  }

  // A prompt photon and a hard jet.
  if (settings.flag("SecondHard:PhotonAndJet")) {
    sigmaPtr = make_shared<Sigma2qg2qgamma>();
    container2Ptrs.push_back( new ProcessContainer(sigmaPtr) );
    sigmaPtr = make_shared<Sigma2qqbar2ggamma>();
    container2Ptrs.push_back( new ProcessContainer(sigmaPtr) );
    sigmaPtr = make_shared<Sigma2gg2ggamma>();
    container2Ptrs.push_back( new ProcessContainer(sigmaPtr) );
  }

  // Two prompt photons.
  if (settings.flag("SecondHard:TwoPhotons")) {
    sigmaPtr = make_shared<Sigma2ffbar2gammagamma>();
    container2Ptrs.push_back( new ProcessContainer(sigmaPtr) );
    sigmaPtr = make_shared<Sigma2gg2gammagamma>();
    container2Ptrs.push_back( new ProcessContainer(sigmaPtr) );
  }

  // Charmonium.
  if (settings.flag("SecondHard:Charmonium")) {
    vector<SigmaProcessPtr> charmoniumSigmaPtrs;
    charmonium.setupSigma2gg(charmoniumSigmaPtrs, true);
    charmonium.setupSigma2qg(charmoniumSigmaPtrs, true);
    charmonium.setupSigma2qq(charmoniumSigmaPtrs, true);
    for (unsigned int i = 0; i < charmoniumSigmaPtrs.size(); ++i)
      container2Ptrs.push_back( new ProcessContainer(charmoniumSigmaPtrs[i]));
  }

  // Bottomonium.
  if (settings.flag("SecondHard:Bottomonium")) {
    vector<SigmaProcessPtr> bottomoniumSigmaPtrs;
    bottomonium.setupSigma2gg(bottomoniumSigmaPtrs, true);
    bottomonium.setupSigma2qg(bottomoniumSigmaPtrs, true);
    bottomonium.setupSigma2qq(bottomoniumSigmaPtrs, true);
    for (unsigned int i = 0; i < bottomoniumSigmaPtrs.size(); ++i)
      container2Ptrs.push_back( new ProcessContainer(bottomoniumSigmaPtrs[i]));
  }

  // A single gamma*/Z0.
  if (settings.flag("SecondHard:SingleGmZ")) {
    sigmaPtr = make_shared<Sigma1ffbar2gmZ>();
    container2Ptrs.push_back( new ProcessContainer(sigmaPtr) );
  }

  // A single W+-.
  if (settings.flag("SecondHard:SingleW")) {
    sigmaPtr = make_shared<Sigma1ffbar2W>();
    container2Ptrs.push_back( new ProcessContainer(sigmaPtr) );
  }

  // A gamma*/Z0 and a hard jet.
  if (settings.flag("SecondHard:GmZAndJet")) {
    sigmaPtr = make_shared<Sigma2qqbar2gmZg>();
    container2Ptrs.push_back( new ProcessContainer(sigmaPtr) );
    sigmaPtr = make_shared<Sigma2qg2gmZq>();
    container2Ptrs.push_back( new ProcessContainer(sigmaPtr) );
  }

  // A W+- and a hard jet.
  if (settings.flag("SecondHard:WAndJet")) {
    sigmaPtr = make_shared<Sigma2qqbar2Wg>();
    container2Ptrs.push_back( new ProcessContainer(sigmaPtr) );
    sigmaPtr = make_shared<Sigma2qg2Wq>();
    container2Ptrs.push_back( new ProcessContainer(sigmaPtr) );
  }

  // Top pair production.
  if (settings.flag("SecondHard:TopPair")) {
    sigmaPtr = make_shared<Sigma2gg2QQbar>(6, 601);
    container2Ptrs.push_back( new ProcessContainer(sigmaPtr) );
    sigmaPtr = make_shared<Sigma2qqbar2QQbar>(6, 602);
    container2Ptrs.push_back( new ProcessContainer(sigmaPtr) );
    sigmaPtr = make_shared<Sigma2ffbar2FFbarsgmZ>(6, 604);
    container2Ptrs.push_back( new ProcessContainer(sigmaPtr) );
  }

  // Single top production.
  if (settings.flag("SecondHard:SingleTop")) {
    sigmaPtr = make_shared<Sigma2qq2QqtW>(6, 603);
    container2Ptrs.push_back( new ProcessContainer(sigmaPtr) );
    sigmaPtr = make_shared<Sigma2ffbar2FfbarsW>(6, 0, 605);
    container2Ptrs.push_back( new ProcessContainer(sigmaPtr) );
  }

  // Two b jets - already part of TwoJets sample above.
  if (settings.flag("SecondHard:TwoBJets")) {
    sigmaPtr = make_shared<Sigma2gg2QQbar>(5, 123);
    container2Ptrs.push_back( new ProcessContainer(sigmaPtr) );
    sigmaPtr = make_shared<Sigma2qqbar2QQbar>(5, 124);
    container2Ptrs.push_back( new ProcessContainer(sigmaPtr) );
  }

  for ( ProcessContainer * cont : container2Ptrs )
    cont->initInfoPtr(*infoPtr);

  // Done.
  return true;

}

//--------------------------------------------------------------------------

// Set up arrays of allowed outgoing SUSY particles.

void SetupContainers::setupIdVecs( Settings& settings) {

  // First array either none, one or many particles.
  idVecA.resize(0);
  if (settings.mode("SUSY:idA") != 0) {
    idVecA.push_back( abs(settings.mode("SUSY:idA")) );
  } else {
    vector<int> idTmpA = settings.mvec("SUSY:idVecA");
    for (int i = 0; i < int(idTmpA.size()); ++i)
      if (idTmpA[i] != 0) idVecA.push_back( abs(idTmpA[i]) );
  }
  nVecA = idVecA.size();

  // Second array either none, one or many particles.
  idVecB.resize(0);
  if (settings.mode("SUSY:idB") != 0) {
    idVecB.push_back( abs(settings.mode("SUSY:idB")) );
  } else {
    vector<int> idTmpB = settings.mvec("SUSY:idVecB");
    for (int i = 0; i < int(idTmpB.size()); ++i)
    if (idTmpB[i] != 0) idVecB.push_back( abs(idTmpB[i]) );
  }
  nVecB = idVecB.size();

}

//--------------------------------------------------------------------------

// Check final state for allowed outgoing SUSY particles.
// Normally check two codes, but allow for only one.

bool SetupContainers::allowIdVals( int idCheck1, int idCheck2) {

  // If empty arrays or id's no need for checks. Else need absolute values.
  if (nVecA == 0 && nVecB == 0) return true;
  if (idCheck1 == 0 && idCheck2 == 0) return true;
  int idChk1 = abs(idCheck1);
  int idChk2 = abs(idCheck2);

  // If only one outgoing particle then check idVecA and idVecB.
  if (idChk1 == 0) swap(idChk1, idChk2);
  if (idChk2 == 0) {
    for (int i = 0; i < nVecA; ++i) if (idChk1 == idVecA[i]) return true;
    for (int i = 0; i < nVecB; ++i) if (idChk1 == idVecB[i]) return true;
    return false;
  }

  // If empty array idVecB then compare with idVecA.
  if (nVecB == 0) {
    for (int i = 0; i < nVecA; ++i)
      if (idChk1 == idVecA[i] || idChk2 == idVecA[i]) return true;
    return false;
  }

  // If empty array idVecA then compare with idVecB.
  if (nVecA == 0) {
    for (int i = 0; i < nVecB; ++i)
      if (idChk1 == idVecB[i] || idChk2 == idVecB[i]) return true;
    return false;
  }

  // Else check that pair matches allowed combinations.
  for (int i = 0; i < nVecA; ++i)
  for (int j = 0; j < nVecB; ++j)
    if ( (idChk1 == idVecA[i] && idChk2 == idVecB[j])
      || (idChk2 == idVecA[i] && idChk1 == idVecB[j]) ) return true;
  return false;

}

//==========================================================================

} // end namespace Pythia8
