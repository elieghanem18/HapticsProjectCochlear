#include "haptics.h"

byte DRV = 0x5A;
byte ModeReg = 0x01;
int cycle = 0;
int buttonPin = 5;

// Configure the PWM clock
#define PWM12k  5


// ================================================================================
// pulse drives the hammer towards the closed end of the TacHammer
// intensity defines the strength of the pulse. It ranges from [0-1] with 1 being strongest
// When the hammer rebounds off of the repelling magnetic array, the inaudible pulse haptic sensation is created
// milliseconds is the length of time the coil is activated in ms
// pulse is intended to be sequenced with subsequent pulse and hit commands and if called
// on its own, the hammer may travel after the rebound and strike the open end

void pulse(double intensity, double milliseconds) {
  int minimumint = 140;
  int maximumint = 255;
  int pwmintensity = (intensity * (maximumint - minimumint)) + minimumint;
  standbyOffB();
  PWM13 = pwmintensity;
  usdelay(milliseconds);
  standbyOnB();
}

// ================================================================================
// singlePulse includes a command to pulse the hammer followed by a command to brake the hammer.
// singlePulse is intended to be called on its own and should be followed by a pause command of
// at least 50ms before the next command is called
void singlePulse(double intensity, double milliseconds) {
  pulse(intensity, milliseconds);
  pause(3);
  pulse(intensity * 3 / 100, milliseconds * 2);
}

// ================================================================================
// hit drives the hammer towards the open end of the TacHammer
// intensity defines the strength of the hit. It ranges from [0-1] with 1 being strongest
// milliseconds is the length of time the coil is activated in ms
// When the hammer strikes the device housing, the audible click haptic sensation is created
void hit(double intensity, double milliseconds) {
  int minimumint = 0;
  int maximumint = 110;
  int pwmintensity = maximumint - (intensity * (maximumint - minimumint));
  standbyOffB();
  PWM13 = pwmintensity;
  usdelay(milliseconds);
  standbyOnB();
}

// ================================================================================
// waits for a duration of time defined in ms
void pause(double milliseconds) {
  double us = milliseconds - ((int)milliseconds);
  standbyOnB();
  for (int i = 0; i <= milliseconds; i++) {
    delay(1);
  }
  delayMicroseconds(us * 1000);
}

void usdelay(double time) {
  double us = time - ((int)time);
  for (int i = 0; i <= time; i++) {
    delay(1);
  }
  delayMicroseconds(us * 1000);
}

// ================================================================================
// Vibrate repeatedly calls the pulse command to drive the hammer into the closed end of the TacHammer
// intensity defines the strength of the vibrate. It ranges from [0-1] with 1 being strongest
// dutycycle defines what percent of the period the TacHammer is active. It ranges from 0-100
// frequency defines the frequency of the vibration
// duration defines the length of vibration in s
//
// suggested duty cycles for vibrate
// FREQ| 10| 30| 50| 70| 90|110|130|150|170|190|210|230|250|270|290
// DUTY| 32| 28| 33| 36| 38| 37| 41| 47| 45| 47| 39| 40| 44| 43| 40
void vibrate(double frequency, double intensity, double duration, int dutycycle) {
  int max_hit = 12;
  int min_hit = 1;
  int crossover = 60;
  int hitduration = 10 * dutycycle / frequency;

  boolean hold = false;
  double delayy;
  delayy = (1 / frequency * 1000) - hitduration;
  double timedown;
  timedown = duration * 1000;

  if (duration == 0) {
    hold =  true;
  }

  while (hold) {
    pulse(intensity, hitduration);
    pause(delayy);
  }

  while (timedown >= 0 && frequency < crossover) {
    pulse(intensity, hitduration);
    pause(3);
    pulse(0.002, delayy-3);

    timedown -= (delayy + hitduration);
  }

  while (timedown >= 0 && frequency >= crossover) {
    pulse(intensity, hitduration);
    pause(delayy);
    timedown -= (delayy + hitduration);
  }
}

// ================================================================================
// =============================== HAPTIC LIBRARY =========================[HAPLIB]
// ================================================================================
// the following are haptic effects created with the basic haptic function calls

// ================================================================================
// click from impact haptic
void impactClick(double intensity) {
    Serial.println("impact click");
    pulse(intensity, 6);
    hit(intensity, 21);
  }


// ================================================================================
// simulates an ERM rumble
void ermRumble() {
  Serial.println("ERM Rumble");
  vibrate(30,0.7,0.33,30);
}

