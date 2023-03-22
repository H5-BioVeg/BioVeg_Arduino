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
WiFiServer server(port);
//DHT11
int DHT11_PIN = 8;
SimpleDHT11 dht11(DHT11_PIN);

//Data from sensor
int smd[6] = { 0, 0, 0, 0, 0, 0 };
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

int state = RECIVE_DATA;

void setup() {
  Serial.begin(9600);

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
  if (state == RECIVE_DATA) {
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
              for (int i = 0; i < 6; i++) {
                smd[i] = jsonBuffer["Soil Moisture Data"][i];
              }
            }
            sendStandardHttpResponse(client, "text/html", "OK");
            Serial.println("\n############################################");
            readDHT();
            changeState();
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
  } else if (state == GET_DATA) {
    Serial.println("############ GET ############");
    //WiFiSSL client for HTTPS connection
    WiFiSSLClient http;
    //Connect to Firebase
    http.connect(FIREBASE_HOST, FIREBASE_PORT);
    delay(500);
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
    delay(2000);
    changeState();
  } else if (state == PATCH_DATA) {
    Serial.println("############ PATCH ############");
    //WiFiSSL client for HTTPS connection
    WiFiSSLClient http;
    //Connect to Firebase
    http.connect(FIREBASE_HOST, FIREBASE_PORT);
    delay(500);
    //Check connection
    if (http.connected()) {
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
      for (int i = 0; i < 6; i++) {
        doc["greenhouses"]["greenhouse1"]["pots"]["pot" + String(i + 1)]["currentSoilMoisture"] = smd[i];
      }
      // Serialize JSON to file
      String jsonString;
      serializeJson(doc, jsonString);
      Serial.println(jsonString);

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
      delay(2000);
      changeState();
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

//Read DHT11 sensor data
void readDHT() {
  dht11.read(&temp, &hum, NULL);
  Serial.print((int)temp);
  Serial.print(" *C, ");
  Serial.print((int)hum);
  Serial.println(" H");
}

void sendStandardHttpResponse(WiFiClient client, String contentType, String content) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:" + contentType);
  client.println();
  client.println(content);
  client.println();
}

void changeState() {
  if (state == RECIVE_DATA) {
    state = GET_DATA;
  } else if (state == GET_DATA) {
    state = PATCH_DATA;
  } else if (state == PATCH_DATA) {
    state = RECIVE_DATA;
  }
}
