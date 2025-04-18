// PhysicsBase.cc is a part of the PYTHIA event generator.
// Copyright (C) 2025 Torbjorn Sjostrand.
// PYTHIA is licenced under the GNU GPL v2 or later, see COPYING for details.
// Please respect the MCnet Guidelines, see GUIDELINES for details.

// Function definitions (not found in the header) for the PhysicsBase class.

#include "Pythia8/BeamSetup.h"
#include "Pythia8/PhysicsBase.h"

namespace Pythia8 {

//==========================================================================

// The PhysicsBase class.

//--------------------------------------------------------------------------

// Make available an assortment of pointers stored in the info object.

void PhysicsBase::initInfoPtr(Info& infoIn) {

  // Store the info object.
  infoPtr = &infoIn;

  // Other objects extracted from Info.
  settingsPtr      = infoPtr->settingsPtr;
  particleDataPtr  = infoPtr->particleDataPtr;
  loggerPtr        = infoPtr->loggerPtr;
  hadronWidthsPtr  = infoPtr->hadronWidthsPtr;
  rndmPtr          = infoPtr->rndmPtr;
  beamSetupPtr     = infoPtr->beamSetupPtr;
  coupSMPtr        = infoPtr->coupSMPtr;
  coupSUSYPtr      = infoPtr->coupSUSYPtr;
  partonSystemsPtr = infoPtr->partonSystemsPtr;
  sigmaTotPtr      = infoPtr->sigmaTotPtr;
  sigmaCmbPtr      = infoPtr->sigmaCmbPtr;
  userHooksPtr     = infoPtr->userHooksPtr;

  beamAPtr         = &beamSetupPtr->beamA;
  beamBPtr         = &beamSetupPtr->beamB;
  beamPomAPtr      = &beamSetupPtr->beamPomA;
  beamPomBPtr      = &beamSetupPtr->beamPomB;
  beamGamAPtr      = &beamSetupPtr->beamGamA;
  beamGamBPtr      = &beamSetupPtr->beamGamB;
  beamVMDAPtr      = &beamSetupPtr->beamVMDA;
  beamVMDBPtr      = &beamSetupPtr->beamVMDB;

  // If the class has sub objects, register them now.
  onInitInfoPtr();
}

//--------------------------------------------------------------------------

// Register a sub object that should have its information in sync with this.
void PhysicsBase::registerSubObject(PhysicsBase & pb) {
  pb.initInfoPtr(*infoPtr);
  subObjects.insert(&pb);
}

//--------------------------------------------------------------------------

// Calls onBeginEvent, then propagates the call to all sub objects
void PhysicsBase::beginEvent() {
  onBeginEvent();
  for (PhysicsBase* subObjectPtr : subObjects)
    subObjectPtr->beginEvent();
}

//--------------------------------------------------------------------------

// Calls onEndEvent, then propagates the call to all sub objects
void PhysicsBase::endEvent(Status status) {
  onEndEvent(status);
  for (PhysicsBase* subObjectPtr : subObjects)
    subObjectPtr->endEvent(status);
}

//--------------------------------------------------------------------------

// Calls onStat, then propagates the call to all sub objects
void PhysicsBase::stat() {
  onStat();
  for (PhysicsBase* subObjectPtr : subObjects)
    subObjectPtr->stat();
}

//==========================================================================

} // end namespace Pythia8
