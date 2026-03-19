#include <AFMotor.h>
#include <SoftwareSerial.h>

// ESP on pins 10 (RX), 9 (TX)
SoftwareSerial espSerial(10, 9);

// Motors
AF_DCMotor Motor1(1, MOTOR12_1KHZ);
AF_DCMotor Motor2(2, MOTOR12_1KHZ);
AF_DCMotor Motor3(3, MOTOR34_1KHZ);
AF_DCMotor Motor4(4, MOTOR34_1KHZ);

char command = 'S';

void setup() {
  Serial.begin(9600);      // Serial Monitor
  espSerial.begin(9600);   // ESP8266

  Serial.println("Ready: Serial + WiFi control");
}

void loop() {

  // 🔹 Read from Serial Monitor
  if (Serial.available()) {
    char incoming = Serial.read();
    incoming = toupper(incoming);

    if (incoming != '\n' && incoming != '\r') {
      command = incoming;
      Serial.print("PC CMD: ");
      Serial.println(command);
    }
  }

  // 🔹 Read from ESP8266
  if (espSerial.available()) {
    char incoming = espSerial.read();
    incoming = toupper(incoming);

    if (incoming != '\n' && incoming != '\r') {
      command = incoming;
      Serial.print("WIFI CMD: ");
      Serial.println(command);
    }
  }

  // 🔥 SAME MOTOR CONTROL
  if (command == 'F') {
    Motor1.setSpeed(80); Motor1.run(FORWARD);
    Motor2.setSpeed(80); Motor2.run(FORWARD);
    Motor3.setSpeed(80); Motor3.run(FORWARD);
    Motor4.setSpeed(80); Motor4.run(FORWARD);
  }

  else if (command == 'B') {
    Motor1.setSpeed(80); Motor1.run(BACKWARD);
    Motor2.setSpeed(80); Motor2.run(BACKWARD);
    Motor3.setSpeed(80); Motor3.run(BACKWARD);
    Motor4.setSpeed(80); Motor4.run(BACKWARD);
  }

  else if (command == 'L') {
    Motor1.setSpeed(80); Motor1.run(BACKWARD);
    Motor2.setSpeed(80); Motor2.run(FORWARD);
    Motor3.setSpeed(80); Motor3.run(FORWARD);
    Motor4.setSpeed(80); Motor4.run(BACKWARD);
  }

  else if (command == 'R') {
    Motor1.setSpeed(80); Motor1.run(FORWARD);
    Motor2.setSpeed(80); Motor2.run(BACKWARD);
    Motor3.setSpeed(80); Motor3.run(BACKWARD);
    Motor4.setSpeed(80); Motor4.run(FORWARD);
  }

  else if (command == 'S') {
    Motor1.run(RELEASE);
    Motor2.run(RELEASE);
    Motor3.run(RELEASE);
    Motor4.run(RELEASE);
  }
}
