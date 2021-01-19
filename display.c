/*
Copyright (c) 2014-  by John Chandler

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include "ssd1331.h"
#include "display.h"
#include "heat.h"
#include "application.h"
#include "errorcheck.h"

//
//		Icons defined in the Symbol font
//

static const alt_font symbol_font = { 10, 9, 8, 1, { // Font Header
            {9, 0x00,0x00,0x00,0x00,0x00,0x00, 0x00, 0x00, 0x00 } ,  /*SPC */
            {9, 0x00,0xc0,0x00,0xF0,0x00,0xFc, 0x00, 0xFF, 0x00 } ,  /* !  */
            {9, 0x00,0x18,0x00,0x3C,0x00,0xFF, 0x00, 0x18, 0x00 } ,  /* "  */
            {9, 0x00,0x00,0x44,0x28,0xFE,0xAA, 0x44, 0x00, 0x00 } ,  /* #  */
            {9, 0x00,0x00,0x00,0x00,0x00,0x00, 0x00, 0x00, 0x00 }    /*null*/
            }
        };

#define WIFI_ICON       "!" 	        	// Wifi icon from Symbol font (Defined above)
#define SENSOR_ICON     "\"" 	   	        // Sensor icon
#define BLUETOOTH_ICON	"#"

#define CENTRE(len)     (width-(6*len))/2       // Centre stringlength on screen
#define RIGHT(len)      (width-(6*len))       // Right Align
#define LEFT(len)       (0)                     // Left align

//
//	display temperature from each zone
//
void display_nodes() {
    char string[20];
    int line;
    int zone, node;
    float temp;
    int callsat;

    line =  12;
    for (zone=1; zone < NUM_ZONES; zone++) {		// For all Zones
	if (strcmp(network.zones[zone].name, "") != 0) { // if Zone defined - print details
	    Display_clearline(line);
	    Print_text(network.zones[zone].name, normal, LEFT(), line, DarkGrey);	// Name
	    line = line +8;

	    callsat  = 0;
	    for (node = 0; node < NUM_NODES_IN_ZONE; node++) {				// All defines nodes
		if ((strcmp(network.zones[zone].nodes[node].name, "") !=0) &&
		    (network.zones[zone].nodes[node].temp > 0.0L)) {
		    Display_clearline(line);
		    Print_text(network.zones[zone].nodes[node].name, normal, LEFT(), line, White); // Name

		    callsat = network.zones[zone].nodes[node].callsat;
		    temp = network.zones[zone].nodes[node].temp;
		    if (temp <= 0.0) { sprintf(string, " n/a"); }
		    else { sprintf(string, "%.01f", temp); }
		    Print_text(string, normal, RIGHT(strlen(string)), line, (callsat ? Red : Cyan));

		    line = line +8;
		}
	    } //efor nodes in zones
	}
    } // efor Zone
    Display_clearlinesto(line, height);
}

//
//	Display 
//

void display_zones() {
    char string[20];
    int line, horiz;
    int zone, node;
    float temp, newtemp;
    int callsat;
    time_t run_time;

    line = 12;
    horiz = 10;
    for (zone=1; zone < NUM_ZONES; zone++) {		// For all Zones
	if (strcmp(network.zones[zone].name, "") != 0) { // if Zone defined - print details
	    Print_text(network.zones[zone].name, normal, ((line == 12) ? LEFT() : RIGHT(strlen(network.zones[zone].name))), line, DarkGrey);	// Name
	    line = line +8;

	    temp = 0.0;
	    newtemp = 0.0;
	    callsat  = 0;
	    for (node = 0; node < NUM_NODES_IN_ZONE; node++) {			// All defined nodes
		newtemp = network.zones[zone].nodes[node].temp;
		if ((strcmp(network.zones[zone].nodes[node].name, "") !=0) &&	// if node defined
		    (newtemp > 0)) {						// and temperature valid
		    callsat = (network.zones[zone].nodes[node].callsat ? 1 : callsat);
		    if (temp > 0) {
//			temp = (newtemp < temp ? newtemp : temp);		// take the lower of te zone - ignore 0.0
		    } else {
			temp = newtemp;
		    }
		}
	    } //efor nodes in zones
	    if (temp <= 0.0) { sprintf(string, " n/a"); }
	    else { sprintf(string, "%.01f", temp); }
	    Print_text(string, high, horiz, 32, (callsat ? Red : Cyan));
	    horiz = horiz + 44;
	}
    } // efor Zones

    run_time = get_run_clock();
    sprintf(string, "Run: %02ld:%02ld\n", run_time/3600, (run_time/60)%60);
    Display_clearline(55);
    Print_text(string, normal, CENTRE(strlen(string)), 55, White);
}

