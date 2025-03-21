// HadronLevel.h is a part of the PYTHIA event generator.
// Copyright (C) 2025 Torbjorn Sjostrand.
// PYTHIA is licenced under the GNU GPL v2 or later, see COPYING for details.
// Please respect the MCnet Guidelines, see GUIDELINES for details.

// This file contains the main class for hadron-level generation.
// HadronLevel: handles administration of fragmentation and decay.

#ifndef Pythia8_HadronLevel_H
#define Pythia8_HadronLevel_H

#include "Pythia8/Basics.h"
#include "Pythia8/BoseEinstein.h"
#include "Pythia8/ColourTracing.h"
#include "Pythia8/DeuteronProduction.h"
#include "Pythia8/Event.h"
#include "Pythia8/FragmentationFlavZpT.h"
#include "Pythia8/FragmentationSystems.h"
#include "Pythia8/HadronWidths.h"
#include "Pythia8/HiddenValleyFragmentation.h"
#include "Pythia8/Info.h"
#include "Pythia8/JunctionSplitting.h"
#include "Pythia8/LowEnergyProcess.h"
#include "Pythia8/SigmaLowEnergy.h"
#include "Pythia8/NucleonExcitations.h"
#include "Pythia8/ParticleData.h"
#include "Pythia8/ParticleDecays.h"
#include "Pythia8/PartonVertex.h"
#include "Pythia8/PhysicsBase.h"
#include "Pythia8/PythiaStdlib.h"
#include "Pythia8/RHadrons.h"
#include "Pythia8/Settings.h"
#include "Pythia8/StringFragmentation.h"
#include "Pythia8/TimeShower.h"
#include "Pythia8/UserHooks.h"

namespace Pythia8 {

//==========================================================================

// The HadronLevel class contains the top-level routines to generate
// the transition from the partonic to the hadronic stage of an event.

class HadronLevel : public PhysicsBase {

public:

  // Constructor.
  HadronLevel() = default;

  // Initialize HadronLevel classes as required.
  bool init( TimeShowerPtr timesDecPtrIn, RHadronsPtr rHadronsPtrIn,
    LundFragmentationPtr fragPtrIn, vector<FragmentationModelPtr>* fragPtrsIn,
    DecayHandlerPtr decayHandlePtr, vector<int> handledParticles,
    StringIntPtr stringInteractionsPtrIn, PartonVertexPtr partonVertexPtrIn,
    SigmaLowEnergy& sigmaLowEnergyIn,
    NucleonExcitations& nucleonExcitationsIn);

  // Get pointer to StringFlav instance (needed by BeamParticle).
  StringFlav* getStringFlavPtr() {return &flavSel;}

  // Generate the next event.
  bool next(Event& event);

  // Try to decay the specified particle. Returns false if decay failed.
  bool decay( int iDec, Event& event) { return
    (event[iDec].isFinal() && event[iDec].canDecay() && event[iDec].mayDecay())
    ? decays.decay( iDec, event) : true;}

  // Special routine to allow more decays if on/off switches changed.
  bool moreDecays(Event& event);

  // Perform rescattering. Return true if new strings must be hadronized.
  bool rescatter(Event& event);

  // Prepare and pick process for a low-energy hadron-hadron scattering.
  bool initLowEnergyProcesses();
  int pickLowEnergyProcess(int idA, int idB, double eCM, double mA, double mB);

  // Special routine to do a low-energy hadron-hadron scattering.
  bool doLowEnergyProcess(int i1, int i2, int procTypeIn, Event& event) {
    if (!lowEnergyProcess.collide( i1, i2, procTypeIn, event)) {
      loggerPtr->ERROR_MSG("low energy collision failed");
      return false;
    }
    return true;
  }

  // Tell if we did an early user-defined veto of the event.
  bool hasVetoedHadronize() const {return doHadronizeVeto; }

protected:

  virtual void onInitInfoPtr() override{
    registerSubObject(flavSel);
    registerSubObject(pTSel);
    registerSubObject(zSel);
    registerSubObject(decays);
    registerSubObject(lowEnergyProcess);
    registerSubObject(boseEinstein);
    registerSubObject(junctionSplitting);
    registerSubObject(deuteronProd);
  }

private:

