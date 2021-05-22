#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsClient.h> //  https://github.com/kakopappa/sinric/wiki/How-to-add-dependency-libraries
#include <ArduinoJson.h> // https://github.com/kakopappa/sinric/wiki/How-to-add-dependency-libraries (use the correct version)
#include <StreamString.h>
#include <SimpleTimer.h>

ESP8266WiFiMulti WiFiMulti;
WebSocketsClient webSocket;
WiFiClient client;

#define HEARTBEAT_INTERVAL 300000 // 5 Minutes 

uint64_t heartbeatTimestamp = 0;
bool isConnected = false;

//DETECT
//SMOOTHING
const int numReadings = 100;
const int gtalarm = 2;

int readings[numReadings]; // the readings from the analog input
int readIndex = 0; // the index of the current reading
int total = 0; // the running total
int average = 0; // the average


bool alarm;
float voltage;
char xvalue;


SimpleTimer timer;


//EXCHANGE VARS
int EXCHANGE_SOURCE;
const int buzzerpin = 14; // The number of the buzzer pin
int buzzerstate = 0; // Variable for reading the buzzer status

//DETECT OFF


//STATICIP

WiFiServer server(80);
IPAddress ip(10, 0, 0, 201); //your arduino IP
IPAddress gateway(10, 0, 0, 1); //your network gateway
IPAddress subnet(255, 255, 255, 0); //your network subnet mask
IPAddress dns(10, 0, 0, 1); // set dns to match your network (look up in your router)


//STATICOFF
 
#define MyApiKey "" // TODO: Change to your sinric API Key. Your API Key is displayed on sinric.com dashboard
#define MySSID "JOENET" // TODO: Change to your Wifi network SSID
#define MyWifiPassword "" // TODO: Change to your Wifi network password

#define DEVICE1 "5dff19b122a7867b6cef249b"  //TODO: Device ID of first device

 
const int relayPin1 = 2; // TODO: Change according to your board



// deviceId is the ID assgined to your smart-home-device in sinric.com dashboard. Copy it from dashboard and paste it here

void turnOn(String deviceId) {
  if (deviceId == DEVICE1)
  {  
    Serial.print("Turn on device id: ");
    Serial.println(deviceId);
    
     digitalWrite(relayPin1, LOW); // turn on relay with voltage HIGH
     delay(777);
     digitalWrite(relayPin1, HIGH); // turn on relay with voltage HIGH

  }
     
}

void turnOff(String deviceId) {
   if (deviceId == DEVICE1)
   {  
     Serial.print("Turn off Device ID: ");
     Serial.println(deviceId);
     digitalWrite(relayPin1, LOW); // turn on relay with voltage HIGH
     delay(777);
     digitalWrite(relayPin1, HIGH); // turn on relay with voltage HIGH
   }

}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      isConnected = false;    
      Serial.printf("[WSc] Webservice disconnected from sinric.com!\n");
      break;
    case WStype_CONNECTED: {
      isConnected = true;
      Serial.printf("[WSc] Service connected to sinric.com at url: %s\n", payload);
      Serial.printf("Waiting for commands from sinric.com ...\n");        
      }
      break;
    case WStype_TEXT: {
        Serial.printf("[WSc] get text: %s\n", payload);
        // Example payloads

        // For Switch or Light device types
        // {"deviceId": xxxx, "action": "setPowerState", value: "ON"} // https://developer.amazon.com/docs/device-apis/alexa-powercontroller.html

        // For Light device type
        // Look at the light example in github
          
#if ARDUINOJSON_VERSION_MAJOR == 5
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject((char*)payload);
#endif
#if ARDUINOJSON_VERSION_MAJOR == 6        
        DynamicJsonDocument json(1024);
        deserializeJson(json, (char*) payload);      
#endif        
        String deviceId = json ["deviceId"];     
        String action = json ["action"];
        
        if(action == "setPowerState") { // Switch or Light
            String value = json ["value"];
            if(value == "ON") {
                turnOn(deviceId);
            } else {
                turnOff(deviceId);
            }
        }
        else if (action == "test") {
            Serial.println("[WSc] received test command from sinric.com");
        }
      }
      break;
    case WStype_BIN:
      Serial.printf("[WSc] get binary length: %u\n", length);
      break;
  }


  
}

