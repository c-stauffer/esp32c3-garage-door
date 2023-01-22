// this code adapted from:
/*********
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp32-async-web-server-espasyncwebserver-library/
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*********/

// Import required libraries. The URLs will download a zip, which you can then add to Arduino IDE.
#include <WiFi.h>
#include <AsyncTCP.h>          // https://github.com/me-no-dev/AsyncTCP/archive/master.zip
#include <ESPAsyncWebServer.h> // https://github.com/me-no-dev/ESPAsyncWebServer/archive/master.zip
#include <Adafruit_Sensor.h>   // https://github.com/adafruit/Adafruit_Sensor/archive/master.zip
#include <DHT.h>               // https://github.com/adafruit/DHT-sensor-library/archive/master.zip

// configurable stuff here:
const char* NET_SSID        = "REDACTED";
const char* NET_PASSPHRASE  = "REDACTED";
const int   LISTEN_PORT     = 6262;
const int   SERIAL_BAUDRATE = 115200;

// TODO: determine which pins to use for the stuff, define them here
const int   DOOR_SWITCH     = 4;
const int   DOOR_OPEN_P     = 5;
const int   DOOR_CLOSED_P   = 6;
const int   DHT_PIN         = 7;

enum DoorStates {
  OPEN,
  CLOSED,
  OPENING,
  CLOSING,
  UNKNOWN
};

// store the last HIGH reed switch, so that if neither switch is
// currently HIGH, we can interpolate whether the door is opening
// or closing.
DoorStates _lastKnownDoorState = UNKNOWN;

// values for configuring static IP
IPAddress my_ip(192, 168, 1, 45);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);

// Create AsyncWebServer object on port
AsyncWebServer server(LISTEN_PORT);
DHT dht(DHT_PIN, DHT22);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(SERIAL_BAUDRATE);
  
  // TODO: pin inits?
  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);

  // config wifi for static ip
  if (!WiFi.config(my_ip, gateway, subnet))
    Serial.println("Could not configure wifi connection for static IP."); 
  // connect to wifi
  WiFi.begin(NET_SSID, NET_PASSPHRASE);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to wifi...");
  }
  Serial.println("Connection successful.");

  // hit the garage door switch:
  server.on("/actuate", HTTP_GET, [] (AsyncWebServerRequest *request) {
    Serial.println("Actuating door.");
    digitalWrite(DOOR_SWITCH, 1);
    delay(200);
    digitalWrite(DOOR_SWITCH, 0);
    request->send(200, "text/plain", "OK");
  });


  // get current door state (open/closed/opening/closing):
  server.on("/state", HTTP_GET, [] (AsyncWebServerRequest *request) {
    Serial.println("Getting door state.");
    if (digitalRead(DOOR_OPEN_P) == HIGH) {
      Serial.println("Door is open.");
      request->send(200, "text/plain", "open");
    }
    else if (digitalRead(DOOR_CLOSED_P) == HIGH) {
      Serial.println("Door is closed.");
      request->send(200, "text/plain", "closed");
    }
    else if (_lastKnownDoorState == OPEN || _lastKnownDoorState == CLOSING) {
      Serial.println("Door is closing.");
      request->send(200, "text/plain", "closing");
    }
    else if (_lastKnownDoorState == CLOSED || _lastKnownDoorState == OPENING) {
      Serial.println("Door is opening.");
      request->send(200, "text/plain", "opening");
    }
    else {
      Serial.println("Door state unknown.");
      request->send(200, "text/plain", "unknown");
    }
  });

  // get temp/humidity from dht22:
  server.on("/temperature", HTTP_GET, [] (AsyncWebServerRequest *request) {
    Serial.println("Getting temperature.");
    request->send_P(200, "text/plain", readTemp().c_str());
  });
  server.on("/humidity", HTTP_GET, [] (AsyncWebServerRequest *request) {
    Serial.println("Getting humidity.");
    request->send_P(200, "text/plain", readHumidity().c_str());
  });
  server.on("/heatindex", HTTP_GET, [] (AsyncWebServerRequest *request) {
    Serial.println("Getting heat index.");
    request->send_P(200, "text/plain", readHeatIndex().c_str());
  });

  server.begin();
}

void loop() { }

String readTemp()
{
  float t = dht.readTemperature();
  if (isnan(t)) {
    Serial.println("Failure reading temperature.");
    return "--";
  }
  return String(t);
}

String readHumidity()
{
  float h = dht.readTemperature();
  if (isnan(h)) {
    Serial.println("Failure reading humidity.");
    return "--";
  }
  return String(h);
}

String readHeatIndex()
{
  float h = dht.computeHeatIndex();
  if (isnan(h)) {
    Serial.println("Failure computing heat index.");
    return "--";
  }
  return String(h);
}
