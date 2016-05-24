//CAN-bus sending sensor board for EGT thermocouples and analog inputs
//Copyright 2015 John Stäck
//Released under GNU General Public License v3

#include <SPI.h>
#include "Adafruit_MAX31855.h"
#include "mcp_can.h"

//Pin CAN chip select is connected to
#define CAN_CS_PIN 10

//Numer of thermocouples
#define N_TK 4

//Number of analog sensors
#define N_SENSORS 8

#define MOSI 11
#define MISO 12
#define SCLK 13

//CAN ID of EGT and sensors. Each additional batch of 4 values will increment this id by 1.
#define CAN_EGT_BASE_ID 0x120
#define CAN_SENSORS_BASE_ID 0x140

//How often to send EGT and analog sensors
#define EGT_INTERVAL 500

#define SENSOR_INTERVAL 500

#define SENSOR_MEASURE_INTERVAL 100

#define CAN_SPEED CAN_1000KBPS

//Average analog sensor measurements over this many data points
#define SENSOR_WINDOW 8

//Pins to the thermocouples chip select
const byte tk_cs_pins[N_TK] = { 6, 7, 8, 9 };

//const byte sensor_pins[N_SENSORS] = {0, 1, 2, 3, 4, 5, 6, 7};
const byte sensor_pins[8] = {0, 1, 2, 3, 4, 5, 6, 7};

double tk_values[N_TK];
int sensor_values[N_SENSORS][SENSOR_WINDOW];

int sensor_pos;

//When EGT and analog sensors last were read
unsigned long last_egt=0, last_sensor_send=0, last_sensor_measure=0;

Adafruit_MAX31855 *thermocouples[N_TK];

MCP_CAN CAN(CAN_CS_PIN);

void setup() {
  Serial.begin(115200);

  byte can_status = CAN.begin(CAN_SPEED);

  byte attempts=0;

  while(can_status != CAN_OK && ++attempts<20)
  {
    Serial.print(F("Could not init CAN, retrying..."));
    Serial.println(attempts);
    delay(250);
    can_status = CAN.begin(CAN_SPEED);
  }

  

  if(can_status== CAN_OK)
  {
    Serial.println(F("CAN MCP2515 init ok!"));
  }
  else
  {
    Serial.println(F("Could not initialize CAN MCP2515!"));
  }

  Serial.println(F("Initializing thermocouples"));
  for(int i=0; i<N_TK; ++i)
  {
    thermocouples[i] = new Adafruit_MAX31855(tk_cs_pins[i]);
  }

  Serial.println(F("Waiting for thermocouples to settle"));

  //Initialize all sensor values with current value
  sensor_pos = 0;
  for(int i=0; i<N_SENSORS; ++i)
  {
    ReadSensors();
  }
  
  delay(1000);

  Serial.println(F("Running"));

}

void SendEGT()
{
  //Send data in batches of 4
  for(int batch=0; batch*4<N_TK; ++batch)
  {
      //Init whole buffer in case not all four slots are used
      uint8_t buf[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
      for(int i=0; i+batch*4 < N_TK; ++i)
      {
        byte n = batch*4+i;

        double fval = tk_values[n] = thermocouples[n]->readCelsius();

        unsigned int ival;

        Serial.print("EGT");
        Serial.print(i);
        Serial.print(" ");
        Serial.println(fval, 1);

        
        if(isnan(fval)) //Just silently skip broken thermocouples
        {
          ival = 0;
        }
        else if(fval < 0) //Set all negative temperatures to zero
        {
          ival = 0;
        }
        else
        {
          ival = fval*10;
        }

        //Enforce little endian
        buf[2*i] = ival & 0x00FF;
        buf[2*i+1] = (ival & 0xFF00) >> 8;
        
        delay(5);
      }
      
      Serial.print("Sending E batch ");
      Serial.println(batch);
//      Serial.print("0x");Serial.print(CAN_EGT_BASE_ID + batch, HEX);Serial.print(" ");
      for(int i=0; i<8; ++i)
      {
        Serial.print(buf[i], HEX);
        Serial.print(" ");
      }
      Serial.println();
      CAN.sendMsgBuf(CAN_EGT_BASE_ID + batch, 0, 8, buf);
  }
  Serial.println();
}


void ReadSensors()
{
  //Serial.println("Reading sensors");
  for(int i=0; i< N_SENSORS; ++i)
  {
    sensor_values[i][sensor_pos] = analogRead(sensor_pins[i]);
 
  }

  //Move to next slot in sensor value list
  if(++sensor_pos >= SENSOR_WINDOW) sensor_pos=0;
  
}
void SendSensors()
{
  //Send data in batches of 4
  for(int batch=0; batch*4<N_SENSORS; ++batch)
  {
      //Serial.print("Sending S batch ");
      //Serial.println(batch);
      //Init whole buffer in case not all four slots are used
      uint8_t buf[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
      
      for(int i=0; i<4; ++i)
      {
        
        byte n = batch*4+i;

        if(n>=N_SENSORS)
        {
          break;
        }

        //Serial.print("SENSOR#");Serial.print(n);
        //Serial.print(" ");
        
        
        //Just send the raw analog value (0-1023) and let the receiver deal with scaling and units
        long val=0;

        for(int j=0; j<SENSOR_WINDOW; ++j)
        {
          val += sensor_values[n][j];
        }
        
        int ival = val / SENSOR_WINDOW;

        //Serial.println(ival);
        

        //Enforce big endian
        buf[2*i] = (ival & 0xFF00) >> 8;
        buf[2*i+1] = ival & 0x00FF;
        
      }

      /*
      for(int i=0; i<8; ++i)
      {
        Serial.print(buf[i], HEX);
        Serial.print(" ");
      }
      Serial.println();
      */
      
      CAN.sendMsgBuf(CAN_SENSORS_BASE_ID + batch, 0, 8, buf);
  }
  //Serial.println();  
}



void loop() {
  long now = millis();
  if(now > last_egt+EGT_INTERVAL)
  {
    last_egt = now;
    SendEGT();
  }
  if(now > last_sensor_measure+SENSOR_MEASURE_INTERVAL)
  {
    last_sensor_measure = now;
    ReadSensors();
  }
  if(now > last_sensor_send+SENSOR_INTERVAL)
  {
    last_sensor_send = now;
    SendSensors();
  }
  digitalWrite(13, HIGH);
  delay(10);
  digitalWrite(13, LOW);
}
