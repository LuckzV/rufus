/*
 * Rufus: The Reliable USB Formatting Utility
 * Real-Time Drive Monitoring System
 * Copyright © 2011-2025 Pete Batard <pete@akeo.ie>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "rufus.h"

// Real-time monitoring constants
#define MONITOR_MAX_DRIVES        16
#define MONITOR_UPDATE_INTERVAL   1000    // 1 second
#define MONITOR_HISTORY_SIZE      3600    // 1 hour of data
#define MONITOR_ALERT_THRESHOLD   0.8f    // 80% threshold
#define MONITOR_CRITICAL_THRESHOLD 0.9f   // 90% threshold

// Monitoring metrics
typedef enum {
    MONITOR_METRIC_TEMPERATURE = 0,
    MONITOR_METRIC_READ_SPEED,
    MONITOR_METRIC_WRITE_SPEED,
    MONITOR_METRIC_ERROR_RATE,
    MONITOR_METRIC_POWER_CONSUMPTION,
    MONITOR_METRIC_VIBRATION,
    MONITOR_METRIC_ELECTROMAGNETIC,
    MONITOR_METRIC_CAPACITY_USAGE,
    MONITOR_METRIC_SECTOR_HEALTH,
    MONITOR_METRIC_MAX
} monitor_metric_t;

// Drive monitoring data
typedef struct {
    char drive_path[MAX_PATH];
    char drive_name[64];
    BOOL is_monitoring;
    BOOL is_healthy;
    float current_metrics[MONITOR_METRIC_MAX];
    float average_metrics[MONITOR_METRIC_MAX];
    float max_metrics[MONITOR_METRIC_MAX];
    float min_metrics[MONITOR_METRIC_MAX];
    uint64_t last_update;
    uint32_t error_count;
    uint32_t warning_count;
    uint32_t data_points;
} drive_monitor_data_t;

// Monitoring alert
typedef struct {
    char drive_path[MAX_PATH];
    monitor_metric_t metric;
    float current_value;
    float threshold_value;
    char alert_message[256];
    uint64_t timestamp;
    BOOL is_critical;
    BOOL is_acknowledged;
} monitor_alert_t;

// Monitoring context
typedef struct {
    drive_monitor_data_t drives[MONITOR_MAX_DRIVES];
    monitor_alert_t alerts[MONITOR_MAX_DRIVES * MONITOR_METRIC_MAX];
    uint32_t drive_count;
    uint32_t alert_count;
    BOOL is_monitoring;
    HANDLE monitor_thread;
    HANDLE stop_event;
    uint64_t start_time;
    uint64_t last_scan_time;
} monitor_context_t;

// Monitoring configuration
typedef struct {
    BOOL enable_temperature_monitoring;
    BOOL enable_speed_monitoring;
    BOOL enable_error_monitoring;
    BOOL enable_power_monitoring;
    BOOL enable_vibration_monitoring;
    BOOL enable_em_monitoring;
    BOOL enable_capacity_monitoring;
    BOOL enable_sector_monitoring;
    uint32_t update_interval;
    float warning_threshold;
    float critical_threshold;
    BOOL auto_alert;
    BOOL log_to_file;
    char log_file_path[MAX_PATH];
} monitor_config_t;

// Function prototypes
BOOL InitRealTimeMonitor(const monitor_config_t* config);
void CleanupRealTimeMonitor(void);
BOOL StartDriveMonitoring(const char* drive_path);
BOOL StopDriveMonitoring(const char* drive_path);
BOOL IsDriveBeingMonitored(const char* drive_path);
BOOL GetDriveHealthStatus(const char* drive_path, drive_monitor_data_t* data);
BOOL GetMonitoringAlerts(monitor_alert_t* alerts, uint32_t* alert_count);
BOOL AcknowledgeAlert(uint32_t alert_index);
BOOL ClearAllAlerts(void);
BOOL SetMonitoringThreshold(monitor_metric_t metric, float threshold);
BOOL GetMonitoringThreshold(monitor_metric_t metric, float* threshold);
BOOL ExportMonitoringData(const char* file_path);
BOOL ImportMonitoringData(const char* file_path);

// Monitoring thread functions
DWORD WINAPI MonitorThreadProc(LPVOID lpParameter);
BOOL UpdateDriveMetrics(const char* drive_path, drive_monitor_data_t* data);
BOOL CheckForAlerts(drive_monitor_data_t* data);
BOOL LogMonitoringData(drive_monitor_data_t* data);

// Metric collection functions
BOOL CollectTemperatureMetric(const char* drive_path, float* temperature);
BOOL CollectSpeedMetric(const char* drive_path, float* read_speed, float* write_speed);
BOOL CollectErrorRateMetric(const char* drive_path, float* error_rate);
BOOL CollectPowerConsumptionMetric(const char* drive_path, float* power_consumption);
BOOL CollectVibrationMetric(const char* drive_path, float* vibration);
BOOL CollectEMMetric(const char* drive_path, float* em_strength);
BOOL CollectCapacityUsageMetric(const char* drive_path, float* capacity_usage);
BOOL CollectSectorHealthMetric(const char* drive_path, float* sector_health);

// Alert management functions
BOOL CreateAlert(const char* drive_path, monitor_metric_t metric, float current_value, 
                float threshold_value, const char* message, BOOL is_critical);
BOOL ProcessAlerts(void);
BOOL SendAlertNotification(const monitor_alert_t* alert);
BOOL SaveAlertsToFile(const char* file_path);
BOOL LoadAlertsFromFile(const char* file_path);

// Utility functions
char* GetMetricName(monitor_metric_t metric);
char* GetMetricUnit(monitor_metric_t metric);
float GetMetricThreshold(monitor_metric_t metric);
BOOL IsMetricCritical(monitor_metric_t metric, float value);
BOOL IsMetricWarning(monitor_metric_t metric, float value);
uint64_t GetMonitoringTimestamp(void);
BOOL ValidateDrivePath(const char* drive_path);
BOOL IsDriveAccessible(const char* drive_path);
