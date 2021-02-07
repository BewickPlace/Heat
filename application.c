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

//#include <unistd.h>
//#include <signal.h>
//#include <assert.h>
//#include <fcntl.h>
#include <sys/time.h>
#include <time.h>
//#include <sys/types.h>
//#include <sys/socket.h>
//#include <sys/uio.h>
//#include <netinet/in.h>
//#include <netinet/ip.h>
//#include <arpa/inet.h>
//#include <net/if.h>

#include "errorcheck.h"
#include "timers.h"
#include "networks.h"
#include "heat.h"
#include "application.h"
#include "dht11.h"

void	perform_daily_log();
int	get_network_mask();

//
//	Callback on Link Up status change
//

void	notify_link_up(char *name) {
	app.active_node = find_active_node();				// record which node is active

	add_timer(TIMER_CONTROL, 12);					// trigger a review of control actions
	if (app.operating_mode == OPMODE_MASTER) {
	    advise_node_bluetooth_candidates(name);			// Advise of the latest Bluetooth Candidates
	    perform_logging(); 						// Maintain network status
	}
    }

//
//	Callback on Link Down status change
//

void	notify_link_down(char *name) {
    app.active_node = find_active_node();				// record if any is active
    if (app.operating_mode == OPMODE_MASTER) { manage_SAT(name, -0.1, 0, 0.0, 0);// reset Master record of temp etc.
    } else { app.callsat = 0; }	  					// Maintain local callsat status
    if (app.operating_mode == OPMODE_MASTER) { perform_logging(); }	// Maintain network status
    }

//
//	Send SAT
//
void	send_SAT() {
    struct payload_pkt app_data;

    if ((app.active_node != -1) &&					// Active MASTER exists
	(app.operating_mode == OPMODE_WATCH) &&				// if WATCH and we were calling
	(app.callsat == 1)) {						// force SAT message
	app.callsat = 0;						// Maintain local callsat status
	app_data.type = HEAT_SAT;					// When temp above then SATisfied
	app_data.d.callsat.temp  = app.temp;
	app_data.d.callsat.setpoint = app.setpoint;
	app_data.d.callsat.hysteresis = app.hysteresis;
	app_data.d.callsat.boost = app.boost;
	app_data.d.callsat.at_home = app.at_home;
	debug(DEBUG_INFO, "Heat SATisfied %0.1f:%.01f:%0.1f:%d\n", app.temp, app.setpoint, app.hysteresis, app.boost);
	send_to_node(app.active_node, (char *) &app_data, SIZE_SAT);
    }
}

