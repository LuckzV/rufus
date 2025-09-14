/*
 * Rufus: The Reliable USB Formatting Utility
 * Real-Time Drive Monitoring System Implementation
 * Copyright © 2011-2025 Pete Batard <pete@akeo.ie>
 */

#include "realtime_monitor.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "time.h"

static monitor_context_t g_monitor_context = { 0 };
static monitor_config_t g_monitor_config = { 0 };
static BOOL g_monitor_initialized = FALSE;

// Initialize real-time monitoring
BOOL InitRealTimeMonitor(const monitor_config_t* config)
{
    if (g_monitor_initialized) {
        return TRUE;
    }

    if (!config) {
        // Use default configuration
        memset(&g_monitor_config, 0, sizeof(g_monitor_config));
        g_monitor_config.enable_temperature_monitoring = TRUE;
        g_monitor_config.enable_speed_monitoring = TRUE;
        g_monitor_config.enable_error_monitoring = TRUE;
        g_monitor_config.enable_power_monitoring = TRUE;
        g_monitor_config.enable_vibration_monitoring = FALSE;
        g_monitor_config.enable_em_monitoring = FALSE;
        g_monitor_config.enable_capacity_monitoring = TRUE;
        g_monitor_config.enable_sector_monitoring = TRUE;
        g_monitor_config.update_interval = MONITOR_UPDATE_INTERVAL;
        g_monitor_config.warning_threshold = MONITOR_ALERT_THRESHOLD;
        g_monitor_config.critical_threshold = MONITOR_CRITICAL_THRESHOLD;
        g_monitor_config.auto_alert = TRUE;
        g_monitor_config.log_to_file = FALSE;
        strcpy(g_monitor_config.log_file_path, "rufus_monitor.log");
    } else {
        memcpy(&g_monitor_config, config, sizeof(monitor_config_t));
    }

    // Initialize context
    memset(&g_monitor_context, 0, sizeof(g_monitor_context));
    g_monitor_context.is_monitoring = FALSE;
    g_monitor_context.start_time = GetTickCount64();

    // Create stop event
    g_monitor_context.stop_event = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!g_monitor_context.stop_event) {
        uprintf("Failed to create monitoring stop event");
        return FALSE;
    }

    g_monitor_initialized = TRUE;
    uprintf("Real-time monitoring system initialized");
    return TRUE;
}

// Cleanup real-time monitoring
void CleanupRealTimeMonitor(void)
{
    if (g_monitor_initialized) {
        // Stop monitoring
        StopDriveMonitoring(NULL);  // Stop all drives

        // Close stop event
        if (g_monitor_context.stop_event) {
            CloseHandle(g_monitor_context.stop_event);
            g_monitor_context.stop_event = NULL;
        }

        // Clear context
        memset(&g_monitor_context, 0, sizeof(g_monitor_context));
        g_monitor_initialized = FALSE;
    }
}

// Start drive monitoring
BOOL StartDriveMonitoring(const char* drive_path)
{
    uint32_t drive_index;
    drive_monitor_data_t* drive_data;

    if (!g_monitor_initialized) {
        return FALSE;
    }

    if (!drive_path) {
        // Start monitoring all drives
        for (uint32_t i = 0; i < g_monitor_context.drive_count; i++) {
            g_monitor_context.drives[i].is_monitoring = TRUE;
        }
        return TRUE;
    }

    // Check if drive is already being monitored
    if (IsDriveBeingMonitored(drive_path)) {
        return TRUE;
    }

    // Find available slot
    if (g_monitor_context.drive_count >= MONITOR_MAX_DRIVES) {
        uprintf("Maximum number of monitored drives reached");
        return FALSE;
    }

    drive_index = g_monitor_context.drive_count;
    drive_data = &g_monitor_context.drives[drive_index];

    // Initialize drive data
    memset(drive_data, 0, sizeof(drive_monitor_data_t));
    strncpy(drive_data->drive_path, drive_path, sizeof(drive_data->drive_path) - 1);
    strncpy(drive_data->drive_name, drive_path, sizeof(drive_data->drive_name) - 1);
    drive_data->is_monitoring = TRUE;
    drive_data->is_healthy = TRUE;
    drive_data->last_update = GetTickCount64();

    // Initialize metrics
    for (int i = 0; i < MONITOR_METRIC_MAX; i++) {
        drive_data->current_metrics[i] = 0.0f;
        drive_data->average_metrics[i] = 0.0f;
        drive_data->max_metrics[i] = 0.0f;
        drive_data->min_metrics[i] = 1000.0f;
    }

    g_monitor_context.drive_count++;

    // Start monitoring thread if not already running
    if (!g_monitor_context.is_monitoring) {
        g_monitor_context.is_monitoring = TRUE;
        g_monitor_context.monitor_thread = CreateThread(NULL, 0, MonitorThreadProc, NULL, 0, NULL);
        if (!g_monitor_context.monitor_thread) {
            uprintf("Failed to create monitoring thread");
            g_monitor_context.is_monitoring = FALSE;
            return FALSE;
        }
    }

    uprintf("Started monitoring drive %s", drive_path);
    return TRUE;
}

