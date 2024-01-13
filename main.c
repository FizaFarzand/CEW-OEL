// main.c
#include "weather.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    fetchWeatherData();

    FILE *jsonFile = fopen(JSON_FILE_PATH, "r");
    if (!jsonFile) {
        fprintf(stderr, "Error opening file: %s\n", JSON_FILE_PATH);
        return 1;
    }

    fseek(jsonFile, 0, SEEK_END);
    long fileSize = ftell(jsonFile);
    fseek(jsonFile, 0, SEEK_SET);

    char *jsonBuffer = (char *)malloc(fileSize + 1);
    fread(jsonBuffer, 1, fileSize, jsonFile);
    fclose(jsonFile);

    jsonBuffer[fileSize] = '\0';

    WeatherData day1 = analyzeWeatherData(jsonBuffer);
    WeatherData day2 = analyzeWeatherData(jsonBuffer);

    size_t num_days = 2;
    WeatherData data[num_days];
    data[0] = day1;
    data[1] = day2;

    detectHumidityAnomalies(data, num_days);

    double averageTemperature = (*(day1.temp_c) + *(day2.temp_c)) / 2.0;
    const char *prediction = predictText(averageTemperature);

    writeProcessedEnvironmentalData(jsonBuffer);
    writeAlertData(prediction, averageTemperature);
    writeHumidityAnomalies(data, num_days);

    generateReport(data, num_days, prediction, averageTemperature);

    for (size_t i = 0; i < num_days; ++i) {
        free(data[i].temp_c);
        free(data[i].humidity);
        free(data[i].humidity_anomaly);
        free(data[i].cloud);
        free(data[i].uv);
    }

    free(jsonBuffer);

    return 0;
}

