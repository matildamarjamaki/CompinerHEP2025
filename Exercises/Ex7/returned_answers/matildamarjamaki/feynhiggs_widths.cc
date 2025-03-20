
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include "CFeynHiggs.h"
#include "CSLHA.h"
#include <fstream>
#include <vector>
#include <complex>

static void setFlags() {
    enum {
        mssmpart = 4,
        higgsmix = 2,
        p2approx = 4,
        looplevel = 2,
        loglevel = 3,
        runningMT = 1,
        botResum = 1,
        tlCplxApprox = 3
    };
    int error;
    FHSetFlags(&error, mssmpart, higgsmix, p2approx, looplevel, loglevel,
                runningMT, botResum, tlCplxApprox);
    if (error) exit(error);
}

static void setPara(double mass) {
    cRealType invAlfa0 = -1;
    cRealType invAlfaMZ = -1;
    cRealType AlfasMZ = -1;
    cRealType GF = -1;
  
    cRealType ME = -1;
    cRealType MU = -1;
    cRealType MD = -1;
    cRealType MM = -1;
    cRealType MC = -1;
    cRealType MS = -1;
    cRealType ML = -1;
    cRealType MB = -1;
  
    cRealType MW = -1;
    cRealType MZ = -1;
    cRealType GammaW = -1;
    cRealType GammaZ = -1;
  
    cRealType CKMlambda = -1;
    cRealType CKMA = -1;
    cRealType CKMrhobar = -1;
    cRealType CKMetabar = -1;
  
    cRealType MT = 173.2;
    cRealType TB = 5;
    cRealType MA0 = mass;
    cRealType MHp = -1;
  
    cRealType MSusy = 1000;
    cRealType M3SL = MSusy, M2SL = M3SL, M1SL = M2SL;
    cRealType M3SE = MSusy, M2SE = M3SE, M1SE = M2SE;
    cRealType M3SQ = MSusy, M2SQ = M3SQ, M1SQ = M2SQ;
    cRealType M3SU = MSusy, M2SU = M3SU, M1SU = M2SU;
    cRealType M3SD = MSusy, M2SD = M3SD, M1SD = M2SD;
  
    cComplexType Af = 2000;
    cComplexType At = Af, Ac = At, Au = Ac;
    cComplexType Ab = Af, As = Ab, Ad = As;
    cComplexType Atau = Af, Amu = Atau, Ae = Amu;
  
    cComplexType MUE = 200;
    cComplexType M_1 = 0;
    cComplexType M_2 = 500;
    cComplexType M_3 = 800;
  
    cRealType Qtau = 0;
    cRealType Qt = 0;
    cRealType Qb = 0;
  
    cRealType scalefactor = 1;
  
    int error;
  
    FHSetSMPara(&error,
      invAlfa0, invAlfaMZ, AlfasMZ, GF,
      ME, MU, MD, MM, MC, MS, ML, MB,
      MW, MZ, GammaW, GammaZ,
      CKMlambda, CKMA, CKMrhobar, CKMetabar);
    if( error ) exit(error);
  
    FHSetPara(&error, scalefactor,
      MT, TB, MA0, MHp,
      M3SL, M3SE, M3SQ, M3SU, M3SD,
      M2SL, M2SE, M2SQ, M2SU, M2SD,
      M1SL, M1SE, M1SQ, M1SU, M1SD,
      MUE,
      Atau, At, Ab,
      Amu, Ac, As,
      Ae, Au, Ad,
      M_1, M_2, M_3,
      Qtau, Qt, Qb);
    if( error ) exit(error);
}

static void setSLHA(const char *filename) {
    int error;
    COMPLEX slhadata[nslhadata];
    SLHARead(&error, slhadata, filename, 1);
    if (error) exit(error);
    FHSetSLHA(&error, slhadata);
    if (error) exit(error);
}

static void getPara() {
    int error;
    if( error ) exit(error);
}

static void higgsCorr() {
  int error;

  RealType MHiggs[4];
  ComplexType SAeff;
  ComplexType UHiggs[3][3], ZHiggs[3][3];
      
      FHHiggsCorr(&error, MHiggs, &SAeff, UHiggs, ZHiggs);
      if (error) exit(error);

      double mass = MHiggs[2];

      
     RealType gammas[ngammas], gammasms[ngammasms];
     ComplexType couplings[ncouplings], couplingsms[ncouplingsms];
     int fast;

      FHCouplings(&error, couplings, couplingsms, gammas, gammasms, fast);
      if (error) exit(error);

     std::cout << mass << "," << GammaSMTot(3) << std::endl;
}


int main() {

  setFlags();
  std::vector<double> masses = {110.0, 112.2, 114.4, 116.7, 118.9, 121.1, 123.3, 125.6, 127.8, 130.0};
  
  for (double mass : masses) {

    setPara(mass);
    getPara();
    higgsCorr();
  }
    return 0;
}
