#include <ArduinoJson.h>
#include <SoftwareSerial.h>
#include "SystemParameters.h"
#include "StreamData.h"

#define UMIDITY_SENSOR A0
#define TEMP_SENSOR 7
#define FLUX_SENSOR A1
#define WATER_V_1 3
#define WATER_V_2 4


//Umidity Config
double umiditySensorLowestValue = 1024;
double umiditySensorHighestValue = 0;
double umidityInPercentage = 0;

//Flux Config
double fluxSensorLowestValue = 1024;
double fluxSensorHighestValue = 0;
double fluxInPercentage = 0;

SystemParameters parameters(0,70,30);
StreamData data(1,1,1);


void setup() {
  Serial.begin(9600);
  setPins();
}

void loop() {
  data.humidity = calibratedUmiditySensorValue(readUmiditySensor());
  data.flux = calibratedFluxSensorValue(readFluxSensor());

  openOrCloseWater();
  Serial.print("Actual val: ");
  Serial.println(analogRead(FLUX_SENSOR));
  Serial.println(data.flux);
  //Every time it receives parameters it will respond with the data
  String result = Serial.readString();
  if(result.startsWith("ESP: ")){
    digitalWrite(LED_BUILTIN, 1);
    delay(500);
    digitalWrite(LED_BUILTIN, 0);
    result.replace("ESP: ", "");
    dealWithEspResult(result);
    //Send response
    sendDataToEsp(data);
  }
}

//=======================================================================================================================================================
//Set Pins
void setPins(){
  pinMode(UMIDITY_SENSOR, INPUT);
  pinMode(FLUX_SENSOR, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(WATER_V_1, OUTPUT);
  pinMode(WATER_V_2, OUTPUT);
}
//=======================================================================================================================================================
//Umidity Sensor Management
double readUmiditySensor(){
  double result = analogRead(UMIDITY_SENSOR);

  if(result > umiditySensorHighestValue){
    umiditySensorHighestValue = result;
  }
  if(result < umiditySensorLowestValue){
    umiditySensorLowestValue = result;
  }

  return result;
}

double calibratedUmiditySensorValue(double raw){
  return map(raw, umiditySensorHighestValue, umiditySensorLowestValue , 0, 100);
}

//Flux Sensor Management
double readFluxSensor(){
  double result = analogRead(FLUX_SENSOR);

  if(result > fluxSensorHighestValue){
    fluxSensorHighestValue = result;
  }
  if(result < fluxSensorLowestValue){
    fluxSensorLowestValue = result;
  }

  return result;
}
double calibratedFluxSensorValue(double raw){
  return map(raw, fluxSensorHighestValue, fluxSensorLowestValue , 0, 100);
}

//Water Management
void openOrCloseWater(){
  if(data.humidity >= parameters.irrigationOffPercentage){
    turnOffWater();
  }
  else if(data.humidity <= parameters.irrigationOnPercentage){
    turnOnWater();
  }
}

void turnOffWater(){
  Serial.println("TURNING OFF WATER");
  digitalWrite(WATER_V_1, 1);
  digitalWrite(WATER_V_2, 1);
}
void turnOnWater(){
  Serial.println("TURNING ON WATER");
  digitalWrite(WATER_V_1, 1);
  digitalWrite(WATER_V_2, 0);

}

//=======================================================================================================================================================
//Connection Management
void dealWithEspResult(String result){
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, result);
  if (error) {
  }
  SystemParameters newParams(doc["humidityOnPercentage"],doc["humidityOffPercentage"],doc["dangerousTemperature"]);
  parameters = newParams;
}

void sendDataToEsp(StreamData data){
  JsonDocument doc;

  doc["humidity"] = data.humidity;
  doc["temperature"] = data.temperature;

  doc.shrinkToFit(); 

  String sendingString;
  serializeJson(doc, sendingString);
  sendingString = "ATMEGA: " + sendingString;
  Serial.println(sendingString.c_str());
}