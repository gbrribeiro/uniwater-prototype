#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include "SystemParameters.h"
#include "StreamData.h"

#include <DNSServer.h>
#include <WiFiManager.h>

#ifndef STASSID
#define STASSID "NTBR"
#define STAPSK "12345678"
#endif

String apiUrl = "https://15.228.58.104:443";
//String apiUrl = "https://192.168.2.237:443";
String apiKey;
// const char* ssid = STASSID;
// const char* password = STAPSK;

SystemParameters parameters(20,80,100);
StreamData data(0,0);
ESP8266WiFiMulti WiFiMulti;


void setup(){

  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  setupWifi();
  authenticate();
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
    delay(4000);
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
  WiFiManager wifiManager;
  wifiManager.setConfigPortalTimeout(240);

  //Cria um AP (Access Point) com: ("nome da rede", "senha da rede")
  if (!wifiManager.autoConnect("Arduino", "arduinowifi")) {
    Serial.println(F("Falha na conexao. Resetar e tentar novamente..."));
    delay(3000);
    ESP.restart();
    delay(5000);
  }
  //Mensagem caso conexao Ok
  Serial.println(F("Conectado na rede Wifi."));
  Serial.print(F("Endereco IP: "));
  Serial.println(WiFi.localIP());
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
  http.addHeader("Authorization", key);
  int responseCode = http.GET();
  if(responseCode == 401){
    authenticate();
    return fetchFromApi();
  }

  // Deserialize the response
  JsonDocument doc;
  deserializeJson(doc, http.getString());

  // Disconnect
  http.end();
  SystemParameters params(doc["humidityOnPercentage"], doc["humidityOffPercentage"], doc["dangerousTemperature"]);
  return params;
}

void authenticate(){
  WiFiClientSecure client;
  HTTPClient http;
  client.setInsecure(); 

  // Send request
  String newUrl = apiUrl + "/api/v1/Identity/Login";

  JsonDocument doc;
  doc["username"] = "SUPERADMIN@ADMIN.com";
  doc["password"] = "Admin@2024";
  doc.shrinkToFit();  // optional

  String json;
  serializeJson(doc, json);

  http.begin(client, newUrl);
  http.addHeader("accept", "text/plain");
  http.addHeader("Content-Type", "application/json");
 
  int resp = http.POST(json);

  apiKey = http.getString();

  http.end();
}

void sendToApi(){
  WiFiClientSecure client;
  HTTPClient http;
  client.setInsecure(); 

  // Send request
  String newUrl = apiUrl + "/api/v1/StreamingData";
  http.begin(client, newUrl);

  String key = "Bearer " + apiKey;
  http.addHeader("Authorization", key);

  http.addHeader("Content-Type", "application/json");

  // Serialize the request
  StaticJsonDocument<96> doc;

  doc["id"] = 1;
  doc["humidity"] = data.humidity;
  doc["temperature"] = data.temperature;
  doc["internalClock"] = "2024-09-17T20:03:08.726Z";

  String json;
  serializeJson(doc, json);
  int statusCode = http.POST(json);
  // Read response
  if(statusCode == 401){
    authenticate();
    // sendToApi();
  }

  // Disconnect
  http.end();
}