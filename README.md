## Intro
This is an implementation of the exercise from WISI.
 
## Requirement (Exercise from WISI)
The exercise is available in Golang and C. 
Pick the language you are most comfortable working in.

At WISI, we have an alert system that will send out alerts in case of issues with the video or health of the system. 
Our email notification system will receive these alerts and send out an email for every alert it receives. 
The email notification system is quite simple but lacks a feature that could be useful for administrators using it: 
the ability to pause email notifications for a period of time when they've receive too many emails.

The goal of this feature is to detect repeated alerts of the same type within a given time interval and pause all email notifications of that alert type for a given period of time.

Here is an example:

```
// Pause Config Options - Units in seconds
const PauseTime = 300
const MaxAlerts = 5
const Period = 60
```

If we see over 5 critical alerts in a minute, 
then we want all critical alerts to be paused for 5 minutes. 
The same applies to other alert types.

## Environment
* C Language
* Windows OS
* Visual Studio IDE

## Implementation
The basic idea is that a kind of state machine runs with using circular array per type of alert. 
Each circular array keeps the time information of alert sent. 
To decide whether an alert can be sent or paused, its time(current time) is compared to the oldest ones in the corresponding array. 
Accordingly, a kind of window of the array moves forward circularly by adding the alert newly sent and removing the oldest ones or the alert is paused under the pause config option below.

* #define MAX_ALERTS 5
* const int PauseTime = 300*SCALE;
* const int Period = 60*SCALE;

SCALE is just for the test and can be set as less value than 1 for faster tests.

## Test
3 test cases are added in the code, which shows the result log in the console as below.

* test case 1: fewer alerts than MAX_ALERTS within Period
* test case 2: more alerts than MAX_ALERTS within Period
* test case 3: mixed case 

An independent test case input like using file I/O seems not to be required due to its time effort.
However, an additional implementation for feeding more test cases without touching source code is highly required for commercial purposes.
