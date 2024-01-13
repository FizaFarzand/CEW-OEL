// weather.c
#include "weather.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    FILE *file = (FILE *)userp;
    return fwrite(contents, size, nmemb, file);
}

void fetchWeatherData() {
    CURL *curl;
    CURLcode res;

    FILE *jsonFile = fopen(JSON_FILE_PATH, "w");
    if (!jsonFile) {
        fprintf(stderr, "Error opening file: %s\n", JSON_FILE_PATH);
        exit(EXIT_FAILURE);
    }

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, API_ENDPOINT);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, jsonFile);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

        curl_easy_cleanup(curl);
        fclose(jsonFile);
    }
}

WeatherData analyzeWeatherData(const char *json) {
    WeatherData data;
    json_t *root, *current, *temp_c, *humidity, *cloud, *uv;

    root = json_loads(json, 0, NULL);
    if (!root) {
        fprintf(stderr, "Error parsing JSON\n");
        exit(EXIT_FAILURE);
    }

    current = json_object_get(root, "current");
    if (!current) {
        fprintf(stderr, "Error: 'current' key not found in JSON\n");
        exit(EXIT_FAILURE);
    }

    temp_c = json_object_get(current, "temp_c");
    humidity = json_object_get(current, "humidity");
    cloud = json_object_get(current, "cloud");
    uv = json_object_get(current, "uv");

    data.temp_c = malloc(sizeof(double));
    if (temp_c && json_is_real(temp_c)) {
        *(data.temp_c) = json_real_value(temp_c);
    } else {
        // Set a default value for missing or invalid temperature data
        *(data.temp_c) = -1.0;  // You can choose another default value if needed
    }

    data.humidity = malloc(sizeof(double));
    if (humidity && json_is_real(humidity)) {
        *(data.humidity) = json_real_value(humidity);
    } else {
        // Set a default value for missing or invalid humidity data
        *(data.humidity) = 16.0;  // You can choose another default value if needed
    }

    data.cloud = malloc(sizeof(double));
    if (cloud && json_is_real(cloud)) {
        *(data.cloud) = json_real_value(cloud);
    } else {
        // Set a default value for missing or invalid cloud data
        *(data.cloud) = 100;  // You can choose another default value if needed
    }

    data.uv = malloc(sizeof(double));
    if (uv && json_is_real(uv)) {
        *(data.uv) = json_real_value(uv);
    } else {
        *(data.uv) = -1.0;
    }

    json_decref(root);
    return data;
}

const char *predictText(double averageTemperature) {
    if (averageTemperature > 30.0) {
        return "It's going to be a hot day!";
    } else if (averageTemperature > 20.0) {
        return "Expect a pleasant day.";
    } else {
        return "It might be a bit chilly.";
    }
}

void writeProcessedEnvironmentalData(const char *json) {
    FILE *processedDataFile = fopen(PROCESSED_DATA_FILE_PATH, "w");
    if (!processedDataFile) {
        fprintf(stderr, "Error opening file: %s\n", PROCESSED_DATA_FILE_PATH);
        exit(EXIT_FAILURE);
    }

    json_t *root = json_loads(json, 0, NULL);
    if (!root) {
        fprintf(stderr, "Error parsing JSON\n");
        exit(EXIT_FAILURE);
    }

    json_dumpf(root, processedDataFile, JSON_INDENT(2));

    json_decref(root);
    fclose(processedDataFile);
}

void sendSlackMessage(const char *message) {
    CURL *curl;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, SLACK_WEBHOOK_URL);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);

        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        json_t *payload = json_object();
        json_object_set_new(payload, "text", json_string(message));

        char *jsonStr = json_dumps(payload, 0);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonStr);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
        free(jsonStr);
        json_decref(payload);
    }

    curl_global_cleanup();
}

void writeAlertData(const char *prediction, double averageTemperature) {
    FILE *alertFile = fopen(ALERT_FILE_PATH, "w");
    if (!alertFile) {
        fprintf(stderr, "Error opening file: %s\n", ALERT_FILE_PATH);
        exit(EXIT_FAILURE);
    }

    json_t *root = json_object();
    json_object_set_new(root, "prediction", json_string(prediction));
    json_object_set_new(root, "averageTemperature", json_real(averageTemperature));

    char *jsonStr = json_dumps(root, JSON_INDENT(2));
    fprintf(alertFile, "%s\n", jsonStr);

    // Send alert to Slack
    sendSlackMessage(jsonStr);

    free(jsonStr);
    json_decref(root);
    fclose(alertFile);
}

void writeHumidityAnomalies(const WeatherData *data, size_t num_days) {
    FILE *humidityAnomaliesFile = fopen(HUMIDITY_ANOMALIES_FILE_PATH, "w");
    if (!humidityAnomaliesFile) {
        fprintf(stderr, "Error opening file: %s\n", HUMIDITY_ANOMALIES_FILE_PATH);
        exit(EXIT_FAILURE);
    }

    json_t *root = json_array();

    for (size_t i = 0; i < num_days; ++i) {
        json_t *entry = json_object();
        json_object_set_new(entry, "day", json_integer(i + 1));
        json_object_set_new(entry, "humidity", json_real(*(data[i].humidity)));
        json_object_set_new(entry, "humidity_anomaly", json_integer(*(data[i].humidity_anomaly)));
        json_object_set_new(entry, "cloud", json_real(*(data[i].cloud)));
        json_object_set_new(entry, "uv", json_real(*(data[i].uv)));

        json_array_append_new(root, entry);
    }

    json_dumpf(root, humidityAnomaliesFile, JSON_INDENT(2));

    json_decref(root);
    fclose(humidityAnomaliesFile);
}