// Stop drive monitoring
BOOL StopDriveMonitoring(const char* drive_path)
{
    if (!g_monitor_initialized) {
        return FALSE;
    }

    if (!drive_path) {
        // Stop monitoring all drives
        for (uint32_t i = 0; i < g_monitor_context.drive_count; i++) {
            g_monitor_context.drives[i].is_monitoring = FALSE;
        }

        // Stop monitoring thread
        if (g_monitor_context.is_monitoring) {
            g_monitor_context.is_monitoring = FALSE;
            SetEvent(g_monitor_context.stop_event);
            
            if (g_monitor_context.monitor_thread) {
                WaitForSingleObject(g_monitor_context.monitor_thread, 5000);
                CloseHandle(g_monitor_context.monitor_thread);
                g_monitor_context.monitor_thread = NULL;
            }
        }

        return TRUE;
    }

    // Stop monitoring specific drive
    for (uint32_t i = 0; i < g_monitor_context.drive_count; i++) {
        if (strcmp(g_monitor_context.drives[i].drive_path, drive_path) == 0) {
            g_monitor_context.drives[i].is_monitoring = FALSE;
            uprintf("Stopped monitoring drive %s", drive_path);
            return TRUE;
        }
    }

    return FALSE;
}

// Check if drive is being monitored
BOOL IsDriveBeingMonitored(const char* drive_path)
{
    if (!drive_path || !g_monitor_initialized) {
        return FALSE;
    }

    for (uint32_t i = 0; i < g_monitor_context.drive_count; i++) {
        if (strcmp(g_monitor_context.drives[i].drive_path, drive_path) == 0) {
            return g_monitor_context.drives[i].is_monitoring;
        }
    }

    return FALSE;
}

// Get drive health status
BOOL GetDriveHealthStatus(const char* drive_path, drive_monitor_data_t* data)
{
    if (!drive_path || !data || !g_monitor_initialized) {
        return FALSE;
    }

    for (uint32_t i = 0; i < g_monitor_context.drive_count; i++) {
        if (strcmp(g_monitor_context.drives[i].drive_path, drive_path) == 0) {
            memcpy(data, &g_monitor_context.drives[i], sizeof(drive_monitor_data_t));
            return TRUE;
        }
    }

    return FALSE;
}

// Get monitoring alerts
BOOL GetMonitoringAlerts(monitor_alert_t* alerts, uint32_t* alert_count)
{
    if (!alerts || !alert_count || !g_monitor_initialized) {
        return FALSE;
    }

    *alert_count = g_monitor_context.alert_count;
    
    if (g_monitor_context.alert_count > 0) {
        memcpy(alerts, g_monitor_context.alerts, 
               g_monitor_context.alert_count * sizeof(monitor_alert_t));
    }

    return TRUE;
}

// Acknowledge alert
BOOL AcknowledgeAlert(uint32_t alert_index)
{
    if (!g_monitor_initialized || alert_index >= g_monitor_context.alert_count) {
        return FALSE;
    }

    g_monitor_context.alerts[alert_index].is_acknowledged = TRUE;
    return TRUE;
}

// Clear all alerts
BOOL ClearAllAlerts(void)
{
    if (!g_monitor_initialized) {
        return FALSE;
    }

    g_monitor_context.alert_count = 0;
    memset(g_monitor_context.alerts, 0, sizeof(g_monitor_context.alerts));
    return TRUE;
}

