# This script supports the WiPi Heat & Power Control Systems
# It's primary aim is to perform certain elevated function and network monitoring
#
# GPIO functions are now fully supported in the app
#
# http://kampis-elektroecke.de/?page_id=3740
# http://raspi.tv/2013/how-to-use-interrupts-with-python-on-the-raspberry-pi-and-rpi-gpio
# https://pypi.python.org/pypi/RPi.GPIO

import time
import os
import socket
import fcntl
import struct
import logging
import subprocess
import datetime

logging.basicConfig(filename='/var/log/monitor.log', level=logging.DEBUG, format='%(asctime)s %(message)s')
logging.info('WiPi monitor process commenced')

shutdown = False
sleeptime = 1
restartenabled = False
networkerror = 99		# Assumes network will start unconnected
networktimer = 0
now = datetime.datetime.now()
lastmonth = now.month

#
# Check Log Status & Putge key logs each month (no longer called)
#
def check_logstatus():
	global lastmonth

	now = datetime.datetime.now()
	currentmonth = now.month
	if lastmonth != currentmonth: 
		os.system('sudo sh -c "cat /dev/null > /var/log/auth.log"')
		os.system('sudo sh -c "cat /dev/null > /var/log/kern.log"')
		os.system('sudo sh -c "cat /dev/null > /var/log/daemon.log"')
		os.system('sudo sh -c "cat /dev/null > /var/log/messages"')
		os.system('sudo sh -c "cat /dev/null > /var/log/syslog"')
		lastmonth = currentmonth
		logging.info("Logfile Monthly Purge Complete")

#
# Force the WiPi to Restart when file exists
#
def check_restart():
	global shutdown
	global flash
	global sleeptime

#	print "checking restart"
        if os.path.isfile('/var/www/restart.force-restart'):
	   # shutdown our Raspberry Pi
           logging.info('User requested restart')
	   shutdown = True
	   time.sleep(2)
	   flash = True
	   sleeptime = 1
	   os.system("sudo shutdown -r now")
        if os.path.isfile('/var/www/restart.force-shutdown'):
	   # shutdown our Raspberry Pi
           logging.info('User requested shutdown')
	   shutdown = True
	   time.sleep(2)
	   flash = True
	   sleeptime = 1
	   os.system("sudo shutdown -h now")
        if os.path.isfile('/var/www/restart.network-restart'):
	   # restart Wireless Network
           logging.info('User requested WiFi Network restart')
           try: os.remove('/var/www/restart.network-restart')
           except: pass
	   flash = True
	   os.system("sudo ifdown wlan0")
	   time.sleep(5)
	   os.system("sudo ifup wlan0")
        if os.path.isfile('/var/www/restart.heat-restart'):
	   # restart heat
           logging.info('User requested Heat restart')
           try: os.remove('/var/www/restart.heat-restart')
           except: pass
	   os.system("sudo systemctl restart heat.service")
        if os.path.isfile('/var/www/restart.power-restart'):
	   # restart power
           logging.info('User requested Power restart')
           try: os.remove('/var/www/restart.power-restart')
           except: pass
	   os.system("sudo systemctl restart power.service")
        if shutdown:
	   # we are shutting down the Pi - remove forced disk check
           try: os.remove('/forcefsck')
           except: pass

#
# Check the network interfaces - if IP address exists then assume network connected
#
def if_connected(ifname1, ifname2):
    global networkerror
    global networktimer

#   Do not check network status
    return True

    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    try:
       a = socket.inet_ntoa(fcntl.ioctl(
           s.fileno(),
           0x8915,  # SIOCGIFADDR
           struct.pack('256s', ifname1[:15])
       )[20:24])
       return True
    except socket.error as i1:
       pass;
    except IOError as i1:
       pass;
    except:
       logging.info("Unknown network error: %s", ifname1)

    try:
       a = socket.inet_ntoa(fcntl.ioctl(
           s.fileno(),
           0x8915,  # SIOCGIFADDR
           struct.pack('256s', ifname2[:15])
       )[20:24])
       return True
    except socket.error as i2:
       pass
    except IOError as i2:
       pass
    except:
       logging.info("Unknown network error: %s", ifname2)


    if i1[0] != networkerror :
        networkerror = i1[0]
	logging.debug('Network Down error (%d:%d) %s:%s', i1[0], i2[0], ifname1, i1[1])
        networktimer = 0


    networktimer = networktimer + 1;
    if networktimer > 9000 :
	networktimer = 0
        logging.debug('...resetting wlan network')
        plugstatus = subprocess.check_output(['ifdown', '--force', 'wlan0'])
        time.sleep(2)
        plugstatus = subprocess.check_output(['ifup', 'wlan0'])

    return False

#
#	We should check if network connection
#
def check_network():
	global sleeptime
        global networkerror


	if if_connected('wlan0', 'eth0'):
#	   print "Network Connected"
#  Connected

	   if networkerror !=0 :
               logging.debug('Network Up')
               networkerror = 0
	   flash = False
	   sleeptime = 1
	else:
#	   print "No connection"
#  NOT connected
	   sleeptime = 0.2

# Delete Restart signal files
# Only enable if file successfully deleted & detection doesn't cause error
# This should avoid errorneous deadly reboot loop!
try: os.remove('/var/www/restart.force-shutdown')
except: pass
try: os.remove('/var/www/restart.force-restart')
except: pass
try: os.remove('/var/www/restart.network-restart')
except: pass
try: os.remove('/var/www/restart.heat-restart')
except: pass
try: os.remove('/var/www/restart.power-restart')
except: pass
restartenabled = (not os.path.isfile('/var/www/restart.force-shutdown') and
                  not os.path.isfile('/var/www/restart.force-restart') and
                  not os.path.isfile('/var/www/restart.network-restart') and
                  not os.path.isfile('/var/www/restart.heat-restart') and
                  not os.path.isfile('/var/www/restart.power-restart'))

# Force /var/log permissions
#os.system("sudo chmod 777 /var/log")
# Create force disk check file
# os.system("sudo touch /forcefsck")

# do nothing while waiting for button to be pressed
# unless told to flash (set in shutdown above)
while True:
	if not shutdown: check_network()
	if (not shutdown) and restartenabled: check_restart()

#	if not shutdown: check_logstatus()
#	if GPIO.input(5):
#		print("Input was HIGH")
#	else:
#		print("Input was LOW")
#	print("Pulse",flash,LED,sleeptime)

	time.sleep(sleeptime)

# never actually reaches here, but included for completeness
#

