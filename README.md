# OsloMet Jet Flow Facility Project 2026

Technical information, firmware, 3D models, and documentation for the extended sub-sonic jet flow facility in the OsloMet Green Energy Lab. This repository continues the work of the 2025 group and adds quantitative airflow measurement to the facility.

## Project Background

This project is part of the European Project Semester (EPS) 2026 at Oslo Metropolitan University. Group 5 has extended an existing single-jet flow facility (originally built by Group 2 in 2025) with a differential pressure sensor and real-time velocity computation, turning a previously qualitative demonstration into a quantitative student experiment.

## Features

- **Fan Control:**
Adjustable fan speed via potentiometer and PWM output, with real-time RPM monitoring using a tachometer input.
- **Speaker Control:**
Potential for speaker volume adjustment via a second potentiometer.
- **Pressure and Velocity Measurement (new):**
Differential pressure across the stagnation chamber is read over I2C and converted to air velocity using Bernoulli's equation.
- **LCD Display:**
A Grove RGB LCD shows the fan speed, the live differential pressure (Pa), and the computed velocity (m/s). See the note below on the single versus dual screen configuration.
- **I2C Multiplexer:**
A TCA9548A multiplexer manages multiple I2C devices (the LCD(s) and the pressure sensor) on a single ESP32 I2C bus.
- **Switch Control:**
A momentary power switch toggles the fan system on and off using edge detection in firmware.
- **Multicore Task Management:**
Fan and switch handling tasks are pinned to separate ESP32 cores for parallel processing.

## Important Note: Single Screen vs Two Screens

The original design used two Grove RGB LCD screens: one for fan speed and one for pressure and velocity. During development the onboard 3.3V voltage regulator of the ESP32 was damaged, most likely during an earlier incident where the I2C hub was destroyed by a loose ground connection under power.

A healthy ESP32 regulator should output 3.3V. After the damage, the regulator only produces about 2.7V even with a healthy 5V input on VIN. This lower voltage is enough to power one LCD with readable contrast, but two LCD backlights together draw too much current, which pulls the voltage down further and makes the text on both screens too faint to read.

Because of this, the current working setup uses a single LCD that shows all values on one screen. When the ESP32 is replaced with a board that has a working 3.3V regulator, the two screen version can be used again.

Two firmware versions are provided:

- `esp-code/esp-code.ino` is the single screen version. All values (fan speed, velocity, and pressure) are shown on one LCD connected to hub channel 0. This is the version that works with the current damaged ESP32.
- `esp-code/esp_code-2screens/` contains the two screen version. Fan speed is shown on the LCD on hub channel 0, and pressure and velocity are shown on the LCD on hub channel 1. Use this version only with an ESP32 that has a healthy 3.3V regulator.

## Installation Instructions

### Hardware Requirements

- ESP32 NodeMCU-32S Development Board
- Fan motor with tachometer feedback (12V DC, 1.3A)
- 1x or 2x Grove RGB LCD V5.0 (I2C interface), depending on the firmware version used
- TCA9548A I2C multiplexer hub
- ELVH-M250D-HRRJ-I-N3A5 differential pressure sensor (±250 mbar, I2C address 0x38)
- Mean Well SD-15A-05 DC-DC converter (12V to 5V, 3A)
- Potentiometers (for fan speed and speaker volume)
- Switches (for power and speaker control)
- LEDs (for status indication)
- 50W audio amplifier with 5" 4Ω speaker
- Supporting resistors and wiring

### Software Requirements

- Arduino IDE (2.x or later)
- NodeMCU-32S board support (ESP32 boards package)

### Arduino Libraries

- `Wire.h`
- `Arduino.h`
- `rgb_lcd.h`

### Installation Steps

1. Clone this repository:
```bash
   git clone https://github.com/jensecool/OsloMet-JetFlowFacility-Project-2026.git
```

