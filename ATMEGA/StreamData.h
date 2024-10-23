class StreamData {
  public:
  double humidity;
  double temperature;
  double flux;

  StreamData(double h, double t, double f){
    humidity = h;
    temperature = t;
    flux = f;
  }
};