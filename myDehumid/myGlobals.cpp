#include "myGlobals.h"  // this includes CPU and NMP

CPU cpu;
EEP eep;
NMP nmp( &eep );        // EEPROM named params
NMP stp;                // volatile named params
MGN mgn;
Globals myp;

//BUF clirsp(1024);       // buffer to be used for all CLI response needs
//BUF webput(1024);       // buffer to be used for web posts
//BUF webret(1024);       // buffer to be used for web returns
