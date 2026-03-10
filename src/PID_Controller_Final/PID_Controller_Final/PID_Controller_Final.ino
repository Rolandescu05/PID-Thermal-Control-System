#include <LiquidCrystal.h>
#include <EEPROM.h>

// ===================================================================
// 1. PIN DEFINITIONS
// ===================================================================

// LCD Pins
const int LCD_RS = 12;
const int LCD_EN = 11;
const int LCD_D4 = 5;
const int LCD_D5 = 4;
const int LCD_D6 = 3;
const int LCD_D7 = 2;

// Initialize LCD library
LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

// Button Pins
const int BTN_OK_PIN = 8;
const int BTN_CANCEL_PIN = 9;
const int BTN_PLUS_PIN = 7;
const int BTN_MINUS_PIN = 6;

// Sensor Pin
const int LM35_PIN = A0;

// H-Bridge Pins (L293D/L298N)
// Channel 1&2 (Heater - Incandescent Bulb)
const int HEATER_ENABLE_PIN = 10;  // ENA (PWM)
const int HEATER_IN1_PIN = A1;
const int HEATER_IN2_PIN = A2;

// Channel 3&4 (Cooler - DC Fan)
const int COOLER_ENABLE_PIN = A3;  // ENB
const int COOLER_IN3_PIN = A4;
const int COOLER_IN4_PIN = A5;

// ===================================================================
// 2. GLOBAL VARIABLES
// ===================================================================

// System Parameters
float TSET = 50.0;               // Target temperature (Celsius)
unsigned long heatingTime = 40;  // Heating phase duration (seconds)
unsigned long holdingTime = 5;   // Holding phase duration (seconds)
unsigned long coolingTime = 20;  // Cooling phase duration (seconds)

// PID Constants
float KP = 10.0;  // Proportional Gain
float KI = 0.5;   // Integral Gain
float KD = 1.0;   // Derivative Gain

// Menu Variables
int menuState = 0;
bool menuChanged = true;

const char *menuItems[] = {
  "Start Program",
  "Set TSET",
  "Set Heat Time",
  "Set Hold Time",
  "Set Cool Time",
  "Set KP",
  "Set KI",
  "Set KD"
};
const int maxMenuState = 7;

// State Machine Variables
enum ProgramState { IDLE,
                    HEATING,
                    HOLDING,
                    COOLING };
ProgramState currentState = IDLE;
unsigned long stateStartTime = 0;
bool programRunning = false;

// Button Debouncing
unsigned long lastButtonPressTime = 0;
const long buttonDebounceDelay = 200;

// PID Calculation Variables
float currentTemp = 0.0;
float pidOutput = 0;
float pidError = 0;
float pidIntegral = 0;
float pidLastError = 0;

// ===================================================================
// 3. SETUP
// ===================================================================
void setup() {
  // Initialize LCD
  lcd.begin(16, 2);
  lcd.print("PID System V1.0");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");

  // Initialize Button Pins
  pinMode(BTN_OK_PIN, INPUT);
  pinMode(BTN_CANCEL_PIN, INPUT);
  pinMode(BTN_PLUS_PIN, INPUT);
  pinMode(BTN_MINUS_PIN, INPUT);

  // Initialize H-Bridge Pins
  pinMode(HEATER_ENABLE_PIN, OUTPUT);
  pinMode(HEATER_IN1_PIN, OUTPUT);
  pinMode(HEATER_IN2_PIN, OUTPUT);

  pinMode(COOLER_ENABLE_PIN, OUTPUT);
  pinMode(COOLER_IN3_PIN, OUTPUT);
  pinMode(COOLER_IN4_PIN, OUTPUT);

  // Turn off all actuators on startup
  controlHeater(0);
  controlCooler(false);

  delay(2000);
  lcd.clear();
}

// ===================================================================
// 4. MAIN LOOP
// ===================================================================
void loop() {
  // Constant sensor reading
  readTemperature();

  // Main Logic Switch
  if (programRunning) {
    handleButtonsProgram();
    runStateMachine();
  } else {
    handleButtonsMenu();
    displayMenu();
  }
}

// ===================================================================
// 5. MENU FUNCTIONS
// ===================================================================

void handleButtonsMenu() {
  if (millis() - lastButtonPressTime < buttonDebounceDelay) return;

  if (digitalRead(BTN_PLUS_PIN) == HIGH) {
    lastButtonPressTime = millis();
    menuState++;
    if (menuState > maxMenuState) menuState = 0;
    menuChanged = true;
  } else if (digitalRead(BTN_MINUS_PIN) == HIGH) {
    lastButtonPressTime = millis();
    menuState--;
    if (menuState < 0) menuState = maxMenuState;
    menuChanged = true;
  } else if (digitalRead(BTN_OK_PIN) == HIGH) {
    lastButtonPressTime = millis();
    if (menuState == 0) {  // Start Program selected
      programRunning = true;
      currentState = HEATING;
      stateStartTime = millis();
      lcd.clear();
      lcd.print("Starting...");
      delay(1000);
    } else {
      editSelectedValue(1);  // Increment
    }
  } else if (digitalRead(BTN_CANCEL_PIN) == HIGH) {
    lastButtonPressTime = millis();
    editSelectedValue(-1);  // Decrement
  }
}

