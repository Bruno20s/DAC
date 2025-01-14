#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>

// Pins for LCD
LiquidCrystal_I2C lcd(0x27,16,2);

int lcdColumns = 16; 
int lcdRows = 2;    
int lcdConfigAddress = 16; 

// Combine the sensor pins for Side A and Side B
const int sensorPins[2][4] = {{7, 8, 9, 10}, // Side A: Axis1, Axis2, DW1, DW2
                              {6, 5, 4, 3}}; // Side B: Axis1, Axis2, DW1, DW2
#define NSIDE 2
const int switchDMM = 11;
const int switchBO = 2;
const int ledPin = 13;

const int axisAddresses[2][3] = {{0, 2, 4},  // EEPROM addresses for Side A: Axis1, Axis2, Double Wheels
                                 {6, 8, 10}}; // EEPROM addresses for Side B: Axis1, Axis2, Double Wheels

int sensorDistanceAddress = 12; // Address in EEPROM
int serialSpeedAddress = 14;

float sensorDistance = 0.4;
int serialSpeed = 9600; // Initial serial port speed
int option = 3; // Initial value

// States and counters
int systemActive = 0;
int lastSystemActive = LOW;
unsigned long switchStartTime = 0;
unsigned long switchDuration = 0;

// Sensor states and timings
int sensorStates[2][4] = {{LOW, LOW, LOW, LOW}, {LOW, LOW, LOW, LOW}};
unsigned long sensorTimings[2][4] = {{0, 0, 0, 0}, {0, 0, 0, 0}};

// Counters
int axisCounters[2][2] = {{0, 0}, {0, 0}};  // Axis 1 and 2 for Side A and Side B
int doubleWheelCounters[2] = {0, 0};        // Double wheels for Side A and Side B

int sensorCount[8] = {0, 0, 0, 0, 0, 0, 0, 0}; // For sensors 1, 2, 3, 4, 7, 8, 9, 0

bool doubleWheelDetected[2] = {false, false};  // Side A and Side B
bool speedWithDoubleWheelSensor[2][2] = {{false, false}, {false, false}};  // Side A and B: DW1, DW2

int sequenceState[2] = {0, 0};  // Sequence state for Side A and Side B

bool reverseDetected = false;
bool speedDisplayed = false;

// Arrays for debounce
unsigned long lastSensorTime[2][4] = {{0, 0, 0, 0}, {0, 0, 0, 0}};
unsigned long debounceTime = 50;

void setup() {
  EEPROM.get(lcdConfigAddress, lcdColumns);
  EEPROM.get(lcdConfigAddress + sizeof(int), lcdRows);
  
  lcd.begin(lcdColumns, lcdRows);
  lcd.backlight();
  lcd.setCursor(0, 0); // Sets the cursor to the top row
  lcd.print("System"); // Displays "System" on the top row
  lcd.setCursor(0, 1); // Sets the cursor to the Sensor row
  lcd.print("Initialized"); // Displays "Initialized" on the Sensor row
  delay(3000); // Shows the message for 3 seconds
  lcd.clear(); // Clears the display

  pinMode(ledPin, OUTPUT);
  pinMode(switchBO, INPUT_PULLUP);
  pinMode(switchDMM, INPUT_PULLUP);

  for (int side = 0; side < 2; side++) {
    for (int i = 0; i < 4; i++) {
      pinMode(sensorPins[side][i], INPUT_PULLUP);
    }
  }

  retrieveSerialSpeedEEPROM();  // Retrieves speed from EEPROM
  Serial.begin(serialSpeed);    // Initializes the serial with the retrieved speed
  retrieveSensorDistanceEEPROM(); // Retrieves sensor distance from EEPROM
}

int state = 0; // Initial state
String commandBuffer = ""; // Stores the received command

void checkCommand(char input) {
  switch (state) {
    case 0: // Initial state
      if (input == '<') {
        state = 1; // Transition to command verification state
        commandBuffer = ""; // Clears the buffer
      }
      break;

    case 1: // Verifies if "<" is present, indicating a command
      if (input == '>') {
        executeCommand(commandBuffer); // Complete command, processes it
        state = 0; // Returns to the initial state
      } 
      else if (input == '<') {
        state = 3; // If "<" appears before ">", discard the command
      } 
      else {
        commandBuffer += input; // Continues accumulating the command
      }
      break;

    case 3: // Discard everything before the previous "<"
      if (input == '>') {
        state = 0; // Returns to the initial state after discarding
      }
      break;

    default:
      state = 0; // In any other case, resets to the initial state
      break;
  }
}

