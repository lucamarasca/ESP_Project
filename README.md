# RACom Branch FREERTOS

```
//Declaration of a variabile RACom type
RACom wireless;
// unsigned long ulIdleCycleCount = 0UL; //Idle task Counter

//Create a task that execute while(true) the method comAlgo();
void task1(void *pvParameters)
{
  for (;;) {
    wireless.comAlgo(); 
    //Serial.println(+ulIdleCycleCount);
    //vTaskDelay(250 / portTICK_PERIOD_MS);
  }
}

void setup()
{
  Serial.begin(9600); // opens serial port, sets data rate to 9600 bps

  // start th serial communication with the host computer

  //Serial.println("Arduino with HC-12 is ready");
  //Untile there are not serial port avaiable, wait
  while (!Serial);
  //Serial.println("OK");
  // Before loading sketch, initialize the library with init method.
//Initialize the wireless it's defined into RACom.h
  wireless.init(1, 3); // First param: id of ANT, Second param: number of ANTS
//Defined into RACom.h and done in RACom.cpp
  wireless.comunicationMode();
  wireless.setupTimers(); //?
//Create an instance of the task with this parameters: pointer of the task, name of the task, stack size, parameter passed, priority, handle
  xTaskCreate(task1, "task1", 128, NULL, 1, NULL);
//Start the scheduler --> run the task with highest priority
  vTaskStartScheduler();
//not stop the execution for doing something else
  for (;;);
}

void vApplicationIdleHook ( void ) {
  Serial.print(F("Idle"));
  Serial.println(xTaskGetTickCount());
}

//Metodo2
/* void vApplicationIdleHookProva ( void ) {
  ulIdleCycleCount ++;  
}/*

void loop()
{
}
```
