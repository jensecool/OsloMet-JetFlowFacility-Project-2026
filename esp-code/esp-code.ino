#include <Wire.h>
#include <Arduino.h>
#include <math.h>
#include "rgb_lcd.h"

// === SWITCH PINS ===
#define POWER_SWITCH_PIN      5

// === POTENTIOMETER PINS ===
#define FAN_POT_PIN           34

// === LED PINS ===
#define POWER_LED_PIN         13

// === FAN PINS ===
#define FAN_PWM_PIN           25  // PWM control pin
#define FAN_TACHO_PIN         14  // RPM feedback pin

// === PWM CONFIG ===
#define PWM_FREQ              12500   // 12.5 kHz
#define PWM_CHANNEL           0
#define PWM_RESOLUTION        8       // 8-bit (0-255)

// === FAN SETTINGS ===
#define FAN_KICKSTART_SPEED   255     // Full speed
#define KICKSTART_DURATION    5000    // in milliseconds

// === I2C MULTIPLEXER ===
#define TCA_ADDR              0x70

// === PRESSURE SENSOR ===
#define SENSOR_ADDR           0x38      // ELVH-M250D-HRRJ-I-N3A5
#define SENSOR_OFFSET_COUNTS  8192.0f   // count at 0 Pa (differential, 10-90% TF)
#define SENSOR_FSS_COUNTS     6554.0f   // +/- full scale span in counts
#define SENSOR_FSS_PA         25000.0f  // +/-250 mbar = +/-25000 Pa

// Air density for Bernoulli.
#define AIR_DENSITY           1.225f

// === GLOBAL VARIABLES ===
volatile unsigned long pulse_count = 0;
unsigned long last_time = 0;
unsigned long rpm = 0;

bool power_on = false;
bool last_power_state = false;

int fan_speed = 178; // 70% default
int fan_speed_percentage = 0;

float pressure_pa = 0.0f;
float velocity_mps = 0.0f;
bool  sensor_present = false;

rgb_lcd lcd;
// All output now goes to a single LCD on channel 0.
int main_lcd_channel = 0;
int pressure_sensor_channel = 2;

// === FUNCTION DECLARATIONS ===
void tca_select(uint8_t channel);
void IRAM_ATTR count_pulse();
void calculate_fan_rpm();
void kickstart_fan();
void set_fan_speed();
void configure_lcd(int channel);
void reset_lcd(int channel);
int get_pot_value_percent(int pin);
int get_pot_value_8bit(int pin);
void handle_power_off();
void update_display();
void control_panel_controller(void *pv_parameters);
void switches_controller(void *pv_parameters);
void scan_i2c_with_tca();
bool read_pressure_sensor(float *out_pa);
float pressure_to_velocity(float pa);


// === I2C Multiplexer Channel Select ===
void tca_select(uint8_t channel) {
    Wire.beginTransmission(TCA_ADDR);
    Wire.write(1 << channel);
    Wire.endTransmission();
    delay(10);
}

// === Tachometer Interrupt ===
void IRAM_ATTR count_pulse() {
    pulse_count++;
}

// === Calculate Fan RPM ===
void calculate_fan_rpm() {
    if (millis() - last_time >= 1000) {
        detachInterrupt(digitalPinToInterrupt(FAN_TACHO_PIN));
        rpm = (pulse_count * 30);  // 60s / 2 pulses = 30
        Serial.printf("Fan speed: %lu RPM\n", rpm);
        pulse_count = 0;
        last_time = millis();
        attachInterrupt(digitalPinToInterrupt(FAN_TACHO_PIN), count_pulse, FALLING);
    }
}

// === Kickstart Fan ===
void kickstart_fan() {
    Serial.println("Kickstarting fan...");
    ledcWrite(FAN_PWM_PIN, FAN_KICKSTART_SPEED);
    delay(KICKSTART_DURATION);
}

// === Set Fan Speed ===
void set_fan_speed() {
    ledcWrite(FAN_PWM_PIN, fan_speed);
}

// === Configure LCD ===
void configure_lcd(int channel) {
    tca_select(channel);
    delay(10);
    lcd.begin(16, 2);
    lcd.setRGB(0, 255, 0);  // Green backlight
    lcd.setCursor(0, 0);
    lcd.print("Jet facility");
    lcd.setCursor(0, 1);
    lcd.print("Starting...");
}

// === Clear LCD ===
void reset_lcd(int channel) {
    tca_select(channel);
    delay(10);
    lcd.setCursor(0, 0);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print("                ");
}

// === Read Potentiometer: % Output ===
int get_pot_value_percent(int pin) {
    uint16_t value = analogRead(pin);
    if (value <= 409) return 0; // Below 10% rounds to 0%
    return map(value, 409, 4095, 0, 100);
}

// === Read Potentiometer: 8-bit Output ===
int get_pot_value_8bit(int pin) {
    uint16_t value = analogRead(pin);
    if (value <= 409) return 0; // Below 10% rounds to 0%
    return map(value, 409, 4095, 26, 255);
}

// === Handle Power Off ===
void handle_power_off() {
    fan_speed = 0;
    pressure_pa = 0.0f;
    velocity_mps = 0.0f;
    reset_lcd(main_lcd_channel);
    set_fan_speed();
}

