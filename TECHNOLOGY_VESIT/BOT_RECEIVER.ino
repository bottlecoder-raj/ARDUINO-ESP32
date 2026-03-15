#include <esp_now.h>
#include <WiFi.h>

typedef struct struct_message {
  int x;
  int y;
  int button;
} struct_message;

struct_message data;
int leftMotor;
int rightMotor;
// NEW callback format
void OnDataRecv(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {

  memcpy(&data, incomingData, sizeof(data));

  Serial.printf("X:%d  Y:%d  Button:%d\n", data.x, data.y, data.button);
  SetMotor(data.x,data.y);

}
void SetMotor(int x, int y) {
  if (x > 0 ) {
    analogWrite(18, abs(x));
    analogWrite(19, 0);
    analogWrite(26, 0);
    analogWrite(25, 0);

  } else if (x < 0) {
    analogWrite(19,0);
    analogWrite(18, 0);
    analogWrite(26, abs(x));
    analogWrite(25, 0);
  } else if (y<0) {
    analogWrite(19, abs(y));
    analogWrite(18, 0);
    analogWrite(25, abs(y));
    analogWrite(26, 0);
  } else if (y>0) {
    analogWrite(18, abs(y));
    analogWrite(19, 0);
    analogWrite(26, abs(y));
    analogWrite(25, 0);
  } else {
    analogWrite(18, 0);
    analogWrite(19, 0);
    analogWrite(26, 0);
    analogWrite(25, 0);
  }
}


void setup() {

  Serial.begin(115200);
  delay(1000);

  WiFi.mode(WIFI_STA);
  delay(500);          // give WiFi time to start
  WiFi.disconnect();

  Serial.print("MAC ADDRESS FOR RECEIVER: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Init Failed");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);
}
void loop() {

}
