#ifndef METRICS_H
#define METRICS_H

#include <Arduino.h>

struct PCMetrics {
    float cpu_percent;
    float ram_percent;
    float gpu_percent;
    float cpu_temp;
    float gpu_temp;
    float ssd_temp;
    float mobo_temp;
    float cpu_freq;
};

extern PCMetrics g_metrics;
extern volatile bool g_metrics_updated;

#endif
