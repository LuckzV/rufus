/*
 * Rufus: The Reliable USB Formatting Utility
 * USB Health Prediction System
 * Copyright © 2011-2025 Pete Batard <pete@akeo.ie>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "rufus.h"

// USB Health Prediction Constants
#define HEALTH_DATA_POINTS_MAX    1000
#define HEALTH_PREDICTION_WINDOW  30      // days
#define HEALTH_CRITICAL_THRESHOLD 0.8f    // 80% failure probability
#define HEALTH_WARNING_THRESHOLD  0.6f    // 60% failure probability

// Health prediction algorithms
typedef enum {
    HEALTH_ALGO_NEURAL_NETWORK = 0,
    HEALTH_ALGO_RANDOM_FOREST,
    HEALTH_ALGO_SVM,
    HEALTH_ALGO_BAYESIAN,
    HEALTH_ALGO_MAX
} health_algorithm_t;

// USB drive health metrics
typedef struct {
    uint64_t total_writes;
    uint64_t total_reads;
    uint64_t error_count;
    uint64_t retry_count;
    uint64_t bad_sectors;
    float write_speed_avg;
    float read_speed_avg;
    float temperature_avg;
    uint32_t power_cycles;
    uint32_t hours_used;
    uint64_t timestamp;
} usb_health_metrics_t;

// Health prediction result
typedef struct {
    float failure_probability;
    uint32_t days_remaining;
    health_algorithm_t algorithm_used;
    char recommendation[256];
    BOOL is_critical;
    BOOL is_warning;
} usb_health_prediction_t;

// Health prediction context
typedef struct {
    usb_health_metrics_t metrics[HEALTH_DATA_POINTS_MAX];
    uint32_t metrics_count;
    uint64_t drive_serial;
    char drive_model[64];
    char drive_manufacturer[32];
    uint64_t first_seen;
    uint64_t last_updated;
} usb_health_context_t;

// Function prototypes
BOOL InitUSBHealthPredictor(void);
void CleanupUSBHealthPredictor(void);
BOOL RecordUSBHealthMetrics(const char* drive_path, usb_health_metrics_t* metrics);
BOOL PredictUSBHealth(const char* drive_path, usb_health_prediction_t* prediction);
BOOL LoadUSBHealthData(const char* drive_path, usb_health_context_t* context);
BOOL SaveUSBHealthData(const char* drive_path, usb_health_context_t* context);
BOOL TrainHealthModel(health_algorithm_t algorithm);
BOOL IsDriveHealthy(const char* drive_path);
BOOL GetDriveHealthRecommendation(const char* drive_path, char* recommendation, size_t max_len);

// Neural network functions for health prediction
typedef struct {
    float weights[64][64];
    float biases[64];
    int layers;
    int neurons_per_layer[8];
} neural_network_t;

BOOL InitNeuralNetwork(neural_network_t* nn);
float PredictWithNeuralNetwork(neural_network_t* nn, usb_health_metrics_t* metrics);
BOOL TrainNeuralNetwork(neural_network_t* nn, usb_health_metrics_t* training_data, int data_count);

// Utility functions
float CalculateHealthScore(usb_health_metrics_t* metrics);
BOOL IsDriveFailing(usb_health_metrics_t* metrics);
uint32_t EstimateDaysRemaining(usb_health_metrics_t* metrics);
char* GetHealthStatusString(float failure_probability);