void editSelectedValue(int direction) {
  float step = 1.0;
  if (menuState >= 5) step = 0.1;  // Smaller steps for PID tuning

  switch (menuState) {
    case 1: TSET += (direction * step); break;
    case 2: heatingTime += direction; break;
    case 3: holdingTime += direction; break;
    case 4: coolingTime += direction; break;
    case 5: KP += (direction * step); break;
    case 6: KI += (direction * step); break;
    case 7: KD += (direction * step); break;
  }
  if (TSET < 0) TSET = 0;
  if (heatingTime < 1) heatingTime = 1;
  menuChanged = true;
}

void displayMenu() {
  if (!menuChanged) return;
  menuChanged = false;
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print(">");
  lcd.print(menuItems[menuState]);

  lcd.setCursor(0, 1);
  switch (menuState) {
    case 0: lcd.print("(Press OK)"); break;
    case 1:
      lcd.print(TSET, 1);
      lcd.print(" C");
      break;
    case 2:
      lcd.print(heatingTime);
      lcd.print(" s");
      break;
    case 3:
      lcd.print(holdingTime);
      lcd.print(" s");
      break;
    case 4:
      lcd.print(coolingTime);
      lcd.print(" s");
      break;
    case 5: lcd.print(KP, 2); break;
    case 6: lcd.print(KI, 2); break;
    case 7: lcd.print(KD, 2); break;
  }
}

// ===================================================================
// 6. SENSOR & STATE MACHINE
// ===================================================================

void readTemperature() {
  int sensorVal = analogRead(LM35_PIN);
  float voltage = (sensorVal / 1024.0) * 5000.0;
  currentTemp = voltage / 10.0;  // 10mV = 1 degree C
}

void handleButtonsProgram() {
  if (millis() - lastButtonPressTime < buttonDebounceDelay) return;

  if (digitalRead(BTN_CANCEL_PIN) == HIGH) {
    lastButtonPressTime = millis();
    programRunning = false;
    currentState = IDLE;
    controlHeater(0);
    controlCooler(false);
    menuState = 0;
    menuChanged = true;
    lcd.clear();
    lcd.print("Stopped.");
    delay(1000);
  }
}

void runStateMachine() {
  unsigned long timeInState = (millis() - stateStartTime) / 1000;

  displayStatus(timeInState);

  switch (currentState) {
    case IDLE:
      controlHeater(0);
      controlCooler(false);
      break;

    case HEATING:
      runPID();
      if (timeInState >= heatingTime) {
        currentState = HOLDING;
        stateStartTime = millis();
      }
      break;

    case HOLDING:
      runPID();
      if (timeInState >= holdingTime) {
        currentState = COOLING;
        stateStartTime = millis();
      }
      break;

    case COOLING:
      controlHeater(0);
      controlCooler(true);
      if (timeInState >= coolingTime) {
        currentState = IDLE;
        programRunning = false;
        controlCooler(false);
        menuState = 0;
        menuChanged = true;
      }
      break;
  }
}

// ===================================================================
// 7. CONTROL LOGIC (PID)
// ===================================================================

void runPID() {
  pidError = TSET - currentTemp;

  pidIntegral += pidError;
  // Anti-windup
  if (pidIntegral > 255) pidIntegral = 255;
  if (pidIntegral < -255) pidIntegral = -255;

  float pidDerivative = pidError - pidLastError;
  pidOutput = (KP * pidError) + (KI * pidIntegral) + (KD * pidDerivative);
  pidLastError = pidError;

  if (pidOutput > 0) {
    controlCooler(false);
    if (pidOutput > 255) pidOutput = 255;
    controlHeater((int)pidOutput);
  } else {
    controlHeater(0);
  }
}

void displayStatus(unsigned long timeInState) {
  // Row 1: State and Timer
  lcd.setCursor(0, 0);
  switch (currentState) {
    case HEATING:
      lcd.print("HEATING  ");
      lcd.print(heatingTime - timeInState);
      break;
    case HOLDING:
      lcd.print("HOLDING  ");
      lcd.print(holdingTime - timeInState);
      break;
    case COOLING:
      lcd.print("COOLING  ");
      lcd.print(coolingTime - timeInState);
      break;
  }
  lcd.print("s ");

  // Row 2: Temperatures
  lcd.setCursor(0, 1);
  lcd.print("T:");
  lcd.print(currentTemp, 1);
  lcd.print("C S:");
  lcd.print(TSET, 1);
}

// ===================================================================
// 8. HARDWARE INTERFACE
// ===================================================================

void controlHeater(int pwmValue) {
  pwmValue = constrain(pwmValue, 0, 255);
  digitalWrite(HEATER_IN1_PIN, HIGH);
  digitalWrite(HEATER_IN2_PIN, LOW);
  analogWrite(HEATER_ENABLE_PIN, pwmValue);
}

void controlCooler(bool enabled) {
  if (enabled) {
    digitalWrite(COOLER_IN3_PIN, LOW);
    digitalWrite(COOLER_IN4_PIN, HIGH);
    digitalWrite(COOLER_ENABLE_PIN, HIGH);
  } else {
    digitalWrite(COOLER_ENABLE_PIN, LOW);
  }
}