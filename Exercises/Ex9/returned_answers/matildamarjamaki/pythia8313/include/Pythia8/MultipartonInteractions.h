// MultipartonInteractions.h is a part of the PYTHIA event generator.
// Copyright (C) 2025 Torbjorn Sjostrand.
// PYTHIA is licenced under the GNU GPL v2 or later, see COPYING for details.
// Please respect the MCnet Guidelines, see GUIDELINES for details.

// This file contains the main classes for multiparton interactions physics.
// SigmaMultiparton stores allowed processes by in-flavour combination.
// MultipartonInteractions: generates multiparton parton-parton interactions.

#ifndef Pythia8_MultipartonInteractions_H
#define Pythia8_MultipartonInteractions_H

#include "Pythia8/Basics.h"
#include "Pythia8/BeamParticle.h"
#include "Pythia8/Event.h"
#include "Pythia8/Info.h"
#include "Pythia8/PartonSystems.h"
#include "Pythia8/PartonVertex.h"
#include "Pythia8/PhysicsBase.h"
#include "Pythia8/PythiaStdlib.h"
#include "Pythia8/Settings.h"
#include "Pythia8/SigmaTotal.h"
#include "Pythia8/SigmaProcess.h"
#include "Pythia8/StandardModel.h"
#include "Pythia8/UserHooks.h"

namespace Pythia8 {

//==========================================================================

// SigmaMultiparton is a helper class to MultipartonInteractions.
// It packs pointers to the allowed processes for different
// flavour combinations and levels of ambition.

class SigmaMultiparton {

public:

  // Constructor.
  SigmaMultiparton() : nChan(), needMasses(), useNarrowBW3(), useNarrowBW4(),
    m3Fix(), m4Fix(), sHatMin(), sigmaT(), sigmaU(), sigmaTval(), sigmaUval(),
    sigmaTsum(), sigmaUsum(), pickOther(), pickedU(), particleDataPtr(),
    rndmPtr() {}

  // Initialize list of processes.
  bool init(int inState, int processLevel, Info* infoPtr,
    BeamParticle* beamAPtr, BeamParticle* beamBPtr);

  // Switch to new beam particle identities.
  void updateBeamIDs() {
    for (int i = 0; i < nChan; ++i) sigmaT[i]->updateBeamIDs();
    for (int i = 0; i < nChan; ++i) sigmaU[i]->updateBeamIDs(); }

  // Calculate cross section summed over possibilities.
  double sigma( int id1, int id2, double x1, double x2, double sHat,
    double tHat, double uHat, double alpS, double alpEM,
    bool restore = false, bool pickOtherIn = false);

  // Return whether the other, rare processes were selected.
  bool pickedOther() {return pickOther;}

  // Return one subprocess, picked according to relative cross sections.
  SigmaProcessPtr sigmaSel();
  bool swapTU() {return pickedU;}

  // Return code or name of a specified process, for statistics table.
  int    nProc() const {return nChan;}
  int    codeProc(int iProc) const {return sigmaT[iProc]->code();}
  string nameProc(int iProc) const {return sigmaT[iProc]->name();}

private:

  // Constants: could only be changed in the code itself.
  static const double MASSMARGIN, OTHERFRAC;

  // Number of processes. Some use massive matrix elements.
  int            nChan;
  vector<bool>   needMasses, useNarrowBW3, useNarrowBW4;
  vector<double> m3Fix, m4Fix, sHatMin;

  // Vector of process list, one for t-channel and one for u-channel.
  vector<SigmaProcessPtr> sigmaT, sigmaU;

  // Values of cross sections in process list above.
  vector<double> sigmaTval, sigmaUval;
  double         sigmaTsum, sigmaUsum;
  bool           pickOther, pickedU;

  // Pointers to the particle data and random number generator.
  ParticleData*  particleDataPtr;
  Rndm*          rndmPtr;

};

//==========================================================================

// The MultipartonInteractions class contains the main methods for the
// generation of multiparton parton-parton interactions in hadronic collisions.

class MultipartonInteractions : public PhysicsBase {

public:

  // Constructor.
  MultipartonInteractions() : allowRescatter(), allowDoubleRes(), canVetoMPI(),
    doPartonVertex(), doVarEcm(), setAntiSame(), setAntiSameNow(),
    pTmaxMatch(), alphaSorder(), alphaEMorder(), alphaSnfmax(), bProfile(),
    processLevel(), bSelScale(), rescatterMode(), nQuarkIn(), nSample(),
    enhanceScreening(), pT0paramMode(), alphaSvalue(), Kfactor(), pT0Ref(),
    ecmRef(), ecmPow(), pTmin(), coreRadius(), coreFraction(), expPow(),
    ySepResc(), deltaYResc(), sigmaPomP(), mPomP(), pPomP(), sigmaPomPom(),
    mMaxPertDiff(), mMinPertDiff(), a1(),
    a0now(), a02now(), bstepNow(), a2max(), b2now(), enhanceBmax(),
    enhanceBnow(), id1Save(), id2Save(), pT2Save(), x1Save(), x2Save(),
    sHatSave(), tHatSave(), uHatSave(), alpSsave(), alpEMsave(), pT2FacSave(),
    pT2RenSave(), xPDF1nowSave(), xPDF2nowSave(), scaleLimitPTsave(),
    dSigmaDtSelSave(), vsc1(), vsc2(), hasPomeronBeams(), hasLowPow(),
    globalRecoilFSR(), iDiffSys(), nMaxGlobalRecoilFSR(), bSelHard(), eCM(),
    sCM(), pT0(), pT20(), pT2min(), pTmax(), pT2max(), pT20R(), pT20minR(),
    pT20maxR(), pT20min0maxR(), pT2maxmin(), sigmaND(), pT4dSigmaMax(),
    pT4dProbMax(), dSigmaApprox(), sigmaInt(), sudExpPT(), zeroIntCorr(),
    normOverlap(), nAvg(), kNow(), normPi(), bAvg(), bDiv(), probLowB(),
    radius2B(), radius2C(), fracA(), fracB(), fracC(), fracAhigh(),
    fracBhigh(), fracChigh(), fracABChigh(), expRev(), cDiv(), cMax(),
    enhanceBavg(), bIsSet(false), bSetInFirst(), isAtLowB(), pickOtherSel(),
    id1(), id2(), i1Sel(), i2Sel(), id1Sel(), id2Sel(), iPDFA(), nPDFA(1),
    bNow(), enhanceB(), pT2(), pT2shift(), pT2Ren(), pT2Fac(), x1(),
    x2(), xT(), xT2(), tau(), y(), sHat(), tHat(), uHat(), alpS(), alpEM(),
    xPDF1now(), xPDF2now(), dSigmaSum(), x1Sel(), x2Sel(), sHatSel(),
    tHatSel(), uHatSel(), iPDFAsave(), nStep(), iStepFrom(), iStepTo(),
    eCMsave(), eStepMin(), eStepMax(), eStepSize(), eStepMix(), eStepFrom(),
    eStepTo(), beamOffset(), mGmGmMin(), mGmGmMax(), hasGamma(),
    isGammaGamma(), isGammaHadron(), isHadronGamma(), partonVertexPtr(),
    sigma2Sel(), dSigmaDtSel() {}

  // Initialize the generation process for given beams.
  bool init( bool doMPIinit, int iDiffSysIn,
    BeamParticle* beamAPtrIn, BeamParticle* beamBPtrIn,
    PartonVertexPtr partonVertexPtrIn, bool hasGammaIn = false);

  // Special setup to allow switching between beam PDFs for MPI handling.
  void initSwitchID( const vector<int>& idAListIn) {
    idAList = idAListIn; nPDFA = idAList.size();
    mpis  = vector<MPIInterpolationInfo>(nPDFA);}

    // Switch to new beam particle identities, and possibly PDFs.
  void setBeamID(int iPDFAin) { iPDFA = iPDFAin;
    sigma2gg.updateBeamIDs(); sigma2qg.updateBeamIDs();
    sigma2qqbarSame.updateBeamIDs(); sigma2qq.updateBeamIDs();
    setAntiSameNow = setAntiSame && particleDataPtr->hasAnti(infoPtr->idA())
      && particleDataPtr->hasAnti(infoPtr->idB());}

  // Reset impact parameter choice and update the CM energy.
  void reset();

  // Select first = hardest pT in nondiffractive process.
  void pTfirst();

  // Set up kinematics for first = hardest pT in nondiffractive process.
  void setupFirstSys( Event& process);

  // Find whether to limit maximum scale of emissions.
  // Provide sum pT / 2 as potential limit where relevant.
  bool limitPTmax( Event& event);
  double scaleLimitPT() const {return scaleLimitPTsave;}

  // Prepare system for evolution.
  void prepare(Event& event, double pTscale = 1000., bool rehashB = false) {
    if (!bSetInFirst) overlapNext(event, pTscale, rehashB); }

  // Select next pT in downwards evolution.
  double pTnext( double pTbegAll, double pTendAll, Event& event);

  // Set up kinematics of acceptable interaction.
  bool scatter( Event& event);

  // Set "empty" values to avoid query of undefined quantities.
  void setEmpty() {pT2Ren = alpS = alpEM = x1 = x2 = pT2Fac
    = xPDF1now = xPDF2now = 0.; bIsSet = false;}