//
//	Check heating setpoint (SLAVE)
//
void	check_heating_setpoint() {
    struct payload_pkt app_data;

    if (app.active_node != -1) {					// As long as we have a controller to talk to
	if (((app.operating_mode == OPMODE_SLAVE) &&
	     (app.temp  >= (app.setpoint + app.boost + app.hysteresis)))// SLAVE - include Boost in calculation
		||
	    ((app.operating_mode == OPMODE_WATCH) && (app.boost) &&	// WATCH - require Boost to activate
	     (app.temp  >= (app.setpoint + app.hysteresis)))
		||
	    ((app.operating_mode == OPMODE_HOTWATER) &&			// HOT WATER
	     (app.temp >= (app.setpoint + app.boost)))
	   ) {
	    if (app.operating_mode == OPMODE_HOTWATER) { callsat(0, 0); }

	    app.callsat = 0;						// Maintain local callsat status
	    app_data.type = HEAT_SAT;					// When temp above then SATisfied
	    app_data.d.callsat.temp  = app.temp;
	    app_data.d.callsat.setpoint = app.setpoint;
	    app_data.d.callsat.hysteresis = app.hysteresis;
	    app_data.d.callsat.boost = app.boost;
	    app_data.d.callsat.at_home = app.at_home;
	    debug(DEBUG_INFO, "Heat SATisfied %0.1f:%.01f:%0.1f:%d\n", app.temp, app.setpoint, app.hysteresis, app.boost);
	    send_to_node(app.active_node, (char *) &app_data, SIZE_SAT);

	} else if (
	     ((app.operating_mode == OPMODE_SLAVE)  &&
	      (app.temp <= (app.setpoint + app.boost - app.hysteresis))) // SLAVE - include Boost in calculation
		||
	      ((app.operating_mode == OPMODE_WATCH)  && (app.boost) &&    // WATCH - require Boost to activate
	       (app.temp <= (app.setpoint - app.hysteresis)))
		||
	      ((app.operating_mode == OPMODE_HOTWATER) &&		// HOT WATER
	       (app.temp <  (app.setpoint + app.boost)))
	   ) {
	    if (app.operating_mode == OPMODE_HOTWATER) { callsat(0, 1); }

	    app.callsat = 1;						// Maintain local callsat status
	    app_data.type = HEAT_CALL;					// When temp below then CALL for heat
	    app_data.d.callsat.temp  = app.temp;
	    app_data.d.callsat.setpoint = app.setpoint;
	    app_data.d.callsat.hysteresis = app.hysteresis;
	    app_data.d.callsat.boost = app.boost;
	    app_data.d.callsat.at_home = app.at_home;
	    debug(DEBUG_INFO, "CALL for %0.1f:%.01f:%0.1f:%d\n", app.temp, app.setpoint, app.hysteresis, app.boost);
	    send_to_node(app.active_node, (char *) &app_data, SIZE_CALL);

	} else {
	    app_data.type = HEAT_TEMP;					// Otherwise just advise of current temp
	    app_data.d.callsat.temp  = app.temp;
	    app_data.d.callsat.setpoint = app.setpoint;
	    app_data.d.callsat.hysteresis = app.hysteresis;
	    app_data.d.callsat.boost = app.boost;
	    app_data.d.callsat.at_home = app.at_home;
	    debug(DEBUG_INFO, "TEMP of %0.1f:%.01f:%0.1f:%d\n", app.temp, app.setpoint, app.hysteresis, app.boost);
	    send_to_node(app.active_node, (char *) &app_data, SIZE_TEMP);
	}
    }
}

//
//	Midnight processing (MASTER or SLAVE)
//
void	midnight_processing() {
    char 	logfile[50];			// Log file
    time_t	seconds;
    struct tm	*info;
    int		day, month, year;
    int		i;

    seconds = time(NULL);				// get the time
    info = localtime(&seconds);				// convert into strctured time

    if ((info->tm_hour == 23) && (info->tm_min >= 55) &&// if Last log interval before Midnight
        (app.operating_mode == OPMODE_MASTER)) {	// Only applicable on MASTER
	debug(DEBUG_TRACE, "Pre-Midnight precessing....\n");

	perform_daily_log();				// Record key daily summary
	perform_logging();				// force a final log
    }

    if (!((info->tm_hour == 0) && (info->tm_min == 0))) return;	// Exit if not Midnight
    debug(DEBUG_TRACE, "Midnight precessing....\n");

    // Delete old Track files - beyond 30 days, 7 day window

    for (i = 0 ; i < 7; i++) {
	day = ((info->tm_mday - 1 + 31 - i) % 31) + 1;
	month = ((info->tm_mon - 1 + 12 - (day > info->tm_mday)) % 12) + 1;
	year = (info->tm_year  - (month > info->tm_mon));
	sprintf(logfile,"%s%s_%04d-%02d-%02d.csv", app.trackdir, my_name(), year + 1900, month, day);
	ERRORCHECK( strlen(logfile) > sizeof(logfile), "Tracking filename too long", EndError);

	debug(DEBUG_TRACE, "Delete logfile %s (%d/%d)\n", logfile, remove(logfile), errno);
    }

    if (app.operating_mode == OPMODE_MASTER) {		// Only applicable on MASTER
	if (!check_any_at_home()) {			// If nobody at home
	    network.at_home_delta = network.at_home_delta_away; // switch to AWAY mode
	    debug(DEBUG_ESSENTIAL, "Bluetooth Candidates Now AWAY\n");
	}

	reset_run_clock();				// reset CALL run timer
	perform_logging();				// force an initial log
    }
ENDERROR;
}

