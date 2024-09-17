#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <fix_fft.h>

class SystemParameters {
  public:             
    double irrigationOnPercentage;
    double irrigationOffPercentage;

    double dangerousTemperature;

    SystemParameters(double iOn, double iOff, double dT){
      irrigationOffPercentage = iOff;
      irrigationOnPercentage = iOn;
      dangerousTemperature = dT;
    }

};

class StreamData {
  public:
  double humidity;
  double temperature;

  StreamData(double h, double t){
    humidity = h;
    temperature = t;
  }
};

#ifndef STASSID
#define STASSID "Desativada"
#define STAPSK "gabrielnr"
#endif

String apiUrl = "https";
String apiKey = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJSb2xlIjoiQWRtaW4iLCJuYmYiOjE3MjY1Nzk5ODksImV4cCI6MTc1ODEzNjkxNSwiaXNzIjoiaHR0cDovL2xvY2FsaG9zdCIsImF1ZCI6IkF1ZGllbmNlIn0.Jj7x3YeMczvOCD2ypJ9poeFMmCGllSl-GW59ov6ZG88";
const char* ssid = STASSID;
const char* password = STAPSK;

// SystemParameters parameters(0,0,0);


void setup(){

  Serial.begin(9600);
  Serial1.begin(9600);

  setupWifi();

  
}

void loop(){
  HTTPClient http;
  WiFiClient client;

 
  //Buscar no banco
  SystemParameters parameters = fetchFromApi(http, client);
  //Mandar o json para o Arduino
  sendMC(parameters);
  //Aguardar uma resposta
  //Mandar a resposta para o servidor
  delay(3000);
}

//=======================================================================================================================================================
//MC and ESP Connection Handlers

void sendMC(SystemParameters parameters){
  JsonDocument doc;

  doc["iOn"] = parameters.irrigationOnPercentage;
  doc["iOff"] = parameters.irrigationOffPercentage;
  doc["dT"] = parameters.dangerousTemperature;

  doc.shrinkToFit();  // optional
  serializeJson(doc, Serial);
}

SystemParameters* readMC(){
  JsonDocument doc;

  String result = Serial1.readString();

  if(result){
    DeserializationError error = deserializeJson(doc, result);
    if (error) {
      Serial1.print("deserializeJson() failed: ");
      Serial1.println(error.c_str());
      return nullptr;
    }
    SystemParameters newParams(doc["iOn"],doc["iOff"],doc["dT"]);
    return &newParams;
  }
  return nullptr;
}

//=======================================================================================================================================================
//HTTP Setup
void setupWifi(){
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  //IPAddress myIp = WiFi.localIP();
}
//=======================================================================================================================================================
//API Management
SystemParameters fetchFromApi(HTTPClient http, WiFiClient client){
  // Send request
  String url = apiUrl + "/api/v1/System";
  http.begin(client, url);
  http.GET();

  // Deserialize the response
  JsonDocument doc;
  deserializeJson(doc, http.getString());

  // Disconnect
  http.end();
  SystemParameters params(doc["humidityOnPercentage"], doc["humidityOffPercentage"], doc["dangerousTemperature"]);
  return params;
}