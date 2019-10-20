#include "cliClass.h"

extern CMDTABLE mypTable[];                             // table of all CLI commands

void setRelay( bool OnOff );
void toggleRelay();					// this DOES NOT suspend pattern during WIFI disconnect
bool rfAsserted();
 
