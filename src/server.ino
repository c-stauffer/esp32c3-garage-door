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
#include "netcreds.h"


#define SERIAL_OUTPUT

const char* APP_VERS        = "1.0.0";

// set serial baudrate for debugging
const int   SERIAL_BAUDRATE = 115200;

// GPIO pin assignments
const int   DOOR_SWITCH     = 0;
const int   DHT_PIN         = 1;
const int   DOOR_OPEN_P     = 3;

// Changed from GPIO 2 (as shown in wiring-diagram.png) to 10 for troubleshooting
// something... Ended up being a different issue, but just going to keep it on the
// new pin.
const int   DOOR_CLOSED_P   = 10; 

// Door states
const int   D_OPEN          = 0;
const int   D_CLOSED        = 1;
const int   D_OPENING       = 2;
const int   D_CLOSING       = 3;
const int   D_UNKNOWN       = 4;

static const char* stateValues[] = {
  [D_OPEN] = "OPEN",
  [D_CLOSED] = "CLOSED",
  [D_OPENING] = "OPENING",
  [D_CLOSING] = "CLOSING",
  [D_UNKNOWN] = "UNKNOWN"
};

int last_door_state = D_UNKNOWN;

// Create AsyncWebServer object on port
AsyncWebServer server(NC_LISTEN_PORT);
DHT dht(DHT_PIN, DHT22);

void setup() {
  // put your setup code here, to run once:
  #ifdef SERIAL_OUTPUT
    Serial.begin(SERIAL_BAUDRATE);
  #endif
  
  dht.begin();

  // GPIO init
  pinMode(DOOR_OPEN_P, INPUT);
  pinMode(DOOR_CLOSED_P, INPUT);
  pinMode(DHT_PIN, INPUT);
  pinMode(DOOR_SWITCH, OUTPUT);
  digitalWrite(DOOR_SWITCH, HIGH);

  // config wifi for static ip
  if (!WiFi.config(NC_MY_IP, NC_GATEWAY, NC_SUBNET))
    serialPrintln("Could not configure wifi connection for static IP."); 
  // connect to wifi
  WiFi.begin(NC_SSID, NC_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    serialPrintln("Connecting to wifi...");
  }
  serialPrintln("Connection successful.");

  // ping request
  server.on("/ping", HTTP_GET, [] (AsyncWebServerRequest *request) {
    serialPrintln("Ping received. Replying.");
    request->send(200, "text/plain", APP_VERS);
  });

  // hit the garage door switch:
  server.on("/actuate", HTTP_GET, [] (AsyncWebServerRequest *request) {
    serialPrintln("Actuating door.");
    digitalWrite(DOOR_SWITCH, LOW);
    delay(250); // give a bit of time in LOW
    digitalWrite(DOOR_SWITCH, HIGH);
    request->send(200, "text/plain", "OK");
  });


  // get current door state (open/closed/opening/closing):
  server.on("/state", HTTP_GET, [] (AsyncWebServerRequest *request) {
    serialPrintln("Getting door state.");
    int newState = calculateDoorState(digitalRead(DOOR_CLOSED_P), digitalRead(DOOR_OPEN_P), last_door_state);
    
    serialPrint("Door is ");
    serialPrintln(stateValues[newState]);
    // changed this to maintain the last *confirmed* door state (OPEN/CLOSED) rather
    // than last *calculated* state, i.e. no longer can have last_door_state of OPENING or CLOSING.
    if (newState == D_OPEN || newState == D_CLOSED)
      last_door_state = newState;
    request->send(200, "text/plain", stateValues[newState]);
  });

  // return raw switch values (and previous state) as json
  server.on("/state2", HTTP_GET, [] (AsyncWebServerRequest *request) {
    serialPrintln("Getting door state2.");
    String jsonResp = "{\"open\":";
    int openVal = digitalRead(DOOR_OPEN_P);
    int closedVal = digitalRead(DOOR_CLOSED_P);

    if (openVal == LOW)
      jsonResp += "\"LOW\",";
    else 
      jsonResp += "\"HIGH\",";
    jsonResp += "\"closed\":";
    if (closedVal == LOW)
      jsonResp += "\"LOW\",";
    else 
      jsonResp += "\"HIGH\",";
    jsonResp += "\"last\":\"" + String(stateValues[last_door_state]) + "\"}";
    // changed this to maintain the last *confirmed* door state (OPEN/CLOSED) rather
    // than last *calculated* state, i.e. no longer can have last_door_state of OPENING or CLOSING.
    int newState = calculateDoorState(closedVal, openVal, last_door_state);
    if (newState == D_OPEN || newState == D_CLOSED)
      last_door_state = newState;
    request->send(200, "application/json", jsonResp);
  });

  // get temp/humidity from dht22:
  server.on("/temperature", HTTP_GET, [] (AsyncWebServerRequest *request) {
    serialPrintln("Getting temperature.");
    request->send_P(200, "text/plain", readTemp().c_str());
  });
  server.on("/humidity", HTTP_GET, [] (AsyncWebServerRequest *request) {
    serialPrintln("Getting humidity.");
    request->send_P(200, "text/plain", readHumidity().c_str());
  });
  server.on("/heatindex", HTTP_GET, [] (AsyncWebServerRequest *request) {
    serialPrintln("Getting heat index.");
    request->send_P(200, "text/plain", readHeatIndex().c_str());
  });

  server.begin();
}

void loop() { }

String readTemp()
{
  float t = dht.readTemperature();
  if (isnan(t)) {
    serialPrintln("Failure reading temperature.");
    return "--";
  }
  return String(t);
}

String readHumidity()
{
  float h = dht.readHumidity();
  if (isnan(h)) {
    serialPrintln("Failure reading humidity.");
    return "--";
  }
  return String(h);
}

String readHeatIndex()
{
  // readTemperature defaults to celsius but computeHeatIndex defaults to Fahrenheit...
  float h = dht.computeHeatIndex(false);
  if (isnan(h)) {
    serialPrintln("Failure computing heat index.");
    return "--";
  }
  return String(h);
}

int calculateDoorState(int closedSwitchValue, int openSwitchValue, int previousState) {
  if (openSwitchValue == HIGH) return D_OPEN;
  else if (closedSwitchValue == HIGH) return D_CLOSED;
  else if (previousState == D_OPEN || previousState == D_CLOSING) return D_CLOSING;
  else if (previousState == D_CLOSED || previousState == D_OPENING) return D_OPENING;
  return D_UNKNOWN;
}

void serialPrint(const char message[])
{
  #ifdef SERIAL_OUTPUT
    Serial.print(message);
  #endif
}

void serialPrintln(const char message[])
{
  #ifdef SERIAL_OUTPUT
    Serial.println(message);
  #endif
}