//
//	Load Run Clock
//
void	load_run_clock() {
    char 	logfile[50];			// Log file
    time_t	seconds;
    struct tm	*info;
    FILE	*log;
    int		hotwater, zone, call, home, network;
    time_t	hours, minutes;

    if ((app.operating_mode != OPMODE_MASTER) ||	// On MASTER only
        (app.trackdir == NULL)) { goto EndError; }	// Do NOT log is directory not specified
    seconds = time(NULL);				// get the time
    info = localtime(&seconds);				// convert into strctured time
    sprintf(logfile,"%s%s_%04d-%02d-%02d.csv", app.trackdir, my_name(), info->tm_year + 1900, info->tm_mon + 1, info->tm_mday);
    ERRORCHECK( strlen(logfile) > sizeof(logfile), "Tracking filename too long", TrackError);

    log = fopen(logfile, "r");				// Open  the file
    if (log != NULL) {					// if file exists

	fscanf(log, "%*[^\n]\n");			// Skip header
	while (fscanf(log, "%02d:%02d,%02ld:%02ld, %d, %d, %d, %d, %d\n", &info->tm_hour, &info->tm_min,
						&hours, &minutes, &hotwater, &zone, &call, &home, &network) >0) {
	}
	app.run_time = (hours * 3600) + (minutes *60);	// update Run Time
	debug(DEBUG_ESSENTIAL, "Run clock updated, %02ld:%02ld\n", hours, minutes);
	fclose(log);
    }

ERRORBLOCK(TrackError);
    debug(DEBUG_ESSENTIAL, "Size: %d:%d %s\n", strlen(logfile), sizeof(logfile), logfile);
ENDERROR;
}
//
//	Perform Daily Logging
//
void	perform_daily_log() {
    char 	logfile[50];			// Log file
    time_t	seconds;
    struct tm	*info;
    FILE	*log;
    int	exists = 0;

    if ((app.operating_mode != OPMODE_MASTER) ||	// On MASTER only
        (app.trackdir == NULL)) { goto EndError; }	// Do NOT log is directory not specified

    seconds = time(NULL);				// get the time
    info = localtime(&seconds);				// convert into strctured time
    sprintf(logfile,"%s%s_%04d.csv", app.trackdir, my_name(), info->tm_year + 1900);
    ERRORCHECK( strlen(logfile) > sizeof(logfile), "Tracking filename too long", TrackError);

    log = fopen(logfile, "r");			// Open  the file
    if (log != NULL) {				// if file exists
	exists = 1;
	fclose(log);
    }
    log = fopen(logfile, "a"); 				// open Tracking file for Append
    ERRORCHECK( (log == NULL) , "Error opening Tracking file", OpenError);

    if (exists == 0) { fprintf(log, "Date, Run Clock\n"); }
    fprintf(log, "%02d-%02d,%02ld:%02ld\n", info->tm_mday, (info->tm_mon+1),
							get_run_clock()/3600, (get_run_clock()/60) % 60);
    fclose(log);
ERRORBLOCK(TrackError);
    debug(DEBUG_ESSENTIAL, "Size: %d:%d %s\n", strlen(logfile), sizeof(logfile), logfile);

ERRORBLOCK(OpenError);
    debug(DEBUG_ESSENTIAL, "Logfile %s Append errno: %d\n2", logfile, errno);
ENDERROR;
}
//
//	Perform Temperature Logging
//
void	perform_logging() {
    char 	logfile[50];			// Log file
    time_t	seconds;
    struct tm	*info;
    FILE	*log;
    int	exists = 0;

    if (app.trackdir == NULL) { goto EndError; }	// Do NOT log is directory not specified
    seconds = time(NULL);				// get the time
    info = localtime(&seconds);				// convert into strctured time
    sprintf(logfile,"%s%s_%04d-%02d-%02d.csv", app.trackdir, my_name(), info->tm_year + 1900, info->tm_mon + 1, info->tm_mday);
    ERRORCHECK( strlen(logfile) > sizeof(logfile), "Tracking filename too long", TrackError);

    log = fopen(logfile, "r");				// Open  the file
    if (log != NULL) {					// if file exists
	exists = 1;
	fclose(log);
    }
    log = fopen(logfile, "a"); 				// open Tracking file for Append
    ERRORCHECK( (log == NULL) , "Error opening Tracking file", OpenError);

    if (app.operating_mode == OPMODE_MASTER) {		// Logfile format for MASTER or SLAVE/WATCH
	if (exists == 0) { fprintf(log, "Time, Run Clock, Hot Water, Zone 1, Zone 2, At Home, Network\n"); }
	fprintf(log, "%02d:%02d,%02ld:%02ld, %d, %d, %d, %d, %d\n", info->tm_hour, info->tm_min,
		get_run_clock()/3600, (get_run_clock()/60) % 60,
		check_any_CALL_in_zone(0),
		check_any_CALL_in_zone(1), check_any_CALL_in_zone(2), check_any_at_home(),
		get_network_mask());

    } else {
	if (exists == 0) { fprintf(log, "Time,Temperture,Septpoint,Boost,Hysteresis\n"); }

	if (app.operating_mode == OPMODE_SLAVE) {	// SLAVE format
	    fprintf(log, "%02d:%02d, %0.1f, %0.1f, %d, %0.1f\n", info->tm_hour, info->tm_min, app.temp, app.setpoint, app.boost, app.hysteresis);
	} else {					// WATCH format
	    fprintf(log, "%02d:%02d, %0.1f, %0.1f, %0.1f, %0.1f\n", info->tm_hour, info->tm_min, app.temp, app.setpoint, ((float)app.boost) /10.0, app.hysteresis);
	}
    }
    fclose(log);
ERRORBLOCK(TrackError);
    debug(DEBUG_ESSENTIAL, "Size: %d:%d %s\n", strlen(logfile), sizeof(logfile), logfile);

ERRORBLOCK(OpenError);
    debug(DEBUG_ESSENTIAL, "Logfile %s Append errno: %d\n2", logfile, errno);
ENDERROR;
}

