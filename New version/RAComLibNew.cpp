// Branch RAComFreeRTOS
//Includes Racom.h into this file
#include <RAComLibNew.h>
//Initialize serial communication on the pin RX & TX
//static SoftwareSerial MySerial (RX, TX); //RX = 8 & TX = 6
static SoftwareSerial MySerial(TX, RX);

static byte MY_ID;               //ID of this ANT
static byte _bufsize;            //Size of the buffer
static char _buffer[BUFFER_DIM]; //Buffer of chracters

static bool ANT_LIST[MAX_ANTS]; //List of ants in the networkvoid setup() {
// put your setup code here, to run once:

static byte succ;            //pointer to next ANT
static byte prev;            //pointer to prev ANT
static byte expected;    //mit of next expected message

/*Timers*/
TimerHandle_t xJoinTimer;
TimerHandle_t xSpecialTurnTimer;
TimerHandle_t xReplyTimer;
TimerHandle_t xStabilizationTimer;

/*semaphore*/
SemaphoreHandle_t xMutex = NULL;

//Condition if a timer expired
static bool joinTimer_expired;
static bool special_turnTimer_expired;
static bool replyTimer_expired;
static bool readable;

// Array of next positions
static byte nextPositions[NUM_NEXT_POS] = {225, 225, 225, 225, 225, 225, 225, 225}; // my next pos to brodcast

// One array for each ant
static byte recvPos1[NUM_NEXT_POS] = {225, 225, 225, 225, 225, 225, 225, 225}; // received next pos from outside ant 1
static byte recvPos2[NUM_NEXT_POS] = {225, 225, 225, 225, 225, 225, 225, 225}; // received next pos from outside ant 2
static byte recvPos3[NUM_NEXT_POS] = {225, 225, 225, 225, 225, 225, 225, 225}; // received next pos from outside ant 3
static byte recvPos4[NUM_NEXT_POS] = {225, 225, 225, 225, 225, 225, 225, 225}; // received next pos from outside ant 4
static byte recvPos5[NUM_NEXT_POS] = {225, 225, 225, 225, 225, 225, 225, 225}; // received next pos from outside ant 5

// Task RGB and motion pointers
//Handler of two task that will be executed
TaskHandle_t *taskBuffer;
TaskHandle_t *taskComm;
TaskHandle_t *taskRGB;
TaskHandle_t *taskMotion;
static bool isMyTurn; //Says if it's my turn to send the message

static byte startAndStop;      // 0 = stop, 1 = start
static byte myCurrentPosition; // my current pos to brodcast
static byte antMode;           //Command to select ant mode; 0 = default random mode

static byte currPos1 = 225; // received current pos from outside ant 1
static byte currPos2 = 225; // received current pos from outside ant 2
static byte currPos3 = 225; // received current pos from outside ant 3
static byte currPos4 = 225; // received current pos from outside ant 4
static byte currPos5 = 225; // received current pos from outside ant 5

void RACom::init(byte id)
{
	MySerial.begin(BAUD_RATE);
	while (!MySerial)
		;
	//print that the serial communication start at the relative BAUD_RATE
	Serial.print(F("Wireless module serial started at "));
	Serial.println(BAUD_RATE);
	pinMode(SET_PIN, OUTPUT);
	MY_ID = id;
	_bufsize = sizeof _buffer;   //Set the buffer's size
	// initialize the buffer adding 0 in all the positions
	memset(_buffer, 0, _bufsize);

	//Init AntList 
	for(int i = 0; i < MAX_ANTS; i++)
	{
		ANT_LIST[i] = false;
	}

	ANT_LIST[MY_ID] = true;
	prev = 100;
	succ = 100;
	isMyTurn = false;
	joinTimer_expired = false;
	special_turnTimer_expired = false;
	replyTimer_expired = false;
	readable = false;
	startAndStop = 1;
	antMode = 0;
	myCurrentPosition = 225;     
	xSemaphoreTake(xMutex, 0);
   //Ant joins the network
	//Join(); 
}

