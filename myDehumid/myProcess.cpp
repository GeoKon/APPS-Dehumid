#include "htu21Class.h"
#include "myGlobals.h"
#include "myProcess.h"
#include "qstatClass.h"

OLED    oled;
HTU21   htu;
SENSORS sensors;
STATISTICS tempStat, humdStat;
QUEUE queF( 60 ), queH( 60 );   // queue of statistics

void SENSORS::init()
{
    htu.init();                 // initialize temp sensor
    readcount = 0;              // statistics counter
    tempStat.resetStats();
    humdStat.resetStats();
}
void SENSORS::resizeQueue( int N )
{
    queF.resize( N );          // depth of the queue
    queH.resize( N );
}
// .read() read is called every tic mark
void SENSORS::read()
{
    tempF = CTOF( htu.readTemperature() );
    humid = htu.readHumidity();
    oled.dsp( 0,"\t\b%.1fF", tempF );
    oled.dsp( 3,"\t\b%.0f%%",humid );
    oled.dsp( 6,"Sample:%d ST:%d", myp.st.postidx % 100, myp.st.cycle );

    tempStat.updateStats( tempF );      
    humdStat.updateStats( humid );

    readcount++;                // increments every time .read() is called
}        
bool SENSORS::statsReady()
{
    if( readcount>=myp.gp.Nave )   // when called more than N1
    {
        readcount = 0;
        STATS x;
        x = tempStat.getStats();
        //tempStat.printStats();
        tempF1 = x.average;
        
        humid1 = humdStat.getStats().average;
        queF.enqueue( tempF1 );             // place in the queue
        queH.enqueue( humid1 );
        
        tempStat.resetStats();
        humdStat.resetStats();
        return true;
    }
    return false;
}
// forms PART OF JSON. No {}. 
void SENSORS::printStats( BUF *bp )
{
    queF.begin();
    if( queF.end() )                    // do nothing if no data
        printb( bp, "'npast':0");
    else
    {
        printb( bp, "'npast':%d,'temps':[", queF.size() );
    
        for( int i=0; !queF.end(); i++ )
            printb( bp, "%.2f%s", queF.get(), queF.end() ? "]," : "," );

        queH.begin();
        printb( bp, "'humids':[");
        for( int i=0; !queH.end(); i++ )
            printb( bp, "%.2f%s", queH.get(), queH.end() ? "]" : "," );
    }
    if( bp )
        bp->quotes();
}
void SENSORS::plotStats( BUF *bp )
{
    queF.begin();
    queH.begin();

    PF("{XYPLOT:Temp|CLEAR}\r\n");
    PF("{XYPLOT:Temp|SET|TITLE=Temperature}\r\n");
    PF("{XYPLOT:Temp|SET|Y-LABEL=Degrees °F}\r\n");
    // PF("{XYPLOT:Temp|xrange|0|40}\r\n");
    // PF("{XYPLOT:Temp|yrange|50|80}\r\n");

    PF("{XYPLOT:Temp|DATA|Temperature");
    for( int i=0; !queF.end(); i++ )
    {
        PF("|%d|%0.1f", i, queF.get());
    }
    PF("}\r\n");

    PF("{XYPLOT:Humid|CLEAR}\r\n");
   // PF("{XYPLOT:Humid|xrange|0|40}\r\n");
   PF("{XYPLOT:Humid|SET|Y-LABEL=Percent (%%)}\r\n");
    PF("{XYPLOT:Humid|SET|TITLE=Humidity}\r\n");
   // PF("{XYPLOT:Humid|yrange|30|40}\r\n");        // do not force scale to allow user selection
     
    PF("{XYPLOT:Humid|DATA|Humidity");
    for( int i=0; !queH.end(); i++ )
    {
        PF("|%d|%0.1f", i,queH.get());
    }
    PF("}\r\n");
    
//    mgn.tplotClear();
//    mgn.tplotYRange( 50.0, 80.0 );
//    mgn.tplotY2Range( 30.0, 70.0 );

//    PFN("{TIMEPLOT:Temperature|yrange|60|80}");
//    PFN("{TIMEPLOT:Temperature|SET|Y-Label=Temperature}");
//    PFN("{TIMEPLOT:Temperature|SET|Y-Visible=1}");
//    
//    PFN("{TIMEPLOT:Humidity|y2range|40|60}");
//    PFN("{TIMEPLOT:Humidity|SET|Y2-Label=Humidity}");
//    PFN("{TIMEPLOT:Humidity|SET|Y2-Visible=1}");

//    for( int i=0; !queF.end(); i++ )
//    {
////        PFN("{TIMEPLOT:Temperature|DATA|Temperature|T|%0.1f}", queF.get());
////        PFN("{TIMEPLOT:Humidity|DATA|Humidity|T|%0.1f}", queH.get());
//        mgn.tplotData( "Temperature", queF.get() );  
//        mgn.tplotData( "Humidity", queH.get() );                
//    }    
}
