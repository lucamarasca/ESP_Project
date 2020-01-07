// Branch RAComFreeRTOS
//Includes Racom.h into this file
#include "RACom.h"
//Initialize serial communication on the pin RX & TX
//static SoftwareSerial MySerial (RX, TX); //RX = 8 & TX = 6
static SoftwareSerial MySerial (TX, RX);
//static NeoSWSerial MySerial (RX, TX);

static byte initFlag;
static byte MY_ID;	//ID of this ANT
static byte NUM_ANTS = 1; // Number of ants in the antNet (Init = 1 for the master)
static byte currSucc; //Id of the next ANT
static byte _bufsize; //Size of the buffer
static char _buffer[BUFFER_DIM]; //Buffer of chracter <--We have to work on this
static byte ID_LIST[ID_LIST_SIZE];

//Those are the 2 timer that are used for understand if an ANTS is dead. 
/* FreeRtos Staff */
TimerHandle_t xGlobalTimer;
TimerHandle_t xResponseTimer;

//Condition if a timer expired
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
//Handler of two task that will be executed
TaskHandle_t* taskRGB;
TaskHandle_t* taskMotion;
//if a task is resumed or not
static bool resumedTasks;
static bool flagHello;

static byte startAndStop; // 0 = stop, 1 = start
static byte myCurrentPosition; // my current pos to brodcast
static byte antMode; //Command to select ant mode; 0 = default random mode

static byte currPos1 = 225; // received current pos from outside ant 1
static byte currPos2 = 225; // received current pos from outside ant 2
static byte currPos3 = 225; // received current pos from outside ant 3
static byte currPos4 = 225; // received current pos from outside ant 4
static byte currPos5 = 225; // received current pos from outside ant 5

//This is the init method, the first method that has to be called for run the comunication
void RACom::init(byte id) {
//Set the data rate for the software serial port
    MySerial.begin(BAUND_RATE);
//Wait for serial port to connect
    while(!MySerial);
//print that the serial communication start at the relative BAUND_RATE
    Serial.print(F("Wireless module serial started at "));
    Serial.println(BAUND_RATE);
//Set the pin 13 as output
    pinMode(SET_PIN, OUTPUT); // Connected to set input
//Initialize constants
    MY_ID = id;
//Add myself to ant list
ID_LIST[id-1] = id;
	flagHello = false;
    currSucc = MY_ID;
//Set the buffer's size  
    _bufsize = sizeof _buffer;
// flush the buffer
    //_buffer[0] = '\0'; 
// initialize the buffer adding 0 in all the positions
    memset(_buffer, 0, _bufsize);

// Start software timers
    initFlag = 0;	//This flag is used for understand if an has been initialized or not
    resumedTasks = false;
    globalTimer_expired = false;	//if global timer expired or not
    responseTimer_expired = false;
    startAndStop = 1;
    antMode = 0;
    myCurrentPosition = 225;
}
//Set my current position 


//Send the hello message
void RACom::Hello() {
  Serial.print(F("<--- Hello Message Sent: "));
    // Wireless send
  MySerial.print('H'); // start char hello
  Serial.print('H'); // start char hello
  MySerial.print('#'); // separator
  Serial.print('#');
  MySerial.print(MY_ID); // mit
  Serial.print(MY_ID);
  MySerial.print('$'); // end char
  AckWait();  
}

//Receive the hello message
void RACom::HelloWait() {
	
//If I read the Hello Message
  if((char)MySerial.read() == 'H') {
	MySerial.readBytesUntil('$', _buffer, _bufsize);
	 int mit;	//ID
	 char copy[2]; //2 is hello message length
   	 strncpy(copy, _buffer, 2);
	 
	 //split string into tokens , suing the delimitator # and retrieve mit
        char * pch = strtok(copy, "#");
        mit = atoi(pch);    
		byte mitb = mit;		

      //Check that ID of ant has not already been added to ID_List
      if (ID_LIST[mit - 1] == 0)	  {
	     //Add id in id-th - 1 position
	     ID_LIST[mit - 1] = mitb;
	     NUM_ANTS++;		
	      	
	      Serial.println(F("HELLO Message received from ANT: "));
	      Serial.println(mit);
	      Ack(mit);
	    }   	 
    }
}



