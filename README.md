# RACom Branch FREERTOS

```
//Declaration of a variabile RACom type
RACom wireless;
//Create a task that execute while(true) the method comAlgo();
void task1()
{
  for (;;) {
    wireless.comAlgo(); 
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
//Create an instance of the task with this parameters: pointer of the task, name of the task, stack size, parameter passed, priority, handle
  xTaskCreate(task1, "task1", 128, NULL, 1, NULL);
//Start the scheduler --> run the task with highest priority
  vTaskStartScheduler();
//not stop the execution
  for (;;);
}

void loop()
{

}
```
