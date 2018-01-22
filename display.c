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
#include "errorcheck.h"

//
//		Icons defined in the Symbol font
//

static const alt_font symbol_font = { 10, 9, 8, 1, { // Font Header
            {9, 0x00,0x00,0x00,0x00,0x00,0x00, 0x00, 0x00, 0x00 } ,  /*SPC */
            {9, 0x00,0xc0,0x00,0xF0,0x00,0xFc, 0x00, 0xFF, 0x00 } ,  /* !  */
            {9, 0x81,0x7E,0x81,0x7E,0x81,0x7E, 0x00, 0x3C, 0x00 } ,  /* "  */
            {9, 0x00,0x00,0x00,0x00,0x00,0x00, 0x00, 0x00, 0x00 }    /*null*/
            }
        };

#define WIFI_ICON       "!" 	        	// Wifi icon from Symbol font (Defibed above)
#define SENSOR_ICON     "\"" 	   	        // Sensor icon

#define CENTRE(len)     (width-(6*len))/2       // Centre stringlength on screen
#define RIGHT(len)      (width-(6*len))       // Right Align
#define LEFT(len)       (0)                     // Left align


void display_process() {
    char	string[10];

    Init_display();				//Initialise the display
    ERRORCHECK( app.operating_mode == OPMODE_MASTER, "Master Mode: no display required", EndError);

    while (!heat_shutdown) {			// run until requested to shutdown
	if (app.display) { 	Display_on();
	} else {		Display_off();	}

	// To avoid using Clear Screen on each refresh, all display items
	// should clear their space

	Print_icon(WIFI_ICON, LEFT(1), 0, (app.active_node == -1? Red: Green));
	Print_icon(SENSOR_ICON, RIGHT(2), 0, (app.temp <= 0.0)? Red: Green);
	Print_time(normal, CENTRE(9), 0, White);

	if (app.temp >  0.0) { 	sprintf(string, "%.01f", app.temp);
	} else {		sprintf(string, " n/a " );	}
	Print_text(string, wh, CENTRE(2*strlen(string)), 25, Cyan);

	if (app.setpoint == 0.0) {		// If no target yet set display n/a
	    sprintf(string, "n/a");
	} else if (app.boost) {			// Boosting
	    sprintf(string, "Set:%0.1f>>%0.1f", app.setpoint, app.setpoint+app.boost);
	} else {
	    sprintf(string, "Set:%0.1f", app.setpoint);
	}
	Print_text("               ", normal, CENTRE(15), 56, White);
	Print_text(string, normal, CENTRE(strlen(string)), 56, White);

        delay(5000);

    }
ENDERROR
    Display_off();
}
