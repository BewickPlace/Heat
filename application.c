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

//
//	Callback on Link Up status change
//

void	notify_link_up(char *name) {
	app.active_node = find_active_node();				// record which node is active

	add_timer(TIMER_CONTROL, 16);					// trigger a review of control actions
    }

//
//	Callback on Link Down status change
//

void	notify_link_down(char *name) {
    app.active_node = find_active_node();				// record if any is active
    if (app.operating_mode == OPMODE_MASTER) { manage_SAT(name, 0.0, 0);// reset Master record of temp etc.
    } else { app.callsat = 0; }	  					// Maintain local callsat status
    }

//
//	Check heating setpoint (SLAVE)
//
void	check_heating_setpoint() {
    struct payload_pkt app_data;

    if (app.active_node != -1) {					// As long as we have a controller to talk to
	if ((app.temp  > (app.setpoint + app.boost + app.hysteresis)) &&
	    (app.operating_mode == OPMODE_SLAVE)) {
	    app.callsat = 0;						// Maintain local callsat status
	    app_data.type = HEAT_SAT;					// When temp above then SATisfied
	    app_data.d.callsat.temp  = app.temp;
	    app_data.d.callsat.setpoint = app.setpoint;
	    app_data.d.callsat.hysteresis = app.hysteresis;
	    app_data.d.callsat.boost = app.boost;
	    app_data.d.callsat.at_home = app.at_home;
	    debug(DEBUG_INFO, "Heat SATisfied %0.1f:%.01f:%0.1f:%d\n", app.temp, app.setpoint, app.hysteresis, app.boost);
	    send_to_node(app.active_node, (char *) &app_data, SIZE_SAT);

	} else if ((app.temp < (app.setpoint + app.boost - app.hysteresis)) &&
	    (app.operating_mode == OPMODE_SLAVE)) {
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
//	Midnight processing
//
void	midnight_processing() {
    char 	logfile[50];			// Log file
    time_t	seconds;
    struct tm	*info;
    int		day, month, year;
    int		i;

    if (app.operating_mode != OPMODE_MASTER) return;	// Only applicable on MASTER

    seconds = time(NULL);				// get the time
    info = localtime(&seconds);				// convert into strctured time

    if ((info->tm_hour == 23) && (info->tm_min >= 55)) {	// if Last log interval before Midnight
	debug(DEBUG_TRACE, "Pre-Midnight precessing....\n");

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

    reset_run_clock();					// reset CALL run timer
    perform_logging();					// force an initial log
ENDERROR;
}

//
//	Perform Temperature Logging (SLAVE)
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

    if (app.operating_mode == OPMODE_MASTER) {		// Logfile format for MASTER or SLAVE
	if (exists == 0) { fprintf(log, "Time, Run Clock, Zone 1, Zone 2, At Home\n"); }
	fprintf(log, "%02d:%02d,%02ld:%02ld, %d, %d, %d\n", info->tm_hour, info->tm_min, get_run_clock()/3600, (get_run_clock()/60) % 60, check_any_CALL_in_zone(0), check_any_CALL_in_zone(1), check_any_at_home());

    } else {
	if (exists == 0) { fprintf(log, "Time,Temperture,Septpoint,Boost,Hysteresis\n"); }
	fprintf(log, "%02d:%02d, %0.1f, %0.1f, %d, %0.1f\n", info->tm_hour, info->tm_min, app.temp, app.setpoint, app.boost, app.hysteresis);
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
	debug(DEBUG_ESSENTIAL, "Payload Received from %s of type %d %s, len %d\n", node_name, payload->type, payload->d.data, payload_len);
	break;

    case HEAT_SETPOINT:
	debug(DEBUG_TRACE, "Heat @ %s Setpoint %0.1f:%0.1f\n", node_name, payload->d.setpoint.value, payload->d.setpoint.hysteresis);
	if ((app.boost) &&					// If Boost ON
	    (payload->d.setpoint.value > app.setpoint)) {	// and setpoint being raised
	    boost_stop();					// Cancel the boost
	}
	app.setpoint = payload->d.setpoint.value;		// Set new values
	app.hysteresis = payload->d.setpoint.hysteresis;	// to be picked up at next check
	break;

    case HEAT_CANDIDATES:
	debug(DEBUG_INFO, "Heat Bluetooth Candidates received\n");
 	manage_candidates(payload->d.candidates);
	break;

    case HEAT_CALL:
	debug(DEBUG_INFO, "CALL from %s for heat %.01f:%.01f:%0.1f:%d\n", node_name, payload->d.callsat.temp, payload->d.callsat.setpoint, payload->d.callsat.hysteresis, payload->d.callsat.boost);

	manage_CALL(node_name, payload->d.callsat.temp, payload->d.callsat.at_home);
	break;

    case HEAT_SAT:
	debug(DEBUG_INFO, "Heat @ %s SATisfied %.01f:%0.1f:%0.1f:%d\n", node_name, payload->d.callsat.temp, payload->d.callsat.setpoint, payload->d.callsat.hysteresis, payload->d.callsat.boost);
	manage_SAT(node_name, payload->d.callsat.temp, payload->d.callsat.at_home);
	break;

    case HEAT_TEMP:
	debug(DEBUG_INFO, "Heat @ %s TEMPerature %.01f:%0.1f:%0.1f:%d\n", node_name, payload->d.callsat.temp, payload->d.callsat.setpoint, payload->d.callsat.hysteresis, payload->d.callsat.boost);
	manage_TEMP(node_name, payload->d.callsat.temp, payload->d.callsat.at_home);
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
	    load_configuration_data();
	    advise_bluetooth_candidates();
	    setpoint_control_process();
	} else {
		// no actions yet"
	}
	add_timer(TIMER_CONTROL, timeto15min()); // wait for the next 15 minute boundry
	break;

    case TIMER_SETPOINT:
        debug(DEBUG_INFO, "Handle Setpoint timeout\n");
	check_heating_setpoint();	// Go check temperature against setpoint
	add_timer(TIMER_SETPOINT, 15);	// wait for another go" in y seconds
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
        debug(DEBUG_TRACE, "Handle Logging timeout\n");

	midnight_processing();		// Perform Midnight processing if appropriate

	if (app.operating_mode != OPMODE_MASTER) perform_logging(); // Perform temperature logging (SLAVE)
	add_timer(TIMER_LOGGING, timeto5min());// wait for next 5 minute boundty
	break;

    default:
	debug(DEBUG_ESSENTIAL, "Unexpected App timeout (%d)\n", timer);

	break;
    }

}
