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
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <sys/stat.h>

//#include <unistd.h>
//#include <signal.h>
//#include <assert.h>
//#include <fcntl.h>
//#include <sys/time.h>
#include <time.h>
//#include <sys/types.h>
//#include <sys/socket.h>
//#include <sys/uio.h>
//#include <netinet/in.h>
//#include <netinet/ip.h>
//#include <arpa/inet.h>
//#include <net/if.h>

#include "errorcheck.h"
#include "dht11.h"
#include "networks.h"
#include "timers.h"
#include "heat.h"
#include "application.h"

#define	HOURS24	(24*60*60)

//
//	start run clock
//

void	start_run_clock() {
    time_t now = time(NULL);
    if (app.run_time_start != 0) {
	app.run_time = app.run_time + (now - app.run_time_start);
    }
    app.run_time_start = now;
}
//
//	stop run clock
//

void	stop_run_clock() {
    time_t now = time(NULL);
    if (app.run_time_start != 0) {
	app.run_time = app.run_time + (now - app.run_time_start);
    }
    app.run_time_start = 0;
}
//
//	reset run clock
//

void	reset_run_clock() {

    app.run_time = 0;
    app.run_time_start = 0;
}

//
//	get run closk
//
time_t	get_run_clock() {
    time_t now = time(NULL);

    if (app.run_time_start == 0) {
	return(app.run_time);
    } else {
	return(app.run_time + (now - app.run_time_start));
    }
}
//
//		Check Any Bluetooth
//
int	check_any_at_home() {
    int zone, node;
    int any = 0;

    for (zone=0; zone < NUM_ZONES; zone++) {				// Check all Zones
	for (node=0 ; node < NUM_NODES_IN_ZONE; node ++) {		// and nodes
//	    if (network.zones[zone].nodes[node].at_home) return(1);	// report if any positive
	    any = any | network.zones[zone].nodes[node].at_home;	// build mask of visible devices
	}
    }
    any = (any | app.at_home) | app.boost;
    return(any);
}

//
//		Check Any CALL in Zone
//
int	check_any_CALL_in_zone(int zone) {
    int node;

    for (node=0 ; node < NUM_NODES_IN_ZONE; node ++) {			// Check all nodes
	if (network.zones[zone].nodes[node].callsat) return(1);		// report if any positive
    }
    return(0);
}

//
//		Check Any CALL
//
int	check_any_CALL() {
    int zone;

    for (zone=0; zone < NUM_ZONES; zone++) {				// Check all Zones
	if (check_any_CALL_in_zone(zone)) return(1);			// report if any positive
    }
    return(0);
}

#define	SIGNAL_AT_HOME(signal) (signal & 0x007F)
#define SIGNAL_CALLING(signal) (signal & 0xFF00)
static	int			last_time = 0x0080;			// Bitmap mask of signals last logged
//
//		Check any signal (CALL or At Home)
//
int	check_any_signals() {
    int signal = 0;
    int zone;

    for (zone=0; zone < NUM_ZONES; zone++) {				// Check all Zones
	signal = signal + check_any_CALL_in_zone(zone);			// Check each zone separately
        signal = signal << 1;
    }
    signal = (signal * 0x100) + check_any_at_home();				// and check At Home
    return(signal);
}

//
//		Manage Call for heat (MASTER)
//