void executeCommand(String command) {
  if (command.startsWith("s:")) {
    configureSerialSpeed(command); 
  } 
  else if (command == "reset") {
    resetSpeed(); 
    Serial.println("{\"status\":\"OK\",\"message\":\"System reset.\"}");
  } 
  else if (command == "status") {
    showReport();
  } 
  else if (command.startsWith("d:")) {
    configureSensorDistance(command);
  } 
  else if (command.startsWith("dp:")) { 
    configureLCD(command);
  } 
  else {
    Serial.println("{\"status\":\"ERROR\",\"message\":\"Unknown command.\"}");
  }
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
  } else if (systemActive == 0) {
    digitalWrite(ledPin, LOW);
    calculateSpeed();
  }

  delay(2);
}

// Saves the sensor distance to EEPROM
void saveSensorDistanceEEPROM(float newDistance) {
  EEPROM.put(sensorDistanceAddress, newDistance);
}

// Saves the serial port speed to EEPROM
void saveSerialSpeedEEPROM(int newSpeed) {
  EEPROM.put(serialSpeedAddress, newSpeed);
}

void saveLCDConfigEEPROM(int columns, int rows) {
  EEPROM.put(lcdConfigAddress, columns);
  EEPROM.put(lcdConfigAddress + sizeof(int), rows);
}

// Configures the sensor distance from the command
void configureSensorDistance(String command) {
  float newDistance = command.substring(3).toFloat();
  if (newDistance > 0) {
    sensorDistance = newDistance;
    saveSensorDistanceEEPROM(newDistance); // Saves to EEPROM only for "d:"
    Serial.print("{\"status\":\"OK\",\"distance\":");
    Serial.print(sensorDistance);
    Serial.println("}");
  } else {
    Serial.println("{\"status\":\"ERROR\",\"message\":\"Invalid distance.\"}");
  }
}

void configureLCD(String command) {
  int commaIndex = command.indexOf(',');
  if (commaIndex == -1) {
    Serial.println("{\"status\":\"ERROR\",\"message\":\"Invalid LCD configuration.\"}");
    return;
  }

  int newColumns = command.substring(3, commaIndex).toInt();
  int newRows = command.substring(commaIndex + 1).toInt();

  if (newColumns > 0 && newRows > 0) {
    lcdColumns = newColumns;
    lcdRows = newRows;
    saveLCDConfigEEPROM(newColumns, newRows); 
    lcd.begin(lcdColumns, lcdRows); 
    Serial.print("{\"status\":\"OK\",\"lcdColumns\":");
    Serial.print(lcdColumns);
    Serial.print(",\"lcdRows\":");
    Serial.print(lcdRows);
    Serial.println("}");
  } else {
    Serial.println("{\"status\":\"ERROR\",\"message\":\"Invalid dimensions.\"}");
  }
}

void configureSerialSpeed(String command) {
  char choice = command.charAt(3); // Gets the character after "<s:"

  if (choice < '1' || choice > '5') {
    Serial.println("{\"status\":\"ERROR\",\"message\":\"Invalid option! Choose between 1 and 5.\"}");
    return;
  }

  switch (choice) {
    case '1':
      serialSpeed = 2400;
      option = 1;
      break;
    case '2':
      serialSpeed = 4800;
      option = 2;
      break;
    case '3':
      serialSpeed = 9600;
      option = 3;
      break;
    case '4':
      serialSpeed = 14400;
      option = 4;
      break;
    case '5':
      serialSpeed = 19200;
      option = 5;
      break;
  }

  saveSerialSpeedEEPROM(serialSpeed);  // Saves the new speed to EEPROM

  Serial.end();
  Serial.begin(serialSpeed);  // Restarts serial communication with the new speed

  Serial.print("{\"status\":\"OK\",\"speed\":");
  Serial.print(serialSpeed);
  Serial.println("}");
}

