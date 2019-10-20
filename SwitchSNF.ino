/* ----------------------------------------------------------------------------------
 *  Copyright (c) George Kontopidis 1990-2019 All Rights Reserved
 *  You may use this code as you like, as long as you attribute credit to the author.
 *  ---------------------------------------------------------------------------------
 */
//  >>>>>>>>>>>>> SELECT CPU TYPE in myGlobals.h <<<<<<<<<<<<<<<<<<<

    #define INC_THINGER             // select this to enable THINGER interface
//  #undef  INC_THINGER

    #define INC_SERVER              // select this to enable WEB server
//  #undef  INC_SERVER

    #define _DEBUG_
    #define _DISABLE_TLS_           // very important for the Thinger.io 
    
    #define USERNAME "GeoKon"
    #define DEVICE_ID "SFGKE69"     // this is changed by setupThinger()
    
    #define DEVICE_CREDENTIAL "success"
    
// ----------------------------------------------------------------------------
    #include <FS.h>
    #include <bufClass.h>       // in GKE-L1
    #include <ticClass.h>       // in GKE-L1
    #include <oledClass.h>      // in GKE-L1
    
    #include "SimpleSRV.h"      // in GKE-Lw
    #include "SimpleSTA.h"      // in GKE-Lw
    #include "CommonCLI.h"      // in GKE-Lw
                                
    #include "myGlobals.h"      // in this project. This includes externIO.h
    #include "myEndPoints.h"
    #include "myCliHandlers.h"

    #include <ThingerESP8266.h>
    
//------------------ References and Class Allocations ------------------------

    CPU cpu;                    // declared by externIO.h
    CLI cli;
    EXE exe;
    EEP eep;
    OLED oled;
    
    ESP8266WebServer server( 80 );
    
    BUF buffer( 1024 );         // buffer to hold the response from the local or remote CLI commands.

    ThingerESP8266 thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);   // changed in the setupThinger();

//------------------------- FORWARD REFERENCES -------------------------------
    void toggleRelay();
    void setupThinger();    
    void processChange();
    void updateOLED();
// ----------------------------- Main Setup -----------------------------------

void setup() 
{
    int runcount = setjmp( myp.env );			// env is allocated in myGlobals
    
    cpu.init( 115200, MYLED+NEGATIVE_LOGIC /* LED */, BUTTON+NEGATIVE_LOGIC /* push button */ );
    
    pinMode(RELAY, OUTPUT); 
    digitalWrite( RELAY, LOW );                 // positive logic for SONOFF
    myp.relayON = false;
    cpu.led( OFF );
    
    ASSERT( SPIFFS.begin() );                   // start the filesystem

    oled.dsp( O_LED130 );                       // initialize OLED
    oled.dsp( 0, "Started OK" );

    myp.initAllParms();                         // initialize volatile & user EEPROM parameters
    
    oled.dsp( 1, "Read %d parms", nmp.getParmCount() );
    
    linkParms2cmnTable( &myp );
    exe.initTables();							// clear all tables
    exe.registerTable( cmnTable );              // register common CLI tables
    exe.registerTable( mypTable );              // register tables to CLI
    
    oled.dsp( 2, "Waiting for CLI" );
    startCLIAfter( 5/*sec*/, &buffer );         // this also initializes cli(). See SimpleSTA.cpp    
                  
	oled.dsp( 3, "Waiting for WiFi" );
    setupWiFi();								// use the EEP to start the WiFi

    oled.dsp( 4, "%s", WiFi.SSID().c_str() );
    oled.dsp( 5, "RSSI:%ddBm", WiFi.RSSI() );
    oled.dsp( 6, "IP=%d", WiFi.localIP()[3] );
    
    #ifdef INC_THINGER
        setupThinger();
    #endif

    #ifdef INC_SERVER
        snfCallbacks( server ); 
        srvCallbacks( server, myLanding_Page );   	// standard WEB callbacks. "staLanding" is /. HTML page
        cliCallbacks( server, buffer );             // enable WEB CLI with buffer specified
   
        setTrace( T_REQUEST | T_JSON );             // default WEB trace    
        server.begin( eep.wifi.port );              // start the server
        PRN("HTTP server started."); 
    #endif
    oled.dsp( 7, "All OK!" );
    delay( 1000 );
}