// ================================================================================
// simulates an LRA Click
void lraClick() {
    Serial.println("LRA Click");
    vibrate(250,1,0.01,70);
}

// ================================================================================
// simulates a quickening heartbeat ("lub"-"dub")
void heartbeat() {
  Serial.println("Heartbeat");
  int hrate = 0;
  for (int i=0;i<16;i++){
    pulse(0.3, 45);        // "lub"
    pause(12);
    pulse(0.3, 3);          // short pulse to sharpen pulse haptic
    pause(250 - hrate);

    pulse(0.1, 45);        // "dub"
    pause(12);
    pulse(0.3, 3);          // short pulse to sharpen pulse haptic
    pause(510 - hrate);

    if (hrate < 200) {
      hrate += 15;
    }
  }   
}

// ================================================================================
// simulates charging then firing a laser. The laser is charged while the button
// is held down, then fires when let go
void laser() {
  Serial.print("Laser Charging");

  // LASER PISTOL CHARGING
  // The effect is created by linking a series of increasing frequency vibrations
  vibrate(35, 0.007, 0.07, 50);
  vibrate(40, 0.008, 0.04, 50);
  vibrate(45, 0.009, 0.04, 50);
  vibrate(60, 0.01, 0.04, 50);
  vibrate(75, 0.03, 0.04, 50);
  vibrate(80, 0.08, 0.04, 50);
  vibrate(85, 0.1, 0.04, 50);
  vibrate(88, 0.1, 0.04, 50);
  vibrate(90, 0.15, 0.04, 50);
  vibrate(130, 0.15, 0.04, 50);
  vibrate(140, 0.2, 0.04, 50);
  vibrate(160, 0.2, 0.04, 50);
  vibrate(200, 0.3, 0.05, 50);

  for (int i=0;i<4;i++){
    vibrate(200, 0.5, 0.01, 50);
    pulse(.1, 2);
    vibrate(200, 0.5, 0.01, 50);
    pulse(.7, 2);
    vibrate(200, 0.5, 0.01, 50);
    pulse(.95, 2);
    vibrate(200, 0.5, 0.01, 50);
    pulse(.7, 2);
    vibrate(200, 0.5, 0.01, 50);
    pulse(.1, 2);
  }
}

void laserRelease() {
  Serial.println("Laser Firing");
  // LASER FIRING
  // The effect is created by linking a series of decreasing intensity vibrations
  hit(0.5,3);
  pulse(1, 10);
  pause(2);
  pulse(0.002,5);
  vibrate(160, 0.8, 0.02, 50);
  vibrate(150, 0.5, 0.02, 50);
  vibrate(130, 0.3, 0.03, 50);
  vibrate(120, 0.1, 0.05, 50); 
}

// ================================================================================
// simulates firing an automatic rifle. The audible mode can be toggled off
// DEFAULT: AUDIBLE
void rifle(bool audible) {
  if (audible) {
    Serial.println("Loud Rifle");
    // a short pulse is sent before the hit to increase the stroke of the hammer
    // so that when the hit is activated the hammer provides a stronger haptic
    pause(20);
    pulse(1, 6);
    hit(1, 21);
  }

  else {
    Serial.println("Quiet Rifle");
    // a short hit is sent before the pulse to increase the stroke of the hammer
    // so that when the pulse is activated the hammer provides a stronger haptic
    hit(1,3);
    pulse(1,21);


    // a short pulse is sent after a strong pulse to catch the hammer as it rebounds
    // to ensure it does not contact the end of the controller to provde an inaudible
    // haptic      
    pause(2);
    pulse(0.002,21);
  }
}

// ================================================================================
// simulates firing a pump shotgun. Two effects are felt:
//    1. shotgun firing
//    2. pumping in the next round
//
// The audible mode can be toggled off
// DEFAULT: AUDIBLE
void shotgun(bool audible, bool reload) {
  if (!reload){
    if (audible) {
        Serial.print("Loud Shotgun Firing");

        //shotgun firing
        pulse(1, 6);
        hit(1, 21);
        pause(250);
    }

    else {
      Serial.print("Quiet Shotgun Firing");

      //shotgun firing
      hit(1, 3);
      singlePulse(1, 21);
      pause(200);
    }
  }

  else{
    if (audible) {
      Serial.println("Loud Shotgun Reloading");

      //shotgun reloading
      pulse(.5, 30);
      hit(.35, 10);
      pause(180);
      hit(.37, 17);      
    }

    else {
      Serial.println("Quiet Shotgun Reloading");

      //shotgun reloading
      pulse(.3, 30);
      pulse(.55, 30);
      pause(2.8);
      pulse(0.03, 60);
      pause(115);
      pulse(.55, 30);
      pause(3);
      pulse(0.03, 60);      
    }
  }
}