  // Get some information on current interaction.
  double Q2Ren()      const {return pT2Ren;}
  double alphaSH()    const {return alpS;}
  double alphaEMH()   const {return alpEM;}
  double x1H()        const {return x1;}
  double x2H()        const {return x2;}
  double Q2Fac()      const {return pT2Fac;}
  double pdf1()       const {return (id1 == 21 ? 4./9. : 1.) * xPDF1now;}
  double pdf2()       const {return (id2 == 21 ? 4./9. : 1.) * xPDF2now;}
  double bMPI()       const {return (bIsSet) ? bNow : 0.;}
  double enhanceMPI() const {return (bIsSet) ? enhanceB / zeroIntCorr : 1.;}
  double enhanceMPIavg() const {return enhanceBavg;}

  // For x-dependent matter profile, return incoming valence/sea
  // decision from trial interactions.
  int    getVSC1()   const {return vsc1;}
  int    getVSC2()   const {return vsc2;}

  // Set the offset wrt. to normal beam particle positions for hard diffraction
  // and for photon beam from lepton.
  int  getBeamOffset()       const {return beamOffset;}
  void setBeamOffset(int offsetIn) {beamOffset = offsetIn;}

  // Update and print statistics on number of processes.
  // Note: currently only valid for nondiffractive systems, not diffraction??
  void accumulate() { int iBeg = (infoPtr->isNonDiffractive()) ? 0 : 1;
    for (int i = iBeg; i < infoPtr->nMPI(); ++i)
    ++nGen[ infoPtr->codeMPI(i) ];}
  void statistics(bool resetStat = false);
  void resetStatistics() { for ( map<int, int>::iterator iter = nGen.begin();
    iter != nGen.end(); ++iter) iter->second = 0; }

private:

  // Constants: could only be changed in the code itself.
  static const bool   SHIFTFACSCALE, PREPICKRESCATTER;
  static const double SIGMAFUDGE, RPT20, PT0STEP, SIGMASTEP, PT0MIN,
                      EXPPOWMIN, PROBATLOWB, BSTEP, BMAX, EXPMAX,
                      KCONVERGE, CONVERT2MB, ROOTMIN, ECMDEV, WTACCWARN,
                      SIGMAMBLIMIT;

  // Initialization data, read from Settings.
  bool   allowRescatter, allowDoubleRes, canVetoMPI, doPartonVertex, doVarEcm,
         setAntiSame, setAntiSameNow, allowIDAswitch;
  int    pTmaxMatch, alphaSorder, alphaEMorder, alphaSnfmax, bProfile,
         processLevel, bSelScale, rescatterMode, nQuarkIn, nSample,
         enhanceScreening, pT0paramMode, reuseInit;
  double alphaSvalue, Kfactor, pT0Ref, ecmRef, ecmPow, pTmin, coreRadius,
         coreFraction, expPow, ySepResc, deltaYResc, sigmaPomP, mPomP, pPomP,
         sigmaPomPom, mMaxPertDiff, mMinPertDiff;
  string initFile;

  // x-dependent matter profile:
  // Constants.
  static const int    XDEP_BBIN;
  static const double XDEP_A0, XDEP_A1, XDEP_BSTEP, XDEP_BSTEPINC,
                      XDEP_CUTOFF, XDEP_WARN, XDEP_SMB2FM;

  // Table of Int( dSigma/dX * O(b, X), dX ) in bins of b for fast
  // integration. Only needed during initialisation.
  vector <double> sigmaIntWgt, sigmaSumWgt;

  // a1:             value of a1 constant, taken from settings database.
  // a0now (a02now): tuned value of a0 (squared value).
  // bstepNow:       current size of bins in b.
  // a2max:          given an xMin, a maximal (squared) value of the
  //                 width, to be used in overestimate Omax(b).
  // enhanceBmax,    retain enhanceB as enhancement factor for the hardest
  // enhanceBnow:    interaction. Use enhanceBmax as overestimate for fastPT2,
  //                 and enhanceBnow to store the value for the current
  //                 interaction.
  double a1, a0now, a02now, bstepNow, a2max, b2now, enhanceBmax, enhanceBnow;

  // Storage for trial interactions.
  int    id1Save, id2Save;
  double pT2Save, x1Save, x2Save, sHatSave, tHatSave, uHatSave,
         alpSsave, alpEMsave, pT2FacSave, pT2RenSave, xPDF1nowSave,
         xPDF2nowSave, scaleLimitPTsave;
  SigmaProcessPtr dSigmaDtSelSave;

  // vsc1, vsc2:     for minimum-bias events with trial interaction, store
  //                 decision on whether hard interaction was valence or sea.
  int    vsc1, vsc2;

