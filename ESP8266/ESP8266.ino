#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include "SystemParameters.h"
#include "StreamData.h"

#ifndef STASSID
#define STASSID "NTBR"
#define STAPSK "12345678"
#endif

String apiUrl = "https://192.168.2.237:81";
String apiKey = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJSb2xlIjoiQWRtaW4iLCJuYmYiOjE3MjY1Nzk5ODksImV4cCI6MTc1ODEzNjkxNSwiaXNzIjoiaHR0cDovL2xvY2FsaG9zdCIsImF1ZCI6IkF1ZGllbmNlIn0.Jj7x3YeMczvOCD2ypJ9poeFMmCGllSl-GW59ov6ZG88";
const char* ssid = STASSID;
const char* password = STAPSK;

SystemParameters parameters(20,80,100);
StreamData data(0,0);
ESP8266WiFiMulti WiFiMulti;


void setup(){

  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  setupWifi();
}

void loop(){
  if(WiFiMulti.run() != WL_CONNECTED){
    Serial.write(".");
    delay(500);
  }
  else {
    //Gets from API, sends to MC than updates the StreamData
    parameters = fetchFromApi();
    sendParametersToMC();
    delay(100);
    readMCResponse();
    sendToApi();
    //Gets the API parameters
    //Send parameters to Arduino MC
    delay(1000);
  }

}

//=======================================================================================================================================================
//MC and ESP Connection Handlers
void sendParametersToMC(){
  JsonDocument doc;
  doc["humidityOnPercentage"] = parameters.irrigationOnPercentage;
  doc["humidityOffPercentage"] = parameters.irrigationOffPercentage;
  doc["dangerousTemperature"] = parameters.dangerousTemperature;

  String sendingMessage;
  doc.shrinkToFit();  // optional
  serializeJson(doc, sendingMessage);
  sendingMessage = "ESP: " + sendingMessage;
  Serial.write(sendingMessage.c_str());
}

void readMCResponse(){
  JsonDocument doc;
  String result = Serial.readString();
  if(result.startsWith("ATMEGA: ")){
    result.replace("ATMEGA: ", "");
  digitalWrite(LED_BUILTIN, HIGH);
  DeserializationError error = deserializeJson(doc, result);
  StreamData newData(doc["humidity"],doc["temperature"]);
  data = newData;
  }
}

//=======================================================================================================================================================
//HTTP Setup
void setupWifi(){
  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(ssid, password);
}

//=======================================================================================================================================================
//API Management
SystemParameters fetchFromApi(){
  WiFiClientSecure client;
  HTTPClient http;
  client.setInsecure(); 

  // Send request
  String newUrl = apiUrl + "/api/v1/System";
  http.begin(client, newUrl);
  String key = "Bearer " + apiKey;
  http.setAuthorization(key);
  http.GET();

  // Deserialize the response
  JsonDocument doc;
  deserializeJson(doc, http.getString());

  // Disconnect
  http.end();
  SystemParameters params(doc["humidityOnPercentage"], doc["humidityOffPercentage"], doc["dangerousTemperature"]);
  return params;
}

void sendToApi(){
  WiFiClientSecure client;
  HTTPClient http;
  client.setInsecure(); 

  // Send request
  String newUrl = apiUrl + "/api/v1/StreamingData";
  http.begin(client, newUrl);

  String key = "Bearer " + apiKey;
  http.setAuthorization(key);

  http.addHeader("Content-Type", "application/json");

  // Serialize the request
  StaticJsonDocument<96> doc;

  doc["id"] = 1;
  doc["humidity"] = data.humidity;
  doc["temperature"] = data.temperature;
  doc["internalClock"] = "2024-09-17T20:03:08.726Z";

  String json;
  serializeJson(doc, json);
  http.POST(json);
  // Read response
  //Serial.write(http.getString());

  // Disconnect
  http.end();
}