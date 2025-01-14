#include "LCDManager.h"

void initializeLCD() {
    lcd.begin(lcdColumns, lcdRows);
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("System");
    lcd.setCursor(0, 1);
    lcd.print("Initialized");
    delay(3000);
    lcd.clear();
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