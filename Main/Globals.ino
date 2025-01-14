#include "Globals.h"

// Pins for LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

int lcdColumns = 16; 
int lcdRows = 2;     
int lcdConfigAddress = 16; 

// Combine the sensor pins for Side A and Side B
const int sensorPins[2][4] = {{7, 8, 9, 10}, {6, 5, 4, 3}};
const int switchDMM = 11;
const int switchBO = 2;
const int ledPin = 13;

const int axisAddresses[2][3] = {{0, 2, 4}, {6, 8, 10}};
int sensorDistanceAddress = 12; // Address in EEPROM
int serialSpeedAddress = 14;

float sensorDistance = 0.4; // Default sensor distance
int serialSpeed = 9600; // Initial serial port speed
int option = 3; // Initial value for options

// States and counters
int systemActive = 0; // Indicates if the system is active
int lastSystemActive = LOW; // Last state of the system
unsigned long switchStartTime = 0; // Start time for switch activation
unsigned long switchDuration = 0; // Duration of switch activation

// Sensor states and timings
int sensorStates[2][4] = {{LOW, LOW, LOW, LOW}, {LOW, LOW, LOW, LOW}}; // States of sensors for both sides
unsigned long sensorTimings[2][4] = {{0, 0, 0, 0}, {0, 0, 0, 0}}; // Timings for sensor activations

// Counters
int axisCounters[2][2] = {{0, 0}, {0, 0}};  // Axis 1 and 2 counters for Side A and Side B
int doubleWheelCounters[2] = {0, 0};        // Double wheel counters for Side A and Side B

int sensorCount[8] = {0, 0, 0, 0, 0, 0, 0, 0}; // Counts for sensors 1, 2, 3, 4, 7, 8, 9, 0

bool doubleWheelDetected[2] = {false, false};  // Flags for double wheel detection on Side A and Side B
bool speedWithDoubleWheelSensor[2][2] = {{false, false}, {false, false}};  // Flags for speed detection with double wheels

int sequenceState[2] = {0, 0};  // Sequence state for Side A and Side B

bool reverseDetected = false; // Flag for reverse detection
bool speedDisplayed = false; // Flag to indicate if speed has been displayed

// Arrays for debounce
unsigned long lastSensorTime[2][4] = {{0, 0, 0, 0}, {0, 0, 0, 0}}; // Last time each sensor was activated
unsigned long debounceTime = 50; // Debounce time in milliseconds