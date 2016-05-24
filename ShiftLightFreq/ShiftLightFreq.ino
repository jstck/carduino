//Shiftlight for Neopixel LED output and tach input
//Copyright 2015 John Stäck
//Released under GNU General Public License v3

#include <Adafruit_NeoPixel.h>
#include <FreqMeasure.h>


#define NUM_PIXELS 8
#define NEOPIXEL_PIN 9

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

#define RPM_START 4500
#define RPM_SHIFT 5500

//Number of samples to average over. Comment out (undefine) for no averaging
#define RPM_AVERAGE 4

//Usually (number of cylinders)/2, on a 4-stroke with one pulse per ignition event.
#define PULSES_PER_REV 2

//Pin for enabling warning flasher. Active low, with pullup.
#define WARNING_PIN 7

//Disable display if no signal received for this many milliseconds
#define TIMEOUT 500

#define LED_PIN 13

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_PIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

//program state
long rpm;
int rpmstage;
int laststage;
long lastsignal;
bool warning;

#ifdef RPM_AVERAGE
long rpmavg[RPM_AVERAGE];
int rpmpos;
#endif

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
  pinMode(WARNING_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  Serial.begin(115200);
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.show();


  rpm=0;
  rpmstage=0;
  laststage=0;
  lastsignal=0;

#ifdef RPM_AVERAGE
  for(int i=0; i<=RPM_AVERAGE; ++i) rpmavg[i]=rpm;
  rpmpos=0;
#endif
  
  FreqMeasure.begin();

  //Short startup flash
  setStage(NUM_STAGES);
  delay(200);
  clearStrip();
  digitalWrite(LED_PIN, LOW);
  
  clearStrip();
}


void diag()
{
  //Run diagnostic pattern
  for(int i=1; i<=NUM_STAGES+4; i++)
  {
    setStage(i);
    delay(500);
  }
}

int rpmToStage(long r)
{
  if(r<RPM_START) return 0;

  if(r>=RPM_SHIFT) return NUM_STAGES+1;

  return (NUM_STAGES*(r-RPM_START)) / (RPM_SHIFT-RPM_START) + 1;
}

int cc=0;

void loop() {

  warning=!digitalRead(WARNING_PIN);

  if (FreqMeasure.available()) {
    rpm = (60/PULSES_PER_REV)* FreqMeasure.countToFrequency(FreqMeasure.read());
    lastsignal=millis();
    
#ifdef RPM_AVERAGE
    rpmavg[rpmpos]=rpm;
    if(++rpmpos>=RPM_AVERAGE) rpmpos=0;

    rpm=0;
    for(int i=0; i<RPM_AVERAGE; ++i)
    {
      rpm += rpmavg[i];
    }
    rpm=rpm/RPM_AVERAGE;
#endif

    rpmstage = rpmToStage(rpm);
    
    if(rpmstage<=NUM_STAGES && rpmstage!=laststage && !warning)
    {
      setStage(rpmstage);
      laststage=rpmstage;
      cc=10;
    }
  }

  if(millis() > lastsignal+TIMEOUT && rpm>0 && !warning)
  {
    rpm=0;
    clearStrip();
  }


  if(rpmstage > NUM_STAGES) //Shift flashing
  {
    int s = NUM_STAGES + ((millis()%(2*SHIFT_FLASH) > SHIFT_FLASH )?2:1);
    
    if(s!=laststage)
    {
      setStage(s);
      laststage=s;
    }        
  }
  else if(warning) //Warning flashing
  {
    int s = NUM_STAGES + ((millis()%(2*WARNING_FLASH) > WARNING_FLASH )?4:3);

    if(s!=laststage)
    {
      setStage(s);
      laststage=s;
    }        
  }
  
  if(cc++>=10)
  {
    cc=0;
    Serial.print(rpm);
    Serial.print(" ");
    Serial.println(laststage);
  }

  //Miniscule delay.
  delay(10);

}
