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