// Monitoring thread procedure
DWORD WINAPI MonitorThreadProc(LPVOID lpParameter)
{
    uint64_t last_update = 0;
    uint64_t current_time;
    DWORD wait_result;

    uprintf("Monitoring thread started");

    while (g_monitor_context.is_monitoring) {
        current_time = GetTickCount64();

        // Check if it's time to update
        if (current_time - last_update >= g_monitor_config.update_interval) {
            // Update all monitored drives
            for (uint32_t i = 0; i < g_monitor_context.drive_count; i++) {
                if (g_monitor_context.drives[i].is_monitoring) {
                    UpdateDriveMetrics(g_monitor_context.drives[i].drive_path, 
                                     &g_monitor_context.drives[i]);
                }
            }

            last_update = current_time;
        }

        // Wait for stop event or timeout
        wait_result = WaitForSingleObject(g_monitor_context.stop_event, 100);
        if (wait_result == WAIT_OBJECT_0) {
            break;  // Stop event signaled
        }
    }

    uprintf("Monitoring thread stopped");
    return 0;
}

// Update drive metrics
BOOL UpdateDriveMetrics(const char* drive_path, drive_monitor_data_t* data)
{
    float new_metrics[MONITOR_METRIC_MAX];
    uint64_t current_time = GetTickCount64();

    if (!drive_path || !data) {
        return FALSE;
    }

    // Collect all metrics
    if (g_monitor_config.enable_temperature_monitoring) {
        CollectTemperatureMetric(drive_path, &new_metrics[MONITOR_METRIC_TEMPERATURE]);
    }

    if (g_monitor_config.enable_speed_monitoring) {
        CollectSpeedMetric(drive_path, &new_metrics[MONITOR_METRIC_READ_SPEED], 
                          &new_metrics[MONITOR_METRIC_WRITE_SPEED]);
    }

    if (g_monitor_config.enable_error_monitoring) {
        CollectErrorRateMetric(drive_path, &new_metrics[MONITOR_METRIC_ERROR_RATE]);
    }

    if (g_monitor_config.enable_power_monitoring) {
        CollectPowerConsumptionMetric(drive_path, &new_metrics[MONITOR_METRIC_POWER_CONSUMPTION]);
    }

    if (g_monitor_config.enable_vibration_monitoring) {
        CollectVibrationMetric(drive_path, &new_metrics[MONITOR_METRIC_VIBRATION]);
    }

    if (g_monitor_config.enable_em_monitoring) {
        CollectEMMetric(drive_path, &new_metrics[MONITOR_METRIC_ELECTROMAGNETIC]);
    }

    if (g_monitor_config.enable_capacity_monitoring) {
        CollectCapacityUsageMetric(drive_path, &new_metrics[MONITOR_METRIC_CAPACITY_USAGE]);
    }

    if (g_monitor_config.enable_sector_monitoring) {
        CollectSectorHealthMetric(drive_path, &new_metrics[MONITOR_METRIC_SECTOR_HEALTH]);
    }

    // Update metrics
    for (int i = 0; i < MONITOR_METRIC_MAX; i++) {
        data->current_metrics[i] = new_metrics[i];

        // Update running averages
        data->average_metrics[i] = (data->average_metrics[i] * data->data_points + new_metrics[i]) / 
                                   (data->data_points + 1);

        // Update min/max
        if (new_metrics[i] > data->max_metrics[i]) {
            data->max_metrics[i] = new_metrics[i];
        }
        if (new_metrics[i] < data->min_metrics[i]) {
            data->min_metrics[i] = new_metrics[i];
        }
    }

    data->data_points++;
    data->last_update = current_time;

    // Check for alerts
    CheckForAlerts(data);

    // Log data if enabled
    if (g_monitor_config.log_to_file) {
        LogMonitoringData(data);
    }

    return TRUE;
}

// Check for alerts
BOOL CheckForAlerts(drive_monitor_data_t* data)
{
    if (!data) {
        return FALSE;
    }

    // Check each metric for alerts
    for (int i = 0; i < MONITOR_METRIC_MAX; i++) {
        if (IsMetricCritical((monitor_metric_t)i, data->current_metrics[i])) {
            CreateAlert(data->drive_path, (monitor_metric_t)i, data->current_metrics[i],
                       GetMetricThreshold((monitor_metric_t)i), "Critical threshold exceeded", TRUE);
            data->error_count++;
        } else if (IsMetricWarning((monitor_metric_t)i, data->current_metrics[i])) {
            CreateAlert(data->drive_path, (monitor_metric_t)i, data->current_metrics[i],
                       GetMetricThreshold((monitor_metric_t)i), "Warning threshold exceeded", FALSE);
            data->warning_count++;
        }
    }

    return TRUE;
}

