#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <WiFi.h>
#include "AdafruitIO_WiFi.h"
#include "arduinoFFT.h"

// =================================================================
// 1. PIN DEFINITIONS & PWM SETTINGS
// =================================================================
#define BUTTON_PIN 23
#define BUZZER_PIN 32

#define LED1_PIN 33  // Green LED: NORMAL
#define LED2_PIN 25  // Yellow LED: WARNING
#define LED3_PIN 26  // Red LED: FAULT

#define SDA_PIN 21
#define SCL_PIN 22

// PWM Configuration
#define BUZZER_PWM_CHANNEL 0
#define BUZZER_PWM_FREQ 2000  // 2kHz tone frequency
#define BUZZER_PWM_RES 8      // 8-bit resolution (0-255)

// Customized PWM Duty Cycles (10 = ON, 255 = OFF)
#define BUZZER_ON_DUTY 128
#define BUZZER_OFF_DUTY 255

// =================================================================
// 2. CONFIGURATION & CREDENTIALS
// =================================================================
#define IO_USERNAME "Rajnarayan"
#define IO_KEY ""

#define WIFI_SSID "Raj_4g"
#define WIFI_PASS ""

// Timers
unsigned long lastCloudUpdate = 0;
const unsigned long CLOUD_INTERVAL = 15000;  // 15 seconds

unsigned long lastBuzzerToggle = 0;
bool buzzerState = false;

// Button Debounce and Toggle Variables
bool isMuted = false;
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_DELAY = 50;  // ms

// Sampling and FFT Settings
#define SAMPLES 128             // Must be a power of 2
#define SAMPLING_FREQUENCY 500  // Hz

// Baseline Calibration Limits (m/s^2)
const double BASELINE_RMS = 0.5;
const double CRITICAL_RMS = 3.5;

// =================================================================
// BASELINE CALIBRATION
// =================================================================
double calibratedBaseline = 0.0;
bool calibrated = false;

const unsigned long CALIBRATION_TIME = 5000;  // 5 seconds
const double CRITICAL_MULTIPLIER = 2.0;       // 2x baseline = critical

// =================================================================
// 3. OBJECT INITIALIZATION
// =================================================================
AdafruitIO_WiFi io(IO_USERNAME, IO_KEY, WIFI_SSID, WIFI_PASS);

AdafruitIO_Feed *feedRMS = io.feed("vibration-rms");
AdafruitIO_Feed *feedPeak = io.feed("vibration-peak");
AdafruitIO_Feed *feedCrest = io.feed("crest-factor");
AdafruitIO_Feed *feedFreq = io.feed("dominant-freq");
AdafruitIO_Feed *feedScore = io.feed("anomaly-score");
AdafruitIO_Feed *feedStatus = io.feed("machine-status");

Adafruit_MPU6050 mpu;
ArduinoFFT<double> FFT = ArduinoFFT<double>();

double vReal[SAMPLES];
double vImag[SAMPLES];
unsigned int sampling_period_us;

// Helper to set buzzer output safely (PWM vs GPIO)
void setBuzzerPWM(uint8_t duty) {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  // Arduino ESP32 Core v3.x syntax
  analogWrite(BUZZER_PIN, duty);
#else
  // Arduino ESP32 Core v2.x syntax
  ledcWrite(BUZZER_PWM_CHANNEL, duty);
#endif
}

