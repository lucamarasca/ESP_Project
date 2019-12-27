#include <RACom.h>
#include <queue.h>
#include <semphr.h>

//ANTS ID
/*
  Natasha -> 1
  Skriniar -> 2
  Milova -> 3
*/
#define ANT_ID 3
#define ANT_TOT 3
//RGB PIN
#define s0 2
#define s1 3
#define s2 4
#define s3 11
#define out 12
#define led A0
//Motor PIN
#define enB 5
#define in3 7
#define in1 9
#define enA 10
#define in2 A4
#define in4 A5

//Define global variable for the detected cell
static byte cellRGB;

//Queue
static QueueHandle_t xQueue1;
static byte newPos[8] = {255, 255, 255, 255, 255, 255, 255, 255};
unsigned long ulIdleCycleCount = 0UL;

//Struct for motor
typedef struct {
  byte radius;
  byte dir;
} Values;

TaskHandle_t xHandleRGB;
TaskHandle_t xHandleMotion;

RACom wireless;

//Matrix of RGB
const int pinMatrix[225][3] PROGMEM  = {
  {24, 21, 179},
  {16, 21, 166},
  {24, 21, 179},
  {24, 11, 179},
  {39, 21, 166},
  {55, 21, 166},
  {55, 21, 166},
  {62, 21, 166},
  {70, 31, 166},
  {85, 31, 166},
  {116, 41, 179},
  {147, 62, 179},
  {170, 62, 192},
  {186, 72, 192},
  {201, 102, 204},
  {16, 21, 166},
  {16, 21, 166},
  {24, 21, 179},
  {31, 21, 166},
  {31, 21, 179},
  {39, 21, 179},
  {55, 21, 179},
  {55, 21, 166},
  {85, 31, 179},
  {109, 41, 179},
  {147, 62, 179},
  {163, 62, 192},
  {186, 72, 192},
  {201, 92, 192},
  {217, 113, 204},
  {16, 21, 179},
  {16, 31, 179},
  {16, 21, 179},
  {31, 21, 179},
  {39, 31, 179},
  {47, 21, 179},
  {55, 21, 179},
  {70, 31, 179},
  {85, 31, 179},
  {109, 41, 179},
  {140, 51, 179},
  {163, 62, 179},
  {186, 82, 192},
  {201, 92, 192},
  {217, 102, 204},
  {16, 41, 192},
  {8, 31, 192},
  {16, 31, 179},
  {24, 31, 179},
  {39, 21, 179},
  {47, 21, 166},
  {62, 31, 166},
  {70, 21, 166},
  {93, 31, 166},
  {124, 51, 179},
  {155, 62, 192},
  {178, 72, 192},
  {201, 92, 192},
  {217, 102, 204},
  {232, 113, 204},
  {16, 62, 204},
  {24, 51, 204},
  {31, 41, 192},
  {39, 41, 179},
  {62, 41, 179},
  {70, 41, 179},
  {78, 41, 179},
  {101, 41, 179},
  {140, 51, 179},
  {163, 62, 179},
  {186, 72, 192},
  {201, 92, 192},
  {217, 102, 192},
  {232, 113, 204},
  {232, 113, 204},
  {31, 123, 230},
  {39, 113, 230},
  {39, 102, 217},
  {47, 82, 204},
  {62, 72, 204},
  {101, 72, 192},
  {85, 41, 179},
  {101, 51, 179},
  {132, 51, 179},
  {155, 62, 179},
  {178, 72, 192},
  {201, 82, 192},
  {217, 92, 192},
  {232, 113, 204},
  {240, 153, 204},
  {62, 153, 230},
  {70, 143, 230},
  {85, 143, 230},
  {93, 143, 230},
  {101, 143, 230},
  {132, 133, 230},
  {132, 113, 217},
  {140, 82, 204},
  {170, 82, 192},
  {178, 82, 192},
  {201, 92, 192},
  {209, 92, 192},
  {232, 102, 192},
  {240, 113, 204},
  {240, 123, 204},
  {85, 164, 204},
  {85, 164, 204},
  {85, 164, 204},
  {93, 153, 204},
  {101, 143, 204},
  {109, 133, 204},
  {124, 113, 204},
  {147, 102, 204},
  {170, 82, 192},
  {186, 82, 192},
  {201, 92, 192},
  {217, 92, 192},
  {240, 102, 192},
  {255, 123, 204},
  {255, 123, 204},
  {132, 194, 166},
  {124, 194, 153},
  {124, 184, 153},
  {124, 174, 166},
  {132, 164, 166},
  {140, 143, 166},
  {155, 123, 166},
  {170, 92, 166},
  {186, 72, 166},
  {209, 72, 179},
  {217, 92, 179},
  {232, 92, 179},
  {240, 102, 179},
  {255, 153, 192},
  {255, 113, 192},
  {170, 235, 141},
  {170, 235, 141},
  {163, 225, 141},
  {155, 204, 128},
  {155, 194, 128},
  {155, 174, 115},
  {163, 143, 115},
  {170, 113, 102},
  {178, 72, 102},
  {201, 41, 115},
  {217, 41, 128},
  {232, 62, 141},
  {240, 82, 153},
  {255, 92, 166},
  {255, 102, 166},
  {170, 235, 141},
  {178, 235, 141},
  {178, 235, 141},
  {155, 215, 115},
  {155, 194, 102},
  {155, 164, 90},
  {163, 133, 77},
  {170, 92, 64},
  {186, 62, 77},
  {201, 31, 77},
  {217, 21, 90},
  {232, 51, 115},
  {255, 92, 153},
  {255, 92, 141},
  {255, 92, 141},
  {186, 235, 141},
  {186, 235, 141},
  {186, 235, 141},
  {178, 235, 141},
  {170, 215, 115},
  {170, 184, 90},
  {170, 153, 64},
  {178, 123, 51},
  {186, 72, 39},
  {201, 31, 39},
  {217, 11, 39},
  {232, 41, 90},
  {248, 72, 115},
  {255, 92, 128},
  {255, 82, 115},
  {194, 235, 135},
  {186, 235, 135},
  {194, 245, 135},
  {194, 245, 135},
  {194, 235, 121},
  {186, 215, 94},
  {186, 194, 68},
  {186, 153, 41},
  {194, 113, 14},
  {194, 113, 14},
  {201, 62, 14},
  {217, 21, 13},
  {232, 31, 39},
  {248, 62, 77},
  {255, 72, 90},
  {209, 245, 135},
  {209, 245, 135},
  {201, 245, 148},
  {201, 245, 135},
  {209, 245, 135},
  {201, 225, 108},
  {201, 184, 68},
  {194, 153, 41},
  {201, 101, 14},
  {217, 62, 0},
  {232, 41, 14},
  {240, 31, 27},
  {248, 62, 41},
  {255, 72, 68},
  {255, 82, 57},
  {217, 255, 142},
  {209, 245, 142},
  {209, 255, 128},
  {217, 245, 142},
  {217, 255, 142},
  {209, 235, 114},
  {201, 215, 85},
  {201, 204, 85},
  {201, 184, 57},
  {201, 143, 29},
  {217, 92, 15},
  {232, 72, 0},
  {240, 41, 15},
  {248, 62, 29},
  {255, 72, 57}
};

