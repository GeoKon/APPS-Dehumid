/* 
 *  This example is a SIMPLE CLIENT to georgekontopidis.com/myp/process1.php
 *  It posts (with GET) dummy data
 *  Use URL: georgekontopidis.com/myp/extract.html to display postings
 *
 *  Developed by GK Nov 11th, 2020
 */

// --- needed for OTA ------
//    #include <WiFi.h>
//    #include <ESPmDNS.h>
//    #include <WiFiUdp.h>
//    #include <ArduinoOTA.h>
// -------------------------

//#include <WiFi.h>
#include <HTTPClient.h>

// ------- Library dependencies ----------------------------
#include <wifiSupport.h>

// ------- In this project folder --------------------------
#include "myGlobals.h"  // this includes CPU, NMP, BUF, etc
#include "myProcess.h"
#include "myCLI.h"
#include "myWeb.h"
//#include "webSupport.h"
// ------------- Allocation of objects ----------------------
    
//char *serverName = "http://georgekontopidis.com/myp/process1.php?x";
char *serverName = "http://georgekontopidis.com/myp/collect1.php";

void setup() 
{
    cpu.init( 115200 );
      
    pinMode( 16, OUTPUT ); 
    digitalWrite( 16, HIGH );               // enable EEPROM
    oled.init();
    oled.dsp( O_BRIGHT1 );
    oled.dsp( 0,"\t\bBEGIN" );

    // -------- 1. Handling the Named Parameters -----------------------------------
    myp.init();                                     // initialize Globals
    
    if( nmp.fetchAllParms( MY_MAGIC ) )             // try to fetch saved parameters.
        nmp.printAllParms("EEPROM fetched OK");
    else
    {
        nmp.printError();
        nmp.printAllParms("Using and Saving Default Parms");
        nmp.saveAllParms( MY_MAGIC );
    }
    stp.printAllParms("STATE Parms");
    tic.setSecCount( myp.gp.sampleT );              // set EEPROM Sampling Rate

    // -------- 2. Initializing CLI -----------------------------------------------
    exe.registerTable( table );
    exe.printTables( "See CLI table(s)" );  
    cli.init( ECHO_ON, "cmd: " );

    // -------- 3. Starting WiFi --------------------------------------------------
    if( myp.gp.wifion )
    {
        WiFi.begin( myp.gp.ssid, myp.gp.pwd );
        connectWiFi( &oled );
        setupOTA ( myp.gp.myname, &oled );
        startupWEB();                                       // default web end-points
    }    
    // -------- 4. Initializing Sensors -------------------------------------------
    sensors.init();
    sensors.resizeQueue( myp.gp.Nque );
    
    cli.prompt();
}
extern void sendQToWeb( char * );                           // forward reference
BUF js(1024);
void manageRelays();

void loop() 
{
    //Send an HTTP GET request every 20 sec

     if( myp.gp.wifion )
     {
        updateOTA();
        updateWEB();
     }     
     updateCLI();
     if( cli.ready() )
     {
        char *cmd = cli.gets();                     // cmd points to the heap allocated by CLI
        exe.dispatchConsole( cmd );                 // using Serial.printf(); unbuffered version
        cli.prompt();
     }
     if( cpu.buttonPressed() )
     {
        if( myp.st.oledon ) {myp.st.oledon=0; oled.dsp( O_BRIGHT0 );}
        else                {myp.st.oledon=1; oled.dsp( O_BRIGHT1 );}
     }        
     if( tic.ready() )                              // take a measurement
     {
        sensors.read();       
       
        mgn.init();
        if( myp.gp.show&2 )
            mgn.controlVarArgs("CURRENT","Temp:%.2f'F, RH:%.2f%%", sensors.tempF, sensors.humid );
        if( myp.gp.show&1 )
            PFN("NOW %.1fF %.1f%%", sensors.tempF, sensors.humid );

        if( sensors.statsReady() )
        {
            if( myp.gp.show&2 )
                mgn.controlVarArgs("AVERAGED","Temp:%.2f'F, RH:%.2f%%", sensors.tempF1, sensors.humid1 );
            if( myp.gp.show&1 )
                PFN("AVE %.1fF %.1f%%", sensors.tempF1, sensors.humid1 );
//          sensors.printStats();
            if( myp.gp.show&4 )
                sensors.plotStats();

            manageRelays();
            
            if( myp.gp.wifion )
            {
                formJSON( &js );                            // create JSON text
                mgn.controlSetText("TOSEND",js.c_str()+1 ); // uses the {} from JSON!
                sendQToWeb( js.c_str() );                   // send data
            }
        }
     }
}

