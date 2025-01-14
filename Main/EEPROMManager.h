#ifndef EEPROMMANAGER_H
#define EEPROMMANAGER_H

void retrieveSerialSpeedEEPROM();
void retrieveSensorDistanceEEPROM();
void saveSerialSpeedEEPROM(int newSpeed);
void saveSensorDistanceEEPROM(float newDistance);

#endif