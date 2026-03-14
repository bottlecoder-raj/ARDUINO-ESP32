#include <esp_now.h>
#include <WiFi.h>

#define VRY 34
#define VRX 35
#define SW 12

uint8_t receiverMAC[] = {0xD4,0x8A,0xFC,0xD0,0xC8,0xF8};

typedef struct struct_message {
  int x;
  int y;
  int button;
} struct_message;

struct_message data;

esp_now_peer_info_t peerInfo;

void OnDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {

  Serial.print("Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");

}

void setup() {

  Serial.begin(115200);
  analogReadResolution(12);

  pinMode(15, OUTPUT);
  pinMode(2, OUTPUT);
  pinMode(SW, INPUT_PULLUP);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Init Failed");
    return;
  }

  esp_now_register_send_cb(OnDataSent);

  memcpy(peerInfo.peer_addr, receiverMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
}

void loop() {

  digitalWrite(2, HIGH);
  digitalWrite(15, LOW);

  int xValue = analogRead(VRX);
  int yValue = analogRead(VRY);
  int button = digitalRead(SW);

  Serial.printf("X:%d  Y:%d  Button:%d\n", xValue, yValue, button);

  data.x = map(xValue,0,4095,-245,245);
  data.y = map(yValue,0,4095,-245,245);
  if (abs(data.x) < 30) data.x = 0;
  if (abs(data.y) < 30) data.y = 0;
  data.button = button;

  esp_now_send(receiverMAC, (uint8_t *)&data, sizeof(data));

  delay(20);
}
