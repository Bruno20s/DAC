#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>
#include "LCDManager.h"
#include "SensorManager.h"
#include "EEPROMManager.h"
#include "CommandManager.h"
#include "globals.h"

void setup() {
    initializeLCD();
    initializeSensors();
    retrieveSerialSpeedEEPROM();
    Serial.begin(serialSpeed);
    retrieveSensorDistanceEEPROM();
}

void loop() {
    while (Serial.available()) {
        char input = Serial.read();
        checkCommand(input);
    }

    checkSwitches();

    if (systemActive == 1) {
        digitalWrite(ledPin, HIGH);
        checkSequence(0);  // Side A
        checkSequence(1);  // Side B
        checkSimultaneousDoubleWheels();

    } else {
        digitalWrite(ledPin, LOW);
        calculateSpeed();
    }

    delay(2);
}