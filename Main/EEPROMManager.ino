#include "EEPROMManager.h"
#include <EEPROM.h>

void retrieveSerialSpeedEEPROM() {
    EEPROM.get(serialSpeedAddress, serialSpeed);
    // Check if the retrieved speed is valid
    if (serialSpeed != 2400 && serialSpeed != 4800 && 
        serialSpeed != 9600 && serialSpeed != 14400 && 
        serialSpeed != 19200) {
        // If the retrieved value is not a valid speed, set it to the default 9600
        serialSpeed = 9600;
    }
}

void retrieveSensorDistanceEEPROM() {
    EEPROM.get(sensorDistanceAddress, sensorDistance);
    // Check if the retrieved distance is valid
    if (isnan(sensorDistance)) {
        sensorDistance = 0.4; // Default value
    }
}

void retriveLCDConfigEEPROM(){
EEPROM.get(lcdConfigAddress, lcdColumns);
  EEPROM.get(lcdConfigAddress + sizeof(int), lcdRows);
}

void saveSerialSpeedEEPROM(int newSpeed) {
    EEPROM.put(serialSpeedAddress, newSpeed);
}

void saveSensorDistanceEEPROM(float newDistance) {
    EEPROM.put(sensorDistanceAddress, newDistance);
}

void saveLCDConfigEEPROM(int columns, int rows) {
  EEPROM.put(lcdConfigAddress, columns);
  EEPROM.put(lcdConfigAddress + sizeof(int), rows);
}