//
//	Display visible Bluetooth candidates by zone
//

void display_bluetooth() {
    int line, horiz, i;
    int zone, node;
    char device[BN_LEN+1];
    int	at_home, signal;
    char devname[2];

    line = 12;
    Display_clearline(line);
    line = line + 8;
    for(i = 0; i < BLUETOOTH_CANDIDATES; i++){				// Look at each of the Bluetooth candidates
	device[BN_LEN] = 0;
	BN_CPY(device, bluetooth.candidates[i].name);
	if(strcmp(device, "") !=0){
	    Display_clearline(line);
	    Print_text(device, normal, LEFT(), line, White);		// Name
	    line = line + 8;
	}
    }
    Display_clearlinesto(line, height);

    horiz = 40;
    devname[1] = 0;
    for (zone=0; zone < NUM_ZONES; zone++) {				// For all Zones
	if (strcmp(network.zones[zone].name, "") != 0) { 		// if Zone defined - print details

	    line = 12;
	    horiz = MAX(horiz, 40+(zone*12));
	    devname[0] = network.zones[zone].name[0];
	    Print_text(devname, normal, horiz+2, line, DarkGrey);	// Name
	    line = line +8;

	    for (node = 0; node < NUM_NODES_IN_ZONE; node++) {		// All defined nodes
		at_home = network.zones[zone].nodes[node].at_home;
		if (at_home > 0) {
		    signal = 2;
		    for (i=0; i < BLUETOOTH_CANDIDATES; i++){
			if (at_home & signal) {Print_icon(BLUETOOTH_ICON, horiz, line, Blue);}
			line = line + 8;
			signal = signal << 1;
		    }
		horiz = horiz + 6;
		}
	    } //efor nodes in zones
	}
	horiz = horiz + 6;
    } // efor Zones
}

//
//	Display Slave or Watch Temperature
//
void display_slave(){
    char string[20];

    if (app.temp >  0.0) { 	sprintf(string, "%.01f", app.temp);
    } else {		sprintf(string, " n/a " );	}
    Print_text(string, wh, CENTRE(2*strlen(string)), 25, (app.callsat ? Red : Cyan));

    if (app.setpoint == 0.0) {			// If no target yet set display n/a
	sprintf(string, "n/a");
    } else if (app.operating_mode == OPMODE_SLAVE) { // SLAVE Mode
	if (app.boost) {			// Boosting
	    sprintf(string, "Set:%0.1f>>%0.1f", app.setpoint, app.setpoint+app.boost);
	} else {
	    sprintf(string, "Set:%0.1f", app.setpoint);
	}
    } else {					// WATCH Mode
	if (app.boost==1) {			// Boosting
	    sprintf(string, "Set:%0.1f", app.setpoint);
	} else if (app.boost>1) {		// extra Boosting
	    sprintf(string, "Fix:%0.1f", app.setpoint);
	} else {
	    sprintf(string, "Watch:%0.1f", app.setpoint);
	}
    }
    Display_clearline(56);
    Print_text(string, normal, CENTRE(strlen(string)), 56, White);
}

