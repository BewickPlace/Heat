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
#ifndef __BLUETOOTH_H
#include <bluetooth/bluetooth.h>
#endif

#define MAX(a,b) (((a)>(b))?(a):(b))

#define	OPMODE_MASTER	0			// Application operating modes
#define OPMODE_SLAVE	1			// Master (controller)/ Slave (Sensor)
#define OPMODE_WATCH	2			// Watch (Sensor) - monitoring only
#define OPMODE_HOTWATER	3			// Monitor the Hot Water as a special Zone

#define BLUETOOTH_CANDIDATES 5			// Number of bluetooth devices for proximity checking
struct app {					// Application key Data
    int		operating_mode;			// - operating mode
    int		active_node;			// - active node
    char	*logfile;			// Debug log file
    char	*confdir;			// Directory for Heating Network conf file
    char	*trackdir;			// Directory for tracking (csv) file
    int		bluetooth_enabled;		// Bluetooth proximity checking enabled on node
    int		at_home;			// Bluetooth proximity state
    int		display;			// - display off/on
    // Master specific data
    int		display_mode;			// Alternate display modes
    time_t	run_time;			// Total run (CALL) time today
    time_t	run_time_start;			// Start time if run(CALL) active
    // Slave specific data
    int		callsat;			// - local call/sat status
    int 	boost;				// - boost uplift
    float	setpoint;			// - sensor setpoint
    float	temp;				// - sensor reading
    float	hysteresis;			// - hysteresis
    };

#define BN_LEN 6				// Bluetooth name length
#define BN_CPY(dest, src) memcpy(dest, src, sizeof(src)) // Bluetooth name copy 

struct	proximity_block {
    bdaddr_t	bdaddr;
    char	name[BN_LEN];
    int		timer;
    };

struct  bluetooth {				// Bluetooth specific data
    struct proximity_block candidates[BLUETOOTH_CANDIDATES];
    };

extern int heat_shutdown;			// Signal heat shutdown between threads
extern struct app app;				// Application key data
extern struct bluetooth bluetooth;		// Bluetooth proximity data
