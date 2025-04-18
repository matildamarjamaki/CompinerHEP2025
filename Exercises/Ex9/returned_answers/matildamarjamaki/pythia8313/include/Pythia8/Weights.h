// Weights.h is a part of the PYTHIA event generator.
// Copyright (C) 2025 Torbjorn Sjostrand.
// PYTHIA is licenced under the GNU GPL v2 or later, see COPYING for details.
// Please respect the MCnet Guidelines, see GUIDELINES for details.

// This file contains classes that keep track of event weights.

#ifndef Pythia8_Weights_H
#define Pythia8_Weights_H

#include "Pythia8/Basics.h"
#include "Pythia8/LHEF3.h"
#include "Pythia8/PythiaStdlib.h"
#include "Pythia8/SharedPointers.h"

namespace Pythia8 {

// Forward declare classes which need internal member access.
class History;
class Info;
class PartonLevel;
class Merging;
class WeightContainer;
class StringFlav;

//==========================================================================

// This is a base class to store weight information in a way that allows
// unified access in the structure that contains all event generation weights
// information (WeightContainer below). The main purpuse of this class is to
// supply convenience features to make defining new categories of weights easy.
// All weights should inherit from this base class. The specialized classes
// may then contain specialized functions, or only act as a glorified
// book-keeping structure.

class WeightsBase {

public:

  // Friends for internal protected members.
  friend class History;
  friend class PartonLevel;

  // Initialize the weights.
  virtual void init() {weightValues.resize(0); weightNames.resize(0);
    bookWeight("Baseline");}
  virtual void init(bool) {}

  // Reset all internal values.
  virtual void clear() {fill(weightValues.begin(), weightValues.end(), 1.);}

  // Store the current event information (replace whitespace with underscore
  // for HepMC).
  virtual void bookVectors(vector<double> weights, vector<string> names) {
    for (int i = 0; i < (int)weights.size(); ++i) {
      replace(names[i].begin(), names[i].end(), ' ', '_');
      bookWeight(names[i], weights[i]);
    }
  }

  // Function to return processed weights to weight container, e.g. if
  // weights should be combined before proceeding.
  virtual void collectWeightValues(vector<double>& outputWeights,
    double norm = 1.);

  // Similar function to return processed weight names.
  virtual void collectWeightNames(vector<string>& outputNames);

  // Get the stored information.
  // Direcly use storage members here in the base class,
  // and access those through non-virtual getters.
  // Note: NOT opting for a map data structure, since information will anyway
  // have to be serialized in output.
  // Change weight names for compatibility with Rivet by replacing colons
  // with full stops.
  string getWeightsName(int iPos) const  {
    string name = iPos >= 0
      && iPos < (int)weightNames.size() ? weightNames[iPos] : "";
    if (name.find(":") != string::npos)
      replace(name.begin(), name.end(), ':', '.');
    return name == "" ? to_string(iPos) : name;}
  virtual double getWeightsValue(int iPos) const { return weightValues[iPos]; }
  int getWeightsSize() const { return weightValues.size(); }

  // Function to create a new, synchronized, pair of weight name and value.
  void bookWeight(string name, double defaultValue = 1.)  {
    // Check if name already exists.
    if (findIndexOfName(name) != -1) setValueByName(name, defaultValue);
    else {
      weightNames.push_back(name);
      weightValues.push_back(defaultValue);
    }
  }

  // Functions to set values of weights.
  void setValueByIndex(int iPos, double val) {
    if (iPos < 0 || iPos >= (int)weightValues.size()) return;
    weightValues[iPos] = val;
  }
  void setValueByName(string name, double val) {
    setValueByIndex(findIndexOfName(name), val);}

  // Functions to reweight weight values, by index or by name.
  virtual void reweightValueByIndex(int iPos, double val) {
    if (iPos < 0 || iPos >= (int)weightValues.size()) return;
    weightValues[iPos] *= val;
  };
  virtual void reweightValueByName(string name, double val) {
    // Use existing functions: Find index of name, then set by index.
    int iPos = findIndexOfName(name);
    reweightValueByIndex(iPos, val);
  }