void showReport() {
  String message = "<\"AX1A\":" + String(axisCounters[0][0]) + 
                   ",\"AX2A\":" + String(axisCounters[0][1]) +
                   ",\"DWA\":" + String(doubleWheelCounters[0]) +
                   ",\"AX1B\":" + String(axisCounters[1][0]) +  
                   ",\"AX2B\":" + String(axisCounters[1][1]) +
                   ",\"DWB\":" + String(doubleWheelCounters[1]) + ">";

  Serial.println(message);

  // Displays the count of sensor presses
String sensorMessage = "<\"sensor1\":" + String(sensorCount[0]) +
                       ",\"sensor2\":" + String(sensorCount[1]) +
                       ",\"sensor3\":" + String(sensorCount[2]) +
                       ",\"sensor4\":" + String(sensorCount[3]) +
                       ",\"sensor7\":" + String(sensorCount[4]) +
                       ",\"sensor8\":" + String(sensorCount[5]) +
                       ",\"sensor9\":" + String(sensorCount[6]) +
                       ",\"sensor0\":" + String(sensorCount[7]) + ">";

Serial.println(sensorMessage);

  String lcdMessage1 = "A1A:" + String(axisCounters[0][0]) + 
                       " A2A:" + String(axisCounters[0][1]) +
                       " DA:" + String(doubleWheelCounters[0]);

  String lcdMessage2 = "A1B:" + String(axisCounters[1][0]) + 
                       " A2B:" + String(axisCounters[1][1]) +
                       " DB:" + String(doubleWheelCounters[1]);

  // Displays the first line
  lcd.setCursor(0, 0);  // Column 0, Row 0
  lcd.print(lcdMessage1);

  // Displays the second line
  lcd.setCursor(0, 1);  // Column 0, Row 1
  lcd.print(lcdMessage2);

  Serial.print("{ \"status\":\"OK\",\"switchDuration\":");
  Serial.print(switchDuration);
  Serial.println("}");

  // Displays the current serial speed
  Serial.print("{ \"status\":\"OK\",\"serialSpeed\":");
  Serial.print(serialSpeed);
  Serial.println("}");
}

// Function to check simultaneous double wheels
void checkSimultaneousDoubleWheels() {
  for (int side = 0; side < 2; side++) {
    if (!digitalRead(sensorPins[side][2]) == HIGH && !digitalRead(sensorPins[side][3]) == HIGH) {
      if (!doubleWheelDetected[side]) {
        doubleWheelCounters[side]++;
        doubleWheelDetected[side] = true;
        Serial.print("Simultaneous double wheels detected on side ");
        Serial.println(side == 0 ? "A" : "B");
      }
    } else {
      doubleWheelDetected[side] = false;
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

void resetSensorCounts() {
  // Resets sensor counts
  memset(sensorCount, 0, sizeof(sensorCount));
  
  // Resets axis and double wheel counts
  memset(axisCounters, 0, sizeof(axisCounters));  
  memset(doubleWheelCounters, 0, sizeof(doubleWheelCounters));
}

void resetSpeed() {
  // Reset states related to speed
  speedDisplayed = false;
  for (int i = 0; i < 2; i++) {
    speedWithDoubleWheelSensor[i][0] = false;
    speedWithDoubleWheelSensor[i][1] = false;
  }
}

// Arrays to store the last time the sensors changed
unsigned long lastSensorTimeA[4] = {0, 0, 0, 0};
unsigned long lastSensorTimeB[4] = {0, 0, 0, 0};

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

void retrieveSensorDistanceEEPROM() {
  EEPROM.get(sensorDistanceAddress, sensorDistance);
  if (isnan(sensorDistance)) {
    sensorDistance = 0.4;
  }
}

// Retrieves the serial speed stored in EEPROM
void retrieveSerialSpeedEEPROM() {
  EEPROM.get(serialSpeedAddress, serialSpeed);
  if (serialSpeed != 2400 && serialSpeed != 4800 && 
      serialSpeed != 9600 && serialSpeed != 14400 && 
      serialSpeed != 19200) {
    // If the recovered value is not a valid speed, set default 9600
    serialSpeed = 9600;
  }
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