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

#include <wiringPi.h>
#include "errorcheck.h"
//#include "timers.h"
#include "application.h"
#include "heat.h"

struct bluetooth bluetooth;			// Bluetooth candidate data
static int	bluetooth_dev_id;		// Devide Id
static int	bluetooth_sock;			// Bluetooth socket
bdaddr_t	zero_bdaddr;			// Zero address

#define BLUE_NAME	40			// Name length
#define BLUE_ADDRESS	19			// Address length

#define OVERALL_TIMER	60			// Overall cycle of actions
#define IDENTIFY_TIMER	60			// Identify potential candidates
#define MAINT_TIMER	5			// Maintain visibility checks
#define VISIBLE_PERIOD	7			// Num of maintenance period
#define VISIBLE_CHECK	4			// How often to check


//
//	Check Bluetooth name
//
int check_bluetooth_name(bdaddr_t bdaddr, char *name, char *addr) {
    int rc;

    rc = hci_read_remote_name(bluetooth_sock, &bdaddr, BLUE_NAME, name, 0);
    if (rc < 0) strcpy(name, "[unknown]");
    ba2str(&bdaddr, addr);
//    printf("%s  %s (%d)\n", addr, name, rc);
    return(rc);
}

//
//	Check Candidate List
//
int	check_candidate_list(struct proximity_block list[], bdaddr_t *item) {
    int	i;

    for( i=0; i < BLUETOOTH_CANDIDATES; i++) {	// Search candidate list
	if (memcmp(&list[i].bdaddr, item, sizeof(bdaddr_t)) == 0) break;
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
	debug(DEBUG_ESSENTIAL, "Identify possible candidates (%d)\n", timer);

	// Obtain the list of visible Bluetooth Devices
	responses = hci_inquiry(bluetooth_dev_id, HCI_LEN, HCI_RSP, NULL, &pinfo, IREQ_CACHE_FLUSH);
	ERRORCHECK( responses < 0, "Bluetooth Inquiry error", EndError);

 	// Check if we can name them
	for (i = 0; i < responses; i++) {
	    rc = check_bluetooth_name(info[i].bdaddr, name, addr);		//  Get candidates name over bluetooth 
	    if (heat_shutdown) return;
	    if (rc < 0) goto next_candidate;					// if known (named) address
	    rc = check_candidate_list(bluetooth.candidates, &info[i].bdaddr); 	// check if already present as full candidate
	    if (rc != -1) goto next_candidate;					// if known (named) address
	    rc = check_candidate_list(bluetooth.possible_candidates, &info[i].bdaddr); // check if already present
	    if (rc != -1) goto next_candidate;
	    rc = check_candidate_list(bluetooth.possible_candidates, &zero_bdaddr); // check for free slot
	    if (rc < 0) goto next_candidate;					// add new candidate
	    memcpy(&bluetooth.possible_candidates[rc].bdaddr, &info[i].bdaddr, sizeof(bdaddr_t));
	    debug(DEBUG_ESSENTIAL, "Bluetooth potential candidate %s %s\n", name, addr);
	    bluetooth.possible_candidates[rc].timer = VISIBLE_PERIOD;		//  Set countdown timer
next_candidate:;
	}
    }
ENDERROR;
	}
//
//	Maintain Candidates
//
void	maintain_candidates(int timer, struct proximity_block list[], int del) {
    int i, rc;
    char	name[BLUE_NAME];
    char	addr[BLUE_ADDRESS];

    if (heat_shutdown) return;

    if ((timer % MAINT_TIMER)==0) {
	debug(DEBUG_ESSENTIAL, "Maintain candidates (%d)\n", timer);

    for (i=0; i < BLUETOOTH_CANDIDATES; i++) {				// check all valid (non-zero) candidates
	if (heat_shutdown) return;
	if ((memcmp(&list[i].bdaddr, &zero_bdaddr, sizeof(bdaddr_t)) != 0) &&
	    (list[i].timer > 0)) {					// and timer hasn't expired
	    if (((list[i].timer % VISIBLE_CHECK) == 0) |		// every x  period check is device visible
		(list[i].timer < VISIBLE_CHECK)) {			// or every interval if mising!
		rc = check_bluetooth_name(list[i].bdaddr, name, addr);	// look for the device
	    	if (rc > -1) list[i].timer = VISIBLE_PERIOD+1;		// if identified reset visibility timer
	    }
	    list[i].timer--;						// Decrement visibity timer
	}
	if (((list[i].timer == 0) && del) && 				// if timer has expired and delete is true
	    (memcmp(&list[i].bdaddr, &zero_bdaddr, sizeof(bdaddr_t)) != 0)) {
	    ba2str(&list[i].bdaddr, addr);
	    debug(DEBUG_ESSENTIAL, "Bluetooth visibility lost:  %s\n", addr);
	    memcpy(&list[i].bdaddr, &zero_bdaddr, sizeof(bdaddr_t));	// Mark address invalid for entry
	}
    }
    }
}

//
//	Bluetooth Proximity Process
//

void proximity_process()	{
    int	cycle_timer = 0;

    str2ba("00:00:00:00:00:00", &zero_bdaddr);		// Create zero address

    bluetooth_dev_id = hci_get_route(NULL);
    bluetooth_sock = hci_open_dev( bluetooth_dev_id );
    ERRORCHECK((bluetooth_dev_id < 0 || bluetooth_sock < 0), "Bluetooth socket failure", EndError);

    while ( !heat_shutdown )	{
	delay( 1000 );
	identify_possible_candidates(cycle_timer);	// Identify list of other potential candidates

	maintain_candidates(cycle_timer, bluetooth.candidates, 0); // Maintain list of visible candidates
	maintain_candidates(cycle_timer, bluetooth.possible_candidates, 1); // Maintain list of visible candidates

        cycle_timer = ((cycle_timer+1) % 60);
    }
    close(bluetooth_sock);

ENDERROR;
}