void setup() {

server.begin();
WiFi.config(ip, gateway, subnet,dns);
  
  Serial.begin(115200);
  
    digitalWrite(relayPin1, HIGH);  
  pinMode(relayPin1, OUTPUT);

  
  WiFiMulti.addAP(MySSID, MyWifiPassword);
  Serial.println();
  Serial.print("Connecting to Wifi: ");
  Serial.println(MySSID);  

  // Waiting for Wifi connect
  while(WiFiMulti.run() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  if(WiFiMulti.run() == WL_CONNECTED) {
    Serial.println("");
    Serial.print("WiFi connected. ");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }

  // server address, port and URL
  webSocket.begin("iot.sinric.com", 80, "/");

  // event handler
  webSocket.onEvent(webSocketEvent);
  webSocket.setAuthorization("apikey", MyApiKey);
  
  // try again every 5000ms if connection has failed
  webSocket.setReconnectInterval(5000);   // If you see 'class WebSocketsClient' has no member named 'setReconnectInterval' error update arduinoWebSockets

  //SMOOTHING

   timer.setInterval(300L, smoothing);
   timer.setInterval(400L, returnsmooth);
   timer.setInterval(600L, readalarm);
   timer.setInterval(150L, exchangevars);

  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    readings[thisReading] = 0;
  }

  //SMOOTHING OFF

}

void loop() {
  webSocket.loop();

//Serial.println("Test voltage function");
//Serial.println(value);
//Serial.println(EXCHANGE_SOURCE);
//delay(1000);*/

  
  if(isConnected) {
      uint64_t now = millis();
      
      // Send heartbeat in order to avoid disconnections during ISP resetting IPs over night. Thanks @MacSass
      if((now - heartbeatTimestamp) > HEARTBEAT_INTERVAL) {
          heartbeatTimestamp = now;
          webSocket.sendTXT("H");          
      }
  } 

    timer.run();

    //SMOOTH 

     


//SMOOTH OFF
}


void smoothing() {
  // subtract the last reading:
  total = total - readings[readIndex];
  // read from the sensor:
  readings[readIndex] = analogRead(A0);
  // add the reading to the total:
  total = total + readings[readIndex];
  // advance to the next position in the array:
  readIndex = readIndex + 1;

  // if we're at the end of the array...
  if (readIndex >= numReadings) {
    // ...wrap around to the beginning:
    readIndex = 0;
  }

  // calculate the average:
  average = total / numReadings;
  // send it to the computer as ASCII digits

    // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 3.2V):
  voltage = average * (3.2 / 1023.0);
  //Serial.println(voltage);

 
}

void returnsmooth() {
//read sensors or do stuff
Serial.println("Test P function");
Serial.println(voltage);


//If average alarm value is MORE than 0.19V, consider it ON
if (voltage >0.19)
{
alarm=true;

}
else {
//If average alarm value is LESS than 0.19V, consider it OFF
alarm=false;
yield();
}

Serial.print("\t\t\t\t");



}

void readalarm(){
   // read the state of the pushbutton value:
  buzzerstate = digitalRead(buzzerpin);
  /*Serial.println(buzzerstate);
  delay(50);*/
}

void exchangevars(){
  {
  if (voltage >0.19)
  {
  EXCHANGE_SOURCE=1;

  }
  else {
  EXCHANGE_SOURCE=0;

  }
}

   WiFiClient client = server.available();
  if (client) {
    if (client.connected()) {
      Serial.println("Sending data..");
      String request = client.readStringUntil('\r');    // reads the message from the client
      client.flush();
      client.print(String(EXCHANGE_SOURCE));        // sends variable 1
      client.print(',');
      client.println(String(buzzerstate));// sends variable 2
      client.println('\r');
    }
    client.stop();                         // disconnects the client 
  }
}
