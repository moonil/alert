#ifndef _WIN32
#error please run on Win32
#endif

//#define LOG_L
#define LOG_M
#define LOG_H

#define TEST

//#include <unistd.h>
#include <windows.h>
#include <stdio.h>
#include <time.h>


#ifdef TEST
#define SCALE 30 // for making test faster
#else
#define SCALE 1
#endif

// Pause Config Options - Units in seconds
#define PAUSE_TIME      (300/SCALE)
#define MAX_ALERTS      5
#define PERIOD          (60/SCALE)

// Types of alerts
typedef enum
{
    ALERT_TYPE_WARNING = 0,
    ALERT_TYPE_ERROR,
    ALERT_TYPE_CRITICAL,
    NUM_TYPE_ALERTS
} Alert;

// Type of status
typedef enum
{
    OK = 0,
    MAX,
    PAUSED
} Status;

const char* Alert_Strings[] =
{
    "Warning",
    "Error",
    "Critical"
};

typedef struct _AlertState {
    int s, e;              // index of circular array, t[] 
    int n;                 // count of alert
    boolean isPaused;      // state of pause
    time_t t[MAX_ALERTS];  // circular array of time log 

    int total, send, discard; // Statics

} AlertState;

// state of alerts
AlertState alertState[NUM_TYPE_ALERTS];

const char* toString(Alert a)
{
    return Alert_Strings[a];
}

void initAlertState(Alert a)
{
#ifdef LOG_L
    printf("initAlertState: %s\n", toString(a));
#endif

    alertState[a].s = 0;
    alertState[a].e = 0;
    alertState[a].n = 0;
    alertState[a].isPaused = FALSE;
}

// initialize state of alert
void initStatistics(Alert a)
{
#ifdef LOG_L
    printf("initStatistics: %s\n", toString(a));
#endif

    alertState[a].total = 0;
    alertState[a].send = 0;
    alertState[a].discard = 0;
}

void startState()
{
#ifdef LOG_L
    printf("startState\n");
#endif
    for (Alert a = 0; a < NUM_TYPE_ALERTS; a++)
    {
        initAlertState(a);
        initStatistics(a);
    }
}

void printAlertState(Alert a)
{
    printf("alertState [%s]\n", toString(a));
    printf(".s : %d\n", alertState[a].s);
    printf(".e : %d\n", alertState[a].e);
    printf(".n : %d\n", alertState[a].n);
    for (int i = 0; i < MAX_ALERTS; i++)
    {
        printf(".t[%d]: %I64d\n", i, alertState[a].t[i]);
    }
}

void printState()
{
    for (Alert a = 0; a < NUM_TYPE_ALERTS; a++)
    {
        printf("---------------\n");
        printAlertState(a);
    }
    printf("---------------\n");
}

void printStatistics()
{
    printf("\n\n\n");
    for (Alert a = 0; a < NUM_TYPE_ALERTS; a++)
    {
        
        printf("[%s] total: %d, send: %d, discard: %d\n", 
            toString(a), 
            alertState[a].total,
            alertState[a].send,
            alertState[a].discard);
    }
}

void addAlert(Alert a, time_t t)
{
#ifdef LOG_L
    printf("addAlert\n");
#endif
    // if n == 0, s == e
    if (alertState[a].n > 0) {
        alertState[a].e++;
        if (alertState[a].e == MAX_ALERTS)
        {
            alertState[a].e = 0;
        }
    }

    alertState[a].n++;
    alertState[a].t[alertState[a].e] = t;
}

void removeAlert(Alert a)
{
#ifdef LOG_L
    printf("removeAlert\n");
#endif

    alertState[a].n--;
    // if n == 0, s == e
    if (alertState[a].n > 0) {
        alertState[a].s++;
        if (alertState[a].s == MAX_ALERTS)
        {
            alertState[a].s = 0;
        }
    }
}

Status checkState(Alert a, time_t t)
{   
    Status result;

    alertState[a].total++;
    if (!alertState[a].isPaused)
    {
        if (alertState[a].n == MAX_ALERTS)
        {
            if ((int)difftime(t, alertState[a].t[alertState[a].s]) 
                < PERIOD)
            {
#ifdef LOG_L
                printf("[WHY] too much %s over MAX_ALERTS(%d) in PERIOD(%d sec)\n", 
                toString(a), MAX_ALERTS, PERIOD);
#endif
                alertState[a].isPaused = TRUE;
                alertState[a].discard++;

                result = MAX;  // discard Alert
            }
            else
            {
                removeAlert(a);
                addAlert(a, t);
                alertState[a].send++;
                result = OK; // send Alert
            }
        }
        else
        {
            addAlert(a, t);
            alertState[a].send++;
            result = OK; // send Alert
        }
    }
    else
    {
        if ((int)difftime(t, alertState[a].t[alertState[a].e]) 
            < PAUSE_TIME)
        {
#ifdef LOG_L
            printf("[WHY] %s is paused for %d sec\n", toString(a),
            PAUSE_TIME - (int)difftime(t, alertState[a].t[alertState[a].e]));
#endif
            alertState[a].discard++;
            result = PAUSED; // discard Alert
        }
        else
        {
            initAlertState(a);
            addAlert(a, t);
            alertState[a].send++;
            result = OK; // send Alert
        }
    }
#ifdef LOG_L
    printAlertState(a);
#endif
    return result;
}