// Create alert
BOOL CreateAlert(const char* drive_path, monitor_metric_t metric, float current_value, 
                float threshold_value, const char* message, BOOL is_critical)
{
    if (!drive_path || !message || !g_monitor_initialized) {
        return FALSE;
    }

    if (g_monitor_context.alert_count >= (MONITOR_MAX_DRIVES * MONITOR_METRIC_MAX)) {
        return FALSE;  // No more space for alerts
    }

    monitor_alert_t* alert = &g_monitor_context.alerts[g_monitor_context.alert_count];
    
    strncpy(alert->drive_path, drive_path, sizeof(alert->drive_path) - 1);
    alert->metric = metric;
    alert->current_value = current_value;
    alert->threshold_value = threshold_value;
    alert->timestamp = GetTickCount64();
    alert->is_critical = is_critical;
    alert->is_acknowledged = FALSE;
    
    strncpy(alert->alert_message, message, sizeof(alert->alert_message) - 1);
    alert->alert_message[sizeof(alert->alert_message) - 1] = '\0';

    g_monitor_context.alert_count++;

    // Send notification if enabled
    if (g_monitor_config.auto_alert) {
        SendAlertNotification(alert);
    }

    uprintf("Alert created: %s - %s", drive_path, message);
    return TRUE;
}

// Collect temperature metric
BOOL CollectTemperatureMetric(const char* drive_path, float* temperature)
{
    // Simplified temperature collection
    // In reality, query drive temperature sensors
    
    if (!drive_path || !temperature) {
        return FALSE;
    }

    // Simulate temperature reading
    *temperature = 35.0f + (rand() % 20);  // 35-55°C
    return TRUE;
}

// Collect speed metric
BOOL CollectSpeedMetric(const char* drive_path, float* read_speed, float* write_speed)
{
    // Simplified speed collection
    // In reality, perform actual speed tests
    
    if (!drive_path || !read_speed || !write_speed) {
        return FALSE;
    }

    // Simulate speed readings
    *read_speed = 20.0f + (rand() % 20);   // 20-40 MB/s
    *write_speed = 15.0f + (rand() % 15);  // 15-30 MB/s
    return TRUE;
}

// Collect error rate metric
BOOL CollectErrorRateMetric(const char* drive_path, float* error_rate)
{
    // Simplified error rate collection
    // In reality, monitor actual error rates
    
    if (!drive_path || !error_rate) {
        return FALSE;
    }

    // Simulate error rate
    *error_rate = (float)(rand() % 100) / 1000.0f;  // 0-0.1%
    return TRUE;
}

// Collect power consumption metric
BOOL CollectPowerConsumptionMetric(const char* drive_path, float* power_consumption)
{
    // Simplified power consumption collection
    // In reality, measure actual power usage
    
    if (!drive_path || !power_consumption) {
        return FALSE;
    }

    // Simulate power consumption
    *power_consumption = 2.0f + (float)(rand() % 20) / 10.0f;  // 2.0-4.0W
    return TRUE;
}

// Collect vibration metric
BOOL CollectVibrationMetric(const char* drive_path, float* vibration)
{
    // Simplified vibration collection
    // In reality, measure actual vibration
    
    if (!drive_path || !vibration) {
        return FALSE;
    }

    // Simulate vibration reading
    *vibration = (float)(rand() % 50) / 10.0f;  // 0-5.0 (normalized)
    return TRUE;
}

// Collect EM metric
BOOL CollectEMMetric(const char* drive_path, float* em_strength)
{
    // Simplified EM signature collection
    // In reality, measure actual electromagnetic emissions
    
    if (!drive_path || !em_strength) {
        return FALSE;
    }

    // Simulate EM strength
    *em_strength = (float)(rand() % 100) / 100.0f;  // 0-1.0 (normalized)
    return TRUE;
}

// Collect capacity usage metric
BOOL CollectCapacityUsageMetric(const char* drive_path, float* capacity_usage)
{
    // Simplified capacity usage collection
    // In reality, query actual drive capacity usage
    
    if (!drive_path || !capacity_usage) {
        return FALSE;
    }

    // Simulate capacity usage
    *capacity_usage = (float)(rand() % 100);  // 0-100%
    return TRUE;
}

// Collect sector health metric
BOOL CollectSectorHealthMetric(const char* drive_path, float* sector_health)
{
    // Simplified sector health collection
    // In reality, check actual sector health
    
    if (!drive_path || !sector_health) {
        return FALSE;
    }

    // Simulate sector health
    *sector_health = 95.0f + (float)(rand() % 5);  // 95-100%
    return TRUE;
}