//Receive the Ack of Hello Message --> Ant has been accepted into the network.
//Ant receives list of current members of the network.
void RACom::AckWait() {
	//If i read the start symbol
      if((char)MySerial.read() == 'A') { //hello answer
	    int mit; //ack mit
		int dest; //ack dest
		int nants; //ack number of ants
		
	   //Parse message to check if I'm the dest
	    MySerial.readBytesUntil('$', _buffer, _bufsize);
	    Serial.println(_bufsize);
		size_t bufsize = sizeof(_buffer);
		char copy[bufsize];
        strncpy(copy, _buffer, bufsize);
        copy[bufsize-1] = '\0';
	    char * pch = strtok(copy, "#");
        int i = 0;
		 
		  while (pch != NULL) {
      // Frame example: mit # dest # n_ants # ant a # ant b ... # ant z $
      // Frame example: 3#1#3#1#2#3$
    
        if(i == 0) {
	     mit = atoi(pch);
		}

      if(i == 1) {
		dest = atoi(pch);
	    }

      if(i == 2) {
		 nants = atoi(pch);
	    }
	  if(i >= 3 && i <= (ID_LIST_SIZE + 3)) {
		  int antId; 
		  antId = atoi(pch);
		  //If ack is for me
		  // Add ants from ack to Id list
		  if(dest == MY_ID){
		  ID_LIST[antId-1] = antId; }
      }
      pch = strtok (NULL, "#");
      i++;
    }
	//If ack is for me
		if(dest == MY_ID){
	     Serial.println(F("ACK received: "));
         Serial.print(_buffer);
		 NUM_ANTS = nants;
		 memset(_buffer, 0, _bufsize);
		flagHello = true; //Stop sending hello messages
		}  
      }
     
}

