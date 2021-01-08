// ---------------- Global Parameters =======================
#pragma once

#include "cpuClass.h"       // This module needs ASSERT. CPU includes myboard.h and Arduino.h
#include "nmpClass.h"
#include "eepClass.h"

extern CPU cpu;             // required for this module
extern NMP nmp;
extern NMP stp;
extern MGN mgn;

#define MY_MAGIC 0x3456     

enum cycle_t                        // values of myp.st.cycle
{
    CYCLE_INACTIVE          = 0,
    CYCLE_COOLING           = 1,    // cooling but still above coolthr
    CYCLE_BELOW_THRES       = 2,    // cooling below coolthr, counting for duration and watching for coolmin
    CYCLE_HEATING           = 3,    // heating but still below heatthr
    CYCLE_ABOVE_THRES       = 4,    // heating above heatthr, counting for duration and watching for heatmax
};

class Globals               // define here the working parameters
{
public:    
    void init()
    {
        initMyEEParms();            // internal initialization of default EEPROM
        initMySTParms();            // internal initialization of state variables
        registerMyEEParms();        // registartion of names of EEPROM parms
        registerMySTParms();        // registartion of names of State parms
    }  
    struct gp_t                     // structure to be saved in eeprom
    {
        char ssid[USER_STR_SIZE];            
        char pwd [USER_STR_SIZE]; 
        char myid[USER_STR_SIZE];
        char site[USER_BIG_SIZE];   // base site -- BIG size
        char url [USER_STR_SIZE];   // base siteserver
        char myname[USER_STR_SIZE];
        int  sampleT;               // how often to sample the sensors (in seconds)
        int  Nave;                  // number of samples to average and report to WEB. Reporting is Naver*T seconds
        int  Nque;                  // depth of reporting queue
        int  wifion;                // 0=no WiFi, 1=wifi client only, 2=web server 
        int  show;                  // 1=printf, 2=MGN, 4=plot 
        
        float  coolthr;
        int    cooldur;
        float  coolmin;

        float  heatthr;
        int    heatdur;
        float  heatmax;
    } gp;   
    
    #define MAX_POST 60
    struct st_t                     // structure of state parameters
    {
        int   oledon;                // LED state
        float postT[ MAX_POST ];    // max number of samples to be posted
        float postH[ MAX_POST ];
        int   postidx;              // 0...MAX_POST-1 index
        
        cycle_t cycle;              // state variable for cooling-heating
        int   Tcount;               // seconds per cycle state
        int   Ncycles;              // number of cycles since reset
    } st;          

    void initMyEEParms()            // initializes memory structure of parameters
    {
        strcpy( gp.ssid, "kontopidis2GHz" );
        strcpy( gp.pwd,  "123456789a" );  
        strcpy( gp.myid, "DTU21a" ); 
        strcpy( gp.myname, "IOT2" );        
        strcpy( gp.site, "http://GeorgeKontopidis.com" );
        strcpy( gp.url,  "/myp/collect1.php");
        
        gp.sampleT   = 2;           // how often to sample sensors
        gp.Nave      = 10;          // every 20 seconds to post
        gp.Nque      = 45;          // depth of the reporting queue
        gp.wifion    = 1;   
        gp.show      = 4+2;       
        
        gp.coolthr   = 28.0;
        gp.cooldur   = 30*60;       // 30min
        gp.coolmin   = 20.0;
        
        gp.heatthr   = 40.0;
        gp.heatdur   = 40;          // 1min
        gp.heatmax   = 60.0;
    }
    void initMySTParms()            // initializes memory structure of parameters
    {
        st.oledon = 1;
        for( int i=0; i< MAX_POST; i++ )
            st.postT[i] = st.postH[i] = 0.0;
        st.postidx = 0;
        st.cycle = CYCLE_INACTIVE;
        st.Tcount = 0;
        st.Ncycles = 0;
    }
    void registerMyEEParms()                // creats the named parameter structure
    {
        nmp.resetRegistry();
        nmp.registerParm( "ssid",   's', gp.ssid,  "my SSID" );
        nmp.registerParm( "pwd",    's', gp.pwd,  "my Password" );
        nmp.registerParm( "myid",   's', gp.myid, "my Sensor ID" );
        nmp.registerParm( "site",   'S', gp.site, "HTTP Site" );
        nmp.registerParm( "myname", 's', gp.myname, "Use http://myname.local" );
        nmp.registerParm( "url",    's', gp.url, "Location of Collector PHP" );
        
        nmp.registerParm( "wifion", 'd',  &gp.wifion,   "1=client on, 2=server on" );
        nmp.registerParm( "show",   'd',  &gp.show,     "1=printf, 2=MGN, 4=graph" );
        nmp.registerParm( "sampleT",'d',   &gp.sampleT, "Sampling rate in sec");
        nmp.registerParm( "Nave",   'd',   &gp.Nave,    "Number of samples to average and post");
        nmp.registerParm( "Nque",   'd',   &gp.Nque,    "Depth of the stats queue");
        
        nmp.registerParm( "coolThr",   'f',   &gp.coolthr,    "Low threshold to start counting time");
        nmp.registerParm( "coolDur",   'd',   &gp.cooldur,    "Number of seconds below 'coolThr'");
        nmp.registerParm( "coolMin",   'f',   &gp.coolmin,    "Absolute minimum temp allowed");
        
        nmp.registerParm( "heatThr",   'f',   &gp.heatthr,    "High threshold to start counting time");
        nmp.registerParm( "heatDur",   'd',   &gp.heatdur,    "Number of seconds above 'heatThr'");
        nmp.registerParm( "heatMax",   'f',   &gp.heatmax,    "Absolute max temp allowed");
        
        PF("%d named EEPROM parms registed. Sizeof %d\r\n", nmp.getParmCount(), nmp.getSize() );
        ASSERT( nmp.getSize() == sizeof( gp_t ) );   
    }
    void registerMySTParms()                // creats the named parameter structure
    {
        stp.resetRegistry();
        stp.registerParm( "oledon",  'd', &st.oledon,   "Brightness 0,1,2,3" );        
        stp.registerParm( "postidx", 'd', &st.postidx,  "Samples enqued" );
        stp.registerParm( "cycle",   'd', &st.cycle,    "1:cooling,2:<coolThr,3:heating,4:>heatThr" );
        stp.registerParm( "Tcount",  'd', &st.Tcount,   "Seconds of threshold exceeded" );
        stp.registerParm( "Ncycles", 'd', &st.Ncycles,  "number of heating cycles" );
        
        PF("%d named State parms registed\r\n", stp.getParmCount() );
        ASSERT( stp.getSize() == 5*4 );   
    }
 };
 extern Globals myp;
 extern BUF clirsp;
 extern BUF webput;
 extern BUF webret;
