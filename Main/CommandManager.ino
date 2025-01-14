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

void resetSpeed() {
  // Reset states related to speed
  speedDisplayed = false;
  for (int i = 0; i < 2; i++) {
    speedWithDoubleWheelSensor[i][0] = false;
    speedWithDoubleWheelSensor[i][1] = false;
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
    configureSensorDistance(command); // Saves to EEPROM 
  } 
  else if (command.startsWith("dp:")) { // New command to configure the LCD 
    configureLCD(command);
  } 
  else {
    Serial.println("{\"status\":\"ERROR\",\"message\":\"Unknown command.\"}"); // Unknown command 
  }
}

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