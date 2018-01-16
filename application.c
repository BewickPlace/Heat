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
//#include <sys/time.h>
//#include <time.h>
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
#include "application.h"
#include "dht11.h"
#include "heat.h"

//
//	Callback on Link Up status change
//

void	notify_link_up() {
	app.active_node = find_active_node();				// record which node is active

	add_timer(TIMER_CONTROL, 1);					// trigger a review of control actions
    }

//
//	Callback on Link Down status change
//

void	notify_link_down() {
    app.active_node = find_active_node();				// record if any is active
    }

//
//	Check heating setpoint
//
void	check_heating_setpoint() {
    struct payload_pkt app_data;

    if (app.active_node != -1) {					// As long as we have a controller to talk to
	if (app.temp  > (app.setpoint + app.boost + app.hysteresis)) {
	    app_data.type = HEAT_SAT;					// When temp above then SATisfied
	    app_data.d.callsat.temp  = app.temp;
	    app_data.d.callsat.setpoint = app.setpoint;
	    app_data.d.callsat.hysteresis = app.hysteresis;
	    app_data.d.callsat.boost = app.boost;
	    debug(DEBUG_INFO, "Heat SATisfied %0.1f:%.01f:%0.1f:%d\n", app.temp, app.setpoint, app.hysteresis, app.boost);
	    send_to_node(app.active_node, (char *) &app_data, sizeof(app_data));

	} else if (app.temp < (app.setpoint + app.boost - app.hysteresis)) {
	    app_data.type = HEAT_CALL;					// When temp below then CALL for heat
	    app_data.d.callsat.temp  = app.temp;
	    app_data.d.callsat.setpoint = app.setpoint;
	    app_data.d.callsat.hysteresis = app.hysteresis;
	    app_data.d.callsat.boost = app.boost;
	    debug(DEBUG_INFO, "CALL for %0.1f:%.01f:%0.1f:%d\n", app.temp, app.setpoint, app.hysteresis, app.boost);
	    send_to_node(app.active_node, (char *) &app_data, sizeof(app_data));
	}
    }
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
	app.setpoint = payload->d.callsat.setpoint;		// Set new values
	app.hysteresis = payload->d.callsat.hysteresis;		// to be picked up at next check
	break;

    case HEAT_CALL:
	debug(DEBUG_INFO, "CALL from %s for heat %.01f:%.01f:%0.1f:%d\n", node_name, payload->d.callsat.temp, payload->d.callsat.setpoint, payload->d.callsat.hysteresis, payload->d.callsat.boost);

	manage_CALL(node_name);
	break;

    case HEAT_SAT:
	debug(DEBUG_INFO, "Heat @ %s SATisfied %.01f:%0.1f:%0.1f:%d\n", node_name, payload->d.callsat.temp, payload->d.callsat.setpoint, payload->d.callsat.hysteresis, payload->d.callsat.boost);
	manage_SAT(node_name);
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

    default:
	debug(DEBUG_ESSENTIAL, "Unexpected App timeout (%d)\n", timer);

	break;
    }

}