  // Function to find the index of a given entry in the weightNames vector,
  // e.g., to be able to synchronize with the weightValues vector.
  int findIndexOfName(string name) {
    vector<string>::iterator it
      = find(weightNames.begin(), weightNames.end(), name);
    unsigned long int index = distance(weightNames.begin(), it);
    if (index == weightNames.size()) return -1;
    return distance(weightNames.begin(), it);
  }

  // Set the pointers.
  void setPtrs(Info* infoPtrIn) { infoPtr = infoPtrIn; }

protected:

  // Parse a WVec of variations into a dictionary.
  void parse(string wvecKey, map<string, map<string, double> > &dct);

  // Weight values and names.
  vector<double> weightValues;
  vector<string> weightNames;

  // Pointers necessary for variation initialization
  Info* infoPtr{};

};

//==========================================================================

// Purely virtual base class for shower weights.

class WeightsShower : public WeightsBase {

public:

  // Initialize weights (more can be booked at any time)
  void init(bool) override {}

  // Weight "groups" (combinations of one or more unique weights).
  virtual void initWeightGroups(bool = false) {}
  virtual int nWeightGroups() const {return 0;}
  virtual string getGroupName( int /*iGroup*/ ) const {return "";}
  virtual double getGroupWeight( int /*iGroup*/ ) const {return 1.;}

};

//==========================================================================

// This shows a WeightsShower example implementation for SimpleShower.

class WeightsSimpleShower : public WeightsShower {

public:

  // Initialize weights (more can be booked at any time)
  void init( bool doMerging) override;

  // Functions to return processed weights to weight container, e.g. if
  // weights should be combined before proceeding.
  void collectWeightNames(vector<string>& outputNames) override;
  void collectWeightValues(vector<double>& outputWeights,
    double norm = 1.) override;

  // Initialize the weight groups for automated variations.
  void initWeightGroups(bool = false) override;

  // Return group name (want to integrate this in weightNameVector?)
  string getGroupName(int iGN) const override;

  // Return group weight (want to integrate this in weightValueVector?)
  double getGroupWeight(int iGW) const override;
  int    nWeightGroups() const override { return externalVariations.size(); }

  // Initialise list of atomic weight variations to be performed by shower.
  bool initUniqueShowerVars();

  // Memorize enhancement factor and pT of enhanced emission
  void setEnhancedTrial( double pTIn, double wtIn) { pTEnhanced = pTIn;
    wtEnhanced = wtIn; }
  double getEnhancedTrialPT() { return pTEnhanced;}
  double getEnhancedTrialWeight() { return wtEnhanced;}

  // Extract full list or subset that matches specific keys (e.g., FSR ones).
  vector<string> getUniqueShowerVars() { return uniqueShowerVars; }
  vector<string> getUniqueShowerVars(vector<string> keys);

  // Initialise map of enhancement factors for shower trial probabilities.
  bool initEnhanceFactors();
  unordered_map<string,double> getEnhanceFactors() { return enhanceFactors; }

  string getInitialName(int iG) const { return initialNameSave[iG]; }

  // Variations that must be known by TimeShower and Spaceshower.
  map<int,double> varPDFplus, varPDFminus, varPDFmember;

  // Vectors for storing shower variatons and enhancement factors.
  vector<string> uniqueShowerVars;
  unordered_map<string,double> enhanceFactors;

  // Vectors for weight group handling
  vector<string>          externalVariations;
  vector<vector<string> > externalVarNames;
  vector<string>          externalGroupNames;
  vector<string>          initialNameSave;
  vector<vector<int> >    externalMap;
  int                     externalVariationsSize{};

  // Vector for merging requested weight handling
  vector<double> getMuRWeightVector();
  vector<vector<string> > mergingVarNames;
  double pTEnhanced{}, wtEnhanced{};

};

//==========================================================================

// This class collects information on weights generated in the merging
// framework. The first weight is always required for CKKW-L, UMEPS and
// NLO merging. The additional weights are required for simultaneous
// renormalization scale variation in matrix element generation and parton
// shower.

class WeightsMerging : public WeightsBase {

public:

  // Friends for internal protected members.
  friend class Merging;
  friend class WeightContainer;

  // Initialize weights (more can be booked at any time)
  void init() override;

  // Reset all internal values;
  void clear() override;

  // Function to create a new, synchronized, pair of weight name and value.
  void bookWeight(string name, double value, double valueFirst);