void RACom::comAlgo()
{  
//If I'm the only ant in the network
	if(succ == 100 && prev == 100)
	{ 
		Serial.print("\nOnly Ant ");
		Send();
		Special_Turn();
	}
	else
	{
		if(isMyTurn)
		{
			Send();
			if(succ == 100)
			{
				Special_Turn();
			}
		}
		else 
		{
			Receive();
		}
	}
}



//Ant is activated and joins the network
//Ant waits for max n seconds. If it doesn't receive any message, it's the first ant.
void RACom::Join()
{
	Serial.print(MY_ID);
	Serial.print(" <-- tries to join the network");
	startJoinTimer();
	bool join = false; //Set to true if I'm not the first ant --> Join network

	// iterate until joinTimer timeout is not expired
	while (!joinTimer_expired)
	{      
		if (MySerial.available())
		{        
			//Check if I'm the only ant in the network
			if ((char)MySerial.read() == '@')  
			{
				MySerial.readBytesUntil('$', _buffer, _bufsize);
				Serial.print(F("<--- Message received: "));
				Serial.println(_buffer);
				join = true;    
				joinTimerCallback(xJoinTimer);        
			}
		}
		memset(_buffer, 0, _bufsize);

	}
	//Ant builds table and sends hello to join the network
	if(join)
	{
		bool isSpecialTurn = false;

		//Wait until special turn
		while (!isSpecialTurn)
		{
			if (MySerial.available())  
			{
				if ((char)MySerial.read() == '@')  
				{
					MySerial.readBytesUntil('$', _buffer, _bufsize);
					int mit;  
					int dest;
					size_t bufsize = sizeof(_buffer);
					char copy[bufsize];
					strncpy(copy, _buffer, bufsize);
					copy[bufsize-1] = '\0';
					char * pch = strtok(copy, "#");
					int i = 0;

					while (pch != NULL) 
					{
						if (i == 0)
						{
							mit = atoi(pch);
						}
						if(i == 1) 
						{ 
							dest = atoi(pch);
						}
						if(i > 1) 
						{
							pch = NULL; 
						}
						else 
						{
							pch = strtok(NULL, "#");
						}
						i++;
					}
					
					Serial.print("\nWaiting for first special turn");
					_buffer[0] = '\0';
					memset(_buffer, 0, _bufsize);
					//If dest is special turn
					if(dest == 100){
						isSpecialTurn = true;
					}            
				}
			}
		}
		BuildTable();
		UpdatePrevSucc();
		Hello();

		//Establish who is the first ant to speak after join
		if (prev == 100) 
		{
			vTaskDelay(50);
			isMyTurn = true;
		}
	}
}

//Build ant table
void RACom::BuildTable(){
	bool building = true;
	while (building){

		if (MySerial.available())  
		{
			//If i read the start symbol
			if ((char)MySerial.read() == '@')  
			{
				//read the content trasmitted until i reach $ or length
				MySerial.readBytesUntil('$', _buffer, _bufsize);
				Serial.print("\nBuilding table for:");
				Serial.print(_buffer);
				int mit;  
				int dest;

				size_t bufsize = sizeof(_buffer);
				char copy[bufsize];
				strncpy(copy, _buffer, bufsize);
				copy[bufsize-1] = '\0';
				char * pch = strtok(copy, "#");
				int i = 0;
				while (pch != NULL) 
				{
					if (i == 0)
					{ 
						mit = atoi(pch); 
					}
					if(i == 1) 
					{ 
						dest = atoi(pch); 
					}

					if(i > 1) 
					{
						pch = NULL;
					} 
					else 
					{
						pch = strtok(NULL, "#");
					}
					i++;
				}
				_buffer[0] = '\0';
				memset(_buffer, 0, _bufsize);

				//Add mit to table
				ANT_LIST[mit] = true;
				if (dest == 100)
				{
					building = false; //Stop building table
				}    		
			}
		}    
	}
}

