// weather.h
#ifndef WEATHER_H
#define WEATHER_H

#include <jansson.h>

#define API_ENDPOINT "http://api.weatherapi.com/v1/current.json?key=8fc7934ad66345579d1105720233112&q=murree&aqi=no"
#define JSON_FILE_PATH "/home/maryam/Desktop/weather_response.json"
#define PROCESSED_DATA_FILE_PATH "/home/maryam/Desktop/processed_environmental_data.json"
#define ALERT_FILE_PATH "/home/maryam/Desktop/alert.json"
#define HUMIDITY_ANOMALIES_FILE_PATH "/home/maryam/Desktop/humidity_anomalies.json"
#define REPORT_FILE_PATH "/home/maryam/Desktop/environmental_report.txt"

typedef struct {
    double *temp_c;
    double *humidity;
    int *humidity_anomaly;
    double *cloud;
    double *uv;
} WeatherData;

void fetchWeatherData();
WeatherData analyzeWeatherData(const char *json);
const char *predictText(double averageTemperature);
void writeProcessedEnvironmentalData(const char *json);
void writeAlertData(const char *prediction, double averageTemperature);
void writeHumidityAnomalies(const WeatherData *data, size_t num_days);
void detectHumidityAnomalies(WeatherData *data, size_t num_days);
void generateReport(const WeatherData *data, size_t num_days, const char *prediction, double averageTemperature);

#endif  // WEATHER_H

