// SLHAinterface.cc is a part of the PYTHIA event generator.
// Copyright (C) 2025 Torbjorn Sjostrand.
// Main authors of this file: N. Desai, P. Skands
// PYTHIA is licenced under the GNU GPL v2 or later, see COPYING for details.
// Please respect the MCnet Guidelines, see GUIDELINES for details.

#include "Pythia8/SLHAinterface.h"

namespace Pythia8 {

//==========================================================================

// The SLHAinterface class.

//--------------------------------------------------------------------------

// Initialize and switch to SUSY couplings if reading SLHA spectrum.

void SLHAinterface::init( bool& useSLHAcouplings,
  stringstream& particleDataBuffer) {

  // By default no SLHA couplings.
  useSLHAcouplings = false;

  // Check if SUSY couplings need to be read in
  if( !initSLHA())
    loggerPtr->ERROR_MSG("Could not read SLHA file");

  // Reset any particle-related user settings.
  string line;
  while (getline(particleDataBuffer, line)
    && settingsPtr->flag("SLHA:allowUserOverride")) {
    bool pass = particleDataPtr->readString(line, true);
    if (!pass) loggerPtr->WARNING_MSG("Unable to process line " + line);
    else loggerPtr->WARNING_MSG("Overwriting SLHA by " + line);
  }

  // SLHA sets isSUSY flag to tell us if there was an SLHA SUSY spectrum
  if (coupSUSYPtr->isSUSY) {
    // Initialize the derived SUSY couplings class.
    coupSUSYPtr->initSUSY(&slha, infoPtr);
    useSLHAcouplings = true;
  }
  // Make sure coupSUSY has a pointer to slha.
  else coupSUSYPtr->slhaPtr = &slha;

  // Make sure SLHA blocks are consistent with (updated) PYTHIA values.
  pythia2slha();

}

//--------------------------------------------------------------------------

// Initialize SUSY Les Houches Accord data.

bool SLHAinterface::initSLHA() {

  // Initial and settings values.
  int    ifailLHE          = 1;
  int    ifailSpc          = 1;
  int    readFrom          = settingsPtr->mode("SLHA:readFrom");
  string lhefFile          = settingsPtr->word("Beams:LHEF");
  string lhefHeader        = settingsPtr->word("Beams:LHEFheader");
  string slhaFile          = settingsPtr->word("SLHA:file");
  int    verboseSLHA       = settingsPtr->mode("SLHA:verbose");
  bool   slhaUseDec        = settingsPtr->flag("SLHA:useDecayTable");
  bool   allowOnlyOffShell = settingsPtr->flag("SLHA:allowOnlyOffShell");
  bool   noSLHAFile        = ( slhaFile == "none" || slhaFile == "void"
    || slhaFile == ""     || slhaFile == " " );

  // Set internal data members
  meMode      = settingsPtr->mode("SLHA:meMode");

  // No SUSY by default
  coupSUSYPtr->isSUSY = false;

  // Option with no SLHA read-in at all.
  if (readFrom == 0) return true;

  // First check LHEF header (if reading from LHEF)
  if (readFrom == 1) {
    // Check if there is a <slha> tag in the LHEF header
    // Note: if the <slha> tag is NOT inside the <header>, it will be ignored.
    string slhaInHeader( infoPtr->header("slha") );
    if (slhaInHeader == "" && noSLHAFile) return true;
    // If there is an <slha> tag, read file.
    if (lhefHeader != "void")
      ifailLHE = slha.readFile(lhefHeader, verboseSLHA, slhaUseDec );
    else if (lhefFile != "void")
      ifailLHE = slha.readFile(lhefFile, verboseSLHA, slhaUseDec );
    else if (noSLHAFile) {
      istringstream slhaInHeaderStream(slhaInHeader);
      ifailLHE = slha.readFile(slhaInHeaderStream, verboseSLHA, slhaUseDec );
    }
  }

  // If LHEF read successful, everything needed should already be ready
  if (ifailLHE == 0) {
    ifailSpc = 0;
    coupSUSYPtr->isSUSY = true;
    // If no LHEF file or no SLHA info in header, read from SLHA:file
  } else {
    lhefFile = "void";
    if ( noSLHAFile ) return true;
    ifailSpc = slha.readFile(slhaFile,verboseSLHA, slhaUseDec);
  }

  // In case of problems, print error and fail init.
  if (ifailSpc != 0) {
    loggerPtr->ERROR_MSG("problem reading SLHA file", slhaFile);
    return false;
  } else {
    coupSUSYPtr->isSUSY = true;
  }

  // Check spectrum for consistency. Switch off SUSY if necessary.
  ifailSpc = slha.checkSpectrum();

  // ifail >= 1 : no MODSEL found -> don't switch on SUSY
  if (ifailSpc == 1) {
    // no SUSY, but MASS ok
    coupSUSYPtr->isSUSY = false;
    loggerPtr->INFO_MSG("No MODSEL found, keeping internal SUSY switched off");
  } else if (ifailSpc >= 2) {
    // no SUSY, but problems
    loggerPtr->WARNING_MSG("No MASS or MODSEL blocks found");
    coupSUSYPtr->isSUSY = false;
  }
  // ifail = 0 : MODSEL found, spectrum OK
  else if (ifailSpc == 0) {
    // Print spectrum. Done.
    slha.listSpectrum(0);
  }
  else if (ifailSpc < 0) {
    loggerPtr->WARNING_MSG("Problem with SLHA spectrum",
      "\n Only using masses and switching off SUSY");
    settingsPtr->flag("SUSY:all", false);
    coupSUSYPtr->isSUSY = false;
    slha.listSpectrum(ifailSpc);
  }

  if (coupSUSYPtr->isSUSY) {

    // SLHA1 : SLHA2 compatibility
    // Check whether scalar particle masses are ordered
    bool isOrderedQ = true;
    bool isOrderedL = true;
    int idSdown[6]={1000001,1000003,1000005,2000001,2000003,2000005};
    int idSup[6]={1000002,1000004,1000006,2000002,2000004,2000006};
    int idSlep[6]={1000011,1000013,1000015,2000011,2000013,2000015};
    for (int j=0;j<=4;j++) {
      if (slha.mass(idSlep[j+1]) < slha.mass(idSlep[j]))
        isOrderedL  = false;
      if (slha.mass(idSup[j+1]) < slha.mass(idSup[j]))
        isOrderedQ  = false;
      if (slha.mass(idSdown[j+1]) < slha.mass(idSdown[j]))
        isOrderedQ  = false;
    }

    // If ordered, change sparticle labels to mass-ordered enumeration
    for (int i=1;i<=6;i++) {
      ostringstream indx;
      indx << i;
      string uName = "~u_"+indx.str();
      string dName = "~d_"+indx.str();
      string lName = "~e_"+indx.str();
      if (isOrderedQ) {
        particleDataPtr->names(idSup[i-1],uName,uName+"bar");
        particleDataPtr->names(idSdown[i-1],dName,dName+"bar");
      }
      if (isOrderedL) particleDataPtr->names(idSlep[i-1],lName+"-",lName+"+");
    }

    // NMSSM spectrum (modify existing Higgs names and add particles)
    if ( (ifailSpc == 1 || ifailSpc == 0) &&  slha.modsel(3) >= 1 ) {
      // Modify Higgs names
      particleDataPtr->name(25,"H_1");
      particleDataPtr->name(35,"H_2");
      particleDataPtr->name(45,"H_3");
      particleDataPtr->name(36,"A_1");
      particleDataPtr->name(46,"A_2");
      particleDataPtr->name(1000045,"~chi_50");
    }

    // SLHA2 spectrum with flavour mixing (modify squark and/or slepton names)
    if ( (ifailSpc == 1 || ifailSpc == 0) &&  slha.modsel(6) >= 1 ) {
      // Squark flavour violation
      if ( (slha.modsel(6) == 1 || slha.modsel(6) >= 3)
           && slha.usqmix.exists() && slha.dsqmix.exists() ) {
        // Modify squark names
        particleDataPtr->names(1000001,"~d_1","~d_1bar");
        particleDataPtr->names(1000002,"~u_1","~u_1bar");
        particleDataPtr->names(1000003,"~d_2","~d_2bar");
        particleDataPtr->names(1000004,"~u_2","~u_2bar");
        particleDataPtr->names(1000005,"~d_3","~d_3bar");
        particleDataPtr->names(1000006,"~u_3","~u_3bar");
        particleDataPtr->names(2000001,"~d_4","~d_4bar");
        particleDataPtr->names(2000002,"~u_4","~u_4bar");
        particleDataPtr->names(2000003,"~d_5","~d_5bar");
        particleDataPtr->names(2000004,"~u_5","~u_5bar");
        particleDataPtr->names(2000005,"~d_6","~d_6bar");
        particleDataPtr->names(2000006,"~u_6","~u_6bar");
      }
      // Slepton flavour violation
      if ( (slha.modsel(6) == 2 || slha.modsel(6) >= 3)
           && slha.selmix.exists()) {
        // Modify slepton names
        particleDataPtr->names(1000011,"~e_1-","~e_1+");
        particleDataPtr->names(1000013,"~e_2-","~e_2+");
        particleDataPtr->names(1000015,"~e_3-","~e_3+");
        particleDataPtr->names(2000011,"~e_4-","~e_4+");
        particleDataPtr->names(2000013,"~e_5-","~e_5+");
        particleDataPtr->names(2000015,"~e_6-","~e_6+");
      }
      // Neutrino flavour violation
      if ( (slha.modsel(6) == 2 || slha.modsel(6) >= 3)
           && slha.upmns.exists()) {
        // Modify neutrino names (note that SM processes may not use UPMNS)
        particleDataPtr->names(12,"nu_1","nu_1bar");
        particleDataPtr->names(14,"nu_2","nu_2bar");
        particleDataPtr->names(16,"nu_3","nu_3bar");
      }
      // Sneutrino flavour violation
      if ( (slha.modsel(6) == 2 || slha.modsel(6) >= 3)
           && slha.snumix.exists()) {
        // Modify sneutrino names
        particleDataPtr->names(1000012,"~nu_1","~nu_1bar");
        particleDataPtr->names(1000014,"~nu_2","~nu_2bar");
        particleDataPtr->names(1000016,"~nu_3","~nu_3bar");
      }
      // Optionally allow for separate scalar and pseudoscalar sneutrinos
      if ( slha.snsmix.exists() && slha.snamix.exists() ) {
        // Scalar sneutrinos
        particleDataPtr->names(1000012,"~nu_S1","~nu_S1bar");
        particleDataPtr->names(1000014,"~nu_S2","~nu_S2bar");
        particleDataPtr->names(1000016,"~nu_S3","~nu_S3bar");
        // Add the pseudoscalar sneutrinos
        particleDataPtr->addParticle(1000017, "~nu_A1", "~nu_A1bar",1, 0., 0);
        particleDataPtr->addParticle(1000018, "~nu_A2", "~nu_A2bar",1, 0., 0);
        particleDataPtr->addParticle(1000019, "~nu_A3", "~nu_A3bar",1, 0., 0);
      }
    }

    // SLHA2 spectrum with RPV
    if ( (ifailSpc == 1 || ifailSpc == 0) &&  slha.modsel(4) >= 1 ) {
      if ( slha.rvnmix.exists() ) {
        // Neutralinos -> neutrinos
        // Maintain R-conserving names since mass-ordering unlikely to change.
        particleDataPtr->names(12,"nu_1","nu_1bar");
        particleDataPtr->names(14,"nu_2","nu_2bar");
        particleDataPtr->names(16,"nu_3","nu_3bar");
        particleDataPtr->name(1000022,"~chi_10");
        particleDataPtr->name(1000023,"~chi_20");
        particleDataPtr->name(1000025,"~chi_30");
        particleDataPtr->name(1000035,"~chi_40");
      }
      if ( slha.rvumix.exists() && slha.rvvmix.exists() ) {
        // Charginos -> charged leptons (note sign convention)
        // Maintain R-conserving names since mass-ordering unlikely to change.
        particleDataPtr->names(11,"e-","e+");
        particleDataPtr->names(13,"mu-","mu+");
        particleDataPtr->names(15,"tau-","tau+");
        particleDataPtr->names(1000024,"~chi_1+","~chi_1-");
        particleDataPtr->names(1000037,"~chi_2+","~chi_2-");
      }
      if ( slha.rvhmix.exists() ) {
        // Sneutrinos -> higgses (general mass-ordered names)
        particleDataPtr->name(25,"H_10");
        particleDataPtr->name(35,"H_20");
        particleDataPtr->names(1000012,"H_30","H_30");
        particleDataPtr->names(1000014,"H_40","H_40");
        particleDataPtr->names(1000016,"H_50","H_50");
      }
      if ( slha.rvamix.exists() ) {
        // Sneutrinos -> higgses (general mass-ordered names)
        particleDataPtr->name(36,"A_10");
        particleDataPtr->names(1000017,"A_20","A_20");
        particleDataPtr->names(1000018,"A_30","A_30");
        particleDataPtr->names(1000019,"A_40","A_40");
      }
      if ( slha.rvlmix.exists() ) {
        // sleptons -> charged higgses (note sign convention)
        particleDataPtr->names(37,"H_1+","H_1-");
        particleDataPtr->names(1000011,"H_2-","H_2+");
        particleDataPtr->names(1000013,"H_3-","H_3+");
        particleDataPtr->names(1000015,"H_4-","H_4+");
        particleDataPtr->names(2000011,"H_5-","H_5+");
        particleDataPtr->names(2000013,"H_6-","H_6+");
        particleDataPtr->names(2000015,"H_7-","H_7+");
      }
    }

    // SLHA2 spectrum with CPV
    if ( (ifailSpc == 1 || ifailSpc == 0) &&  slha.modsel(5) >= 1 ) {
      // no scalar/pseudoscalar distinction
      particleDataPtr->name(25,"H_10");
      particleDataPtr->name(35,"H_20");
      particleDataPtr->name(36,"H_30");
    }
  }

  // Import qnumbers
  vector<int> isQnumbers;
  bool foundLowCode = false;
  if ( (ifailSpc == 1 || ifailSpc == 0) && slha.qnumbers.size() > 0) {
    for (int iQnum=0; iQnum < int(slha.qnumbers.size()); iQnum++) {
      // Always use positive id codes
      int id = abs(slha.qnumbers[iQnum](0));
      ostringstream idCode;
      idCode << id;
      if (particleDataPtr->isParticle(id)) {
        loggerPtr->WARNING_MSG("ignoring QNUMBERS", "for id = "
          + idCode.str() + " (already exists)", true);
      } else {
        // Note: qnumbers entries stored internally as doubles to allow for
        // millicharged particles. Round to nearest int when handing to PYTHIA.
        // (So charge ~ 0 treated as charge = 0; good enough for showers.)
        int qEM3    = lrint(slha.qnumbers[iQnum](1));
        int nSpins  = lrint(slha.qnumbers[iQnum](2));
        int colRep  = lrint(slha.qnumbers[iQnum](3));
        int hasAnti = lrint(slha.qnumbers[iQnum](4));
        // Translate colRep to PYTHIA colType
        int colType = 0;
        if (colRep == 3) colType = 1;
        else if (colRep == -3) colType = -1;
        else if (colRep == 8) colType = 2;
        else if (colRep == 6) colType = 3;
        else if (colRep == -6) colType = -3;
        // Default name: PDG code
        string name, antiName;
        ostringstream idStream;
        idStream<<id;
        name     = idStream.str();
        antiName = "-"+name;
        if (iQnum < int(slha.qnumbersName.size())) {
          name = slha.qnumbersName[iQnum];
          antiName = slha.qnumbersAntiName[iQnum];
          if (antiName == "") {
            if (name.find("+") != string::npos) {
              antiName = name;
              antiName.replace(antiName.find("+"),1,"-");
            } else if (name.find("-") != string::npos) {
              antiName = name;
              antiName.replace(antiName.find("-"),1,"+");
            } else {
              antiName = name+"bar";
            }
          }
        }
        if ( hasAnti == 0) {
          antiName = "";
          particleDataPtr->addParticle(id, name, nSpins, qEM3, colType);
        } else {
          particleDataPtr->addParticle(id, name, antiName, nSpins, qEM3,
            colType);
        }
        // Store list of particle codes added by QNUMBERS
        isQnumbers.push_back(id);
        if (id < 1000000) foundLowCode = true;
      }
    }
  }
  // Inform user that BSM particles should ideally be assigned id codes > 1M
  if (foundLowCode)
    loggerPtr->WARNING_MSG(
      "using QNUMBERS for id codes < 1000000 may clash with SM");

  // Import mass spectrum.
  double minMassSM = settingsPtr->parm("SLHA:minMassSM");
  map<int, bool> idModified;
  vector<pair<int, double> > idMass;
  if (ifailSpc == 1 || ifailSpc == 0) {

    // Start at beginning of mass array
    int    id = slha.mass.first();
    vector<int> ignoreMassKeepSM;
    vector<int> ignoreMassM0;
    vector<int> importMass;
    // Loop through to update particle data.
    for (int i = 1; i <= slha.mass.size() ; i++) {
      // Step to next mass entry
      if (i>1) id = slha.mass.next();
      double mass = abs(slha.mass(id));

      // Check if this ID was added by qnumbers
      bool isInternal = true;
      for (unsigned int iq = 0; iq<isQnumbers.size(); ++iq)
        if (id == isQnumbers[iq]) isInternal = false;

      // Ignore masses for known SM particles or particles with
      // default masses < minMassSM; overwrite masses for rest.
      // SM particles: (idRes < 25 || idRes > 80 && idRes < 1000000);
      // Top and Higgs (25) can be overwritten if mass > minMassSM
      // Extra higgses (26 - 40) & DM (51 - 60) not SM
      // minMassSM : mass above which SM masses may be overwritten
      bool isSM = id < 26 || ( id > 80 && id < 1000000);
      if (isSM && isInternal) {
        if (particleDataPtr->m0(id) < minMassSM) {
          ignoreMassM0.push_back(id);
        }
      } else {
        ParticleDataEntryPtr tmpPtr = particleDataPtr->findParticle(id);
        if( tmpPtr == nullptr ) {
          ostringstream idCode;
          idCode << id;
          loggerPtr->INFO_MSG("attempting to set properties",
            "for unknown id = {" + idCode.str() + "}", true);
          continue;
        }
        // Catch multiple re-inits of SLHA data
        if ( mass != particleDataPtr-> m0(id) && (
          ( particleDataPtr->hasChangedMMin(id) &&
          particleDataPtr->mMin(id) > mass ) ||
          ( particleDataPtr->hasChangedMMax(id) &&
          particleDataPtr->mMax(id) < mass ) ) )
          particleDataPtr->findParticle(id)->setHasChanged(false);
        particleDataPtr->m0(id,mass);
        idModified[id] = true;
        idMass.push_back(make_pair(id,mass));
        importMass.push_back(id);
        // If the mMin and mMax cutoffs on Breit-Wigner tails were not already
        // set by user, set default bounds to at most m0 +- m0/2.
        // Treat these values as new defaults: do not set hasChanged flags.
        // Note: tighter bounds may apply if a width is given later; see below.
        if (!particleDataPtr->hasChangedMMin(id))
          particleDataPtr->findParticle(id)->setMMinNoChange( mass/2. );
        if (!particleDataPtr->hasChangedMMax(id))
          particleDataPtr->findParticle(id)->setMMaxNoChange( 3.*mass/2. );
      }
    };
    // Give summary of any imported/ignored MASS entries, and state reason
    if (importMass.size() >= 1) {
      string idImport;
      for (unsigned int i=0; i<importMass.size(); ++i) {
        ostringstream idCode;
        idCode << importMass[i];
        if (i != 0) idImport +=",";
        idImport += idCode.str();
      }
      loggerPtr->INFO_MSG("importing MASS entries","for id = {"
        + idImport + "}", true);
    }
    if (ignoreMassM0.size() >= 1) {
      string idIgnore;
      for (unsigned int i=0; i<ignoreMassM0.size(); ++i) {
        ostringstream idCode;
        idCode << ignoreMassM0[i];
        if (i != 0) idIgnore +=",";
        idIgnore += idCode.str();
      }
      loggerPtr->WARNING_MSG("ignoring MASS entries", "for id = {"
        + idIgnore + "}" + " (m0 < SLHA:minMassSM)", true);
    }
  }

  // Update decay data.
  vector<int> ignoreDecayKeepSM;
  vector<int> ignoreDecayM0;
  vector<int> ignoreDecayBR;
  vector<int> importDecay;
  for (int iTable=0; iTable < int(slha.decays.size()); iTable++) {

    // Pointer to this SLHA table
    LHdecayTable* slhaTable=&(slha.decays[iTable]);

    // Extract ID and create pointer to corresponding particle data object
    int idRes     = slhaTable->getId();
    ostringstream idCode;
    idCode << idRes;
    ParticleDataEntryPtr particlePtr
      = particleDataPtr->particleDataEntryPtr(idRes);

    // Check if this ID was added by qnumbers
    bool isInternal = true;
    for (unsigned int iq = 0; iq<isQnumbers.size(); ++iq)
      if (idRes == isQnumbers[iq]) isInternal = false;


    // Ignore decay channels for known SM particles or particles with
    // default masses < minMassSM; overwrite masses for rest.
    // Let extra Higgses & Dark Matter sector be non-SM
    bool isSM = idRes < 26 || ( idRes > 80 && idRes < 1000000);
    if (isSM && isInternal) {
      if(particleDataPtr->m0(idRes) < minMassSM) {
        ignoreDecayM0.push_back(idRes);
        continue;
      }
    }

    // Extract and store total width (absolute value, neg -> switch off)
    double widRes         = abs(slhaTable->getWidth());
    double pythiaMinWidth = settingsPtr->parm("ResonanceWidths:minWidth");
    if (widRes > 0. && widRes < pythiaMinWidth) {
      loggerPtr->WARNING_MSG("forcing width = 0 ","for id = "
        + idCode.str() + " (width < ResonanceWidths:minWidth)" , true);
      widRes = 0.0;
    }
    particlePtr->setMWidth(widRes);
    // If the mMin and mMax cutoffs on Breit-Wigner tails were not already
    // set by user, set default values for them to 5*width though at most m0/2.
    // Treat these values as defaults, ie do not set hasChanged flags.
    // (After all channels have been read in, we also check that mMin is
    // high enough to allow at least one channel to be on shell; see below.)
    if (!particlePtr->hasChangedMMin()) {
      double m0   = particlePtr->m0();
      double mMin = m0 - min(5*widRes , m0/2.);
      particlePtr->setMMinNoChange(mMin);
    }
    if (!particlePtr->hasChangedMMax()) {
      double m0   = particlePtr->m0();
      double mMax = m0 + min(5*widRes , m0/2.);
      particlePtr->setMMaxNoChange(mMax);
    }

    // Set lifetime for displaced vertex calculations (convert GeV^-1 to mm).
    if (widRes > 0.) {
      double decayLength = HBARC * FM2MM / widRes;
      particlePtr->setTau0(decayLength);

      // Reset decay table of the particle. Allow decays, treat as resonance.
      if (slhaTable->size() > 0) {
        particlePtr->clearChannels();
        particleDataPtr->mayDecay(idRes,true);
        particleDataPtr->isResonance(idRes,true);
        // Let user know we are using these tables.
        importDecay.push_back(idRes);
      } else
        ignoreDecayBR.push_back(idRes);
    }

    // Reset to stable if width <= 0.0
    else {
      particlePtr->clearChannels();
      particleDataPtr->mayDecay(idRes,false);
      particleDataPtr->isResonance(idRes,false);
    }

    // Loop over SLHA channels, import into Pythia, treating channels
    // with negative branching fractions as having the equivalent positive
    // branching fraction, but being switched off for this run
    for (int iChannel=0 ; iChannel<slhaTable->size(); iChannel++) {
      LHdecayChannel slhaChannel = slhaTable->getChannel(iChannel);
      double brat      = slhaChannel.getBrat();
      vector<int> idDa = slhaChannel.getIdDa();
      if (idDa.size() >= 9) {
        loggerPtr->ERROR_MSG("max number of DECAY products is 8 ",
          "for id = "+idCode.str(), true);
      } else if (idDa.size() <= 1) {
        loggerPtr->ERROR_MSG("min number of DECAY products is 2 ",
          "for id = "+idCode.str(), true);
      }
      else {
        int onMode = 1;
        if (brat < 0.0) onMode = 0;
        int meModeNow = meMode;

        // Check phase space, including margin ~ sqrt(sum(widths^2))
        double massSum(0.);
        double widSqSum = pow2(widRes);
        int nDa = idDa.size();
        for (int jDa=0; jDa<nDa; ++jDa) {
          massSum  += particleDataPtr->m0( idDa[jDa] );
          widSqSum +=  pow2(particleDataPtr->mWidth( idDa[jDa] ));
        }
        double deltaM = particleDataPtr->m0(idRes) - massSum;
        // Negative mass difference: intrinsically off shell
        if (onMode == 1 && brat > 0.0 && deltaM < 0.) {
          // String containing decay name
          ostringstream errCode;
          errCode << idRes <<" ->";
          for (int jDa=0; jDa<nDa; ++jDa) errCode<<" "<<idDa[jDa];
          // Could mass fluctuations at all give the needed deltaM ?
          if (abs(deltaM) > 100. * sqrt(widSqSum)) {
            loggerPtr->WARNING_MSG("switched off DECAY mode",
              ": " + errCode.str()+" (too far off shell)",true);
            onMode = 0;
          }
          // If ~ OK within fluctuations
          else {
            // Ignore user-selected meMode
            if (meModeNow != 100) {
              loggerPtr->WARNING_MSG("adding off shell DECAY mode",
                ": "+errCode.str()+" (forced meMode = 100)",true);
              meModeNow = 100;
            } else {
              loggerPtr->WARNING_MSG("adding off shell DECAY mode",
                errCode.str(), true);
            }
          }
        }

        // Add channel
        int id0 = idDa[0];
        int id1 = idDa[1];
        int id2 = (idDa.size() >= 3) ? idDa[2] : 0;
        int id3 = (idDa.size() >= 4) ? idDa[3] : 0;
        int id4 = (idDa.size() >= 5) ? idDa[4] : 0;
        int id5 = (idDa.size() >= 6) ? idDa[5] : 0;
        int id6 = (idDa.size() >= 7) ? idDa[6] : 0;
        int id7 = (idDa.size() >= 8) ? idDa[7] : 0;
        particlePtr->addChannel(onMode,abs(brat),meModeNow,
                                id0,id1,id2,id3,id4,id5,id6,id7);

      }
    }

    // Add to list of particles that have been modified
    idModified[idRes]=true;
  }

  // Give summary of imported/ignored DECAY tables, and state reason
  if (importDecay.size() >= 1) {
    string idImport;
    for (unsigned int i=0; i<importDecay.size(); ++i) {
      ostringstream idCode;
      idCode << importDecay[i];
      if (i != 0) idImport +=",";
      idImport += idCode.str();
    }
    loggerPtr->INFO_MSG("importing DECAY tables","for id = {"
      + idImport + "}", true);
  }
  if (ignoreDecayM0.size() >= 1) {
    string idIgnore;
    for (unsigned int i=0; i<ignoreDecayM0.size(); ++i) {
      ostringstream idCode;
      idCode << ignoreDecayM0[i];
      if (i != 0) idIgnore +=",";
      idIgnore += idCode.str();
    }
    loggerPtr->WARNING_MSG("ignoring DECAY tables", "for id = {"
      + idIgnore + "}" + " (m0 < SLHA:minMassSM)", true);
  }
  if (ignoreDecayBR.size() >= 1) {
    string idIgnore;
    for (unsigned int i=0; i<ignoreDecayBR.size(); ++i) {
      ostringstream idCode;
      idCode << ignoreDecayBR[i];
      if (i != 0) idIgnore +=",";
      idIgnore += idCode.str();
    }
    loggerPtr->WARNING_MSG("ignoring empty DECAY tables", "for id = {"
      + idIgnore + "}" + " (total width provided but no Branching Ratios)",
      true);
  }

  // Sort the IDs by masses.
  sort(idMass.begin(), idMass.end(), [](
      const pair<int, double> &left, const pair<int, double> &right) {
      return left.second < right.second;});

  // Sanity check of all decay tables with modified MASS or DECAY info.
  for (auto it = idMass.begin(); it != idMass.end(); ++it) {
    int id = it->first;
    if (idModified[id] == false) continue;
    ParticleDataEntryPtr particlePtr(
      particleDataPtr->particleDataEntryPtr(id));
    double m0(particlePtr->m0()), wid(particlePtr->mWidth());
    // Always set massless particles stable.
    if (m0 <= 0.0 && (wid > 0.0 || particlePtr->mayDecay())) {
      loggerPtr->WARNING_MSG("massless particle forced stable",
        " id = " + to_string(id), true);
      particlePtr->clearChannels();
      particlePtr->setMWidth(0.0);
      particlePtr->setMayDecay(false);
      particleDataPtr->isResonance(id,false);
      continue;
    }
    // Declare zero-width particles to be stable (for now).
    if (wid == 0.0 && particlePtr->mayDecay()) {
      particlePtr->setMayDecay(false);
      continue;
    }
    // Check at least one on-shell channel is available.
    double mSumMin = 10. * m0;
    int nChannels = particlePtr->sizeChannels();
    if (nChannels < 1 ) continue;
    // Check that at least one decay channel is on.
    bool openModeCheck(false);

    int readChannels = 0;
    vector<DecayChannel> savedChannels;
    for (int iChannel=0; iChannel<nChannels; ++iChannel) {
      DecayChannel channel = particlePtr->channel(iChannel);
      if (channel.onMode() <= 0) {
        savedChannels.push_back(channel);
        readChannels++;
        continue;
      }
      // If at least one channel is open, then off-shell decays allowed.
      int nProd = channel.multiplicity();
      double mSum(0.), wSum(0.);
      for (int iDa = 0; iDa < nProd; ++iDa) {
        int idDa = channel.product(iDa);
        mSum += particleDataPtr->m0(idDa);
        wSum += particleDataPtr->mMin(idDa);
      }
      // Require that the fluctuation for this to occur is reasonable.
      if (mSum > m0 && wSum > m0 && channel.bRatio() > 0.0) {
        ostringstream errCode;
        errCode << id <<" ->";
        for (int jDa = 0; jDa < nProd; ++jDa)
          errCode << " " << channel.product(jDa);
        loggerPtr->WARNING_MSG("switched off DECAY mode",
          ": " + errCode.str()+" (too far off shell)",true);
        continue;
      }
      openModeCheck = true;
      mSumMin = min(mSumMin, mSum);
      savedChannels.push_back(channel);
      readChannels++;
    }
    // Set only the allowed channels if necessary.
    if (readChannels < nChannels) {
      particlePtr->clearChannels();
      for (auto chn : savedChannels )
        particlePtr->addChannel(chn.onMode(), chn.bRatio(), chn.meMode(),
          chn.product(0), chn.product(1), chn.product(2), chn.product(3),
          chn.product(4), chn.product(5), chn.product(6), chn.product(7));
    }
    // No channels are open.
    if (!openModeCheck) {
      loggerPtr->WARNING_MSG("particle forced stable",
        " id = " + to_string(id) + " (no decay channels on)", true);
    // Only off-shell channels.
    } else if (mSumMin > m0) {
      // Set particle stable if only off-shell channels and on-shell required.
      if (!allowOnlyOffShell) {
        loggerPtr->WARNING_MSG("particle forced stable",
          " id = " + to_string(id) +
          " (no on-shell decay channels and SLHA::allowOnly"
          "OffShell is false)", true);
        particlePtr->setMWidth(0.0);
        particlePtr->setMayDecay(false);
        continue;
      // Allow decay if only off-shell channels allowed.
      } else
        loggerPtr->WARNING_MSG("allowing particle with no on-shell decays",
          "id = " + to_string(id), true);
    } else {
      // mMin: lower cutoff on Breit-Wigner; see above.
      // Increase minimum if needed to ensure at least one channel on shell
      double mMin1 = particlePtr->mMin();
      double mMin0 = (mMin1 < m0 ) ? mMin1 : min(m0-0.5*wid,0.5*m0);
      double mMin = max(mSumMin, mMin0);
      particlePtr->setMMin(mMin);
    }
  }

  return true;

}

//--------------------------------------------------------------------------

// Initialize SLHA blocks SMINPUTS and MASS from PYTHIA SM parameter values.
// E.g., to make sure that there are no important unfilled entries.
// Also fill SLHA CKM blocks if not done already.

void SLHAinterface::pythia2slha() {

  // Initialize block SMINPUTS.
  string blockName = "sminputs";
  double mZ = particleDataPtr->m0(23);
  slha.set(blockName,1,1.0/coupSMPtr->alphaEM(pow2(mZ)));
  slha.set(blockName,2,coupSMPtr->GF());
  slha.set(blockName,3,coupSMPtr->alphaS(pow2(mZ)));
  slha.set(blockName,4,mZ);
  // b mass (should be running mass, here pole for time being)
  slha.set(blockName,5,particleDataPtr->m0(5));
  slha.set(blockName,6,particleDataPtr->m0(6));
  slha.set(blockName,7,particleDataPtr->m0(15));
  slha.set(blockName,8,particleDataPtr->m0(16));
  slha.set(blockName,11,particleDataPtr->m0(11));
  slha.set(blockName,12,particleDataPtr->m0(12));
  slha.set(blockName,13,particleDataPtr->m0(13));
  slha.set(blockName,14,particleDataPtr->m0(14));
  // Force 3 lightest quarks massless
  slha.set(blockName,21,double(0.0));
  slha.set(blockName,22,double(0.0));
  slha.set(blockName,23,double(0.0));
  // c mass (should be running mass, here pole for time being)
  slha.set(blockName,24,particleDataPtr->m0(4));

  // Initialize block MASS.
  blockName = "mass";
  int id = 1;
  int count = 0;
  while (particleDataPtr->nextId(id) > id) {
    slha.set(blockName,id,particleDataPtr->m0(id));
    id = particleDataPtr->nextId(id);
    ++count;
    if (count > 10000) {
      loggerPtr->ERROR_MSG("encountered infinite loop when saving mass block");
      break;
    }
  }

  // Initialize block VCKMIN
  blockName = "vckmin";
  double lambda = coupSMPtr->VCKMgen(1,2)
    / sqrt(coupSMPtr->V2CKMgen(1,1) + coupSMPtr->V2CKMgen(1,2));
  double A = abs(coupSMPtr->VCKMgen(2,3)/coupSMPtr->VCKMgen(1,2))/lambda;
  double rho = 0.5*(1. + coupSMPtr->V2CKMgen(1,3)-coupSMPtr->V2CKMgen(3,1)
    / pow2(A * pow3(lambda)));
  double eta = sqrt(coupSMPtr->V2CKMgen(1,3)/pow2(A * pow3(lambda))
    - pow2(rho));

  slha.set(blockName, 1, lambda);
  slha.set(blockName, 2, A);
  slha.set(blockName, 3, rho);
  slha.set(blockName, 4, eta);

  // Copy CKM to block "wolfenstein" (used by MG5 instead of VCKMIN).
  blockName = "wolfenstein";
  slha.set(blockName, 1, lambda);
  slha.set(blockName, 2, A);
  slha.set(blockName, 3, rho);
  slha.set(blockName, 4, eta);

}

//==========================================================================

} // end namespace Pythia8
