// Branch RAComFreeRTOS
#include "RACom.h"
static SoftwareSerial MySerial (RX, TX);
//static NeoSWSerial MySerial (RX, TX);

static byte initFlag;
static byte MY_ID;
static byte NUM_ANTS; // Number of ants in the antNet
static byte currSucc;
static byte _bufsize;
static char _buffer[BUFFER_DIM];

/* FreeRtos Staff */
TimerHandle_t xGlobalTimer;
TimerHandle_t xResponseTimer;

static bool globalTimer_expired;
static bool responseTimer_expired;

// Array of next positions 
static byte nextPositions[NUM_NEXT_POS] = { 225, 225, 225, 225, 225, 225, 225, 225  }; // my next pos to brodcast

// One array for each ant
static byte recvPos1[NUM_NEXT_POS] = { 225, 225, 225, 225, 225, 225, 225, 225  }; // received next pos from outside ant 1
static byte recvPos2[NUM_NEXT_POS] = { 225, 225, 225, 225, 225, 225, 225, 225  }; // received next pos from outside ant 2
static byte recvPos3[NUM_NEXT_POS] = { 225, 225, 225, 225, 225, 225, 225, 225  }; // received next pos from outside ant 3
static byte recvPos4[NUM_NEXT_POS] = { 225, 225, 225, 225, 225, 225, 225, 225  }; // received next pos from outside ant 4
static byte recvPos5[NUM_NEXT_POS] = { 225, 225, 225, 225, 225, 225, 225, 225  }; // received next pos from outside ant 5

// Task RGB and motion pointers
TaskHandle_t* taskRGB;
TaskHandle_t* taskMotion;
static bool resumedTasks;

static byte startAndStop; // 0 = stop, 1 = start
static byte myCurrentPosition; // my current pos to brodcast

static byte currPos1 = 225; // received current pos from outside ant 1
static byte currPos2 = 225; // received current pos from outside ant 2
static byte currPos3 = 225; // received current pos from outside ant 3
static byte currPos4 = 225; // received current pos from outside ant 4
static byte currPos5 = 225; // received current pos from outside ant 5


void RACom::init(byte id, byte number_of_ants) {
    MySerial.begin(BAUND_RATE);
    while(!MySerial);
    
    Serial.print(F("Wireless module serial started at "));
    Serial.println(BAUND_RATE);

    pinMode(SET_PIN, OUTPUT); // Connected to set input

    MY_ID = id;
    NUM_ANTS = number_of_ants;
    currSucc = MY_ID;
    
    _bufsize = sizeof _buffer;
    //_buffer[0] = '\0'; // flush the buffer
    memset(_buffer, 0, _bufsize);

    // Start softweare timers
    initFlag = 0;
    resumedTasks = false;
    globalTimer_expired = false;
    responseTimer_expired = false;
    startAndStop = 1;
    myCurrentPosition = 225;
}

void RACom::comunicationMode() {
  digitalWrite(SET_PIN, HIGH);
  //analogWrite(SET_PIN, 255);
}

void RACom::commandMode() {
  digitalWrite(SET_PIN, LOW);
  //analogWrite(SET_PIN, 0);
}

void RACom::testCom() {
  if(MySerial.available()) {            // If HC-12 has data
    Serial.write(MySerial.read());      // Send the data to Serial monitor
  }
  if(Serial.available()) {              // If Serial monitor has data
    MySerial.write(Serial.read());      // Send that data to HC-12
  }
}

void RACom::broadcastPhase() {
  bool isMyTurn;
  do 
  {
    isMyTurn = false;
    findMyNext();
    broadcast();
    startResponseTimer();
    //_buffer[0] = '\0';
    memset(_buffer, 0, _bufsize);

    // iterate until response timeout is not expired
    while( !responseTimer_expired ) {
      if(MySerial.available()) {
        
        if((char)MySerial.read() == '@') {
          MySerial.readBytesUntil('$', _buffer, _bufsize);
          break;
        }

      }
    }

    if(setRecvPosArray() == MY_ID) {
      currSucc = MY_ID;
      isMyTurn = true;
    } 

    vTaskDelay( TASK_DELAY );
    Serial.print(F("<--- Message received after broadcast: "));
    Serial.println(_buffer);
    
  } 
  while(strlen(_buffer) == 0 || isMyTurn == true);

  // At the end of brodcast phase, restart the global timer
  startGlobalTimer();
}