// === Read the pressure sensor ===
// Returns true if a valid reading was obtained, false otherwise.
bool read_pressure_sensor(float *out_pa) {
    tca_select(pressure_sensor_channel);

    uint8_t n = Wire.requestFrom(SENSOR_ADDR, (uint8_t)2);
    if (n != 2) {
        return false;
    }

    uint8_t b0 = Wire.read();
    uint8_t b1 = Wire.read();

    // top 2 bits of byte 0 are status; 00 means normal
    uint8_t status = (b0 >> 6) & 0x03;
    if (status != 0x00) {
        return false;
    }

    uint16_t raw = ((b0 & 0x3F) << 8) | b1;
    *out_pa = ((float)raw - SENSOR_OFFSET_COUNTS)
            / SENSOR_FSS_COUNTS
            * SENSOR_FSS_PA;
    return true;
}

// === Convert differential pressure to velocity via Bernoulli ===
// v = sqrt(2 * dP / rho). Negative dP clamps to 0.
float pressure_to_velocity(float pa) {
    if (pa <= 0.0f) return 0.0f;
    return sqrtf(2.0f * pa / AIR_DENSITY);
}

// === Update the single display with all values ===
// Line 1: Fs:XX% v:XX.Xm/s
// Line 2: P: XXXX Pa
void update_display() {
    // Read fan pot
    fan_speed_percentage = get_pot_value_percent(FAN_POT_PIN);
    fan_speed = get_pot_value_8bit(FAN_POT_PIN);
    set_fan_speed();
    calculate_fan_rpm();

    // Read pressure sensor
    float pa = 0.0f;
    sensor_present = read_pressure_sensor(&pa);
    if (sensor_present) {
        pressure_pa = pa;
        velocity_mps = pressure_to_velocity(pa);
    }

    // Build the two display lines
    char l0[17];
    char l1[17];

    // Line 1: fan speed percentage and velocity with unit
    snprintf(l0, sizeof(l0), "Fs:%d%% v:%.1fm/s",
             fan_speed_percentage, velocity_mps);

    // Line 2: pressure, or a warning if the sensor is missing
    if (sensor_present) {
        snprintf(l1, sizeof(l1), "P: %.0f Pa", pressure_pa);
    } else {
        snprintf(l1, sizeof(l1), "Sensor missing");
    }

    // Write to the single LCD on channel 0
    tca_select(main_lcd_channel);
    delay(10);
    lcd.setCursor(0, 0);
    lcd.print("                ");  // clear line 1
    lcd.setCursor(0, 0);
    lcd.print(l0);
    lcd.setCursor(0, 1);
    lcd.print("                ");  // clear line 2
    lcd.setCursor(0, 1);
    lcd.print(l1);

    // Also log to serial
    Serial.printf("Fan=%d%%  Pressure=%.1f Pa  Velocity=%.2f m/s\n",
                  fan_speed_percentage, pressure_pa, velocity_mps);
}

// === Control Panel Task (Core 0) ===
void control_panel_controller(void *pv_parameters) {
    configure_lcd(main_lcd_channel);

    while (1) {
        if (!power_on) {
            handle_power_off();
            last_power_state = false;
            delay(500);
            continue;
        }

        // If power JUST came back on, re-init the LCD to recover from any glitch
        if (!last_power_state) {
            configure_lcd(main_lcd_channel);
            last_power_state = true;
            delay(100);
        }

        update_display();
        delay(100);
    }
}

// === Switches Task (Core 1) ===
// Momentary switch with edge detection: one press toggles power_on.
void switches_controller(void *pv_parameters) {
    bool last_switch_reading = HIGH;  // INPUT_PULLUP: not pressed = HIGH

    while (1) {
        bool current_reading = digitalRead(POWER_SWITCH_PIN);

        // Detect transition from not-pressed (HIGH) to pressed (LOW)
        if (last_switch_reading == HIGH && current_reading == LOW) {
            power_on = !power_on;
            digitalWrite(POWER_LED_PIN, power_on);
            Serial.printf("Fan is %s\n", power_on ? "ON" : "OFF");
            delay(50);  // short debounce
        }

        last_switch_reading = current_reading;
        delay(20);
    }
}

// === I2C Scanner for TCA ===
void scan_i2c_with_tca() {
    for (uint8_t ch = 0; ch < 8; ch++) {
        Serial.printf("Scanning channel %u\n", ch);
        tca_select(ch);
        delay(5);

        for (uint8_t addr = 1; addr < 127; addr++) {
            Wire.beginTransmission(addr);
            if (Wire.endTransmission() == 0) {
                Serial.printf("  Device found at 0x%02X\n", addr);
            }
        }
    }
}

// === Arduino Setup ===
void setup() {
    Serial.begin(115200);
    Wire.begin();

    pinMode(POWER_SWITCH_PIN, INPUT_PULLUP);
    pinMode(POWER_LED_PIN, OUTPUT);
    pinMode(FAN_TACHO_PIN, INPUT_PULLUP);

    ledcAttach(FAN_PWM_PIN, PWM_FREQ, PWM_RESOLUTION);
    attachInterrupt(digitalPinToInterrupt(FAN_TACHO_PIN), count_pulse, FALLING);

    xTaskCreatePinnedToCore(control_panel_controller, "controlPanelController", 10000, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(switches_controller, "switchesController", 10000, NULL, 1, NULL, 1);

    kickstart_fan();
    last_time = millis();

    scan_i2c_with_tca();
}

// === Arduino Loop ===
void loop() {
    // Main logic is handled by tasks.
}