  // Constants: could only be changed in the code itself.
  static const double MTINY;

  // Initialization data, read from Settings.
  bool doHadronize{}, doDecay{}, doPartonVertex{}, doBoseEinstein{},
    doDeuteronProd{}, allowRH{}, closePacking{}, doNonPertAll{},
    doQED;
  double pNormJunction{}, widthSepBE{}, widthSepRescatter{};
  vector<int> nonPertProc{};

  // Configuration of colour-singlet systems.
  ColConfig      colConfig;

  // Colour and mass information.
  vector<int>    iParton{}, iJunLegA{}, iJunLegB{}, iJunLegC{},
                 iAntiLegA{}, iAntiLegB{}, iAntiLegC{}, iGluLeg{};
  vector<double> m2Pair{};

  // The generator class for normal decays.
  ParticleDecays decays;

  // Pointer to TimeShower for interleaved QED in hadron decays.
  TimeShowerPtr timesDecPtr;

  // The generator class for Bose-Einstein effects.
  BoseEinstein boseEinstein;

  // The generator class for deuteron production.
  DeuteronProduction deuteronProd;

  // Classes for flavour, pT and z generation.
  StringFlav flavSel;
  StringPT   pTSel;
  StringZ    zSel;

  // Class for colour tracing.
  ColourTracing colTrace;

  // Junction splitting class.
  JunctionSplitting junctionSplitting;

  // The RHadrons class is used to fragment off and decay R-hadrons.
  RHadronsPtr rHadronsPtr{};

  // The LundFragmentation class is used for fragmentation and low
  // energy processes.
  LundFragmentationPtr fragPtr{};

  // The fragmentation model pointer vector allows multiple
  // fragmentation models to be used sequentially.
  vector<FragmentationModelPtr>* fragPtrs{};

  // Special case: colour-octet onium decays, to be done initially.
  bool decayOctetOnia(Event& event);

  // Trace colour flow in the event to form colour singlet subsystems.
  // Option to keep junctions, needed for rope hadronization.
  bool findSinglets(Event& event, bool keepJunctions = false);

  // Class to displace hadron vertices from parton impact-parameter picture.
  PartonVertexPtr partonVertexPtr;

  // Hadronic rescattering.
  class PriorityNode;
  bool doRescatter{}, scatterManyTimes{}, scatterQuickCheck{},
    scatterNeighbours{}, delayRegeneration{};
  double b2Max, tauRegeneration{};
  void queueDecResc(Event& event, int iStart,
    priority_queue<HadronLevel::PriorityNode>& queue);
  int boostDir;
  double boost;
  bool doBoost;
  bool useVelocityFrame;

  // User veto performed right after string hadronization.
  bool doHadronizeVeto;

  // The generator class for low-energy hadron-hadron collisions.
  LowEnergyProcess lowEnergyProcess;
  int    impactModel{};
  double impactOpacity{};

  // Cross sections for low-energy processes.
  SigmaLowEnergy* sigmaLowEnergyPtr = {};

  // Nucleon excitations data.
  NucleonExcitations* nucleonExcitationsPtr = {};

  // Class for event geometry for Rope Hadronization. Production vertices.
  StringRepPtr stringRepulsionPtr;
  FragModPtr fragmentationModifierPtr;

  // Extract rapidity pairs.
  vector< vector< pair<double,double> > > rapidityPairs(Event& event);

  // Calculate the rapidity for string ends, protected against too large y.
  double yMax(Particle pIn, double mTiny) {
    double temp = log( ( pIn.e() + abs(pIn.pz()) ) / max( mTiny, pIn.mT()) );
    return (pIn.pz() > 0) ? temp : -temp; }
  double yMax(Vec4 pIn, double mTiny) {
    double mTemp = pIn.m2Calc() + pIn.pT2();
    mTemp = (mTemp >= 0.) ? sqrt(mTemp) : -sqrt(-mTemp);
    double temp = log( ( pIn.e() + abs(pIn.pz()) ) / max( mTiny, mTemp) );
    return (pIn.pz() > 0) ? temp : -temp; }

  // Fragmentation weights container.
  WeightsFragmentation* wgtsPtr{};

};

//==========================================================================

} // end namespace Pythia8

#endif // Pythia8_HadronLevel_H