int sendEmail(Alert a)
{
    char currentTimeStr[128];
    time_t currentTime = time(NULL);

    struct tm CurrentTimeStructure;
    localtime_s(&CurrentTimeStructure, &currentTime);
    strftime(currentTimeStr, 128, "%Y-%m-%d %T", &CurrentTimeStructure);

    if (checkState(a, currentTime) != OK)
    {
        // discard Alert
#ifdef LOG_M
        printf("[%s] Email discarded: %s (remain time: %d sec)\n", 
        currentTimeStr, toString(a),
        PAUSE_TIME - (int)difftime(currentTime, alertState[a].t[alertState[a].e]));
#else
        printf("[%s] Email discarded: %s\n", currentTimeStr, toString(a));
#endif
        return 0; 
    }
    else
    {
        // send Alert
        printf("[%s] Email sent: %s\n", currentTimeStr, toString(a));
        return 1; // send the alert
    }
}

int alert(Alert a)
{
    sendEmail(a);
    //sendSMS(a); // could have different requirement

    return 0;
}

#ifdef TEST
// for testcase
// a: alert type
// r: repeated times
// d: duration in milisecond
int alerts(Alert a, int r, int d)
{
    int s;
    
    if (d == 0) s = 0;
    else s = (int)(d / r);

    for (int i = 0; i < r; i++)
    {
        alert(a);
        Sleep(s);
    }
}

void printRequirement()
{
    printf("\nMAX_ALERTS : %d\n", MAX_ALERTS);
    printf("PERIOD     : %d sec\n", PERIOD);
    printf("PAUSE_TIME : %d sec\n\n", PAUSE_TIME);
}

void testCase1()
{
    printRequirement();
    for (int i = 0; i < 10; i++)
    {
        alerts(ALERT_TYPE_WARNING, 1, 0);
        alerts(ALERT_TYPE_ERROR, 1, 0);
        alerts(ALERT_TYPE_CRITICAL, 1, 0);
        Sleep(PERIOD * 1000 / (MAX_ALERTS - 1) );
    }
}

void testCase2()
{
    printRequirement();
    for (int i = 0; i < 50; i++)
    {
        alerts(ALERT_TYPE_WARNING, 1, 0);
        alerts(ALERT_TYPE_ERROR, 1, 0);
        alerts(ALERT_TYPE_CRITICAL, 1, 0);
        Sleep(PERIOD * 1000 / (MAX_ALERTS + 1));
    }
}

void testCase3()
{
    printRequirement();
    for (int i = 0; i < 50; i++)
    {
        alerts(ALERT_TYPE_WARNING, 2, PERIOD);
        alerts(ALERT_TYPE_ERROR, 3, PERIOD);
        alerts(ALERT_TYPE_CRITICAL, MAX_ALERTS, PERIOD);

        alerts(ALERT_TYPE_WARNING, 1, 0);
        alerts(ALERT_TYPE_ERROR, 1, 0);
        alerts(ALERT_TYPE_CRITICAL, 1, 0);

        Sleep(PERIOD * 1000);
        alerts(ALERT_TYPE_WARNING, MAX_ALERTS, PERIOD);
        alerts(ALERT_TYPE_ERROR, 3, PERIOD);
        alerts(ALERT_TYPE_CRITICAL, 1, PERIOD);

        alerts(ALERT_TYPE_WARNING, 1, 0);
        alerts(ALERT_TYPE_ERROR, 1, 0);
        alerts(ALERT_TYPE_CRITICAL, 1, 0);
    }
}
#endif

int main(int argc, char** argv)
{
    startState();

    /*
    for (int i = 0; i < 100; i++)
    {
        alert(ALERT_TYPE_WARNING);
        Sleep(1000);
    }
    */

#ifdef TEST    
    // there is no over sending within PERIOD
    testCase1();
    testCase2();
    testCase3();
#endif

#ifdef LOG_H
    printStatistics();
#endif

    return 0;
}
