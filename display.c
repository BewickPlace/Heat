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

//
//		Icons defined in the Symbol font
//

static const alt_font symbol_font = { 10, 9, 8, 1, { // Font Header
            {9, 0x00,0x00,0x00,0x00,0x00,0x00, 0x00, 0x00, 0x00 } ,  /*SPC */
            {9, 0x00,0xc0,0x00,0xF0,0x00,0xFc, 0x00, 0xFF, 0x00 } ,  /* !  */
            {9, 0x04,0x03,0x04,0x03,0x00,0x00, 0x00, 0x00, 0x00 } ,  /* "  */
            {9, 0x00,0x40,0x21,0xA4,0xAA,0xAA, 0xAA, 0xA4, 0x21 } ,  /* #  */
            {9, 0x24,0x2A,0x7F,0x2A,0x12,0x00, 0x00, 0x00, 0x00 } ,  /* $  */
            {9, 0x23,0x13,0x08,0x64,0x62,0x00, 0x00, 0x00, 0x00 } ,  /* %  */
            {9, 0x00,0x00,0x00,0x00,0x00,0x00, 0x00, 0x00, 0x00 }    /*null*/
            }
        };

#define WIFI_ICON       "!" 	        	// Wifi icon from Symbol font (Defibed above)
#define SENSOR_ICON     "\""    	        // Sensor icon

void display_process() {
    Init_display();				//Initialise the display

    while (!heat_shutdown) {			// eun until requested to shutdown
	Display_on();
        Display_cls();				// Clear the screen

	Print_icon(WIFI_ICON, 0, 0, Green);
	Print_icon(SENSOR_ICON, 0, 86, Red);
	Print_time(normal, 0, 10, White);
	Print_text("19.6%", wh, 25, 30, White);
	Print_text("Set: 21%", normal, 56, 20, White);

        delay(3000);
	Display_off();
        delay(5000);

    }
    Display_off();
    return;
}
