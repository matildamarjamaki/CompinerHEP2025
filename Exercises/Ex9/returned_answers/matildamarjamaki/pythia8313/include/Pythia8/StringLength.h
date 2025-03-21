// StringLength.h is a part of the PYTHIA event generator.
// Copyright (C) 2025 Torbjorn Sjostrand.
// PYTHIA is licenced under the GNU GPL v2 or later, see COPYING for details.
// Please respect the MCnet Guidelines, see GUIDELINES for details.

// This file contains the class StringLength.
// It is used to calculate the lambda measure of strings and junctions.

#ifndef Pythia8_StringLength_H
#define Pythia8_StringLength_H

#include "Pythia8/Event.h"
#include "Pythia8/Info.h"
#include "Pythia8/Settings.h"
#include "Pythia8/StringFragmentation.h"

namespace Pythia8 {

//==========================================================================

// StringLength class. It is used to calculate the lambda measure.

class StringLength {

public:

  // Constructor.
  StringLength() : loggerPtr(), m0(), sqrt2(), juncCorr(), lambdaForm(),
    infoPtr() {}

  // Initialize.
  void init(Info* infoPtrIn, Settings& settings);

  // Calculate string length of a single particle.
  // The first vector is the 4 vector of the particle.
  // The second vector represents (1,0,0,0) in dipole restframe.
  double getLength(const Vec4& p, const Vec4& v, bool isJunc = false) const;

  // Calculate string length for two indices in the event record.
  double getStringLength(Event& event, int i, int j);

  // Calculate string length for two particles given their four-momenta.
  double getStringLength(Vec4 p1, Vec4 p2) const;

  // Calculate the length of a single junction given the 3 entries in event.
  double getJuncLength(Event& event, int i, int j, int k);

  // Calculate the length of a single junction given the 3 four-momenta.
  double getJuncLength(const Vec4& p1, const Vec4& p2, const Vec4& p3) const;

  // Calculate the length of a double junction given the 4 entries in event.
  // The first two are expected to be quarks, the second two to be antiquarks.
  double getJuncLength(Event& event, int i, int j, int k, int l);

  // Calculate the length of a double junction given the 4 four-momenta.
  // The first two are expected to be quarks, the second two to be antiquarks.
  double getJuncLength(const Vec4& p1, const Vec4& p2, const Vec4& p3,
    const Vec4& p4) const;

private:

  static const double TINY, MINANGLE;

  // Pointer to logger.
  Logger* loggerPtr;

  double m0, sqrt2, juncCorr;
  int lambdaForm;

  // Pointer to various information on the generation.
  Info* infoPtr;

  // This is only to access the function call junctionRestFrame.
  StringFragmentation stringFragmentation;

};

//==========================================================================

} // end namespace Pythia8

#endif // Pythia8_StringLength_H
