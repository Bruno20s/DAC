#ifndef GLOBALS_H
#define GLOBALS_H

#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>

// Pins for LCD
extern LiquidCrystal_I2C lcd;

extern int lcdColumns; 
extern int lcdRows;     
extern int lcdConfigAddress;

// Combine the sensor pins for Side A and Side B
extern const int sensorPins[2][4];
extern const int switchDMM;
extern const int switchBO;
extern const int ledPin;

extern const int axisAddresses[2][3];
extern int sensorDistanceAddress;
extern int serialSpeedAddress;

extern float sensorDistance;
extern int serialSpeed;
extern int option;

// States and counters
extern int systemActive;
extern int lastSystemActive;
extern unsigned long switchStartTime;
extern unsigned long switchDuration;

// Sensor states and timings
extern int sensorStates[2][4];
extern unsigned long sensorTimings[2][4];

// Counters
extern int axisCounters[2][2];
extern int doubleWheelCounters[2];
extern int sensorCount[8];

extern bool doubleWheelDetected[2];
extern bool speedWithDoubleWheelSensor[2][2];

extern int sequenceState[2];

extern bool reverseDetected;
extern bool speedDisplayed;

// Arrays for debounce
extern unsigned long lastSensorTime[2][4];
extern unsigned long debounceTime;

void retrieveSerialSpeedEEPROM();
void retrieveSensorDistanceEEPROM();
void saveSerialSpeedEEPROM(int newSpeed);
void saveSensorDistanceEEPROM(float newDistance);
void showReport();
void initializeSensors();
void checkSequence(int side);
void checkSimultaneousDoubleWheels();
void resetSensorCounts();
void checkCommand(char input);
void executeCommand(String command);
void configureSensorDistance(String command);
void configureSerialSpeed(String command);
void checkSwitches();
void calculateSpeed();

#endif