// =================================================================
// 5-SECOND AUTOMATIC BASELINE CALIBRATION
// =================================================================
void calibrateBaseline() {

  Serial.println();
  Serial.println("========================================");
  Serial.println("   BASELINE CALIBRATION STARTED");
  Serial.println("   Keep machine in normal operation");
  Serial.println("   Calibration time: 5 seconds");
  Serial.println("========================================");

  digitalWrite(LED1_PIN, HIGH);  // Green = calibration
  digitalWrite(LED2_PIN, LOW);
  digitalWrite(LED3_PIN, LOW);

  setBuzzerPWM(BUZZER_OFF_DUTY);

  double calibrationSum = 0.0;
  unsigned long sampleCount = 0;

  unsigned long calibrationStart = millis();

  while (millis() - calibrationStart < CALIBRATION_TIME) {

    unsigned long start_time = micros();

    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    // Calculate acceleration magnitude
    double acc_mag = sqrt(
      pow(a.acceleration.x, 2) + pow(a.acceleration.y, 2) + pow(a.acceleration.z, 2));

    // Remove gravity
    acc_mag = acc_mag - 9.81;

    calibrationSum += acc_mag * acc_mag;
    sampleCount++;

    // Maintain approximately 500 Hz sampling
    while (micros() - start_time < sampling_period_us) {
      // Wait
    }
  }

  // Calculate RMS of the normal machine vibration
  calibratedBaseline = sqrt(calibrationSum / sampleCount);

  // Prevent zero baseline
  if (calibratedBaseline < 0.05) {
    calibratedBaseline = 0.05;
  }

  calibrated = true;

  Serial.println();
  Serial.println("========================================");
  Serial.println("   BASELINE CALIBRATION COMPLETE");
  Serial.printf("   Samples      : %lu\n", sampleCount);
  Serial.printf("   Baseline RMS : %.3f m/s^2\n", calibratedBaseline);
  Serial.println("========================================");
  Serial.println();

  // Short indication that calibration is complete
  digitalWrite(LED3_PIN, HIGH);
  digitalWrite(LED2_PIN, HIGH);
  setBuzzerPWM(BUZZER_ON_DUTY);
  delay(200);
}