void 	manage_CALL(char *node_name, float temp, int at_home) {
    int	zone;
    int node;
    int this_time;
    int this_call, last_call;
    time_t time24;
    struct tm *info;

    time24 = time(NULL);					// Get the current time
    info = localtime(&time24);					// convert to DST
    time24 = (((info->tm_hour * 60) + info->tm_min) * 60) + info->tm_sec; // and get 24hout time

    if((time24 > network.on) && (time24 < network.off)) {	// Only handle CALL when Network control is ON

    for( zone = 0; zone < NUM_ZONES; zone++) {			// check what zone and node we match
	node = match_node(node_name, zone);
	if (node != -1) break;
    }
    ERRORCHECK(node < 0, "Live node mismatch with configuration", NodeError);
								// Valid Zone and node index
    last_call = check_any_CALL();				// Check if anyone is already CALLing before we process
    network.zones[zone].nodes[node].temp = temp;		// Save the current temperature
    network.zones[zone].nodes[node].at_home = at_home;		// Save the current bluetooth status
    if (network.zones[zone].nodes[node].callsat) { goto Checks; } // if already in CALL skip to checks

    network.zones[zone].nodes[node].callsat = 1;		// Mark as CALLing

    this_call = check_any_CALL();				// Check if anyone now CALLing
    if (last_call != this_call) {
	start_run_clock();					// if starting up START the run clock
	debug(DEBUG_ESSENTIAL, "CALL for heat  @ %-12s (%d:%d) - Clock Start\n",node_name, zone, node);
    } else {
	debug(DEBUG_ESSENTIAL, "CALL for heat  @ %-12s (%d:%d)\n",node_name, zone, node);
    }
    callsat(zone, 1);					    	// interface with DHT11 module to action

Checks:
    this_time = check_any_signals();				// Check if any Signal has changed
    if (last_time != this_time) {
	perform_logging();					// Log the fact
    }
    if (SIGNAL_AT_HOME(this_time) && !SIGNAL_AT_HOME(last_time)) { // if are now signalling at home
	debug(DEBUG_ESSENTIAL, "Bluetooth Candidates Now At Home - revise setpoints\n"); // Note
	network.at_home_delta = network.at_home_delta_out;	// revert to OUT mode
	add_timer(TIMER_CONTROL, 1);				// and trigger setpoint reassessment
    }
    last_time = this_time;					// Save signals Bit mask for next time
    } else {							// Network Control OFF
	manage_SAT(node_name, temp, at_home);			// Treat this as a SAT
    }

ERRORBLOCK(NodeError);
   debug(DEBUG_ESSENTIAL, "Mismatch %s\n", node_name);
ENDERROR;
}

//
//		Manage SAT heat (MASTER)
//

void 	manage_SAT(char *node_name, float temp, int at_home) {
    int	zone;
    int node;
    int this_time;
    int this_call, last_call;

    for( zone = 0; zone < NUM_ZONES; zone++) {			// check what zone and node we match
	node = match_node(node_name, zone);
	if (node != -1) break;
    }
    if(node <0) { goto EndError; }				// Ignore error - likely to come from Watch device
								// Valid Zone and node index
    last_call = check_any_CALL();				// Check if anyone is already CALLing before we process
    network.zones[zone].nodes[node].temp = temp;		// Save the current temperature
    network.zones[zone].nodes[node].at_home = at_home;		// Save the current temperature
    if (!network.zones[zone].nodes[node].callsat) { goto Checks; } // if already SAT skip to end
    network.zones[zone].nodes[node].callsat = 0;		// Mark as SATisfied

    if(!check_any_CALL_in_zone(zone)) {				// Check if zone now fully satisfied
	callsat(zone, 0);				   	// interface with DHT11 module to action
    }
    this_call = check_any_CALL();				// Check if anyone now CALLing
    if (last_call != this_call) {
	stop_run_clock();					// if starting up STOP the run clock
	debug(DEBUG_ESSENTIAL, "Heat SATisfied @ %-12s (%d:%d) - Clock Stop\n",node_name, zone, node);

    } else {
	debug(DEBUG_ESSENTIAL, "Heat SATisfied @ %-12s (%d:%d)\n",node_name, zone, node);
    }

Checks:
    this_time = check_any_signals();				// Check if any Signal has changed
    if (last_time != this_time) {
	perform_logging();					// Log the fact
    }
    if (SIGNAL_AT_HOME(this_time) && !SIGNAL_AT_HOME(last_time)) { // if are now signalling at home
	debug(DEBUG_ESSENTIAL, "Bluetooth Candidates Now At Home - revise setpoints\n"); // Note
	network.at_home_delta = network.at_home_delta_out;	// revert to OUT mode
	add_timer(TIMER_CONTROL, 1);				// and trigger setpoint reassessment
    }
    last_time = this_time;					// Save signals Bit mask for next time

ENDERROR;
}

//
//		Manage CLOSE of control function (MASTER)
//

void 	manage_CLOSE() {
    int	zone;

    if (app.operating_mode == OPMODE_MASTER) {
    debug(DEBUG_TRACE, "Zone controls shut down\n");
    for( zone = 0; zone < NUM_ZONES; zone++) {			// check what zone and node we match
	callsat(zone, 0);
    }
    }
}
//
//		Manage TEMPerature advise (MASTER)
//