  // Other initialization data.
  // Number of points in the interpolation of Sudakov exponents.
  static const int NSUDPTS = 50;
  friend class MPIInterpolationInfo;
  bool   hasPomeronBeams, hasLowPow, globalRecoilFSR;
  int    iDiffSys, nMaxGlobalRecoilFSR, bSelHard;
  double eCM, sCM, pT0, pT20, pT2min, pTmax, pT2max, pT20R, pT20minR,
         pT20maxR, pT20min0maxR, pT2maxmin, sigmaND, pT4dSigmaMax,
         pT4dProbMax, dSigmaApprox, sigmaInt, sudExpPT[NSUDPTS + 1],
         zeroIntCorr, normOverlap, nAvg, kNow, normPi, bAvg, bDiv,
         probLowB, radius2B, radius2C, fracA, fracB, fracC, fracAhigh,
         fracBhigh, fracChigh, fracABChigh, expRev, cDiv, cMax,
         enhanceBavg;

  // Properties specific to current system.
  bool   bIsSet, bSetInFirst, isAtLowB, pickOtherSel;
  int    id1, id2, i1Sel, i2Sel, id1Sel, id2Sel, iPDFA, nPDFA;
  vector<int> idAList;
  double bNow, enhanceB, pT2, pT2shift, pT2Ren, pT2Fac, x1, x2, xT, xT2,
         tau, y, sHat, tHat, uHat, alpS, alpEM, xPDF1now, xPDF2now,
         dSigmaSum, x1Sel, x2Sel, sHatSel, tHatSel, uHatSel;

  // Local values for beam particle switch and mass interpolation.
  int    iPDFAsave, nStep, iStepFrom, iStepTo;
  double eCMsave, eStepMin, eStepMax, eStepSize, eStepMix, eStepFrom, eStepTo;

  // Stored values for mass interpolation. First index projectile particle.
  struct MPIInterpolationInfo {
    int nStepSave;
    double eStepMinSave, eStepMaxSave, eStepSizeSave;
    vector<double> pT0Save, pT4dSigmaMaxSave, pT4dProbMaxSave,
         sigmaIntSave, zeroIntCorrSave, normOverlapSave, kNowSave,
         bAvgSave, bDivSave, probLowBSave,
         fracAhighSave, fracBhighSave, fracChighSave,
         fracABChighSave, cDivSave, cMaxSave;
    vector<array<double, MultipartonInteractions::NSUDPTS + 1> >  sudExpPTSave;

    void init(int nStepIn);
  };

  vector<MPIInterpolationInfo> mpis;

  // Beam offset wrt. normal situation and other photon-related parameters.
  int    beamOffset;
  double mGmGmMin, mGmGmMax;
  bool   hasGamma, isGammaGamma, isGammaHadron, isHadronGamma;

  // Pointer to assign space-time vertices during parton evolution.
  PartonVertexPtr  partonVertexPtr;

  // Collections of parton-level 2 -> 2 cross sections. Selected one.
  SigmaMultiparton  sigma2gg, sigma2qg, sigma2qqbarSame, sigma2qq;
  SigmaMultiparton* sigma2Sel;
  SigmaProcessPtr   dSigmaDtSel;

  // Statistics on generated 2 -> 2 processes.
  map<int, int>  nGen;

  // alphaStrong and alphaEM calculations.
  AlphaStrong    alphaS;
  AlphaEM        alphaEM;

  // Scattered partons.
  vector<int>    scatteredA, scatteredB;

  // Determine constant in d(Prob)/d(pT2) < const / (pT2 + r * pT20)^2.
  void upperEnvelope();

  // Integrate the parton-parton interaction cross section.
  void jetCrossSection();

  // Read or write initialization data from/to settings/file, to save
  // startup time.
  bool saveMPIdata();
  bool loadMPIdata();

  // Evaluate "Sudakov form factor" for not having a harder interaction.
  double sudakov(double pT2sud, double enhance = 1.);

  // Do a quick evolution towards the next smaller pT.
  double fastPT2( double pT2beg);

  // Calculate the actual cross section, either for the first interaction
  // (including at initialization) or for any subsequent in the sequence.
  double sigmaPT2scatter(bool isFirst = false, bool doSymmetrize = false);

  // Find the partons that may rescatter.
  void findScatteredPartons( Event& event);

  // Calculate the actual cross section for a rescattering.
  double sigmaPT2rescatter( Event& event);

  // Calculate factor relating matter overlap and interaction rate.
  void overlapInit();

  // Pick impact parameter and interaction rate enhancement,
  // either before the first interaction (for nondiffractive) or after it.
  void overlapFirst();
  void overlapNext(Event& event, double pTscale, bool rehashB);

};

//==========================================================================

} // end namespace Pythia8

#endif // Pythia8_MultipartonInteractions_H
