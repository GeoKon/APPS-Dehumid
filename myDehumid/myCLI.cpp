// --- needed for OTA ------
    #include <WiFi.h>
//    #include <ESPmDNS.h>
//    #include <WiFiUdp.h>
//    #include <ArduinoOTA.h>
// -------------------------

// ------- Library dependencies ----------------------------
#include <wifiSupport.h>

// ------- In this project folder --------------------------
#include "myGlobals.h"                      
#include "myProcess.h"
#include "myWeb.h"
#include "myCLI.h"

CLI cli;
EXE exe;
TICsec tic(2);                                // sampling rate

void showAllParms( int n, char **arg )
{
    BINIT( bp, arg );
    if( !exe.cmd1stUpper() )
    {
        nmp.printAllParms("EEPROM Parms", bp);
        stp.printAllParms("STATE Parms", bp );
    }
    else
    {
        MGN mgn;
        nmp.printAllParms( "EEPROM", &mgn );
        stp.printAllParms( "STATE",  &mgn );
    }
}
void showStatus( BUF *bp )
{
    uint8_t m[6];
    WiFi.macAddress( m );
    IPAddress ipa; 
    ipa = WiFi.localIP();  

//    printb( bp, "Cores: %d\r\n",        (int) ESP.getChipCores() );
  
// WIFI    
//    printb( bp, " DHCP: %s\r\n",        WiFi.hostname().c_str() );   
    printb( bp, "  MAC: %02x:%02x:%02x:%02x:%02x:%02x\r\n", m[0],m[1],m[2],m[3],m[4],m[5]);
    printb( bp, "   IP: %s\r\n",        ipa.toString().c_str() );
    printb( bp, " SSID: %s\r\n",        WiFi.SSID().c_str() );
    printb( bp, "  PWD: %s\r\n",        WiFi.psk().c_str() );
    printb( bp, " Chan: %d\r\n",        WiFi.channel() );
    printb( bp, " RSSI: %d dBm\r\n",    WiFi.RSSI() );
    printb( bp, "\r\n");

//Internal RAM
    uint64_t chipid = ESP.getEfuseMac();                    //The chip ID is essentially its MAC address(length: 6 bytes).
    printb( bp, " ChipID-MAC: %04X-%08X\r\n", (uint16_t)(chipid >> 32), (uint32_t)chipid); //print High 2 bytes, then low bytes
    printb( bp, " Cores-Freq: %d-%dMHz\r\n",  0, (int) ESP.getCpuFreqMHz() );  
    int hs = ESP.getHeapSize()/1000;
    int fh = ESP.getFreeHeap()/1000;
    int us = ESP.getMinFreeHeap()/1000;
    printb( bp, " Total Heap: %dk\r\n",    hs );
    printb( bp, "  Free Heap: %dk\r\n",    fh );
    printb( bp, "   Heap now: %dk (%d%%)\r\n",   hs-fh, ((hs-fh)*100)/hs );
    printb( bp, "MxHeap Used: %dk (%d%%)\r\n",   hs-us, ((hs-us)*100)/hs );

//SPI RAM
    printb( bp, "   PSR size: %dk\r\n",   ESP.getPsramSize()/1000 );
    printb( bp, "   Free PSR: %dk\r\n",   ESP.getFreePsram()/1000 );
//    printb( bp, " MxPSR Used: %dk\r\n",   (ESP.getPsrSize()-ESP.getMinFreePsram())/1000 );
}

// ---------------------------------------- Trigger Functions ---------------------------------------------------
// These functions are called if the respective parameter or state needs to change
// They return true is change was made. Respective data structures are changed but SET is needed to update the
// EEPROM.

bool NpostChange( BUF *bp, char *pn, char *pv )
{
    if( atoi(pv) > MAX_POST )
    {
        printb( bp, "%s value exceeds max:%d\r\n", pn, MAX_POST );
        return false;
    }
    for( int i=0; i< MAX_POST; i++ )
        myp.st.postT[i] = myp.st.postH[i] = 0.0;
    myp.st.postidx = 0;    
    return true;
}
bool sampleTChange( BUF *bp, char *pn, char *pv )
{
    int value = atoi( pv );
    //PFN("*** TRIGGER %s to change to %d ****", pn, value );
    
    if( value >=1 )
        tic.setSecCount( value );    
    return true;
}
bool oledFlip( BUF *bp, char *pn, char *pv )
{
    int value = atoi( pv );
    myp.st.oledon = value;
    switch( value )
    {
        case 0:     oled.dsp( O_BRIGHT0 ); break;
        case 1:     oled.dsp( O_BRIGHT1 ); break;
        case 2:     oled.dsp( O_BRIGHT2 ); break;
        default:    oled.dsp( O_BRIGHT3 ); break;
    }
    return true;
}
typedef struct
{
  char *name;
  bool (*func)( BUF *bp, char *, char *);

} TRGTABLE;

