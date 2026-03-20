#include <ESP8266WiFi.h>

const char* ssid = "HELLO";
const char* password = "12345678";

WiFiServer server(80);

// LED Pins
#define LED_POWER 2    // GPIO2 (D4)
#define LED_STATUS 16  // GPIO16 (D0)

// Blink control
unsigned long lastBlink = 0;
bool ledState = LOW;

void setup() {
  Serial.begin(9600);

  pinMode(LED_POWER, OUTPUT);
  pinMode(LED_STATUS, OUTPUT);

  digitalWrite(LED_POWER, LOW);   // Power ON LED

  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");

  // 🔥 WAIT until connected
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    
    // Slow blink while connecting
    digitalWrite(LED_STATUS, LOW);
    delay(100);
    digitalWrite(LED_STATUS, HIGH);
  }

  Serial.println("\nConnected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.begin();
  Serial.write('S');
}

void loop() {

  // ✅ Connected → Blink LED continuously
  unsigned long currentMillis = millis();
  if (currentMillis - lastBlink >= 1000) {
    lastBlink = currentMillis;
    ledState = !ledState;
    digitalWrite(LED_STATUS, ledState);
  }

  // 🌐 Handle client
  WiFiClient client = server.available();
  if (!client) return;

  String request = client.readStringUntil('\r');
  client.flush();

  // Send commands to Arduino
  if (request.indexOf("/F") != -1) Serial.write('F');
  if (request.indexOf("/B") != -1) Serial.write('B');
  if (request.indexOf("/L") != -1) Serial.write('L');
  if (request.indexOf("/R") != -1) Serial.write('R');
  if (request.indexOf("/S") != -1) Serial.write('S');

  // Web Page
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
  client.stop();
}
