#include "haptics.h"
#include <Wire.h>
#include <ODriveUART.h>
#include <math.h>
#include <SoftwareSerial.h>

#define ODRIVE_RX 10
#define ODRIVE_TX 11

const byte TCA_ADDR = 0x70;

const float EXO_TARGET_SCALE = 0.08f / 450.0f;
const float EXO_TARGET_OFFSET = 0.0f;
const float EXO_TARGET_DEADBAND = 0.0001f;
const float EXO_APPROACH_SPEED = 2.0f;
const float EXO_POSITION_FILTER_BANDWIDTH = 20.0f;
const unsigned long EXO_TIMEOUT_MS = 5000;
const unsigned long EXO_STATUS_PERIOD_MS = 100;

const int NUM_MOTORS = 4;
const float DEPTH_RANGE = 405.0f;
const float INTENSITY_SINGLE = 1.0f;
const float INTENSITY_DOUBLE = 0.5f;
const float LOW_SPEED_LIMIT = 0.25f;
const float MEDIUM_SPEED_LIMIT = 0.5f;
const float FAST_SPEED_LIMIT = 0.75f;
const float MAX_SPEED_LIMIT = 1.0f;

SoftwareSerial odriveSerial(ODRIVE_RX, ODRIVE_TX);
ODriveUART odrive(odriveSerial);

float exoTargetPos = 0.0f;
float currentPos = 0.0f;
float currentVel = 0.0f;
bool exoEnabled = false;
bool exoHasTarget = false;
bool completionLimit = true;
unsigned long lastDataTime = 0;

/* ===================== EXOSKELETON ACTUATION ===================== */

void setODrivePositionMode() {
  odriveSerial.print("w axis0.controller.config.vel_limit ");
  odriveSerial.println(EXO_APPROACH_SPEED, 3);
  delay(20);

  odriveSerial.println("w axis0.controller.config.control_mode 3");
  delay(20);

  odriveSerial.println("w axis0.controller.config.input_mode 3");
  delay(20);

  odriveSerial.print("w axis0.controller.config.input_filter_bandwidth ");
  odriveSerial.println(EXO_POSITION_FILTER_BANDWIDTH, 3);
  delay(20);
}

void releaseExoskeleton() {
  Serial.println("Releasing exoskeleton");
  odrive.setState(AXIS_STATE_IDLE);
  exoEnabled = false;
  exoHasTarget = false;
}

void testODriveCommunication() {
  Serial.println("Testing ODrive UART...");

  odriveSerial.println("r axis0.current_state");
  String stateReply = odriveSerial.readStringUntil('\n');
  Serial.print("ODrive current_state: ");
  Serial.println(stateReply);

  odriveSerial.println("r axis0.error");
  String axisErrorReply = odriveSerial.readStringUntil('\n');
  Serial.print("ODrive axis error: ");
  Serial.println(axisErrorReply);

  odriveSerial.println("r axis0.motor.error");
  String motorErrorReply = odriveSerial.readStringUntil('\n');
  Serial.print("ODrive motor error: ");
  Serial.println(motorErrorReply);
}

void enableExoskeleton() {
  odrive.clearErrors();
  delay(50);
  setODrivePositionMode();
  odrive.setState(AXIS_STATE_CLOSED_LOOP_CONTROL);
  delay(100);
  exoEnabled = true;
}

void ExoActuation(float depthValue) {
  lastDataTime = millis();

  float newTarget = EXO_TARGET_OFFSET - depthValue * EXO_TARGET_SCALE;

  if (!exoHasTarget) {
    exoTargetPos = newTarget;
    exoHasTarget = true;

    enableExoskeleton();
    odrive.setPosition(exoTargetPos);

    Serial.print("Exo target: ");
    Serial.println(exoTargetPos, 4);
    return;
  }

  if (!exoEnabled) {
    enableExoskeleton();
  }

  if (fabs(newTarget - exoTargetPos) > EXO_TARGET_DEADBAND) {
    exoTargetPos = newTarget;
    odrive.setPosition(exoTargetPos);

    Serial.print("Exo target: ");
    Serial.println(exoTargetPos, 4);
  }
}

