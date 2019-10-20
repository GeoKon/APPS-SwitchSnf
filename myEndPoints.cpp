/* ----------------------------------------------------------------------------------
 *  Copyright (c) George Kontopidis 1990-2019 All Rights Reserved
 *  You may use this code as you like, as long as you attribute credit to the author.
 * ---------------------------------------------------------------------------------
 */
#include "SimpleSRV.h"      // in GKE-Lw neededfor showArg() and showJson()
#include "myGlobals.h"      // in this project
#include "myEndPoints.h"    // in this project
#include "myCliHandlers.h"

char *myLanding_Page 	= 	"<h2 align='center'><b>SWITCH END POINTS (hardcoded)</b><br/><br/>"\
							"<a href=\"on\">Set Relay ON</a> (/on)<br/>"\
							"<a href=\"off\">Set Relay OFF</a> (/off)<br/>"\
							"<a href=\"toggle\">Toggle Relay</a> (/toggle)<br/>"\
							"<a href=\"check\">Get Relay status</a> (/check)<br/>"\
							"<a href=\"restart\">Restart (soft)</a> (/restart)<br/>"\
							"Click HELP below for additional End Points<br/>"\
							"<br/>"\
							"Click for <a href=\"index.htm\">INDEX</a> | <a href=\"help\">HELP</a> | <a href='/'>ROOT</a></h1></h2>";
							
// ------------- main Callbacks (add your changes here) -----------------

#define SERVER_ON             servpt->on
#define SERVER_SEND         servpt->send

static ESP8266WebServer *servpt;

static void showState()
{
    B80( state );                                       // in the stack

    state.set("{'devID':'%s','label':'%s','model':'%s','relay':%d}\r\n", 
        myp.gp.devID,
        myp.gp.label,
        WiFi.hostname().c_str(),
        myp.relayON ? 1: 0 );
    state.quotes();
    showJson( state.c_str() );
    SERVER_SEND(200, "application/json", state.c_str() );
}
void snfCallbacks( ESP8266WebServer &myserver )
{
    servpt = &myserver;                                // save pointer to server
    
    SERVER_ON("/on", 
    [](){
        showArgs();
        setRelay( true );
        showState();
    });
    SERVER_ON("/off", 
    [](){
        showArgs();
        setRelay( false );
        showState();
    });
    SERVER_ON("/toggle", 
    [](){
        showArgs();
        toggleRelay();
        showState();
    });
    SERVER_ON("/check", 
    [](){
        showArgs();
        showState();
    });
    SERVER_ON("/restart", 
    [](){
        showArgs();
       longjmp( myp.env, 1 );
    });
}
