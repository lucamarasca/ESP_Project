//RACom Branch FREERTOS
#ifndef RACom_H	//tests if RACom_H has not been defined
#define RACom_H	//define RACom_H

#include "Arduino.h"	//includes the library Arduino.h
#include "SoftwareSerial.h"	 //Includes the library SoftwareSerial.h
//#include <NeoSWSerial.h> 
//#include <limits.h>

/* Kernel includes. */
#include "Arduino_FreeRTOS.h"
#include "timers.h"     
    
//an enum with constants
enum
{
	//rate of trnsmission
    BAUND_RATE = 9600,
	//pin used
    RX = 8,
    TX = 6,
    SET_PIN = 13,
    RING_ROUND_TRIP_TIMEOUT = 10000 / portTICK_PERIOD_MS, // 10 sec for test
    RESPONSE_TIMEOUT = 400 / portTICK_PERIOD_MS, // 400 millisec for test
    NUM_NEXT_POS = 8,
    SPECIAL_ANT_ID = 3,
    BUFFER_DIM = 60,
	//delay applied to a task
    TASK_DELAY = 10
};
//Define RACom class with relatives methods
class RACom {
public:
    void init(byte id);
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
    void setAntMode(byte mode); // 0 = default ant mode
    byte getAntMode();
    void setMyCurrentPosition(byte pos);
    byte getCurrentPosOfAnt(byte num_ant);
	void Hello();
	void HelloRecive();
	void HelloResponse();

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