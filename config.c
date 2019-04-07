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
//#include <time.h>
//#include <sys/types.h>
//#include <sys/socket.h>
//#include <sys/uio.h>
//#include <netinet/in.h>
//#include <netinet/ip.h>
//#include <arpa/inet.h>
//#include <net/if.h>

#include "errorcheck.h"
#include "dht11.h"
#include "heat.h"
#include "application.h"

//
//	Local impleentation of Strpos
//
int	strpos(char *haystack, char *needle, int offset) {
    char *p = strstr((haystack+offset), needle);

    if (p)  return(p-haystack); else return(-1);
}

//
//	Extract the element in quotes after the defined key
//	Extract looking for newline if quotes not found
//

char	*find_key(char *haystack, char *key, char *field, char *upper) {
    int	o, x, y;

    o = strpos(haystack, key, 0);
    if ((o == -1) | (o > (upper - haystack))) return(NULL);
    o = o + strlen(key);
    x = strpos(haystack, "\"", o)+1;
    if (x > o) {
	y = strpos(haystack, "\"", x);
    } else {
	x = o;
   	y = strpos(haystack, "\n", x);
    }
    memcpy(field, haystack + x, y-x);
    field[y-x] = '\0';
    return (haystack + y);
}
//
//	Find Time
//
char	*find_time(char *haystack, char *key, time_t *time, char *upper) {
    char *p;
    char time_string[20];
    int	hours, minutes;

    p =  find_key(haystack, key, time_string, upper);
    sscanf(time_string, "%d:%d", &hours, &minutes);
    *time = (((hours * 60) + minutes)*60);

    return(p);
}
//
//	Extract the element in quotes after the defined key
//	in the format HH:MM, xx.y
//

char	*find_change(char *haystack, char *key, time_t *time, float *setpoint, char *upper) {
    int	o, x, y;
    char change[20];
    int	hours, minutes;

    o = strpos(haystack, key, 0);
    if ((o == -1) | (o > (upper-haystack))) return(NULL);
    o = o + strlen(key);
    x = strpos(haystack, "\"", o)+1;
    if (x > o) {
	y = strpos(haystack, "\"", x);
    } else {
	x = o;
   	y = strpos(haystack, "\n", x);
    }
    memcpy(change, haystack + x, y-x);
    change[y-x] = '\0';
    sscanf(change, "%d:%d, %f", &hours, &minutes, setpoint);
    *time = (((hours * 60) + minutes)*60);

    return (haystack + y);
}

//
//	Find Descriptor Block
//
char	*find_block(char *haystack, char *needle){
    char	*p;

    p = strstr(haystack, needle);
    if (p!=NULL) p = p + strlen(needle);
    return(p);
}

//
//	Match Profile name
//
struct profile	*match_profile(char *name) {
    int		i=0;

    for (i = 0; i < MAX_PROFILES; i++) {
	if (strcmp(name, profiles[i].name) == 0) return( &profiles[i] );
    }
    return(0);
}
//
//	Match Zone  name
//
int	match_zone(char *name) {
    int		i=0;

    for (i = 0; i < NUM_ZONES; i++) {
	if (strcmp(name, network.zones[i].name) == 0) return(i);
    }
    return(-1);
}
//
//	Match Node name
//
int	match_node(char *name, int zone) {
    int		i=0;

    for (i = 0; i < NUM_NODES_IN_ZONE; i++) {
	if (strcmp(name, network.zones[zone].nodes[i].name) == 0) return(i);
    }
    return(-1);
}
//
//	Parse Network
//
int	parse_network(char **haystack) {
    char	*p, *block_end = NULL;
    char	string[10];

    p = find_block(*haystack, "network {");
    ERRORCHECK( p == NULL, "Network block not found", EndError);
    block_end = find_block(*haystack, "}");
    ERRORCHECK( block_end == NULL, "Network block end not found", EndError);

    p = find_key(p, "name", &network.name[0], block_end);
    ERRORCHECK(p == NULL, "Network: Name Block not found", EndError);

    p = find_time(p, "on", &network.on, block_end);
    ERRORCHECK(p == NULL, "Network: On Block not found", EndError);
    p = find_time(p, "off", &network.off, block_end);
    ERRORCHECK(p == NULL, "Network: Off Block not found", EndError);
    p = find_key(p, "delta", &string[0], block_end);
    sscanf(string, "%f,%f", &network.at_home_delta_out, &network.at_home_delta_away);
    network.at_home_delta = network.at_home_delta_out;

    *haystack = block_end;
    return(1);

ENDERROR;
    return(0);
}

