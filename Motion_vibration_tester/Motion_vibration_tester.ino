#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <WiFi.h>
#include "AdafruitIO_WiFi.h"
#include "arduinoFFT.h"

// =================================================================
// 1. PIN DEFINITIONS
// =================================================================
#define BUTTON_PIN 23
#define BUZZER_PIN 4

#define LED1_PIN 25  // Green LED: NORMAL
#define LED2_PIN 26  // Yellow LED: WARNING
#define LED3_PIN 33  // Red LED: FAULT

#define SDA_PIN 21
#define SCL_PIN 22

// =================================================================
// 2. CONFIGURATION & CREDENTIALS
// =================================================================
#define IO_USERNAME  
#define IO_KEY       

#define WIFI_SSID    
#define WIFI_PASS    

// Sampling and FFT Settings
#define SAMPLES 128             // Must be a power of 2
#define SAMPLING_FREQUENCY 500  // Hz

// Baseline Calibration Limits (m/s^2)
const double BASELINE_RMS = 0.5;   
const double CRITICAL_RMS = 3.5;   

// =================================================================
// 3. OBJECT INITIALIZATION
// =================================================================
AdafruitIO_WiFi io(IO_USERNAME, IO_KEY, WIFI_SSID, WIFI_PASS);

AdafruitIO_Feed *feedRMS      = io.feed("vibration-rms");
AdafruitIO_Feed *feedPeak     = io.feed("vibration-peak");
AdafruitIO_Feed *feedCrest    = io.feed("crest-factor");
AdafruitIO_Feed *feedFreq     = io.feed("dominant-freq");
AdafruitIO_Feed *feedScore    = io.feed("anomaly-score");
AdafruitIO_Feed *feedStatus   = io.feed("machine-status");

Adafruit_MPU6050 mpu;
ArduinoFFT<double> FFT = ArduinoFFT<double>();

double vReal[SAMPLES];
double vImag[SAMPLES];
unsigned int sampling_period_us;

void setup() {
  Serial.begin(115200);

  // Initialize GPIO Pins
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(LED3_PIN, OUTPUT);

  // Turn off output indicators
  digitalWrite(BUZZER_PIN, LOW);
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
    digitalWrite(LED2_PIN, !digitalRead(LED2_PIN)); // Blink yellow during connection
    delay(500);
  }
  digitalWrite(LED2_PIN, LOW);
  Serial.println("\nConnected to Adafruit IO!");

  sampling_period_us = round(1000000 * (1.0 / SAMPLING_FREQUENCY));
}

void loop() {
  io.run();

  double sum_sq = 0;
  double peak = 0;

  // ---------------------------------------------------------------
  // Step A: Signal Acquisition & Time-Domain Calculation
  // ---------------------------------------------------------------
  for (int i = 0; i < SAMPLES; i++) {
    unsigned long start_time = micros();

    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    double acc_mag = sqrt(pow(a.acceleration.x, 2) + 
                          pow(a.acceleration.y, 2) + 
                          pow(a.acceleration.z, 2)) - 9.81;

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
  double anomaly_score = ((rms - BASELINE_RMS) / (CRITICAL_RMS - BASELINE_RMS)) * 100.0;
  anomaly_score = constrain(anomaly_score, 0.0, 100.0);

  String machine_status = "NORMAL";

  if (anomaly_score >= 75.0) {
    machine_status = "FAULT";
    
    // Status Indicator: Red LED
    digitalWrite(LED1_PIN, LOW);
    digitalWrite(LED2_PIN, LOW);
    digitalWrite(LED3_PIN, HIGH);
    
    // Local Audio Alert: Tone pattern
    tone(BUZZER_PIN, 1000, 500);
  } else if (anomaly_score >= 35.0) {
    machine_status = "WARNING";
    
    // Status Indicator: Yellow LED
    digitalWrite(LED1_PIN, LOW);
    digitalWrite(LED2_PIN, HIGH);
    digitalWrite(LED3_PIN, LOW);
    
    noTone(BUZZER_PIN);
  } else {
    // Status Indicator: Green LED
    digitalWrite(LED1_PIN, HIGH);
    digitalWrite(LED2_PIN, LOW);
    digitalWrite(LED3_PIN, LOW);
    
    noTone(BUZZER_PIN);
  }

  // Optional: Reset/Override state via Push Button
  if (digitalRead(BUTTON_PIN) == LOW) {
    noTone(BUZZER_PIN);
    Serial.println("Manual Mute Button Pressed!");
  }

  // Serial Diagnostics Logging
  Serial.printf("RMS: %.2f | Peak: %.2f | Crest: %.2f | Freq: %.1f Hz | Score: %.1f%% | Status: %s\n",
                rms, peak, crest_factor, dominant_freq, anomaly_score, machine_status.c_str());

  // ---------------------------------------------------------------
  // Step D: Publish Data to Adafruit IO
  // ---------------------------------------------------------------
  feedRMS->save(rms);
  feedPeak->save(peak);
  feedCrest->save(crest_factor);
  feedFreq->save(dominant_freq);
  feedScore->save(anomaly_score);
  feedStatus->save(machine_status);

  delay(3000); 
}
