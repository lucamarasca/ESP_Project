#include <RAComLibNew.h>
TaskHandle_t xHandleBuffer;
TaskHandle_t xHandleComm;


RACom wireless;

void task1(void *pvParameters)
{
 wireless.init(1);
  for (;;) {
 wireless.comAlgo(); 
  }
}

void taskBuffer(void *pvParameters)
{
  vTaskDelay(50);
  for (;;) {
   wireless.readBuffer(); 
  }
}

void setup()
{
  Serial.begin(9600); // opens serial port, sets data rate to 9600 bps
  while (!Serial);
 wireless.setupTimers();
 wireless.setupMutex();
 wireless.comunicationMode();
 xTaskCreate(task1, "task1", 200, NULL, 2, &xHandleComm);
 xTaskCreate(taskBuffer, "taskBuffer", 60, NULL, 2, &xHandleBuffer);
wireless.setBufferTaskHandle(&xHandleBuffer, &xHandleComm );
 vTaskStartScheduler();
  for (;;);
}

void loop()
{
 // Serial.println("task idle");
}