//	Parse Zone
int	parse_zone(char **haystack) {
    char	*p, *block_end = NULL;
    int		zone = 0;
    int		i;
    char	name [NAMELEN];

    p = find_block(*haystack, "zone {");
    ERRORCHECK( p == NULL, "Zone Block not found", EndError);
    while ((p!=0) &&				// Look for as many zones as defined
	   (zone < NUM_ZONES)) {
	block_end = find_block(*haystack, "}");
	ERRORCHECK( block_end == NULL, "Zone Block end not found", EndError);

	p = find_key(p, "name", &network.zones[zone].name[0], block_end); // get the name in this block

	for (i = 0; i < 7; i++) {		// find the daily profiles
	    p = find_key(p, "profile", &name[0], block_end); // get the name of profiles in this block
	    ERRORCHECK( p == NULL, "Zone Profile not found", EndError);
	    network.zones[zone].profiles[i] = match_profile(name);
	}
	zone++;
	*haystack = block_end;
	p = find_block(*haystack, "zone {");	// Look for next Zone
    }
    return(1);
ENDERROR;
    return(0);}

//	Parse Node
int	parse_node(char **haystack) {
    char	*p, *block_end = NULL;
    char	name [NAMELEN];
    char	string[10];
    int		zone = 0;
    int		node = 0;

    p = find_block(*haystack, "node {");
    ERRORCHECK( p == NULL, "Node Block not found", EndError);
    while (p != 0) {
	block_end = find_block(*haystack, "}");
	ERRORCHECK( block_end == NULL, "Node block end not found", EndError);
	p = find_key(p, "name", &name[0], block_end);			// find the node name
	p = find_key(p, "zone", &string[0], block_end);			// and the zone we are in
	zone = match_zone(string);					// match the zone
	node = match_node("", zone);					// and look for empty slots
	ERRORCHECK(node < 0, "Zone no node space", EndError);
	memcpy(network.zones[zone].nodes[node].name, name, strlen(name));

	p = find_key(p, "delta", &string[0], block_end);
	sscanf(string, "%f", &network.zones[zone].nodes[node].delta);
	p = find_key(p, "hysteresis", &string[0], block_end);
	sscanf(string, "%f", &network.zones[zone].nodes[node].hysteresis);

	*haystack = block_end;
	p = find_block(*haystack, "node {");
    }
    return(1);

ENDERROR;
    return(0);
}

//	Parse Profile
int	parse_profile(char **haystack) {
    char	*p, *block_end = NULL;;
    int		profile = 0;
    int		i;

    p = find_block(*haystack, "profile {");
    ERRORCHECK( p == NULL, "Profile Block not found", EndError);
    while ((p!=0) &&				// Look for as many profiles as defined
	   (profile < MAX_PROFILES)) {
	block_end = find_block(*haystack, "}");
	ERRORCHECK( block_end == NULL, "Profile Block end not found", EndError);

	p = find_key(p, "name", &profiles[profile].name[0],block_end); // get the name within this block

	for (i = 0; i < MAX_TIME_BLOCKS; i++) {	// find the change blocks
	    p = find_change( p, "change", &profiles[profile].blocks[i].time, &profiles[profile].blocks[i].setpoint, block_end); // get the time & setpoint
	    if (p == NULL) { break; }
	}
	*haystack = block_end;			// Adjust pointer
	profile++;				// and move on to next block
	p = find_block(*haystack, "profile {");	// Look for next profile
    }
    return(1);

ENDERROR;
    return(0);
}

//	Parse Bluetooth
int	parse_bluetooth(char **haystack) {
    char	*p, *block_end = NULL;;
    int		i;
    char	bluetooth_entry[BN_LEN+19+2];	// Full bluetooth entry string
    char	name[BN_LEN+1];
    char	addr[19];			// Bluetooth address string

    p = find_block(*haystack, "bluetooth {");
    ERRORCHECK( p == NULL, "Bluetooth Block not found", EndError);
    block_end = find_block(*haystack, "}");
    ERRORCHECK( block_end == NULL, "Bluetooth Block end not found", EndError);

    for (i = 0; i < BLUETOOTH_CANDIDATES; i++) { 	// find as many devices as defined
//	p = find_key(p, "device", addr, block_end);	// extract devic addres string
	p = find_key(p, "device", bluetooth_entry, block_end);	// extract full entry string string
	if (p == NULL) { break; }
	sscanf(bluetooth_entry, "%[^,],%s", name, addr); // parse name & address
	BN_CPY(bluetooth.candidates[i].name, name);
	str2ba(addr, &bluetooth.candidates[i].bdaddr);	// and save as bluetooth address
	bluetooth.candidates[i].timer = -1;		// and default timer
    }
    *haystack = block_end;			// Adjust pointer
    return(1);

ENDERROR;
    return(0);
}

//
//	Initialise Internal Configuration Data
//