void 	manage_TEMP(char *node_name, float temp, int at_home) {
    int	zone;
    int node;
    int this_time;
    time_t time24;
    struct tm *info;

    time24 = time(NULL);					// Get the current time
    info = localtime(&time24);					// convert to DST
    time24 = (((info->tm_hour * 60) + info->tm_min) * 60) + info->tm_sec; // and get 24hout time

    for( zone = 0; zone < NUM_ZONES; zone++) {			// check what zone and node we match
	node = match_node(node_name, zone);
	if (node != -1) break;
    }
    if(node <0) { goto EndError; }				// Ignore error - likely to come from Watch device
								// Valid Zone and node index

    if((time24 > network.on) && (time24 < network.off)) {	// Only handle CALL when Network control is ON

    network.zones[zone].nodes[node].temp = temp;		// Save the current temperature
    network.zones[zone].nodes[node].at_home = at_home;		// Save the current at home status

    this_time = check_any_signals();				// Check if anyone now CALLing
    if (last_time != this_time) {
	perform_logging();					// Log the fact
    }
    if (SIGNAL_AT_HOME(this_time) && !SIGNAL_AT_HOME(last_time)) { // if are now signalling at home
	debug(DEBUG_ESSENTIAL, "Bluetooth Candidates Now At Home - revise setpoints\n"); // Note
	network.at_home_delta = network.at_home_delta_out;	// revert to OUT mode
	add_timer(TIMER_CONTROL, 1);				// and trigger setpoint reassessment
    }
    last_time = this_time;					// Save signals Bit mask for next time
    } else {
	manage_SAT(node_name, temp, at_home);			// Treat this as a SAT
    }

ENDERROR;
}
//
//		Perform Setpoint control (MASTER)
//

void 	setpoint_control_process(){
    int		zone, node, change;
    struct 	payload_pkt app_data;
    int		network_id = -1;
    int		day = dayoftheweek();
    time_t	time24;
    struct profile 	*profile_ptr;;
    struct timeblock	*timeblock_ptr = NULL;
    float	proximity_delta;
    struct tm *info;

    if (app.active_node != -1) {				// As long as some node is active
	debug(DEBUG_INFO, "Setpoint Control, active nodes\n");

	if (network.fresh) {					// If new copy of configuration
	    proximity_delta = 0.0;				// override At Home adjustment this time
	    network.fresh = 0;
	} else {
	    proximity_delta = ( check_any_at_home() ? 0.0 :	network.at_home_delta ); // Set the profile deviation if not At Home
	}

	time24 = time(NULL);					// Get the current time
	info = localtime(&time24);					// convert to DST
	time24 = (((info->tm_hour * 60) + info->tm_min) * 60) + info->tm_sec; // and get 24hout time
	debug(DEBUG_DETAIL, "Profile time %ld (%2d:%2d)\n", time24, time24/(60*60), (time24/60)%60);

	for ( zone = 0; zone < NUM_ZONES; zone++) {		// Looking through each zone
	    profile_ptr = network.zones[zone].profiles[day];  	// get the current profile

	    debug(DEBUG_DETAIL, "Selecting Profile for Zone %s: %s\n", network.zones[zone].name, profile_ptr->name);

	    for(change = MAX_TIME_BLOCKS-1; change >= 0; change--) {// Seach through the timeblocks from top to bottom
		timeblock_ptr = &profile_ptr->blocks[change];
		if ((timeblock_ptr->setpoint != 0.0) &&		//Check Valid blocks until we reach
		    (timeblock_ptr->time <= time24)) {		// we find one that is currently active
		    debug(DEBUG_DETAIL, "Time block %d [%ld:%0.1f]\n", change, timeblock_ptr->time, timeblock_ptr->setpoint);
		    break;
		}
		timeblock_ptr = NULL;				// This timeblock not active;
	    }
	    //	We have the valid timeblock for this Zone
	    //  now apply it to each Zone
	    ERRORCHECK(timeblock_ptr == NULL, "No valid timeblock identified", EndError);

	    for ( node = 0; node <  NUM_NODES_IN_ZONE; node++) {	// and confihured nodes
		if (strcmp(network.zones[zone].nodes[node].name, "") != 0) { // with valid name
		    network_id = get_active_node(network.zones[zone].nodes[node].name); //find if it is currently active
		    if (network_id >= 0) {
			app_data.type = HEAT_SETPOINT;			// Contruct SETPOINT packet to be sent to slave
			app_data.d.setpoint.value  = timeblock_ptr->setpoint + network.zones[zone].nodes[node].delta - proximity_delta;
			app_data.d.setpoint.hysteresis = network.zones[zone].nodes[node].hysteresis;
			debug(DEBUG_TRACE, "Heat Setpoint packet %0.1f:%0.1f to %s\n", app_data.d.setpoint.value, app_data.d.setpoint.hysteresis, network.zones[zone].nodes[node].name);
			send_to_node(network_id, (char *) &app_data, SIZE_SETPOINT);
		    }
		}
	    }
	}
    }
ENDERROR;
}