// Get metric name
char* GetMetricName(monitor_metric_t metric)
{
    switch (metric) {
        case MONITOR_METRIC_TEMPERATURE: return "Temperature";
        case MONITOR_METRIC_READ_SPEED: return "Read Speed";
        case MONITOR_METRIC_WRITE_SPEED: return "Write Speed";
        case MONITOR_METRIC_ERROR_RATE: return "Error Rate";
        case MONITOR_METRIC_POWER_CONSUMPTION: return "Power Consumption";
        case MONITOR_METRIC_VIBRATION: return "Vibration";
        case MONITOR_METRIC_ELECTROMAGNETIC: return "EM Signature";
        case MONITOR_METRIC_CAPACITY_USAGE: return "Capacity Usage";
        case MONITOR_METRIC_SECTOR_HEALTH: return "Sector Health";
        default: return "Unknown";
    }
}

// Get metric unit
char* GetMetricUnit(monitor_metric_t metric)
{
    switch (metric) {
        case MONITOR_METRIC_TEMPERATURE: return "°C";
        case MONITOR_METRIC_READ_SPEED: return "MB/s";
        case MONITOR_METRIC_WRITE_SPEED: return "MB/s";
        case MONITOR_METRIC_ERROR_RATE: return "%";
        case MONITOR_METRIC_POWER_CONSUMPTION: return "W";
        case MONITOR_METRIC_VIBRATION: return "Hz";
        case MONITOR_METRIC_ELECTROMAGNETIC: return "strength";
        case MONITOR_METRIC_CAPACITY_USAGE: return "%";
        case MONITOR_METRIC_SECTOR_HEALTH: return "%";
        default: return "";
    }
}

// Get metric threshold
float GetMetricThreshold(monitor_metric_t metric)
{
    switch (metric) {
        case MONITOR_METRIC_TEMPERATURE: return 60.0f;
        case MONITOR_METRIC_READ_SPEED: return 5.0f;
        case MONITOR_METRIC_WRITE_SPEED: return 5.0f;
        case MONITOR_METRIC_ERROR_RATE: return 0.5f;
        case MONITOR_METRIC_POWER_CONSUMPTION: return 5.0f;
        case MONITOR_METRIC_VIBRATION: return 3.0f;
        case MONITOR_METRIC_ELECTROMAGNETIC: return 0.8f;
        case MONITOR_METRIC_CAPACITY_USAGE: return 90.0f;
        case MONITOR_METRIC_SECTOR_HEALTH: return 80.0f;
        default: return 0.0f;
    }
}

// Check if metric is critical
BOOL IsMetricCritical(monitor_metric_t metric, float value)
{
    float threshold = GetMetricThreshold(metric);
    return (value >= threshold * g_monitor_config.critical_threshold);
}

// Check if metric is warning
BOOL IsMetricWarning(monitor_metric_t metric, float value)
{
    float threshold = GetMetricThreshold(metric);
    return (value >= threshold * g_monitor_config.warning_threshold);
}

// Send alert notification
BOOL SendAlertNotification(const monitor_alert_t* alert)
{
    if (!alert) {
        return FALSE;
    }

    // Simplified notification
    // In reality, send email, show popup, etc.
    uprintf("ALERT: %s - %s (%.2f %s)", alert->drive_path, alert->alert_message,
            alert->current_value, GetMetricUnit(alert->metric));
    
    return TRUE;
}

// Log monitoring data
BOOL LogMonitoringData(drive_monitor_data_t* data)
{
    FILE* fp;
    char log_entry[512];

    if (!data || !g_monitor_config.log_to_file) {
        return FALSE;
    }

    fp = fopen(g_monitor_config.log_file_path, "a");
    if (!fp) {
        return FALSE;
    }

    static_sprintf(log_entry, "%llu,%s,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n",
                   data->last_update, data->drive_path,
                   data->current_metrics[MONITOR_METRIC_TEMPERATURE],
                   data->current_metrics[MONITOR_METRIC_READ_SPEED],
                   data->current_metrics[MONITOR_METRIC_WRITE_SPEED],
                   data->current_metrics[MONITOR_METRIC_ERROR_RATE],
                   data->current_metrics[MONITOR_METRIC_POWER_CONSUMPTION],
                   data->current_metrics[MONITOR_METRIC_VIBRATION],
                   data->current_metrics[MONITOR_METRIC_ELECTROMAGNETIC],
                   data->current_metrics[MONITOR_METRIC_CAPACITY_USAGE],
                   data->current_metrics[MONITOR_METRIC_SECTOR_HEALTH]);

    fputs(log_entry, fp);
    fclose(fp);

    return TRUE;
}

// Get monitoring timestamp
uint64_t GetMonitoringTimestamp(void)
{
    return GetTickCount64();
}