//Update prev and succ (for new ant, or for dead ant)
void RACom::UpdatePrevSucc(){

	//Update prev and succ
	int newSucc = 0;
	int newPrev = 0;
	//UPDATE SUCC
	//Start iterating from the position after my ID
	//Check if ant is less or more than current prev / succ
	for (int i = MY_ID + 1; i < MAX_ANTS; i++)
	{
		if(ANT_LIST[i] == true)
		{
			newSucc = i;
			break;
		}
	} //If I didn't find succ, succ is 100
	if(newSucc == 0)
	{
		newSucc = 100;
	}      
	//UPDATE PREV
	for (int i = MY_ID - 1 ; i > 0; i--)
	{
		if(ANT_LIST[i] == true)
		{
			newPrev = i;
			break;
		}
	} //If I didn't find prev, prev is 100
	if(newPrev == 0)
	{
		newPrev = 100;
	}
	prev = newPrev;
	succ = newSucc;
	Serial.println("\nNew PREV: ");
	Serial.println(prev);
	Serial.println("\nNew SUCC: ");
	Serial.println(succ);
}


//Broadcast Hello message to join the network
void RACom::Hello()
{	
	Serial.print(F("\nHello message sent: "));
	// Wireless send
	MySerial.print('H');  // start char hello
	Serial.print('H');    // start char hello
	MySerial.print(MY_ID); // dest
	Serial.print(MY_ID);
	MySerial.print('$'); // end char
}

//Receive the hello message and update local table
void RACom::ReceiveHello()
{
	_buffer[0] = '\0';
	memset(_buffer, 0, _bufsize);
	xSemaphoreGive(xMutex);
	vTaskDelay(50);
	while( xSemaphoreTake( xMutex, ( TickType_t ) 10 ) == pdFALSE );
		int id = 0;
		size_t bufsize = sizeof(_buffer);
		char copy[bufsize];
		strncpy(copy, _buffer, bufsize);
		copy[bufsize - 1] = '\0';
		id = atoi(copy);
		//Serial.println("\nHello received from: " +id);

		//If i received hello message
		if (id > 0){
			//Update local table
			ANT_LIST[id] = true;
			UpdatePrevSucc();	
		}		
}

  //Broadcast message addressed to my succ
