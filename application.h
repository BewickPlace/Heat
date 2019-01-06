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

#define	NAMELEN		14			// Length of names allowed
#define MAX_TIME_BLOCKS 10			// time change blocks in a profile
#define MAX_PROFILES	10			// total number of profiles
#define NUM_NODES_IN_ZONE 3			// Max nodes per zone
#define NUM_ZONES	2			// Max zones supported

struct	timeblock {				// Setpoint  Change Time  Block
    time_t	time;
    float	setpoint;
    };

struct profile {				// Daily profile of Timeblock changes
    char	name[NAMELEN];
    struct timeblock blocks[MAX_TIME_BLOCKS];
    };

struct node {					// Node information including  daily profiles
    char	name[NAMELEN];
//    int		network_id;
    float	delta;
    float	hysteresis;
    int		at_home;
    int		callsat;
    float	temp;
    };

struct zone {					// Zonal information
    char	name[NAMELEN];
    struct node nodes[NUM_NODES_IN_ZONE];
    struct profile *profiles[7];
    };

struct network {				// Overall Network data
    int		fresh;				// Signal a fresh copy of config data
    char	name[NAMELEN];
    time_t	on;
    time_t	off;
    float	at_home_delta;
    float	at_home_delta_out, at_home_delta_away;
    struct zone zones[NUM_ZONES];
    };

struct network 	network;			// Network data (held on Master node)
struct profile	profiles[MAX_PROFILES];	// Profile data (held on Master node)


#define PAYLOAD_TYPE	10			// Add possible payload types here
#define HEAT_SETPOINT 	11			// New setpoint active
#define HEAT_CALL	12			// CALL for heat
#define HEAT_SAT	13			// heat SATisfied
#define HEAT_TEMP	14			// advise currenp TEMPerature
#define HEAT_CANDIDATES	15			// Bluetooth proximity candidates

#define PAYLOAD_DATA_LEN 12

#define SIZE_SETPOINT	(sizeof(struct setpoint_pkt)+sizeof(int))
#define SIZE_CALL	(sizeof(struct callsat_pkt)+sizeof(int))
#define SIZE_SAT	(sizeof(struct callsat_pkt)+sizeof(int))
#define SIZE_TEMP	(sizeof(struct callsat_pkt)+sizeof(int))
#define SIZE_CANDIDATES	((sizeof(struct proximity_block)*BLUETOOTH_CANDIDATES)+sizeof(int))

struct setpoint_pkt {				// Setpoint data packet
    float 	value;				// Setpoint value
    float	hysteresis;			// Hysteresis
    };

struct callsat_pkt {				// CALL & SAT  data packet
    float	temp;				// Current Temperature
    float 	setpoint;			// Setpoint value
    float	hysteresis;			// Hysteresis
    int 	boost;				// Boost status
    int		at_home;			// Bluetooth At_home status
    };

union payload_data {				// Union of possible payloads
    char	data[PAYLOAD_DATA_LEN];
    struct 	setpoint_pkt 	setpoint;
    struct	callsat_pkt 	callsat;
    struct	proximity_block candidates[BLUETOOTH_CANDIDATES];
    };

struct payload_pkt {
    int	 type;
    union payload_data d;
    };						// Expand definition of payload here

void    notify_link_up();
void    notify_link_down();
void	handle_app_msg(char *node_name, struct payload_pkt *packet, int payload_len);
void	handle_app_timer(int timer);

void	perform_logging();

void    load_configuration_data();              // Main process
int	match_node(char *name, int zone);

void	setpoint_control_process();
void	manage_CALL(char *node_name, float temp, int at_home);
void	manage_SAT(char *node_name, float temp, int at_home);
void	manage_TEMP(char *node_name, float temp, int at_home);
void	manage_CLOSE();

void	proximity_process();			// Bluetooth proximity process
void	advise_bluetooth_candidates();		// advise candidates
void	manage_candidates(struct proximity_block[]); // Manage new candidates list from Master

void	stop_run_clock();			// Stop the run time (CALL) clock
void	reset_run_clock();			// Reset run time for CALL clock
time_t	get_run_clock();			// Get the total time run(CALL) for today

int	check_any_at_home();			// Check if anyone is At Home
int	check_any_CALL_in_zone();		// Check if any current CALL for heat

void	send_SAT();				// Send SATisfied message to master
void	PiRevision(char *rev);			// Obtain CPUinfo Revision
