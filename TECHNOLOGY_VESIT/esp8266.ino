#include <ESP8266WiFi.h>

const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

WiFiServer server(80);

void setup() {
  Serial.begin(9600);   // Must match Arduino espSerial

  WiFi.begin(ssid, password);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  server.begin();

  // Print IP (open Serial Monitor ON ESP to see this)
  Serial.println(WiFi.localIP());
}

void loop() {
  WiFiClient client = server.available();
  if (!client) return;

  String request = client.readStringUntil('\r');
  client.flush();

  // 🔥 Send commands to Arduino
  if (request.indexOf("/F") != -1) Serial.write('F');
  if (request.indexOf("/B") != -1) Serial.write('B');
  if (request.indexOf("/L") != -1) Serial.write('L');
  if (request.indexOf("/R") != -1) Serial.write('R');
  if (request.indexOf("/S") != -1) Serial.write('S');

  // Simple Web Page
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("");

  client.println("<h1>Robot Control</h1>");
  client.println("<a href=\"/F\">Forward</a><br>");
  client.println("<a href=\"/B\">Backward</a><br>");
  client.println("<a href=\"/L\">Left</a><br>");
  client.println("<a href=\"/R\">Right</a><br>");
  client.println("<a href=\"/S\">Stop</a><br>");

  delay(1);
}
