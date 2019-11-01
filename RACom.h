//RACom Branch FREERTOS
#ifndef RACom_H
#define RACom_H

#include "Arduino.h"
#include "SoftwareSerial.h"
//#include <NeoSWSerial.h>
//#include <limits.h>

/* Kernel includes. */
#include "Arduino_FreeRTOS.h"
#include "timers.h"         

enum
{
    BAUND_RATE = 9600,
    RX = 8,
    TX = 6,
    SET_PIN = 13,
    RING_ROUND_TRIP_TIMEOUT = 10000 / portTICK_PERIOD_MS, // 10 sec for test
    RESPONSE_TIMEOUT = 400 / portTICK_PERIOD_MS, // 400 millisec for test
    NUM_NEXT_POS = 8,
    SPECIAL_ANT_ID = 3,
    BUFFER_DIM = 60,
    TASK_DELAY = 10
};

class RACom {
public:
    void init(byte id, byte number_of_ants);
    void comunicationMode();
    void commandMode();
    void testCom();
    void comAlgo();
    void setupTimers();
    void setNextPosArray(byte replace[]);
    byte* getRecvPosArray(byte num_ant);
    void setTaskHandle(TaskHandle_t* xHandleRGB, TaskHandle_t* xHandleMotion);
    void setStartAndStop(byte state); // 0 = stop, 1 = start
    byte getStartAndStop();
    void setMyCurrentPosition(byte pos);
    byte getCurrentPosOfAnt(byte num_ant);

private:
    // methods for comAlgo
    void broadcastPhase();
    void findMyNext();
    void broadcast();
    int setRecvPosArray();
    void resetNextPosArray();

    void startGlobalTimer();
    void startResponseTimer();
    static void globalTimerCallback(TimerHandle_t xTimer);
    static void responseTimerCallback(TimerHandle_t xTimer);
};

#endif