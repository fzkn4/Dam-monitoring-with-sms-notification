# Water Level & Dam Monitoring System with SMS Notification

This project is an Arduino-based water level monitoring system designed for water tanks or small-scale dam models. It continuously tracks the water level using an ultrasonic sensor, displays real-time data on an LCD, provides visual feedback through LED indicators, and features a priority float switch that triggers a 4G GSM module to send SMS alerts when the tank reaches full capacity.

## Features

* **Real-time Monitoring:** Continuously tracks water distance and percentage level.
* **LCD Display:** Shows live data on an I2C 16x2 LCD screen (distance in cm, level in %).
* **LED Status Indicators:** 4 LEDs light up incrementally to represent the water level in 25% blocks.
* **Oversight Protection:** A float switch provides a hard-failure backup. If the water hits the float switch, it immediately overrides normal operation and triggers a "Tank Full" state.
* **Audible Alarm:** A buzzer sounds when the water level gets dangerously close to the top (<= 5cm measured by ultrasonic) or when the float switch is triggered.
* **SMS Notifications:** Integrates an `FS-HCORE-A7670C` (4G LTE) GSM module to send an immediate SMS alert (`WARNING: TANK IS FULL!`) to a designated phone number when the float switch is triggered.

## Hardware Components

1. **Microcontroller:** Arduino Uno (or compatible equivalent)
2. **Ultrasonic Sensor:** HC-SR04 (measures distance from the top of the tank to the water surface)
3. **Float Switch:** Acts as a maximum capacity limit switch.
4. **GSM Module:** FS-HCORE-A7670C (4G LTE Module)
5. **Display:** 16x2 I2C LCD Display
6. **LEDs:** 4x Standard LEDs
7. **Buzzer:** Active Buzzer
8. **Power Supply:** 
   - 5V for Arduino and standard sensors.
   - **Dedicated 4V/2A External Power Supply** for the GSM Module (Crucial for handling transmission current spikes).

## Wiring Guide

### Sensors & Indicators
| Component | Arduino Pin | Notes |
| :--- | :--- | :--- |
| **I2C LCD** | A4 (SDA), A5 (SCL) | Needs 5V & GND. Default I2C address is `0x27`. |
| **Ultrasonic Trig** | D10 | |
| **Ultrasonic Echo** | D11 | |
| **LED 1 (Lowest)** | D2 | Use inline resistors (e.g., 220Ω-330Ω). |
| **LED 2** | D3 | Use inline resistors (e.g., 220Ω-330Ω). |
| **LED 3** | D4 | Use inline resistors (e.g., 220Ω-330Ω). |
| **LED 4 (Highest)**| D5 | Use inline resistors (e.g., 220Ω-330Ω). |
| **Float Switch** | D6 | Uses internal pull-up (`INPUT_PULLUP`). Wire other side to GND. |
| **Buzzer** | D9 | |

### GSM Module (FS-HCORE-A7670C)
**CRITICAL WARNING:** Do NOT power the GSM module directly from the Arduino's 5V pin. The module requires up to 2 Amps during SMS transmission, which will instantly brown-out or damage the Arduino.

| GSM Pin | Connection |
| :--- | :--- |
| **TX** | Arduino D8 (SoftwareSerial RX) |
| **RX** | Arduino D7 (SoftwareSerial TX) |
| **PWRKEY** | Arduino D12 |
| **NET** | Arduino D13 |
| **VIN** | **External 4V Power Supply** (Must handle 2A current spikes) |
| **GND** | **Common Ground** (Must be connected to Arduino GND & Ext Power GND) |

## Setup & Installation

1. **Clone the Repository:**
   Download or clone the code to your local machine.

2. **Install Required Libraries:**
   Open the Arduino IDE and ensure you have the following libraries installed via the Library Manager (`Sketch` -> `Include Library` -> `Manage Libraries...`):
   * `LiquidCrystal I2C` by Frank de Brabander

3. **Configure the Phone Number:**
   Open `waterlevelSys.ino` in the Arduino IDE.
   Find the following line:
   ```cpp
   const String phoneNumber = "+639xxxxxxxxx"; // REPLACE WITH ACTUAL NUMBER
   ```
   Replace `+639xxxxxxxxx` with the actual mobile number you want the system to alert. Ensure you use the correct international format (e.g., `+63` for the Philippines).

4. **Upload the Code:**
   Select your Arduino board and COM port, then upload the sketch.

5. **Power On Routine:**
   * Turn on the external power supply for the GSM module.
   * Power on the Arduino.
   * Open the Serial Monitor at `9600 baud`. The initialization sequence will start.
   * The system will say `Waiting 15s for the module to connect to the cell network...`. Check the Serial Monitor to ensure `AT+CSQ` returns a valid signal (e.g., `+CSQ: 18,99`) and `AT+CREG?` returns `+CREG: 0,1` (registered to home network).

## Calibration

The ultrasonic sensor logic currently assumes a tank depth of 20cm max to 1cm min:
```cpp
if (distance < 1) distance = 1;
if (distance > 20) distance = 20;
percentage = map(distance, 1, 20, 0, 100);
```
If your physical tank or dam is deeper, simply modify the `20` value in the `map()` and `if` statements to reflect your actual distance (in centimeters) from the sensor to the empty tank bottom.
