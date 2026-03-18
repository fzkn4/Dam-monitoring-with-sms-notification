#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>

// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);
// LiquidCrystal_I2C lcd(0x3F, 16, 2); // Try 0x3f if 0x27 shows pixel blocks

// Ultrasonic
const int trigPin = 10;
const int echoPin = 11;

// LEDs (Level Indicator)
int leds[] = {2, 3, 4, 5};
const int ledCount = 4;

// Buzzer
const int buzzer = 9;

// Float switch (Tank Full Sensor)
const int floatSwitch = 6;

// -----------------------------
// FS-HCORE-A7670C (4G LTE Module) Pins
// -----------------------------
const int gsmTX = 8;      // Module TX -> Arduino D8 (Arduino RX)
const int gsmRX = 7;      // Module RX -> Arduino D7 (Arduino TX)
const int gsmPWRKEY = 12; // Module PWRKEY -> Arduino D12
const int gsmNET = 13;    // Module NET -> Arduino D13

SoftwareSerial sim(gsmTX, gsmRX);

unsigned long lastSmsTime = 0;
const unsigned long smsInterval = 5000; // 5 seconds
const String phoneNumbers[] = {
  "+63XXXXXXX", 
};
const int numRecipients = sizeof(phoneNumbers) / sizeof(phoneNumbers[0]);

// VIN -> External 4V Supply (NOT Arduino 5V)
// GND -> Arduino GND (Common Ground)
// -----------------------------

long duration;
int distance;
int percentage;

void setup() {

  // LED setup
  for (int i = 0; i < ledCount; i++) {
    pinMode(leds[i], OUTPUT);
  }

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(buzzer, OUTPUT);

  // Float switch with internal pull-up
  pinMode(floatSwitch, INPUT_PULLUP);

  // GSM pins setup
  // Start hardware serial for logs
  Serial.begin(9600);
  Serial.println("System Initializing...");

  // FS-HCORE-A7670C often defaults to 115200 baud, 
  // but SoftwareSerial struggles at 115200. Let's try to negotiate.
  sim.begin(115200); 
  delay(1000);
  
  sim.println("AT"); // Send a dummy AT to sync baud rate
  delay(100);
  sim.println("AT+IPR=9600"); // Force module to 9600 baud for stability
  delay(500);
  
  // Restart SoftwareSerial at the stable 9600 baud rate
  sim.begin(9600);
  delay(100);
  sim.println("AT&W"); // SAVE THE SETTING to non-volatile memory
  delay(500);
  
  Serial.println("Checking GSM Module...");
  sim.println("AT");
  delay(500);
  updateSerial(); // Print response
  
  // Wait to allow the modem to find a cell tower and register to it
  Serial.println("Waiting 15s for the module to connect to the cell network...");
  
  // NON-BLOCKING MONITOR LOOP
  // Instead of delay(15000), we read from sim and write to Serial
  // This helps us see if the module resets or sends URCs (like +CREG: 1)
  unsigned long startWait = millis();
  while (millis() - startWait < 15000) {
    updateSerial();
  }

  // Check signal quality
  sim.println("AT+CSQ");
  delay(1000);
  while(sim.available()) Serial.write(sim.read());

  // Check network registration (should normally reply with 0,1 or 0,5)
  sim.println("AT+CREG?");
  delay(1000);
  while(sim.available()) Serial.write(sim.read());

  Serial.println("\n--- End of Init Check ---");
  delay(1000); // Power stabilization delay
  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Water Tank");
  lcd.setCursor(0, 1);
  lcd.print("Level Monitor");
  delay(2000);
  lcd.clear();
}

void loop() {

  // ---- FLOAT SWITCH PRIORITY (TANK FULL) ----
  if (digitalRead(floatSwitch) == LOW) {

    for (int i = 0; i < ledCount; i++) {
      digitalWrite(leds[i], LOW);
    }

    lcd.setCursor(0, 0);
    lcd.print("STATUS: TANK");
    lcd.setCursor(0, 1);
    lcd.print("FULL LOAD     ");

    Serial.println("ALERT: Tank is FULL! Float switch triggered.");

    digitalWrite(buzzer, HIGH);

    // Check if we need to send an SMS (every 5 seconds)
    if (millis() - lastSmsTime >= smsInterval || lastSmsTime == 0) {
      for (int i = 0; i < numRecipients; i++) {
        sendSMS(phoneNumbers[i], "WARNING FLOOD!!!");
      }
      lastSmsTime = millis();
      if (lastSmsTime == 0) lastSmsTime = 1; // Prevent zero
    }

    delay(300);

    return;
  } else {
    lastSmsTime = 0; // Reset SMS timer when not full
  }

  // ---- ULTRASONIC NORMAL LEVEL MODE ----
  distance = readUltrasonic();

  if (distance < 1) distance = 1;
  if (distance > 20) distance = 20;

  percentage = map(distance, 1, 20, 0, 100);

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.print(" cm | Level: ");
  Serial.print(percentage);
  Serial.println(" %");

  updateLEDs(percentage);
  updateLCD(distance, percentage);
  controlBuzzer(distance);

  delay(200);
}

// -------- FUNCTIONS --------

void updateSerial() {
  while (sim.available()) {
    Serial.write(sim.read());
  }
}

int readUltrasonic() {

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);

  int dist = duration * 0.034 / 2;
  return dist;
}

void updateLEDs(int percent) {

  int level = percent / 25;

  for (int i = 0; i < ledCount; i++) {
    if (i < level)
      digitalWrite(leds[i], HIGH);
    else
      digitalWrite(leds[i], LOW);
  }
}

void updateLCD(int dist, int percent) {

  lcd.setCursor(0, 0);
  lcd.print("Distance: ");
  lcd.print(dist);
  lcd.print("cm   ");

  lcd.setCursor(0, 1);
  lcd.print("Level: ");
  lcd.print(percent);
  lcd.print("%   ");
}

void controlBuzzer(int dist) {

  if (dist <= 5) {
    digitalWrite(buzzer, HIGH);
  } else {
    digitalWrite(buzzer, LOW);
  }
}

void sendSMS(String number, String message) {
  Serial.print("Attempting to send SMS to ");
  Serial.println(number);

  // Clear any old data
  while(sim.available()) sim.read();

  // Test connection
  sim.println("AT");
  delay(300);
  updateSerial();

  // Set to text mode
  sim.println("AT+CMGF=1"); 
  delay(300);
  updateSerial();

  // Set recipient
  sim.print("AT+CMGS=\"");
  sim.print(number);
  sim.println("\"");
  delay(300);
  updateSerial();

  // Message content
  sim.print(message);
  delay(300);
  updateSerial();

  // Send Ctrl+Z
  sim.write(26); 
  
  Serial.println("\nWaiting for network dispatch (up to 10s)...");
  // Wait for the SMS command to complete and output its response
  unsigned long startWait = millis();
  while (millis() - startWait < 10000) { 
    updateSerial();
  }

  Serial.println("\nSMS sending sequence complete.");
}