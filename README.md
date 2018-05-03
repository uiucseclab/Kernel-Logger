# Kernel-Logger

A Linux keylogger that will log keystrokes and send important keystrokes data through a port

## Getting Started

### Prerequisites
Before installing make sure you install the proper headers

```
# apt-get install linux-headers-$(uname -r)
# apt-get install build-essential
```

This has been tested on Ubuntu 16.04.3

### Installing
A makefile has provided to use with the proj.c file
To compile and install to Ubuntu:

```
# make
# insmod proj.ko
```

In actual use, we would add a cron job using:
This would make it run once an hour to send information over a port 

```
# crontab -e
0 * * * * /usr/bin/python3 (Location where helper.py file is)/helper.py
```

### Uninstalling
```
# rmmod proj
```

### Examples/Features
This program will run hourly in order to log important information such as logins to websites. Examples include:
```
the quick brown fox jumped over the lazy dog facebook.com abce @ gmail.com that abc qrstuv the quick brown fox the quick brown 
fox Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna 
aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute 
irure dolor in reprehenderit in google.com username passWORD voluptate velit esse cillum dolore eu fugiat nulla pariatur. 
Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum
```

This would return:

```
jumped over the lazy dog facebook.com abce @ gmail.com that abc --MSG
lazy dog facebook.com abce @ gmail.com that abc qrstuv the --MSG
irure dolor in reprehenderit in google.com username passWORD voluptate velit -- MSG
```

Then it would send these messages through a port. For this proof of concept, I sent it through localhost.

To see the logs that aren't filtered yet stored in real time you can 'cat' the file:
```
$ cat /proc/proj/status
```

The python 'helper.py' script does most of the filtering and shifting to uppercase. For example, the kernel logger would read the key presses as:
```
pass leftshiftd word leftshiftu
```

This would be translated to:
```
passWORD
```

### Files included
```
helper.py -> The script that runs every hour
proj.c -> the Kernel module that will log the keys
table.html -> Keycode table translation website
mapMaker.py -> Helper for the proj.c file that prints out the map from keycode to ascii value
```
### References
https://www.gadgetweb.de/programming/39-how-to-building-your-own-kernel-space-keylogger.html
http://www.comptechdoc.org/os/linux/howlinuxworks/linux_hlkeycodes.html
