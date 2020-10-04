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


void display_process() {
    char	string[20];
    int		old_mode = 0;
    int		i;
    char	device[BN_LEN+1];
    char	devname[2];
    int		zone, node;
    int		at_home, signal;
    int		line, horiz;
    float	temp, newtemp;
    int		callsat;
    time_t	run_time;

    Init_display();				//Initialise the display

    while (!heat_shutdown) {			// run until requested to shutdown
	if (app.display) { 	Display_on();
	} else {		Display_off();	}

	// To avoid using Clear Screen on each refresh, all display items
	// should clear their space  On switching display modes it is used 
	if (app.display_mode != old_mode) { Display_cls(); old_mode = app.display_mode; }


	Print_icon(WIFI_ICON, LEFT(1), 0, (app.active_node == -1? Red: Green));

	if ((app.operating_mode == OPMODE_MASTER) &&	// MASTER Mode
	    (app.display_mode == 1)) {			// Display Mode 1

//	Display_cls();

	    Print_time(normal, CENTRE(9), 0, White);
	    Print_icon(BLUETOOTH_ICON, RIGHT(2)+3, 0, (!check_any_at_home())? Red: Blue);

	    line =  12;
	    for (zone=0; zone < NUM_ZONES; zone++) {		// For all Zones
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
if (temp >0.0) { debug(DEBUG_ESSENTIAL,"Display first temp %d, %.01f\n", app.active_node, temp);}
			    if (temp <= 0.0) { sprintf(string, " n/a"); }
			    else { sprintf(string, "%.01f", temp); }
			    Print_text(string, normal, RIGHT(strlen(string)), line, (callsat ? Red : Cyan));

			    line = line +8;
			}
		    } //efor nodes in zones
		}
	    } // efor Zone
	    Display_clearlinesto(line, height);


	} else if ((app.operating_mode == OPMODE_MASTER) && 	// MASTER Mode
		   (app.display_mode == 0)) {			// Display mode 0

	    Print_time(normal, CENTRE(9), 0, White);
	    Print_icon(BLUETOOTH_ICON, RIGHT(2)+3, 0, (!check_any_at_home())? Red: Blue);

	    line = 12;
	    horiz = 10;
	    for (zone=0; zone < NUM_ZONES; zone++) {		// For all Zones
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
//				temp = (newtemp < temp ? newtemp : temp);		// take the lower of te zone - ignore 0.0
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
	    Display_clearline(56);
	    Print_text(string, normal, CENTRE(strlen(string)), 56, White);

	} else if ((app.operating_mode == OPMODE_MASTER) && 	// MASTER Mode
		   (app.display_mode == 2)) {			// Display mode 2

	    Print_time(normal, CENTRE(9), 0, White);
	    Print_icon(BLUETOOTH_ICON, RIGHT(2)+3, 0, (!check_any_at_home())? Red: Blue);

	    line = 16;
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

		    line = 16;
		    devname[0] = network.zones[zone].name[0];
		    Print_text(devname, normal, horiz, line, DarkGrey);		// Name
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

	} else {					// SLAVE or WATCH Mode
	    Print_icon(BLUETOOTH_ICON, RIGHT(2)+3, 0, (!app.at_home)? Red: Blue);
	    Print_icon(SENSOR_ICON, RIGHT(2)-9, 0, (app.temp <= 0.0)? Red: Green);
	    Print_time(normal, CENTRE(9)-5, 0, White);

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

        delay(3000);
    }
    Display_cls();
    Print_text("Shutting", high, CENTRE(8), 20, Cyan);
    Print_text("Down...", high, CENTRE(7), 40, Cyan);
    Display_on();
    delay(2000);


//ENDERROR
    Display_off();
}
