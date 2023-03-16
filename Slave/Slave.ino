#include <WiFi101.h>

#define ssid "ZBC-E-CH-SKP019 0986"
#define pass "710%dK14"

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
 
}


