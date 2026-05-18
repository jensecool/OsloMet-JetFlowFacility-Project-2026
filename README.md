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
Differential pressure across the stagnation chamber is read over I2C and converted to air velocity using Bernoulli's equation. Both values are displayed on a dedicated LCD in real time.
- **LCD Displays:**
Two RGB LCD displays. One shows current fan speed, the other shows live pressure (Pa) and velocity (m/s).
- **I2C Multiplexer:**
A TCA9548A multiplexer manages multiple identical I2C devices (two LCDs and the pressure sensor) on a single ESP32 I2C bus.
- **Switch Control:**
Dedicated physical switches to toggle power to the fan and (optionally) the speaker.
- **Multicore Task Management:**
Fan and switch handling tasks are pinned to separate ESP32 cores for parallel processing.
## Installation Instructions
 
### Hardware Requirements
 
- ESP32 NodeMCU-32S Development Board
- Fan motor with tachometer feedback (12V DC, 1.3A)
- 2x Grove RGB LCD V5.0 (I2C interface)
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
 
2. Open `esp-code/esp-code.ino` in the Arduino IDE.
3. Install the necessary libraries via the Library Manager.
4. Select the NodeMCU-32S board and the correct COM port.
5. Upload the code.
## Repository Structure
 
```
OsloMet-JetFlowFacility-Project-2026/
├── 3d models/         3D model files for printable enclosures and adapters
├── Circuit design/    Schematic diagrams of the electronics
├── esp-code/          ESP32 Arduino firmware
└── Worksheet/         Student experiment worksheet
```
 
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
 
| Hub Channel | Device          | I2C Address     |
|:------------|:----------------|:----------------|
| 0           | Fan speed LCD   | `0x30`, `0x3E`  |
| 1           | Pressure LCD    | `0x30`, `0x3E`  |
| 2           | Pressure sensor | `0x38`          |
| 3 to 7      | Free            | n/a             |
 
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
- **I2C Multiplexer:** The project uses the TCA9548A I2C switch to manage multiple identical LCDs and the pressure sensor on the same I2C bus.
- **Sensor zero offset:** The pressure sensor exhibits a small static offset (typically around 200 to 230 Pa) at zero airflow. This is normal manufacturing tolerance and should be subtracted from raw readings when computing absolute velocity values for analysis.
- **Velocity calculation:** Air velocity is computed using Bernoulli's equation `v = sqrt(2 * dP / rho)` with air density `rho = 1.225 kg/m^3`. Negative differential pressures are clamped to zero velocity in the firmware.
## Team
 
**Group 5, European Project Semester, OsloMet 2026:**
Jense Cool, Juan Trigueros, Alec Vroon, Siiri Vainopaa.
 
Supervisor: Ramis Örlü.
 
## Acknowledgements
 
This project builds upon the original jet-flow facility designed and constructed by **Group 2 (UFO – SSJFF) in 2025**: Finja Dittmann, Jelte Hoekstra, Robin Lind, Noud van der Meulen. Their report and original repository (`EPS-subsonic-jet-facility`) provided the hardware base, the smoke visualisation modules, and the acoustic forcing module that this work extends.