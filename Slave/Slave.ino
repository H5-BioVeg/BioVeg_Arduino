#include <WiFi101.h>
#include <ArduinoJson.h>

// WiFi settings
#define ssid "ZBC-E-CH-SKP019 0986"
#define pass "710%dK14"

// Server settings
#define ip "192.168.137.29"
#define port 63584

char deviceName[] = "Arduino";
//Soil sensor settings 
int sensorPower[6] = {7,8,9,10,11,12};
int sensorPin[6] = {A0,A1,A2,A3,A4,A5};

void setup() {
  Serial.begin(9600);
  setupSoilSensor();

  //Connect to WiFi
  WiFi.hostname(deviceName);
  WiFi.begin(ssid, pass);
  //Check WiFi connection status
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
  //Check WiFi connection status
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient http;
    //Connect to server
    http.connect(ip, port);
    //Check connection status
    if (http.connected()) {
      //Create JSON object to send to server
      StaticJsonDocument<200> doc;
      JsonArray data = doc.createNestedArray("Soil Moisture Data");
      //Read data from soil sensor and add to JSON object
      for (int i = 0; i < 6; i++) {
        data.add(readDataFromSoilSensor(sensorPower[i],sensorPin[i]));
      }
      //Convert JSON object to string
      String json;
      serializeJson(doc, json);
      //Send HTTP request to server
      http.println("POST / HTTP/1.1");
      http.println("Host: " + String(ip));
      http.println("Content-Type: application/json");
      http.println("Content-Length: " + String(json.length()));
      http.println("Accept-Encoding: gzip, deflate, br");
      http.println("Accept: */*");
      http.println("Connection: keep-alive");
      http.print("\n");

      Serial.println(json);
      http.println(json);
      http.println();

      //Read response from server
      while (http.connected()) {
        String line = http.readStringUntil('\r');
        Serial.println(line);
      }
      //Close connection
      http.stop();
    }
  }
}

void setupSoilSensor()
{
  for (int i = 0; i < 6; i++) {
    //Set soil sensor power pin to output
    pinMode(sensorPower[i], OUTPUT);
    //Turn off soil sensor power
    digitalWrite(sensorPower[i], LOW);
  }
}
//Read data from soil sensor
int readDataFromSoilSensor(int sensorPowerPin, int sensorAnalogPin)
{
  //Turn on soil sensor power
  digitalWrite(sensorPowerPin, HIGH);	
	delay(10);
  //Read soil sensor value
	int value = analogRead(sensorAnalogPin);
  //Turn off soil sensor power
	digitalWrite(sensorPowerPin, LOW);

  return value;
}