  // Store the current event information.
  void bookVectors(vector<double> weights, vector<string> names) override;
  void bookVectors(vector<double> weights, vector<double> weightsFirst,
            vector<string> names);
  // Modified weight getter to include first order weight
  double getWeightsValue(int iPos) const override {
    return weightValues[iPos] - weightValuesFirst[iPos]; }
  // Also add getters for UNLOPS-P and -PC schemes
  double getWeightsValueP(int iPos) const {
    return weightValuesP[iPos] - weightValuesFirstP[iPos]; }
  double getWeightsValuePC(int iPos) const {
    return weightValuesPC[iPos] - weightValuesFirstPC[iPos]; }

  // Functions to set values of weights.
  void reweightValueByIndex(int iPos, double val) override;
  void reweightValueByName(string name, double val) override;

  // Functions to set values of first order weights.
  void setValueFirstByIndex(int iPos, double val);
  void setValueFirstByName(string name, double val);

  // Functions to set values as whole vector.
  void setValueVector(vector<double> ValueVector);
  void setValueFirstVector(vector<double> ValueFirstVector);

  // Function telling merging which muR variations to perform
  vector<double> getMuRVarFactors();

  // Set up mapping between LHEF variations.
  void setLHEFvariationMapping();

  // Function to collect weight names.
  void collectWeightNames(vector<string>& outputNames) override;

  // Function collecting weight values.
  void collectWeightValues(vector<double>& outputWeights,
     double norm = 1.) override;

protected:

  // Corresponding vector with respective LHEF weight indices.
  map<int,int> muRVarLHEFindex;

  // Data member for first order weight.
  vector<double> weightValuesFirst;

  // Data members for UNLOPS-P and -PC.
  vector<double> weightValuesP, weightValuesPC,
    weightValuesFirstP, weightValuesFirstPC;

  // Boolean to memorize if LHEF weight needs to be applied (only for NLO).
  bool isNLO;

};

//==========================================================================

// This is a short example class to collect information on Les Houches
// Event weights into a container class that can be part of Weight,
// which in turn is part of Info.

class WeightsLHEF : public WeightsBase {

public:

  // Friends for internal protected members.
  friend class WeightsMerging;

  // Central weight, needed for normalization, set from ProcessContainer.cc
  double centralWeight;

  // Reset all internal values;
  void clear() override;

  // Store the current event information.
  void bookVectors(vector<double> weights, vector<string> names) override;

  // Function to return processed weights to weight container, e.g. if
  // weights should be combined before proceeding.
  void collectWeightValues(vector<double>& outputWeights,
     double norm = 1.) override;
  void collectWeightNames(vector<string>& outputNames) override;

  // Convert weight names in MadGraph5 convention to the convention outlined
  // in https://arxiv.org/pdf/1405.1067.pdf, page 162ff.
  vector<string> convertNames(vector<string> names);

  void identifyVariationsFromLHAinit(map<string,LHAweight>* weights);

protected:

  // Renormalization variations.
  map<int,double> muRvars;

};

//==========================================================================

// This is class to collect information on fragmentation weighting,
// and is in turn part of Info.

class WeightsFragmentation : public WeightsBase {

public:

  // Friends for fragmentation reweighitng.
  friend class StringFlav;

  // Initialize the weights.
  void init() override;

  // Clear the weights.
  void clear() override {
    WeightsBase::clear(); fill(flavBreaks.begin(), flavBreaks.end(), 0);}

  int nWeightGroups() const {return externalGroupNames.size();}

  string getGroupName(int iGN) const {return iGN < 0 || iGN >= nWeightGroups()
      ? "Null" : externalGroupNames[iGN];}

  // Return group weight.
  double getGroupWeight(int iGW) const {
    if (iGW < 0 || iGW >= nWeightGroups()) return 1.0;
    double wgt(1.);
    for (const int &iWgt : externalMap[iGW]) wgt *= getWeightsValue(iWgt);
    return wgt;}

  // Functions to return processed weights to weight container, e.g. if
  // weights should be combined before proceeding.
  void collectWeightNames(vector<string>& outputNames) override;
  void collectWeightValues(vector<double>& outputWeights,
    double norm = 1.) override;

  // Calculate the derived flavor parameters.
  vector<double> flavParms(double xi, double rho, double x, double y);

