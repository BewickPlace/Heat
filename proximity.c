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
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <errno.h>

#include <wiringPi.h>
#include "errorcheck.h"
//#include "timers.h"
#include "networks.h"
#include "heat.h"
#include "application.h"

struct bluetooth bluetooth;			// Bluetooth candidate data
static int	bluetooth_dev_id;		// Devide Id
static int	bluetooth_sock;			// Bluetooth socket

#define BLUE_NAME	40			// Name length
#define BLUE_ADDRESS	19			// Address length

#define OVERALL_TIMER	(60*60)			// Overall cycle of actions
#define IDENTIFY_TIMER	(60*60)			// Identify potential candidates
#define MAINT_TIMER	(60*5)			// Maintain visibility checks
#define VISIBLE_PERIOD	7			// Num of maintenance period
#define VISIBLE_CHECK	4			// How often to check

void	display_candidates();

//
//	Check Bluetooth name
//
int check_bluetooth_name(bdaddr_t bdaddr, char *name, char *addr) {
    int rc;

    delay(3000);
    if (heat_shutdown) return(-1);
    debug(DEBUG_INFO, "Ckecking ");
    rc = hci_read_remote_name(bluetooth_sock, &bdaddr, BLUE_NAME, name, 1500);
    if (rc < 0) strcpy(name, "[unknown]");
    ba2str(&bdaddr, addr);
    debug(DEBUG_INFO, "%s ... %s\n", addr, name);
    return(rc);
}

//
//	Check Candidate List
//
int	check_candidate_list(struct proximity_block list[], bdaddr_t *item) {
    int	i;

    for( i=0; i < BLUETOOTH_CANDIDATES; i++) {	// Search candidate list
	if (bacmp(&list[i].bdaddr, item) == 0) break;
    }
return(i < BLUETOOTH_CANDIDATES? i : -1);
}

#define	HCI_LEN	8
#define HCI_RSP 10
//
//	identify_possible_candidates
//
void	identify_possible_candidates(int timer) {
    int		responses = 0;
    inquiry_info info[HCI_RSP];
    inquiry_info *pinfo = info;
    int	i, rc;
    char	name[BLUE_NAME];
    char	addr[BLUE_ADDRESS];

    if (heat_shutdown) return;
    if ((timer % IDENTIFY_TIMER)==0) {
	debug(DEBUG_TRACE, "Bluetooth At Home status: %d (%d)\n", app.at_home, timer);

	debug(DEBUG_INFO, "Inquiry\n");
	// Obtain the list of visible Bluetooth Devices
	responses = hci_inquiry(bluetooth_dev_id, HCI_LEN, HCI_RSP, NULL, &pinfo, IREQ_CACHE_FLUSH);
	ERRORCHECK( responses < 0, "Bluetooth Inquiry error", BlueError);
	debug(DEBUG_INFO, "Inquiry complete\n");

 	// Check if we can name them
	for (i = 0; i < responses; i++) {
	    if (heat_shutdown) return;
	    rc = check_candidate_list(bluetooth.candidates, &info[i].bdaddr); 	// check if already present as full candidate
	    if (rc != -1) goto next_candidate;
	    rc = check_candidate_list(bluetooth.possible_candidates, &info[i].bdaddr); // check if already present
	    if (rc != -1) goto next_candidate;

	    rc = check_bluetooth_name(info[i].bdaddr, name, addr);		//  Get candidates name over bluetooth 
	    if (rc < 0) goto next_candidate;					// if known (named) address

	    rc = check_candidate_list(bluetooth.possible_candidates, BDADDR_ANY); // check for free slot
	    if (rc < 0) goto next_candidate;					// add new candidate
	    bacpy(&bluetooth.possible_candidates[rc].bdaddr, &info[i].bdaddr);
	    debug(DEBUG_ESSENTIAL, "Bluetooth potential candidate %s %s\n", name, addr);
	    bluetooth.possible_candidates[rc].timer = VISIBLE_PERIOD;		//  Set countdown timer
next_candidate:;
	}
    }
ERRORBLOCK(BlueError);
    debug(DEBUG_ESSENTIAL, "Bluetooth error %d, errno %d\n", responses, errno);
ENDERROR;
	}
//
//	Maintain Candidates
//
void	maintain_candidates(int timer, struct proximity_block list[], int del) {
    int i, rc;
    char	name[BLUE_NAME];
    char	addr[BLUE_ADDRESS];
    int		found =0;

    if (heat_shutdown) return;

    if ((timer % MAINT_TIMER)==0) {

    if (!del) debug(DEBUG_TRACE, "Bluetooth Maintenance: %d (%d)\n", app.at_home, timer);

    for (i=0; i < BLUETOOTH_CANDIDATES; i++) {				// check all valid (non-zero) candidates
	if (heat_shutdown) return;
	if (bacmp(&list[i].bdaddr, BDADDR_ANY) != 0) {
	    if (((list[i].timer % VISIBLE_CHECK) == 0) |		// every x  period check is device visible
		(list[i].timer < VISIBLE_CHECK)) {			// or every interval if mising!
		rc = check_bluetooth_name(list[i].bdaddr, name, addr);	// look for the device
	    	if (rc > -1) {						// if identified ...
		    if (list[i].timer < 0) debug(DEBUG_ESSENTIAL, "Bluetooth At Home: %s %s\n", name, addr);
		    list[i].timer = VISIBLE_PERIOD+1;			// ...reset visibility timer
		}
	    }
	    if (list[i].timer > 0) list[i].timer--;			// Decrement visibity timer
	}
	if ((list[i].timer == 0) && 					// if timer has expired 
	    (bacmp(&list[i].bdaddr, BDADDR_ANY) != 0)) {
	    ba2str(&list[i].bdaddr, addr);
	    if (del) {							// Delete from potential list
		debug(DEBUG_ESSENTIAL, "Bluetooth visibility lost for:  %s\n", addr);
		bacpy(&list[i].bdaddr, BDADDR_ANY);			// Mark address invalid for entry
	    } else {
		debug(DEBUG_ESSENTIAL, "Bluetooth no longer At Home:  %s\n", addr);
		list[i].timer = -1;
	    }
	}
	if (list[i].timer > 0) found= 1;				// Mark Visible device still in list
    }
    if (del) display_candidates();
    if (!del) app.at_home = found;					// Update applicatio at home status
    }
}