2. Open the firmware you need in the Arduino IDE:
   - `esp-code/esp-code.ino` for the single screen setup (current hardware)
   - `esp-code/esp_code-2screens/` for the two screen setup (requires a healthy ESP32)
3. Install the necessary libraries via the Library Manager.
4. Select the NodeMCU-32S board and the correct COM port.
5. Upload the code.

## Repository Structure

OsloMet-JetFlowFacility-Project-2026/
├── 3d models/                      3D model files for printable enclosures and adapters
├── Circuit design/                 Schematic diagrams of the electronics
├── esp-code/
│   ├── esp-code.ino                Single screen firmware (current hardware)
│   └── esp_code-2screens/          Two screen firmware (for a healthy ESP32)
└── Worksheet/                      Student experiment worksheet

## ESP32 Pinout

| Function                | ESP32 Pin | Description                            |
|:------------------------|:----------|:---------------------------------------|
| Power Switch Input      | 5         | Toggles fan system ON/OFF              |
| Fan Speed Potentiometer | 34        | Analog read for fan speed setting      |
| Power LED Output        | 13        | Indicates fan system status            |
| Fan PWM Control         | 25        | PWM signal to control fan speed        |
| Fan Tachometer Input    | 14        | Reads pulses for RPM calculation       |
| I2C SDA                 | 21        | I2C data line to multiplexer hub       |
| I2C SCL                 | 22        | I2C clock line to multiplexer hub      |

## I2C Bus Layout

All I2C devices are connected through the TCA9548A multiplexer (address `0x70`) on the ESP32 bus:

| Hub Channel | Device                       | I2C Address     |
|:------------|:-----------------------------|:----------------|
| 0           | Main LCD (fan speed LCD)     | `0x30`, `0x3E`  |
| 1           | Pressure LCD (2 screen only) | `0x30`, `0x3E`  |
| 2           | Pressure sensor              | `0x38`          |
| 3 to 7      | Free                         | n/a             |

## Pressure Sensor Pinout

The ELVH-M250D-HRRJ-I-N3A5 sensor uses the HRRJ surface-mount J-lead package. Note that on this variant SCL is on pin 4, not pin 5 as the standard ELVH datasheet pin code table 1 suggests. The verified connections are:

| Sensor Pin | Function     | Wire Color |
|:-----------|:-------------|:-----------|
| 1          | GND          | Black      |
| 2          | Vs (3.3V)    | Red        |
| 3          | SDA          | White      |
| 4          | SCL          | Yellow     |
| 5 to 8     | Not connected| n/a        |

## Notes

- **Kickstart:** Fan starts at 100% PWM for 5 seconds on boot to overcome initial inertia.
- **I2C Multiplexer:** The project uses the TCA9548A I2C switch to manage the LCD(s) and the pressure sensor on the same I2C bus.
- **Power switch:** The APEM 5232 power switch is a momentary type. The firmware uses edge detection so that one press toggles the system on and the next press toggles it off.
- **Sensor zero offset:** The pressure sensor exhibits a small static offset (typically around 200 to 230 Pa) at zero airflow. This is normal manufacturing tolerance and should be subtracted from raw readings when computing absolute velocity values for analysis.
- **Velocity calculation:** Air velocity is computed using Bernoulli's equation `v = sqrt(2 * dP / rho)` with air density `rho = 1.225 kg/m^3`. Negative differential pressures are clamped to zero velocity in the firmware.

## Team

**Group 5, European Project Semester, OsloMet 2026:**
Jense Cool, Juan Trigueros, Alec Vroon, Siiri Vainopaa.

Supervisor: Ramis Örlü.

## Acknowledgements

This project builds upon the original jet-flow facility designed and constructed by **Group 2 (UFO – SSJFF) in 2025**: Finja Dittmann, Jelte Hoekstra, Robin Lind, Noud van der Meulen. Their report and original repository (`EPS-subsonic-jet-facility`) provided the hardware base, the smoke visualisation modules, and the acoustic forcing module that this work extends.