void RACom::Send(){
	MySerial.flush();
	Serial.flush();
	vTaskDelay(50);

	Serial.print(F("<--- Message Sent: "));

	// Wireless send
	MySerial.print('@'); // start char

	MySerial.print(MY_ID); // mit
	Serial.print(MY_ID);

	MySerial.print('#'); // separator
	Serial.print('#');

	MySerial.print(succ); // succ
	Serial.print(succ);

	MySerial.print('#'); // separator
	Serial.print('#');

	// next positions
	for (int i = 0; i < NUM_NEXT_POS; i++)
	{
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

    //Update expected
	if (succ == 100){
		expected = getFirstAnt();
	}else{
		expected = succ;
	}
	//Set my turn to false
	isMyTurn = false;
 }

void RACom::readBuffer(){
  	if ( xSemaphoreTake( xMutex, ( TickType_t ) 10 ) == pdTRUE ){
	Serial.print("\nRead buffer");
	startReplyTimer(); //I listen for 2 seconds.
	while(!replyTimer_expired){
		if (MySerial.available())
		{
			char firstChar = (char)MySerial.read();
			if (firstChar == 'H' || firstChar == '@' )
			{
				MySerial.readBytesUntil('$', _buffer, _bufsize);
				//Serial.println("\nMessage received:");
				Serial.println(_buffer);  			
				replyTimerCallback(xReplyTimer);
			}			
		}
	}
	 xSemaphoreGive( xMutex );
	 vTaskDelay(100);
	}
}

  //Receive data
 void RACom::Receive(){
	//Flush buffer
	_buffer[0] = '\0';
	memset(_buffer, 0, _bufsize);
	bool alive = false;
	bool isSpecialTurn = false;
	byte mode; 
	int mit;  
	int dest;
	int ss;  
	
 	xSemaphoreGive( xMutex); 
	 vTaskDelay(100);
	 while( xSemaphoreTake( xMutex, ( TickType_t ) 10 ) == pdFALSE );
		//If I don't read all the separators, the ant died while sending the message
		int separator = count_underscores(_buffer);
		if( separator == 12 )
		{
			alive = true;
			//Serial.print(_buffer);  
			size_t bufsize = sizeof(_buffer);
			char copy[bufsize];
			strncpy(copy, _buffer, bufsize);
			copy[bufsize-1] = '\0';
			char * pch = strtok(copy, "#");
			int i = 0;
			while (pch != NULL) 
			{
				if (i == 0)
				{
					mit = atoi(pch);
				}
				if(i == 1) 
				{
					dest = atoi(pch);
					if( dest != 100){
							expected = dest;
					}else
					{
						expected = getFirstAnt();
					}
				}			
				if (i >= 2 && i <= 9)
				{
				//put inside recvpos the tokens
					if (mit == 1)
						recvPos1[i - 2] = (byte)atoi(pch);
					if (mit == 2)
						recvPos2[i - 2] = (byte)atoi(pch);
					if (mit == 3)
						recvPos3[i - 2] = (byte)atoi(pch);
					if (mit == 4)
						recvPos4[i - 2] = (byte)atoi(pch);
					if (mit == 5)
						recvPos5[i - 2] = (byte)atoi(pch);
				}
			
				if (i == 10 && mit == SPECIAL_ANT_ID)
				{
					ss = atoi(pch);
					if (ss == 0)
					{
						startAndStop = (byte)ss;
					}
					else if (ss == 1)
					{
						startAndStop = (byte)ss;
					}
				}
				if (i == 11 && mit == SPECIAL_ANT_ID)
				{
					mode = atoi(pch);
					antMode = (byte)mode;
				}
				if(i == 12) 
				{
					if(mit == 1) currPos1 = (byte) atoi(pch);
					if(mit == 2) currPos2 = (byte) atoi(pch);
					if(mit == 3) currPos3 = (byte) atoi(pch);
					if(mit == 4) currPos4 = (byte) atoi(pch);
					if(mit == 5) currPos5 = (byte) atoi(pch);
				}
				pch = strtok(NULL, "#");
				i++;
			}
		}	
		//If alive flag is set to false, no message was received and the expected ant is dead
		if (!alive)
		{
			Death(expected);
		}
		else //If a message is received, check if next turn is mine 
		{
			//If the message is for me, next turn is mine
			if(dest == MY_ID)
			{
				isMyTurn = true;
			}
			//If the dest is the special turn, start the special turn procedure
			if(dest == 100)
			{
	 			Special_Turn();  						
			}	 
		}
			
}

//Count message separators
int RACom::count_underscores(char *s) {
  int count = 0;

  for (int i = 0; i < strlen(s); i++)
    if (s[i] == '#') count++;

  return count;
}

 //Updates variables to remove dead ant
void RACom::Death(int mit){
 for (int i = mit; i < MAX_ANTS; i++)
	{
		if (ANT_LIST[i])
		{
			ANT_LIST[i] = false;
			UpdatePrevSucc();
			if (mit == prev)
				isMyTurn = true;
			return;
		}
	}
	for (int i = 0; i < MAX_ANTS; i++)
	{
		if (ANT_LIST[i])
		{
			ANT_LIST[i] = false;
			UpdatePrevSucc();
			if (mit == prev)
				isMyTurn = true;
			return;
		}
	}
	
}

// Starts the special turn
void RACom::Special_Turn(){

 	ReceiveHello();
	//Establish who is the first ant to speak after special turn
	if (prev == 100) 
	{
	 isMyTurn = true;
	}
}

//Return ID of first ant in the table
 int RACom::getFirstAnt(){
   	for(int i = 1; i <= MAX_ANTS; i++){
	   if(ANT_LIST[i]){
		   return i;
		}
	   }
 }

//This method is used for set the 13s pin as HIGH (5V)
void RACom::comunicationMode()
{
	digitalWrite(SET_PIN, HIGH);
	//analogWrite(SET_PIN, 255);
}
//this method is used for set the 13s pin as LOW (0V)
void RACom::commandMode()
{
	digitalWrite(SET_PIN, LOW);
	//analogWrite(SET_PIN, 0);
}

//Set the handle of the tasks
void RACom::setTaskHandle(TaskHandle_t *xHandleRGB, TaskHandle_t *xHandleMotion)
{
	taskRGB = xHandleRGB;
	taskMotion = xHandleMotion;
}

//Set the handle of the tasks
void RACom::setBufferTaskHandle(TaskHandle_t *xHandleBuffer, TaskHandle_t *xHandleComm)
{
	taskBuffer = xHandleBuffer;
	taskComm = xHandleComm;

}


//Set the startandstop state
void RACom::setStartAndStop(byte state)
{
	startAndStop = state;
}

//get the start and stop state
byte RACom::getStartAndStop()
{
	return startAndStop;
}

//set the antMode state
void RACom::setAntMode(byte mode)
{
	antMode = mode;
}

//get the antMode state
byte RACom::getAntMode()
{
	return antMode;
}

//Set my current position
void RACom::setMyCurrentPosition(byte pos)
{
	myCurrentPosition = pos;
}

//get the position of the five ants
byte RACom::getCurrentPosOfAnt(byte num_ant)
{
	if (num_ant == 1)
		return currPos1;
	if (num_ant == 2)
		return currPos2;
	if (num_ant == 3)
		return currPos3;
	if (num_ant == 4)
		return currPos4;
	if (num_ant == 5)
		return currPos5;
}


void RACom::setupMutex()
{
	Serial.println("Setup mutex");
	xMutex = xSemaphoreCreateMutex();
	if (xMutex == NULL)
		{
	 	Serial.println(F("Failure creating mutex"));
			for (;;)
				;
		}else {
			Serial.println("Mutex created successfully.");
		}
}

void RACom::setupTimers()
{
	Serial.println("Setup timers");

	
	xSpecialTurnTimer = xTimerCreate(
		"Special_Timer",     /* A text name, purely to help debugging. */
		(SPECIAL_TURN_TIMOUT),   /* The timer period. */
		pdFALSE,              /* This is an auto-reload timer, so xAutoReload is set to pdTRUE. */
		(void *)0,            /* The ID is not used, so can be set to anything. */
		specialTurnTimerCallback /* The callback function that inspects the status of all the other tasks. */
		);
	xReplyTimer = xTimerCreate(
		"Reply_Timer",     /* A text name, purely to help debugging. */
		(REPLY_TIMEOUT),   /* The timer period. */
		pdFALSE,              /* This is an auto-reload timer, so xAutoReload is set to pdTRUE. */
		(void *)0,            /* The ID is not used, so can be set to anything. */
		replyTimerCallback /* The callback function that inspects the status of all the other tasks. */
		);
	

	if (xSpecialTurnTimer == NULL || xReplyTimer == NULL)
	{
		Serial.println(F("failure creating Timers"));
		for (;;)
			;
	}else{
			Serial.println("Timers created successfully.");
	}
}

  //Starts the timer for adding a new ant to the network
void RACom::startJoinTimer()
{
	Serial.print('\n');
	Serial.println(F("J. timer started"));
	joinTimer_expired = false;
	xTimerStart(xJoinTimer, 0);
}

//Starts the timer to check if an ant is dead
void RACom::startSpecialTurnTimer()
{
	Serial.print('\n');
	Serial.println(F("Special timer started"));
	special_turnTimer_expired = false;
	xTimerStart(xSpecialTurnTimer, 0);
}

//Starts the timer to check if an ant is dead
void RACom::startReplyTimer()
{
	Serial.print('\n');
	//Serial.println(F("Reply timer started"));
	replyTimer_expired = false;
	xTimerStart(xReplyTimer, 0);
}


//when we have a callback to the join timer it means that it has expired
void RACom::joinTimerCallback(TimerHandle_t xTimer)
{
	Serial.print('\n');
	Serial.println(F("Join Timer Expired"));
	joinTimer_expired = true;	
	xTimerStopFromISR(xJoinTimer, 0);

}
//when we have a callback to the join timer it means that it has expired
void RACom::specialTurnTimerCallback(TimerHandle_t xTimer)
{
	Serial.print('\n');
	Serial.println(F("Special Timer Expired"));
	special_turnTimer_expired = true;
	xTimerStopFromISR(xSpecialTurnTimer, 0);
}
//when we have a callback to the join timer it means that it has expired
void RACom::replyTimerCallback(TimerHandle_t xTimer)
{
	Serial.print('\n');
	Serial.println(F("Reply Timer Expired"));
	replyTimer_expired = true;
	xTimerStopFromISR(xReplyTimer, 0);
}
