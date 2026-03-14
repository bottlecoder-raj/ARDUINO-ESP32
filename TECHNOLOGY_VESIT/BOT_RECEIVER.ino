#include <esp_now.h>
#include <WiFi.h>

typedef struct struct_message {
  int x;
  int y;
  int button;
} struct_message;

struct_message data;

// NEW callback format
void OnDataRecv(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {

  memcpy(&data, incomingData, sizeof(data));

  Serial.printf("X:%d  Y:%d  Button:%d\n", data.x, data.y, data.button);
}

void setup() {

  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();     // important
  delay(100);

  Serial.println("MAC ADDRESS FOR RECEIVER: ");
  Serial.println(WiFi.macAddress());
  esp_now_init();

  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
}
