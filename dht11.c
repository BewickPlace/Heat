/*
copyright (c) 2017-  by John Chandler

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
#include <stdlib.h>
#include <stdint.h>

#include <wiringPi.h>
#include "errorcheck.h"
#include "timers.h"
#include "heat.h"
#include "application.h"

//
//	Button Device variables
//

#define BUTTON_READ_PIN		5		// Read & Write Pins for illuminated button
#define BUTTON_WRITE_PIN	7
#define BUTTON_LONG_PRESS	3000		// 3 sec long press
#define BUTTON_LONGER_PRESS	5000		// 5 sec long press
#define BUTTON_EXTRALONG_PRESS	10000		// 10 sec extra long press

typedef struct {
	int last_pin_state;			// last know pin state
	int new_pin_state;			// latest pin state
	unsigned int edge1;			// edge timer
	unsigned int edge2;			// edge timer
    } pin;

static volatile pin Button_pin;			// Button related pin details

//
//	CALL?SAT  Heating Control Output Pin Mapping
//
static int zone_pin_map[NUM_ZONES] = { 15, 11, 13}; // Pin mapping maximum all zones
#define HOTWATER_READ_PIN	10		// Read pin for Hot Water
#define HOTWATER_WRITE_PIN	12		// Write pin for Hot Water
//
//	DHT11 device variables
//

#define MAX_PULSE_RESPONSES	3		// DHT11 response pulses - transitions to be measured
#define MAX_PULSE_DATA		40		// DHT11 data pulses
#define MAX_PULSE_TIMINGS	(MAX_PULSE_RESPONSES+(2*MAX_PULSE_DATA)) // Maximum number of pulse transitions Response Pulses (2*2) + Data (2*40)
#define MAX_PULSE_WIDTH		59		// Maximum pulse width on data pulses
#define DHT_PIN			8		// DHT control pin (physical Pin number)
#define DHT_PRIORITY		90		// DHT process priority


static int last_pin_state;			// last known state of DHT11 pin
static uint8_t pulse_count;			// count of response pulses
static  int timings[MAX_PULSE_TIMINGS]; 	// record of pulse duration

static int dht11_data[5] = { 0, 0, 0, 0, 0 };

static int read_count;
static int success_count;
static int crc_count;


//
//	Boost Start
//
void	boost_start() {
    app.boost = 1;				// Signal boost
    pinMode(BUTTON_WRITE_PIN, OUTPUT );		// & light Illuminated switch
    digitalWrite(BUTTON_WRITE_PIN, 1);
    if (app.operating_mode == OPMODE_SLAVE) { add_timer(TIMER_BOOST, (60*120)); } //  Boost timesout after 2 hours (not MASTER)
						// Watch node in basic boost will NOT timeout
};

//
//	Boost Stop
//
void	boost_stop() {

    app.boost = 0;				// Signal boost no longer active
    pinMode(BUTTON_WRITE_PIN, OUTPUT );		// &switch off  Illuminated switch
    digitalWrite(BUTTON_WRITE_PIN, 0);
    cancel_timer(TIMER_BOOST);			// cancel timer if running
    send_SAT();					// Send SAT if required
};

//
//	Flash button
//
void	flash_button(int n) {
    int i;
    int flash = 1;

    for(i=0; i< n; i++) {			// repeat x times (even finishes off
	digitalWrite(BUTTON_WRITE_PIN, flash);  // Flash the button

	flash = ( flash ? 0 : 1);		// Toggle the output
	delay(200);
    }
}

//
//	Button Interrupot Handler
//
void	Button_interrupt() {
    int	button_press;				// Length of time button has been pressed

//    piHiPri(DHT_PRIORITY);			// ensure interrupt is given highest priority
    Button_pin.new_pin_state = digitalRead(BUTTON_READ_PIN);// check the new state of the pin
    if (Button_pin.new_pin_state == Button_pin.last_pin_state) { goto EndError; } // debounce...
    Button_pin.edge2 = millis();		// record edge timestamp

    if (Button_pin.new_pin_state == HIGH) {	// if this is the transition back to high
						// handle the button press
	button_press = Button_pin.edge2 - Button_pin.edge1;	// get lebgth of time pressed

        if (button_press < BUTTON_LONG_PRESS) {			// Short press - display
	    debug(DEBUG_TRACE, "Button - Short Press\n");

	    if ((app.operating_mode == OPMODE_MASTER) && 	// On MASTER
		(app.display)) {				// If display already visible
		app.display_mode = ((app.display_mode+1)%4);	// Toggle display mode
	    }

	} else if (button_press < BUTTON_LONGER_PRESS) {	// Long press - Boost
	    debug(DEBUG_TRACE, "Button - Long Press\n");
	    if(!app.boost) {			// If not already boosting
		boost_start();			// Start off the boost
	    } else {
		boost_stop();			// Stop the boost
	    }

	} else if ((button_press < BUTTON_EXTRALONG_PRESS) &&	// Longer press - Extra boost (slave only)
		   (app.operating_mode != OPMODE_MASTER) &&
		   (app.boost)) {			// when already boosting
	    debug(DEBUG_TRACE, "Button - Longer Press\n");
	    app.boost++;
	    flash_button(5);			// briefly flash button to signal extra boost
	    if (app.operating_mode == OPMODE_WATCH) { add_timer(TIMER_BOOST, (60*360)); } //  Boost times out after 6 hours (not MASTER)

	} else if (button_press >= BUTTON_EXTRALONG_PRESS) {	// Extra Long press - Shutdown
	    debug(DEBUG_TRACE, "Button - Extra Long Press\n");
	    heat_shutdown = 1;
	    system("shutdown -h now");

	} else {				//  Non-effective combination
	    debug(DEBUG_TRACE, "Button - No effect\n");
	}
	app.display = 1;			// In all cases ensure display active
	add_timer(TIMER_DISPLAY, 30);		// and timeout after y seconds
    };
    Button_pin.last_pin_state = Button_pin.new_pin_state;  // update last known pin status
    Button_pin.edge1 = Button_pin.edge2;

ENDERROR;
}

//
//	CALL/SAT Zone Control Outputs
//
void	callsat(int zone, int callsat) {
   int	pin = zone_pin_map[zone];			// Obtain the GPIO Pin for this zone

    if (((app.operating_mode == OPMODE_MASTER) && (zone >0)) ||
	((app.operating_mode == OPMODE_HOTWATER) && (zone == 0))) {
	pinMode( pin, OUTPUT );				// Signal to DHT11 read request
	digitalWrite( pin, callsat );			// Write CALL (HIGH) or SAT (LOW)
    } else {
	warn("Incorrect Zone Controls");
    }
}

//
//	DHT Signal Read Request
//

void	dht_signal_read_request() {
    int	new_pin_state;					// Latest state of DHT11 pin
    uint8_t pulse_width;				// count of usec pulse width
    read_count++;

							// ADJUSTED to maximise performace
    last_pin_state = LOW;				// Assume we will miss first edge transition
							// and start looking from next transition to high

    pinMode( DHT_PIN, OUTPUT );				// Signal to DHT11 read request
    digitalWrite( DHT_PIN, LOW );
    delay( 18 );
    digitalWrite( DHT_PIN, HIGH );
    pinMode( DHT_PIN, INPUT );
    delayMicroseconds( 20 );

    for (pulse_count = 0; pulse_count < MAX_PULSE_TIMINGS; pulse_count++) { // For all of pulses in the expected pulse train
	pulse_width = 0;
	new_pin_state = digitalRead(DHT_PIN);		// check the new state of the pin
	while ((new_pin_state == last_pin_state)  &&	// loop measuing the width of the pulse in usec
	       (pulse_width < MAX_PULSE_WIDTH)) {	// stop if pulse width too high (implies missed pulse transition)
	    delayMicroseconds(1);
	    pulse_width++;
	    new_pin_state = digitalRead(DHT_PIN);	// check the new state of the pin
	}
 	timings[pulse_count] = pulse_width;		// save for later analysis
	if (pulse_width == MAX_PULSE_WIDTH) { break; }	// drop out if invalid pulse
	last_pin_state = new_pin_state;			// update last known pin status
    }
}

//
//	display captured timings
//


void	display_timings() {
    int i;
    char string[(30+3*(MAX_PULSE_TIMINGS/2))];	// Risky string length - CAREFUL  if you change strings
    char reading[5];				// Reading: 2 digits + ":" + NULL + 1 spare

    debug(DEBUG_TRACE, "Pulse: %d\n", pulse_count);
    sprintf(string, "Timings:   Low - ");
    for ( i = 0; i < MAX_PULSE_TIMINGS; i+=2 ) { sprintf(reading, "%2d:", timings[i]); strcat(string, reading); }
    strcat(string, "\n");
    debug(DEBUG_TRACE, string);

    sprintf(string, "           High- ");
    for ( i = 1; i < MAX_PULSE_TIMINGS; i+=2 ) { sprintf(reading, "%2d:", timings[i]); strcat(string, reading); }
    strcat(string, "\n");
    debug(DEBUG_TRACE, string);
}

#define NUMDEV	4				// Number of possible device types
#define DEVLEN  10				// Device Revision length

struct devblock {				// Device Block
    char revision[DEVLEN];			// Revision Id (from CPUinfo
    int  dht;					// DHT threshold for this device revision
};

struct devblock	devices[NUMDEV] =	{{"0008",    13},
					 {"0010",    13},
					 {"9000c1",  17},
					 {"unknown", 17}};

static int	dht_threshold;			// High/Low threshold
static int 	min_high = MAX_PULSE_WIDTH;	// Lowest of the high signal pulse widths
static int 	max_low = 0;			// Highest of the low signal pulse widths
static int 	tot_max_low = 0;		// Sum of max_lows used to provide average
//
//	Set DHT threshold
//
void	set_dht_threshold() {
    char rev[DEVLEN];				// Device revision
    int	 i = 0;					// index

    PiRevision(rev);				// Obtain current device Revision from CPUinfo
    while (( i < NUMDEV) &&
	   ( strcmp(rev, devices[i].revision) != 0)) { // Find match or unknown entry
	i++;
    }
    if (i >= NUMDEV) { i = NUMDEV -1;}		// if not found use last entry in the table
    dht_threshold = devices[i].dht;		// set DHT threshold accordingly
    debug(DEBUG_TRACE, "DHT11 Threshold set to %d based on revision %s\n", dht_threshold, devices[i].revision);
}

//
//	Parse Data
//
int	parse_data(int dht) {
    int data_count = 0;
    int	i;
    int ret;

    for (i=0; i < 5; i++) dht11_data[i] = 0;	// initialise data
    min_high = MAX_PULSE_WIDTH;
    max_low = 0;

    for (i = MAX_PULSE_RESPONSES; i < pulse_count; i++) { // for each of the pulses (ignoring startup pulses)
	if (i % 2 == 1) {			// place bit in appropriate data array
	    dht11_data[data_count / 8] <<= 1;	// check pulse width for data 1 or 0
	    if ( timings[i] > dht ) {
		dht11_data[data_count / 8] |= 1;
		min_high = (timings[i] < min_high ? timings[i] : min_high);
	    } else {
		max_low  = (timings[i] > max_low  ? timings[i] : max_low );
	    }
	    data_count++;
	}
    }
    ret = (dht11_data[4] == ((dht11_data[0] + dht11_data[1] + dht11_data[2] + dht11_data[3]) & 0xFF));
    debug(DEBUG_INFO, "DHT11 Result %d T/L/H [%2d:%2d:%2d] Data:[%3d][%3d][%3d][%3d][%3d]\n", ret, dht, max_low, min_high, dht11_data[0], dht11_data[1], dht11_data[2], dht11_data[3], dht11_data[4]);
    return(ret);
}
//
//	Interpret DHT response tinings & extract data
//

int	dht_interpret_data() {
    int good = 0;
    int dht_reparse_low;
    int dht_reparse_high;
						// Throws out incomplete data
    if (pulse_count <((2* MAX_PULSE_DATA)+MAX_PULSE_RESPONSES)) goto ReadError;

    good = parse_data(dht_threshold);		// Parse good data
    if (good) {					// If OK

    } else {					// otherwise
	dht_reparse_low = max_low - 1;
	dht_reparse_high= min_high + 0;
	if ((!good) &&
	    (dht_reparse_high <= (dht_threshold+2))) {// possible higher match
	    good = parse_data(dht_reparse_high);// Reparse using higher threshold
	    if (good) { debug(DEBUG_TRACE, "DHT11 Re-parsed, T   [%d>>%d]\n", dht_threshold, dht_reparse_high);}
	}
	if ((!good) &&
	    (dht_reparse_low >= dht_threshold-5)) {// possible lower match
	    good = parse_data(dht_reparse_low);	// Reparse using lower threshold
	    if (good) { debug(DEBUG_TRACE, "DHT11 Re-parsed, T   [%d<<%d]\n", dht_reparse_low, dht_threshold);}
	}
    }
    if (!good) goto CRCError;			// If still no good CRC error
    success_count++;
    tot_max_low = tot_max_low + max_low;

ERRORBLOCK(ReadError);
    debug(DEBUG_INFO, "DHT11 Incomplete Read data, count[%d/%d]\n", pulse_count, MAX_PULSE_TIMINGS);
    DEBUG_FUNCTION( DEBUG_DETAIL, display_timings());
    return(0);
ERRORBLOCK(CRCError);
    debug(DEBUG_TRACE, "DHT11 CRC error, L/H [%d]\n", dht_threshold);
    DEBUG_FUNCTION( DEBUG_INFO, display_timings());
    crc_count++;
    return(0);
ENDERROR;
    DEBUG_FUNCTION( DEBUG_DETAIL, display_timings());
    return(1);
}

//
//	Read DHT11 Pressure & Temperature sensor device
//

#define		MAX_DHT_RETRYS	10		// Maximum nuber of reties to get a valid reading
#define         DHT_SIGN	0x80		// DHT22 Sign bit
#define         DHT_DATA	0x7F		// DHT22 Data bits

void read_dht11() {
    int	i;
    int raw_temperature;
    float new_temp;
						// initialise data handling variables
    for ( i = 0; i < MAX_PULSE_TIMINGS; i++ ) { timings[i] = 0; } // record of pulse durations

    piHiPri(DHT_PRIORITY);			// ensure thread is given highest priority
    dht_signal_read_request();			// Signal to DHT11 read request
    i= 0;
    while ((!dht_interpret_data()) && 		// Interpret the data, check for completeness and CRC
	   (i < MAX_DHT_RETRYS)) {		// Too many Data Errors
	i++;					// Increment error count
	delay(2000);				// allow DHT11 to stabalise
	if (heat_shutdown) goto EndError;	// nut exit if shutdown requested
	dht_signal_read_request();		// and retry Read request
    }
    if ((i== MAX_DHT_RETRYS) &&  (app.temp < 0.0)) { goto ReadError; } // Fail but without (re-)reporting the error
    ERRORCHECK(i== MAX_DHT_RETRYS, "DHT11 Persistant Read Failure", ReadError);  // Fail

    //	Support for DHT11 or DHT22 devices
    //	DHT11 - uses data[2].data[3]
    //  DHT22 - uses data[2]*256 + data[3] /10, plus sign bit in data[2]
    //

    if ((dht11_data[2] & DHT_DATA) < 30) {	// Data looks valid (have encountered some problems with adaptive crc check)
	if ((dht11_data[2] & DHT_DATA) < 4) {	// Device is most likely a DHT22
	    raw_temperature = ((dht11_data[2] & DHT_DATA) << 8) + dht11_data[3];
	    if(dht11_data[2] & DHT_SIGN) { raw_temperature  = -raw_temperature;}
	    debug(DEBUG_INFO, "DHT Device DHT22 [%d.%d] => %d.%d\n", dht11_data[2], dht11_data[3], raw_temperature/10, abs(raw_temperature%10));

	} else {				// Device is most liekly a DHT11
	    raw_temperature = (dht11_data[2] * 10) + dht11_data[3];
	    debug(DEBUG_INFO, "DHT Device DHT11 [%d.%d] => %d.%d\n", dht11_data[2], dht11_data[3], raw_temperature/10, abs(raw_temperature%10));
	}
	new_temp = (float) raw_temperature / 10.0;
	if ((app.temp <= 0)  ||			// if existing temp invalid - override
	    ((new_temp > (app.temp - 5.0)) &&	// or +- 5 degrees on current
	     (new_temp < (app.temp + 5.0)))) {
	    app.temp = new_temp;
	} else {
	    debug(DEBUG_TRACE, "DHT11 temperature out of range %0.1f [%d.%d] - ignored", new_temp, dht11_data[2], dht11_data[3]);
	}

    } else {					// Data is out of realistic range - don't chage reported temp
	debug(DEBUG_TRACE, "DHT11 temperature out of range [%d.%d] - ignored", dht11_data[2], dht11_data[3]);
    }

ERRORBLOCK(ReadError);
    app.temp = -0.1;
ENDERROR;
}

#define	DHT11_OVERALL	(300)			// DHT11 Cycle overall timer
#define DHT11_READ	(10)			// DHT11 Read cycle

//
//	Initialise wiring Pi
//
void	initialise_GPIO() {
    if ( wiringPiSetupPhys() == -1 )
	exit( 1 );
}
//
//	Monitor Sensor Process
//

void monitor_process()	{
    int	rc = -1;
    int cycle_time = DHT11_OVERALL;
    float efficiency;
    int new_dht_threshold;

    read_count = 0;				// Initialise read status
    success_count = 0;
    crc_count = 0;

    boost_stop();				// Initialise with no boost
    pinMode(HOTWATER_READ_PIN, INPUT );		// Hot Water Read pin
    pullUpDnControl(HOTWATER_READ_PIN, PUD_DOWN);  // ensure pulldown for open contact
    pinMode(HOTWATER_WRITE_PIN, OUTPUT );	// Hot Water write pin

    Button_pin.last_pin_state = HIGH;		// last known pin state
    Button_pin.edge1 = millis();		// record starting edge timestamp
    rc = wiringPiISR(BUTTON_READ_PIN, INT_EDGE_BOTH, &Button_interrupt);  // Interrupt on rise or fall of DHT Pin
    ERRORCHECK( rc < 0, "DHT Error - Pi ISR problem", EndError);

    flash_button(10);				// Flash button to signal start
    set_dht_threshold();			// Set the threshold for DHT data values

    while ( !heat_shutdown )	{
	if (((cycle_time % DHT11_READ) == 0) &&		// Every x seconds
	    (app.operating_mode != OPMODE_MASTER)) {	// and only on slave

	    if(app.operating_mode == OPMODE_HOTWATER) {	// Hotwater node - check via GPIO
		digitalWrite(HOTWATER_WRITE_PIN, 1);	// Raise the write pin
		app.temp = (digitalRead(HOTWATER_READ_PIN) ? 0.8 : 1.2); // set temperature if  signal found
		digitalWrite(HOTWATER_WRITE_PIN, 0);	// Lower the write pin

	    } else {					// Temperature node - check via DHT11/22
		read_dht11();

		if(cycle_time == 0) {
	    	    efficiency = ((float)success_count/ (float)read_count)* 100.0;
						// Adjust the DHT threshold based on the
						// average of successful reads low threshold
						// adjusted
		    new_dht_threshold = ((success_count>0) ? (tot_max_low/success_count)+4 : dht_threshold);
		    if ((efficiency < 65.0)&& (!(app.temp < 0.0))) {	// report poor DHT effeciency unless we have persistent failure
			warn("DHT11 efficiency %2.0f%, read[%d], ok[%d], crc[%d] L/H[%d>>%d]", efficiency, read_count, success_count, crc_count, dht_threshold, new_dht_threshold);
		    } else {
			debug(DEBUG_TRACE, "DHT11 efficiency %2.0f%, read[%d], ok[%d], crc[%d] L/H[%d>>%d]\n", efficiency, read_count, success_count, crc_count, dht_threshold, new_dht_threshold);
		    }
		    if (dht_threshold != new_dht_threshold) {debug(DEBUG_TRACE, "Adjust DHT threshold [%d>>%d]\n", dht_threshold, new_dht_threshold);}
		    dht_threshold = new_dht_threshold;
		    read_count = 0;
		    success_count = 0;
		    crc_count = 0;
		    tot_max_low = 0;
		}
	    }
	}
	delay( 1000 );
	cycle_time =( cycle_time + 1) % DHT11_OVERALL;
    }
    boost_stop();				// Tidy up with no boost
    flash_button(10);				// Flash button to signal shutdown

ENDERROR;
    return;
}
