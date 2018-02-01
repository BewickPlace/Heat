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

//
//		Manage Call for heat (MASTER)
//

void 	manage_CALL(char *node_name) {
    int	zone;
    int node;

    for( zone = 0; zone < NUM_ZONES; zone++) {			// check what zone and node we match
	node = match_node(node_name, zone);
	if (node != -1) break;
    }
    ERRORCHECK(node < 0, "Live node mismatch with configuration", NodeError);
								// Valid Zone and node index
    if (network.zones[zone].nodes[node].callsat) { goto EndError; } // if already in CALL skip to end

    network.zones[zone].nodes[node].callsat = 1;			// Mark as CALLing
    debug(DEBUG_ESSENTIAL, "Call for heat @ %s (%d:%d)\n",node_name, zone, node);

    callsat(zone, 1);					    	// interface with DHT11 module to action

ERRORBLOCK(NodeError);
   debug(DEBUG_ESSENTIAL, "Mismatch %s\n", node_name);
ENDERROR;
}
//
//		Manage SAT heat (MASTER)
//

void 	manage_SAT(char *node_name) {
    int	zone;
    int node;
    int i;

    for( zone = 0; zone < NUM_ZONES; zone++) {			// check what zone and node we match
	node = match_node(node_name, zone);
	if (node != -1) break;
    }
    ERRORCHECK(node < 0, "Live node mismatch with configuration", NodeError);
								// Valid Zone and node index
    if (!network.zones[zone].nodes[node].callsat) { goto EndError; } // if already SAT skip to end
    network.zones[zone].nodes[node].callsat = 0;		// Mark as SATisfied

    for(i = 0; i < NUM_NODES_IN_ZONE; i++) {			// Check if Zone fully satisfied
	if (network.zones[zone].nodes[i].callsat) { goto EndError; } // Skip if CALL still required anywhere in zone
    }
    debug(DEBUG_ESSENTIAL, "Heat SATisfied @ %s (%d:%d)\n",node_name, zone, node);

    callsat(zone, 0);					    	// interface with DHT11 module to action

ERRORBLOCK(NodeError);
   debug(DEBUG_ESSENTIAL, "Mismatch %s\n", node_name);
ENDERROR;
}
//
//		Perform Setpoint control (MASTER)
//
#define	HOURS24	(24*60*60)

void 	setpoint_control_process(){
    int		zone, node, change;
    struct 	payload_pkt app_data;
    int		network_id = -1;
    int		day = dayoftheweek();
    time_t	time24;
    struct profile 	*profile_ptr;;
    struct timeblock	*timeblock_ptr = NULL;

    if (app.active_node != -1) {				// As long as some node is active
	debug(DEBUG_INFO, "Setpoint Control, active nodes\n");

	time24 = time(NULL);					// Get the current time
	time24 = time24 % HOURS24;				// and get 24hour time
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
			app_data.d.setpoint.value  = timeblock_ptr->setpoint;
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