  // Calculate a flavor weight.
  double flavWeight(const vector<double>& parms) {
    return flavWeight(parms, flavBreaks);}
  double flavWeight(const vector<double>& parms, const vector<int>& breaks);

  // Vectors for weight group handling.
  vector<map<vector<double>, int> > weightParms{};
  vector<string>       externalGroupNames{};
  vector<vector<int> > externalMap{};

  // Track the breaks needed for reweighting.
  vector<int> flavBreaks;

  // Factorization indices.
  enum FactIndex{Z, Flav, PT};

private:

  // Ordering of the fragmentation weight keys.
  const vector<vector< pair<string, string> > > keyOrder{
    {{"frag:alund", "StringZ:aLund"}, {"frag:blund", "StringZ:bLund"},
     {"frag:rfactc", "StringZ:rFactC"}, {"frag:rfactb", "StringZ:rFactB"}},
    {{"frag:xi", "StringFlav:ProbQQtoQ"}, {"frag:rho", "StringFlav:ProbStoUD"},
     {"frag:x", "StringFlav:ProbSQtoQQ"},
     {"frag:y", "StringFlav:ProbQQ1toQQ0"}},
    {{"frag:ptsigma", "StringPT:sigma"}}};

  // Flavor spin ratios to store in derived.
  const vector<int> flavIdxs{0, 1, 2, 3, 6};

  // Stored parameters for flavor reweighting.
  vector<double> flavBase;

  // Count the flavor breaks for variations.
  void flavCount(int idIn, bool early, bool noChoice);

};

//==========================================================================

// This is a container class to collect all event generation weight
// information into a wrapper which is in turn is part of Info. In this
// way, we could avoid cluttering Info.

class WeightContainer {

public:

  // Default constructor only ensures that members are initialized with
  // sensible default values.
  WeightContainer() : weightNominal(1.0), xsecIsInit(false) {}

  // The nominal Pythia weight, in pb for lha strategy 4 and -4
  double weightNominal;
  void setWeightNominal( double weightNow );
  double collectWeightNominal();

  // First example of a weight subcategory.
  WeightsLHEF          weightsLHEF{};

  // Shower weights. SimpleShower one hardcoded and made default since
  // Merging explicitly depends on it, but allow to point to a
  // different weightsShower-derived object, eg for Vincia (with own
  // merging).
  WeightsShower*       weightsShowerPtr{};
  WeightsSimpleShower  weightsSimpleShower{};

  // Merging weights.
  WeightsMerging       weightsMerging{};

  // Fragmentation weights.
  WeightsFragmentation weightsFragmentation{};

  // Userhooks weights.
  WeightsBase          weightsUserHooks{};

  // Functions to retrieve information stored in the subcategory members.
  int numberOfWeights();
  double weightValueByIndex(int key = 0);
  string weightNameByIndex(int key = 0);

  // Function to return the vector of weight values, combining all
  // weights from all subcategories. Currently, only the nominal
  // weight and LHEF weights are considered. Internal Pythia weights
  // should also be included eventually.
  vector<double> weightValueVector();

  // Function to return the vector of weight names, combining all names from
  // all subcategories, cf. weightValueVector function.
  vector<string> weightNameVector();

  // Reset all members to default stage.
  void clear();

  // Reset total cross section estimate.
  void clearTotal();

  // Init, for those classes that need it.
  void init(bool doMerging);

  // Function to set Pointers in weight classes.
  void initPtrs(Info* infoPtrIn);

  // Initialize the cross-section vector.
  void initXsecVec();

  // Get cross-section vectors.
  vector<double> getSampleXsec();
  vector<double> getTotalXsec();
  vector<double> getSampleXsecErr();
  vector<double> getTotalXsecErr();

  // Accumulate cross section for all weights.
  void accumulateXsec(double norm = 1.);

private:

  // Pointers necessary for variation initialization.
  Info* infoPtr{};

  // Suppress AUX_ weights.
  bool doSuppressAUXweights{};

  // Vectors for cross-section.
  vector<double> sigmaTotal, sigmaSample, errorTotal, errorSample;
  bool xsecIsInit;

};

//==========================================================================

} // end namespace Pythia8

#endif // Pythia8_Weights_H
