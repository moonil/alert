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
#define SCALE 1/30 // for making test faster
#else
#define SCALE 1
#endif

// Pause Config Options - Units in seconds
#define MAX_ALERTS 5
const int PauseTime = 300*SCALE;
const int Period = 60*SCALE;


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

    int total, send, pause; // Statics

} AlertState;

// state of alerts
AlertState alertState[NUM_TYPE_ALERTS];

const char* toString(Alert a)
{
    return Alert_Strings[a];
}

// initialize state of alert
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

// initialize statistics of alert
void initStatistics(Alert a)
{
#ifdef LOG_L
    printf("initStatistics: %s\n", toString(a));
#endif

    alertState[a].total = 0;
    alertState[a].send = 0;
    alertState[a].pause = 0;
}

// initialize all state
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
        
        printf("[%s] total: %d, send: %d, paused: %d\n", 
            toString(a), 
            alertState[a].total,
            alertState[a].send,
            alertState[a].pause);
    }
}

// adde alerts to be sent
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

// remove alerts from circular array of state
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

// check if alert can be sent
Status checkState(Alert a, time_t t)
{   
    Status result;

    alertState[a].total++;
    if (!alertState[a].isPaused)
    {
        if (alertState[a].n == MAX_ALERTS)
        {
            if ((int)difftime(t, alertState[a].t[alertState[a].s]) 
                < Period)
            {
#ifdef LOG_L
                printf("[WHY] too much %s over MAX_ALERTS(%d) in Period(%d sec)\n", 
                toString(a), MAX_ALERTS, Period);
#endif
                alertState[a].isPaused = TRUE;
                alertState[a].pause++;

                result = MAX;  // pause Alert
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
            < PauseTime)
        {
#ifdef LOG_L
            printf("[WHY] %s is paused for %d sec\n", toString(a),
            PauseTime - (int)difftime(t, alertState[a].t[alertState[a].e]));
#endif
            alertState[a].pause++;
            result = PAUSED; // paused Alert
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

// try to send alert
int sendEmail(Alert a)
{
    char currentTimeStr[128];
    time_t currentTime = time(NULL);

    struct tm CurrentTimeStructure;
    localtime_s(&CurrentTimeStructure, &currentTime);
    strftime(currentTimeStr, 128, "%Y-%m-%d %T", &CurrentTimeStructure);

    if (checkState(a, currentTime) != OK)
    {
        // pause Alert
#ifdef LOG_M
        printf("[%s] Email paused: %s (remaining time: %d sec)\n", 
        currentTimeStr, toString(a),
        PauseTime - (int)difftime(currentTime, alertState[a].t[alertState[a].e]));
#else
        printf("[%s] Email paused: %s\n", currentTimeStr, toString(a));
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

// trigger alert
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
    else s = (int)(d / r); // caculate slepp time

    for (int i = 0; i < r; i++)
    {
        alert(a);
        Sleep(s);
    }
}

void printRequirement()
{
    printf("\nMAX_ALERTS : %d\n", MAX_ALERTS);
    printf("Period     : %d sec\n", Period);
    printf("PauseTime : %d sec\n\n", PauseTime);
}

// case 1: send fewer alerts than MAX_ALERTS within Period
void testCase1()
{
    printRequirement();
    for (int i = 0; i < 100; i++)
    {
        alert(ALERT_TYPE_WARNING);
        alert(ALERT_TYPE_ERROR);
        alert(ALERT_TYPE_CRITICAL);
        Sleep(Period * 1000 / (MAX_ALERTS - 1) );
    }
}

// case 2: send more alerts than MAX_ALERTS within Period
void testCase2()
{
    printRequirement();
    for (int i = 0; i < 100; i++)
    {
        alert(ALERT_TYPE_WARNING);
        alert(ALERT_TYPE_ERROR);
        alert(ALERT_TYPE_CRITICAL);
        Sleep(Period * 1000 / (MAX_ALERTS + 1));
    }
}

// mixed case
void testCase3()
{
    printRequirement();
    for (int i = 0; i < 50; i++)
    {
        alerts(ALERT_TYPE_WARNING, 2, Period);
        alerts(ALERT_TYPE_ERROR, 3, Period);
        alerts(ALERT_TYPE_CRITICAL, MAX_ALERTS, Period);

        alerts(ALERT_TYPE_WARNING, 1, 0);
        alerts(ALERT_TYPE_ERROR, 1, 0);
        alerts(ALERT_TYPE_CRITICAL, 1, 0);

        Sleep(Period * 1000);
        alerts(ALERT_TYPE_WARNING, MAX_ALERTS, Period);
        alerts(ALERT_TYPE_ERROR, 3, Period);
        alerts(ALERT_TYPE_CRITICAL, 1, Period);

        alerts(ALERT_TYPE_WARNING, 1, 0);
        alerts(ALERT_TYPE_ERROR, 1, 0);
        alerts(ALERT_TYPE_CRITICAL, 1, 0);
    }
}
#endif

int main(int argc, char** argv)
{
    // initalize state 
    startState();

#ifndef TEST
#error need integration with real system
#else
    testCase1(); // fewer alerts than MAX_ALERTS within Period
    testCase2(); // more alerts than MAX_ALERTS within Period
    testCase3(); // mixed case 
#endif

#ifdef LOG_H
    printStatistics();
#endif

    return 0;
}