void RACom::comAlgo() {
  if(initFlag == 0) {
    MySerial.flush();
    startGlobalTimer();
    initFlag = 1;
  }
  
  // Global timeout
  if(!globalTimer_expired) {
    // Read phase
    if(MySerial.available()) {

      if((char)MySerial.read() == '@') {
        MySerial.readBytesUntil('$', _buffer, _bufsize);
        
        Serial.print(F("<--- Message received: "));
        Serial.println(_buffer);

        if(setRecvPosArray() == MY_ID) {
          currSucc = MY_ID;
          broadcastPhase();
        }
          
        //_buffer[0] = '\0';
        memset(_buffer, 0, _bufsize);
      }

    }
  }
  else {
    // I'm the only one in the network
    broadcastPhase();
  }

  vTaskDelay( TASK_DELAY ); 
  
}

void RACom::setNextPosArray(byte replace[]) {
  for(int i = 0; i < NUM_NEXT_POS; i++) {
    nextPositions[i] = replace[i];
  }
}

byte* RACom::getRecvPosArray(byte num_ant) {
  if(num_ant == 1) return recvPos1;
  if(num_ant == 2) return recvPos2;
  if(num_ant == 3) return recvPos3;
  if(num_ant == 4) return recvPos4;
  if(num_ant == 5) return recvPos5;
}

void RACom::setTaskHandle(TaskHandle_t* xHandleRGB, TaskHandle_t* xHandleMotion) {
  taskRGB = xHandleRGB;
  taskMotion = xHandleMotion;
}

void RACom::setStartAndStop(byte state) {
  startAndStop = state;
}

byte RACom::getStartAndStop() {
  return startAndStop;
}

void RACom::setMyCurrentPosition(byte pos) {
  myCurrentPosition = pos;
}

byte RACom::getCurrentPosOfAnt(byte num_ant) {
  if(num_ant == 1) return currPos1;
  if(num_ant == 2) return currPos2;
  if(num_ant == 3) return currPos3;
  if(num_ant == 4) return currPos4;
  if(num_ant == 5) return currPos5;
}

void RACom::findMyNext() {
  currSucc++;

  if(NUM_ANTS == 2 || MY_ID == NUM_ANTS) {
    if(currSucc >= NUM_ANTS) currSucc = 1; 
  }
  else {
    if(currSucc > NUM_ANTS) currSucc = 1; 
  }
  
  if(currSucc == MY_ID) currSucc++;
}

void RACom::broadcast() {
  Serial.print(F("<--- Message Sent: "));
  
  // Wireless send
  MySerial.print('@'); // start char
  
  MySerial.print(MY_ID); // mit
  Serial.print(MY_ID);
  
  MySerial.print('#'); // separator
  Serial.print('#');

  MySerial.print(currSucc); // succ
  Serial.print(currSucc);
  
  MySerial.print('#'); // separator
  Serial.print('#');
  
  // next positions
  for(int i = 0; i < NUM_NEXT_POS; i++) {
    MySerial.print(nextPositions[i]); 
    Serial.print(nextPositions[i]);
    
    MySerial.print('#');
    Serial.print('#');
  }

  MySerial.print(startAndStop); // start and stop
  Serial.print(startAndStop);

  MySerial.print('#'); // separator
  Serial.print('#');

  MySerial.print(myCurrentPosition); // current position
  Serial.print(myCurrentPosition);

  MySerial.print('$'); // end char

}

