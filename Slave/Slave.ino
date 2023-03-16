#include <WiFi101.h>
#include <ArduinoJson.h>

#define ssid "ZBC-E-CH-SKP019 0986"
#define pass "710%dK14"

#define ip "192.168.137.206"
#define port 63584

char deviceName[] = "Arduino";

#define sensorPower 7
#define sensorPin A0

#define sensorPower1 8
#define sensorPin1 A1

#define sensorPower2 9
#define sensorPin2 A2

#define sensorPower3 10
#define sensorPin3 A3

#define sensorPower4 11
#define sensorPin4 A4

#define sensorPower5 12
#define sensorPin5 A5

void setup() {
  Serial.begin(9600);
  setupSoilSensor();


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
      data.add(readDataFromSoilSensor(sensorPower,sensorPin));
      data.add(readDataFromSoilSensor(sensorPower1,sensorPin1));
      data.add(readDataFromSoilSensor(sensorPower2,sensorPin2));
      data.add(readDataFromSoilSensor(sensorPower3,sensorPin3));
      data.add(readDataFromSoilSensor(sensorPower4,sensorPin4));
      data.add(readDataFromSoilSensor(sensorPower5,sensorPin5));

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

void setupSoilSensor()
{
  pinMode(sensorPower, OUTPUT);
	pinMode(sensorPower1, OUTPUT);
  pinMode(sensorPower2, OUTPUT);
	pinMode(sensorPower3, OUTPUT);
	pinMode(sensorPower4, OUTPUT);
  pinMode(sensorPower5, OUTPUT);

  digitalWrite(sensorPower, LOW);
	digitalWrite(sensorPower1, LOW);
  digitalWrite(sensorPower2, LOW);
  digitalWrite(sensorPower3, LOW);
	digitalWrite(sensorPower4, LOW);
  digitalWrite(sensorPower5, LOW);
}

int readDataFromSoilSensor(int sensorPowerPin, int sensorAnalogPin)
{
  digitalWrite(sensorPowerPin, HIGH);	
	delay(10);

	int value = analogRead(sensorAnalogPin);

	digitalWrite(sensorPowerPin, LOW);

  return value;
}


