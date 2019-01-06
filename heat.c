/*
Copyright (c) 2014- by John Chandler

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
#include <pthread.h>
#include <bluetooth/bluetooth.h>
//#include <unistd.h>
//#include <signal.h>
//#include <assert.h>
//#include <fcntl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
//#include <sys/time.h>
//#include <time.h>
//#include <sys/types.h>
//#include <sys/socket.h>
//#include <sys/uio.h>
//#include <netinet/in.h>
//#include <netinet/ip.h>
//#include <arpa/inet.h>
//#include <net/if.h>

#include "heat.h"
#include "errorcheck.h"
#include "timers.h"
#include "networks.h"
#include "application.h"
#include "dht11.h"
#include "display.h"

int 		heat_shutdown = 0;				// Shutdown flag
struct app 	app = {OPMODE_MASTER, -1, NULL, "./scripts/", NULL, 0, 0, 1, 0, 0, 0,0, 0, 0.0, 0.0, 0.25};// Application key data

void usage(char *progname) {
    printf("Usage: %s [options...]\n", progname);

    printf("\n");
    printf("Mandatory arguments to long options are mandatory for short options too.\n");

    printf("\n");
    printf("Options:\n");
    printf("    -h, --help          show this help\n");
    printf("    -m, --master        Master operating mode\n");
    printf("    -s, --slave         Slave operating mode\n");
    printf("    -b, --bluetooth     Bluetppth proximity enabled\n");

    printf("    -c, --config=DIR    Network Configuration file directory\n");
    printf("    -l, --log=FILE      redirect shairport's error output to FILE\n");
    printf("    -t, --track=DIR     specify Directory for tracking file (.csv)\n");

    printf("\n");
}

int parse_options(int argc, char **argv) {

    static struct option long_options[] = {
        {"help",    no_argument,        NULL, 'h'},
        {"master",  no_argument,        NULL, 'm'},
        {"slave",   no_argument,        NULL, 's'},
        {"watch",   no_argument,        NULL, 'w'},

        {"config",  required_argument,  NULL, 'c'},
        {"log",     required_argument,  NULL, 'l'},
        {"track",   required_argument,  NULL, 't'},
        {NULL, 0, NULL, 0}
    };
    int opt;

    while ((opt = getopt_long(argc, argv,
                              "+hmswbvc:l:t:",
                              long_options, NULL)) > 0) {
        switch (opt) {
            default:
            case 'h':
                usage(argv[0]);
                exit(1);
            case 'm':
		app.operating_mode = OPMODE_MASTER;			// Set Node as Master
                break;
            case 's':
		app.operating_mode = OPMODE_SLAVE;			// Set Node as Slave
                break;
            case 'w':
		app.operating_mode = OPMODE_WATCH;			// Set Node as Watch (non-cotrolling Slave
                break;
            case 'b':
                app.bluetooth_enabled = 1;				// Enable procimity checking on this node
                break;
            case 'v':
                debuglev++;
                break;
            case 'c':
		app.confdir = optarg;
                break;
            case 'l':
		app.logfile = optarg;
                break;
            case 't':
		app.trackdir = optarg;
                break;
        }
    }
    return optind;
}

//
//	Open Correct Logfile
//

void	open_logfile() {

    if (app.logfile) {					// Logfile is specified on command line
        int log_fd = open(app.logfile,			// Open appropriately
                O_WRONLY | O_CREAT | O_APPEND,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
							// Warn and continue if can't open
	ERRORCHECK( log_fd < 0, "Could not open logfile", EndError);

        dup2(log_fd, STDERR_FILENO);
        setvbuf (stderr, NULL, _IOLBF, BUFSIZ);
    }
ENDERROR;
}

//
//	Signal support functions
//
static void sig_ignore(int foo, siginfo_t *bar, void *baz) {
}
static void sig_shutdown(int foo, siginfo_t *bar, void *baz) {
    heat_shutdown = 1;
}

//static void sig_child(int foo, siginfo_t *bar, void *baz) {
//    pid_t pid;
//    while ((pid = waitpid((pid_t)-1, 0, WNOHANG)) > 0) {
//        if (pid == mdns_pid && !shutting_down) {
//            die("MDNS child process died unexpectedly!");
//        }
//    }
//}

static void sig_logrotate(int foo, siginfo_t *bar, void *baz) {
    open_logfile();
}


//
//	Signal Setup
//
void signal_setup(void) {
    // mask off all signals before creating threads.
    // this way we control which thread gets which signals.
    // for now, we don't care which thread gets the following.
    sigset_t set;
    sigfillset(&set);
    sigdelset(&set, SIGINT);
    sigdelset(&set, SIGTERM);
    sigdelset(&set, SIGHUP);
    sigdelset(&set, SIGSTOP);
    sigdelset(&set, SIGCHLD);
    pthread_sigmask(SIG_BLOCK, &set, NULL);

    // setting this to SIG_IGN would prevent signalling any threads.
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = &sig_ignore;
    sigaction(SIGUSR1, &sa, NULL);

    sa.sa_flags = SA_SIGINFO | SA_RESTART;
    sa.sa_sigaction = &sig_shutdown;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    sa.sa_sigaction = &sig_logrotate;
    sigaction(SIGHUP, &sa, NULL);

    sa.sa_sigaction = &sig_ignore;
    sigaction(SIGCHLD, &sa, NULL);
}

//
//
//	Main procedure
//
//
int main(int argc, char **argv) {
    int		timer;					// timer index
    int payload_len;					// length of payload returned
    struct payload_pkt app_data;			// App payload data
    pthread_t display_thread, monitor_thread, proximity_thread;	// thread IDs
    char 	node_name[HOSTNAME_LEN];			// Node name
    signal_setup();					// Set up signal handling

    app.setpoint = 18.0;				// Start with dummy setpoint

    parse_options(argc, argv);				// Parse command line parameters
    open_logfile();					// Open correct logfile
    debug(DEBUG_ESSENTIAL, "Heat starting in %s mode\n", (app.operating_mode == OPMODE_MASTER ? "MASTER" :app.operating_mode == OPMODE_SLAVE ? "SLAVE" : "WATCH"));

    initialise_network(sizeof(struct payload_pkt),notify_link_up, notify_link_down);	// Initialise the network details with callbacks
    initialise_timers();				// and set all timers
    initialise_GPIO();					// initialise wiringPi
    load_configuration_data();				// Load the Configuration on MASTER node

    pthread_create(&monitor_thread, NULL, (void *) monitor_process, NULL);	// create Monitor thread
    ERRORCHECK( monitor_thread == 0, "Monitor thread creation failed\n", EndError);
    pthread_create(&display_thread, NULL, (void *) display_process, NULL);	// create display thread
    ERRORCHECK( display_thread == 0, "Display thread creation failed\n", EndError);
    pthread_create(&proximity_thread, NULL, (void *) proximity_process, NULL);	// create proximity thread
    ERRORCHECK( proximity_thread == 0, "Proximity thread creation failed\n", EndError);

    switch (app.operating_mode) {
    case OPMODE_MASTER:					// Only Master nodes are responsible for broadcasting
	add_timer(TIMER_BROADCAST, timeto1min());	// Set to refresh network in y seconds
	break;

    case OPMODE_SLAVE:
    case OPMODE_WATCH:
	add_timer(TIMER_SETPOINT, 20);			// Set to refresh setpoint in y seconds
	break;
    }
    add_timer(TIMER_DISPLAY, 75);			// and timeout the screen in z seconds
    add_timer(TIMER_LOGGING, 15);			// atart off the logging process
    add_timer(TIMER_CONTROL, 15);			// Set to perform Master & slabe control actions in y seconds

    while (!heat_shutdown) {					// While NOT shutdown
	wait_on_network_timers(); 			// Wait for message or timer expiory

	if (check_network_msg()) {			// If a message is available
	    handle_network_msg(&node_name[0],(char *)&app_data, &payload_len); // handle the network message
	    handle_app_msg(node_name, &app_data, payload_len);	// handle application specific messages
	}
	timer = check_timers();				// check which timer has expired
	switch (timer) { 				//
	case TIMER_NONE:				// No expired timers
	    break;					// Do nothing

	case TIMER_BROADCAST:				// On Broadcast timer
	    broadcast_network();			// send out broadcast message to contact other nodes
	    add_timer(TIMER_BROADCAST, timeto1min());	// and set to broadcast again in y seconds
	    break;

	case TIMER_PING:
	    if (check_live_nodes()) {			// On Ping check the network
//		add_timer(TIMER_REPLY, 2);		// Expire replies if not received within x secoonds
		add_timer(TIMER_REPLY, 5);		// Expire replies if not received within x secoonds
		add_timer(TIMER_PING, timetosec(20)+1);	// and set to Ping again in y seconds
	    }
	    break;

	case TIMER_REPLY:
	    expire_live_nodes();			// Expire other nodes where reply has timed out
	    break;

	default:
	    handle_app_timer(timer);			// ask application to handle all other timers
	    break;
	}
	DEBUG_FUNCTION( DEBUG_DETAIL, display_live_network());
	DEBUG_FUNCTION( DEBUG_DETAIL, display_timers());
    }
    debug(DEBUG_ESSENTIAL, "Heat node starting shut down\n");
    manage_CLOSE();					// Close down control functions
    network_CLOSE();					// Close network 
    pthread_join(proximity_thread, NULL);
    pthread_join(display_thread, NULL);
    pthread_join(monitor_thread, NULL);

ENDERROR;
    debug(DEBUG_ESSENTIAL, "Heat node shut down\n");
    return 0;
}




