//Shiftlight for Neopixel LED output and CAN input
//Copyright 2015 John Stäck
//Released under GNU General Public License v3

#include <Adafruit_NeoPixel.h>
#include <SPI.h>
#include "mcp_can.h"


//Chip select for CAN interface
#define CAN_CS_PIN 10

//CAN bus bitrate to use. Note that this gets halved when MCP2515 is clocked at 8MHz from the nominal 16MHz
#define CAN_BITRATE CAN_1000KBPS

//ID for RPM on CAN bus
#define CAN_RPM_ID 0x280

//Most and least significant bytes in the 8byte data buffer for RPM
#define CAN_RPM_MSB 3
#define CAN_RPM_LSB 2

//Multiply or divide value by this. Some ECUs require dividing by 4.
//Leave undefined if not used
//#define CAN_RPM_MULTIPLIER 4
//#define CAN_RPM_DIVISOR 4


//ID for "warning" status on CAN bus. This can be a CEL signal or a status bitfield
//0x623 is Megasquirt group 51 (0x520 as default base + 0x33), where CEL status bitfield is in last byte
#define CAN_WARNING_ID 0x623

//Bits to check for each byte in the 8-bit data buffer.
//Incoming data is masked with these bits. If anything non-zero remains, warning is enabled.
uint8_t warning_mask[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF}


//Number of pixels on strip
#define NUM_PIXELS 8 

//Data pin for neopixel strip
#define NEOPIXEL_PIN 6

//Brightness to use (0-255)
#define BRIGHTNESS 100

//Number of stages between upper and lower RPM
#define NUM_STAGES 4

//Stages for shift flash and warn flash
#define STAGE_FLASH1 NUM_STAGES
#define STAGE_FLASH2 NUM_STAGES+1
#define STAGE_WARNING1 NUM_STAGES+2
#define STAGE_WARNING2 NUM_STAGES+3

#define SHIFT_FLASH 35
#define WARNING_FLASH 250

#define RPM_START 2000
#define RPM_SHIFT 6000

//Number of samples to average over. Comment out (undefine) for no averaging
#define RPM_AVERAGE 4

//Pin for enabling warning flasher
#define WARNING_PIN 7

//Disable display if no signal received for this many milliseconds
#define TIMEOUT 2000

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_PIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
MCP_CAN CAN(CAN_CS_PIN); 


//program state in globals because why not
long rpm;
int rpmstage;
int laststage;
long lastsignal;
bool warning;
bool canwarning;
bool lastwarning; //Last warning signal over can bus


//Make some colors!
uint32_t red     = strip.Color(255,0,0);
uint32_t green   = strip.Color(0,255,0);
uint32_t blue    = strip.Color(0,0,255);

uint32_t cyan    = strip.Color(0,255,255);
uint32_t magenta = strip.Color(255,0,255);
uint32_t yellow  = strip.Color(255,255,0);

uint32_t white   = strip.Color(255,255,255);
uint32_t black   = strip.Color(0,0,0);

//Static table for all the stages. Include two each for flashing above shiftpoint and at warning condition.
//Treated as 1-based index, as stage 0 is "blank"
uint32_t stages[NUM_STAGES+4][NUM_PIXELS] = {
  { black,   black,   black,   green,   green,   black,   black,   black  }, //Stage 1
  { black,   black,   green,   green,   green,   green,   black,   black  }, //Stage 2
  { black,   yellow,  green,   green,   green,   green,   yellow,  black  }, //Stage 3
  { red,     yellow,  green,   green,   green,   green,   yellow,  red    }, //Stage 4
  { white,   cyan,   cyan,     blue,    blue,    cyan,    cyan,    white  }, //Above shiftpoint 1
  { black,   black,   black,   black,   black,   black,   black,   black  }, //Above shiftpoint 2
  { black,   black,   black,   red,     red,     black,   black,   black  }, //Warning light 1
  { black,   black,   black,   yellow,  yellow,  black,   black,   black  }  //Warning light 2
};

void clearStrip()
{
  for(int i=0; i<NUM_PIXELS; ++i)
  {
    strip.setPixelColor(i, black);
  }
  strip.show();
}