void	initialise_configuration() {
    int	i;

    memset(&network,0, sizeof(network));		// Seroise internal configuration data
    network.fresh = 1;					// Signal this is a fresh copy of configuration
    stop_run_clock();					// Ensure Run Clock has been stopped
    for( i=0; i < NUM_ZONES; i++) {			// For each zone
	callsat(i, 0);					// Ensure output is off
    }

    for (i=0; i < BLUETOOTH_CANDIDATES; i++) {		// Setup  default candidate list overwriting previous
	bacpy(&bluetooth.candidates[i].bdaddr, BDADDR_ANY);
	bluetooth.candidates[i].timer = 0;
	// leave potential candidates untouched
    }
}

#define MAX_CONFIG_DATA 500
#define ENTRIES		6

static time_t	last_mtime = 0;				// Time config was last modified

//
//	Load configuration data
//
int load_configuration_data() {
    FILE	*fp;					// File Descriptor
    struct stat file_info;				// File status info
    char	file_data[MAX_CONFIG_DATA][ENTRIES];	// Buffer for file data
    char	*unprocessed_data;			// pointer to unprocessed element of data
    size_t	size;					// size in bytes of data read
    int		rc;					// return code
    char	conf_file[40];				// full path and file name

    if (app.operating_mode != OPMODE_MASTER) { goto EndError; } // Only Load on MASTER node
    sprintf(conf_file, "%s%s", app.confdir, "heating.conf");
    fp = fopen(conf_file, "r");				// Open config file read only
    ERRORCHECK(fp==NULL, "Configuration Open Error", OpenError);

    rc = fstat(fileno(fp), &file_info);				// check the file details
    ERRORCHECK(rc < 0, "Configuration File Stat error", StatEnd);

    if (file_info.st_mtime <= last_mtime) { goto StatEnd; } // If configuration hasn't change skip load
    last_mtime = file_info.st_mtime;

    ERRORCHECK( file_info.st_size > (MAX_CONFIG_DATA *ENTRIES), "Configuration file too big!", StatEnd);// check our buffer is large enough!!
    // Proceed with Loading configuration details
    debug(DEBUG_ESSENTIAL, "Load Configuration File\n");
    initialise_configuration();				// Re-initialise all elements of the config

    size = fread(file_data, MAX_CONFIG_DATA, ENTRIES, fp);	// Read the file into memory
    ERRORCHECK( size < 0, "Configuration Read Error", ParseError);

    unprocessed_data = (char *)file_data;		// start at the beggining of the data

    rc = parse_network(&unprocessed_data);		// Parse the network block
    ERRORCHECK(rc == 0, "Error parsing Network config", ParseError);

    rc = parse_profile(&unprocessed_data);		// Parse the profile blocks
    ERRORCHECK(rc == 0, "Error parsing Profile config", ParseError);

    rc = parse_zone(&unprocessed_data); 		// Parse the zone blocks
    ERRORCHECK(rc == 0, "Error parsing Zone config", ParseError);

    rc = parse_node(&unprocessed_data);			// Parse the node block
    ERRORCHECK(rc == 0, "Error parsing Node config", ParseError);

    rc = parse_bluetooth(&unprocessed_data);		// Parse the bluetooth proximity block
    ERRORCHECK(rc == 0, "Error parsing Bluetooth config", ParseError);

    debug(DEBUG_ESSENTIAL, "Load Configuration Complete\n");
    fclose(fp);
    return(1);

ERRORBLOCK(OpenError);
    debug(DEBUG_ESSENTIAL, "File: %s\n", conf_file);
    return(0);

ERRORBLOCK(StatEnd);
    fclose(fp);
    return(0);

ERRORBLOCK(ParseError);
    fclose(fp);
    return(0);

ENDERROR;
    return(1);
}

//
//	Read Pi Revision code from /proc/CPUinfo
//
void PiRevision(char *rev) {
    FILE	*fp;					// File Descriptor
    char	file_data[MAX_CONFIG_DATA][1];		// Buffer for file data
    size_t	size;					// size in bytes of data read
    char	*p;					// data pointer

    fp = fopen("/proc/cpuinfo", "r");			// Open processor config file read only
    ERRORCHECK(fp==NULL, "CPUinfo Open Error", EndError);

    size = fread(file_data, MAX_CONFIG_DATA, ENTRIES, fp);	// Read the file into memory
    ERRORCHECK( size < 0, "CPUinfo Read Error", ReadError);

    p = find_block((char *)file_data, "Revision");		// Find the revision line
    ERRORCHECK(p==NULL, "CPUinfo Revision not found", ReadError);
    p = find_block(p, ": ");				// Find the revision line
    ERRORCHECK(p==NULL, "CPUinfo Revision not found", ReadError);

    sscanf(p,"%s", rev);				// Extract the string after the colon
    fclose(fp);


ERRORBLOCK(ReadError);
    fclose(fp);
ENDERROR;
}
