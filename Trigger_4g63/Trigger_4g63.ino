//Simple 4G63 trigger wheel simulator for Arduino
//Copyright 2016 John Stäck
//Licensed under GNU General Public License v3

#include <TimerOne.h>


//How many revolutions the engine makes for one full cycle of the whole trigger table.
//Most 4-stroke engines will have a value of 2.
#define REVS_PER_CYCLE 2

//Number of outputs
#define N_OUTPUTS 4

//Max and min RPM (for 0 or 5V input)
#define MIN_RPM 300.0
#define MAX_RPM 8000.0

#define RPM_LOG 10 //Logarithmicness of RPM dial

#define USE_RPM_DIAL true //Use potentiometer for setting RPM, if false use hardcoded value

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

//Mostly based on chart in http://speeduino.com/wiki/index.php/4G63. Also thanks to noisymime.
//Third pin is a purely diagnostic output, to trigger once per cycle (useful as oscilloscope trigger)

//Fourth pin is just a "as fast as you can switch" output for VSS testing


//Each "phase" (step) is 5 degrees, whole cycle is 720 degrees.
#define N_PHASES 144

uint8_t phases[N_PHASES][N_OUTPUTS] =
{
  {0, 0, 1, 0}, //  0
  {0, 0, 1, 1}, //  5
  {0, 0, 1, 0}, // 10
  {0, 0, 1, 1}, // 15
  {0, 0, 1, 0}, // 20
  {0, 0, 1, 1}, // 25
  {0, 0, 1, 0}, // 30
  {0, 0, 1, 1}, // 35
  {0, 0, 1, 0}, // 40
  {0, 0, 1, 1}, // 45
  {0, 0, 1, 0}, // 50
  {0, 0, 1, 1}, // 55
  {0, 0, 1, 0}, // 60
  {0, 0, 1, 1}, // 65
  {0, 0, 1, 0}, // 70
  {0, 0, 1, 1}, // 75
  {0, 0, 1, 0}, // 80
  {0, 0, 1, 1}, // 85
  {0, 0, 0, 0}, // 90
  {0, 0, 0, 1}, // 95
  {0, 0, 0, 0}, //100
  {1, 0, 0, 1}, //105
  {1, 0, 0, 0}, //110
  {1, 0, 0, 1}, //115
  {1, 0, 0, 0}, //120
  {1, 0, 0, 1}, //125
  {1, 0, 0, 0}, //130
  {1, 0, 0, 1}, //135
  {1, 0, 0, 0}, //140
  {1, 0, 0, 1}, //145
  {1, 0, 0, 0}, //150
  {1, 0, 0, 1}, //155
  {1, 0, 0, 0}, //160
  {1, 0, 0, 1}, //165
  {1, 0, 0, 0}, //170
  {0, 0, 0, 1}, //175
  {0, 0, 0, 0}, //180
  {0, 0, 0, 1}, //185
  {0, 0, 0, 0}, //190
  {0, 0, 0, 1}, //195
  {0, 0, 0, 0}, //200
  {0, 0, 0, 1}, //205
  {0, 0, 0, 0}, //210
  {0, 0, 0, 1}, //215
  {0, 0, 0, 0}, //220
  {0, 0, 0, 1}, //225
  {0, 0, 0, 0}, //230
  {0, 0, 0, 1}, //235
  {0, 0, 0, 0}, //240
  {0, 0, 0, 1}, //245
  {0, 0, 0, 0}, //250
  {0, 0, 0, 1}, //255
  {0, 0, 0, 0}, //260
  {0, 0, 0, 1}, //265
  {0, 1, 0, 0}, //270
  {0, 1, 0, 1}, //275
  {0, 1, 0, 0}, //280
  {1, 1, 0, 1}, //285
  {1, 1, 0, 0}, //290
  {1, 1, 0, 1}, //295
  {1, 1, 0, 0}, //300
  {1, 1, 0, 1}, //305
  {1, 1, 0, 0}, //310
  {1, 1, 0, 1}, //315
  {1, 1, 0, 0}, //320
  {1, 1, 0, 1}, //325
  {1, 1, 0, 0}, //330
  {1, 1, 0, 1}, //335
  {1, 1, 0, 0}, //340
  {1, 1, 0, 1}, //345
  {1, 1, 0, 0}, //350
  {0, 1, 0, 1}, //355
  {0, 1, 0, 0}, //360
  {0, 1, 0, 1}, //365
  {0, 1, 0, 0}, //370
  {0, 1, 0, 1}, //375
  {0, 1, 0, 0}, //380
  {0, 1, 0, 1}, //385
  {0, 1, 0, 0}, //390
  {0, 1, 0, 1}, //395
  {0, 1, 0, 0}, //400
  {0, 1, 0, 1}, //405
  {0, 0, 0, 0}, //410
  {0, 0, 0, 1}, //415
  {0, 0, 0, 0}, //420
  {0, 0, 0, 1}, //425
  {0, 0, 0, 0}, //430
  {0, 0, 0, 1}, //435
  {0, 0, 0, 0}, //440
  {0, 0, 0, 1}, //445
  {0, 0, 0, 0}, //450
  {0, 0, 0, 1}, //455
  {0, 0, 0, 0}, //460
  {1, 0, 0, 1}, //465
  {1, 0, 0, 0}, //470
  {1, 0, 0, 1}, //475
  {1, 0, 0, 0}, //480
  {1, 0, 0, 1}, //485
  {1, 0, 0, 0}, //490
  {1, 0, 0, 1}, //495
  {1, 0, 0, 0}, //500
  {1, 0, 0, 1}, //505
  {1, 0, 0, 0}, //510
  {1, 0, 0, 1}, //515
  {1, 0, 0, 0}, //520
  {1, 0, 0, 1}, //525
  {1, 0, 0, 0}, //530
  {0, 0, 0, 1}, //535
  {0, 0, 0, 0}, //540
  {0, 0, 0, 1}, //545
  {0, 0, 0, 0}, //550
  {0, 0, 0, 1}, //555
  {0, 0, 0, 0}, //560
  {0, 0, 0, 1}, //565
  {0, 0, 0, 0}, //570
  {0, 0, 0, 1}, //575
  {0, 0, 0, 0}, //580
  {0, 0, 0, 1}, //585
  {0, 0, 0, 0}, //590
  {0, 0, 0, 1}, //595
  {0, 0, 0, 0}, //600
  {0, 0, 0, 1}, //605
  {0, 0, 0, 0}, //610
  {0, 0, 0, 1}, //615
  {0, 0, 0, 0}, //620
  {0, 0, 0, 1}, //625
  {0, 1, 0, 0}, //630
  {0, 1, 0, 1}, //635
  {0, 1, 0, 0}, //640
  {1, 1, 0, 1}, //645
  {1, 1, 0, 0}, //650
  {1, 1, 0, 1}, //655
  {1, 1, 0, 0}, //660
  {1, 1, 0, 1}, //665
  {1, 1, 0, 0}, //670
  {1, 1, 0, 1}, //675
  {1, 1, 0, 0}, //680
  {1, 1, 0, 1}, //685
  {1, 1, 0, 0}, //690
  {1, 1, 0, 1}, //695
  {1, 1, 0, 0}, //700
  {1, 0, 0, 1}, //705
  {1, 0, 0, 0}, //710
  {0, 0, 0, 1}  //715
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

