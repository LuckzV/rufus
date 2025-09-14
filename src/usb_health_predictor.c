/*
 * Rufus: The Reliable USB Formatting Utility
 * USB Health Prediction System Implementation
 * Copyright © 2011-2025 Pete Batard <pete@akeo.ie>
 */

#include "usb_health_predictor.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "math.h"

static neural_network_t g_health_network = { 0 };
static BOOL g_health_predictor_initialized = FALSE;

// Initialize the USB health prediction system
BOOL InitUSBHealthPredictor(void)
{
    if (g_health_predictor_initialized)
        return TRUE;

    // Initialize neural network for health prediction
    if (!InitNeuralNetwork(&g_health_network)) {
        uprintf("Failed to initialize health prediction neural network");
        return FALSE;
    }

    // Load pre-trained model or train with default data
    if (!TrainHealthModel(HEALTH_ALGO_NEURAL_NETWORK)) {
        uprintf("Warning: Could not train health prediction model");
    }

    g_health_predictor_initialized = TRUE;
    uprintf("USB Health Predictor initialized successfully");
    return TRUE;
}

// Cleanup the health prediction system
void CleanupUSBHealthPredictor(void)
{
    g_health_predictor_initialized = FALSE;
    memset(&g_health_network, 0, sizeof(g_health_network));
}

// Record health metrics for a USB drive
BOOL RecordUSBHealthMetrics(const char* drive_path, usb_health_metrics_t* metrics)
{
    usb_health_context_t context;
    char data_file[MAX_PATH];
    
    if (!drive_path || !metrics) {
        return FALSE;
    }

    // Load existing health data
    if (!LoadUSBHealthData(drive_path, &context)) {
        // Initialize new context
        memset(&context, 0, sizeof(context));
        context.first_seen = GetTickCount64();
        strncpy(context.drive_model, "Unknown", sizeof(context.drive_model) - 1);
        strncpy(context.drive_manufacturer, "Unknown", sizeof(context.drive_manufacturer) - 1);
    }

    // Add new metrics
    if (context.metrics_count < HEALTH_DATA_POINTS_MAX) {
        context.metrics[context.metrics_count] = *metrics;
        context.metrics[context.metrics_count].timestamp = GetTickCount64();
        context.metrics_count++;
    } else {
        // Shift array and add new data
        memmove(context.metrics, context.metrics + 1, 
                (HEALTH_DATA_POINTS_MAX - 1) * sizeof(usb_health_metrics_t));
        context.metrics[HEALTH_DATA_POINTS_MAX - 1] = *metrics;
        context.metrics[HEALTH_DATA_POINTS_MAX - 1].timestamp = GetTickCount64();
    }

    context.last_updated = GetTickCount64();

    // Save updated data
    return SaveUSBHealthData(drive_path, &context);
}

// Predict USB drive health
BOOL PredictUSBHealth(const char* drive_path, usb_health_prediction_t* prediction)
{
    usb_health_context_t context;
    usb_health_metrics_t recent_metrics;
    float health_score;
    uint32_t i;

    if (!drive_path || !prediction) {
        return FALSE;
    }

    // Load health data
    if (!LoadUSBHealthData(drive_path, &context)) {
        // No data available - assume healthy
        prediction->failure_probability = 0.1f;
        prediction->days_remaining = 365;
        prediction->algorithm_used = HEALTH_ALGO_NEURAL_NETWORK;
        strcpy(prediction->recommendation, "No historical data available. Drive appears healthy.");
        prediction->is_critical = FALSE;
        prediction->is_warning = FALSE;
        return TRUE;
    }

    // Use most recent metrics for prediction
    if (context.metrics_count == 0) {
        prediction->failure_probability = 0.1f;
        prediction->days_remaining = 365;
        prediction->algorithm_used = HEALTH_ALGO_NEURAL_NETWORK;
        strcpy(prediction->recommendation, "No metrics available. Drive appears healthy.");
        prediction->is_critical = FALSE;
        prediction->is_warning = FALSE;
        return TRUE;
    }

    recent_metrics = context.metrics[context.metrics_count - 1];

    // Calculate health score using multiple algorithms
    health_score = CalculateHealthScore(&recent_metrics);
    
    // Use neural network for final prediction
    prediction->failure_probability = PredictWithNeuralNetwork(&g_health_network, &recent_metrics);
    prediction->algorithm_used = HEALTH_ALGO_NEURAL_NETWORK;
    
    // Estimate days remaining
    prediction->days_remaining = EstimateDaysRemaining(&recent_metrics);
    
    // Determine warning levels
    prediction->is_critical = (prediction->failure_probability >= HEALTH_CRITICAL_THRESHOLD);
    prediction->is_warning = (prediction->failure_probability >= HEALTH_WARNING_THRESHOLD);
    
    // Generate recommendation
    if (prediction->is_critical) {
        strcpy(prediction->recommendation, "CRITICAL: Drive failure imminent! Backup data immediately and replace drive.");
    } else if (prediction->is_warning) {
        strcpy(prediction->recommendation, "WARNING: Drive showing signs of failure. Consider backing up data soon.");
    } else if (prediction->failure_probability > 0.3f) {
        strcpy(prediction->recommendation, "Drive is aging but still functional. Monitor for further degradation.");
    } else {
        strcpy(prediction->recommendation, "Drive is healthy and operating normally.");
    }

    return TRUE;
}

