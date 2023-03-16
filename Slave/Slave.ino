#include <WiFi101.h>
#include <ArduinoJson.h>

#define ssid "ZBC-E-CH-SKP019 0986"
#define pass "710%dK14"

#define ip "192.168.137.106"
#define port 63584

char deviceName[] = "Arduino";

void setup() {
  Serial.begin(9600);

  WiFi.hostname(deviceName);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connecting to WiFi...");
    delay(5000);
    WiFi.begin(ssid, pass);
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Connected!!");
    }
  }
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient http;
    http.connect(ip, port);
    if (http.connected()) {

      StaticJsonBuffer<200> jsonBuffer;
      JsonObject& root = jsonBuffer.createObject();
      JsonArray& data = root.createNestedArray("Soil Moisture Data");
      data.add(0);
      data.add(200);
      data.add(400);
      data.add(600);
      data.add(800);
      data.add(1000);

      String json;
      root.printTo(json);
      //Send HTTP POST request
      http.println("POST / HTTP/1.1");
      http.println("Host: " + String(ip));
      http.println("Content-Type: text/plain");
      http.println("Content-Length: " + String(json.length()));
      http.println("Accept-Encoding: gzip, deflate, br");
      http.println("Accept: */*");
      http.println("Connection: keep-alive");
      http.print("\n");

      Serial.println(json);
      http.println(json);
      http.println();

      //Read HTTP response from server

      while (http.connected()) {
        String line = http.readStringUntil('\r');
        Serial.println(line);
      }

      http.stop();
      delay(1000);
    }
  }
}