//
//	Handle incomming Application messages
//

void	handle_app_msg(char *node_name, struct payload_pkt *payload, int payload_len) {

    if( payload_len == 0) { return; }		// skip out if nothing to do
    switch(payload-> type) {
    case PAYLOAD_TYPE:
	debug(DEBUG_TRACE, "Payload Received from %s of type %d %s, len %d\n", node_name, payload->type, payload->d.data, payload_len);
	break;

    case HEAT_SETPOINT:
	debug(DEBUG_TRACE, "Heat @ %s Setpoint %0.1f:%0.1f\n", node_name, payload->d.setpoint.value, payload->d.setpoint.hysteresis);
	if ((app.boost) &&					// If Boost ON
	    ((app.operating_mode == OPMODE_SLAVE) ||		// on a Slave node
	    (app.operating_mode == OPMODE_HOTWATER)) &&		// or HOTWATER node
	    (payload->d.setpoint.value > app.setpoint)) {	// and setpoint being raised
	    boost_stop();					// Cancel the boost
	}
	if ((app.boost>1) &&					// If Boost ON Fix mode
	    (app.operating_mode == OPMODE_WATCH) &&		// on a Watching node
	    (payload->d.setpoint.value < app.setpoint)) {	// and setpoint target is to reduce
								// ignore change whilst boost is active
	} else {
	    app.setpoint = payload->d.setpoint.value;		// Set new values
	    app.hysteresis = payload->d.setpoint.hysteresis;	// to be picked up at next check
	}
	break;

    case HEAT_CANDIDATES:
	debug(DEBUG_INFO, "Heat Bluetooth Candidates received\n");
 	manage_candidates(payload->d.candidates);
	break;

    case HEAT_CALL:
	debug(DEBUG_INFO, "CALL from %s for heat %.01f:%.01f:%0.1f:%d\n", node_name, payload->d.callsat.temp, payload->d.callsat.setpoint, payload->d.callsat.hysteresis, payload->d.callsat.boost);

	manage_CALL(node_name, payload->d.callsat.temp, payload->d.callsat.at_home, payload->d.callsat.setpoint, payload->d.callsat.boost);
	break;

    case HEAT_SAT:
	debug(DEBUG_INFO, "Heat @ %s SATisfied %.01f:%0.1f:%0.1f:%d\n", node_name, payload->d.callsat.temp, payload->d.callsat.setpoint, payload->d.callsat.hysteresis, payload->d.callsat.boost);
	manage_SAT(node_name, payload->d.callsat.temp, payload->d.callsat.at_home, payload->d.callsat.setpoint, payload->d.callsat.boost);
	break;

    case HEAT_TEMP:
	debug(DEBUG_INFO, "Heat @ %s TEMPerature %.01f:%0.1f:%0.1f:%d\n", node_name, payload->d.callsat.temp, payload->d.callsat.setpoint, payload->d.callsat.hysteresis, payload->d.callsat.boost);
	manage_TEMP(node_name, payload->d.callsat.temp, payload->d.callsat.at_home, payload->d.callsat.setpoint, payload->d.callsat.boost);
	break;

    default:
	debug(DEBUG_ESSENTIAL, "Unexpected App Msg (%d) from node: %s\n", payload->type, node_name);
	break;

    }
}

