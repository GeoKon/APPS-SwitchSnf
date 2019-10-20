/* ----------------------------------------------------------------------------------
 *  Copyright (c) George Kontopidis 1990-2019 All Rights Reserved
 *  You may use this code as you like, as long as you attribute credit to the author.
 * ---------------------------------------------------------------------------------
 */
#pragma once

// =================== GLOBAL HARDWARE CONSTANTS ===================================

//#define NODEMCU           // 2. Choose COU either NODEMCU or SONOFF
  #define SONOFF            // Flash=DOUT, Size=1M (256k SPIFFS)

// =================================================================================
/*
    To Program Sonoff
        Select “Generic ESP8266 Module”
        Flash size “1M (128k SPIFFS)”
        Flash Mode: DOUT
        Reset method “ck”
        Crystal “26MHz”; Flash “40MHz”, CPU “80MHz”
        Build in LED “2”
    Disconnect power  Press & Hold Flash  Connect power  Release Flash
    Download new code
    Use command line to initialize Unit Label
*/

#include "IGlobal.h"        // in GKE-Lw. Includes and externs of cpu,...,eep
#include <nmpClass.h>

// ----------------- Exported by this module ---------------------------------------

    extern NMP nmp;             // allocated in myGlobals.cpp; used only by this                             

    #ifdef SONOFF 
        #define BUTTON       0      // INPUTS
        #define RFINP       15      // Do not use 02 (must be high during RESET)
        #define MYLED       13      // OUTPUTS
        #define RELAY       12
    #endif
    
    #ifdef NODEMCU
        #define BUTTON       0      // INPUTS
        #define RFINP        4      // this is D2, aka SDA
        #define MYLED       16      // 2 for old MCU, 16 for new ones
        #define RELAY       12      // NodeMCU D4
    #endif

#define MAGIC_CODE 0x1457

// --------------- Definition of Global parameters ---------------------------------

    enum wifi_state { TRYING_WIFI=0, WIFI_CONNECTED=1, WIFI_DISCONNECTED=-1 };
 
    class Global: public IGlobal
    {
      public:												// ======= A1. Add here all volatile parameters 
        wifi_state wifiOK;                                  // state variable of WiFi
        bool relayON;                                       // state of the relay
        int refreshRelay;
        
		void initVolatile()                                 // ======= A2. Initialize here the volatile parameters
		{
			wifiOK = TRYING_WIFI; 
 			relayON = false;
            refreshRelay = 0;
		}    
		void printVolatile( char *prompt="",                // ======= A3. Add to buffer (or print) all volatile parms
		                    BUF *bp=NULL ){;}
		struct gp_t                                         // ======= B1. Add here all non-volatile parameters into a structure
		{                           
            char label[USER_STR_SIZE];
            char devID[USER_STR_SIZE];
            int  rfpin;                                     // -1=no RF, 1=Use basic v1, 2=use basic v2
            int  ifttt;                                     // bit0=IFTTT         // in deg F
		} gp;
		
		void initMyEEParms()                                // ======= B2. Initialize here the non-volatile parameters
		{
            strcpy( gp.label, "Unit00"  );
            strcpy( gp.devID, "SFGKE00" );
            gp.rfpin = RFINP;
            gp.ifttt = 0;
		}		
        void registerMyEEParms()                           // ======= B3. Register parameters by name
        {
            nmp.resetRegistry();
            nmp.registerParm( "label",     's', &gp.label,    "Location of the unit" );
            nmp.registerParm( "devID",     's', &gp.devID,    "Device ID as known by Thinger" );
            nmp.registerParm( "RFpin",     'd', &gp.rfpin,    "GPIO pin connected to RF module" );
            nmp.registerParm( "IFTTT",     'd', &gp.ifttt,    "0:OFF, 1:ON trigger IFTTT" );

			PF("%d named parameters registed\r\n", nmp.getParmCount() );
			ASSERT( nmp.getSize() == sizeof( gp_t ) );                             
        }
        void printMyEEParms( char *prompt="",               // ======= B4. Add to buffer (or print) all volatile parms
                             BUF *bp=NULL ) 
		{
			nmp.printAllParms( prompt );
		}
        void initAllParms()
        {
            initTheseParms( MAGIC_CODE, (byte *)&gp, sizeof( gp ) );
        }
	};
    
    extern Global myp;                                      // exported class by this module
