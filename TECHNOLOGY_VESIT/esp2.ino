#include <ESP8266WiFi.h>

const char* ssid = "Raj_4g";
const char* password = "55556666";

WiFiServer server(80);

void setup() {
  Serial.begin(9600);   // Must match Arduino espSerial

  WiFi.begin(ssid, password);
  Serial.println("Connecting...");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.begin();
}

void loop() {
  WiFiClient client = server.available();
  if (!client) return;

  while (!client.available()) {
    delay(1);
  }

  String request = client.readStringUntil('\r');
  Serial.println(request);
  client.flush();

  char cmd = 'S';  // default

  // Detect command
  if (request.indexOf("/F") != -1) cmd = 'F';
  else if (request.indexOf("/B") != -1) cmd = 'B';
  else if (request.indexOf("/L") != -1) cmd = 'L';
  else if (request.indexOf("/R") != -1) cmd = 'R';
  else if (request.indexOf("/S") != -1) cmd = 'S';

  // Send command to Arduino
  Serial.write(cmd);
  Serial.print("Sent: ");
  Serial.println(cmd);

  // 🌐 Responsive Web Page
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("");

  client.println("<!DOCTYPE html><html>");
  client.println("<head>");
  client.println("<meta name='viewport' content='width=device-width, initial-scale=1'>");

  client.println("<style>");
  client.println("body { text-align:center; font-family:Arial; background:#111; color:white; margin:0; }");
  client.println("h1 { margin-top:20px; }");

  client.println(".btn {");
  client.println("  width:90px; height:90px;");
  client.println("  font-size:28px;");
  client.println("  margin:8px;");
  client.println("  border:none;");
  client.println("  border-radius:20px;");
  client.println("  background:#00adb5;");
  client.println("  color:white;");
  client.println("}");

  client.println(".btn:active { background:#007b80; }");

  client.println(".container { margin-top:30px; }");

  client.println("@media (min-width:600px) {");
  client.println("  .btn { width:120px; height:120px; font-size:32px; }");
  client.println("}");
  client.println("</style>");

  client.println("</head>");
  client.println("<body>");

  client.println("<h1>Robot Control</h1>");
  client.println("<div class='container'>");

  // Forward
  client.println("<div><a href='/F'><button class='btn'>↑</button></a></div>");

  // Left, Stop, Right
  client.println("<div>");
  client.println("<a href='/L'><button class='btn'>←</button></a>");
  client.println("<a href='/S'><button class='btn'>■</button></a>");
  client.println("<a href='/R'><button class='btn'>→</button></a>");
  client.println("</div>");

  // Backward
  client.println("<div><a href='/B'><button class='btn'>↓</button></a></div>");

  client.println("</div>");
  client.println("</body></html>");

  delay(1);
  client.stop();
}
