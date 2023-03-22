#include <SPI.h>
#include <WiFi101.h>      //0.16.0
#include <ArduinoJson.h>  //5.13.4
#include <SimpleDHT.h>    //1.0.14

//Firebase
#define FIREBASE_HOST "bioveg-ca9ae-default-rtdb.europe-west1.firebasedatabase.app"
#define FIREBASE_AUTH "CpQZ7z1x4FyqEvY2NV6Fveum6cXaKizIVheCUQqT"
#define FIREBASE_PORT 443
//WiFi
char ssid[] = "ZBC-E-CH-SKP019 0986";
char pass[] = "710%dK14";
int status = WL_IDLE_STATUS;
//Server
#define DEVICE_NAME "Master Arduino"
#define port 63584
//DHT11
int DHT11_PIN = 8;
SimpleDHT11 dht11(DHT11_PIN);

int statusH = WL_IDLE_STATUS;
WiFiServer server(port);

//Data from sensor
int smd[6] = {0, 0, 0, 0, 0, 0};

byte temp = 0;
byte hum = 0;
//Data from Firebase
String jsonData;

//State machine
enum state {
  RECIVE_DATA,
  GET_DATA,
  PATCH_DATA
};
//State
int pr = 0;

void setup() {
  Serial.begin(9600);
  while (!Serial) { ; }


  WiFi.hostname(DEVICE_NAME);
  status = WiFi.begin(ssid, pass);
  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    Serial.println("Connecting to WiFi...");
    delay(5000);
    status = WiFi.begin(ssid, pass);
  }

  if (status == WL_CONNECTED) {
    Serial.println("Connected!!");
  }
  WiFi.scanNetworks();

  // start the server:
  status = WiFi.beginAP(ssid);
  if (status != WL_AP_LISTENING) {
    Serial.println("Failed!!");
  } else {
    server.begin();
    printWiFiStatus();
    delay(2000);
  }
}

void loop() {
  // listen for incoming clients
  WiFiClient client = server.available();
  delay(500);
  // make a String to hold incoming data from the client
  String currentLine = "";
  if (pr == 0) {
    Serial.println("############ RECIVE ############");
    // if you get a client,
    while (client.connected()) {
      // if there's bytes to read from the client,
      if (client.available()) {
        // read a byte
        char c = client.read();
        delay(10);
        Serial.write(c);
        // if the byte is a newline character
        if (c == '\n') {
          if (currentLine.length() == 0) {
            String requestBody;
            // the HTTP request ends with another blank line
            while (client.available()) {
              // read all the lines of the request body
              requestBody += (char)client.read();
            }

            if (requestBody.length()) {
              Serial.println("################ Body JSON ################");
              Serial.println(requestBody);
              Serial.println();

              // Parse JSON object
              Serial.println("################ Body EXTRACTED ################");
              StaticJsonDocument<200> jsonBuffer;
              // Allocate the JSON document
              char* json = new char[requestBody.length() + 1];
              // Copy string into buffer
              strcpy(json, requestBody.c_str());
              // Deserialize the JSON document
              deserializeJson(jsonBuffer, json);
              // Extract values
              const char* sensor = jsonBuffer["sensor"];
              smd[0] = jsonBuffer["Soil Moisture Data"][0];
              smd[1] = jsonBuffer["Soil Moisture Data"][1];
              smd[2] = jsonBuffer["Soil Moisture Data"][2];
              smd[3] = jsonBuffer["Soil Moisture Data"][3];
              smd[4] = jsonBuffer["Soil Moisture Data"][4];
              smd[5] = jsonBuffer["Soil Moisture Data"][5];
            }
            // send a standard http response header
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();
            client.println("OK");
            client.println();
            Serial.println("\n############################################");
            // read the data from the DHT sensor
            dht11.read(&temp, &hum, NULL);
            Serial.print((int)temp);
            Serial.print(" *C, ");
            Serial.print((int)hum);
            Serial.println(" H");
            //Change state
            pr = 1;
            // break out of the while loop
            break;
          } else {
            // if get a newline then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }
    // close the connection:
    client.stop();
    client.flush();
  } else if (pr == 1) {
    Serial.println("############ GET ############");
    //WiFiSSL client for HTTPS connection
    WiFiSSLClient http;
    //Connect to Firebase
    http.connect(FIREBASE_HOST, FIREBASE_PORT);
    delay(2000);
    //Check connection
    if (http.connected()) {
      //Send HTTP request to Firebase
      http.println("GET /.json HTTP/1.1");
      http.println("Host: " + String(FIREBASE_HOST));
      http.println("Content-Type: application/json");
      http.println("Connection: close");
      http.println();
      http.println();
    }
    //Read response from Firebase
    while (http.connected()) {
      jsonData = http.readStringUntil('\r');
      Serial.println(jsonData);
    }
    //Close connection
    http.stop();
    http.flush();
    delay(5000);
    //Change state
    pr = 2;
  } else {
    Serial.println("############ PATCH ############");
    //WiFiSSL client for HTTPS connection
    WiFiSSLClient http;
    //Connect to Firebase
    http.connect(FIREBASE_HOST, FIREBASE_PORT);
    delay(5000);
    //Check connection
    if (http.connected()) {
      Serial.println("Debug");
      // json data to send to firebase
      char json[jsonData.length() + 1];
      strcpy(json, jsonData.c_str());
      // Parse JSON object
      StaticJsonDocument<1000> doc;
      // Deserialize the JSON document
      deserializeJson(doc, json);
      // Extract values from object and change them
      doc["greenhouses"]["greenhouse1"]["humidity"] = (int)hum;
      doc["greenhouses"]["greenhouse1"]["temperature"] = (int)temp;
      doc["greenhouses"]["greenhouse1"]["pots"]["pot1"]["currentSoilMoisture"] = smd[0];
      doc["greenhouses"]["greenhouse1"]["pots"]["pot2"]["currentSoilMoisture"] = smd[1];
      doc["greenhouses"]["greenhouse1"]["pots"]["pot3"]["currentSoilMoisture"] = smd[2];
      doc["greenhouses"]["greenhouse1"]["pots"]["pot4"]["currentSoilMoisture"] = smd[3];
      doc["greenhouses"]["greenhouse1"]["pots"]["pot5"]["currentSoilMoisture"] = smd[4];
      doc["greenhouses"]["greenhouse1"]["pots"]["pot6"]["currentSoilMoisture"] = smd[5];
      // Serialize JSON to file
      String jsonString;
      serializeJson(doc, jsonString);
      Serial.println(jsonString);

      delay(1000);

      //Send HTTP request to Firebase
      http.println("PATCH /.json HTTP/1.1");
      http.println("Host: " + String(FIREBASE_HOST));
      http.println("Content-Type: application/json");
      http.println("Content-Length: " + String(jsonString.length()));
      //http.println("Authorization: Bearer " + String(token));
      http.println("Connection: close");
      http.println();

      http.println(jsonString);
      http.println();
      //Read response from Firebase
      while (http.connected()) {
        String line = http.readStringUntil('\r');
        Serial.println(line);
      }
      //Close connection
      http.stop();
      http.flush();
      delay(5000);
      //Change state
      pr = 0;
    }
  }
}

// Print the SSID and IP address
void printWiFiStatus() {
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  Serial.print("http://");
  Serial.println(ip);
}