void taskRGB(void *pvParameters) {
  TickType_t xLastWakeTime;

  byte red_frequency = 0, blue_frequency = 0, green_frequency = 0, mod_red = 0, mod_green = 0, mod_blue = 0;
  byte buffCellTot = 254, count = 0, lecture[10], moda = 0, counter[10], maxim = 0, j = 0, k = 0, h = 0, z = 0;
  byte raw = 0, valR = 0, valG = 0, valB = 0, subR = 0, subG = 0, subB = 0, abR = 0, abG = 0, abB = 0, maximum = 0;
  Values values;

  pinMode(s0, OUTPUT);
  pinMode(s1, OUTPUT);
  pinMode(s2, OUTPUT);
  pinMode(s3, OUTPUT);
  pinMode(out, INPUT);
  pinMode(led, OUTPUT);
  analogWrite(led, 255);
  digitalWrite(s0, HIGH);
  digitalWrite(s1, LOW);

  for (;;) {
    Serial.print(F("R"));
    //Serial.println(xTaskGetTickCount());

    values = {100, 0};

    //The photodiode is set to detect the red color
    digitalWrite(s2, LOW);
    digitalWrite(s3, LOW);
    red_frequency = pulseIn(out, LOW);
    mod_red = map(red_frequency, 7, 40, 255, 0);

    /*
      Serial.print(F("RED: "));
      Serial.print(F("f-> "));
      Serial.print(red_frequency);
      Serial.print(F(" ----- "));
      Serial.print(F("v-> "));
      Serial.print(mod_red);
      Serial.println();
    */

    //The photodiode is set to detect the green color
    digitalWrite(s2, HIGH);
    digitalWrite(s3, HIGH);
    green_frequency = pulseIn(out, LOW);
    mod_green = map(green_frequency, 10, 35, 255, 0);

    /*
      Serial.print(F("GREEN: "));
      Serial.print(F("f-> "));
      Serial.print(green_frequency);
      Serial.print(F(" ----- "));
      Serial.print(F("v-> "));
      Serial.println(mod_green);
      Serial.println();
    */

    //The photodiode is set to detect the blue color
    digitalWrite(s2, LOW);
    digitalWrite(s3, HIGH);
    blue_frequency = pulseIn(out, LOW);
    mod_blue = map(blue_frequency, 7, 27, 255, 0);

    /*
      Serial.print(F("BLUE: "));
      Serial.print(F("f-> "));
      Serial.print(blue_frequency);
      Serial.print(F(" ----- "));
      Serial.print(F("v-> "));
      Serial.println(mod_blue);
      Serial.println();
      Serial.println();
    */

    for (raw = 0; raw < 225; raw++) {
      valR = pgm_read_word(&pinMatrix[raw][0]);
      valG = pgm_read_word(&pinMatrix[raw][1]);
      valB = pgm_read_word(&pinMatrix[raw][2]);

      subR = mod_red - valR;
      subG = mod_green - valG;
      subB = mod_blue - valB;

      abR = abs(subR);
      abG = abs(subG);
      abB = abs(subB);

      if (abR < 20 && abG < 20 && abB < 20) {
        buffCellTot = raw;
        j++;
      }
    }
    count++;
    j = 0;

    lecture[count] = buffCellTot;

    if (count == 10) {
      if ((mod_red < 25 && mod_green < 75) && mod_blue < 40) {
        maxim = 226;
        count = 0;
      } else {
        for (k = 0; k < 10; k++) {
          for (j = 0; j < 10; j++) {
            if (lecture[k] == lecture[j]) {
              moda++;
            }
          }
          counter[k] = moda;
          moda = 0;
        }
        for (h = 0; h < 9; h++) {
          z = h + 1;
          if (counter[h] > counter[z] && counter[h] > maximum) {
            maximum = counter[h];
            maxim = lecture[h];
          } else {
            maximum = counter[z];
            maxim = lecture[z];
          }
        }
        z = 0;
        count = 0;
      }
      wireless.setMyCurrentPosition(maxim);
      cellRGB = maxim;
      xQueueSendToBack(xQueue1, (void *) &values, portMAX_DELAY);

      vTaskDelay(5);
    }
  }
}

