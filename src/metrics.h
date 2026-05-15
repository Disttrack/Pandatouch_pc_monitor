#ifndef METRICS_H
#define METRICS_H

#include <Arduino.h>

struct PCMetrics {
    float cpu_percent;
    float ram_percent;
    float gpu_percent;
    float cpu_temp;
    float gpu_temp;
    float gpu_hotspot_temp;
    float gpu_mem_temp;
    float ssd_temp;
    float mobo_temp;
    float cpu_freq;
    float gpu_freq;
    float disk_percent;
    float ram_total_gb;
    float ram_used_gb;
    float vram_percent;
    float vram_used_gb;
    float vram_total_gb;
    float cpu_power;
    float gpu_power;
    float gpu_fan;
    float dimm_temp_min;
    float dimm_temp_max;
    float gpu_mem_freq;
    float pch_temp;
    float vrm_temp;
    float cpu_fan;
    float sys_fan_min;
    float sys_fan_max;
    float pump_fan;
    float fps;
};

extern PCMetrics g_metrics;
extern volatile bool g_metrics_updated;

#endif