void setStage(int stage)
{
  if(stage==0 || stage > NUM_STAGES+4){
    clearStrip();
    return;
  }
  
  for(int i=0; i<NUM_PIXELS; ++i)
  {
    strip.setPixelColor(i, stages[stage-1][i]);
  }
  strip.show();

}

void setup() {
  Serial.begin(115200);
  strip.begin();
  strip.setBrightness(BRIGHTNESS);

    
  //Light up a solid warning stage we initialize CAN bus and other things
  setStage(NUM_STAGES+4);

  pinMode(WARNING_PIN, INPUT_PULLUP);

  rpm=0;
  rpmstage=0;
  laststage=0;
  lastsignal=0;
  lastwarning=0;

  uint8_t can_status = CAN.begin(CAN_BITRATE);

  while(can_status != CAN_OK)
  {
    Serial.println("Could not init CAN, retrying...");
    delay(100);
    can_status = CAN.begin(CAN_BITRATE);
  }

  Serial.println("CAN inited.");
  
  clearStrip();
}

int rpmToStage(long r)
{
  if(r<RPM_START) return 0;

  if(r>=RPM_SHIFT) return NUM_STAGES+1;

  return (NUM_STAGES*(r-RPM_START)) / (RPM_SHIFT-RPM_START) + 1;
}

//Try to read from CAN, return true if updated RPM data was received
bool ReadCAN() {
  if(CAN.checkReceive() != CAN_MSGAVAIL)
  {
    return false;
  }

  uint8_t len = 0;
  uint8_t buf[8];
  uint16_t id;

  CAN.readMsgBuf(&len, buf);
  id = CAN.getCanId();

  if(id == CAN_RPM_ID)
  {
    lastsignal = millis();
  
    //Assemble rpm from two bytes.
    rpm = buf[CAN_RPM_MSB] << 8 | buf[CAN_RPM_LSB];
  
    //Apply multiplier or divisor
    
    #ifdef CAN_RPM_MULTIPLIER
      rpm = rpm * CAN_RPM_MULTIPLIER;
    #endif
  
    #ifdef CAN_RPM_DIVISOR
      rpm = rpm / CAN_RPM_DIVISOR;
    #endif
  
    Serial.println(rpm);
    
    return true;
  }
  else if(id == CAN_WARNING_ID)
  {
    uint8_t warnbits = 0x00;
    for(int i=0; i<8; ++i)
    {
      warnbits = warnbits | (buf[i] & warning_mask[i]);
    }
    warning = (bool)warnbits;
    if(warning)
    {
      lastwarning = millis();
    }
    
    //We didn't actually update RPM, just the warning status
    return false;
  }
  else
  {
    return false;
  }
}

void loop() {

  long now = millis();

  //Possibly timeout an old can warning. Can warning isn't updated until ReadCAN() is called later so a CAN warning state can be one loop old.
  //Should not be a big deal, since CAN status is usually just sent periodically and not very often at that.
  if(canwarning && (now > lastwarning+TIMEOUT))
  {
    canwarning = false;
  }

  warning = canwarning || !digitalRead(WARNING_PIN);

  if(ReadCAN()) {
    
    rpmstage = rpmToStage(rpm);
    
    if(rpmstage<=NUM_STAGES && rpmstage!=laststage && !warning)
    {
      setStage(rpmstage);
      laststage=rpmstage;
    }
  }

  //Clear strip on no data received (engine stopped?)
  if(now > lastsignal+TIMEOUT && rpm>0 && !warning)
  {
    rpm=0;
    clearStrip();
  }
  

  if(rpmstage > NUM_STAGES) //Shift flashing
  {
    int s = NUM_STAGES + ((now%(2*SHIFT_FLASH) > SHIFT_FLASH )?2:1);
    
    if(s!=laststage)
    {
      setStage(s);
      laststage=s;
    }        
  }
  else if(warning) //Warning flashing
  {
    int s = NUM_STAGES + ((now%(2*WARNING_FLASH) > WARNING_FLASH )?4:3);

    if(s!=laststage)
    {
      setStage(s);
      laststage=s;
    }        
  }

  //Miniscule delay.
  delay(5);

}