void taskMotion(void *pvParameters) {
  TickType_t xLastWakeTime;
  Values buff, dodge = {1, 0};
  Values noW = {0, 1};
  byte cell = 0, dir = 0, newCell = 0, now = 0, alt = 0, h = 0, i = 0, j = 0, scartoCella = 0, n = 0, o = 0;
  byte right_motor, left_motor, flagRotate = 0, c1, c2, c3, c4, c5, c6, c7, c8, start_stop;
  byte* antPos;

  pinMode(enA, OUTPUT);
  pinMode(enB, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);

  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  digitalWrite(in3, HIGH);
  digitalWrite(in4, LOW);
  for (;;) {
    Serial.print(F("M"));
    //Serial.println(xTaskGetTickCount());

    xQueueReceive(xQueue1, &buff, portMAX_DELAY);
    cell = cellRGB;

    start_stop = wireless.getStartAndStop();
    if (start_stop == 0) {
      analogWrite(enB, 0);
      analogWrite(enA, 0);
    } else {
      if (cell == 226) {
        xQueueReset(xQueue1);
        xQueueSendToBack(xQueue1, (void *) &noW, portMAX_DELAY);
      } else if (cell >= 0 && cell < 224) {
        c1 = cell + 2;
        newPos[0] = c1;
        c2 = cell + 15;
        newPos[1] = c2;
        c3 = cell + 16;
        newPos[2] = c3;
        c4 = cell + 17;
        newPos[3] = c4;
        c5 = cell - 2;
        newPos[4] = c5;
        c6 = cell - 15;
        newPos[5] = c6;
        c7 = cell - 16;
        newPos[6] = c7;
        c8 = cell - 17;
        newPos[7] = c8;
      }
      wireless.setNextPosArray(newPos);
      h = 0;

      for (i = 1; i <= ANT_TOT ; i++) {
        if (i != ANT_ID) {
          antPos = wireless.getRecvPosArray(i);
          h++;
          for (n = 0; n < 8 ; n++) {
            for (o = 0; o < 8; o++) {
              if ((antPos[n] == newPos[o]) && (antPos[n] != 225)) {
                Serial.println(F("Occupato!"));
                newPos[o] = 225;
                flagRotate = 1;
              }
            }
          }
        }
      }
      h = 0;

      if (flagRotate == 1) {
        for (j = 0; j < 8; j++) {
          if (newPos[j] < 225) {
            scartoCella = newPos[j] - cell;
            if (scartoCella > 10) {
              buff.dir = 2;
              buff.radius = 0;
              break;
            } else if (scartoCella == -2) {
              buff.dir = 1;
              buff.radius = 0;
              break;
            } else if (scartoCella == 2) {
              buff.dir = 1;
              buff.radius = 0;
              break;
            } else if (scartoCella < -10) {
              buff.dir = 1;
              buff.radius = 0;
              break;
            }
          } else if (newPos[j] == 225) {
            buff.dir = 1;
            buff.radius = 0;
            break;
          }
        }
        flagRotate++;
      }

      if (buff.dir == 0) {
        //Forward
        Serial.println(F("F"));

        if (ANT_ID == 1) {
          //Natasha
          analogWrite(enB, 105); //Right
          analogWrite(enA, 98); //Left
        } else if (ANT_ID == 2) {
          //Skriniar
          analogWrite(enB, 156);  //Right
          analogWrite(enA, 87);   //Left
        } else if (ANT_ID == 3) {
          //Milova
          analogWrite(enB, 105);  //Right
          analogWrite(enA, 135);   //Left
        }
      } else if (buff.dir == 1) {
        //Right
        Serial.println(F("R"));
        right_motor = map(buff.radius, 0, 140, 50, 89);
        analogWrite(enB, right_motor);
        analogWrite(enA, 155);
      } else {
        //Left
        Serial.println(F("L"));
        left_motor = map(buff.radius, 0, 140, 50, 80);
        analogWrite(enA, left_motor);
        analogWrite(enB, 126);
      }
    }
    vTaskDelay(5);
  }
}