TRGTABLE mytrg[]                                // Table of tigger functions
{
    {"Npost",           NpostChange},
    {"sampleT",         sampleTChange},
    {"oledon",          oledFlip},
    {NULL, NULL}    
};

bool findTrigger( BUF *bp, char *name, char *val )
{
    TRGTABLE *row = &mytrg[ 0 ];
    for( int j=0; row->name ; j++, row++ )
    {
        if( strcasecmp( row->name, name ) == 0 )
        {
            PFN("Calling trigger func %sChanged()", name );
            return (*row->func)( bp, name, val );
        }
    }
    return false;
}
// ---------------------------------------- SET <name> <value> --------------------------------------------------
void setAnyParm( int n, char **arg )
{
    BINIT( bp, arg );
    if( exe.missingArgs( 2 ) )
        return;
    
    char *pname  = arg[1];
    char *pvalue = arg[2];    
    MGN mgn;
    bool ismeg = exe.cmd1stUpper();

    if( findTrigger( bp, pname, pvalue ) )           // check first if Trigger function is defined
    {                                                // if parameter was found
        if( nmp.setParmByStr( pname, pvalue ) )       // is it an EEPROM parm?
        {
            PFN("*** %s changed to %s ****", pname, pvalue );
            nmp.saveAllParms( MY_MAGIC );           // SAVE in EEPROM
            if( ismeg ) nmp.printParm( "EEPROM", pname, &mgn );
            else        nmp.printParm( pname, bp );
        }
        else                                        // then, must be STATE parm
        {
            if( ismeg ) stp.printParm( "STATE", pname, &mgn );  
            else        stp.printParm( pname, bp );
        }
        return;
    }                                               // Come here if trigger function not detected
    
    if( nmp.setParmByStr( pname, pvalue ) )         // EEPROM parm found and set successfully
    {
        nmp.saveAllParms( MY_MAGIC );
        if( ismeg )     nmp.printParm( "EEPROM", pname, &mgn );
        else            nmp.printParm( pname, bp );
    }
    else                                            // Not EEPROM; try STATE parm
    {
        if( stp.setParmByStr( pname, pvalue ) )
        {
            if( ismeg ) stp.printParm( "STATE", pname, &mgn );
            else        stp.printParm( pname, bp );
        }
        else
        {
            printb( bp, "? %s not found\r\n", pname );   
        }
    }
}
// ---------------------------------------- HELP CLI Function --------------------------------------------------
void help( int n, char **arg )
{
    BINIT( bp, arg );
    BUF xb(1024);        
    MGN mgn;
    exe.help( &xb );    // fill up the buffer  
        
    if( !exe.cmd1stUpper() )
        printb( bp, xb.c_str());
    else
    {
        mgn.controlSetText( "help", " " );      // clear area
        mgn.controlSetText( "help", (const char *)xb.c_str() );        
    }
}

// -------------------------------- INPUT FUNCTIONS --------------------------------

/* Class to periodically check a set of inputs
 * Use:
 *      inp.start()
 *      inp.report()    // in main loop
 *      inp.stop()
 */
class INP
{
private:
    byte pins[10];          // pin number
    byte oldp[10];          // old state
    int npins;
    byte state;             // 0=idle, 1=started, 2=reporting
    uint32_t t0;
public:
    INP()
    {
        for( int i=0; i<10; i++ )
        {
            pins[i] = 0;
            oldp[i] = 0;
            npins = 0;
            BUF *mybp = NULL;
        }
    }
    void stop()
    {
        state = 0;
    }
    void start( int n, char *arg[] )
    {
        int i;
        for( i=0; (i<n) && (i<10); i++ )
        {
            int pin = atoi( arg[i] );
            pinMode( pin, INPUT);
            pins[i] = pin;
        }
        npins = i;
        state = 1;                  // started
    }    
    void reportPins()              // reports state of the pins if different than old ones
    {
        if( state == 0 )            // do nothing
            return;
        if( state == 1 )            // started
        {
            CRLF();                 // clear line after prompt
            for( int i=0; (i<npins) && (i<10); i++ )
            {
                PF( "PIN#%02d ", pins[i] );
                oldp[i]=-1; 
            }
            CRLF();
            state = 2;
            t0=millis();
            return;
        }
        if( millis()< (t0+20) )     // ignore is less than 20ms
            return;
        t0 = millis();
        bool found = false;
        byte state;
        for( int i=0; i<npins; i++ )
        {
            state = digitalRead( pins[i] );
            if( state != oldp[i] )
                found = true;
        }
        if( found )
        {
            for( int i=0; i<npins; i++ )
            {
                state = digitalRead( pins[i] );
                PF( "%s", state?" HIGH  ":"  LOW  " ); 
                oldp[i] = state;
            }
            CRLF();
        }
    }
} inp;

