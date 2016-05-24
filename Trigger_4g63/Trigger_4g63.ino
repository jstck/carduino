//Simple 4G63 trigger wheel simulator for Arduino
//Copyright 2015 John Stäck
//Released under GNU General Public License v3

#include <TimerOne.h>

//Number of phases per full engine cycle (usually two crankshaft / one camshaft rotation)
//Whatever resolution is needed to model the full detail of the trigger wheel(s)
#define N_PHASES 32

//How many revolutions the engine makes for one full cycle of the whole trigger table.
//Most 4-stroke engines will have a value of 2.
#define REVS_PER_CYCLE 2

//Number of outputs
#define N_OUTPUTS 4

//Max and min RPM (for 0 or 5V input)
#define MIN_RPM 300.0
#define MAX_RPM 8000.0

#define RPM_LOG 10 //Logarithmicness of RPM dial

#define USE_RPM_DIAL true

//Use second input for fine-tuning rpm
//#define USE_RPM_FINE true

//Digital output pins assigned for each output
const uint8_t output_pins[N_OUTPUTS] = {7, 8, 9, 6};

//Analog input pin for RPM dial (potentiometers as voltage dividers)
const uint8_t rpm_dial_pin = 0;
const uint8_t rpm_fine_pin = 1;

//Digital input pin for on/off toggle
const uint8_t run_pin = 13;

const float rpm_fine_part = 0.05; //Adjustment span of fine tune.

//4G63 trigger wheel setup (for some Mitsubishi engines, and many Miatas)

//First column is SGC, two pulses per camshaft rotation, one of which is longer
//Second is SGT, four pulses per camshaft rotation (one per cylinder)
//SGT pulses are delayed slightly after the SGC ones

//For Megasquirt SGC -> CMP+ (sometimes called CAM+), SGT -> CKP+

//Leave the corresponding "-" pins disconnected. They're used for different kind of sensors. The 5V pulse out
//of the arduino is appropriate for triggering the Megasquirt as if it where a hall or optical trigger

//This has been built largely on the information on http://www.rivercityroad.com/garage/cas.htm
//and has successfully provided timing info for a Megasquirt unit.
//Unknown what max speed is, but has been tested up to 30K RPM on an Arduino Uno.

//Third pin is a purely diagnostic output, to trigger once per cycle (useful as oscilloscope trigger)

//Fourth pin is just a "as fast as you can switch" output for VSS testing

//TDC is somewhere at step 3-4 for each cylider as marked below.

uint8_t phases[N_PHASES][N_OUTPUTS] =
{
  {1, 0, 1, 0},  //Cyl1
  {1, 1, 1, 1},
  {1, 1, 1, 0},
  {1, 1, 1, 1},
  {1, 1, 1, 0},
  {1, 0, 1, 1},
  {0, 0, 1, 0},
  {0, 0, 1, 1},
  {0, 0, 0, 0},  //Cyl3
  {0, 1, 0, 1},
  {0, 1, 0, 0},
  {0, 1, 0, 1},
  {0, 1, 0, 0},
  {0, 0, 0, 1},
  {0, 0, 0, 0},
  {0, 0, 0, 1},
  {1, 0, 0, 0},  //Cyl4
  {1, 1, 0, 1},
  {1, 1, 0, 0},
  {0, 1, 0, 1},
  {0, 1, 0, 0},
  {0, 0, 0, 1},
  {0, 0, 0, 0},
  {0, 0, 0, 1},
  {0, 0, 0, 0},  //Cyl2
  {0, 1, 0, 1},
  {0, 1, 0, 0},
  {0, 1, 0, 1},
  {0, 1, 0, 0},
  {0, 0, 0, 1},
  {0, 0, 0, 0},
  {0, 0, 0, 1}
};

float rpm = 1200.0;
int phase = 0;
int oldrpmval = 0;
int oldrpmfine = 0;

float time_per_phase;

//Time in microseconds per step
inline float stepTime(float rpm)
{
  //60M microseconds per minute
  //60M/rpm microseconds per rev
  return 60000000.0/rpm/N_PHASES*REVS_PER_CYCLE;
}


//Logarithmically scale input value (0-1023) to RPM
float scale_rpm(int val, int fine)
{
  #ifdef USE_RPM_FINE
  float p = 1.0*(val + rpm_fine_part*(fine-512))/1023;
  #else
  float p = 1.0*val/1023;
  #endif
  
  p = (pow(RPM_LOG, p)-1)/(RPM_LOG-1);
    
  return MIN_RPM + p*(MAX_RPM-MIN_RPM);
}

//Interrupt for one tick
void tick(void)
{
  //If engine is not running, do absolutely nothing
  if(!digitalRead(run_pin))
  {
    return;
  }
  
  //Move a step forward, and maybe wrap around
  if(++phase >= N_PHASES) phase=0;

  uint8_t *phaseOutputs = phases[phase];

  for(int i=0; i<N_OUTPUTS; ++i)
  {
    digitalWrite(output_pins[i], phaseOutputs[i]);
  }
  
}

void setup() {
  Serial.begin(115200);
  pinMode(run_pin, INPUT);
  for(int i=0; i<N_OUTPUTS; ++i)
  {
    pinMode(output_pins[i], OUTPUT);

    time_per_phase = stepTime(rpm);

    Timer1.initialize(time_per_phase);
    Timer1.attachInterrupt(tick);
  }
}


void loop() {
  //Serial.println(digitalRead(run_pin)?"ON":"OFF");
  if(USE_RPM_DIAL)
  {
    int newVal = analogRead(rpm_dial_pin);
    
    #ifdef USE_RPM_FINE
    int newFine = analogRead(rpm_fine_pin);
    #else
    int newFine = 512;
    #endif
    
    if( (abs(newVal-oldrpmval) > 0) || ((abs(newFine-oldrpmfine) > 1)))
    {
      rpm = scale_rpm(newVal, newFine);
      if(digitalRead(run_pin))
      {
        Serial.println(rpm);
      }
      else
      {
        //Serial.println("[stopped]");
      }
      oldrpmval = newVal;
      oldrpmfine = newFine;
      time_per_phase = stepTime(rpm);
      Timer1.setPeriod(time_per_phase);
    }
    
  }
  delay(100);
}

