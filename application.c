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
    }

//
//	Callback on Link Down status change
//

void	notify_link_down() {
    app.active_node = find_active_node();				// record if any is active
    }

//
//	Handle incomming Application messages
//

void	handle_app_msg(struct payload_pkt *payload, int payload_len) {

    if( payload_len == 0) { return; }		// skip out if nothing to do

    debug(DEBUG_ESSENTIAL, "Payload Received of type %d %s, len %d\n", payload->type, payload->d.data, payload_len);
}

//
//	Handle application timer expiries
//

void	handle_app_timer(int timer) {
    struct payload_pkt app_data = {PAYLOAD_TYPE,{"ABCDEFGHIJK\0"} };

    switch (timer) {			// Check application based timers
    case NO_TIMERS:
	break;				// No timer, just skip through


    case TIMER_APPLICATION:
        debug(DEBUG_ESSENTIAL, "Handle App timeout\n");
	send_to_node(app.active_node, (char *) &app_data, sizeof(app_data));

	break;

    case TIMER_BOOST:			// Noost Timeout
        debug(DEBUG_ESSENTIAL, "Handle Boost timeout\n");

	boost_stop();			// Stop the boost
	break;

    case TIMER_DISPLAY:			// Noost Timeout
        debug(DEBUG_ESSENTIAL, "Handle Screen  timeout\n");

	app.display = 0;		//  Turn display off at next screen refresh
	break;

    default:
	debug(DEBUG_ESSENTIAL, "Unexpected App timeout (%d)\n", timer);

	break;
    }

}
