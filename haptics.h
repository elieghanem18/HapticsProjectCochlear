#ifndef HAPTICS_H
#define HAPTICS_H

#include <Arduino.h>
#include <Wire.h>

#define PWM6    OCR4D
#define PWM9    OCR1AL
#define PWM10   OCR1BL
#define PWM13   OCR4A

extern byte DRV;
extern byte ModeReg;
extern int cycle;
extern int buttonPin;

void pulse(double intensity, double milliseconds);
void singlePulse(double intensity, double milliseconds);
void hit(double intensity, double milliseconds);
void pause(double milliseconds);
void usdelay(double time);
void vibrate(double frequency, double intensity, double duration, int dutycycle);

void impactClick(double intensity);
void ermRumble();
void lraClick();
void heartbeat();
void laser();
void laserRelease();
void rifle(bool audible);
void shotgun(bool audible, bool reload);

void vibrateRamp();
void pulseRamp();
void hitRamp();

void standbyOnB();
void standbyOffB();
void initializeDRV2605();

void pwm613configure();
void pwm910configure();
void pwmSet6();
void pwmSet13();
void pwmSet9();
void pwmSet10();

#endif