void detectHumidityAnomalies(WeatherData *data, size_t num_days) {
    for (size_t i = 0; i < num_days; ++i) {
        data[i].humidity_anomaly = malloc(sizeof(int));
        if (*(data[i].humidity) > 80.0) {
            *(data[i].humidity_anomaly) = 1;
        } else {
            *(data[i].humidity_anomaly) = 0;
        }
    }
}

void generateReport(const WeatherData *data, size_t num_days, const char *prediction, double averageTemperature) {
    FILE *reportFile = fopen(REPORT_FILE_PATH, "w");
    if (!reportFile) {
        fprintf(stderr, "Error opening file for report\n");
        return;
    }

    fprintf(reportFile, "Environmental Report\n\n");
    fprintf(reportFile, "Summary:\n");
    fprintf(reportFile, "-----------------\n");
    fprintf(reportFile, "Average Temperature: %.2f\n", averageTemperature);
    fprintf(reportFile, "Prediction: %s\n", prediction);
    fprintf(reportFile, "\n");

    fprintf(reportFile, "Daily Data:\n");
    fprintf(reportFile, "-----------\n");

    for (size_t i = 0; i < num_days; ++i) {
        fprintf(reportFile, "Day %zu:\n", i + 1);
        fprintf(reportFile, "  Temperature: %.2f\n", *(data[i].temp_c));
        fprintf(reportFile, "  Humidity: %.2f\n", *(data[i].humidity));

        // Analyze and print Cloud conditions
        double cloudValue = *(data[i].cloud);
        if (cloudValue >= 0 && cloudValue <= 10) {
            fprintf(reportFile, "  Cloud: %.2f%% - Clear Sky (Less Cloudy)\n", cloudValue);
            fprintf(reportFile, "  Description: Mostly clear skies with very few or no clouds.\n");
            fprintf(reportFile, "  Likelihood of Rain: Low to none.\n");
        } else if (cloudValue > 10 && cloudValue <= 50) {
            fprintf(reportFile, "  Cloud: %.2f%% - Partly Cloudy (Moderate Cloudiness)\n", cloudValue);
            fprintf(reportFile, "  Description: Some clouds present, but with breaks of clear sky.\n");
            fprintf(reportFile, "  Likelihood of Rain: Low, but there may be a chance of brief showers.\n");
        } else if (cloudValue > 50 && cloudValue <= 80) {
            fprintf(reportFile, "  Cloud: %.2f%% - Mostly Cloudy (Considerable Cloudiness)\n", cloudValue);
            fprintf(reportFile, "  Description: A significant portion of the sky covered with clouds.\n");
            fprintf(reportFile, "  Likelihood of Rain: Moderate, with a higher chance of rain compared to partly cloudy conditions.\n");
        } else {
            fprintf(reportFile, "  Cloud: %.2f%% - Overcast (Cloudy)\n", cloudValue);
            fprintf(reportFile, "  Description: The sky is mostly or completely covered by clouds.\n");
            fprintf(reportFile, "  Likelihood of Rain: High, especially in overcast conditions.\n");
        }

        // Analyze and print UV conditions
        double uvValue = *(data[i].uv);
        fprintf(reportFile, "  UV: %.2f\n", uvValue);
        if (uvValue >= 0 && uvValue <= 2) {
            fprintf(reportFile, "  Description: Minimal risk of harm. Wear sunglasses on bright days; use sunscreen if there is snow on the ground, which reflects UV radiation.\n");
        } else if (uvValue > 2 && uvValue <= 5) {
            fprintf(reportFile, "  Description: Moderate risk of harm from unprotected sun exposure. Take precautions, such as wearing protective clothing, a hat, and sunglasses, and using sunscreen.\n");
        } else if (uvValue > 5 && uvValue <= 7) {
            fprintf(reportFile, "  Description: High risk of harm from unprotected sun exposure. Protection against skin and eye damage is needed. Reduce time in the sun during peak hours and take protective measures.\n");
        } else if (uvValue > 7 && uvValue <= 10) {
            fprintf(reportFile, "  Description: Very high risk of harm. Extra precautions are needed. Unprotected skin will be damaged and can burn quickly.\n");
        } else {
            fprintf(reportFile, "  Description: Extremely high risk of harm. Avoid the sun during peak hours, seek shade, and take all precautions to protect against sunburn.\n");
        }

        fprintf(reportFile, "  Humidity Anomaly: %s\n", *(data[i].humidity_anomaly) ? "Yes" : "No");
        fprintf(reportFile, "\n");
    }

    fclose(reportFile);

    printf("Report generated successfully: %s\n", REPORT_FILE_PATH);
}

