#pragma once

#include <bufClass.h>
#include <oledClass.h>              // bufClass.h included

// HTU21 htu not needed to be exported!

class SENSORS
{
   private:
    int readcount;
    
public:
    float tempF;                    // instantenous measurements
    float humid;

    float tempF1;                   // averaged over N1 measurements
    float humid1;
    
    void init();                    // reset everything
    void read();   
    void resizeQueue( int N );
    bool statsReady();              // true every N1 measurements   
    void printStats( BUF *bp=NULL );     // show stats at any point
    void plotStats( BUF *bp=NULL);
};
extern SENSORS sensors;
extern OLED oled;