//
//	Bluetooth Proximity Process
//

void proximity_process()	{
    int	cycle_timer = (MAINT_TIMER*4)/5;		// Kick off processes after short delay

    bluetooth_dev_id = hci_get_route(NULL);
    bluetooth_sock = hci_open_dev( bluetooth_dev_id );
    ERRORCHECK((bluetooth_dev_id < 0 || bluetooth_sock < 0), "Bluetooth socket failure", EndError);

    while ( !heat_shutdown )	{
	delay( 1000 );
	identify_possible_candidates(cycle_timer);	// Identify list of other potential candidates

	maintain_candidates(cycle_timer, bluetooth.candidates, 0); // Maintain list of visible candidates
	maintain_candidates(cycle_timer, bluetooth.possible_candidates, 1); // Maintain list of visible possible candidates
        cycle_timer = ((cycle_timer+1) % OVERALL_TIMER);
    }
    close(bluetooth_sock);

ENDERROR;
}

//
//	Advise bluetooth candicates
//
void	advise_bluetooth_candidates() {
    int	node, network_id, zone;				// Active node
    struct payload_pkt	app_data;			// Packet information

    app_data.type = HEAT_CANDIDATES;			// Contruct CANDIDATES packet to be sent to slave
    memcpy(app_data.d.candidates, bluetooth.candidates, (sizeof(struct proximity_block) * BLUETOOTH_CANDIDATES));

    for ( zone = 0; zone < NUM_ZONES; zone++) {		//Check each Zone
	for ( node = 0; node <  NUM_NODES_IN_ZONE; node++) { // and configured nodes
	    if (strcmp(network.zones[zone].nodes[node].name, "") != 0) { // with valid name
		network_id = get_active_node(network.zones[zone].nodes[node].name); //find if it is currently active
		if (network_id >= 0) {
		    debug(DEBUG_TRACE, "Heat Bluetooth Candidates for %s\n", network.zones[zone].nodes[node].name);
		    send_to_node(network_id, (char *) &app_data, SIZE_CANDIDATES);
		}
	    }
	}
    }
}

//
//	Manage Candidates
//
void	manage_candidates(struct proximity_block new_candidates[]) {
    int i, new_slot;

	// Merge Bluetooth candidate lists

	// First Remove any existing nodes from potential candidate list
    for( i=0; i < BLUETOOTH_CANDIDATES; i++) {
	if ((bacmp(&new_candidates[i].bdaddr, BDADDR_ANY) != 0) &&
	    (check_candidate_list(bluetooth.possible_candidates, &new_candidates[i].bdaddr) > -1)) {
	    bacpy(&bluetooth.possible_candidates[i].bdaddr, BDADDR_ANY);
	    bluetooth.possible_candidates[i].timer = 0;
	    debug(DEBUG_DETAIL, "Remove slot: %d from potentials\n", i);
	}
    }
	// Then remove any existing nodes that do not appear in new list
    for( i=0; i < BLUETOOTH_CANDIDATES; i++) {
	if (check_candidate_list(new_candidates, &bluetooth.candidates[i].bdaddr) < 0) {
	    bacpy(&bluetooth.candidates[i].bdaddr, BDADDR_ANY);
	    bluetooth.candidates[i].timer = 0;
	    debug(DEBUG_DETAIL, "Remove slot: %d\n", i);
	}
    }
	// Then add back in any new candidaes not in the list
    for( i=0; i < BLUETOOTH_CANDIDATES; i++) {
	if (check_candidate_list(bluetooth.candidates, &new_candidates[i].bdaddr) < 0) {
	    new_slot = check_candidate_list(bluetooth.candidates, BDADDR_ANY);
	    ERRORCHECK(new_slot < 0, "Error Merging bluetooth candidate lists\n", EndError);
	    debug(DEBUG_DETAIL, "Add slot: %d in %d\n", i, new_slot);
	    bacpy(&bluetooth.candidates[new_slot].bdaddr, &new_candidates[i].bdaddr);
	    bluetooth.candidates[new_slot].timer = -1;
	}
    }
    display_candidates();
ENDERROR;
}

//
//	Display Candidates
//
void	display_candidates() {
    int 	i;
    char	addr[BLUE_ADDRESS];
    char	string[100];

    for (i=0; i < BLUETOOTH_CANDIDATES; i++) {

	ba2str(&bluetooth.candidates[i].bdaddr, addr);
	sprintf(string, "[%d] %s (%2d)   -", i, addr, bluetooth.candidates[i].timer);
	ba2str(&bluetooth.possible_candidates[i].bdaddr, addr);
	sprintf(string, "%s %s (%2d)\n", string, addr, bluetooth.possible_candidates[i].timer);
	debug(DEBUG_DETAIL, string);
    }
}