void updateExoskeletonStatus() {
  static unsigned long lastStatusTime = 0;

  if (!exoHasTarget) return;

  if (millis() - lastDataTime > EXO_TIMEOUT_MS) {
    if (exoEnabled) {
      Serial.println("No data for 5 seconds");
      releaseExoskeleton();
    }
    return;
  }

  if (!exoEnabled) return;
  if (millis() - lastStatusTime < EXO_STATUS_PERIOD_MS) return;

  lastStatusTime = millis();

  auto feedback = odrive.getFeedback();
  currentPos = feedback.pos;
  currentVel = feedback.vel;
  float positionError = currentPos - exoTargetPos;

  Serial.print("Exo target: ");
  Serial.print(exoTargetPos, 4);
  Serial.print(" Pos: ");
  Serial.print(currentPos, 4);
  Serial.print(" Error: ");
  Serial.print(positionError, 4);
  Serial.print(" Vel: ");
  Serial.println(currentVel, 4);
}

/* ===================== HAPTIC MOTORS ===================== */

bool selectMotor(int motorIndex) {
  if (motorIndex < 0 || motorIndex > 7) return false;

  Wire.beginTransmission(TCA_ADDR);
  Wire.write(1 << motorIndex);
  byte err = Wire.endTransmission();

  if (err != 0) {
    Serial.print("TCA select failed ch=");
    Serial.print(motorIndex);
    Serial.print(" err=");
    Serial.println(err);
    return false;
  }

  delay(2);
  return true;
}

bool selectTwoMotors(int motorIndex1, int motorIndex2) {
  if (motorIndex1 < 0 || motorIndex1 > 7) return false;
  if (motorIndex2 < 0 || motorIndex2 > 7) return false;

  Wire.beginTransmission(TCA_ADDR);
  Wire.write((1 << motorIndex1) | (1 << motorIndex2));
  byte err = Wire.endTransmission();

  if (err != 0) {
    Serial.print("TCA dual select failed ch=");
    Serial.print(motorIndex1);
    Serial.print(",");
    Serial.print(motorIndex2);
    Serial.print(" err=");
    Serial.println(err);
    return false;
  }

  delay(2);
  return true;
}

void insertionComplete() {
  pause(200);

  for (int i = 0; i < 2; i++) {
    for (int m = 0; m < NUM_MOTORS; m++) {
      selectMotor(m);
      vibrate(50, 0.1, 0.05, 33);
      pause(60);
      vibrate(130, 1, 0.1, 41);
    }
  }

  pause(200);
  completionLimit = false;
}

void mediumSpeed(float speedValue, bool twoMotors) {
  float intensity = twoMotors ? INTENSITY_DOUBLE : INTENSITY_SINGLE;

  if (speedValue < LOW_SPEED_LIMIT) {
    pulse(intensity, 7);
    hit(intensity, 7);
    pause(152.5);
    pulse(intensity, 7);
    hit(intensity, 7);
  } else if (speedValue < MEDIUM_SPEED_LIMIT) {
    pulse(intensity, 7);
    hit(intensity, 7);
    pause(97);
    pulse(intensity, 7);
    hit(intensity, 7);
    pause(97);
    pulse(intensity, 7);
    hit(intensity, 7);
  } else if (speedValue < FAST_SPEED_LIMIT) {
    pulse(intensity, 7);
    hit(intensity, 7);
    pause(69.25);
    pulse(intensity, 7);
    hit(intensity, 7);
    pause(69.25);
    pulse(intensity, 7);
    hit(intensity, 7);
    pause(69.25);
    pulse(intensity, 7);
    hit(intensity, 7);
  } else {
    pulse(intensity, 7);
    hit(intensity, 7);
    pause(52.6);
    pulse(intensity, 7);
    hit(intensity, 7);
    pause(52.6);
    pulse(intensity, 7);
    hit(intensity, 7);
    pause(52.6);
    pulse(intensity, 7);
    hit(intensity, 7);
    pause(52.6);
    pulse(intensity, 7);
    hit(intensity, 7);
  }
}