int RACom::setRecvPosArray() {
  if( strlen(_buffer) != 0  ) {
    int mit;
    int ss;
    int succ;

    char copy[BUFFER_DIM];
    size_t len = sizeof(copy);
    strncpy(copy, _buffer, len);
    copy[len-1] = '\0';

    char * pch = strtok(copy, "#");
    int i = 0;
    
    while (pch != NULL) {
      // Frame example: @1#2#225#225#225#225#225#225#225#225#1#225$
      // @ mit # succ # next_pos # next_pos # next_pos # next_pos # next_pos # next_pos # next_pos # next_pos # start_stop # current_pos $

      if(i == 0) {
        mit = atoi(pch);
      }

      if(i == 1) {
        succ = atoi(pch);
      }

      if(i >= 2 && i <= 9) {
        if(mit == 1) recvPos1[i - 2] = (byte) atoi(pch);
        if(mit == 2) recvPos2[i - 2] = (byte) atoi(pch);
        if(mit == 3) recvPos3[i - 2] = (byte) atoi(pch);
        if(mit == 4) recvPos4[i - 2] = (byte) atoi(pch);
        if(mit == 5) recvPos5[i - 2] = (byte) atoi(pch);
      }

      if(i == 10 && mit == SPECIAL_ANT_ID) {
        ss = atoi(pch);
        
        if(ss == 0) {
          startAndStop = (byte) ss;
        }
        else if(ss == 1) {
          startAndStop = (byte) ss;
        }
      }

      if(i == 11) {
        if(mit == 1) currPos1 = (byte) atoi(pch);
        if(mit == 2) currPos2 = (byte) atoi(pch);
        if(mit == 3) currPos3 = (byte) atoi(pch);
        if(mit == 4) currPos4 = (byte) atoi(pch);
        if(mit == 5) currPos5 = (byte) atoi(pch);
      }

      pch = strtok (NULL, "#");
      i++;
    }

    return succ;

  } else {
    return NUM_ANTS + 1; // not existing ANT
  }
} 

void RACom::resetNextPosArray() {
  for(int i = 0; i < NUM_NEXT_POS; i++) {
    nextPositions[i] = 225;
  }
}

void RACom::setupTimers() {
  Serial.println(F("Setup timers"));

  xGlobalTimer = xTimerCreate(
        "Global_Timer",               /* A text name, purely to help debugging. */
        ( RING_ROUND_TRIP_TIMEOUT ),  /* The timer period. */
		    pdFALSE,						          /* This is an auto-reload timer, so xAutoReload is set to pdTRUE. */
		    ( void * ) 0,				          /* The ID is not used, so can be set to anything. */
		    globalTimerCallback           /* The callback function that inspects the status of all the other tasks. */
  );

  xResponseTimer = xTimerCreate(
        "Response_Timer",             /* A text name, purely to help debugging. */
        ( RESPONSE_TIMEOUT ),         /* The timer period. */
		    pdFALSE,						          /* This is an auto-reload timer, so xAutoReload is set to pdTRUE. */
		    ( void * ) 0,				          /* The ID is not used, so can be set to anything. */
		    responseTimerCallback         /* The callback function that inspects the status of all the other tasks. */
  );

  if(xGlobalTimer == NULL || xResponseTimer == NULL) {
    Serial.println(F("failure creating Timers"));
    for(;;);
  }
}

void RACom::startGlobalTimer() {
  Serial.print('\n');
  Serial.println(F("G. timer started"));
  globalTimer_expired = false;
  xTimerStart( xGlobalTimer, 0 );
}

void RACom::startResponseTimer() {
  Serial.print('\n');
  Serial.println(F("R. timer started"));
  responseTimer_expired = false;
  xTimerStart( xResponseTimer, 0 );
}

void RACom::globalTimerCallback( TimerHandle_t xTimer ) {
  Serial.print('\n');
  Serial.println(F("Global Timer Expired"));
	
  if(resumedTasks == false && taskRGB != NULL && taskMotion != NULL) {
    vTaskResume( *taskRGB );
    vTaskResume( *taskMotion );
    resumedTasks = true;
  }
  globalTimer_expired = true;
}

void RACom::responseTimerCallback( TimerHandle_t xTimer ) {
  Serial.print('\n');
  Serial.println(F("Response Timer Expired"));
	responseTimer_expired = true;
}