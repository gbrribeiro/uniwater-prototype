#include <ArduinoJson.h>
#include <SoftwareSerial.h>

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

#define UMIDITY_SENSOR A0

SoftwareSerial espSerial(10,11);

double umiditySensorLowestValue = 1024;
double umiditySensorHighestValue = 0;
double umidityInPercentage = 0;

SystemParameters parameters(0,100,100);
StreamData data(1,1);

void setup() {
  Serial.begin(9600);
  espSerial.begin(9600);
  setPins();
}

void loop() {
  //Read esp commands
  SystemParameters* espParameters = readEsp();

  if(espParameters != nullptr){
    parameters = *espParameters;
  }

  data.humidity = calibratedUmiditySensorValue(readUmiditySensor());

  //Send to esp
  sendEsp(data);
}

//=======================================================================================================================================================
//Set Pins
void setPins(){
  pinMode(UMIDITY_SENSOR, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
}
//=======================================================================================================================================================
//Umidity Sensor Management
double readUmiditySensor(){
  double result = analogRead(UMIDITY_SENSOR);

  if(result > umiditySensorHighestValue){
    umiditySensorHighestValue = result;
  }
  if(result < umiditySensorLowestValue){
    umiditySensorHighestValue = result;
  }

  return result;
}

double calibratedUmiditySensorValue(double raw){
  return map(raw, umiditySensorLowestValue, umiditySensorHighestValue , 0, 100);
}

//=======================================================================================================================================================
//Connection Management
SystemParameters* readEsp(){
  JsonDocument doc;

  String result = Serial.readString();

  if(result){
    DeserializationError error = deserializeJson(doc, result);
    if (error) {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
      return nullptr;
    }

  SystemParameters newParams(doc["iOn"],doc["iOff"],doc["dT"]);

  return &newParams;
  }

}

void sendEsp(StreamData data){
  JsonDocument doc;

  doc["humidity"] = data.humidity;
  doc["temperature"] = data.temperature;

  doc.shrinkToFit();  // optional

  serializeJson(doc, espSerial);
}