//
//	Handle application timer expiries
//

void	handle_app_timer(int timer) {

    switch (timer) {			// Check application based timers
    case NO_TIMERS:
	break;				// No timer, just skip through

    case TIMER_APPLICATION:
        debug(DEBUG_ESSENTIAL, "Handle App timeout\n");
	break;

    case TIMER_CONTROL:
        debug(DEBUG_INFO, "Handle Control timeout\n");

	if (app.operating_mode == OPMODE_MASTER) {
	    if (load_configuration_data()) {		// Check if new configuration available
		advise_bluetooth_candidates();		// Advise candidates in case of change
	    }
	    setpoint_control_process();
	} else {
		// no actions yet"
	}
	add_timer(TIMER_CONTROL, AT_CONTROL); // wait for the next 15 minute boundry
	break;

    case TIMER_SETPOINT:
        debug(DEBUG_INFO, "Handle Setpoint timeout\n");
	check_heating_setpoint();	// Go check temperature against setpoint
	add_timer(TIMER_SETPOINT, AT_SETPOINT); // wait for another go" in y seconds
	break;

    case TIMER_BOOST:			// Noost Timeout
        debug(DEBUG_INFO, "Handle Boost timeout\n");

	boost_stop();			// Stop the boost
	break;

    case TIMER_DISPLAY:			// Noost Timeout
        debug(DEBUG_INFO, "Handle Screen  timeout\n");

	app.display = 0;		//  Turn display off at next screen refresh
	break;

    case TIMER_LOGGING:
        debug(DEBUG_DETAIL, "Handle Logging timeout\n");

	midnight_processing();		// Perform Midnight processing if appropriate

	if (app.operating_mode != OPMODE_MASTER) perform_logging(); // Perform temperature logging (SLAVE)
	add_timer(TIMER_LOGGING, timeto5min()+30);// wait for next 5 minute boundty - log @ 30 seconds
	break;

    default:
	debug(DEBUG_ESSENTIAL, "Unexpected App timeout (%d)\n", timer);

	break;
    }

}

//
//	Get Network Mask
//

int	get_network_mask() {
    int mask, result;;
    int zone, node;
    char *node_name;

    mask = 0;							// Initialise mask
    result = 0;

    for (zone = 0; zone < NUM_ZONES; zone++) {			// Check each Zone
	mask = 1 << (zone * 4);					// Allow 4 bits per Zone

	for (node = 0; node < NUM_NODES_IN_ZONE; node++) {	// and all Nodes in Zone
	    node_name = network.zones[zone].nodes[node].name;
	    if ((*node_name != 0) &&				// If node exists and is currently UP
	       (get_active_node(node_name) > -1)) { result = result + mask; }  // Add to result mask
	    mask = mask << 1;					// Move mask on
	}
    }
    return(result);
}

