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

    p = find_block(*haystack, "network {");
    ERRORCHECK( p == NULL, "Network block not found", EndError);
    block_end = find_block(*haystack, "}");
    ERRORCHECK( block_end == NULL, "Network block end not found", EndError);

    p = find_key(p, "name", &network.name[0], block_end);

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
	    network.zones[zone].profile[i] = match_profile(name);
	}
	zone++;
    }
    *haystack = block_end;
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
	    p = find_change( p, "change", &profiles[profile].block[i].time, &profiles[profile].block[i].setpoint, block_end); // get the time & setpoint
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

//
//	Initialise Internal Configuration Data
//

void	initialise_configuration() {
    int	zone, node = 0;;

    memset(&network,0, sizeof(network));		// Seroise internal configuration data

    for(zone = 0; zone < NUM_ZONES; zone++) {
    for(node = 0; node < NUM_NODES_IN_ZONE; node++) {
	network.zones[zone].nodes[node].network_id = -1;
    }}
}

#define MAX_CONFIG_DATA 500

static time_t	last_mtime = 0;				// Time config was last modified

//
//	Load configuration data
//
void load_configuration_data() {
    FILE	*fp;					// File Descriptor
    struct stat file_info;				// File status info
    char	file_data[MAX_CONFIG_DATA][4];		// Buffer for file data
    char	*unprocessed_data;			// pointer to unprocessed element of data
    size_t	size;					// size in bytes of data read
    int		rc;					// return code

    fp = fopen("heating.conf", "r");			// Open config file read only
    ERRORCHECK(fp==NULL, "Configuration Open Error", EndError);

    rc = fstat(fileno(fp), &file_info);				// check the file details
    ERRORCHECK(rc < 0, "Configuration File Stat error", StatEnd);

    if (file_info.st_mtime <= last_mtime) { goto StatEnd; } // If configuration hasn't change skip load
    last_mtime = file_info.st_mtime;

    ERRORCHECK( file_info.st_size > (MAX_CONFIG_DATA *4), "Configuration file too big!", StatEnd);// check our buffer is large enough!!
    // Proceed with Loading configuration details
    debug(DEBUG_ESSENTIAL, "Load Configuration File\n");
    initialise_configuration();				// Re-initialise all elements of the config

    size = fread(file_data, MAX_CONFIG_DATA, 4, fp);	// Read the file into memory
    ERRORCHECK( size < 0, "Configuration Read Error", EndError);

    unprocessed_data = (char *)file_data;		// start at the beggining of the data

    rc = parse_network(&unprocessed_data);		// Parse the network block
    ERRORCHECK(rc == 0, "Error parsing Network config", ParseError);

    rc = parse_profile(&unprocessed_data);		// Parse the profile blocks
    ERRORCHECK(rc == 0, "Error parsing Profile config", ParseError);

    rc = parse_zone(&unprocessed_data); 		// Parse the zone blocks
    ERRORCHECK(rc == 0, "Error parsing Zone config", ParseError);

    rc = parse_node(&unprocessed_data);			// Parse the node block
    ERRORCHECK(rc == 0, "Error parsing Node config", ParseError);

    debug(DEBUG_ESSENTIAL, "Load Configuration Complete\n");
    fclose(fp);

ERRORBLOCK(StatEnd);
    fclose(fp);

ERRORBLOCK(ParseError);
    fclose(fp);

ENDERROR;
}