void setup() {
  Serial.begin(115200);

  // Initialize GPIO Pins
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(LED3_PIN, OUTPUT);

// Setup ESP32 PWM Channel for Buzzer
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  pinMode(BUZZER_PIN, OUTPUT);
#else
  ledcSetup(BUZZER_PWM_CHANNEL, BUZZER_PWM_FREQ, BUZZER_PWM_RES);
  ledcAttachPin(BUZZER_PIN, BUZZER_PWM_CHANNEL);
#endif

  // Set default state to OFF (255)
  setBuzzerPWM(BUZZER_OFF_DUTY);
  digitalWrite(LED1_PIN, LOW);
  digitalWrite(LED2_PIN, LOW);
  digitalWrite(LED3_PIN, LOW);

  // Initialize I2C with explicit SDA/SCL pins
  Wire.begin(SDA_PIN, SCL_PIN);

  // Initialize MPU6050
  if (!mpu.begin(0x68, &Wire)) {
    Serial.println("Error: MPU6050 not detected!");
    while (1) {
      digitalWrite(LED3_PIN, HIGH);
      delay(200);
      digitalWrite(LED3_PIN, LOW);
      delay(200);
    }
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  // Initialize Adafruit IO connection
  Serial.print("Connecting to Adafruit IO");
  io.connect();

  while (io.status() < AIO_CONNECTED) {
    Serial.print(".");
    digitalWrite(LED2_PIN, !digitalRead(LED2_PIN));
    delay(500);
  }
  digitalWrite(LED2_PIN, LOW);
  Serial.println("\nConnected to Adafruit IO!");

  sampling_period_us = round(1000000 * (1.0 / SAMPLING_FREQUENCY));
  // Perform automatic 5-second baseline calibration
  calibrateBaseline();
}

void loop() {
  io.run();

  // ---------------------------------------------------------------
  // Check Manual Mute Button Toggle (Non-blocking with Debounce)
  // ---------------------------------------------------------------
  bool currentButtonReading = digitalRead(BUTTON_PIN);
  if (currentButtonReading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    static bool buttonState = HIGH;
    if (currentButtonReading != buttonState) {
      buttonState = currentButtonReading;
      if (buttonState == LOW) {  // Button Pressed
        isMuted = !isMuted;
        Serial.printf("Buzzer Mute Toggled -> %s\n", isMuted ? "MUTED" : "ENABLED");
      }
    }
  }
  lastButtonState = currentButtonReading;

  double sum_sq = 0;
  double peak = 0;

  // ---------------------------------------------------------------
  // Step A: Signal Acquisition & Time-Domain Calculation
  // ---------------------------------------------------------------
  for (int i = 0; i < SAMPLES; i++) {
    unsigned long start_time = micros();

    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    double acc_mag = sqrt(pow(a.acceleration.x, 2) + pow(a.acceleration.y, 2) + pow(a.acceleration.z, 2)) - 9.81;

    vReal[i] = acc_mag;
    vImag[i] = 0;

    sum_sq += pow(acc_mag, 2);
    if (abs(acc_mag) > peak) {
      peak = abs(acc_mag);
    }

    while (micros() - start_time < sampling_period_us) {
      // Precise sampling timing loop
    }
  }

  double rms = sqrt(sum_sq / SAMPLES);
  Serial.printf("DEBUG -> RMS = %.3f\n", rms);
  double crest_factor = (rms > 0) ? (peak / rms) : 0;

  // ---------------------------------------------------------------
  // Step B: Frequency Domain Analysis (FFT)
  // ---------------------------------------------------------------
  FFT.windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.compute(vReal, vImag, SAMPLES, FFT_FORWARD);
  FFT.complexToMagnitude(vReal, vImag, SAMPLES);
  double dominant_freq = FFT.majorPeak(vReal, SAMPLES, SAMPLING_FREQUENCY);

  // ---------------------------------------------------------------
  // Step C: Anomaly Scoring & Decision Logic
  // ---------------------------------------------------------------
  // How many times higher current vibration is compared to normal
  double vibration_ratio = rms / calibratedBaseline;

  // Convert ratio into anomaly score
  double anomaly_score = ((vibration_ratio - 1.0) / (CRITICAL_MULTIPLIER - 1.0)) * 100.0;

  anomaly_score = constrain(anomaly_score, 0.0, 100.0);

  String machine_status = "NORMAL";

  if (anomaly_score >= 75.0) {
    machine_status = "FAULT";

    // Status Indicator: Red LED ON
    digitalWrite(LED1_PIN, LOW);
    digitalWrite(LED2_PIN, LOW);
    digitalWrite(LED3_PIN, HIGH);

    // Buzzer State: Beep every 100ms
    if (!isMuted) {
      if (millis() - lastBuzzerToggle >= 100) {
        lastBuzzerToggle = millis();
        buzzerState = !buzzerState;
        setBuzzerPWM(buzzerState ? BUZZER_ON_DUTY : BUZZER_OFF_DUTY);
      }
    } else {
      setBuzzerPWM(BUZZER_OFF_DUTY);
    }
  } else if (anomaly_score >= 35.0) {
    machine_status = "WARNING";

    // Status Indicator: Yellow LED ON
    digitalWrite(LED1_PIN, LOW);
    digitalWrite(LED2_PIN, HIGH);
    digitalWrite(LED3_PIN, LOW);

    // Buzzer State: Beep every 2 seconds (2000ms)
    if (!isMuted) {
      if (millis() - lastBuzzerToggle >= 2000) {
        lastBuzzerToggle = millis();
        buzzerState = !buzzerState;
        setBuzzerPWM(buzzerState ? BUZZER_ON_DUTY : BUZZER_OFF_DUTY);
      }
    } else {
      setBuzzerPWM(BUZZER_OFF_DUTY);
    }
  } else {
    // Status Indicator: Green LED ON
    digitalWrite(LED1_PIN, HIGH);
    digitalWrite(LED2_PIN, LOW);
    digitalWrite(LED3_PIN, LOW);

    // Buzzer State: OFF
    setBuzzerPWM(BUZZER_OFF_DUTY);
  }

  // Serial Diagnostics Logging
  Serial.printf("RMS: %.2f | Peak: %.2f | Crest: %.2f | Freq: %.1f Hz | Score: %.1f%% | Status: %s | Muted: %s\n",
                rms, peak, crest_factor, dominant_freq, anomaly_score, machine_status.c_str(), isMuted ? "YES" : "NO");

  // ---------------------------------------------------------------
  // Step D: Publish Data to Adafruit IO every 15 seconds
  // ---------------------------------------------------------------
  if (millis() - lastCloudUpdate >= CLOUD_INTERVAL) {

    feedRMS->save(rms);
    feedPeak->save(peak);
    feedCrest->save(crest_factor);
    feedFreq->save(dominant_freq);
    feedScore->save(anomaly_score);
    feedStatus->save(machine_status);

    lastCloudUpdate = millis();

    Serial.println("Data sent to Adafruit IO");
  }
}