//Send Ack for Hello Message
void RACom::Ack(int dest) {
	Serial.println("Ack Sent: ");
	MySerial.print('A'); // start char helloanswer
	Serial.print('A');
	MySerial.print(MY_ID); // mit
	Serial.print(MY_ID);
	MySerial.print('#'); // separator
	Serial.print('#');
	MySerial.print(dest); // dest of ack (ant that sent hello)
    Serial.print(dest);
	MySerial.print('#'); // separator
	Serial.print('#');
	MySerial.print(NUM_ANTS); // number of ants
	Serial.print(NUM_ANTS);
    // list of ants in the network
     for(int i = 1; i < ID_LIST_SIZE; i++) {
     if(ID_LIST[i-1] != 0){
		Serial.print('#');
		MySerial.print('#');
	    Serial.print(i);
		MySerial.print(i); // separator
     }

  }
	MySerial.print('$'); // end char
}
//This method is used for set the 13s pin as HIGH (5V)
void RACom::comunicationMode() {
  digitalWrite(SET_PIN, HIGH);
  //analogWrite(SET_PIN, 255);
}
//this method is used for set the 13s pin as LOW (0V)
void RACom::commandMode() {
  digitalWrite(SET_PIN, LOW);
  //analogWrite(SET_PIN, 0);
}
//This method is used for test the connection (non viene mai chiamato, perche viene utilizzato solo in ambito di testing)
void RACom::testCom() {
  if(MySerial.available()) {            // If HC-12 has data
    Serial.write(MySerial.read());      // Send the data to Serial monitor
  }
  if(Serial.available()) {              // If Serial monitor has data
    MySerial.write(Serial.read());      // Send that data to HC-12
  }
}
//This method execute all the operation that has to be executed in the broadcastphase
void RACom::broadcastPhase() {
//this variable is used for understand if it's my turn or not for communicating 
 bool isMyTurn;
  do 
  {
    isMyTurn = false;
//Find the next ANTS which has to communicate
    findMyNext();
//send information about my status to the next node
    broadcast();
//This method start the response timer
    startResponseTimer();
    //_buffer[0] = '\0';
//Initialize the buffer 
    memset(_buffer, 0, _bufsize);

    // iterate until response timeout is not expired
    while( !responseTimer_expired ) {
		//if port 13 is avaiable
      if(MySerial.available()) {
        //if i read a @, than i'll read until $ or until he reach buffsize and it put inside the buffer the data's 
        if((char)MySerial.read() == '@') {
          MySerial.readBytesUntil('$', _buffer, _bufsize);
          break;
        }
   
      }
    }
	//if it's my turn
    if(setRecvPosArray() == MY_ID) {
      currSucc = MY_ID;
      isMyTurn = true;
    } 
	//block the task for 10
    vTaskDelay( TASK_DELAY );
	//Print the message recived
    Serial.print(F("<--- Message received after broadcast: "));
    Serial.println(_buffer);
    
  } 
  while(strlen(_buffer) == 0 || isMyTurn == true);

  // At the end of brodcast phase, restart the global timer
  startGlobalTimer();
}
//this is the third and the last method that has to be launched
void RACom::comAlgo() {
  if(initFlag == 0) {
    MySerial.flush();
    startGlobalTimer();
    initFlag = 1;
  }
  
  //If I am not the master, and if I haven't received a response to my Hello
  if(MY_ID != SPECIAL_ANT_ID && flagHello == false){
		  Hello();
	} 
  
  //Receive hello message
  HelloWait();
	  
	   
	  
  // Global timeout
  if(!globalTimer_expired) {
    // Read phase
    if(MySerial.available()) {
		//If i read the start symbol
      if((char)MySerial.read() == '@') {
		  //read the content trasmitted until i reach $ or length
        MySerial.readBytesUntil('$', _buffer, _bufsize);
        //print the message recived
        Serial.print(F("<--- Message received: "));
        Serial.println(_buffer);
		//start the broadcast phase
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
	// if there's only one ant
    broadcastPhase();
  }
	//apply delay of 10
  vTaskDelay( TASK_DELAY ); 
  
}
//not getting called
void RACom::setNextPosArray(byte replace[]) {
  for(int i = 0; i < NUM_NEXT_POS; i++) {
    nextPositions[i] = replace[i];
  }
}
//not getting called
byte* RACom::getRecvPosArray(byte num_ant) {
  if(num_ant == 1) return recvPos1;
  if(num_ant == 2) return recvPos2;
  if(num_ant == 3) return recvPos3;
  if(num_ant == 4) return recvPos4;
  if(num_ant == 5) return recvPos5;
}
//Set the handle of the tasks
void RACom::setTaskHandle(TaskHandle_t* xHandleRGB, TaskHandle_t* xHandleMotion) {
  taskRGB = xHandleRGB;
  taskMotion = xHandleMotion;
}
//Set the startandstop state
void RACom::setStartAndStop(byte state) {
  startAndStop = state;
}
//get the start and stop state
byte RACom::getStartAndStop() {
  return startAndStop;
}

//set the antMode state
void RACom::setAntMode(byte mode) {
  antMode = mode;
}

//get the antMode state
byte RACom::getAntMode() {
  return antMode;
}

//Set my current position 
void RACom::setMyCurrentPosition(byte pos) {
  myCurrentPosition = pos;
}
//get the position of the five ants 
byte RACom::getCurrentPosOfAnt(byte num_ant) {
  if(num_ant == 1) return currPos1;
  if(num_ant == 2) return currPos2;
  if(num_ant == 3) return currPos3;
  if(num_ant == 4) return currPos4;
  if(num_ant == 5) return currPos5;
}
//Find the next node that has to be called 
//L'ho commentato perch� e sbagliato
/*
void RACom::findMyNext() {
	
		currSucc++;

	  if(NUM_ANTS == 2 || MY_ID == NUM_ANTS) {
		  //Significa che sono l'ultimo
		if(currSucc >= NUM_ANTS) currSucc = 0; 
	  }
	  else {
		if(currSucc > NUM_ANTS) currSucc = 1; 
	  }
	  
	  if(currSucc == MY_ID) currSucc++;
	
	
}
*/
void RACom::findMyNext() {
	currSucc = 99;
	for(int i = 0; i < ID_LIST_SIZE; i++){
		if (MY_ID < ID_LIST[i] && currSucc > ID_LIST)
			currSucc = ID_LIST[i];
	}
	
	
}
//send information about my status to the next node
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

  MySerial.print(antMode); // ant Mode
  Serial.print(antMode);

  MySerial.print('#'); // separator
  Serial.print('#');

  MySerial.print(myCurrentPosition); // current position
  Serial.print(myCurrentPosition);

  MySerial.print('$'); // end char

}
//this method is used for store the message recived
int RACom::setRecvPosArray() {
	
  if( strlen(_buffer) != 0  ) {
	byte mode; 
    int mit;	//ID
    int ss;		//start-stop
    int succ;	//Id of the next node
	//copy inside an array fo character, the content of the buffer
    char copy[BUFFER_DIM];
    size_t len = sizeof(copy);
    strncpy(copy, _buffer, len);
    copy[len-1] = '\0';
	//divide the string into a series of token , suing the delimitator #
    char * pch = strtok(copy, "#");
    int i = 0;
    
    while (pch != NULL) {
      // Frame example: @1#2#225#225#225#225#225#225#225#225#1#0#225$
      // @ mit # succ # next_pos # next_pos # next_pos # next_pos # next_pos # next_pos # next_pos # next_pos # start_stop # antMode # current_pos $

      if(i == 0) {
		  //convert pch in int
        mit = atoi(pch);
      }

      if(i == 1) {
		  //put into succ the id of the next node
        succ = atoi(pch);
      }

      if(i >= 2 && i <= 9) {
		  //put inside recvpos the tokens
        if(mit == 1) recvPos1[i - 2] = (byte) atoi(pch);
        if(mit == 2) recvPos2[i - 2] = (byte) atoi(pch);
        if(mit == 3) recvPos3[i - 2] = (byte) atoi(pch);
        if(mit == 4) recvPos4[i - 2] = (byte) atoi(pch);
        if(mit == 5) recvPos5[i - 2] = (byte) atoi(pch);
      }

      if(i == 10 && mit == SPECIAL_ANT_ID) {
		//put inside ss start and stop condition
        ss = atoi(pch);
        
        if(ss == 0) {
          startAndStop = (byte) ss;
        }
        else if(ss == 1) {
          startAndStop = (byte) ss;
        }
      }

       if(i == 11 && mit == SPECIAL_ANT_ID) {
        mode = atoi(pch);
        antMode = (byte) mode;      
      }

      if(i == 12) {
        if(mit == 1) currPos1 = (byte) atoi(pch);
        if(mit == 2) currPos2 = (byte) atoi(pch);
        if(mit == 3) currPos3 = (byte) atoi(pch);
        if(mit == 4) currPos4 = (byte) atoi(pch);
        if(mit == 5) currPos5 = (byte) atoi(pch);
      }

      pch = strtok (NULL, "#");
      i++;
    }
	//return the id of the next ANT
    return succ;

  } else {
    return NUM_ANTS + 1; // not existing ANT
  }
} 
//not getting called
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

//This method is used for starting the global timer 
void RACom::startGlobalTimer() {
  Serial.print('\n');
  Serial.println(F("G. timer started"));
  globalTimer_expired = false;
  //Starts the global timer (which timer, how many time has to wait)
  xTimerStart( xGlobalTimer, 0 );
}
//Starts a timer that is used for take trak of the response
void RACom::startResponseTimer() {
  Serial.print('\n');
  Serial.println(F("R. timer started"));
  responseTimer_expired = false;
  xTimerStart( xResponseTimer, 0 );
}
//when we have a callback to the global timer that means that it has expired
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
//when we have a callback to the reponse timer that means that it has expired
void RACom::responseTimerCallback( TimerHandle_t xTimer ) {
  Serial.print('\n');
  Serial.println(F("Response Timer Expired"));
	responseTimer_expired = true;
}