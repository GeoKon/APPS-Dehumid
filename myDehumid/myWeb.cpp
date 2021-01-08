

/*  
String SendHTML(uint8_t led1stat )
{
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr +="<title>LED Control</title>\n";
  ptr +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr +="body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
  ptr +=".button {display: block;width: 80px;background-color: #3498db;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}\n";
  ptr +=".button-on {background-color: #3498db;}\n";
  ptr +=".button-on:active {background-color: #2980b9;}\n";
  ptr +=".button-off {background-color: #34495e;}\n";
  ptr +=".button-off:active {background-color: #2c3e50;}\n";
  ptr +="p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";
  ptr +="</style>\n";
  ptr +="</head>\n";
  ptr +="<body>\n";
  ptr +="<h1>ESP32 Web Server</h1>\n";
  ptr +="<h3>Using Access Point(AP) Mode</h3>\n";
  
   if(led1stat)
  {ptr +="<p>LED1 Status: ON</p><a class=\"button button-off\" href=\"/OFF\">OFF</a>\n";}
  else
  {ptr +="<p>LED1 Status: OFF</p><a class=\"button button-on\" href=\"/ON\">ON</a>\n";}

  ptr +="</body>\n";
  ptr +="</html>\n";
  return ptr;
}
*/

const char dlinks[]  =      "<h1>LINKS</h1><p>"\
                            "<a href='/wifi'>       Show WiFi Status        </a> (/status)<br/>"\
                            "<a href=\"reset\">     Reset CPU               </a> (/reset)<br/>"\
                            "<a href='/cli?cmd=h'> List of CLI commands     </a> (/cli?cmd=h)<br/>"\
                            "<a href='/act'>       Interactive CLI mode    </a> (/act)<br/>"\
                            "<a href='/cli?cmd=show'>Show Parms             </a> (/cli?cmd=show)<br/>"\
                            "</p>";

const char droot[]          = "<h1>Temperature and Humidity Sensing</h1>\n";
                      //"<h3>Using Access Point(AP) Mode</h3>\n";
                      //"<p>LED1 Status: ON</p><a class=\"button button-off\" href=\"/OFF\">OFF</a>\n"; 
                      //"<p>LED1 Status: OFF</p><a class=\"button button-on\" href=\"/ON\">ON</a>\n"\
// ---------------------------------- APPLICATION SPECIFIC --------------------------
#include "myGlobals.h"
#include "myProcess.h"
#include "webSupport.h"

#include <WebServer.h>
WebServer server(80);
BUF webpage( 2024 );

#define SERVER_ON   server.on
#define SERVER_SEND server.send

void startupWEB( ) 
{
    SERVER_ON("/",                  // application must define the root
    []()
    {
        webpage.set( "%s %s<p class='field'>%.1f &deg;F</p> <p class='field'>%.1f %%</p>",
            drefr, 
            droot, 
            sensors.tempF,
            sensors.humid );
        
        char *sc;
        switch( myp.st.cycle )
        {
            case CYCLE_COOLING: sc ="Cooling"; break;
            case CYCLE_HEATING: sc ="Heating"; break;
            case CYCLE_ABOVE_THRES: sc ="HiThr"; break;
            case CYCLE_BELOW_THRES: sc ="LoThr"; break;
            default: sc = "Unknown"; break;
        }
                    
        webpage.add( "<p>%s %d:%02d</p>%s",
            sc, 
            myp.st.Tcount/60, myp.st.Tcount%60,
            dterm );
        webpage.quotes();
        SERVER_SEND(200, "text/html", webpage.c_str() ); 
    });
    SERVER_ON("/links",             // application must define app links
    []()
    {
        webpage.set( "%s %s %s", dhead, dlinks, dterm );
        webpage.quotes();
        SERVER_SEND(200, "text/html", webpage.c_str() ); 
    });
    webSupport( &server, &webpage );        // expand with embedded WEB end-points
    server.begin();
    PRN("HTTP server started");
}
void updateWEB()
{
    server.handleClient();
}
#undef SEND_ON
#undef SEND_SEND