//
//	Display Hot Water
//
void display_hotwater(char *name, float temp, int callsat, float setpoint, int boost) {
    char string[20];
    int line;

    line = 12;
    Print_text(name, normal, LEFT(), line, DarkGrey);
    line = line +16;

    if (temp >= 1.0) 		  { sprintf(string, "  Hot  "); }
    if ((temp < 1.0)&&(callsat))  { sprintf(string, "Heating"); }
    if ((temp < 1.0)&&(!callsat)) { sprintf(string, " Cool  "); }
    if (temp < 0.0)  		  { sprintf(string, "  n/a  "); }
    Print_text(string, high, CENTRE(strlen(string)), line, (callsat ? Red : Cyan));

    if (setpoint == 0.0) {sprintf(string, "HW: OFF");}
    if (setpoint == 1.0) {sprintf(string, "HW: ON");}
    if (boost)		 {sprintf(string, "H/W: Boost");}

    Display_clearline(56);
    Print_text(string, normal, CENTRE(strlen(string)), 56, White);
}

//
//	Main display process loop
//

void display_process() {
    int		old_mode = 0;

    Init_display();				//Initialise the display
    while (!heat_shutdown) {			// run until requested to shutdown
	if (app.display) { 	Display_on();
	} else {		Display_off();	}

	// To avoid using Clear Screen on each refresh, all display items
	// should clear their space  On switching display modes it is used 
	if (app.display_mode != old_mode) { Display_cls(); old_mode = app.display_mode; }

	Print_icon(WIFI_ICON, LEFT(1), 0, (app.active_node == -1? Red: Green));
	switch (app.operating_mode) {
	case OPMODE_MASTER:
	    switch (app.display_mode) {
	    case 0:{
		Print_time(normal, CENTRE(9), 0, White);
		Print_icon(BLUETOOTH_ICON, RIGHT(2)+3, 0, (!check_any_at_home())? Red: Blue);

		display_zones();
		break;}

	    case 1:
		Print_time(normal, CENTRE(9), 0, White);
		Print_icon(BLUETOOTH_ICON, RIGHT(2)+3, 0, (!check_any_at_home())? Red: Blue);

		display_nodes();
		break;

	    case 2:
		if (strcmp(network.zones[0].nodes[0].name, "") != 0) { 		// if HW node defined - print details
		    Print_time(normal, CENTRE(9), 0, White);
		    Print_icon(BLUETOOTH_ICON, RIGHT(2)+3, 0, (!check_any_at_home())? Red: Blue);

		    display_hotwater(network.zones[0].name,
				     network.zones[0].nodes[0].temp,
				     network.zones[0].nodes[0].callsat,
				     network.zones[0].nodes[0].setpoint,
				     network.zones[0].nodes[0].boost);
		    break;
	        } else {
		    app.display_mode++;						// move to next display (fall through switch)
		}

	    case 3:
		Print_time(normal, CENTRE(9), 0, White);
		Print_icon(BLUETOOTH_ICON, RIGHT(2)+3, 0, (!check_any_at_home())? Red: Blue);

		display_bluetooth();
		break;
	    }
	    break;

	case OPMODE_SLAVE:
	case OPMODE_WATCH:
	    Print_time(normal, CENTRE(9)-5, 0, White);
	    Print_icon(BLUETOOTH_ICON, RIGHT(2)+3, 0, (!app.at_home)? Red: Blue);
	    Print_icon(SENSOR_ICON, RIGHT(2)-9, 0, (app.temp <= 0.0)? Red: Green);

	    display_slave();
	    break;

	case OPMODE_HOTWATER:
	    Print_time(normal, CENTRE(9)-5, 0, White);
	    Print_icon(BLUETOOTH_ICON, RIGHT(2)+3, 0, (!app.at_home)? Red: Blue);
	    Print_icon(SENSOR_ICON, RIGHT(2)-9, 0, (app.temp <= 0.0)? Red: Green);

	    display_hotwater("Hot Water", app.temp, app.callsat, app.setpoint, app.boost);
	    break;
	}
        delay(3000);
    }
    Display_cls();
    Print_text("Shutting", high, CENTRE(8), 20, Cyan);
    Print_text("Down...", high, CENTRE(7), 40, Cyan);
    Display_on();
    delay(2000);

    Display_off();
}