// ================================================================================
// =============================== TEST FUNCTIONS =========================[TSTFNC]
// ================================================================================
// the following are test functions that can be called to test the operation of the
// TacHammer component. They are not currently bound to any buttons

// ================================================================================
// plays a series of vibrates at varying frequencies and duty cycles
void vibrateRamp() {
  double i = 0;
  for (int j=0;j<20;j++){
    Serial.print("Vibrate Frequency: ");
    Serial.println(250-i);

    vibrate(200 - i, 0.7, 0.2, 70);

    if (i < 210) {
      i += 10;
    }
  }  
}

// ================================================================================
// sweeps the Pulse command from 0-1 intensity
void pulseRamp() {
  double i = 0;
    for (int j=0;j<20;j++){
    Serial.print("Hit Intensity: ");
    Serial.println(i);

    pulse(i, 6);
    hit(i, 21);
    pause(100);
    if (i < 0.99) {
      i += 0.05;
    }
  }
}

// ================================================================================
// sweeps the Hit command from 0-1 intensity
void hitRamp() {
  double i = 0;
    for (int j=0;j<20;j++){
    Serial.print("Hit Intensity: ");
    Serial.println(i);

    pulse(i, 6);
    hit(i, 21);
    pause(100);
    if (i < 0.99) {
      i += 0.05;
    }
  }
}

// ================================================================================
// =============================== DRV SETUP ============================= [DRVSET]
// ================================================================================
// sets up the DRV2605 to drive the TacHammer components. It is strongly suggested
// to not edit this section
void standbyOnB() {
  Wire.beginTransmission(DRV);
  Wire.write(ModeReg);               // sets register pointer to the mode register (0x01)
  Wire.write(0x43);               // Puts the device pwm mode
  Wire.endTransmission();
}

void standbyOffB() {
  Wire.beginTransmission(DRV);
  Wire.write(ModeReg);               // sets register pointer to the mode register (0x01)
  Wire.write(0x03);               // Sets Waveform Mode to pwm
  Wire.endTransmission();
}

void initializeDRV2605() {
  Wire.beginTransmission(DRV);
  Wire.write(ModeReg);               // sets register pointer to the mode register (0x01)
  Wire.write(0x00);               // clear standby
  Wire.endTransmission();

  Wire.beginTransmission(DRV);
  Wire.write(0x1D);              // sets register pointer to the Libarary Selection register (0x1D)
  Wire.write(0xA8);              // set RTP unsigned
  Wire.endTransmission();

  Wire.beginTransmission(DRV);
  Wire.write(0x03);
  Wire.write(0x02);              // set to Library B, most aggresive
  Wire.endTransmission();

  Wire.beginTransmission(DRV);
  Wire.write(0x17);               // sets full scale reference
  Wire.write(0xff);               //
  Wire.endTransmission();

  Wire.beginTransmission(DRV);
  Wire.write(ModeReg);               // sets register pointer to the mode register (0x01)
  Wire.write(0x03);               // Sets Mode to pwm
  Wire.endTransmission();

  delay(100);

}

void pwm613configure() {
  TCCR4A = 0;
  TCCR4B = PWM12k;
  TCCR4C = 0;
  TCCR4D = 0;

  PLLFRQ = (PLLFRQ & 0xCF) | 0x30;
  OCR4C = 255;
}

void pwm910configure() {
  TCCR1A = 0;
  TCCR1B = 0;
  TCCR1C = 0;

  TCCR1A |= _BV(WGM10);   // Fast PWM 8-bit
  TCCR1B |= _BV(WGM12);
  TCCR1B |= _BV(CS10);    // no prescaler
}

void pwmSet6() {
  OCR4D = 0;
  DDRD |= _BV(7);
  TCCR4C |= _BV(COM4D1) | _BV(PWM4D);
}

void pwmSet13() {
  OCR4A = 0;
  DDRC |= _BV(7);
  TCCR4A |= _BV(COM4A1) | _BV(PWM4A);
}

void pwmSet9() {
  OCR1A = 0;
  DDRB |= _BV(5);
  TCCR1A |= _BV(COM1A1);
}

void pwmSet10() {
  OCR1B = 0;
  DDRB |= _BV(6);
  TCCR1A |= _BV(COM1B1);
}