// State machine for cooling-heating
void flipCool()
{
    pinMode( 32, OUTPUT );          
    pinMode( 33, OUTPUT );
    digitalWrite( 32, HIGH );       // cooling
    digitalWrite( 33, LOW );
}
void flipHeat()
{
    pinMode( 32, OUTPUT );          
    pinMode( 33, OUTPUT );
    digitalWrite( 32, LOW );       // heating
    digitalWrite( 33, HIGH );
}
   
void manageRelays()
{
    int secs = myp.gp.sampleT *         // how often to sample sensors
               myp.gp.Nave;             // how many to average before reporting
    
    // whoever asks to transition, is responsible to set Tcount=0
    // in every state, Tcount increments              
    
    switch( (int)myp.st.cycle  )
    {
        default:
        case CYCLE_INACTIVE:
            flipCool();
            myp.st.cycle = CYCLE_COOLING;
            myp.st.Tcount = 0;
            break;

        case CYCLE_COOLING:            
            myp.st.Tcount += secs;
            if( sensors.tempF1 <= myp.gp.coolthr )
            {
                myp.st.cycle = CYCLE_BELOW_THRES;
                myp.st.Tcount = 0;
            }
            break; // else stay in COOLING
        
        case CYCLE_BELOW_THRES:
            myp.st.Tcount += secs;
            if( sensors.tempF1 <= myp.gp.coolthr )
            {
                if( (myp.st.Tcount >= myp.gp.cooldur) || (sensors.tempF1 <= myp.gp.coolmin) )
                {
                    flipHeat();
                    myp.st.Ncycles++;
                    myp.st.Tcount = 0;
                    myp.st.cycle = CYCLE_HEATING;
                }
                // else stay counting duration
            }
            else        // did not maintain temp below 'coolthr'
            {    
                myp.st.Tcount = 0;
                myp.st.cycle = CYCLE_COOLING;
            }
            break;
        
        case CYCLE_HEATING:
            myp.st.Tcount += secs;
            if( sensors.tempF1 >= myp.gp.heatthr )
            {
                myp.st.Tcount = 0;
                myp.st.cycle = CYCLE_ABOVE_THRES;
            }
            break; // else stay in HEATING
        
        case CYCLE_ABOVE_THRES:
            myp.st.Tcount += secs;
            if( sensors.tempF1 >= myp.gp.heatthr )
            {
                if( (myp.st.Tcount >= myp.gp.heatdur) || (sensors.tempF1 >= myp.gp.heatmax) )
                {
                    flipCool();
                    myp.st.Tcount = 0;
                    myp.st.cycle = CYCLE_COOLING;
                }
                // else stay counting duration
            }
            else        // did not maintain temp below 'heatthr'
            {    
                myp.st.Tcount = 0;
                myp.st.cycle = CYCLE_HEATING;
            }
            break;
    }   
}

// See: https://randomnerdtutorials.com/esp32-http-get-post-arduino/
// void formJSON( BUF *jp );
// This should move to webSupport.cpp
// Create semaphore to avoid conflict

void sendQToWeb( char *jst )                        // uses POST to send data to 'Collector'
{
    if(WiFi.status()!= WL_CONNECTED)            //Check WiFi connection status
    {
        oled.dsp( 7, "\tNo WiFi!" );
        PRN("No WiFi Connection !");  
        return;                                             // do nothing if no WiFi
    }
    B80( url );                                             // create the server URL
    
    url.set( "%s%s?id=%s", myp.gp.site, myp.gp.url, myp.gp.myname );
    
    oled.dsp( 6, "Sending..." );
    oled.dsp( 7, " ");
    HTTPClient http;
    http.begin( url.c_str() );                                      // try to connect with the server
    //http.addHeader("Content-Type", "application/json");   // define Content type
    http.addHeader("Content-Type", "text/plain");           // define Content type /plain or /json is fine
    int httpResponseCode = http.POST( jst );                // POST request to the server,
    oled.dsp( 7, "Code:%d", httpResponseCode );             // wait for results. Display to OLED
       
    if (httpResponseCode>0) 
        PF( "Response:%d, %s\r\n", httpResponseCode, http.getString().c_str() );
        
    http.end();                                         // Free resources
}

// creates a TYPICAL JSON record
void formJSON( BUF *jp )
{
    static int count = 0;       // wraps around 999
    jp->set( "{'sensor':'DHU21D','count':%d,'tempF':%.1f,'humid':%.0f,",
        count++, 
        sensors.tempF1,      // use statistics, not instanteneous
        sensors.humid1 );
    if( count>=999 )
        count = 0;
    sensors.printStats( jp );       // append statistics
    jp->add("}\r\n");
    jp->quotes();    
}