void taskWireless(void *pvParameters) {
  Serial.println(F("Task Wireless"));

  for (;;) {
    //Serial.print(F("Wir"));
    //Serial.println(xTaskGetTickCount());
    wireless.comAlgo();
  }
}

void vApplicationIdleHook(void) {
  //Serial.print(F("Idle"));
  //Serial.println(xTaskGetTickCount());
  ulIdleCycleCount++;
}

void setup() {
  Serial.begin(9600);

  wireless.init(ANT_ID);
  wireless.comunicationMode();
  wireless.setupTimers();

  xQueue1 = xQueueCreate(12, sizeof(Values));

  Values values = {100, 0};
  xQueueSendToBack(xQueue1, (void *) &values, portMAX_DELAY);
  xQueueSendToBack(xQueue1, (void *) &values, portMAX_DELAY);

  //pvTaskCode, pcName, usStackDepth, pvParameters, uxPriority, pxCreatedTask
  xTaskCreate(taskRGB, "TaskRGB", 128, NULL, 1, &xHandleRGB);
  xTaskCreate(taskMotion, "taskMotion", 128, NULL,  1, &xHandleMotion);
  xTaskCreate(taskWireless, "taskWireless", 128, NULL, 1, NULL);

  wireless.setTaskHandle(&xHandleRGB, &xHandleMotion);

  vTaskStartScheduler();
  for (;;);
}

void loop() {}