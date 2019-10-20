/* ----------------------------------------------------------------------------------
 *  Copyright (c) George Kontopidis 1990-2019 All Rights Reserved
 *  You may use this code as you like, as long as you attribute credit to the author.
 * ---------------------------------------------------------------------------------
 */
// Minimum necessary for this file to compile

#include <FS.h>
#include <externIO.h>       // IO includes and cpu...exe extern declarations

#include "myGlobals.h"
#include "myCliHandlers.h"

static char *channel = "";
#define RESPONSE( A, ... ) if( bp )                     \
                            bp->add(A, ##__VA_ARGS__ ); \
                           else                         \
                            PF(A, ##__VA_ARGS__);
                            
// ---------------------- Support Routines used by main() ---------------------------

    void setRelay( bool OnOff )       // Main routine to turn relay ON or OFF
    {                            
        char *st;
        
        digitalWrite( RELAY, OnOff );
        myp.relayON = OnOff;
        cpu.led( (onoff_t) OnOff );
        if( OnOff )
        {            
            PF("{UI:CONFIG|SET|dRelay.Text=ON}\r\n");
            PF("{UI:CONFIG|SET|dRelay.BackColor=Green}\r\n");
        }
        else
        {
            PF("{UI:CONFIG|SET|dRelay.Text=OFF}\r\n");
            PF("{UI:CONFIG|SET|dRelay.BackColor=Gray}\r\n");
        }
        myp.refreshRelay = 1; 			// this flag indicates to main to send back a response
    }
	
	// generic routine to check if a pin is asserted; can be called in a tight loop
    bool pinAsserted( int pin )
    {
        static bool justpressed = false;
      
        if( digitalRead(pin) && (!justpressed) )
        {
            justpressed = true;
            return true;
        }
        if( justpressed )                   
        {
            if( !digitalRead(pin) )                  // continue pressing?
                justpressed = false;
        }
        return false;
    }
    bool rfAsserted()
    {
        if( myp.gp.rfpin >= 0 )
        {
            pinMode( myp.gp.rfpin, INPUT );
            return pinAsserted( myp.gp.rfpin );
        }
        return false;
    }
    void toggleRelay()
    {
        if( myp.relayON )
            setRelay( false );
        else
            setRelay( true );
    }   
    void cliSetRelay( int n, char **arg )
    {
        BINIT( bp, arg );
        bool onoff = (n>=1)? atoi( arg[1] ) : 0;            
        setRelay( onoff );
    }

// ----------------- 5. CLI command table ----------------------------------------------
                     
    CMDTABLE mypTable[]= 
    {
        { "relay",  "0=off 1=on. Control Relay", cliSetRelay },
        { "toggle", "Toggle relay", 			[](int, char**){ toggleRelay(); } 		},
        {NULL, NULL, NULL}
    };

 #undef RESPONSE
 