void controlHapticMotors(float speedValue, float depthValue) {
  if (depthValue == 0.0f) completionLimit = true;

  if (depthValue > DEPTH_RANGE - 5.0f && completionLimit) {
    insertionComplete();
  }

  float hapticAngle = depthValue;
  while (hapticAngle < 0.0f) hapticAngle += 360.0f;
  while (hapticAngle >= 360.0f) hapticAngle -= 360.0f;

  float angleInc = 360.0f / NUM_MOTORS;
  int motorIndex = (int)(hapticAngle / angleInc);
  int nextMotor = (motorIndex + 1) % NUM_MOTORS;
  float midAngle = angleInc * motorIndex + angleInc / 2.0f;
  bool twoMotors = hapticAngle >= midAngle;

  if (!twoMotors) {
    selectMotor(motorIndex);

    if (speedValue < 0.05f) {
      pulse(INTENSITY_SINGLE, 7);
      hit(INTENSITY_SINGLE, 7);
    } else if (speedValue < MAX_SPEED_LIMIT) {
      mediumSpeed(speedValue, false);
    } else {
      vibrate(130, INTENSITY_SINGLE, 0.2, 41);
    }
  } else {
    selectTwoMotors(motorIndex, nextMotor);

    if (speedValue < 0.05f) {
      pulse(INTENSITY_DOUBLE, 7);
      hit(INTENSITY_DOUBLE, 7);
    } else if (speedValue < MAX_SPEED_LIMIT) {
      mediumSpeed(speedValue, true);
    } else {
      vibrate(130, INTENSITY_DOUBLE, 0.2, 41);
    }
  }

  Serial.print("Depth: ");
  Serial.print(depthValue);
  Serial.print(" Motor: ");
  Serial.print(motorIndex);
  Serial.print(" Next: ");
  Serial.print(nextMotor);
  Serial.print(" Two: ");
  Serial.println(twoMotors);
}

/* ===================== COMMUNICATION AND MAIN LOOP ===================== */

void readBluetoothData() {
  static String buffer = "";

  while (Serial1.available() > 0) {
    char c = Serial1.read();

    if (c == '\n' || c == '\r') {
      if (buffer.length() > 0) {
        int commaIndex = buffer.indexOf(',');

        if (commaIndex > 0) {
          float speedValue = fabs(buffer.substring(0, commaIndex).toFloat());
          float depthValue = buffer.substring(commaIndex + 1).toFloat();

          ExoActuation(depthValue);
          controlHapticMotors(speedValue, depthValue);
        }

        buffer = "";
      }
    } else {
      buffer += c;
      if (buffer.length() > 30) buffer = "";
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Setup started");

  Wire.begin();
  Wire.setClock(100000);
  Wire.setWireTimeout(2500, true);

  Serial1.begin(9600);
  odriveSerial.begin(115200);
  odriveSerial.setTimeout(100);
  testODriveCommunication();
  odriveSerial.setTimeout(20);

  initializeDRV2605();

  unsigned long startTime = millis();

  while (odrive.getState() == AXIS_STATE_UNDEFINED && millis() - startTime < 3000) {
    delay(100);
    Serial.println("Waiting for ODrive...");
  }

  if (odrive.getState() == AXIS_STATE_UNDEFINED) {
    Serial.println("ODrive not detected");
  } else {
    Serial.println("ODrive detected");
    odrive.clearErrors();
    delay(100);
    odrive.setState(AXIS_STATE_IDLE);
    delay(200);
    Serial.println("ODrive ready");
  }
}

void loop() {
  readBluetoothData();
  updateExoskeletonStatus();
}