// ----------------------------- Initialize Thniger ------------------------------

    void setupThinger()
    {
#ifdef INC_THINGER
        PF("----- Initializing THING\r\n");
        thing.add_wifi( eep.wifi.ssid, eep.wifi.pwd );  // initialize Thinker WiFi

        if( strncmp( myp.gp.devID, "SFGKE", 5 )==0 ) // double check correct DeviceID
        {                                          
            thing.set_credentials( USERNAME, myp.gp.devID, DEVICE_CREDENTIAL );
            PF("Thing DEVICE_ID is %s\r\n", myp.gp.devID );
        }     
        thing["setLed"]   << invertedDigitalPin( MYLED ); 

        thing["setRelay"] << [](pson& in )       // control switch from the Internet
        {   
            if( in.is_empty() )
                in=0; //myp.relayON;
            else
            {
//                myp.relayON=in;
//                setRelay(  myp.relayON );    
                toggleRelay();
            }
        };
        
//        thing["external"] = []()       // control switch from the Internet
//        {   
//            PF("*** external called ****\r\n");
//        };
//        thing["extJson"] << [](pson &in)       // control switch from the Internet
//        {   
//            PF("Received from IFTTT Value1=[%s], Value2=[%s]\r\n", 
//                (const char *)in["value1"], (const char *)in["value2"] );
//    
//            const char *p1;
//            p1 = (const char *)in["value1"];
//            if( strcmp( p1, "On" ) == 0 ||
//                strcmp( p1, "1" )==0 )
//                    turnOnOff( true, false );
//            if( strcmp( p1, "Off" ) == 0 ||
//                strcmp( p1, "0" )==0 )
//                    turnOnOff( false, false );            
//        };
    
    //  4. Define all status reporting, i.e. from device to internet
    //  ------------------------------------------------------------
        static char temp[32];
        
        thing["status"] >> [](pson& ot )       // meter is read asynchronously by the loop() every N-seconds
        {   
            ot["relayIsON" ] = myp.relayON ? 1 : 0;          // status ON/OFF
            IPAddress myIP = WiFi.localIP();
            //PF("%s last=%d\r\n", myIP.toString().c_str(), myIP[3] );
            sf( temp, 32, "%s IP:%d", myp.gp.label, myIP[3] ); 
            ot["label"  ] = (const char *) temp;
        };
#endif
    }

// ----------------------------------- main loop ---------------------------------
void loopStd()                                          // CLI loop not requiring WiFi
{
    if( cli.ready() )                                   // handle serial interactions
    {
        char *p = cli.gets();
        exe.dispatchBuf( p, buffer );               // required by Meguno
        buffer.print();
        cli.prompt();
    }
    if( cpu.buttonPressed( NO_LED ) )                   // check if button is pressed (and released afterwards)
    {
        PF("FLASH pressed\r\n");
        toggleRelay();
    }
    if( rfAsserted() )                                  // check if button is pressed (and released afterwards)
    {
        PF("RF received\r\n");
        toggleRelay();
    }    
}
void controlAlways()                                    // used as a callback of connectToNetwork() so we always respond to pushbutton!
{
    if( cpu.buttonPressed( NO_LED ) )               // check if button is pressed (and released afterwards)
        toggleRelay();
    if( rfAsserted() )                              // check if button is pressed (and released afterwards)
        toggleRelay();
}

// ---------------------- loop() with or without WiFi ---------------------------

TICsec tk(1);                                       // how often to update OLED

void loop()
{
    loopStd();                                      // no delays in this loop

    if( tk.ready() )
        updateOLED();
        
    if( checkWiFi() )                               // Good WiFi connection?
    {    
        cpu.led( myp.relayON?ON:OFF );				// ensure that CPU light reflects relay state
        #ifdef INC_THINGER         
        if( myp.refreshRelay )
        {
            processChange();
            myp.refreshRelay = 0;
        }
        thing.handle();                             // Continue polling. If WiFi is diconnected
        #endif                                      //   this will have delay loops!

        #ifdef INC_SERVER
        server.handleClient();
        #endif
    }
    else                                            // no WiFi connection
    {        
		if( myp.relayON )                           // force LED ON if relay is set
            cpu.led( ON );                          //  otherwise, play pattern
		reconnectWiFi( controlAlways );	
    }
}
void updateOLED()
{
    oled.dsp( 0, "----------------" );
    oled.dsp( 1, "\t\b%s", myp.relayON?" ON ":" OFF" );
    oled.dsp( 3, "----------------" );
    oled.dsp( 4, "");                               
    oled.dsp( 5, "\tChn:%d %ddBm", WiFi.channel(), WiFi.RSSI() );
    oled.dsp( 6, "");
    oled.dsp( 7, "\tIP .%d:%d", WiFi.localIP()[3], eep.wifi.port );
}
// --------------------------- Reporting to Thinger ------------------------------------

#ifdef INC_THINGER 
    void processChange()                             
    {                                               
        //if( myp.refreshRelay <= 0 )                     // this indicates a change in the state of the relay
        //  return;        
        
        static uint32 T0;
        static char temp[32];
        
        thing.stream( thing["status"] );                // stream to Thinger
        
        //PF("Repeat = %d\r\n", myp.refreshRelay );     // old code just in case we need to refresh Thinger multiple times
        //myp.refreshRelay--;
        
        if( myp.gp.ifttt )                              // is IFTTT activated?
        {
            pson data;
            data["value1"] = (const char *) myp.gp.label;
            if( myp.relayON )
            {
                data["value2"] = "ON";
                T0 = millis();
                PF("IFTTT ON\r\n" );
            }
            else
            {
                int sc = (millis()-T0)/1000L;
                int mn = sc/60;
                sc -= mn*60;                
                sf( temp, 32, "OFF, %d min, %d sec", mn, sc );
                data["value2"] = (const char *)temp;
                PF("IFTTT %s\r\n", temp);
            }
            thing.call_endpoint( "IFTTT_Sonoff", data );  // add here device ID. Add duration to Dropbox
        }
    }
#endif
 
