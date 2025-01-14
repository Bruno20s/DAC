#include "SensorManager.h"
#include <Arduino.h>

void initializeSensors() {
    for (int side = 0; side < 2 ; side++) {
        for (int i = 0; i < 4; i++) {
            pinMode(sensorPins[side][i], INPUT_PULLUP); // Set sensor pins as input with pull-up
            sensorStates[side][i] = LOW; // Initialize sensor states
            sensorTimings[side][i] = 0; // Initialize sensor timings
        }
    }
    pinMode(ledPin, OUTPUT);
  pinMode(switchBO, INPUT_PULLUP);
  pinMode(switchDMM, INPUT_PULLUP);
}

void checkSequence(int side) {
    for (int i = 0; i < 4; i++) {
        if (debounce(side, i)) {
            if (sensorStates[side][i] == HIGH) {
                sensorTimings[side][i] = millis();
                sensorCount[i + (side == 1 ? 4 : 0)]++;

                if (sequenceState[side] == i) {
                    sequenceState[side]++;
                    Serial.print("# Correct sequence: Sensor ");
                    Serial.print(i + 1);
                    Serial.print(" (side ");
                    Serial.print(side == 0 ? "A" : "B");
                    Serial.println(")!");

                    if (i < 2) {
                        axisCounters[side][i]++;
                    }

                    if (i == 3) {
                        sequenceState[side] = 0;
                        Serial.print("# Double wheels completed on side ");
                        Serial.println(side == 0 ? "A" : "B");
                    }

                    if (i >= 2) {
                        speedWithDoubleWheelSensor[side][i - 2] = true;
                    }
                } else if (sequenceState[side] > i) {
                    Serial.print("# Backtracking detected on side ");
                    Serial.println(side == 0 ? "A" : "B");

                    if (sequenceState[side] == 2 && axisCounters[side][1] > 0) {
                        axisCounters[side][1]--;
                    }
                    if (sequenceState[side] == 1 && axisCounters[side][0] > 0) {
                        axisCounters[side][0]--;
                    }
                    sequenceState[side]--;
                    reverseDetected = true;
                }
            }
        }
    }
}

void checkSimultaneousDoubleWheels() {
    for (int side = 0; side < 2; side++) {
        if (digitalRead(sensorPins[side][2]) == LOW && digitalRead(sensorPins[side][3]) == LOW) {
            if (!doubleWheelDetected[side]) {
                doubleWheelCounters[side]++;
                doubleWheelDetected[side] = true; // Set the flag to true
                Serial.print("Simultaneous double wheels detected on side ");
                Serial.println(side == 0 ? "A" : "B");
            }
        } else {
            doubleWheelDetected[side] = false; // Reset the flag
        }
    }
}

void checkSwitches() {
  int currentDMMState = !digitalRead(switchDMM);
  int currentBOState = !digitalRead(switchBO);

  systemActive = currentDMMState && currentBOState;

  if (systemActive && lastSystemActive == LOW) {
    lcd.print("Start of advance");
    switchStartTime = millis();
    resetSpeed();
    speedDisplayed = false; // Allows recalculating when the system is active
  }

  if (!currentBOState && lastSystemActive == HIGH) {
    switchDuration = millis() - switchStartTime;
    Serial.println("End of advance");
    showReport();
    saveSensorDistanceEEPROM(sensorDistance);
    resetSensorCounts();
  }

  lastSystemActive = systemActive;
}

// Debounce function to check if the sensor was truly triggered
bool debounce(int side, int sensor) {
  int currentState = !digitalRead(sensorPins[side][sensor]);
  unsigned long currentTime = millis();

  if (currentState != sensorStates[side][sensor]) {
    if (currentTime - lastSensorTime[side][sensor] > debounceTime) {
      sensorStates[side][sensor] = currentState;
      lastSensorTime[side][sensor] = currentTime;
      return true;
    }
  }

  return false;
}

void calculateSpeed() {
  if (!speedDisplayed) { // Ensures speed is displayed only once
    for (int i = 0; i < 2; i++) {
      if (speedWithDoubleWheelSensor[i][0] && speedWithDoubleWheelSensor[i][1]) {
        unsigned long totalTime = (i == 0 ? sensorTimings[0][3] : sensorTimings[1][3]) - 
                                  (i == 0 ? sensorTimings[0][2] : sensorTimings[1][2]);      
        if (totalTime > 0) {
          float speed = sensorDistance / (totalTime / 1000.0);  // Speed calculation
          Serial.print("Speed side ");
          Serial.print(i == 0 ? "A" : "B");
          Serial.print(": ");
          Serial.print(speed);
          Serial.println(" m/s");
        }
      }
    }
    speedDisplayed = true;  // Ensures speed is displayed only once per cycle
  }
}

void resetSensorCounts() {
    // Reset sensor counts
    memset(sensorCount, 0, sizeof(sensorCount));
    
    // Reset axis and double wheel counts
    memset(axisCounters, 0, sizeof(axisCounters));  
    memset(doubleWheelCounters, 0, sizeof(doubleWheelCounters));
}