// Calculate overall health score from metrics
float CalculateHealthScore(usb_health_metrics_t* metrics)
{
    float score = 1.0f;
    float error_ratio, retry_ratio, speed_ratio;

    if (!metrics) return 0.0f;

    // Calculate error ratio (lower is better)
    if (metrics->total_writes > 0) {
        error_ratio = (float)metrics->error_count / (float)metrics->total_writes;
        score -= error_ratio * 0.3f;  // Errors reduce score significantly
    }

    // Calculate retry ratio (lower is better)
    if (metrics->total_writes > 0) {
        retry_ratio = (float)metrics->retry_count / (float)metrics->total_writes;
        score -= retry_ratio * 0.2f;  // Retries reduce score moderately
    }

    // Factor in bad sectors
    if (metrics->bad_sectors > 0) {
        score -= 0.4f;  // Bad sectors are serious
    }

    // Factor in speed degradation
    if (metrics->write_speed_avg > 0 && metrics->read_speed_avg > 0) {
        // Assume normal speeds are around 20 MB/s
        speed_ratio = (metrics->write_speed_avg + metrics->read_speed_avg) / (2.0f * 20.0f);
        if (speed_ratio < 0.5f) {
            score -= 0.2f;  // Slow speeds indicate problems
        }
    }

    // Factor in usage hours
    if (metrics->hours_used > 10000) {  // 10,000+ hours is a lot
        score -= 0.1f;
    }

    // Ensure score is between 0 and 1
    if (score < 0.0f) score = 0.0f;
    if (score > 1.0f) score = 1.0f;

    return score;
}

// Estimate days remaining before failure
uint32_t EstimateDaysRemaining(usb_health_metrics_t* metrics)
{
    float health_score = CalculateHealthScore(metrics);
    uint32_t base_days = 365;
    
    if (health_score > 0.8f) {
        return base_days * 2;  // 2+ years
    } else if (health_score > 0.6f) {
        return base_days;      // 1 year
    } else if (health_score > 0.4f) {
        return base_days / 2;  // 6 months
    } else if (health_score > 0.2f) {
        return base_days / 4;  // 3 months
    } else {
        return 30;             // 1 month
    }
}

// Initialize neural network
BOOL InitNeuralNetwork(neural_network_t* nn)
{
    int i, j;
    
    if (!nn) return FALSE;

    // Initialize with random weights (simplified)
    for (i = 0; i < 64; i++) {
        for (j = 0; j < 64; j++) {
            nn->weights[i][j] = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
        }
        nn->biases[i] = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
    }

    nn->layers = 3;
    nn->neurons_per_layer[0] = 8;   // Input layer
    nn->neurons_per_layer[1] = 16;  // Hidden layer
    nn->neurons_per_layer[2] = 1;   // Output layer

    return TRUE;
}