void cliINP( int n, char *arg[] )
{
    BINIT( bp, arg );
    if( n==1 )                      // no args stops reporting
    {
        inp.stop();
        printb( bp, "Stopped\r\n");
    }
    else if( n>1 )                  // pass the pin numbers
        inp.start( n-1, &arg[1] );
}
// -------------------------------- OUTPUT FUNCTIONS --------------------------------
class OUTP
{   
    uint32_t t0, dur;           // used for SQR generator
    int  iopin;                 // which pin
    bool iostate;               // what state
    bool active;                // indicates that SQR is in progress

    int  hipin;                 // pin used for high/low
public:
    OUTP()
    {
        active = false;
        hipin = 16;             // default HI/LO pin
    }
    int pin()                   // returns default pin for HI, LO
    {
        return hipin;
    }
    void high( int pin )       
    {
        if( pin )               // if pin!=0, define pin, mode, and set high
        {
            hipin = pin;
            pinMode( hipin, OUTPUT );
        }
        digitalWrite( hipin, HIGH );        
    }
    void low( int pin )        
    {
        if( pin )               // if pin!=0, define pin, mode, and set low
        {
            hipin = pin;
            pinMode( hipin, OUTPUT );
        }
        digitalWrite( hipin, LOW );
    }
    void start( int pin, int ms )
    {
        dur = ms;
        if( pin )               // if pin!=0, define pin, mode, and set high
        {
            iopin = pin;
            pinMode( iopin, OUTPUT );
        }
        iostate = true;
        active = true;
        digitalWrite( iopin, HIGH );
    }
    void stop()
    {
        active = false;
    }
    void update()
    {
        if( !active ) 
            return;
        if( millis()<t0+dur)
            return;
        t0=millis();
        iostate = !iostate;
        digitalWrite( iopin, iostate?HIGH:LOW );
        PF("PIN#%d %s\r\n", iopin, iostate?"HIGH":"LOW" );
    }
} outp;

// SQR          -- stops
// SQR pin dur  -- uses default pin
void cliSQR( int n, char *arg[] )
{
    BINIT( bp, arg );
    if( n<=2 )
    {
        outp.stop();
        printb( bp, "Stopped\r\n");
        return;
    }
    int pin = atoi( arg[1] );
    int dur = atoi( arg[2] );
    outp.start( pin, dur );
    printb( bp, "SQR#%d started %dms\r\n", pin, dur );    
}
void cliHIGH( int n, char **arg )
{
    BINIT( bp, arg );
    if( n<=1 )              // no arguments
        outp.high( 0 );     // use previously defined pin
    else
        outp.high( atoi( arg[1] ) );
    printb( bp, "PIN#%d set HIGH\r\n", outp.pin() );  
}
void cliLOW( int n, char **arg )
{
    BINIT( bp, arg );
    if( n<=1 )              // no arguments
        outp.low( 0 );      // use previously defined pin
    else
        outp.low( atoi( arg[1] ) );
    printb( bp, "PIN#%d set LOW\r\n", outp.pin() );  
}
//// IN or OUT 2(LED), 4, 5*, 13, 14*, 15*, 16-32. *=PWM on boot
//// IN only 34-39

void cliPWM( int n, char **arg )    // Syntax PWM duty [freq] [pin]
{
    BINIT( bp, arg );
    if( exe.missingArgs(1) )
        return;
    
    int pin = atoi( arg[1] );
    int duty = (n>2)? atoi( arg[2] ) : 50;
    int freq = (n>3)? atoi( arg[3] ) : 1000;
    
    ledcAttachPin( pin, 1 );        // channel #1 of PWM
    ledcSetup( 1, freq,  8 );       // 8-bit resolution
    ledcWrite( 1, duty );    
    printb( bp, "PWM#%d of %dHz at %d%%\r\n", pin, freq, duty );
}
void updateCLI()
{
    inp.reportPins();
    outp.update();
}
// ------------------------------- CLI Functions -----------------------------------
CMDTABLE table[]
{
  {"h",    "[select] help",                                         help },
  {"b",    "[select] brief",                                    [](int n, char **arg){exe.brief( n, arg );  }   },
  {"status","Show System Status",                               [](int n, char **arg){ showStatus( (BUF *)arg[0] );}   },
  {"parms", "Shows all EEP and STP parameters",                 showAllParms },
  {"p",     "same as above",                                    showAllParms },
  {"set",  "name value. Set EEP or STP parameter",              setAnyParm },
  {"hi",    "[pin=4 13 16-32]. Set pin HIGH",                    cliHIGH },
  {"lo",    "[pin=4 13 16-32]. Set pin LOW",                     cliLOW },
  {"sqr",   "pin ms. Create square waveform. No arg stops",     cliSQR },
  {"pwm",   "pin [duty=128] [freq=1000].",                        cliPWM },
  {"inp",   "pins... (4 13 16-39). Sample all. No arg stops",     cliINP },
  {NULL, NULL, NULL}
};