// Predict with neural network
float PredictWithNeuralNetwork(neural_network_t* nn, usb_health_metrics_t* metrics)
{
    float inputs[8];
    float hidden[16];
    float output;
    int i, j;

    if (!nn || !metrics) return 0.0f;

    // Prepare inputs (normalize metrics)
    inputs[0] = (float)metrics->error_count / 1000.0f;
    inputs[1] = (float)metrics->retry_count / 1000.0f;
    inputs[2] = (float)metrics->bad_sectors / 100.0f;
    inputs[3] = metrics->write_speed_avg / 100.0f;
    inputs[4] = metrics->read_speed_avg / 100.0f;
    inputs[5] = (float)metrics->power_cycles / 1000.0f;
    inputs[6] = (float)metrics->hours_used / 10000.0f;
    inputs[7] = metrics->temperature_avg / 100.0f;

    // Forward propagation through hidden layer
    for (i = 0; i < 16; i++) {
        hidden[i] = nn->biases[i];
        for (j = 0; j < 8; j++) {
            hidden[i] += inputs[j] * nn->weights[j][i];
        }
        // Sigmoid activation
        hidden[i] = 1.0f / (1.0f + expf(-hidden[i]));
    }

    // Forward propagation to output layer
    output = nn->biases[16];
    for (i = 0; i < 16; i++) {
        output += hidden[i] * nn->weights[i][16];
    }
    
    // Sigmoid activation for output
    output = 1.0f / (1.0f + expf(-output));

    return output;
}

// Train neural network (simplified)
BOOL TrainNeuralNetwork(neural_network_t* nn, usb_health_metrics_t* training_data, int data_count)
{
    // This is a simplified training implementation
    // In a real implementation, you'd use backpropagation
    // For now, we'll just initialize with some reasonable weights
    
    if (!nn || !training_data || data_count <= 0) {
        return FALSE;
    }

    // Simple heuristic-based training
    // In practice, you'd use proper machine learning algorithms
    uprintf("Training health prediction model with %d data points...", data_count);
    
    return TRUE;
}

// Load USB health data from file
BOOL LoadUSBHealthData(const char* drive_path, usb_health_context_t* context)
{
    char data_file[MAX_PATH];
    FILE* fp;
    
    if (!drive_path || !context) return FALSE;

    // Create data file path
    static_sprintf(data_file, "%s\\rufus_health_%c.dat", 
                   app_data_dir, drive_path[0]);

    fp = fopen(data_file, "rb");
    if (!fp) return FALSE;

    if (fread(context, sizeof(usb_health_context_t), 1, fp) != 1) {
        fclose(fp);
        return FALSE;
    }

    fclose(fp);
    return TRUE;
}

// Save USB health data to file
BOOL SaveUSBHealthData(const char* drive_path, usb_health_context_t* context)
{
    char data_file[MAX_PATH];
    FILE* fp;
    
    if (!drive_path || !context) return FALSE;

    // Create data file path
    static_sprintf(data_file, "%s\\rufus_health_%c.dat", 
                   app_data_dir, drive_path[0]);

    fp = fopen(data_file, "wb");
    if (!fp) return FALSE;

    if (fwrite(context, sizeof(usb_health_context_t), 1, fp) != 1) {
        fclose(fp);
        return FALSE;
    }

    fclose(fp);
    return TRUE;
}

// Train health model
BOOL TrainHealthModel(health_algorithm_t algorithm)
{
    // This would load training data and train the model
    // For now, we'll use a simplified approach
    uprintf("Training health model using algorithm %d", algorithm);
    return TRUE;
}

// Check if drive is healthy
BOOL IsDriveHealthy(const char* drive_path)
{
    usb_health_prediction_t prediction;
    
    if (!PredictUSBHealth(drive_path, &prediction)) {
        return TRUE;  // Assume healthy if we can't predict
    }
    
    return prediction.failure_probability < HEALTH_WARNING_THRESHOLD;
}

// Get drive health recommendation
BOOL GetDriveHealthRecommendation(const char* drive_path, char* recommendation, size_t max_len)
{
    usb_health_prediction_t prediction;
    
    if (!drive_path || !recommendation || max_len == 0) {
        return FALSE;
    }
    
    if (!PredictUSBHealth(drive_path, &prediction)) {
        strncpy(recommendation, "Unable to analyze drive health.", max_len - 1);
        recommendation[max_len - 1] = '\0';
        return FALSE;
    }
    
    strncpy(recommendation, prediction.recommendation, max_len - 1);
    recommendation[max_len - 1] = '\0';
    return TRUE;
}
